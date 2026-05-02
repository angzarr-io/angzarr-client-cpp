// Bridge header — pulls all generated proto messages into `angzarr::*`.
//
// Background: angzarr-project's .proto files were renamed from
// `package angzarr;` to `package angzarr_client.proto.angzarr;`. The
// generated C++ types live in `::angzarr_client::proto::angzarr::`.
// Existing client source code uses unqualified `EventBook`, `Cover`,
// etc. inside `namespace angzarr { ... }` blocks. The
// `using namespace` directive below preserves those references without
// touching the call sites.
//
// Include this header from any client source that needs proto types.
#pragma once

#include "angzarr/cloudevents.pb.h"
#include "angzarr/command_handler.pb.h"
#include "angzarr/meta.pb.h"
#include "angzarr/process_manager.pb.h"
#include "angzarr/projector.pb.h"
#include "angzarr/query.pb.h"
#include "angzarr/saga.pb.h"
#include "angzarr/stream.pb.h"
#include "angzarr/types.pb.h"
#include "angzarr/upcaster.pb.h"

#include "angzarr/cloudevents.grpc.pb.h"
#include "angzarr/command_handler.grpc.pb.h"
#include "angzarr/process_manager.grpc.pb.h"
#include "angzarr/projector.grpc.pb.h"
#include "angzarr/query.grpc.pb.h"
#include "angzarr/saga.grpc.pb.h"
#include "angzarr/stream.grpc.pb.h"
#include "angzarr/upcaster.grpc.pb.h"

namespace angzarr {
using namespace ::angzarr_client::proto::angzarr;  // NOLINT(google-build-using-namespace)
}  // namespace angzarr
