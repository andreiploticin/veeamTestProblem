#include <iostream>

#include "chunk.hpp"
#include "file_functions.hpp"
#include "hasher.hpp"
#include "thread_pool.hpp"
#include "ts_queue.hpp"

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cout << "\nWrong options.\nTry " << argv[0] << " <input file> <output file> [block size in bytes]\n"
                << std::endl;
      return 1;
    }

    Chunk::setSize(CHUNK_SIZE);
    if (4 == argc) {
      try {
        size_t blockSize{std::stol(std::string(argv[3]))};
        if (blockSize > 0) {
          Chunk::setSize(blockSize);
        }
      } catch (std::exception const &e) {
        std::cout << "\nWrong options.\nTry " << argv[0] << " <input file> <output file> [block size in bytes]\n"
                  << std::endl;
        return 1;
      }
    }

    std::cout << "Process with block size = " << Chunk::getSize() << std::endl;

    // approximate queue size
    size_t queueSize{(4ULL * 1024ULL * 1024ULL * 1024ULL - 30ULL) / Chunk::getSize()};

    // Queue of readed chunks of data
    TSLimitQueue<Chunk>                 readDataQueue{(300UL < queueSize) ? 300UL : queueSize};
    // Queue of resulting hashes
    TSLimitQueue<std::future<uint32_t>> futureHashQueue{300UL};

    // Control flags
    std::atomic<bool> bPoolEmpty{false};
    std::atomic<bool> bStopAll{false};
    std::atomic<bool> bWriterDone{false};

    // File reader
    FileReader reader{argv[1], readDataQueue, bStopAll};

    // Hasher with pool of 'workers'
    Hasher hasher(readDataQueue, futureHashQueue, reader.isDone(), bStopAll);

    // File writer
    FileWriter writer{argv[2], futureHashQueue, bStopAll, bWriterDone};

    return 0;
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}