// AbslLogger implementation — bridge from the angzarr Logger interface
// to absl::log. Audit #87..#91.
//
// Format: ``event_name field1=value1 field2=value2`` — same plain-text
// shape Python's structlog emits in its default "key_value" renderer,
// suitable for `grep` and for routers that don't parse JSON. Callers
// who want JSON install a JSON-emitting Logger backend instead of
// modifying this one.

#include "angzarr/logging.hpp"

// absl/log/log.h landed in abseil 20230125. The mutation-test
// container ships Ubuntu 24.04's libabsl-dev 20220623, which predates
// that header. Guard with ``__has_include`` so the same source tree
// builds under both absl versions: when the header is present we
// route through ``LOG()`` / ``VLOG()`` macros; otherwise we fall back
// to a plain ``stderr`` emitter (parity with Py's default ``print``
// behavior when structlog isn't installed).
#if __has_include(<absl/log/log.h>)
#include <absl/log/log.h>
#define ANGZARR_HAS_ABSL_LOG 1
#else
#include <cstdio>
#endif

#include <mutex>
#include <sstream>

namespace angzarr {

namespace detail {

std::shared_ptr<Logger>& default_logger_storage() {
  static std::shared_ptr<Logger> storage;
  return storage;
}

std::mutex& default_logger_mutex() {
  static std::mutex mu;
  return mu;
}

void AbslLogger::log(LogLevel level, const std::string& event,
                     const LogFields& fields) {
  std::ostringstream out;
  out << event;
  for (const auto& [k, v] : fields) {
    out << ' ' << k << '=' << v;
  }
  auto formatted = out.str();
#ifdef ANGZARR_HAS_ABSL_LOG
  switch (level) {
    case LogLevel::DEBUG:
      // absl::log defaults to filtering INFO+, so DEBUG routes
      // through VLOG(1). Operators bump verbosity with
      // ``GLOG_v=1`` or ``--v=1`` to see these.
      VLOG(1) << formatted;
      break;
    case LogLevel::INFO:
      LOG(INFO) << formatted;
      break;
    case LogLevel::WARN:
      LOG(WARNING) << formatted;
      break;
    case LogLevel::ERROR:
      LOG(ERROR) << formatted;
      break;
  }
#else
  // Plain-text fallback for older absl toolchains. Same key=value
  // shape as the absl path so log greppers see equivalent records.
  std::fprintf(stderr, "[%s] %s\n", level_name(level), formatted.c_str());
#endif
}

}  // namespace detail

}  // namespace angzarr
