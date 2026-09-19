#include "handle.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <string_view>

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

constexpr const char* internal_fallback() noexcept
{
    return R"({"jsonrpc":"2.0","id":null,"error":{"code":-32603,"message":"Internal error"}})";
}

[[nodiscard]] inline mcp_map_t map_from_json(nlohmann::json const& j)
{
    mcp_map_t out{};
    for (auto const& [k, v] : j.items()) {
        if (v.is_string()) {
            out.emplace(k, v.get<std::string>());
        } else if (v.is_null()) {
            out.emplace(k, std::string{});
        } else {
            out.emplace(k, v.dump());
        }
    }
    return out;
}

[[nodiscard]] inline mcp_map_t parse_to_map(std::string_view text)
{
    try {
        nlohmann::json const j = nlohmann::json::parse(text);
        if (!j.is_object()) {
            return mcp_map_t{ { "error", "not_object" } };
        }
        return map_from_json(j);
    } catch (...) {
        return mcp_map_t{ { "error", "parse_failed" },
                          { "raw", std::string{ text } } };
    }
}

[[nodiscard]] inline std::string convert_to_string(mcp_map_t const& m)
{
    nlohmann::json const j = m;
    return j.dump();
}

namespace {

[[nodiscard]] std::string create_jrpc_message(nlohmann::json   id,
                                              std::string_view key,
                                              nlohmann::json   payload)
{
    try {
        nlohmann::json out      = nlohmann::json::object();
        out["jsonrpc"]          = "2.0";
        out["id"]               = std::move(id);
        out[std::string{ key }] = std::move(payload);
        return out.dump();
    } catch (...) {
        return internal_fallback();
    }
}

[[nodiscard]] nlohmann::json make_error_payload(rpc_error_code   code,
                                                std::string_view message)
{
    nlohmann::json payload = nlohmann::json::object();
    payload["code"]        = static_cast<std::int32_t>(code);
    payload["message"]     = std::string{ message };
    return payload;
}

[[nodiscard]] std::string rpc_error(nlohmann::json   id,
                                    rpc_error_code   code,
                                    std::string_view message)
{
    return create_jrpc_message(
        std::move(id), "error", make_error_payload(code, message));
}

[[nodiscard]] std::string negotiate_version(server_t const&  server,
                                            std::string_view client)
{
    for (auto const& v : server.protocol_version) {
        if (!v.empty() && v == client) {
            return v;
        }
    }
    for (auto it = server.protocol_version.rbegin();
         it != server.protocol_version.rend();
         ++it) {
        if (!it->empty()) {
            return *it;
        }
    }
    return std::string{ "2026-07-28" };
}

[[nodiscard]] nlohmann::json supported_versions_json(server_t const& server)
{
    nlohmann::json arr = nlohmann::json::array();
    for (auto const& v : server.protocol_version) {
        if (!v.empty()) {
            arr.push_back(v);
        }
    }
    if (arr.empty()) {
        arr.push_back("2026-07-28");
    }
    return arr;
}

[[nodiscard]] nlohmann::json initialize_payload(server_t const&  server,
                                                std::string_view client_version)
{
    nlohmann::json payload     = nlohmann::json::object();
    payload["protocolVersion"] = negotiate_version(server, client_version);
    payload["capabilities"] =
        nlohmann::json{ { "tools", nlohmann::json::object() },
                        { "resources", nlohmann::json::object() },
                        { "prompts", nlohmann::json::object() } };
    payload["serverInfo"] = nlohmann::json{ { "name", server.name },
                                            { "version", server.version } };
    return payload;
}

[[nodiscard]] nlohmann::json discover_payload(server_t const& server)
{
    return nlohmann::json{
        { "resultType", "complete" },
        { "supportedVersions", supported_versions_json(server) },
        { "capabilities",
          { { "tools", nlohmann::json::object() },
            { "resources", nlohmann::json::object() } } },
        { "_meta",
          { { "io.modelcontextprotocol/serverInfo",
              { { "name", server.name }, { "version", server.version } } } } },
        { "instructions", server.instructions },
        { "ttlMs", server.ttl_ms },
        { "cacheScope", server.cache_scope }
    };
}

[[nodiscard]] nlohmann::json tools_list_payload(server_t const& server)
{
    auto           tools{ server.get_tools() };
    nlohmann::json arr = nlohmann::json::array();
    for (auto const& vec : tools) {
        nlohmann::json schema = nlohmann::json::object();
        try {
            nlohmann::json parsed = nlohmann::json::parse(vec.input_schema);
            if (parsed.is_object()) {
                schema = std::move(parsed);
            } else {
                schema = nlohmann::json{ { "type", "object" } };
            }
        } catch (...) {
            schema = nlohmann::json::object();
        }
        nlohmann::json item = nlohmann::json::object();
        item["name"]        = vec.name;
        item["description"] = vec.description;
        item["inputSchema"] = std::move(schema);
        arr.push_back(std::move(item));
    }
    nlohmann::json out = nlohmann::json::object();
    out["tools"]       = std::move(arr);
    return out;
}

} // namespace

std::string server_t::handle_request(std::string const& raw) const
{
    nlohmann::json doc = nlohmann::json(nullptr);
    try {
        doc = nlohmann::json::parse(raw);
    } catch (...) {
        return create_jrpc_message(
            nlohmann::json(nullptr),
            "error",
            make_error_payload(rpc_error_code::parse, "Parse error"));
    }
    try {
        if (!doc.is_object()) {
            return create_jrpc_message(
                nlohmann::json(nullptr),
                "error",
                make_error_payload(rpc_error_code::invalid_request,
                                   "Invalid Request"));
        }

        auto const it_method{ doc.find("method") };
        auto const it_id{ doc.find("id") };
        if (it_id == doc.end() || it_id->is_null()) {
            return {};
        }
        nlohmann::json id_copy = *it_id;
        if (it_method == doc.end() || !it_method->is_string()) {
            return create_jrpc_message(
                std::move(id_copy),
                "error",
                make_error_payload(rpc_error_code::invalid_request,
                                   "Invalid Request"));
        }
        std::string const method = it_method->get<std::string>();
        if (method == "initialize") {
            std::string client_version{};
            if (auto it_params{ doc.find("params") };
                it_params != doc.end() && it_params->is_object()) {
                if (auto it_ver{ it_params->find("protocolVersion") };
                    it_ver != it_params->end() && it_ver->is_string()) {
                    client_version = it_ver->get<std::string>();
                }
            }
            return create_jrpc_message(
                std::move(id_copy),
                "result",
                initialize_payload(*this, client_version));
        }
        if (method == "notifications/initialized") {
            return {};
        }
        if (method == "server/discover") {
            return create_jrpc_message(
                std::move(id_copy), "result", discover_payload(*this));
        }
        if (method == "tools/list") {
            return create_jrpc_message(
                std::move(id_copy), "result", tools_list_payload(*this));
        }
        return create_jrpc_message(
            std::move(id_copy),
            "error",
            make_error_payload(rpc_error_code::method_not_found,
                               "Method not found"));
    } catch (...) {
        return internal_fallback();
    }
}

} // namespace libmcp
