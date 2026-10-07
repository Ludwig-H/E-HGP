// Comparaison d'une feuille a la reference leaf.cpp d'un vidage MHGP12LF (hote seulement) : ENSEMBLE des emissions
// (enregistrement et population), tri par S* en rangs locaux. Partage par l'outil d'identite et le banc CUDA.
#pragma once

#include <algorithm>
#include <cstring>
#include <vector>

#include "mhgp12/leaf/dump_format.hpp"

namespace mhgp12::dump {

struct Item {
  Record record;
  std::vector<u8> population;
  bool operator<(const Item& o) const { return std::memcmp(record.support, o.record.support, 4) < 0; }
  bool operator==(const Item& o) const {
    return std::memcmp(&record, &o.record, sizeof(record)) == 0 && population == o.population;
  }
};

// Emissions de reference de la feuille j, triees par S*.
inline void reference_items(const LeafDump& d, u64 j, std::vector<Item>& out) {
  out.clear();
  u64 at = d.population_begin[j];
  for (u64 b = d.record_begin[j]; b < d.record_begin[j + 1]; ++b) {
    const Record& r = d.records[b];
    const u64 n = u64(r.p) + r.m;
    out.push_back(Item{r, std::vector<u8>(d.population.begin() + static_cast<long>(at),
                                          d.population.begin() + static_cast<long>(at + n))});
    at += n;
  }
  std::sort(out.begin(), out.end());
}

}  // namespace mhgp12::dump

#include "mhgp12/leaf/arena.hpp"

namespace mhgp12::dump {

// Bilan de la verification d'une arene contre le vidage.
struct ArenaCheck {
  u64 unresolved = 0, mismatched_counts = 0, mismatched_emissions = 0, foreign_records = 0;
  std::vector<u64> totals = std::vector<u64>(kCounters, 0);
  bool identity() const { return mismatched_counts == 0 && mismatched_emissions == 0 && foreign_records == 0; }
};

// status[j] (0 resolue), counts[j * kCounters + f], enregistrements d'arene dans un ordre quelconque (l'ordre d'arrivee
// est conserve a l'interieur d'une feuille mais n'est pas exige), populations en rangs locaux.
inline ArenaCheck verify_arena(const LeafDump& d, const std::vector<u8>& status, const std::vector<u32>& counts,
                               const std::vector<leaf::ArenaRecord>& records, const std::vector<u8>& population) {
  ArenaCheck out;
  const u64 n = d.header.n_leaves;
  std::vector<u64> first(n + 1, 0);
  for (const auto& r : records) {
    if (r.leaf < n) ++first[r.leaf + 1];
    else ++out.foreign_records;
  }
  for (u64 j = 0; j < n; ++j) first[j + 1] += first[j];
  std::vector<u64> placed(first.begin(), first.end() - 1), by_leaf(records.size());
  for (u64 b = 0; b < records.size(); ++b)
    if (records[b].leaf < n) by_leaf[placed[records[b].leaf]++] = b;
  std::vector<Item> mine, ref;
  for (u64 j = 0; j < n; ++j) {
    if (status[j] != 0) {
      ++out.unresolved;
      continue;  // les enregistrements d'une feuille non resolue sont ignores (l'hote la rejoue)
    }
    const u32* c = d.leaf_counts(j);
    bool same = true;
    for (u32 f = 0; f < kCounters; ++f) {
      out.totals[f] += counts[j * kCounters + f];
      same = same && counts[j * kCounters + f] == c[f];
    }
    if (!same) ++out.mismatched_counts;
    mine.clear();
    bool ok = true;
    for (u64 t = first[j]; t < first[j + 1]; ++t) {
      const auto& a = records[by_leaf[t]];
      Record rec{};
      for (int k = 0; k < 4; ++k) rec.support[k] = a.support[k];
      rec.p = a.p;
      rec.m = a.m;
      rec.qmin = a.qmin;
      rec.pad = 0;
      const u64 need = u64(a.p) + a.m;
      if (u64(a.population_at) + need > population.size()) {
        ok = false;
        break;
      }
      const auto from = population.begin() + static_cast<long>(a.population_at);
      mine.push_back(Item{rec, std::vector<u8>(from, from + static_cast<long>(need))});
    }
    std::sort(mine.begin(), mine.end());
    reference_items(d, j, ref);
    if (!ok || !(mine == ref)) ++out.mismatched_emissions;
  }
  return out;
}

}  // namespace mhgp12::dump
