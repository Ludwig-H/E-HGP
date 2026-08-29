// MorseHGP3D v5 — SONDE (jamais un claim) : la route « WSPD + 64 patches » des
// auditeurs, mesuree sur sa SEULE question decisive.
//
// L IDEE (REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md, § « Certificat sur :
// center-cover conditionne par C ») : pour un rectangle WSPD (A,B), couvrir une
// fois pour toutes l espace des centres q3 possibles par 64 patches entiers,
// puis, pour chaque handle C, ne garder le bit j que si les medaitrices AB, AC
// et BC sont faisables sur ce patch. Un masque VIDE prouve qu aucun triangle
// aigu (a,b,x) de A x B x C n a de centre : le bloc entier meurt AVANT toute
// materialisation de (a,b,x).
//
// C est le seul mecanisme identifie qui attaque la PROPOSITION et non le cout
// par seed — lequel est deja court (11 a 13 tests de sites, mesure de la vraie
// lane au pin ac02e3c7). La sonde mesure donc :
//   1. le taux de masques vides par bloc (rectangle, handle) ;
//   2. la masse de seeds aigus reellement retiree par ces morts ;
//   3. l INVARIANT DE SURETE : aucun bloc portant un vrai seed aigu ne doit
//      avoir un masque vide. Une seule violation est une contradiction
//      mathematique, et la sonde sort en code 3.
//
// LES 64 PATCHES. Le centre c d un triangle aigu dont ab est l arete maximale
// verifie |ab|/2 <= |c-a| = |c-b| <= |ab|/sqrt(3) : l angle oppose a ab est le
// plus grand, donc dans [60,90) degres. La boite des centres du rectangle est
// donc Box(A) dilatee de Rmax = sqrt(maxdist2(A,B)/3), intersectee avec la
// meme dilatation de Box(B). Sur chaque axe [l,h] de cette boite, les bornes
// l + j(h-l)/4 pour j = 0..4 sont ENTIERES a l ECHELLE 4 : tout ce fichier
// travaille en centres x4 (c chapeau = 4c), les points restant en coordonnees
// d origine. patch_id = j_x | (j_y << 2) | (j_z << 4), exactement comme la
// route P1a de la v3 (prototype/center_cover_mass_probe.cpp).
//
// LES TESTS. Tous sont des relaxations par intervalles entiers sur les produits
// de boites : ils conservent tout vrai centre et tolerent des faux (fail-open).
// Un intervalle est infaisable exactement quand il ne contient pas zero (ou,
// pour une inegalite, quand elle est violee sur tout l intervalle).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/pipeline/generate.hpp"

#ifndef MHGP5_PROBE_PIN
#define MHGP5_PROBE_PIN "unstamped"
#endif
#ifndef MHGP5_PROBE_DIRTY
#define MHGP5_PROBE_DIRTY "unknown"
#endif

using namespace mhgp5;

namespace {

inline u64 mix64(u64 x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

struct Iv {
  i128 lo = 0, hi = 0;
};

inline Iv iv_add(const Iv& a, const Iv& b) { return Iv{a.lo + b.lo, a.hi + b.hi}; }

inline Iv iv_mul(const Iv& a, const Iv& b) {
  const i128 p0 = a.lo * b.lo, p1 = a.lo * b.hi, p2 = a.hi * b.lo, p3 = a.hi * b.hi;
  return Iv{std::min(std::min(p0, p1), std::min(p2, p3)), std::max(std::max(p0, p1), std::max(p2, p3))};
}

// Carre d un intervalle : le minimum vaut zero si l intervalle traverse zero.
inline Iv iv_sq(const Iv& a) {
  const i128 lo2 = a.lo * a.lo, hi2 = a.hi * a.hi;
  if (a.lo <= 0 && a.hi >= 0) return Iv{0, std::max(lo2, hi2)};
  return Iv{std::min(lo2, hi2), std::max(lo2, hi2)};
}

struct Box128 {
  i128 lo[3] = {0, 0, 0};
  i128 hi[3] = {0, 0, 0};
};

inline Box128 from_axis(const AxisBox& b) {
  Box128 o;
  for (int i = 0; i < 3; ++i) { o.lo[i] = b.lo[i]; o.hi[i] = b.hi[i]; }
  return o;
}

// |c^ - 4P|^2 sur (patch Q, boite de points P), en ECHELLE 16 des distances.
inline Iv dist2_center_point(const Box128& q, const Box128& p, i128 sc) {
  Iv total{0, 0};
  for (int i = 0; i < 3; ++i)
    total = iv_add(total, iv_sq(Iv{q.lo[i] - sc * p.hi[i], q.hi[i] - sc * p.lo[i]}));
  return total;
}

// |P - R|^2 entre deux boites de points, ECHELLE 1.
inline Iv dist2_point_point(const Box128& p, const Box128& r) {
  Iv total{0, 0};
  for (int i = 0; i < 3; ++i)
    total = iv_add(total, iv_sq(Iv{p.lo[i] - r.hi[i], p.hi[i] - r.lo[i]}));
  return total;
}

// |c^ - 4P|^2 - |c^ - 4R|^2 = somme_i 4(R_i - P_i) (2 c^_i - 4 P_i - 4 R_i).
// Ecriture PRODUIT : elle evite la soustraction de deux carres relaxes
// separement, qui perdrait tout le pouvoir de coupe.
inline Iv bisector(const Box128& q, const Box128& p, const Box128& r, i128 sc) {
  Iv total{0, 0};
  for (int i = 0; i < 3; ++i) {
    const Iv t1{sc * (r.lo[i] - p.hi[i]), sc * (r.hi[i] - p.lo[i])};
    const Iv t2{2 * q.lo[i] - sc * p.hi[i] - sc * r.hi[i], 2 * q.hi[i] - sc * p.lo[i] - sc * r.lo[i]};
    total = iv_add(total, iv_mul(t1, t2));
  }
  return total;
}

inline bool contains_zero(const Iv& v) { return v.lo <= 0 && v.hi >= 0; }

// Racine entiere SUPERIEURE : ceil(sqrt(v)), v >= 0. Jamais une troncature —
// la fixture jung-root de la v3 montre qu elle omet des centres.
inline i64 isqrt_ceil(i128 v) {
  if (v <= 0) return 0;
  i64 r = (i64)std::sqrt((double)v);
  while (r > 0 && (i128)(r - 1) * (r - 1) >= v) --r;
  while ((i128)r * r < v) ++r;
  return r;
}

// K = subdivisions par axe ; le pavage compte K^3 patches. K = 4 donne les 64
// patches de la note ; K = 2 et K = 8 servent a mesurer si le pouvoir de coupe
// est limite par la RESOLUTION du pavage ou par la geometrie.
struct RectPatches {
  int k = 4;
  std::vector<Box128> patch;      // K^3
  std::vector<u8> ab_alive;       // K^3 : patch survivant aux contraintes (A,B)
  u64 ab_count = 0;
  bool valid = false;
};

// Boite des centres du rectangle, ECHELLE 4, puis pavage 4 x 4 x 4.
inline RectPatches build_patches(const AxisBox& ba, const AxisBox& bb, int k, bool mutant_rho = false) {
  RectPatches out;
  out.k = k;
  const Box128 A = from_axis(ba), B = from_axis(bb);
  const Iv d2 = dist2_point_point(A, B);
  if (d2.hi <= 0) return out;
  // BOITE SERREE (lemme du rayon HORS AXE). Le centre vaut c = m + t avec
  // m = (a+b)/2, t PERPENDICULAIRE a d = b-a, et |t|^2 = R^2 - D^2/4 <= D^2/12
  // puisque R <= D/sqrt(3). De t.d = 0 on tire, par Cauchy-Schwarz,
  //     t_i^2 d_i^2 <= (|t|^2 - t_i^2)(D^2 - d_i^2)  d ou  t_i^2 <= (D^2 - d_i^2)/12,
  // c est-a-dire que la dilatation de l axe i n utilise QUE l etendue HORS AXE.
  // C est tout le gain : la dilatation isotrope par Rmax = sqrt(maxdist2/3)
  // ignorait cette annulation. La boite serree est PROUVEE incluse dans
  // l ancienne (la difference vaut (sqrt(M)/(2 sqrt3))(2 sin(theta+30) - 2) <= 0),
  // c est donc un raffinement, jamais un concurrent.
  // Tout est a l ECHELLE 2 : lo2 = A.lo + B.lo represente 2 * borne du milieu.
  i128 w[3];
  for (int i = 0; i < 3; ++i) {
    const i128 g1 = A.hi[i] - B.lo[i], g2 = B.hi[i] - A.lo[i];
    w[i] = std::max(g1 < 0 ? -g1 : g1, g2 < 0 ? -g2 : g2);
  }
  const i128 wsum = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
  i128 lo[3], hi[3];
  for (int i = 0; i < 3; ++i) {
    const i128 moff = wsum - w[i] * w[i];               // etendue HORS AXE au carre
    i64 rho2 = isqrt_ceil((moff + 2) / 3 + 1);         // 2|t_i| <= sqrt(moff/3), majore
    if (mutant_rho && rho2 > 0) --rho2;                // mutant `rho-moins-un`
    lo[i] = A.lo[i] + B.lo[i] - rho2;
    hi[i] = A.hi[i] + B.hi[i] + rho2;
    if (lo[i] > hi[i]) return out;
  }
  // Bornes de pavage, ECHELLE 4 : b_j = 4 lo + j (hi - lo), j = 0..4 — entieres.
  // Bornes ENTIERES a l ECHELLE 2k : lo/hi sont deja a l echelle 2.
  std::vector<std::vector<i128>> bnd(3, std::vector<i128>((size_t)k + 1, 0));
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j <= k; ++j) bnd[(size_t)i][(size_t)j] = (i128)k * lo[i] + (i128)j * (hi[i] - lo[i]);
  out.patch.assign((size_t)(k * k * k), Box128{});
  out.ab_alive.assign((size_t)(k * k * k), 0);
  for (int jx = 0; jx < k; ++jx)
    for (int jy = 0; jy < k; ++jy)
      for (int jz = 0; jz < k; ++jz) {
        const int id = jx + k * (jy + k * jz);
        Box128& q = out.patch[(size_t)id];
        q.lo[0] = bnd[0][(size_t)jx]; q.hi[0] = bnd[0][(size_t)jx + 1];
        q.lo[1] = bnd[1][(size_t)jy]; q.hi[1] = bnd[1][(size_t)jy + 1];
        q.lo[2] = bnd[2][(size_t)jz]; q.hi[2] = bnd[2][(size_t)jz + 1];
      }
  // Contraintes ne dependant que de (A,B) : mediatrice AB, et le rayon
  // encadre par |ab|/2 <= R <= |ab|/sqrt(3), en ECHELLE 16.
  const Iv ab2 = dist2_point_point(A, B);
  const i128 sc = 2 * (i128)k, sc2 = sc * sc;   // ECHELLE 2k (la boite est a l echelle 2)
  for (size_t id = 0; id < out.patch.size(); ++id) {
    const Box128& q = out.patch[id];
    if (!contains_zero(bisector(q, A, B, sc))) continue;
    const Iv ra = dist2_center_point(q, A, sc);             // k^2 |c-a|^2
    // 3 |c-a|^2 <= |ab|^2 : faisable si min(3 ra - k^2 ab2) <= 0
    if (3 * ra.lo - sc2 * ab2.hi > 0) continue;
    // 4 |c-a|^2 >= |ab|^2 : faisable si max(4 ra - k^2 ab2) >= 0
    if (4 * ra.hi - sc2 * ab2.lo < 0) continue;
    out.ab_alive[id] = 1;
    ++out.ab_count;
  }
  out.valid = true;
  return out;
}

// Masque du bloc (A,B,C) : bits AB survivants dont les mediatrices AC et BC
// sont faisables. Renvoie aussi le nombre de tests patch-noeud payes.
inline u64 block_mask(const RectPatches& rp, const Box128& A, const Box128& B, const Box128& C,
                      std::vector<u8>* m, u64* tests) {
  const i128 sc = 2 * (i128)rp.k;
  m->assign(rp.patch.size(), 0);
  u64 pc = 0;
  for (size_t id = 0; id < rp.patch.size(); ++id) {
    if (!rp.ab_alive[id]) continue;
    ++*tests;
    const Box128& q = rp.patch[id];
    if (!contains_zero(bisector(q, A, C, sc))) continue;
    ++*tests;
    if (!contains_zero(bisector(q, B, C, sc))) continue;
    (*m)[id] = 1;
    ++pc;
  }
  return pc;
}


// ---------------------------------------------------------------------------
// CREDIT TEMOIN g_AB[j] (le VRAI mecanisme de la route, § V84 des auditeurs).
// psi(o,a;z) = |o-a|^2 - |o-z|^2 est AFFINE en o ; f_A(o;z) = dist^2(o,Box(A))
// - |o-z|^2 en est un minimum ponctuel, donc CONCAVE : son minimum sur un
// patch est atteint a l un des huit sommets. Ainsi
//     L_A(Q,z) = min_{sommets C de Q} [ dist^2(C, 4 Box(A)) - |C - 4z|^2 ]
// et max(L_A, L_B) > 0 certifie z STRICTEMENT interieur a toute boule dont le
// centre vit dans Q. Tout est a l ECHELLE 4 des centres, donc 16 des carres.
// L egalite ECHOUE : le predicat est strict.
struct PatchVerts {
  i64 v[8][3] = {};
  i64 da[8] = {};  // dist^2(sommet, 4 Box(A))
  i64 db[8] = {};
};

inline i64 dist2_pt_box4(const i64 c[3], const Box128& box, i64 sc) {
  i64 total = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 lo = (i64)(sc * box.lo[i]), hi = (i64)(sc * box.hi[i]);
    const i64 g = c[i] < lo ? lo - c[i] : (c[i] > hi ? c[i] - hi : 0);
    total += g * g;
  }
  return total;
}

inline void patch_verts(const Box128& q, const Box128& A, const Box128& B, i64 sc, PatchVerts* out) {
  for (int k = 0; k < 8; ++k) {
    out->v[k][0] = (i64)((k & 1) ? q.hi[0] : q.lo[0]);
    out->v[k][1] = (i64)((k & 2) ? q.hi[1] : q.lo[1]);
    out->v[k][2] = (i64)((k & 4) ? q.hi[2] : q.lo[2]);
    out->da[k] = dist2_pt_box4(out->v[k], A, sc);
    out->db[k] = dist2_pt_box4(out->v[k], B, sc);
  }
}

// Le temoin z est-il credite au patch ? max(L_A, L_B) > 0.
inline bool patch_credits(const PatchVerts& pv, const P3& z, i64 sc) {
  const i64 z4[3] = {sc * (i64)z.x, sc * (i64)z.y, sc * (i64)z.z};
  i64 la = 0, lb = 0;
  bool first = true;
  for (int k = 0; k < 8; ++k) {
    i64 d2 = 0;
    for (int i = 0; i < 3; ++i) { const i64 t = pv.v[k][i] - z4[i]; d2 += t * t; }
    const i64 fa = pv.da[k] - d2, fb = pv.db[k] - d2;
    if (first) { la = fa; lb = fb; first = false; }
    else { la = std::min(la, fa); lb = std::min(lb, fb); }
    if (la <= 0 && lb <= 0) return false;  // les deux minorants sont deja perdus
  }
  return la > 0 || lb > 0;
}


// LOCALISATION EXACTE DU CENTRE d un vrai seed dans le pavage. Le centre vaut
// c = a + W/(2G) avec G > 0 (q3.hpp) ; a l ECHELLE K, K c est compare aux
// bornes entieres du pavage par produit croise exact :
//     K c_i >= bnd  <=>  2 G K a_i + K W_i >= 2 G bnd.
// Rend l indice du patch, ou -1 si le centre tombe hors de la boite couverte —
// ce qui serait une faute du COVER, pas du credit.
// LOCALISATION EXACTE DU CENTRE d un vrai seed dans le pavage. Le centre vaut
// c = a + W/(2G) avec G > 0 (q3.hpp) ; a l ECHELLE K, K c est compare aux
// bornes entieres du pavage par produit croise exact :
//     K c_i >= bnd  <=>  2 G K a_i + K W_i >= 2 G bnd.
// Les patches sont FERMES : un centre sur une frontiere appartient a deux
// patches d un meme axe, donc jusqu a huit patches. On les rend TOUS — un test
// de surete qui n en garderait qu un serait plus faible que le predicat.
inline void locate_center_patches(const Q3Form& f, const RectPatches& rp, std::vector<int>* out) {
  out->clear();
  if (f.g <= 0) return;
  const i128 k = 2 * (i128)rp.k;
  const i64 ai[3] = {f.a.x, f.a.y, f.a.z};
  const i128 den2 = 2 * f.g;
  i128 num[3];
  for (int i = 0; i < 3; ++i) num[i] = den2 * k * (i128)ai[i] + k * f.w[i];  // = 2G * (K c_i)
  int hits[3][2];
  int nhit[3] = {0, 0, 0};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < rp.k && nhit[i] < 2; ++j) {
      const size_t id = (size_t)(i == 0 ? j : (i == 1 ? rp.k * j : rp.k * rp.k * j));
      if (num[i] >= den2 * rp.patch[id].lo[i] && num[i] <= den2 * rp.patch[id].hi[i])
        hits[i][nhit[i]++] = j;
    }
  if (!nhit[0] || !nhit[1] || !nhit[2]) return;
  for (int x = 0; x < nhit[0]; ++x)
    for (int y = 0; y < nhit[1]; ++y)
      for (int z = 0; z < nhit[2]; ++z)
        out->push_back(hits[0][x] + rp.k * (hits[1][y] + rp.k * hits[2][z]));
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 8000, coord = 0, seed = 3;
  u64 blocs_cible = 200000, rects_cible = ~(u64)0;
  int tuile = 4;
  i64 core_min = -1;  // ne paver QUE les rectangles de core >= seuil : `core` dit
                      // gratuitement de combien de temoins W_3 a manque son coup,
                      // et le pavage ne vaut que la ou il en manque peu. Pur choix
                      // de COUT : un rectangle non pave n est pas elague.
  u64 min_seeds = 0, min_blocs = 0, min_credit_evals = 0, min_oracle_profondeurs = 0;
  // MUTANT LOCAL A L ORACLE : compose `core` et `g_AB[j]` par une SOMME au
  // lieu du `max` sur. Les deux credits peuvent reconnaitre le MEME site :
  // la somme surcompte et tue des seeds non profonds. La porte doit voir
  // violations_credit > 0 (code 3). Il sera promu au registre `kMutants`
  // avec sa cible CTest a code 4 quand la sonde aura sa cible CMake.
  bool mutant_credit_somme = false;
  // MUTANT DE LA BOITE SERREE : un cran de moins sur le rayon HORS AXE. La
  // boite cesse alors de contenir tous les vrais centres, et l oracle
  // `centres_hors_cover` doit le voir (code 3). Sans cette porte, une erreur
  // d arrondi sur rho2 passerait pour un gain de resolution.
  bool mutant_rho_moins_un = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--seed=", 0) == 0) seed = std::atoi(arg.c_str() + 7);
    else if (arg.rfind("--coord=", 0) == 0) coord = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--blocs=", 0) == 0) blocs_cible = (u64)std::atoll(arg.c_str() + 8);
    else if (arg.rfind("--rects=", 0) == 0) rects_cible = (u64)std::atoll(arg.c_str() + 8);
    else if (arg.rfind("--tuile=", 0) == 0) tuile = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--core-min=", 0) == 0) core_min = std::atoll(arg.c_str() + 11);
    else if (arg.rfind("--min-seeds=", 0) == 0) min_seeds = (u64)std::atoll(arg.c_str() + 12);
    else if (arg.rfind("--min-blocs=", 0) == 0) min_blocs = (u64)std::atoll(arg.c_str() + 12);
    else if (arg.rfind("--min-credit-evals=", 0) == 0) min_credit_evals = (u64)std::atoll(arg.c_str() + 19);
    else if (arg.rfind("--min-profondeurs=", 0) == 0) min_oracle_profondeurs = (u64)std::atoll(arg.c_str() + 18);
    else if (arg == "--inject=credit-sum-core-gab") mutant_credit_somme = true;
    else if (arg == "--inject=rho-moins-un") mutant_rho_moins_un = true;
    else return 2;
  }
  if (n < 4) return 2;
  if (tuile != 2 && tuile != 4 && tuile != 8) return 2;  // K^3 patches : 8, 64, 512
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, seed));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h3 = lane_h(Lane::kQ3, smax);
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), h3, lane_h(Lane::kQ4, smax)};
  std::vector<AliveRect> alive;
  u64 wspd_visits = 0, workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &wspd_visits, &workers);
  generate_detail::AnchorScratch sc;
  std::vector<u8> bloc_mask;
  std::vector<int> centre_patches;

  u64 rects = 0, rects_sans_patch = 0, blocs = 0, blocs_vides = 0, tests = 0;
  u64 seeds_total = 0, seeds_dans_blocs_vides = 0, triples = 0;
  u64 violations = 0;
  u64 popcount_hist[9] = {};  // 0, 1-8, puis >8 dans la case 8
  u64 patchs_vivants = 0, patchs_morts_credit = 0, credit_evals = 0, f_min_sum = 0, rects_paves = 0;
  u64 blocs_morts_credit = 0, seeds_dans_blocs_morts_credit = 0, violations_credit = 0;
  u64 oracle_seeds = 0, oracle_profondeurs = 0, centres_hors_cover = 0, centres_hors_masque = 0;
  u64 plafond_atteint = 0;
  // ECHANTILLONNAGE PAR HACHAGE sur TOUTE la liste des rectangles vivants.
  // Prendre le prefixe biaise la population : l ordre de la vague WSPD trie les
  // rectangles par niveau, donc le prefixe n a pas la meme geometrie a n=2000
  // et a n=8000, et la pente lue serait un artefact.
  const u64 rects_vises = rects_cible == ~(u64)0 ? (u64)alive.size() : rects_cible;
  const u64 seuil = alive.empty() ? 0 : rects_vises;
  for (size_t irect = 0; irect < alive.size(); ++irect) {
    if (seuil < (u64)alive.size() && mix64((u64)irect ^ mix64((u64)(u32)seed)) % (u64)alive.size() >= seuil) continue;
    const AliveRect& ar = alive[irect];
    if (blocs >= blocs_cible) { plafond_atteint = 1; break; }
    ++rects;
    const AxisBox ba = ix.box_of(ar.r.a), bb = ix.box_of(ar.r.b);
    const RectPatches rp = build_patches(ba, bb, tuile, mutant_rho_moins_un);
    if (!rp.valid || rp.ab_count == 0) { ++rects_sans_patch; }
    const Box128 A = from_axis(ba), B = from_axis(bb);
    rect_cover_handles(ix, ba, bb, 3, &sc.handles, &sc.cover_nodes);
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    // CREDIT PAR PATCH, une seule fois par rectangle : g_AB[j] sature a h3, sur
    // les temoins du cover prives de A et de B. base_j = max(core, g_AB[j]) ;
    // le patch est mort des que base_j >= h3 (f = h_a + h_b pris a zero, donc
    // MINORANT du pouvoir de coupe reel).
    // f = min_a h_a(a) + min_b h_b(b) : domaines DISJOINTS de celui de g_AB
    // (temoins pris dans A, dans B, et hors A union B), donc l addition est
    // legitime. C est la regle exacte des auditeurs : patch mort ssi
    // max(core, g_AB[j]) + f >= h3.
    u64 f_endpoints = 0;
    if (rp.valid && rp.ab_count) {
      generate_detail::corner_histograms(ix, Lane::kQ3, ar.r, &sc.ha, &sc.hb);
      u64 amin = ~(u64)0, bmin = ~(u64)0;
      for (u64 v : sc.ha) amin = std::min(amin, v);
      for (u64 v : sc.hb) bmin = std::min(bmin, v);
      if (amin == ~(u64)0) amin = 0;
      if (bmin == ~(u64)0) bmin = 0;
      f_endpoints = amin + bmin;
      f_min_sum += f_endpoints;
    }
    std::vector<u8> patch_mort;
    const bool paver = core_min < 0 || (i64)ar.core >= core_min;
    if (paver) ++rects_paves;
    if (paver && rp.valid && rp.ab_count) {
      patch_mort.assign(rp.patch.size(), 0);
      for (size_t id = 0; id < rp.patch.size(); ++id) {
        if (!rp.ab_alive[id]) continue;
        ++patchs_vivants;
        if (ar.core + f_endpoints >= h3) { patch_mort[id] = 1; ++patchs_morts_credit; continue; }
        PatchVerts pv;
        patch_verts(rp.patch[id], A, B, 2 * (i64)tuile, &pv);
        // COMPOSITION SURE : `core` et `g_AB[j]` peuvent reconnaitre le MEME
        // site. Sans rangs de positions, leur seule composition sure est
        // max(core, g_AB[j]) — jamais la somme (la somme donne 228 violations
        // de l invariant de profondeur sur terrain n=2000, graine 3 : fixture).
        u64 g = 0;
        for (const NodeRef hw : sc.handles) {
          const NodeRange rw = ix.range_of(hw);
          const u64 appoint = mutant_credit_somme ? ar.core : 0;  // borne d arret PROPRE a la composition
          for (i32 uz = rw.first; uz <= rw.last && g + appoint + f_endpoints < h3; ++uz) {
            if ((uz >= ra.first && uz <= ra.last) || (uz >= rb.first && uz <= rb.last)) continue;
            ++credit_evals;
            if (patch_credits(pv, ix.upos[(size_t)uz], 2 * (i64)tuile)) ++g;
          }
          if (g + (mutant_credit_somme ? ar.core : 0) + f_endpoints >= h3) break;
        }
        const u64 base = mutant_credit_somme ? g + ar.core : std::max(g, ar.core);
        if (base + f_endpoints >= h3) { patch_mort[id] = 1; ++patchs_morts_credit; }
      }
    }
    for (const NodeRef hnd : sc.handles) {
      ++blocs;
      const AxisBox bc = ix.box_of(hnd);
      const Box128 C = from_axis(bc);
      const u64 m = (rp.valid && rp.ab_count) ? block_mask(rp, A, B, C, &bloc_mask, &tests) : 0;
      const int pc = (int)m;
      ++popcount_hist[pc > 8 ? 8 : pc];
      // VERITE TERRAIN du bloc : les vrais seeds aigus de A x B x C.
      const NodeRange rc = ix.range_of(hnd);
      u64 vrais = 0;
      for (i32 ua = ra.first; ua <= ra.last; ++ua)
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (!D2) continue;
          for (i32 ux = rc.first; ux <= rc.last; ++ux) {
            if (ux == ua || ux == ub) continue;
            ++triples;
            if (is_acute_seed(pa, pb, ix.upos[(size_t)ux], D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(ux)))
              ++vrais;
          }
        }
      seeds_total += vrais;
      if (m == 0) {
        ++blocs_vides;
        seeds_dans_blocs_vides += vrais;
        if (vrais != 0) ++violations;  // CONTRADICTION : mort d un bloc habite
      }
      // MORT PAR CREDIT : tous les bits du masque du bloc sont des patchs morts.
      bool tout_mort = m != 0 && !patch_mort.empty();
      if (tout_mort)
        for (size_t id = 0; id < bloc_mask.size() && tout_mort; ++id)
          if (bloc_mask[id] && !patch_mort[id]) tout_mort = false;
      if (tout_mort) {
        ++blocs_morts_credit;
        seeds_dans_blocs_morts_credit += vrais;
      }
      // ORACLE DE CREDIT, VERSION FORTE (demande de l audit 262d2819). Pour
      // CHAQUE vrai seed du bloc, et non plus seulement quand tout le bloc
      // meurt : localiser exactement son centre dans le pavage, puis
      //   (a) au moins un patch le contenant doit etre dans le masque du bloc
      //       — sinon le MASQUE est faux ;
      //   (b) tout patch le contenant et marque MORT impose que le seed soit
      //       PROFOND — sinon le CREDIT est faux.
      if (vrais != 0 && rp.valid && !patch_mort.empty()) {
        for (i32 ua = ra.first; ua <= ra.last; ++ua)
          for (i32 ub = rb.first; ub <= rb.last; ++ub) {
            const P3& pa = ix.upos[(size_t)ua];
            const P3& pb = ix.upos[(size_t)ub];
            const i64 D2 = p3_norm2(p3_sub(pb, pa));
            if (!D2) continue;
            for (i32 ux = rc.first; ux <= rc.last; ++ux) {
              if (ux == ua || ux == ub) continue;
              if (!is_acute_seed(pa, pb, ix.upos[(size_t)ux], D2, ix.point_id(ua), ix.point_id(ub),
                                 ix.point_id(ux)))
                continue;
              const Q3Form f3 = q3_form(pa, pb, ix.upos[(size_t)ux]);
              locate_center_patches(f3, rp, &centre_patches);
              ++oracle_seeds;
              if (centre_patches.empty()) { ++centres_hors_cover; continue; }
              bool dans_masque = false, un_mort = false;
              for (int id : centre_patches) {
                if (bloc_mask[(size_t)id]) dans_masque = true;
                if (bloc_mask[(size_t)id] && patch_mort[(size_t)id]) un_mort = true;
              }
              if (!dans_masque) { ++centres_hors_masque; continue; }
              if (!un_mort) continue;
              u64 prof = 0;
              for (const NodeRef hw : sc.handles) {
                const NodeRange rw = ix.range_of(hw);
                for (i32 uz = rw.first; uz <= rw.last && prof < h3; ++uz) {
                  if (uz == ua || uz == ub) continue;
                  if (q3_power(f3, ix.upos[(size_t)uz]) < 0) ++prof;
                }
                if (prof >= h3) break;
              }
              ++oracle_profondeurs;
              if (prof < h3) ++violations_credit;  // CONTRADICTION MATHEMATIQUE
            }
          }
      }
    }
  }

  std::printf("q3_patch_block pin=%s worktree_modifie=%s\n", MHGP5_PROBE_PIN,
              std::string(MHGP5_PROBE_DIRTY) == "non" ? "non" : "OUI");
  std::printf("  famille=%s n=%d coord=%d seed=%d h3=%llu alive_rects=%zu tuile=%d (%d patches)\n",
              cloud_family_name(family), n, coord, seed, (unsigned long long)h3, alive.size(), tuile, tuile * tuile * tuile);
  std::printf("  rectangles_parcourus=%llu sans_patch_AB=%llu blocs=%llu tests_patch_noeud=%llu triples_enumeres=%llu\n",
              (unsigned long long)rects, (unsigned long long)rects_sans_patch,
              (unsigned long long)blocs, (unsigned long long)tests, (unsigned long long)triples);
  std::printf("  BLOCS MORTS (masque vide) : %llu / %llu = %.1f %%\n",
              (unsigned long long)blocs_vides, (unsigned long long)blocs,
              blocs ? 100.0 * (double)blocs_vides / (double)blocs : 0.0);
  std::printf("  seeds aigus : total=%llu dans_blocs_morts=%llu\n",
              (unsigned long long)seeds_total, (unsigned long long)seeds_dans_blocs_vides);
  std::printf("  popcount du masque 0..8+ :");
  for (int k = 0; k <= 8; ++k) std::printf(" %llu", (unsigned long long)popcount_hist[k]);
  std::printf("\n  CREDIT g_AB : patchs_vivants=%llu morts=%llu (%.1f %%) evaluations_temoins=%llu\n",
              (unsigned long long)patchs_vivants, (unsigned long long)patchs_morts_credit,
              patchs_vivants ? 100.0 * (double)patchs_morts_credit / (double)patchs_vivants : 0.0,
              (unsigned long long)credit_evals);
  std::printf("  PORTE core_min=%lld rects_paves=%llu/%llu (%.1f %%)\n", (long long)core_min,
              (unsigned long long)rects_paves, (unsigned long long)rects,
              rects ? 100.0 * (double)rects_paves / (double)rects : 0.0);
  std::printf("  f = min h_a + min h_b : moyenne par rectangle = %.2f (h3 = %llu)\n",
              rects ? (double)f_min_sum / (double)rects : 0.0, (unsigned long long)h3);
  std::printf("  BLOCS MORTS PAR CREDIT : %llu / %llu = %.1f %% ; seeds retires=%llu / %llu = %.1f %%\n",
              (unsigned long long)blocs_morts_credit, (unsigned long long)blocs,
              blocs ? 100.0 * (double)blocs_morts_credit / (double)blocs : 0.0,
              (unsigned long long)seeds_dans_blocs_morts_credit, (unsigned long long)seeds_total,
              seeds_total ? 100.0 * (double)seeds_dans_blocs_morts_credit / (double)seeds_total : 0.0);
  std::printf("  ORACLE seeds_juges=%llu profondeurs_verifiees=%llu centres_hors_cover=%llu centres_hors_masque=%llu\n",
              (unsigned long long)oracle_seeds, (unsigned long long)oracle_profondeurs,
              (unsigned long long)centres_hors_cover, (unsigned long long)centres_hors_masque);
  std::printf("  INVARIANT blocs_morts_habites=%llu violations_credit=%llu plafond_atteint=%llu (tous doivent valoir 0)\n",
              (unsigned long long)violations, (unsigned long long)violations_credit,
              (unsigned long long)plafond_atteint);
  // SORTIE FAIL-CLOSED (audit 262d2819). Un code 0 ne doit jamais couvrir un
  // run vide : chaque plancher demande est une condition SEPAREE, et le
  // plafond de blocs atteint invalide l echantillon (il retronque la liste
  // tiree par hachage dans l ordre d origine, donc reintroduit un biais).
  int code = 0;
  if (violations != 0 || violations_credit != 0) code = 3;
  if (centres_hors_cover != 0 || centres_hors_masque != 0) code = 3;
  if (blocs == 0 || rects == 0) code = 3;
  if (plafond_atteint != 0) {
    std::printf("REFUS : plafond de blocs atteint — echantillon retronque, taux non recevable\n");
    code = 3;
  }
  if (seeds_total < min_seeds) { std::printf("REFUS : plancher seeds (%llu < %llu)\n",
      (unsigned long long)seeds_total, (unsigned long long)min_seeds); code = 3; }
  if (blocs < min_blocs) { std::printf("REFUS : plancher blocs (%llu < %llu)\n",
      (unsigned long long)blocs, (unsigned long long)min_blocs); code = 3; }
  if (credit_evals < min_credit_evals) { std::printf("REFUS : plancher evaluations de credit (%llu < %llu)\n",
      (unsigned long long)credit_evals, (unsigned long long)min_credit_evals); code = 3; }
  if (oracle_profondeurs < min_oracle_profondeurs) {
    std::printf("REFUS : plancher profondeurs verifiees (%llu < %llu) — l oracle de credit n a rien juge\n",
                (unsigned long long)oracle_profondeurs, (unsigned long long)min_oracle_profondeurs);
    code = 3;
  }
  return code;
}
