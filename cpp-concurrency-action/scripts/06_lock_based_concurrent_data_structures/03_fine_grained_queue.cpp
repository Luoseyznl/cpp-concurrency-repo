/**
 * @file 03_fine_grained_queue.cpp
 * @brief 基于锁的细粒度线程安全队列实现
 * 重点关注对于 链表、树 这类数据结构，使用手递手（步进式）锁定。
 */

#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
class fine_grained_queue {
 private:
  struct node {
    std::shared_ptr<T> data;     // 数据需要在多个节点间传递
    std::unique_ptr<node> next;  // 指针不能被共享，只能移交
  };

  std::mutex head_mutex;
  std::mutex tail_mutex;
  std::unique_ptr<node> head;

  node* tail;  // 原生指针，不拥有所有权，指向 head 链表的最后一个

  node* get_tail() {
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    return tail;
  }

  std::unique_ptr<node> pop_head() {
    std::unique_ptr<node> old_head = std::move(head);
    head = std::move(old_head->next);
    return old_head;
  }

 public:
  fine_grained_queue() : head(new node), tail(head.get()) {}  // 哑节点开头

  void push(T new_value) {
    std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
    std::unique_ptr<node> p(new node);
    node* const new_tail = p.get();

    {
      std::lock_guard<std::mutex> tail_lock(tail_mutex);  // 只锁尾部
      tail->data = new_data;  // 更新末尾节点数据并更新 tail 指针
      tail->next = std::move(p);
      tail = new_tail;
    }
  }

  std::shared_ptr<T> try_pop() {
    std::lock_guard<std::mutex> head_lock(head_mutex);

    // 检查是否为空 (head == tail)，为空就返回 nullptr
    if (head.get() == get_tail()) {
      return std::shared_ptr<T>();
    }

    std::shared_ptr<T> const res = head->data;
    std::unique_ptr<node> old_head = pop_head();
    return res;
  }
};

int main() {
  fine_grained_queue<int> fq;
  fq.push(42);
  auto p = fq.try_pop();
  if (p) std::cout << "Fine-grained Pop: " << *p << "\n";
  return 0;
}