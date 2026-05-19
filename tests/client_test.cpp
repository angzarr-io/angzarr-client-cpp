// Tests for client behaviors that drift between Python/Rust audits
// and the C++ port.
//   - Audit #39: lenient UDS prefix detection. Python's _create_channel
//     accepts /abs, ./rel, and any unix:-prefixed endpoint (single
//     slash, double slash, or triple-slash forms).  C++'s
//     `format_endpoint` is the analog and must recognize the same
//     shapes (Postel's Law).
//   - Audit #20 / P2.4a: ``command_new`` auto-materializes a fresh
//     UUID v4 root client-side. Each invocation must yield an
//     independent UUID.

#include <gtest/gtest.h>

#include <set>
#include <string>

#include "angzarr/builder.hpp"
#include "angzarr/client.hpp"

using namespace angzarr;

// =============================================================================
// Audit #39 — lenient UDS prefix detection
// =============================================================================

TEST(FormatEndpoint, AbsolutePathIsTreatedAsUds) {
    EXPECT_EQ(detail::format_endpoint("/var/run/foo.sock"), "unix:///var/run/foo.sock");
}

TEST(FormatEndpoint, RelativePathIsTreatedAsUds) {
    EXPECT_EQ(detail::format_endpoint("./local.sock"), "unix://./local.sock");
}

TEST(FormatEndpoint, UnixDoubleSlashIsPassedThrough) {
    EXPECT_EQ(detail::format_endpoint("unix:///abs/path.sock"), "unix:///abs/path.sock");
}

TEST(FormatEndpoint, UnixSingleSlashAbsoluteIsAccepted) {
    // unix:/abs -> normalized to unix:///abs (gRPC URI canonical form).
    // Audit #39: was silently mis-treated as TCP before this port.
    auto out = detail::format_endpoint("unix:/abs/path.sock");
    EXPECT_TRUE(out.find("/abs/path.sock") != std::string::npos)
        << "got: " << out;
    EXPECT_TRUE(out.find("unix:") == 0) << "got: " << out;
}

TEST(FormatEndpoint, UnixSchemeRelativeIsAccepted) {
    // unix:relative — single-slash, relative. Must be treated as UDS.
    auto out = detail::format_endpoint("unix:relative.sock");
    EXPECT_TRUE(out.find("relative.sock") != std::string::npos) << "got: " << out;
    EXPECT_TRUE(out.find("unix:") == 0) << "got: " << out;
}

TEST(FormatEndpoint, TcpHostPortIsPassedThrough) {
    EXPECT_EQ(detail::format_endpoint("localhost:1310"), "localhost:1310");
}

TEST(FormatEndpoint, HttpSchemeIsStripped) {
    EXPECT_EQ(detail::format_endpoint("http://localhost:1310"), "localhost:1310");
}

// =============================================================================
// Audit #20 — command_new auto-generates UUID v4 root
// =============================================================================

TEST(CommandBuilderNew, CommandNewMaterializesRootClientSide) {
    // Audit P2.4a / finding #20: command_new auto-fills a UUID v4
    // root so the server-side never sees a missing-root command for
    // a new aggregate.
    auto builder = CommandBuilder::command_new(nullptr, "orders");

    google::protobuf::Any payload;
    payload.set_type_url("type.googleapis.com/test.Create");

    auto book = builder.with_sequence(0)
                    .with_command("type.googleapis.com/test.Create", payload)
                    .build();

    ASSERT_TRUE(book.cover().has_root()) << "command_new must auto-fill the root";
    EXPECT_EQ(book.cover().root().value().size(), 16u)
        << "auto-generated UUID must be 16 bytes (UUID v4)";
}

TEST(CommandBuilderNew, EachCallYieldsIndependentRoot) {
    // command_new called twice must produce two distinct UUIDs.
    google::protobuf::Any payload;
    payload.set_type_url("type.googleapis.com/test.Create");

    auto book1 = CommandBuilder::command_new(nullptr, "orders")
                     .with_sequence(0)
                     .with_command("type.googleapis.com/test.Create", payload)
                     .build();
    auto book2 = CommandBuilder::command_new(nullptr, "orders")
                     .with_sequence(0)
                     .with_command("type.googleapis.com/test.Create", payload)
                     .build();

    ASSERT_TRUE(book1.cover().has_root());
    ASSERT_TRUE(book2.cover().has_root());
    EXPECT_NE(book1.cover().root().value(), book2.cover().root().value())
        << "each command_new call must yield a fresh UUID";
}
