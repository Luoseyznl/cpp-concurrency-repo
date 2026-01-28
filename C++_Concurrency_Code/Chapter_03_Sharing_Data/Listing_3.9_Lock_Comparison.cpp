#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class BankAccount {
 public:
  int balance;
  std::mutex m;
  BankAccount(int money) : balance(money) {}
};

// ================================================================
// 1. std::lock + std::adopt_lock (已经拿到锁，找个管家领养)
// ================================================================
void transfer_adopt(BankAccount& from, BankAccount& to, int amount) {
  if (&from == &to) return;

  // 第一步：直接锁住物理互斥量（使用全局 std::lock 避免死锁）
  std::lock(from.m, to.m);

  // 第二步：使用 adopt_lock。此时线程已拥有所有权，
  // lock_guard 不再加锁，只负责退出作用域时释放。
  std::lock_guard<std::mutex> lock_a(from.m, std::adopt_lock);
  std::lock_guard<std::mutex> lock_b(to.m, std::adopt_lock);

  from.balance -= amount;
  to.balance += amount;
  std::cout << "[Adopt] Transfer done using adopt_lock.\n";
}

// ================================================================
// 2. std::unique_lock + std::defer_lock (先找管家，等会儿再锁)
// ================================================================
void transfer_defer(BankAccount& from, BankAccount& to, int amount) {
  if (&from == &to) return;

  // 第一步：先创建 unique_lock，但使用 defer_lock 标记
  // 此时管家已就位，但“门没锁”。
  std::unique_lock<std::mutex> lock_a(from.m, std::defer_lock);
  std::unique_lock<std::mutex> lock_b(to.m, std::defer_lock);

  // 第二步：在需要的时候，一次性锁住。
  // 这里 std::lock 操作的是 unique_lock 对象。
  std::lock(lock_a, lock_b);

  from.balance -= amount;
  to.balance += amount;
  std::cout << "[Defer] Transfer done using unique_lock + defer_lock.\n";

  // unique_lock 的好处：你可以根据需要提前解锁
  lock_a.unlock();
  std::cout << "[Defer] lock_a released early to increase concurrency.\n";
}

// ================================================================
// 3. std::try_to_lock (不强求，能锁上就做，锁不上就撤)
// ================================================================
void audit_try_lock(BankAccount& acc) {
  // 尝试获取所有权，如果不成功，函数立即返回，不会阻塞线程。
  std::unique_lock<std::mutex> lock(acc.m, std::try_to_lock);

  // 检查是否成功获得了锁（owns_lock() 会检查标志位）
  if (lock.owns_lock()) {
    std::cout << "[Try] Audit success: Balance is " << acc.balance << "\n";
  } else {
    // 锁被别人占着，审计员决定先去干别的事，而不是在这里死等
    std::cout << "[Try] Audit failed: Account is busy, skipping for now.\n";
  }
}

int main() {
  BankAccount my_acc(1000);
  BankAccount your_acc(1000);

  // 演示 adopt 模式
  transfer_adopt(my_acc, your_acc, 100);

  // 演示 defer 模式
  transfer_defer(my_acc, your_acc, 200);

  // 演示 try_to_lock 模式
  std::thread t1(audit_try_lock, std::ref(my_acc));
  t1.join();

  return 0;
}