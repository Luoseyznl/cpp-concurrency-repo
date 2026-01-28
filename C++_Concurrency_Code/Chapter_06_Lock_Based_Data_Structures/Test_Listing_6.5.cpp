#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "Listing_6.5_Coupled_Lock_Linked_List.cpp"

// 全局计数，用于验证数据
std::atomic<int> push_count(0);
std::atomic<int> found_count(0);

// --- 任务 1：不断向头部推送数据 ---
void writer_task(threadsafe_list<int>& list, int num_ops) {
  for (int i = 0; i < num_ops; ++i) {
    list.push_front(i);
    push_count++;
    // 模拟间歇
    if (i % 10 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

// --- 任务 2：遍历并查找特定数据 ---
void finder_task(threadsafe_list<int>& list, int target) {
  for (int i = 0; i < 50; ++i) {
    auto res = list.find_first_if(
        [target](int const& value) { return value == target; });
    if (res) {
      found_count++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}

// --- 任务 3：删除符合条件的节点 ---
void remover_task(threadsafe_list<int>& list) {
  for (int i = 0; i < 20; ++i) {
    // 删除所有偶数节点
    list.remove_if([](int const& value) { return value % 2 == 0; });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

int main() {
  threadsafe_list<int> my_list;
  const int num_writers = 4;
  const int ops_per_writer = 100;

  std::cout << "--- Starting Threadsafe List Stress Test ---" << std::endl;
  auto start = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;

  // 启动写者
  for (int i = 0; i < num_writers; ++i) {
    threads.emplace_back(writer_task, std::ref(my_list), ops_per_writer);
  }

  // 启动查找者（查找数字 42）
  for (int i = 0; i < 2; ++i) {
    threads.emplace_back(finder_task, std::ref(my_list), 42);
  }

  // 启动清理者
  threads.emplace_back(remover_task, std::ref(my_list));

  // 等待所有任务完成
  for (auto& t : threads) {
    t.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  // 最后的盘点
  int remaining_count = 0;
  my_list.for_each([&remaining_count](int const& v) { remaining_count++; });

  std::cout << "--- Test Finished ---" << std::endl;
  std::cout << "Total Pushed: " << push_count.load() << std::endl;
  std::cout << "Remaining (after removals): " << remaining_count << std::endl;
  std::cout << "Find Successes: " << found_count.load() << std::endl;
  std::cout << "Execution Time: " << diff.count() << " s" << std::endl;

  return 0;
}