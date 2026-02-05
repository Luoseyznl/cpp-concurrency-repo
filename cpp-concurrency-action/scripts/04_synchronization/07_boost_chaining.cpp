/**
 * @file 07_boost_chaining.cpp
 * @brief 使用 Boost 库实现 Future 链式操作
 * 标准 C++ std::future 目前(C++20)不支持 .then() 等链式操作，
 * 但 Boost.Future 提供了类似功能。
 */
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY

#include <boost/thread/future.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace boost;

int authenticate() {
  std::cout << "[Auth] Verifying user...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return 101;
}

std::string get_username(int id) {
  std::cout << "[DB] Getting name for ID " << id << "...\n";
  return "Alice";
}

void log_user_login(int id, const std::string& name) {
  std::cout << "[Log] User " << name << " (" << id << ") logged in.\n";
}

int main() {
  // 1. 认证用户，返回用户ID future<int>
  future<int> f1 = async(authenticate);

  // 2. f1 返回的 future<int> 传递给 then，返回用户名 future<string>
  future<std::string> f2 = f1.then([](future<int> prev_f) {
    int user_id = prev_f.get();
    return get_username(user_id);  // 返回结果，future 自动展平并传递
  });

  // ------------------------- when_all -------------------------
  auto t1 = async([] { return 100; });
  auto t2 = async([] { return std::string("Bob"); });

  future<void> f3 =
      when_all(std::move(t1), std::move(t2))
          .then([](future<std::tuple<future<int>, future<std::string>>>
                       result_f) {
            auto results = result_f.get();
            int id = std::get<0>(results).get();
            std::string name = std::get<1>(results).get();
            log_user_login(id, name);
          });  // Wait(A & B) -> C

  f3.wait();  // 等待异步任务完成
  return 0;
}
