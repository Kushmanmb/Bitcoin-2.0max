cmake_minimum_required(VERSION 3.16)

project(Bitcoin2Max
    VERSION 2.0.0
    DESCRIPTION "Bitcoin 2.0max — supercharged block speed and scalability"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type defaults to Release for maximum performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags for optimised release build
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# ── Options ───────────────────────────────────────────────────────────────────
option(BUILD_TESTS          "Build unit tests"                  ON)
option(ENABLE_ELECTRUM      "Enable built-in Electrum client"   ON)
option(ENABLE_TOR           "Route Electrum traffic over Tor"   OFF)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Optional: Boost (headers only for asio)
cmake_policy(SET CMP0167 OLD)
find_package(Boost 1.71 COMPONENTS system)

# ── Source files ──────────────────────────────────────────────────────────────
set(BITCOIN2MAX_SOURCES
    src/main.cpp
    src/config/config.cpp
    src/node/node.cpp
    src/electrum/electrum_client.cpp
    src/crypto/signature.cpp
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(bitcoin2maxd ${BITCOIN2MAX_SOURCES})

target_include_directories(bitcoin2maxd
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(bitcoin2maxd
    PRIVATE
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
)

if(Boost_FOUND)
    target_include_directories(bitcoin2maxd PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(bitcoin2maxd PRIVATE ${Boost_LIBRARIES})
    target_compile_definitions(bitcoin2maxd PRIVATE HAVE_BOOST)
endif()

if(ENABLE_ELECTRUM)
    target_compile_definitions(bitcoin2maxd PRIVATE ENABLE_ELECTRUM)
endif()

if(ENABLE_TOR)
    target_compile_definitions(bitcoin2maxd PRIVATE ENABLE_TOR)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS bitcoin2maxd
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES conf/bitcoin2max.conf
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/bitcoin2max
)

# ── Tests ─────────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Summary ───────────────────────────────────────────────────────────────────
message(STATUS "------------------------------------------------------------")
message(STATUS "  Bitcoin 2.0max  v${PROJECT_VERSION}")
message(STATUS "  Build type      : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Electrum client : ${ENABLE_ELECTRUM}")
message(STATUS "  Tor routing     : ${ENABLE_TOR}")
message(STATUS "  Tests           : ${BUILD_TESTS}")
message(STATUS "------------------------------------------------------------")
