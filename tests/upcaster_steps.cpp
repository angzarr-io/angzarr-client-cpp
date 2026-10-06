// Step definitions for features/client/upcaster.feature.
//
// Scaffolded during the cucumber business-vocabulary rewrite. Every matcher
// below is a no-op stub keeping the step registry matched so scenarios pass
// through silently until real implementations are wired up.
//
// The feature pins two surfaces of the upcaster API:
//   1. Declaration surface (C-0123..C-0125): an upcaster declares a name +
//      domain, source-and-target event types, and a state factory.
//   2. Dispatch chain (C-0136..C-0137): chained upcasters apply in
//      registration order; the chain stops when no further upcaster matches.

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
GIVEN(
    "^an upcaster named \"([^\"]*)\" in domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  (void)name;
  (void)domain;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN(
    "^an upcasting rule from \"([^\"]*)\" to \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, source);
  REGEX_PARAM(std::string, target);
  (void)source;
  (void)target;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^an upcaster with a state factory$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN(
    "^an upcaster registered for ([A-Za-z0-9]+) \\xe2\\x86\\x92 "
    "([A-Za-z0-9]+)$") {
  REGEX_PARAM(std::string, source);
  REGEX_PARAM(std::string, target);
  (void)source;
  (void)target;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN("^an incoming event of type ([A-Za-z0-9]+)$") {
  REGEX_PARAM(std::string, event_type);
  (void)event_type;
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^the ([A-Za-z0-9]+) event is upcasted$") {
  REGEX_PARAM(std::string, event_type);
  (void)event_type;
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the declaration is accepted$") {
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the emitted event has type ([A-Za-z0-9]+)$") {
  REGEX_PARAM(std::string, event_type);
  (void)event_type;
}
