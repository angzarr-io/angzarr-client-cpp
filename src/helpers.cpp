#include "angzarr/helpers.hpp"

#include <cctype>
#include <cstdio>
#include <string>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

namespace angzarr {
namespace helpers {

std::string root_id_hex(const EventBook& book) {
  const UUID* root = root_uuid(book);
  if (!root) return "";

  // Convert binary UUID to hex string
  const std::string& value = root->value();
  std::string hex;
  hex.reserve(value.size() * 2);

  static const char hex_chars[] = "0123456789abcdef";
  for (unsigned char c : value) {
    hex.push_back(hex_chars[c >> 4]);
    hex.push_back(hex_chars[c & 0x0f]);
  }
  return hex;
}

google::protobuf::Timestamp now() {
  auto time_point = std::chrono::system_clock::now();
  auto duration = time_point.time_since_epoch();
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
  auto nanos =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

  google::protobuf::Timestamp ts;
  ts.set_seconds(seconds.count());
  ts.set_nanos(static_cast<int32_t>(nanos.count()));
  return ts;
}

google::protobuf::Timestamp parse_timestamp(const std::string& rfc3339) {
  // Spec MED-2.x — full RFC 3339 grammar (parity with the QueryBuilder
  // ``as_of_time`` parser fixed under spec MED-3.6). Shared logic
  // kept localized to avoid header bloat: the QueryBuilder version
  // is private inside the class. Both paths produce identical
  // protobuf Timestamps for identical inputs.
  google::protobuf::Timestamp ts;

  auto throw_invalid = [&]() {
    throw InvalidTimestampError(
        error_codes::messages::TIMESTAMP_PARSE_FAILED,
        error_codes::codes::TIMESTAMP_PARSE_FAILED,
        ClientError::DetailsMap{{error_codes::keys::INPUT, rfc3339}});
  };

  if (rfc3339.size() < 19 || rfc3339[10] != 'T') throw_invalid();

  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  if (std::sscanf(rfc3339.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day,
                  &hour, &minute, &second) != 6) {
    throw_invalid();
  }
  if (month < 1 || month > 12 || day < 1 || day > 31) throw_invalid();

  size_t pos = 19;
  int32_t nanos = 0;
  if (pos < rfc3339.size() && rfc3339[pos] == '.') {
    ++pos;
    std::string digits;
    while (pos < rfc3339.size() &&
           std::isdigit(static_cast<unsigned char>(rfc3339[pos]))) {
      if (digits.size() < 9) digits.push_back(rfc3339[pos]);
      ++pos;
    }
    if (digits.empty()) throw_invalid();
    while (digits.size() < 9) digits.push_back('0');
    try {
      nanos = std::stoi(digits);
    } catch (...) {
      throw_invalid();
    }
  }
  int tz_offset_seconds = 0;
  if (pos >= rfc3339.size()) throw_invalid();
  char tz = rfc3339[pos];
  if (tz == 'Z' || tz == 'z') {
    ++pos;
  } else if (tz == '+' || tz == '-') {
    ++pos;
    if (pos + 5 > rfc3339.size() || rfc3339[pos + 2] != ':') throw_invalid();
    int oh = 0, om = 0;
    if (std::sscanf(rfc3339.c_str() + pos, "%d:%d", &oh, &om) != 2)
      throw_invalid();
    if (oh < 0 || oh > 23 || om < 0 || om > 59) throw_invalid();
    tz_offset_seconds = (oh * 3600 + om * 60) * (tz == '+' ? 1 : -1);
    pos += 5;
  } else {
    throw_invalid();
  }
  if (pos != rfc3339.size()) throw_invalid();

  auto y = static_cast<int64_t>(year) - (month <= 2 ? 1 : 0);
  auto era = (y >= 0 ? y : y - 399) / 400;
  auto yoe = static_cast<uint32_t>(y - era * 400);
  auto doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  auto doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days_since_1970 = era * 146097 + static_cast<int64_t>(doe) - 719468;
  int64_t seconds = days_since_1970 * 86400 + hour * 3600 + minute * 60 +
                    second - tz_offset_seconds;

  ts.set_seconds(seconds);
  ts.set_nanos(nanos);
  return ts;
}

}  // namespace helpers
}  // namespace angzarr
