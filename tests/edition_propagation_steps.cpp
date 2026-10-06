// Step definitions for features/coordinator-contract/edition_propagation.feature.
//
// WIP stubs for cross-domain edition propagation across saga and
// process-manager emissions (C-0138..C-0145). Every matcher below FAILs
// so unimplemented scenarios are visible at run-time rather than passing
// silently.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN(
    "^a saga \"([^\"]*)\" translating from \"([^\"]*)\" to "
    "\"([^\"]*)\"$") {
  REGEX_PARAM(std::string, saga_name);
  REGEX_PARAM(std::string, from_domain);
  REGEX_PARAM(std::string, to_domain);
  (void)saga_name;
  (void)from_domain;
  (void)to_domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN(
    "^the saga handles OrderCreated by emitting a ReserveStock command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN(
    "^the saga handles OrderCreated by emitting an OrderObserved event$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the source event has edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the source event has no edition set$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN(
    "^the source event has edition \"([^\"]*)\" with divergence at "
    "\"([^\"]*)\"=(\\d+)$") {
  REGEX_PARAM(std::string, edition);
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(int64_t, sequence);
  (void)edition;
  (void)domain;
  (void)sequence;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the saga handler sets outgoing edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN(
    "^a process manager \"([^\"]*)\" with sources \"([^\"]*)\" and "
    "targets \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, pm_name);
  REGEX_PARAM(std::string, sources);
  REGEX_PARAM(std::string, targets);
  (void)pm_name;
  (void)sources;
  (void)targets;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the trigger event has edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN(
    "^the PM also emits an OrderTracked process_event on OrderCreated$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the PM handler sets outgoing edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^an OrderCreated event is dispatched to the saga$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^an OrderCreated trigger is dispatched to the PM$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the emitted command's cover has edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the emitted event's cover has edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the persisted command's cover has edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the emitted command's cover has no edition set$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN(
    "^the emitted command's cover has edition \"([^\"]*)\" with "
    "divergence at \"([^\"]*)\"=(\\d+)$") {
  REGEX_PARAM(std::string, edition);
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(int64_t, sequence);
  (void)edition;
  (void)domain;
  (void)sequence;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN(
    "^every emitted process_events book's cover has edition "
    "\"([^\"]*)\"$") {
  REGEX_PARAM(std::string, edition);
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}
