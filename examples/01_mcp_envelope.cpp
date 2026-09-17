#include <2026-07-28/schema.hpp>

#include <any>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <typeinfo>
#include <variant>

namespace tb {
/// Throws std::bad_any_cast on type mismatch, returns fallback otherwise.
[[nodiscard]] inline bool json_has_value(const JsonValue& v) noexcept
{
    return v.has_value();
}

/// Throws nothing, true when empty or nullptr_t.
[[nodiscard]] inline bool json_is_null(const JsonValue& v) noexcept
{
    return !v.has_value() || v.type() == typeid(std::nullptr_t);
}

template <typename T>
[[nodiscard]] inline bool json_holds(const JsonValue& v) noexcept
{
    return v.type() == typeid(T);
}

template <typename T>
[[nodiscard]] inline const T* json_try_get(const JsonValue& v) noexcept
{
    return std::any_cast<T>(&v);
}

/// Throws std::bad_any_cast on mismatch.
template <typename T> [[nodiscard]] inline T json_get(const JsonValue& v)
{
    return std::any_cast<T>(v);
}

template <typename T>
[[nodiscard]] inline T json_get_or(const JsonValue& v, T fallback) noexcept(
    std::is_nothrow_move_constructible_v<T>)
{
    if (const T* p = std::any_cast<T>(&v)) {
        return *p;
    }
    return fallback;
}

/// Throws nothing, normalizes double/int64/string IDs.
[[nodiscard]] inline std::string to_string(const RequestId& id)
{
    if (const auto* s = std::get_if<std::string>(&id)) {
        return *s;
    }
    const auto d = std::get<double>(id);
    const auto i = static_cast<std::int64_t>(d);
    if (static_cast<double>(i) == d) {
        return std::to_string(i);
    }
    return std::to_string(d);
}

/// Throws nothing, value copy stands in for wire codec.
template <typename T> [[nodiscard]] inline T roundtrip_value(const T& v)
{
    return v;
}

/// Throws nothing, builds minimal client/server identity.
[[nodiscard]] inline Implementation make_impl(std::string name,
                                              std::string version)
{
    Implementation impl;
    impl.name    = std::move(name);
    impl.version = std::move(version);
    return impl;
}

/// Throws nothing, builds complete result marker.
[[nodiscard]] inline ClientResult make_complete(
    std::optional<ResultMetaObject> meta = std::nullopt)
{
    ClientResult r;
    r.result_type = "complete";
    r.meta        = std::move(meta);
    return r;
}

/// Throws nothing, builds protocol error.
[[nodiscard]] inline Error make_error(double      code,
                                      std::string message,
                                      JsonValue   data = JsonValue{})
{
    Error e;
    e.code    = code;
    e.message = std::move(message);
    e.data    = std::move(data);
    return e;
}
} // namespace tb

int main()
{
    Implementation client = tb::make_impl("check", "0.1.0");
    Implementation server = tb::make_impl("check-server", "0.1.0");

    ResultMetaObject meta;
    meta.io_modelcontextprotocol_server_info = server;

    ClientResult result = tb::make_complete(std::move(meta));
    RequestId    req_id = std::string{ "req-1" };
    Error        err    = tb::make_error(-32600.0, "Invalid Request");

    const Implementation back_client = tb::roundtrip_value(client);
    const ClientResult   back_result = tb::roundtrip_value(result);
    const RequestId      back_id     = tb::roundtrip_value(req_id);
    const Error          back_err    = tb::roundtrip_value(err);

    assert(back_client.name == "check");
    assert(back_client.version == "0.1.0");
    assert(back_result.result_type == "complete");
    assert(back_result.meta.has_value());
    assert(back_result.meta->io_modelcontextprotocol_server_info.has_value());
    assert(back_result.meta->io_modelcontextprotocol_server_info->name ==
           "check-server");
    assert(tb::to_string(back_id) == "req-1");
    assert(back_err.code == -32600.0);
    assert(back_err.message == "Invalid Request");
    assert(tb::json_has_value(back_err.data));
    assert(!tb::json_is_null(back_err.data) || tb::json_is_null(back_err.data));

    JsonValue any_str = JsonValue{ std::string{ "2026-07-28" } };
    JsonValue any_int = JsonValue{ std::int64_t{ 1 } };
    assert(tb::json_holds<std::string>(any_str));
    assert(tb::json_get<std::string>(any_str) == "2026-07-28");
    assert(tb::json_get_or<std::string>(any_str, "fallback") == "2026-07-28");
    assert(tb::json_get_or<std::string>(any_int, "fallback") == "fallback");

    std::cout << back_client.name << ' ' << back_client.version << '\n';
    std::cout << back_result.result_type << ' ' << tb::to_string(back_id)
              << '\n';
    return std::cout.good() ? EXIT_SUCCESS : EXIT_FAILURE;
}
