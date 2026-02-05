#include <atomic>
#include <chrono>
#include <iostream>
#include <new>
#include <optional>
#include <thread>
#include <vector>

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

template <typename T, size_t Capacity>
class SPSCQueue {
 private:
  struct alignas(hardware_destructive_interference_size) AlignedAtomic {
    std::atomic<size_t> val;
  };

  std::vector<T> buffer;
  AlignedAtomic head;
  AlignedAtomic tail;

 public:
  SPSCQueue() : buffer(Capacity + 1), head{0}, tail{0} {}

  bool push(const T& item) {
    const size_t current_tail = tail.val.load(std::memory_order_relaxed);
    const size_t next_tail = (current_tail + 1) % (Capacity + 1);

    if (next_tail == head.val.load(std::memory_order_acquire)) {
      return false;
    }
    buffer[current_tail] = item;
    tail.val.store(next_tail, std::memory_order_release);
    return true;
  }

  std::optional<T> pop() {
    const size_t current_head = head.val.load(std::memory_order_relaxed);
    if (current_head == tail.val.load(std::memory_order_acquire)) {
      return std::nullopt;
    }
    T item = buffer[current_head];
    head.val.store((current_head + 1) % (Capacity + 1),
                   std::memory_order_release);
    return item;
  }
};

int main() {
  SPSCQueue<long, 1024> queue;
  const long COUNT = 1000000;

  std::thread p([&]() {
    for (long i = 0; i < COUNT; ++i)
      while (!queue.push(i)) std::this_thread::yield();
  });

  std::thread c([&]() {
    for (long i = 0; i < COUNT; ++i) {
      std::optional<long> v;
      while (!(v = queue.pop())) std::this_thread::yield();
      if (*v != i) {
        std::cerr << "Order check failed!\n";
        exit(1);
      }
    }
  });

  p.join();
  c.join();
  std::cout << "03_spsc_ring_buffer: Processed " << COUNT << " items correctly."
            << std::endl;
  return 0;
}