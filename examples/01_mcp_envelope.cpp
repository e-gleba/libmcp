#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <handle.hpp>

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    static std::mutex     write_mu{};
    static constexpr auto k_delay{ std::chrono::milliseconds{ 1500 } };

    for (;;) {
        std::string line{};
        if (!std::getline(std::cin, line)) {
            std::cin.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
            continue;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        std::thread([req = std::move(line)] {
            try {
                std::this_thread::sleep_for(k_delay);
                std::string const res{ libmcp::handle_request(req) };
                std::lock_guard<std::mutex> const lock{ write_mu };
                std::cout << res << '\n' << std::flush;
            } catch (...) {
                std::lock_guard<std::mutex> const lock{ write_mu };
                std::cout << "error: handler threw\n" << std::flush;
            }
        }).detach();
    }
}
