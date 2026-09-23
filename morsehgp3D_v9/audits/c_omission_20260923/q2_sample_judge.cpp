// Auditeur C, 23 septembre 2026 — juge d'echantillon q2 independant du generateur (audit, hors produit).
//
// Pour un site a tire et TOUT autre site b, la boule diametrale de {a,b}
// (centre au milieu, donc toujours dans l'interieur relatif du support) est
// recensee par balayage brut de tous les sites, en entiers exacts :
// x interieur strict <=> (x-a).(x-b) < 0, x sur la sphere <=> = 0 (u18 : |.| < 2^38).
// Si elle a p <= Kmax-1 interieurs, elle est admise (p + q_min <= Kmax+1 avec
// q_min = 2) : le catalogue DOIT contenir une boule de p interieurs dont la
// coquille contient a et b et est portee par cette meme sphere, avec autant de
// sites de coquille que le balayage en trouve. Aucune structure du generateur
// (WSPD, temoins, Pool) n'est utilisee. Cout par site tire : O(n) candidats b,
// chacun un balayage O(n) arrete des que p depasse Kmax-1, soit O(S n^2) pour S sites
// tires (O(n^3) si S = n : on garde S borne, 50 a 1 000 dans les campagnes).
// Recoupement (contrelecture adverse de C) : niveau exact egal (Boost cpp_int), memes interieurs, arite 2 ;
// sens inverse (EXTRA) : toute boule reguliere a 2 sites, d'arite 2, p <= Kmax-1, passant par un site tire,
// doit etre retrouvee. Une coquille de plus de 12 sites trouvee sur une chaine complete est une omission.
// Anti-vacuite ciblee : une cle trouvee de rang p = Kmax-1 est retiree et son site rejuge ; plancher
// --min-top de cles p = Kmax-1. Sites tires par permutation a graine publiee (--seed), index verifie contre
// les points d'entree ; sorties : incidences (site tire, partenaire), cles distinctes, empreinte FNV-1a.
//
//   q2_sample_judge family <uniform|terrain|clusters> <n> <Kmax> <sites> <workers>
//   q2_sample_judge file <cut.u32le> <Kmax> <sites> <workers>
//
// Options : --seed=S, --min-top=N (cles regulieres p = Kmax-1, arite 2), --inject=level | --inject=shell-dup |
// --inject=key (mutants : niveaux faux, coquille a doublon, cle seule faussee ; doivent rendre 1). La cle
// canonique de la boule diametrale est reconstruite et comparee a ball.key.
// Mutants d'index (refus en code 2 avant echantillonnage, marqueurs INDEX_*) : --inject=index-out-of-range |
// --inject=index-duplicate | --inject=index-missing.
// Code 0 conforme ; 1 manquante, EXTRA ou recoupement faux ; 2 argument/chaine ; 3 vacuite.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "src/chain/tower_chain.hpp"
#include "src/tower/tree/cloud_index.hpp"
#include "tests/gen/front_fixtures.hpp"

using mhgp9::gen::Point3;
using mhgp9::tower::BallData;
using mhgp9::tower::P3;
using boost::multiprecision::cpp_int;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;

namespace {

std::vector<Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() % 12) throw std::invalid_argument("bad length");
  std::vector<Point3> pts(bytes.size() / 12);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside 18 bits");
      c[a] = v;
    }
    pts[i] = Point3{(mhgp9::gen::Coordinate)c[0], (mhgp9::gen::Coordinate)c[1], (mhgp9::gen::Coordinate)c[2]};
  }
  return pts;
}

inline std::int64_t power(const P3& x, const P3& a, const P3& b) {
  return (x.x - a.x) * (x.x - b.x) + (x.y - a.y) * (x.y - b.y) + (x.z - a.z) * (x.z - b.z);
}

std::uint64_t fnv_points(const std::vector<Point3>& pts) {
  std::uint64_t h = 1469598103934665603ull;
  for (const auto& p : pts)
    for (const std::int64_t v : {std::int64_t(p.x), std::int64_t(p.y), std::int64_t(p.z)})
      for (int b = 0; b < 8; ++b) { h ^= (std::uint64_t(v) >> (8 * b)) & 0xff; h *= 1099511628211ull; }
  return h;
}

cpp_int big(i128 v) {
  const bool neg = v < 0;
  const u128 u = neg ? (u128)(-(v + 1)) + 1 : (u128)v;
  cpp_int r = static_cast<std::uint64_t>(u >> 64);
  r <<= 64;
  r += static_cast<std::uint64_t>(u);
  return neg ? cpp_int(-r) : r;
}

// Niveau du catalogue (U192 lo, mid, hi / den) egal a |a-b|^2 / 4 ? Avec a, b sur la coquille, ce rayon fixe la
// sphere : la boule diametrale.
bool same_level(const BallData& ball, const P3& a, const P3& b) {
  cpp_int num = ball.level.num[2];
  num <<= 64; num += ball.level.num[1];
  num <<= 64; num += ball.level.num[0];
  const std::int64_t dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return num * 4 == cpp_int(dx * dx + dy * dy + dz * dz) * big(ball.level.den);
}

// Cle canonique independante (forme puissance a|z|^2 + b.z + c, a > 0, reduite par le pgcd) comparee a
// ball.key, champ par champ (contrelecture B v4 : la cle consommee par FULL, pas seulement le niveau).
bool same_key(const BallData& ball, const cpp_int (&f)[5]) {
  cpp_int g = abs(f[0]);
  for (int i = 1; i < 5; ++i) g = gcd(g, abs(f[i]));
  if (g == 0) return false;
  const cpp_int k[5] = {big(ball.key.a), big(ball.key.b[0]), big(ball.key.b[1]), big(ball.key.b[2]), big(ball.key.c)};
  for (int i = 0; i < 5; ++i) if (f[i] / g != k[i]) return false;
  return true;
}

// Tirage des sites : permutation de Fisher-Yates par splitmix64, graine publiee (distincte du juge q3).
std::vector<std::size_t> sample_sites(std::size_t n, std::size_t take, std::uint64_t seed) {
  std::vector<std::size_t> v(n);
  std::iota(v.begin(), v.end(), std::size_t{0});
  std::uint64_t s = seed;
  const auto next = [&]() {
    std::uint64_t z = (s += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  };
  for (std::size_t i = 0; i + 1 < n && i < take; ++i) std::swap(v[i], v[i + next() % (n - i)]);
  v.resize(std::min(take, n));
  return v;
}

struct Totals {
  std::uint64_t pairs = 0, expected = 0, found = 0, missing = 0, extended = 0, over_shell = 0, cross_fail = 0,
                extra = 0, by_p[10] = {};
};

// Garde d'index (contrelecture B du juge v7, 23 septembre 2026), AVANT tout echantillonnage : l'index ne porte
// pas de position dupliquee ; chaque rang geometrique u a un PointId < n (verifie avant d'indexer l'entree) ;
// les PointIds sont deux a deux distincts (bitset de n bits) et couvrent 0..n-1 ; la position du rang u est
// celle du point d'entree de meme PointId. Le multiensemble (coordonnees, PointId) de l'index egale donc celui
// de l'entree. Refus en code 2, marqueur INDEX_*, jamais de ligne de synthese. Mutants (--inject=index-*) :
// l'identifiant du rang 1 hors bornes ou egal a celui du rang 0, ou le dernier rang retire.
enum class IndexInject { kNone, kOutOfRange, kDuplicate, kMissing };
IndexInject g_index_inject = IndexInject::kNone;

int check_index(const std::string& label, const std::vector<Point3>& points, const mhgp9::tower::CloudIndex& ix) {
  const std::size_t n = points.size();
  if (ix.has_duplicate_positions()) {
    std::printf("%s INDEX_DUPLICATE_POSITIONS\n", label.c_str());
    return 2;
  }
  std::vector<std::uint64_t> ids(ix.upos.size());
  std::vector<P3> pos = ix.upos;
  for (std::size_t u = 0; u < ids.size(); ++u) ids[u] = ix.point_id(static_cast<std::int32_t>(u));
  if (ids.size() >= 2 && g_index_inject == IndexInject::kOutOfRange) ids[1] = n;
  if (ids.size() >= 2 && g_index_inject == IndexInject::kDuplicate) ids[1] = ids[0];
  if (!ids.empty() && g_index_inject == IndexInject::kMissing) { ids.pop_back(); pos.pop_back(); }
  std::vector<bool> seen(n, false);
  for (std::size_t u = 0; u < ids.size(); ++u) {
    if (ids[u] >= n) {
      std::printf("%s INDEX_ID_OUT_OF_RANGE u=%zu id=%llu n=%zu\n", label.c_str(), u, (unsigned long long)ids[u], n);
      return 2;
    }
    if (seen[ids[u]]) {
      std::printf("%s INDEX_ID_DUPLICATE u=%zu id=%llu\n", label.c_str(), u, (unsigned long long)ids[u]);
      return 2;
    }
    seen[ids[u]] = true;
  }
  if (ids.size() != n) {  // distincts et < n : couverture complete si et seulement si |ids| = n
    std::printf("%s INDEX_ID_MISSING covered=%zu n=%zu\n", label.c_str(), ids.size(), n);
    return 2;
  }
  for (std::size_t u = 0; u < n; ++u) {
    const auto& q = points[ids[u]];
    if (pos[u].x != q.x || pos[u].y != q.y || pos[u].z != q.z) {
      std::printf("%s INDEX_POSITION_MISMATCH u=%zu\n", label.c_str(), u);
      return 2;
    }
  }
  return 0;
}

int run(const std::string& label, const std::vector<Point3>& points, unsigned kmax, std::size_t sites,
        std::size_t workers, std::uint64_t seed, std::uint64_t min_top, bool corrupt_level, bool shell_dup,
        bool corrupt_key) {
  if (kmax < 2 || kmax > 10) { std::fprintf(stderr, "Kmax must be in 2..10\n"); return 2; }
  mhgp9::ChainOptions o;
  o.kmax = kmax;
  o.workers = workers;
  o.tower_static_threads = static_cast<int>(workers);
  o.keep_catalogue = true;
  o.run_tower = false;  // le juge porte sur le catalogue
  const auto r = mhgp9::run_tower_chain(points, o);
  if (r.status != mhgp9::ChainStatus::kComplete) {
    std::printf("%s kmax=%u chain_status=%s reason=%s\n", label.c_str(), kmax, mhgp9::chain_status_name(r.status),
                r.reason.c_str());
    return 2;
  }
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i),
                                        P3{points[i].x, points[i].y, points[i].z}};
  const auto ix = mhgp9::tower::build_cloud_index(input);
  const auto& pos = ix.upos;
  const std::size_t n = pos.size();
  // Coherence de l'index avec les points d'entree : bijection des PointIds puis positions (check_index).
  if (const int bad = check_index(label, points, ix)) return bad;
  if (n != points.size()) { std::printf("%s index_size_mismatch\n", label.c_str()); return 2; }
  std::vector<BallData> cat = r.catalogue_balls;
  if (corrupt_level) for (auto& ball : cat) ball.level.den += 1;  // mutant : niveaux faux
  if (shell_dup)  // mutant : dernier site de coquille remplace par le premier (doublon)
    for (auto& ball : cat)
      if (ball.n_shell >= 2) ball.shell_ids[ball.n_shell - 1] = ball.shell_ids[0];
  if (corrupt_key) for (auto& ball : cat) ball.key.c += 1;  // mutant : cle seule faussee
  std::vector<std::vector<std::uint32_t>> by_site(n);
  for (std::size_t i = 0; i < cat.size(); ++i)
    for (const auto s : cat[i].shell()) by_site[static_cast<std::size_t>(s)].push_back(static_cast<std::uint32_t>(i));
  const unsigned pmax = kmax - 1;

  const auto judge_site = [&](std::size_t a, std::int64_t exclude, Totals& t, std::unordered_set<std::uint32_t>& keys,
                              std::vector<std::string>& lines, bool& target_missed) {
    std::vector<std::int32_t> shell, inside;
    std::unordered_set<std::uint32_t> matched_here;
    char buf[160];
    for (std::size_t b = 0; b < n; ++b) {
      if (b == a) continue;
      ++t.pairs;
      shell.clear(); inside.clear();
      bool over = false;
      for (std::size_t x = 0; x < n; ++x) {
        const auto w = power(pos[x], pos[a], pos[b]);
        if (w < 0) {
          inside.push_back(static_cast<std::int32_t>(x));
          if (inside.size() > pmax) { over = true; break; }
        } else if (w == 0) {
          shell.push_back(static_cast<std::int32_t>(x));
        }
      }
      if (over) continue;
      const unsigned p = static_cast<unsigned>(inside.size());
      ++t.expected;
      ++t.by_p[p];
      if (shell.size() > 12) {  // la chaine refuse toute coquille > 12 : sur une chaine complete, omission
        ++t.over_shell; ++t.missing;
        std::snprintf(buf, sizeof buf, "MISSING_SHELL_OVER_12 a=%zu b=%zu p=%u shell=%zu", a, b, p, shell.size());
        lines.push_back(buf);
        continue;
      }
      if (shell.size() > 2) ++t.extended;
      std::vector<std::int32_t> shell_sorted(shell.begin(), shell.end());
      std::sort(shell_sorted.begin(), shell_sorted.end());
      std::int64_t hit = -1;
      bool cross_ok = false;
      for (const auto bi : by_site[a]) {
        if (static_cast<std::int64_t>(bi) == exclude) continue;
        const auto& ball = cat[bi];
        if (ball.n_shell != shell.size()) continue;
        // Ensemble exact des sites de coquille (listes triees egales : ni doublon, ni site substitue).
        std::vector<std::int32_t> theirs(ball.shell().begin(), ball.shell().end());
        std::sort(theirs.begin(), theirs.end());
        if (theirs != shell_sorted || !same_level(ball, pos[a], pos[b])) continue;
        // Forme de la boule diametrale : (x-a).(x-b) = |x|^2 - (a+b).x + a.b.
        const cpp_int form[5] = {cpp_int(1), cpp_int(-(pos[a].x + pos[b].x)), cpp_int(-(pos[a].y + pos[b].y)),
                                 cpp_int(-(pos[a].z + pos[b].z)),
                                 cpp_int(pos[a].x * pos[b].x + pos[a].y * pos[b].y + pos[a].z * pos[b].z)};
        if (!same_key(ball, form)) continue;
        hit = bi;
        // Recoupement : memes interieurs (ids distincts), arite 2 (la paire a, b est antipodale).
        std::vector<std::int32_t> ids(ball.interior().begin(), ball.interior().end());
        std::sort(ids.begin(), ids.end());
        std::vector<std::int32_t> mine(inside.begin(), inside.end());
        std::sort(mine.begin(), mine.end());
        cross_ok = ball.n_interior == p && ids == mine && std::adjacent_find(ids.begin(), ids.end()) == ids.end() &&
                   ball.arity == 2;
        break;
      }
      if (hit >= 0 && cross_ok) {
        ++t.found;
        keys.insert(static_cast<std::uint32_t>(hit));
        matched_here.insert(static_cast<std::uint32_t>(hit));
      } else if (hit >= 0) {
        ++t.cross_fail;
        std::snprintf(buf, sizeof buf, "CROSS_CHECK_FAILED a=%zu b=%zu p=%u shell=%zu", a, b, p, shell.size());
        lines.push_back(buf);
      } else {
        ++t.missing;
        if (exclude >= 0) {
          const auto sh = cat[static_cast<std::size_t>(exclude)].shell();
          target_missed = target_missed || std::find(sh.begin(), sh.end(), static_cast<std::int32_t>(b)) != sh.end();
        }
        std::snprintf(buf, sizeof buf, "MISSING a=%zu b=%zu p=%u shell=%zu", a, b, p, shell.size());
        lines.push_back(buf);
      }
    }
    // Sens inverse : boules regulieres a 2 sites, d'arite 2, p <= pmax, passant par a, non retrouvees.
    if (exclude < 0) {
      for (const auto bi : by_site[a]) {
        const auto& ball = cat[bi];
        if (ball.n_shell != 2 || ball.arity != 2 || ball.n_interior > pmax) continue;
        if (matched_here.count(bi)) continue;
        ++t.extra;
        std::snprintf(buf, sizeof buf, "EXTRA a=%zu ball=%u p=%u", a, bi, unsigned(ball.n_interior));
        lines.push_back(buf);
      }
    }
  };
  const auto sampled = sample_sites(n, sites, seed);
  Totals t;
  std::unordered_set<std::uint32_t> keys;
  std::vector<std::string> lines;
  bool dummy = false;
  for (const auto a : sampled) judge_site(a, -1, t, keys, lines, dummy);
  for (const auto& l : lines) std::printf("%s %s\n", label.c_str(), l.c_str());
  std::uint64_t top_keys = 0, top_population = 0;
  const auto top = [&](std::uint32_t k) { return cat[k].n_interior == pmax && cat[k].arity == 2 && cat[k].n_shell == 2; };
  for (const auto k : keys) if (top(k)) ++top_keys;
  for (const auto& ball : cat) if (ball.n_shell == 2 && ball.arity == 2 && ball.n_interior == pmax) ++top_population;
  // Mutant cible : une cle trouvee de rang p = Kmax-1, retiree, rejugee depuis un site tire de sa coquille ; un
  // manquant doit porter l'autre site de sa coquille.
  std::uint32_t target = UINT32_MAX;
  for (const auto k : keys) if (top(k) && (target == UINT32_MAX || k < target)) target = k;
  bool mutant_killed = false;
  if (target != UINT32_MAX) {
    for (const auto a : sampled) {
      const auto sh = cat[target].shell();
      if (std::find(sh.begin(), sh.end(), static_cast<std::int32_t>(a)) == sh.end()) continue;
      Totals tm; std::unordered_set<std::uint32_t> km; std::vector<std::string> lm;
      judge_site(a, target, tm, km, lm, mutant_killed);
      if (mutant_killed) break;
    }
  }
  std::printf("%s n=%zu kmax=%u balls=%zu input_fnv=%016llx seed=%016llx sampled_sites=%zu pairs=%llu "
              "incidences=%llu found=%llu missing=%llu cross_fail=%llu extra=%llu unique_keys=%zu top_keys=%llu "
              "top_population=%llu extended=%llu shell_over_12=%llu mutant_killed=%d",
              label.c_str(), n, kmax, cat.size(), (unsigned long long)fnv_points(points), (unsigned long long)seed,
              sampled.size(), (unsigned long long)t.pairs, (unsigned long long)t.expected, (unsigned long long)t.found,
              (unsigned long long)t.missing, (unsigned long long)t.cross_fail, (unsigned long long)t.extra, keys.size(),
              (unsigned long long)top_keys, (unsigned long long)top_population, (unsigned long long)t.extended,
              (unsigned long long)t.over_shell, mutant_killed ? 1 : 0);
  for (unsigned p = 0; p <= pmax && p < 10; ++p) std::printf(" p%u=%llu", p, (unsigned long long)t.by_p[p]);
  std::printf("\n");
  if (t.missing || t.cross_fail || t.extra) return 1;
  if (!mutant_killed || top_keys < min_top) return 3;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::uint64_t seed = 0xc3a5c85c97cb3127ull, min_top = 1;
    bool corrupt_level = false, shell_dup = false, corrupt_key = false;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
      const std::string a = argv[i];
      if (a == "--inject=level") corrupt_level = true;
      else if (a == "--inject=shell-dup") shell_dup = true;
      else if (a == "--inject=key") corrupt_key = true;
      else if (a == "--inject=index-out-of-range") g_index_inject = IndexInject::kOutOfRange;
      else if (a == "--inject=index-duplicate") g_index_inject = IndexInject::kDuplicate;
      else if (a == "--inject=index-missing") g_index_inject = IndexInject::kMissing;
      else if (a.rfind("--seed=", 0) == 0) seed = std::stoull(a.substr(7), nullptr, 0);
      else if (a.rfind("--min-top=", 0) == 0) min_top = std::stoull(a.substr(10));
      else if (a.rfind("--", 0) == 0) { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
      else args.push_back(a);
    }
    if (args.size() == 6 && args[0] == "family") {
      const auto fx = mhgp9::gen::bench::make_front_fixture(std::stoul(args[2]), args[1], 3);
      return run(args[1] + "_" + args[2], fx.points, std::stoul(args[3]), std::stoul(args[4]), std::stoul(args[5]),
                 seed, min_top, corrupt_level, shell_dup, corrupt_key);
    }
    if (args.size() == 5 && args[0] == "file") {
      const auto slash = args[1].find_last_of('/');
      return run(args[1].substr(slash == std::string::npos ? 0 : slash + 1), read_u32le(args[1]), std::stoul(args[2]),
                 std::stoul(args[3]), std::stoul(args[4]), seed, min_top, corrupt_level, shell_dup, corrupt_key);
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 2;
  }
  std::fprintf(stderr, "usage: q2_sample_judge family <name> <n> <Kmax> <sites> <workers> [--seed=S] [--min-top=N] | "
                       "file <cut.u32le> <Kmax> <sites> <workers> [--seed=S] [--min-top=N]\n");
  return 2;
}
