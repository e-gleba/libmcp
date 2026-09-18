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
using namespace mcp::v2026_07_28;

[[nodiscard]] ServerResult discover(const ClientRequest& cr)
{
    return {
        ._meta = { .iomodelcontextprotocolserverInfo{
            .name{ "ExampleServer" },
            .version{ "1.0.0" },
        } },
        .resultType{ "complete" },
        .cacheScope = cacheScope::cacheScope_public,
        .capabilities{
            .prompts{ .listChanged = true },
            .resources{},
            .tools{},
        },
        .instructions{ "This server provides weather and resource utilities." },
        .supportedVersions{
            "2026-07-28", "2025-11-25", "2025-06-18", "2024-11-05" },
        .ttlMs = 3600000,
    };
}

[[nodiscard]] ListToolsResult tools(const ListToolsRequest& ltr)
{
    const Tool get_weather{
        .name{ "get_weather" },
        .title{ "Weather Information Provider" },
        .description{ "Get current weather information for a location" },
        .inputSchema{
            .type = requestedSchema_type::object,
            .schema{
                R"({"type":"object","properties":{"location":{"type":"string","description":"City name or zip code"}},"required":["location"]})" } },
        .icons{ { .src{ "https://example.com/weather-icon.png" },
                  .mimeType{ "image/png" },
                  .sizes{ "48x48" } } }
    };

    const std::vector<Tool> tools{ get_weather };

    return { .resultType{ "complete" },
             .tools{ tools },
             .nextCursor{ "next-page-cursor" },
             .ttlMs{ 300000 },
             .cacheScope{ cacheScope::cacheScope_public } };
}

[[nodiscard]] ListPromptsResult prompts(const ListPromptsRequest& lpr)
{
    const Prompt code_review{
        .name{ "code_review" },
        .title{ "Request Code Review" },
        .description{
            "Asks the LLM to analyze code quality and suggest improvements" },
        .arguments{ { .name{ "code" },
                      .description{ "The code to review" },
                      .required{ true } } },
        .icons{ { .src{ "https://example.com/review-icon.svg" },
                  .mimeType{ "image/svg+xml" },
                  .sizes{ "any" } } }
    };

    const std::vector<Prompt> prompt_list{ code_review };

    return { .resultType{ "complete" },
             .prompts{ prompt_list },
             .nextCursor{ "next-page-cursor" },
             .ttlMs      = 600000,
             .cacheScope = cacheScope::cacheScope_public };
}

[[nodiscard]] ListResourcesResult resources(const ListResourcesRequest& lrr)
{
    const Resource main_rs{
        .uri{ "file:///project/src/main.rs" },
        .title{ "Rust Software Application Main File" },
        .name{ "main.rs" },
        .description{ "Primary application entry point" },
        .mimeType{ "text/x-rust" },
        .icons{ { .src{ "https://example.com/rust-file-icon.png" },
                  .mimeType{ "image/png" },
                  .sizes{ "48x48" } } }
    };

    const std::vector<Resource> resource_list{ main_rs };

    return { .resultType{ "complete" },
             .resources{ resource_list },
             .nextCursor{ "next-page-cursor" },
             .ttlMs      = 300000,
             .cacheScope = cacheScope::cacheScope_public };
}

} // namespace tb

int main()
{

    return std::cout.good() ? EXIT_SUCCESS : EXIT_FAILURE;
}
