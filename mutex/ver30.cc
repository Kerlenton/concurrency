#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::vector<bool> stuff;
std::mutex lock_stuff;

// There's also timed_mutex, recursive_mutex, and recursive_timed_mutex.

void some_thread() {
    for (unsigned i = 0; i < 1000; i++) {
        lock_stuff.lock();
        // ^ Blocks until we have exclusive access to stuff

        // There is also try_lock() which doesn't block, it returns
        // a true/false indicating whether a lock was acquired or not.

        stuff.emplace_back();

        lock_stuff.unlock();
        // ^ Release the lock, so other threads can use struff

        std::cout << '\b';
    }
}

int main() {
    std::thread t1(some_thread), t2(some_thread);
    t1.join();
    t2.join();
    std::cout << "counter = " << stuff.size() << std::endl;
}
