cmake_minimum_required(VERSION 3.20)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_STRING)
string(STRIP "${PROJECT_VERSION_STRING}" PROJECT_VERSION_STRING)

project(angzarr-client VERSION ${PROJECT_VERSION_STRING} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

# Proto generation
#
# angzarr-project relocated proto files from proto/angzarr/*.proto to
# proto/angzarr_client/proto/angzarr/*.proto (Python and Rust adapted
# previously). For C++ we want the generated headers to keep landing at
# generated/angzarr/types.pb.h so existing `#include "angzarr/types.pb.h"`
# source lines stay valid. We achieve this with two -I paths:
#   1. angzarr-project/proto/angzarr_client/proto  → makes angzarr/* files
#      have a SHORT relative path (`angzarr/types.proto`) and emit at
#      `${GENERATED_DIR}/angzarr/types.pb.h`.
#   2. angzarr-project/proto                       → keeps cross-imports
#      like `angzarr_client/proto/angzarr/types.proto` resolvable AND
#      lets google/api/* files keep their relative path
#      (`google/api/annotations.proto`).
set(PROTO_DIR "${CMAKE_SOURCE_DIR}/angzarr-project/proto")
set(PROTO_INCLUDE_DIR "${PROTO_DIR}/angzarr_client/proto")
set(PROTO_FILES
    ${PROTO_DIR}/google/api/annotations.proto
    ${PROTO_DIR}/google/api/http.proto
    ${PROTO_INCLUDE_DIR}/angzarr/types.proto
    ${PROTO_INCLUDE_DIR}/angzarr/command_handler.proto
    ${PROTO_INCLUDE_DIR}/angzarr/saga.proto
    ${PROTO_INCLUDE_DIR}/angzarr/projector.proto
    ${PROTO_INCLUDE_DIR}/angzarr/process_manager.proto
    ${PROTO_INCLUDE_DIR}/angzarr/query.proto
    ${PROTO_INCLUDE_DIR}/angzarr/stream.proto
    ${PROTO_INCLUDE_DIR}/angzarr/upcaster.proto
    ${PROTO_INCLUDE_DIR}/angzarr/meta.proto
    ${PROTO_INCLUDE_DIR}/angzarr/cloudevents.proto
)

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})
file(MAKE_DIRECTORY ${GENERATED_DIR}/angzarr)
file(MAKE_DIRECTORY ${GENERATED_DIR}/google/api)

# Generate protobuf and gRPC sources.
# Compute output subdir against PROTO_INCLUDE_DIR for angzarr files so
# they emit at generated/angzarr/*; fall back to PROTO_DIR for everything
# else (the google/api/* files).
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    # Pick the longest matching include root for output-path computation.
    string(FIND ${PROTO_FILE} ${PROTO_INCLUDE_DIR}/ _MATCH_INCLUDE)
    if(${_MATCH_INCLUDE} EQUAL 0)
        file(RELATIVE_PATH PROTO_REL_PATH ${PROTO_INCLUDE_DIR} ${PROTO_FILE})
    else()
        file(RELATIVE_PATH PROTO_REL_PATH ${PROTO_DIR} ${PROTO_FILE})
    endif()
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
             -I${PROTO_INCLUDE_DIR}
             -I${PROTO_DIR}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating C++ protobuf/gRPC sources for ${PROTO_SUBDIR}/${PROTO_NAME}"
    )
endforeach()

# Client library
add_library(angzarr-client
    src/helpers.cpp
    src/validation.cpp
    src/router.cpp
    src/aggregate.cpp
    src/saga_context.cpp
    src/cloudevents.cpp
    src/upcaster.cpp
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
)

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

include(GoogleTest)
gtest_discover_tests(rejection_handler_response_test)
gtest_discover_tests(builder_test)
gtest_discover_tests(error_introspection_test)
gtest_discover_tests(router_test)
gtest_discover_tests(compensation_test)
gtest_discover_tests(state_building_test)
gtest_discover_tests(component_router_test)

# Mutation testing binary (all test sources combined for Mull)
add_executable(mutation_tests
    tests/rejection_handler_response_test.cpp
    tests/builder_test.cpp
    tests/error_introspection_test.cpp
    tests/router_test.cpp
    tests/compensation_test.cpp
    tests/state_building_test.cpp
    tests/component_router_test.cpp
)

target_link_libraries(mutation_tests
    PRIVATE
        angzarr-client
        GTest::gtest
        GTest::gtest_main
)
