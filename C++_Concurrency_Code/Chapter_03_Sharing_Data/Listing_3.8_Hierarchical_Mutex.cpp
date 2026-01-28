#include <climits>  // For ULONG_MAX
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

class hierarchical_mutex {
 private:
  std::mutex internal_mutex;

  // 该锁自身的层级值
  unsigned long const hierarchy_value;

  // 存储“上一次”的层级值，用于解锁时恢复
  unsigned long previous_hierarchy_value;

  // 核心：每个线程独立拥有的“当前层级值”
  // 初始化为最大值，表示还没拿任何锁
  static thread_local unsigned long this_thread_hierarchy_value;

  // 检查是否违反层级规则
  void check_for_hierarchy_violation() {
    if (this_thread_hierarchy_value <= hierarchy_value) {
      throw std::logic_error(
          "Mutex hierarchy violated: You are trying to lock a higher-level "
          "mutex!");
    }
  }

  // 更新当前线程的层级
  void update_hierarchy_value() {
    previous_hierarchy_value = this_thread_hierarchy_value;
    this_thread_hierarchy_value = hierarchy_value;
  }

 public:
  explicit hierarchical_mutex(unsigned long value)
      : hierarchy_value(value), previous_hierarchy_value(0) {}

  void lock() {
    // 1. 先检查（如果违反规则直接抛异常，不加锁）
    check_for_hierarchy_violation();

    // 2. 获取底层的物理锁
    internal_mutex.lock();

    // 3. 更新层级状态
    update_hierarchy_value();
  }

  void unlock() {
    // 解锁时必须逆序恢复
    // 如果当前线程记录的值不是本锁的值，说明解锁顺序乱了（虽然 lock_guard
    // 保证了 RAII，但手动 unlock 可能出错）
    if (this_thread_hierarchy_value != hierarchy_value) {
      throw std::logic_error(
          "Mutex hierarchy violated: Unlocking in wrong order!");
    }

    // 恢复之前的层级
    this_thread_hierarchy_value = previous_hierarchy_value;
    internal_mutex.unlock();
  }

  bool try_lock() {
    check_for_hierarchy_violation();
    if (!internal_mutex.try_lock()) return false;
    update_hierarchy_value();
    return true;
  }
};

// 静态成员变量需要在类外初始化
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(
    ULONG_MAX);

/** 实验演示 **/

hierarchical_mutex high_level(10000);
hierarchical_mutex low_level(5000);

void good_thread() {
  try {
    std::cout << "[Good Thread] Trying to lock high then low..." << std::endl;
    std::lock_guard<hierarchical_mutex> lk1(high_level);
    std::lock_guard<hierarchical_mutex> lk2(low_level);
    std::cout << "[Good Thread] Success!" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "[Good Thread] Error: " << e.what() << std::endl;
  }
}

void bad_thread() {
  try {
    std::cout << "[Bad Thread] Trying to lock low then high (Violation!)..."
              << std::endl;
    std::lock_guard<hierarchical_mutex> lk1(low_level);
    // 此时线程层级变为了 5000，尝试拿 10000 的锁会失败
    std::lock_guard<hierarchical_mutex> lk2(high_level);
  } catch (const std::exception& e) {
    std::cerr << "[Bad Thread] Expected Violation: " << e.what() << std::endl;
  }
}

int main() {
  std::thread t1(good_thread);
  t1.join();

  std::thread t2(bad_thread);
  t2.join();

  return 0;
}