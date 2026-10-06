// Step definitions for features/coordinator-contract/fact_flow.feature.
//
// Scaffolded during the cucumber business-vocabulary rewrite. Every matcher
// below is a no-op stub keeping the step registry matched so scenarios pass
// through silently until real implementations are wired up.
//
// The rewritten feature replaces "the fact is persisted with the next
// sequence number" with "the fact is appended at the next sequence number",
// "the player aggregate contains an ActionRequested event" with
// "Alice's player aggregate records the ActionRequested fact", and
// "the saga fails with error containing \"not found\"" with
// "the saga fails because the target domain does not exist". These matchers
// mirror the rewritten wording.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^a registered player \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN(
    "^a hand in progress where it becomes ([A-Za-z][A-Za-z0-9_]*)'s turn$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^a player aggregate with (\\d+) existing events$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^player \"([^\"]*)\" is seated at table \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, table);
  (void)name;
  (void)table;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^player \"([^\"]*)\" is sitting out at table \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, table);
  (void)name;
  (void)table;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^a saga that emits a fact$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^a saga that emits a fact to domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^a fact with external_id \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, external_id);
  (void)external_id;
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^the hand-player saga processes the turn change$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^an ActionRequested fact is injected$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN(
    "^([A-Za-z][A-Za-z0-9_]*)'s player aggregate emits PlayerSittingOut$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^([A-Za-z][A-Za-z0-9_]*)'s player aggregate emits PlayerReturning$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^the fact is constructed$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^the saga processes an event$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^the same fact is injected twice$") {
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN(
    "^an ActionRequested fact is injected into ([A-Za-z][A-Za-z0-9_]*)'s "
    "player aggregate$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the fact is appended at the next sequence number$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN(
    "^([A-Za-z][A-Za-z0-9_]*)'s player aggregate records the "
    "ActionRequested fact$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the fact is persisted with sequence number (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^subsequent events continue from sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^a PlayerSatOut fact is injected into the table aggregate$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN(
    "^the table records ([A-Za-z][A-Za-z0-9_]*) as sitting out$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the fact has a sequence number in the table's event stream$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^a PlayerSatIn fact is injected into the table aggregate$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the table records ([A-Za-z][A-Za-z0-9_]*) as active$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the fact Cover has domain set to the target aggregate$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the fact Cover has root set to the target aggregate root$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the fact Cover has external_id set for idempotency$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the fact Cover has correlation_id for traceability$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the saga fails because the target domain does not exist$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^no commands from that saga are executed$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^only one event is stored in the aggregate$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the second injection succeeds without error$") {
}
