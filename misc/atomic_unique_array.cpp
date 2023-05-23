#include <memory>
#include <atomic>
#include <cstddef>

using Channel = int;

template<typename T>
class atomic_unique_array {
public:
    explicit atomic_unique_array(T *ptr) : p{ptr} {}

    atomic_unique_array(atomic_unique_array &&other) : p{other.p.exchange(nullptr)} {}

    atomic_unique_array& operator=(atomic_unique_array &&other) {
        delete[] p.exchange(other.p.exchange(nullptr));
        return *this;
    }

    T& operator[](std::size_t i) { return p[i]; }

    operator bool() const { return p; }

    bool assign_if_empty(T *ptr) {
        T *expected = nullptr;
        return p.compare_exchange_strong(expected, ptr);
    }

    ~atomic_unique_array() { delete[] p; }

private:
    std::atomic<T*> p;
};

class Foo {
public:
    void bar() {
        auto insertInLineCache = [this] (const Channel z, const int y, const float* linePtr) { 
            atomic_unique_array<const float*> &cachePtr = _cachedLinePtrs[z]; 
            if (!cachePtr) { 
                const int yLength = _yMax - _yMin; 
                // do the expensive operation outside of the mutex (new[])
                auto newCacheLine = std::unique_ptr<const float*[]>(new const float*[yLength]()); 
                if(cachePtr.assign_if_empty(newCacheLine.get()))
                    newCacheLine.release();
            } 
            cachePtr[y-_yMin] = linePtr; 
        }; 
    }

    static constexpr int Chan_Last = 5;
    mutable atomic_unique_array<const float*> _cachedLinePtrs[Chan_Last];
    int _yMax, _yMin;
};
