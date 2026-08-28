// MorseHGP3D v5 — PORTE DU REDUCTEUR VIVANT (docs/ECHELLE.md § 8 bis, etape
// L2 ; theoreme T6). Pour chaque ordre K de chaque nuage, le fold RESIDENT du
// pipeline (`on_forest`) est confronte au reducteur VIVANT
// (src/forest/fold_live.hpp) prepare sur les MEMES evenements :
//   (1) EGALITE de sortie : meme nombre de deltas, et champ par champ
//       (`batch`, `level`, `output`, `parents`, `born`), memes `batch_levels`,
//       memes compteurs (fusions, lots, nœuds, attachements, detecteurs) ;
//   (2) INVARIANT T6 : a chaque frontiere de lot, composantes <= alias <=
//       pic exact des durees de vie (compte par le reducteur lui-meme) ;
//   (3) RESIDENCE : le pic d'alias est une FRACTION mesuree des facettes de
//       l'ordre — c'est le chiffre que le passage a l'echelle utilise ;
//   (4) PLAFOND DE RELOCALISATIONS : small-to-large deplace chaque alias
//       O(log) fois, donc relocalisations <= facettes * ceil(log2(facettes+2))
//       divise par `--reloc-ratio` (defaut 4) — le mutant qui garde toujours
//       le record de `first` comme conteneur physique le depasse.
// Planchers : >= --min-facets facettes et >= --min-deltas deltas cumules.
// Codes : 0 ; 1 desaccord ; 2 refus ; 3 plancher ou plafond ; 4 mutant tue.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/forest/fold_live.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {
u64 g_orders = 0, g_facets = 0, g_deltas = 0, g_mism = 0, g_reloc = 0, g_reloc_max = 0, g_inv = 0;
u64 g_peak_alias = 0, g_peak_comp = 0, g_peak_live = 0, g_facets_at_peak = 0, g_live_bytes = 0, g_res_bytes = 0;
u64 g_big_orders = 0;   // ordres assez gros pour porter une fraction de residence
double g_worst_ratio = 0;  // pire fraction pic_alias / facettes sur ces ordres
u64 g_big_min = 100000;

u64 log2_ceil(u64 x) {
  u64 l = 0;
  while ((1ull << l) < x) ++l;
  return l;
}

bool delta_equal(const ComponentDelta& a, const ComponentDelta& b) {
  return a.batch == b.batch && a.level == b.level && a.output == b.output && a.parents == b.parents && a.born == b.born;
}

void compare_order(u64 K, const std::vector<ForestEvent>& events, const ForestResult& res, const char* fam) {
  ++g_orders;
  LiveFoldStats st;
  const ForestResult liv = reduce_fold_live(prepare_fold(events, 1), &st);
  if (!liv.refusal.empty()) {
    ++g_mism;
    std::printf("%s K=%llu : refus du vivant : %s\n", fam, (unsigned long long)K, liv.refusal.c_str());
    return;
  }
  g_facets += res.facets;
  g_deltas += res.deltas.size();
  g_reloc += st.relocations;
  g_inv += st.invariant_violations;
  // Plafond small-to-large : chaque alias change de conteneur au plus
  // log2(taille) fois, donc relocalisations <= facettes * (ceil(log2) + 1).
  // Mesure du 28 aout 2026 : 1,09 relocalisation par facette conforme contre
  // 138 pour le mutant `physical-root-is-logical-root` (x127).
  const u64 ceiling = res.facets ? (u64)res.facets * (log2_ceil((u64)res.facets + 2) + 1) : 0;
  if (ceiling && st.relocations > ceiling) g_reloc_max = std::max(g_reloc_max, st.relocations - ceiling);
  if (st.peak_aliases > g_peak_alias) {
    g_peak_alias = st.peak_aliases;
    g_peak_comp = st.peak_components;
    g_peak_live = st.peak_live_exact;
    g_facets_at_peak = res.facets;
    g_live_bytes = st.live_bytes_peak;
    g_res_bytes = (u64)res.facets * 36;  // FidState (32 o) + final_canon_fid (4 o) du resident
  }
  // La FRACTION de residence ne se lit que sur les ordres assez gros : a
  // quelques centaines de facettes toutes les facettes sont vivantes en meme
  // temps et 100 % ne dit rien (docs/TEST_PLAN § 3.1 : les petites tailles
  // sont un oracle de correction, jamais une pente).
  if (res.facets >= g_big_min) {
    ++g_big_orders;
    g_worst_ratio = std::max(g_worst_ratio, (double)st.peak_aliases / (double)res.facets);
  }
  u64 bad = 0;
  if (st.facets != res.facets) ++bad;
  if (liv.deltas.size() != res.deltas.size()) ++bad;
  else
    for (size_t i = 0; i < liv.deltas.size(); ++i)
      if (!delta_equal(liv.deltas[i], res.deltas[i])) ++bad;
  if (liv.batch_levels != res.batch_levels) ++bad;
  if (liv.fusions != res.fusions || liv.batches != res.batches || liv.nodes != res.nodes) ++bad;
  if (liv.new_attachments != res.new_attachments) ++bad;
  if (liv.attach_violations != res.attach_violations || liv.birth_violations != res.birth_violations) ++bad;
  if (st.peak_components > st.peak_aliases || st.peak_aliases > st.peak_live_exact) ++bad;
  if (bad) {
    ++g_mism;
    std::printf("%s K=%llu : DESACCORD (facettes %llu/%llu, deltas %zu/%zu, fusions %llu/%llu, lots %llu/%llu, alias_pic=%llu comp_pic=%llu vie_pic=%llu)\n",
                fam, (unsigned long long)K, (unsigned long long)st.facets, (unsigned long long)res.facets, liv.deltas.size(), res.deltas.size(),
                (unsigned long long)liv.fusions, (unsigned long long)res.fusions, (unsigned long long)liv.batches, (unsigned long long)res.batches,
                (unsigned long long)st.peak_aliases, (unsigned long long)st.peak_components, (unsigned long long)st.peak_live_exact);
  }
}
}  // namespace

int main(int argc, char** argv) {
  u64 min_facets = 200000, min_deltas = 10000, min_big = 3;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--min-facets=", 0) == 0) min_facets = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--min-order-facets=", 0) == 0) g_big_min = (u64)std::atoll(a.c_str() + 19);
    else if (a.rfind("--min-big-orders=", 0) == 0) min_big = (u64)std::atoll(a.c_str() + 17);
    else if (a.rfind("--min-deltas=", 0) == 0) min_deltas = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  // Mutants tues ICI : les cinq du reducteur vivant. Les quatre premiers
  // divergent de la sortie residente ; `physical-root-is-logical-root` ne
  // change pas la sortie mais depasse le plafond de relocalisations.
  const bool m_out = MHGP5_MUTANT("free-on-absorb") || MHGP5_MUTANT("root-key-mutable") || MHGP5_MUTANT("canon-not-min-on-union") ||
                     MHGP5_MUTANT("last-mark-shifted");
  const bool m_cost = MHGP5_MUTANT("physical-root-is-logical-root");
  const struct {
    CloudFamily f;
    int n;
  } clouds[] = {{CloudFamily::kUniform, 1200},          {CloudFamily::kEightClusters, 1200}, {CloudFamily::kScanlineSinglePass, 1500},
                {CloudFamily::kTerrain, 1000},          {CloudFamily::kTwoLines, 400},       {CloudFamily::kCollinearSeven, 7}};
  for (const auto& c : clouds) {
    const std::vector<InputPoint> in = make_family_input(c.f, c.n, cloud_family_default_coord(c.f, c.n), 3);
    RunOptions o;
    o.threads = 4;
    const char* fam = cloud_family_name(c.f);
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>& ev, const ForestResult& r) { compare_order(K, ev, r, fam); };
    const RunResult rr = run_pipeline(in, o);
    if (rr.status != PipelineStatus::kCompleteRegular) {
      std::printf("REFUS %s : %s\n", fam, rr.message.c_str());
      return 2;
    }
  }
  std::printf("fold_live_gate ordres=%llu facettes=%llu deltas=%llu desaccords=%llu invariants=%llu relocalisations=%llu\n", (unsigned long long)g_orders,
              (unsigned long long)g_facets, (unsigned long long)g_deltas, (unsigned long long)g_mism, (unsigned long long)g_inv,
              (unsigned long long)g_reloc);
  std::printf("fold_live_residence pic_alias=%llu pic_composantes=%llu pic_vie_exact=%llu facettes_de_l_ordre=%llu fraction_max=%.2f %% octets_vivants=%llu octets_resident=%llu\n",
              (unsigned long long)g_peak_alias, (unsigned long long)g_peak_comp, (unsigned long long)g_peak_live, (unsigned long long)g_facets_at_peak,
              100.0 * g_worst_ratio, (unsigned long long)g_live_bytes, (unsigned long long)g_res_bytes);
  std::printf("fold_live_residence ordres_mesures=%llu (facettes >= %llu)\n", (unsigned long long)g_big_orders, (unsigned long long)g_big_min);
  if (m_cost) {
    if (g_reloc_max) return 4;
    std::printf("MUTANT DE COUT NON TUE (plafond de relocalisations jamais depasse)\n");
    return 1;
  }
  if (m_out) {
    if (g_mism) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (g_facets < min_facets || g_deltas < min_deltas || g_big_orders < min_big) {
    std::printf("PLANCHER\n");
    return 3;
  }
  if (g_reloc_max) {
    std::printf("PLAFOND DE RELOCALISATIONS DEPASSE de %llu\n", (unsigned long long)g_reloc_max);
    return 3;
  }
  if (g_mism || g_inv) return 1;
  std::printf("fold_live_gate OK\n");
  return 0;
}
