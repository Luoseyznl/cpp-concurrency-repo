/**
 * @file 02_thread_safe_queue.cpp
 * @brief 基于锁的线程安全队列实现
 * 重点关注使用 condition_variable 解决 生产者-消费者 问题。
 */
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

template <typename T>
class thread_safe_queue {
 private:
  mutable std::mutex mut;
  std::queue<T> data_queue;
  std::condition_variable data_cond;

 public:
  thread_safe_queue() {}

  void push(T new_value) {
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(std::move(new_value));
    data_cond.notify_one();  // 唤醒一个等待者
  }

  // 阻塞式 pop
  void wait_and_pop(T& value) {
    std::unique_lock<std::mutex> lk(mut);
    // 等待直到队列非空
    data_cond.wait(lk, [this] { return !data_queue.empty(); });
    value = std::move(data_queue.front());
    data_queue.pop();
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, [this] { return !data_queue.empty(); });
    std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
    data_queue.pop();
    return res;
  }

  // 非阻塞式 pop (try_pop)
  bool try_pop(T& value) {
    std::lock_guard<std::mutex> lk(mut);
    if (data_queue.empty()) return false;
    value = std::move(data_queue.front());
    data_queue.pop();
    return true;
  }

  bool empty() const {
    std::lock_guard<std::mutex> lk(mut);
    return data_queue.empty();
  }
};

int main() {
  thread_safe_queue<int> queue;

  std::thread producer([&] {
    for (int i = 0; i < 5; ++i) {
      std::cout << "Pushing " << i << "\n";
      queue.push(i);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  std::thread consumer([&] {
    for (int i = 0; i < 5; ++i) {
      int val;
      queue.wait_and_pop(val);
      std::cout << "Pop " << val << "\n";
    }
  });

  producer.join();
  consumer.join();
  return 0;
}
