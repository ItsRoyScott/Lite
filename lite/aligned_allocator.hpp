#pragma once

#include "Essentials.hpp"

namespace lite
{
  // TODO: Determine if this is actually needed:

  // An allocator object which allocates the objects on an aligned boundary.
  template <class T, size_t Alignment = 16>
  struct aligned_allocator 
  {
    typedef T value_type;

    aligned_allocator() {}
    template <class T> aligned_allocator(const aligned_allocator<T>& other) {}

    // Allocates an aligned block of memory.
    T* allocate(std::size_t n)
    {
      // 0-sized allocation returns null.
      if (n == 0) return nullptr;

      // Throw if the size is large enough to create an integer overflow.
      if (n > max_size()) throw std::length_error("aligned_allocator<T>::allocate() - Integer overflow.");

      // Use _aligned_malloc to allocate with the given alignment.
      void* pv = _aligned_malloc(n * sizeof(T), Alignment);

      // Throw bad_alloc if malloc failed.
      if (pv == nullptr) throw std::bad_alloc();

      return static_cast<T*>(pv);
    }

    // Deallocates an aligned block of memory.
    void deallocate(T* p, std::size_t)
    {
      _aligned_free(p);
    }

    // Maximum size to prevent integer overflow.
    std::size_t max_size() const
    {
      return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(T);
    }

    // Rebinds the aligned_allocator to a different type.
    template <class U>
    struct rebind
    {
      typedef aligned_allocator<U> other;
    };
  };

  // Whether 'b' can free allocations made by 'a'.
  template <class T, class U>
  bool operator==(const aligned_allocator<T>&, const aligned_allocator<U>&)
  {
    return true;
  }

  // Whether 'b' cannot free allocations made by 'a'.
  template <class T, class U>
  bool operator!=(const aligned_allocator<T>& a, const aligned_allocator<U>& b)
  {
    return false;
  }

  // A vector capable of allocating on aligned boundaries.
  template <class T, size_t Alignment = 16>
  using aligned_vector = vector < T, aligned_allocator<T, Alignment> > ;

  template <size_t Alignment = 16>
  struct Align
  {
    template <class T>
    static void Delete(T* ptr)
    {
      if (!ptr) return;
      ptr->~T();
      _aligned_free(ptr);
    }

    template <class T, class... Args>
    static T* New(Args&&... args)
    {
      T* ptr = (T*)_aligned_malloc(sizeof(T), Alignment);
      new (ptr) T(forward<Args>(args)...);
      return ptr;
    }
  };
} // namespace lite