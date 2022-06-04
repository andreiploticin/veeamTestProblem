#ifndef HASHER_HPP
#define HASHER_HPP

#include <boost/crc.hpp>

#include "pool_allocator.hpp"
#include "thread_pool.hpp"
#include "ts_queue.hpp"

class Hasher {
  BlkQueue<std::unique_ptr<PoolableData>> &m_inQueue;
  BlkQueue<std::future<uint32_t>>         &m_resultQueue;
  std::unique_ptr<ThreadPool>              m_pool;
  std::atomic<bool>                       &m_error;
  std::atomic<bool>                       &m_stopWait;
  std::atomic<bool>                        m_done;
  std::thread                              m_thread;

public:
  explicit Hasher(BlkQueue<std::unique_ptr<PoolableData>> &inQueue,
                  BlkQueue<std::future<uint32_t>>         &resultQueue,
                  std::atomic<bool>                       &errorFlag,
                  std::atomic<bool>                       &stopWait)
      : m_inQueue(inQueue), m_resultQueue(resultQueue), m_error(errorFlag), m_stopWait(stopWait), m_done(false) {
    try {
      m_pool   = std::make_unique<ThreadPool>();
      m_thread = std::thread(&Hasher::process, this);
    } catch (std::exception const &e) {
      m_error = true;
      m_done  = true;
      std::cerr << "Error on creating hasher thread: " << e.what() << std::endl;
    }
  }

  ~Hasher() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
    m_done = true;
  }
  std::atomic<bool> &isDone() {
    return m_done;
  }
  void process();
};

void Hasher::process() {
  std::unique_ptr<PoolableData> chunk;
  try {
    while (!m_error) {
      if (m_inQueue.try_pop(chunk)) {
        std::future<uint32_t> futureCRC = m_pool->submit([chunk = std::move(chunk)] {
          boost::crc_32_type result;
          result.process_bytes((char *)chunk.get(), PoolableData::getSize());
          return result.checksum();
        });

        m_resultQueue.push(std::move(futureCRC));
      }
      if (m_inQueue.empty() && m_stopWait) {
        break;
      }
    }
  } catch (std::exception const &e) {
    m_error = true;
    std::cerr << "Error during hash calculating: " << e.what() << std::endl;
  }
  (void)m_inQueue.try_pop(chunk); // unblock queue's 'pusher'
  m_done = true;
}

#endif // HASHER_HPP