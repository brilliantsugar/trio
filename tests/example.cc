#include <trio.h>

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals::chrono_literals;

bs::Trio<std::vector<std::string>> buf;
std::vector<std::string> consumer_strings;
std::vector<std::string> producer_strings;

std::atomic<bool> stop{false};

namespace {
    const size_t k_num_writes = 10000000;
}

void consumer() {
    while (!stop) {
        const auto& [strings, changed] = buf.Read();
        for (auto& str : strings) {
            if (changed) {
                consumer_strings.emplace_back(std::move(str));
            }
        }
        strings.clear();
        std::this_thread::yield();
    }
}

void producer() {
    while (!stop) {
        auto& v = buf.Write();
        v.clear();
        int sz = rand() % 1000;
        for (int i = 0; i < sz; ++i) {
            int len = rand() % 40;
            std::string s;
            for (int j = 0; j < len; ++j) {
                s += 'a' + (rand() % 26);
            }
            producer_strings.push_back(s);
            v.emplace_back(std::move(s));
            if (producer_strings.size() >= k_num_writes) {
                stop = true;
                break;
            }
        }
        buf.Commit();
        std::this_thread::yield();
    }
}

int main() {
    std::thread consumer_thread(consumer);
    std::thread producer_thread(producer);
    while (!stop) {
        std::this_thread::sleep_for(10ms);
    }
    consumer_thread.join();
    producer_thread.join();

    int cons = 0, prod = 0;
    while (cons < consumer_strings.size() && prod < producer_strings.size()) {
        if (consumer_strings[cons] != producer_strings[prod]) {
            ++prod;
            continue;
        }
        ++cons;
    }
    if (cons < consumer_strings.size()) {
        std::cout << "Data mismatch: cannot find string '" << consumer_strings[cons] << "'" << std::endl;
        return 1;
    }
    std::cout << "Success. produced: " << producer_strings.size() << "; consumed: " << consumer_strings.size() << std::endl;
    return 0;
}
