#pragma once

// Explicit reuse of the qualified parallel probe's canonical paid callback,
// not of any previous result or timing. File input is a new protocol branch.
#include "parallel_probe_common.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

const std::array dispatch_fields{
    Field<mhgp8::WspdFrontDispatchWork>{"seeds_started", &mhgp8::WspdFrontDispatchWork::seeds_started},
    Field<mhgp8::WspdFrontDispatchWork>{"seeds_completed", &mhgp8::WspdFrontDispatchWork::seeds_completed},
    Field<mhgp8::WspdFrontDispatchWork>{"donations", &mhgp8::WspdFrontDispatchWork::donations},
    Field<mhgp8::WspdFrontDispatchWork>{"donor_checks", &mhgp8::WspdFrontDispatchWork::donor_checks},
    Field<mhgp8::WspdFrontDispatchWork>{"offer_attempts", &mhgp8::WspdFrontDispatchWork::offer_attempts},
    Field<mhgp8::WspdFrontDispatchWork>{"offer_full", &mhgp8::WspdFrontDispatchWork::offer_full},
    Field<mhgp8::WspdFrontDispatchWork>{"offer_busy", &mhgp8::WspdFrontDispatchWork::offer_busy},
    Field<mhgp8::WspdFrontDispatchWork>{"offer_no_demand", &mhgp8::WspdFrontDispatchWork::offer_no_demand},
    Field<mhgp8::WspdFrontDispatchWork>{"stolen_started", &mhgp8::WspdFrontDispatchWork::stolen_started},
    Field<mhgp8::WspdFrontDispatchWork>{"stolen_completed", &mhgp8::WspdFrontDispatchWork::stolen_completed},
    Field<mhgp8::WspdFrontDispatchWork>{"waits", &mhgp8::WspdFrontDispatchWork::waits},
    Field<mhgp8::WspdFrontDispatchWork>{"wakes", &mhgp8::WspdFrontDispatchWork::wakes},
    Field<mhgp8::WspdFrontDispatchWork>{"max_queue_size", &mhgp8::WspdFrontDispatchWork::max_queue_size},
    Field<mhgp8::WspdFrontDispatchWork>{"max_local_stack_size", &mhgp8::WspdFrontDispatchWork::max_local_stack_size}};

void validate_dispatch(const mhgp8::WspdFrontDispatchWork& work, std::size_t capacity) {
  require(work.seeds_started == work.seeds_completed && work.stolen_started == work.stolen_completed &&
          work.donor_checks == work.offer_attempts + work.offer_no_demand &&
          work.offer_attempts == work.donations + work.offer_full + work.offer_busy &&
          work.waits == work.wakes && work.max_queue_size <= capacity && work.max_local_stack_size <= 97,
          "dispatcher ledger mismatch");
}

bool file_family(std::string_view family) { return family.starts_with("file:"); }

std::string_view input_path(std::string_view family) {
  return file_family(family) ? family.substr(5) : std::string_view{};
}

void validate_input_size(std::size_t n, std::string_view family, u64 seed) {
  if (!file_family(family)) {
    mhgp8::bench::validate_front_fixture_size(n, family);
    return;
  }
  const auto path = input_path(family);
  require(n >= 2 && size_counter(n) <= (u64{1} << 48U) && seed == 0 &&
          std::filesystem::path(path).is_absolute(), "file input requires n>=2, absolute path and seed zero");
  for (const unsigned char byte : path)
    require(byte >= 32, "control character in input path");
  static_cast<void>(product(size_counter(n), 6));
}

mhgp8::bench::FrontFixture make_input(std::size_t n, std::string_view family, u64 seed) {
  validate_input_size(n, family, seed);
  if (!file_family(family)) return mhgp8::bench::make_front_fixture(n, family, seed);
  const auto expected = product(size_counter(n), 6);
  std::ifstream stream(std::filesystem::path(input_path(family)), std::ios::binary | std::ios::ate);
  require(stream.is_open(), "cannot open u16le input");
  const auto length = stream.tellg();
  require(length >= 0 && std::cmp_equal(static_cast<std::streamoff>(length), expected),
          "file must contain exactly 6*n bytes");
  stream.seekg(0);
  mhgp8::bench::FrontFixture result;
  result.points.reserve(n);
  mhgp8::bench::front_hash_word(result.input_hash, 1);
  mhgp8::bench::front_hash_word(result.input_hash, size_counter(n));
  for (std::size_t i = 0; i < n; ++i) {
    std::array<unsigned char, 6> bytes{};
    stream.read(reinterpret_cast<char*>(bytes.data()), 6);
    require(stream.gcount() == 6 && !stream.bad(), "truncated u16le input");
    const auto coordinate = [&](std::size_t offset) {
      return static_cast<mhgp8::Coordinate>(static_cast<unsigned>(bytes[offset]) |
             (static_cast<unsigned>(bytes[offset + 1]) << 8U));
    };
    const mhgp8::Point3 point{coordinate(0), coordinate(2), coordinate(4)};
    result.points.push_back(point);
    for (std::size_t axis = 0; axis < 3; ++axis)
      mhgp8::bench::front_hash_word(result.input_hash, point[axis]);
  }
  require(stream.peek() == std::char_traits<char>::eof() && !stream.bad(), "u16le input length changed");
  stream.close();
  return result;  // No deduplication/reordering here; PreparedCloud validates.
}

}  // namespace
