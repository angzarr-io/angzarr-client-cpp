// Step definitions for features/client/saga.feature.
//
// WIP scaffolding for the saga-dispatch feature: source-to-target
// translation, no-match silence, destination sequence stamping, and
// multi-target independence. Every matcher below is a FAILING stub
// until real implementations are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a saga \"([^\"]*)\" translating from \"([^\"]*)\" to \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, source);
  REGEX_PARAM(std::string, target);
  (void)name;
  (void)source;
  (void)target;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the saga handles OrderCreated by emitting a ReserveStock command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the router is built with the OrderFulfillment saga$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^destination sequences inventory=(\\d+) and fulfillment=(\\d+)$") {
  REGEX_PARAM(int64_t, inv_seq);
  REGEX_PARAM(int64_t, ful_seq);
  (void)inv_seq;
  (void)ful_seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a saga \"([^\"]*)\" translating from \"([^\"]*)\" to \"([^\"]*)\" and \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, source);
  REGEX_PARAM(std::string, target_a);
  REGEX_PARAM(std::string, target_b);
  (void)name;
  (void)source;
  (void)target_a;
  (void)target_b;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the saga handles OrderCreated by emitting a ReserveStock for \"([^\"]*)\" and a CreateShipment for \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, target_a);
  REGEX_PARAM(std::string, target_b);
  (void)target_a;
  (void)target_b;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^an OrderCreated event is dispatched to the saga router$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^a StockReserved event is dispatched to the saga router$") {
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
THEN("^the command targets the \"([^\"]*)\" domain$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response contains no commands$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the saga observed destination inventory = (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the saga observed destination fulfillment = (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the ReserveStock command carries destination sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the CreateShipment command carries destination sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}
