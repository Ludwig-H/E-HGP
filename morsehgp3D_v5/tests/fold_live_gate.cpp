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
//   (4) REJEU T5 : les deltas du VIVANT sont rejoues par un union-find frais
//       sur le catalogue du resident, et la partition reconstruite est
//       comparee a `final_canon_fid` — les deltas ne sont pas seulement
//       egaux, ils reconstruisent la meme partition ;
//   (5) PLAFONDS DE RELOCALISATION, deux temoins : agrege
//       `relocalisations <= facettes * (ceil(log2 facettes) + 1)`, et PAR
//       ALIAS `deplacements <= ceil(log2 facettes) + 1`. Deux cas
//       SYNTHETIQUES adverses sont joues EN PREMIER (chaine d'absorptions ou
//       le singleton est toujours `first`, arbre de fusions equilibre) : le
//       mutant qui garde le record de `first` comme conteneur physique y
//       meurt en quelques millisecondes, avant tout nuage.
// Planchers : >= --min-facets facettes, >= --min-deltas deltas, >= --min-big-orders
// ordres d'au moins --min-order-facets facettes.
// Codes : 0 ; 1 desaccord ; 2 refus ; 3 plancher ou plafond ; 4 mutant tue.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
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
u64 g_wr_alias = 0, g_wr_comp = 0, g_wr_facets = 0;  // le temoin de CE pire ratio (jamais melange avec le pic absolu)
u64 g_big_min = 100000;
u64 g_moves_max = 0, g_life = 0, g_struct = 0, g_final = 0, g_scans = 0, g_alloc_bytes = 0;
bool g_kill = false;  // un mutant est deja mort : la porte sort sans jouer la suite

u64 log2_ceil(u64 x) {
  u64 l = 0;
  while ((1ull << l) < x) ++l;
  return l;
}

bool delta_equal(const ComponentDelta& a, const ComponentDelta& b) {
  return a.batch == b.batch && a.level == b.level && a.output == b.output && a.parents == b.parents && a.born == b.born;
}

// REJEU INDEPENDANT (T5) : union-find FRAIS sur le catalogue, alimente par les
// seuls deltas du VIVANT ; la partition reconstruite doit egaler
// `final_canon_fid` du resident. Retourne le nombre de fid en desaccord.
u64 replay_partition(const std::vector<FacetKey>& keys, const std::vector<ComponentDelta>& deltas, const std::vector<u32>& canon_ref) {
  const size_t nf = keys.size();
  if (canon_ref.size() != nf) return nf + 1;
  std::vector<u32> parent(nf), canon(nf);
  for (size_t i = 0; i < nf; ++i) {
    parent[i] = (u32)i;
    canon[i] = (u32)i;
  }
  const std::function<u32(u32)> find = [&](u32 v) {
    while (parent[v] != v) {
      parent[v] = parent[parent[v]];
      v = parent[v];
    }
    return v;
  };
  const auto fid_of = [&](const FacetKey& k) -> u32 {
    const auto it = std::lower_bound(keys.begin(), keys.end(), k, [](const FacetKey& x, const FacetKey& y) { return x < y; });
    return (it == keys.end() || !(*it == k)) ? UINT32_MAX : (u32)(it - keys.begin());
  };
  for (const ComponentDelta& d : deltas) {
    u32 first = UINT32_MAX;
    const auto join = [&](const FacetKey& k) {
      const u32 f = fid_of(k);
      if (f == UINT32_MAX) return;
      if (first == UINT32_MAX) {
        first = f;
        return;
      }
      const u32 a = find(first), b = find(f);
      if (a == b) return;
      parent[b] = a;
      canon[a] = std::min(canon[a], canon[b]);
    };
    for (const FacetKey& k : d.born) join(k);
    for (const FacetKey& k : d.parents) join(k);
  }
  u64 diff = 0;
  for (size_t f = 0; f < nf; ++f)
    if (canon[find((u32)f)] != canon_ref[f]) ++diff;
  return diff;
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
  const u64 per_alias = log2_ceil((u64)res.facets + 2) + 1;
  const u64 ceiling = res.facets ? (u64)res.facets * per_alias : 0;
  if (ceiling && st.relocations > ceiling) g_reloc_max = std::max(g_reloc_max, st.relocations - ceiling);
  if (st.max_moves_per_alias > per_alias) g_reloc_max = std::max(g_reloc_max, st.max_moves_per_alias - per_alias);
  g_moves_max = std::max(g_moves_max, st.max_moves_per_alias);
  g_life += st.life_violations;
  g_struct += st.structure_violations;
  g_final += st.final_nonempty;
  g_scans += st.structure_scans;
  g_alloc_bytes = std::max(g_alloc_bytes, st.allocated_bytes);
  if (st.peak_aliases > g_peak_alias) {
    g_peak_alias = st.peak_aliases;
    g_peak_comp = st.peak_components;
    g_peak_live = st.peak_live_exact;
    g_facets_at_peak = res.facets;
    g_live_bytes = st.logical_live_bytes;
    g_res_bytes = (u64)res.facets * 36;  // FidState (32 o) + final_canon_fid (4 o) du resident
  }
  // La FRACTION de residence ne se lit que sur les ordres assez gros : a
  // quelques centaines de facettes toutes les facettes sont vivantes en meme
  // temps et 100 % ne dit rien (docs/TEST_PLAN § 3.1 : les petites tailles
  // sont un oracle de correction, jamais une pente).
  if (res.facets >= g_big_min) {
    ++g_big_orders;
    const double ratio = (double)st.peak_aliases / (double)res.facets;
    if (ratio > g_worst_ratio) {  // le temoin du PIRE RATIO est publie avec SES propres champs
      g_worst_ratio = ratio;
      g_wr_alias = st.peak_aliases;
      g_wr_comp = st.peak_components;
      g_wr_facets = res.facets;
    }
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
  if (st.life_violations || st.structure_violations || st.final_nonempty) ++bad;
  // (4) rejeu T5 des deltas du VIVANT contre la partition du resident.
  if (!bad) {
    const u64 diff = replay_partition(res.facet_keys, liv.deltas, res.final_canon_fid);
    if (diff) {
      ++bad;
      std::printf("%s K=%llu : REJEU des deltas vivants : %llu fid de partition differents\n", fam, (unsigned long long)K, (unsigned long long)diff);
    }
  }
  if (bad) {
    ++g_mism;
    std::printf("%s K=%llu : DESACCORD (facettes %llu/%llu, deltas %zu/%zu, fusions %llu/%llu, lots %llu/%llu, alias_pic=%llu comp_pic=%llu vie_pic=%llu)\n",
                fam, (unsigned long long)K, (unsigned long long)st.facets, (unsigned long long)res.facets, liv.deltas.size(), res.deltas.size(),
                (unsigned long long)liv.fusions, (unsigned long long)res.fusions, (unsigned long long)liv.batches, (unsigned long long)res.batches,
                (unsigned long long)st.peak_aliases, (unsigned long long)st.peak_components, (unsigned long long)st.peak_live_exact);
  }
}

// CAS SYNTHETIQUES ADVERSES (joues AVANT les nuages : un mutant de cout y
// meurt en millisecondes). `ev` : K = q + d - 1, l'ordre des supports fixe la
// facette `first`, donc la racine LOGIQUE.
ForestEvent ev1(PointId a, PointId b, u16 act, u64 level) {
  ForestEvent e;
  e.q = 2;
  e.d = 0;
  e.active_mask = act;
  e.support[0] = a;
  e.support[1] = b;
  e.level = ExactLevel{{level, 0, 0}, 1};
  return e;
}

// (i) CHAINE D'ABSORPTIONS : a chaque niveau un SINGLETON frais est `first` et
// absorbe logiquement la composante qui grossit. Small-to-large ne deplace que
// le singleton : 1 deplacement par alias. Le mutant qui garde le record de
// `first` deplace toute la composante a chaque pas : O(n^2) et n deplacements
// pour le pire alias.
// (ii) ARBRE DE FUSIONS EQUILIBRE de 64 feuilles : chaque alias doit bouger au
// plus log2(64) = 6 fois.
void synthetic_cases(u64 n_chain, u64* worst_moves, u64* over_ceiling, u64* mismatches) {
  const struct {
    const char* name;
    std::vector<ForestEvent> e;
  } cases[] = {
      {"chaine_absorptions",
       [&] {
         std::vector<ForestEvent> e;
         e.push_back(ev1(0, 1000, 0, 1));  // {1000} et {0} naissent
         for (u32 i = 2; i <= (u32)n_chain; ++i) e.push_back(ev1(0, 1000 + i, 2u, i));  // first = {1000+i}, {0} ACTIVE
         return e;
       }()},
      {"arbre_equilibre",
       [&] {
         std::vector<ForestEvent> e;
         for (u32 step = 1, lvl = 1; step < 64; step *= 2, ++lvl)
           for (u32 i = 0; i + step < 64; i += 2 * step) e.push_back(ev1(i, i + step, step == 1 ? 0 : 3u, lvl));
         return e;
       }()},
  };
  for (const auto& c : cases) {
    LiveFoldStats st;
    const ForestResult res = build_forest(c.e, 1);
    const ForestResult liv = reduce_fold_live(prepare_fold(c.e, 1), &st);
    const u64 per_alias = log2_ceil((u64)res.facets + 2) + 1;
    u64 bad = 0;
    if (liv.deltas.size() != res.deltas.size()) ++bad;
    else
      for (size_t i = 0; i < liv.deltas.size(); ++i)
        if (!delta_equal(liv.deltas[i], res.deltas[i])) ++bad;
    if (st.life_violations || st.structure_violations || st.final_nonempty) ++bad;
    if (!bad && replay_partition(res.facet_keys, liv.deltas, res.final_canon_fid)) ++bad;
    if (st.max_moves_per_alias > per_alias) *over_ceiling += st.max_moves_per_alias - per_alias;
    if (st.relocations > (u64)res.facets * per_alias) *over_ceiling += st.relocations - (u64)res.facets * per_alias;
    *worst_moves = std::max(*worst_moves, st.max_moves_per_alias);
    *mismatches += bad;
    std::printf("fold_live_synthetique %s facettes=%llu deplacements_max=%llu (plafond %llu) relocalisations=%llu desaccords=%llu\n", c.name,
                (unsigned long long)res.facets, (unsigned long long)st.max_moves_per_alias, (unsigned long long)per_alias,
                (unsigned long long)st.relocations, (unsigned long long)bad);
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
  // ---- Cas SYNTHETIQUES adverses d'abord : un mutant y meurt sans payer les nuages.
  u64 syn_moves = 0, syn_over = 0, syn_bad = 0;
  synthetic_cases(200, &syn_moves, &syn_over, &syn_bad);
  g_reloc_max = std::max(g_reloc_max, syn_over);
  g_mism += syn_bad;
  if (m_cost && g_reloc_max) {
    std::printf("fold_live_gate MUTANT DE COUT TUE sur les cas synthetiques (deplacements_max=%llu)\n", (unsigned long long)syn_moves);
    return 4;
  }
  if (m_out && g_mism) {
    std::printf("fold_live_gate MUTANT TUE sur les cas synthetiques\n");
    return 4;
  }
  const struct {
    CloudFamily f;
    int n;
  } clouds[] = {{CloudFamily::kUniform, 1200},          {CloudFamily::kEightClusters, 1200}, {CloudFamily::kScanlineSinglePass, 1500},
                {CloudFamily::kTerrain, 1000},          {CloudFamily::kTwoLines, 400},       {CloudFamily::kCollinearSeven, 7}};
  for (const auto& c : clouds) {
    if (g_kill) break;  // COURT-CIRCUIT : la mise a mort est acquise, la porte ne joue pas la suite
    const std::vector<InputPoint> in = make_family_input(c.f, c.n, cloud_family_default_coord(c.f, c.n), 3);
    RunOptions o;
    o.threads = 4;
    const char* fam = cloud_family_name(c.f);
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>& e, const ForestResult& r) {
      if (g_kill) return;
      compare_order(K, e, r, fam);
      if ((m_out && g_mism) || (m_cost && g_reloc_max)) g_kill = true;  // court-circuit des mutants
    };
    const RunResult rr = run_pipeline(in, o);
    if (rr.status != PipelineStatus::kCompleteRegular) {
      std::printf("REFUS %s : %s\n", fam, rr.message.c_str());
      return 2;
    }
  }
  std::printf("fold_live_gate ordres=%llu facettes=%llu deltas=%llu desaccords=%llu invariants=%llu relocalisations=%llu\n", (unsigned long long)g_orders,
              (unsigned long long)g_facets, (unsigned long long)g_deltas, (unsigned long long)g_mism, (unsigned long long)g_inv,
              (unsigned long long)g_reloc);
  // DEUX TEMOINS, jamais melanges (audit du 28 aout) : le plus grand PIC
  // ABSOLU d'alias, et le PIRE RATIO alias/facettes ; chacun avec ses propres
  // champs. Les octets logiques sont l'etat vivant du reducteur ; les octets
  // alloues sont ce qu'il tient reellement (arenes, listes libres, table) ;
  // les octets residents sont les deux champs par facette du fold resident
  // (`FidState` 32 o + `final_canon_fid` 4 o) — ce rapport est une ESTIMATION
  // de structures choisies, jamais un gain de RSS mesure.
  std::printf("fold_live_pic_absolu pic_alias=%llu pic_composantes=%llu pic_vie_exact=%llu facettes_de_l_ordre=%llu fraction=%.2f %% octets_logiques=%llu octets_alloues=%llu octets_resident_estimes=%llu\n",
              (unsigned long long)g_peak_alias, (unsigned long long)g_peak_comp, (unsigned long long)g_peak_live, (unsigned long long)g_facets_at_peak,
              g_facets_at_peak ? 100.0 * (double)g_peak_alias / (double)g_facets_at_peak : 0.0, (unsigned long long)g_live_bytes,
              (unsigned long long)g_alloc_bytes, (unsigned long long)g_res_bytes);
  std::printf("fold_live_pire_ratio fraction=%.2f %% pic_alias=%llu pic_composantes=%llu facettes_de_l_ordre=%llu ordres_mesures=%llu (facettes >= %llu)\n",
              100.0 * g_worst_ratio, (unsigned long long)g_wr_alias, (unsigned long long)g_wr_comp, (unsigned long long)g_wr_facets,
              (unsigned long long)g_big_orders, (unsigned long long)g_big_min);
  std::printf("fold_live_structure deplacements_max_par_alias=%llu vie_par_lot=%llu structure=%llu vacuite_finale=%llu balayages=%llu\n",
              (unsigned long long)g_moves_max, (unsigned long long)g_life, (unsigned long long)g_struct, (unsigned long long)g_final,
              (unsigned long long)g_scans);
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
  if (g_mism || g_inv || g_life || g_struct || g_final) return 1;
  std::printf("fold_live_gate OK\n");
  return 0;
}
