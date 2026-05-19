#pragma once

// Namespace shim for the 2026-04-19 proto move (a7d6303).
//
// The proto files moved from ``proto/angzarr/*.proto`` to
// ``proto/angzarr_client/proto/angzarr/*.proto`` and the package was
// renamed from ``angzarr`` to ``angzarr_client.proto.angzarr`` so
// generated Python imports as ``from angzarr_client.proto.angzarr
// import types_pb2`` natively. The C++ generated namespace tracks the
// proto package, so messages now land in
// ``angzarr_client::proto::angzarr``.
//
// Existing C++ call sites (and the audit-era tests) wrote ``Cover``,
// ``EventBook``, etc. unqualified, expecting them to resolve via
// ``using namespace angzarr;`` from the old ``namespace angzarr {}``
// of the generated headers. This shim restores that contract by
// re-exporting every public message + service stub into ``namespace
// angzarr``. No call site needs to change.
//
// Include this header (or the umbrella ``angzarr.hpp``) before any
// reference to a proto type.

#include "angzarr_client/proto/angzarr/cloudevents.grpc.pb.h"
#include "angzarr_client/proto/angzarr/cloudevents.pb.h"
#include "angzarr_client/proto/angzarr/command_handler.grpc.pb.h"
#include "angzarr_client/proto/angzarr/command_handler.pb.h"
#include "angzarr_client/proto/angzarr/meta.pb.h"
#include "angzarr_client/proto/angzarr/process_manager.grpc.pb.h"
#include "angzarr_client/proto/angzarr/process_manager.pb.h"
#include "angzarr_client/proto/angzarr/projector.grpc.pb.h"
#include "angzarr_client/proto/angzarr/projector.pb.h"
#include "angzarr_client/proto/angzarr/query.grpc.pb.h"
#include "angzarr_client/proto/angzarr/query.pb.h"
#include "angzarr_client/proto/angzarr/saga.grpc.pb.h"
#include "angzarr_client/proto/angzarr/saga.pb.h"
#include "angzarr_client/proto/angzarr/stream.grpc.pb.h"
#include "angzarr_client/proto/angzarr/stream.pb.h"
#include "angzarr_client/proto/angzarr/types.pb.h"
#include "angzarr_client/proto/angzarr/upcaster.grpc.pb.h"
#include "angzarr_client/proto/angzarr/upcaster.pb.h"

namespace angzarr {

// ---- types.proto ----

// messages
using ::angzarr_client::proto::angzarr::AggregateRoot;
using ::angzarr_client::proto::angzarr::AngzarrDeadLetter;
using ::angzarr_client::proto::angzarr::AngzarrDeferredSequence;
using ::angzarr_client::proto::angzarr::CascadeCommit;
using ::angzarr_client::proto::angzarr::CascadeConflictDetail;
using ::angzarr_client::proto::angzarr::CascadeRollback;
using ::angzarr_client::proto::angzarr::CommandBook;
using ::angzarr_client::proto::angzarr::CommandPage;
using ::angzarr_client::proto::angzarr::CommandRequest;
using ::angzarr_client::proto::angzarr::Compensate;
using ::angzarr_client::proto::angzarr::ComponentDescriptor;
using ::angzarr_client::proto::angzarr::Confirmation;
using ::angzarr_client::proto::angzarr::ContextualCommand;
using ::angzarr_client::proto::angzarr::ContextualCommandRequest;
using ::angzarr_client::proto::angzarr::Cover;
using ::angzarr_client::proto::angzarr::DomainDivergence;
using ::angzarr_client::proto::angzarr::Edition;
using ::angzarr_client::proto::angzarr::EventBook;
using ::angzarr_client::proto::angzarr::EventPage;
using ::angzarr_client::proto::angzarr::EventProcessingFailedDetails;
using ::angzarr_client::proto::angzarr::EventRequest;
using ::angzarr_client::proto::angzarr::EventStreamFilter;
using ::angzarr_client::proto::angzarr::ExternalDeferredSequence;
using ::angzarr_client::proto::angzarr::GetDescriptorRequest;
using ::angzarr_client::proto::angzarr::NoOp;
using ::angzarr_client::proto::angzarr::Notification;
using ::angzarr_client::proto::angzarr::PageHeader;
using ::angzarr_client::proto::angzarr::PayloadReference;
using ::angzarr_client::proto::angzarr::PayloadRetrievalFailedDetails;
// NOTE: ``Projection`` (proto message) is intentionally NOT aliased
// here because :file:`include/angzarr/projector.hpp` defines a C++
// helper ``struct Projection`` with the same name in ``namespace
// angzarr``. Pulling in both via ``using`` would be an ambiguous
// redefinition. Call sites that need the proto reference it as
// ``angzarr_client::proto::angzarr::Projection`` directly.
using ::angzarr_client::proto::angzarr::Query;
using ::angzarr_client::proto::angzarr::RejectionNotification;
using ::angzarr_client::proto::angzarr::Revocation;
using ::angzarr_client::proto::angzarr::SequenceMismatchDetails;
using ::angzarr_client::proto::angzarr::SequenceRange;
using ::angzarr_client::proto::angzarr::SequenceSet;
using ::angzarr_client::proto::angzarr::Snapshot;
using ::angzarr_client::proto::angzarr::Target;
using ::angzarr_client::proto::angzarr::TemporalQuery;
using ::angzarr_client::proto::angzarr::UUID;

// enums + constants
using ::angzarr_client::proto::angzarr::CASCADE_ERROR_COMPENSATE;
using ::angzarr_client::proto::angzarr::CASCADE_ERROR_CONTINUE;
using ::angzarr_client::proto::angzarr::CASCADE_ERROR_DEAD_LETTER;
using ::angzarr_client::proto::angzarr::CASCADE_ERROR_FAIL_FAST;
using ::angzarr_client::proto::angzarr::CascadeErrorMode;
using ::angzarr_client::proto::angzarr::MERGE_AGGREGATE_HANDLES;
using ::angzarr_client::proto::angzarr::MERGE_COMMUTATIVE;
using ::angzarr_client::proto::angzarr::MERGE_MANUAL;
using ::angzarr_client::proto::angzarr::MERGE_STRICT;
using ::angzarr_client::proto::angzarr::MergeStrategy;
using ::angzarr_client::proto::angzarr::SnapshotRetention;
using ::angzarr_client::proto::angzarr::SYNC_MODE_ASYNC;
using ::angzarr_client::proto::angzarr::SYNC_MODE_CASCADE;
using ::angzarr_client::proto::angzarr::SYNC_MODE_DECISION;
using ::angzarr_client::proto::angzarr::SYNC_MODE_ISOLATED;
using ::angzarr_client::proto::angzarr::SYNC_MODE_SIMPLE;
using ::angzarr_client::proto::angzarr::SyncMode;

// ---- command_handler.proto ----
using ::angzarr_client::proto::angzarr::BusinessResponse;
using ::angzarr_client::proto::angzarr::CommandHandlerCoordinatorService;
using ::angzarr_client::proto::angzarr::CommandHandlerService;
using ::angzarr_client::proto::angzarr::CommandResponse;
using ::angzarr_client::proto::angzarr::FactInjectionResponse;
using ::angzarr_client::proto::angzarr::FactRequest;
using ::angzarr_client::proto::angzarr::ReplayRequest;
using ::angzarr_client::proto::angzarr::ReplayResponse;
using ::angzarr_client::proto::angzarr::RevocationResponse;
using ::angzarr_client::proto::angzarr::SpeculateCommandHandlerRequest;

// ---- query.proto ----
using ::angzarr_client::proto::angzarr::EventQueryService;

// ---- projector.proto ----
using ::angzarr_client::proto::angzarr::ProjectorCoordinatorService;
using ::angzarr_client::proto::angzarr::ProjectorService;
using ::angzarr_client::proto::angzarr::SpeculateProjectorRequest;

// ---- saga.proto ----
using ::angzarr_client::proto::angzarr::SagaCompensationFailed;
using ::angzarr_client::proto::angzarr::SagaCoordinatorService;
using ::angzarr_client::proto::angzarr::SagaHandleRequest;
using ::angzarr_client::proto::angzarr::SagaResponse;
using ::angzarr_client::proto::angzarr::SagaService;
using ::angzarr_client::proto::angzarr::SpeculateSagaRequest;

// ---- process_manager.proto ----
using ::angzarr_client::proto::angzarr::ProcessManagerCoordinatorRequest;
using ::angzarr_client::proto::angzarr::ProcessManagerCoordinatorService;
using ::angzarr_client::proto::angzarr::ProcessManagerHandleRequest;
using ::angzarr_client::proto::angzarr::ProcessManagerHandleResponse;
using ::angzarr_client::proto::angzarr::ProcessManagerService;
using ::angzarr_client::proto::angzarr::SpeculatePmRequest;

// ---- upcaster.proto ----
using ::angzarr_client::proto::angzarr::UpcasterService;
using ::angzarr_client::proto::angzarr::UpcastRequest;
using ::angzarr_client::proto::angzarr::UpcastResponse;

// ---- stream.proto ----
using ::angzarr_client::proto::angzarr::EventStreamService;

// ---- cloudevents.proto ----
using ::angzarr_client::proto::angzarr::CloudEvent;
using ::angzarr_client::proto::angzarr::CloudEventsResponse;

}  // namespace angzarr
