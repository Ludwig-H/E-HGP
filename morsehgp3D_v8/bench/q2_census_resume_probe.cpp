#include "front_fixtures.hpp"
#include "pipeline/q2_census_resume.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace {
using Clock = std::chrono::steady_clock;
using mhgp8::u64;

void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
template<class T> T integer(std::string_view text) {
  T value{};
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    throw std::invalid_argument("invalid unsigned integer");
  return value;
}
double ms(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// Test/measurement digest of the complete ORDERED stream including every
// payload ID. Non-cryptographic: the bounded gate, not this digest, is the oracle.
struct Digest {
  u64 supports{}, interiors{}, shells{}, hash{14695981039346656037ULL};
  void consume(const mhgp8::Q2Support& support) {
    const auto word = [&](u64 v) { mhgp8::bench::front_hash_word(hash, v); };
    word(support.a_id); word(support.b_id);
    for (const auto v : support.key.center_twice) word(v);
    word(support.key.diameter_squared);
    word(support.interior.size());
    for (const auto id : support.interior) word(id);
    word(support.shell.size());
    for (const auto id : support.shell) word(id);
    mhgp8::counter_add(supports);
    mhgp8::counter_add(interiors, support.interior.size());
    mhgp8::counter_add(shells, support.shell.size());
  }
  bool operator==(const Digest&) const = default;
};

void print_work(const mhgp8::Q2CensusWork& w) {
  bool first = true;
  const auto field = [&](const char* name, u64 value) {
    if (!first) std::cout << ',';
    first = false; std::cout << '"' << name << "\":" << value;
  };
#define MHGP8_RESUME_FIELD(name) field(#name, w.name)
  MHGP8_RESUME_FIELD(query_build_point_visits); MHGP8_RESUME_FIELD(query_build_nodes);
  MHGP8_RESUME_FIELD(query_build_max_depth); MHGP8_RESUME_FIELD(input_descriptors);
  MHGP8_RESUME_FIELD(query_cover_visits); MHGP8_RESUME_FIELD(query_tasks);
  MHGP8_RESUME_FIELD(query_splits); MHGP8_RESUME_FIELD(witness_splits);
  MHGP8_RESUME_FIELD(count_root_starts); MHGP8_RESUME_FIELD(shared_splits_after_credit);
  MHGP8_RESUME_FIELD(cursor_advances); MHGP8_RESUME_FIELD(cursor_reuses);
  MHGP8_RESUME_FIELD(count_node_visits); MHGP8_RESUME_FIELD(count_bound_tests);
  MHGP8_RESUME_FIELD(count_point_tests); MHGP8_RESUME_FIELD(uniform_credited_pairs);
  MHGP8_RESUME_FIELD(uniform_rejected_pairs); MHGP8_RESUME_FIELD(uniform_accepted_pairs);
  MHGP8_RESUME_FIELD(consumed_witness_sites); MHGP8_RESUME_FIELD(frontier_restarts);
  MHGP8_RESUME_FIELD(payload_node_visits); MHGP8_RESUME_FIELD(payload_bound_tests);
  MHGP8_RESUME_FIELD(payload_point_tests); MHGP8_RESUME_FIELD(payload_interior_sites);
  MHGP8_RESUME_FIELD(payload_shell_sites); MHGP8_RESUME_FIELD(payload_supports);
#undef MHGP8_RESUME_FIELD
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 7) throw std::invalid_argument(
        "usage: mhgp8_q2_census_resume_probe n uniform|terrain|clusters|rows Kmax s seed quantum");
    const auto n = integer<std::size_t>(argv[1]);
    const std::string_view family = argv[2];
    const auto k = integer<unsigned>(argv[3]), separation = integer<unsigned>(argv[4]);
    const auto seed = integer<u64>(argv[5]);
    const auto quantum = integer<std::size_t>(argv[6]);
    mhgp8::bench::validate_front_fixture_size(n, family);
    if (k == 0 || k > 10 || separation == 0 || quantum == 0)
      throw std::invalid_argument("requires Kmax 1..10, positive separation and quantum");
    const auto started = Clock::now();
    auto fixture = mhgp8::bench::make_front_fixture(n, family, seed);
    const auto generation_ms = ms(started);
    auto begin = Clock::now();
    const auto cloud = mhgp8::prepare_cloud(fixture.points);
    const auto index = mhgp8::make_q2_cloud_index(cloud);
    const auto preparation_ms = ms(begin);
    const auto nodes = index->spatial_nodes();
    std::size_t selected_a{}, selected_b{};
    bool selected = false;
    begin = Clock::now();
    const auto front = mhgp8::run_wspd_front(*index, k, separation,
        mhgp8::WspdFrontMode::MidpointSamples, [&](const mhgp8::WspdRectangle& rectangle) {
          auto a = rectangle.a_node, b = rectangle.b_node;
          if (nodes[a].range.size() > nodes[b].range.size()) std::swap(a, b);
          // Largest B, then largest smaller factor, then smallest node IDs.
          // One reproducible root only; never a whole-front census measure.
          const auto priority = [&](std::size_t x, std::size_t y) {
            return std::tuple(nodes[y].range.size(), nodes[x].range.size(),
                std::numeric_limits<std::size_t>::max() - x,
                std::numeric_limits<std::size_t>::max() - y);
          };
          if (!selected || priority(a, b) > priority(selected_a, selected_b)) {
            selected = true; selected_a = a; selected_b = b;
          }
        }, 1);
    const auto front_ms = ms(begin);
    require(selected, "nonempty fixture lost every q2 root");
    const auto range = nodes[selected_a].range;
    const auto anchor_rank = range.first + range.size() / 2;
    Digest reference_digest, resume_digest;
    const auto reference = mhgp8::run_q2_anchor_reference(index, anchor_rank, selected_b, k,
        [&](const mhgp8::Q2Support& s) { reference_digest.consume(s); },
        mhgp8::Q2SiblingMode::Saturating, mhgp8::Q2WitnessOrder::ComplementFirst);
    begin = Clock::now();
    auto continuation = mhgp8::make_q2_census_continuation(index, anchor_rank, selected_b, k,
        mhgp8::Q2SiblingMode::Saturating, mhgp8::Q2WitnessOrder::ComplementFirst);
    const auto creation_ms = ms(begin);
    double max_slice_ms = 0;
    const mhgp8::Q2CensusConsumer consumer = [&](const mhgp8::Q2Support& s) { resume_digest.consume(s); };
    begin = Clock::now();
    for (;;) {
      const auto slice = Clock::now();
      const bool done = continuation->advance(quantum, consumer);
      max_slice_ms = std::max(max_slice_ms, ms(slice));
      if (done) break;
    }
    const auto resume_enclosing_ms = ms(begin);
    const auto result = continuation->snapshot();
    const auto memory = continuation->memory();
    const auto& c = result.census;
    require(c.work == reference.census.work && result.sibling_work == reference.sibling_work &&
            result.order_work == reference.order_work && c.candidate_pairs == reference.census.candidate_pairs &&
            c.accepted_pairs == reference.census.accepted_pairs && c.rejected_pairs == reference.census.rejected_pairs &&
            resume_digest == reference_digest, "resume/reference work or ordered payload mismatch");
    const auto& r = result.resume_work;
    std::cout.imbue(std::locale::classic());
    std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_q2_selected_anchor_resume_v1\",\"public_status\":\"not_claimed\","
      << "\"scope\":\"one_selected_anchor_not_whole_front_or_full\",\"full_contract_qualified\":false,"
      << "\"n\":" << n << ",\"family\":\"" << family << "\",\"kmax\":" << k
      << ",\"separation_s\":" << separation << ",\"seed\":" << seed << ",\"quantum\":" << quantum
      << ",\"input_hash\":" << fixture.input_hash << ",\"a_node\":" << selected_a << ",\"b_node\":" << selected_b
      << ",\"anchor_rank\":" << anchor_rank << ",\"anchor_id\":" << index->spatial_order()[anchor_rank]
      << ",\"a_size\":" << range.size() << ",\"b_size\":" << nodes[selected_b].range.size()
      << ",\"front_products\":" << front.work.product_visits << ",\"front_rectangles\":" << front.work.emitted_rectangles
      << ",\"front_residual_pairs\":" << front.work.residual_pair_mass[0]
      << ",\"candidate_pairs\":" << c.candidate_pairs << ",\"accepted_pairs\":" << c.accepted_pairs
      << ",\"rejected_pairs\":" << c.rejected_pairs << ",\"digest\":" << resume_digest.hash
      << ",\"work_equal\":true,\"ordered_payload_equal\":true,\"census_work\":{ ";
    print_work(c.work);
    const auto& sibling = result.sibling_work;
    const auto& order = result.order_work;
    std::cout << "},\"sibling_work\":{\"proposals\":" << sibling.proposals
      << ",\"cardinality_skips\":" << sibling.cardinality_skips << ",\"bound_tests\":" << sibling.bound_tests
      << ",\"rejected_tasks\":" << sibling.rejected_tasks << ",\"rejected_pairs\":" << sibling.rejected_pairs
      << ",\"rejected_after_credit\":" << sibling.rejected_after_credit
      << "},\"order_work\":{\"structural_splits\":" << order.structural_splits
      << ",\"deferred_skips\":" << order.deferred_skips << ",\"anchor_skips\":" << order.anchor_skips
      << ",\"phase_switches\":" << order.phase_switches
      << "},\"resume_work\":{\"advance_calls\":" << r.advance_calls
      << ",\"transitions\":" << r.transitions << ",\"entry_steps\":" << r.entry_steps
      << ",\"witness_steps\":" << r.witness_steps << ",\"admission_steps\":" << r.admission_steps
      << ",\"payload_steps\":" << r.payload_steps << ",\"pauses\":" << r.pauses
      << ",\"pauses_after_credit\":" << r.pauses_after_credit << ",\"pauses_inside_deferred\":" << r.pauses_inside_deferred
      << ",\"pauses_during_emission\":" << r.pauses_during_emission << ",\"max_pending_tasks\":" << r.max_pending_tasks
      << "},\"memory\":{\"stack_capacity\":" << memory.stack_capacity << ",\"stack_bytes\":" << memory.stack_bytes
      << ",\"payload_bytes\":" << memory.payload_bytes << ",\"retained_bytes\":" << memory.retained_bytes
      << "},\"timing_ms\":{\"generation\":" << generation_ms << ",\"preparation\":" << preparation_ms
      << ",\"front_selection\":" << front_ms << ",\"reference\":" << reference.census.total_ms
      << ",\"creation\":" << creation_ms << ",\"resume_active\":" << c.total_ms
      << ",\"resume_enclosing\":" << resume_enclosing_ms << ",\"max_slice\":" << max_slice_ms
      << ",\"probe\":" << ms(started) << "}}\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mhgp8_q2_census_resume_probe: " << e.what() << '\n';
    return 2;
  }
}
