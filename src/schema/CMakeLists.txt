include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CheckIncludeFileCXX)

# Regeneration needs network (fetch schema.ts) + Node/npm (quicktype).
# Default OFF: use the committed pre-generated header, so normal builds
# (incl. MSVC / MinGW CI without Node) never spawn npm at configure time.
option(MCP_SCHEMA_REGENERATE
       "Regenerate MCP schema header from upstream schema.ts via npm quicktype"
       OFF)

set(mcp_date
    "2026-07-28"
    CACHE STRING "MCP schema version")
set(mcp_hpp
    "${CMAKE_CURRENT_LIST_DIR}/${mcp_date}/schema.hpp"
    CACHE PATH "Generated MCP header")

foreach(
    mcp_hdr IN
    ITEMS cstdint
          map
          memory
          string
          variant
          vector)
    string(TOUPPER "${mcp_hdr}" mcp_hdr_up)
    check_include_file_cxx("${mcp_hdr}" "MCP_HAS_${mcp_hdr_up}")
    if(NOT MCP_HAS_${mcp_hdr_up})
        message(FATAL_ERROR "missing <${mcp_hdr}>")
    endif()
endforeach()
unset(mcp_hdr)
unset(mcp_hdr_up)

string(
    REPLACE "-"
            "."
            mcp_version
            "${mcp_date}")

if(NOT TARGET mcp_schema)
    add_library(mcp_schema INTERFACE)
    add_library(mcp::schema ALIAS mcp_schema)
    target_sources(
        mcp_schema
        INTERFACE FILE_SET
                  HEADERS
                  BASE_DIRS
                  "${CMAKE_CURRENT_LIST_DIR}"
                  FILES
                  "${mcp_hpp}")
    target_include_directories(
        mcp_schema SYSTEM
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mcp>)
    target_compile_features(mcp_schema INTERFACE cxx_std_20)
    install(
        TARGETS mcp_schema
        EXPORT mcp_targets
        FILE_SET HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mcp")
    install(
        EXPORT mcp_targets
        NAMESPACE mcp::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcp")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/mcp-config-version.cmake"
        VERSION "${mcp_version}"
        COMPATIBILITY ExactVersion)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mcp-config.cmake"
         [[include("${CMAKE_CURRENT_LIST_DIR}/mcp_targets.cmake")]])
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcp-config.cmake"
                  "${CMAKE_CURRENT_BINARY_DIR}/mcp-config-version.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcp")
endif()

# Fast path first: committed header, no network/npm.
if(NOT MCP_SCHEMA_REGENERATE)
    if(NOT EXISTS "${mcp_hpp}")
        message(
            FATAL_ERROR
                "pre-generated MCP header missing: ${mcp_hpp}; re-configure with -DMCP_SCHEMA_REGENERATE=ON (needs network + Node/npm)"
        )
    endif()
    message(STATUS "using pre-generated MCP header: ${mcp_hpp}")
    return()
endif()

# --- regeneration below (needs network + Node/npm) ---

set(mcp_url
    "https://raw.githubusercontent.com/modelcontextprotocol/modelcontextprotocol/main/schema/${mcp_date}/schema.ts"
)

set(mcp_tmp_dir "${CMAKE_CURRENT_BINARY_DIR}/${mcp_date}")
set(mcp_ts "${mcp_tmp_dir}/schema.ts")
set(mcp_full "${mcp_tmp_dir}/schema.full.hpp")
set(mcp_body_tmp "${mcp_tmp_dir}/schema.body.hpp")

string(
    REPLACE "-"
            "_"
            mcp_ns_suffix
            "${mcp_date}")
set(mcp_ns "mcp::v${mcp_ns_suffix}")

# Prefer npm.cmd on Windows: the extensionless
# 'C:/Program Files/nodejs/npm' is a POSIX shell script, and
# CreateProcess fails on it with
# 'inappropriate file type or format'. A .cmd still needs cmd /c.
find_program(npm_executable NAMES npm.cmd npm REQUIRED)

cmake_path(
    GET
    mcp_hpp
    PARENT_PATH
    mcp_out_dir)
file(MAKE_DIRECTORY "${mcp_tmp_dir}" "${mcp_out_dir}")

message(CHECK_START "fetch MCP schema ${mcp_date}")
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
message(CHECK_PASS "done")

message(CHECK_START "quicktype TS->C++ schema-only")
# Host check: cross Linux -> Windows must run the host's npm, not cmd.
if(CMAKE_HOST_WIN32)
    execute_process(
        COMMAND
            cmd /c ${npm_executable} exec --yes -- quicktype --src-lang
            typescript --lang c++ --code-format with-struct --just-types
            --type-style original-case --member-style original-case
            --enumerator-style original-case --no-boost --source-style
            single-source --out "${mcp_full}" "${mcp_ts}"
        WORKING_DIRECTORY
            "${mcp_tmp_dir}"
            COMMAND_ECHO
            STDOUT
            COMMAND_ERROR_IS_FATAL
            ANY)
else()
    execute_process(
        COMMAND
            "${npm_executable}" exec --yes -- quicktype --src-lang typescript
            --lang c++ --code-format with-struct --just-types --type-style
            original-case --member-style original-case --enumerator-style
            original-case --no-boost --source-style single-source --out
            "${mcp_full}" "${mcp_ts}"
        WORKING_DIRECTORY
            "${mcp_tmp_dir}"
            COMMAND_ECHO
            STDOUT
            COMMAND_ERROR_IS_FATAL
            ANY)
endif()
message(CHECK_PASS "done")

function(mcp_strip_optional line_var)
    set(text "${${line_var}}")
    foreach(prefix IN ITEMS "std::optional" "boost::optional")
        string(LENGTH "${prefix}<" prefix_len)
        while(TRUE)
            string(FIND "${text}" "${prefix}<" pos)
            if(pos EQUAL -1)
                break()
            endif()
            string(LENGTH "${text}" text_len)
            math(EXPR inner_start "${pos} + ${prefix_len}")
            set(depth 1)
            set(i ${inner_start})
            while(i LESS text_len AND depth GREATER 0)
                string(
                    SUBSTRING "${text}"
                              ${i}
                              1
                              ch)
                if(ch STREQUAL "<")
                    math(EXPR depth "${depth} + 1")
                elseif(ch STREQUAL ">")
                    math(EXPR depth "${depth} - 1")
                endif()
                math(EXPR i "${i} + 1")
            endwhile()
            if(depth GREATER 0)
                break()
            endif()
            math(EXPR inner_len "${i} - 1 - ${inner_start}")
            string(
                SUBSTRING "${text}"
                          ${inner_start}
                          ${inner_len}
                          inner)
            string(
                SUBSTRING "${text}"
                          0
                          ${pos}
                          before)
            math(EXPR after_len "${text_len} - ${i}")
            if(after_len GREATER 0)
                string(
                    SUBSTRING "${text}"
                              ${i}
                              ${after_len}
                              after)
            else()
                set(after "")
            endif()
            set(text "${before}${inner}${after}")
        endwhile()
    endforeach()
    set(${line_var}
        "${text}"
        PARENT_SCOPE)
endfunction()

file(STRINGS "${mcp_full}" mcp_lines)
set(mcp_body "")
set(in_skip OFF)
set(in_json_alias OFF)
set(in_ns OFF)
foreach(line IN LISTS mcp_lines)
    if(line MATCHES "#ifndef NLOHMANN_")
        set(in_skip ON)
        continue()
    endif()
    if(in_skip)
        if(line MATCHES "#endif")
            set(in_skip OFF)
        endif()
        continue()
    endif()
    if(line MATCHES
       "using[ \t]+(JSONValue|JSONObject|JSONArray|JsonValue)[^A-Za-z0-9_]")
        set(in_json_alias ON)
        if(line MATCHES ";")
            set(in_json_alias OFF)
        endif()
        continue()
    endif()
    if(in_json_alias)
        if(line MATCHES ";")
            set(in_json_alias OFF)
        endif()
        continue()
    endif()
    if(line MATCHES "^[ \t]*namespace[ \t]+quicktype[ \t]*[{]")
        set(in_ns ON)
        continue()
    endif()
    if(in_ns)
        if(line MATCHES "^[ \t]*[}][ \t]*$")
            set(in_ns OFF)
        endif()
        continue()
    endif()
    if(line MATCHES "^(#pragma once|#include|using nlohmann::json;)")
        continue()
    endif()
    string(
        REPLACE "nlohmann::json"
                "JsonValue"
                line
                "${line}")
    string(
        REPLACE "JSONValue"
                "JsonValue"
                line
                "${line}")
    mcp_strip_optional(line)
    string(
        REGEX
        REPLACE " *= *(std::nullopt|boost::none)"
                ""
                line
                "${line}")
    if(line MATCHES
       "^[ \t]*(struct|class)[ \t]+[A-Za-z_][A-Za-z0-9_]*[ \t]*[{:]"
       AND NOT
           line
           MATCHES
           "final[ \t]*[{:]")
        string(
            REGEX
            REPLACE
                "^([ \t]*(struct|class)[ \t]+[A-Za-z_][A-Za-z0-9_]*)[ \t]*([{:])"
                "\\1 final \\3"
                line
                "${line}")
    endif()
    set(mcp_body "${mcp_body}${line}\n")
endforeach()

file(
    CONFIGURE
    OUTPUT
    "${mcp_body_tmp}"
    CONTENT
    [[#pragma once
// generated from MCP schema @mcp_date@, do not edit
// See https://modelcontextprotocol.io/docs/2026-07-28/getting-started/intro
// See https://github.com/modelcontextprotocol/inspector/blob/main/clients/cli/README.md
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace @mcp_ns@ {

struct JsonValue final {
    using array = std::vector<JsonValue>;
    using object = std::map<std::string, JsonValue>;
    std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, array, object> data;
};
]]
    @ONLY)
file(APPEND "${mcp_body_tmp}" "${mcp_body}")
file(APPEND "${mcp_body_tmp}" "} // namespace ${mcp_ns}\n")
file(
    COPY_FILE
    "${mcp_body_tmp}"
    "${mcp_hpp}"
    ONLY_IF_DIFFERENT)
file(
    REMOVE
    "${mcp_ts}"
    "${mcp_full}"
    "${mcp_body_tmp}")
