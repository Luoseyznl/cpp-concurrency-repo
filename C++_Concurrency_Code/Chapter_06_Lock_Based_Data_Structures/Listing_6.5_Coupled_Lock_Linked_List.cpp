#include <iostream>
#include <memory>
#include <mutex>

/**
 * @brief 线程安全链表
 * 设计思想：锁耦合（Lock Coupling / Hand-over-hand locking）
 * 1. 放弃全局锁，每个节点自带一把互斥量。
 * 2. 遍历时像爬梯子一样，先锁住下一个节点，再释放当前节点。
 * 3. 使用哨兵节点（Dummy Head）简化边界条件处理。
 */
template <typename T>
class threadsafe_list {
 private:
  struct node {
    std::mutex m;  // 每个节点都有自己的锁
    std::shared_ptr<T> data;
    std::unique_ptr<node> next;

    node() : next() {}  // 哨兵节点构造函数
    node(T const& value)
        :  // 普通节点构造函数
          data(std::make_shared<T>(value)) {}
  };

  node head;  // 哨兵头节点

 public:
  threadsafe_list() {}

  ~threadsafe_list() {
    // 删除所有节点，确保清理安全
    remove_if([](T const&) { return true; });
  }

  // 禁止拷贝和赋值，防止复杂的锁管理问题
  threadsafe_list(threadsafe_list const&) = delete;
  threadsafe_list& operator=(threadsafe_list const&) = delete;

  /**
   * @brief 在链表头部插入元素
   * 思想：只需锁住哨兵头节点，分配内存放在锁外以提高并发度
   */
  void push_front(T const& value) {
    std::unique_ptr<node> new_node(new node(value));
    std::lock_guard<std::mutex> lk(head.m);
    new_node->next = std::move(head.next);
    head.next = std::move(new_node);
  }

  /**
   * @brief 遍历操作
   * 思想：手递手加锁。在处理当前节点时，锁住当前节点；在移动前，先锁住
   * next，再释放当前。
   */
  template <typename Function>
  void for_each(Function f) {
    node* current = &head;
    std::unique_lock<std::mutex> lk(head.m);  // 起步：锁住头节点

    while (node* const next = current->next.get()) {
      std::unique_lock<std::mutex> next_lk(next->m);  // 1. 先锁住下一节点
      lk.unlock();  // 2. 解锁当前节点（手递手完成）

      f(*next->data);  // 3. 在锁保护下处理数据

      current = next;
      lk = std::move(next_lk);  // 4. 将 next 的锁所有权转交给 lk，继续循环
    }
  }

  /**
   * @brief 查找操作
   * 逻辑同 for_each，但在命中谓词时提前返回
   */
  template <typename Predicate>
  std::shared_ptr<T> find_first_if(Predicate p) {
    node* current = &head;
    std::unique_lock<std::mutex> lk(head.m);
    while (node* const next = current->next.get()) {
      std::unique_lock<std::mutex> next_lk(next->m);
      lk.unlock();
      if (p(*next->data)) {
        return next->data;  // 返回智能指针，确保外部访问时数据依然有效
      }
      current = next;
      lk = std::move(next_lk);
    }
    return std::shared_ptr<T>();
  }

  /**
   * @brief 条件删除
   * 思想：删除节点时必须持有“前驱节点”的锁，以修改其 next 指针
   */
  template <typename Predicate>
  void remove_if(Predicate p) {
    node* current = &head;
    std::unique_lock<std::mutex> lk(head.m);
    while (node* const next = current->next.get()) {
      std::unique_lock<std::mutex> next_lk(next->m);
      if (p(*next->data)) {
        // 命中删除条件：
        std::unique_ptr<node> old_next = std::move(current->next);
        current->next = std::move(next->next);  // 绕过 next 节点
        next_lk.unlock();
        // old_next 离开作用域，节点正式销毁
      } else {
        // 未命中：正常手递手移动
        lk.unlock();
        current = next;
        lk = std::move(next_lk);
      }
    }
  }
};