#ifndef TS_QUEUE_HPP
#define TS_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template<typename T> class TSLimitQueue {
  std::queue<std::shared_ptr<T>> m_queue;
  size_t                         m_maxSize;
  mutable std::mutex             m_dataMutex;
  std::condition_variable        m_notFullCond;
  std::condition_variable        m_dataCond;

public:
  explicit TSLimitQueue(size_t size) : m_maxSize(size) {
  }

  void wait_and_move(T &&value);
  T    wait_and_pop();
  bool try_pop(T &value) {
    std::lock_guard lk{m_dataMutex};
    if (m_queue.empty()) {
      return false;
    }
    value = std::move(*m_queue.front());
    m_queue.pop();
    if (m_queue.size() < m_maxSize) {
      m_notFullCond.notify_one();
    }
    return true;
  }
  bool empty() const {
    std::lock_guard lk{m_dataMutex};
    return m_queue.empty();
  }
};

template<typename T> void TSLimitQueue<T>::wait_and_move(T &&value) {
  std::shared_ptr<T> ptr(std::make_shared<T>(std::move(value)));
  std::unique_lock   lk{m_dataMutex};
  m_notFullCond.wait(lk, [this] {
    return m_queue.size() < m_maxSize;
  });
  m_queue.push(ptr);
  m_dataCond.notify_one();
}

template<typename T> T TSLimitQueue<T>::wait_and_pop() {
  std::unique_lock lk{m_dataMutex};
  m_dataCond.wait(lk, [this] {
    return !m_queue.empty();
  });
  T value{std::move(*m_queue.front())};
  m_queue.pop();
  if (m_queue.size() < m_maxSize) {
    m_notFullCond.notify_one();
  }
  return value; // move
}

template<typename T> class ThreadSafeQueue {
  mutable std::mutex             m_mutex;
  mutable std::mutex             m_mutex2;
  std::queue<std::shared_ptr<T>> m_queue;
  std::condition_variable        m_dataCond;
  std::condition_variable        m_someSpace;

  void pop_and_notify() {
    m_queue.pop();
    if (m_queue.empty()) {
      m_someSpace.notify_one();
    }
  }

public:
  ThreadSafeQueue() {
  }

  void push(T value) {
    std::shared_ptr<T> ptr(std::make_shared<T>(std::move(value)));
    std::lock_guard    lck{m_mutex};
    m_queue.push(ptr);
    m_dataCond.notify_one();
  }

  void wait_and_pop(T &value) {
    std::unique_lock lk{m_mutex};
    m_dataCond.wait(lk, [this] {
      return !m_queue.empty();
    });
    value = std::move(*m_queue.front());
    pop_and_notify();
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock lk{m_mutex};
    m_dataCond.wait(lk, [this] {
      return !m_queue.empty();
    });
    auto res = m_queue.front();
    pop_and_notify();
    return res;
  }

  bool try_pop(T &value) {
    std::lock_guard lk{m_mutex};
    if (m_queue.empty()) {
      return false;
    }
    value = std::move(*m_queue.front());
    pop_and_notify();
    return true;
  }

  std::shared_ptr<T> try_pop() {
    std::unique_lock lk{m_mutex};
    if (m_queue.empty()) {
      return nullptr;
    }
    auto res = m_queue.front();
    pop_and_notify();
    return res;
  }

  bool empty() {
    std::lock_guard lk{m_mutex};
    return m_queue.empty();
  }

  size_t size() {
    std::lock_guard lk(m_mutex);
    return m_queue.size();
  }

  std::condition_variable &someSpace() noexcept {
    return m_someSpace;
  }
};

#endif // TS_QUEUE_HPP
