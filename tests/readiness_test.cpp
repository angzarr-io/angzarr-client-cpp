// Tests for the readiness supervisor and probes.
//
// Mirrors Python ``tests/test_readiness.py`` and Rust
// ``src/readiness.rs#tests``. The C++ port uses synchronous probes
// (``std::function<bool()>``) rather than coroutines, because gRPC C++
// is a thread-pool server, not an event-loop one — the supervisor
// runs on a dedicated std::thread and aggregates per-tick.
//
// Audit findings covered:
//   - #68: probe interface + supervisor
//   - #74: BusProbe for async-only saga / PM targets
//   - #82: panicking probe must not unwind the supervisor; each cause
//          (panic / timeout / probe-returned-false) emits its own
//          WARN log (assertable via CaptureLogger).

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "angzarr/logging.hpp"
#include "angzarr/readiness.hpp"

using namespace angzarr;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// probe_config_from_env (#68)
// ---------------------------------------------------------------------------

namespace {

// Tiny env-guard so per-test mutations don't leak between cases. The
// readiness module reads ``ANGZARR_READINESS_*`` once per call; we
// scope mutations to a single test by saving + restoring.
class EnvGuard {
   public:
    explicit EnvGuard(const std::string& key) : key_(key) {
        const char* v = std::getenv(key.c_str());
        had_ = v != nullptr;
        if (had_) previous_ = v;
    }
    ~EnvGuard() {
        if (had_) ::setenv(key_.c_str(), previous_.c_str(), 1);
        else ::unsetenv(key_.c_str());
    }
    void set(const std::string& value) { ::setenv(key_.c_str(), value.c_str(), 1); }
    void unset() { ::unsetenv(key_.c_str()); }

   private:
    std::string key_;
    bool had_ = false;
    std::string previous_;
};

}  // namespace

TEST(ProbeConfigFromEnv, DefaultsWhenUnset) {
    EnvGuard g1(readiness::ENV_INTERVAL);
    EnvGuard g2(readiness::ENV_TIMEOUT);
    g1.unset();
    g2.unset();
    auto [interval, timeout] = readiness::probe_config_from_env();
    EXPECT_EQ(interval, readiness::DEFAULT_PROBE_INTERVAL);
    EXPECT_EQ(timeout, readiness::DEFAULT_PROBE_TIMEOUT);
}

TEST(ProbeConfigFromEnv, ReadsValuesFromEnv) {
    EnvGuard g1(readiness::ENV_INTERVAL);
    EnvGuard g2(readiness::ENV_TIMEOUT);
    g1.set("5");
    g2.set("1");
    auto [interval, timeout] = readiness::probe_config_from_env();
    EXPECT_EQ(interval, std::chrono::seconds(5));
    EXPECT_EQ(timeout, std::chrono::seconds(1));
}

TEST(ProbeConfigFromEnv, FallsBackOnGarbageValues) {
    EnvGuard g1(readiness::ENV_INTERVAL);
    EnvGuard g2(readiness::ENV_TIMEOUT);
    g1.set("thirty");
    g2.set("two");
    auto [interval, timeout] = readiness::probe_config_from_env();
    EXPECT_EQ(interval, readiness::DEFAULT_PROBE_INTERVAL);
    EXPECT_EQ(timeout, readiness::DEFAULT_PROBE_TIMEOUT);
}

// ---------------------------------------------------------------------------
// TransportProbe / TransportSignal (#68)
// ---------------------------------------------------------------------------

TEST(TransportProbe, StartsUnbound) {
    auto [probe, signal] = readiness::TransportProbe::create();
    EXPECT_EQ(probe->name(), "transport");
    EXPECT_FALSE(probe->check());
    EXPECT_FALSE(signal->is_bound());
}

TEST(TransportProbe, FlipsAfterMarkBound) {
    auto [probe, signal] = readiness::TransportProbe::create();
    signal->mark_bound();
    EXPECT_TRUE(probe->check());
}

TEST(TransportProbe, SignalIsOneWay) {
    auto [probe, signal] = readiness::TransportProbe::create();
    signal->mark_bound();
    EXPECT_TRUE(probe->check());
    EXPECT_TRUE(signal->is_bound());
    EXPECT_TRUE(probe->check());
}

// ---------------------------------------------------------------------------
// Endpoint parsing (#68)
// ---------------------------------------------------------------------------

TEST(ParseEndpoint, TcpHostPort) {
    auto ep = readiness::parse_endpoint("inventory:1310");
    EXPECT_TRUE(ep.is_tcp());
    EXPECT_EQ(ep.address, "inventory:1310");
}

TEST(ParseEndpoint, UnixPrefixIsUds) {
    auto ep = readiness::parse_endpoint("unix:/tmp/sock");
    EXPECT_FALSE(ep.is_tcp());
    EXPECT_EQ(ep.address, "/tmp/sock");
}

TEST(ParseEndpoint, UnixRelativeIsUds) {
    auto ep = readiness::parse_endpoint("unix:relative.sock");
    EXPECT_FALSE(ep.is_tcp());
    EXPECT_EQ(ep.address, "relative.sock");
}

TEST(ParseEndpoint, LeadingSlashIsUds) {
    auto ep = readiness::parse_endpoint("/var/run/angzarr.sock");
    EXPECT_FALSE(ep.is_tcp());
    EXPECT_EQ(ep.address, "/var/run/angzarr.sock");
}

// ---------------------------------------------------------------------------
// OutputDomainProbe (#74)
// ---------------------------------------------------------------------------

namespace {

// Helper: bind a TCP listener on an ephemeral port. Returns (fd, port).
std::pair<int, uint16_t> bind_ephemeral_tcp() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_GE(fd, 0);
    int yes = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    EXPECT_EQ(::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
    EXPECT_EQ(::listen(fd, 8), 0);
    socklen_t len = sizeof(addr);
    EXPECT_EQ(::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len), 0);
    return {fd, ntohs(addr.sin_port)};
}

}  // namespace

TEST(OutputDomainProbe, TcpCheckSucceedsWhenListenerBound) {
    auto [fd, port] = bind_ephemeral_tcp();
    readiness::OutputDomainProbe probe("downstream",
                                       readiness::Endpoint::tcp("127.0.0.1:" + std::to_string(port)));
    EXPECT_EQ(probe.name(), "downstream");
    EXPECT_TRUE(probe.check());
    ::close(fd);
}

TEST(OutputDomainProbe, TcpCheckFailsWhenNothingListening) {
    // Bind, capture the port, then close — gives us a port nobody listens on.
    auto [fd, port] = bind_ephemeral_tcp();
    ::close(fd);
    readiness::OutputDomainProbe probe("downstream",
                                       readiness::Endpoint::tcp("127.0.0.1:" + std::to_string(port)));
    EXPECT_FALSE(probe.check());
}

TEST(OutputDomainProbe, TcpCheckFailsOnGarbageAddress) {
    readiness::OutputDomainProbe probe("garbage",
                                       readiness::Endpoint::tcp("not-a-host:notaport"));
    EXPECT_FALSE(probe.check());
}

TEST(OutputDomainProbe, UdsCheckFailsWhenSocketMissing) {
    readiness::OutputDomainProbe probe("uds-downstream",
                                       readiness::Endpoint::uds("/tmp/this-socket-does-not-exist-12345.sock"));
    EXPECT_FALSE(probe.check());
}

// ---------------------------------------------------------------------------
// BusProbe (#74)
// ---------------------------------------------------------------------------

TEST(BusProbe, FromEnvReturnsNullWhenUnset) {
    EnvGuard g(readiness::ENV_BUS_ENDPOINT);
    g.unset();
    auto probe = readiness::BusProbe::from_env();
    EXPECT_EQ(probe, nullptr);
}

TEST(BusProbe, FromEnvReturnsNullWhenBlank) {
    EnvGuard g(readiness::ENV_BUS_ENDPOINT);
    g.set("");
    auto probe = readiness::BusProbe::from_env();
    EXPECT_EQ(probe, nullptr);
}

TEST(BusProbe, FromEnvParsesTcpEndpoint) {
    EnvGuard g(readiness::ENV_BUS_ENDPOINT);
    g.set("kafka.bus.svc:9092");
    auto probe = readiness::BusProbe::from_env();
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->name(), "bus");
}

TEST(BusProbe, FromEnvParsesUnixEndpoint) {
    EnvGuard g(readiness::ENV_BUS_ENDPOINT);
    g.set("unix:/var/run/bus.sock");
    auto probe = readiness::BusProbe::from_env();
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->name(), "bus");
}

// ---------------------------------------------------------------------------
// Supervisor tick — aggregation (#68)
// ---------------------------------------------------------------------------

namespace {

class StaticProbe : public readiness::Probe {
   public:
    StaticProbe(std::string name, bool result) : name_(std::move(name)), result_(result) {}
    const std::string& name() const override { return name_; }
    bool check() override {
        ++calls;
        return result_;
    }
    std::atomic<int> calls{0};

   private:
    std::string name_;
    bool result_;
};

class ThrowingProbe : public readiness::Probe {
   public:
    explicit ThrowingProbe(std::string name) : name_(std::move(name)) {}
    const std::string& name() const override { return name_; }
    bool check() override { throw std::runtime_error("boom"); }

   private:
    std::string name_;
};

class SlowProbe : public readiness::Probe {
   public:
    SlowProbe(std::string name, std::chrono::milliseconds delay)
        : name_(std::move(name)), delay_(delay) {}
    const std::string& name() const override { return name_; }
    bool check() override {
        std::this_thread::sleep_for(delay_);
        return true;
    }

   private:
    std::string name_;
    std::chrono::milliseconds delay_;
};

}   // namespace

TEST(SupervisorTick, ReturnsTrueWhenAllProbesOk) {
    std::vector<std::shared_ptr<readiness::Probe>> probes = {
        std::make_shared<StaticProbe>("a", true),
        std::make_shared<StaticProbe>("b", true),
    };
    EXPECT_TRUE(readiness::supervisor_tick(probes, 100ms));
}

TEST(SupervisorTick, ReturnsFalseWhenAnyProbeReturnsFalse) {
    std::vector<std::shared_ptr<readiness::Probe>> probes = {
        std::make_shared<StaticProbe>("a", true),
        std::make_shared<StaticProbe>("b", false),
    };
    EXPECT_FALSE(readiness::supervisor_tick(probes, 100ms));
}

TEST(SupervisorTick, SurvivesThrowingProbeAndReturnsFalse) {
    // Audit #82: a throwing probe must not unwind the supervisor;
    // the tick reports the failure and the supervisor keeps running.
    std::vector<std::shared_ptr<readiness::Probe>> probes = {
        std::make_shared<StaticProbe>("ok", true),
        std::make_shared<ThrowingProbe>("bad"),
        std::make_shared<StaticProbe>("also-ok", true),
    };
    EXPECT_FALSE(readiness::supervisor_tick(probes, 100ms));
}

TEST(SupervisorTick, TreatsSlowProbeAsFailureViaTimeout) {
    std::vector<std::shared_ptr<readiness::Probe>> probes = {
        std::make_shared<SlowProbe>("slow", 200ms),
    };
    EXPECT_FALSE(readiness::supervisor_tick(probes, 20ms));
}

// ---------------------------------------------------------------------------
// Per-cause WARN logging (#82)
// ---------------------------------------------------------------------------

TEST(SupervisorTick, EmitsCauseWarnForReturnedFalse) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    std::vector<std::shared_ptr<readiness::Probe>> probes = {
        std::make_shared<StaticProbe>("downstream", false),
    };
    EXPECT_FALSE(readiness::supervisor_tick(probes, 100ms));

    bool saw_failed = false;
    for (const auto& r : cap->records()) {
        if (r.event == "readiness_probe_failed" && r.fields.at("probe") == "downstream") {
            EXPECT_EQ(r.level, LogLevel::WARN);
            saw_failed = true;
        }
    }
    EXPECT_TRUE(saw_failed) << "expected a readiness_probe_failed WARN log for the failing probe";

    set_default_logger(original);
}

TEST(SupervisorTick, EmitsCauseWarnForThrowingProbe) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    std::vector<std::shared_ptr<readiness::Probe>> probes = {
        std::make_shared<ThrowingProbe>("crashy"),
    };
    EXPECT_FALSE(readiness::supervisor_tick(probes, 100ms));

    bool saw_panic = false;
    for (const auto& r : cap->records()) {
        if (r.event == "readiness_probe_panicked" && r.fields.at("probe") == "crashy") {
            EXPECT_EQ(r.level, LogLevel::WARN);
            EXPECT_NE(r.fields.find("error"), r.fields.end());
            saw_panic = true;
        }
    }
    EXPECT_TRUE(saw_panic) << "expected a readiness_probe_panicked WARN log";

    set_default_logger(original);
}

TEST(SupervisorTick, EmitsCauseWarnForTimeout) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    std::vector<std::shared_ptr<readiness::Probe>> probes = {
        std::make_shared<SlowProbe>("slowpoke", 200ms),
    };
    EXPECT_FALSE(readiness::supervisor_tick(probes, 20ms));

    bool saw_timeout = false;
    for (const auto& r : cap->records()) {
        if (r.event == "readiness_probe_timed_out" && r.fields.at("probe") == "slowpoke") {
            EXPECT_EQ(r.level, LogLevel::WARN);
            saw_timeout = true;
        }
    }
    EXPECT_TRUE(saw_timeout) << "expected a readiness_probe_timed_out WARN log";

    set_default_logger(original);
}
