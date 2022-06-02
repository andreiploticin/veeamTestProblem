#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <memory>

class Chunk {
  std::unique_ptr<char[]> m_data;
  static size_t           m_size;

public:
  Chunk() {
    m_data = std::unique_ptr<char[]>(new char[m_size]);
  }

  char *const data() const {
    return m_data.get();
  }

  static void setSize(size_t size) noexcept {
    m_size = size;
  }

  static size_t getSize() noexcept {
    return m_size;
  }
};

size_t Chunk::m_size{};

#endif // CHUNK_HPP
