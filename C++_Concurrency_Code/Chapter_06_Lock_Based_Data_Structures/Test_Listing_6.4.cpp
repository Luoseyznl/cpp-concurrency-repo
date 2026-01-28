#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "Listing_6.4_Threadsafe_Lookup_Table.cpp"

void tester(threadsafe_lookup_table<int, std::string>& table, int thread_id) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(0, 100);  // 键范围 0-100

  for (int i = 0; i < 50; ++i) {
    int key = dist(rng);

    // 1. 随机进行写入/更新
    table.add_or_update_mapping(key, "User_" + std::to_string(key) + "_from_T" +
                                         std::to_string(thread_id));

    // 2. 随机进行读取
    std::string val = table.value_for(key, "Unknown");

    // 3. 偶尔进行删除
    if (i % 10 == 0) {
      table.remove_mapping(key);
    }

    // 模拟一点点业务耗时，增加线程交替的概率
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

int main() {
  // 初始化 19 个桶（默认）
  threadsafe_lookup_table<int, std::string> table;

  std::cout << "--- Starting Lookup Table Stress Test ---" << std::endl;
  auto start = std::chrono::high_resolution_clock::now();

  // 启动 10 个线程同时操作
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(tester, std::ref(table), i);
  }

  for (auto& t : threads) {
    t.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  std::cout << "--- Test Finished ---" << std::endl;
  std::cout << "Time taken: " << diff.count() << " seconds" << std::endl;

  // 验证最后的一些数据
  std::cout << "Checking sample data (Key 42): "
            << table.value_for(42, "Not Found") << std::endl;

  return 0;
}