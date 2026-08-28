// MorseHGP3D v5 — PORTE DU RAFFINEMENT POST-SEPARATION
// (docs/MESURES_ECHELLE.md § 4). Le raffinement prolonge la descente ternaire
// A L INTERIEUR d'un rectangle vivant : les sous-rectangles PARTITIONNENT les
// paires du parent. Cette partition est necessaire mais ne suffit pas a prouver
// la conservation q3/q4, qui depend aussi du recomptage ponctuel exact de Wq
// avant les seeds. Ce que la porte exige :
//   (1) DIGESTS DES BOULES ET FORETS, candidats par lane, evenements et niveaux
//       de lots identiques a L = 0, 1, 2, 3 et, a L = 3, entre un et quatre
//       fils ;
//   (2) GRAND-LIVRE exact par lane : `emis + tues == base`, sans quoi une
//       paire a ete perdue ou comptee deux fois ;
//   (3) ROUTE q2 INTERDITE : `tues[q2] == 0` et `emis[q2] == base[q2]` a tout
//       L — en q2 aucun pretest ponctuel ne referme la couture du temoin du
//       frere ;
//   (4) la contre-fixture radix q2 a six points reveille exactement un candidat
//       quand la route est ouverte par mutant, tandis que son ledger reste
//       vert : la fermeture q2 est donc gardee par l'objet, pas par la masse ;
//   (5) PLANCHERS de non-vacuite : le raffinement doit tuer une masse de
//       paires strictement positive en q3 ET en q4, sinon la porte est verte
//       par vacuite ;
//   (6) MONOTONIE DU CŒUR : une fixture q3 minimale exige l'autorite de coin
//       pour que `fresh_child >= parent.core`; son retrait doit etre refuse par
//       l'invariant alors que le ledger de paires reste exact.
// Codes : 0 ; 1 desaccord ; 2 refus ; 3 plancher ; 4 mutant tue.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {
int failures = 0;
u64 g_killed3 = 0, g_killed4 = 0;
void expect(bool ok, const char* what) {
  if (!ok) { std::printf("ECHEC : %s\n", what); ++failures; }
}

struct Out {
  std::string digest_raw_candidates, digest_balls, digest_all;
  std::vector<std::string> event_level_digests;
  u64 base[3] = {0, 0, 0}, emitted[3] = {0, 0, 0}, killed[3] = {0, 0, 0};
  u64 parents[3] = {0, 0, 0}, emitted_rects[3] = {0, 0, 0}, rect_alive[3] = {0, 0, 0};
  u64 subrects[3] = {0, 0, 0}, core_evals[3] = {0, 0, 0}, core_nodes[3] = {0, 0, 0};
  u64 corner_evals[3] = {0, 0, 0}, rollbacks[3] = {0, 0, 0};
  u64 candidates[3] = {0, 0, 0};
  PipelineStatus status = PipelineStatus::kInvalidInput;
  bool ok = false;
};

struct GenerationOut {
  std::string digest_raw_candidates, digest_balls;
  u64 candidates_q2 = 0, base_q2 = 0, emitted_q2 = 0, killed_q2 = 0;
  bool ok = false;
};

std::string digest_events_and_levels(u64 K, const std::vector<ForestEvent>& events, const ForestResult& forest) {
  digest_detail::Writer d;
  d.tag("mhgp5-test:postsep-events-levels");
  d.u64v(K);
  d.u64v(events.size());
  for (const ForestEvent& e : events) {
    d.u8v(e.q);
    d.u8v(e.d);
    d.u32v(e.active_mask);
    for (u8 i = 0; i < e.q; ++i) d.u32v(e.support[i]);
    for (u8 i = 0; i < e.d; ++i) d.u32v(e.interior[i]);
    d.level(e.level);
  }
  d.u64v(forest.batch_levels.size());
  for (const ExactLevel& level : forest.batch_levels) d.level(level);
  return d.hex();
}

Out run_one(const std::vector<InputPoint>& in, u32 levels, i64 s, u64 smax, int threads) {
  Out o;
  RunOptions opt;
  opt.s = s;
  opt.smax = smax;
  opt.threads = threads;
  opt.digest = true;
  opt.diagnostic_raw_candidates_digest = true;
  opt.postsep_refine_levels = levels;
  opt.on_forest = [&](u64 K, const std::vector<ForestEvent>& events, const ForestResult& forest) {
    if (o.event_level_digests.size() <= K) o.event_level_digests.resize((size_t)K + 1);
    o.event_level_digests[(size_t)K] = digest_events_and_levels(K, events, forest);
  };
  const RunResult rr = run_pipeline(in, opt);
  o.status = rr.status;
  if (rr.status != PipelineStatus::kCompleteRegular) return o;
  o.digest_raw_candidates = rr.digest_raw_candidates;
  o.digest_balls = rr.digest_balls;
  o.digest_all = rr.digest_all;
  for (int i = 0; i < 3; ++i) {
    o.base[i] = rr.gen.postsep_base_mass[i];
    o.emitted[i] = rr.gen.postsep_emitted_mass[i];
    o.killed[i] = rr.gen.postsep_killed_mass[i];
    o.parents[i] = rr.gen.postsep_parent_rects[i];
    o.emitted_rects[i] = rr.gen.postsep_emitted_rects[i];
    o.rect_alive[i] = rr.gen.rect_alive[i];
    o.subrects[i] = rr.gen.postsep_subrects[i];
    o.core_evals[i] = rr.gen.postsep_core_evals[i];
    o.core_nodes[i] = rr.gen.postsep_core_nodes[i];
    o.corner_evals[i] = rr.gen.postsep_corner_evals[i];
    o.rollbacks[i] = rr.gen.postsep_rollbacks[i];
    o.candidates[i] = rr.gen.candidates[i];
  }
  o.ok = true;
  return o;
}

// Chemin test-only sans la garde structurelle de run_pipeline : il conserve
// la preuve constructive du reveil q2 sous mutant, tandis que le vrai pipeline
// doit maintenant refuser toute activite de raffinement dans cette lane.
GenerationOut run_generation_only(const std::vector<InputPoint>& in, u32 levels, i64 s, u64 smax) {
  GenerationOut o;
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid || ix.has_duplicate_positions()) return o;
  GenerateOptions opt;
  opt.s = s;
  opt.smax = smax;
  opt.threads = 1;
  opt.postsep_refine_levels = levels;
  std::vector<BallCandidate> candidates;
  GenerateStats stats;
  generate_candidates(ix, opt, &candidates, &stats);
  sort_candidates(&candidates, 1);
  o.digest_raw_candidates = digest_raw_candidates_v5(candidates);
  deduplicate_candidates(&candidates);
  o.digest_balls = digest_balls_v4(candidates);
  o.candidates_q2 = stats.candidates[0];
  o.base_q2 = stats.postsep_base_mass[0];
  o.emitted_q2 = stats.postsep_emitted_mass[0];
  o.killed_q2 = stats.postsep_killed_mass[0];
  o.ok = true;
  return o;
}

bool same_output(const Out& a, const Out& b) {
  if (a.digest_raw_candidates != b.digest_raw_candidates || a.digest_balls != b.digest_balls ||
      a.digest_all != b.digest_all ||
      a.event_level_digests != b.event_level_digests)
    return false;
  for (int q = 0; q < 3; ++q)
    if (a.candidates[q] != b.candidates[q]) return false;
  return true;
}

bool same_postsep_counters(const Out& a, const Out& b) {
  for (int q = 0; q < 3; ++q) {
    if (a.base[q] != b.base[q] || a.emitted[q] != b.emitted[q] || a.killed[q] != b.killed[q] ||
        a.parents[q] != b.parents[q] || a.emitted_rects[q] != b.emitted_rects[q] ||
        a.rect_alive[q] != b.rect_alive[q] || a.subrects[q] != b.subrects[q] ||
        a.core_evals[q] != b.core_evals[q] || a.core_nodes[q] != b.core_nodes[q] ||
        a.corner_evals[q] != b.corner_evals[q] || a.rollbacks[q] != b.rollbacks[q])
      return false;
  }
  return true;
}

using PairKey = u64;

void append_pairs(const CloudIndex& ix, const WspdRect& r, std::vector<PairKey>* out) {
  const NodeRange a = ix.range_of(r.a), b = ix.range_of(r.b);
  for (i32 i = a.first; i <= a.last; ++i)
    for (i32 j = b.first; j <= b.last; ++j) {
      const u32 lo = (u32)std::min(i, j), hi = (u32)std::max(i, j);
      out->push_back(((u64)lo << 32) | hi);
    }
}

// Oracle quadratique strictement borne au test : pour chaque rectangle parent,
// la reunion des produits emis et certifies morts doit etre son multiensemble
// exact de couples. Le produit ne materialise jamais cette matrice.
bool literal_pair_partition(const std::vector<InputPoint>& in, i64 s, int lane_idx, bool* split_a, bool* split_b,
                            bool* killed_any) {
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid || ix.has_duplicate_positions()) return false;
  const u64 h_of[3] = {lane_h(Lane::kQ2, 11), lane_h(Lane::kQ3, 11), lane_h(Lane::kQ4, 11)};
  std::vector<AliveRect> parents;
  u64 visited = 0, workers = 0;
  generate_detail::PostsepLedger base_ledger;
  generate_detail::alive_rectangles(ix, s, h_of, lane_idx, 1, &parents, &visited, &workers, 0, &base_ledger);
  std::sort(parents.begin(), parents.end(), [](const AliveRect& x, const AliveRect& y) {
    return std::tie(x.r.a, x.r.b) < std::tie(y.r.a, y.r.b);
  });
  parents.erase(std::unique(parents.begin(), parents.end(), [](const AliveRect& x, const AliveRect& y) {
                  return x.r.a == y.r.a && x.r.b == y.r.b;
                }),
                parents.end());
  bool ok = true;
  for (const AliveRect& parent : parents) {
    std::vector<AliveRect> emitted;
    std::vector<WspdRect> killed;
    generate_detail::PostsepLedger led;
    generate_detail::postsep_refine(ix, parent.r, parent.core, h_of, lane_idx, (u8)(1u << lane_idx), s, 3,
                                    &emitted, &led, &killed);
    std::vector<PairKey> want, got;
    append_pairs(ix, parent.r, &want);
    for (const AliveRect& child : emitted) {
      append_pairs(ix, child.r, &got);
      *split_a = *split_a || child.r.a != parent.r.a;
      *split_b = *split_b || child.r.b != parent.r.b;
    }
    for (const WspdRect& child : killed) {
      append_pairs(ix, child, &got);
      *split_a = *split_a || child.a != parent.r.a;
      *split_b = *split_b || child.b != parent.r.b;
    }
    *killed_any = *killed_any || !killed.empty();
    std::sort(want.begin(), want.end());
    std::sort(got.begin(), got.end());
    ok = ok && want == got && (u128)led.emitted + led.killed == led.base;
  }
  return ok;
}

NodeRef find_box_ref(const CloudIndex& ix, i64 lo, i64 hi) {
  for (size_t v = 0; v < ix.nodes.size(); ++v) {
    const AxisBox b = ix.box_of((NodeRef)v);
    if (b.lo[0] == lo && b.hi[0] == hi && b.lo[1] == 0 && b.hi[1] == 0 && b.lo[2] == 0 && b.hi[2] == 0)
      return (NodeRef)v;
  }
  return std::numeric_limits<NodeRef>::max();
}
}  // namespace

int main(int argc, char** argv) {
  u64 min_killed = 1000;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--min-killed=", 0) == 0) min_killed = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool q2_mutant = MHGP5_MUTANT("postsep-refine-q2");
  const bool core_mutant = MHGP5_MUTANT("postsep-core-without-corners");
  const bool mutant = MHGP5_MUTANT("postsep-drop-child") || MHGP5_MUTANT("postsep-duplicate-child") ||
                      MHGP5_MUTANT("postsep-kill-h-minus-one") || q2_mutant || core_mutant;

  // Le digest brut doit voir la multiplicite que le RLE masque. Cette petite
  // identite garde l'instrument lui-meme, sans passer par un mutant dont le
  // ledger ferait echouer le pipeline avant le hachage.
  {
    const BallCandidate c{{1, {0, 0, 0}, -1}, {{1, 0, 0}, 1}, 2};
    const BallCandidate d{{1, {0, 0, 0}, -4}, {{4, 0, 0}, 1}, 2};
    std::vector<BallCandidate> with_duplicate = {c, c, d};
    std::vector<BallCandidate> without_duplicate = {c, d};
    sort_candidates(&with_duplicate, 1);
    sort_candidates(&without_duplicate, 1);
    expect(digest_raw_candidates_v5(with_duplicate) != digest_raw_candidates_v5(without_duplicate),
           "digest brut : multiplicite visible avant RLE");
    deduplicate_candidates(&with_duplicate);
    deduplicate_candidates(&without_duplicate);
    expect(digest_balls_v4(with_duplicate) == digest_balls_v4(without_duplicate),
           "digest des boules : multiplicite masquee apres RLE");
  }

  // ---- Oracle litteral de partition, avec scissions des deux facteurs.
  bool split_a = false, split_b = false, killed_any = false;
  const std::vector<InputPoint> literal = make_family_input(CloudFamily::kUniform, 160, 0, 19);
  const bool literal_ok = literal_pair_partition(literal, 4, 1, &split_a, &split_b, &killed_any) &&
                          literal_pair_partition(literal, 4, 2, &split_a, &split_b, &killed_any);
  if (mutant && !literal_ok) {
    std::printf("MUTANT TUE PAR MULTIENSEMBLE LITTERAL\n");
    return 4;
  }
  expect(literal_ok, "multiensemble litteral emis+mort == parent, exact-once");
  expect(split_a && split_b, "oracle litteral : scissions non vacantes de A et B");
  expect(killed_any, "oracle litteral : au moins une branche certifiee morte");

  // ---- `separated` non hereditaire : rollback atomique avant tout comptage.
  {
    const std::vector<InputPoint> in = {{0, {0, 0, 0}}, {1, {99, 0, 0}}, {2, {100, 0, 0}},
                                        {3, {512, 0, 0}}, {4, {612, 0, 0}}};
    const CloudIndex ix = build_cloud_index(in);
    const NodeRef a = find_box_ref(ix, 0, 100), b = find_box_ref(ix, 512, 612);
    expect(a != std::numeric_limits<NodeRef>::max() && b != std::numeric_limits<NodeRef>::max(),
           "rollback : nœuds [0,100] et [512,612] trouves");
    if (a != std::numeric_limits<NodeRef>::max() && b != std::numeric_limits<NodeRef>::max()) {
      const WspdRect parent{a, b};
      expect(wspd_detail::separated(ix.box_of(a), ix.box_of(b), 8, 1), "rollback : parent separe");
      const u64 high[3] = {100, 100, 100};
      std::vector<AliveRect> emitted;
      generate_detail::PostsepLedger led;
      generate_detail::postsep_refine(ix, parent, 0, high, 1, 0b010, 8, 1, &emitted, &led);
      expect(led.rollbacks == 1 && led.parents == 1 && led.emitted_rects == 1 && led.subrects == 1 &&
                 led.core_evals == 0 && led.core_nodes == 0 && led.corner_evals == 0 &&
                 led.killed == 0 && led.emitted == led.base && emitted.size() == 1 &&
                 emitted[0].r.a == a && emitted[0].r.b == b,
             "rollback : parent emis sans effet enfant");
    }
  }

  // ---- Monotonie du cœur frais : n=4 est minimal (trois positions pour
  // scinder les facteurs, une quatrieme comme temoin exterieur). Le parent
  // radix (-1,2) et ses deux enfants sont separes. Avec les coins, leurs
  // comptes 1 et 2 dominent le compte parent 1 ; sans les coins ils tombent a
  // zero. Le ledger reste exact, donc seule la garde de monotonie voit la faute.
  {
    const std::vector<InputPoint> in = {{0, {15, 61, 36}}, {1, {61, 19, 31}},
                                        {2, {3, 60, 45}}, {3, {51, 27, 34}}};
    const CloudIndex ix = build_cloud_index(in);
    const u64 h_of[3] = {lane_h(Lane::kQ2, 5), lane_h(Lane::kQ3, 5), lane_h(Lane::kQ4, 5)};
    const WspdRect parent{leaf_ref(0), 2};
    const WspdRect child0{leaf_ref(0), leaf_ref(2)};
    const WspdRect child1{leaf_ref(0), leaf_ref(3)};
    const bool radix_ok = ix.valid && ix.nodes.size() == 3 && ix.point_id(0) == 1 && ix.point_id(1) == 3 &&
                          ix.point_id(2) == 0 && ix.point_id(3) == 2 && ix.nodes[2].first == 2 &&
                          ix.nodes[2].last == 3 && ix.nodes[2].left == leaf_ref(2) &&
                          ix.nodes[2].right == leaf_ref(3);
    expect(radix_ok, "core monotone : ordre Morton et parent radix graves");
    if (!radix_ok) return 3;
    const bool separated = wspd_detail::separated(ix.box_of(parent.a), ix.box_of(parent.b), 1, 1) &&
                           wspd_detail::separated(ix.box_of(child0.a), ix.box_of(child0.b), 1, 1) &&
                           wspd_detail::separated(ix.box_of(child1.a), ix.box_of(child1.b), 1, 1);
    expect(separated, "core monotone : parent et deux enfants separes");
    const FusedCounts p_true = count_universal_witnesses(ix, parent.a, parent.b, h_of, 0b010, true);
    const FusedCounts p_false = count_universal_witnesses(ix, parent.a, parent.b, h_of, 0b010, false);
    const FusedCounts c0_true = count_universal_witnesses(ix, child0.a, child0.b, h_of, 0b010, true);
    const FusedCounts c0_false = count_universal_witnesses(ix, child0.a, child0.b, h_of, 0b010, false);
    const FusedCounts c1_true = count_universal_witnesses(ix, child1.a, child1.b, h_of, 0b010, true);
    const FusedCounts c1_false = count_universal_witnesses(ix, child1.a, child1.b, h_of, 0b010, false);
    const bool counts_ok = p_true.c[1] == 1 && p_false.c[1] == 0 && c0_true.c[1] == 1 &&
                           c0_false.c[1] == 0 && c1_true.c[1] == 2 && c1_false.c[1] == 0;
    expect(counts_ok, "core monotone : comptes true/false graves a 1/0, 1/0 et 2/0");

    std::vector<AliveRect> emitted;
    generate_detail::PostsepLedger led;
    generate_detail::postsep_refine(ix, parent, p_true.c[1], h_of, 1, 0b010, 1, 1, &emitted, &led);
    const bool ledger_ok = led.base == 2 && led.emitted == 2 && led.killed == 0;
    expect(ledger_ok, "core monotone : ledger local exact 2=2+0");
    const Out l0 = run_one(in, 0, 1, 5, 1), l1 = run_one(in, 1, 1, 5, 1);
    if (core_mutant) {
      if (separated && counts_ok && ledger_ok && led.core_regressions == 2 && l0.ok &&
          l1.status == PipelineStatus::kInvariantViolated) {
        std::printf("MUTANT TUE PAR REGRESSION DU CŒUR AVEC LEDGER VERT\n");
        return 4;
      }
      std::printf("MUTANT CORE SANS COINS NON TUE\n");
      return 1;
    }
    if (!mutant) {
      expect(led.core_regressions == 0, "core monotone : aucune regression nominale");
      expect(l0.ok && l1.ok && same_output(l0, l1), "core monotone : L=0 et L=1 identiques");
    }
  }

  // ---- (4) CONTRE-FIXTURE q2 REALISABLE sur de vrais nœuds radix.
  // Ordre Morton des PointId : 0,2,1,3,5,4. Avec la route q2 nominalement
  // fermee, L=3 est identique a L=0. Le mutant ouvre la subdivision : l'ancre
  // (2,5) se reveille, sans masse q2 tuee, donc seul le contrat de candidats
  // detecte la faute.
  {
    const std::vector<InputPoint> in = {{0, {59, 3, 7}},   {1, {62, 50, 9}}, {2, {24, 55, 14}},
                                        {3, {56, 44, 46}}, {4, {426, 17, 62}}, {5, {424, 36, 5}}};
    if (q2_mutant) {
      const GenerationOut a = run_generation_only(in, 0, 1, 3);
      const GenerationOut b = run_generation_only(in, 3, 1, 3);
      const Out guarded = run_one(in, 3, 1, 3, 1);
      const bool wakeup = a.ok && b.ok && a.candidates_q2 == 13 && b.candidates_q2 == 14 &&
                          a.digest_raw_candidates != b.digest_raw_candidates &&
                          a.digest_balls != b.digest_balls && b.killed_q2 == 0 &&
                          b.emitted_q2 == b.base_q2;
      if (wakeup && guarded.status == PipelineStatus::kInvariantViolated) {
        std::printf("MUTANT TUE PAR REVEIL Q2 AVEC LEDGER VERT ET GARDE STRUCTURELLE\n");
        return 4;
      }
      std::printf("MUTANT Q2 NON TUE\n");
      return 1;
    }
    const Out a = run_one(in, 0, 1, 3, 1), b = run_one(in, 3, 1, 3, 1);
    if (!a.ok || !b.ok) {
      if (mutant && (a.status == PipelineStatus::kInvariantViolated || b.status == PipelineStatus::kInvariantViolated))
        return 4;
      std::printf("REFUS : contre-fixture refine-hist-wakeup-q2\n");
      return 2;
    }
    std::printf("refine-hist-wakeup-q2 L=0 raw=%.16s balls=%.16s all=%.16s candidats=%llu | L=3 raw=%.16s balls=%.16s all=%.16s candidats=%llu masse=%llu+%llu/%llu\n",
                a.digest_raw_candidates.c_str(), a.digest_balls.c_str(), a.digest_all.c_str(),
                (unsigned long long)a.candidates[0], b.digest_raw_candidates.c_str(), b.digest_balls.c_str(),
                b.digest_all.c_str(), (unsigned long long)b.candidates[0],
                (unsigned long long)b.emitted[0], (unsigned long long)b.killed[0], (unsigned long long)b.base[0]);
    // La mise a mort porte sur la premiere divergence semantique observable :
    // candidat + boule RLE avec ledger vert. `digest_all` est encore identique
    // sur cette fixture parce que la boule reveillee est profonde, mais cette
    // caracterisation aval ne doit pas rendre le mutant plus difficile a tuer :
    // une future divergence de foret serait une faute plus forte, pas un vert.
    const bool wakeup = a.candidates[0] == 13 && b.candidates[0] == 14 &&
                        a.digest_raw_candidates != b.digest_raw_candidates && a.digest_balls != b.digest_balls &&
                        b.killed[0] == 0 && b.emitted[0] == b.base[0];
    expect(!wakeup, "refine-hist-wakeup-q2 : aucun reveil sur la route nominalement fermee");
    expect(same_output(a, b), "refine-hist-wakeup-q2 : route q2 nominale fermee");
    expect(a.candidates[0] == 13 && b.candidates[0] == 13,
           "refine-hist-wakeup-q2 : 13 candidats de part et d'autre quand la route est fermee");
    expect(b.killed[0] == 0 && b.emitted[0] == b.base[0],
           "refine-hist-wakeup-q2 : ledger q2 nominal exact");
  }

  // ---- (1) (2) (3) (5) sur les familles de mesure.
  const struct { CloudFamily f; int n; } clouds[] = {{CloudFamily::kScanlineSinglePass, 1500}, {CloudFamily::kUniform, 1200},
                                                     {CloudFamily::kTerrain, 1000}, {CloudFamily::kEightClusters, 1200},
                                                     {CloudFamily::kTwoLines, 400}, {CloudFamily::kCollinearSeven, 7}};
  for (const auto& c : clouds) {
    const std::vector<InputPoint> in = make_family_input(c.f, c.n, cloud_family_default_coord(c.f, c.n), 3);
    const char* fam = cloud_family_name(c.f);
    const Out ref = run_one(in, 0, 8, 11, 4);
    if (!ref.ok) { std::printf("REFUS %s\n", fam); return 2; }
    for (int q = 0; q < 3; ++q) {
      char what[160];
      std::snprintf(what, sizeof(what), "%s L=0 q%d : un produit emis par parent et rect_alive historique", fam, q + 2);
      expect(ref.parents[q] == ref.emitted_rects[q] && ref.rect_alive[q] == ref.emitted_rects[q] &&
                 ref.subrects[q] == 0 && ref.core_evals[q] == 0,
             what);
    }
    for (u32 L = 1; L <= 3; ++L) {
      const Out o = run_one(in, L, 8, 11, 4);
      if (!o.ok) {
        if (mutant && o.status == PipelineStatus::kInvariantViolated) return 4;
        std::printf("REFUS %s L=%u\n", fam, L);
        return 2;
      }
      char what[160];
      std::snprintf(what, sizeof(what), "%s L=%u : boules/evenements/niveaux/forets identiques a L=0", fam, L);
      expect(same_output(o, ref), what);
      for (int q = 0; q < 3; ++q) {
        std::snprintf(what, sizeof(what), "%s L=%u q%d : grand-livre emis+tues==base (%llu+%llu vs %llu)", fam, L, q + 2,
                      (unsigned long long)o.emitted[q], (unsigned long long)o.killed[q], (unsigned long long)o.base[q]);
        expect(o.emitted[q] + o.killed[q] == o.base[q], what);
        std::snprintf(what, sizeof(what), "%s L=%u q%d : base inchangee par L", fam, L, q + 2);
        expect(o.base[q] == ref.base[q], what);
        std::snprintf(what, sizeof(what), "%s L=%u q%d : parents invariants et rect_alive=produits emis", fam, L, q + 2);
        expect(o.parents[q] == ref.parents[q] && o.rect_alive[q] == o.emitted_rects[q], what);
      }
      std::snprintf(what, sizeof(what), "%s L=%u : route q2 INTERDITE (tues=%llu)", fam, L, (unsigned long long)o.killed[0]);
      expect(o.killed[0] == 0 && o.emitted[0] == o.base[0] && o.parents[0] == o.emitted_rects[0] &&
                 o.subrects[0] == 0 && o.core_evals[0] == 0 && o.core_nodes[0] == 0 &&
                 o.corner_evals[0] == 0 && o.rollbacks[0] == 0,
             what);
      if (L == 3) { g_killed3 += o.killed[1]; g_killed4 += o.killed[2]; }
    }
    const Out one_thread = run_one(in, 3, 8, 11, 1);
    if (!one_thread.ok) {
      if (mutant && one_thread.status == PipelineStatus::kInvariantViolated) return 4;
      std::printf("REFUS %s L=3 threads=1\n", fam);
      return 2;
    }
    char what[160];
    const Out four_threads = run_one(in, 3, 8, 11, 4);
    std::snprintf(what, sizeof(what), "%s L=3 : sortie et compteurs identiques entre 1 et 4 fils", fam);
    expect(four_threads.ok && same_output(one_thread, four_threads) &&
               same_postsep_counters(one_thread, four_threads),
           what);
  }

  std::printf("postsep_refine_gate echecs=%d paires_tuees_L3 q3=%llu q4=%llu\n", failures, (unsigned long long)g_killed3,
              (unsigned long long)g_killed4);
  if (mutant) {
    if (failures) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (g_killed3 < min_killed || g_killed4 < min_killed) { std::printf("PLANCHER\n"); return 3; }
  if (failures) return 1;
  std::printf("postsep_refine_gate OK\n");
  return 0;
}
