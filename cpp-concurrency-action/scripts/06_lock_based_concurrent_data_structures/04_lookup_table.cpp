/**
 * @file lookup_table.cpp
 * @brief 基于锁的线程安全查找表实现
 * 重点关注使用 分段锁 (Striped Locking) 提高并发性能。
 * 每个桶（bucket）使用独立的读写锁保护，允许多个线程并发访问不同桶的数据。
 */

#include <algorithm>
#include <iostream>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class thread_safe_lookup_table {
 private:
  // 1. 定义桶类型，包含数据（键值对的链表）和保护数据的读写锁
  class bucket_type {
   private:
    typedef std::pair<Key, Value> bucket_value;
    typedef std::list<bucket_value> bucket_data;
    bucket_data data;
    mutable std::shared_mutex mutex;  // 使用读写锁

   public:
    // 读操作：共享锁
    Value value_for(Key const& key, Value const& default_value) const {
      std::shared_lock<std::shared_mutex> lock(mutex);
      auto it = std::find_if(
          data.begin(), data.end(),
          [&](bucket_value const& item) { return item.first == key; });
      return (it == data.end()) ? default_value : it->second;
    }

    // 写操作：独占锁
    void add_or_update_mapping(Key const& key, Value const& value) {
      std::unique_lock<std::shared_mutex> lock(mutex);
      auto it = std::find_if(
          data.begin(), data.end(),
          [&](bucket_value const& item) { return item.first == key; });
      if (it == data.end()) {
        data.push_back(std::make_pair(key, value));
      } else {
        it->second = value;
      }
    }
  };

  std::vector<std::unique_ptr<bucket_type>> buckets;
  Hash hasher;

  bucket_type& get_bucket(Key const& key) const {
    std::size_t const bucket_index = hasher(key) % buckets.size();
    return *buckets[bucket_index];
  }

 public:
  // 简化构造函数，没有扩容桶数量的机制
  thread_safe_lookup_table(unsigned num_buckets = 19) : buckets(num_buckets) {
    for (unsigned i = 0; i < num_buckets; ++i) {
      buckets[i].reset(new bucket_type);
    }
  }

  Value value_for(Key const& key, Value const& default_value = Value()) const {
    return get_bucket(key).value_for(key, default_value);
  }

  void add_or_update_mapping(Key const& key, Value const& value) {
    get_bucket(key).add_or_update_mapping(key, value);
  }
};

int main() {
  thread_safe_lookup_table<std::string, int> table;

  std::thread writer([&] {
    table.add_or_update_mapping("Apple", 100);
    table.add_or_update_mapping("Banana", 200);
    table.add_or_update_mapping("Cherry", 300);
  });

  std::thread reader([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::cout << "Apple price: " << table.value_for("Apple", -1) << "\n";
  });

  writer.join();
  reader.join();
  return 0;
}
