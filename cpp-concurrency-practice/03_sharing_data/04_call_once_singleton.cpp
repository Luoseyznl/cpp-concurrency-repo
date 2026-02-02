#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

class Singleton {
 public:
  // 方案 A [推荐]: Magic Static (C++11 起线程安全)
  static Singleton& get_instance() {
    static Singleton instance;  // 只有在第一次执行到这里时初始化，且自带锁
    return instance;
  }

  void do_something() {
    std::cout << "Singleton instance address: " << this << "\n";
  }

 private:
  Singleton() { std::cout << "Singleton Constructed!\n"; }
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;
};

class LazyResource {
  std::once_flag init_flag_;
  std::unique_ptr<int> resource_;

 public:
  // 方案 B: std::call_once (适用于成员变量的延迟初始化)
  void use_resource() {
    std::call_once(init_flag_, [&]() {
      std::cout << "Initializing resource exactly once...\n";
      resource_ = std::make_unique<int>(42);
    });

    std::cout << "Resource value: " << *resource_ << "\n";
  }
};

int main() {
  std::cout << "--- Testing Magic Static ---\n";
  std::thread t1([] { Singleton::get_instance().do_something(); });
  std::thread t2([] { Singleton::get_instance().do_something(); });
  t1.join();
  t2.join();

  std::cout << "\n--- Testing std::call_once ---\n";
  LazyResource lr;
  std::thread t3([&] { lr.use_resource(); });  // lazy initialization
  std::thread t4([&] { lr.use_resource(); });
  t3.join();
  t4.join();

  return 0;
}
