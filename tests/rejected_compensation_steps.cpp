// Step definitions for features/client/rejected_compensation.feature.
//
// WIP scaffolding for the rejection-compensation detail feature: state
// rebuild before compensation, (source_domain, command) routing,
// multi-handler isolation, sequence stamping, and empty-handler case.
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
GIVEN("^a command handler \"([^\"]*)\" for domain \"([^\"]*)\" with stateful rejection$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  (void)name;
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^deposits update Payment's bankroll$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment compensates a rejected ReserveStock from inventory by emitting FundsReleased with the current bankroll$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment is configured$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a prior history with a FundsDeposited event of bankroll (\\d+)$") {
  REGEX_PARAM(int64_t, bankroll);
  (void)bankroll;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a command handler \"([^\"]*)\" for domain \"([^\"]*)\" with two compensation handlers$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  (void)name;
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment compensates a rejected ReserveStock from inventory by emitting FundsReleased$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment compensates a rejected ProcessPayment from payment by emitting WorkflowFailed$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Payment compensates a rejected ReserveStock from inventory by emitting two FundsReleased events$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a prior history ending at sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a command handler \"([^\"]*)\" for domain \"([^\"]*)\" with no rejection handlers$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  (void)name;
  (void)domain;
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
WHEN("^a rejection of ProcessPayment arrives from payment$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^a rejection of CreateShipment arrives from fulfillment$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the response contains one FundsReleased event$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the FundsReleased event carries amount (\\d+)$") {
  REGEX_PARAM(int64_t, amount);
  (void)amount;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response contains one WorkflowFailed event$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^no FundsReleased event is emitted$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response contains no events$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^compensation events are appended after sequence (\\d+), taking sequences (\\d+) and (\\d+)$") {
  REGEX_PARAM(int64_t, base);
  REGEX_PARAM(int64_t, a);
  REGEX_PARAM(int64_t, b);
  (void)base;
  (void)a;
  (void)b;
  FAIL() << "WIP: step needs implementation";
}
