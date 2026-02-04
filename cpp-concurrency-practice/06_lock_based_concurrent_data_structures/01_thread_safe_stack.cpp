/**
 * @file 01_thread_safe_stack.cpp
 * @brief 基于锁的线程安全栈实现
 * 重点关注如何解决经典的 接口安全 问题：
 * if (!stack.empty()) {
 *    int value = stack.top();
 *    stack.pop();
 * }
 */
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <thread>

struct empty_stack : std::exception {
  const char* what() const noexcept override { return "Stack is empty"; }
};

template <typename T>
class thread_safe_stack {
 private:
  std::stack<T> data;
  mutable std::mutex m;

 public:
  thread_safe_stack() {}

  // 拷贝构造需要锁住 source
  thread_safe_stack(const thread_safe_stack& other) {
    std::lock_guard<std::mutex> lock(other.m);
    data = other.data;
  }

  // 禁用拷贝赋值操作符，因为可能引发死锁
  thread_safe_stack& operator=(const thread_safe_stack&) = delete;

  void push(T new_value) {
    std::lock_guard<std::mutex> lock(m);
    data.push(std::move(new_value));
  }

  // 1. 返回 shared_ptr
  std::shared_ptr<T> pop() {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) throw empty_stack();  // 或者返回 nullptr

    std::shared_ptr<T> res(std::make_shared<T>(data.top()));
    data.pop();  // 先构造结果再移除，避免拷贝异常导致数据丢失
    return res;
  }

  // 2. 传出参数版本，可以在外部复用对象
  void pop(T& value) {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) throw empty_stack();

    value = data.top();
    data.pop();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
  }
};

int main() {
  thread_safe_stack<int> ts;
  ts.push(10);
  ts.push(20);

  auto ptr = ts.pop();
  std::cout << "Popped: " << *ptr << "\n";

  int val;
  ts.pop(val);
  std::cout << "Popped: " << val << "\n";

  ts.pop();  // 栈空，抛出异常

  return 0;
}
