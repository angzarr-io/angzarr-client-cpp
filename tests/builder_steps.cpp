// Step definitions for features/client/builder.feature.
//
// WIP scaffolding for the handler-routing builder feature. Each matcher
// below is a FAILING stub so unimplemented scenarios surface as work in
// progress until real assertions are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an empty handler configuration$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a component that has not been marked as a handler kind$") {
  FAIL() << "WIP: step needs implementation";
}

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
GIVEN("^another command handler \"([^\"]*)\" for domain \"([^\"]*)\" with ([A-Za-z]+) state$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, state_kind);
  (void)name;
  (void)domain;
  (void)state_kind;
  FAIL() << "WIP: step needs implementation";
}

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
GIVEN("^two command handlers ([A-Za-z]+) and ([A-Za-z]+) for domain \"([^\"]*)\" both handling the same command$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  REGEX_PARAM(std::string, domain);
  (void)alpha;
  (void)beta;
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the handler reports how many times it has been introspected$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^I build the router$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I attempt to register it$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I register the handler and build the router$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the configuration is rejected because no handlers are registered$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the configuration is rejected because the component is not a recognised handler$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the result routes commands to their handlers$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the configuration is rejected for mixing handler kinds$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the configuration is rejected because two command handlers share the same domain and command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the handler has been introspected exactly once$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the result routes saga notifications to their handlers$") {
  FAIL() << "WIP: step needs implementation";
}
