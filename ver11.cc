#include <iostream>
#include <future>
#include <chrono>

int main() {
    auto show = [](auto&& t) { std::cout << t << std::endl; };

    // Let's create a simple "future" object.
    std::future<int> value;

    // There was nothing that supplies a value to the
    // "future" object. For instance, these will not compile:
    // value = 4;    // Doesn't compile!
    // value.set(4); // Doesn't compile!

    // One of the mechanisms to provide a value is called a "promise".
    std::promise<int> maker;
    value = maker.get_future();

    // Right now, it contains no value.

    // Is it valid?
    show( value.valid() ); // true

    // Let's set a value!
    maker.set_value(4);

    // Let's wait until it contains a value!
    // But only wait 2 seconds at most.
    value.wait_for(std::chrono::seconds(2));
    // value.wait(); // Wait indefinitely

    // What is the value now? Note: get() also calls wait().
    show( value.get() ); // 4

    // The value can only be read once. Calling get() again
    // would produce std::future_error "No associated state".

    // While a future can be reused, a promise can only be satisfied once.
    maker.set_value(13); // std::future_error "Promise already satisfied"
    show( value.get() ); // Not reached
}
