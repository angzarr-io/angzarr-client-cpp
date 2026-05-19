// Spec HIGH-4.2 / HIGH-4.3 / HIGH-4.4 convergence — C++ UnifiedRouter
// + BuildError + Built parity with Py/Rs/Ja/Cs.

#include "angzarr/unified_router.hpp"

#include <gtest/gtest.h>

#include <string>

#include "angzarr/component_router.hpp"
#include "angzarr/error_codes.hpp"
#include "angzarr/handler_traits.hpp"
#include "angzarr/router.hpp"
#include "angzarr/upcaster.hpp"

using namespace angzarr;

// ──────────────────────────────────────────────────────────────────────────────
// Minimal test handler types — concrete shapes satisfying the
// CommandHandlerDomainHandler / SagaDomainHandler base contracts so we
// can instantiate the typed routers the UnifiedRouter wraps.
// ──────────────────────────────────────────────────────────────────────────────
namespace {

struct DummyState {};

class DummyCmdHandler : public CommandHandlerDomainHandler<DummyState> {
   public:
    std::vector<std::string> command_types() const override { return {"Dummy"}; }
    const StateRouter<DummyState>& state_router() const override { return router_; }
    EventBook handle(const CommandBook&, const google::protobuf::Any&, const DummyState&,
                     int) override {
        return EventBook{};
    }

   private:
    StateRouter<DummyState> router_{[]() { return DummyState{}; }};
};

class DummyCmdHandlerB : public CommandHandlerDomainHandler<DummyState> {
   public:
    std::vector<std::string> command_types() const override { return {"DummyB"}; }
    const StateRouter<DummyState>& state_router() const override { return router_; }
    EventBook handle(const CommandBook&, const google::protobuf::Any&, const DummyState&,
                     int) override {
        return EventBook{};
    }

   private:
    StateRouter<DummyState> router_{[]() { return DummyState{}; }};
};

class DummySagaHandler : public SagaDomainHandler {
   public:
    std::vector<std::string> event_types() const override { return {"Started"}; }
    SagaHandlerResponse execute(const EventBook&, const google::protobuf::Any&,
                                const Destinations&) override {
        return SagaHandlerResponse::empty();
    }
};

}  // namespace

// ─── HIGH-4.3 BuildError shape ────────────────────────────────────────────────

TEST(BuildErrorShape, IsClientError) {
    BuildError e("msg", "CODE", {{"k", "v"}});
    EXPECT_EQ(std::string(e.what()), "msg");
    EXPECT_EQ(e.code(), "CODE");
    EXPECT_EQ(e.details().at("k"), "v");
}

TEST(BuildErrorShape, IsCatchableAsClientError) {
    try {
        throw BuildError("msg", "CODE");
    } catch (const ClientError& ce) {
        EXPECT_EQ(ce.code(), "CODE");
    }
}

// ─── HIGH-4.4 Built shape ─────────────────────────────────────────────────────

TEST(BuiltShape, CarriesKindAndHandlerCount) {
    UnifiedRouter ur("test");
    CommandHandlerRouter<DummyState, DummyCmdHandler> ch_router("test", "orders",
                                                                DummyCmdHandler{});
    ur.add_command_handler(ch_router);
    auto built = ur.build();
    EXPECT_EQ(built.router_name, "test");
    EXPECT_EQ(built.kind, RouterKind::CommandHandler);
    EXPECT_EQ(built.handler_count, 1);
}

TEST(BuiltShape, FiveKindsHaveDistinctRouterKindValues) {
    EXPECT_NE(RouterKind::CommandHandler, RouterKind::Saga);
    EXPECT_NE(RouterKind::Saga, RouterKind::ProcessManager);
    EXPECT_NE(RouterKind::ProcessManager, RouterKind::Projector);
    EXPECT_NE(RouterKind::Projector, RouterKind::Upcaster);
}

TEST(BuiltShape, RouterKindNamesAreStable) {
    EXPECT_STREQ(router_kind_name(RouterKind::CommandHandler), "CommandHandler");
    EXPECT_STREQ(router_kind_name(RouterKind::Saga), "Saga");
    EXPECT_STREQ(router_kind_name(RouterKind::ProcessManager), "ProcessManager");
    EXPECT_STREQ(router_kind_name(RouterKind::Projector), "Projector");
    EXPECT_STREQ(router_kind_name(RouterKind::Upcaster), "Upcaster");
}

// ─── HIGH-4.4 UpcasterRouter recognition (was unreachable from Built before) ──

TEST(UnifiedRouter_HIGH44, AddUpcaster_BuildsAsUpcasterKind) {
    UnifiedRouter ur("upcaster-name");
    UpcasterRouter up("orders");
    ur.add_upcaster(up);
    auto built = ur.build();
    EXPECT_EQ(built.kind, RouterKind::Upcaster);
    EXPECT_EQ(built.handler_count, 1);
}

// ─── HIGH-4.4 Saga + outputs ──────────────────────────────────────────────────

TEST(UnifiedRouter_Saga, AddSaga_RegistersTargetDomain) {
    UnifiedRouter ur("saga-x-y");
    SagaRouter<DummySagaHandler> saga("saga-x-y", "x", "y", DummySagaHandler{});
    ur.add_saga(saga);
    auto built = ur.build();
    EXPECT_EQ(built.kind, RouterKind::Saga);
    ASSERT_EQ(built.output_domains.size(), 1u);
    EXPECT_EQ(built.output_domains[0], "y");
    EXPECT_EQ(built.sync_output_domains[0], "y");
}

// ─── Validation invariants ────────────────────────────────────────────────────

TEST(UnifiedRouter_BuildValidation, EmptyRouterStampsRouterNoHandlers) {
    UnifiedRouter ur("empty");
    try {
        ur.build();
        FAIL() << "expected throw";
    } catch (const BuildError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::ROUTER_NO_HANDLERS);
        EXPECT_EQ(std::string(e.what()), error_codes::messages::ROUTER_NO_HANDLERS);
        EXPECT_EQ(e.details().at("router_name"), "empty");
    }
}

TEST(UnifiedRouter_BuildValidation, MixedKindsStampsMixedHandlerKinds) {
    UnifiedRouter ur("mixed");
    CommandHandlerRouter<DummyState, DummyCmdHandler> ch_router("router-x", "orders",
                                                                DummyCmdHandler{});
    ur.add_command_handler(ch_router);
    UpcasterRouter up("orders");
    try {
        ur.add_upcaster(up);
        FAIL() << "expected throw";
    } catch (const BuildError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::MIXED_HANDLER_KINDS);
        EXPECT_EQ(e.details().at("handler_kind"), "CommandHandler");
        EXPECT_EQ(e.details().at("other_kind"), "Upcaster");
    }
}

TEST(UnifiedRouter_BuildValidation, DuplicateCommandHandlerStampsCode) {
    UnifiedRouter ur("dup");
    CommandHandlerRouter<DummyState, DummyCmdHandler> r1("r1", "orders", DummyCmdHandler{});
    CommandHandlerRouter<DummyState, DummyCmdHandler> r2("r2", "orders", DummyCmdHandler{});
    ur.add_command_handler(r1);
    try {
        ur.add_command_handler(r2);
        FAIL() << "expected throw on duplicate (domain, handler-type)";
    } catch (const BuildError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::DUPLICATE_COMMAND_HANDLER);
        EXPECT_EQ(e.details().at("router_name"), "dup");
        EXPECT_EQ(e.details().at("domain"), "orders");
    }
}

TEST(UnifiedRouter_HandlerCount, IncrementsByOnePerAdd) {
    // Mutation guard: unified_router.hpp:155 uses ``++handler_count_``.
    // A ``--`` mutation would make handler_count negative or zero.
    UnifiedRouter ur("counts");
    UpcasterRouter u1("d1"), u2("d2"), u3("d3");
    ur.add_upcaster(u1);
    EXPECT_EQ(ur.build().handler_count, 1);
    ur.add_upcaster(u2);
    EXPECT_EQ(ur.build().handler_count, 2);
    ur.add_upcaster(u3);
    EXPECT_EQ(ur.build().handler_count, 3);
}

TEST(UnifiedRouter_BuildValidation, SameHandlerDifferentDomain_NoDuplicate) {
    UnifiedRouter ur("dist");
    CommandHandlerRouter<DummyState, DummyCmdHandler> r1("r1", "orders", DummyCmdHandler{});
    CommandHandlerRouter<DummyState, DummyCmdHandlerB> r2("r2", "inventory",
                                                          DummyCmdHandlerB{});
    ur.add_command_handler(r1);
    EXPECT_NO_THROW(ur.add_command_handler(r2));
    auto built = ur.build();
    EXPECT_EQ(built.handler_count, 2);
}
