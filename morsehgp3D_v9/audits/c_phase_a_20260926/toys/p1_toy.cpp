// P1 toy (scratch, not a receipt): today's phase-A loop (order_lots /
// order_block_lean / order_lot / order_new_node / order_root of
// full_ball_tower.hpp @ f44a8db03; flat draft reduced to parents +
// contributions) with ONE helper thread that precomputes hint(f) by chasing
// `compressed` read-only from anchors[tau(f)].  The committer accepts a hint
// iff hint < prior_count of the current lot, else resolves from the anchor.
//
// Protocol argued sufficient by the lens:
//  * every concurrent access to compressed / anchors through std::atomic_ref
//    (relaxed), including the committer's own loads and stores;
//  * a NEW node's slot is constructed by push_back (plain) and published by
//    a release store of `committed` (node count) after each lot; the helper
//    never dereferences a node >= its acquire-loaded `committed` (it
//    re-acquires once, then stops at the last node below it);
//  * the helper reads raw pointers captured AFTER reserve_lean (N <= n0 + M
//    -> no reallocation; data() equality checked at the end);
//  * one hint slot per facet ordinal (identity by index), NONE sentinel,
//    release store / acquire load;
//  * the helper is started after reserve and stopped + joined on every exit.
//
// Modes: default (safe), --racy (the committer keeps today's PLAIN writes to
// compressed / anchors, full_ball_tower.hpp:1154, 1167, 1301, 1350: TSan must
// report), --noprior (accept any hint: today's order_root require then
// throws on a hint >= prior_count; only reachable by a forced hint).
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using u64 = std::uint64_t;
using u32 = std::uint32_t;
static constexpr u64 absent = ~u64{0};
static constexpr u32 kAbsent32 = ~u32{0};
static constexpr u64 NONE = ~u64{0};

struct Inst {
  u32 n0 = 0;
  std::vector<u32> runs;
  std::vector<std::vector<std::pair<bool, u32>>> facets;  // (is_site, site | earlier position)
  std::vector<char> contrib;
};

static Inst make_instance(std::mt19937_64& rng, bool k1, u32 M) {
  Inst in;
  in.n0 = k1 ? 1 + u32(rng() % 64) : 0;
  std::uniform_real_distribution<double> U(0, 1);
  const double p = (rng() % 3 == 0) ? 0.2 : ((rng() % 2) ? 0.5 : 0.8);
  u32 r = 0;
  for (u32 j = 0; j < M; ++j) { if (U(rng) < p) ++r; in.runs.push_back(r); }
  const u32 r0 = in.runs[0];
  for (auto& x : in.runs) x -= r0;
  std::vector<u32> lot_begin(M);
  for (u32 j = 0; j < M; ++j) lot_begin[j] = (j && in.runs[j] == in.runs[j - 1]) ? lot_begin[j - 1] : j;
  in.facets.resize(M);
  static const int cs[] = {0, 0, 1, 1, 2, 2, 3, 4};
  for (u32 j = 0; j < M; ++j) {
    const u32 lower = lot_begin[j];
    const int c = cs[rng() % 8];
    for (int f = 0; f < c; ++f) {
      if (k1 && (lower == 0 || U(rng) < 0.5)) in.facets[j].push_back({true, u32(rng() % in.n0)});
      else if (lower) {
        const u32 t = (U(rng) < 0.5) ? u32(rng() % lower) : u32(lower - 1 - rng() % std::min<u32>(lower, 64));
        in.facets[j].push_back({false, t});
      }
    }
    in.contrib.push_back(in.facets[j].empty() ? 1 : (U(rng) < 0.3));
  }
  return in;
}

struct Out {
  std::vector<u64> next, parents, parent_begin;
  std::vector<u32> birth, runs, anchors, contribs;
  u64 births = 0, merges = 0, inert = 0, grouped = 0, singleton = 0, reps = 0, live = 0;
  bool operator==(const Out&) const = default;
};

enum class Mode { Plain, Safe, Racy, NoPrior };

struct Order {
  const Inst& in;
  Mode mode;
  Out o;
  std::vector<u64> comp;
  std::vector<u64> hint;
  std::atomic<u64> committed{0}, consumed{0};
  u64 accepted = 0, rejected = 0, missing = 0, hops = 0;
  u64* comp_data = nullptr;
  u32* anchors_data = nullptr;

  Order(const Inst& i, Mode m) : in(i), mode(m) {}
  bool atomic_mode() const { return mode == Mode::Safe || mode == Mode::NoPrior; }
  u64 load(u64 i) {
    return atomic_mode() ? std::atomic_ref<u64>(comp[i]).load(std::memory_order_relaxed) : comp[i];
  }
  void store(u64 i, u64 v) {
    if (atomic_mode()) std::atomic_ref<u64>(comp[i]).store(v, std::memory_order_relaxed);
    else comp[i] = v;
  }
  u32 anchor_load(u32 b) {
    return atomic_mode() ? std::atomic_ref<u32>(o.anchors[b]).load(std::memory_order_relaxed) : o.anchors[b];
  }
  void anchor_store(u32 b, u32 v) {
    if (atomic_mode()) std::atomic_ref<u32>(o.anchors[b]).store(v, std::memory_order_relaxed);
    else o.anchors[b] = v;
  }
  u64 root(u64 token, u64 prior) {  // order_root (1149-1158)
    if (!(token < prior)) throw std::runtime_error("full_ball_anchor_not_prior");
    u64 r = token;
    for (u64 x; (x = load(r)) != r; r = x) ++hops;
    for (u64 x; (x = load(token)) != token; token = x) store(token, r);
    if (!(r < prior && o.next[r] == absent)) throw std::runtime_error("full_ball_root_not_prior");
    return r;
  }
  u64 new_node(const std::vector<u64>& parents, u32 birth, u32 run) {  // order_new_node (1160-1172)
    const u64 id = o.next.size();
    o.next.push_back(absent);
    comp.push_back(id);
    for (u64 p : parents) { o.next[p] = id; store(p, id); }
    o.birth.push_back(birth);
    o.runs.push_back(run);
    if (parents.empty()) ++o.births; else ++o.merges;
    return id;
  }
  void run(const std::function<void()>& after_reserve) {
    const u32 M = u32(in.runs.size());
    o.anchors.assign(M, kAbsent32);                        // 1355
    for (u32 j = 0; j < in.n0; ++j) new_node({}, kAbsent32, 0);  // K1 domain (1365-1370), BEFORE reserve
    o.next.reserve(in.n0 + M); comp.reserve(in.n0 + M);   // reserve_lean (1228-1229)
    comp_data = comp.data(); anchors_data = o.anchors.data();
    committed.store(o.next.size(), std::memory_order_release);
    after_reserve();
    u64 ordinal = 0;
    std::vector<std::vector<u64>> roots;
    for (u32 begin = 0; begin < M;) {
      u32 end = begin + 1;
      while (end < M && in.runs[end] == in.runs[begin]) ++end;
      const u64 prior = o.next.size();
      const u32 lot_run = in.runs[begin] + 1;
      roots.assign(end - begin, {});
      for (u32 j = begin; j < end; ++j) {
        for (auto [site, t] : in.facets[j]) {
          ++o.reps;
          const u64 c = ordinal++;
          u64 anchor;
          if (site) anchor = t;
          else {
            if (!(in.runs[t] < in.runs[j])) throw std::runtime_error("full_ball_static_target_not_strict");
            const u32 a = anchor_load(t);
            if (a == kAbsent32) throw std::runtime_error("full_ball_static_closed_anchor_missing");
            anchor = a;
          }
          u64 r;
          if (mode == Mode::Plain) r = root(anchor, prior);
          else {
            consumed.store(c + 1, std::memory_order_relaxed);
            const u64 h = std::atomic_ref<u64>(hint[c]).load(std::memory_order_acquire);
            if (h == NONE) { ++missing; r = root(anchor, prior); }
            else if (mode == Mode::NoPrior || h < prior) { ++accepted; r = root(h, prior); }
            else { ++rejected; r = root(anchor, prior); }
          }
          roots[j - begin].push_back(r);
        }
        auto& rs = roots[j - begin];
        std::sort(rs.begin(), rs.end());
        rs.erase(std::unique(rs.begin(), rs.end()), rs.end());
      }
      const u32 n = end - begin;  // order_lot (1277-1352)
      if (n == 1) ++o.singleton; else ++o.grouped;
      std::vector<u32> dsu(n);
      for (u32 b = 0; b < n; ++b) dsu[b] = b;
      auto find = [&](u32 a) { while (dsu[a] != a) { dsu[a] = dsu[dsu[a]]; a = dsu[a]; } return a; };
      std::vector<std::pair<u64, u32>> owners;
      for (u32 b = 0; b < n; ++b) for (u64 p : roots[b]) owners.push_back({p, b});
      std::sort(owners.begin(), owners.end());
      for (size_t k = 1; k < owners.size(); ++k)
        if (owners[k - 1].first == owners[k].first) {
          const u32 a = find(owners[k - 1].second), b = find(owners[k].second);
          dsu[std::max(a, b)] = std::min(a, b);
        }
      std::vector<std::vector<u32>> groups(n);
      for (u32 b = 0; b < n; ++b) groups[find(b)].push_back(b);
      std::vector<u64> targets(n, absent);
      for (auto& g : groups) {
        if (g.empty()) continue;
        std::vector<u64> parents;
        std::vector<u32> cons;
        for (u32 b : g) {
          parents.insert(parents.end(), roots[b].begin(), roots[b].end());
          if (in.contrib[begin + b]) cons.push_back(begin + b);
        }
        std::sort(parents.begin(), parents.end());
        parents.erase(std::unique(parents.begin(), parents.end()), parents.end());
        u64 target;
        if (parents.size() == 1) target = parents.front();
        else {
          if (parents.empty() && !(g.size() == 1 && cons.size() == 1)) throw std::runtime_error("full_ball_distinct_births");
          target = new_node(parents, parents.empty() ? begin + g.front() : kAbsent32, lot_run);
        }
        for (u32 b : g) targets[b] = target;
        if (parents.size() != 1 || !cons.empty()) {
          o.parents.insert(o.parents.end(), parents.begin(), parents.end());
          o.parent_begin.push_back(o.parents.size());
          o.contribs.insert(o.contribs.end(), cons.begin(), cons.end());
        } else o.inert += g.size();
      }
      for (u32 b = 0; b < n; ++b) {
        if (o.anchors[begin + b] != kAbsent32) throw std::runtime_error("full_ball_anchor_duplicate");
        anchor_store(begin + b, u32(targets[b]));
      }
      committed.store(o.next.size(), std::memory_order_release);  // lot closed: nodes + anchors published
      begin = end;
    }
    if (comp.data() != comp_data) throw std::runtime_error("reallocated");
    for (u64 x : o.next) o.live += x == absent;  // abstract instances may end with several components (recorded, compared)
  }
};

// The helper: hint(c) for each facet ordinal, in order.
static void helper(Order& R, const std::vector<std::pair<bool, u32>>& targets, const std::atomic<bool>& stop,
                   u64& published, u64& waits) {
  u64* comp = R.comp_data;
  u32* anchors = R.anchors_data;
  u64 committed = R.committed.load(std::memory_order_acquire);
  for (u64 c = 0; c < targets.size(); ++c) {
    if (stop.load(std::memory_order_relaxed)) return;
    if (const u64 done = R.consumed.load(std::memory_order_relaxed); done > c) {  // behind: jump ahead of the committer
      c = done + 16;
      if (c >= targets.size()) return;
    }
    u64 a;
    if (targets[c].first) a = targets[c].second;
    else {
      u32 v;
      for (;;) {
        v = std::atomic_ref<u32>(anchors[targets[c].second]).load(std::memory_order_relaxed);
        if (v != kAbsent32) break;
        if (R.consumed.load(std::memory_order_relaxed) > c || stop.load(std::memory_order_relaxed)) break;
        ++waits;
        std::this_thread::yield();
      }
      if (v == kAbsent32) continue;
      a = v;
    }
    if (a >= committed) {
      committed = R.committed.load(std::memory_order_acquire);
      if (a >= committed) continue;
    }
    u64 x = a;
    for (;;) {
      const u64 v = std::atomic_ref<u64>(comp[x]).load(std::memory_order_relaxed);
      if (v == x) break;
      if (v >= committed) {
        committed = R.committed.load(std::memory_order_acquire);
        if (v >= committed) break;
      }
      x = v;
    }
    std::atomic_ref<u64>(R.hint[c]).store(x, std::memory_order_release);
    ++published;
  }
}

int main(int argc, char** argv) {
  Mode mode = Mode::Safe;
  u64 count = 200, seed = 20260926;
  u32 maxM = 4000;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--racy") mode = Mode::Racy;
    else if (a == "--noprior") mode = Mode::NoPrior;
    else if (a.rfind("--count=", 0) == 0) count = std::stoull(a.substr(8));
    else if (a.rfind("--seed=", 0) == 0) seed = std::stoull(a.substr(7));
    else if (a.rfind("--maxM=", 0) == 0) maxM = u32(std::stoul(a.substr(7)));
  }
  std::mt19937_64 rng(seed);
  u64 same = 0, diff = 0, fails = 0, acc = 0, rej = 0, mis = 0, pub = 0, waits = 0, hops_plain = 0, hops_hint = 0;
  u64 facets = 0;
  std::string first_fail;
  for (u64 i = 0; i < count; ++i) {
    const Inst in = make_instance(rng, i % 2 == 0, 1 + u32(rng() % maxM));
    Order P(in, Mode::Plain);
    P.run([] {});
    hops_plain += P.hops;
    facets += P.o.reps;
    Order H(in, mode);
    std::vector<std::pair<bool, u32>> targets;
    for (auto& fs : in.facets) for (auto& f : fs) targets.push_back(f);
    H.hint.assign(targets.size(), NONE);
    std::atomic<bool> stop{false};
    std::thread th;
    u64 published = 0, w = 0;
    try {
      struct Join {
        std::atomic<bool>& s; std::thread& t;
        ~Join() { s.store(true); if (t.joinable()) t.join(); }
      } join{stop, th};
      H.run([&] { th = std::thread([&] { helper(H, targets, stop, published, w); }); });
      // run() returned: stop + join before comparing (Join dtor).
    } catch (const std::exception& e) {
      ++fails;
      if (first_fail.empty()) first_fail = e.what();
      continue;
    }
    acc += H.accepted; rej += H.rejected; mis += H.missing; pub += published; waits += w; hops_hint += H.hops;
    if (H.o == P.o) ++same; else ++diff;
  }
  std::printf("mode=%s count=%llu facets=%llu same=%llu diff=%llu fails=%llu first_fail=%s\n",
              mode == Mode::Safe ? "safe" : mode == Mode::Racy ? "racy" : "noprior", (unsigned long long)count,
              (unsigned long long)facets, (unsigned long long)same, (unsigned long long)diff,
              (unsigned long long)fails, first_fail.c_str());
  std::printf("hints: published=%llu accepted=%llu rejected=%llu missing=%llu helper_waits=%llu\n",
              (unsigned long long)pub, (unsigned long long)acc, (unsigned long long)rej, (unsigned long long)mis,
              (unsigned long long)waits);
  std::printf("committer find hops: plain=%llu hinted=%llu (not a tower_work counter)\n",
              (unsigned long long)hops_plain, (unsigned long long)hops_hint);
  return (diff || fails) ? 1 : 0;
}
