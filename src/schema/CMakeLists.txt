include(GNUInstallDirs)

set(mcp_date
    "2026-07-28"
    CACHE STRING "MCP schema version")
set(mcp_src_dir
    "${CMAKE_CURRENT_BINARY_DIR}/mcp"
    CACHE PATH "Generated MCP dir")

set(mcp_dir "${mcp_src_dir}/${mcp_date}")
set(mcp_ts "${mcp_dir}/schema.ts")
set(mcp_hpp "${mcp_dir}/schema.hpp")
set(mcp_url
    "https://raw.githubusercontent.com/modelcontextprotocol/modelcontextprotocol/main/schema/${mcp_date}/schema.ts"
)

find_program(npm_executable NAMES npm REQUIRED)

file(MAKE_DIRECTORY "${mcp_dir}")
file(
    DOWNLOAD "${mcp_url}" "${mcp_ts}.tmp"
    TLS_VERIFY ON
    STATUS mcp_status)
list(
    GET
    mcp_status
    0
    mcp_code)
if(NOT
   mcp_code
   EQUAL
   0)
    message(FATAL_ERROR "fetch ${mcp_url}: ${mcp_status}")
endif()
file(
    COPY_FILE
    "${mcp_ts}.tmp"
    "${mcp_ts}"
    ONLY_IF_DIFFERENT)
file(REMOVE "${mcp_ts}.tmp")

execute_process(
    COMMAND
        "${npm_executable}" exec --yes -- quicktype --src-lang typescript
        --lang c++ --code-format with-struct --out "${mcp_hpp}" "${mcp_ts}"
        COMMAND_ERROR_IS_FATAL ANY)
file(READ "${mcp_hpp}" mcp_content)
string(
    REPLACE [[#include "json.hpp"]]
            [[#include <nlohmann/json.hpp>]]
            mcp_content
            "${mcp_content}")
string(
    REGEX
    REPLACE [[using JsonValue = [^;]*;]]
            [[using JsonValue = nlohmann::json;]]
            mcp_content
            "${mcp_content}")
file(WRITE "${mcp_hpp}" "${mcp_content}")

if(NOT TARGET mcp_schema)
    add_library(mcp_schema INTERFACE)
    add_library(mcp::schema ALIAS mcp_schema)
    target_sources(
        mcp_schema
        INTERFACE FILE_SET
                  HEADERS
                  BASE_DIRS
                  "${mcp_src_dir}"
                  FILES
                  "${mcp_hpp}")
    target_include_directories(
        mcp_schema SYSTEM
        INTERFACE $<BUILD_INTERFACE:${mcp_src_dir}>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mcp>)
    target_link_libraries(
        mcp_schema INTERFACE $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>)
    install(
        TARGETS mcp_schema
        EXPORT mcp_targets
        FILE_SET HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mcp")
    install(
        EXPORT mcp_targets
        NAMESPACE mcp::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcp")
    file(
        WRITE "${CMAKE_CURRENT_BINARY_DIR}/mcp-config.cmake"
        [[
include(CMakeFindDependencyMacro)
find_dependency(nlohmann_json)
include("${CMAKE_CURRENT_LIST_DIR}/mcp_targets.cmake")
if(TARGET mcp::schema AND TARGET nlohmann_json::nlohmann_json)
  target_link_libraries(mcp::schema INTERFACE nlohmann_json::nlohmann_json)
endif()
]])
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcp-config.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcp")
endif()
