cmake_minimum_required(VERSION 4.3)

option(${PROJECT_NAME}_BUILD_EXAMPLES "Build examples" ON)
if(NOT ${PROJECT_NAME}_BUILD_EXAMPLES)
    return()
endif()

function(mcp_example name src)
    cmake_parse_arguments(
        arg
        ""
        ""
        "LINK"
        ${ARGN})

    if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        add_library("example_${name}" SHARED "${src}")
    else()
        add_executable("example_${name}" "${src}")
    endif()

    target_compile_features("example_${name}" PRIVATE cxx_std_23)
    set_target_properties(
        "example_${name}" PROPERTIES CXX_EXTENSIONS OFF OUTPUT_NAME
                                                        "example_${name}")

    target_link_libraries("example_${name}" PRIVATE warnings
                                                    mcp_handler::mcp_handler)
    if(arg_LINK)
        target_link_libraries("example_${name}" PRIVATE ${arg_LINK})
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        install(TARGETS "example_${name}"
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/mcp-examples")
    else()
        install(TARGETS "example_${name}"
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/mcp-examples")
    endif()
endfunction()

mcp_example(parse_schema 01_mcp_envelope.cpp)
