#ifndef FILE_FUNCTIONNS_HPP
#define FILE_FUNCTIONNS_HPP

#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>

#include "pool_allocator.hpp"
#include "ts_queue.hpp"

class FileReader {
  std::ifstream                                 m_fileStream;
  BlkQueue<std::unique_ptr<PoolableData>> &m_dataQueue;
  std::atomic<bool>                            &m_stopWait;
  std::atomic<bool>                             m_done;
  std::thread                                   m_thread;

  bool open(std::string filePath);
  void process();

public:
  explicit FileReader(std::string                                   filePath,
                      BlkQueue<std::unique_ptr<PoolableData>> &dataQueue,
                      std::atomic<bool>                            &stopWait)
      : m_dataQueue(dataQueue), m_stopWait(stopWait), m_done{false} {
    try {
      if (open(filePath)) {
        m_thread = std::thread(&FileReader::process, this);
      }
    } catch (std::exception const &e) {
      std::cerr << "Error on creating reader thread: " << e.what() << std::endl;
      m_done     = true;
      m_stopWait = true;
    }
  }
  FileReader(FileReader const &)            = delete;
  FileReader &operator=(FileReader const &) = delete;

  ~FileReader() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
    m_done = true;
  }

  std::atomic<bool> &isDone() {
    return m_done;
  }
};

bool FileReader::open(std::string filePath) {
  if (!std::filesystem::exists(filePath)) {
    std::cerr << "Error: file " << filePath << " doesn't exists" << std::endl;
    m_done     = true;
    m_stopWait = true;
    return false;
  }

  m_fileStream.open(filePath, std::ios::binary | std::ios::in);
  if (!m_fileStream.is_open()) {
    m_done     = true;
    m_stopWait = true;
    std::cerr << "Error on opening input file" << std::endl;
    return false;
  }

  return true;
}

void FileReader::process() {
  size_t n{0};
  try {
    while (m_fileStream.good() && (!m_fileStream.eof()) && (!m_stopWait)) {
      auto chunk = std::unique_ptr<PoolableData>(new PoolableData);
      m_fileStream.read((char*)chunk.get(), PoolableData::getSize());
      auto a = m_fileStream.gcount();
      if (a != PoolableData::getSize()) {
        if (!m_fileStream.eof()) {
          std::cerr << "Error during file reading: unexpected size of read data" << std::endl;
          m_stopWait = true;
          break;
        } else {
          // tailing zeros
          memset((char*)chunk.get() + m_fileStream.gcount(), 0x0, PoolableData::getSize() - m_fileStream.gcount());
        }
      }
      m_dataQueue.push(std::move(chunk));
    }
  } catch (std::exception const &e) {
    std::cerr << "Error during file reading: " << e.what() << '\n';
    m_stopWait = true;
  }
  m_done = true;
}

class FileWriter {
  std::ofstream                        m_fileStream;
  BlkQueue<std::future<uint32_t>> &m_futureHashs;
  std::atomic<bool>                   &m_stopWait;
  std::thread                          m_thread;

  bool open(std::string filePath);
  void process();

public:
  explicit FileWriter(std::string                          filePath,
                      BlkQueue<std::future<uint32_t>> &futureHashs,
                      std::atomic<bool>                   &stopWait,
                      std::atomic<bool>                   &stopOther)
      : m_futureHashs(futureHashs), m_done(stopOther), m_stopWait(stopWait) {
    try {
      if (open(filePath)) {
        m_thread = std::thread(&FileWriter::process, this);
      }
    } catch (std::exception const &e) {
      std::cerr << "Error on creating writing thread: " << e.what() << std::endl;
      m_done     = true;
      m_stopWait = true;
    }
  }
  FileWriter(FileWriter const &)            = delete;
  FileWriter &operator=(FileWriter const &) = delete;

  ~FileWriter() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
    m_done = true;
  }
  std::atomic<bool> &m_done;
};

bool FileWriter::open(std::string filePath) {
  m_fileStream.open(filePath, std::ios::binary | std::ios::out);
  if (!m_fileStream.is_open()) {
    m_done     = true;
    m_stopWait = true;
    std::cerr << "Error on opening file for output" << std::endl;
    return false;
  }

  return true;
}

void FileWriter::process() {
  try {
    std::future<uint32_t> hashResult;
    while (true) {
      if (m_futureHashs.try_pop(hashResult)) {
        uint32_t value{hashResult.get()};
        m_fileStream.write(reinterpret_cast<char *>(&value), 4);
      }
      if (m_futureHashs.empty() && m_stopWait) {
        break;
      }
    }
  } catch (std::exception const &e) {
    std::cerr << "Error during writing of the resulting file: " << e.what() << std::endl;
  }
  m_done     = true;
  m_stopWait = true;
}

#endif // FILE_FUNCTIONNS_HPP
