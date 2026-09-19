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
               OUTPUT_NAME mcp_handler)

target_include_directories(
    mcp_handler PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
    mcp_handler PRIVATE $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
                        $<BUILD_INTERFACE:warnings>)

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
