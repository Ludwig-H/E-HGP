// Auto-test sur l'hote de la verification d'arene du banc CUDA (dump::verify_arena) : la forme j3 (warp simule) ecrit
// ses emissions dans une arene comme l'appareil (enregistrement de 16 octets portant sa feuille, populations par rang
// de bit), les feuilles etant jouees dans un ordre melange et leurs enregistrements entrelaces ; la verification doit
// rendre l'identite, puis detecter six alterations (code 4 de la v11 : mutant tue).
//
// Usage : mhgp12_arena_selftest <vidage.bin> [feuilles=20000]   Codes : 0 conforme, 1 ecart, 2 refus, 3 mutant vivant.
// Le vidage passe l'admission de dump::read (CST-0215) ; la sortie cite son empreinte FNV-1a (dump_fnv1a).
#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "mhgp12/leaf/compare.hpp"
#include "mhgp12/leaf/leaf_j3.hpp"

namespace {

namespace dump = mhgp12::dump;
namespace leaf = mhgp12::leaf;
using dump::u32;
using dump::u64;
using dump::u8;

struct HostArenaSink {
  std::vector<leaf::ArenaRecord>* records;
  std::vector<u8>* population;
  u32 leaf_id;
  void emit(const leaf::Emission& e) {
    leaf::ArenaRecord r{};
    r.leaf = leaf_id;
    for (int k = 0; k < 4; ++k) r.support[k] = e.support[k];
    r.p = e.p;
    r.m = e.m;
    r.qmin = e.qmin;
    r.q = e.q;
    r.population_at = static_cast<u32>(population->size());
    // Meme placement que ArenaSink : rang de bit parmi les bits inferieurs.
    const u64 base = population->size();
    population->resize(base + e.p + e.m);
    for (u32 s = 0; s < 32; ++s) {
      if ((e.interior >> s) & 1u) (*population)[base + __builtin_popcount(e.interior & ((1u << s) - 1u))] = u8(s);
      if ((e.shell >> s) & 1u) (*population)[base + e.p + __builtin_popcount(e.shell & ((1u << s) - 1u))] = u8(s);
    }
    records->push_back(r);
  }
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) return 2;
  dump::LeafDump d;
  std::string error;
  if (!dump::read(argv[1], d, error)) {
    std::cerr << error << '\n';
    return 2;
  }
  u64 limit = 20000;
  try {
    if (argc == 3) limit = std::stoull(argv[2]);
  } catch (const std::exception&) {
    return 2;
  }
  if (limit == 0) return 2;  // aucun mutant ne serait juge : refus, jamais un vert vide
  // Vidage restreint aux premieres feuilles (meme reference).
  const u64 n = std::min<u64>(limit, d.header.n_leaves);
  d.header.n_leaves = n;
  d.jobs.resize(n);
  d.counts.resize(n * dump::kCounters);
  d.status.resize(n);
  d.record_begin.resize(n + 1);
  d.population_begin.resize(n + 1);
  std::vector<u32> order(n);
  for (u64 j = 0; j < n; ++j) order[j] = static_cast<u32>(j);
  std::mt19937_64 rng(20261007);
  std::shuffle(order.begin(), order.end(), rng);
  // Deux arenes partielles entrelacees (deux « warps » qui alternent), puis concatenation melangee par blocs.
  std::vector<leaf::ArenaRecord> records;
  std::vector<u8> population;
  std::vector<u8> status(n, 0);
  std::vector<u32> counts(n * dump::kCounters, 0);
  auto shared = std::make_unique<leaf::j3::SharedJ3>();
  for (u32 j : order) {
    leaf::Input in;
    in.x = d.x.data();
    in.y = d.y.data();
    in.z = d.z.data();
    in.sites = d.leaf_sites(j);
    in.m = d.jobs[j].m;
    for (int a = 0; a < 3; ++a) {
      in.lo[a] = d.jobs[j].lo[a];
      in.hi[a] = d.jobs[j].hi[a];
    }
    in.kmax = static_cast<int>(d.header.kmax);
    in.cache = (d.header.flags & dump::kFlagCache) != 0;
    leaf::Counts c;
    HostArenaSink sink{&records, &population, j};
    status[j] = static_cast<u8>(leaf::j3::run_leaf(in, *shared, c, sink));
    for (u32 f = 0; f < dump::kCounters; ++f) counts[u64(j) * dump::kCounters + f] = c.c[f];
  }
  // Entrelacement des enregistrements de feuilles differentes (l'ordre interne d'une feuille est libre aussi).
  std::shuffle(records.begin(), records.end(), rng);
  const auto base = dump::verify_arena(d, status, counts, records, population);
  std::cout << "{\"dump_fnv1a\":\"" << dump::digest_hex(d.digest) << "\",\"leaves\":" << n
            << ",\"records\":" << records.size() << ",\"population\":" << population.size()
            << ",\"identity\":" << (base.identity() ? "true" : "false") << ",\"unresolved\":" << base.unresolved;
  if (!base.identity()) {
    std::cout << "}\n";
    return 1;
  }
  if (records.size() < 2 || population.empty() || n < 2) {  // mutants sans objet : refus, jamais un vert vide
    std::cout << ",\"mutants\":\"sans_objet\"}\n";
    return 2;
  }
  int alive = 0;
  auto mutant = [&](const char* name, auto&& change) {
    auto r = records;
    auto p = population;
    auto c = counts;
    change(r, p, c);
    const auto check = dump::verify_arena(d, status, c, r, p);
    const bool killed = !check.identity();
    alive += killed ? 0 : 1;
    std::cout << ",\"" << name << "\":" << (killed ? "\"tue\"" : "\"VIVANT\"");
  };
  mutant("population_alteree", [](auto&, auto& p, auto&) { p[p.size() / 2] ^= 1u; });
  mutant("support_altere", [](auto& r, auto&, auto&) { r[r.size() / 3].support[0] ^= 1u; });
  mutant("boule_perdue", [](auto& r, auto&, auto&) { r.pop_back(); });
  mutant("boule_doublee", [](auto& r, auto&, auto&) { r.push_back(r.front()); });
  mutant("feuille_echangee", [n](auto& r, auto&, auto&) { r[0].leaf = (r[0].leaf + 1) % static_cast<u32>(n); });
  mutant("compteur_altere", [](auto&, auto&, auto& c) { c[dump::kCounters + 1] += 1; });
  std::cout << ",\"mutants_vivants\":" << alive << "}\n";
  return alive == 0 ? 0 : 3;
}
