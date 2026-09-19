#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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

struct server_info_t final
{
    std::string name{ "ExampleServer" };
    std::string version{ "1.0.0" };
    std::string instructions{
        "This server provides weather and resource utilities."
    };
    std::array<std::string, 2> protocol_version{ "2026-07-28", "2025-11-25" };
    std::string                cache_scope{ "public" };
    std::int64_t               ttl_ms{ 3600000 };
};

template <typename R, typename T>
concept range_of =
    std::ranges::input_range<R> &&
    std::convertible_to<std::ranges::range_reference_t<R>, T const&>;

class server_t final
{
public:
    explicit server_t(server_info_t info)
        : info_(std::move(info)) {};

    void add(std::convertible_to<tool_t> auto&& v)
    {
        tools_.add(std::forward<decltype(v)>(v));
    }
    void add(range_of<tool_t> auto&& r)
    {
        tools_.add(std::forward<decltype(r)>(r));
    }

    void add(std::convertible_to<resource_t> auto&& v)
    {
        resources_.add(std::forward<decltype(v)>(v));
    }
    void add(range_of<resource_t> auto&& r)
    {
        resources_.add(std::forward<decltype(r)>(r));
    }

    void add(std::convertible_to<prompt_t> auto&& v)
    {
        prompts_.add(std::forward<decltype(v)>(v));
    }
    void add(range_of<prompt_t> auto&& r)
    {
        prompts_.add(std::forward<decltype(r)>(r));
    }

    [[nodiscard]] auto get_tools() const { return tools_.get(); }
    [[nodiscard]] auto get_resources() const { return resources_.get(); }
    [[nodiscard]] auto get_prompts() const { return prompts_.get(); }

    [[nodiscard]] server_info_t get_info() const { return info_; }

    [[nodiscard]] std::string handle_request(std::string const& raw) const;

private:
    template <typename T> class tracked_t final
    {
    public:
        void add(std::convertible_to<T> auto&& v)
        {
            std::unique_lock lock(mutex_);
            self_.emplace_back(std::forward<decltype(v)>(v));
            seen_.store(false, std::memory_order::release);
        }

        void add(range_of<T> auto&& r)
        {
            std::unique_lock lock(mutex_);
            if constexpr (std::ranges::sized_range<
                              std::remove_cvref_t<decltype(r)>>) {
                self_.reserve(self_.size() + std::ranges::size(r));
            }
            if constexpr (std::is_lvalue_reference_v<decltype(r)> ||
                          std::is_const_v<
                              std::remove_reference_t<decltype(r)>>) {
                std::ranges::copy(std::forward<decltype(r)>(r),
                                  std::back_inserter(self_));
            } else {
                std::ranges::move(std::forward<decltype(r)>(r),
                                  std::back_inserter(self_));
            }
            seen_.store(false, std::memory_order::release);
        }

        [[nodiscard]] std::vector<T> get() const
        {
            std::shared_lock lock(mutex_);
            std::vector<T>   out{ self_ };
            seen_.store(true, std::memory_order::release);
            return out;
        }

        [[nodiscard]] bool have_changed() const noexcept
        {
            return !seen_.load(std::memory_order::acquire);
        }

    private:
        std::vector<T>            self_{};
        mutable std::shared_mutex mutex_{};
        mutable std::atomic<bool> seen_{ true };
    };

    tracked_t<tool_t>     tools_{};
    tracked_t<resource_t> resources_{};
    tracked_t<prompt_t>   prompts_{};

    const server_info_t info_{};
};

} // namespace libmcp
