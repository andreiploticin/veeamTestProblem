
#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <atomic>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "ts_queue.hpp"

//
// exception-safe mechanism of threads destruction
//
class JoinThreads {
  std::vector<std::thread> &m_threads;

public:
  explicit JoinThreads(std::vector<std::thread> &threads) : m_threads(threads) {
  }
  ~JoinThreads() {
    for (auto &t : m_threads) {
      if (t.joinable()) {
        t.join();
      }
    }
  }
};

//
// move-only wrapper for std::packaged_task to use in thread-safe queue
// C++ Concurrency in Action, A.Williams
//
class FunctionWrapper {
  struct impl_base {
    virtual void call() = 0;
    virtual ~impl_base() {
    }
  };

  std::unique_ptr<impl_base> impl;

  template<typename F> struct impl_type : impl_base {
    F m_f;
    impl_type(F &&f) : m_f(std::move(f)) {
    }
    void call() {
      m_f();
    }
  };

public:
  template<typename F> FunctionWrapper(F &&f) : impl(new impl_type<F>(std::move(f))) {
  }
  FunctionWrapper(FunctionWrapper &&other) : impl(std::move(other.impl)) {
  }
  void operator()() {
    impl->call();
  }
  FunctionWrapper() = default;
  FunctionWrapper &operator=(FunctionWrapper &&other) {
    impl = std::move(other.impl);
    return *this;
  }
  FunctionWrapper(FunctionWrapper const &)            = delete;
  FunctionWrapper(FunctionWrapper &)                  = delete;
  FunctionWrapper &operator=(FunctionWrapper const &) = delete;
};

class ThreadPool {
private:
  std::atomic_bool                 m_done;
  ThreadSafeQueue<FunctionWrapper> m_workQueue;
  std::vector<std::thread>         m_threads;
  JoinThreads                      m_joiner;

  void workerThread();

public:
  ThreadPool(unsigned tn = 4);
  ~ThreadPool() {
    m_done = true;
  }
  template<typename FunctionType> std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
    typedef typename std::result_of<FunctionType()>::type result_type;
    std::packaged_task<result_type()>                     task(std::move(f));
    std::future<result_type>                              res(task.get_future());
    m_workQueue.push(std::move(task));
    return res;
  }
  bool empty() {
    return m_workQueue.empty();
  }
};

ThreadPool::ThreadPool(unsigned tn) : m_done(false), m_joiner(m_threads) {
  unsigned detected{std::thread::hardware_concurrency()};
  detected = ((detected - 2) > tn) ? (detected - 2) : tn;
  std::cout << "Process with " << detected << " thread(s)" << std::endl;

  try {
    for (unsigned i{0U}; i < 16; ++i) {
      m_threads.push_back(std::thread(&ThreadPool::workerThread, this));
    }
  } catch (std::exception &e) {
    m_done = true;
    std::cerr << "Error during creating \'worker\' tread: " << e.what() << std::endl;
    throw;
  }
}

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

#endif // THREAD_POOL_HPP
