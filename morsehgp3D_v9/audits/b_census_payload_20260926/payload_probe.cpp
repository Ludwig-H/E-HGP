// Audit-only consumer experiment. No GPU producer and no product seal.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../src/tower/forest/ball_data.hpp"
#include "../../src/tower/lanes/q4.hpp"
#include "../../src/tower/pipeline/census.hpp"

namespace mhgp9::audit_payload {
using namespace tower;
using Clock = std::chrono::steady_clock;

void require(bool yes, const char* reason) {
  if (!yes) throw std::runtime_error(reason);
}

// The chain's original IDs are a permutation of 0..n-1. Neither input order
// nor Morton order is that ID order. Copy before certification; no mutable
// index or coordinates escape the owner.
class Owner {
 public:
  static std::shared_ptr<const Owner> make(std::span<const InputPoint> input) {
    return std::shared_ptr<const Owner>(new Owner(input));
  }
  const CloudIndex& index() const { return ix_; }
  i32 rank(PointId id) const {
    require(id < by_id_.size(), "id_outside_owner");
    return by_id_[id];
  }
  const P3& point(PointId id) const { return ix_.upos[static_cast<size_t>(rank(id))]; }
 private:
  explicit Owner(std::span<const InputPoint> input) {
    const std::vector<InputPoint> copied(input.begin(), input.end());
    ix_ = build_cloud_index(copied);
    require(ix_.valid && !ix_.has_duplicate_positions(), "owner_invalid");
    by_id_.assign(copied.size(), -1);
    for (size_t u = 0; u < ix_.upos.size(); ++u) {
      const PointId id = ix_.point_id(static_cast<i32>(u));
      require(id < by_id_.size() && by_id_[id] == -1, "owner_non_dense_ids");
      by_id_[id] = static_cast<i32>(u);
    }
  }
  CloudIndex ix_;
  std::vector<i32> by_id_;
};

struct Support {
  unsigned arity = 0;
  std::array<PointId, 4> ids{};
};

struct Geometry {
  BallKey key;
  ExactLevel level;
};

// Same u18 support formulas as the chain, with positivity checked before a
// denominator is consumed. No floating predicate, no approximate key.
Geometry geometry(const Owner& owner, const Support& support) {
  require(support.arity == 3 || support.arity == 4, "support_arity");
  std::array<P3, 4> p{};
  for (unsigned j = 0; j < support.arity; ++j) {
    for (unsigned i = 0; i < j; ++i) require(support.ids[i] != support.ids[j], "support_duplicate");
    p[j] = owner.point(support.ids[j]);
  }
  if (support.arity == 3) {
    const Q3Form f = q3_form(p[0], p[1], p[2]);
    require(f.g > 0 && p3_dot(p3_sub(p[1], p[0]), p3_sub(p[2], p[0])) > 0 &&
                p3_dot(p3_sub(p[0], p[1]), p3_sub(p[2], p[1])) > 0 &&
                p3_dot(p3_sub(p[0], p[2]), p3_sub(p[1], p[2])) > 0, "support_nonpositive");
    return {q3_ball_key(f), promote_level(q3_exact_level(p[0], p[1], p[2]))};
  }
  const Q4Form f = q4_form(p[0], p[1], p[2], p[3]);
  require(f.det > 0 && q4_center_strictly_inside(f, p[0], p[1], p[2], p[3]), "support_nonpositive");
  return {ball_key_reduce(q4_ball_form(f)), q4_level_raw(f)};
}

struct Scratch {
  std::vector<i32> in, shell;
  std::vector<NodeRef> stack;
  DepthStats work;
};

// Private construction is deliberate: this prototype certifies counts by a
// global reference census, not by accepting arbitrary caller-supplied counts.
// AuditAccess below is the test-only corruption seam.
class Packet {
 public:
  static Packet certify(std::shared_ptr<const Owner> owner, Support support) {
    require(owner != nullptr, "null_owner");
    const Geometry g = geometry(*owner, support);
    Scratch st;
    require(ball_census(owner->index(), g.key, std::numeric_limits<size_t>::max(),
                        std::numeric_limits<size_t>::max(), &st.in, &st.shell, &st.work, &st.stack) == CensusStatus::kOk,
            "producer_census");
    Packet out(std::move(owner), support, g.key, st.in.size(), st.shell.size());
    for (i32 u : st.in) out.interior_.push_back(out.owner_->index().point_id(u));
    return out;
  }
  unsigned arity() const { return support_.arity; }
  size_t depth() const { return depth_; }
  size_t shell() const { return shell_; }
  const std::vector<PointId>& interior() const { return interior_; }
  const std::shared_ptr<const Owner>& owner() const { return owner_; }
  const Support& support() const { return support_; }
  const BallKey& key() const { return key_; }
 private:
  Packet(std::shared_ptr<const Owner> owner, Support support, BallKey key, size_t depth, size_t shell)
      : owner_(std::move(owner)), support_(support), key_(key), depth_(depth), shell_(shell) {}
  friend struct AuditAccess;
  std::shared_ptr<const Owner> owner_;
  Support support_;
  BallKey key_;
  size_t depth_, shell_;
  std::vector<PointId> interior_;
};

struct AuditAccess {
  static void missing(Packet& p) { p.interior_.pop_back(); }
  static void duplicate(Packet& p) { p.interior_[1] = p.interior_[0]; }
  static void boundary(Packet& p) { p.interior_[0] = p.support_.ids[0]; }
  static void outside(Packet& p) { p.interior_[0] = UINT32_MAX; }
  static void foreign_inside(Packet& p, PointId id) { p.interior_[0] = id; }
  static void wrong_key(Packet& p) { ++p.key_.c; }
  static void wrong_shell(Packet& p) { p.shell_ = p.support_.arity - 1; }
  static void wrong_depth(Packet& p) { ++p.depth_; }
  static void wrong_support(Packet& p) { p.support_.ids[1] = p.support_.ids[0]; }
  static void reorder(Packet& p) { std::reverse(p.interior_.begin(), p.interior_.end()); }
  static void drop_and_forge_count(Packet& p) { p.interior_.pop_back(); --p.depth_; }
};

enum class ImportStatus { regular, needs_global_census };

// Consumer: O(K) geometry/membership checks and a tiny O(K log K) sort, no
// search of the global tree. Completeness requires the packet producer's
// exact counts; local validation alone cannot authenticate forged counts.
ImportStatus import_regular(const Owner& owner, const Packet& p, unsigned kmax, BallData& out) {
  require(p.owner().get() == &owner, "packet_owner_mismatch");
  require(kmax >= 1 && kmax <= 10, "kmax_domain");
  const Geometry g = geometry(owner, p.support());
  require(g.key == p.key(), "packet_key_mismatch");
  require(p.shell() >= p.arity(), "packet_shell_below_support");
  if (p.shell() != p.arity()) return ImportStatus::needs_global_census;
  require(p.depth() == p.interior().size(), "packet_depth_mismatch");
  require(p.depth() + p.arity() <= static_cast<size_t>(kmax) + 1 && p.depth() <= kBallInteriorMax,
          "packet_rank_window");
  BallData b{};
  b.key = g.key; b.level = g.level; b.arity = static_cast<u8>(p.arity());
  b.n_interior = static_cast<u8>(p.depth()); b.n_shell = static_cast<u8>(p.shell());
  for (size_t i = 0; i < p.depth(); ++i) {
    const i32 u = owner.rank(p.interior()[i]);
    require(g.key.power(owner.index().upos[static_cast<size_t>(u)]) < 0, "packet_noninterior_id");
    b.interior_ids[i] = u;
  }
  std::sort(b.interior_ids, b.interior_ids + b.n_interior);
  require(std::adjacent_find(b.interior_ids, b.interior_ids + b.n_interior) == b.interior_ids + b.n_interior,
          "packet_duplicate_interior");
  for (unsigned i = 0; i < p.arity(); ++i) {
    const i32 u = owner.rank(p.support().ids[i]);
    require(g.key.power(owner.index().upos[static_cast<size_t>(u)]) == 0, "packet_nonshell_support");
    b.shell_ids[i] = u;
  }
  std::sort(b.shell_ids, b.shell_ids + b.n_shell);
  out = b;  // nothing is published on a refusal or fallback
  return ImportStatus::regular;
}

BallData recensus_regular(const Owner& owner, const Packet& p, unsigned kmax, Scratch& st) {
  require(p.owner().get() == &owner, "packet_owner_mismatch");
  const Geometry g = geometry(owner, p.support());
  require(g.key == p.key(), "packet_key_mismatch");
  require(ball_census(owner.index(), g.key, p.depth(), std::numeric_limits<size_t>::max(),
                      &st.in, &st.shell, &st.work, &st.stack) == CensusStatus::kOk, "reference_overflow");
  require(st.in.size() == p.depth() && st.shell.size() == p.shell(), "reference_count_mismatch");
  require(st.shell.size() == p.arity(), "reference_extra_shell");
  require(st.in.size() + p.arity() <= static_cast<size_t>(kmax) + 1 && st.in.size() <= kBallInteriorMax,
          "reference_rank_window");
  BallData b{};
  b.key = g.key; b.level = g.level; b.arity = static_cast<u8>(p.arity());
  b.n_interior = static_cast<u8>(st.in.size()); b.n_shell = static_cast<u8>(st.shell.size());
  std::sort(st.in.begin(), st.in.end()); std::sort(st.shell.begin(), st.shell.end());
  std::copy(st.in.begin(), st.in.end(), b.interior_ids);
  std::copy(st.shell.begin(), st.shell.end(), b.shell_ids);
  return b;
}

bool same(const BallData& a, const BallData& b) {
  return a.key == b.key && a.level == b.level && a.arity == b.arity && a.n_interior == b.n_interior &&
      a.n_shell == b.n_shell && std::equal(a.interior().begin(), a.interior().end(), b.interior().begin()) &&
      std::equal(a.shell().begin(), a.shell().end(), b.shell().begin());
}

struct Scene {
  std::shared_ptr<const Owner> owner;
  std::vector<Packet> packets;
  double owner_ms = 0, packet_producer_ms = 0;
};

double elapsed(Clock::time_point t) { return std::chrono::duration<double, std::milli>(Clock::now() - t).count(); }

Scene make_scene(size_t n, std::uint32_t seed) {
  require(n >= 12 && n <= 196608, "synthetic_n_domain");
  std::vector<InputPoint> input;
  std::vector<Support> supports;
  for (size_t cluster = 0; input.size() + 6 <= n; ++cluster) {
    const P3 c{32 + 64 * static_cast<i64>(cluster % 32), 32 + 64 * static_cast<i64>((cluster / 32) % 32),
               32 + 64 * static_cast<i64>(cluster / 1024)};
    const unsigned q = cluster % 2 ? 4 : 3;
    const std::array<P3, 6> triangle{{{-12, 0, 0}, {12, 0, 0}, {0, 16, 0}, {0, 4, 0}, {0, 5, 0}, {1, 4, 0}}};
    const std::array<P3, 6> tetra{{{10, 10, 10}, {10, -10, -10}, {-10, 10, -10}, {-10, -10, 10}, {0, 0, 0}, {1, 0, 0}}};
    Support s{q, {}};
    for (size_t j = 0; j < 6; ++j) {
      const PointId id = static_cast<PointId>(input.size());
      if (j < q) s.ids[j] = id;
      input.push_back({id, p3_add(c, q == 3 ? triangle[j] : tetra[j])});
    }
    supports.push_back(s);
  }
  while (input.size() < n) input.push_back({static_cast<PointId>(input.size()),
                                         {200000 + static_cast<i64>(input.size() % 1000), 7, 9}});
  std::vector<PointId> permutation(n);
  std::iota(permutation.begin(), permutation.end(), PointId{0});
  std::mt19937 rng(seed);
  std::shuffle(permutation.begin(), permutation.end(), rng);
  for (auto& p : input) p.id = permutation[p.id];
  for (auto& s : supports) for (unsigned j = 0; j < s.arity; ++j) s.ids[j] = permutation[s.ids[j]];
  std::shuffle(input.begin(), input.end(), rng);  // physical order also differs
  Scene scene;
  auto t = Clock::now(); scene.owner = Owner::make(input); scene.owner_ms = elapsed(t);
  // Copy-isolation test: changing caller storage cannot change the packet.
  for (auto& p : input) p.position = {0, 0, 0};
  t = Clock::now();
  for (const auto& s : supports) scene.packets.push_back(Packet::certify(scene.owner, s));
  scene.packet_producer_ms = elapsed(t);
  return scene;
}

template<class Fn> void rejects(Fn&& fn, const char* expected) {
  try { fn(); } catch (const std::runtime_error& e) {
    require(std::string(e.what()) == expected, "wrong_refusal"); return;
  }
  throw std::runtime_error("missing_refusal");
}

void selftest() {
  size_t compared = 0, refusal_count = 0, permutations = 0;
  for (const std::uint32_t seed : {1U, 7U, 42U}) {
    auto scene = make_scene(192, seed);
    Scratch scratch;
    for (auto p : scene.packets) {
      const auto ref = recensus_regular(*scene.owner, p, 5, scratch);
      BallData b{};
      require(import_regular(*scene.owner, p, 5, b) == ImportStatus::regular && same(ref, b), "differential");
      ++compared;
      AuditAccess::reorder(p);
      require(import_regular(*scene.owner, p, 10, b) == ImportStatus::regular && same(ref, b), "id_order");
      ++permutations;
    }
    const Packet p = scene.packets.front();
    const auto corrupt = [&](auto mutation, const char* expected) {
      Packet bad = p; mutation(bad); BallData b{};
      rejects([&] { (void)import_regular(*scene.owner, bad, 5, b); }, expected); ++refusal_count;
    };
    corrupt(AuditAccess::missing, "packet_depth_mismatch");
    corrupt(AuditAccess::duplicate, "packet_duplicate_interior");
    corrupt(AuditAccess::boundary, "packet_noninterior_id");
    corrupt(AuditAccess::outside, "id_outside_owner");
    corrupt(AuditAccess::wrong_key, "packet_key_mismatch");
    corrupt(AuditAccess::wrong_shell, "packet_shell_below_support");
    corrupt(AuditAccess::wrong_depth, "packet_depth_mismatch");
    corrupt(AuditAccess::wrong_support, "support_duplicate");
    corrupt([&](Packet& bad) { AuditAccess::foreign_inside(bad, scene.packets.back().interior().front()); },
            "packet_noninterior_id");
    auto other = make_scene(192, seed);
    BallData b{};
    rejects([&] { (void)import_regular(*other.owner, p, 5, b); }, "packet_owner_mismatch"); ++refusal_count;
    rejects([&] { (void)import_regular(*scene.owner, p, 4, b); }, "packet_rank_window"); ++refusal_count;
    // Deliberately publish the limitation: local tests cannot reject a
    // coordinated corruption of BOTH the certified count and its ID list.
    Packet forged = p; AuditAccess::drop_and_forge_count(forged);
    require(import_regular(*scene.owner, forged, 5, b) == ImportStatus::regular, "forgery_fixture_not_exercised");
    rejects([&] { (void)recensus_regular(*scene.owner, forged, 5, scratch); }, "reference_overflow");
  }
  // Positive q3 on a sphere with a fourth shell point: must fall back, not
  // silently replace the complete shell by the three support IDs.
  const std::vector<InputPoint> extra{{0,{8,4,4}}, {1,{4,8,4}}, {2,{0,4,4}}, {3,{4,0,4}}, {4,{4,4,4}}};
  const auto extra_owner = Owner::make(extra);
  // The first three are a right triangle, deliberately rejected BEFORE key
  // formation. A strictly acute four-point circle fixture follows.
  rejects([&] { (void)Packet::certify(extra_owner, {3,{0,1,2,0}}); }, "support_nonpositive"); ++refusal_count;
  const std::vector<InputPoint> ring{{0,{15,10,10}}, {1,{7,14,10}}, {2,{7,6,10}}, {3,{10,15,10}}, {4,{10,10,10}}};
  const auto ring_owner = Owner::make(ring);
  const Packet e = Packet::certify(ring_owner, {3,{0,1,2,0}});
  BallData untouched{}; untouched.arity = 99;
  require(e.shell() == 4 && import_regular(*ring_owner, e, 5, untouched) == ImportStatus::needs_global_census &&
              untouched.arity == 99, "extra_shell_fallback");
  // Owner remains alive after the factory's external handle is released.
  Packet lifetime = [] { auto s = make_scene(12, 99); return s.packets.front(); }();
  BallData b{};
  require(import_regular(*lifetime.owner(), lifetime, 5, b) == ImportStatus::regular, "owner_lifetime");
  // Empty interior, maximum admitted interior at K10, wide u18 coordinates,
  // and every permutation of the support: positivity/orientation and the
  // support's raw q4 fraction need not have one representation, hence exact
  // level comparison for that permutation gate.
  for (unsigned q : {3U, 4U}) for (i64 scale : {i64{1}, i64{127}, i64{4000}}) for (bool full : {false, true}) {
    const std::array<P3, 4> triangle{{{-12, 0, 0}, {12, 0, 0}, {0, 16, 0}, {}}};
    const std::array<P3, 4> tetra{{{10, 10, 10}, {10, -10, -10}, {-10, 10, -10}, {-10, -10, 10}}};
    std::vector<InputPoint> input;
    const auto push = [&](P3 p) {
      input.push_back({static_cast<PointId>(input.size()),
                       {150000 + p.x * scale, 150000 + p.y * scale, 150000 + p.z * scale}});
    };
    for (unsigned j = 0; j < q; ++j) push(q == 3 ? triangle[j] : tetra[j]);
    if (full) for (unsigned j = 0; j < 11 - q; ++j) push({static_cast<i64>(j), q == 3 ? 4 : 0, 0});
    const auto owner = Owner::make(input);
    std::array<PointId, 4> ids{0, 1, 2, 3};
    std::optional<BallData> first;
    do {
      const Packet p = Packet::certify(owner, {q, ids});
      Scratch scratch;
      const BallData ref = recensus_regular(*owner, p, 10, scratch);
      require(import_regular(*owner, p, 10, b) == ImportStatus::regular && same(ref, b), "extreme_differential");
      require(p.depth() == (full ? 11 - q : 0), "extreme_depth");
      if (first) require(first->key == b.key && same_exact_level(first->level, b.level) &&
                             std::equal(first->interior().begin(), first->interior().end(), b.interior().begin()),
                         "support_permutation");
      else first = b;
      ++compared; ++permutations;
    } while (std::next_permutation(ids.begin(), ids.begin() + q));
  }
  std::cout << "{\"mode\":\"selftest\",\"status\":\"pass\",\"differential\":" << compared
            << ",\"id_permutations\":" << permutations << ",\"refusals\":" << refusal_count
            << ",\"extra_shell_fallbacks\":1,\"coordinated_count_forgery_survives_local\":3}\n";
}

u64 digest(const BallData& b) {
  u64 h = static_cast<u64>(b.key.c) ^ static_cast<u64>(b.level.den) ^ b.arity;
  for (i32 u : b.interior()) h = (h * 1099511628211ULL) ^ static_cast<u64>(u);
  for (i32 u : b.shell()) h = (h * 1099511628211ULL) ^ static_cast<u64>(u);
  return h;
}

void benchmark(size_t n, size_t repeats) {
  require(repeats > 0, "benchmark_repetitions");
  auto scene = make_scene(n, 20260926);
  Scratch st;
  for (const auto& p : scene.packets) {
    BallData b{};
    require(import_regular(*scene.owner, p, 5, b) == ImportStatus::regular &&
                same(b, recensus_regular(*scene.owner, p, 5, st)), "benchmark_differential");
  }
  for (size_t rep = 0; rep < repeats; ++rep) {
    double local_ms = 0, global_ms = 0;
    u64 local_digest = 0, global_digest = 0;
    DepthStats global_work;
    const auto local = [&] {
      const auto t = Clock::now();
      for (const auto& p : scene.packets) { BallData b{}; require(import_regular(*scene.owner, p, 5, b) == ImportStatus::regular,
          "benchmark_fallback"); local_digest += digest(b); }
      local_ms = elapsed(t);
    };
    const auto global = [&] {
      st.work = {};
      const auto t = Clock::now();
      for (const auto& p : scene.packets) global_digest += digest(recensus_regular(*scene.owner, p, 5, st));
      global_ms = elapsed(t); global_work = st.work;
    };
    if (rep % 2) { local(); global(); } else { global(); local(); }
    require(local_digest == global_digest, "benchmark_digest");
    std::cout << "{\"mode\":\"benchmark_consumer_only\",\"sites\":" << n << ",\"packets\":" << scene.packets.size()
              << ",\"repeat\":" << rep << ",\"workers\":1,\"owner_ms\":" << scene.owner_ms
              << ",\"packet_producer_ms_excluded\":" << scene.packet_producer_ms << ",\"recensus_ms\":" << global_ms
              << ",\"import_ms\":" << local_ms << ",\"recensus_nodes\":" << global_work.nodes
              << ",\"recensus_leaf_tests\":" << global_work.leaf_tests << ",\"import_tree_nodes\":0,\"digest\":"
              << local_digest << "}\n";
  }
}
}  // namespace mhgp9::audit_payload

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--selftest") { mhgp9::audit_payload::selftest(); return 0; }
    if (argc == 4 && std::string(argv[1]) == "--benchmark") {
      mhgp9::audit_payload::benchmark(std::stoull(argv[2]), std::stoull(argv[3])); return 0;
    }
    std::cerr << "usage: payload_probe --selftest | --benchmark n repetitions\n"; return 2;
  } catch (const std::exception& e) { std::cerr << "failure=" << e.what() << '\n'; return 1; }
}
