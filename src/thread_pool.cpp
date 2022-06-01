#include "thread_pool.hpp"

void ThreadPool::workerThread() {
    while (!m_done) {
      FunctionWrapper task;
      if (m_workQueue.try_pop(task)) {
        task();
      } else {
        std::this_thread::yield();
      }
    }
  }
  