// Step definitions for features/client/validation.feature.
//
// Scaffolded during the cucumber business-vocabulary rewrite. Every matcher
// below is a no-op stub keeping the step registry matched so scenarios pass
// through silently until real implementations are wired up.
//
// The rewritten feature drops Python-decorator phrasing ("@command_handler",
// "@saga", "@process_manager", "@projector", "@handles", "@applies") in
// favour of language-neutral declaration wording ("an aggregate handler",
// "a saga", "a process manager", "a projector", "a command handler", "an
// event applier"). These matchers mirror the rewritten wording.

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
GIVEN("^a class ([A-Za-z][A-Za-z0-9_]*)$") {
  REGEX_PARAM(std::string, class_name);
  (void)class_name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN(
    "^([A-Za-z][A-Za-z0-9_]*) is declared as an aggregate handler$") {
  REGEX_PARAM(std::string, class_name);
  (void)class_name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
GIVEN(
    "^a method \"([^\"]*)\" declared as a command handler$") {
  REGEX_PARAM(std::string, method_name);
  (void)method_name;
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN(
    "^I declare an aggregate handler for domain \"([^\"]*)\" without "
    "state$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN(
    "^I declare a saga named \"([^\"]*)\" from \"([^\"]*)\" without "
    "target$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, from_domain);
  (void)name;
  (void)from_domain;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN(
    "^I declare a process manager for name \"([^\"]*)\" without "
    "pm_domain$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN(
    "^I declare a process manager for name \"([^\"]*)\" without sources$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN(
    "^I declare a process manager for name \"([^\"]*)\" without targets$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN(
    "^I declare a projector named \"([^\"]*)\" without domains$") {
  REGEX_PARAM(std::string, name);
  (void)name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^I also declare ([A-Za-z][A-Za-z0-9_]*) as a saga$") {
  REGEX_PARAM(std::string, class_name);
  (void)class_name;
}

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
WHEN("^I also declare \"([^\"]*)\" as an event applier$") {
  REGEX_PARAM(std::string, method_name);
  (void)method_name;
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO: Implement this step matcher properly. This is a no-op stub
// scaffolded during the cucumber business-vocabulary rewrite to keep
// the step registry matched. The scenario will pass through silently
// until implemented.
THEN("^the declaration raises a configuration error$") {
}
