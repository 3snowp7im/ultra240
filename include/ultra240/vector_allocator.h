#pragma once

#include <list>
#include <vector>

namespace ultra {

  /**
   * Generic vector allocator.
   *
   * A vector allocator is a dynamic allocator with a fixed size backing array.
   * This is useful as a backend for link-lists data structures because it
   * eliminates dynamic heap allocation while preserving the benefits of a non-
   * contiguous collection.
   */
  template <typename T>
  class VectorAllocator {
  public:

    typedef T value_type;

    typedef T* pointer;

    typedef size_t size_type;

    template <class Type> struct rebind {
      typedef VectorAllocator<Type> other;
    };

    struct Chunk {
      pointer start;
      size_t size;
    };

    VectorAllocator() = delete;

    VectorAllocator(size_t size) {
      vector.reserve(size);
      chunks.reserve(size);
      chunks.push_back({&vector[0], size});
    }

    VectorAllocator(const VectorAllocator<T>& alloc) {
      vector.reserve(alloc.vector.capacity());
      chunks.reserve(alloc.vector.capacity());
      chunks.push_back({&vector[0], alloc.vector.capacity()});
    }

    template <class U>
    VectorAllocator(const VectorAllocator<U>& alloc) {
      vector.reserve(alloc.vector.capacity());
      chunks.reserve(alloc.vector.capacity());
      chunks.push_back({&vector[0], alloc.vector.capacity()});
    }

    pointer allocate(size_type n, void* hint = nullptr) {
      auto it = chunks.begin();
      while (it != chunks.end()) {
        if (it->size >= n
            && (hint == nullptr
                || it->start == reinterpret_cast<T*>(hint) + 1)) {
          T* start = it->start;
          if (it->size > n) {
            it->start += n;
            it->size -= n;
          } else {
            chunks.erase(it);
          }
          return start;
        }
        it++;
      }
      throw std::bad_alloc();
    }

    void deallocate(pointer p, size_t n) {
      auto before = chunks.end();
      auto after = chunks.end();
      auto it = chunks.begin();
      while (it != chunks.end()
             && (before == chunks.end() || after == chunks.end())) {
        if (it->start == p + n) {
          after = it;
        } else if (it->start + it->size == p) {
          before = it;
        }
        it++;
      }
      if (before == chunks.end() && after == chunks.end()) {
        chunks.push_back({p, n});
      } else if (before == chunks.end()) {
        after->start -= n;
        after->size += n;
      } else if (after == chunks.end()) {
        before->size += n;
      } else {
        before->size += n;
        before->size += after->size;
        chunks.erase(after);
      }
    }

  private:

    std::vector<T> vector;

    std::vector<Chunk> chunks;

    template <class U> friend class VectorAllocator;
  };

  template <typename T>
  using VectorAllocatorList = std::list<T, VectorAllocator<T>>;

}
