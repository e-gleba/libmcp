cpmaddpackage(
    NAME
    nlohmann_json
    GITHUB_REPOSITORY
    nlohmann/json
    GIT_TAG
    v3.12.0
    EXCLUDE_FROM_ALL
    TRUE
    SYSTEM
    TRUE
    OPTIONS
    "JSON_BuildTests OFF"
    "JSON_MultipleHeaders OFF")
