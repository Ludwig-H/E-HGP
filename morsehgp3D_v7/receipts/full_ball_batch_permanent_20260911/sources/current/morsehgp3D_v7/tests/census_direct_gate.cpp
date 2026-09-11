// v7 : reference exhaustive des puissances, ordre de feuilles independant des
// bornes du census ; publication, residence et expansion reguliere controlees.
// Codes : 0 conforme, 1 desaccord, 2 arguments, 3 plancher, 4 mutant cible tue.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/q2.hpp"
#include "../src/pipeline/expand.hpp"

using namespace mhgp7;

namespace {

std::vector<i32> leaf_order(const CloudIndex& ix) {
  std::vector<i32> out;
  std::vector<NodeRef> pending{ix.root()};
  while (!pending.empty()) {
    const NodeRef v = pending.back();
    pending.pop_back();
    if (is_leaf(v)) out.push_back(leaf_index(v));
    else {
      pending.push_back(ix.nodes[(size_t)v].left);
      pending.push_back(ix.nodes[(size_t)v].right);
    }
  }
  return out;
}

BallData reference_ball(const CloudIndex& ix, const BallCandidate& c) {
  BallData b;
  b.key = c.key;
  b.level = c.level;
  b.arity = c.arity;
  for (const i32 u : leaf_order(ix)) {
    const i128 p = c.key.power(ix.upos[(size_t)u]);
    if (p < 0) b.interior_ids[b.n_interior++] = u;
    if (p == 0) b.shell_ids[b.n_shell++] = u;
  }
  return b;
}

bool equal_ball(const BallData& a, const BallData& b) {
  return a.key == b.key && a.level == b.level && a.arity == b.arity &&
      a.n_interior == b.n_interior && a.n_shell == b.n_shell &&
      std::equal(a.interior().begin(), a.interior().end(), b.interior().begin()) &&
      std::equal(a.shell().begin(), a.shell().end(), b.shell().begin());
}

bool equal_event(const ForestEvent& a, const ForestEvent& b) {
  return a.q == b.q && a.d == b.d && a.active_mask == b.active_mask && a.level == b.level &&
      std::equal(a.support, a.support + a.q, b.support) &&
      std::equal(a.interior, a.interior + a.d, b.interior);
}

struct Counts {
  u64 balls = 0, regular = 0, plateau = 0, events = 0, refusals = 0;
  u64 object_errors = 0, publish_errors = 0, residence_errors = 0;
};

void check_cloud(const CloudIndex& ix, Counts* counts) {
  std::vector<BallCandidate> cands;
  for (int a = 0; a < ix.unique_count(); ++a)
    for (int b = a + 1; b < ix.unique_count(); ++b) {
      const P3& x = ix.upos[(size_t)a];
      const P3& y = ix.upos[(size_t)b];
      const i64 dx = x.x - y.x, dy = x.y - y.y, dz = x.z - y.z;
      cands.push_back({q2_ball_key(x, y), promote_level(q2_exact_level(dx * dx + dy * dy + dz * dz)), 2});
    }
  rle_candidates(&cands);
  for (const u64 smax : {6ull, 11ull}) {
    std::vector<Survivor> survivors;
    std::vector<BallData> expected;
    for (size_t i = 0; i < cands.size(); ++i) {
      u64 interior = 0, shell = 0;
      for (const P3& p : ix.upos) {
        const i128 power = cands[i].key.power(p);
        interior += power < 0;
        shell += power == 0;
      }
      if (interior > smax - cands[i].arity || shell > kBallShellMax) continue;
      survivors.push_back({(u32)i, interior});
      expected.push_back(reference_ball(ix, cands[i]));
    }
    for (const int threads : {1, 4, 8}) {
      std::vector<BallData> actual(3);  // une ancienne sortie doit etre remplacee
      ExpandStats stats;
      const PipelineStatus rc = census_balls(ix, cands, survivors, smax, 12, threads, &actual, &stats);
      if (rc != PipelineStatus::kCompleteRegular || actual.size() != expected.size()) {
        ++counts->object_errors;
        continue;
      }
      counts->balls += actual.size();
      u64 int_total = 0, shell_total = 0;
      for (size_t i = 0; i < actual.size(); ++i) {
        counts->object_errors += !equal_ball(actual[i], expected[i]);
        int_total += expected[i].n_interior;
        shell_total += expected[i].n_shell;
        counts->regular += expected[i].n_shell == expected[i].arity;
        counts->plateau += expected[i].n_shell != expected[i].arity;
      }
      counts->object_errors += stats.census_interior != int_total || stats.census_shell != shell_total;
      counts->residence_errors += stats.census_merge_peak_bytes != actual.capacity() * sizeof(BallData);

      // Reference complete de plateaux pour TOUS les records, y compris les
      // reguliers : elle n'emprunte pas le raccourci std::array de v7.
      for (u64 k = 1; k < smax; ++k) {
        std::vector<ForestEvent> want, got;
        for (const BallData& bd : expected) {
          std::vector<PlateauEvent> pe;
          expand_plateau(ball_center(bd.key), ix.upos, bd.interior(), bd.shell(), (size_t)smax, &pe);
          for (const PlateauEvent& p : pe) {
            if (p.tpart.size() + p.ipart.size() != k + 1) continue;
            ForestEvent ev;
            ev.q = (u8)p.tpart.size();
            ev.d = (u8)p.ipart.size();
            ev.active_mask = p.active_mask;
            ev.level = bd.level;
            for (size_t i = 0; i < p.tpart.size(); ++i) ev.support[i] = ix.point_id(p.tpart[i]);
            for (size_t i = 0; i < p.ipart.size(); ++i) ev.interior[i] = ix.point_id(p.ipart[i]);
            want.push_back(ev);
          }
        }
        expand_events_k(ix, actual, k, smax - 1, threads, &got, &stats);
        counts->events += want.size();
        if (got.size() != want.size()) ++counts->object_errors;
        else for (size_t i = 0; i < got.size(); ++i) counts->object_errors += !equal_event(got[i], want[i]);
      }

      // Contradiction tardive count-only/census : tous les records precedents
      // peuvent deja etre ecrits, mais aucun ne doit quitter la transaction.
      if (survivors.size() >= 2) {
        auto wrong = survivors;
        ++wrong.back().depth;
        actual.resize(3);
        const PipelineStatus failed = census_balls(ix, cands, wrong, smax, 12, threads, &actual, &stats);
        counts->object_errors += failed != PipelineStatus::kInvariantViolated;
        counts->publish_errors += !actual.empty();
        ++counts->refusals;
      }
    }
  }
}

void check_shell_refusal(Counts* counts) {
  const CloudIndex ix = build_cloud_index(std::vector<P3>{{9,10,10}, {11,10,10}, {10,9,10},
                                                        {10,11,10}, {10,10,9}, {10,10,11}});
  const P3 a{9,10,10}, b{10,9,10}, opposite{11,10,10};
  const BallCandidate valid{q2_ball_key(a, b), promote_level(q2_exact_level(2)), 2};
  const BallCandidate overflow{q2_ball_key(a, opposite), promote_level(q2_exact_level(4)), 2};
  const std::vector<BallCandidate> cands{valid, overflow};
  const std::vector<Survivor> survivors{{0,0}, {1,0}};
  for (const int threads : {1,4}) {
    std::vector<BallData> out(1);
    ExpandStats stats;
    const PipelineStatus rc = census_balls(ix, cands, survivors, 11, 4, threads, &out, &stats);
    counts->object_errors += rc != PipelineStatus::kResourceExhausted;
    counts->publish_errors += !out.empty();
    ++counts->refusals;
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  if (argc == 2 && std::string(argv[1]).starts_with("--inject=")) inject = std::string(argv[1]).substr(9);
  else if (argc != 1) return 2;
  if (argc == 2 && inject.empty()) return 2;
  if (!inject.empty() && inject != "census-direct-offset" && inject != "census-direct-publish-prefix" &&
      inject != "keep-ball-chunks") return 2;
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  Counts c;
  check_cloud(build_cloud_index(make_family_input(CloudFamily::kUniform, 64, 200, 3)), &c);
  check_cloud(build_cloud_index(std::vector<P3>{{9,10,10}, {11,10,10}, {10,9,10},
                                               {10,11,10}, {10,10,9}, {10,10,11}}), &c);
  check_shell_refusal(&c);
  std::printf("census_direct version=%s balls=%llu regular=%llu plateau=%llu events=%llu refusals=%llu "
              "object_errors=%llu publish_errors=%llu residence_errors=%llu\n", kCensusStorageVersion,
              (unsigned long long)c.balls, (unsigned long long)c.regular, (unsigned long long)c.plateau,
              (unsigned long long)c.events, (unsigned long long)c.refusals, (unsigned long long)c.object_errors,
              (unsigned long long)c.publish_errors, (unsigned long long)c.residence_errors);
  if (c.balls < 100 || c.regular < 100 || c.plateau < 6 || c.events < 100 || c.refusals != 14) return 3;
  const bool object = c.object_errors != 0, publish = c.publish_errors != 0, residence = c.residence_errors != 0;
  if (inject.empty()) return object || publish || residence ? 1 : 0;
  if (inject == "census-direct-offset") return object && !publish && !residence ? 4 : 1;
  if (inject == "census-direct-publish-prefix") return !object && publish && !residence ? 4 : 1;
  return !object && !publish && residence ? 4 : 1;
}
