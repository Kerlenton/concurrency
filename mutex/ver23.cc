#include <iostream>
#include <thread>
#include <vector>

std::vector<bool> stuff;
// std::atomic< std::vector<bool> > stuff; ???

void some_thread() {
    for (unsigned i = 0; i < 1000; i++) {
        stuff.emplace_back();
        std::cout << '\b';
    }
}

int main() {
    std::thread t1(some_thread), t2(some_thread);
    t1.join();
    t2.join();
    std::cout << "counter = " << stuff.size() << std::endl;
}
