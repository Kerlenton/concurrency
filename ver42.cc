#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <mutex>
#include <regex>
#include <condition_variable>

using std::operator""s;

namespace {
    std::string Download(const std::string& url) {
        std::string result;

        cURLpp::Easy req;
        req.setOpt(cURLpp::Options::Url(url));
        req.setOpt(cURLpp::Options::FollowLocation(true));
        req.setOpt(cURLpp::Options::WriteFunction([&](const char* p, std::size_t size, std::size_t nmemb)
        {
            result.append(p, size*nmemb);
            return size*nmemb;
        }));
        req.perform();
        return result;
    }

    auto root = "http://iki.fi/bisqwit/ctu85/"s; // http://www.unicode.org/Public/MAPPINGS/ISO8859/

    std::vector<std::string> files;
    bool                     end = false;
    std::mutex               files_lock;
    std::condition_variable  status_changed;
}

void Producer() {
    cURLpp::initialize();
    std::vector<std::future<void>> tasks;
    for(const auto& p: {"8859-1.TXT"s, "8859-2.TXT"s, "8859-3.TXT"s, "8859-4.TXT"s, "8859-5.TXT"s,
                        "8859-6.TXT"s, "8859-7.TXT"s, "8859-8.TXT"s, "8859-9.TXT"s, "8859-10.TXT"s,
                        "8859-11.TXT"s,"8859-13.TXT"s,"8859-14.TXT"s,"8859-15.TXT"s,"8859-16.TXT"s})
        tasks.emplace_back(std::async(std::launch::async, [p]
        {
            std::string data = Download(root + p);
            // Use the mutex to get exclusive access to files[]
            std::unique_lock<std::mutex> lk(files_lock);
            files.emplace_back( std::move(data) );
            // Don't need exclusive access anymore
            lk.unlock();
            // Notify consumers
            status_changed.notify_one();
        }));
    tasks.clear();
    // Also notify consumers when we're done, so they know to quit.
    // Because of how condition_variable works, any notify_* calls must be preceded by
    // acquiring the mutex, even if the variable we're changing does not really need it.
    { std::unique_lock<std::mutex> lk(files_lock); end = true; }
    status_changed.notify_all();
}

void Consumer() {
    std::regex pat("([^\r\n]*)\r?\n"), pat2("ALEF", std::regex_constants::icase);
    // Acquire exclusive access to files[]
    std::unique_lock<std::mutex> lk(files_lock);
    for(;;) {
        // Check if we've got a file.
        while(!files.empty()) {
            std::string fil = std::move(files.back());
            files.pop_back();
            // Relinquish exclusive access now that we don't need to access files[] for a while.
            // This is optional, but makes sense.
            lk.unlock();

            // Split file data into lines, find lines matching pat2
            std::smatch res;
            for(auto b = fil.cbegin(), e = fil.cend(); std::regex_search(b,e,res,pat); b = res[0].second) {
                if(std::regex_search(res[1].first, res[1].second, pat2)) {
                    std::cout << res[1] << std::endl;
                }
            }
            // Reacquire exclusive access
            lk.lock();
        }
        // Nope, don't have a file. Are we about to end?
        if(end) break;
        // Wait until we get a signal from status_changed.
        // This also temporarily relinquishes exclusive access to files[].
        status_changed.wait(lk);
        // Note: wait() can also complete without status_changed having actually been signalled. This is
        // called a "spurious wakeup". Technically a bug, but it's a bug that the standard accounts for.
    }
}

int main() {
    std::async(std::launch::async, Producer), Consumer(); // Run Producer and Consumer simultaneously
}
