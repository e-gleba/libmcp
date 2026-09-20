#include <handle.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <charconv>
#include <concepts>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace app {

// NDK libc++ (and old libstdc++): <stop_token>/<jthread> may be absent even
// in C++20/23 mode. Close() unblocks pop(); workers poll closed_ instead.
template <typename H>
concept request_handler = requires(H& h, std::string const& s) {
    { h.handle_request(s) } -> std::convertible_to<std::string>;
};
static_assert(request_handler<libmcp::server_t>);

[[nodiscard]] inline bool is_space(unsigned char c) noexcept
{
    return std::isspace(c) != 0;
}

[[nodiscard]] inline bool is_blank(std::string_view s) noexcept
{
    return std::ranges::all_of(s, is_space);
}

[[nodiscard]] inline std::string_view trim_left(std::string_view s) noexcept
{
    auto it{ std::ranges::find_if_not(s, is_space) };
    return { it, s.end() };
}

inline void strip_crlf(std::string& s) noexcept
{
    if (s.ends_with('\r')) {
        s.pop_back();
    }
}

[[nodiscard]] inline bool starts_with_ci(std::string_view s,
                                         std::string_view prefix) noexcept
{
    if (s.size() < prefix.size()) {
        return false;
    }
    return std::ranges::equal(
        prefix, s.substr(0, prefix.size()), [](char a, char b) noexcept {
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        });
}

static constexpr std::string_view k_length_prefix{ "Content-Length:" };
static constexpr std::string_view k_handler_error{
    R"({"error":"handler threw"})"
};

[[nodiscard]] inline std::optional<std::size_t> parse_content_length(
    std::string_view line) noexcept
{
    std::string_view t{ trim_left(line) };
    if (!starts_with_ci(t, k_length_prefix)) {
        return std::nullopt;
    }
    std::string_view num{ trim_left(t.substr(k_length_prefix.size())) };
    std::size_t      value{ 0 };
    auto [ptr,
          ec]{ std::from_chars(num.data(), num.data() + num.size(), value) };
    if (ec != std::errc{}) {
        return std::nullopt;
    }
    return value;
}

template <std::movable T> class channel final
{
public:
    void push(T v)
    {
        {
            std::lock_guard<std::mutex> lock{ mu_ };
            queue_.push_back(std::move(v));
        }
        cv_.notify_one();
    }

    [[nodiscard]] bool pop(T& out)
    {
        std::unique_lock<std::mutex> lock{ mu_ };
        cv_.wait(lock, [this] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void close() noexcept
    {
        {
            std::lock_guard<std::mutex> lock{ mu_ };
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::mutex              mu_{};
    std::condition_variable cv_{};
    std::deque<T>           queue_{};
    bool                    closed_{ false };
};

class frame_assembler final
{
public:
    frame_assembler() { pending_.reserve(4096); }

    [[nodiscard]] std::optional<std::string> feed(std::string_view line)
    {
        pending_.append(line).push_back('\n');
        std::ranges::for_each(line, [this](char ch) { feed_char(ch); });
        if (!complete()) {
            return std::nullopt;
        }
        return take();
    }

    [[nodiscard]] std::optional<std::string> flush()
    {
        if (is_blank(pending_)) {
            clear();
            return std::nullopt;
        }
        return take();
    }

    [[nodiscard]] bool complete() const noexcept
    {
        return has_data_ && depth_ == 0 && !in_string_;
    }

    void clear() noexcept
    {
        pending_.clear();
        depth_     = 0;
        has_data_  = false;
        in_string_ = false;
        escaped_   = false;
    }

private:
    constexpr void feed_char(char ch) noexcept
    {
        if (in_string_) {
            if (escaped_) {
                escaped_ = false;
            } else if (ch == '\\') {
                escaped_ = true;
            } else if (ch == '"') {
                in_string_ = false;
            }
            return;
        }
        if (ch == '"') {
            in_string_ = true;
            has_data_  = true;
        } else if (ch == '{' || ch == '[') {
            ++depth_;
            has_data_ = true;
        } else if (ch == '}' || ch == ']') {
            depth_ = std::max(0, depth_ - 1);
        }
    }

    [[nodiscard]] std::optional<std::string> take()
    {
        std::string out{ std::move(pending_) };
        clear();
        pending_.reserve(4096);
        return out;
    }

    std::string pending_{};
    int         depth_{ 0 };
    bool        has_data_{ false };
    bool        in_string_{ false };
    bool        escaped_{ false };
};

[[nodiscard]] inline std::string read_body(std::istream& in, std::size_t n)
{
    if (int const c{ in.peek() }; c == '\r' || c == '\n') {
        std::string sep{};
        std::getline(in, sep);
    }
    std::string body(n, '\0');
    in.read(body.data(), static_cast<std::streamsize>(n));
    body.resize(
        static_cast<std::size_t>(std::max<std::streamsize>(0, in.gcount())));
    return body;
}

// Closes the channel and joins every spawned worker on scope exit, so an
// exception between emplace_back and the manual join cannot destroy a
// joinable std::thread (std::terminate) before the outer catch runs.
class worker_pool final
{
public:
    worker_pool(channel<std::string>& in,
                std::vector<std::thread>& workers) noexcept
        : in_{ in }
        , workers_{ workers }
    {
    }

    worker_pool(worker_pool const&)            = delete;
    worker_pool(worker_pool&&)                 = delete;
    worker_pool& operator=(worker_pool const&) = delete;
    worker_pool& operator=(worker_pool&&)      = delete;

    ~worker_pool() noexcept
    {
        in_.close();
        for (auto& w : workers_) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

private:
    channel<std::string>&    in_;
    std::vector<std::thread>& workers_;
};

} // namespace app

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    try {
        app::channel<std::string> in{};
        std::mutex                write_mu{};
        std::mutex                server_mu{};
        std::atomic<bool>         io_ok{ true };

        libmcp::server_info_t info{};
        libmcp::server_t      server{ info };

        libmcp::tool_t weather{};
        weather.name        = "get_weather";
        weather.description = "Get current weather information for a location";
        weather.input_schema =
            R"({"type":"object","properties":{"location":{"type":"string","description":"City name or zip code"}},"required":["location"],"additionalProperties":false})";
        weather.handler = [](std::string const& args_json) -> std::string {
            if (app::is_blank(args_json)) {
                return "Missing location. Pass {\"location\":\"Minsk\"}.";
            }
            return std::string{ "Weather stub, args=" } + args_json;
        };
        server.add(weather);

        std::size_t const n_workers{ std::ranges::max(
            { std::size_t{ 1 },
              static_cast<std::size_t>(
                  std::thread::hardware_concurrency()) }) };

        std::vector<std::thread> workers{};
        workers.reserve(n_workers);
        app::worker_pool pool{ in, workers };
        for ([[maybe_unused]] auto _ :
             std::views::iota(std::size_t{ 0 }, n_workers)) {
            workers.emplace_back([&in, &write_mu, &server_mu, &io_ok, &server] {
                std::string req{};
                while (in.pop(req)) {
                    std::string res{};
                    try {
                        std::lock_guard<std::mutex> slock{ server_mu };
                        res = server.handle_request(req);
                    } catch (...) {
                        res = std::string{ app::k_handler_error };
                    }
                    if (res.empty()) {
                        continue;
                    }
                    std::lock_guard<std::mutex> wlock{ write_mu };
                    std::cout << res << std::endl;
                    if (!std::cout) {
                        io_ok.store(false, std::memory_order_relaxed);
                        break;
                    }
                }
            });
        }

        app::frame_assembler framer{};
        std::string          line{};

        while (std::getline(std::cin, line)) {
            app::strip_crlf(line);

            if (auto len{ app::parse_content_length(line) }) {
                std::string body{ app::read_body(std::cin, len.value()) };
                if (!app::is_blank(body)) {
                    in.push(std::move(body));
                }
                framer.clear();
                continue;
            }

            if (app::is_blank(line)) {
                if (auto msg{ framer.flush() }) {
                    in.push(std::move(msg.value()));
                }
                continue;
            }

            if (auto msg{ framer.feed(line) }) {
                in.push(std::move(msg.value()));
            }
        }

        if (auto msg{ framer.flush() }) {
            in.push(std::move(msg.value()));
        }
    } catch (...) {
        return EXIT_FAILURE;
    }

    bool const ok{ true };
    (void)ok;
    return EXIT_SUCCESS;
}