#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <string>

namespace libmcp {

struct tool_t final
{
    std::string name{};
    std::string description{};
    std::string input_schema{ "{}" };
    std::function<std::string(std::string const& args_json)> handler{};
};

struct resource_t final
{
    std::string uri{};
    std::string name{};
    std::string title{};
    std::string description{};
    std::string mime_type{ "text/plain" };
    std::function<std::string(std::string const& uri)> handler{};
};

struct prompt_arg_t final
{
    std::string name{};
    std::string title{};
    std::string description{};
    bool        required{ false };
};

struct prompt_t final
{
    std::string               name{};
    std::string               title{};
    std::string               description{};
    std::vector<prompt_arg_t> arguments{};
    std::function<std::string(std::map<std::string, std::string> const& args)>
        handler{};
};

template <typename R, typename T>
concept range_of =
    std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

class server_t final
{
public:
    const std::string name{ "ExampleServer" };
    const std::string version{ "1.0.0" };
    const std::string instructions{
        "This server provides weather and resource utilities."
    };
    const std::string  protocol_version{ "2026-07-28" };
    const std::string  cache_scope{ "public" };
    const std::int64_t ttl_ms{ 3600000 };

    void add(range_of<tool_t> auto& tools) { tools_.add(tools); }
    void add(range_of<resource_t> auto& resources)
    {
        resources_.add(resources);
    }
    void add(range_of<prompt_t> auto& prompts) { prompts_.add(prompts); }

    [[nodiscard]] std::string handle_request(std::string const& raw) const;

private:
    template <typename T> class tracked_t final
    {
    private:
        std::vector<T>    self_{};
        std::shared_mutex mutex_;
        std::atomic<bool> seen_;

    public:
        void add(range_of<T> auto& args)
        {
            std::unique_lock lock(mutex_);
            self_.push_back(args);
            seen_ = false;
        }

        [[nodiscard]] auto get()
        {
            seen_ = true;
            std::shared_lock lock(mutex_);
            return std::to_array({ self_ });
        }

        [[nodiscard]] bool have_changed() noexcept { return seen_; }
    };

    tracked_t<tool_t>     tools_{};
    tracked_t<resource_t> resources_{};
    tracked_t<prompt_t>   prompts_{};
};

} // namespace libmcp
