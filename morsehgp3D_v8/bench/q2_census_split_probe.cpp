#include "front_fixtures.hpp"
#include "pipeline/q2_census_parallel.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <locale>
#include <tuple>

namespace {
using mhgp8::u64;
using Clock = std::chrono::steady_clock;
template<class T> T integer(std::string_view text) {
  T v{};
  const auto p = std::from_chars(text.data(), text.data() + text.size(), v);
  if (text.empty() || p.ec != std::errc{} || p.ptr != text.data() + text.size())
    throw std::invalid_argument("invalid unsigned integer");
  return v;
}
void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
double ms(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
// Order-independent digest of COMPLETE supports, not a geometric oracle.
// Unsigned modular sum/xor deliberately wrap; cardinality counters do not.
struct Digest {
  u64 supports{}, interiors{}, shells{}, hash_sum{}, hash_xor{};
  void consume(const mhgp8::Q2Support& s) {
    u64 h = 14695981039346656037ULL;
    const auto word = [&](u64 v) { mhgp8::bench::front_hash_word(h, v); };
    word(s.a_id); word(s.b_id);
    for (auto v : s.key.center_twice) word(v);
    word(s.key.diameter_squared);
    word(s.interior.size()); for (auto id : s.interior) word(id);
    word(s.shell.size()); for (auto id : s.shell) word(id);
    hash_sum += h; hash_xor ^= h;
    mhgp8::counter_add(supports);
    mhgp8::counter_add(interiors, s.interior.size());
    mhgp8::counter_add(shells, s.shell.size());
  }
  void merge(const Digest& v) {
    mhgp8::counter_add(supports, v.supports); mhgp8::counter_add(interiors, v.interiors);
    mhgp8::counter_add(shells, v.shells); hash_sum += v.hash_sum; hash_xor ^= v.hash_xor;
  }
  bool operator==(const Digest&) const = default;
};
void geometry(const mhgp8::Q2CensusResumeSnapshot& r) {
  bool first = true;
  const auto field = [&](const char* name, u64 value) {
    if (!first) std::cout << ',';
    first = false; std::cout << '"' << name << "\":" << value;
  };
#define MHGP8_SPLIT_GEOM(name) field(#name, r.census.work.name)
  MHGP8_SPLIT_GEOM(query_build_point_visits); MHGP8_SPLIT_GEOM(query_build_nodes);
  MHGP8_SPLIT_GEOM(query_build_max_depth); MHGP8_SPLIT_GEOM(input_descriptors);
  MHGP8_SPLIT_GEOM(query_cover_visits); MHGP8_SPLIT_GEOM(query_tasks);
  MHGP8_SPLIT_GEOM(query_splits); MHGP8_SPLIT_GEOM(witness_splits);
  MHGP8_SPLIT_GEOM(count_root_starts); MHGP8_SPLIT_GEOM(shared_splits_after_credit);
  MHGP8_SPLIT_GEOM(cursor_advances); MHGP8_SPLIT_GEOM(cursor_reuses);
  MHGP8_SPLIT_GEOM(count_node_visits); MHGP8_SPLIT_GEOM(count_bound_tests);
  MHGP8_SPLIT_GEOM(count_point_tests); MHGP8_SPLIT_GEOM(uniform_credited_pairs);
  MHGP8_SPLIT_GEOM(uniform_rejected_pairs); MHGP8_SPLIT_GEOM(uniform_accepted_pairs);
  MHGP8_SPLIT_GEOM(consumed_witness_sites); MHGP8_SPLIT_GEOM(frontier_restarts);
  MHGP8_SPLIT_GEOM(payload_node_visits); MHGP8_SPLIT_GEOM(payload_bound_tests);
  MHGP8_SPLIT_GEOM(payload_point_tests); MHGP8_SPLIT_GEOM(payload_interior_sites);
  MHGP8_SPLIT_GEOM(payload_shell_sites); MHGP8_SPLIT_GEOM(payload_supports);
#undef MHGP8_SPLIT_GEOM
#define MHGP8_SPLIT_SIB(name) field("sibling_" #name, r.sibling_work.name)
  MHGP8_SPLIT_SIB(proposals); MHGP8_SPLIT_SIB(cardinality_skips);
  MHGP8_SPLIT_SIB(bound_tests); MHGP8_SPLIT_SIB(rejected_tasks);
  MHGP8_SPLIT_SIB(rejected_pairs); MHGP8_SPLIT_SIB(rejected_after_credit);
#undef MHGP8_SPLIT_SIB
#define MHGP8_SPLIT_ORDER(name) field("order_" #name, r.order_work.name)
  MHGP8_SPLIT_ORDER(structural_splits); MHGP8_SPLIT_ORDER(deferred_skips);
  MHGP8_SPLIT_ORDER(anchor_skips); MHGP8_SPLIT_ORDER(phase_switches);
#undef MHGP8_SPLIT_ORDER
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 9) throw std::invalid_argument(
        "usage: mhgp8_q2_census_split_probe n family K s seed quantum workers queue");
    const auto n = integer<std::size_t>(argv[1]);
    const std::string_view family = argv[2];
    const auto k = integer<unsigned>(argv[3]), separation = integer<unsigned>(argv[4]);
    const auto seed = integer<u64>(argv[5]);
    const mhgp8::Q2CensusParallelOptions options{integer<std::size_t>(argv[7]),
                                               integer<std::size_t>(argv[6]),
                                               integer<std::size_t>(argv[8])};
    mhgp8::bench::validate_front_fixture_size(n, family);
    if (k == 0 || k > 10 || separation == 0 || options.quantum == 0 ||
        options.workers == 0 || options.queue_capacity == 0)
      throw std::invalid_argument("requires K1..10 and positive s/quantum/workers/queue");
    const auto start = Clock::now();
    const auto fixture = mhgp8::bench::make_front_fixture(n, family, seed);
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixture.points));
    const double prepare_ms = ms(start);
    const auto nodes = index->spatial_nodes();
    std::size_t selected_a{}, selected_b{};
    bool selected = false;
    const auto front_start = Clock::now();
    const auto front = mhgp8::run_wspd_front(*index, k, separation,
        mhgp8::WspdFrontMode::MidpointSamples, [&](const mhgp8::WspdRectangle& r) {
          auto a = r.a_node, b = r.b_node;
          if (nodes[a].range.size() > nodes[b].range.size()) std::swap(a, b);
          const auto priority = [&](std::size_t x, std::size_t y) {
            return std::tuple(nodes[y].range.size(), nodes[x].range.size(),
                std::numeric_limits<std::size_t>::max() - x,
                std::numeric_limits<std::size_t>::max() - y);
          };
          if (!selected || priority(a, b) > priority(selected_a, selected_b)) {
            selected = true; selected_a = a; selected_b = b;
          }
        }, 1);
    const auto front_ms = ms(front_start);
    require(selected, "no selected anchor");
    const auto a_range = nodes[selected_a].range;
    const auto anchor = a_range.first + a_range.size() / 2;
    Digest reference_digest;
    const auto reference = mhgp8::run_q2_anchor_reference(index, anchor, selected_b, k,
        [&](const mhgp8::Q2Support& s) { reference_digest.consume(s); },
        mhgp8::Q2SiblingMode::Saturating, mhgp8::Q2WitnessOrder::ComplementFirst);
    std::vector<Digest> slots(options.workers);
    std::vector<mhgp8::Q2CensusConsumer> consumers;
    for (std::size_t slot = 0; slot < options.workers; ++slot)
      consumers.emplace_back([&, slot](const mhgp8::Q2Support& s) { slots[slot].consume(s); });
    const auto parallel = mhgp8::run_q2_anchor_parallel(index, anchor, selected_b, k, options,
        consumers, mhgp8::Q2SiblingMode::Saturating, mhgp8::Q2WitnessOrder::ComplementFirst);
    Digest digest;
    for (const auto& slot : slots) digest.merge(slot);
    const auto& r = parallel.sum;
    require(digest == reference_digest, "parallel anchor full payload mismatch");
    require(r.census.work == reference.census.work && r.sibling_work == reference.sibling_work &&
        r.order_work == reference.order_work && r.census.candidate_pairs == reference.census.candidate_pairs &&
        r.census.accepted_pairs == reference.census.accepted_pairs &&
        r.census.rejected_pairs == reference.census.rejected_pairs, "parallel anchor work mismatch");
    std::cout.imbue(std::locale::classic());
    std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_q2_selected_anchor_split_v1\",\"scope\":\"one_anchor_not_full\","
      << "\"public_status\":\"not_claimed\",\"work_equal\":true,\"payload_equal\":true,"
      << "\"n\":" << n << ",\"family\":\"" << family << "\",\"kmax\":" << k
      << ",\"separation_s\":" << separation << ",\"seed\":" << seed
      << ",\"quantum\":" << options.quantum << ",\"workers\":" << options.workers
      << ",\"queue\":" << options.queue_capacity << ",\"anchor_rank\":" << anchor
      << ",\"a_node\":" << selected_a << ",\"b_node\":" << selected_b
      << ",\"input_hash\":" << fixture.input_hash
      << ",\"front_products\":" << front.work.product_visits
      << ",\"front_rectangles\":" << front.work.emitted_rectangles
      << ",\"candidates\":" << r.census.candidate_pairs << ",\"accepted\":" << r.census.accepted_pairs
      << ",\"rejected\":" << r.census.rejected_pairs << ",\"hash_sum\":" << digest.hash_sum
      << ",\"hash_xor\":" << digest.hash_xor << ",\"geometry\":{";
    geometry(r);
    std::cout << "},\"resume\":{";
    bool first = true;
    const auto field = [&](const char* name, u64 value) {
      if (!first) std::cout << ',';
      first = false; std::cout << '"' << name << "\":" << value;
    };
#define MHGP8_SPLIT_R(name) field(#name, r.resume_work.name)
    MHGP8_SPLIT_R(advance_calls); MHGP8_SPLIT_R(transitions); MHGP8_SPLIT_R(entry_steps);
    MHGP8_SPLIT_R(witness_steps); MHGP8_SPLIT_R(admission_steps); MHGP8_SPLIT_R(payload_steps);
    MHGP8_SPLIT_R(pauses); MHGP8_SPLIT_R(pauses_after_credit); MHGP8_SPLIT_R(pauses_inside_deferred);
    MHGP8_SPLIT_R(pauses_during_emission); MHGP8_SPLIT_R(max_pending_tasks);
#undef MHGP8_SPLIT_R
    std::cout << "},\"detach\":{"; first = true;
#define MHGP8_SPLIT_D(name) field(#name, r.detach_work.name)
    MHGP8_SPLIT_D(attempts); MHGP8_SPLIT_D(detached_frames); MHGP8_SPLIT_D(imported_frames);
    MHGP8_SPLIT_D(transferred_pairs); MHGP8_SPLIT_D(moved_frames);
#undef MHGP8_SPLIT_D
    std::cout << "},\"schedule\":{"; first = true;
#define MHGP8_SPLIT_S(name) field(#name, parallel.schedule.name)
    MHGP8_SPLIT_S(offer_checks); MHGP8_SPLIT_S(offer_busy); MHGP8_SPLIT_S(offer_full);
    MHGP8_SPLIT_S(offer_no_sibling); MHGP8_SPLIT_S(donations); MHGP8_SPLIT_S(fragments_started);
    MHGP8_SPLIT_S(fragments_completed); MHGP8_SPLIT_S(waits); MHGP8_SPLIT_S(wakes);
    MHGP8_SPLIT_S(max_queue_size); MHGP8_SPLIT_S(max_active_fragments);
#undef MHGP8_SPLIT_S
    std::cout << "},\"queue_storage_bytes\":" << parallel.queue_storage_bytes
      << ",\"max_fragment_bytes\":" << parallel.max_fragment_bytes
      << ",\"timings_ms\":{\"prepare\":" << prepare_ms << ",\"front\":" << front_ms
      << ",\"reference\":" << reference.census.total_ms << ",\"parallel_enclosing\":" << parallel.total_ms
      << ",\"advance_sum\":" << r.census.total_ms << ",\"payload_sum\":" << r.census.payload_ms
      << "},\"worker_transitions\":[";
    for (std::size_t i = 0; i < parallel.workers.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << parallel.workers[i].sum.resume_work.transitions;
    }
    std::cout << "]}\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mhgp8 split probe: " << e.what() << '\n';
    return 2;
  }
}
