#include <iostream>

#include "file_functions.hpp"
#include "hasher.hpp"
#include "pool_allocator.hpp"
#include "thread_pool.hpp"
#include "ts_queue.hpp"

int printInfo(char *bin, char const *msg = "") {
  std::cout << "Wrong options" << msg << ".\nTry " << bin << " <input file> <output file> [block size in bytes >=8]"
            << std::endl;
  return 1;
}

int main(int argc, char **argv) {
  constexpr size_t CHUNK_SIZE{1'000'000};

  try {
    if ((argc < 3 || 4 < argc)) {
      return printInfo(argv[0]);
    }
    PoolableData::setSize(CHUNK_SIZE);
    if (4 == argc) {
      long blockSize{};
      try {
        blockSize = std::stol(std::string(argv[3]));
      } catch (std::exception const &e) {
        return printInfo(argv[0], ": invalid block size format");
      }
      // Pool allocator limitation: data and pointer to next free data in pool share the same memory,
      // so data chunk have at least sizeof(void*).
      // Another approach without such limitation would require a control container with pointers\indexes.
      if (blockSize < 8) {
        return printInfo(argv[0], ": invalid block size");
      }
    }

    std::cout << "Process with block size = " << PoolableData::getSize() << std::endl;

    // Approximate queue size
    size_t queueSize{(4ULL * 1024ULL * 1024ULL * 1024ULL - 30ULL) / PoolableData::getSize()};

    // Queue of readed chunks of data
    BlkQueue<std::unique_ptr<PoolableData>> readDataQueue{(300UL < queueSize) ? 300UL : queueSize};

    // Queue of resulting hashes
    BlkQueue<std::future<uint32_t>> futureHashQueue{300UL};

    // Control flags
    std::atomic<bool> errorFlag{false};

    // File reader
    FileReader reader{argv[1], readDataQueue, errorFlag};

    // Hasher with pool of 'workers'
    Hasher hasher(readDataQueue, futureHashQueue, errorFlag, reader.isDone());

    // File writer
    FileWriter writer{argv[2], futureHashQueue, errorFlag, hasher.isDone()};

    return errorFlag;
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
