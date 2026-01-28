#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>

template <typename T>
class coarse_grained_queue {
 private:
  mutable std::mutex m;
  std::queue<std::shared_ptr<T>> data_queue;
  std::condition_variable data_cond;

 public:
  coarse_grained_queue() {}

  void push(T new_value) {
    // 锁外构造，可能抛出 bad_alloc，但不影响队列状态
    std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
    {
      std::lock_guard<std::mutex> lk(m);
      data_queue.push(data);
    }
    data_cond.notify_one();
  }

  // --- 阻塞版本 ---
  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock<std::mutex> lk(m);
    data_cond.wait(lk, [this] { return !data_queue.empty(); });
    std::shared_ptr<T> res = data_queue.front();
    data_queue.pop();
    return res;
  }

  // --- 非阻塞版本 ---
  std::shared_ptr<T> try_pop() {
    std::lock_guard<std::mutex> lk(m);
    if (data_queue.empty()) {
      return std::shared_ptr<T>();  // 返回空指针
    }
    std::shared_ptr<T> res = data_queue.front();
    data_queue.pop();
    return res;
  }

  bool empty() const noexcept {
    std::lock_guard<std::mutex> lk(m);
    return data_queue.empty();
  }
};