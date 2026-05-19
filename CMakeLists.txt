cmake_minimum_required(VERSION 3.20)

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
    ${PROTO_DIR}/angzarr_client/proto/angzarr/types.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/command_handler.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/saga.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/projector.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/process_manager.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/query.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/stream.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/upcaster.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/meta.proto
    ${PROTO_DIR}/angzarr_client/proto/angzarr/cloudevents.proto
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

add_executable(rejection_handler_response_test
    tests/rejection_handler_response_test.cpp
)

target_link_libraries(rejection_handler_response_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(builder_test
    tests/builder_test.cpp
)

target_link_libraries(builder_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(error_introspection_test
    tests/error_introspection_test.cpp
)

target_link_libraries(error_introspection_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(router_test
    tests/router_test.cpp
)

target_link_libraries(router_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(compensation_test
    tests/compensation_test.cpp
)

target_link_libraries(compensation_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(compensation_mirror_test
    tests/compensation_mirror_test.cpp
)

target_link_libraries(compensation_mirror_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(error_defaults_mirror_test
    tests/error_defaults_mirror_test.cpp
)

target_link_libraries(error_defaults_mirror_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(state_building_test
    tests/state_building_test.cpp
)

target_link_libraries(state_building_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(component_router_test
    tests/component_router_test.cpp
)

target_link_libraries(component_router_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(helpers_test
    tests/helpers_test.cpp
)

target_link_libraries(helpers_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(destinations_test
    tests/destinations_test.cpp
)

target_link_libraries(destinations_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(error_codes_test
    tests/error_codes_test.cpp
)

target_link_libraries(error_codes_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(client_test
    tests/client_test.cpp
)

target_link_libraries(client_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(logger_test
    tests/logger_test.cpp
)

target_link_libraries(logger_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(readiness_test
    tests/readiness_test.cpp
)

target_link_libraries(readiness_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(testing_helpers_test
    tests/testing_helpers_test.cpp
)

target_link_libraries(testing_helpers_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

add_executable(projector_unknown_test
    tests/projector_unknown_test.cpp
)

target_link_libraries(projector_unknown_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec HIGH-2.2 — cross-language compute_root parity (Phase D-C++).
add_executable(identity_test
    tests/identity_test.cpp
)

target_link_libraries(identity_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec HIGH-6.1 — transport-mode + endpoint resolver parity (Phase D-C++).
add_executable(transport_test
    tests/transport_test.cpp
)

target_link_libraries(transport_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec HIGH-5.1 — server module parity (Phase D-C++).
add_executable(server_test
    tests/server_test.cpp
)

target_link_libraries(server_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec HIGH-4.2/4.3/4.4 — unified Router aggregator + BuildError +
# UpcasterRouter recognition (Phase D-C++).
add_executable(unified_router_test
    tests/unified_router_test.cpp
)

target_link_libraries(unified_router_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec HIGH-3.1/3.2 + MED-3.x — CommandBuilder spec convergence
# (rootless ctors, code stamps, RFC3339 parser, UUID v4 gen, with_sync_mode).
add_executable(builder_convergence_test
    tests/builder_convergence_test.cpp
)

target_link_libraries(builder_convergence_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec HIGH-6.2 + MED/LOW — RetryPolicy value-returning execute,
# on_retry, public compute_delay, FromStub seam (deferred per spec).
add_executable(retry_convergence_test
    tests/retry_convergence_test.cpp
)

target_link_libraries(retry_convergence_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec HIGH-1.3 + MED-1.8 + MED-4.10 — gRPC status translator and
# Any-decode wrapping (Phase D-C++).
add_executable(grpc_translator_test
    tests/grpc_translator_test.cpp
)

target_link_libraries(grpc_translator_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec MED-2.x — helper parity (cache_key, routing_key, edition,
# parse_timestamp, full_type_name/url, type_matches, try_unpack, unpack).
add_executable(helpers_convergence_test
    tests/helpers_convergence_test.cpp
)

target_link_libraries(helpers_convergence_test
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(rejection_handler_response_test)
gtest_discover_tests(builder_test)
gtest_discover_tests(error_introspection_test)
gtest_discover_tests(router_test)
gtest_discover_tests(compensation_test)
gtest_discover_tests(compensation_mirror_test)
gtest_discover_tests(error_defaults_mirror_test)
gtest_discover_tests(state_building_test)
gtest_discover_tests(component_router_test)
gtest_discover_tests(helpers_test)
gtest_discover_tests(destinations_test)
gtest_discover_tests(error_codes_test)
gtest_discover_tests(client_test)
gtest_discover_tests(logger_test)
gtest_discover_tests(readiness_test)
gtest_discover_tests(testing_helpers_test)
gtest_discover_tests(projector_unknown_test)
gtest_discover_tests(identity_test)
gtest_discover_tests(transport_test)
gtest_discover_tests(server_test)
gtest_discover_tests(unified_router_test)
gtest_discover_tests(builder_convergence_test)
gtest_discover_tests(retry_convergence_test)
gtest_discover_tests(grpc_translator_test)
gtest_discover_tests(helpers_convergence_test)

# Mutation testing binary (all test sources combined for Mull)
add_executable(mutation_tests
    tests/rejection_handler_response_test.cpp
    tests/builder_test.cpp
    tests/error_introspection_test.cpp
    tests/router_test.cpp
    tests/compensation_test.cpp
    tests/state_building_test.cpp
    tests/component_router_test.cpp
    tests/helpers_test.cpp
    tests/destinations_test.cpp
    tests/error_codes_test.cpp
    tests/client_test.cpp
    tests/logger_test.cpp
    tests/readiness_test.cpp
    tests/testing_helpers_test.cpp
    tests/projector_unknown_test.cpp
    tests/identity_test.cpp
    tests/transport_test.cpp
    tests/server_test.cpp
    tests/unified_router_test.cpp
    tests/builder_convergence_test.cpp
    tests/retry_convergence_test.cpp
    tests/grpc_translator_test.cpp
    tests/helpers_convergence_test.cpp
)

target_link_libraries(mutation_tests
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)

# Spec D-C++ Phase D: dedicated scoped mutation binary covering only the
# new convergence test files. The mull-runner image (clang-17 + Ubuntu
# 24.04 libabsl 20220623) segfaults on several pre-existing tests
# (CommandRouterTest, AggregateRouterTest, ProcessManagerRouterTest)
# during mull's warmup run — see fix-log discoveries. Splitting the
# new tests into ``mutation_tests_d_cpp`` lets mull get a clean warmup
# pass without depending on a container-image refresh.
add_executable(mutation_tests_d_cpp
    tests/identity_test.cpp
    tests/transport_test.cpp
    tests/server_test.cpp
    tests/unified_router_test.cpp
    tests/builder_convergence_test.cpp
    tests/retry_convergence_test.cpp
    tests/grpc_translator_test.cpp
    tests/helpers_convergence_test.cpp
)

target_link_libraries(mutation_tests_d_cpp
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)
