/**
 * @file 01_lock_free_stack.cpp
 * @brief 使用原子操作实现无锁栈 (CAS)
 */

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

struct Node {
  int value;
  Node* next;
};

std::atomic<Node*> head(nullptr);  // 原子指针

// 通过 CAS 自旋实现无锁入栈
void push(int val) {
  Node* new_node = new Node{val, nullptr};

  Node* old_head = head.load();  // 1. 读取旧值
  do {
    new_node->next = old_head;  // 将新节点的 next 指向旧头节点
  } while (!head.compare_exchange_weak(old_head, new_node));  // 2. CAS
}

// 演示结束后的打印/清理函数，非线程安全，仅供测试使用
void print_and_clear_stack() {
  Node* node = head.load();
  while (node) {
    std::cout << node->value << " -> ";
    Node* temp = node;
    node = node->next;
    delete temp;  // 清理节点
  }
  std::cout << std::endl;
}

int main() {
  std::thread t1([] {
    for (int i = 0; i < 5; ++i) push(i);
  });

  std::thread t2([] {
    for (int i = 5; i < 10; ++i) push(i);
  });

  t1.join();
  t2.join();

  print_and_clear_stack();

  return 0;
}