#ifndef FILE_FUNCTIONNS_HPP
#define FILE_FUNCTIONNS_HPP

#include <fstream>
#include <future>
#include <string>

#include "ts_queue.hpp"

class FileReader {
  std::ifstream m_fileStream;
  bool          m_done;

public:
  explicit FileReader(std::string filePath) : m_done{false} {
    m_fileStream.open(filePath, std::ios::binary);
  }
  ~FileReader() {
    std::cout << "~FileReader\n";
  }
  FileReader(FileReader const &)            = delete;
  FileReader &operator=(FileReader const &) = delete;

  void process(TSLimitQueue<Chunk> &dataQueue, std::atomic<bool> &endCond);
};

void FileReader::process(TSLimitQueue<Chunk> &dataQueue, std::atomic<bool> &endCond) {
  size_t n{0};
  while (m_fileStream.good() && !m_fileStream.eof()) {
    Chunk chunk(n++);
    m_fileStream.read(chunk.data(), Chunk::getSize());
    dataQueue.wait_and_move(chunk);
    std::cout << "read " << n << " s chunk\n";
  }
  std::cout << "\nlast ckunk size is " << m_fileStream.gcount() << '\n';
  endCond = true;
}

class FileWriter {
  std::ofstream m_fileStream;
  bool          m_done;

public:
  explicit FileWriter(std::string filePath) : m_done(false) {
    m_fileStream.open(filePath, std::ios::binary);
  }
  FileWriter(FileWriter const &)            = delete;
  FileWriter &operator=(FileWriter const &) = delete;

  void process(TSLimitQueue<std::future<uint32_t>> &futureHashs, std::atomic<bool> &endCond);
};

void FileWriter::process(TSLimitQueue<std::future<uint32_t>> &futureHashs, std::atomic<bool> &endCond) {
  while (!m_done) {
    auto     result = futureHashs.wait_and_pop();
    uint32_t value{result.get()};
    m_fileStream.write(reinterpret_cast<char *>(&value), 4);
    std::cout << "write\n";
    if (futureHashs.empty() && endCond) {
      break;
    }
  }
  std::cout << "FileWriter::process\n";
}

#endif // FILE_FUNCTIONNS_HPP