#include <chrono>
#include <iostream>
#include <map>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

// 需使用 C++17 编译: g++ -std=c++17 ...
class DnsCache {
  std::map<std::string, std::string> entries;
  mutable std::shared_mutex sm;

 public:
  // 读取操作使用共享锁，允许并发读取
  std::string resolve(const std::string& domain) const {
    std::shared_lock<std::shared_mutex> lk(sm);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 模拟读延迟

    auto it = entries.find(domain);
    return (it != entries.end()) ? it->second : "NXDOMAIN";
  }

  // 写操作一般有更高优先级，容易阻塞读操作
  void update(const std::string& domain, const std::string& ip) {
    std::lock_guard<std::shared_mutex> lk(sm);
    std::cout << "[Writer] Updating " << domain << "...\n";
    entries[domain] = ip;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模拟写延迟
  }
};

int main() {
  DnsCache cache;
  cache.update("google.com", "8.8.8.8");

  auto reader_task = [&](int id) {
    for (int i = 0; i < 5; ++i) {
      std::string ip = cache.resolve("google.com");
      std::cout << "[Reader " << id << "] Resolved: " << ip << "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  };

  std::thread writer([&]() {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(10));  // 让读者先跑一会儿
    cache.update("google.com", "8.8.4.4");
  });

  std::vector<std::thread> threads;
  {
    for (int i = 0; i < 10; ++i) threads.emplace_back(reader_task, i);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  writer.join();
  for (auto& t : threads) t.join();

  return 0;
}
