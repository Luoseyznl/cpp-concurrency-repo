template <typename T>

// push 时只拿 tail 锁，pop 时只拿 head 锁，最大化并发
class fine_grained_queue {
 private:
  struct node {
    std::shared_ptr<T> data;
    std::unique_ptr<node> next;
  };
  std::mutex head_mutex;
  std::unique_ptr<node> head;
  std::mutex tail_mutex;
  node* tail;

  node* get_tail() {
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    return tail;
  }

  std::unique_ptr<node> pop_head() {
    std::lock_guard<std::mutex> head_lock(head_mutex);
    // 在 head 锁内去读 tail 的快照
    if (head.get() == get_tail()) {
      return nullptr;
    }
    std::unique_ptr<node> old_head = std::move(head);
    head = std::move(old_head->next);
    return old_head;
  }

 public:
  fine_grained_queue() : head(new node), tail(head.get()) {}

  void push(T new_value) {
    // 1. 锁外：分配内存构造数据和新节点（高开销）
    std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
    std::unique_ptr<node> p(new node);
    node* const new_tail = p.get();

    // 2. 锁内：极速修改指针
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    tail->data = new_data;
    tail->next = std::move(p);
    tail = new_tail;
  }

  std::shared_ptr<T> try_pop() {
    std::unique_ptr<node> old_head = pop_head();
    return old_head ? old_head->data : std::shared_ptr<T>();
  }
};