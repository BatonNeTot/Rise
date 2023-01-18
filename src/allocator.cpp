//☀Rise☀
#include "allocator.h"

namespace Rise {

    Allocator::Allocator(uint32_t blockSize) 
        : _blockSize(blockSize) {}
    Allocator::~Allocator() {
        for (auto& pool : _pools) {
            ::operator delete(pool);
        }
    }

    void* Allocator::allocate() {
        if (_freeBlocks.empty()) {
            auto* pool = _pools.emplace_back(reinterpret_cast<uint8_t *>(::operator new(_blockSize * RISE_ALLOCATOR_POOL_SIZE)));
            for (ptrdiff_t i = 0; i < RISE_ALLOCATOR_POOL_SIZE; ++i) {
                _freeBlocks.emplace_back(pool + _blockSize * i);
            }
        }
        auto ptr = _freeBlocks.back();
        _freeBlocks.pop_back();
        return ptr;
    }
    void Allocator::deallocate(void* ptr) {
        _freeBlocks.emplace_back(ptr);
    }
}
