#include <iostream>
#include <thread>
#include <atomic>

std::atomic<double> counter{0};

void some_thread() {
    for (unsigned i = 0; i < 1000; i++) {
        // ++counter; // Does not compile
        for (double v = counter.load(); counter.compare_exchange_weak(v, v + 1) == false;)
            {}
        std::cout << '\b';
    }
}

int main() {
    std::thread t1(some_thread), t2(some_thread);
    t1.join();
    t2.join();
    std::cout << "counter = " << counter << std::endl;
}
