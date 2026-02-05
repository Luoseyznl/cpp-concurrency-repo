#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <thread>
#include <vector>

template <typename T>
class ThreadSafeStack {
  std::stack<T> data;
  mutable std::mutex m;

 public:
  ThreadSafeStack() {}

  void push(T new_value) {
    std::lock_guard<std::mutex> lock(m);  // 优先用 lock_guard，性能更高
    data.push(std::move(new_value));
  }

  // 方案 A: 传入引用，优点是由调用者分配内存
  void pop(T& value) {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) throw std::runtime_error("Empty stack");
    value = data.top();  // 异常安全：先构造再弹出
    data.pop();
  }

  // 方案 B: 返回智能指针，优点是安全
  std::shared_ptr<T> pop() {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) throw std::runtime_error("Empty stack");
    // make_shared 比 new T 更高效且异常安全
    std::shared_ptr<T> res = std::make_shared<T>(data.top());
    data.pop();
    return res;
  }

  bool empty() const {  // empty 这类函数默认为 const，也要加锁
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
  }
};

int main() {
  ThreadSafeStack<int> ts_stack;
  ts_stack.push(1);
  ts_stack.push(2);
  ts_stack.push(3);

  std::thread t1([&ts_stack]() {
    for (int i = 0; i < 3; ++i) {
      try {
        auto ptr = ts_stack.pop();
        std::cout << "Thread 1 popped: " << *ptr << std::endl;
      } catch (const std::runtime_error& e) {
        std::cout << "Thread 1: " << e.what() << std::endl;
      }
    }
  });

  std::thread t2([&]() {
    int val;
    try {
      ts_stack.pop(val);
      std::cout << "Thread 2 popped: " << val << "\n";
    } catch (...) {
      std::cout << "Thread 2 stack empty\n";
    }
  });

  t1.join();
  t2.join();
  return 0;
}
