#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

const size_t EPOCH_COUNT = 3;
const size_t NULL_EPOCH = 999;

struct RetiredNode {
  void* ptr;
  std::function<void(void*)> deleter;
};

struct ThreadControlBlock {
  std::atomic<size_t> local_epoch{NULL_EPOCH};
  std::atomic<bool> active{false};
  std::vector<RetiredNode> retire_bags[EPOCH_COUNT];
};

class EpochManager {
  std::atomic<size_t> global_epoch_{0};
  std::list<ThreadControlBlock*> threads_;
  std::mutex threads_mutex_;
  static thread_local ThreadControlBlock* local_tcb_;

 public:
  void registerThread() {
    if (local_tcb_) return;
    std::lock_guard<std::mutex> lock(threads_mutex_);
    local_tcb_ = new ThreadControlBlock();
    local_tcb_->active = true;
    threads_.push_back(local_tcb_);
  }

  void enter() {
    if (!local_tcb_) registerThread();
    size_t g = global_epoch_.load(std::memory_order_relaxed);
    local_tcb_->local_epoch.store(g, std::memory_order_seq_cst);
  }

  void exit() {
    local_tcb_->local_epoch.store(NULL_EPOCH, std::memory_order_release);
  }

  template <typename T>
  void retire(T* ptr) {
    size_t current_epoch = global_epoch_.load(std::memory_order_relaxed);
    local_tcb_->retire_bags[current_epoch % EPOCH_COUNT].push_back(
        {ptr, [](void* p) { delete static_cast<T*>(p); }});
    try_advance_epoch();
  }

  // 【新增】调试接口
  size_t getSelfEpoch() const {
    if (local_tcb_)
      return local_tcb_->local_epoch.load(std::memory_order_relaxed);
    return NULL_EPOCH;
  }

 private:
  void try_advance_epoch() {
    size_t current_g = global_epoch_.load(std::memory_order_acquire);
    std::lock_guard<std::mutex> lock(threads_mutex_);

    for (auto* tcb : threads_) {
      size_t local = tcb->local_epoch.load(std::memory_order_acquire);
      if (local != NULL_EPOCH && local != current_g) return;
    }

    size_t new_g = (current_g + 1);
    global_epoch_.store(new_g, std::memory_order_release);
    clean_local_bag((new_g + 1) % EPOCH_COUNT);
  }

  void clean_local_bag(size_t index) {
    auto& bag = local_tcb_->retire_bags[index];
    if (!bag.empty()) {
      std::cout << "[EBR Thread " << std::this_thread::get_id()
                << "] Reclaiming " << bag.size() << " nodes." << std::endl;
      for (auto& node : bag) node.deleter(node.ptr);
      bag.clear();
    }
  }
};

thread_local ThreadControlBlock* EpochManager::local_tcb_ = nullptr;

struct Data {
  int value;
};

int main() {
  EpochManager mgr;
  std::thread t1([&]() {
    mgr.registerThread();
    mgr.enter();
    std::cout << "Thread 1 Epoch: " << mgr.getSelfEpoch() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr.exit();
  });

  std::thread t2([&]() {
    mgr.registerThread();
    for (int i = 0; i < 3; ++i) {
      mgr.enter();
      mgr.retire(new Data{i});
      mgr.exit();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  });

  t1.join();
  t2.join();
  return 0;
}