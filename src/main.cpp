#include <boost/crc.hpp>
#include <filesystem>
#include <map>

#include "chunk.hpp"
#include "file_functions.hpp"
#include "thread_pool.hpp"
#include "ts_queue.hpp"

void consumeChunks(TSLimitQueue<Chunk>                 &queue,
                   TSLimitQueue<std::future<uint32_t>> &resultQueue,
                   ThreadPool                          &pool,
                   std::atomic<bool>                   &endCond,
                   std::atomic<bool>                   &writerEndCond) {
  while (true) {
    if (endCond && queue.empty()) {
      break;
    }
    Chunk                 chunk{queue.wait_and_pop()};
    auto                  id        = chunk.getId();
    std::future<uint32_t> futureCRC = pool.submit([chunk = std::move(chunk)] {
      boost::crc_32_type result;
      result.process_bytes(chunk.data(), Chunk::getSize());
      std::cout << "in thread " << std::this_thread::get_id() << " calculate " << chunk.getId() << " chunk with crc "
                << result.checksum() << "\n";
      return result.checksum();
    });

    resultQueue.wait_and_move(futureCRC);
  }
  writerEndCond = true;
  std::cout << "i'm done!\n";
}

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cout << "not enough arguments\n";
      return 1;
    }
    std::cout << argv[0] << " " << argv[1] << " " << argv[2] << '\n';

    if (!std::filesystem::exists(argv[1])) {
      std::cout << "file " << argv[1] << " doesn't exists\n";
      return 1;
    }

    std::map<size_t, uint32_t> hashes;

    Chunk::setSize(CHUNK_SIZE);

    // Queue of readed chunks of data
    TSLimitQueue<Chunk>                 readDataQueue(300);
    TSLimitQueue<std::future<uint32_t>> futureHashQueue(300);

    std::atomic<bool> bEof{false};
    std::atomic<bool> bPoolEmpty{false};

    // File reader
    FileReader fr{argv[1]};

    // Pool of the hashing threads
    ThreadPool pool{1};

    // File writer
    FileWriter fw{argv[2]};

    std::thread thread1(&FileReader::process, &fr, std::ref(readDataQueue), std::ref(bEof));

    std::thread thread2(consumeChunks,
                        std::ref(readDataQueue),
                        std::ref(futureHashQueue),
                        std::ref(pool),
                        std::ref(bEof),
                        std::ref(bPoolEmpty));

    std::thread thread3(&FileWriter::process, &fw, std::ref(futureHashQueue), std::ref(bPoolEmpty));

    thread1.join();
    thread2.join();
    thread3.join();

    return 0;
  } catch (std::exception const &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
