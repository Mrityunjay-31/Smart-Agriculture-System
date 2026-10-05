cmake_minimum_required(VERSION 3.16)
project(smart_agriculture VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Dependencies ───────────────────────────────────────────────────────────────

# libcurl (system)
find_package(CURL REQUIRED)

# Threads (pthreads on Linux)
find_package(Threads REQUIRED)

# ZLIB (required by Crow compression)
find_package(ZLIB REQUIRED)

# FetchContent for header-only libs
include(FetchContent)

# Crow – lightweight C++ web framework (header-only)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
    GIT_SHALLOW    TRUE
)

# nlohmann/json – header-only JSON library
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# ASIO standalone (header-only, Crow dependency)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(crow json asio)

# ─── Main Application ──────────────────────────────────────────────────────────

set(SOURCES
    src/main.cpp
    src/SerialPort.cpp
    src/SensorData.cpp
    src/SensorParser.cpp
    src/SensorValidator.cpp
    src/SensorProcessor.cpp
    src/ThresholdConfig.cpp
    src/SensorSimulator.cpp
    src/FirebaseClient.cpp
    src/AuthManager.cpp
    src/AlertManager.cpp
    src/SessionManager.cpp
    src/WebServer.cpp
    src/DataPipeline.cpp
    src/ConfigManager.cpp
)

add_executable(smart_agriculture ${SOURCES})

target_include_directories(smart_agriculture PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${asio_SOURCE_DIR}/asio/include
)

target_link_libraries(smart_agriculture PRIVATE
    CURL::libcurl
    Threads::Threads
    ZLIB::ZLIB
    Crow::Crow
    nlohmann_json::nlohmann_json
)

target_compile_definitions(smart_agriculture PRIVATE
    CROW_ENABLE_COMPRESSION=0
    ASIO_STANDALONE
    CROW_STATIC_DIRECTORY="web/static/"
)

# Copy web assets and config to build directory
add_custom_command(TARGET smart_agriculture POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web
        $<TARGET_FILE_DIR:smart_agriculture>/web
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:smart_agriculture>/config
)

# ─── Tests ──────────────────────────────────────────────────────────────────────

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(googletest)

    set(TEST_SOURCES
        tests/test_parser.cpp
        tests/test_validator.cpp
        tests/test_processor.cpp
        tests/test_thresholds.cpp
        tests/test_alerts.cpp
        tests/test_firebase_errors.cpp
        # Shared source files needed by tests
        src/SensorData.cpp
        src/SensorParser.cpp
        src/SensorValidator.cpp
        src/SensorProcessor.cpp
        src/ThresholdConfig.cpp
        src/AlertManager.cpp
        src/FirebaseClient.cpp
        src/ConfigManager.cpp
    )

    add_executable(smart_agriculture_tests ${TEST_SOURCES})

    target_include_directories(smart_agriculture_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(smart_agriculture_tests PRIVATE
        GTest::gtest_main
        CURL::libcurl
        Threads::Threads
        ZLIB::ZLIB
        nlohmann_json::nlohmann_json
    )

    include(GoogleTest)
    gtest_discover_tests(smart_agriculture_tests)
endif()
