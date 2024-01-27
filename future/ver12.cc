#include <future>
#include <chrono>

#include <exception>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <regex>
#include <mutex>

// Out() is a variadic helper function that allows use a,b,c,d
// syntax instead of a<<b<<c<<d when outputting to ostreams.
// This is necessary when working with parameter packs.
template <typename O>
static O& Out(O& r) { return r; }
template <typename O, typename T, typename... R>
static O& Out(O& r, T&& a, R&&... rest) {
    return Out((r << std::forward<T>(a), r), std::forward<R>(rest)...);
}
// Color() returns a string that wraps the output in an ansi-escape code sequence.
template <typename... R>
static std::string Color(int n, R&&... values) {
    std::ostringstream o;
    Out(o, n >= 0 ? "\33[38;5;" : "\33[\3\10;\5;");
    if (n >= 0) {
        Out(o, n);
    } else {
        auto t = [](int n){ return char(n % 10 ? n % 10 : 10); };
        if (n <= -100) Out(o, t((-n) / 100));
        if (n <= -10) Out(o, t((-n) / 10));
        Out(o, t(-n));
    }
    return Out(o, 'm', std::forward<R>(values)..., "\33[m").str();
}
// Source() syntax-colors the source code of this very program,
// and manages the cur_line and end_line variables that are line-number
// indexes to the source code.
static std::vector<std::string> lines;
static unsigned cur_line = 0, end_line = 0;
static void Source(unsigned line, unsigned numlines) {
    if (lines.empty()) {
        for (std::fstream s(__FILE__); s.good();) {
            std::string l, l2;
            std::smatch res;
            std::getline(s, l);
            if (std::regex_match(l, res, std::regex("(.*)(//.*)"))) {
                l2 = Color(167, res[2]);
                l = res[1];
            }
            l = std::regex_replace(l, std::regex("[,;{}[\\]+&*%|^<>]"), Color(-2, "$&"));
            l = std::regex_replace(l, std::regex("::"),                 Color(-108, "$&"));
            l = std::regex_replace(l, std::regex("[-.()]"),             Color(-37, "$&"));
            l = std::regex_replace(l, std::regex("static|void|auto|int|const|char|try|throw|catch|return"), Color(-15, "$&"));
            l = std::regex_replace(l, std::regex("\"([^\"]*)\""),       Color(-32, "\"\33[\3\10;\5;\3\11m$1") + Color(-32, "\""));
            l = std::regex_replace(l, std::regex("[0-9]+"),             Color(135, "$&"));
            for(int n = 1; n <= 10; n++) {
                l = std::regex_replace(l, std::regex(std::string(1,char(n))), std::string(1, '0' + n % 10));
            }
            lines.push_back(l + l2);
        }
    }
    cur_line = line - 1;
    end_line = line - 1 + numlines;
}
// Write() outputs the given values to std::cout, adding a timestamp
// in the beginning of the line, and a source code line in the end of the line.
static std::mutex print_lock;
static bool printed = false;
template<bool do_lock = true, std::ostream& output = std::cout, typename... R>
static void Write(R&&... values) {
    std::unique_lock<std::mutex> l(print_lock, std::defer_lock);
    if (do_lock) l.lock();

    std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream s;
    std::string line = Out(s, std::put_time(std::localtime(&now_c), "%T"), Color(37, '|'), std::forward<R>(values)...).str();
    output << line;
    if (cur_line < end_line) {
        unsigned width = std::regex_replace(line, std::regex("\33[^Am]*[Am]"), "").size();
        output << Color(37, std::string(42 - width, ' '), '|') << lines[cur_line];
    }
    ++cur_line;
    output << std::endl;
    printed = true;
}

static void future_test(std::future<int>&& value, const char* label, unsigned line, unsigned num_lines, int mode = 0) {
    static const char states[4][9] = {"timeout", "ready", "deferred", "invalid"};
    auto state = [&] {
        if (!value.valid()) return 3u;
        switch (value.wait_for(std::chrono::hours(0))) {
            case std::future_status::timeout: default: return 0u;
            case std::future_status::ready:            return 1u;
            case std::future_status::deferred:         return 2u;
        }
    };

    if (mode < 2) Source(line, num_lines);
    Write("Future \"", label, "\": ", Color(35, states[state()]));
    for (int n = 0; (n < 4) && (!state()); n++) {
        std::unique_lock<std::mutex> l(print_lock);
        Write<false>("Waiting 0.5 seconds...");
        printed = false;

        l.unlock();
        value.wait_for(std::chrono::milliseconds(500));
        l.lock();

        if (printed) {
            Write<false>(                       "                       ", Color(35, states[state()]));
        } else {
            Write<false>("\33[1;", --cur_line, "AWaiting 0.5 seconds... ", Color(35, states[state()]));
        }
    }
    if (state() != 1) Write("I'll just call get() right now.");
    try {
        Write("The \"", label, "\" gives ", Color(35, value.get()), ".");
    } catch (const std::exception& e) {
        Write("");
        Write("Exception received!");
        Write("Type:   ", Color(35, typeid(e).name()));
        Write("Detail: ", Color(35, e.what()));
    }
    if (mode != 1) while (cur_line < end_line) Write("");
}

static void Sleep(float num_s, const char* label = "THREAD") {
    Write(Color(32, label, "_START"));
    std::this_thread::sleep_for(std::chrono::milliseconds(int(num_s * 1e3f)));
    Write(Color(32, label, "_END"));
}

// Asynchronous function call (thread with return value)
static void future_from_async_async() {
    std::future<int> value = std::async(std::launch::async,
                                        [] { Sleep(2.25, "ASYNC"); return 7; });
    future_test(std::move(value), "async task", __LINE__ - 4, 6);
}

// Deferred function call (like async, but without threads)
static void future_from_async_defer() {
    std::future<int> value = std::async(std::launch::deferred,
                                        [] { Sleep(3, "DEFER"); return 11; });
    future_test(std::move(value), "deferred task", __LINE__ - 4, 6);
}

// Promise: A channel for passing a single message across threads
//          without returning (thread can continue after set_value)
static void future_from_promise() {
    std::promise<int> p;
    std::thread test_thread( [&] { Sleep(1, "THREAD"); p.set_value(13); } );
    future_test(p.get_future(), "promise", __LINE__ - 5, 8);
    test_thread.join();
}

// The promise channel can also be used to pass an exception to anonther thread.
static void future_from_promise_taketwo() {
    std::promise<int> p;
    auto v = std::async(std::launch::async, [&]
    {
        try         { Sleep(0.5, "THREAD"); throw std::runtime_error("Kupo!"); }
        catch (...) { p.set_exception(std::current_exception()); }
    });
    future_test(p.get_future(), "promise", __LINE__ - 8, 10);
}

// Package: Call a function, return value will be received elsewhere
static int test_func(int num) { Sleep(1, "FUNC"); return num + 57; }

static void future_from_package() {
    std::packaged_task<int(int)> task{test_func};
    std::thread test_thread( [&] { Sleep(3, "THREAD"), task(42); } );
    future_test(task.get_future(), "packaged task", __LINE__ - 6, 9);
    test_thread.join();
}

// A future object must have an associated "state" to be used.
static void future_from_nothing() {
    future_test(std::future<int>{}, "nothing", __LINE__ - 2, 4);
}


int main() {
    std::cout << Color(37,"----------------\n"); future_from_nothing();
    std::cout << Color(37,"----------------\n"); future_from_async_async();
    std::cout << Color(37,"----------------\n"); future_from_async_defer();
    std::cout << Color(37,"----------------\n"); future_from_promise();
    std::cout << Color(37,"----------------\n"); future_from_promise_taketwo();
    std::cout << Color(37,"----------------\n"); future_from_package();
}
