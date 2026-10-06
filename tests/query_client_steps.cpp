// Step definitions for features/client/query_client.feature.
//
// WIP scaffolding for the query-client feature: read-side event
// retrieval, range / temporal / edition / correlation queries, snapshot
// integration, and error handling. Every matcher below is a FAILING stub
// until real implementations are wired up.

// GTest must be included before cucumber-cpp autodetect for framework detection
#include <gtest/gtest.h>

#include <cucumber-cpp/autodetect.hpp>

using cucumber::ScenarioScope;

// ==========================================================================
// Given Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
GIVEN("^a query surface available$") {
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
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" has event \"([^\"]*)\" with data \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(std::string, event_type);
  REGEX_PARAM(std::string, data);
  (void)domain;
  (void)root;
  (void)event_type;
  (void)data;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" has events at known timestamps$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  (void)domain;
  (void)root;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" in edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(std::string, edition);
  (void)domain;
  (void)root;
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" has (\\d+) events in main$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, count);
  (void)domain;
  (void)root;
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" has (\\d+) events in edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, count);
  REGEX_PARAM(std::string, edition);
  (void)domain;
  (void)root;
  (void)count;
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^events with correlation ID \"([^\"]*)\" exist in multiple aggregates$") {
  REGEX_PARAM(std::string, corr);
  (void)corr;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^an aggregate \"([^\"]*)\" with root \"([^\"]*)\" has a snapshot at sequence (\\d+) and (\\d+) events$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, snap_seq);
  REGEX_PARAM(int64_t, count);
  (void)domain;
  (void)root;
  (void)snap_seq;
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
GIVEN("^the query service is unavailable$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// When Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events for \"([^\"]*)\" root \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  (void)domain;
  (void)root;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events for \"([^\"]*)\" root \"([^\"]*)\" from sequence (\\d+)$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, from_seq);
  (void)domain;
  (void)root;
  (void)from_seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events for \"([^\"]*)\" root \"([^\"]*)\" from sequence (\\d+) to (\\d+)$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, from_seq);
  REGEX_PARAM(int64_t, to_seq);
  (void)domain;
  (void)root;
  (void)from_seq;
  (void)to_seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events for \"([^\"]*)\" root \"([^\"]*)\" as of sequence (\\d+)$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(int64_t, seq);
  (void)domain;
  (void)root;
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events for \"([^\"]*)\" root \"([^\"]*)\" as of time \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(std::string, ts);
  (void)domain;
  (void)root;
  (void)ts;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events for \"([^\"]*)\" root \"([^\"]*)\" in edition \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, domain);
  REGEX_PARAM(std::string, root);
  REGEX_PARAM(std::string, edition);
  (void)domain;
  (void)root;
  (void)edition;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events by correlation ID \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, corr);
  (void)corr;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I query events with empty domain$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
WHEN("^I attempt to query events$") {
  FAIL() << "WIP: step needs implementation";
}

// ==========================================================================
// Then Steps
// ==========================================================================

// TODO (WIP): Implement this step matcher properly.
THEN("^the history is empty and the next sequence is (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I receive (\\d+) events$") {
  REGEX_PARAM(int64_t, count);
  (void)count;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the events are in sequence order (\\d+) to (\\d+)$") {
  REGEX_PARAM(int64_t, from_seq);
  REGEX_PARAM(int64_t, to_seq);
  (void)from_seq;
  (void)to_seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the first event has type \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, event_type);
  (void)event_type;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the first event has payload \"([^\"]*)\"$") {
  REGEX_PARAM(std::string, payload);
  (void)payload;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the first event has sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the last event has sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I receive no events$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I receive events up to that timestamp$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I receive events from that edition only$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^I receive events from all correlated aggregates$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the result carries a snapshot taken at sequence (\\d+)$") {
  REGEX_PARAM(int64_t, seq);
  (void)seq;
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the query is refused because a domain is required$") {
  FAIL() << "WIP: step needs implementation";
}

// TODO (WIP): Implement this step matcher properly.
THEN("^the query fails because the backend is unreachable$") {
  FAIL() << "WIP: step needs implementation";
}
