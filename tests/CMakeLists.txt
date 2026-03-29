cmake_minimum_required(VERSION 3.16)

# ── Fetch Catch2 (header-only v3) ────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.3
)
FetchContent_MakeAvailable(Catch2)

# ── Test sources ─────────────────────────────────────────────────────────────
add_executable(bitcoin2max_tests
    test_config.cpp
    test_params.cpp
    test_signature.cpp
    test_wallet.cpp
)

target_include_directories(bitcoin2max_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(bitcoin2max_tests
    PRIVATE
        Catch2::Catch2WithMain
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Pull in the implementation objects we want to test
target_sources(bitcoin2max_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/config/config.cpp
        ${CMAKE_SOURCE_DIR}/src/crypto/signature.cpp
)

include(CTest)
include(Catch)
catch_discover_tests(bitcoin2max_tests)
