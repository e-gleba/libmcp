#include "handle.hpp"
#include "detail/json.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace libmcp {
namespace views = std::views;

using mcp_map_t = std::map<std::string, std::string>;

enum class rpc_error_code : std::int32_t
{
    parse            = -32700,
    invalid_request  = -32600,
    method_not_found = -32601,
    invalid_params   = -32602,
    internal         = -32603
};

constexpr std::string_view k_fallback_version{ "2026-07-28" };
constexpr std::string_view k_jsonrpc{ "2.0" };

constexpr const char* internal_fallback() noexcept
{
    return R"({"jsonrpc":"2.0","id":null,"error":{"code":-32603,"message":"Internal error"}})";
}

[[nodiscard]] mcp_map_t map_from_json(json_doc const& doc)
{
    try {
        return doc.flat_map();
    } catch (...) {
        return {};
    }
}

[[nodiscard]] mcp_map_t parse_to_map(std::string_view text)
{
    try {
        json_doc d = json_doc::parse(text);
        if (!d.is_object()) {
            return { { "error", "not_object" } };
        }
        return d.flat_map();
    } catch (...) {
        return { { "error", "parse_failed" }, { "raw", std::string{ text } } };
    }
}

[[nodiscard]] std::string convert_to_string(mcp_map_t const& m)
{
    return json_doc::object_from(m).dump();
}

namespace {

struct rpc_fail final
{
    rpc_error_code code;
    std::string    message;
};

[[nodiscard]] constexpr bool is_tpl(std::string_view u) noexcept
{
    return u.find('{') != std::string_view::npos &&
           u.find('}') != std::string_view::npos;
}

[[nodiscard]] inline std::string pack_msg(json_doc         id,
                                          std::string_view k,
                                          json_doc         p)
{
    try {
        json_doc o = json_doc::object();
        o.set("jsonrpc", k_jsonrpc);
        o.set("id", std::move(id));
        o.set(k, std::move(p));
        return o.dump();
    } catch (...) {
        return internal_fallback();
    }
}

[[nodiscard]] inline json_doc err_obj(rpc_error_code c, std::string_view m)
{
    return json_doc::object(
        { { "code", static_cast<std::int64_t>(c) }, { "message", m } });
}

[[nodiscard]] inline std::string rpc_error(json_doc         id,
                                           rpc_error_code   c,
                                           std::string_view m)
{
    return pack_msg(std::move(id), "error", err_obj(c, m));
}

[[nodiscard]] inline std::string rpc_result(json_doc id, json_doc p)
{
    return pack_msg(std::move(id), "result", std::move(p));
}

[[nodiscard]] inline std::string negotiate_version(server_t const&  s,
                                                   std::string_view client)
{
    auto vers = s.get_info().protocol_version;
    if (!client.empty()) {
        auto it = std::ranges::find(vers, client);
        if (it != vers.end()) {
            return *it;
        }
    }
    auto it = std::ranges::find_if(vers | views::reverse,
                                   [](auto const& v) { return !v.empty(); });
    if (it != (vers | views::reverse).end()) {
        return *it;
    }
    return std::string{ k_fallback_version };
}

[[nodiscard]] inline json_doc versions_json(server_t const& s)
{
    auto     vers = s.get_info().protocol_version;
    auto     v = vers | views::filter([](auto const& x) { return !x.empty(); });
    json_doc a = json_doc::array_from(v);
    if (a.size() == 0) {
        a.push(k_fallback_version);
    }
    return a;
}

[[nodiscard]] inline json_doc caps_json()
{
    return json_doc::object({ { "tools", json_doc::object() },
                              { "resources", json_doc::object() },
                              { "prompts", json_doc::object() } });
}

[[nodiscard]] inline json_doc sinfo_json(server_t const& s)
{
    auto i = s.get_info();
    return json_doc::object({ { "name", i.name }, { "version", i.version } });
}

[[nodiscard]] inline json_doc parse_schema(std::string const& s)
{
    try {
        if (json_doc p = json_doc::parse(s); p.is_object()) {
            return p;
        }
    } catch (...) {
    }
    return json_doc::object({ { "type", "object" } });
}

[[nodiscard]] inline json_doc tool_json(tool_t const& t)
{
    return json_doc::object(
        { { "name", t.name },
          { "description", t.description },
          { "inputSchema", parse_schema(t.input_schema) } });
}

[[nodiscard]] inline json_doc res_json(resource_t const& r)
{
    return json_doc::object({ { "uri", r.uri },
                              { "name", r.name },
                              { "title", r.title },
                              { "description", r.description },
                              { "mimeType", r.mime_type } });
}

[[nodiscard]] inline json_doc tpl_json(resource_t const& r)
{
    return json_doc::object({ { "uriTemplate", r.uri },
                              { "name", r.name },
                              { "title", r.title },
                              { "description", r.description },
                              { "mimeType", r.mime_type } });
}

[[nodiscard]] inline json_doc prompt_json(prompt_t const& p)
{
    return json_doc::object(
        { { "name", p.name },
          { "title", p.title },
          { "description", p.description },
          { "arguments", json_doc::array_from(p.arguments, [](auto const& a) {
                return json_doc::object({ { "name", a.name },
                                          { "title", a.title },
                                          { "description", a.description },
                                          { "required", a.required } });
            }) } });
}

[[nodiscard]] inline json_doc text_result(std::string_view t, bool err)
{
    json_doc c = json_doc::array();
    c.push(json_doc::object({ { "type", "text" }, { "text", t } }));
    json_doc o = json_doc::object();
    o.set("content", std::move(c));
    o.set("isError", err);
    return o;
}

template <std::ranges::input_range R, typename Proj>
[[nodiscard]] inline auto const& by_name(R const&         r,
                                         std::string_view name,
                                         Proj             proj,
                                         std::string_view kind)
{
    auto it = std::ranges::find(r, name, proj);
    if (it == std::ranges::end(r)) {
        throw rpc_fail{ .code    = rpc_error_code::invalid_params,
                        .message = std::string{ "Unknown " } +
                                   std::string{ kind } + ": " +
                                   std::string{ name } };
    }
    return *it;
}

[[nodiscard]] inline std::string need_str(json_doc const*  p,
                                          std::string_view k,
                                          std::string_view what)
{
    if (p == nullptr || !p->is_object()) {
        throw rpc_fail{ .code    = rpc_error_code::invalid_params,
                        .message = "Invalid params" };
    }
    json_doc d = p->find(k);
    if (!d.valid() || !d.is_string()) {
        throw rpc_fail{ .code = rpc_error_code::invalid_params,
                        .message =
                            std::string{ "Missing " } + std::string{ what } };
    }
    return d.as_string();
}

[[nodiscard]] inline json_doc on_init(server_t const& s, json_doc const* p)
{
    std::string      buf;
    std::string_view client;
    if (p != nullptr && p->is_object()) {
        if (json_doc v = p->find("protocolVersion");
            v.valid() && v.is_string()) {
            buf    = v.as_string();
            client = buf;
        }
    }
    return json_doc::object(
        { { "protocolVersion", negotiate_version(s, client) },
          { "capabilities", caps_json() },
          { "serverInfo", sinfo_json(s) } });
}

[[nodiscard]] inline json_doc on_disc(server_t const& s, json_doc const*)
{
    auto i = s.get_info();
    return json_doc::object(
        { { "resultType", "complete" },
          { "supportedVersions", versions_json(s) },
          { "capabilities", caps_json() },
          { "_meta",
            json_doc::object(
                { { "io.modelcontextprotocol/serverInfo", sinfo_json(s) } }) },
          { "instructions", i.instructions },
          { "ttlMs", i.ttl_ms },
          { "cacheScope", i.cache_scope } });
}

[[nodiscard]] inline json_doc on_tools(server_t const& s, json_doc const*)
{
    return json_doc::object(
        { { "tools", json_doc::array_from(s.get_tools(), tool_json) } });
}

[[nodiscard]] inline json_doc on_res(server_t const& s, json_doc const*)
{
    auto rs = s.get_resources();
    return json_doc::object(
        { { "resources",
            json_doc::array_from(rs | views::filter([](auto const& r) {
                                     return !is_tpl(r.uri);
                                 }),
                                 res_json) } });
}

[[nodiscard]] inline json_doc on_tpl(server_t const& s, json_doc const*)
{
    auto rs = s.get_resources();
    return json_doc::object(
        { { "resourceTemplates",
            json_doc::array_from(
                rs | views::filter([](auto const& r) { return is_tpl(r.uri); }),
                tpl_json) } });
}

[[nodiscard]] inline json_doc on_prompts(server_t const& s, json_doc const*)
{
    return json_doc::object(
        { { "prompts", json_doc::array_from(s.get_prompts(), prompt_json) } });
}

[[nodiscard]] inline json_doc on_ping(server_t const&, json_doc const*)
{
    return json_doc::object();
}

[[nodiscard]] inline json_doc on_call(server_t const& s, json_doc const* p)
{
    std::string name = need_str(p, "name", "tool name");
    std::string args = "{}";
    if (json_doc a = p->find("arguments"); a.valid()) {
        args = a.dump();
    }
    auto        tools = s.get_tools();
    auto const& t     = by_name(tools, name, &tool_t::name, "tool");
    try {
        return text_result(t.handler ? t.handler(args) : std::string{}, false);
    } catch (std::exception const& e) {
        return text_result(e.what(), true);
    }
}

[[nodiscard]] inline json_doc on_read(server_t const& s, json_doc const* p)
{
    std::string uri = need_str(p, "uri", "resource uri");
    auto        rss = s.get_resources();
    auto const& r   = by_name(rss, uri, &resource_t::uri, "resource");
    json_doc    e   = json_doc::object(
        { { "uri", r.uri },
          { "mimeType", r.mime_type },
          { "text", r.handler ? r.handler(uri) : std::string{} } });
    json_doc c = json_doc::array();
    c.push(std::move(e));
    return json_doc::object({ { "contents", std::move(c) } });
}

[[nodiscard]] inline json_doc on_get(server_t const& s, json_doc const* p)
{
    std::string name = need_str(p, "name", "prompt name");
    auto        prs  = s.get_prompts();
    auto const& pr   = by_name(prs, name, &prompt_t::name, "prompt");
    mcp_map_t   args;
    if (json_doc a = p->find("arguments"); a.valid() && a.is_object()) {
        args = a.flat_map();
    }
    json_doc content = json_doc::object(
        { { "type", "text" },
          { "text", pr.handler ? pr.handler(args) : std::string{} } });
    json_doc msg = json_doc::object(
        { { "role", "user" }, { "content", std::move(content) } });
    json_doc arr = json_doc::array();
    arr.push(std::move(msg));
    return json_doc::object(
        { { "description", pr.description }, { "messages", std::move(arr) } });
}

struct route_t final
{
    std::string_view method;
    json_doc (*fn)(server_t const&, json_doc const*);
};

constexpr std::array<route_t, 10> k_routes{ {
    { .method = "initialize", .fn = &on_init },
    { .method = "server/discover", .fn = &on_disc },
    { .method = "tools/list", .fn = &on_tools },
    { .method = "tools/call", .fn = &on_call },
    { .method = "resources/list", .fn = &on_res },
    { .method = "resources/templates/list", .fn = &on_tpl },
    { .method = "resources/read", .fn = &on_read },
    { .method = "prompts/list", .fn = &on_prompts },
    { .method = "prompts/get", .fn = &on_get },
    { .method = "ping", .fn = &on_ping },
} };

[[nodiscard]] inline json_doc current_id_of(json_doc const& doc)
{
    if (doc.is_object()) {
        if (json_doc id = doc.find("id"); id.valid() && !id.is_null()) {
            return id;
        }
    }
    return json_doc::null();
}

} // namespace

std::string server_t::handle_request(std::string const& raw) const
{
    json_doc doc{};
    try {
        doc = json_doc::parse(raw);
    } catch (...) {
        return rpc_error(
            json_doc::null(), rpc_error_code::parse, "Parse error");
    }
    try {
        if (!doc.is_object()) {
            return rpc_error(current_id_of(doc),
                             rpc_error_code::invalid_request,
                             "Invalid Request");
        }
        if (json_doc id = doc.find("id"); !id.valid() || id.is_null()) {
            return {};
        }
        json_doc id_copy = doc.find("id");
        json_doc m       = doc.find("method");
        if (!m.valid() || !m.is_string()) {
            return rpc_error(std::move(id_copy),
                             rpc_error_code::invalid_request,
                             "Invalid Request");
        }
        std::string method = m.as_string();
        if (std::string_view{ method }.starts_with("notifications/")) {
            return {};
        }
        json_doc        params_hold = doc.find("params");
        json_doc const* params = params_hold.valid() ? &params_hold : nullptr;
        if (auto it = std::ranges::find(
                k_routes, std::string_view{ method }, &route_t::method);
            it != k_routes.end()) {
            return rpc_result(std::move(id_copy), it->fn(*this, params));
        }
        return rpc_error(std::move(id_copy),
                         rpc_error_code::method_not_found,
                         "Method not found");
    } catch (rpc_fail const& e) {
        return rpc_error(current_id_of(doc), e.code, e.message);
    } catch (...) {
        return rpc_error(
            current_id_of(doc), rpc_error_code::internal, "Internal error");
    }
}

} // namespace libmcp
