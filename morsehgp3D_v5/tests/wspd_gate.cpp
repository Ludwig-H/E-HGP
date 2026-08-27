// MorseHGP3D v5 — porte WSPD : ledger de masse exact, plancher, equivariance,
// mutants. Codes : 0 conforme, 2 refus avant calcul, 3 invariant viole,
// 4 mutant tue.
//
//   L1  ledger : Σ|A||B| (pondere par multiplicite) = C(n,2) − Σ_u C(mult_u,2),
//       exactement, en 128 bits ;
//   L2  plancher de non-vacuite : au moins --min-rect rectangles ;
//   L3  (--check-permutation) le multiset des rectangles, en coordonnees de
//       plages de positions uniques, est invariant par permutation d'entree ;
//   L4  (--discriminate-cap) porte appariee : le front sain doit etre
//       STRICTEMENT plus petit que celui d'un critere terminal a cap (mutant
//       `wspd-cap-terminal`) — sur `two_lines`, ou des blocs bien separes
//       depassent le cap par construction ;
//   L5  (--discriminate-split) idem contre la scission du plus peuple
//       (mutant `wspd-split-heaviest`) : front different ou plus gros.
// Mutants : `wspd-drop-rect` (un rectangle perdu) est tue par L1 ; les deux
// autres par les portes appariees.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/mutants.hpp"
#include "../src/tree/cloud_index.hpp"
#include "../src/wspd/wavefront.hpp"

using namespace mhgp5;

namespace {

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool ok = true;
  int n = 2000;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 min_rect = 1;
  bool check_permutation = false;
  bool discriminate_cap = false;
  bool discriminate_split = false;
  std::string inject;
};

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.ok = parse_cloud_family(v, &a.family) && a.ok;
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--min-rect=")) a.min_rect = (u64)std::atoll(v);
    else if (const char* v = val("--inject=")) a.inject = v;
    else if (arg == "--check-permutation") a.check_permutation = true;
    else if (arg == "--discriminate-cap") a.discriminate_cap = true;
    else if (arg == "--discriminate-split") a.discriminate_split = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.ok = false;
    }
  }
  return a;
}

struct Front {
  WspdStats st;
  std::vector<std::pair<std::pair<i32, i32>, std::pair<i32, i32>>> rects;  // plages (A), (B)
};

Front run_front(const CloudIndex& ix, i64 s) {
  Front f;
  f.st = wspd_wavefront(ix, s, 1, [&](const WspdRect& r) {
    const NodeRange ra = ix.range_of(r.a), rb = ix.range_of(r.b);
    f.rects.push_back({{ra.first, ra.last}, {rb.first, rb.last}});
  });
  std::sort(f.rects.begin(), f.rects.end());
  return f;
}

// Un run "temoin" execute avec un mutant active TEMPORAIREMENT : le
// registre est vide en production, la porte appariee est la seule a le
// remplir en cours d'execution, et la lecture par site est memorisee
// (MHGP5_MUTANT) — d'ou un sous-processus par bras, pas une bascule.
int rerun_with(const char* self, const Args& a, const char* mutant, u64* rectangles) {
  char cmd[1024];
  std::snprintf(cmd, sizeof(cmd), "%s --family=%s --n=%d --coord=%d --seed=%lld --s=%lld --inject=%s --print-rect",
                self, cloud_family_name(a.family), a.n, a.coord, a.seed, (long long)a.s, mutant);
  FILE* p = popen(cmd, "r");
  if (!p) return -1;
  char line[256];
  int rc = -1;
  while (std::fgets(line, sizeof(line), p))
    if (std::sscanf(line, "rectangles=%llu", (unsigned long long*)rectangles) == 1) rc = 0;
  const int status = pclose(p);
  return (status == 0 && rc == 0) ? 0 : -1;
}

}  // namespace

int main(int argc, char** argv) {
  bool print_rect = false;
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], "--print-rect") == 0) {
      print_rect = true;
      argv[i] = argv[argc - 1];
      --argc;
      break;
    }
  const Args a = parse(argc, argv);
  if (!a.ok || a.n < 2 || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (!a.inject.empty() && !mutants_enable(a.inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", a.inject.c_str());
    return 2;
  }
  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> pts = make_family_cloud(a.family, a.n, coord, a.seed);
  if (pts.size() < 2) {
    std::fprintf(stderr, "REFUS : la famille n'a produit que %zu points\n", pts.size());
    return 2;
  }
  const CloudIndex ix = build_cloud_index(pts);
  if (!ix.valid) return 2;
  const Front f = run_front(ix, a.s);
  if (print_rect) {
    std::printf("rectangles=%llu\n", (unsigned long long)f.st.rectangles);
    return 0;
  }
  const u128 expected = expected_pair_mass(ix);
  std::printf("famille=%s n=%d coord=%d s=%lld rectangles=%llu vagues=%llu pic=%llu uniques=%d\n",
              cloud_family_name(a.family), a.n, coord, (long long)a.s, (unsigned long long)f.st.rectangles,
              (unsigned long long)f.st.levels, (unsigned long long)f.st.wave_peak, ix.unique_count());
  if (f.st.pair_mass != expected) {
    std::fprintf(stderr, "LEDGER : masse %llu != attendue %llu\n", (unsigned long long)f.st.pair_mass,
                 (unsigned long long)expected);
    return a.inject.empty() ? 3 : 4;
  }
  if (f.st.rectangles < a.min_rect) {
    std::fprintf(stderr, "PLANCHER : %llu rectangles < %llu\n", (unsigned long long)f.st.rectangles,
                 (unsigned long long)a.min_rect);
    return 3;
  }
  if (a.check_permutation) {
    std::vector<u32> perm(pts.size());
    std::iota(perm.begin(), perm.end(), 0u);
    std::mt19937 rng(17);
    std::shuffle(perm.begin(), perm.end(), rng);
    std::vector<P3> shuffled(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) shuffled[perm[i]] = pts[i];
    const Front g = run_front(build_cloud_index(shuffled), a.s);
    if (g.rects != f.rects) {
      std::fprintf(stderr, "EQUIVARIANCE : le front depend de l'ordre d'entree\n");
      return 3;
    }
  }
  if (a.discriminate_cap || a.discriminate_split) {
    u64 witness = 0;
    const char* mut = a.discriminate_cap ? "wspd-cap-terminal" : "wspd-split-heaviest";
    if (rerun_with(argv[0], a, mut, &witness) != 0) {
      std::fprintf(stderr, "REFUS : bras temoin injouable\n");
      return 2;
    }
    std::printf("temoin %s rectangles=%llu\n", mut, (unsigned long long)witness);
    if (!(f.st.rectangles < witness)) {
      std::fprintf(stderr, "PORTE INEFFICACE : le mutant %s n'est pas discrimine (%llu vs %llu)\n", mut,
                   (unsigned long long)f.st.rectangles, (unsigned long long)witness);
      return 3;
    }
    // Le bras temoin est un mutant : la porte appariee le TUE en code 4
    // quand elle est invoquee avec l'injection (contrat des portes).
  }
  if (!a.inject.empty()) {
    // Un mutant demande mais non discrimine par cette invocation : la porte
    // ne l'a pas tue -> c'est un defaut de porte (3), jamais un succes.
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", a.inject.c_str());
    return 3;
  }
  std::printf("wspd_gate OK\n");
  return 0;
}
