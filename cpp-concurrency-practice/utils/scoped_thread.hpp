/**
 * @file scoped_thread.hpp
 * @brief std::thread 的包装类（RAII）
 *
 * 通过 RAII 原则，确保线程在作用域结束时被正确 join，防止资源泄漏或未定义行为。
 * 可以和工厂函数结合使用：std::scoped_thread scope_t{spawn_worker(...)};
 *
 * @author gew
 * @date 2025-01-30
 * @version 1.0
 */

#pragma once  // 防止头文件重复包含
#include <stdexcept>
#include <thread>
#include <utility>

class scoped_thread {
  std::thread t;

 public:
  // 值传递 + std::move 启动移动语义，这样可以统一处理 左值/右值 对象
  explicit scoped_thread(std::thread t_) : t(std::move(t_)) {
    if (!t.joinable()) throw std::logic_error("No thread");
  }
  ~scoped_thread() {
    if (t.joinable()) t.join();  // 核心：析构时自动 join
  }
  scoped_thread(const scoped_thread&) = delete;
  scoped_thread& operator=(const scoped_thread&) = delete;
};