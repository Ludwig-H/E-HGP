// MorseHGP3D v5 — porte de RELABELING : la sortie publique vit dans l'espace
// des PointId fournis par l'appelant, jamais dans les rangs Morton.
// Trois runs sur la MEME geometrie :
//   A  ids brouilles (non monotones, au-dela du bit 31) ;
//   B  ids π(A), bijection non monotone, positions fixes ;
//   C  permutation physique des enregistrements de A.
// Exigences : digest_balls identique aux trois runs (la generation est aveugle
// aux ids) ; C bit-identique a A (digest_all) ; B = π(A) facette a facette :
// facet_keys de B = image triee par π de celles de A, blocs de la partition
// finale transportes par π, deltas transportes (parents/born) ; aucune cle
// publique hors de l'ensemble d'ids fourni. Mutant `dense-pointid` (cast du
// rang) tue.
// Codes : 0 conforme, 2 refus, 3 invariant, 4 mutant tue.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {
u32 base_id(u32 i) { return (i + 1u) * 0x9E3779B9u ^ 0x5A5A5A5Au; }
u32 pi(u32 v) { return v * 0x85EBCA6Bu ^ 0xC2B2AE35u; }

struct Snapshot {
  std::vector<std::vector<FacetKey>> facets;                 // par K
  std::vector<std::vector<u32>> canon;                       // par K
  std::vector<std::vector<ComponentDelta>> deltas;           // par K
  std::string dg_balls, dg_all;
};

Snapshot run(const std::vector<InputPoint>& in) {
  Snapshot s;
  s.facets.assign(11, {});
  s.canon.assign(11, {});
  s.deltas.assign(11, {});
  RunOptions o;
  o.threads = 4;
  o.digest = true;
  o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult& r) {
    s.facets[K] = r.facet_keys;
    s.canon[K] = r.final_canon_fid;
    s.deltas[K] = r.deltas;
  };
  const RunResult rr = run_pipeline(in, o);
  if (rr.status != PipelineStatus::kCompleteRegular) std::exit(2);
  s.dg_balls = rr.digest_balls;
  s.dg_all = rr.digest_all;
  return s;
}

FacetKey map_facet(const FacetKey& f) {
  FacetKey g = f;
  for (u8 i = 0; i < f.k; ++i) g.p[i] = pi(f.p[i]);
  std::sort(g.p.begin(), g.p.begin() + f.k);
  return g;
}
}  // namespace

int main(int argc, char** argv) {
  int n = 300;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const int coord = cloud_family_default_coord(CloudFamily::kUniform, n);
  const std::vector<P3> pts = make_family_cloud(CloudFamily::kUniform, n, coord, 3);
  std::vector<InputPoint> A(pts.size()), B(pts.size()), C(pts.size());
  bool above31 = false;
  for (size_t i = 0; i < pts.size(); ++i) {
    A[i] = InputPoint{base_id((u32)i), pts[i]};
    B[i] = InputPoint{pi(A[i].id), pts[i]};
    if (A[i].id >= (1u << 31)) above31 = true;
  }
  if (!above31) return 3;
  for (size_t i = 0; i < pts.size(); ++i) C[(i * 7919u) % pts.size()] = A[i];
  const Snapshot sa = run(A), sb = run(B), sc = run(C);
  int bad = 0;
  if (sa.dg_balls != sb.dg_balls || sa.dg_balls != sc.dg_balls) { std::fprintf(stderr, "digest_balls depend des ids\n"); ++bad; }
  if (sa.dg_all != sc.dg_all) { std::fprintf(stderr, "permutation physique : sortie non bit-identique\n"); ++bad; }
  std::vector<u32> idsA;
  for (const InputPoint& p : A) idsA.push_back(p.id);
  std::sort(idsA.begin(), idsA.end());
  for (u64 K = 1; K <= 10 && K < sa.facets.size(); ++K) {
    // Facettes : image par π.
    std::vector<FacetKey> mapped;
    for (const FacetKey& f : sa.facets[K]) {
      for (u8 i = 0; i < f.k; ++i)
        if (!std::binary_search(idsA.begin(), idsA.end(), f.p[i])) { std::fprintf(stderr, "K=%llu : cle hors ids\n", (unsigned long long)K); ++bad; break; }
      mapped.push_back(map_facet(f));
    }
    std::sort(mapped.begin(), mapped.end());
    if (mapped != sb.facets[K]) { std::fprintf(stderr, "K=%llu : facettes non transportees par pi\n", (unsigned long long)K); ++bad; }
    // Blocs de la partition : ensembles de facettes transportes.
    std::map<FacetKey, std::vector<FacetKey>> blocksA, blocksB;
    for (size_t fid = 0; fid < sa.facets[K].size(); ++fid) blocksA[map_facet(sa.facets[K][sa.canon[K][fid]])].push_back(map_facet(sa.facets[K][fid]));
    for (size_t fid = 0; fid < sb.facets[K].size(); ++fid) blocksB[sb.facets[K][sb.canon[K][fid]]].push_back(sb.facets[K][fid]);
    std::vector<std::vector<FacetKey>> la, lb;
    for (auto& kv : blocksA) { std::sort(kv.second.begin(), kv.second.end()); la.push_back(kv.second); }
    for (auto& kv : blocksB) { std::sort(kv.second.begin(), kv.second.end()); lb.push_back(kv.second); }
    std::sort(la.begin(), la.end());
    std::sort(lb.begin(), lb.end());
    if (la != lb) { std::fprintf(stderr, "K=%llu : blocs de partition non transportes\n", (unsigned long long)K); ++bad; }
    // Deltas : (lot, parents, born) transportes.
    std::vector<std::vector<FacetKey>> da, db;
    // Les parents et l'output sont des REPRESENTANTS canoniques (minima de
    // FacetKey) : equivariants par blocs, pas point a point sous une
    // bijection non monotone — on compare (lot, |parents|, nees transportees).
    for (const ComponentDelta& d : sa.deltas[K]) {
      std::vector<FacetKey> v;
      FacetKey tag; tag.k = 2; tag.p[0] = (u32)d.batch; tag.p[1] = (u32)d.parents.size(); v.push_back(tag);
      std::vector<FacetKey> born;
      for (const FacetKey& f : d.born) born.push_back(map_facet(f));
      std::sort(born.begin(), born.end());
      v.insert(v.end(), born.begin(), born.end());
      da.push_back(v);
    }
    for (const ComponentDelta& d : sb.deltas[K]) {
      std::vector<FacetKey> v;
      FacetKey tag; tag.k = 2; tag.p[0] = (u32)d.batch; tag.p[1] = (u32)d.parents.size(); v.push_back(tag);
      v.insert(v.end(), d.born.begin(), d.born.end());
      db.push_back(v);
    }
    std::sort(da.begin(), da.end()); std::sort(db.begin(), db.end());
    if (da != db) { std::fprintf(stderr, "K=%llu : deltas non transportes\n", (unsigned long long)K); ++bad; }
  }
  if (!inject.empty()) {
    if (bad) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (bad) return 3;
  std::printf("relabel_gate OK (n=%d, %zu facettes K=10)\n", n, sa.facets[10].size());
  return 0;
}
