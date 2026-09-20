cpmaddpackage(
  NAME jsoncpp
  GITHUB_REPOSITORY open-source-parsers/jsoncpp
  GIT_TAG 1.9.8
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "JSONCPP_WITH_TESTS OFF"
    "JSONCPP_WITH_POST_BUILD_UNITTEST OFF"
    "JSONCPP_WITH_EXAMPLE OFF"
    "JSONCPP_WITH_PKGCONFIG_SUPPORT OFF"
    "JSONCPP_WITH_CMAKE_PACKAGE OFF"
    "JSONCPP_WITH_STRICT_ISO OFF"
    "JSONCPP_WITH_WARNING_AS_ERROR OFF"
    "BUILD_SHARED_LIBS OFF"
    "BUILD_STATIC_LIBS ON"
    "BUILD_OBJECT_LIBS OFF"
    # MSVC: match the project's static CRT (MultiThreaded[$Debug]).
    # Without this jsoncpp defaults to the dynamic CRT and every consumer
    # fails with LNK2038 RuntimeLibrary MD_DynamicRelease vs MT_StaticRelease.
    "JSONCPP_STATIC_WINDOWS_RUNTIME ON"
)

# Android x86 shared link needs PIC objects in the static archive:
# without this, ld.lld fails with R_386_PC32 / R_386_GOTOFF against
# libjsoncpp.a when linking libexample_parse_schema.so.
if(TARGET jsoncpp_static)
  set_target_properties(jsoncpp_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
