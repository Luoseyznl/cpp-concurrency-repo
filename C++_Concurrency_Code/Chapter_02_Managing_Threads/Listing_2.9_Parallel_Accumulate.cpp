#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <thread>
#include <vector>

/**
 * 结构体：accumulate_block
 * 作用：作为线程运行的可调用对象，计算指定范围的累加值。
 */
template <typename Iterator, typename T>
struct accumulate_block {
  void operator()(Iterator first, Iterator last, T& result) {
    result = std::accumulate(first, last, result);
  }
};

/**
 * 函数：parallel_accumulate
 * 作用：并行版本的累加
 */
template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
  unsigned long const length = std::distance(first, last);

  if (!length) return init;  // 1. 范围为空直接返回

  unsigned long const min_per_thread = 25;
  unsigned long const max_threads =
      (length + min_per_thread - 1) / min_per_thread;  // 2. 逻辑最大线程数

  unsigned long const hardware_threads = std::thread::hardware_concurrency();

  // 3. 确定最终启动的线程数：取硬件支持数与计算需求数的较小值
  unsigned long const num_threads =
      std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

  unsigned long const block_size =
      length / num_threads;  // 4. 每个线程负责的数据量（向下取整）

  std::vector<T> results(num_threads);
  std::vector<std::thread> threads(
      num_threads - 1);  // 5. 已有主线程，所以只需创建 n-1 个新线程

  Iterator block_start = first;
  for (unsigned long i = 0; i < (num_threads - 1); ++i) {
    Iterator block_end = block_start;
    std::advance(block_end, block_size);  // 6. 移动迭代器到块末尾

    // 7. std::thread 无条件拷贝传入参数，使用 std::ref 传递引用
    threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start,
                             block_end, std::ref(results[i]));
    block_start = block_end;  // 8. 更新下一个块的起始位置
  }

  // 9. 主线程亲自计算最后一个块（剩余的所有元素）
  accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);

  // 10. 等待所有线程完成
  for (auto& entry : threads) entry.join();

  // 11. 将所有中间结果汇总
  return std::accumulate(results.begin(), results.end(), init);
}

int main() {
  // 测试数据：0 到 10000 的序列
  std::vector<int> vec(10001);
  std::iota(vec.begin(), vec.end(), 0);

  int sum = parallel_accumulate(vec.begin(), vec.end(), 0);

  std::cout << "The sum of 0-10000 is: " << sum << std::endl;
  std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency()
            << std::endl;

  return 0;
}