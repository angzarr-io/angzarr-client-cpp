// Mirrors tests from Python's tests/test_compensation.py and Rust's
// src/compensation.rs #[cfg(test)] block. These bring C++'s compensation
// test surface to structural parity with Py/Rs.
//
// Naming: idiomatic gtest (TEST(Group, Name)). One Py test maps to one
// TEST. Existing compensation_test.cpp covers a small handler subset;
// this file adds the structural assertions (defaults, dispatch_key
// branches, is_notification edge cases, delegate flag combinations,
// pm_emit_compensation flags).

#include "angzarr/compensation.hpp"

#include <google/protobuf/any.pb.h>
#include <gtest/gtest.h>

#include "angzarr/helpers.hpp"
#include "angzarr/proto_aliases.hpp"

namespace {

using namespace angzarr;

// Helper mirroring Py's _make_rejection_notification.
Notification MakeRejectionNotification(const CommandBook* rejected_command,
                                       const std::string& reason) {
    RejectionNotification rej;
    rej.set_rejection_reason(reason);
    if (rejected_command != nullptr) {
        *rej.mutable_rejected_command() = *rejected_command;
    }
    Notification notif;
    notif.mutable_payload()->PackFrom(rej);
    return notif;
}

// ============================================================================
// CompensationContext::from_notification — mirrors
// tests/test_compensation.py::TestCompensationContextFromNotification
// ============================================================================

// Mirrors test_empty_notification_yields_defaults.
TEST(CompensationContextMirror, EmptyNotification_YieldsDefaults) {
    Notification empty;
    auto ctx = CompensationContext::from_notification(empty);
    EXPECT_EQ(ctx.source_event_sequence(), 0u);
    EXPECT_EQ(ctx.rejection_reason(), "");
    EXPECT_FALSE(ctx.rejected_command().has_value());
    EXPECT_FALSE(ctx.source_aggregate().has_value());
}

// Mirrors test_rejection_reason_extracted.
TEST(CompensationContextMirror, RejectionReasonExtracted) {
    auto notif = MakeRejectionNotification(nullptr, "out of stock");
    auto ctx = CompensationContext::from_notification(notif);
    EXPECT_EQ(ctx.rejection_reason(), "out of stock");
}

// Mirrors test_rejected_command_extracted.
TEST(CompensationContextMirror, RejectedCommandExtracted) {
    CommandBook cmd;
    cmd.mutable_cover()->set_domain("inventory");
    auto notif = MakeRejectionNotification(&cmd, "bad");
    auto ctx = CompensationContext::from_notification(notif);
    ASSERT_TRUE(ctx.rejected_command().has_value());
    EXPECT_EQ(ctx.rejected_command()->cover().domain(), "inventory");
}

// Mirrors test_no_deferred_header_yields_no_source.
TEST(CompensationContextMirror, NoDeferredHeader_YieldsNoSource) {
    CommandBook cmd;
    auto* page = cmd.add_pages();
    page->mutable_header()->set_sequence(1);
    auto notif = MakeRejectionNotification(&cmd, "bad");
    auto ctx = CompensationContext::from_notification(notif);
    EXPECT_FALSE(ctx.source_aggregate().has_value());
    EXPECT_EQ(ctx.source_event_sequence(), 0u);
}

// Mirrors test_deferred_without_source_subfield.
TEST(CompensationContextMirror, DeferredWithoutSourceSubfield_StillPropagatesSeq) {
    CommandBook cmd;
    auto* page = cmd.add_pages();
    page->mutable_header()->mutable_angzarr_deferred()->set_source_seq(7);
    auto notif = MakeRejectionNotification(&cmd, "bad");
    auto ctx = CompensationContext::from_notification(notif);
    EXPECT_EQ(ctx.source_event_sequence(), 7u);
    EXPECT_FALSE(ctx.source_aggregate().has_value());
}

// Mirrors test_source_from_angzarr_deferred — full source extraction.
TEST(CompensationContextMirror, SourceFromAngzarrDeferred_PopulatesAggregateAndSeq) {
    CommandBook cmd;
    auto* page = cmd.add_pages();
    auto* deferred = page->mutable_header()->mutable_angzarr_deferred();
    deferred->set_source_seq(42);
    deferred->mutable_source()->set_domain("orders");
    auto notif = MakeRejectionNotification(&cmd, "bad");
    auto ctx = CompensationContext::from_notification(notif);
    EXPECT_EQ(ctx.source_event_sequence(), 42u);
    ASSERT_TRUE(ctx.source_aggregate().has_value());
    EXPECT_EQ(ctx.source_aggregate()->domain(), "orders");
}

// ============================================================================
// CompensationContext property accessors — mirrors
// tests/test_compensation.py::TestCompensationContextProperties
// ============================================================================

// Mirrors test_rejected_command_type_none_when_no_rejected_command.
TEST(CompensationContextMirror, RejectedCommandType_EmptyWhenNoRejectedCommand) {
    Notification empty;
    auto ctx = CompensationContext::from_notification(empty);
    EXPECT_EQ(ctx.rejected_command_type(), "");
}

// Mirrors test_rejected_command_type_none_when_no_command_in_page.
TEST(CompensationContextMirror, RejectedCommandType_EmptyWhenPageHasNoCommand) {
    CommandBook cmd;
    auto* page = cmd.add_pages();
    page->mutable_header()->set_sequence(1);
    auto notif = MakeRejectionNotification(&cmd, "");
    auto ctx = CompensationContext::from_notification(notif);
    EXPECT_EQ(ctx.rejected_command_type(), "");
}

// Mirrors test_dispatch_key_empty_without_rejected_command.
TEST(CompensationContextMirror, DispatchKey_EmptyWithoutRejectedCommand) {
    Notification empty;
    auto ctx = CompensationContext::from_notification(empty);
    EXPECT_EQ(ctx.dispatch_key(), "");
}

// Mirrors test_dispatch_key_empty_when_no_command_type — command page without
// packed command.
TEST(CompensationContextMirror, DispatchKey_EmptyWhenNoCommandType) {
    CommandBook cmd;
    cmd.mutable_cover()->set_domain("inventory");
    auto* page = cmd.add_pages();
    page->mutable_header()->set_sequence(1);
    auto notif = MakeRejectionNotification(&cmd, "");
    auto ctx = CompensationContext::from_notification(notif);
    EXPECT_EQ(ctx.dispatch_key(), "");
}

// Mirrors test_dispatch_key_empty_when_no_cover — packed command but no cover.
TEST(CompensationContextMirror, DispatchKey_EmptyWhenNoCover) {
    CommandBook cmd;
    auto* page = cmd.add_pages();
    page->mutable_header()->set_sequence(1);
    Cover c;
    c.set_domain("x");
    page->mutable_command()->PackFrom(c);
    auto notif = MakeRejectionNotification(&cmd, "");
    auto ctx = CompensationContext::from_notification(notif);
    EXPECT_EQ(ctx.dispatch_key(), "");
}

// Mirrors test_dispatch_key_domain_slash_command — happy path.
TEST(CompensationContextMirror, DispatchKey_DomainSlashCommand_OnSuccess) {
    CommandBook cmd;
    cmd.mutable_cover()->set_domain("inventory");
    auto* page = cmd.add_pages();
    page->mutable_header()->set_sequence(1);
    Cover c;
    page->mutable_command()->PackFrom(c);
    auto notif = MakeRejectionNotification(&cmd, "");
    auto ctx = CompensationContext::from_notification(notif);
    // type_name_from_url strips the leading "type.googleapis.com/" prefix.
    EXPECT_EQ(ctx.dispatch_key(), "inventory/angzarr_client.proto.angzarr.Cover");
}

// ============================================================================
// Aggregate helpers — mirrors tests/test_compensation.py::TestAggregateHelpers
// ============================================================================

// Mirrors test_delegate_to_framework_defaults.
TEST(DelegateToFrameworkMirror, Defaults) {
    auto resp = delegate_to_framework("no compensation logic");
    ASSERT_TRUE(resp.has_revocation());
    const auto& rev = resp.revocation();
    EXPECT_TRUE(rev.emit_system_revocation());
    EXPECT_FALSE(rev.send_to_dead_letter_queue());
    EXPECT_FALSE(rev.escalate());
    EXPECT_FALSE(rev.abort());
    EXPECT_EQ(rev.reason(), "no compensation logic");
}

// Mirrors test_delegate_to_framework_all_flags.
TEST(DelegateToFrameworkMirror, AllFlagsOverridden) {
    auto resp = delegate_to_framework("escalate!", /*emit_system_event=*/false,
                                      /*send_to_dlq=*/true, /*escalate=*/true,
                                      /*abort=*/true);
    ASSERT_TRUE(resp.has_revocation());
    const auto& rev = resp.revocation();
    EXPECT_FALSE(rev.emit_system_revocation());
    EXPECT_TRUE(rev.send_to_dead_letter_queue());
    EXPECT_TRUE(rev.escalate());
    EXPECT_TRUE(rev.abort());
    EXPECT_EQ(rev.reason(), "escalate!");
}

// Mirrors test_emit_compensation_events_returns_events.
TEST(EmitCompensationEventsMirror, ReturnsEventsCoverPreserved) {
    EventBook book;
    book.mutable_cover()->set_domain("orders");
    auto resp = emit_compensation_events(book);
    ASSERT_TRUE(resp.has_events());
    EXPECT_EQ(resp.events().cover().domain(), "orders");
}

// ============================================================================
// Process Manager helpers — mirrors
// tests/test_compensation.py::TestProcessManagerHelpers
// ============================================================================

// Mirrors test_pm_delegate_default.
TEST(PmDelegateToFrameworkMirror, DefaultsEmitSystemRevocationTrue) {
    auto resp = pm_delegate_to_framework("no pm logic");
    EXPECT_FALSE(resp.process_events.has_value());
    EXPECT_TRUE(resp.revocation.emit_system_revocation());
    EXPECT_EQ(resp.revocation.reason(), "no pm logic");
}

// Mirrors test_pm_delegate_no_system_event — overload with flag.
TEST(PmDelegateToFrameworkMirror, OverrideEmitSystemRevocationFalse) {
    auto resp = pm_delegate_to_framework("silent", /*emit_system_event=*/false);
    EXPECT_FALSE(resp.revocation.emit_system_revocation());
}

// Mirrors test_pm_emit_compensation_events_minimal — also_emit=false.
TEST(PmEmitCompensationEventsMirror, MinimalKeepsEmitFalse) {
    EventBook book;
    book.mutable_cover()->set_domain("fulfillment");
    auto resp = pm_emit_compensation_events(book, /*also_emit=*/false, /*reason=*/"");
    ASSERT_TRUE(resp.process_events.has_value());
    EXPECT_EQ(resp.process_events->cover().domain(), "fulfillment");
    EXPECT_FALSE(resp.revocation.emit_system_revocation());
    EXPECT_EQ(resp.revocation.reason(), "");
}

// Mirrors test_pm_emit_compensation_events_with_system_event — also_emit=true.
TEST(PmEmitCompensationEventsMirror, WithSystemEventSetsEmitTrue) {
    EventBook book;
    auto resp = pm_emit_compensation_events(book, /*also_emit=*/true, "because");
    EXPECT_TRUE(resp.revocation.emit_system_revocation());
    EXPECT_EQ(resp.revocation.reason(), "because");
}

// ============================================================================
// is_notification — mirrors tests/test_compensation.py::TestIsNotification
// ============================================================================

// Mirrors test_matches_canonical_notification_url.
TEST(IsNotificationMirror, MatchesCanonicalUrl) {
    EXPECT_TRUE(is_notification(
        std::string(helpers::TYPE_URL_PREFIX) + kNotificationTypeName));
}

// Mirrors test_rejects_pre_58_short_form — pre-audit-#58 short form no longer
// recognized.
TEST(IsNotificationMirror, RejectsPre58ShortForm) {
    EXPECT_FALSE(is_notification("type.googleapis.com/angzarr.Notification"));
}

// Mirrors test_rejects_unrelated_type_url.
TEST(IsNotificationMirror, RejectsUnrelated) {
    EXPECT_FALSE(is_notification("type.googleapis.com/examples.Foo"));
}

// Mirrors test_rejects_empty_string.
TEST(IsNotificationMirror, RejectsEmpty) { EXPECT_FALSE(is_notification("")); }

// Mirrors test_case_sensitive.
TEST(IsNotificationMirror, CaseSensitive) {
    EXPECT_FALSE(is_notification(
        "type.googleapis.com/angzarr_client.proto.angzarr.notification"));
}

}  // namespace
