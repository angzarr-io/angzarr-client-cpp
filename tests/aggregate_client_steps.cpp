// Step definitions for features/client/aggregate_client.feature.
//
// WIP scaffolding for the new aggregate-client business-vocabulary feature.
// Every matcher below is a FAILING stub so untouched scenarios surface
// loudly as work in progress. Replace `FAIL()` with real assertions as
// each step gains a real implementation.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a client connected to the test backend$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a new aggregate root in domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" at sequence (\\d+)$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, seq);
  (void)domain;
  (void)root;
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  (void)domain;
  (void)root;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^projectors are configured for \"([^\"]*)\" domain$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^sagas are configured for \"([^\"]*)\" domain$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the aggregate service is unavailable$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the aggregate service does not respond in time$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^no aggregate exists for domain \"([^\"]*)\" root \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  (void)domain;
  (void)root;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a \"([^\"]*)\" command with data \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, cmd);
  REGEX_PARAM(std::string, data);
  (void)cmd;
  (void)data;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send an \"([^\"]*)\" command at sequence (\\d+)$") {
  REGEX_PARAM(std::string, cmd);
  REGEX_PARAM(int64_t, seq);
  (void)cmd;
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command tagged with correlation ID \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, corr);
  (void)corr;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command at sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^two commands are sent concurrently at sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I look up the current sequence for \"([^\"]*)\" root \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  (void)domain;
  (void)root;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I retry the command at that sequence$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command without waiting for downstream work$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command and wait for projectors$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command and wait for downstream sagas$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command with a malformed payload$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command missing required fields$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command to domain \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  (void)domain;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command that produces (\\d+) events$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I read back the events for \"([^\"]*)\" root \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  (void)domain;
  (void)root;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I attempt to send a command$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a command with a short timeout$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I send a \"([^\"]*)\" command for root \"([^\"]*)\" at sequence (\\d+)$") {
  REGEX_PARAM(std::string, cmd);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, seq);
  (void)cmd;
  (void)root;
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the command is accepted$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^a single \"([^\"]*)\" event is recorded$") {
  REGEX_PARAM(std::string, event);
  (void)event;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the new events continue the history from sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the resulting events carry correlation ID \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, corr);
  (void)corr;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the command is refused because the aggregate has moved on$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^one command is accepted$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the other is refused because the aggregate has moved on$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response returns before any projectors have caught up$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response reflects the projectors having processed the event$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the response reflects the downstream sagas having completed$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the command is refused as invalid$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the refusal names the missing field$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the command is refused because the domain is unknown$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^(\\d+) events are recorded$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the events occupy consecutive sequences starting at (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^either all (\\d+) events are present or none of them are$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the call fails because the service cannot be reached$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the call fails because the deadline was exceeded$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the aggregate now exists with one event$") {
  FAIL() << "WIP: step needs implementation";
}
