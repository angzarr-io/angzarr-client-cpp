// Spec HIGH-5.1 — C++ server module parity with Py/Rs/Ja/C#.
//
// Covers: ServerConfig + config_from_env, resolve_bind_address,
// cleanup_socket, get_transport_config, configure_logging, run_supervisor
// (audit #68/#74) + publish_shutdown_status (audit #83).

#include "angzarr/server.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "angzarr/logging.hpp"
#include "angzarr/readiness.hpp"

using namespace angzarr;
using angzarr::server::ServerConfig;

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

TEST(ServerConfig, ConstantsAreCanonical) {
    EXPECT_STREQ(server::ENV_BIND_ADDRESS, "ANGZARR_BIND_ADDRESS");
    EXPECT_STREQ(server::DEFAULT_BIND_HOST, "[::]");
    EXPECT_EQ(server::DEFAULT_PORT, 50052u);
}

TEST(ServerConfig, HealthNamesAreCanonical) {
    // Cross-language canonical per-kind health service names. Operators
    // querying ``grpc.health.v1.HealthCheckRequest{service=...}`` from
    // a probe agent see the same name regardless of which language
    // language the pod runs.
    EXPECT_STREQ(server::HEALTH_NAME_COMMAND_HANDLER,
                 "angzarr_client.proto.angzarr.v1.CommandHandlerCoordinatorService");
    EXPECT_STREQ(server::HEALTH_NAME_SAGA,
                 "angzarr_client.proto.angzarr.v1.SagaCoordinatorService");
    EXPECT_STREQ(server::HEALTH_NAME_PROCESS_MANAGER,
                 "angzarr_client.proto.angzarr.v1.ProcessManagerCoordinatorService");
    EXPECT_STREQ(server::HEALTH_NAME_PROJECTOR,
                 "angzarr_client.proto.angzarr.v1.ProjectorCoordinatorService");
    EXPECT_STREQ(server::HEALTH_NAME_UPCASTER,
                 "angzarr_client.proto.angzarr.v1.UpcasterCoordinatorService");
}

TEST(ResolveBindAddress, DefaultsToIPv6Wildcard) {
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT");
    // Audit #77: ``[::]`` is the IPv6 wildcard dual-stack default.
    EXPECT_EQ(server::resolve_bind_address(50052), "[::]:50052");
}

TEST(ResolveBindAddress, EnvOverrideWinsOverDefault) {
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT");
    bind.set("[::1]:12345");
    EXPECT_EQ(server::resolve_bind_address(50052), "[::1]:12345");
}

TEST(ResolveBindAddress, PortEnvUsedWhenNoOverride) {
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT");
    port.set("9000");
    EXPECT_EQ(server::resolve_bind_address(50052), "[::]:9000");
}

TEST(ConfigFromEnv, TcpDefault) {
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT"), gport("GRPC_PORT"),
        uds("UDS_BASE_PATH");
    auto cfg = server::config_from_env(50052);
    EXPECT_FALSE(cfg.uds_path.has_value());
    EXPECT_EQ(cfg.port, 50052u);
    EXPECT_EQ(cfg.bind_address, "[::]:50052");
}

TEST(ConfigFromEnv, EmptyPortEnvFallsThroughToDefault) {
    // Mutation guard: server.cpp:76 uses ``port_env == nullptr || *port_env == '\0'``
    // — both branches must lead to the default. Setting PORT="" must
    // behave identically to PORT-unset.
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT"), gport("GRPC_PORT"),
        uds("UDS_BASE_PATH");
    port.set("");
    auto cfg = server::config_from_env(50052);
    EXPECT_EQ(cfg.port, 50052u);
}

TEST(ConfigFromEnv, OutOfRangePortFallsThroughToDefault) {
    // Mutation guard: server.cpp:80 uses ``p >= 0 && p <= 65535``
    // — mutating either boundary should be caught by an out-of-range
    // input that must fall through to default_port.
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT"), gport("GRPC_PORT"),
        uds("UDS_BASE_PATH");
    port.set("999999");
    auto cfg = server::config_from_env(50052);
    EXPECT_EQ(cfg.port, 50052u);
}

TEST(ConfigFromEnv, NegativePortFallsThroughToDefault) {
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT"), gport("GRPC_PORT"),
        uds("UDS_BASE_PATH");
    port.set("-1");
    auto cfg = server::config_from_env(50052);
    EXPECT_EQ(cfg.port, 50052u);
}

TEST(ConfigFromEnv, TcpRespectsGrpcPortFallback) {
    EnvGuard bind(server::ENV_BIND_ADDRESS), port("PORT"), gport("GRPC_PORT"),
        uds("UDS_BASE_PATH");
    gport.set("60001");
    auto cfg = server::config_from_env(50052);
    EXPECT_EQ(cfg.port, 60001u);
}

TEST(ConfigFromEnv, UdsMode_WithDomain) {
    EnvGuard bind(server::ENV_BIND_ADDRESS), uds("UDS_BASE_PATH"), svc("SERVICE_NAME"),
        dom("DOMAIN");
    uds.set("/tmp/angzarr-test-cppconfig");
    svc.set("ch");
    dom.set("orders");
    auto cfg = server::config_from_env();
    ASSERT_TRUE(cfg.uds_path.has_value());
    EXPECT_EQ(*cfg.uds_path, "/tmp/angzarr-test-cppconfig/ch-orders.sock");
}

TEST(CleanupSocket, NoOpOnEmpty) { server::cleanup_socket(""); /* no throw */ }

TEST(CleanupSocket, NoOpOnMissing) {
    server::cleanup_socket("/tmp/__angzarr_definitely_not_there.sock");
}

TEST(CleanupSocket, RemovesExistingFile) {
    auto p = std::filesystem::temp_directory_path() / "angzarr-cleanup-test.sock";
    {
        std::FILE* f = std::fopen(p.string().c_str(), "w");
        ASSERT_NE(f, nullptr);
        std::fclose(f);
    }
    ASSERT_TRUE(std::filesystem::exists(p));
    server::cleanup_socket(p.string());
    EXPECT_FALSE(std::filesystem::exists(p));
}

TEST(GetTransportConfig, TcpDefault) {
    EnvGuard t("TRANSPORT_TYPE"), bind(server::ENV_BIND_ADDRESS);
    auto [transport, address] = server::get_transport_config(50052);
    EXPECT_EQ(transport, "tcp");
    EXPECT_EQ(address, "[::]:50052");
}

TEST(GetTransportConfig, UdsCreatesParentDirAndCleansStaleSocket) {
    EnvGuard t("TRANSPORT_TYPE"), base("UDS_BASE_PATH"), svc("SERVICE_NAME"),
        dom("DOMAIN");
    auto base_dir = std::filesystem::temp_directory_path() /
                    "angzarr-server-test-uds-side-effects";
    std::error_code ec;
    std::filesystem::remove_all(base_dir, ec);  // ensure clean state
    t.set("uds");
    base.set(base_dir.string());
    svc.set("ch");
    dom.set("orders");
    auto stale = base_dir / "ch-orders.sock";
    // Pre-create a "stale" socket file
    std::filesystem::create_directories(base_dir, ec);
    {
        std::FILE* f = std::fopen(stale.string().c_str(), "w");
        ASSERT_NE(f, nullptr);
        std::fclose(f);
    }
    auto [transport, address] = server::get_transport_config();
    EXPECT_EQ(transport, "uds");
    EXPECT_EQ(address, std::string("unix:") + stale.string());
    EXPECT_FALSE(std::filesystem::exists(stale));
    // Parent dir survives because create_directories was invoked.
    EXPECT_TRUE(std::filesystem::exists(base_dir));
    std::filesystem::remove_all(base_dir, ec);
}

TEST(ConfigureLogging, IdempotentAcrossMultipleCalls) {
    server::configure_logging();
    server::configure_logging();
    server::configure_logging();
    // No crash; absl init is single-call internally.
    SUCCEED();
}

namespace {
struct DummyOkProbe : readiness::Probe {
    std::string n_;
    DummyOkProbe() : n_("dummy") {}
    const std::string& name() const override { return n_; }
    bool check() override { return true; }
};
struct DummyFailProbe : readiness::Probe {
    std::string n_;
    DummyFailProbe() : n_("dummy-fail") {}
    const std::string& name() const override { return n_; }
    bool check() override { return false; }
};
}  // namespace

TEST(RunSupervisor, FlipsToServingWhenAllProbesGreen) {
    std::vector<std::shared_ptr<readiness::Probe>> probes;
    probes.push_back(std::make_shared<DummyOkProbe>());

    std::vector<std::pair<std::string, bool>> sets;
    server::HealthSetter setter = [&](const std::string& n, bool s) {
        sets.emplace_back(n, s);
    };

    std::atomic<int> ticks(0);
    auto stop = [&]() {
        // stop after first flip is observed.
        return ticks.load() > 0 && !sets.empty();
    };
    // tick loop runs once, sees ok, flips, marks ticks++, then stops.
    std::thread t([&]() {
        server::run_supervisor(probes, setter, {"", server::HEALTH_NAME_COMMAND_HANDLER},
                               std::chrono::seconds(0), std::chrono::milliseconds(50), stop);
    });
    // Bump ticks so the stop predicate flips after the supervisor's
    // first iteration.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ticks++;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    t.join();
    // Expect SERVING flip for both ``""`` and the kind name.
    bool found_empty_serving = false;
    bool found_kind_serving = false;
    for (auto& [n, s] : sets) {
        if (n.empty() && s) found_empty_serving = true;
        if (n == server::HEALTH_NAME_COMMAND_HANDLER && s) found_kind_serving = true;
    }
    EXPECT_TRUE(found_empty_serving);
    EXPECT_TRUE(found_kind_serving);
}

TEST(PublishShutdownStatus, FlipsAllRegisteredNamesToNotServing) {
    auto capture = std::make_shared<CaptureLogger>();
    auto previous = set_default_logger(capture);

    std::vector<std::pair<std::string, bool>> sets;
    server::HealthSetter setter = [&](const std::string& n, bool s) {
        sets.emplace_back(n, s);
    };
    server::publish_shutdown_status(setter, {"", server::HEALTH_NAME_SAGA}, "test-svc",
                                    "saga");
    EXPECT_EQ(sets.size(), 2u);
    EXPECT_FALSE(sets[0].second);
    EXPECT_FALSE(sets[1].second);

    auto records = capture->records();
    bool found_shutdown = false;
    for (auto& r : records) {
        if (r.event == "server_shutdown") {
            found_shutdown = true;
            EXPECT_EQ(r.fields.at("service"), "test-svc");
            EXPECT_EQ(r.fields.at("name"), "saga");
        }
    }
    EXPECT_TRUE(found_shutdown);

    set_default_logger(previous);
}
