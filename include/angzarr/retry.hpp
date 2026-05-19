#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>

namespace angzarr {

/**
 * Strategy for retrying failed operations.
 *
 * Spec HIGH-6.2: the previous interface accepted only ``std::function<void()>``
 * (side-effect only). All five sibling languages (Py/Rs/Ja, plus newly
 * D-Go and D-C#) support a value-returning ``execute<T>(F)`` so a
 * retried ``connect()`` can return the channel directly without
 * capturing it via a side-effect closure. C++ now joins via the
 * templated :func:`execute` below.
 */
class RetryPolicy {
 public:
  virtual ~RetryPolicy() = default;

  /**
   * Run the operation, retrying on failure according to the policy.
   *
   * @param operation The operation to execute
   * @throws The last exception if all attempts fail
   */
  virtual void execute(std::function<void()> operation) = 0;

  /**
   * Spec HIGH-6.2 — value-returning execute. ``F`` is any callable
   * returning ``T``; this defaults to the void overload pattern when
   * ``T == void``.
   *
   * Implemented in terms of :func:`do_execute_attempt` so concrete
   * policies don't need to repeat the for-loop scaffolding.
   */
  template <typename F>
  auto execute(F operation) -> decltype(operation()) {
    using R = decltype(operation());
    if constexpr (std::is_void_v<R>) {
      execute(std::function<void()>{[&]() { operation(); }});
    } else {
      R result{};
      execute(std::function<void()>{[&]() { result = operation(); }});
      return result;
    }
  }
};

/**
 * Retries with exponential backoff and optional jitter.
 */
class ExponentialBackoffRetry : public RetryPolicy {
 public:
  /// Spec MED-6.3 — optional callback invoked before each backoff
  /// sleep (after a failed attempt, never before the first attempt
  /// and never after the final failure). Mirrors Rs ``OnRetry`` and
  /// Ja ``BiConsumer<Integer, Throwable>``.
  using OnRetry = std::function<void(int attempt, const std::exception_ptr&)>;

  ExponentialBackoffRetry(
      std::chrono::milliseconds min_delay = std::chrono::milliseconds(100),
      std::chrono::milliseconds max_delay = std::chrono::milliseconds(5000),
      int max_attempts = 10, bool jitter = true, OnRetry on_retry = nullptr)
      : min_delay_(min_delay),
        max_delay_(max_delay),
        max_attempts_(max_attempts),
        jitter_(jitter),
        on_retry_(std::move(on_retry)) {}

  using RetryPolicy::execute;  // bring template overload into scope.

  void execute(std::function<void()> operation) override {
    std::exception_ptr last_error;
    for (int attempt = 0; attempt < max_attempts_; ++attempt) {
      try {
        operation();
        return;
      } catch (...) {
        last_error = std::current_exception();
        if (attempt < max_attempts_ - 1) {
          if (on_retry_) on_retry_(attempt, last_error);
          auto delay = compute_delay(attempt);
          std::this_thread::sleep_for(delay);
        }
      }
    }
    if (last_error) {
      std::rethrow_exception(last_error);
    }
  }

  int max_attempts() const { return max_attempts_; }

  /**
   * Spec MED-6.7 — exposed publicly for cross-language test parity
   * (Rs/Ja/C# expose it; Go/Py have it private). Useful for pinning
   * the exponential-backoff curve in unit tests without using
   * sleep-and-measure timing tricks.
   */
  std::chrono::milliseconds compute_delay(int attempt) const {
    // Spec MED-6.4 — cap the exponent at 30 to avoid IEEE-754 +Inf
    // (matches Py/Rs/Ja). The cap is a no-op for any realistic
    // max_attempts but defends against the curve being raised in
    // the future without the cap.
    int capped_attempt = std::min(attempt, 30);
    double delay_ms =
        static_cast<double>(min_delay_.count()) * std::pow(2, capped_attempt);
    delay_ms = std::min(delay_ms, static_cast<double>(max_delay_.count()));
    if (jitter_) {
      static thread_local std::mt19937 gen(std::random_device{}());
      std::uniform_real_distribution<> dist(0.5, 1.0);
      delay_ms *= dist(gen);
    }
    return std::chrono::milliseconds(static_cast<long>(delay_ms));
  }

 private:
  std::chrono::milliseconds min_delay_;
  std::chrono::milliseconds max_delay_;
  int max_attempts_;
  bool jitter_;
  OnRetry on_retry_;
};

/**
 * Returns the default retry policy matching Rust's backoff config.
 */
inline std::unique_ptr<RetryPolicy> default_retry_policy() {
  return std::make_unique<ExponentialBackoffRetry>();
}

}  // namespace angzarr
