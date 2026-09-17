#include <2026-07-28/schema.hpp>

#include <cassert>
#include <iostream>
#include <string>

namespace tb {
/// Throws std::exception on invalid JSON or schema mismatch.
[[nodiscard]] inline nlohmann::json roundtrip(const nlohmann::json& value)
{
    const std::string    text{ value.dump() };
    const nlohmann::json parsed{ nlohmann::json::parse(text) };
    return parsed;
}
} // namespace tb

int main()
{
    nlohmann::json request{
        { "jsonrpc", "2.0" },
        { "id", 1 },
        { "method", "initialize" },
        { "params",
          { { "protocolVersion", "2026-07-28" },
            { "capabilities", nlohmann::json::object() },
            { "clientInfo",
              { { "name", "check" }, { "version", "0.1.0" } } } } },
    };

    nlohmann::json back{ tb::roundtrip(request) };
    assert(back.at("jsonrpc") == "2.0");
    assert(back.at("id") == 1);
    assert(back.at("method") == "initialize");
    assert(back.at("params").at("protocolVersion") == "2026-07-28");

    std::cout << back.dump(2) << '\n';
}
