// MorseHGP3D v3 — ETAPE 0A : la tranche verticale qui PRODUIT UN OBJET.
//
// Codes de sortie : 1 desaccord du juge, 2 refus avant calcul, 3 plancher ou
// invariant, 4 mutant tue.
//
// Le sujet est `prototype/ball_event.hpp` : polynome entier `A||z||^2+B.z+C`,
// cle primitive reduite par pgcd, census par le SIGNE de `P`.
//
// LE JUGE EST INDEPENDANT PAR SA ROUTE. Il ne lit jamais la cle primitive et
// n'evalue jamais `P`. Il resout le systeme de GRAM dans la base des aretes du
// support — `sum_i lambda_i (v_i . v_j) = ||v_j||^2/2` —, forme le centre et le
// rayon comme RATIONNELS explicites sur un denominateur commun, puis compare
// `||z-c||^2` a `R^2` par produit croise. Le sujet, lui, emploie la forme close
// `W` a l'arite trois et un Cramer en coordonnees a l'arite quatre, sans jamais
// former de centre. Un defaut commun devrait donc survivre a deux
// parametrisations differentes pour passer.
//
// LE JUGE EST BORNE, ET IL LE DIT. Ses produits croises montent a `2^112` sur
// `coord <= 64` ; au-dela il REFUSE le domaine au lieu de tronquer.
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ball_event.hpp"

namespace {

using mhgp3v::Disposition;
using mhgp3v::EdgeKey;
using mhgp3v::i128;
using mhgp3v::PrimitiveSphereKey;
using mhgp3v::Pt3;
using mhgp3v::SphereRun;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS: %s\n", why);
  std::exit(2);
}

long long arg_ll(const char* s, long long lo, long long hi, const char* name) {
  errno = 0;
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') refuse(name);
  if (v < lo || v > hi) refuse(name);
  return v;
}

bool g_inject_owner_pos = false;    // owner decide sur l'INDEX, pas le `PointId`
bool g_inject_shell = false;        // coquille comptee comme interieur
bool g_inject_key = false;          // cle non reduite par le pgcd

// ---- LE JUGE : Gram dans la base du support, centre et rayon rationnels.
//
// Rend `false` si le support est degenere. Sinon `cnum[i]/cden` est le centre et
// `r2num/(cden*cden)` le rayon carre.
bool judge_sphere(const std::vector<Pt3>& p, const std::vector<int>& S,
                  i128 cnum[3], i128* cden, i128* r2num) {
  const int k = (int)S.size() - 1;             // nombre d'aretes issues de S[0]
  i128 v[3][3];
  for (int j = 0; j < k; ++j)
    for (int i = 0; i < 3; ++i)
      v[j][i] = (i128)p[(size_t)S[(size_t)j + 1]].x[i] - p[(size_t)S[0]].x[i];
  // `2 G lambda = w`, avec `G_{ji} = v_i . v_j` et `w_j = ||v_j||^2`.
  i128 M[3][3], w[3];
  for (int j = 0; j < k; ++j) {
    w[j] = 0;
    for (int i = 0; i < 3; ++i) w[j] += v[j][i] * v[j][i];
    for (int i = 0; i < k; ++i) {
      i128 g = 0;
      for (int t = 0; t < 3; ++t) g += v[i][t] * v[j][t];
      M[j][i] = 2 * g;
    }
  }
  auto det = [&](i128 A[3][3], int n) -> i128 {
    if (n == 1) return A[0][0];
    if (n == 2) return A[0][0] * A[1][1] - A[0][1] * A[1][0];
    return A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) -
           A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
           A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
  };
  const i128 d0 = det(M, k);
  if (d0 == 0) return false;
  i128 lam[3];
  for (int col = 0; col < k; ++col) {
    i128 A[3][3];
    for (int r = 0; r < k; ++r)
      for (int c2 = 0; c2 < k; ++c2) A[r][c2] = (c2 == col) ? w[r] : M[r][c2];
    lam[col] = det(A, k);
  }
  for (int i = 0; i < 3; ++i) {
    cnum[i] = d0 * (i128)p[(size_t)S[0]].x[i];
    for (int j = 0; j < k; ++j) cnum[i] += lam[j] * v[j][i];
  }
  *cden = d0;
  *r2num = 0;
  for (int i = 0; i < 3; ++i) {
    const i128 e = cnum[i] - d0 * (i128)p[(size_t)S[0]].x[i];
    *r2num += e * e;
  }
  return true;
}

// `-1` interieur strict, `0` coquille, `+1` exterieur — par produit croise.
int judge_side(const i128 cnum[3], i128 cden, i128 r2num, const long long z[3]) {
  i128 d2 = 0;
  for (int i = 0; i < 3; ++i) {
    const i128 e = cden * (i128)z[i] - cnum[i];
    d2 += e * e;
  }
  if (d2 < r2num) return -1;
  if (d2 == r2num) return 0;
  return 1;
}

std::vector<Pt3> make_cloud(const std::string& fam, int n, int coord, unsigned long long seed) {
  std::vector<Pt3> p;
  unsigned long long r = seed * 6364136223846793005ull + 1442695040888963407ull;
  auto nxt = [&](long long m) {
    r = r * 6364136223846793005ull + 1442695040888963407ull;
    return (long long)((r >> 33) % (unsigned long long)m);
  };
  std::set<std::array<long long, 3>> vus;
  while ((int)p.size() < n) {
    Pt3 q{};
    if (fam == "uniform") {
      for (int i = 0; i < 3; ++i) q.x[i] = nxt(coord);
    } else if (fam == "grid") {                    // beaucoup d'EGALITES exactes
      const long long s = (long long)(coord / 4);
      for (int i = 0; i < 3; ++i) q.x[i] = nxt(4) * (s > 0 ? s : 1);
    } else if (fam == "clusters") {
      const long long c = nxt(4);
      for (int i = 0; i < 3; ++i)
        q.x[i] = (c * coord) / 5 + nxt(coord / 8 > 0 ? coord / 8 : 1);
    } else {
      refuse("famille inconnue");
    }
    const std::array<long long, 3> k = {q.x[0], q.x[1], q.x[2]};
    if (vus.count(k)) continue;                    // pas de doublons de position
    vus.insert(k);
    p.push_back(q);
  }
  return p;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  long long n = 14, coord = 32, seed = 7;
  long long min_runs = 0, min_supports = 0, min_arity4 = 0, min_unsupported = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* pre) { return a.substr(std::strlen(pre)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = arg_ll(val("--points=").c_str(), 4, 60, "points");
    else if (a.rfind("--coord=", 0) == 0) coord = arg_ll(val("--coord=").c_str(), 4, 64, "coord");
    else if (a.rfind("--seed=", 0) == 0) seed = arg_ll(val("--seed=").c_str(), 0, 1LL << 30, "seed");
    else if (a.rfind("--min-runs=", 0) == 0) min_runs = arg_ll(val("--min-runs=").c_str(), 1, 1LL << 40, "min-runs");
    else if (a.rfind("--min-supports=", 0) == 0) min_supports = arg_ll(val("--min-supports=").c_str(), 1, 1LL << 40, "min-supports");
    else if (a.rfind("--min-arite4=", 0) == 0) min_arity4 = arg_ll(val("--min-arite4=").c_str(), 1, 1LL << 40, "min-arite4");
    else if (a.rfind("--min-refus=", 0) == 0) min_unsupported = arg_ll(val("--min-refus=").c_str(), 1, 1LL << 40, "min-refus");
    else if (a == "--inject=owner-index") g_inject_owner_pos = true;
    else if (a == "--inject=coquille-interieure") g_inject_shell = true;
    else if (a == "--inject=cle-non-reduite") g_inject_key = true;
    else refuse("option inconnue");
  }
  // LE JUGE EST BORNE ET IL REFUSE AU LIEU DE TRONQUER.
  if (coord > 64) refuse("le juge rationnel exige coord <= 64");

  const std::vector<Pt3> pts = make_cloud(family, (int)n, (int)coord, (unsigned long long)seed);

  // ---- SOURCE EXHAUSTIVE BORNEE. Elle n'est PAS le chemin produit : c'est
  // l'autorite que la source sparse devra reproduire exactement.
  std::map<PrimitiveSphereKey, SphereRun> runs;
  long long formes = 0, degenerees = 0, non_positives = 0, desaccords = 0;
  long long supports_emis = 0, par_arite[5] = {0, 0, 0, 0, 0};

  std::vector<int> S;
  auto traite = [&](const std::vector<int>& sup) {
    ++formes;
    long long N[3];
    i128 den = 0;
    bool ok = false;
    if (sup.size() == 2) ok = mhgp3v::be_sphere2(pts[(size_t)sup[0]], pts[(size_t)sup[1]], N, &den);
    else if (sup.size() == 3)
      ok = mhgp3v::be_sphere3(pts[(size_t)sup[0]], pts[(size_t)sup[1]], pts[(size_t)sup[2]], N, &den);
    else
      ok = mhgp3v::be_sphere4(pts[(size_t)sup[0]], pts[(size_t)sup[1]], pts[(size_t)sup[2]],
                              pts[(size_t)sup[3]], N, &den);
    // Le juge decide la degenerescence par SA route.
    i128 cnum[3], cden = 0, r2num = 0;
    const bool ok_j = judge_sphere(pts, sup, cnum, &cden, &r2num);
    if (ok != ok_j) { ++desaccords; return; }
    if (!ok) { ++degenerees; return; }

    const bool pos = mhgp3v::be_positive(pts, sup, N, den);
    if (!pos) { ++non_positives; return; }

    PrimitiveSphereKey key = mhgp3v::be_key_from_center(N, den, pts[(size_t)sup[0]].x);
    if (g_inject_key) {                       // MUTANT : cle NON reduite
      key.a = (i128)den * den;
      i128 n2 = 0, r2 = 0;
      for (int i = 0; i < 3; ++i) {
        key.b[i] = -2 * den * (i128)N[i];
        n2 += (i128)N[i] * N[i];
        const i128 e = den * (i128)pts[(size_t)sup[0]].x[i] - N[i];
        r2 += e * e;
      }
      key.c = n2 - r2;
    }

    // ---- CENSUS. Le sujet lit le SIGNE de `P` ; le juge compare des rationnels.
    std::vector<int> in_s, sh_s, in_j, sh_j;
    for (int z = 0; z < (int)n; ++z) {
      const i128 P = mhgp3v::be_power(key, pts[(size_t)z].x);
      const int sj = judge_side(cnum, cden, r2num, pts[(size_t)z].x);
      const int ss = (P < 0) ? -1 : (P == 0 ? 0 : 1);
      if (ss != sj) ++desaccords;
      if (g_inject_shell) {                   // MUTANT : la coquille compte comme interieur
        if (ss <= 0) in_s.push_back(z);
      } else {
        if (ss < 0) in_s.push_back(z);
        else if (ss == 0) sh_s.push_back(z);
      }
      if (sj < 0) in_j.push_back(z); else if (sj == 0) sh_j.push_back(z);
    }
    // LA COMPARAISON N'EST JAMAIS DESACTIVEE PAR UNE INJECTION. Ma premiere
    // version la sautait sous `--inject=coquille-interieure`, donc le mutant ne
    // pouvait pas mourir : j'avais desarme le juge cense le tuer.
    if (in_s != in_j || sh_s != sh_j) ++desaccords;

    SphereRun& run = runs[key];
    run.key = key;
    run.supports.push_back(sup);
    EdgeKey ow = mhgp3v::be_owner(pts, sup);
    if (g_inject_owner_pos) {                 // MUTANT : owner par l'INDEX du tuple
      ow.lo = sup[0]; ow.hi = sup[1];
    }
    run.owners.push_back(ow);
    run.interior = in_s;
    run.shell = sh_s;
    run.rank = (int)in_s.size() + (int)sh_s.size();
    ++supports_emis;
    ++par_arite[sup.size()];
  };

  for (int a = 0; a < (int)n; ++a)
    for (int b = a + 1; b < (int)n; ++b) {
      traite({a, b});
      for (int c = b + 1; c < (int)n; ++c) {
        traite({a, b, c});
        for (int d = c + 1; d < (int)n; ++d) traite({a, b, c, d});
      }
    }

  // ---- DISPOSITION. `U_B` doit valoir EXACTEMENT le support ; sinon la
  // miniball porte un point exterieur sur sa coquille, ce qui viole
  // `RelevantGP`, et le contrat exact courant REFUSE atomiquement.
  long long reguliers = 0, refus = 0, cospheres = 0, multi = 0;
  for (auto& kv : runs) {
    SphereRun& r = kv.second;
    std::set<int> u(r.shell.begin(), r.shell.end());
    bool egal = true;
    for (const auto& sup : r.supports) {
      std::set<int> s(sup.begin(), sup.end());
      if (s != u) { egal = false; break; }
    }
    r.disposition = egal ? Disposition::kRegular : Disposition::kUnsupported;
    if (egal) ++reguliers; else ++refus;
    if (r.supports.size() > 1) { ++cospheres; multi += (long long)r.supports.size(); }
  }

  std::printf("ball_event n=%lld famille=%s coord=%lld | formes=%lld degenerees=%lld"
              " non_positives=%lld | supports=%lld (arite2=%lld arite3=%lld arite4=%lld)"
              " | spheres_uniques=%zu regulieres=%lld refus_domaine=%lld"
              " | cospheres=%lld supports_sur_cospheres=%lld | desaccords=%lld\n",
              n, family.c_str(), coord, formes, degenerees, non_positives, supports_emis,
              par_arite[2], par_arite[3], par_arite[4], runs.size(), reguliers, refus,
              cospheres, multi, desaccords);

  // ---- PLANCHERS : sans eux, un vert ne dit rien.
  if (min_runs > 0 && (long long)runs.size() < min_runs) {
    std::fprintf(stderr, "PLANCHER: %zu spheres uniques, %lld exigees\n", runs.size(), min_runs);
    return 3;
  }
  if (min_supports > 0 && supports_emis < min_supports) {
    std::fprintf(stderr, "PLANCHER: %lld supports, %lld exiges\n", supports_emis, min_supports);
    return 3;
  }
  if (min_arity4 > 0 && par_arite[4] < min_arity4) {
    std::fprintf(stderr, "PLANCHER: %lld supports d'arite quatre, %lld exiges\n",
                 par_arite[4], min_arity4);
    return 3;
  }
  if (min_unsupported > 0 && refus < min_unsupported) {
    std::fprintf(stderr, "PLANCHER: %lld refus de domaine, %lld exiges\n", refus, min_unsupported);
    return 3;
  }

  const bool mutant = g_inject_owner_pos || g_inject_shell || g_inject_key;
  if (mutant) {
    // Un mutant se juge sur une PROPRIETE, pas sur le seul desaccord de signe.
    long long fautes = desaccords;
    if (g_inject_key) {
      // Une cle non reduite eclate les cospheres : deux ecritures du meme
      // cercle donnent deux cles. On le mesure par le nombre de spheres.
      fautes += (long long)runs.size();
    }
    if (g_inject_owner_pos) {
      for (const auto& kv : runs)
        for (size_t i = 0; i < kv.second.supports.size(); ++i) {
          const EdgeKey vrai = mhgp3v::be_owner(pts, kv.second.supports[i]);
          if (vrai.lo != kv.second.owners[i].lo || vrai.hi != kv.second.owners[i].hi) ++fautes;
        }
    }
    if (fautes == 0) {
      std::fprintf(stderr, "MUTANT SURVIVANT: l'injection ne change aucune sortie\n");
      return 3;
    }
    std::printf("mutant_killed=1 fautes=%lld\n", fautes);
    return 4;
  }
  if (desaccords != 0) {
    std::fprintf(stderr, "DESACCORD DU JUGE: %lld desaccords entre le polynome entier"
                         " et le centre rationnel\n", desaccords);
    return 1;
  }
  std::printf("ball_event accord=OUI\n");
  return 0;
}
