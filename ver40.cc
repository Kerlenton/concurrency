#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <mutex>
#include <regex>

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

    auto root = "http://iki.fi/bisqwit/ctu85/"s;

    std::vector<std::string> files;
    bool                     end = false;
}

void Producer() {
    cURLpp::initialize();
    std::vector<std::future<void>> tasks;
    for(const auto& p: {"8859-1.TXT"s, "8859-4.TXT"s, "8859-7.TXT"s, "8859-10.TXT"s, "8859-14.TXT"s,
                        "8859-2.TXT"s, "8859-5.TXT"s, "8859-8.TXT"s, "8859-11.TXT"s, "8859-15.TXT"s,
                        "8859-3.TXT"s, "8859-6.TXT"s, "8859-9.TXT"s, "8859-13.TXT"s, "8859-16.TXT"s})
        tasks.emplace_back(std::async(std::launch::async, [p]
        {
            std::string data = Download(root + p);
            files.emplace_back( std::move(data) );
        }));
    tasks.clear();
    // Notify feeders that we're done
    end = true;
}

void Feeder() {
    std::regex pat("([^\r\n]*)\r?\n"), pat2("ALEF", std::regex_constants::icase);
    for(;;) {
        // Check if we've got a file.
        while(!files.empty()) {
            std::string fil = std::move(files.back());
            files.pop_back();

            // Split file data into lines, find lines matching pat2
            std::smatch res;
            for(auto b = fil.cbegin(), e = fil.cend(); std::regex_search(b,e,res,pat); b = res[0].second) {
                if(std::regex_search(res[1].first, res[1].second, pat2)) {
                    std::cout << res[1] << std::endl;
                }
            }
        }
        // Nope, don't have a file. Are we about to end?
        if(end) break;
    }
}

int main() {
    std::async(std::launch::async, Producer), Feeder(); // Run Producer and Feeder simultaneously
}
