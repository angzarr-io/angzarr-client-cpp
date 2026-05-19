// Spec HIGH-6.1 — C++ transport-mode + endpoint resolver parity with
// Py/Rs/Go/Ja/C#.

#include "angzarr/transport.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

using namespace angzarr::transport;
using namespace angzarr;

namespace {
struct EnvGuard {
    const char* name;
    std::string prior;
    bool had;
    explicit EnvGuard(const char* n) : name(n), had(false) {
        const char* v = std::getenv(n);
        if (v) {
            had = true;
            prior = v;
        }
        unsetenv(n);
    }
    void set(const std::string& value) { setenv(name, value.c_str(), 1); }
    ~EnvGuard() {
        if (had)
            setenv(name, prior.c_str(), 1);
        else
            unsetenv(name);
    }
};
}  // namespace

TEST(TransportMode, FromString_Standalone) {
    EXPECT_EQ(transport_mode_from_string("standalone"), TransportMode::Standalone);
}

TEST(TransportMode, FromString_Distributed) {
    EXPECT_EQ(transport_mode_from_string("distributed"), TransportMode::Distributed);
}

TEST(TransportMode, FromString_BadValue_LoudFailsWithCode) {
    // Audit #40: loud-fail with INVALID_TRANSPORT_MODE.
    try {
        transport_mode_from_string("Distrib");
        FAIL() << "expected throw";
    } catch (const InvalidArgumentError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::INVALID_TRANSPORT_MODE);
        EXPECT_EQ(std::string(e.what()), error_codes::messages::INVALID_TRANSPORT_MODE);
        EXPECT_EQ(e.details().at("input"), "Distrib");
        EXPECT_EQ(e.details().at("env_var"), "ANGZARR_MODE");
    }
}

TEST(TransportMode, FromEnv_Unset_ReturnsDefault) {
    EnvGuard g(ENV_MODE);
    EXPECT_EQ(transport_mode_from_env(), DEFAULT_TRANSPORT_MODE);
}

TEST(TransportMode, FromEnv_Empty_ReturnsDefault) {
    EnvGuard g(ENV_MODE);
    g.set("");
    EXPECT_EQ(transport_mode_from_env(), DEFAULT_TRANSPORT_MODE);
}

TEST(TransportMode, FromEnv_Standalone_ReturnsStandalone) {
    EnvGuard g(ENV_MODE);
    g.set("standalone");
    EXPECT_EQ(transport_mode_from_env(), TransportMode::Standalone);
}

TEST(TransportMode, FromEnv_BadValue_ThrowsWithCode) {
    EnvGuard g(ENV_MODE);
    g.set("Standalone");  // wrong case
    EXPECT_THROW(transport_mode_from_env(), InvalidArgumentError);
}

TEST(TransportParsePort, ValidPort) { EXPECT_EQ(parse_port("1310"), 1310u); }

TEST(TransportParsePort, ZeroAllowed) { EXPECT_EQ(parse_port("0"), 0u); }

TEST(TransportParsePort, MaxAllowed) { EXPECT_EQ(parse_port("65535"), 65535u); }

TEST(TransportParsePort, OutOfRange_ThrowsWithCode) {
    try {
        parse_port("65536");
        FAIL() << "expected throw";
    } catch (const InvalidArgumentError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::INVALID_PORT);
        EXPECT_EQ(e.details().at("input"), "65536");
        EXPECT_EQ(e.details().at("env_var"), "ANGZARR_CH_PORT");
    }
}

TEST(TransportParsePort, TrailingSpace_LoudFails) {
    // Audit #40: ``"1310 "`` must raise (operator typo). Matches Rust /
    // Python strict parse.
    EXPECT_THROW(parse_port("1310 "), InvalidArgumentError);
}

TEST(TransportParsePort, LeadingSpace_LoudFails) {
    EXPECT_THROW(parse_port(" 1310"), InvalidArgumentError);
}

TEST(TransportParsePort, Empty_LoudFails) {
    EXPECT_THROW(parse_port(""), InvalidArgumentError);
}

TEST(TransportParsePort, NonNumeric_LoudFails) {
    EXPECT_THROW(parse_port("abc"), InvalidArgumentError);
}

TEST(TransportDetectUds, AbsolutePath) {
    EXPECT_EQ(detect_uds_path("/tmp/angzarr/ch-x.sock").value(),
              "/tmp/angzarr/ch-x.sock");
}

TEST(TransportDetectUds, RelativeDotSlash) {
    EXPECT_EQ(detect_uds_path("./local.sock").value(), "./local.sock");
}

TEST(TransportDetectUds, UnixTripleSlash) {
    EXPECT_EQ(detect_uds_path("unix:///tmp/x.sock").value(), "/tmp/x.sock");
}

TEST(TransportDetectUds, UnixDoubleSlash) {
    EXPECT_EQ(detect_uds_path("unix://tmp/x.sock").value(), "tmp/x.sock");
}

TEST(TransportDetectUds, UnixSingleSlash) {
    EXPECT_EQ(detect_uds_path("unix:relative").value(), "relative");
}

TEST(TransportDetectUds, TcpEndpoint_ReturnsNullopt) {
    EXPECT_FALSE(detect_uds_path("localhost:1310").has_value());
}

TEST(TransportDetectUds, Empty_ReturnsNullopt) {
    EXPECT_FALSE(detect_uds_path("").has_value());
}

TEST(TransportDetectUds, SingleCharNotStartingWithSlash_ReturnsNullopt) {
    // Mutation guard: transport.cpp:72 uses ``endpoint.size() >= 2 &&
    // endpoint[0] == '.' && endpoint[1] == '/'``. Mutating ``>=`` to
    // ``>`` would let a 2-char "./..." through but reject a single-
    // char input — covered by this test.
    EXPECT_FALSE(detect_uds_path(".").has_value());
}

TEST(TransportDetectUds, TwoCharRelativeAccepted) {
    EXPECT_EQ(detect_uds_path("./").value(), "./");
}

TEST(ResolveCHEndpoint, StandaloneDefault_UsesDefaultBase) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    EXPECT_EQ(resolve_ch_endpoint("orders", TransportMode::Standalone),
              std::string("/tmp/angzarr/ch-orders.sock"));
}

TEST(ResolveCHEndpoint, StandaloneOverrideBase) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    EXPECT_EQ(resolve_ch_endpoint("orders", TransportMode::Standalone, std::string("/var/run")),
              std::string("/var/run/ch-orders.sock"));
}

TEST(ResolveCHEndpoint, StandaloneEnvBaseWinsOverArg) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    base.set("/env/path");
    EXPECT_EQ(resolve_ch_endpoint("orders", TransportMode::Standalone, std::string("/arg/path")),
              std::string("/env/path/ch-orders.sock"));
}

TEST(ResolveCHEndpoint, DistributedDefault) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    EXPECT_EQ(resolve_ch_endpoint("orders", TransportMode::Distributed),
              std::string("ch-orders.angzarr.svc:1310"));
}

TEST(ResolveCHEndpoint, DistributedOverrideNamespaceAndPort) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    EXPECT_EQ(resolve_ch_endpoint("orders", TransportMode::Distributed, std::nullopt,
                                  std::string("prod"), static_cast<uint16_t>(50051)),
              std::string("ch-orders.prod.svc:50051"));
}

TEST(ResolveCHEndpoint, DistributedEnvPortWinsOverArg) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    port.set("7777");
    EXPECT_EQ(resolve_ch_endpoint("orders", TransportMode::Distributed, std::nullopt, std::nullopt,
                                  static_cast<uint16_t>(50051)),
              std::string("ch-orders.angzarr.svc:7777"));
}

TEST(ResolveCHEndpoint, DistributedEnvPortLoudFailsOnTypo) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    port.set("not-a-number");
    EXPECT_THROW(resolve_ch_endpoint("orders", TransportMode::Distributed),
                 InvalidArgumentError);
}

TEST(ResolveCHEndpoint, NullModeDelegatesToEnv) {
    EnvGuard mode(ENV_MODE), base(ENV_UDS_BASE), ns(ENV_NAMESPACE), port(ENV_CH_PORT);
    mode.set("standalone");
    EXPECT_EQ(resolve_ch_endpoint("orders").substr(0, std::string("/tmp/").size()),
              std::string("/tmp/"));
}

TEST(TransportConstants, MatchCanonicalStrings) {
    EXPECT_STREQ(ENV_MODE, "ANGZARR_MODE");
    EXPECT_STREQ(ENV_UDS_BASE, "ANGZARR_UDS_BASE");
    EXPECT_STREQ(ENV_NAMESPACE, "ANGZARR_NAMESPACE");
    EXPECT_STREQ(ENV_CH_PORT, "ANGZARR_CH_PORT");
    EXPECT_STREQ(DEFAULT_UDS_BASE, "/tmp/angzarr");
    EXPECT_STREQ(DEFAULT_NAMESPACE, "angzarr");
    EXPECT_EQ(DEFAULT_CH_PORT, 1310u);
}
