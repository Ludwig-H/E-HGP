// Microprobe autonome du gateway `A x B x C`, avec oracle exhaustif.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed. GCP non utilise.
//
// CODES DE SORTIE : 1 desaccord du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
//
// ---------------------------------------------------------------------------
// DEUX CHOSES SONT MESUREES ICI, ET IL NE FAUT PAS LES CONFONDRE
//
// 1. LA SURETE DU CLASSIFIEUR. Sur de petites boites, on enumere TOUS les
//    triplets entiers et on confronte le verdict a la verite. `DEAD_*` doit
//    n'avoir aucun porteur, `ALL_STRICT` doit n'avoir que des porteurs, et
//    `MIXED` n'est jamais faux — c'est l'aveu d'ignorance.
//
// 2. LA SPARSITE. Sur un vrai nuage, la recursion ternaire ne doit expanser
//    AUCUNE paire sur `two_lines`. Le compteur `pairid_expanded` compte les
//    couples `(a,b)` reellement materialises ; il doit valoir zero.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/acute_owner_gateway.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/wspd_wavefront.hpp"

using namespace mhgp3v::acute;
using mhgp3v::WfNode;

namespace {

struct P3 {
  short x, y, z;
};

// Le predicat POINT, avec owner canonique — la verite que le gateway
// sur-approche. `pa < pb` est impose par l'appelant pour qu'un triangle ne
// compte pas deux fois sous la meme arete lue dans les deux sens.
bool porteur_canonique(const P3& a, const P3& b, const P3& x, int pa, int pb, int px) {
  const i64 ex = x.x - a.x, ey = x.y - a.y, ez = x.z - a.z;
  const i64 tx = b.x - x.x, ty = b.y - x.y, tz = b.z - x.z;
  const i64 E = ex * ex + ey * ey + ez * ez;
  const i64 X = tx * tx + ty * ty + tz * tz;
  if (E == 0 || X == 0) return false;
  const i64 dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
  const i64 D = dx * dx + dy * dy + dz * dz;
  if (D == 0) return false;
  if (E > D || X > D) return false;
  auto cle_moins = [](int u1, int v1, int u2, int v2) {
    const int a1 = u1 < v1 ? u1 : v1, b1 = u1 < v1 ? v1 : u1;
    const int a2 = u2 < v2 ? u2 : v2, b2 = u2 < v2 ? v2 : u2;
    return a1 != a2 ? a1 < a2 : b1 < b2;
  };
  if (E == D && !cle_moins(pa, pb, pa, px)) return false;
  if (X == D && !cle_moins(pa, pb, pb, px)) return false;
  return ex * tx + ey * ty + ez * tz < 0;  // `H < 0`, STRICTE
}

// ---------------------------------------------------------------------------
// PARTIE 1 : L'ORACLE EXHAUSTIF SUR PETITES BOITES.
//
// On n'y applique PAS l'owner canonique : sur des boites, le gateway ne connait
// aucun `PointId`. La verite comparee est donc l'owner MAXIMAL FAIBLE, et
// `ALL_STRICT` exige les `Delta` strictement positifs precisement pour que les
// deux coincident sur les blocs qu'il certifie.
bool porteur_faible(const i64 a[3], const i64 b[3], const i64 x[3]) {
  i64 D = 0, E = 0, X = 0, H = 0;
  for (int k = 0; k < 3; ++k) {
    D += (a[k] - b[k]) * (a[k] - b[k]);
    E += (a[k] - x[k]) * (a[k] - x[k]);
    X += (b[k] - x[k]) * (b[k] - x[k]);
    H += (x[k] - a[k]) * (b[k] - x[k]);
  }
  if (D == 0 || E == 0 || X == 0) return false;
  return E <= D && X <= D && H < 0;
}

unsigned long long rng_etat = 0x9E3779B97F4A7C15ULL;
unsigned long long rng() {
  rng_etat += 0x9E3779B97F4A7C15ULL;
  unsigned long long z = rng_etat;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// ---- BOITES BIAISEES, POUR QUE `ALL_STRICT` SOIT REELLEMENT ATTEINT.
//
// Des boites aleatoires ne satisfont presque jamais les trois conditions
// strictes a la fois : l'oracle non biaise rend `all_strict = 0`, donc la
// branche n'est pas testee et le mutant `all-strict-lache` survit par VACUITE.
// C'est exactement le vert-par-vacuite que le protocole interdit.
//
// On fabrique donc la configuration qui la declenche : `A` autour de l'origine,
// `B` autour de `(L,0,0)`, et `C` autour de `(L/2, h, 0)` avec
// `L/2 < h < L sqrt(3)/2` — au-dessus de la boule diametrale, sous la lentille.
// Avec `L = 20`, `h = 14` tient largement, et une gigue de +/-1 laisse les
// inegalites strictes.
BoxI boite_biaisee(int quoi) {
  const i64 L = 20, h = 14;
  const i64 c[3][3] = {{0, 0, 0}, {L, 0, 0}, {L / 2, h, 0}};
  BoxI B{};
  for (int k = 0; k < 3; ++k) {
    const i64 j = (i64)(rng() % 3) - 1;  // gigue dans [-1,1]
    B.ax[k].lo = c[quoi][k] + j;
    B.ax[k].hi = c[quoi][k] + j + (i64)(rng() % 2);
  }
  return B;
}

BoxI boite_alea(int etendue) {
  BoxI B{};
  for (int k = 0; k < 3; ++k) {
    const i64 u = (i64)(rng() % (unsigned)(2 * etendue + 1)) - etendue;
    const i64 v = (i64)(rng() % (unsigned)(2 * etendue + 1)) - etendue;
    B.ax[k].lo = u < v ? u : v;
    B.ax[k].hi = u < v ? v : u;
  }
  return B;
}

struct BilanOracle {
  long long blocs = 0;
  long long dead = 0, all_strict = 0, mixed = 0;
  long long dead_faux = 0;        // un `DEAD` qui contenait un porteur : FATAL
  long long all_faux = 0;         // un `ALL_STRICT` qui contenait un non-porteur
  long long mixed_tout_porteur = 0;   // conservatisme mesure, pas une faute
  long long mixed_aucun_porteur = 0;
};

BilanOracle oracle(int tours, int etendue, GwMutant mu, bool biais) {
  BilanOracle g;
  for (int t = 0; t < tours; ++t) {
    const BoxI A = biais ? boite_biaisee(0) : boite_alea(etendue);
    const BoxI B = biais ? boite_biaisee(1) : boite_alea(etendue);
    const BoxI C = biais ? boite_biaisee(2) : boite_alea(etendue);
    const Verdict v = classifie(extrema(A, B, C), mu);
    ++g.blocs;
    long long porteurs = 0, total = 0;
    for (i64 a0 = A.ax[0].lo; a0 <= A.ax[0].hi; ++a0)
    for (i64 a1 = A.ax[1].lo; a1 <= A.ax[1].hi; ++a1)
    for (i64 a2 = A.ax[2].lo; a2 <= A.ax[2].hi; ++a2)
    for (i64 b0 = B.ax[0].lo; b0 <= B.ax[0].hi; ++b0)
    for (i64 b1 = B.ax[1].lo; b1 <= B.ax[1].hi; ++b1)
    for (i64 b2 = B.ax[2].lo; b2 <= B.ax[2].hi; ++b2)
    for (i64 c0 = C.ax[0].lo; c0 <= C.ax[0].hi; ++c0)
    for (i64 c1 = C.ax[1].lo; c1 <= C.ax[1].hi; ++c1)
    for (i64 c2 = C.ax[2].lo; c2 <= C.ax[2].hi; ++c2) {
      const i64 a[3] = {a0, a1, a2}, b[3] = {b0, b1, b2}, x[3] = {c0, c1, c2};
      ++total;
      if (porteur_faible(a, b, x)) ++porteurs;
    }
    if (v == Verdict::kDeadPhi || v == Verdict::kDeadOwnerE || v == Verdict::kDeadOwnerX) {
      ++g.dead;
      if (porteurs > 0) ++g.dead_faux;
    } else if (v == Verdict::kAllStrict) {
      ++g.all_strict;
      if (porteurs != total) ++g.all_faux;
    } else {
      ++g.mixed;
      if (porteurs == total && total > 0) ++g.mixed_tout_porteur;
      if (porteurs == 0) ++g.mixed_aucun_porteur;
    }
  }
  return g;
}

// ---- SEPARATION, portee depuis le prefiltre combine. Le ledger `W_4` n'a de
// sens que sur des rectangles ou `A` et `B` sont SEPARES : depuis la racine,
// aucun nœud n'est jamais disjoint de `A`, donc aucun point n'est jamais temoin
// universel et `L4_open` reste a zero.
i64 isqrt_plancher(i128 v) {
  if (v <= 0) return 0;
  i64 x = (i64)1 << 40;
  while ((i128)x * x > v) x >>= 1;
  for (i64 b = x; b > 0; b >>= 1)
    while ((i128)(x + b) * (x + b) <= v) x += b;
  return x;
}
struct Sph { i64 c2[3]; i64 r2; };
Sph sphere_de(const BoxI& b) {
  Sph s{};
  i128 acc = 0;
  for (int i = 0; i < 3; ++i) {
    s.c2[i] = b.ax[i].lo + b.ax[i].hi;
    const i64 e = b.ax[i].hi - b.ax[i].lo;
    acc += (i128)e * e;
  }
  s.r2 = isqrt_plancher(acc);
  if ((i128)s.r2 * s.r2 < acc) ++s.r2;  // le VRAI plafond
  return s;
}
bool separes(const Sph& A, const Sph& B, i64 sep) {
  i128 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 e = B.c2[i] - A.c2[i];
    acc += (i128)e * e;
  }
  const i64 d = isqrt_plancher(acc);
  const i64 rmax = A.r2 > B.r2 ? A.r2 : B.r2;
  return (i128)d - A.r2 - B.r2 >= (i128)sep * rmax;
}

// ---------------------------------------------------------------------------
// PARTIE 2 : LA RECURSION TERNAIRE SUR UN VRAI NUAGE.
struct BilanSparse {
  long long noeuds = 0;             // triplets `(A,B,C)` classes
  long long dead_phi = 0, dead_e = 0, dead_x = 0;
  long long all_strict = 0;         // CarrierBlocks SYMBOLIQUES
  long long masse_all_strict = 0;   // leur masse logique, jamais materialisee
  long long feuilles = 0;           // triplets de feuilles testes point a point
  long long pairid_expanded = 0;    // paires `(a,b)` reellement materialisees
  long long carriers = 0;           // porteurs sous owner canonique
  long long carriers_symboliques = 0;
  long long blocs_faux = 0;  // triples d'un `ALL_STRICT` qui ne portent pas
  // ---- LEDGER CONJOINT (audit `1d9425d`, section 3).
  long long dead_w4 = 0;            // blocs morts par vivacite, avant tout aigu
  long long active_edge = 0;        // `ActiveOwnerEdgeBlock` emis
  long long seed3_emitted = 0;      // seeds ternaires reellement materialises
  long long pending = 0;            // debordements : jamais `DEAD`
  long long l4_credits = 0;         // credits `W_4` acquis en bloc
  long long rectangles = 0;         // rectangles WSPD, la vraie source
  long long residuel_blocs = 0;     // blocs `MIXED` a exactifier par arete
  long long residuel_rects = 0;     // ... et rectangles DISTINCTS concernes
  long long front2_rects = 0;       // rectangles relances au second front
  long long relation_spans = 0;     // spans endpoint CONSERVES pour rejeu
  long long residuel_paires = 0;    // et leur masse de paires
  long long frontiere_max = 0;      // plus grande frontiere indecise portee
};

}  // namespace

int main(int argc, char** argv) {
  int n = 400, tours = 4000, etendue = 2, min_dead = 0, min_all = 0;
  long long seed = 12345;
  std::string famille = "two_lines";
  bool mode_oracle = false, mode_sparse = false, mode_brute = false, biais = false;
  bool mode_fixture = false;
  bool ab_fige = false, deux_fronts = false;
  long long max_pairid = -1, min_carriers = -1, min_noeuds = 0;
  int r4 = 8;   // seuil de rejet q4, `h_4 = s_max - 3`
  int sep = 8;  // separation WSPD
  GwMutant mu = GwMutant::kNone;

  auto entier = [&](const std::string& a, const char* cle, long long* out) {
    const size_t L = std::strlen(cle);
    if (a.rfind(cle, 0) != 0) return false;
    const std::string v = a.substr(L);
    long long r = 0;
    const auto res = std::from_chars(v.data(), v.data() + v.size(), r);
    if (res.ec != std::errc() || res.ptr != v.data() + v.size()) {
      std::fprintf(stderr, "REFUS : valeur invalide pour %s\n", cle);
      std::exit(2);
    }
    *out = r;
    return true;
  };

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    long long t = 0;
    if (a == "--fixtures") { mode_fixture = true; continue; }
    if (a == "--ab-fige") { ab_fige = true; continue; }
    if (a == "--deux-fronts") { ab_fige = true; deux_fronts = true; continue; }
    if (a == "--oracle") { mode_oracle = true; continue; }
    if (a == "--oracle-biais") { mode_oracle = true; biais = true; continue; }
    if (a == "--sparse") { mode_sparse = true; continue; }
    if (a == "--brute") { mode_sparse = true; mode_brute = true; continue; }
    if (entier(a, "--points=", &t)) { n = (int)t; continue; }
    if (entier(a, "--tours=", &t)) { tours = (int)t; continue; }
    if (entier(a, "--etendue=", &t)) { etendue = (int)t; continue; }
    if (entier(a, "--seed=", &t)) { seed = t; continue; }
    if (entier(a, "--min-dead=", &t)) { min_dead = (int)t; continue; }
    if (entier(a, "--min-all-strict=", &t)) { min_all = (int)t; continue; }
    if (entier(a, "--max-pairid=", &t)) { max_pairid = t; continue; }
    if (entier(a, "--min-carriers=", &t)) { min_carriers = t; continue; }
    if (entier(a, "--min-noeuds=", &t)) { min_noeuds = t; continue; }
    if (entier(a, "--r4=", &t)) { r4 = (int)t; continue; }
    if (entier(a, "--separation=", &t)) { sep = (int)t; continue; }
    if (a.rfind("--family=", 0) == 0) { famille = a.substr(9); continue; }
    if (a.rfind("--inject=", 0) == 0) {
      const std::string m = a.substr(9);
      if (m == "phi-large") mu = GwMutant::kPhiLarge;
      else if (m == "dead-large") mu = GwMutant::kDeadLarge;
      else if (m == "all-strict-lache") mu = GwMutant::kAllStrictLache;
      else if (m == "juge-compense") mu = GwMutant::kJugeCompense;
      else { std::fprintf(stderr, "REFUS : mutant inconnu %s\n", m.c_str()); return 2; }
      continue;
    }
    std::fprintf(stderr, "REFUS : argument inconnu %s\n", a.c_str());
    return 2;
  }
  // ---- DEUX CONTRE-FIXTURES GRAVEES, ET ELLES REFUTENT MES DEUX PHRASES.
  //
  // 1. L'ANNEAU : une ancre `W_4`-vivante peut porter `Theta(n)` porteurs.
  //
  //    `a = (1000,1000,1000)`, `b = (1020,1000,1000)`, et tous les entiers
  //    `x = (1010, 1000+u, 1000+v)` avec `100 < u^2+v^2 < 300`. Dans le plan
  //    mediateur, `E = X = R^2 + s` et `H = R^2 - s` avec `R = 10` : pour
  //    `R^2 < s < 3R^2` on a `E < D`, `X < D` et `H < 0`. Donc `ab` est l'arete
  //    maximale STRICTE et `abx` est un porteur aigu — mais `H < 0` place `x`
  //    HORS de la boule diametrale, donc hors de `W_2`, donc hors de `W_4`.
  //
  //    Les deux regions sont GEOMETRIQUEMENT DISJOINTES : `W_4` vit du cote
  //    `H > 0`, un porteur du cote `H < 0`. Le rapport de volumes
  //    `|L|/|W_4| = 10,86` controle donc une MOYENNE sous homogeneite, et ne
  //    domine aucune cardinalite. Ma phrase « porteurs = `O(h)` » etait une
  //    borne en esperance deguisee en borne deterministe.
  //
  //    Mesure : `632` points, `632` porteurs, `0` temoin `W_2`/`W_3`/`W_4`.
  //
  // 2. LE TETRAEDRE REGULIER : le cœur de Jung ne contient PAS le quatrieme
  //    sommet. Sur les quatre sommets alternes de `{0,2}^3`, le seed `(a,b,x)`
  //    a pour circumcentre plan `c0 = (2/3, 2/3, 4/3)` et
  //    `|y - c0|^2 = 16/3`, contre un rayon de cœur `(|ab|/4)^2 = 1/2` — un
  //    facteur `32/3`. Le cœur sert a tuer un seed par INTERIEURS PERMANENTS,
  //    jamais a produire l'apex, qui vit sur le SHELL d'une sphere.
  if (mode_fixture) {
    const i64 a[3] = {1000, 1000, 1000}, b[3] = {1020, 1000, 1000};
    i64 D = 0;
    for (int k = 0; k < 3; ++k) D += (a[k] - b[k]) * (a[k] - b[k]);
    long long pts_anneau = 0, porteurs = 0, w2 = 0, w3 = 0, w4 = 0;
    for (i64 u = -20; u <= 20; ++u)
      for (i64 v = -20; v <= 20; ++v) {
        const i64 sq = u * u + v * v;
        if (!(sq > 100 && sq < 300)) continue;
        ++pts_anneau;
        const i64 x[3] = {1010, 1000 + u, 1000 + v};
        i64 E = 0, X = 0, H = 0;
        for (int k = 0; k < 3; ++k) {
          E += (x[k] - a[k]) * (x[k] - a[k]);
          X += (b[k] - x[k]) * (b[k] - x[k]);
          H += (x[k] - a[k]) * (b[k] - x[k]);
        }
        if (E <= D && X <= D && H < 0) ++porteurs;
        if (H > 0) {
          ++w2;
          if ((i128)4 * H * H > (i128)E * X) ++w3;
          if ((i128)3 * H * H > (i128)E * X) ++w4;
        }
      }
    std::printf("anneau points=%lld carriers=%lld W2=%lld W3=%lld W4=%lld\n",
                pts_anneau, porteurs, w2, w3, w4);
    // Le tetraedre regulier, en entiers quadruples pour eviter les fractions :
    // `c0 = (2/3,2/3,4/3)` donc `3 c0 = (2,2,4)`, et on compare
    // `|3y - 3c0|^2` a `9 (|ab|/4)^2 = 9 D / 16`.
    {
      const i64 y3[3] = {6, 6, 0}, c3[3] = {2, 2, 4};  // `3y` et `3 c0`
      i64 dd = 0;
      for (int k = 0; k < 3; ++k) dd += (y3[k] - c3[k]) * (y3[k] - c3[k]);
      const i64 Dt = 8;  // `|ab|^2` du tetraedre regulier
      // `|y-c0|^2 > (|ab|/4)^2`  <=>  `16 dd > 9 Dt`
      std::printf("tetra_regulier 16_dy2=%lld 9_D=%lld apex_hors_coeur=%d\n",
                  (long long)(16 * dd), (long long)(9 * Dt), 16 * dd > 9 * Dt ? 1 : 0);
      if (!(16 * dd > 9 * Dt)) {
        std::fprintf(stderr, "PLANCHER : l'apex du tetraedre regulier serait dans le cœur\n");
        return 3;
      }
    }
    if (pts_anneau != 632 || porteurs != 632 || w2 != 0 || w3 != 0 || w4 != 0) {
      std::fprintf(stderr, "PLANCHER : anneau %lld pts, %lld carriers, W4=%lld\n",
                   pts_anneau, porteurs, w4);
      return 3;
    }
    std::printf("contre_fixtures ancre_vivante_carriers=632 temoins_W4=0 "
                "apex_hors_coeur=1 borne_O_h=REFUTEE\n");
    return 0;
  }
  if (!mode_oracle && !mode_sparse) {
    std::fprintf(stderr, "REFUS : choisir --fixtures, --oracle ou --sparse\n");
    return 2;
  }
  // L'oracle est CUBIQUE en la masse des boites : `(2e+1)^9` triplets au pire.
  // Une etendue de 3 donne deja 40 353 607 triplets par bloc.
  if (etendue < 0 || etendue > 3) {
    std::fprintf(stderr, "REFUS : --etendue hors [0,3], l'oracle serait non borne\n");
    return 2;
  }
  if (tours < 1 || tours > 200000) {
    std::fprintf(stderr, "REFUS : --tours hors [1,200000]\n");
    return 2;
  }
  if (n < 4 || n > 65535) {
    std::fprintf(stderr, "REFUS : n hors profil u16\n");
    return 2;
  }
  // La force brute est en `C(n,3) x n` ; elle est bornee explicitement.
  if (mode_brute && n > 400) {
    std::fprintf(stderr, "REFUS : --brute borne a 400 points, n=%d\n", n);
    return 2;
  }
  if (mu != GwMutant::kNone && !mode_oracle && !mode_sparse) {
    std::fprintf(stderr, "REFUS : un mutant sans juge ne prouve rien\n");
    return 2;
  }

  std::printf("AcuteOwnerGatewayReceipt-v1\n");
  std::printf("cadre phase=exploration_v3_hors_registre backend=cpu_reference "
              "profile=quantized_u16_input_only mode=diagnostic_counter_only "
              "public_status=not_claimed\n");

  if (mode_oracle) {
    rng_etat = (unsigned long long)seed * 0x9E3779B97F4A7C15ULL + 1;
    const BilanOracle g = oracle(tours, etendue, mu, biais);
    std::printf("oracle blocs=%lld dead=%lld all_strict=%lld mixed=%lld "
                "dead_faux=%lld all_faux=%lld mixed_tout_porteur=%lld "
                "mixed_aucun_porteur=%lld\n",
                g.blocs, g.dead, g.all_strict, g.mixed, g.dead_faux, g.all_faux,
                g.mixed_tout_porteur, g.mixed_aucun_porteur);
    // ---- LA SURETE EST LA SEULE CHOSE NON NEGOCIABLE.
    if (g.dead_faux > 0 || g.all_faux > 0) {
      if (mu != GwMutant::kNone) {
        std::fprintf(stderr, "MUTANT TUE : %s produit %lld DEAD faux et %lld "
                     "ALL_STRICT faux\n", mu == GwMutant::kPhiLarge ? "phi-large"
                     : (mu == GwMutant::kDeadLarge ? "dead-large" : "all-strict-lache"),
                     g.dead_faux, g.all_faux);
        return 4;
      }
      std::fprintf(stderr, "DESACCORD DU JUGE : %lld DEAD faux, %lld ALL_STRICT faux\n",
                   g.dead_faux, g.all_faux);
      return 1;
    }
    // ---- DEUX ESPECES DE MUTANTS, ET IL NE FAUT PAS LES CONFONDRE.
    //
    // `dead-large` et `all-strict-lache` rendent le classifieur FAUX : ils se
    // tuent par `dead_faux` ou `all_faux`, ci-dessus.
    //
    // `phi-large` ne ment pas — il rend `MIXED` la ou `DEAD` etait justifie,
    // donc il reste SUR et seulement plus cher. Le declarer « survivant »
    // serait une erreur de categorie. Il se tue par PERTE DE COUVERTURE : le
    // nombre de blocs certifies morts doit chuter par rapport au plancher.
    if (mu == GwMutant::kPhiLarge) {
      if (g.dead >= min_dead && min_dead > 0) {
        std::fprintf(stderr, "MUTANT SURVIVANT : phi-large garde %lld DEAD, plancher %d\n",
                     g.dead, min_dead);
        return 3;
      }
      std::fprintf(stderr, "MUTANT TUE : phi-large tombe a %lld DEAD sous le plancher %d\n",
                   g.dead, min_dead);
      return 4;
    }
    if (mu != GwMutant::kNone) {
      std::fprintf(stderr, "MUTANT SURVIVANT : il n'a pas ete vu\n");
      return 3;
    }
    // ---- LES PLANCHERS, contre le vert par vacuite.
    if (g.dead < min_dead || g.all_strict < min_all) {
      std::fprintf(stderr, "PLANCHER : dead=%lld < %d ou all_strict=%lld < %d\n",
                   g.dead, min_dead, g.all_strict, min_all);
      return 3;
    }
    std::printf("oracle verdict=SUR planchers=OK\n");
  }

  if (mode_sparse) {
    const int coord = mhgp3v::cloud_family_default_coord(
        famille == "two_lines" ? mhgp3v::CloudFamily::kTwoLines
        : famille == "terrain" ? mhgp3v::CloudFamily::kTerrain
        : famille == "eight_clusters" ? mhgp3v::CloudFamily::kEightClusters
                                      : mhgp3v::CloudFamily::kUniform, n);
    const mhgp3v::CloudFamily fam =
        famille == "two_lines" ? mhgp3v::CloudFamily::kTwoLines
        : famille == "terrain" ? mhgp3v::CloudFamily::kTerrain
        : famille == "eight_clusters" ? mhgp3v::CloudFamily::kEightClusters
        : famille == "uniform" ? mhgp3v::CloudFamily::kUniform
                               : mhgp3v::CloudFamily::kUniform;
    if (famille != "two_lines" && famille != "terrain" && famille != "uniform" &&
        famille != "eight_clusters") {
      std::fprintf(stderr, "REFUS : famille inconnue %s\n", famille.c_str());
      return 2;
    }
    const auto brut = mhgp3v::make_family_cloud(fam, n, coord, seed);
    const int m = (int)brut.size();
    if (m != n) {
      std::fprintf(stderr, "REFUS : le generateur a rendu %d points pour --points=%d\n", m, n);
      return 2;
    }
    std::vector<unsigned long long> keys(m);
    std::vector<int> order(m);
    for (int i = 0; i < m; ++i) {
      keys[i] = mhgp3v::wf_morton48(brut[i].x, brut[i].y, brut[i].z);
      order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](int i, int j) {
      return keys[i] != keys[j] ? keys[i] < keys[j] : i < j;
    });
    std::vector<unsigned long long> sk(m);
    std::vector<P3> pts(m);
    std::vector<int> pid(m);
    for (int i = 0; i < m; ++i) {
      sk[i] = keys[order[i]];
      pts[i] = P3{(short)brut[order[i]].x, (short)brut[order[i]].y, (short)brut[order[i]].z};
      pid[i] = order[i];
    }
    std::vector<WfNode> nodes = mhgp3v::wf_build(sk);
    {
      std::vector<std::array<long long, 3>> raw(m);
      for (int i = 0; i < m; ++i) raw[i] = {pts[i].x, pts[i].y, pts[i].z};
      mhgp3v::wf_tight_boxes(&nodes, raw);
    }
    auto premier = [&](int h) { return h >= 0 ? nodes[h].first : -1 - h; };
    auto dernier = [&](int h) { return h >= 0 ? nodes[h].last : -1 - h; };
    auto pop = [&](int h) { return dernier(h) - premier(h) + 1; };
    auto feuille = [&](int h) { return h < 0; };
    auto boite = [&](int h) {
      BoxI B{};
      if (h >= 0) {
        // `tlo/thi` EST LA BOITE SERREE. `lo/hi` est la CELLULE de Morton,
        // alignee sur un prefixe de cle — beaucoup plus large, et surtout
        // FAUSSE des que le nuage sort du profil u16, puisque l'encodage
        // suppose des coordonnees positives sur seize bits. Ma premiere version
        // lisait `lo/hi`, et `two_lines` — qui produit `z = -1` — rendait alors
        // deux blocs `ALL_STRICT` dont AUCUN des quatre triples n'etait porteur.
        for (int k = 0; k < 3; ++k) { B.ax[k].lo = nodes[h].tlo[k]; B.ax[k].hi = nodes[h].thi[k]; }
      } else {
        const int i = -1 - h;
        B.ax[0] = {pts[i].x, pts[i].x};
        B.ax[1] = {pts[i].y, pts[i].y};
        B.ax[2] = {pts[i].z, pts[i].z};
      }
      return B;
    };

    // ---- LA RECURSION TERNAIRE.
    //
    // On descend sur les TROIS boites. Un bloc mort n'expanse rien ; un bloc
    // `ALL_STRICT` est emis SYMBOLIQUEMENT — sa masse est comptee, ses triplets
    // ne sont jamais formes ; seuls les blocs indecis descendent.
    //
    // `pairid_expanded` ne s'incremente qu'au niveau des FEUILLES, c'est-a-dire
    // au seul endroit ou une paire `(a,b)` existe reellement en memoire. C'est
    // le compteur que `two_lines` doit laisser a zero.
    BilanSparse g;
    // ---- LA TACHE PORTE SON LEDGER, ET C'EST CE QUI EVITE DE TOUT RECALCULER.
    //
    // `L4` est le nombre d'IDs universellement `W_4`-interieurs pour TOUTE paire
    // du rectangle `A x B`. Raffiner `A` ou `B` AFFAIBLIT le « pour toute
    // paire », donc `L4` ne peut que CROITRE : le credit acquis est definitif et
    // s'herite. Seule la FRONTIERE indecise se re-teste chez les enfants.
    //
    // `U4 = L4 + masse de la frontiere` majore le compte vrai. D'ou les deux
    // seuils de la section 3.2 de l'audit :
    //
    //   `L4 >= r4`  -> toutes les paires du bloc sont mortes ;
    //   `U4 <  r4`  -> toutes les paires du bloc sont `W_4`-vivantes.
    //
    // La frontiere est une liste de handles de nœuds, jamais de points : c'est
    // elle qui remplace le CSR de sites par seed que l'audit refuse.
    struct Tache {
      int A, B, C;
      long long L4;
      std::vector<int> frontiere;
      // ---- LE LEDGER NE DEPEND QUE DE `(A,B)`.
      //
      // Scinder `C` ne change ni `L4_open`, ni la frontiere : refaire la passe
      // ledger dans ce cas est du travail PUR. Ma premiere version le faisait, et
      // le coût passait de `47,9 M` a `158 M` nœuds sur `terrain` a `n=800`.
      // `ab_neuf` dit si `A` ou `B` vient de changer ; sinon on herite tel quel.
      //
      // C'est aussi la forme GPU que l'audit demande : le cover spatial est
      // construit UNE FOIS par arete owner et partage par ses seeds.
      bool ab_neuf;
      long long U4;
      bool tronquee;
      int rect;  // rectangle WSPD d'origine, pour compter les residuels UNE fois
      bool front2;  // second front : le raffinement de `(A,B)` est autorise
    };
    std::vector<Tache> st;
    const int racine = nodes.empty() ? -1 : 0;
    // ---- LA SOURCE EST LA PARTITION WSPD, PAS LA RACINE.
    //
    // Partir de `(racine, racine, racine)` rendait le ledger structurellement
    // inerte : la racine contient `A`, donc aucun point n'est jamais disjoint de
    // `A` et de `B`, donc `L4_open` reste a zero. On construit donc d'abord les
    // rectangles SEPARES — la meme descente que le prefiltre combine — et on
    // lance une recursion ternaire par rectangle, `C` partant de la racine.
    //
    // Le recouvrement reste exact : la WSPD couvre chaque paire d'indices
    // exactement une fois, et le juge par force brute le verifie.
    long long rectangles = 0;
    {
      struct R { int u, v; };
      std::vector<R> pile;
      for (size_t i = 0; i < nodes.size(); ++i) pile.push_back({nodes[i].left, nodes[i].right});
      if (nodes.empty() && m == 1) { /* un point : aucune paire */ }
      while (!pile.empty()) {
        const R r = pile.back();
        pile.pop_back();
        const bool fu = feuille(r.u), fv = feuille(r.v);
        if (separes(sphere_de(boite(r.u)), sphere_de(boite(r.v)), sep) || (fu && fv)) {
          ++rectangles;
          Tache t0{r.u, r.v, racine, 0, {}, true, 0, false, (int)rectangles - 1, false};
          t0.frontiere.push_back(racine);
          st.push_back(std::move(t0));
          continue;
        }
        const bool su = !fu && (fv || pop(r.u) >= pop(r.v));
        if (su) {
          pile.push_back({nodes[r.u].left, r.v});
          pile.push_back({nodes[r.u].right, r.v});
        } else {
          pile.push_back({r.u, nodes[r.v].left});
          pile.push_back({r.u, nodes[r.v].right});
        }
      }
    }
    std::vector<unsigned char> rect_residuel((size_t)rectangles, 0);
    bool front2_fait = false;
    // ---- COMPTABILITE PAR RECTANGLE, POUR DEFALQUER.
    //
    // Le front 1 emet des porteurs pour des rectangles qui se reveleront
    // RESIDUELS, et que le front 2 rejouera : sans defalcation, ils sont
    // comptes deux fois. L'exces passait de `1,180` a `1,674` sur `terrain`,
    // et c'est le juge qui l'a montre.
    std::vector<long long> carr_par_rect((size_t)rectangles, 0);
    // ---- LES CLES, POUR UN JUGE PAR IDENTITES ET NON PAR CARDINAL.
    //
    // `sparse >= brute` accepte qu'une incidence VRAIE manquante soit compensee
    // par une incidence surnumeraire venue d'une ancre morte. L'audit `79e73b6`
    // a raison de le refuser. On collecte donc la cle
    // `(EdgeKey(a,b), PointId(x))` des deux cotes et on compare les ENSEMBLES.
    //
    // L'expansion des blocs symboliques n'a lieu QUE dans ce mode : le juge a le
    // droit d'etre cher, le chemin nominal non.
    std::vector<long long> cles_sparse;
    auto cle_de = [&](int ia, int ib, int ic) {
      const long long u = pid[ia] < pid[ib] ? pid[ia] : pid[ib];
      const long long v = pid[ia] < pid[ib] ? pid[ib] : pid[ia];
      return (u * 100000LL + v) * 100000LL + pid[ic];
    };
    // Cap de frontiere : un depassement rend `PENDING`, jamais `DEAD`.
    const int kCapFrontiere = 64;
    // ---- L'AUTO-JOINTURE EN PAS CADENCE, ET POURQUOI ELLE EST NECESSAIRE.
    //
    // Partir de `(racine, racine, racine)` sans precaution fait visiter chaque
    // paire de blocs DEUX FOIS — `(A,B,C)` et `(B,A,C)`. Le juge l'a vu tout de
    // suite : `sparse` valait exactement `2 x brute` sur les trois familles
    // denses. Au niveau des feuilles l'orientation `pid` reglait le cas, mais
    // les blocs `ALL_STRICT` comptaient leur masse deux fois.
    //
    // J'ai d'abord essaye d'elaguer la moitie mal ordonnee. C'etait FAUX, et le
    // juge l'a vu aussi : on passe alors de `+2 x` a `-13 %`, parce qu'un nœud
    // peut etre l'ANCETRE de l'autre, et couper sur `premier(A) > premier(B)`
    // supprime des descendants legitimes.
    //
    // La regle correcte maintient l'INVARIANT « `A == B`, ou `A` et `B`
    // disjoints avec `A` avant `B` ». Les nœuds d'un octree etant laminaires,
    // scinder `A == B` en TROIS — `(A.g,A.g)`, `(A.g,A.d)`, `(A.d,A.d)`, la
    // diagonale inverse etant omise — le preserve, et chaque paire non ordonnee
    // n'est alors visitee qu'une fois.
  boucle:
    while (!st.empty()) {
      const Tache t = std::move(st.back());
      st.pop_back();
      ++g.noeuds;
      const BoxI BA = boite(t.A), BB = boite(t.B), BC = boite(t.C);

      // ---- PASSE LEDGER : SEULEMENT SI `(A,B)` A CHANGE.
      long long L4 = t.L4;
      bool tronquee = t.tronquee;
      std::vector<int> frontiere;
      if (!t.ab_neuf) {
        frontiere = t.frontiere;  // herite tel quel : `C` seul a bouge
      } else {
        // LA TRONCATURE SE REEVALUE. Elle etait COLLANTE dans ma premiere
        // version : une fois vraie, `U4` restait infini pour toute la
        // descendance, `ALL_STRICT` ne pouvait plus se declencher et la
        // recursion descendait jusqu'aux feuilles — `3,4` milliards de nœuds a
        // `n=800` sur `terrain`, contre `158` millions. Raffiner `(A,B)` reduit
        // la frontiere ; il n'y a aucune raison de garder l'aveu d'ignorance.
        tronquee = false;
        std::vector<int> pile = t.frontiere;
        while (!pile.empty() && (int)frontiere.size() <= kCapFrontiere) {
          const int h = pile.back();
          pile.pop_back();
          const int pf = premier(h), pl = dernier(h);
          // ---- LE MASQUE ENDPOINT EST RELATIONNEL, PAS GEOMETRIQUE.
          //
          // Ma version precedente SUPPRIMAIT definitivement un span des qu'il
          // recouvrait `A` ou `B`. C'est faux, et l'audit `79e73b6` a raison :
          // un `z` de `A` est endpoint pour CERTAINES paires de `A x B`, mais
          // il reste un temoin possible pour toute paire `(a,b)` avec `a != z`.
          // Apres restriction de `A` a un enfant qui ne le contient plus, il
          // doit REDEVENIR un temoin ordinaire.
          //
          // La regle correcte : jamais credite au minorant, CONSERVE dans le
          // majorant, et rejoue apres toute restriction de `A` ou de `B`. On
          // garde donc le span avec son masque au lieu de le jeter.
          const bool ov_a = !(pl < premier(t.A) || pf > dernier(t.A));
          const bool ov_b = !(pl < premier(t.B) || pf > dernier(t.B));
          if (ov_a || ov_b) {
            if (!feuille(h)) {
              pile.push_back(nodes[h].left);
              pile.push_back(nodes[h].right);
            } else {
              // Feuille endpoint : elle n'est PAS creditee, mais elle reste
              // dans la frontiere pour etre rejouee chez les enfants.
              ++g.relation_spans;
              frontiere.push_back(h);
            }
            continue;
          }
          const BoxI BZ = boite(h);
          if (bloc_tout_w4(BA, BB, BZ, &g.noeuds)) { L4 += pl - pf + 1; ++g.l4_credits; continue; }
          if (bloc_aucun_w2(extrema(BA, BB, BZ))) continue;  // hors `W_2`, donc hors `W_4`
          if (feuille(h)) { frontiere.push_back(h); continue; }
          pile.push_back(nodes[h].left);
          pile.push_back(nodes[h].right);
        }
        if (!pile.empty()) {
          // ---- FRONTIERE TRONQUEE : ON NE PERD RIEN.
          //
          // Ma premiere version abandonnait la tache. C'etait une PERTE SECHE de
          // porteurs — le juge l'a lue en `ecart` negatif. Or les credits deja
          // acquis restent VRAIS (`L4_open` est un minorant certain), et il
          // suffit de rendre `U4` inconnu pour interdire `ACTIVE_ALL` sans
          // jamais tuer. On reverse donc le reste de la pile dans la frontiere
          // et on continue : `PENDING` compte l'evenement, il ne jette plus.
          ++g.pending;
          for (int h : pile) frontiere.push_back(h);
          tronquee = true;
        }
      }
      if ((long long)frontiere.size() > g.frontiere_max)
        g.frontiere_max = (long long)frontiere.size();
      // Frontiere tronquee : `U4` est INCONNU, donc majore par l'infini —
      // `ACTIVE_ALL` ne peut pas se declencher, et rien n'est tue pour autant.
      const long long U4 = t.ab_neuf
                               ? (tronquee ? (long long)1 << 60
                                           : L4 + (long long)frontiere.size())
                               : t.U4;

      const Extrema ex = extrema(BA, BB, BC);
      const VerdictConjoint vc = classifie_conjoint(ex, L4, U4, r4, mu);
      if (vc == VerdictConjoint::kDeadW4) { ++g.dead_w4; continue; }
      const Verdict v = classifie(ex, mu);
      if (v == Verdict::kDeadPhi) { ++g.dead_phi; continue; }
      if (v == Verdict::kDeadOwnerE) { ++g.dead_e; continue; }
      if (v == Verdict::kDeadOwnerX) { ++g.dead_x; continue; }
      if (vc == VerdictConjoint::kActiveAll) {
        // `ActiveOwnerEdgeBlock` : ni `PairId`, ni face materialisee.
        ++g.active_edge;
      }
      // Une ancre du bloc peut etre morte sans que le bloc entier le soit :
      // `MIXED` doit alors descendre, et c'est le cas nominal.
      if (v == Verdict::kAllStrict) {
        // CARRIER BLOCK SYMBOLIQUE : on retient `(A,B,C)`, pas ses triplets.
        ++g.all_strict;
        // La masse compte des paires NON ORDONNEES : `A == B` donne
        // `C(|A|,2)`, pas `|A|^2`.
        const long long masse =
            (t.A == t.B) ? (long long)pop(t.A) * (pop(t.A) - 1) / 2 * pop(t.C)
                         : (long long)pop(t.A) * pop(t.B) * pop(t.C);
        g.masse_all_strict += masse;
        g.carriers_symboliques += masse;
        carr_par_rect[(size_t)t.rect] += masse;
        if (mode_brute) {
          for (int ia = premier(t.A); ia <= dernier(t.A); ++ia)
            for (int ib = premier(t.B); ib <= dernier(t.B); ++ib) {
              if (t.A == t.B && ia >= ib) continue;
              for (int ic = premier(t.C); ic <= dernier(t.C); ++ic)
                cles_sparse.push_back(cle_de(ia, ib, ic));
            }
        }
        if (mode_brute) {
          // VERIFICATION DU BLOC, point par point. Elle ne sert qu'au juge : un
          // bloc `ALL_STRICT` doit etre integralement porteur, et un ecart ici
          // LOCALISE la faute bien mieux qu'un total qui ne tombe pas.
          for (int ia = premier(t.A); ia <= dernier(t.A); ++ia)
            for (int ib = premier(t.B); ib <= dernier(t.B); ++ib) {
              if (t.A == t.B && ia >= ib) continue;  // une seule fois par paire
              for (int ic = premier(t.C); ic <= dernier(t.C); ++ic) {
                const bool ok =
                    pid[ia] < pid[ib]
                        ? porteur_canonique(pts[ia], pts[ib], pts[ic], pid[ia], pid[ib], pid[ic])
                        : porteur_canonique(pts[ib], pts[ia], pts[ic], pid[ib], pid[ia], pid[ic]);
                if (!ok) {
                  ++g.blocs_faux;
                  if (g.blocs_faux <= 4)
                    std::fprintf(stderr,
                        "BLOC FAUX a=(%d,%d,%d)#%d b=(%d,%d,%d)#%d x=(%d,%d,%d)#%d\n",
                        pts[ia].x, pts[ia].y, pts[ia].z, pid[ia],
                        pts[ib].x, pts[ib].y, pts[ib].z, pid[ib],
                        pts[ic].x, pts[ic].y, pts[ic].z, pid[ic]);
                }
              }
            }
        }
        continue;
      }
      const bool fa = feuille(t.A), fb = feuille(t.B), fc = feuille(t.C);
      if (fa && fb && fc) {
        ++g.feuilles;
        const int ia = premier(t.A), ib = premier(t.B), ic = premier(t.C);
        if (ia == ib || ia == ic || ib == ic) continue;
        // PAS DE SECOND FILTRE D'ORIENTATION ICI. L'invariant du pas cadence
        // garantit deja que la paire non ordonnee `{A,B}` n'est visitee qu'une
        // fois. J'avais laisse un `pid[ia] >= pid[ib] -> continue` en plus, et
        // il coupait les paires dont l'ordre Morton et l'ordre `pid` divergent :
        // le juge a lu `-196`, `-157` et `-45` porteurs manquants. On ORDONNE
        // l'appel au lieu de filtrer.
        ++g.pairid_expanded;
        ++g.seed3_emitted;
        const bool porte =
            pid[ia] < pid[ib]
                ? porteur_canonique(pts[ia], pts[ib], pts[ic], pid[ia], pid[ib], pid[ic])
                : porteur_canonique(pts[ib], pts[ia], pts[ic], pid[ib], pid[ia], pid[ic]);
        if (porte) {
          // ---- LE MUTANT QUI PROUVE LA FORCE DU JUGE.
          //
          // `juge-compense` OMET le premier porteur de feuille et EMET a la
          // place une cle bidon. Le CARDINAL reste exact — une omission, une
          // surprise — donc l'ancien juge `sparse >= brute` passe. Seul un juge
          // par IDENTITES voit `manquantes = 1` et `fausses = 1`.
          if (mu == GwMutant::kJugeCompense && g.carriers == 0 && mode_brute) {
            ++g.carriers;
            carr_par_rect[(size_t)t.rect] += 1;
            cles_sparse.push_back(cle_de(ia, ib, ic) + 1);  // cle decalee : bidon
            continue;
          }
          ++g.carriers;
          carr_par_rect[(size_t)t.rect] += 1;
          if (mode_brute) cles_sparse.push_back(cle_de(ia, ib, ic));
        }
        continue;
      }
      // On coupe la PLUS GROSSE des trois, ce qui fait decroitre le produit des
      // populations et garantit la terminaison.
      const int pa = fa ? 1 : pop(t.A), pb = fb ? 1 : pop(t.B), pc = fc ? 1 : pop(t.C);
      // ---- `(A,B)` FIGE : le rectangle WSPD EST deja la partition des paires.
      //
      // Le raffiner a nouveau refait le travail de la WSPD, et cela se multiplie
      // avec la descente de `C` : exposant `noeuds` `2,95` puis `4,73` sur
      // `terrain`. Ce mode l'interdit, et ne descend que `C`.
      //
      // J'avais REFUTE cette voie au `53815f` — « le certificat au niveau
      // rectangle n'elague que des feuilles, `1,1` point par bloc ». Cette
      // mesure est INVALIDE : elle datait d'avant le correctif `tlo/thi`, donc
      // elle jugeait le certificat sur des cellules de Morton alignees, bien
      // plus larges que les boites serrees. Il fallait la refaire.
      //
      // Un bloc `MIXED` dont `A` ou `B` n'est pas une feuille ne peut alors plus
      // etre decide : il devient un RESIDUEL, compte a part, que la phase
      // d'exactification par arete traitera. C'est la structure a deux fronts de
      // la section 8 de l'audit.
      // ---- DEUX FRONTS : figer d'abord, ne raffiner que le residuel.
      //
      // Les deux extremes sont mesures, et aucun ne marche seul :
      //
      //   `(A,B)` scindable  `terrain n=800` : `3 416 M` nœuds, exposant `4,73`,
      //                      mais `two_lines` tue TOUT sans une paire ;
      //   `(A,B)` fige       `54 M` nœuds, exposant `1,39` — soit `63x` moins —
      //                      mais `47,7 %` des paires restent residuelles, et
      //                      `65,9 %` sur `eight_clusters`.
      //
      // Le premier front fige `(A,B)` et ne descend que `C` : il tue `63` a
      // `82 %` des rectangles pour un coût sous-quadratique. Le second ne
      // raffine `(A,B)` que sur les rectangles qui ont SURVECU au premier. Le
      // coût du raffinement n'est donc paye que la ou il rapporte, ce qui est le
      // critere de scission de la section 3.4 de l'audit, sous sa forme la plus
      // simple : un seuil binaire au lieu d'un ratio continu.
      if (ab_fige && !(fa && fb) && !t.front2) {
        if (!fc) {
          st.push_back({t.A, t.B, nodes[t.C].left, L4, frontiere, false, U4, tronquee, t.rect, t.front2});
          st.push_back({t.A, t.B, nodes[t.C].right, L4, frontiere, false, U4, tronquee, t.rect, t.front2});
        } else {
          // `C` epuise et `(A,B)` non ponctuel : residuel a exactifier.
          //
          // LE COMPTE DOIT ETRE PAR RECTANGLE DISTINCT. Ma premiere version
          // ajoutait `|A| |B|` a chaque bloc residuel, donc le meme rectangle
          // etait compte une fois par feuille `C` atteinte : `residuel_paires`
          // valait `123 fois` `C(n,2)`, un nombre sans aucun sens.
          ++g.residuel_blocs;
          if (!rect_residuel[t.rect]) {
            rect_residuel[t.rect] = 1;
            ++g.residuel_rects;
            g.residuel_paires += (long long)pop(t.A) * pop(t.B);
          }
        }
        continue;
      }
      if (t.A == t.B && !fa) {
        // LA DIAGONALE, DEPLIEE EN TROIS. `(A.d, A.g)` est omise : c'est la
        // meme paire non ordonnee que `(A.g, A.d)`.
        const int g1 = nodes[t.A].left, d1 = nodes[t.A].right;
        st.push_back({g1, g1, t.C, L4, frontiere, true, U4, tronquee, t.rect, t.front2});
        st.push_back({g1, d1, t.C, L4, frontiere, true, U4, tronquee, t.rect, t.front2});
        st.push_back({d1, d1, t.C, L4, frontiere, true, U4, tronquee, t.rect, t.front2});
      } else if (t.A == t.B) {
        // `A == B` feuille : la seule paire possible est `(a,a)`, degeneree.
        // Il ne reste qu'a descendre `C`, ou a s'arreter.
        if (!fc) {
          st.push_back({t.A, t.B, nodes[t.C].left, L4, frontiere, false, U4, tronquee, t.rect, t.front2});
          st.push_back({t.A, t.B, nodes[t.C].right, L4, frontiere, false, U4, tronquee, t.rect, t.front2});
        }
      } else if (!fa && pa >= pb && pa >= pc) {
        st.push_back({nodes[t.A].left, t.B, t.C, L4, frontiere, true, U4, tronquee, t.rect, t.front2});
        st.push_back({nodes[t.A].right, t.B, t.C, L4, frontiere, true, U4, tronquee, t.rect, t.front2});
      } else if (!fb && pb >= pc) {
        st.push_back({t.A, nodes[t.B].left, t.C, L4, frontiere, true, U4, tronquee, t.rect, t.front2});
        st.push_back({t.A, nodes[t.B].right, t.C, L4, frontiere, true, U4, tronquee, t.rect, t.front2});
      } else if (!fc) {
        st.push_back({t.A, t.B, nodes[t.C].left, L4, frontiere, false, U4, tronquee, t.rect, t.front2});
        st.push_back({t.A, t.B, nodes[t.C].right, L4, frontiere, false, U4, tronquee, t.rect, t.front2});
      }
    }
    // ---- L'ORACLE AU NIVEAU NUAGE, qui juge la recursion entiere.
    //
    // La sûrete du classifieur est etablie sur des boites ; elle ne dit rien de
    // la RECURSION — un bug de descente, de coupe ou de double comptage y
    // echapperait entierement. On enumere donc tous les triples, une fois, sous
    // owner canonique, et on exige l'egalite A L'UNITE avec
    // `carriers + carriers_symboliques`.
    //
    // Coût `C(n,3)`, donc borne a petit `n` : c'est un juge, pas un chemin.
    long long ref_brute = -1;
    std::vector<long long> cles_brute;
    if (mode_brute) {
      ref_brute = 0;
      // ---- LE JUGE MESURE LA CONJONCTION, ET C'EST TOUT L'INTERET.
      //
      // Ma premiere version comptait TOUS les triangles aigus. Le classifieur
      // conjoint retire a dessein ceux des ancres `W_4`-MORTES, donc les deux ne
      // comparaient pas le meme objet et l'ecart etait negatif par construction.
      // Le juge applique donc lui aussi le filtre de vivacite, exactement :
      // `|P inter W_4(a,b)| < r4`, compte sur tous les points.
      for (int i = 0; i < m; ++i)
        for (int j = i + 1; j < m; ++j) {
          const int ia = pid[i] < pid[j] ? i : j, ib = pid[i] < pid[j] ? j : i;
          // Compte EXACT des temoins `W_4` de l'ancre.
          int w4 = 0;
          for (int k = 0; k < m && w4 < r4; ++k) {
            if (k == ia || k == ib) continue;
            const i64 e[3] = {(i64)pts[k].x - pts[ia].x, (i64)pts[k].y - pts[ia].y,
                              (i64)pts[k].z - pts[ia].z};
            const i64 t2[3] = {(i64)pts[ib].x - pts[k].x, (i64)pts[ib].y - pts[k].y,
                               (i64)pts[ib].z - pts[k].z};
            const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
            if (e2 == 0) continue;
            const i64 h = e[0] * t2[0] + e[1] * t2[1] + e[2] * t2[2];
            if (h <= 0) continue;
            const i64 tt = t2[0] * t2[0] + t2[1] * t2[1] + t2[2] * t2[2];
            if ((i128)3 * (i128)h * (i128)h > (i128)e2 * (i128)tt) ++w4;
          }
          if (w4 >= r4) continue;  // ancre morte : ses porteurs ne comptent pas
          for (int k = 0; k < m; ++k) {
            if (k == ia || k == ib) continue;
            if (porteur_canonique(pts[ia], pts[ib], pts[k], pid[ia], pid[ib], pid[k])) {
              ++ref_brute;
              cles_brute.push_back(cle_de(ia, ib, k));
            }
          }
        }
    }
    // ---- SECOND FRONT : on relance les seuls rectangles residuels, avec le
    // raffinement de `(A,B)` autorise.
    // UNE SEULE FOIS. Sans ce garde le `goto` se re-arme et la boucle ne
    // termine jamais : elle a tourne dix minutes avant que je le voie.
    if (ab_fige && deux_fronts && !front2_fait) {
      front2_fait = true;
      g.front2_rects = g.residuel_rects;
      // DEFALCATION : tout ce que le front 1 a emis pour un rectangle residuel
      // est retire, puisque le front 2 va le recalculer entierement.
      for (size_t i = 0; i < rect_residuel.size(); ++i)
        if (rect_residuel[i]) {
          g.carriers_symboliques -= carr_par_rect[i];
          carr_par_rect[i] = 0;
        }
      // Les porteurs de feuilles sont dans `carriers`, pas dans
      // `carriers_symboliques` ; on les a defalques ensemble ci-dessus, donc on
      // recale en reportant tout sur le symbolique — le total seul est juge.
      g.carriers = 0;
      std::vector<Tache> st2;
      {
        struct R { int u, v; };
        std::vector<R> pile;
        for (size_t i = 0; i < nodes.size(); ++i) pile.push_back({nodes[i].left, nodes[i].right});
        long long idx = 0;
        while (!pile.empty()) {
          const R r = pile.back();
          pile.pop_back();
          const bool fu = feuille(r.u), fv = feuille(r.v);
          if (separes(sphere_de(boite(r.u)), sphere_de(boite(r.v)), sep) || (fu && fv)) {
            if (rect_residuel[(size_t)idx]) {
              Tache t0{r.u, r.v, racine, 0, {}, true, 0, false, (int)idx, true};
              t0.frontiere.push_back(racine);
              st2.push_back(std::move(t0));
            }
            ++idx;
            continue;
          }
          const bool su = !fu && (fv || pop(r.u) >= pop(r.v));
          if (su) { pile.push_back({nodes[r.u].left, r.v}); pile.push_back({nodes[r.u].right, r.v}); }
          else { pile.push_back({r.u, nodes[r.v].left}); pile.push_back({r.u, nodes[r.v].right}); }
        }
      }
      st.swap(st2);
      g.residuel_blocs = 0;
      g.residuel_rects = 0;
      g.residuel_paires = 0;
      std::fill(rect_residuel.begin(), rect_residuel.end(), 0);
      goto boucle;
    }
    std::printf("sparse famille=%s n=%d noeuds=%lld dead_phi=%lld dead_owner_e=%lld "
                "dead_owner_x=%lld all_strict=%lld masse_all_strict=%lld "
                "feuilles=%lld pairid_expanded=%lld carriers=%lld "
                "carriers_symboliques=%lld blocs_faux=%lld dead_w4=%lld active_edge=%lld "
                "seed3_emitted=%lld pending=%lld l4_credits=%lld frontiere_max=%lld "
                "rectangles=%lld residuel_blocs=%lld residuel_rects=%lld residuel_paires=%lld "
                "front2_rects=%lld relation_spans=%lld\n",
                famille.c_str(), n, g.noeuds, g.dead_phi, g.dead_e, g.dead_x,
                g.all_strict, g.masse_all_strict, g.feuilles, g.pairid_expanded,
                g.carriers, g.carriers_symboliques, g.blocs_faux, g.dead_w4,
                g.active_edge, g.seed3_emitted, g.pending, g.l4_credits,
                g.frontiere_max, rectangles, g.residuel_blocs, g.residuel_rects,
                g.residuel_paires, g.front2_rects, g.relation_spans);
    if (mode_brute) {
      const long long total = g.carriers + g.carriers_symboliques;
      // ---- LE SENS DE L'ECART, ET POURQUOI IL N'EST PLUS ZERO.
      //
      // La source conjointe est FAIL-OPEN : `L4_open >= r4` exige la mort pour
      // TOUTES les paires du bloc, donc un bloc peut contenir des ancres mortes
      // sans etre tue. Elle rend donc un MAJORANT du compte vrai, et c'est le
      // contrat. Le juge exige `sparse >= brute` — l'inegalite inverse serait
      // une fermeture fausse, le defaut le plus grave possible — et publie
      // l'exces, qui mesure le mou du classifieur conjoint.
      // ---- LE JUGE PAR IDENTITES.
      //
      // Trois fautes distinctes, qu'un cardinal confond :
      //   `manquantes` : une incidence VRAIE absente du sparse — FERMETURE
      //                  FAUSSE, la faute la plus grave ;
      //   `doublons`   : la meme cle emise deux fois — le recouvrement exact-once
      //                  est casse ;
      //   `fausses`    : une cle emise qui n'est PAS un porteur — le certificat
      //                  symbolique ment.
      // Et une quantite qui n'est PAS une faute, publiee a part :
      //   `surcouverture` : un porteur reel dont l'ancre est morte. La source
      //                  etant fail-open au niveau bloc, elle en emet.
      std::sort(cles_sparse.begin(), cles_sparse.end());
      long long doublons = 0;
      for (size_t i = 1; i < cles_sparse.size(); ++i)
        if (cles_sparse[i] == cles_sparse[i - 1]) ++doublons;
      std::vector<long long> uniq = cles_sparse;
      uniq.erase(std::unique(uniq.begin(), uniq.end()), uniq.end());
      long long manquantes = 0;
      for (long long k : cles_brute)
        if (!std::binary_search(uniq.begin(), uniq.end(), k)) ++manquantes;
      std::sort(cles_brute.begin(), cles_brute.end());
      long long fausses = 0, surcouverture = 0;
      for (long long k : uniq) {
        if (std::binary_search(cles_brute.begin(), cles_brute.end(), k)) continue;
        // Hors de la verite conjointe : porteur d'ancre morte, ou pas porteur ?
        const int px = (int)(k % 100000);
        const long long e = k / 100000;
        const int pv = (int)(e % 100000), pu = (int)(e / 100000);
        int ia = -1, ib = -1, ic = -1;
        for (int i = 0; i < m; ++i) {
          if (pid[i] == pu) ia = i;
          if (pid[i] == pv) ib = i;
          if (pid[i] == px) ic = i;
        }
        if (ia >= 0 && ib >= 0 && ic >= 0 &&
            porteur_canonique(pts[ia], pts[ib], pts[ic], pid[ia], pid[ib], pid[ic]))
          ++surcouverture;
        else
          ++fausses;
      }
      std::printf("identites cles_sparse=%zu uniques=%zu cles_brute=%zu "
                  "manquantes=%lld doublons=%lld fausses=%lld surcouverture=%lld\n",
                  cles_sparse.size(), uniq.size(), cles_brute.size(),
                  manquantes, doublons, fausses, surcouverture);
      if (manquantes > 0 || doublons > 0 || fausses > 0) {
        if (mu != GwMutant::kNone) {
          std::fprintf(stderr, "MUTANT TUE : manquantes=%lld doublons=%lld fausses=%lld\n",
                       manquantes, doublons, fausses);
          return 4;
        }
        std::fprintf(stderr,
                     "DESACCORD DU JUGE : manquantes=%lld doublons=%lld fausses=%lld\n",
                     manquantes, doublons, fausses);
        return 1;
      }
      const double exces = ref_brute > 0 ? (double)total / (double)ref_brute : 0.0;
      std::printf("juge brute=%lld sparse=%lld ecart=%lld exces=%.3f\n",
                  ref_brute, total, total - ref_brute, exces);
      if (total < ref_brute || g.blocs_faux > 0) {
        // CONVENTION v3 : un ecart SOUS MUTANT est un mutant tue (4), un ecart
        // sans mutant est un desaccord du juge (1).
        if (mu != GwMutant::kNone) {
          std::fprintf(stderr, "MUTANT TUE : manque %lld porteurs, blocs_faux %lld\n",
                       total - ref_brute, g.blocs_faux);
          return 4;
        }
        std::fprintf(stderr, "DESACCORD DU JUGE : FERMETURE FAUSSE — recursion %lld sous la"
                     " force brute %lld, blocs_faux %lld\n", total, ref_brute, g.blocs_faux);
        return 1;
      }
      if (mu != GwMutant::kNone && mu != GwMutant::kPhiLarge) {
        std::fprintf(stderr, "MUTANT SURVIVANT : %s n'a pas ete vu\n",
                     mu == GwMutant::kDeadLarge ? "dead-large" : "all-strict-lache");
        return 3;
      }
      if (ref_brute == 0 && min_carriers > 0) {
        std::fprintf(stderr, "PLANCHER : la force brute ne trouve aucun porteur\n");
        return 3;
      }
    }
    if (max_pairid >= 0 && g.pairid_expanded > max_pairid) {
      std::fprintf(stderr, "PLANCHER : pairid_expanded=%lld depasse %lld\n",
                   g.pairid_expanded, max_pairid);
      return 3;
    }
    if (min_carriers >= 0 && g.carriers + g.carriers_symboliques < min_carriers) {
      std::fprintf(stderr, "PLANCHER : carriers=%lld sous %lld\n",
                   g.carriers + g.carriers_symboliques, min_carriers);
      return 3;
    }
    if (g.noeuds < min_noeuds) {
      std::fprintf(stderr, "PLANCHER : noeuds=%lld sous %lld\n", g.noeuds, min_noeuds);
      return 3;
    }
  }
  std::printf("OK : gateway aigu mesure\n");
  return 0;
}
