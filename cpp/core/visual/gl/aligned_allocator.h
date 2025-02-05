

#ifndef __ALIGNED_ALLOCATOR_H__
#define __ALIGNED_ALLOCATOR_H__

#include <malloc.h> // _aligned_malloc and _aligned_free
#include <memory> // std::allocator

#if defined(_M_IX86) || defined(_M_X64)

#include <intrin.h>

// STL allocator
template <class T, int TAlign = 16>
struct aligned_allocator : public std::allocator<T> {
    static const int ALIGN_SIZE = TAlign;
    template <class U>
    struct rebind {
        typedef aligned_allocator<U, TAlign> other;
    };
    aligned_allocator() noexcept {}
    aligned_allocator(const aligned_allocator &) noexcept {}
    template <class U>
    aligned_allocator(const aligned_allocator<U, TAlign> &) noexcept {}
    template <class U>
    aligned_allocator &
    operator=(const aligned_allocator<U, TAlign> &) noexcept {}
    // allocate
    pointer allocate(size_type c, const void *hint = 0) {
        return static_cast<pointer>(_mm_malloc(sizeof(T) * c, TAlign));
    }
    // deallocate
    void deallocate(pointer p, size_type n) { _mm_free(p); }
};
#else
#include <cstdlib> // for posix_memalign, free

// STL allocator
template <class T, int TAlign = 16>
struct aligned_allocator : public std::allocator<T> {
    static const int ALIGN_SIZE = TAlign;

    template <class U>
    struct rebind {
        typedef aligned_allocator<U, TAlign> other;
    };

    aligned_allocator() noexcept {}
    aligned_allocator(const aligned_allocator &) noexcept {}
    template <class U>
    aligned_allocator(const aligned_allocator<U, TAlign> &) noexcept {}
    template <class U>
    aligned_allocator &
    operator=(const aligned_allocator<U, TAlign> &) noexcept {}

    // allocate
    T *allocate(std::size_t c, const void *hint = 0) {
        void *ptr = nullptr;
        if(posix_memalign(&ptr, TAlign, sizeof(T) * c)) {
            throw std::bad_alloc();
        }
        return static_cast<T *>(ptr);
    }

    // deallocate
    void deallocate(T *p, std::size_t n) { free(p); }
};
#endif

#endif // __ALIGNED_ALLOCATOR_H__
