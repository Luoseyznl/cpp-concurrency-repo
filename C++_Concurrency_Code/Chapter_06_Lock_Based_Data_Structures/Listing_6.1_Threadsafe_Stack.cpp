#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <thread>

// 定义自定义异常
struct empty_stack : std::exception {
  const char* what() const noexcept override {
    return "Empty stack! Action aborted.";
  }
};

template <typename T>
class threadsafe_stack {
 private:
  std::stack<T> data;
  mutable std::mutex m;

 public:
  threadsafe_stack() = default;

  // 拷贝构造：必须锁住 source 保证读取时的一致性
  threadsafe_stack(const threadsafe_stack& other) {
    std::lock_guard<std::mutex> lock(other.m);
    data = other.data;
  }

  threadsafe_stack& operator=(const threadsafe_stack&) = delete;

  void push(T new_value) {
    std::lock_guard<std::mutex> lock(m);
    data.push(std::move(new_value));
  }

  // 事务性 pop
  std::shared_ptr<T> pop() {
    // 【准备步】：锁外准备数据（这里没有）
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) throw empty_stack();  // 检查空栈

    // 【风险步】：分配内存并移动构造。若崩了，数据仍在栈顶。
    std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top())));

    // 【提交步】：确定性删除。
    data.pop();
    return res;
  }

  void pop(T& value) {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) throw empty_stack();

    value = std::move(data.top());  // 若此处赋值抛异常，pop 还没执行，数据安全
    data.pop();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
  }
};

// --- 测试逻辑 ---
int main() {
  threadsafe_stack<int> st;

  // 生产者线程
  std::thread t1([&st]() {
    for (int i = 0; i < 5; ++i) st.push(i);
  });

  // 消费者线程：演示异常捕获
  std::thread t2([&st]() {
    try {
      while (true) {
        auto p = st.pop();
        std::cout << "Popped: " << *p << std::endl;
      }
    } catch (const empty_stack& e) {
      std::cout << "[Expected Error]: " << e.what() << std::endl;
    }
  });

  t1.join();
  t2.join();

  return 0;
}