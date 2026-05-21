cmake_minimum_required(VERSION 3.21)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_STRING)
string(STRIP "${PROJECT_VERSION_STRING}" PROJECT_VERSION_STRING)

project(angzarr-client VERSION ${PROJECT_VERSION_STRING} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages.
#
# OpenSSL was added as an explicit link (audit #50) once the testing
# helpers grew a UUID v5 dependency on EVP_Digest (SHA-1). It was
# already a transitive dep through gRPC; the explicit link makes the
# contract visible at the CMake layer and protects against gRPC
# switching off OpenSSL in a future package build.
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Proto generation
#
# 2026-05-15 submodule bump (a7d6303): proto files moved from
# proto/angzarr/*.proto to proto/angzarr_client/proto/angzarr/*.proto
# and the package was renamed from `angzarr` to
# `angzarr_client.proto.angzarr` for cross-language Python-import
# parity. C++ keeps the legacy ``angzarr::Cover`` namespace alias via
# a namespace-import in :file:`include/angzarr/proto_aliases.hpp` so
# existing call sites stay byte-identical.
set(PROTO_DIR "${CMAKE_SOURCE_DIR}/angzarr-project/proto")
# PR #11 (vendor sererr in submodule): sererr.proto lives at
# ${PROTO_DIR}/sererr/v1/sererr.proto when present, so the submodule's
# PROTO_DIR is the single include root. The submodule pinned SHA in
# this repo may pre-date PR #11 — gate the file's inclusion on
# existence so unrelated submodule states still configure cleanly
# (mirrors the C# pilot's Condition="Exists(...)" gating). When sererr
# is missing, types.proto must not import it; the upstream angzarr-project
# SHA controls whether that import is present.
set(PROTO_FILES
    ${PROTO_DIR}/google/api/annotations.proto
    ${PROTO_DIR}/google/api/http.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/types.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/command_handler.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/saga.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/projector.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/process_manager.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/query.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/stream.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/upcaster.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/meta.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/v1/cloudevents.proto
)
if(EXISTS "${PROTO_DIR}/sererr/v1/sererr.proto")
    list(APPEND PROTO_FILES "${PROTO_DIR}/sererr/v1/sererr.proto")
endif()

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})
file(MAKE_DIRECTORY ${GENERATED_DIR}/angzarr)
file(MAKE_DIRECTORY ${GENERATED_DIR}/angzarr_client/proto/angzarr)
file(MAKE_DIRECTORY ${GENERATED_DIR}/google/api)
if(EXISTS "${PROTO_DIR}/sererr/v1/sererr.proto")
    file(MAKE_DIRECTORY ${GENERATED_DIR}/sererr/v1)
endif()

# Generate protobuf and gRPC sources
# Use relative path from PROTO_DIR to preserve subdirectory structure
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    file(RELATIVE_PATH PROTO_REL_PATH ${PROTO_DIR} ${PROTO_FILE})
    get_filename_component(PROTO_SUBDIR ${PROTO_REL_PATH} DIRECTORY)

    set(OUT_DIR "${GENERATED_DIR}/${PROTO_SUBDIR}")
    file(MAKE_DIRECTORY ${OUT_DIR})

    list(APPEND PROTO_SRCS "${OUT_DIR}/${PROTO_NAME}.pb.cc")
    list(APPEND PROTO_HDRS "${OUT_DIR}/${PROTO_NAME}.pb.h")
    list(APPEND GRPC_SRCS "${OUT_DIR}/${PROTO_NAME}.grpc.pb.cc")
    list(APPEND GRPC_HDRS "${OUT_DIR}/${PROTO_NAME}.grpc.pb.h")

    add_custom_command(
        OUTPUT "${OUT_DIR}/${PROTO_NAME}.pb.cc"
               "${OUT_DIR}/${PROTO_NAME}.pb.h"
               "${OUT_DIR}/${PROTO_NAME}.grpc.pb.cc"
               "${OUT_DIR}/${PROTO_NAME}.grpc.pb.h"
        COMMAND protobuf::protoc
        ARGS --grpc_out=${GENERATED_DIR}
             --cpp_out=${GENERATED_DIR}
             --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
             -I${PROTO_DIR}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating C++ protobuf/gRPC sources for ${PROTO_SUBDIR}/${PROTO_NAME}"
    )
endforeach()

# Discrete proto-generation aggregator target.
#
# The justfile's `generate-proto` recipe drives this target so the protoc
# custom-commands can run WITHOUT compiling the library. Keeping the regen
# entrypoint outside the main build graph matches the cross-language
# pattern (project_proto_generation_model) — the `add_custom_command`
# rules above stay as the EXECUTOR, but `just generate-proto` is the
# canonical TRIGGER.
add_custom_target(proto_gen
    DEPENDS ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
)

# Client library
add_library(angzarr-client
    src/helpers.cpp
    src/validation.cpp
    src/router.cpp
    src/aggregate.cpp
    src/saga_context.cpp
    src/cloudevents.cpp
    src/upcaster.cpp
    src/logging.cpp
    src/readiness.cpp
    src/testing_helpers.cpp
    src/identity.cpp
    src/transport.cpp
    src/server.cpp
    src/grpc_translator.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(angzarr-client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${GENERATED_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(angzarr-client
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
        gRPC::grpc++_reflection
        absl::strings
        OpenSSL::Crypto
)

# absl::log / absl::log_initialize only exist on absl >= 20230125.
# Ubuntu 24.04's libabsl-dev ships 20220623 which predates them; the
# C++ source guards the includes behind ``__has_include`` (see
# :file:`src/logging.cpp` + :file:`src/server.cpp`). Mirror the
# conditional at the link layer so the mutation-test container build
# (clang-17 + older absl) doesn't fail with "cannot find -labsl::log".
if(TARGET absl::log)
    target_link_libraries(angzarr-client PUBLIC absl::log)
endif()
if(TARGET absl::log_initialize)
    target_link_libraries(angzarr-client PUBLIC absl::log_initialize)
endif()

# Inject version as compile definition
target_compile_definitions(angzarr-client
    PUBLIC
        ANGZARR_VERSION="${PROJECT_VERSION}"
)

# Install
install(TARGETS angzarr-client
    EXPORT angzarr-client-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/angzarr
    DESTINATION include
)

install(DIRECTORY ${GENERATED_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# CMake package config for find_package() support
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/angzarr-client-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/angzarr-client-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/angzarr-client-config.cmake"
    INSTALL_DESTINATION lib/cmake/angzarr-client
)

install(EXPORT angzarr-client-targets
    FILE angzarr-client-targets.cmake
    NAMESPACE angzarr::
    DESTINATION lib/cmake/angzarr-client
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/angzarr-client-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/angzarr-client-config-version.cmake"
    DESTINATION lib/cmake/angzarr-client
)

# Testing
enable_testing()
find_package(GTest CONFIG REQUIRED)

# Test catalog. Every entry must be a file named tests/<name>.cpp linking
# against angzarr-client + GTest::gtest + GTest::gtest_main. Splitting the
# catalog by purpose keeps `mutation_tests`/`mutation_tests_d_cpp` source
# lists in sync with the gtest target list without per-test repetition.
#
# ANGZARR_CLIENT_TESTS — every per-target gtest binary.
# ANGZARR_MUTATION_TESTS — sources combined into the `mutation_tests`
#   binary that mull-runner mutates. Excludes *_mirror tests (those are
#   cross-language byte-parity checks that depend on absolute paths under
#   the runner host; mull's IR rewrite makes them non-deterministic).
# ANGZARR_MUTATION_D_CPP_TESTS — scoped Phase D-C++ subset for the
#   `mutation_tests_d_cpp` binary; see the lifelong note in fix-log about
#   the clang-17/abseil-20220623 segfault on pre-existing tests.
set(ANGZARR_CLIENT_TESTS
    rejection_handler_response_test
    builder_test
    error_introspection_test
    router_test
    compensation_test
    compensation_mirror_test
    error_defaults_mirror_test
    state_building_test
    component_router_test
    helpers_test
    destinations_test
    error_codes_test
    client_test
    logger_test
    readiness_test
    testing_helpers_test
    projector_unknown_test
    identity_test
    transport_test
    server_test
    unified_router_test
    builder_convergence_test
    retry_convergence_test
    grpc_translator_test
    helpers_convergence_test
)

set(ANGZARR_MUTATION_TESTS ${ANGZARR_CLIENT_TESTS})
list(REMOVE_ITEM ANGZARR_MUTATION_TESTS
    compensation_mirror_test
    error_defaults_mirror_test
)

set(ANGZARR_MUTATION_D_CPP_TESTS
    identity_test
    transport_test
    server_test
    unified_router_test
    builder_convergence_test
    retry_convergence_test
    grpc_translator_test
    helpers_convergence_test
)

include(GoogleTest)

foreach(test_name IN LISTS ANGZARR_CLIENT_TESTS)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name}
        PRIVATE
            angzarr-client
            GTest::gtest
            GTest::gtest_main
    )
    # PRE_TEST defers enumeration until ctest runtime, after the binary
    # exists. POST_BUILD (the default) tries to introspect the binary
    # during configure on fresh trees where it does not yet exist.
    gtest_discover_tests(${test_name} DISCOVERY_MODE PRE_TEST)
endforeach()

# Aggregated mull mutation target. `mutation_tests` is what
# `just mutation-test` builds; the per-test binaries above stay around for
# normal ctest runs.
set(_mutation_sources)
foreach(t IN LISTS ANGZARR_MUTATION_TESTS)
    list(APPEND _mutation_sources tests/${t}.cpp)
endforeach()
add_executable(mutation_tests ${_mutation_sources})
target_link_libraries(mutation_tests
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)
unset(_mutation_sources)

set(_mutation_d_cpp_sources)
foreach(t IN LISTS ANGZARR_MUTATION_D_CPP_TESTS)
    list(APPEND _mutation_d_cpp_sources tests/${t}.cpp)
endforeach()
add_executable(mutation_tests_d_cpp ${_mutation_d_cpp_sources})
target_link_libraries(mutation_tests_d_cpp
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)
unset(_mutation_d_cpp_sources)
