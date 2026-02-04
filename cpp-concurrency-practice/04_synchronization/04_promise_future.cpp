#include <exception>
#include <future>
#include <iostream>
#include <thread>

int main() {
  std::promise<int> p;
  std::future<int> f = p.get_future();  // p -> f 建立结果通道

  std::thread t([&p] {  // 捕获 promise 对象
    try {
      bool error = false;
      if (error) throw std::runtime_error("Error in promise!");

      p.set_value(99);  // 发送结果
    } catch (...) {
      p.set_exception(std::current_exception());  // 发送异常
    }
  });

  try {
    std::cout << "Promise Result: " << f.get() << "\n";
  } catch (const std::exception& e) {
    std::cout << "Promise Exception: " << e.what() << "\n";
  }
  t.join();
  return 0;
}