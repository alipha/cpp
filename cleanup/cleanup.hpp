#ifndef LIPH_CLEANUP_HPP
#define LIPH_CLEANUP_HPP

#include <utility>

template<typename Func>
struct cleanup {
    cleanup(Func f) : func(std::move(f)) {}
    ~cleanup() noexcept(noexcept(std::declval<Func>()())) { func(); }
    
private:
    cleanup(const cleanup &) = delete;
    cleanup &operator=(const cleanup &) = delete;
    
    Func func;
};

#endif
