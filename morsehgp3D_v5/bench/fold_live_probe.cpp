// MorseHGP3D v5 — SONDE MIROIR RESIDENT / VIVANT (audit du 28 aout : « comparer
// resident/vivant en ordre miroir sur le meme endpoint, setup et
// catalogue/rejeu inclus, avec temps par etape et RSS »).
//
// DEUX REGIMES, et il faut les distinguer (audit du 28 aout) :
//   `--dump=<f>` puis `--from=<f>` : le MIROIR VRAI. La premiere invocation
//     ecrit les evenements de l'ordre le plus gros ; les suivantes ne lancent
//     AUCUN pipeline et n'executent qu'un seul reducteur — le pic de RSS est
//     alors attribuable a ce reducteur et a lui seul.
//   sans `--from` : un MICRO-BANC INCREMENTAL, et rien de plus. `run_pipeline`
//     execute deja le fold RESIDENT avant le callback dans les DEUX bras ; le
//     bras vivant ajoute ensuite un second reducteur. Les temps par etape y
//     restent comparables (les deux reducteurs sont mesures au meme endroit,
//     sur le meme etat prepare), mais le RSS n'y departage RIEN.
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
// Le RSS est le pic du PROCESSUS (`getrusage`) ; les deux cotes sont joues
// dans deux processus separes (`--cote=resident` / `--cote=vivant`). Cela ne
// suffit a l'attribuer qu'en regime `--from`.
//
// Sortie : une ligne par ordre K et une ligne de total, en millisecondes.
#include <sys/resource.h>

#include <algorithm>
#include <cstdio>
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
  std::string dump, from;
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
    else if (a.rfind("--dump=", 0) == 0) dump = a.substr(7);
    else if (a.rfind("--from=", 0) == 0) from = a.substr(7);
    else if (a == "--rejeu") replay = true;
    else return 2;
  }
  if (side != "resident" && side != "vivant") return 2;
  if (n < 2 || threads < 1) return 2;
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  opt.threads = threads;

  double t_prepare = 0, t_reduce = 0, t_life = 0, t_replay = 0;
  u64 orders = 0, facets = 0, deltas = 0;
  u64 peak_alias = 0, logical_bytes = 0, alloc_bytes = 0, map_bytes = 0;
  const long rss_before = rss_kb();
  std::vector<ForestEvent> biggest;  // ordre le plus gros, pour `--dump`

  // Un reducteur, un seul, sur un jeu d'evenements donne.
  const auto measure = [&](const std::vector<ForestEvent>& events) {
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
      return;
    }
    // Le cote vivant paie EN PLUS les durees de vie (dans `reduce_fold_live`)
    // et, si demande, le rejeu qui lui rend la partition que le resident
    // produit d'office.
    // Le catalogue n'est copie QUE si le rejeu est demande (il lui est
    // necessaire) : le copier sans cela ajoutait 274 Mo au bras vivant et
    // faussait entierement l'attribution du RSS.
    std::vector<FacetKey> keys;
    if (replay) keys = fp.keys;
    LiveFoldStats st;
    const double t2 = now_ms();
    const ForestResult r = reduce_fold_live(std::move(fp), &st);
    const double t3 = now_ms();
    t_reduce += t3 - t2;
    facets += r.facets;
    deltas += r.deltas.size();
    peak_alias = std::max(peak_alias, st.peak_aliases);
    logical_bytes = std::max(logical_bytes, st.logical_live_bytes);
    alloc_bytes = std::max(alloc_bytes, st.persistent_bytes);
    map_bytes = std::max(map_bytes, st.mapping_bytes);
    if (replay) {
      std::vector<u32> canon;
      replay_partition(keys, r.deltas, &canon);
      t_replay += now_ms() - t3;
    }
  };

  // ---- MIROIR VRAI : aucun pipeline, un seul reducteur dans ce processus.
  if (!from.empty()) {
    std::FILE* f = std::fopen(from.c_str(), "rb");
    if (!f) return 2;
    u64 count = 0;
    if (std::fread(&count, sizeof(u64), 1, f) != 1 || count > (u64)1e9) {
      std::fclose(f);
      return 2;
    }
    std::vector<ForestEvent> events((size_t)count);
    const bool ok_read = count == 0 || std::fread(events.data(), sizeof(ForestEvent), (size_t)count, f) == (size_t)count;
    std::fclose(f);
    if (!ok_read) return 2;
    measure(events);
    std::printf(
        "fold_live_probe regime=miroir cote=%s evenements=%llu ordres=%llu facettes=%llu deltas=%llu "
        "preparation_ms=%.1f reduction_ms=%.1f rejeu_ms=%.1f total_fold_ms=%.1f "
        "pic_alias=%llu octets_logiques=%llu octets_persistants=%llu octets_mapping=%llu rss_max_kb=%ld rss_avant_kb=%ld\n",
        side.c_str(), (unsigned long long)count, (unsigned long long)orders, (unsigned long long)facets, (unsigned long long)deltas, t_prepare,
        t_reduce, t_replay, t_prepare + t_reduce + t_replay, (unsigned long long)peak_alias, (unsigned long long)logical_bytes,
        (unsigned long long)alloc_bytes, (unsigned long long)map_bytes, rss_kb(), rss_before);
    return 0;
  }

  opt.on_forest = [&](u64, const std::vector<ForestEvent>& events, const ForestResult&) {
    if (!dump.empty() && events.size() > biggest.size()) biggest = events;
    measure(events);
  };
  const std::vector<InputPoint> in = make_family_input(family, n, coord, seed);
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  if (!dump.empty()) {
    std::FILE* f = std::fopen(dump.c_str(), "wb");
    if (!f) return 2;
    const u64 count = (u64)biggest.size();
    const bool ok_w = std::fwrite(&count, sizeof(u64), 1, f) == 1 &&
                      (count == 0 || std::fwrite(biggest.data(), sizeof(ForestEvent), biggest.size(), f) == biggest.size());
    std::fclose(f);
    if (!ok_w) return 3;
    std::printf("fold_live_probe dump=%s evenements=%llu octets=%llu\n", dump.c_str(), (unsigned long long)count,
                (unsigned long long)(count * sizeof(ForestEvent) + sizeof(u64)));
  }
  std::printf(
      "fold_live_probe regime=micro_banc_incremental cote=%s famille=%s n=%d fils=%d ordres=%llu facettes=%llu deltas=%llu "
      "preparation_ms=%.1f reduction_ms=%.1f durees_de_vie_incluses=%s rejeu_ms=%.1f total_fold_ms=%.1f "
      "pic_alias=%llu octets_logiques=%llu octets_persistants=%llu octets_mapping=%llu rss_max_kb=%ld rss_avant_kb=%ld\n",
      side.c_str(), cloud_family_name(family), n, threads, (unsigned long long)orders, (unsigned long long)facets,
      (unsigned long long)deltas, t_prepare, t_reduce, side == "vivant" ? "oui" : "sans objet", t_replay,
      t_prepare + t_reduce + t_replay, (unsigned long long)peak_alias, (unsigned long long)logical_bytes,
      (unsigned long long)alloc_bytes, (unsigned long long)map_bytes, rss_kb(), rss_before);
  (void)t_life;
  return 0;
}
