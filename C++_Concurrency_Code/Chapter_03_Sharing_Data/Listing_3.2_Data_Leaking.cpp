#include <iostream>
#include <thread>
#include <mutex>
#include <string>

// 1. 受保护的数据结构
struct Data {
    int a;
    std::string b;
};

// 2. 恶意攻击者类：用来存放“潜逃”出来的数据地址
class Malicious_Entity {
public:
    Data* leaked_ptr = nullptr;

    // 恶意函数：看起来在干正事，其实在偷地址
    void steal_data(Data& data) {
        leaked_ptr = &data;
        std::cout << "[Attacker] I've stolen the pointer to protected data!" << std::endl;
    }
};

// 3. 看似安全的包装类
class Data_Wrapper {
private:
    Data data;
    std::mutex m;

public:
    Data_Wrapper() : data{10, "Protected Content"} {}

    // 【风险途径 A】：通过返回值泄露
    Data& get_data_leaking_via_return() {
        std::lock_guard<std::mutex> lock(m);
        return data; // 锁在这里就释放了，但引用被传出去了
    }

    // 【风险途径 B】：通过外部函数参数泄露
    template<typename Function>
    void process_data(Function f) {
        std::lock_guard<std::mutex> lock(m);
        f(data); // 危险！调用了外部不可控的函数
    }

    void print_status() {
        std::lock_guard<std::mutex> lock(m);
        std::cout << "[Status] a = " << data.a << ", b = " << data.b << std::endl;
    }
};

int main() {
    Data_Wrapper my_safe_box;
    Malicious_Entity attacker;

    std::cout << "--- Starting Experiment ---" << std::endl;
    my_safe_box.print_status();

    // 展示途径 B：恶意函数作为参数传入
    // 尽管 process_data 内部有锁，但恶意函数记录了指针
    my_safe_box.process_data([&](Data& d) {
        attacker.steal_data(d);
    });

    // 展示途径 A：通过非法返回值直接获取引用
    // Data& leaked_ref = my_safe_box.get_data_leaking_via_return();

    // 此时主线程已经没有任何锁了，但我们可以直接通过“偷来”的指针修改数据！
    if (attacker.leaked_ptr) {
        attacker.leaked_ptr->a = 999;
        attacker.leaked_ptr->b = "Hacked without Mutex!";
        std::cout << "[Main] Modified data directly via stolen pointer." << std::endl;
    }

    // 检查结果
    my_safe_box.print_status();

    return 0;
}