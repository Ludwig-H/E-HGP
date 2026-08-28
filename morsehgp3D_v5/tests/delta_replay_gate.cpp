// MorseHGP3D v5 — PORTE DE REJEU « catalogue de facettes + deltas -> partition »
// (docs/ECHELLE.md § 8 bis, commit 1 des auditeurs ; theoreme T5). Autorite
// INDEPENDANTE du fold : les deltas de chaque ordre sont rejoues sur un
// union-find FRAIS indexe par les fid du catalogue trie (`facet_keys`), sans
// consulter `final_canon_fid`, et le resultat est compare champ a champ :
//   (1) chaque delta a `output` = minimum de (parents ∪ nees) (T5) ;
//   (2) chaque facette nait exactement une fois (`born`) ou n'apparait jamais
//       nee : alors elle est son propre canonique final (feuille jamais
//       fusionnee) — et toute facette est couverte par la reconstruction ;
//   (3) la partition reconstruite (canonique = minimum du bloc) est EGALE a
//       `final_canon_fid`, fid par fid ;
//   (4) chaque parent est, au moment du delta, le canonique d'un bloc vivant
//       (jamais deux fois le meme bloc dans un delta, jamais un bloc absorbe).
// Planchers : >= --min-deltas deltas et >= --min-facets facettes cumules sur
// les nuages (jamais vert par vacuite). Codes : 0 ; 1 desaccord ; 2 refus ;
// 3 plancher ; 4 mutant tue (`drop-nonmerge`, `attach-prebatch` : le flux de
// deltas ne reconstruit plus la partition).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {
u64 g_deltas = 0, g_facets = 0, g_mism = 0, g_orders = 0;

struct UF {
  std::vector<u32> parent, canon;
  explicit UF(size_t n) : parent(n), canon(n) {
    for (size_t i = 0; i < n; ++i) { parent[i] = (u32)i; canon[i] = (u32)i; }
  }
  u32 find(u32 v) { while (parent[v] != v) { parent[v] = parent[parent[v]]; v = parent[v]; } return v; }
  void unite(u32 a, u32 b) {
    a = find(a); b = find(b);
    if (a == b) return;
    parent[b] = a;
    canon[a] = std::min(canon[a], canon[b]);
  }
};

u32 fid_of(const std::vector<FacetKey>& keys, const FacetKey& k) {
  const auto it = std::lower_bound(keys.begin(), keys.end(), k, [](const FacetKey& x, const FacetKey& y) { return x < y; });
  if (it == keys.end() || !(*it == k)) return UINT32_MAX;
  return (u32)(it - keys.begin());
}

void replay_order(u64 K, const ForestResult& r, const char* family) {
  ++g_orders;
  const std::vector<FacetKey>& keys = r.facet_keys;
  const size_t nf = keys.size();
  g_facets += nf;
  for (size_t i = 1; i < nf; ++i)
    if (!(keys[i - 1] < keys[i])) { ++g_mism; std::printf("%s K=%llu : catalogue non strictement croissant\n", family, (unsigned long long)K); return; }
  UF uf(nf);
  std::vector<u8> born_count(nf, 0), alive_root(nf, 0);  // alive_root[fid] : fid est le canonique d'un bloc vivant
  u64 bad = 0;
  for (const ComponentDelta& d : r.deltas) {
    ++g_deltas;
    // (1) output = min(parents ∪ born)
    FacetKey mn;
    bool has = false;
    for (const FacetKey& p : d.parents) if (!has || p < mn) { mn = p; has = true; }
    for (const FacetKey& b : d.born) if (!has || b < mn) { mn = b; has = true; }
    if (!has || !(mn == d.output)) { ++bad; continue; }
    // membres : nees (nouvelles feuilles) et parents (blocs vivants, canoniques)
    u32 first = UINT32_MAX;
    for (const FacetKey& b : d.born) {
      const u32 f = fid_of(keys, b);
      if (f == UINT32_MAX) { ++bad; continue; }
      if (born_count[f]++ != 0) ++bad;  // nee deux fois
      if (alive_root[f]) ++bad;         // nee alors qu'un bloc la porte deja
      if (first == UINT32_MAX) first = f; else uf.unite(first, f);
    }
    for (const FacetKey& p : d.parents) {
      const u32 f = fid_of(keys, p);
      if (f == UINT32_MAX) { ++bad; continue; }
      const u32 rt = uf.find(f);
      // (4) parent = canonique d'un bloc vivant, pas deja absorbe dans ce delta
      if (uf.canon[rt] != f) ++bad;
      if (!alive_root[f] && !(uf.canon[rt] == f && rt == f && born_count[f] == 0)) {
        // feuille jamais nee (singleton implicite) ou bloc vivant : sinon parent illegal
        if (alive_root[f] == 0 && born_count[f] == 0) { /* singleton implicite : accepte */ }
        else ++bad;
      }
      if (first == UINT32_MAX) first = f; else uf.unite(first, f);
    }
    // apres le delta : les anciens canoniques des parents ne sont plus vivants, l'output l'est
    for (const FacetKey& p : d.parents) { const u32 f = fid_of(keys, p); if (f != UINT32_MAX) alive_root[f] = 0; }
    const u32 fo = fid_of(keys, d.output);
    if (fo == UINT32_MAX || uf.canon[uf.find(fo)] != fo) ++bad;
    else alive_root[fo] = 1;
  }
  // (3) partition reconstruite == final_canon_fid
  if (r.final_canon_fid.size() != nf) { ++g_mism; std::printf("%s K=%llu : taille de final_canon_fid\n", family, (unsigned long long)K); return; }
  u64 diff = 0;
  for (size_t f = 0; f < nf; ++f)
    if (uf.canon[uf.find((u32)f)] != r.final_canon_fid[f]) ++diff;
  if (bad || diff) {
    ++g_mism;
    std::printf("%s K=%llu : DESACCORD deltas incoherents=%llu canoniques differents=%llu / %zu\n", family, (unsigned long long)K,
                (unsigned long long)bad, (unsigned long long)diff, nf);
  }
}
}  // namespace

int main(int argc, char** argv) {
  u64 min_deltas = 10000, min_facets = 100000;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--min-deltas=", 0) == 0) min_deltas = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--min-facets=", 0) == 0) min_facets = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  // Mutants tues : `drop-nonmerge` (naissances et croissances absentes du flux : les nees ne sont plus retrouvees) et
  // `attach-prebatch` (attachements traites comme actifs pre-lot : parents illegaux). `canonical-is-uf-root` est
  // refuse en amont par les invariants du pipeline (statut invariant, code 2) : il n'atteint jamais cette porte.
  const bool mutant = MHGP5_MUTANT("drop-nonmerge") || MHGP5_MUTANT("attach-prebatch");
  const struct { CloudFamily f; int n; } clouds[] = {{CloudFamily::kUniform, 1200}, {CloudFamily::kEightClusters, 1200}, {CloudFamily::kScanlineSinglePass, 1500},
                                                    {CloudFamily::kTerrain, 1000}, {CloudFamily::kTwoLines, 400}, {CloudFamily::kCollinearSeven, 7}};
  for (const auto& c : clouds) {
    const std::vector<InputPoint> in = make_family_input(c.f, c.n, cloud_family_default_coord(c.f, c.n), 3);
    RunOptions o;
    o.threads = 4;
    const char* fam = cloud_family_name(c.f);
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult& r) { replay_order(K, r, fam); };
    const RunResult rr = run_pipeline(in, o);
    if (rr.status != PipelineStatus::kCompleteRegular) { std::printf("REFUS %s : %s\n", fam, rr.message.c_str()); return 2; }
  }
  std::printf("delta_replay_gate ordres=%llu deltas=%llu facettes=%llu desaccords=%llu\n", (unsigned long long)g_orders, (unsigned long long)g_deltas,
              (unsigned long long)g_facets, (unsigned long long)g_mism);
  if (g_deltas < min_deltas || g_facets < min_facets) { std::printf("PLANCHER\n"); return 3; }
  if (mutant) { if (g_mism) return 4; std::printf("MUTANT NON TUE\n"); return 1; }
  if (g_mism) return 1;
  std::printf("delta_replay_gate OK\n");
  return 0;
}
