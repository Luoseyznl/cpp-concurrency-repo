#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

template <typename T>
class LockFreeStack {
 private:
  struct Node {
    std::shared_ptr<T> data;
    Node* next;
    Node(T const& data_) : data(std::make_shared<T>(data_)), next(nullptr) {}
  };

  std::atomic<Node*> head;

 public:
  LockFreeStack() : head(nullptr) {}

  void push(T const& data) {
    Node* new_node = new Node(data);
    new_node->next = head.load(std::memory_order_relaxed);

    while (!head.compare_exchange_weak(new_node->next, new_node,
                                       std::memory_order_release,
                                       std::memory_order_relaxed));
  }

  std::shared_ptr<T> pop() {
    Node* old_head = head.load(std::memory_order_relaxed);
    while (old_head && !head.compare_exchange_weak(old_head, old_head->next,
                                                   std::memory_order_acquire,
                                                   std::memory_order_relaxed));
    return old_head ? old_head->data : std::shared_ptr<T>();
  }
};

int main() {
  LockFreeStack<int> stack;
  std::thread t1([&]() {
    for (int i = 0; i < 100; ++i) stack.push(i);
  });
  std::thread t2([&]() {
    for (int i = 0; i < 100; ++i) stack.pop();
  });
  t1.join();
  t2.join();
  std::cout << "01_lock_free_stack: Run successfully." << std::endl;
  return 0;
}