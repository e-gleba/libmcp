#include "handle.hpp"
#include "json.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include <2026-07-28/schema.hpp>

namespace libmcp {

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

constexpr const char* internal_fallback() noexcept
{
    return R"({"jsonrpc":"2.0","id":null,"error":{"code":-32603,"message":"Internal error"}})";
}

[[nodiscard]] mcp_map_t map_from_json(json_doc const& doc)
{
    try {
        return doc.flat_map();
    } catch (...) {
        return mcp_map_t{};
    }
}

[[nodiscard]] mcp_map_t parse_to_map(std::string_view text)
{
    try {
        json_doc doc = json_doc::parse(text);
        if (!doc.is_object())
            return mcp_map_t{ { "error", "not_object" } };
        return doc.flat_map();
    } catch (...) {
        return mcp_map_t{ { "error", "parse_failed" },
                          { "raw", std::string{ text } } };
    }
}

[[nodiscard]] std::string convert_to_string(mcp_map_t const& m)
{
    json_doc out = json_doc::object();
    for (auto const& [k, v] : m)
        out.set(k, json_doc::string(v));
    return out.dump();
}

namespace {

[[nodiscard]] std::string pack_msg(json_doc         id,
                                   std::string_view key,
                                   json_doc         payload)
{
    try {
        json_doc out = json_doc::object();
        out.set("jsonrpc", json_doc::string("2.0"));
        out.set("id", std::move(id));
        out.set(key, std::move(payload));
        return out.dump();
    } catch (...) {
        return internal_fallback();
    }
}

[[nodiscard]] json_doc err_pay(rpc_error_code code, std::string_view message)
{
    json_doc out = json_doc::object();
    out.set("code", json_doc::integer(static_cast<std::int64_t>(code)));
    out.set("message", json_doc::string(message));
    return out;
}

[[nodiscard]] std::string rpc_error(json_doc         id,
                                    rpc_error_code   code,
                                    std::string_view message)
{
    return pack_msg(std::move(id), "error", err_pay(code, message));
}

[[nodiscard]] std::string rpc_result(json_doc id, json_doc payload)
{
    return pack_msg(std::move(id), "result", std::move(payload));
}

struct rpc_fail
{
    rpc_error_code code;
    std::string    message;
};

[[nodiscard]] constexpr bool is_tpl(std::string_view uri) noexcept
{
    return uri.find('{') != std::string_view::npos &&
           uri.find('}') != std::string_view::npos;
}

[[nodiscard]] std::string_view negotiate_version(server_t const&  server,
                                                 std::string_view client)
{
    if (!client.empty()) {
        auto it = std::ranges::find(server.protocol_version, client);
        if (it != server.protocol_version.end())
            return *it;
    }
    for (auto const& v : server.protocol_version | std::views::reverse)
        if (!v.empty())
            return v;
    return k_fallback_version;
}

[[nodiscard]] json_doc versions_json(server_t const& server)
{
    json_doc arr = json_doc::array();
    for (auto const& v :
         server.protocol_version | std::views::filter([](std::string const& v) {
             return !v.empty();
         }))
        arr.push(json_doc::string(v));
    if (arr.size() == 0)
        arr.push(json_doc::string(k_fallback_version));
    return arr;
}

[[nodiscard]] json_doc caps_json()
{
    json_doc out = json_doc::object();
    out.set("tools", json_doc::object());
    out.set("resources", json_doc::object());
    out.set("prompts", json_doc::object());
    return out;
}

[[nodiscard]] json_doc sinfo_json(server_t const& server)
{
    json_doc out = json_doc::object();
    out.set("name", json_doc::string(server.name));
    out.set("version", json_doc::string(server.version));
    return out;
}

[[nodiscard]] json_doc parse_schema(std::string const& s)
{
    try {
        json_doc p = json_doc::parse(s);
        if (p.is_object())
            return p;
    } catch (...) {
    }
    json_doc out = json_doc::object();
    out.set("type", json_doc::string("object"));
    return out;
}

[[nodiscard]] json_doc on_init(server_t const& server, json_doc const* params)
{
    std::string      buf{};
    std::string_view client{};
    if (params != nullptr && params->is_object()) {
        json_doc v = params->find("protocolVersion");
        if (v.valid() && v.is_string()) {
            buf    = v.as_string();
            client = buf;
        }
    }
    json_doc out = json_doc::object();
    out.set("protocolVersion",
            json_doc::string(negotiate_version(server, client)));
    out.set("capabilities", caps_json());
    out.set("serverInfo", sinfo_json(server));
    return out;
}

[[nodiscard]] json_doc on_disc(server_t const& server, json_doc const*)
{
    json_doc meta = json_doc::object();
    meta.set("io.modelcontextprotocol/serverInfo", sinfo_json(server));
    json_doc out = json_doc::object();
    out.set("resultType", json_doc::string("complete"));
    out.set("supportedVersions", versions_json(server));
    out.set("capabilities", caps_json());
    out.set("_meta", std::move(meta));
    out.set("instructions", json_doc::string(server.instructions));
    out.set("ttlMs", json_doc::integer(server.ttl_ms));
    out.set("cacheScope", json_doc::string(server.cache_scope));
    return out;
}

[[nodiscard]] json_doc on_tools(server_t const& server, json_doc const*)
{
    auto     tools{ server.get_tools() };
    json_doc arr = json_doc::array();
    for (auto const& t : tools) {
        json_doc item = json_doc::object();
        item.set("name", json_doc::string(t.name));
        item.set("description", json_doc::string(t.description));
        item.set("inputSchema", parse_schema(t.input_schema));
        arr.push(std::move(item));
    }
    json_doc out = json_doc::object();
    out.set("tools", std::move(arr));
    return out;
}

[[nodiscard]] json_doc on_res(server_t const& server, json_doc const*)
{
    auto     resources{ server.get_resources() };
    json_doc arr = json_doc::array();
    for (auto const& r :
         resources | std::views::filter(
                         [](resource_t const& r) { return !is_tpl(r.uri); })) {
        json_doc item = json_doc::object();
        item.set("uri", json_doc::string(r.uri));
        item.set("name", json_doc::string(r.name));
        item.set("title", json_doc::string(r.title));
        item.set("description", json_doc::string(r.description));
        item.set("mimeType", json_doc::string(r.mime_type));
        arr.push(std::move(item));
    }
    json_doc out = json_doc::object();
    out.set("resources", std::move(arr));
    return out;
}

[[nodiscard]] json_doc on_tpl(server_t const& server, json_doc const*)
{
    auto     resources{ server.get_resources() };
    json_doc arr = json_doc::array();
    for (auto const& r :
         resources | std::views::filter(
                         [](resource_t const& r) { return is_tpl(r.uri); })) {
        json_doc item = json_doc::object();
        item.set("uriTemplate", json_doc::string(r.uri));
        item.set("name", json_doc::string(r.name));
        item.set("title", json_doc::string(r.title));
        item.set("description", json_doc::string(r.description));
        item.set("mimeType", json_doc::string(r.mime_type));
        arr.push(std::move(item));
    }
    json_doc out = json_doc::object();
    out.set("resourceTemplates", std::move(arr));
    return out;
}

[[nodiscard]] json_doc on_prompts(server_t const& server, json_doc const*)
{
    auto     prompts{ server.get_prompts() };
    json_doc arr = json_doc::array();
    for (auto const& p : prompts) {
        json_doc args = json_doc::array();
        for (auto const& a : p.arguments) {
            json_doc item = json_doc::object();
            item.set("name", json_doc::string(a.name));
            item.set("title", json_doc::string(a.title));
            item.set("description", json_doc::string(a.description));
            item.set("required", json_doc::boolean(a.required));
            args.push(std::move(item));
        }
        json_doc item = json_doc::object();
        item.set("name", json_doc::string(p.name));
        item.set("title", json_doc::string(p.title));
        item.set("description", json_doc::string(p.description));
        item.set("arguments", std::move(args));
        arr.push(std::move(item));
    }
    json_doc out = json_doc::object();
    out.set("prompts", std::move(arr));
    return out;
}

[[nodiscard]] json_doc on_ping(server_t const&, json_doc const*)
{
    return json_doc::object();
}

[[nodiscard]] json_doc on_call(server_t const& server, json_doc const* params)
{
    if (params == nullptr || !params->is_object())
        throw rpc_fail{ rpc_error_code::invalid_params, "Invalid params" };
    json_doc name_doc = params->find("name");
    if (!name_doc.valid() || !name_doc.is_string())
        throw rpc_fail{ rpc_error_code::invalid_params, "Missing tool name" };
    std::string name = name_doc.as_string();

    std::string args_json{ "{}" };
    json_doc    args_doc = params->find("arguments");
    if (args_doc.valid())
        args_json = args_doc.dump();

    auto tools{ server.get_tools() };
    auto it = std::ranges::find(tools, name, &tool_t::name);
    if (it == tools.end())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Unknown tool: " + name };
    try {
        std::string text = it->handler ? it->handler(args_json) : std::string{};
        json_doc    content = json_doc::array();
        json_doc    entry   = json_doc::object();
        entry.set("type", json_doc::string("text"));
        entry.set("text", json_doc::string(text));
        content.push(std::move(entry));
        json_doc out = json_doc::object();
        out.set("content", std::move(content));
        out.set("isError", json_doc::boolean(false));
        return out;
    } catch (std::exception const& e) {
        json_doc content = json_doc::array();
        json_doc entry   = json_doc::object();
        entry.set("type", json_doc::string("text"));
        entry.set("text", json_doc::string(e.what()));
        content.push(std::move(entry));
        json_doc out = json_doc::object();
        out.set("content", std::move(content));
        out.set("isError", json_doc::boolean(true));
        return out;
    }
}

[[nodiscard]] json_doc on_read(server_t const& server, json_doc const* params)
{
    if (params == nullptr || !params->is_object())
        throw rpc_fail{ rpc_error_code::invalid_params, "Invalid params" };
    json_doc uri_doc = params->find("uri");
    if (!uri_doc.valid() || !uri_doc.is_string())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Missing resource uri" };
    std::string uri = uri_doc.as_string();

    auto resources{ server.get_resources() };
    auto it = std::ranges::find(resources, uri, &resource_t::uri);
    if (it == resources.end())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Unknown resource: " + uri };
    std::string text  = it->handler ? it->handler(uri) : std::string{};
    json_doc    entry = json_doc::object();
    entry.set("uri", json_doc::string(it->uri));
    entry.set("mimeType", json_doc::string(it->mime_type));
    entry.set("text", json_doc::string(text));
    json_doc contents = json_doc::array();
    contents.push(std::move(entry));
    json_doc out = json_doc::object();
    out.set("contents", std::move(contents));
    return out;
}

[[nodiscard]] json_doc on_get(server_t const& server, json_doc const* params)
{
    if (params == nullptr || !params->is_object())
        throw rpc_fail{ rpc_error_code::invalid_params, "Invalid params" };
    json_doc name_doc = params->find("name");
    if (!name_doc.valid() || !name_doc.is_string())
        throw rpc_fail{ rpc_error_code::invalid_params, "Missing prompt name" };
    std::string name = name_doc.as_string();

    auto prompts{ server.get_prompts() };
    auto it = std::ranges::find(prompts, name, &prompt_t::name);
    if (it == prompts.end())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Unknown prompt: " + name };
    mcp_map_t args{};
    json_doc  args_doc = params->find("arguments");
    if (args_doc.valid() && args_doc.is_object())
        args = args_doc.flat_map();

    std::string text    = it->handler ? it->handler(args) : std::string{};
    json_doc    content = json_doc::object();
    content.set("type", json_doc::string("text"));
    content.set("text", json_doc::string(text));
    json_doc message = json_doc::object();
    message.set("role", json_doc::string("user"));
    message.set("content", std::move(content));
    json_doc arr = json_doc::array();
    arr.push(std::move(message));
    json_doc out = json_doc::object();
    out.set("description", json_doc::string(it->description));
    out.set("messages", std::move(arr));
    return out;
}

struct route_t
{
    std::string_view method;
    json_doc (*fn)(server_t const&, json_doc const*);
};

route_t const k_routes[] = {
    { "initialize", &on_init },     { "server/discover", &on_disc },
    { "tools/list", &on_tools },    { "tools/call", &on_call },
    { "resources/list", &on_res },  { "resources/templates/list", &on_tpl },
    { "resources/read", &on_read }, { "prompts/list", &on_prompts },
    { "prompts/get", &on_get },     { "ping", &on_ping },
};

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
    auto current_id = [&]() -> json_doc {
        if (doc.is_object()) {
            json_doc id = doc.find("id");
            if (id.valid() && !id.is_null()) {
                try {
                    return id;
                } catch (...) {
                }
            }
        }
        return json_doc::null();
    };
    try {
        if (!doc.is_object())
            return rpc_error(current_id(),
                             rpc_error_code::invalid_request,
                             "Invalid Request");

        json_doc id_doc = doc.find("id");
        if (!id_doc.valid() || id_doc.is_null())
            return {};
        json_doc id_copy = id_doc;

        json_doc method_doc = doc.find("method");
        if (!method_doc.valid() || !method_doc.is_string())
            return rpc_error(std::move(id_copy),
                             rpc_error_code::invalid_request,
                             "Invalid Request");
        std::string method = method_doc.as_string();
        if (method.starts_with("notifications/"))
            return {};

        json_doc        params_doc{};
        json_doc const* params      = nullptr;
        json_doc        params_hold = doc.find("params");
        if (params_hold.valid()) {
            params_doc = std::move(params_hold);
            params     = &params_doc;
        }

        std::string_view target{ method };
        for (auto const& r : k_routes)
            if (r.method == target)
                return rpc_result(std::move(id_copy), r.fn(*this, params));

        return rpc_error(std::move(id_copy),
                         rpc_error_code::method_not_found,
                         "Method not found");
    } catch (rpc_fail const& e) {
        return rpc_error(current_id(), e.code, e.message);
    } catch (...) {
        return rpc_error(
            current_id(), rpc_error_code::internal, "Internal error");
    }
}

} // namespace libmcp
