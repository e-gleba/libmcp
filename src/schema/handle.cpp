#include "handle.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

namespace libmcp {
[[nodiscard]] inline std::map<std::string, std::string> parse_to_map(
    std::string_view text)
{
    std::map<std::string, std::string> out{};
    try {
        nlohmann::json const j{ nlohmann::json::parse(text) };
        if (!j.is_object()) {
            out.emplace("error", "not_object");
            return out;
        }
        for (auto const& [k, v] : j.items()) {
            if (v.is_string()) {
                out.emplace(k, v.get<std::string>());
            } else if (v.is_null()) {
                out.emplace(k, std::string{});
            } else {
                out.emplace(k, v.dump());
            }
        }
    } catch (...) {
        out.emplace("error", "parse_failed");
        out.emplace("raw", std::string{ text });
    }
    return out;
}

[[nodiscard]] inline std::string convert_to_string(
    std::map<std::string, std::string> const& m)
{
    nlohmann::json const j{ m };
    return j.dump();
}

std::string handle_request(std::string const& raw)
{
    std::map<std::string, std::string> const req{ parse_to_map(raw) };
    std::map<std::string, std::string>       res{};
    if (req.contains("error")) {
        res.emplace("resut", "error");
        res.emplace("detail", req.at("error"));
        return convert_to_string(res);
    }
    std::string key{};
    for (auto const* cand : { "resut", "result", "cmd", "req" }) {
        auto const it{ req.find(cand) };
        if (it != req.end()) {
            key = it->first;
            res.emplace("resut", it->second);
            break;
        }
    }
    if (key.empty()) {
        res.emplace("resut", raw);
    }
    return convert_to_string(res);
}
} // namespace libmcp