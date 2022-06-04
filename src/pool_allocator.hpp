#ifndef POOL_ALLOCATOR_HPP
#define POOL_ALLOCATOR_HPP
#include <iostream>
#include <queue>
#include <cassert>

//
// Allocate big Block of objects
//
class PoolAllocator {
public:
  struct Chunk {
    Chunk *next;
  };
  explicit PoolAllocator(size_t chunksPerBlock) : m_chunksPerBlock(chunksPerBlock) {
  }
  ~PoolAllocator() {
    while (!m_allocatedBlocks.empty()) {
      auto block = m_allocatedBlocks.front();
      if (nullptr != block) {
        free(block);
      }
      m_allocatedBlocks.pop();
    }
  }
  void *allocate(size_t size);
  void  deallocate(void *ptr);

private:
  std::mutex          m_mtx;
  size_t              m_chunksPerBlock;
  Chunk              *m_alloc{nullptr};
  Chunk              *allocateBlock(size_t chunkSize);
  std::queue<Chunk *> m_allocatedBlocks;
};

PoolAllocator::Chunk *PoolAllocator::allocateBlock(size_t chunkSize) {
  Chunk *blockStart{nullptr};
  try {
    blockStart = (Chunk *)malloc(m_chunksPerBlock * chunkSize);
  } catch (std::exception const &e) {
    std::cerr << "Error on allocation block of data in pool allocator : " << e.what() << '\n';
    throw(e);
  }

  auto chunk = blockStart;
  for (int i{0}; i < (m_chunksPerBlock - 1); ++i) {
    chunk->next = (Chunk *)((char *)chunk + chunkSize);
    chunk       = chunk->next;
  }
  chunk->next = nullptr;

  m_allocatedBlocks.push(blockStart);
  return blockStart;
}

void *PoolAllocator::allocate(size_t chunkSize) {
  std::lock_guard lk(m_mtx);
  if (nullptr == m_alloc) {
    m_alloc = allocateBlock(chunkSize);
  }

  Chunk *freeChunk = m_alloc;
  m_alloc          = m_alloc->next;
  return (void *)freeChunk;
}

void PoolAllocator::deallocate(void *ptr) {
  std::lock_guard lk(m_mtx);
  Chunk          *newFree = (Chunk *)ptr;
  newFree->next           = m_alloc;
  m_alloc                 = newFree;
}

//
// Can be allocated in pool
//
class PoolableData {
  static size_t        m_size;
  static PoolAllocator m_poolAllocator;

public:
  PoolableData() = default;
  static size_t getSize() noexcept {
    return m_size;
  }
  static void setSize(size_t size) noexcept {
    assert(size >= 8);
    m_size = size;
  }
  static void *operator new(size_t size) {
    return m_poolAllocator.allocate(getSize());
  }
  static void operator delete(void *p) {
    m_poolAllocator.deallocate(p);
  }
};
PoolAllocator PoolableData::m_poolAllocator{10};
size_t        PoolableData::m_size{0};

#endif // POOL_ALLOCATOR_HPP