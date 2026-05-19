// Tests for ProjectorDomainHandler catch-all + WARN log.
//
// Rust 804c362: projectors with a closed ``#[handles(T)]`` set
// silently dropped events whose type_url matched no arm. Two changes
// ported here:
//
// 1. Always log a WARN with the projector + type_url when no handler
//    matches, so unhandled events surface in production telemetry.
// 2. Optional catch-all handler — if the projector registers one via
//    ``set_unknown_handler``, the WARN still fires and dispatch
//    invokes that handler so user code can render or count unknowns.
//
// The C++ port doesn't have a proc-macro; ``ProjectorDomainHandler``
// gets an explicit ``set_unknown_handler(callback)`` instead of an
// attribute. Same behavioral contract.

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>

#include "angzarr/component_router.hpp"
#include "angzarr/handler_traits.hpp"
#include "angzarr/logging.hpp"
#include "angzarr/projector.hpp"

using namespace angzarr;

namespace {

class StrictHandler : public ProjectorDomainHandler {
   public:
    std::vector<std::string> event_types() const override {
        return {"player.PlayerRegistered"};
    }
    Projection project(const EventBook& /*events*/) override {
        ++project_calls;
        return Projection::upsert("key", "value");
    }
    std::atomic<int> project_calls{0};
};

EventBook event_book_with(const std::string& domain, const std::string& type_url) {
    EventBook book;
    book.mutable_cover()->set_domain(domain);
    auto* page = book.add_pages();
    page->mutable_header()->set_sequence(1);
    page->mutable_event()->set_type_url(type_url);
    return book;
}

}  // namespace

// ---------------------------------------------------------------------------
// WARN log fires on unknown event (#804c362 part 1)
// ---------------------------------------------------------------------------

TEST(ProjectorUnknown, WarnFiresWhenNoHandlerMatchesAndNoCatchAll) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    auto handler = std::make_shared<StrictHandler>();
    ProjectorRouter router("prj-output");
    router.domain("player", handler);

    // Event domain matches but type_url doesn't match any declared arm.
    auto book = event_book_with("player",
                                "type.googleapis.com/player.PlayerDeregistered");
    router.dispatch_or_warn(book);

    bool saw_warn = false;
    for (const auto& r : cap->records()) {
        if (r.event == "projector_unknown_event") {
            EXPECT_EQ(r.level, LogLevel::WARN);
            EXPECT_EQ(r.fields.at("projector"), "prj-output");
            EXPECT_EQ(r.fields.at("type_url"),
                      "type.googleapis.com/player.PlayerDeregistered");
            saw_warn = true;
        }
    }
    EXPECT_TRUE(saw_warn)
        << "expected a projector_unknown_event WARN for the unmatched type_url";

    set_default_logger(original);
}

// ---------------------------------------------------------------------------
// Catch-all hook fires after WARN (#804c362 part 2)
// ---------------------------------------------------------------------------

TEST(ProjectorUnknown, CatchAllHandlerInvokedAfterWarn) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    auto handler = std::make_shared<StrictHandler>();
    std::string captured_type_url;
    handler->set_unknown_handler(
        [&](const std::string& type_url) { captured_type_url = type_url; });

    ProjectorRouter router("prj-output");
    router.domain("player", handler);

    auto book = event_book_with("player",
                                "type.googleapis.com/player.PlayerDeregistered");
    router.dispatch_or_warn(book);

    // Both: warn fired, AND catch-all received the type URL.
    bool saw_warn = false;
    for (const auto& r : cap->records()) {
        if (r.event == "projector_unknown_event") saw_warn = true;
    }
    EXPECT_TRUE(saw_warn);
    EXPECT_EQ(captured_type_url, "type.googleapis.com/player.PlayerDeregistered");

    set_default_logger(original);
}

// ---------------------------------------------------------------------------
// Known events still route to ``project()`` (catch-all does not
// short-circuit normal dispatch).
// ---------------------------------------------------------------------------

TEST(ProjectorUnknown, MatchedEventsBypassCatchAll) {
    auto original = default_logger_shared();
    auto cap = std::make_shared<CaptureLogger>();
    set_default_logger(cap);

    auto handler = std::make_shared<StrictHandler>();
    int unknown_calls = 0;
    handler->set_unknown_handler([&](const std::string&) { ++unknown_calls; });

    ProjectorRouter router("prj-output");
    router.domain("player", handler);

    // Note: dispatch routes by domain, then ProjectorDomainHandler is
    // expected to handle event-type matching internally. The known
    // event matches the handler's declared event_types so the
    // catch-all must NOT be called.
    auto book = event_book_with("player",
                                "type.googleapis.com/player.PlayerRegistered");
    router.dispatch_or_warn(book);

    EXPECT_EQ(handler->project_calls, 1);
    EXPECT_EQ(unknown_calls, 0);

    // No unknown_event log expected.
    for (const auto& r : cap->records()) {
        EXPECT_NE(r.event, "projector_unknown_event");
    }

    set_default_logger(original);
}
