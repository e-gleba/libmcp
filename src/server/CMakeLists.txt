cmake_minimum_required(VERSION 4.3)

file(
    GLOB_RECURSE
    mcp_handler_sources
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

file(
    GLOB_RECURSE
    mcp_handler_public_headers
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.hxx")

list(
    FILTER
    mcp_handler_public_headers
    EXCLUDE
    REGEX
    ".*/detail/.*")

file(
    GLOB_RECURSE
    mcp_handler_private_headers
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/detail/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/detail/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/detail/*.hxx")

add_library(mcp_handler STATIC)
add_library(mcp_handler::mcp_handler ALIAS mcp_handler)

target_sources(
    mcp_handler
    PRIVATE ${mcp_handler_sources} ${mcp_handler_private_headers}
    PUBLIC FILE_SET
           HEADERS
           BASE_DIRS
           "${CMAKE_CURRENT_LIST_DIR}"
           FILES
           ${mcp_handler_public_headers})

target_compile_features(mcp_handler PUBLIC cxx_std_20)

set_target_properties(
    mcp_handler
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_STANDARD_REQUIRED ON
               POSITION_INDEPENDENT_CODE ON
               OUTPUT_NAME mcp_handler
               MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_include_directories(
    mcp_handler PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

option(MCP_HANDLER_USE_JSONCPP "Use jsoncpp backend instead of nlohmann_json"
       ON)

# Build-only deps: BUILD_INTERFACE keeps headers/flags for this build while
# stripping every usage requirement from the install interface, so
# install(EXPORT) never demands an export set and nothing leaks to consumers.
target_link_libraries(
    mcp_handler
    PRIVATE
        $<BUILD_INTERFACE:mcp::schema>
        $<BUILD_INTERFACE:warnings>
        $<$<BOOL:${MCP_HANDLER_USE_JSONCPP}>:$<BUILD_INTERFACE:jsoncpp_static>>
        $<$<NOT:$<BOOL:${MCP_HANDLER_USE_JSONCPP}>>:$<BUILD_INTERFACE:nlohmann_json::nlohmann_json>>
)

if(MCP_HANDLER_USE_JSONCPP)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/detail/jsoncpp.cpp
                                PROPERTIES HEADER_FILE_ONLY FALSE)
    set_source_files_properties(
        ${CMAKE_CURRENT_LIST_DIR}/detail/nlohmann_json.cpp
        PROPERTIES HEADER_FILE_ONLY TRUE)
else()
    set_source_files_properties(
        ${CMAKE_CURRENT_LIST_DIR}/detail/nlohmann_json.cpp
        PROPERTIES HEADER_FILE_ONLY FALSE)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/detail/jsoncpp.cpp
                                PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# MinGW cross: jsoncpp_static's usage include dir does not propagate through
# the BUILD_INTERFACE-wrapped link on this generator (observed as missing
# jsoncpp/json/json.h). PRIVATE here, not PUBLIC: consumers never see it,
# install(EXPORT) stays clean, and only this TU's compile gets the -I.
if(MCP_HANDLER_USE_JSONCPP)
    get_target_property(ct_jsoncpp_inc jsoncpp_static
                        INTERFACE_INCLUDE_DIRECTORIES)
    if(ct_jsoncpp_inc)
        target_include_directories(mcp_handler PRIVATE ${ct_jsoncpp_inc})
    endif()
endif()

target_link_options(
    mcp_handler
    PUBLIC
    "$<$<AND:$<CXX_COMPILER_FRONTEND_VARIANT:GNU>,$<NOT:$<CXX_COMPILER_ID:AppleClang>>>:-static-libgcc;-static-libstdc++>"
)

install(
    TARGETS mcp_handler
    EXPORT mcp_handlerTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            FILE_SET HEADERS
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    INCLUDES
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(
    EXPORT mcp_handlerTargets
    NAMESPACE mcp_handler::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcp_handler")
