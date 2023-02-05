//☀Rise☀
#ifndef allocator_h
#define allocator_h

#include <list>
#include <vector>

#ifndef RISE_ALLOCATOR_POOL_SIZE
#define RISE_ALLOCATOR_POOL_SIZE 1024
#endif //RISE_ALLOCATOR_POOL_SIZE

namespace Rise {

    class Allocator {
    public:

        Allocator(uint32_t blockSize);
        ~Allocator();


        template <class T>
        void destroy(T* t) {
            if (t == nullptr) {
                return;
            }

            t->~T();
            deallocate(t);
        }

        void* allocate();
        void deallocate(void* ptr);

        uint32_t capacity() const {
            return static_cast<uint32_t>(_pools.size() * RISE_ALLOCATOR_POOL_SIZE);
        }

        uint32_t freeAllocations() const {
            return static_cast<uint32_t>(_freeBlocks.size());
        }

    private:

        const uint32_t _blockSize;
        std::list<void*> _freeBlocks;
        std::vector<uint8_t*> _pools;
    };

}

#endif /* allocator_h */
