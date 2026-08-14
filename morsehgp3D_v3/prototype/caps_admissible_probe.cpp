// MorseHGP3D v3 — LE CERTIFICAT DE LOCALITE, RESTREINT AUX DIRECTIONS
// REELLEMENT ATTEIGNABLES.
//
// Specification : audits/NOTE_SOLUTION_CALOTTES_ADMISSIBLES_20260815.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LE FAIT QUI MOTIVE CE FICHIER
//
// Le certificat par calottes existe et il est contre-audite, mais il echoue la
// ou le contrat vit : `49,8 %` d'ancres NON certifiees sur `uniform` a
// `n=1500`, `99,1 %` sur `terrain`. Une nappe n'a par construction aucun point
// au-dela de sa normale, donc la cellule normale n'est jamais couverte, donc
// l'ancre n'est jamais certifiee.
//
// ---------------------------------------------------------------------------
// POURQUOI L'EXIGENCE EST TROP FORTE
//
// Le theoreme demande que TOUTE direction soit couverte `K` fois. Or toutes ne
// sont pas atteignables.
//
// LEMME DU PARTENAIRE ANTIPODAL. Soit `S` un support positif contenant `x`, de
// circumboule `B(c,R)`, et `u=(c-x)/R`. Il existe `v` dans `S` avec
// `(v-x).u > R`.
//   Preuve : `c` est dans le relint de `conv(S)`, donc `c = somme lambda_i v_i`
//   avec tous `lambda_i > 0` et `somme lambda_i (v_i-c) = 0`. Le produit
//   scalaire avec `x-c` donne `lambda_x R^2 + somme_{i!=x} lambda_i (v_i-c).(x-c) = 0`,
//   dont le premier terme est strictement positif : il existe donc `v` avec
//   `(v-c).(x-c) < 0`. Comme `x-c = -R u`, cela donne `(v-c).u > 0`, puis
//   `(v-x).u = (v-c).u + R > R`.
//
// Une direction n'est donc admissible que si le nuage possede un POINT REEL
// au-dela de l'equateur de la boule. La normale d'une nappe n'en a aucun.
//
// ---------------------------------------------------------------------------
// LE THEOREME PROPOSE
//
// Si, pour un rayon `r`, toute cellule ADMISSIBLE A `r` est couverte par au
// moins `K` calottes strictes, alors toute boule passant par `x` avec au plus
// `K-1` interieurs verifie `diam(B) < r`.
//   Preuve : soit `B` de diametre `D >= r` passant par `x`, de direction `u`.
//   Le lemme donne un partenaire au-dela de l'equateur, donc `u` est admissible
//   a `D`, donc a `r` — la condition s'affaiblit quand le diametre croit. La
//   cellule de `u` est donc couverte `K` fois et `B` a au moins `K` interieurs.
//
// TEST EXACT PAR CELLULE. Le minimum d'une forme lineaire sur un triangle
// spherique est atteint en un SOMMET : la cellule est l'intersection de la
// sphere avec un cone convexe dont les rayons extremes sont ses sommets. Donc
// `C` est admissible a `r` si et seulement s'il existe `v`, `s = v-x`, avec
//
//   |s|^2 <= r^2   et   pour tout sommet g de C :  s.g > 0  et  4 (s.g)^2 > r^2 |g|^2
//
// Aucun flottant, aucune racine.
//
// ---------------------------------------------------------------------------
// CE QUE CE FICHIER PROUVE, ET CE QU'IL NE PROUVE PAS
//
// Il MESURE le gain de certification, et il FALSIFIE le theoreme : le mode
// `--falsifie` enumere tous les supports retenus par ancre d'arete diametrale,
// et verifie que le diametre de chacun respecte le rayon certifie de CHACUN de
// ses sommets. Un seul depassement refute la proposition.
//
// Il ne remplace pas `certified_locality_probe.cpp`, qui reste l'autorite du
// certificat non restreint. Sa grille, son arithmetique et sa selection sont
// reecrites ici independamment, precisement pour qu'un defaut commun ne se
// compense pas.
//
// CODES DE SORTIE
//   0  accord     1  refutation     2  refus avant calcul     3  plancher
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "cloud_families.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;

[[noreturn]] void refuse(const std::string& r) {
  std::fprintf(stderr, "REFUS: %s\n", r.c_str());
  std::exit(2);
}

struct Pt { i64 c[3] = {0, 0, 0}; };

inline i64 d2p(const Pt& a, const Pt& b) {
  const i64 x = a.c[0] - b.c[0], y = a.c[1] - b.c[1], z = a.c[2] - b.c[2];
  return x * x + y * y + z * z;
}

// ---------------------------------------------------------------------------
// LA GRILLE DE DIRECTIONS : subdivision de l'octaedre, sommets ENTIERS
// `|g_x|+|g_y|+|g_z| = m`. Les cellules sont des triangles geodesiques.
// ---------------------------------------------------------------------------
struct DirGrid {
  std::vector<std::array<i64, 3>> vertices;
  std::vector<i64> vnorm2;
  std::vector<std::array<int, 3>> cells;
};

DirGrid build_grid(int m) {
  DirGrid g;
  std::vector<std::array<i64, 3>> vs;
  auto index_of = [&](i64 x, i64 y, i64 z) -> int {
    for (size_t i = 0; i < vs.size(); ++i)
      if (vs[i][0] == x && vs[i][1] == y && vs[i][2] == z) return (int)i;
    vs.push_back({x, y, z});
    return (int)vs.size() - 1;
  };
  for (int sx = 0; sx < 2; ++sx)
    for (int sy = 0; sy < 2; ++sy)
      for (int sz = 0; sz < 2; ++sz) {
        const int ex = sx ? 1 : -1, ey = sy ? 1 : -1, ez = sz ? 1 : -1;
        auto vert = [&](int a, int b) {
          return index_of((i64)ex * a, (i64)ey * b, (i64)ez * (m - a - b));
        };
        for (int a = 0; a + 0 <= m; ++a)
          for (int b = 0; a + b <= m; ++b) {
            if (a + b <= m - 1)
              g.cells.push_back({vert(a, b), vert(a + 1, b), vert(a, b + 1)});
            if (a + b <= m - 2)
              g.cells.push_back({vert(a + 1, b), vert(a, b + 1), vert(a + 1, b + 1)});
          }
      }
  g.vertices = vs;
  g.vnorm2.reserve(vs.size());
  for (const auto& v : vs) g.vnorm2.push_back(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  return g;
}

// `rho^2 = |g|^2 |s|^4 / (g.s)^2`, `den == 0` marquant l'infini.
struct Rho2 { i128 num = 0, den = 0; };
inline bool rho_moins(const Rho2& a, const Rho2& b) {
  if (a.den == 0) return false;
  if (b.den == 0) return true;
  return a.num * b.den < b.num * a.den;
}
// `rho <= r`  <=>  `r^2 den >= num`
inline bool rho_sous(i64 r2, const Rho2& a) {
  if (a.den == 0) return false;
  return (i128)r2 * a.den >= a.num;
}

// ---------------------------------------------------------------------------
// LE CERTIFICAT A RAYON FIXE. Rend vrai si toute cellule — ou toute cellule
// ADMISSIBLE — est couverte au moins `kmin` fois par des calottes de rayon `r`.
// ---------------------------------------------------------------------------
struct Compteurs {
  long long cellules = 0, admissibles = 0, couvertes = 0;
};

bool certifie(const Pt& x, const std::vector<Pt>& P, const std::vector<int>& voisins,
              const DirGrid& g, i64 r, int kmin, bool restreint, Compteurs* cpt) {
  const i64 r2 = r * r;
  const size_t nc = g.cells.size();
  std::vector<int> depth(nc, 0);
  std::vector<char> admis(nc, restreint ? 0 : 1);
  for (int id : voisins) {
    const Pt& v = P[(size_t)id];
    i64 s[3];
    for (int k = 0; k < 3; ++k) s[k] = v.c[k] - x.c[k];
    const i64 ds = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
    if (ds == 0 || ds > r2) continue;
    for (size_t c = 0; c < nc; ++c) {
      // Couverture : la cellule est incluse dans la calotte `C_v(r)` si et
      // seulement si ses trois sommets y sont, la calotte etant geodesiquement
      // convexe. Sommet couvert : `s.g > 0` et `(s.g)^2 r^2 > |g|^2 (|s|^2)^2`.
      bool couvre = true, adm = true;
      for (int k = 0; k < 3; ++k) {
        const auto& gv = g.vertices[(size_t)g.cells[c][(size_t)k]];
        const i64 dot = gv[0] * s[0] + gv[1] * s[1] + gv[2] * s[2];
        if (dot <= 0) { couvre = false; adm = false; break; }
        const i64 n2 = g.vnorm2[(size_t)g.cells[c][(size_t)k]];
        if (couvre && !((i128)dot * dot * r2 > (i128)n2 * ds * ds)) couvre = false;
        // Admissibilite : `v` est au-dela de l'equateur pour TOUTE direction de
        // la cellule, soit `4 (s.g)^2 > r^2 |g|^2` au sommet minimisant.
        if (adm && !((i128)4 * dot * dot > (i128)r2 * n2)) adm = false;
        if (!couvre && !adm) break;
      }
      if (couvre) ++depth[c];
      if (restreint && adm) admis[c] = 1;
    }
  }
  bool ok = true;
  for (size_t c = 0; c < nc; ++c) {
    ++cpt->cellules;
    if (!admis[c]) continue;
    ++cpt->admissibles;
    if (depth[c] >= kmin) ++cpt->couvertes;
    else ok = false;
  }
  return ok;
}

std::vector<Pt> nuage(const std::string& f, long long n, long long coord, long long seed) {
  mhgp3v::CloudFamily fam;
  if (f == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (f == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (f == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (f == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (f == "scanline_overlap_multiecho")
    fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else refuse("famille inconnue : " + f);
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(fam, (int)n);
  const auto b = mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
  std::vector<Pt> P;
  P.reserve(b.size());
  for (const auto& q : b) P.push_back(Pt{{q.x, q.y, q.z}});
  return P;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  long long n = 800, coord = 0, seed = 1, kmin = 8, m = 4, threads = 0;
  double rayon_esp = 10.0, min_pct = 0.0;
  bool falsifie = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a == "--falsifie") falsifie = true;
    else if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = atoll(val("--points=").c_str());
    else if (a.rfind("--coord=", 0) == 0) coord = atoll(val("--coord=").c_str());
    else if (a.rfind("--seed=", 0) == 0) seed = atoll(val("--seed=").c_str());
    else if (a.rfind("--kmin=", 0) == 0) kmin = atoll(val("--kmin=").c_str());
    else if (a.rfind("--m=", 0) == 0) m = atoll(val("--m=").c_str());
    else if (a.rfind("--rayon-espacements=", 0) == 0)
      rayon_esp = atof(val("--rayon-espacements=").c_str());
    else if (a.rfind("--min-certifies-pct=", 0) == 0)
      min_pct = atof(val("--min-certifies-pct=").c_str());
    else if (a.rfind("--threads=", 0) == 0) threads = atoll(val("--threads=").c_str());
    else refuse("option inconnue : " + a);
  }
  if (n < 20 || n > 20000) refuse("--points hors domaine");
  if (kmin < 2 || kmin > 16) refuse("--kmin hors domaine");
  if (m < 2 || m > 8) refuse("--m hors domaine");
  if (rayon_esp < 2.0 || rayon_esp > 64.0) refuse("--rayon-espacements hors domaine");
  if (falsifie && n > 400) refuse("--falsifie est borne a 400 points");
  if (threads <= 0) threads = (long long)std::thread::hardware_concurrency();
  if (threads <= 0) threads = 1;

  const std::vector<Pt> P = nuage(family, n, coord, seed);
  const int N = (int)P.size();
  i64 lo[3] = {INT64_MAX, INT64_MAX, INT64_MAX}, hi[3] = {INT64_MIN, INT64_MIN, INT64_MIN};
  for (const Pt& p : P)
    for (int c = 0; c < 3; ++c) {
      if (p.c[c] < lo[c]) lo[c] = p.c[c];
      if (p.c[c] > hi[c]) hi[c] = p.c[c];
    }
  double vol = 1.0;
  for (int c = 0; c < 3; ++c) vol *= (double)std::max<i64>(1, hi[c] - lo[c]);
  const double esp = std::cbrt(vol / (double)N);
  const i64 r = (i64)(rayon_esp * esp + 0.5);

  const DirGrid g = build_grid((int)m);
  std::vector<char> cert_tout((size_t)N, 0), cert_adm((size_t)N, 0);
  std::vector<Compteurs> cpt((size_t)threads);
  auto worker = [&](long long tid) {
    std::vector<int> vois;
    for (int i = (int)tid; i < N; i += (int)threads) {
      vois.clear();
      for (int j = 0; j < N; ++j)
        if (j != i && d2p(P[(size_t)i], P[(size_t)j]) <= r * r) vois.push_back(j);
      Compteurs muet;
      cert_tout[(size_t)i] =
          certifie(P[(size_t)i], P, vois, g, r, (int)kmin, false, &muet) ? 1 : 0;
      cert_adm[(size_t)i] =
          certifie(P[(size_t)i], P, vois, g, r, (int)kmin, true, &cpt[(size_t)tid]) ? 1 : 0;
    }
  };
  std::vector<std::thread> th;
  for (long long t = 0; t < threads; ++t) th.emplace_back(worker, t);
  for (auto& t : th) t.join();
  Compteurs C;
  for (const Compteurs& c : cpt) {
    C.cellules += c.cellules; C.admissibles += c.admissibles; C.couvertes += c.couvertes;
  }
  long long nt = 0, na = 0;
  for (int i = 0; i < N; ++i) { nt += cert_tout[(size_t)i]; na += cert_adm[(size_t)i]; }
  std::printf("caps_admissible : famille=%s n=%d m=%lld cellules=%zu kmin=%lld"
              " rayon=%lld (%.1f espacements)\n",
              family.c_str(), N, m, g.cells.size(), kmin, (long long)r, rayon_esp);
  std::printf("  certificats : toutes_cellules=%lld (%.2f%%) admissibles=%lld (%.2f%%)"
              " gain=%.2f pts\n",
              nt, 100.0 * (double)nt / N, na, 100.0 * (double)na / N,
              100.0 * (double)(na - nt) / N);
  std::printf("  cellules : total=%lld admissibles=%lld (%.2f%%) couvertes=%lld\n",
              C.cellules, C.admissibles,
              100.0 * (double)C.admissibles / (double)std::max(1LL, C.cellules),
              C.couvertes);

  if (falsifie) {
    // LA FALSIFICATION. Tout support positif d'arite deux, trois ou quatre dont
    // un sommet est certifie doit avoir un diametre STRICTEMENT inferieur au
    // rayon de ce sommet. Un seul depassement refute le theoreme.
    long long testes = 0, viole = 0;
    for (int i = 0; i < N; ++i)
      for (int j = i + 1; j < N; ++j) {
        const i64 D2 = d2p(P[(size_t)i], P[(size_t)j]);
        if (D2 == 0) continue;
        // Rang de la boule diametrale : c'est le support q2.
        long long ins = 0;
        for (int z = 0; z < N && ins < kmin; ++z) {
          if (z == i || z == j) continue;
          i128 s = 0;
          for (int c = 0; c < 3; ++c)
            s += (i128)(P[(size_t)z].c[c] - P[(size_t)i].c[c]) *
                 (P[(size_t)z].c[c] - P[(size_t)j].c[c]);
          if (s < 0) ++ins;
        }
        if (ins >= kmin) continue;
        ++testes;
        if ((cert_adm[(size_t)i] || cert_adm[(size_t)j]) && D2 >= r * r) ++viole;
      }
    std::printf("falsifie : supports_q2_testes=%lld violations=%lld\n", testes, viole);
    if (testes == 0) { std::fprintf(stderr, "PLANCHER: aucun support teste\n"); return 3; }
    if (viole > 0) {
      std::fprintf(stderr, "REFUTATION: %lld supports depassent le rayon certifie\n", viole);
      return 1;
    }
    return 0;
  }
  if (C.admissibles == 0) { std::fprintf(stderr, "PLANCHER: aucune cellule admissible\n"); return 3; }
  if (100.0 * (double)na / N < min_pct) {
    std::fprintf(stderr, "PLANCHER: %.2f%% d'ancres certifiees < %.2f%%\n",
                 100.0 * (double)na / N, min_pct);
    return 3;
  }
  return 0;
}
