#include "callback_factory.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include <sys/mman.h>
#include <unistd.h>


constexpr bool debug = false
#ifdef DEBUG
    || true
#endif
;


#ifdef DEBUG

#include <iostream>

template<typename... Args>
void debug_out(const Args&... args) {
    ((std::cout << args), ...);
    std::cout << '\n';
}

#else
template<typename...> void debug_out(const Args&...) {}
#endif


// sizeof == 37
constexpr unsigned char callback_template[] = {
        0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0,     // 0, 2: mov rax, {function address}
        0x4d, 0x89, 0xc1,                       //   10: mov r9, r8
        0x49, 0x89, 0xc8,                       //   13: mov r8, rcx
        0x48, 0x89, 0xd1,                       //   16: mov rcx, rdx
        0x48, 0x89, 0xf2,                       //   19: mov rdx, rsi
        0x48, 0x89, 0xfe,                       //   22: mov rsi, rdi
        0x48, 0xbf, 0, 0, 0, 0, 0, 0, 0, 0,     //25,27: mov rdi, {this pointer}
        0xff, 0xe0};                            //   35: jmp rax

static_assert(sizeof(callback_block::code) == sizeof(callback_template));


const int callback_page::page_size = getpagesize();
const int callback_page::max_block_count = page_size / sizeof(callback_block);


template<typename T>
std::string ptr_to_str(T *p) {
    return std::to_string(reinterpret_cast<std::uintptr_t>(p));
}


callback_page::callback_page(callback_factory *factory) : 
    factory(factory),
    start(map_page()),
    next_available(start),
    unused_block_count(0) {}


callback_page::callback_page(callback_page &&other) :
    factory(other.factory),
    start(other.start),
    next_available(other.next_available),
    unused_block_count(other.unused_block_count)
{    
    other.start = nullptr;
}
    

callback_page &callback_page::operator=(callback_page &&other) {
    factory = other.factory;
    start = other.start;
    next_available = other.next_available;
    unused_block_count = other.unused_block_count;

    other.start = nullptr;
    return *this;
}


callback_page::~callback_page() {
    if(!start)
        return;

    for(callback_block *block = start; block < next_available; ++block)
        block->deleter(block->code);

    munmap(start, page_size);
}


callback_block *callback_page::allocate() const {

    if(next_available - start >= max_block_count) {
        if(debug && next_available - start > max_block_count)
            throw std::logic_error("page was over-allocated: " + std::to_string(next_available - start) 
                    + " > " + std::to_string(max_block_count));
        return nullptr;
    }

    return next_available++;
}


bool callback_page::deallocate(callback_block *ptr) const {
    if(debug && (ptr < start || ptr >= start + max_block_count))
        throw std::logic_error("deallocating pointer " + ptr_to_str(ptr)
                + " which doesn't belong to page " + ptr_to_str(start));

    if(debug && ptr >= next_available)
        throw std::logic_error("deallocating unallocated pointer " + ptr_to_str(ptr)
                + " > " + ptr_to_str(next_available));

    ptr->deleter(ptr->code);
    ptr->deleter = [](unsigned char*){};

    ++unused_block_count;
    factory->unused_blocks.push_back(block_ref{ptr, this});

    if(debug && start + unused_block_count > next_available)
        throw std::logic_error("page was over-deallocated: " + std::to_string(next_available - start)
                + std::to_string(unused_block_count));
    
    if(start + unused_block_count == next_available) {
        next_available = start;
        return true;
    } else {
        return false;
    }
}


callback_block *callback_page::map_page() {
    void *ptr = mmap(nullptr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED, -1, 0); 
    if(ptr == MAP_FAILED)
        throw std::bad_alloc();

    for(int i = 0; i < 50; i++)
    ((char*)ptr)[i] = 3;
    return static_cast<callback_block*>(ptr);
}



void callback_factory::free_raw_callback(callback_factory::func_ptr ptr) {
    auto it = pages.upper_bound(ptr);

    if(debug && pages.empty())
        throw std::logic_error("pages is empty");

    if(debug && it == pages.begin())
        throw std::logic_error("pointer doesn't belong to any page. ptr = " + ptr_to_str(ptr)
                + ", first page = " + ptr_to_str(pages.begin()->start));

    --it;
    if(it->deallocate(reinterpret_cast<callback_block*>(ptr))) {
        unused_blocks.erase(std::remove_if(unused_blocks.begin(), unused_blocks.end(), 
                    [it](block_ref ref) { return ref.page_ptr == &*it; }), unused_blocks.end());
        if(&*it != current_page)
            pages.erase(it);
    }
}


void callback_factory::patch_code(unsigned char *code, std::uint64_t obj_ptr, mem_func_ptr func) {
    constexpr std::size_t int_arg_count = 0;

    unsigned char *code_ptr = std::copy_n(callback_template, 10 + 3 * int_arg_count, code);
    std::copy(callback_template + 25, std::end(callback_template), code_ptr);

    (std::uint64_t&)code[2] = resolve_func(obj_ptr, func);
    (std::uint64_t&)code_ptr[2] = obj_ptr + func.obj_offset;
    std::cout << "func: " << std::hex << *(std::uint64_t*)&code[2] << std::endl;
}
    

std::uint64_t callback_factory::resolve_func(std::uint64_t obj_ptr, mem_func_ptr func) {
    std::cout << "obj_ptr " << std::hex << obj_ptr << std::endl;
    std::cout << std::hex << func.func_ptr_or_vtable_off << '\t' << func.obj_offset << std::endl;
    if(func.func_ptr_or_vtable_off & 1) {
        if(debug && func.func_ptr_or_vtable_off % 8 != 1)
            throw std::logic_error("expected vtable offset % 8 == 1. " 
                    + std::to_string(func.func_ptr_or_vtable_off));
        std::uint64_t *vtable = *reinterpret_cast<std::uint64_t**>(obj_ptr + func.obj_offset);
        return vtable[func.func_ptr_or_vtable_off / 8];
    } else {
        return func.func_ptr_or_vtable_off;
    }
}


block_ref callback_factory::allocate(callback_block &&block) {
    if(!unused_blocks.empty()) {
        block_ref ret = unused_blocks.back();
        unused_blocks.pop_back();

        if(debug && ret.page_ptr->unused_block_count == 0)
            throw std::logic_error("unused_block_count is 0. " + ptr_to_str(ret.page_ptr));

        if(unused_blocks.capacity() >= 128 && unused_blocks.size() * 4 <= unused_blocks.capacity())
            unused_blocks.shrink_to_fit();

        *ret.block_ptr = std::move(block);
        --ret.page_ptr->unused_block_count;
        return ret;
    }

    callback_block *block_ptr = new (allocate_space()) callback_block(std::move(block));
    return {block_ptr, current_page};
}


callback_block *callback_factory::allocate_space() {
    callback_block *block = current_page->allocate();
    if(block)
        return block;

    auto [it, success] = pages.emplace(this);
    current_page = &*it;

    if(debug && !success)
        throw std::logic_error("callback_page already exists: " + ptr_to_str(it->start));

    block = current_page->allocate();

    if(debug && !block)
        throw std::logic_error("allocate failed on new page: " + ptr_to_str(current_page->start)
                + ", " + ptr_to_str(current_page->next_available) + ", " 
                + std::to_string(current_page->unused_block_count));

    return block;
}



