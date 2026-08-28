// MorseHGP3D v5 — SONDE MIROIR RESIDENT / VIVANT (audit du 28 aout : « comparer
// resident/vivant en ordre miroir sur le meme endpoint, setup et
// catalogue/rejeu inclus, avec temps par etape et RSS »).
//
// Ce n'est pas une porte : c'est une MESURE, et elle est faite pour pouvoir
// REFUTER l'idee que le reducteur vivant serait plus rapide ou plus leger de
// bout en bout. Le perimetre est identique des deux cotes :
//   preparation (tri des evenements, lots, internement, fusion) — COMMUNE ;
//   resident : `reduce_fold` (FidState 32 o par facette + final_canon_fid) ;
//   vivant   : durees de vie PREMIERE/DERNIERE puis `reduce_fold_live`, plus,
//              si `--rejeu`, la reconstruction de la partition par les deltas
//              (ce que le resident obtient gratuitement) — car sans elle les
//              deux cotes ne rendent pas le meme objet.
// Le RSS est le pic du PROCESSUS (`getrusage`), donc un majorant commun ; les
// deux cotes sont joues dans deux processus separes (`--cote=resident` /
// `--cote=vivant`) pour que ce pic soit attribuable. Un seul processus jouant
// les deux ne pourrait pas les separer.
//
// Sortie : une ligne par ordre K et une ligne de total, en millisecondes.
#include <sys/resource.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/forest/fold_live.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {

double now_ms() {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

long rss_kb() {
  struct rusage ru;
  return getrusage(RUSAGE_SELF, &ru) == 0 ? ru.ru_maxrss : 0;
}

// Reconstruction de la partition a partir des seuls deltas (le vivant doit la
// payer pour rendre le meme objet que le resident).
u64 replay_partition(const std::vector<FacetKey>& keys, const std::vector<ComponentDelta>& deltas, std::vector<u32>* canon_out) {
  const size_t nf = keys.size();
  std::vector<u32> parent(nf), canon(nf);
  for (size_t i = 0; i < nf; ++i) {
    parent[i] = (u32)i;
    canon[i] = (u32)i;
  }
  const auto find = [&](u32 v) {
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
  canon_out->resize(nf);
  for (size_t f = 0; f < nf; ++f) (*canon_out)[f] = canon[find((u32)f)];
  return nf;
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 8000, coord = 0, threads = 1;
  long long seed = 3;
  std::string side = "resident";
  bool replay = false;
  RunOptions opt;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) {
      if (!parse_cloud_family(a.c_str() + 9, &family)) return 2;
    } else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--seed=", 0) == 0) seed = std::atoll(a.c_str() + 7);
    else if (a.rfind("--threads=", 0) == 0) threads = std::atoi(a.c_str() + 10);
    else if (a.rfind("--smax=", 0) == 0) opt.smax = (u64)std::atoll(a.c_str() + 7);
    else if (a.rfind("--cote=", 0) == 0) side = a.substr(7);
    else if (a == "--rejeu") replay = true;
    else return 2;
  }
  if (side != "resident" && side != "vivant") return 2;
  if (n < 2 || threads < 1) return 2;
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  opt.threads = threads;

  double t_prepare = 0, t_reduce = 0, t_life = 0, t_replay = 0;
  u64 orders = 0, facets = 0, deltas = 0;
  u64 peak_alias = 0, logical_bytes = 0, alloc_bytes = 0;
  const long rss_before = rss_kb();

  opt.on_forest = [&](u64, const std::vector<ForestEvent>& events, const ForestResult&) {
    ++orders;
    const double t0 = now_ms();
    FoldPrepared fp = prepare_fold(events, threads);
    const double t1 = now_ms();
    t_prepare += t1 - t0;
    if (side == "resident") {
      const ForestResult r = reduce_fold(std::move(fp));
      t_reduce += now_ms() - t1;
      facets += r.facets;
      deltas += r.deltas.size();
    } else {
      // Le cote vivant paie EN PLUS les durees de vie (mesurees a part : elles
      // deviennent externes a L3) et, si demande, le rejeu qui lui rend la
      // partition que le resident produit d'office.
      const std::vector<FacetKey> keys = fp.keys;  // copie : le catalogue survit au reduce
      LiveFoldStats st;
      const double t2 = now_ms();
      const ForestResult r = reduce_fold_live(std::move(fp), &st);
      const double t3 = now_ms();
      t_reduce += t3 - t2;
      facets += r.facets;
      deltas += r.deltas.size();
      peak_alias = std::max(peak_alias, st.peak_aliases);
      logical_bytes = std::max(logical_bytes, st.logical_live_bytes);
      alloc_bytes = std::max(alloc_bytes, st.allocated_bytes);
      if (replay) {
        std::vector<u32> canon;
        replay_partition(keys, r.deltas, &canon);
        t_replay += now_ms() - t3;
      }
    }
  };
  const std::vector<InputPoint> in = make_family_input(family, n, coord, seed);
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  std::printf(
      "fold_live_probe cote=%s famille=%s n=%d fils=%d ordres=%llu facettes=%llu deltas=%llu "
      "preparation_ms=%.1f reduction_ms=%.1f durees_de_vie_incluses=%s rejeu_ms=%.1f total_fold_ms=%.1f "
      "pic_alias=%llu octets_logiques=%llu octets_alloues=%llu rss_max_kb=%ld rss_avant_kb=%ld\n",
      side.c_str(), cloud_family_name(family), n, threads, (unsigned long long)orders, (unsigned long long)facets,
      (unsigned long long)deltas, t_prepare, t_reduce, side == "vivant" ? "oui" : "sans objet", t_replay,
      t_prepare + t_reduce + t_replay, (unsigned long long)peak_alias, (unsigned long long)logical_bytes,
      (unsigned long long)alloc_bytes, rss_kb(), rss_before);
  (void)t_life;
  return 0;
}
