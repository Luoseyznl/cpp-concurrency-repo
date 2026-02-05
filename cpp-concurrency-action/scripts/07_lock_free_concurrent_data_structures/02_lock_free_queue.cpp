#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

template <typename T>
class LockFreeQueue {
 private:
  struct Node {
    std::shared_ptr<T> data;
    std::atomic<Node*> next;
    Node() : next(nullptr) {}
    Node(T val) : data(std::make_shared<T>(val)), next(nullptr) {}
  };

  std::atomic<Node*> head;
  std::atomic<Node*> tail;

 public:
  LockFreeQueue() {
    Node* dummy = new Node();
    head.store(dummy);
    tail.store(dummy);
  }

  // 简单的析构，注意生产环境需要更严谨的内存回收
  ~LockFreeQueue() {
    Node* curr = head.load();
    while (curr) {
      Node* next = curr->next.load();
      delete curr;
      curr = next;
    }
  }

  void enqueue(T value) {
    Node* new_node = new Node(value);
    Node* p_tail;
    while (true) {
      p_tail = tail.load(std::memory_order_acquire);
      Node* next = p_tail->next.load(std::memory_order_acquire);

      if (p_tail == tail.load(std::memory_order_acquire)) {
        if (next == nullptr) {
          if (p_tail->next.compare_exchange_weak(next, new_node)) {
            tail.compare_exchange_strong(p_tail, new_node);
            return;
          }
        } else {
          tail.compare_exchange_strong(p_tail, next);  // Helping
        }
      }
    }
  }

  std::shared_ptr<T> dequeue() {
    Node* p_head;
    while (true) {
      p_head = head.load(std::memory_order_acquire);
      Node* p_tail = tail.load(std::memory_order_acquire);
      Node* next = p_head->next.load(std::memory_order_acquire);

      if (p_head == head.load(std::memory_order_acquire)) {
        if (p_head == p_tail) {
          if (next == nullptr) return std::shared_ptr<T>();
          tail.compare_exchange_strong(p_tail, next);  // Helping
        } else {
          std::shared_ptr<T> res = next->data;
          if (head.compare_exchange_weak(p_head, next)) {
            // delete p_head; // 需要 SMR (Hazard Pointers/EBR) 支持才能安全删除
            return res;
          }
        }
      }
    }
  }
};

int main() {
  LockFreeQueue<int> queue;
  const int NUM_THREADS = 4;
  const int OPS = 10000;
  std::atomic<int> counter{0};

  std::vector<std::thread> threads;
  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < OPS; ++j) queue.enqueue(j);
    });
    threads.emplace_back([&]() {
      for (int j = 0; j < OPS; ++j) {
        if (queue.dequeue()) counter++;
      }
    });
  }

  for (auto& t : threads) t.join();

  // 由于消费者可能在队列为空时空转，这里只做简单的运行验证
  std::cout << "02_lock_free_queue: Dequeued " << counter.load() << " items."
            << std::endl;
  return 0;
}