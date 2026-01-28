#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

/**
 * Listing 4.5 线程安全队列 (完整版)
 */
template <typename T>
class threadsafe_queue {
 private:
  // 允许 const 成员函数（如 empty）在逻辑只读的情况下修改锁状态
  mutable std::mutex mut;
  std::queue<T> data_queue;
  std::condition_variable data_cond;

 public:
  threadsafe_queue() {}

  // 拷贝构造函数：需要锁定对方的互斥量以保证数据一致性
  threadsafe_queue(threadsafe_queue const& other) {
    std::lock_guard<std::mutex> lk(other.mut);
    data_queue = other.data_queue;
  }

  // 严禁简单的赋值操作，防止复杂的锁死锁逻辑
  threadsafe_queue& operator=(const threadsafe_queue&) = delete;

  void push(T new_value) {
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(std::move(new_value));
    // 底层：将一个线程的 TCB 从 Wait Queue 搬回 Runqueue (变为 Ready 态)
    data_cond.notify_one();
  }

  // 阻塞式弹出：引用版本
  void wait_and_pop(T& value) {
    std::unique_lock<std::mutex> lk(mut);
    // 底点：原子性释放锁并进入 Blocked 状态。醒来后重新抢锁并在 while
    // 中检查条件
    data_cond.wait(lk, [this] { return !data_queue.empty(); });
    value = std::move(data_queue.front());
    data_queue.pop();
  }

  // 阻塞式弹出：智能指针版本
  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, [this] { return !data_queue.empty(); });
    std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
    data_queue.pop();
    return res;
  }

  // 非阻塞弹出：引用版本
  bool try_pop(T& value) {
    std::lock_guard<std::mutex> lk(mut);
    if (data_queue.empty()) return false;
    value = std::move(data_queue.front());
    data_queue.pop();
    return true;
  }

  // 非阻塞弹出：智能指针版本
  std::shared_ptr<T> try_pop() {
    std::lock_guard<std::mutex> lk(mut);
    if (data_queue.empty()) return std::shared_ptr<T>();
    std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
    data_queue.pop();
    return res;
  }

  bool empty() const {
    std::lock_guard<std::mutex> lk(mut);
    return data_queue.empty();
  }
};

// --- 测试脚本 ---

void producer(threadsafe_queue<int>& q) {
  for (int i = 0; i < 5; ++i) {
    // 模拟生产耗时，此时生产者处于 Blocked 或 Ready (由 sleep 实现)
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    std::cout << "[Producer] 生产了数据: " << i << " (准备入队并 notify_one)"
              << std::endl;
    q.push(i);
  }
}

void consumer(threadsafe_queue<int>& q, int id) {
  while (true) {
    int value;
    std::cout << "[Consumer " << id
              << "] 尝试取数... 若为空则进入 Blocked (离场休息)" << std::endl;

    q.wait_and_pop(value);

    std::cout << "[Consumer " << id << "] 唤醒成功！取到数据: " << value
              << std::endl;

    if (value == 4) {
      q.push(4);  // 转发下班信
      std::cout << "[Consumer " << id << "] 检测到结束标记，退出循环。"
                << std::endl;
      break;
    }
  }
}

int main() {
  std::cout << "--- 线程安全队列 Listing 4.5 实验开始 ---" << std::endl;
  threadsafe_queue<int> dq;

  // 开启一个生产者，两个消费者
  // 消费者会因为队列为空立刻进入 Blocked 状态，不占 CPU
  std::thread t1(producer, std::ref(dq));
  std::thread t2(consumer, std::ref(dq), 1);
  std::thread t3(consumer, std::ref(dq), 2);

  t1.join();
  t2.join();
  t3.join();

  std::cout << "--- 所有线程已销毁 (TCB 已从内核抹除) ---" << std::endl;
  return 0;
}