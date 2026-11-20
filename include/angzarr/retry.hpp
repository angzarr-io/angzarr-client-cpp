#pragma once

#include <chrono>
#include <cmath>
#include <functional>
#include <random>
#include <stdexcept>
#include <thread>

namespace angzarr {

/**
 * Strategy for retrying failed operations.
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
};

/**
 * Retries with exponential backoff and optional jitter.
 */
class ExponentialBackoffRetry : public RetryPolicy {
   public:
    ExponentialBackoffRetry(
        std::chrono::milliseconds min_delay = std::chrono::milliseconds(100),
        std::chrono::milliseconds max_delay = std::chrono::milliseconds(5000),
        int max_attempts = 10,
        bool jitter = true)
        : min_delay_(min_delay),
          max_delay_(max_delay),
          max_attempts_(max_attempts),
          jitter_(jitter) {}

    void execute(std::function<void()> operation) override {
        std::exception_ptr last_error;
        for (int attempt = 0; attempt < max_attempts_; ++attempt) {
            try {
                operation();
                return;
            } catch (...) {
                last_error = std::current_exception();
                if (attempt < max_attempts_ - 1) {
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

   private:
    std::chrono::milliseconds min_delay_;
    std::chrono::milliseconds max_delay_;
    int max_attempts_;
    bool jitter_;

    std::chrono::milliseconds compute_delay(int attempt) {
        double delay_ms = static_cast<double>(min_delay_.count()) * std::pow(2, attempt);
        delay_ms = std::min(delay_ms, static_cast<double>(max_delay_.count()));
        if (jitter_) {
            static thread_local std::mt19937 gen(std::random_device{}());
            std::uniform_real_distribution<> dist(0.5, 1.0);
            delay_ms *= dist(gen);
        }
        return std::chrono::milliseconds(static_cast<long>(delay_ms));
    }
};

/**
 * Returns the default retry policy matching Rust's backoff config.
 */
inline std::unique_ptr<RetryPolicy> default_retry_policy() {
    return std::make_unique<ExponentialBackoffRetry>();
}

}  // namespace angzarr
