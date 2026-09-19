#include "handle.hpp"

#include <nlohmann/json.hpp>

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
using json      = nlohmann::json;

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

[[nodiscard]] inline mcp_map_t map_from_json(json const& j)
{
    mcp_map_t out{};
    for (auto const& [k, v] : j.items())
        out.emplace(k,
                    v.is_string() ? v.get<std::string>()
                    : v.is_null() ? std::string{}
                                  : v.dump());
    return out;
}

[[nodiscard]] inline mcp_map_t parse_to_map(std::string_view text)
{
    try {
        json const j = json::parse(text);
        if (!j.is_object())
            return mcp_map_t{ { "error", "not_object" } };
        return map_from_json(j);
    } catch (...) {
        return mcp_map_t{ { "error", "parse_failed" },
                          { "raw", std::string{ text } } };
    }
}

[[nodiscard]] inline std::string convert_to_string(mcp_map_t const& m)
{
    json const j = m;
    return j.dump();
}

namespace {

// Single envelope builder.
[[nodiscard]] std::string create_jrpc_message(json             id,
                                              std::string_view key,
                                              json             payload)
{
    try {
        json out                = json::object();
        out["jsonrpc"]          = "2.0";
        out["id"]               = std::move(id);
        out[std::string{ key }] = std::move(payload);
        return out.dump();
    } catch (...) {
        return internal_fallback();
    }
}

[[nodiscard]] json make_error_payload(rpc_error_code   code,
                                      std::string_view message)
{
    json out       = json::object();
    out["code"]    = static_cast<std::int32_t>(code);
    out["message"] = std::string{ message };
    return out;
}

[[nodiscard]] std::string rpc_error(json             id,
                                    rpc_error_code   code,
                                    std::string_view message)
{
    return create_jrpc_message(
        std::move(id), "error", make_error_payload(code, message));
}

[[nodiscard]] std::string rpc_result(json id, json payload)
{
    return create_jrpc_message(std::move(id), "result", std::move(payload));
}

// Handlers stay pure: return payload. Errors throw, dispatcher maps to RPC.
struct rpc_fail
{
    rpc_error_code code;
    std::string    message;
};

// Stepanov: one predicate, two projections via views::filter.
[[nodiscard]] constexpr bool is_uri_template(std::string_view uri) noexcept
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

[[nodiscard]] json supported_versions_json(server_t const& server)
{
    json arr = json::array();
    for (auto const& v :
         server.protocol_version | std::views::filter([](std::string const& v) {
             return !v.empty();
         }))
        arr.push_back(v);
    if (arr.empty())
        arr.push_back(std::string{ k_fallback_version });
    return arr;
}

[[nodiscard]] json capabilities_json()
{
    json out         = json::object();
    out["tools"]     = json::object();
    out["resources"] = json::object();
    out["prompts"]   = json::object();
    return out;
}

[[nodiscard]] json server_info_json(server_t const& server)
{
    json out       = json::object();
    out["name"]    = server.name;
    out["version"] = server.version;
    return out;
}

[[nodiscard]] json parse_schema(std::string const& s)
{
    try {
        json parsed = json::parse(s);
        if (parsed.is_object())
            return parsed;
    } catch (...) {
    }
    return json{ { "type", "object" } };
}

[[nodiscard]] json initialize_payload(server_t const&  server,
                                      std::string_view client_version)
{
    json out               = json::object();
    out["protocolVersion"] = negotiate_version(server, client_version);
    out["capabilities"]    = capabilities_json();
    out["serverInfo"]      = server_info_json(server);
    return out;
}

[[nodiscard]] json discover_payload(server_t const& server)
{
    json meta                                  = json::object();
    meta["io.modelcontextprotocol/serverInfo"] = server_info_json(server);
    json out                                   = json::object();
    out["resultType"]                          = "complete";
    out["supportedVersions"] = supported_versions_json(server);
    out["capabilities"]      = capabilities_json();
    out["_meta"]             = std::move(meta);
    out["instructions"]      = server.instructions;
    out["ttlMs"]             = server.ttl_ms;
    out["cacheScope"]        = server.cache_scope;
    return out;
}

[[nodiscard]] json tools_list_payload(server_t const& server)
{
    auto tools{ server.get_tools() };
    json arr = json::array();
    for (auto const& t : tools) {
        json item           = json::object();
        item["name"]        = t.name;
        item["description"] = t.description;
        item["inputSchema"] = parse_schema(t.input_schema);
        arr.push_back(std::move(item));
    }
    json out     = json::object();
    out["tools"] = std::move(arr);
    return out;
}

[[nodiscard]] json resources_list_payload(server_t const& server)
{
    auto resources{ server.get_resources() };
    json arr = json::array();
    for (auto const& r :
         resources | std::views::filter([](resource_t const& r) {
             return !is_uri_template(r.uri);
         })) {
        json item           = json::object();
        item["uri"]         = r.uri;
        item["name"]        = r.name;
        item["title"]       = r.title;
        item["description"] = r.description;
        item["mimeType"]    = r.mime_type;
        arr.push_back(std::move(item));
    }
    json out         = json::object();
    out["resources"] = std::move(arr);
    return out;
}

[[nodiscard]] json resource_templates_list_payload(server_t const& server)
{
    auto resources{ server.get_resources() };
    json arr = json::array();
    for (auto const& r :
         resources | std::views::filter([](resource_t const& r) {
             return is_uri_template(r.uri);
         })) {
        json item           = json::object();
        item["uriTemplate"] = r.uri;
        item["name"]        = r.name;
        item["title"]       = r.title;
        item["description"] = r.description;
        item["mimeType"]    = r.mime_type;
        arr.push_back(std::move(item));
    }
    json out                 = json::object();
    out["resourceTemplates"] = std::move(arr);
    return out;
}

[[nodiscard]] json prompts_list_payload(server_t const& server)
{
    auto prompts{ server.get_prompts() };
    json arr = json::array();
    for (auto const& p : prompts) {
        json args = json::array();
        for (auto const& a : p.arguments) {
            json item           = json::object();
            item["name"]        = a.name;
            item["title"]       = a.title;
            item["description"] = a.description;
            item["required"]    = a.required;
            args.push_back(std::move(item));
        }
        json item           = json::object();
        item["name"]        = p.name;
        item["title"]       = p.title;
        item["description"] = p.description;
        item["arguments"]   = std::move(args);
        arr.push_back(std::move(item));
    }
    json out       = json::object();
    out["prompts"] = std::move(arr);
    return out;
}

[[nodiscard]] json ping_payload(server_t const&, json const*)
{
    return json::object();
}

[[nodiscard]] json on_initialize(server_t const& server, json const* params)
{
    std::string      buf{};
    std::string_view client{};
    if (params != nullptr && params->is_object()) {
        if (auto it = params->find("protocolVersion");
            it != params->end() && it->is_string()) {
            buf    = it->get<std::string>();
            client = buf;
        }
    }
    return initialize_payload(server, client);
}

[[nodiscard]] json on_discover(server_t const& server, json const*)
{
    return discover_payload(server);
}

[[nodiscard]] json on_tools_list(server_t const& server, json const*)
{
    return tools_list_payload(server);
}

[[nodiscard]] json on_resources_list(server_t const& server, json const*)
{
    return resources_list_payload(server);
}

[[nodiscard]] json on_resource_templates_list(server_t const& server,
                                              json const*)
{
    return resource_templates_list_payload(server);
}

[[nodiscard]] json on_prompts_list(server_t const& server, json const*)
{
    return prompts_list_payload(server);
}

[[nodiscard]] json on_ping(server_t const& server, json const* params)
{
    return ping_payload(server, params);
}

[[nodiscard]] json on_tool_call(server_t const& server, json const* params)
{
    if (params == nullptr || !params->is_object())
        throw rpc_fail{ rpc_error_code::invalid_params, "Invalid params" };
    auto it_name = params->find("name");
    if (it_name == params->end() || !it_name->is_string())
        throw rpc_fail{ rpc_error_code::invalid_params, "Missing tool name" };
    std::string const& name = it_name->get_ref<json::string_t const&>();

    std::string args_json{ "{}" };
    if (auto it = params->find("arguments"); it != params->end())
        args_json = it->dump();

    auto tools{ server.get_tools() };
    auto it = std::ranges::find(tools, name, &tool_t::name);
    if (it == tools.end())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Unknown tool: " + name };
    try {
        std::string text = it->handler ? it->handler(args_json) : std::string{};
        json        content = json::array();
        json        entry   = json::object();
        entry["type"]       = "text";
        entry["text"]       = std::move(text);
        content.push_back(std::move(entry));
        json out       = json::object();
        out["content"] = std::move(content);
        out["isError"] = false;
        return out;
    } catch (std::exception const& e) {
        json content  = json::array();
        json entry    = json::object();
        entry["type"] = "text";
        entry["text"] = e.what();
        content.push_back(std::move(entry));
        json out       = json::object();
        out["content"] = std::move(content);
        out["isError"] = true;
        return out;
    }
}

[[nodiscard]] json on_resource_read(server_t const& server, json const* params)
{
    if (params == nullptr || !params->is_object())
        throw rpc_fail{ rpc_error_code::invalid_params, "Invalid params" };
    auto it_uri = params->find("uri");
    if (it_uri == params->end() || !it_uri->is_string())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Missing resource uri" };
    std::string const& uri = it_uri->get_ref<json::string_t const&>();

    auto resources{ server.get_resources() };
    auto it = std::ranges::find(resources, uri, &resource_t::uri);
    if (it == resources.end())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Unknown resource: " + uri };
    std::string text  = it->handler ? it->handler(uri) : std::string{};
    json        entry = json::object();
    entry["uri"]      = it->uri;
    entry["mimeType"] = it->mime_type;
    entry["text"]     = std::move(text);
    json contents     = json::array();
    contents.push_back(std::move(entry));
    json out        = json::object();
    out["contents"] = std::move(contents);
    return out;
}

[[nodiscard]] json on_prompt_get(server_t const& server, json const* params)
{
    if (params == nullptr || !params->is_object())
        throw rpc_fail{ rpc_error_code::invalid_params, "Invalid params" };
    auto it_name = params->find("name");
    if (it_name == params->end() || !it_name->is_string())
        throw rpc_fail{ rpc_error_code::invalid_params, "Missing prompt name" };
    std::string const& name = it_name->get_ref<json::string_t const&>();

    auto prompts{ server.get_prompts() };
    auto it = std::ranges::find(prompts, name, &prompt_t::name);
    if (it == prompts.end())
        throw rpc_fail{ rpc_error_code::invalid_params,
                        "Unknown prompt: " + name };
    mcp_map_t args{};
    if (auto it_args = params->find("arguments");
        it_args != params->end() && it_args->is_object())
        args = map_from_json(*it_args);

    std::string text    = it->handler ? it->handler(args) : std::string{};
    json        content = json::object();
    content["type"]     = "text";
    content["text"]     = std::move(text);
    json message        = json::object();
    message["role"]     = "user";
    message["content"]  = std::move(content);
    json messages       = json::array();
    messages.push_back(std::move(message));
    json out           = json::object();
    out["description"] = it->description;
    out["messages"]    = std::move(messages);
    return out;
}

struct route_t
{
    std::string_view method;
    json (*fn)(server_t const&, json const*);
};

const route_t k_routes[] = {
    { "initialize", &on_initialize },
    { "server/discover", &on_discover },
    { "tools/list", &on_tools_list },
    { "tools/call", &on_tool_call },
    { "resources/list", &on_resources_list },
    { "resources/templates/list", &on_resource_templates_list },
    { "resources/read", &on_resource_read },
    { "prompts/list", &on_prompts_list },
    { "prompts/get", &on_prompt_get },
    { "ping", &on_ping },
};

} // namespace

std::string server_t::handle_request(std::string const& raw) const
{
    json doc = json(nullptr);
    try {
        doc = json::parse(raw);
    } catch (...) {
        return rpc_error(json(nullptr), rpc_error_code::parse, "Parse error");
    }
    auto current_id = [&]() -> json {
        if (doc.is_object()) {
            if (auto it = doc.find("id"); it != doc.end() && !it->is_null()) {
                try {
                    return *it;
                } catch (...) {
                }
            }
        }
        return json(nullptr);
    };
    try {
        if (!doc.is_object())
            return rpc_error(current_id(),
                             rpc_error_code::invalid_request,
                             "Invalid Request");

        auto const it_id = doc.find("id");
        if (it_id == doc.end() || it_id->is_null())
            return {};
        json id_copy = *it_id;

        auto const it_method = doc.find("method");
        if (it_method == doc.end() || !it_method->is_string())
            return rpc_error(std::move(id_copy),
                             rpc_error_code::invalid_request,
                             "Invalid Request");

        std::string const& method = it_method->get_ref<json::string_t const&>();
        if (method.starts_with("notifications/"))
            return {};

        json const* params = nullptr;
        if (auto it = doc.find("params"); it != doc.end())
            params = &*it;

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
