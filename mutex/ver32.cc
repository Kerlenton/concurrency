#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::vector<bool> stuff;
std::mutex lock_stuff;

// There's also timed_mutex, recursive_mutex, and recursive_timed_mutex.

void some_thread() {
    for (unsigned i = 0; i < 1000; i++) {
        std::lock_guard<std::mutex> lk(lock_stuff);
        // ^ Blocks until we have exclusive access to stuff
        //   Released automatically when it goes out of scope

        // There is also try_lock() which doesn't block, it returns
        // a true/false indicating whether a lock was acquired or not.

        stuff.emplace_back();

        if (i == 500) break;

        std::cout << '\b';
    }
}

int main() {
    std::thread t1(some_thread), t2(some_thread);
    t1.join();
    t2.join();
    std::cout << "counter = " << stuff.size() << std::endl;
}
