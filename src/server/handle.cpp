#include "handle.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <map>
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
        nlohmann::json const j{ nlohmann::json::parse(text) };
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
    nlohmann::json const j{ m };
    return j.dump();
}

namespace {

[[nodiscard]] std::string create_jrpc_message(nlohmann::json   id,
                                              std::string_view key,
                                              nlohmann::json   payload)
{
    try {
        const nlohmann::json out{
            { { "jsonrpc", "2.0" }, { "id", id }, { key, std::move(payload) } }
        };
        return out.dump();
    } catch (...) {
        return internal_fallback();
    }
}

[[nodiscard]] std::string rpc_error(nlohmann::json   id,
                                    rpc_error_code   code,
                                    std::string_view message)
{
    nlohmann::json const payload{ { "code", static_cast<std::int32_t>(code) },
                                  { "message", std::string{ message } } };
    return create_jrpc_message(std::move(id), "error", payload);
}

// Returns payload only; handle_request packs the header.
[[nodiscard]] nlohmann::json discover_payload(server_t const& server)
{
    return nlohmann::json{
        { "resultType", "complete" },
        { "supportedVersions", { server.protocol_version } },
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

} // namespace

std::string server_t::handle_request(std::string const& raw) const
{
    nlohmann::json doc{};
    try {
        doc = nlohmann::json::parse(raw);
    } catch (...) {
        return create_jrpc_message(
            nlohmann::json{ nullptr },
            "error",
            nlohmann::json{ { "code", rpc_error_code::parse },
                            { "message", "Parse error" } });
    }
    try {
        if (!doc.is_object()) {
            return create_jrpc_message(
                nlohmann::json{ nullptr },
                "error",
                nlohmann::json{ { "code", rpc_error_code::invalid_request },
                                { "message", "Invalid Request" } });
        }

        auto const it_method{ doc.find("method") };

        auto const it_id{ doc.find("id") };
        if (it_id == doc.end() || it_id->is_null()) {
            return {};
        }
        nlohmann::json    id_copy{ *it_id };
        std::string const method{ it_method->get<std::string>() };
        if (method == "server/discover") {
            // Header packed once here, never inside discover_payload.
            return create_jrpc_message(
                std::move(id_copy), "result", discover_payload(*this));
        }
        return create_jrpc_message(
            std::move(id_copy),
            "error",
            nlohmann::json{ { "code", rpc_error_code::method_not_found },
                            { "message", "Method not found" } });
    } catch (...) {
        return internal_fallback();
    }
}

} // namespace libmcp
