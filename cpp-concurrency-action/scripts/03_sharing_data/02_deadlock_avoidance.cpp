#include <iostream>
#include <mutex>
#include <thread>

struct Account {
  std::mutex m;
  int balance;
  std::string name;

  Account(std::string n, int b) : name(n), balance(b) {}
};

// 交叉转账要同时获取两个账户的锁，可能导致死锁
void transfer(Account& from, Account& to, int amount) {
  if (&from == &to) return;
  std::cout << "Transferring from " << from.name << " to " << to.name
            << "...\n";

#if __cplusplus >= 201703L  // C++17 及以上版本，使用 std::scoped_lock 避免死锁
  std::scoped_lock lock(from.m, to.m);
#else  // C++11/14，使用 std::lock + std::lock_guard
  std::lock(from.m, to.m);
  std::lock_guard<std::mutex> lock1(from.m, std::adopt_lock);
  std::lock_guard<std::mutex> lock2(to.m, std::adopt_lock);
#endif

  from.balance -= amount;
  to.balance += amount;
  std::cout << "Done. " << from.name << ": " << from.balance << ", " << to.name
            << ": " << to.balance << "\n";
}

int main() {
  Account a("Gevy", 100);
  Account b("Luose", 100);

  std::thread t1(transfer, std::ref(a), std::ref(b), 10);
  std::thread t2(transfer, std::ref(b), std::ref(a), 20);

  t1.join();
  t2.join();
  return 0;
}