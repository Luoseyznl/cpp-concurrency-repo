#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

/**
 * 设计思想：分片/桶锁 (Sharding/Bucket Locking)
 * 1. 放弃全局锁：不保护整个容器，而是保护容器的一部分（桶）。
 * 2. 读写分离：使用 shared_mutex，允许多个读者同时访问同一个桶。
 * 3. 固定拓扑：桶的数量在初始化时固定，避免动态扩容（Rehash）导致的全局停顿。
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table {
 private:
  // --- 内部类：桶 (Bucket) ---
  class bucket_type {
   private:
    typedef std::pair<Key, Value> bucket_value;
    typedef std::list<bucket_value> bucket_data;  // 使用链表处理哈希冲突
    typedef typename bucket_data::iterator bucket_iterator;

    bucket_data data;
    mutable std::shared_mutex mutex;  // 【关键】每个桶独立的读写锁

    // 内部辅助函数：在桶内寻找匹配的键（调用前必须已拿锁）
    bucket_iterator find_entry_for(Key const& key) {
      return std::find_if(
          data.begin(), data.end(),
          [&](bucket_value const& item) { return item.first == key; });
    }

   public:
    // 【读操作】：支持并发
    Value value_for(Key const& key, Value const& default_value) const {
      // 思想：shared_lock 允许多个线程同时进入这个桶进行读取
      std::shared_lock<std::shared_mutex> lock(mutex);
      auto it = std::find_if(
          data.begin(), data.end(),
          [&](bucket_value const& item) { return item.first == key; });
      return (it == data.end()) ? default_value : it->second;
    }

    // 【写操作】：独占访问
    void add_or_update_mapping(Key const& key, Value const& value) {
      // 思想：unique_lock 确保修改数据时没有其他读者或作者
      std::unique_lock<std::shared_mutex> lock(mutex);
      auto found_entry = find_entry_for(key);
      if (found_entry == data.end()) {
        data.push_back(
            bucket_value(key, value));  // 事务性：若 push 失败，数据无损
      } else {
        found_entry->second = value;  // 更新现有值
      }
    }

    // 【删除操作】：独占访问
    void remove_mapping(Key const& key) {
      std::unique_lock<std::shared_mutex> lock(mutex);
      auto found_entry = find_entry_for(key);
      if (found_entry != data.end()) {
        data.erase(found_entry);
      }
    }
  };

  // --- 外部结构 ---
  std::vector<std::unique_ptr<bucket_type>> buckets;  // 桶数组
  Hash hasher;

  // 思想：由于 buckets.size() 是固定的，此函数不需要加锁即可并发调用
  bucket_type& get_bucket(Key const& key) const {
    std::size_t const bucket_index = hasher(key) % buckets.size();
    return *buckets[bucket_index];
  }

 public:
  // 默认桶数采用质数（如 19），有助于哈希均匀分布
  threadsafe_lookup_table(unsigned num_buckets = 19,
                          Hash const& hasher_ = Hash())
      : buckets(num_buckets), hasher(hasher_) {
    for (unsigned i = 0; i < num_buckets; ++i) {
      buckets[i].reset(new bucket_type);
    }
  }

  // 禁止拷贝
  threadsafe_lookup_table(const threadsafe_lookup_table&) = delete;
  threadsafe_lookup_table& operator=(const threadsafe_lookup_table&) = delete;

  // 暴露给外部的接口：完全解耦
  Value value_for(Key const& key, Value const& default_value = Value()) const {
    return get_bucket(key).value_for(key, default_value);
  }

  void add_or_update_mapping(Key const& key, Value const& value) {
    get_bucket(key).add_or_update_mapping(key, value);
  }

  void remove_mapping(Key const& key) { get_bucket(key).remove_mapping(key); }
};