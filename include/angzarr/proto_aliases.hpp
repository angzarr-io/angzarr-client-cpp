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

#include "angzarr_client/proto/angzarr/v1/cloudevents.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/cloudevents.pb.h"
#include "angzarr_client/proto/angzarr/v1/command_handler.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/command_handler.pb.h"
#include "angzarr_client/proto/angzarr/v1/meta.pb.h"
#include "angzarr_client/proto/angzarr/v1/process_manager.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/process_manager.pb.h"
#include "angzarr_client/proto/angzarr/v1/projector.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/projector.pb.h"
#include "angzarr_client/proto/angzarr/v1/query.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/query.pb.h"
#include "angzarr_client/proto/angzarr/v1/saga.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/saga.pb.h"
#include "angzarr_client/proto/angzarr/v1/stream.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/stream.pb.h"
#include "angzarr_client/proto/angzarr/v1/types.pb.h"
#include "angzarr_client/proto/angzarr/v1/upcaster.grpc.pb.h"
#include "angzarr_client/proto/angzarr/v1/upcaster.pb.h"

namespace angzarr {

// ---- types.proto ----

// messages
using ::angzarr_client::proto::angzarr::v1::AggregateRoot;
using ::angzarr_client::proto::angzarr::v1::AngzarrDeadLetter;
using ::angzarr_client::proto::angzarr::v1::AngzarrDeferredSequence;
using ::angzarr_client::proto::angzarr::v1::CascadeCommit;
using ::angzarr_client::proto::angzarr::v1::CascadeConflictDetail;
using ::angzarr_client::proto::angzarr::v1::CascadeRollback;
using ::angzarr_client::proto::angzarr::v1::CommandBook;
using ::angzarr_client::proto::angzarr::v1::CommandPage;
using ::angzarr_client::proto::angzarr::v1::CommandRequest;
using ::angzarr_client::proto::angzarr::v1::Compensate;
using ::angzarr_client::proto::angzarr::v1::ComponentDescriptor;
using ::angzarr_client::proto::angzarr::v1::Confirmation;
using ::angzarr_client::proto::angzarr::v1::ContextualCommand;
using ::angzarr_client::proto::angzarr::v1::ContextualCommandRequest;
using ::angzarr_client::proto::angzarr::v1::Cover;
using ::angzarr_client::proto::angzarr::v1::DomainDivergence;
using ::angzarr_client::proto::angzarr::v1::Edition;
using ::angzarr_client::proto::angzarr::v1::EventBook;
using ::angzarr_client::proto::angzarr::v1::EventPage;
using ::angzarr_client::proto::angzarr::v1::EventProcessingFailedDetails;
using ::angzarr_client::proto::angzarr::v1::EventRequest;
using ::angzarr_client::proto::angzarr::v1::EventStreamFilter;
using ::angzarr_client::proto::angzarr::v1::ExternalDeferredSequence;
using ::angzarr_client::proto::angzarr::v1::GetDescriptorRequest;
using ::angzarr_client::proto::angzarr::v1::NoOp;
using ::angzarr_client::proto::angzarr::v1::Notification;
using ::angzarr_client::proto::angzarr::v1::PageHeader;
using ::angzarr_client::proto::angzarr::v1::PayloadReference;
using ::angzarr_client::proto::angzarr::v1::PayloadRetrievalFailedDetails;
// NOTE: ``Projection`` (proto message) is intentionally NOT aliased
// here because :file:`include/angzarr/projector.hpp` defines a C++
// helper ``struct Projection`` with the same name in ``namespace
// angzarr``. Pulling in both via ``using`` would be an ambiguous
// redefinition. Call sites that need the proto reference it as
// ``angzarr_client::proto::angzarr::v1::Projection`` directly.
using ::angzarr_client::proto::angzarr::v1::Query;
using ::angzarr_client::proto::angzarr::v1::RejectionNotification;
using ::angzarr_client::proto::angzarr::v1::Revocation;
using ::angzarr_client::proto::angzarr::v1::SequenceMismatchDetails;
using ::angzarr_client::proto::angzarr::v1::SequenceRange;
using ::angzarr_client::proto::angzarr::v1::SequenceSet;
using ::angzarr_client::proto::angzarr::v1::Snapshot;
using ::angzarr_client::proto::angzarr::v1::Target;
using ::angzarr_client::proto::angzarr::v1::TemporalQuery;
using ::angzarr_client::proto::angzarr::v1::UUID;

// enums + constants
using ::angzarr_client::proto::angzarr::v1::CASCADE_ERROR_COMPENSATE;
using ::angzarr_client::proto::angzarr::v1::CASCADE_ERROR_CONTINUE;
using ::angzarr_client::proto::angzarr::v1::CASCADE_ERROR_DEAD_LETTER;
using ::angzarr_client::proto::angzarr::v1::CASCADE_ERROR_FAIL_FAST;
using ::angzarr_client::proto::angzarr::v1::CascadeErrorMode;
using ::angzarr_client::proto::angzarr::v1::MERGE_AGGREGATE_HANDLES;
using ::angzarr_client::proto::angzarr::v1::MERGE_COMMUTATIVE;
using ::angzarr_client::proto::angzarr::v1::MERGE_MANUAL;
using ::angzarr_client::proto::angzarr::v1::MERGE_STRICT;
using ::angzarr_client::proto::angzarr::v1::MergeStrategy;
using ::angzarr_client::proto::angzarr::v1::SnapshotRetention;
using ::angzarr_client::proto::angzarr::v1::SYNC_MODE_ASYNC;
using ::angzarr_client::proto::angzarr::v1::SYNC_MODE_CASCADE;
using ::angzarr_client::proto::angzarr::v1::SYNC_MODE_DECISION;
using ::angzarr_client::proto::angzarr::v1::SYNC_MODE_ISOLATED;
using ::angzarr_client::proto::angzarr::v1::SYNC_MODE_SIMPLE;
using ::angzarr_client::proto::angzarr::v1::SyncMode;

// ---- command_handler.proto ----
using ::angzarr_client::proto::angzarr::v1::BusinessResponse;
using ::angzarr_client::proto::angzarr::v1::CommandHandlerCoordinatorService;
using ::angzarr_client::proto::angzarr::v1::CommandHandlerService;
using ::angzarr_client::proto::angzarr::v1::CommandResponse;
using ::angzarr_client::proto::angzarr::v1::FactInjectionResponse;
using ::angzarr_client::proto::angzarr::v1::FactRequest;
using ::angzarr_client::proto::angzarr::v1::ReplayRequest;
using ::angzarr_client::proto::angzarr::v1::ReplayResponse;
using ::angzarr_client::proto::angzarr::v1::RevocationResponse;
using ::angzarr_client::proto::angzarr::v1::SpeculateCommandHandlerRequest;

// ---- query.proto ----
using ::angzarr_client::proto::angzarr::v1::EventQueryService;

// ---- projector.proto ----
using ::angzarr_client::proto::angzarr::v1::ProjectorCoordinatorService;
using ::angzarr_client::proto::angzarr::v1::ProjectorService;
using ::angzarr_client::proto::angzarr::v1::SpeculateProjectorRequest;

// ---- saga.proto ----
using ::angzarr_client::proto::angzarr::v1::SagaCompensationFailed;
using ::angzarr_client::proto::angzarr::v1::SagaCoordinatorService;
using ::angzarr_client::proto::angzarr::v1::SagaHandleRequest;
using ::angzarr_client::proto::angzarr::v1::SagaResponse;
using ::angzarr_client::proto::angzarr::v1::SagaService;
using ::angzarr_client::proto::angzarr::v1::SpeculateSagaRequest;

// ---- process_manager.proto ----
using ::angzarr_client::proto::angzarr::v1::ProcessManagerCoordinatorRequest;
using ::angzarr_client::proto::angzarr::v1::ProcessManagerCoordinatorService;
using ::angzarr_client::proto::angzarr::v1::ProcessManagerHandleRequest;
using ::angzarr_client::proto::angzarr::v1::ProcessManagerHandleResponse;
using ::angzarr_client::proto::angzarr::v1::ProcessManagerService;
using ::angzarr_client::proto::angzarr::v1::SpeculatePmRequest;

// ---- upcaster.proto ----
using ::angzarr_client::proto::angzarr::v1::UpcasterService;
using ::angzarr_client::proto::angzarr::v1::UpcastRequest;
using ::angzarr_client::proto::angzarr::v1::UpcastResponse;

// ---- stream.proto ----
using ::angzarr_client::proto::angzarr::v1::EventStreamService;

// ---- cloudevents.proto ----
using ::angzarr_client::proto::angzarr::v1::CloudEvent;
using ::angzarr_client::proto::angzarr::v1::CloudEventsResponse;

}  // namespace angzarr
