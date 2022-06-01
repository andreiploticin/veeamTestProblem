#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <memory>
#include <iostream>

constexpr size_t CHUNK_SIZE{10 * 1024 * 1024};

class Chunk {
  std::unique_ptr<char[]> m_data;
  uint32_t                m_id;
  static size_t           m_size;

public:
  explicit Chunk(uint32_t id = 0) : m_id(id) {
    m_data = std::unique_ptr<char[]>(new char[m_size]);
  }

  Chunk(Chunk const &)            = delete;
  Chunk &operator=(Chunk const &) = delete;

  Chunk(Chunk &&other) : m_id(other.m_id), m_data(std::move(other.m_data)) {
  }

  Chunk &operator=(Chunk &&other) {
    m_id   = other.m_id;
    m_data = std::move(other.m_data);
    return *this;
  }

  ~Chunk() {
    if (nullptr != m_data.get()) {
      std::cout << "~Chunk(id=" << m_id << ")\n";
    }
  }

  char *const data() const {
    return m_data.get();
  }

  size_t getId() const noexcept {
    return m_id;
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
