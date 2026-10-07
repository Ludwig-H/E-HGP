// Identite sur l'hote des deux formes de la feuille data-parallele (MES-M2, hors produit).
//
// Le meme texte que l'appareil (include/mhgp12/leaf/*.hpp), le warp etant simule de facon deterministe (voies 0..31
// l'une apres l'autre a chaque super-pas). Pour chaque feuille d'un vidage MHGP12LF : statut, quinze compteurs logiques
// et ENSEMBLE des emissions (S*, p, m, qmin, populations I puis U) compares a la sortie de reference de leaf.cpp
// (v11). L'ordre des emissions dans une feuille est libre (le catalogue est trie ensuite par (niveau, S*)) : les deux
// listes sont triees par S* avant comparaison. Les feuilles non resolues sont comptees (l'hote les rejouerait par
// leaf.cpp avant admission) et exclues de l'identite.
//
// Usage : mhgp12_leaf_identity [--forms j3,coherent] [--threads N] [--leaves N] [--json] <vidage.bin> [...]
// Codes : 0 identite sur toutes les feuilles resolues, 1 ecart, 2 refus (arguments, vidage illisible).
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "mhgp12/leaf/dump_format.hpp"
#include "mhgp12/leaf/leaf_coherent.hpp"
#include "mhgp12/leaf/leaf_j3.hpp"

namespace {

namespace dump = mhgp12::dump;
namespace leaf = mhgp12::leaf;
using dump::u32;
using dump::u64;
using dump::u8;

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count());
}

struct VectorSink {
  std::vector<dump::Record> records;
  std::vector<u8> population;
  std::vector<u64> population_at;  // debut de la population de chaque enregistrement
  void clear() {
    records.clear();
    population.clear();
    population_at.clear();
  }
  void emit(const leaf::Emission& e) {
    dump::Record r{};
    for (int k = 0; k < 4; ++k) r.support[k] = e.support[k];
    r.p = e.p;
    r.m = e.m;
    r.qmin = e.qmin;
    r.pad = 0;
    records.push_back(r);
    population_at.push_back(population.size());
    for (u32 rest = e.interior; rest != 0; rest &= rest - 1) population.push_back(static_cast<u8>(__builtin_ctz(rest)));
    for (u32 rest = e.shell; rest != 0; rest &= rest - 1) population.push_back(static_cast<u8>(__builtin_ctz(rest)));
  }
};

// Emission comparable : enregistrement et population.
struct Item {
  dump::Record record;
  std::vector<u8> population;
  bool operator<(const Item& o) const { return std::memcmp(record.support, o.record.support, 4) < 0; }
  bool operator==(const Item& o) const {
    return std::memcmp(&record, &o.record, sizeof(record)) == 0 && population == o.population;
  }
};

enum class Form { j3, coherent };
const char* name_of(Form f) { return f == Form::j3 ? "j3" : "coherent"; }

struct Tally {
  u64 leaves = 0, resolved = 0, unresolved = 0, mismatched_counts = 0, mismatched_emissions = 0;
  u64 emissions = 0, population = 0, ns = 0;
  std::array<u64, dump::kCounters> totals{};
  leaf::Diag diag;
  u64 first_bad = ~u64{0};
  void add(const Tally& t) {
    leaves += t.leaves;
    resolved += t.resolved;
    unresolved += t.unresolved;
    mismatched_counts += t.mismatched_counts;
    mismatched_emissions += t.mismatched_emissions;
    emissions += t.emissions;
    population += t.population;
    ns = std::max(ns, t.ns);
    for (u32 f = 0; f < dump::kCounters; ++f) totals[f] += t.totals[f];
    diag.leaves += t.diag.leaves;
    for (int q = 0; q < 5; ++q) {
      diag.rounds[q] += t.diag.rounds[q];
      diag.items[q] += t.diag.items[q];
    }
    diag.censuses += t.diag.censuses;
    diag.chunks += t.diag.chunks;
    diag.steps += t.diag.steps;
    diag.active += t.diag.active;
    first_bad = std::min(first_bad, t.first_bad);
  }
};

void check_range(const dump::LeafDump& d, Form form, u64 begin, u64 end, Tally& t, bool with_diag) {
  auto shared = std::make_unique<leaf::j3::SharedJ3>();
  VectorSink sink;
  std::vector<Item> mine, ref;
  const u64 start = now_ns();
  for (u64 j = begin; j < end; ++j) {
    const auto& job = d.jobs[j];
    leaf::Input in;
    in.x = d.x.data();
    in.y = d.y.data();
    in.z = d.z.data();
    in.sites = d.leaf_sites(j);
    in.m = job.m;
    for (int a = 0; a < 3; ++a) {
      in.lo[a] = job.lo[a];
      in.hi[a] = job.hi[a];
    }
    in.kmax = static_cast<int>(d.header.kmax);
    in.cache = (d.header.flags & dump::kFlagCache) != 0;
    leaf::Counts counts;
    sink.clear();
    leaf::Diag* diag = with_diag ? &t.diag : nullptr;
    const u32 status = form == Form::j3 ? leaf::j3::run_leaf(in, *shared, counts, sink, diag)
                                        : leaf::coherent::run_leaf(in, *shared, counts, sink, diag);
    ++t.leaves;
    if (status != leaf::kStatusOk) {
      ++t.unresolved;
      continue;
    }
    ++t.resolved;
    const u32* expected = d.leaf_counts(j);
    bool counts_ok = true;
    for (u32 f = 0; f < dump::kCounters; ++f) {
      t.totals[f] += counts.c[f];
      counts_ok = counts_ok && counts.c[f] == expected[f];
    }
    if (!counts_ok) {
      ++t.mismatched_counts;
      t.first_bad = std::min(t.first_bad, j);
    }
    mine.clear();
    ref.clear();
    for (u64 b = 0; b < sink.records.size(); ++b) {
      const u64 at = sink.population_at[b], n = u64(sink.records[b].p) + sink.records[b].m;
      mine.push_back(Item{sink.records[b], std::vector<u8>(sink.population.begin() + static_cast<long>(at),
                                                           sink.population.begin() + static_cast<long>(at + n))});
    }
    u64 at = d.population_begin[j];
    for (u64 b = d.record_begin[j]; b < d.record_begin[j + 1]; ++b) {
      const auto& r = d.records[b];
      const u64 n = u64(r.p) + r.m;
      ref.push_back(Item{r, std::vector<u8>(d.population.begin() + static_cast<long>(at),
                                            d.population.begin() + static_cast<long>(at + n))});
      at += n;
    }
    t.emissions += mine.size();
    t.population += sink.population.size();
    std::sort(mine.begin(), mine.end());
    std::sort(ref.begin(), ref.end());
    if (!(mine == ref)) {
      ++t.mismatched_emissions;
      t.first_bad = std::min(t.first_bad, j);
    }
  }
  t.ns = now_ns() - start;
}

int run(int argc, char** argv) {
  std::vector<Form> forms = {Form::j3, Form::coherent};
  u32 threads = 1;
  u64 limit = ~u64{0};
  bool diag = true;
  std::vector<std::string> paths;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--forms" && i + 1 < argc) {
      forms.clear();
      std::stringstream ss(argv[++i]);
      std::string f;
      while (std::getline(ss, f, ',')) {
        if (f == "j3") forms.push_back(Form::j3);
        else if (f == "coherent") forms.push_back(Form::coherent);
        else return 2;
      }
    } else if (a == "--threads" && i + 1 < argc) {
      threads = static_cast<u32>(std::stoul(argv[++i]));
    } else if (a == "--leaves" && i + 1 < argc) {
      limit = std::stoull(argv[++i]);
    } else if (a == "--no-diag") {
      diag = false;
    } else if (!a.empty() && a[0] == '-') {
      return 2;
    } else {
      paths.push_back(a);
    }
  }
  if (paths.empty() || threads == 0 || threads > 64) return 2;
  bool all_ok = true;
  for (const auto& path : paths) {
    dump::LeafDump d;
    std::string error;
    if (!dump::read(path, d, error)) {
      std::cerr << error << '\n';
      return 2;
    }
    const u64 n = std::min<u64>(d.header.n_leaves, limit);
    std::array<u64, dump::kCounters> reference{};
    for (u64 j = 0; j < n; ++j)
      for (u32 f = 0; f < dump::kCounters; ++f) reference[f] += d.leaf_counts(j)[f];
    for (Form form : forms) {
      std::vector<Tally> parts(threads);
      std::vector<std::thread> pool;
      const u64 step = (n + threads - 1) / threads;
      const u64 start = now_ns();
      for (u32 w = 0; w < threads; ++w) {
        const u64 b = std::min<u64>(n, w * step), e = std::min<u64>(n, b + step);
        pool.emplace_back([&, w, b, e]() { check_range(d, form, b, e, parts[w], diag); });
      }
      for (auto& th : pool) th.join();
      const u64 wall = now_ns() - start;
      Tally t;
      for (const auto& p : parts) t.add(p);
      const bool ok = t.mismatched_counts == 0 && t.mismatched_emissions == 0 && t.leaves == n;
      all_ok = all_ok && ok;
      std::cout << "{\"dump\":\"" << path << "\",\"form\":\"" << name_of(form) << "\",\"kmax\":" << d.header.kmax
                << ",\"leaf_size\":" << d.header.leaf_size << ",\"leaves\":" << t.leaves
                << ",\"resolved\":" << t.resolved << ",\"unresolved\":" << t.unresolved
                << ",\"mismatched_counts\":" << t.mismatched_counts
                << ",\"mismatched_emissions\":" << t.mismatched_emissions << ",\"emissions\":" << t.emissions
                << ",\"population\":" << t.population << ",\"identity\":" << (ok ? "true" : "false")
                << ",\"threads\":" << threads << ",\"wall_ns\":" << wall;
      if (t.first_bad != ~u64{0}) std::cout << ",\"first_bad_leaf\":" << t.first_bad;
      std::cout << ",\"counts\":{";
      for (u32 f = 0; f < dump::kCounters; ++f)
        std::cout << (f ? "," : "") << '"' << dump::kCounterNames[f] << "\":[" << t.totals[f] << ',' << reference[f]
                  << ']';
      std::cout << "}";
      if (diag) {
        std::cout << ",\"diag\":{\"rounds\":[" << t.diag.rounds[2] << ',' << t.diag.rounds[3] << ','
                  << t.diag.rounds[4] << "],\"items\":[" << t.diag.items[2] << ',' << t.diag.items[3] << ','
                  << t.diag.items[4] << "],\"censuses\":" << t.diag.censuses << ",\"chunks\":" << t.diag.chunks
                  << ",\"steps\":" << t.diag.steps << ",\"active\":" << t.diag.active << '}';
      }
      std::cout << "}\n" << std::flush;
    }
  }
  return all_ok ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) { return run(argc, argv); }
