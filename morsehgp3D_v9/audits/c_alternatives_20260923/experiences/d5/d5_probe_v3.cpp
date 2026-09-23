// Auditeur C (v9), famille D5 — sonde hors produit de la tour FULL.
// Usage :
//   d5_probe dump  <nuage.u32le> K <catalogue.bin>        (chaine -> catalogue, 1 fil)
//   d5_probe probe <nuage.u32le> K <catalogue.bin> [--no-product]
// Mesure, sur le MEME catalogue que la tour produit :
//   * la tour produit (voie statique, 1 fil) : temps, noeuds/naissances/fusions par K,
//     octets de la sortie explicite ;
//   * la resolution des facettes par K : graines, coups directs, longueur des chaines,
//     et ce que deux index de hachage exacts (graines I u U ; ensembles S_C u I_C - z)
//     remplaceraient ;
//   * une phase A "maigre" (tableaux u32, aucune allocation par bloc) sur les cibles
//     resolues, dont les comptes doivent egaler ceux du produit ;
//   * la taille d'un format de sortie compact exact.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/chain/tower_chain.hpp"

using namespace mhgp9;
using namespace mhgp9::tower;

static std::vector<gen::Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() % 12) throw std::invalid_argument("bad length");
  std::vector<gen::Point3> pts(bytes.size() / 12);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      c[a] = v;
    }
    pts[i] = gen::Point3{(gen::Coordinate)c[0], (gen::Coordinate)c[1], (gen::Coordinate)c[2]};
  }
  return pts;
}

using Clock = std::chrono::steady_clock;
static double ms_since(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

using KSet = std::array<i32, kFacetMaxK>;
struct KSetHash {
  std::size_t operator()(const KSet& k) const {
    std::uint64_t h = 0x9e3779b97f4a7c15ull;
    for (i32 s : k) { h ^= (std::uint32_t)s; h *= 0x100000001b3ull; h ^= h >> 29; }
    return (std::size_t)h;
  }
};
struct BallKeyHash {
  std::size_t operator()(const BallKey& k) const {
    std::uint64_t h = 0x9e3779b97f4a7c15ull;
    for (i128 v : {k.a, k.b[0], k.b[1], k.b[2], k.c}) {
      for (std::uint64_t w : {(std::uint64_t)(u128)v, (std::uint64_t)((u128)v >> 64)}) {
        h ^= w + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
      }
    }
    h ^= h >> 31; h *= 0xbf58476d1ce4e5b9ull; h ^= h >> 29;
    return (std::size_t)h;
  }
};

#include <ctime>
static double cpu_ms() {
  timespec ts{};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec * 1e-6;
}
static void fail(const char* what) { std::fprintf(stderr, "FAIL %s\n", what); std::exit(3); }

int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: d5_probe dump|probe cloud K catalogue.bin [--no-product]\n"); return 2; }
  const std::string mode = argv[1];
  const auto pts = read_u32le(argv[2]);
  const unsigned kmax = (unsigned)std::stoul(argv[3]);
  const std::string cat_path = argv[4];
  bool run_product = true;
  for (int a = 5; a < argc; ++a) if (std::string(argv[a]) == "--no-product") run_product = false;

  std::vector<BallData> balls;
  if (mode == "dump") {
    ChainOptions opt;
    opt.kmax = kmax; opt.workers = 1; opt.run_tower = false; opt.keep_catalogue = true;
    const auto t = Clock::now();
    auto res = run_tower_chain(pts, opt);
    if (res.status != ChainStatus::kComplete) { std::fprintf(stderr, "chain %s\n", res.reason.c_str()); return 1; }
    balls = std::move(res.catalogue_balls);
    std::FILE* f = std::fopen(cat_path.c_str(), "wb");
    const std::uint64_t count = balls.size();
    std::fwrite(&count, 8, 1, f);
    std::fwrite(balls.data(), sizeof(BallData), balls.size(), f);
    std::fclose(f);
    std::printf("{\"mode\":\"dump\",\"balls\":%zu,\"chain_ms\":%.1f,\"cpu_s\":%.2f}\n", balls.size(), ms_since(t), res.times.cpu_s);
    return 0;
  }
  {
    std::FILE* f = std::fopen(cat_path.c_str(), "rb");
    if (!f) fail("catalogue_open");
    std::uint64_t count = 0;
    if (std::fread(&count, 8, 1, f) != 1) fail("catalogue_read");
    balls.resize(count);
    if (std::fread(balls.data(), sizeof(BallData), count, f) != count) fail("catalogue_read");
    std::fclose(f);
    // Un catalogue K10 contient exactement le catalogue K < 10 : p + q_min <= K + 1.
    std::vector<BallData> kept;
    kept.reserve(balls.size());
    for (const auto& b : balls) if ((unsigned)b.n_interior + b.arity <= kmax + 1) kept.push_back(b);
    balls.swap(kept);
  }
  // Index de la tour, construit exactement comme la chaine (PointId = rang d'entree).
  std::vector<InputPoint> input(pts.size());
  for (std::size_t i = 0; i < pts.size(); ++i) input[i] = InputPoint{(PointId)i, P3{pts[i].x, pts[i].y, pts[i].z}};
  const CloudIndex ix = build_cloud_index(input);
  const i32 m = ix.unique_count();
  std::vector<i32> geo_of_id(pts.size(), -1);
  for (i32 u = 0; u < m; ++u) geo_of_id[ix.point_id(u)] = u;

  std::printf("{\"n\":%zu,\"kmax\":%u,\"balls\":%zu", pts.size(), kmax, balls.size());

  // ---------------- Tour produit (voie statique, 1 fil) ----------------
  std::vector<std::uint64_t> prod_nodes(kmax + 1, 0), prod_merges(kmax + 1, 0), prod_births(kmax + 1, 0);
  if (run_product) {
    const auto t = Clock::now();
    const double c0 = cpu_ms();
    auto tw = build_full_ball_tower(ix, balls, kmax, 1, {}, true);
    const double tower_ms = ms_since(t);
    const double tower_cpu_ms = cpu_ms() - c0;
    if (tw.status != FullBallStatus::kCompleteRelative) { std::fprintf(stderr, "tower %s\n", tw.reason); return 1; }
    std::uint64_t node_b = 0, parent_b = 0, succ_b = 0, contrib_b = 0, lower_b = 0, pop_b = 0, pop_rows = 0, pop_ids = 0;
    for (unsigned k = 1; k <= tw.orders.size(); ++k) {
      const auto& o = tw.orders[k - 1];
      prod_nodes[k] = o.forest.nodes().size();
      for (const auto& nd : o.forest.nodes()) { if (nd.parent_count >= 2) ++prod_merges[k]; else ++prod_births[k]; }
      node_b += o.forest.nodes().size() * sizeof(FullNode);
      parent_b += o.forest.parents().size() * sizeof(FullNodeId);
      succ_b += o.forest.successors().size() * sizeof(FullNodeId);
      contrib_b += o.forest.contributions().size() * sizeof(FullDatedContribution);
      lower_b += o.lower_nodes.size() * sizeof(FullNodeId);
    }
    const auto& bank = *tw.orders[0].forest.populations();
    pop_rows = bank.rows().size();
    for (const auto& row : bank.rows()) pop_ids += row.interior.size() + row.shell.size();
    pop_b = pop_rows * sizeof(FullCoveragePopulation) + pop_ids * sizeof(PointId);
    const auto& s = tw.stats;
    std::printf(",\"product_cpu_ms\":%.1f", tower_cpu_ms);
    std::printf(",\"product\":{\"tower_ms_1thread\":%.1f,\"representatives\":%llu,\"meb_calls\":%llu,\"anchor_hits\":%llu,"
                "\"intruder_queries\":%llu,\"intruder_nodes\":%llu,\"births\":%llu,\"merges\":%llu,\"singleton_lots\":%llu,"
                "\"grouped_lots\":%llu,\"bytes\":{\"nodes\":%llu,\"parents\":%llu,\"successors\":%llu,\"contributions\":%llu,"
                "\"lower_nodes\":%llu,\"population_rows\":%llu,\"population_bytes_no_malloc_overhead\":%llu,"
                "\"sizeof_node\":%zu,\"sizeof_contribution\":%zu,\"sizeof_population\":%zu}}",
                tower_ms, (unsigned long long)s.representatives, (unsigned long long)s.resolve_work.calls,
                (unsigned long long)s.anchor_hits, (unsigned long long)s.intruder_queries,
                (unsigned long long)s.intruder_nodes, (unsigned long long)s.births, (unsigned long long)s.merges,
                (unsigned long long)s.singleton_lots, (unsigned long long)s.grouped_lots,
                (unsigned long long)node_b, (unsigned long long)parent_b, (unsigned long long)succ_b,
                (unsigned long long)contrib_b, (unsigned long long)lower_b, (unsigned long long)pop_rows,
                (unsigned long long)pop_b, sizeof(FullNode), sizeof(FullDatedContribution), sizeof(FullCoveragePopulation));
  }

  // ---------------- Programmes par K (meme ordre que le produit) ----------------
  const auto t_prog = Clock::now();
  std::unordered_map<std::uint32_t, local_plateau::ShellTable> extra;
  std::vector<std::uint8_t> qmin(balls.size());
  for (std::size_t j = 0; j < balls.size(); ++j) {
    const auto& b = balls[j];
    qmin[j] = b.arity;
    if (b.n_shell != b.arity) {
      local_plateau::LocalCensus local{b.key, {}, {}};
      for (i32 s : b.interior()) local.interior.push_back({ix.point_id(s), ix.upos[s]});
      for (i32 s : b.shell()) local.shell.push_back({ix.point_id(s), ix.upos[s]});
      auto table = local_plateau::ShellTable::prepare(std::move(local));
      if (table.q_min() != b.arity) fail("qmin");
      extra.emplace((std::uint32_t)j, std::move(table));
    }
  }
  std::vector<std::uint32_t> by_level(balls.size());
  for (std::size_t j = 0; j < balls.size(); ++j) by_level[j] = (std::uint32_t)j;
  {
    std::vector<double> approx(balls.size());
    for (std::size_t j = 0; j < balls.size(); ++j) approx[j] = level_approximation(balls[j].level);
    std::sort(by_level.begin(), by_level.end(), [&](std::uint32_t a, std::uint32_t b) {
      const double x = approx[a], y = approx[b];
      if (x < y * kLevelFilterMargin) return true;
      if (y < x * kLevelFilterMargin) return false;
      const int c = compare_exact_level(balls[a].level, balls[b].level);
      return c != 0 ? c < 0 : a < b;
    });
  }
  // rang de plateau exact
  std::vector<std::uint32_t> plateau(balls.size());
  std::uint32_t nplateaus = 0;
  for (std::size_t j = 0; j < by_level.size(); ++j) {
    if (j && !same_exact_level(balls[by_level[j - 1]].level, balls[by_level[j]].level)) ++nplateaus;
    plateau[by_level[j]] = nplateaus;
  }
  std::vector<std::vector<std::uint32_t>> programs(kmax + 1);
  for (std::uint32_t b : by_level) {
    const unsigned lo = balls[b].n_interior + qmin[b] - 1;
    const unsigned hi = std::min<unsigned>(kmax, balls[b].n_interior + balls[b].n_shell);
    for (unsigned k = lo; k <= hi; ++k) programs[k].push_back(b);
  }
  const double prog_ms = ms_since(t_prog);

  // Catalogue par cle exacte.
  std::unordered_map<BallKey, std::uint32_t, BallKeyHash> by_key;
  by_key.reserve(balls.size() * 2);
  for (std::size_t j = 0; j < balls.size(); ++j) by_key.emplace(balls[j].key, (std::uint32_t)j);

  // Facettes d'un bloc (replique de visit_block_at).
  auto visit = [&](unsigned k, std::uint32_t id, auto&& emit) {
    const auto& b = balls[id];
    if (b.n_shell == b.arity) {
      if (k == (unsigned)b.n_interior + b.n_shell) return;
      if (k + 1 != (unsigned)b.n_interior + b.n_shell) fail("regular_rank");
      for (std::size_t omit = 0; omit < b.n_shell; ++omit) {
        KSet s{};
        std::size_t n = 0;
        for (i32 x : b.interior()) s[n++] = x;
        for (std::size_t j = 0; j < b.n_shell; ++j) if (j != omit) s[n++] = b.shell_ids[j];
        std::sort(s.begin(), s.begin() + n);
        emit(s, n);
      }
    } else {
      const auto& table = extra.at(id);
      const auto rank = table.rank(k);
      if (!rank.present) fail("absent_block");
      for (const auto& comp : rank.strict_components) {
        KSet s{};
        std::size_t n = 0;
        for (std::size_t j = 0; j < comp.interior_prefix; ++j) s[n++] = geo_of_id[table.census().interior[j].id];
        for (std::size_t j = 0; j < table.census().shell.size(); ++j)
          if (comp.representative_shell & (1u << j)) s[n++] = geo_of_id[table.census().shell[j].id];
        std::sort(s.begin(), s.begin() + n);
        emit(s, n);
      }
    }
  };

  // Recherche d'intrus (replique de intruder_work) avec compteur de noeuds.
  std::vector<NodeRef> stack;
  auto intruder = [&](const BallKey& key, const i32* sel, std::size_t nsel, std::uint64_t& nodes) -> i32 {
    auto member = [&](i32 u) { return std::binary_search(sel, sel + nsel, u); };
    const census_detail::AxisBounds bounds(key);
    stack.clear(); stack.push_back(ix.root());
    while (!stack.empty()) {
      const auto node = stack.back(); stack.pop_back(); ++nodes;
      i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
      if (lo >= 0) continue;
      if (hi < 0) {
        const auto range = ix.range_of(node);
        for (i32 u = range.first; u <= range.last; ++u) if (!member(u)) return u;
      } else if (is_leaf(node)) {
        const i32 u = leaf_index(node);
        if (!member(u) && key.power(ix.upos[u]) < 0) return u;
      } else {
        stack.push_back(ix.nodes[node].right); stack.push_back(ix.nodes[node].left);
      }
    }
    return -1;
  };

  std::printf(",\"programs_ms\":%.1f,\"plateaus\":%u,\"orders\":[", prog_ms, nplateaus + 1);

  // Etat de la phase A maigre, K-1 garde pour rien (les images verticales ne sont pas rejouees ici).
  std::uint64_t tot_compact = 0, tot_nodes = 0;
  double tot_lean_ms = 0, tot_resolve_ms = 0;
  for (unsigned k = 1; k <= kmax; ++k) {
    const auto& program = programs[k];
    // --- Index exacts de hachage : graines (I u U, |.|=K) et ensembles S_C u (I_C - z).
    std::unordered_map<KSet, std::uint32_t, KSetHash> seeds, hits;
    std::uint64_t hit_entries = 0;
    for (std::uint32_t id : program) {
      const auto& b = balls[id];
      const unsigned pu = (unsigned)b.n_interior + b.n_shell;
      if (k == pu) {
        KSet s{}; std::size_t n = 0;
        for (i32 x : b.interior()) s[n++] = x;
        for (i32 x : b.shell()) s[n++] = x;
        std::sort(s.begin(), s.begin() + n);
        if (!seeds.emplace(s, id).second) fail("dup_seed");
      } else if (b.n_shell == b.arity && k + 1 == pu) {
        for (std::size_t z = 0; z < b.n_interior; ++z) {
          KSet s{}; std::size_t n = 0;
          for (std::size_t j = 0; j < b.n_interior; ++j) if (j != z) s[n++] = b.interior_ids[j];
          for (i32 x : b.shell()) s[n++] = x;
          std::sort(s.begin(), s.begin() + n);
          if (!hits.emplace(s, id).second) fail("dup_hit");  // MEB unique : une seule boule par ensemble
          ++hit_entries;
        }
      }
    }
    // --- Resolution des facettes (K >= 2), dedoublonnees par cle.
    std::uint64_t reps = 0, uniq = 0, seed0 = 0, hit0 = 0, hit0_idx = 0, chain = 0, meb_calls = 0, iq = 0, inodes = 0;
    std::uint64_t cat_intruder = 0, term_seed = 0, term_hit = 0, term_hit_idx = 0, max_depth = 0;
    std::array<std::uint64_t, 8> depth_hist{};
    std::unordered_map<KSet, std::uint32_t, KSetHash> target;  // facette -> uid
    std::vector<std::uint32_t> facet_target;  // dans l'ordre du programme
    std::vector<std::uint32_t> facet_uid;
    const auto t_res = Clock::now();
    const double c_res = cpu_ms();
    std::vector<std::pair<KSet, std::uint32_t>> uniq_list;
    AnchorMebWork work;
    if (k >= 2) {
      for (std::uint32_t id : program) {
        visit(k, id, [&](const KSet& f, std::size_t n) {
          if (n != k) fail("facet_size");
          ++reps;
          auto it = target.find(f);
          if (it != target.end()) { facet_target.push_back(uniq_list[it->second].second); facet_uid.push_back(it->second); return; }
          ++uniq;
          std::uint32_t t;
          const auto sd = seeds.find(f);
          const bool in_hits = hits.count(f) != 0;
          if (sd != seeds.end()) { t = sd->second; ++seed0; ++depth_hist[0]; }
          else {
            KSet s = f;
            std::array<P3, kFacetMaxK> pos{};
            auto meb = [&]() {
              for (std::size_t j = 0; j < k; ++j) pos[j] = ix.upos[s[j]];
              ++meb_calls;
              auto r = anchor_meb_proposed(std::span<const P3>(pos.data(), k), work);
              if (r.status != AnchorMebStatus::kOk) fail("meb");
              return r;
            };
            auto local = meb();
            std::uint64_t depth = 0;
            for (;;) {
              const auto found = by_key.find(local.key);
              if (found != by_key.end()) {
                const auto& b = balls[found->second];
                const unsigned lo = b.n_interior + b.arity - 1;
                if (k >= lo && k <= (unsigned)b.n_interior + b.n_shell) {
                  t = found->second;
                  if (depth == 0) { ++hit0; if (in_hits) ++hit0_idx; }
                  ++term_hit;
                  if (hits.count(s)) ++term_hit_idx;
                  break;
                }
                ++cat_intruder;  // cle cataloguee, intrus lisible dans BallData::interior()
              }
              ++iq;
              const i32 z = intruder(local.key, s.data(), k, inodes);
              if (z < 0) fail("no_intruder");
              s[local.support_slots[0]] = z;
              std::sort(s.begin(), s.begin() + k);
              ++depth;
              const auto sd2 = seeds.find(s);
              if (sd2 != seeds.end()) { t = sd2->second; ++term_seed; break; }
              auto next = meb();
              if (compare_exact_level(next.level, local.level) > 0) fail("radius_increased");
              local = next;
            }
            if (depth) ++chain;
            max_depth = std::max(max_depth, depth);
            ++depth_hist[std::min<std::uint64_t>(depth, 7)];
          }
          target.emplace(f, (std::uint32_t)uniq_list.size());
          facet_uid.push_back((std::uint32_t)uniq_list.size());
          uniq_list.emplace_back(f, t);
          facet_target.push_back(t);
        });
      }
    } else {
      for (std::uint32_t id : program) visit(k, id, [&](const KSet& f, std::size_t n) {
        if (n != 1) fail("k1_facet");
        ++reps;
        facet_target.push_back((std::uint32_t)ix.point_id(f[0]));
      });
    }
    const double res_ms = ms_since(t_res);
    const double res_cpu = cpu_ms() - c_res;
    tot_resolve_ms += res_ms;
    // --- Resolution D5 : graines, index S_C u (I_C - z), memo des etats de chaine,
    // intrus lu dans le catalogue quand la cle de la MEB y figure ; comparee facette par facette.
    const double c_d5 = cpu_ms();
    std::uint64_t d5_seed = 0, d5_hit = 0, d5_meb = 0, d5_meb_hit = 0, d5_cat = 0, d5_tree = 0, d5_nodes = 0;
    std::uint64_t d5_memo_hits = 0, d5_mismatch = 0, d5_cat_checked = 0;
    std::unordered_map<KSet, std::uint32_t, KSetHash> memo;
    std::vector<KSet> path;
    AnchorMebWork work5;
    for (const auto& [f, expect] : uniq_list) {
      std::uint32_t t = 0xffffffffu;
      if (auto it = seeds.find(f); it != seeds.end()) { t = it->second; ++d5_seed; }
      else if (auto ih = hits.find(f); ih != hits.end()) { t = ih->second; ++d5_hit; }
      else {
        path.clear(); path.push_back(f);
        KSet s = f;
        for (;;) {
          std::array<P3, kFacetMaxK> pos{};
          for (std::size_t j = 0; j < k; ++j) pos[j] = ix.upos[s[j]];
          ++d5_meb;
          const auto local = anchor_meb_proposed(std::span<const P3>(pos.data(), k), work5);
          if (local.status != AnchorMebStatus::kOk) fail("meb5");
          i32 z = -1;
          const auto found = by_key.find(local.key);
          if (found != by_key.end()) {
            const auto& b = balls[found->second];
            const unsigned lo = b.n_interior + b.arity - 1;
            if (k >= lo && k <= (unsigned)b.n_interior + b.n_shell) { t = found->second; ++d5_meb_hit; break; }
            // Intrus = plus petit rang Morton de I_D hors de l'etat (census complet du catalogue).
            ++d5_cat;
            for (i32 u : b.interior()) if (!std::binary_search(s.begin(), s.begin() + k, u) && (z < 0 || u < z)) z = u;
            std::uint64_t dummy = 0;
            ++d5_cat_checked;
            if (z != intruder(local.key, s.data(), k, dummy)) fail("catalogued_intruder_differs");
          } else {
            ++d5_tree;
            z = intruder(local.key, s.data(), k, d5_nodes);
          }
          if (z < 0) fail("no_intruder5");
          s[local.support_slots[0]] = z;
          std::sort(s.begin(), s.begin() + k);
          if (auto a = seeds.find(s); a != seeds.end()) { t = a->second; break; }
          if (auto a = hits.find(s); a != hits.end()) { t = a->second; break; }
          if (auto a = memo.find(s); a != memo.end()) { t = a->second; ++d5_memo_hits; break; }
          path.push_back(s);
        }
        for (const auto& st : path) memo.emplace(st, t);
      }
      if (t != expect) ++d5_mismatch;
    }
    const double d5_cpu = cpu_ms() - c_d5 ;
    if (d5_mismatch) fail("d5_terminal_mismatch");
    // --- Resolution "saut" : depuis D = MEB(etat) avec intrus, etat suivant = les K sites de D
    // les plus proches du centre (ordre exact des puissances, egalites par rang). Temoin de la
    // meme composante a lambda- (centre de D a distance <= r_D de F et de G) ; repli sur l'echange
    // du produit si le rayon ne baisse pas. Le terminal peut differer : la phase A maigre compare
    // les RACINES pre-lot des deux cibles.
    const double c_j = cpu_ms();
    std::uint64_t j_meb = 0, j_census = 0, j_census_nodes = 0, j_census_sites = 0, j_jumps = 0, j_fallback = 0;
    std::uint64_t j_seed = 0, j_hit = 0, j_memo_hits = 0, j_same_terminal = 0, j_max_steps = 0;
    std::vector<std::uint32_t> jump_target(uniq_list.size(), 0xffffffffu);
    std::unordered_map<KSet, std::uint32_t, KSetHash> jmemo;
    std::vector<std::pair<i128, i32>> inside;
    for (std::size_t uid = 0; uid < uniq_list.size(); ++uid) {
      const KSet& f = uniq_list[uid].first;
      std::uint32_t t = 0xffffffffu;
      if (auto it = seeds.find(f); it != seeds.end()) { t = it->second; ++j_seed; }
      else if (auto ih = hits.find(f); ih != hits.end()) { t = ih->second; ++j_hit; }
      else {
        path.clear(); path.push_back(f);
        KSet s = f;
        std::uint64_t steps = 0;
        for (;;) {
          if (++steps > 100000) fail("jump_no_termination");
          std::array<P3, kFacetMaxK> pos{};
          for (std::size_t j = 0; j < k; ++j) pos[j] = ix.upos[s[j]];
          ++j_meb;
          const auto local = anchor_meb_proposed(std::span<const P3>(pos.data(), k), work5);
          if (local.status != AnchorMebStatus::kOk) fail("mebj");
          if (const auto found = by_key.find(local.key); found != by_key.end()) {
            const auto& b = balls[found->second];
            const unsigned lo = b.n_interior + b.arity - 1;
            if (k >= lo && k <= (unsigned)b.n_interior + b.n_shell) { t = found->second; break; }
          }
          // Census exact de la boule fermee D.
          ++j_census;
          inside.clear();
          {
            const census_detail::AxisBounds bounds(local.key);
            stack.clear(); stack.push_back(ix.root());
            while (!stack.empty()) {
              const auto node = stack.back(); stack.pop_back(); ++j_census_nodes;
              i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
              if (lo > 0) continue;
              if (is_leaf(node)) {
                const i32 u = leaf_index(node);
                const i128 pw = local.key.power(ix.upos[u]);
                if (pw <= 0) inside.emplace_back(pw, u);
              } else if (hi <= 0) {
                const auto range = ix.range_of(node);
                for (i32 u = range.first; u <= range.last; ++u) inside.emplace_back(local.key.power(ix.upos[u]), u);
              } else { stack.push_back(ix.nodes[node].right); stack.push_back(ix.nodes[node].left); }
            }
          }
          j_census_sites += inside.size();
          if (inside.size() < k + 1) fail("jump_census_too_small");
          std::partial_sort(inside.begin(), inside.begin() + k, inside.end());
          KSet g{};
          for (std::size_t j = 0; j < k; ++j) g[j] = inside[j].second;
          std::sort(g.begin(), g.begin() + k);
          std::array<P3, kFacetMaxK> gp{};
          for (std::size_t j = 0; j < k; ++j) gp[j] = ix.upos[g[j]];
          ++j_meb;
          const auto dg = anchor_meb_proposed(std::span<const P3>(gp.data(), k), work5);
          if (dg.status != AnchorMebStatus::kOk) fail("mebj2");
          if (compare_exact_level(dg.level, local.level) < 0) { s = g; ++j_jumps; }
          else {
            // Repli : echange du produit (premier intrus en rang, premier site du support).
            ++j_fallback;
            i32 z = -1;
            for (const auto& [pw, u] : inside) if (pw < 0 && !std::binary_search(s.begin(), s.begin() + k, u) && (z < 0 || u < z)) z = u;
            if (z < 0) fail("jump_no_intruder");
            s[local.support_slots[0]] = z;
            std::sort(s.begin(), s.begin() + k);
          }
          if (auto a = seeds.find(s); a != seeds.end()) { t = a->second; break; }
          if (auto a = hits.find(s); a != hits.end()) { t = a->second; break; }
          if (auto a = jmemo.find(s); a != jmemo.end()) { t = a->second; ++j_memo_hits; break; }
          path.push_back(s);
        }
        j_max_steps = std::max(j_max_steps, steps);
        for (const auto& st : path) jmemo.emplace(st, t);
      }
      jump_target[uid] = t;
      if (t == uniq_list[uid].second) ++j_same_terminal;
    }
    const double j_cpu = cpu_ms() - c_j;
    std::vector<std::uint32_t> facet_target_jump(facet_uid.size());
    for (std::size_t f = 0; f < facet_uid.size(); ++f) facet_target_jump[f] = jump_target[facet_uid[f]];
    if (k == 1) facet_target_jump = facet_target;
    std::printf("%s{\"jump\":{\"seed\":%llu,\"hit\":%llu,\"meb\":%llu,\"census\":%llu,\"census_nodes\":%llu,"
                "\"census_sites\":%llu,\"jumps\":%llu,\"fallback_swaps\":%llu,\"memo_hits\":%llu,\"states\":%zu,"
                "\"same_terminal\":%llu,\"max_steps\":%llu,\"cpu_ms\":%.1f},",
                k > 1 ? "," : "", (unsigned long long)j_seed, (unsigned long long)j_hit, (unsigned long long)j_meb,
                (unsigned long long)j_census, (unsigned long long)j_census_nodes, (unsigned long long)j_census_sites,
                (unsigned long long)j_jumps, (unsigned long long)j_fallback, (unsigned long long)j_memo_hits, jmemo.size(),
                (unsigned long long)j_same_terminal, (unsigned long long)j_max_steps, j_cpu);

    // --- Phase A maigre : tableaux u32, union-find a compression, aucun vecteur par bloc.
    const auto t_lean = Clock::now();
    const double c_lean = cpu_ms();
    constexpr std::uint32_t NONE = 0xffffffffu;
    std::vector<std::uint32_t> anchor(balls.size(), NONE);
    std::vector<std::uint32_t> comp;   // par noeud : lui-meme si vivant, sinon vers un successeur
    std::vector<std::uint32_t> nnext;  // successeur exact (sortie)
    std::vector<std::uint32_t> nball;  // boule du noeud (niveau et population implicites), NONE pour les sites
    comp.reserve(program.size() + pts.size()); nnext.reserve(comp.capacity()); nball.reserve(comp.capacity());
    std::uint64_t births = 0, merges = 0, conts = 0, singleton = 0, grouped = 0;
    if (k == 1) for (std::size_t j = 0; j < pts.size(); ++j) { comp.push_back((std::uint32_t)j); nnext.push_back(NONE); nball.push_back(NONE); }
    auto find = [&](std::uint32_t x) {
      std::uint32_t r = x;
      while (comp[r] != r) r = comp[r];
      while (comp[x] != r) { const std::uint32_t nx = comp[x]; comp[x] = r; x = nx; }
      return r;
    };
    auto new_node = [&](std::uint32_t ball, const std::uint32_t* par, std::size_t np) {
      const std::uint32_t id = (std::uint32_t)comp.size();
      comp.push_back(id); nnext.push_back(NONE); nball.push_back(ball);
      for (std::size_t i = 0; i < np; ++i) { comp[par[i]] = id; nnext[par[i]] = id; }
      if (np == 0) ++births; else ++merges;
      return id;
    };
    // Offsets des facettes par bloc (dans l'ordre du programme).
    std::vector<std::uint32_t> foff(program.size() + 1, 0);
    {
      std::size_t at = 0;
      for (std::size_t j = 0; j < program.size(); ++j) {
        const auto& b = balls[program[j]];
        std::uint32_t c = 0;
        if (b.n_shell == b.arity) c = (k == (unsigned)b.n_interior + b.n_shell) ? 0 : b.n_shell;
        else c = (std::uint32_t)extra.at(program[j]).rank(k).strict_components.size();
        foff[j + 1] = foff[j] + c; at += c;
      }
      if (at != facet_target.size()) fail("facet_offsets");
    }
    std::vector<std::uint32_t> roots, lot_roots, lot_off, lot_dsu;
    std::uint64_t jump_root_equal = 0, jump_root_mismatch = 0, jump_anchor_missing = 0;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> owners;
    std::vector<std::uint32_t> lot_target;
    for (std::size_t begin = 0; begin < program.size();) {
      std::size_t end = begin + 1;
      while (end < program.size() && plateau[program[end]] == plateau[program[begin]]) ++end;
      // Lecture de l'etat pre-lot : racines de chaque bloc.
      lot_roots.clear(); lot_off.assign(1, 0);
      for (std::size_t j = begin; j < end; ++j) {
        const std::size_t first = lot_roots.size();
        for (std::uint32_t f = foff[j]; f < foff[j + 1]; ++f) {
          const std::uint32_t t = facet_target[f];
          std::uint32_t tok;
          if (k == 1) tok = t;  // site : noeud initial = PointId
          else { tok = anchor[t]; if (tok == NONE) fail("anchor_missing"); }
          lot_roots.push_back(find(tok));
          if (k > 1) {
            const std::uint32_t tj = anchor[facet_target_jump[f]];
            if (tj == NONE) ++jump_anchor_missing;
            else if (find(tj) != lot_roots.back()) ++jump_root_mismatch;
            else ++jump_root_equal;
          }
        }
        std::sort(lot_roots.begin() + first, lot_roots.end());
        lot_roots.erase(std::unique(lot_roots.begin() + first, lot_roots.end()), lot_roots.end());
        lot_off.push_back((std::uint32_t)lot_roots.size());
      }
      const std::size_t nb = end - begin;
      if (nb == 1) {
        ++singleton;
        const std::uint32_t np = lot_off[1];
        std::uint32_t tgt;
        if (np == 1) { tgt = lot_roots[0]; ++conts; }
        else tgt = new_node(program[begin], lot_roots.data(), np);
        anchor[program[begin]] = tgt;
      } else {
        ++grouped;
        lot_dsu.resize(nb);
        for (std::size_t b = 0; b < nb; ++b) lot_dsu[b] = (std::uint32_t)b;
        auto f2 = [&](std::uint32_t a) { while (lot_dsu[a] != a) { lot_dsu[a] = lot_dsu[lot_dsu[a]]; a = lot_dsu[a]; } return a; };
        owners.clear();
        for (std::size_t b = 0; b < nb; ++b) for (std::uint32_t r = lot_off[b]; r < lot_off[b + 1]; ++r) owners.emplace_back(lot_roots[r], (std::uint32_t)b);
        std::sort(owners.begin(), owners.end());
        for (std::size_t j = 1; j < owners.size(); ++j) if (owners[j - 1].first == owners[j].first) {
          const auto a = f2(owners[j - 1].second), b = f2(owners[j].second);
          lot_dsu[std::max(a, b)] = std::min(a, b);
        }
        lot_target.assign(nb, NONE);
        for (std::size_t g = 0; g < nb; ++g) {
          if (f2((std::uint32_t)g) != g) continue;
          roots.clear();
          std::size_t members = 0;
          for (std::size_t b = g; b < nb; ++b) if (f2((std::uint32_t)b) == g) {
            ++members;
            roots.insert(roots.end(), lot_roots.begin() + lot_off[b], lot_roots.begin() + lot_off[b + 1]);
          }
          std::sort(roots.begin(), roots.end());
          roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
          std::uint32_t tgt;
          if (roots.size() == 1) { tgt = roots[0]; conts += members; }
          else {
            if (roots.empty() && members != 1) fail("distinct_births");
            tgt = new_node(program[begin + g], roots.data(), roots.size());
          }
          for (std::size_t b = g; b < nb; ++b) if (f2((std::uint32_t)b) == g) lot_target[b] = tgt;
        }
        for (std::size_t b = 0; b < nb; ++b) anchor[program[begin + b]] = lot_target[b];
      }
      begin = end;
    }
    const double lean_ms = ms_since(t_lean);
    const double lean_cpu = cpu_ms() - c_lean;
    std::printf("\"lean_cpu_ms\":%.2f,\"jump_roots\":{\"equal\":%llu,\"mismatch\":%llu,\"anchor_missing\":%llu},",
                lean_cpu, (unsigned long long)jump_root_equal, (unsigned long long)jump_root_mismatch,
                (unsigned long long)jump_anchor_missing);
    tot_lean_ms += lean_ms;
    const std::uint64_t nodes = comp.size();
    tot_nodes += nodes;
    // Format compact : par noeud (boule u32, successeur u32, image verticale u32) = 12 octets.
    const std::uint64_t compact = nodes * 12;
    tot_compact += compact;
    std::printf("\"d5\":{\"seed\":%llu,\"hit\":%llu,\"meb\":%llu,\"meb_hit\":%llu,\"catalogued_intruders\":%llu,"
                "\"tree_intruders\":%llu,\"tree_nodes\":%llu,\"memo_hits\":%llu,\"distinct_chain_states\":%zu,"
                "\"cpu_ms\":%.1f,\"product_equiv_cpu_ms\":%.1f,\"mismatches\":%llu},",
                (unsigned long long)d5_seed, (unsigned long long)d5_hit, (unsigned long long)d5_meb,
                (unsigned long long)d5_meb_hit, (unsigned long long)d5_cat, (unsigned long long)d5_tree,
                (unsigned long long)d5_nodes, (unsigned long long)d5_memo_hits, memo.size(), d5_cpu - 0.0, res_cpu,
                (unsigned long long)d5_mismatch);
    (void)d5_cat_checked;
    std::printf("\"K\":%u,\"blocks\":%zu,\"facets\":%llu,\"unique\":%llu,\"seed_index\":%zu,\"hit_index\":%llu,"
                "\"seed0\":%llu,\"hit0\":%llu,\"hit0_in_index\":%llu,\"chains\":%llu,\"depth_hist\":[",
                k, program.size(), (unsigned long long)reps, (unsigned long long)uniq, seeds.size(),
                (unsigned long long)hit_entries, (unsigned long long)seed0, (unsigned long long)hit0,
                (unsigned long long)hit0_idx, (unsigned long long)chain);
    for (int i = 0; i < 8; ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)depth_hist[i]);
    std::printf("],\"max_depth\":%llu,\"meb_calls\":%llu,\"intruder_queries\":%llu,\"intruder_nodes\":%llu,"
                "\"catalogued_intruder_steps\":%llu,\"terminal_seed_after_step\":%llu,\"terminal_hit\":%llu,"
                "\"terminal_hit_in_index\":%llu,\"resolve_ms\":%.1f,\"lean\":{\"ms\":%.2f,\"nodes\":%llu,\"births\":%llu,"
                "\"merges\":%llu,\"continuations\":%llu,\"singleton_lots\":%llu,\"grouped_lots\":%llu,"
                "\"product_nodes\":%llu,\"product_births\":%llu,\"product_merges\":%llu}}",
                (unsigned long long)max_depth, (unsigned long long)meb_calls, (unsigned long long)iq,
                (unsigned long long)inodes, (unsigned long long)cat_intruder, (unsigned long long)term_seed,
                (unsigned long long)term_hit, (unsigned long long)term_hit_idx, res_ms, lean_ms,
                (unsigned long long)nodes, (unsigned long long)births, (unsigned long long)merges,
                (unsigned long long)conts, (unsigned long long)singleton, (unsigned long long)grouped,
                (unsigned long long)prod_nodes[k], (unsigned long long)prod_births[k], (unsigned long long)prod_merges[k]);
    if (run_product && (nodes != prod_nodes[k] || merges != prod_merges[k])) fail("lean_counts_differ");
  }
  std::printf("],\"total_nodes\":%llu,\"compact_bytes\":%llu,\"lean_ms_total\":%.1f,\"resolve_ms_total\":%.1f}\n",
              (unsigned long long)tot_nodes, (unsigned long long)tot_compact, tot_lean_ms, tot_resolve_ms);
  return 0;
}
