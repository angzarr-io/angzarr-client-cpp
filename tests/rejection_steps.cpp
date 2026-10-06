// Step definitions for features/client/rejection.feature.
//
// WIP scaffolding for the rejection-compensation feature: targeted
// compensation, fan-out across multiple compensators, and selective
// routing (rejection nobody subscribes to is a no-op). Every matcher
// below is a FAILING stub until real implementations are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment is a component in domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment compensates a rejected ReserveStock from inventory by releasing funds$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment is the active component$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a second compensation handler for the same rejection also releases funds$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment then Payment2 are configured$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^a rejection of ReserveStock arrives from inventory$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^a rejection of ProcessPayment arrives from inventory$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^a FundsReleased event is emitted$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^no events are emitted$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^two FundsReleased events are emitted in registration order$") {
  FAIL() << "WIP: step needs implementation";
}
