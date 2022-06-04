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
  std::ifstream                            m_fileStream;
  BlkQueue<std::unique_ptr<PoolableData>> &m_dataQueue;
  std::atomic<bool>                       &m_error;
  std::atomic<bool>                        m_done;
  std::thread                              m_thread;

  bool open(std::string filePath);
  void process();

public:
  explicit FileReader(std::string                              filePath,
                      BlkQueue<std::unique_ptr<PoolableData>> &dataQueue,
                      std::atomic<bool>                       &errorFlag)
      : m_dataQueue(dataQueue), m_error(errorFlag), m_done{false} {
    try {
      if (open(filePath)) {
        m_thread = std::thread(&FileReader::process, this);
      }
    } catch (std::exception const &e) {
      m_error = true;
      m_done  = true;
      std::cerr << "Error on creating reader thread: " << e.what() << std::endl;
    }
  }
  FileReader(FileReader const &)            = delete;
  FileReader &operator=(FileReader const &) = delete;

  ~FileReader() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  std::atomic<bool> &isDone() {
    return m_done;
  }
};

bool FileReader::open(std::string filePath) {
  if (!std::filesystem::exists(filePath)) {
    m_error = true;
    m_done  = true;
    std::cerr << "Error: file " << filePath << " doesn't exists" << std::endl;
    return false;
  }

  m_fileStream.open(filePath, std::ios::binary | std::ios::in);
  if (!m_fileStream.is_open()) {
    m_error = true;
    m_done  = true;
    std::cerr << "Error on opening input file" << std::endl;
    return false;
  }
  return true;
}

void FileReader::process() {
  size_t n{0};
  try {
    while (m_fileStream.good() && (!m_fileStream.eof()) && (!m_error)) {
      auto chunk = std::unique_ptr<PoolableData>(new PoolableData);
      m_fileStream.read((char *)chunk.get(), PoolableData::getSize());
      auto a = m_fileStream.gcount();
      if (a != PoolableData::getSize()) {
        if (!m_fileStream.eof()) {
          m_error = true;
          std::cerr << "Error during file reading: unexpected size of read data" << std::endl;
          break;
        } else {
          // tailing zeros
          memset((char *)chunk.get() + m_fileStream.gcount(), 0x0, PoolableData::getSize() - m_fileStream.gcount());
        }
      }
      m_dataQueue.push(std::move(chunk));
    }
  } catch (std::exception const &e) {
    m_error = true;
    std::cerr << "Error during file reading: " << e.what() << '\n';
  }
  m_done = true;
}

class FileWriter {
  std::ofstream                    m_fileStream;
  BlkQueue<std::future<uint32_t>> &m_futureHashs;
  std::atomic<bool>               &m_error;
  std::atomic<bool>               &m_stopWait;
  std::atomic<bool>                m_done;
  std::thread                      m_thread;

  bool open(std::string filePath);
  void process();

public:
  explicit FileWriter(std::string                      filePath,
                      BlkQueue<std::future<uint32_t>> &futureHashs,
                      std::atomic<bool>               &errorFlag,
                      std::atomic<bool>               &stopFlag)
      : m_futureHashs(futureHashs), m_error(errorFlag), m_stopWait(stopFlag), m_done(false) {
    try {
      if (open(filePath)) {
        m_thread = std::thread(&FileWriter::process, this);
      }
    } catch (std::exception const &e) {
      m_error = true;
      m_done  = true;
      std::cerr << "Error on creating writing thread: " << e.what() << std::endl;
    }
  }
  FileWriter(FileWriter const &)            = delete;
  FileWriter &operator=(FileWriter const &) = delete;

  ~FileWriter() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }
};

bool FileWriter::open(std::string filePath) {
  m_fileStream.open(filePath, std::ios::binary | std::ios::out);
  if (!m_fileStream.is_open()) {
    m_error = true;
    m_done  = true;
    std::cerr << "Error on opening file for output" << std::endl;
    return false;
  }
  return true;
}

void FileWriter::process() {
  std::future<uint32_t> hashResult;
  try {
    while (!m_error) {
      if (m_futureHashs.try_pop(hashResult)) {
        uint32_t value{hashResult.get()};
        m_fileStream.write(reinterpret_cast<char *>(&value), 4);
      }
      if (m_futureHashs.empty() && m_stopWait) {
        break;
      }
    }
  } catch (std::exception const &e) {
    m_error = true;
    std::cerr << "Error during writing of the resulting file: " << e.what() << std::endl;
  }
  (void)m_futureHashs.try_pop(hashResult); // unblock queue's 'pusher'
  m_done = true;
}

#endif // FILE_FUNCTIONNS_HPP
