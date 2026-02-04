/**
 * @file 04_atomic_smart_ptr.cpp
 * @brief 使用原子智能指针管理全局配置(C++20及以上)
 */

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

struct Config {
  int id;
  std::string name;
};

#if __cplusplus >= 202002L
std::atomic<std::shared_ptr<Config>> global_config;

void updater() {
  for (int i = 0; i < 100; ++i) {
    auto new_conf = std::make_shared<Config>(Config{i, "Updated"});
    global_config.store(new_conf, std::memory_order_release);
  }
}

void reader() {
  const Config* last = nullptr;
  for (int i = 0; i < 1000; ++i) {
    auto ptr = global_config.load(std::memory_order_acquire);
    const Config* cur = ptr.get();

    if (cur != last) {
      std::cout << "Config switched: " << cur << " id=" << ptr->id << "\n";
      last = cur;
    }
  }
}

int main() {
  global_config.store(std::make_shared<Config>(Config{0, "Init"}));

  std::thread t1(updater);
  std::thread t2(reader);

  t1.join();
  t2.join();

  return 0;
}
#else
int main() {
  std::cout << "This example requires C++20 compiler.\n";
  return 0;
}
#endif
