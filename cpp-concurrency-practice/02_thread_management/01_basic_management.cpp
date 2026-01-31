#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../utils/scoped_thread.hpp"

void task(int& id, std::string data, std::unique_ptr<int> ptr) {
  //   std::cout << "[Thread " << std::this_thread::get_id() << "] "
  //             << "Processing ID:" << id << ", Data:" << data
  //             << ", PtrVal:" << *ptr << std::endl;
  for (int i = 0; i < 100; ++i) {
    ++id;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 工厂模式：封装构造逻辑细节 + 返回对象触发移动语义（由调用者负责析构）
std::thread spawn_worker(int& counter) {
  return std::thread(task, std::ref(counter), "SpawnedData",
                     std::make_unique<int>(42)  // 移动语义
  );
}

int main() {
  int shared_id = 0;

  try {
    // 1. 使用线程包装器（推荐）
    scoped_thread scope_t{spawn_worker(shared_id)};

    // 2. 批量管理
    std::vector<std::thread> workers;
    for (int i = 0; i < 100; ++i) {
      workers.emplace_back(spawn_worker(shared_id));
    }

    for (auto& t : workers) {
      if (t.joinable()) t.join();  // 手动 join 批量线程
    }

  } catch (const std::exception& e) {
    // 这里 scoped_thread 会自动 join，批量线程也已 join，不用再担心资源泄漏
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  std::cout << "Final ID value: " << shared_id << " (Expected 10200)"
            << std::endl;
  return 0;
}