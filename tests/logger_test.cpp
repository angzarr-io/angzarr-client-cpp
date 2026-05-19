// Tests for the Logger abstraction.
//
// Audit findings #87..#91: C++ previously had no integrated logging
// story; structured fields, panic level, and lifecycle event shape
// were all "TBD". This pass picks ``absl::log`` as the default backend
// (it's already a transitive dep through gRPC) and wraps it behind a
// pluggable ``Logger`` interface so callers can install their own
// sink (stdout / file / test capture / spdlog adapter).
//
// Parity targets:
//   - structlog event names + key=value fields (Python)
//   - tracing::warn!{key=value} (Rust)
//
// The captured-log tests use ``CaptureLogger`` to assert the exact
// event name + field set produced by lifecycle log call sites — same
// contract as Python's ``server_started`` / ``server_shutdown`` and
// Rust's matching tracing events.

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "angzarr/logging.hpp"

using namespace angzarr;

// ---------------------------------------------------------------------------
// LogLevel string conversion (#87..#90 — every cause warning must
// surface its level so log routers can filter on it).
// ---------------------------------------------------------------------------

TEST(LogLevel, ToStringIsLowerCase) {
    EXPECT_EQ(level_name(LogLevel::DEBUG), "debug");
    EXPECT_EQ(level_name(LogLevel::INFO), "info");
    EXPECT_EQ(level_name(LogLevel::WARN), "warn");
    EXPECT_EQ(level_name(LogLevel::ERROR), "error");
}

// ---------------------------------------------------------------------------
// CaptureLogger — pluggable backend for test assertions.
// ---------------------------------------------------------------------------

TEST(CaptureLogger, RecordsEventNameAndLevel) {
    auto cap = std::make_shared<CaptureLogger>();
    cap->log(LogLevel::INFO, "server_started", {});

    ASSERT_EQ(cap->records().size(), 1u);
    EXPECT_EQ(cap->records()[0].level, LogLevel::INFO);
    EXPECT_EQ(cap->records()[0].event, "server_started");
    EXPECT_TRUE(cap->records()[0].fields.empty());
}

TEST(CaptureLogger, RecordsStructuredFields) {
    auto cap = std::make_shared<CaptureLogger>();
    cap->log(LogLevel::WARN, "readiness_probe_failed", {{"probe", "bus"}, {"reason", "timeout"}});

    auto records = cap->records();  // records() returns by value — bind to local
    ASSERT_EQ(records.size(), 1u);
    const auto& rec = records[0];
    EXPECT_EQ(rec.event, "readiness_probe_failed");
    EXPECT_EQ(rec.fields.at("probe"), "bus");
    EXPECT_EQ(rec.fields.at("reason"), "timeout");
}

TEST(CaptureLogger, MultipleRecordsAccumulateInOrder) {
    auto cap = std::make_shared<CaptureLogger>();
    cap->log(LogLevel::INFO, "a", {});
    cap->log(LogLevel::WARN, "b", {});
    cap->log(LogLevel::ERROR, "c", {});

    ASSERT_EQ(cap->records().size(), 3u);
    EXPECT_EQ(cap->records()[0].event, "a");
    EXPECT_EQ(cap->records()[1].event, "b");
    EXPECT_EQ(cap->records()[2].event, "c");
}

// ---------------------------------------------------------------------------
// Default logger singleton — must always be non-null. Pre-fix
// audit #91, callers fell through to bare ``printf`` which dropped
// fields entirely; the default must preserve them.
// ---------------------------------------------------------------------------

TEST(DefaultLogger, IsNonNullOutOfTheBox) {
    auto* logger = default_logger();
    ASSERT_NE(logger, nullptr);
}

TEST(DefaultLogger, CanBeSwappedAndRestored) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    log_info("test_event", {{"k", "v"}});
    EXPECT_EQ(cap->records().size(), 1u);
    EXPECT_EQ(cap->records()[0].event, "test_event");
    EXPECT_EQ(cap->records()[0].fields.at("k"), "v");

    // Restore so subsequent tests aren't polluted.
    set_default_logger(original);
}

// ---------------------------------------------------------------------------
// Convenience helpers (#88..#91): every cause warning + every lifecycle
// info must be reachable via a one-line call that routes through the
// installed Logger backend, never through std::cout/printf.
// ---------------------------------------------------------------------------

TEST(LogHelpers, AllLevelsRouteThroughDefaultLogger) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    log_debug("dbg", {});
    log_info("nfo", {});
    log_warn("wrn", {});
    log_error("err", {});

    ASSERT_EQ(cap->records().size(), 4u);
    EXPECT_EQ(cap->records()[0].level, LogLevel::DEBUG);
    EXPECT_EQ(cap->records()[1].level, LogLevel::INFO);
    EXPECT_EQ(cap->records()[2].level, LogLevel::WARN);
    EXPECT_EQ(cap->records()[3].level, LogLevel::ERROR);

    set_default_logger(original);
}

// ---------------------------------------------------------------------------
// Audit #89: server_started / server_shutdown log shape parity. Both
// languages emit the same event name + the same field set so operators
// can grep across pods regardless of language.
// ---------------------------------------------------------------------------

TEST(LifecycleLog, ServerStartedEmitsExpectedFields) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    log_server_started("CommandHandlerCoordinatorService", "orders", "tcp", "0.0.0.0:1310");

    auto records = cap->records();
    ASSERT_EQ(records.size(), 1u);
    const auto& rec = records[0];
    EXPECT_EQ(rec.level, LogLevel::INFO);
    EXPECT_EQ(rec.event, "server_started");
    EXPECT_EQ(rec.fields.at("service"), "CommandHandlerCoordinatorService");
    EXPECT_EQ(rec.fields.at("name"), "orders");
    EXPECT_EQ(rec.fields.at("transport"), "tcp");
    EXPECT_EQ(rec.fields.at("address"), "0.0.0.0:1310");

    set_default_logger(original);
}

TEST(LifecycleLog, ServerShutdownEmitsExpectedFields) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    log_server_shutdown("ProjectorCoordinatorService", "leaderboard");

    auto records = cap->records();
    ASSERT_EQ(records.size(), 1u);
    const auto& rec = records[0];
    EXPECT_EQ(rec.level, LogLevel::INFO);
    EXPECT_EQ(rec.event, "server_shutdown");
    EXPECT_EQ(rec.fields.at("service"), "ProjectorCoordinatorService");
    EXPECT_EQ(rec.fields.at("name"), "leaderboard");

    set_default_logger(original);
}
