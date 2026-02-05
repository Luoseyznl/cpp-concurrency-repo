#include <algorithm>
#include <atomic>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

const int HP_PER_THREAD = 2;

struct HPRecord {
  std::atomic<void*> hp[HP_PER_THREAD];
  std::atomic<bool> active{false};
  HPRecord* next = nullptr;
};

class HazardPointerManager {
  std::atomic<HPRecord*> head_{nullptr};
  static thread_local HPRecord* local_record_;           // 线程本地 HPRecord
  static thread_local std::vector<void*> retired_list_;  // 垃圾链表

 public:
  void registerThread() {
    if (local_record_) return;
    HPRecord* p = head_.load(std::memory_order_acquire);
    while (p) {
      bool expected = false;
      if (!p->active.load() &&  // 尝试复用空闲的 HPRecord
          p->active.compare_exchange_strong(expected, true)) {
        local_record_ = p;
        return;
      }
      p = p->next;
    }
    HPRecord* new_rec = new HPRecord();
    new_rec->active = true;
    HPRecord* old_head = head_.load();
    do {
      new_rec->next = old_head;
    } while (!head_.compare_exchange_weak(old_head, new_rec));
    local_record_ = new_rec;
  }

  void acquire(int index, void* ptr) {
    local_record_->hp[index].store(ptr, std::memory_order_release);
  }

  void release(int index) {
    local_record_->hp[index].store(nullptr, std::memory_order_release);
  }

  void retire(void* ptr) {
    retired_list_.push_back(ptr);
    if (retired_list_.size() >= 10) scan();
  }

  void scan() {
    std::vector<void*> hazard_ptrs;
    HPRecord* p = head_.load(std::memory_order_acquire);
    while (p) {
      if (p->active.load()) {
        for (int i = 0; i < HP_PER_THREAD; ++i) {
          void* ptr = p->hp[i].load(std::memory_order_acquire);
          if (ptr) hazard_ptrs.push_back(ptr);
        }
      }
      p = p->next;
    }
    std::sort(hazard_ptrs.begin(), hazard_ptrs.end());

    auto it = retired_list_.begin();
    while (it != retired_list_.end()) {
      if (!std::binary_search(hazard_ptrs.begin(), hazard_ptrs.end(), *it)) {
        // delete (Node*)*it;
        std::cout << "[HP] Safely deleted: " << *it << std::endl;
        it = retired_list_.erase(it);
      } else {
        ++it;
      }
    }
  }
};

thread_local HPRecord* HazardPointerManager::local_record_ = nullptr;
thread_local std::vector<void*> HazardPointerManager::retired_list_;

int main() {
  HazardPointerManager hp_mgr;
  hp_mgr.registerThread();

  int* data = new int(42);
  hp_mgr.acquire(0, data);
  hp_mgr.retire(data);

  std::cout << "04_hazard_pointers: Scanning (should hold)..." << std::endl;
  hp_mgr.scan();

  hp_mgr.release(0);
  std::cout << "04_hazard_pointers: Scanning (should delete)..." << std::endl;
  hp_mgr.scan();

  return 0;
}