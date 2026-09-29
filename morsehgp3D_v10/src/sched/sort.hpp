// Tri parallele par echantillonnage regulier (PSRS) sur le Pool unique.
//
// Contrat : `less` est un ordre strict ; pour un ordre TOTAL (aucune paire d'elements equivalents), le resultat
// est identique a std::sort, quel que soit le nombre de fils. Etapes : blocs tries en parallele, P echantillons
// reguliers par bloc, P - 1 separateurs, decoupage de chaque bloc par lower_bound, puis chaque seau (pieces de
// tous les blocs, concatenees a une position fixee par des prefixes) est trie en parallele.
#pragma once

#include <algorithm>
#include <vector>

#include "sched/pool.hpp"

namespace mhgp10::sched {

template <class T, class Less>
void parallel_sort(Pool& pool, std::vector<T>& v, Less less) {
  const u64 n = v.size();
  const u64 P = pool.size();
  if (P <= 1 || n < 8192 || in_parallel_region()) {
    std::sort(v.begin(), v.end(), less);
    return;
  }
  std::vector<u64> bound(P + 1);
  for (u64 i = 0; i <= P; ++i) bound[i] = n * i / P;
  pool.parallel_for(P, 1, [&](u64 b, u64 e, unsigned) {
    for (u64 i = b; i < e; ++i) std::sort(v.begin() + bound[i], v.begin() + bound[i + 1], less);
  });
  std::vector<T> sample;
  sample.reserve(P * P);
  for (u64 i = 0; i < P; ++i)
    for (u64 s = 0; s < P; ++s) sample.push_back(v[bound[i] + (bound[i + 1] - bound[i]) * s / P]);
  std::sort(sample.begin(), sample.end(), less);
  std::vector<T> split;
  for (u64 j = 1; j < P; ++j) split.push_back(sample[j * P]);
  // cut[i * (P + 1) + j] : debut du seau j dans le bloc i
  std::vector<u64> cut(P * (P + 1));
  pool.parallel_for(P, 1, [&](u64 b, u64 e, unsigned) {
    for (u64 i = b; i < e; ++i) {
      cut[i * (P + 1)] = bound[i];
      for (u64 j = 1; j < P; ++j)
        cut[i * (P + 1) + j] = u64(std::lower_bound(v.begin() + cut[i * (P + 1) + j - 1], v.begin() + bound[i + 1],
                                                    split[j - 1], less) - v.begin());
      cut[i * (P + 1) + P] = bound[i + 1];
    }
  });
  std::vector<u64> off(P + 1, 0);
  for (u64 j = 0; j < P; ++j) {
    u64 size = 0;
    for (u64 i = 0; i < P; ++i) size += cut[i * (P + 1) + j + 1] - cut[i * (P + 1) + j];
    off[j + 1] = off[j] + size;
  }
  std::vector<T> out(n);
  pool.parallel_for(P, 1, [&](u64 b, u64 e, unsigned) {
    for (u64 j = b; j < e; ++j) {
      u64 at = off[j];
      for (u64 i = 0; i < P; ++i)
        at = u64(std::copy(v.begin() + cut[i * (P + 1) + j], v.begin() + cut[i * (P + 1) + j + 1], out.begin() + at) -
                 out.begin());
      std::sort(out.begin() + off[j], out.begin() + off[j + 1], less);
    }
  });
  v.swap(out);
}

}  // namespace mhgp10::sched
