// Step definitions for features/client/domain-client.feature.
//
// WIP scaffolding for the unified domain-client feature (single
// connection sharing read + write surfaces). Every matcher below is a
// FAILING stub until real implementations are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a running aggregate coordinator for domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a registered aggregate handler for domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" has (\\d+) events$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, count);
  (void)domain;
  (void)root;
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a connected domain client$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^environment variable \"([^\"]*)\" is set to the coordinator endpoint$") {
  REGEX_PARAM(std::string, var);
  (void)var;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^I create a domain client for the coordinator endpoint$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I create a domain client for domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I use the command builder to send a command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I use the query builder to fetch events for that root$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query for the resulting events$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I close the domain client$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I create a domain client from environment variable \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, var);
  (void)var;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^I should be able to query events$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I should be able to send commands$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I should receive a command response$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I should receive (\\d+) event pages$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^both operations should succeed on the same connection$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^subsequent commands should fail with a connection error$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^subsequent queries should fail with a connection error$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the domain client should be connected$") {
  FAIL() << "WIP: step needs implementation";
}
