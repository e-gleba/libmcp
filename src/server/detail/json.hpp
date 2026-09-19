#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace libmcp {

struct json_error final : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct jprop; // proxy, defined after json_doc

class json_doc final
{
public:
    json_doc() noexcept = default;
    ~json_doc();
    json_doc(json_doc&&) noexcept;
    json_doc& operator=(json_doc&&) noexcept;
    json_doc(json_doc const&);
    json_doc& operator=(json_doc const&);

    // implicit scalars: enable {"k", 42} / {.key="a", .value="x"}
    json_doc(std::nullptr_t) noexcept
        : json_doc(null())
    {
    }
    json_doc(bool b)
        : json_doc(boolean(b))
    {
    }
    json_doc(std::string_view s)
        : json_doc(string(s))
    {
    }
    json_doc(std::string const& s)
        : json_doc(string(s))
    {
    }
    json_doc(char const* s)
        : json_doc(string(s ? std::string_view{ s } : std::string_view{}))
    {
    }

    template <typename T>
        requires(!std::same_as<std::decay_t<T>, json_doc> &&
                 std::integral<std::decay_t<T>> &&
                 !std::same_as<std::decay_t<T>, bool>)
    json_doc(T v)
        : json_doc(integer(static_cast<std::int64_t>(v)))
    {
    }

    template <typename T>
        requires(!std::same_as<std::decay_t<T>, json_doc> &&
                 std::floating_point<std::decay_t<T>>)
    json_doc(T v)
        : json_doc(real(static_cast<double>(v)))
    {
    }

    template <typename T>
        requires(!std::same_as<std::decay_t<T>, json_doc> &&
                 std::constructible_from<json_doc, T const&>)
    json_doc(std::optional<T> const& o)
        : json_doc(o ? json_doc(*o) : null())
    {
    }

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool is_object() const noexcept;
    [[nodiscard]] bool is_array() const noexcept;
    [[nodiscard]] bool is_string() const noexcept;
    [[nodiscard]] bool is_bool() const noexcept;
    [[nodiscard]] bool is_number() const noexcept;
    [[nodiscard]] bool is_number_integer() const noexcept;
    [[nodiscard]] bool is_number_float() const noexcept;
    [[nodiscard]] bool is_null() const noexcept;

    [[nodiscard]] bool        contains(std::string_view key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] json_doc              find(std::string_view key) const;
    [[nodiscard]] json_doc              at(std::size_t i) const;
    [[nodiscard]] std::vector<json_doc> elements() const;
    [[nodiscard]] std::vector<std::pair<std::string, json_doc>> items() const;

    [[nodiscard]] std::string                        as_string() const;
    [[nodiscard]] bool                               as_bool() const;
    [[nodiscard]] std::int64_t                       as_int() const;
    [[nodiscard]] double                             as_double() const;
    [[nodiscard]] std::string                        dump() const;
    [[nodiscard]] std::map<std::string, std::string> flat_map() const;

    void set(std::string_view key, json_doc value);
    void push(json_doc value);

    template <typename T>
        requires(!std::same_as<std::decay_t<T>, json_doc> &&
                 std::constructible_from<json_doc, T &&>)
    void set(std::string_view key, T&& v)
    {
        set(key, json_doc(std::forward<T>(v)));
    }

    template <typename T>
        requires(!std::same_as<std::decay_t<T>, json_doc> &&
                 std::constructible_from<json_doc, T &&>)
    void push(T&& v)
    {
        push(json_doc(std::forward<T>(v)));
    }

    [[nodiscard]] static json_doc parse(std::string_view text);
    [[nodiscard]] static json_doc object();
    [[nodiscard]] static json_doc array();
    [[nodiscard]] static json_doc null();
    [[nodiscard]] static json_doc string(std::string_view s);
    [[nodiscard]] static json_doc boolean(bool b);
    [[nodiscard]] static json_doc integer(std::int64_t v);
    [[nodiscard]] static json_doc real(double v);

    template <typename T>
        requires(std::constructible_from<json_doc, T &&>)
    [[nodiscard]] static json_doc from(T&& v)
    {
        return json_doc(std::forward<T>(v));
    }

    // init-list entry points (defined after jprop, reuse
    // object_from/array_from)
    [[nodiscard]] static json_doc object(std::initializer_list<jprop> xs);
    [[nodiscard]] static json_doc array(std::initializer_list<json_doc> xs);

    // ranges: array_from(r) / array_from(r, proj)
    template <std::ranges::input_range R>
        requires(std::constructible_from<json_doc,
                                         std::ranges::range_reference_t<R>>)
    [[nodiscard]] static json_doc array_from(R&& r)
    {
        return array_from(std::forward<R>(r), std::identity{});
    }

    template <std::ranges::input_range R, typename Proj = std::identity>
        requires(
            std::invocable<Proj&, std::ranges::range_reference_t<R>> &&
            std::constructible_from<
                json_doc,
                std::invoke_result_t<Proj&, std::ranges::range_reference_t<R>>>)
    [[nodiscard]] static json_doc array_from(R&& r, Proj&& proj)
    {
        json_doc a = array();
        std::ranges::for_each(std::forward<R>(r), [&](auto&& x) {
            a.push(json_doc(std::invoke(proj, std::forward<decltype(x)>(x))));
        });
        return a;
    }

    // ranges: object_from(r of jprop-or-pair) / object_from(r, proj ->
    // jprop-or-pair)
    template <std::ranges::input_range R>
    [[nodiscard]] static json_doc object_from(R&& r)
    {
        json_doc o = object();
        std::ranges::for_each(std::forward<R>(r), [&](auto&& e) {
            put(o, std::forward<decltype(e)>(e));
        });
        return o;
    }

    template <std::ranges::input_range R, typename Proj>
        requires(std::invocable<Proj&, std::ranges::range_reference_t<R>>)
    [[nodiscard]] static json_doc object_from(R&& r, Proj&& proj)
    {
        json_doc o = object();
        std::ranges::for_each(std::forward<R>(r), [&](auto&& x) {
            put(o, std::invoke(proj, std::forward<decltype(x)>(x)));
        });
        return o;
    }

private:
    struct impl;
    std::shared_ptr<impl> rep_{};

    // single duck-typed put: {key,value} or [k,v] pair, perfect-forwarded
    template <typename F> static void put(json_doc& o, F&& f)
    {
        if constexpr (requires {
                          f.key;
                          f.value;
                      }) {
            o.set(std::forward<F>(f).key, std::forward<F>(f).value);
        } else {
            auto&& [k, v] = std::forward<F>(f);
            o.set(std::forward<decltype(k)>(k),
                  json_doc(std::forward<decltype(v)>(v)));
        }
    }
};

// aggregate proxy: keeps {.key=, .value=} + {"k", 42} without extra types
struct jprop final
{
    std::string key;
    json_doc    value;
};

inline json_doc json_doc::object(std::initializer_list<jprop> xs)
{
    return object_from(xs);
}

inline json_doc json_doc::array(std::initializer_list<json_doc> xs)
{
    return array_from(xs);
}

} // namespace libmcp
