// Step definitions for features/client/command_handler.feature.
//
// WIP scaffolding for the command-handler dispatch feature. Every matcher
// below is a FAILING stub until real implementations are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a command handler \"([^\"]*)\" for domain \"([^\"]*)\" with ([A-Za-z]+) state$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, state_kind);
  (void)name;
  (void)domain;
  (void)state_kind;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^OrderCreated marks the order as created$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^CreateOrder emits OrderCreated$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Order is the active aggregate handler$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a prior history with an OrderCreated event at sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a command handler whose handler returns None for CreateOrder$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the aggregate supplies its own initial state with created = true$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Order handles CreateOrder by emitting OrderCreated only when the order is already created$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the aggregate does not supply its own initial state$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^Order handles CreateOrder by reading whether the order is created$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^no prior events in the incoming ContextualCommand$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the incoming command has cover\\.ext set to a packed parent Cover$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a command handler whose emit step sets EventBook cover\\.ext explicitly$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the incoming command also has a different cover\\.ext set$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the incoming command's cover has no ext field set$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^CreateOrder\\(order_id=\"([^\"]*)\"\\) is dispatched$") {
  REGEX_PARAM(std::string, order_id);
  (void)order_id;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^a command is dispatched against the aggregate$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^CompleteOrder\\(order_id=\"([^\"]*)\"\\) is dispatched$") {
  REGEX_PARAM(std::string, order_id);
  (void)order_id;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the response emits an OrderCreated event$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the emitted event sequence is (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the order is treated as already created$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the unknown command is rejected as invalid input$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^when the handler emits nothing, no events are produced$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the handler observes that the order is not created$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response's EventBook cover\\.ext is the same packed parent Cover$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response's EventBook cover\\.ext is the handler-set value$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response's EventBook cover has no ext field set$") {
  FAIL() << "WIP: step needs implementation";
}
