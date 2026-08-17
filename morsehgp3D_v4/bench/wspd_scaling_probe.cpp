// MorseHGP3D v4 — PROBE D'ECHELLE WSPD, counter-only.
//
// Mesure la taille du front WSPD aux tailles d'interet du plan de test
// (n = 8000, 16000, 32000, voire 64000) sous s = 6, 8, 10, sur les familles
// declarees. Aucune geometrie scientifique n'est decidee ici : ce probe ne
// compte que des rectangles et juge ses propres invariants.
//
// PORTES (codes de sortie exacts, jamais de crash par signal) :
//   0 = campagne conforme ;
//   2 = refus avant calcul (arguments, capacite de famille, plancher) ;
//   3 = invariant viole (ledger de masse, permutation) ;
//   4 = mutant tue (l'injection demandee est detectee par la porte).
//
// INVARIANTS JUGES :
//   L1  ledger de masse : somme |A||B| ponderee = C(n,2) - somme_u C(mult_u,2),
//       exactement, en 128 bits ;
//   L2  plancher de non-vacuite : au moins `--min-rect` rectangles ;
//   L3  (--check-permutation) le multiset des rectangles, en coordonnees de
//       plages de positions uniques, est invariant par permutation de l'ordre
//       d'entree des points.
//
// MUTANTS :
//   --inject=drop-rect     un rectangle terminal est perdu -> L1 le tue ;
//   --inject=cap-terminal  le bug v3 du 16 aout 2026 : le critere terminal
//       exige aussi |A||B| <= 512, ce qui force un pavage quadratique ; la
//       porte --discriminate-cap le tue par comparaison appariee.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/tree/radix_tree.hpp"
#include "../src/wspd/wavefront.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  int n = 8000;
  int coord = 0;  // 0 : emprise par defaut de la famille
  long long seed = 3;
  i64 s = 8;
  u64 min_rect = 1;
  bool check_permutation = false;
  bool discriminate_cap = false;
  std::string inject;
};

bool parse_family(const char* name, CloudFamily* out) {
  const CloudFamily all[] = {CloudFamily::kUniform,
                             CloudFamily::kTerrain,
                             CloudFamily::kScanlineSinglePass,
                             CloudFamily::kScanlineOverlapMultiecho,
                             CloudFamily::kEightClusters,
                             CloudFamily::kTwoLines,
                             CloudFamily::kCollinearSeven};
  for (const CloudFamily f : all)
    if (std::strcmp(name, cloud_family_name(f)) == 0) {
      *out = f;
      return true;
    }
  return false;
}

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.family_ok = parse_family(v, &a.family);
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--min-rect=")) a.min_rect = (u64)std::atoll(v);
    else if (const char* v = val("--inject=")) a.inject = v;
    else if (arg == "--check-permutation") a.check_permutation = true;
    else if (arg == "--discriminate-cap") a.discriminate_cap = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

// Variante instrumentee du front : reproduit wspd_wavefront en ajoutant les
// deux injections. Le chemin sain reste `wspd_wavefront` (src/), la copie
// locale n'existe que pour porter les mutants sans les compiler dans la lib.
struct FrontResult {
  WspdStats st;
  u64 digest = 0;
};

u64 mix64(u64 x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdull;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ull;
  x ^= x >> 33;
  return x;
}

FrontResult run_front(const CloudIndex& ix, i64 s, const std::string& inject) {
  FrontResult res;
  const bool cap_terminal = inject == "cap-terminal";
  bool drop_pending = inject == "drop-rect";
  const auto range_of = [&](NodeRef v, u64* lo, u64* hi) {
    if (v >= 0) {
      *lo = (u64)ix.nodes[(size_t)v].first;
      *hi = (u64)ix.nodes[(size_t)v].last;
    } else {
      *lo = *hi = (u64)(-1 - v);
    }
  };
  if (ix.nodes.empty()) return res;
  std::vector<WspdRect> wave, next;
  for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
  while (!wave.empty()) {
    ++res.st.levels;
    res.st.wave_peak = std::max(res.st.wave_peak, (u64)wave.size());
    next.clear();
    for (const WspdRect& r : wave) {
      i64 ba[3], bb[3];
      const auto va = detail::node_view(ix, r.a, ba);
      const auto vb = detail::node_view(ix, r.b, bb);
      const u64 wa = detail::node_weight(ix, r.a);
      const u64 wb = detail::node_weight(ix, r.b);
      bool terminal = detail::separated(va, vb, s, 1);
      if (terminal && cap_terminal && wa * wb > 512 && (r.a >= 0 || r.b >= 0))
        terminal = false;  // LE BUG v3 : le cap de masse fuit dans le critere
      if (terminal) {
        if (drop_pending) {
          drop_pending = false;  // un rectangle perdu, une seule fois
          continue;
        }
        ++res.st.rectangles;
        res.st.pair_mass += (u128)wa * wb;
        u64 alo, ahi, blo, bhi;
        range_of(r.a, &alo, &ahi);
        range_of(r.b, &blo, &bhi);
        if (blo < alo || (blo == alo && bhi < ahi)) {
          std::swap(alo, blo);
          std::swap(ahi, bhi);
        }
        // Digest independant de l'ordre d'emission : somme de hachages.
        res.digest += mix64(alo * 0x9e3779b97f4a7c15ull + ahi) ^
                      mix64(blo * 0xc2b2ae3d27d4eb4full + bhi);
        continue;
      }
      const i64 w2a = detail::box_w2(va);
      const i64 w2b = detail::box_w2(vb);
      const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
      next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
    }
    wave.swap(next);
  }
  return res;
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 2 || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  const int coord =
      a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> pts = make_family_cloud(a.family, a.n, coord, a.seed);
  // Capacite de famille : les contre-familles rendent leur propre cardinal ;
  // toute autre famille doit servir n points, sinon la campagne est refusee.
  const bool counter_family =
      a.family == CloudFamily::kTwoLines || a.family == CloudFamily::kCollinearSeven;
  if (!counter_family && (int)pts.size() != a.n) {
    std::fprintf(stderr, "REFUS : famille %s a rendu %zu points pour n=%d\n",
                 cloud_family_name(a.family), pts.size(), a.n);
    return 2;
  }

  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  const auto t1 = std::chrono::steady_clock::now();

  if (a.discriminate_cap) {
    // Porte discriminante du bug historique : le cap dans le critere terminal
    // doit produire STRICTEMENT plus de rectangles que le front sain sur au
    // moins une configuration sensible (il force ~C(n,2)/cap^2 tuiles).
    const FrontResult sain = run_front(ix, a.s, "");
    const FrontResult mutant = run_front(ix, a.s, "cap-terminal");
    if (mutant.st.pair_mass != sain.st.pair_mass) {
      std::fprintf(stderr, "PORTE : le mutant a change la masse couverte\n");
      return 3;  // le mutant v3 pavait toujours exactement C(n,2)
    }
    if (mutant.st.rectangles > sain.st.rectangles) {
      std::printf("MUTANT TUE : cap-terminal %llu rectangles contre %llu\n",
                  (unsigned long long)mutant.st.rectangles,
                  (unsigned long long)sain.st.rectangles);
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : le mutant n'est pas discrimine\n");
    return 3;
  }

  const FrontResult front = run_front(ix, a.s, a.inject);
  const auto t2 = std::chrono::steady_clock::now();

  const u128 expected = expected_pair_mass(ix);
  const bool mass_ok = front.st.pair_mass == expected;
  const bool floor_ok = front.st.rectangles >= a.min_rect;

  bool perm_ok = true;
  if (a.check_permutation) {
    std::vector<u32> perm(pts.size());
    std::iota(perm.begin(), perm.end(), 0u);
    std::mt19937 rng(1729u);
    std::shuffle(perm.begin(), perm.end(), rng);
    std::vector<P3> shuffled(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) shuffled[perm[i]] = pts[i];
    const CloudIndex ix2 = build_cloud_index(shuffled);
    const FrontResult front2 = run_front(ix2, a.s, a.inject);
    perm_ok = front2.digest == front.digest &&
              front2.st.rectangles == front.st.rectangles;
  }

  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  std::printf(
      "famille=%s n=%d coord=%d s=%lld seed=%lld uniques=%d rectangles=%llu "
      "rect_par_point=%.1f vague_max=%llu vagues=%llu masse=%s permutation=%s "
      "t_index_ms=%.1f t_front_ms=%.1f\n",
      cloud_family_name(a.family), a.n, coord, (long long)a.s, a.seed,
      ix.unique_count(), (unsigned long long)front.st.rectangles,
      ix.unique_count() ? (double)front.st.rectangles / (double)pts.size() : 0.0,
      (unsigned long long)front.st.wave_peak, (unsigned long long)front.st.levels,
      mass_ok ? "EXACTE" : "ECART", perm_ok ? "OK" : "ECART", ms(t1 - t0),
      ms(t2 - t1));

  const bool violated = !mass_ok || !floor_ok || !perm_ok;
  if (violated) return a.inject.empty() ? 3 : 4;
  if (!a.inject.empty()) {
    std::fprintf(stderr, "PORTE INEFFICACE : injection %s non detectee\n",
                 a.inject.c_str());
    return 3;
  }
  return 0;
}
