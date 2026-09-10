// Auditeur : MonotoneHistory (vue DSU a coupes fermees croissantes) contre
// History::root_at (chaine immuable) sur des histoires aleatoires valides.
#include <cstdio>
#include <cstdlib>
#include <random>
#include "../src/forest/full_ball_tower.hpp"
using namespace mhgp7;
using namespace mhgp7::full_ball_detail;
static ExactLevel lv(u64 n) { return ExactLevel{{n, 0, 0}, 1}; }
int main(int argc, char** argv) {
  const unsigned histories = argc > 1 ? static_cast<unsigned>(std::atoi(argv[1])) : 40;
  const unsigned nodes_max = argc > 2 ? static_cast<unsigned>(std::atoi(argv[2])) : 3000;
  std::mt19937_64 rng(20260910);
  u64 total_queries = 0, total_nodes = 0, equal_level_fusions = 0, refused_nonmonotone = 0, max_chain = 0;
  for (unsigned h = 0; h < histories; ++h) {
    History hist;
    const unsigned births = 2 + rng() % (nodes_max / 2);
    for (unsigned i = 0; i < births; ++i) { hist.levels.push_back(lv(0)); hist.next.push_back(absent); }
    std::vector<u64> live(births); std::iota(live.begin(), live.end(), u64{0});
    u64 level = 0;
    const bool caterpillar = (h % 4 == 3);  // chaine profonde : chaque fusion absorbe la racine courante + une naissance
    if (caterpillar) {
      u64 spine = live.back(); live.pop_back();
      while (!live.empty()) {
        level += 1;
        const u64 id = hist.levels.size();
        hist.levels.push_back(lv(level)); hist.next.push_back(absent);
        hist.next[spine] = id; hist.next[live.back()] = id; live.pop_back(); spine = id;
      }
      live.push_back(spine);
    }
    while (live.size() > 1 && hist.levels.size() < nodes_max) {
      // un lot : plusieurs fusions au MEME niveau sur des racines de niveau < level
      level += 1 + rng() % 3;
      const unsigned fusions = 1 + rng() % 3;
      std::vector<u64> pool;
      for (u64 r : live) if (compare_exact_level(hist.levels[r], lv(level)) < 0) pool.push_back(r);
      std::shuffle(pool.begin(), pool.end(), rng);
      size_t at = 0;
      for (unsigned f = 0; f < fusions && at + 2 <= pool.size(); ++f) {
        const unsigned arity = 2 + rng() % 3;
        const size_t take = std::min<size_t>(arity, pool.size() - at);
        if (take < 2) break;
        const u64 id = hist.levels.size();
        hist.levels.push_back(lv(level)); hist.next.push_back(absent);
        for (size_t j = 0; j < take; ++j) { hist.next[pool[at + j]] = id; live.erase(std::find(live.begin(), live.end(), pool[at + j])); }
        at += take; live.push_back(id);
        if (f) ++equal_level_fusions;
      }
    }
    total_nodes += hist.levels.size();
    FullBallStats st;
    MonotoneHistory mono(hist, st);
    // suite monotone de coupes : niveaux 0..level+1, ouvert puis ferme
    for (u64 c = 0; c <= level + 1; ++c) for (bool closed : {false, true}) {
      const ExactLevel cut = lv(c);
      for (u64 token = 0; token < hist.levels.size(); token += 1 + (hist.levels.size() > 400 ? rng() % 7 : 0)) {
        if (!full_coverage_detail::admitted(hist.levels[token], cut, closed)) continue;
        const u64 expected = hist.root_at(token, cut, closed);
        const u64 got = mono.root_at(token, cut, closed);
        if (expected != got) { std::printf("{\"status\":\"divergent\",\"history\":%u,\"token\":%llu,\"cut\":%llu,\"closed\":%d,\"expected\":%llu,\"got\":%llu}\n", h, (unsigned long long)token, (unsigned long long)c, closed, (unsigned long long)expected, (unsigned long long)got); return 1; }
        ++total_queries;
      }
    }
    // profondeur maximale de chaine (cout de l'immuable)
    for (u64 token = 0; token < hist.levels.size(); ++token) { u64 d = 0, t = token; while (hist.next[t] != absent) { t = hist.next[t]; ++d; } max_chain = std::max(max_chain, d); }
    // refus d'une requete non monotone (retour a une coupe anterieure)
    try { (void)mono.root_at(0, lv(0), false); std::printf("{\"status\":\"nonmonotone_accepted\"}\n"); return 1; }
    catch (const Failure& f) { if (std::string_view(f.reason) != "full_ball_lower_cut_not_monotone") { std::printf("{\"status\":\"wrong_reason\",\"reason\":\"%s\"}\n", f.reason); return 1; } ++refused_nonmonotone; }
    if (h == 0) std::printf("{\"first_history_stats\":{\"lower_edges_indexed\":%llu,\"lower_nodes_activated\":%llu,\"lower_edges_activated\":%llu,\"lower_queries\":%llu,\"lower_find_steps\":%llu,\"lower_path_writes\":%llu}}\n",
      (unsigned long long)st.lower_edges_indexed, (unsigned long long)st.lower_nodes_activated, (unsigned long long)st.lower_edges_activated, (unsigned long long)st.lower_queries, (unsigned long long)st.lower_find_steps, (unsigned long long)st.lower_path_writes);
  }
  std::printf("{\"status\":\"passed\",\"histories\":%u,\"nodes\":%llu,\"queries\":%llu,\"equal_level_fusions\":%llu,\"max_chain_depth\":%llu,\"refused_nonmonotone\":%llu}\n",
    histories, (unsigned long long)total_nodes, (unsigned long long)total_queries, (unsigned long long)equal_level_fusions, (unsigned long long)max_chain, (unsigned long long)refused_nonmonotone);
  return 0;
}
