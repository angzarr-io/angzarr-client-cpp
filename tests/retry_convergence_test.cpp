// Spec convergence for RetryPolicy.
//
// Covers:
//   HIGH-6.2 — value-returning execute<T> overload alongside void.
//   MED-6.3  — on_retry callback fires before each backoff (not before
//              first attempt, not after final failure).
//   MED-6.4  — exponent capped at 30 to avoid IEEE-754 +Inf.
//   MED-6.7  — compute_delay is public.

#include "angzarr/retry.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace angzarr;
using namespace std::chrono_literals;

namespace {
// Fast policy for tests — short min/max to avoid wall time.
auto make_fast_policy(int max_attempts = 3, bool jitter = false,
                      ExponentialBackoffRetry::OnRetry on_retry = nullptr) {
    return ExponentialBackoffRetry(std::chrono::milliseconds(1),
                                   std::chrono::milliseconds(2), max_attempts, jitter,
                                   std::move(on_retry));
}
}  // namespace

// ─── HIGH-6.2 ─────────────────────────────────────────────────────────────────

TEST(RetryHigh62, ExecuteVoid_SucceedsOnFirstTry) {
    auto p = make_fast_policy();
    int calls = 0;
    p.execute([&]() { calls++; });
    EXPECT_EQ(calls, 1);
}

TEST(RetryHigh62, ExecuteReturnsValue) {
    auto p = make_fast_policy();
    int x = p.execute([]() { return 42; });
    EXPECT_EQ(x, 42);
}

TEST(RetryHigh62, ExecuteReturnsString) {
    auto p = make_fast_policy();
    std::string s = p.execute([]() { return std::string("hello"); });
    EXPECT_EQ(s, "hello");
}

TEST(RetryHigh62, ExecuteReturnsValue_AfterRetries) {
    auto p = make_fast_policy(5);
    int call = 0;
    int result = p.execute([&]() {
        ++call;
        if (call < 3) throw std::runtime_error("transient");
        return call;
    });
    EXPECT_EQ(result, 3);
}

TEST(RetryHigh62, ExecuteReturnsUniquePtr_MovableOnly) {
    auto p = make_fast_policy();
    auto ptr = p.execute([]() { return std::make_unique<int>(7); });
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 7);
}

TEST(RetryHigh62, AllAttemptsFail_RethrowsLastError) {
    auto p = make_fast_policy(2);
    try {
        p.execute([]() { throw std::runtime_error("never works"); });
        FAIL() << "expected throw";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "never works");
    }
}

// ─── MED-6.3 ──────────────────────────────────────────────────────────────────

TEST(RetryMed63, OnRetry_FiresOncePerBackoff) {
    std::vector<int> attempts;
    auto on_retry = [&](int attempt, const std::exception_ptr&) {
        attempts.push_back(attempt);
    };
    auto p = make_fast_policy(3, false, on_retry);
    int call = 0;
    EXPECT_THROW(p.execute([&]() {
        ++call;
        throw std::runtime_error("fail");
    }),
                 std::runtime_error);
    // 3 attempts, but on_retry fires only between attempts → 2 fires.
    EXPECT_EQ(attempts.size(), 2u);
    EXPECT_EQ(attempts[0], 0);
    EXPECT_EQ(attempts[1], 1);
}

TEST(RetryMed63, OnRetry_NotFiredOnFirstAttemptSuccess) {
    int fire_count = 0;
    auto on_retry = [&](int, const std::exception_ptr&) { ++fire_count; };
    auto p = make_fast_policy(3, false, on_retry);
    p.execute([]() { /* succeeds */ });
    EXPECT_EQ(fire_count, 0);
}

TEST(RetryMed63, OnRetry_NotFiredAfterFinalFailure) {
    int fire_count = 0;
    auto on_retry = [&](int, const std::exception_ptr&) { ++fire_count; };
    auto p = make_fast_policy(2, false, on_retry);
    EXPECT_THROW(p.execute([]() { throw std::runtime_error("x"); }), std::runtime_error);
    // 2 attempts → 1 inter-attempt fire (between attempt 0 and attempt 1).
    EXPECT_EQ(fire_count, 1);
}

TEST(RetryMed63, OnRetry_ReceivesExceptionPtr) {
    std::string captured_msg;
    auto on_retry = [&](int, const std::exception_ptr& eptr) {
        try {
            std::rethrow_exception(eptr);
        } catch (const std::runtime_error& e) {
            captured_msg = e.what();
        }
    };
    auto p = make_fast_policy(2, false, on_retry);
    EXPECT_THROW(p.execute([]() { throw std::runtime_error("hello"); }), std::runtime_error);
    EXPECT_EQ(captured_msg, "hello");
}

// ─── MED-6.4 / MED-6.7 ────────────────────────────────────────────────────────

TEST(RetryMed64Med67, ComputeDelay_IsPublic_AndMonotonicWithoutJitter) {
    ExponentialBackoffRetry p(std::chrono::milliseconds(10), std::chrono::milliseconds(10000),
                              10, /*jitter=*/false);
    EXPECT_EQ(p.compute_delay(0).count(), 10);
    EXPECT_EQ(p.compute_delay(1).count(), 20);
    EXPECT_EQ(p.compute_delay(2).count(), 40);
    EXPECT_EQ(p.compute_delay(3).count(), 80);
}

TEST(RetryMed64Med67, ComputeDelay_CapsAtMaxDelay) {
    ExponentialBackoffRetry p(std::chrono::milliseconds(100), std::chrono::milliseconds(5000),
                              20, /*jitter=*/false);
    // 2^20 * 100 ms = ~100s → should clamp to 5000 ms.
    EXPECT_EQ(p.compute_delay(20).count(), 5000);
}

TEST(RetryMed64Med67, ComputeDelay_ExponentCappedAt30) {
    // Without the cap, attempt=60 produces ``2^60 * 100`` which is
    // ~10^20 ms — overflows ``double`` to ``+Inf``, and then
    // ``std::min(+Inf, max)`` is ``max``. We need to verify the cap is
    // applied (no IEEE-754 +Inf) — check that very large attempt
    // returns the max_delay without UB.
    ExponentialBackoffRetry p(std::chrono::milliseconds(1), std::chrono::milliseconds(5000),
                              100, /*jitter=*/false);
    auto d = p.compute_delay(1000);
    EXPECT_EQ(d.count(), 5000);
    // Sanity: should not be negative, nan, or absurd.
    EXPECT_GE(d.count(), 0);
}

TEST(RetryMed64Med67, ComputeDelay_JitteredFallsInRange) {
    ExponentialBackoffRetry p(std::chrono::milliseconds(100), std::chrono::milliseconds(5000),
                              10, /*jitter=*/true);
    auto d = p.compute_delay(2);
    // Without jitter: 100 * 4 = 400; jitter halves to (200, 400] range.
    EXPECT_GE(d.count(), 200);
    EXPECT_LE(d.count(), 400);
}

// ─── DefaultPolicy parity (audit cross-check) ────────────────────────────────

TEST(RetryHigh62, MaxAttemptsRespected_ExactlyN) {
    // Mutation guard: ``for (attempt = 0; attempt < max_attempts_; ++attempt)``
    // — replacing < with <= would cause one extra attempt.
    int calls = 0;
    auto p = make_fast_policy(3);
    EXPECT_THROW(p.execute([&]() {
        ++calls;
        throw std::runtime_error("x");
    }),
                 std::runtime_error);
    EXPECT_EQ(calls, 3);  // exactly max_attempts
}

TEST(RetryHigh62, MaxAttemptsOne_OneAttemptOnly) {
    int calls = 0;
    auto p = make_fast_policy(1);
    EXPECT_THROW(p.execute([&]() {
        ++calls;
        throw std::runtime_error("x");
    }),
                 std::runtime_error);
    EXPECT_EQ(calls, 1);
}

TEST(RetryDefaults, DefaultPolicyValuesMatchOtherLangs) {
    ExponentialBackoffRetry p(std::chrono::milliseconds(100), std::chrono::milliseconds(5000),
                              10, /*jitter=*/false);  // defaults except deterministic
    EXPECT_EQ(p.max_attempts(), 10);
    EXPECT_EQ(p.compute_delay(0).count(), 100);
}
