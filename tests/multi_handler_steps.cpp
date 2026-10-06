// Step definitions for features/client/multi_handler.feature.
//
// WIP scaffolding for the multi-handler dispatch feature: CommandHandler
// uniqueness enforcement at build time, and legitimate saga / PM /
// projector fan-out. Every matcher below is a FAILING stub until real
// implementations are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^two command handlers ([A-Za-z]+) and ([A-Za-z]+) for domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  REGEX_PARAM(std::string, domain);
  (void)alpha;
  (void)beta;
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^both handle CreateOrder$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a command handler ([A-Za-z]+) for domain \"([^\"]*)\" handling CreateOrder$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  (void)name;
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a command handler ([A-Za-z]+) for domain \"([^\"]*)\" handling RegisterPlayer and DepositFunds$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, domain);
  (void)name;
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^two sagas ([A-Za-z]+) and ([A-Za-z]+) both listening to source \"([^\"]*)\" for OrderCreated$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  REGEX_PARAM(std::string, source);
  (void)alpha;
  (void)beta;
  (void)source;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^([A-Za-z]+) emits a ReserveStock command for \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, target);
  (void)name;
  (void)target;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^([A-Za-z]+) emits a CreateShipment command for \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(std::string, target);
  (void)name;
  (void)target;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the saga router is built with ([A-Za-z]+) then ([A-Za-z]+)$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  (void)alpha;
  (void)beta;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^two process managers ([A-Za-z]+) and ([A-Za-z]+) both sourcing from \"([^\"]*)\" and handling OrderCreated$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  REGEX_PARAM(std::string, source);
  (void)alpha;
  (void)beta;
  (void)source;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^([A-Za-z]+) emits a ReserveStock command$") {
  REGEX_PARAM(std::string, name);
  (void)name;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^([A-Za-z]+) emits a CreateShipment command$") {
  REGEX_PARAM(std::string, name);
  (void)name;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the PM router is built with ([A-Za-z]+) then ([A-Za-z]+)$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  (void)alpha;
  (void)beta;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^two projectors ([A-Za-z]+) and ([A-Za-z]+) both consuming domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  REGEX_PARAM(std::string, domain);
  (void)alpha;
  (void)beta;
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^([A-Za-z]+) appends to a log on OrderCreated$") {
  REGEX_PARAM(std::string, name);
  (void)name;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^([A-Za-z]+) appends to a different log on OrderCreated$") {
  REGEX_PARAM(std::string, name);
  (void)name;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the projector router is built with ([A-Za-z]+) then ([A-Za-z]+)$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  (void)alpha;
  (void)beta;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^the router is built with ([A-Za-z]+) then ([A-Za-z]+)$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  (void)alpha;
  (void)beta;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^the router is built with ([A-Za-z]+) then ([A-Za-z]+) across domains$") {
  REGEX_PARAM(std::string, alpha);
  REGEX_PARAM(std::string, beta);
  (void)alpha;
  (void)beta;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^the router is built with ([A-Za-z]+)$") {
  REGEX_PARAM(std::string, name);
  (void)name;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^an OrderCreated event is dispatched to the saga router$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^an OrderCreated trigger is dispatched to the PM router$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^an EventBook with one OrderCreated event is dispatched$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^registration is rejected because two command handlers claim CreateOrder in \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the configuration is accepted$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response contains two commands in registration order$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the first command targets the \"([^\"]*)\" domain$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the second command targets the \"([^\"]*)\" domain$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^([A-Za-z]+)'s log has (\\d+) entry$") {
  REGEX_PARAM(std::string, name);
  REGEX_PARAM(int64_t, count);
  (void)name;
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^each saga handles the event exactly once$") {
  FAIL() << "WIP: step needs implementation";
}
