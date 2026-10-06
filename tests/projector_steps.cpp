// Step definitions for features/client/projector.feature.
//
// WIP scaffolding for the projector dispatch feature: per-event handler
// fan-out, unknown-type skipping, source-domain filtering, and instance
// reuse across one delivery. Every matcher below is a FAILING stub until
// real implementations are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a projector \"([^\"]*)\" consuming domains \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domains);
  (void)name;
  (void)domains;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the projector handles OrderCreated by appending to a write log$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Output is the active projector$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^an EventBook with three OrderCreated events is dispatched$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^an EventBook mixing OrderCreated and OrderCompleted is dispatched$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^an EventBook in domain \"([^\"]*)\" is dispatched$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^an EventBook with five OrderCreated events is dispatched$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the write log contains (\\d+) entries$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the write log contains only OrderCreated entries$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the write log remains empty$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^every entry was appended by the same projector instance$") {
  FAIL() << "WIP: step needs implementation";
}
