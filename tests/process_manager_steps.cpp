// Step definitions for features/client/process_manager.feature.
//
// WIP scaffolding for the process-manager dispatch feature: source-domain
// filtering, state rebuild from process events, and command emission.
// Every matcher below is a FAILING stub until real implementations are
// wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a process manager \"([^\"]*)\" for the fulfillment domain$") {
  REGEX_PARAM(std::string, name);
  (void)name;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the PM sources from \"([^\"]*)\" and \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, source_a);
  REGEX_PARAM(std::string, source_b);
  (void)source_a;
  (void)source_b;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the PM targets \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, target);
  (void)target;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the PM tracks the number of orders seen$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^OrderCompleted advances the orders-seen count$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the PM handles OrderCreated by emitting a ReserveStock command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Fulfillment is the active process manager$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^process state events: OrderCompleted, OrderCompleted$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^an OrderCreated trigger is dispatched to the PM router$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^a StockReserved trigger with a domain outside sources is dispatched$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the response contains exactly one command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the PM has seen (\\d+) completed orders$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response contains no commands$") {
  FAIL() << "WIP: step needs implementation";
}
