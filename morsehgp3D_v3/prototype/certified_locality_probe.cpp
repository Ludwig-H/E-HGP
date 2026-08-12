// MorseHGP3D v3 — LA LOCALITE CERTIFIEE PAR INVERSION : le certificat qui
// remplace l'enumeration de C(n,2) par une generation par ancre.
//
// ---------------------------------------------------------------------------
// Le verrou que ce fichier attaque
// ---------------------------------------------------------------------------
//
// Toutes les routes q2/q3/q4 mesurees jusqu'ici partent de C(n,2) et PRUNENT.
// A 50 000 points cela fait 1 249 975 000 ancres avant tout travail utile, et
// les compteurs de travail des rampes recues croissent en n^1,5 a n^2,1. Le
// certificat de coupe employe — dix temoins dans la MEME chambre de Yao48, a
// distance inferieure a la moitie du diametre — est la cause structurelle :
//
//   * la chambre a un diametre angulaire de 60 degres, donc `cos` vaut au pire
//     1/2 et le rayon utile est divise par deux : facteur 8 en volume ;
//   * seuls les temoins de la chambre de la cible comptent, alors que la vraie
//     region de temoins est une calotte pouvant aller jusqu'a 90 degres :
//     facteur 48 supplementaire.
//
// Soit environ 384 fois trop de travail par rapport a la geometrie reelle. Ce
// n'est pas une constante que l'on optimise : c'est ce qui rend la lane
// superlineaire.
//
// ---------------------------------------------------------------------------
// Le theoreme (inversion en l'ancre)
// ---------------------------------------------------------------------------
//
// Fixons une ancre `x`. Toute boule `B` dont `x` est sur le BORD est la boule
// de diametre `[x, x + D u]`, ou `u` est la direction unitaire de l'antipode et
// `D` le diametre. Pour `z != x`, en posant `s = z - x`, `d = ||s||` et
// `v = s/d` :
//
//     z strictement interieur a B   <=>   d < D cos(angle(u,v)).            (1)
//
// (Preuve : ||z - (x + (D/2)u)||^2 < (D/2)^2 <=> ||s||^2 < D (u . s).)
//
// L'identite (1) se lit de deux facons. Par INVERSION zeta(z) = s/||s||^2 elle
// devient l'inegalite LINEAIRE `u . zeta(z) > 1/D` : les boules passant par `x`
// sont exactement les demi-espaces de l'espace inverse, et leurs interieurs
// sont exactement les points inverses au-dela du plan. A ancre fixee, le
// catalogue des boules a au plus neuf interieurs est donc le <=9-niveau du
// nuage inverse local — un objet output-sensitive, sans mosaique.
//
// Par CALOTTE, (1) dit que le point `z` interdit toutes les directions
//
//     C_z(D) = { u : u . v > d/D },
//
// calotte spherique centree en `v`, de rayon angulaire `arccos(d/D)`, qui
// CROIT avec `D`. D'ou le certificat, qui est le coeur de ce fichier :
//
//   THEOREME (localite certifiee). Soit r > 0. Si toute direction de la sphere
//   appartient a au moins K = 10 calottes C_z(r), alors toute boule B ayant x
//   sur son bord et au plus 9 points interieurs verifie diam(B) < r. En
//   particulier B est incluse dans la boule B(x,r), donc TOUT son support et
//   TOUS ses interieurs sont parmi les points de P a distance < r de x.
//
//   Preuve. Les calottes croissent avec D, donc si D >= r la profondeur de u
//   dans { C_z(D) } majore celle dans { C_z(r) }, soit au moins dix interieurs
//   stricts, soit p >= 10 > 9. Contraposee.                               [QED]
//
// Le seuil dix couvre les trois arites d'un coup : une activation retenue
// verifie p + q <= 11, donc p <= 9 en q2, p <= 8 en q3 et p <= 7 en q4 ; dix
// interieurs stricts les tuent toutes. Un seul certificat par ancre borne donc
// simultanement q2, q3 et q4.
//
// COROLLAIRE (generation par ancre). Si le certificat tient au rayon
// r = d_M(x), la M-ieme distance, alors toute activation dont `x` est un point
// du support se calcule EXACTEMENT dans la liste des M plus proches voisins de
// x. Chaque activation ayant au moins deux points de support, elle est vue par
// chacun d'eux : l'owner canonique (plus petit PointId certifie) l'emet une
// seule fois. Aucune table globale de paires, de cellules ou de cofaces.
//
// Le certificat de Yao48 est le cas particulier grossier de ce theoreme : il
// sous-approxime C_z(D) par « la chambre de v, si d < D/2 ». Le theoreme rend
// la calotte exacte.
//
// ---------------------------------------------------------------------------
// Le predicat, en entiers u16 exacts
// ---------------------------------------------------------------------------
//
// Les directions sont discretisees par la SUBDIVISION DE L'OCTAEDRE : sommets
// = vecteurs ENTIERS g avec |g_x| + |g_y| + |g_z| = m, cellules = triangles
// geodesiques de la projection radiale. Une calotte de rayon angulaire
// strictement inferieur a 90 degres est geodesiquement CONVEXE ; elle contient
// donc un triangle geodesique des qu'elle en contient les trois sommets. La
// couverture d'une cellule se teste donc sur ses trois sommets entiers :
//
//     g . s > 0   et   (g . s)^2 * R2 > ||g||^2 * (||s||^2)^2,        R2 = r^2.
//
// Aucun flottant, aucun rationnel, aucune racine. Les amplitudes tiennent en
// `__int128` : `||s||^2 <= 3*65535^2 < 2^34`, donc `(||s||^2)^2 < 2^68` et
// `||g||^2 <= m^2` ; le membre droit reste sous `2^{68+2 log2 m}`.
//
// Le certificat est CONSERVATEUR : une cellule non couverte ne prouve rien et
// fait croitre M. Une sous-estimation de la profondeur ne peut donc jamais
// certifier a tort — elle ne peut que refuser.
//
// ---------------------------------------------------------------------------
// Ce que ce probe est, et n'est pas
// ---------------------------------------------------------------------------
//
// C'EST : (a) une fixture permanente qui confronte le theoreme a un juge
// exhaustif independant sur les trois arites a petit n ; (b) une mesure de la
// taille locale certifiee M*(x) par famille et par n — le chiffre qui decide si
// la generation par ancre est lineaire ; (c) le census q2 EXACT obtenu par
// cette localite, c'est-a-dire la TAILLE DE SORTIE reelle, que aucune campagne
// du dossier n'a encore publiee.
//
// CE N'EST PAS : une source produit, un backend, une mesure de SLO. Aucun
// `public_status` n'avance ici. Les temps imprimes sont des temps de mesure sur
// une machine declaree, jamais un `warm_e2e`.
//
// Codes de sortie : 0 OK ; 1 desaccord du juge ; 2 campagne refusee avant
// calcul (CLI) ; 3 plancher de couverture ou invariant viole ; 4 mutant
// survivant.
#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"

namespace {

using i64 = long long;
using i128 = __int128;

// ---------------------------------------------------------------------------
// Injections : chaque mutant casse UNE clause du theoreme et doit etre tue.
// ---------------------------------------------------------------------------
struct Injections {
  bool cap_drop_strictness = false;  // `>=` au lieu de `>` dans la calotte
  bool cap_nine = false;             // seuil neuf au lieu de dix
  bool cap_corner_any = false;       // un seul sommet couvert suffit
  bool cap_half_radius = false;      // la borne Yao48 `d < r/2` au lieu du cos
  bool census_drop_ties = false;     // ignore les points a d^2 == R2
  bool rho_min_corner = false;       // min au lieu de max sur les trois sommets
  bool rho_first_corner = false;     // un seul sommet decide de la cellule
  bool rho_kth_short = false;        // K-1 -ieme plus petit rho au lieu du K-ieme
  bool locate_drop_boundary = false; // oublie les cellules de bord du reseau
  bool insphere_sign_flip = false;   // la regle d'orientation q4 fautive `==`
};

// ---------------------------------------------------------------------------
// La grille de directions : subdivision de l'octaedre, sommets ENTIERS.
// ---------------------------------------------------------------------------
struct DirGrid {
  std::vector<std::array<int, 3>> vertices;
  std::vector<i64> vertex_norm2;
  std::vector<std::array<int, 3>> cells;  // indices de sommets
  std::vector<int> cell_slot;             // octant*(m+1)^2 + i*(m+1) + j
  std::vector<std::vector<int>> cells_at; // slot -> cellules qui y commencent
  int subdivision = 0;
};

DirGrid make_dir_grid(int m) {
  DirGrid grid;
  grid.subdivision = m;
  std::map<std::array<int, 3>, int> index;
  auto vertex_of = [&](const std::array<int, 3>& v) {
    auto it = index.find(v);
    if (it != index.end()) return it->second;
    const int id = (int)grid.vertices.size();
    index.emplace(v, id);
    grid.vertices.push_back(v);
    grid.vertex_norm2.push_back((i64)v[0] * v[0] + (i64)v[1] * v[1] + (i64)v[2] * v[2]);
    return id;
  };
  grid.cells_at.assign((std::size_t)8 * (std::size_t)(m + 1) * (std::size_t)(m + 1), {});
  for (int sx = -1; sx <= 1; sx += 2)
    for (int sy = -1; sy <= 1; sy += 2)
      for (int sz = -1; sz <= 1; sz += 2) {
        const int octant = ((sx > 0) ? 0 : 4) + ((sy > 0) ? 0 : 2) + ((sz > 0) ? 0 : 1);
        // Face barycentrique a + b + c = m, a,b,c >= 0 ; (i,j) -> (m-i-j, i, j).
        auto corner = [&](int i, int j) {
          const std::array<int, 3> v{sx * (m - i - j), sy * i, sz * j};
          return vertex_of(v);
        };
        auto emit = [&](int i, int j, const std::array<int, 3>& tri) {
          const int slot = octant * (m + 1) * (m + 1) + i * (m + 1) + j;
          grid.cell_slot.push_back(slot);
          grid.cells_at[(std::size_t)slot].push_back((int)grid.cells.size());
          grid.cells.push_back(tri);
        };
        for (int i = 0; i + 1 <= m; ++i)
          for (int j = 0; i + j + 1 <= m; ++j) {
            emit(i, j, {corner(i, j), corner(i + 1, j), corner(i, j + 1)});
            if (i + j + 2 <= m)
              emit(i, j, {corner(i + 1, j), corner(i, j + 1), corner(i + 1, j + 1)});
          }
      }
  return grid;
}

// ---------------------------------------------------------------------------
// Le predicat de couverture d'un sommet par la calotte de `s` au rayon r.
//   couvert  <=>  g.s > 0  et  (g.s)^2 R2 > ||g||^2 (||s||^2)^2
// ---------------------------------------------------------------------------
inline bool corner_covered(const std::array<int, 3>& g, i64 gn2, const i64 s[3], i64 ls2, i64 r2,
                           const Injections& inject) {
  const i64 dotv = (i64)g[0] * s[0] + (i64)g[1] * s[1] + (i64)g[2] * s[2];
  if (dotv <= 0) return false;
  if (inject.cap_half_radius) {
    // Mutant : la borne Yao48 « d < r/2 » sans le cosinus. Elle est plus
    // FAIBLE (donc encore sure) sur sa chambre, mais appliquee a tout sommet
    // elle certifie des directions non couvertes.
    return 4 * ls2 < r2;
  }
  const i128 lhs = (i128)dotv * (i128)dotv * (i128)r2;
  const i128 rhs = (i128)gn2 * ((i128)ls2 * (i128)ls2);
  return inject.cap_drop_strictness ? (lhs >= rhs) : (lhs > rhs);
}

// ---------------------------------------------------------------------------
// Voisin trie : (d^2, PointId).
// ---------------------------------------------------------------------------
struct Neighbour {
  i64 d2 = 0;
  int id = 0;
  bool operator<(const Neighbour& o) const { return d2 != o.d2 ? d2 < o.d2 : id < o.id; }
};

// kNN exact borne par le LBVH Morton partage.
void knn(const mhgp3v::MortonLbvh& tree, const std::vector<mhgp::P3>& pts, int anchor, int want,
         std::vector<Neighbour>* out) {
  out->clear();
  const mhgp::P3& a = pts[(std::size_t)anchor];
  i64 worst = -1;  // -1 = tas non plein
  std::vector<int> stack;
  stack.reserve(64);
  stack.push_back(0);
  auto box_lower = [&](const mhgp3v::LbvhNode& node) {
    i64 acc = 0;
    const i64 c[3] = {a.x, a.y, a.z};
    for (int d = 0; d < 3; ++d) {
      const i64 lo = node.lo[d], hi = node.hi[d];
      const i64 gap = c[d] < lo ? lo - c[d] : (c[d] > hi ? c[d] - hi : 0);
      acc += gap * gap;
    }
    return acc;
  };
  while (!stack.empty()) {
    const int id = stack.back();
    stack.pop_back();
    const mhgp3v::LbvhNode& node = tree.nodes[(std::size_t)id];
    if (worst >= 0 && (int)out->size() >= want && box_lower(node) > worst) continue;
    if (node.left < 0) {
      for (int t = node.begin; t < node.end; ++t) {
        const int q = tree.order[(std::size_t)t];
        if (q == anchor) continue;
        const mhgp::P3& p = pts[(std::size_t)q];
        const i64 dx = p.x - a.x, dy = p.y - a.y, dz = p.z - a.z;
        const i64 d2 = dx * dx + dy * dy + dz * dz;
        if ((int)out->size() < want) {
          out->push_back(Neighbour{d2, q});
          std::push_heap(out->begin(), out->end());
          if ((int)out->size() == want) worst = out->front().d2;
        } else if (d2 < worst || (d2 == worst && q < out->front().id)) {
          std::pop_heap(out->begin(), out->end());
          out->back() = Neighbour{d2, q};
          std::push_heap(out->begin(), out->end());
          worst = out->front().d2;
        }
      }
      continue;
    }
    const int kids[2] = {node.left, node.right};
    i64 key[2];
    for (int s = 0; s < 2; ++s) key[s] = box_lower(tree.nodes[(std::size_t)kids[s]]);
    // Le plus proche en dernier : il sera depile en premier.
    if (key[0] < key[1]) {
      stack.push_back(kids[1]);
      stack.push_back(kids[0]);
    } else {
      stack.push_back(kids[0]);
      stack.push_back(kids[1]);
    }
  }
  std::sort_heap(out->begin(), out->end());
}

// ---------------------------------------------------------------------------
// LE RAYON PAR CELLULE — la forme utile du theoreme.
//
// Un rayon GLOBAL par ancre est correct mais inutilisable au bord du nuage :
// une direction ouverte (moins de K points dans le demi-espace) le rend infini
// et emporte toute l'ancre. Or le theoreme est DIRECTIONNEL. Pour une cellule
// `c` de la grille et un point `z`, la calotte C_z(r) contient `c` des que
//
//     r > rho(c,z) = max sur les trois sommets g de  ||g|| ||s||^2 / (g . s),
//
// avec rho = +infini des qu'un sommet verifie g.s <= 0. En posant `r_c` la
// K-ieme plus petite valeur de rho(c,.), toute boule dont l'antipode pointe
// dans `c` et dont le diametre depasse `r_c` a au moins K interieurs stricts.
// Une activation de direction `c` verifie donc D <= r_c.
//
// La masse de travail d'une ancre n'est plus |B(x, max_c r_c)| mais la somme
// directionnelle sum_c |{ y de direction c : d_y <= r_c }| : les quelques
// cellules ouvertes du bord ne contaminent plus les autres.
//
// Les rho sont compares EXACTEMENT sans racine : rho^2 = num/den avec
// num = ||g||^2 ||s||^4 et den = (g.s)^2, et `num_a den_b < num_b den_a` tient
// dans `__int128` (num < 2^76 pour m <= 16, den < 2^42).
// ---------------------------------------------------------------------------
struct Rho2 {
  i128 num = 0;  // ||g||^2 ||s||^4
  i128 den = 0;  // (g.s)^2   ; den == 0 marque +infini
};

inline bool rho_less(const Rho2& a, const Rho2& b) {
  if (a.den == 0) return false;
  if (b.den == 0) return true;
  return a.num * b.den < b.num * a.den;
}

// d2 <= rho^2  <=>  d2 * den <= num
inline bool within_rho(i64 d2, const Rho2& r) {
  if (r.den == 0) return true;  // cellule ouverte : aucune borne
  return (i128)d2 * r.den <= r.num;
}

// ---------------------------------------------------------------------------
// Le certificat : toute cellule de la grille est-elle couverte `kmin` fois ?
// ---------------------------------------------------------------------------
struct CertifyScratch {
  std::vector<unsigned long long> covered;  // bitset des sommets
  std::vector<int> depth;                   // profondeur par cellule
};

bool certificate_holds(const mhgp::P3& anchor, const std::vector<Neighbour>& nbrs, std::size_t take,
                       i64 r2, const DirGrid& grid, int kmin, CertifyScratch* scratch,
                       const std::vector<mhgp::P3>& pts, const Injections& inject,
                       i64* corner_tests) {
  const std::size_t words = (grid.vertices.size() + 63) / 64;
  scratch->covered.assign(words, 0ULL);
  scratch->depth.assign(grid.cells.size(), 0);
  const int need = inject.cap_nine ? kmin - 1 : kmin;
  i64 tests = 0;
  for (std::size_t t = 0; t < take; ++t) {
    const mhgp::P3& p = pts[(std::size_t)nbrs[t].id];
    const i64 s[3] = {p.x - anchor.x, p.y - anchor.y, p.z - anchor.z};
    const i64 ls2 = nbrs[t].d2;
    std::fill(scratch->covered.begin(), scratch->covered.end(), 0ULL);
    for (std::size_t v = 0; v < grid.vertices.size(); ++v) {
      ++tests;
      if (corner_covered(grid.vertices[v], grid.vertex_norm2[v], s, ls2, r2, inject))
        scratch->covered[v >> 6] |= 1ULL << (v & 63);
    }
    for (std::size_t c = 0; c < grid.cells.size(); ++c) {
      const std::array<int, 3>& cell = grid.cells[c];
      const auto bit = [&](int v) {
        return (scratch->covered[(std::size_t)v >> 6] >> ((std::size_t)v & 63)) & 1ULL;
      };
      const unsigned long long b0 = bit(cell[0]), b1 = bit(cell[1]), b2 = bit(cell[2]);
      const bool hit = inject.cap_corner_any ? (b0 | b1 | b2) : (b0 & b1 & b2);
      if (hit) ++scratch->depth[c];
    }
  }
  *corner_tests += tests;
  for (int d : scratch->depth)
    if (d < need) return false;
  return true;
}

// Plus petit `take` (donc plus petit rayon r = d_take) qui certifie. 0 = jamais.
std::size_t certified_take(const mhgp::P3& anchor, const std::vector<Neighbour>& nbrs,
                           const DirGrid& grid, int kmin, CertifyScratch* scratch,
                           const std::vector<mhgp::P3>& pts, const Injections& inject,
                           i64* corner_tests) {
  const std::size_t total = nbrs.size();
  if (total < (std::size_t)kmin) return 0;
  auto ok = [&](std::size_t take) {
    return certificate_holds(anchor, nbrs, take, nbrs[take - 1].d2, grid, kmin, scratch, pts,
                             inject, corner_tests);
  };
  std::size_t hi = (std::size_t)kmin;
  while (hi <= total && !ok(hi)) {
    if (hi == total) return 0;
    hi = hi * 2 < total ? hi * 2 : total;
  }
  if (hi > total) return 0;
  std::size_t lo = (std::size_t)kmin;  // lo-1 echoue toujours (ou lo == kmin)
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;
    if (ok(mid)) hi = mid; else lo = mid + 1;
  }
  return lo;
}

// Les K plus petits rho par cellule. `radii[c]` recoit r_c^2 (den == 0 si la
// cellule reste OUVERTE : moins de K points la couvrent, quelle que soit la
// distance). `scanned` compte les points effectivement lus.
void compute_cell_radii(const mhgp::P3& anchor, const std::vector<Neighbour>& nbrs,
                        const DirGrid& grid, int kmin, const std::vector<mhgp::P3>& pts,
                        std::vector<Rho2>* radii, std::vector<std::vector<Rho2>>* heaps,
                        i64* scanned, i64* cell_tests, const Injections& inject) {
  const std::size_t cells = grid.cells.size();
  const int need = inject.rho_kth_short ? (kmin > 1 ? kmin - 1 : 1) : kmin;
  radii->assign(cells, Rho2{});
  heaps->assign(cells, {});
  for (std::vector<Rho2>& h : *heaps) h.reserve((std::size_t)need);
  std::size_t closed = 0;
  i64 read = 0, tests = 0;
  for (const Neighbour& nb : nbrs) {
    // Arret global : plus aucun point plus lointain ne peut ameliorer une
    // cellule, car rho(c,z) >= d_z par Cauchy--Schwarz.
    if (closed == cells) {
      bool useful = false;
      for (std::size_t c = 0; c < cells && !useful; ++c)
        if (within_rho(nb.d2, (*radii)[c])) useful = true;
      if (!useful) break;
    }
    ++read;
    const mhgp::P3& p = pts[(std::size_t)nb.id];
    const i64 s[3] = {p.x - anchor.x, p.y - anchor.y, p.z - anchor.z};
    const i128 ls4 = (i128)nb.d2 * (i128)nb.d2;
    for (std::size_t c = 0; c < cells; ++c) {
      std::vector<Rho2>& heap = (*heaps)[c];
      const bool full = (int)heap.size() >= need;
      // rho(c,z) >= d_z : inutile si la cellule est deja fermee plus court.
      if (full && !within_rho(nb.d2, (*radii)[c])) continue;
      ++tests;
      const std::array<int, 3>& cell = grid.cells[c];
      Rho2 worst{};
      bool reachable = true;
      const int corners = inject.rho_first_corner ? 1 : 3;
      for (int k = 0; k < corners; ++k) {
        const std::array<int, 3>& g = grid.vertices[(std::size_t)cell[k]];
        const i64 dotv = (i64)g[0] * s[0] + (i64)g[1] * s[1] + (i64)g[2] * s[2];
        if (dotv <= 0) { reachable = false; break; }
        const Rho2 cand{grid.vertex_norm2[(std::size_t)cell[k]] * ls4, (i128)dotv * (i128)dotv};
        if (worst.den == 0) worst = cand;
        else if (inject.rho_min_corner ? rho_less(cand, worst) : rho_less(worst, cand))
          worst = cand;
      }
      if (!reachable) continue;
      if (!full) {
        heap.push_back(worst);
        std::push_heap(heap.begin(), heap.end(), rho_less);
        if ((int)heap.size() == need) {
          (*radii)[c] = heap.front();
          ++closed;
        }
      } else if (rho_less(worst, heap.front())) {
        std::pop_heap(heap.begin(), heap.end(), rho_less);
        heap.back() = worst;
        std::push_heap(heap.begin(), heap.end(), rho_less);
        (*radii)[c] = heap.front();
      }
    }
  }
  *scanned += read;
  *cell_tests += tests;
}

// La (ou les) cellules dont le triangle geodesique ferme contient la direction
// `s`. Conservateur par construction : on renvoie le bloc de reseau (fi,fj) et
// ses voisins immediats dans chaque octant compatible avec les signes nuls.
void locate_cells(const i64 s[3], int m, const DirGrid& grid,
                  const std::vector<std::vector<int>>& cells_at, std::vector<int>* out,
                  bool drop_boundary) {
  out->clear();
  (void)grid;
  const i64 a = s[0] < 0 ? -s[0] : s[0];
  const i64 b = s[1] < 0 ? -s[1] : s[1];
  const i64 c = s[2] < 0 ? -s[2] : s[2];
  const i64 total = a + b + c;
  if (total == 0) return;  // direction nulle : positions colocalisees, refusees en amont
  // Coordonnees barycentriques exactes de la face : i = m*b/T, j = m*c/T.
  const i64 bi = (i64)m * b, bj = (i64)m * c;
  const i64 fi = bi / total, ri = bi % total;
  const i64 fj = bj / total, rj = bj % total;
  for (int sx = -1; sx <= 1; sx += 2) {
    if (s[0] != 0 && ((s[0] > 0) != (sx > 0))) continue;
    for (int sy = -1; sy <= 1; sy += 2) {
      if (s[1] != 0 && ((s[1] > 0) != (sy > 0))) continue;
      for (int sz = -1; sz <= 1; sz += 2) {
        if (s[2] != 0 && ((s[2] > 0) != (sz > 0))) continue;
        const int octant = ((sx > 0) ? 0 : 4) + ((sy > 0) ? 0 : 2) + ((sz > 0) ? 0 : 1);
        auto push_slot = [&](i64 i, i64 j, bool up_only, bool down_only) {
          if (i < 0 || j < 0 || i + j > (i64)m) return;
          const std::size_t slot = (std::size_t)octant * (std::size_t)(m + 1) * (std::size_t)(m + 1) +
                                   (std::size_t)i * (std::size_t)(m + 1) + (std::size_t)j;
          const std::vector<int>& here = cells_at[slot];
          for (std::size_t k = 0; k < here.size(); ++k) {
            if (up_only && k != 0) continue;
            if (down_only && k != 1) continue;
            out->push_back(here[k]);
          }
        };
        // Triangle « haut » si frac_i + frac_j < 1, « bas » si > 1, les deux a
        // l'egalite. L'arete partagee et les lignes du reseau appartiennent a
        // plusieurs cellules : on les ajoute TOUTES (sur-ensemble sur, le
        // filtre etant une disjonction, donc fail-open).
        const bool up = ri + rj <= total;
        const bool down = ri + rj >= total;
        push_slot(fi, fj, !down, !up);
        if (drop_boundary) continue;
        if (ri == 0) push_slot(fi - 1, fj, false, false);
        if (rj == 0) push_slot(fi, fj - 1, false, false);
        if (ri == 0 && rj == 0) push_slot(fi - 1, fj - 1, false, false);
        if (fi + fj >= (i64)m) push_slot(fi - 1, fj, false, false);
        if (fi + fj >= (i64)m) push_slot(fi, fj - 1, false, false);
      }
    }
  }
  std::sort(out->begin(), out->end());
  out->erase(std::unique(out->begin(), out->end()), out->end());
}

// ---------------------------------------------------------------------------
// LE JUGE EXHAUSTIF — arithmetique ecrite ici, distincte de celle du sujet :
// determinants entiers et Cramer explicite, aucune calotte, aucune inversion.
// ---------------------------------------------------------------------------
namespace judge {

struct V3 {
  i64 x, y, z;
};
inline V3 sub(const mhgp::P3& a, const mhgp::P3& b) { return V3{a.x - b.x, a.y - b.y, a.z - b.z}; }
inline i64 dot(const V3& a, const V3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline i128 det3(const V3& a, const V3& b, const V3& c) {
  return (i128)a.x * ((i128)b.y * c.z - (i128)b.z * c.y) -
         (i128)a.y * ((i128)b.x * c.z - (i128)b.z * c.x) +
         (i128)a.z * ((i128)b.x * c.y - (i128)b.y * c.x);
}

// q2 : boule diametrale. z strictement interieur <=> (z-x).(z-y) < 0.
inline bool inside_q2(const mhgp::P3& x, const mhgp::P3& y, const mhgp::P3& z) {
  const V3 a = sub(z, x), b = sub(z, y);
  return dot(a, b) < 0;
}

// q3 : boule circonscrite au triangle, centre dans son plan.
// centre = a + (Na u + Nb v)/D, D = 2(A C - B^2), A=u.u, B=u.v, C=v.v.
struct Q3Ball {
  V3 u{}, v{};
  i128 Na = 0, Nb = 0, D = 0;
  bool ok = false;       // triangle non degenere
  bool positive = false; // support propre positif (triangle acutangle)
};

inline Q3Ball q3_ball(const mhgp::P3& a, const mhgp::P3& b, const mhgp::P3& c) {
  Q3Ball out;
  out.u = sub(b, a);
  out.v = sub(c, a);
  const i128 A = dot(out.u, out.u), B = dot(out.u, out.v), C = dot(out.v, out.v);
  const i128 g = A * C - B * B;
  if (g <= 0) return out;  // colineaires
  out.D = 2 * g;
  out.Na = C * (A - B);
  out.Nb = A * (C - B);
  out.ok = true;
  out.positive = out.Na > 0 && out.Nb > 0 && out.D - out.Na - out.Nb > 0;
  return out;
}

// z interieur strict <=> D ||w||^2 < 2 (Na (w.u) + Nb (w.v)), w = z - a.
inline bool inside_q3(const Q3Ball& ball, const mhgp::P3& a, const mhgp::P3& z) {
  const V3 w = sub(z, a);
  const i128 lhs = ball.D * (i128)dot(w, w);
  const i128 rhs = 2 * (ball.Na * (i128)dot(w, ball.u) + ball.Nb * (i128)dot(w, ball.v));
  return lhs < rhs;
}

// q4 : sphere circonscrite au tetraedre. Positivite par les quatre
// coordonnees barycentriques ; interiorite par le determinant InSphere 4x4.
struct Q4Ball {
  bool ok = false;
  bool positive = false;
  int orient = 0;
};

inline i128 det4_insphere(const mhgp::P3& a, const mhgp::P3& b, const mhgp::P3& c,
                          const mhgp::P3& d, const mhgp::P3& z) {
  const V3 r[4] = {sub(b, a), sub(c, a), sub(d, a), sub(z, a)};
  i128 l[4];
  for (int i = 0; i < 4; ++i) l[i] = (i128)dot(r[i], r[i]);
  // Developpement sur la derniere colonne (les normes).
  i128 acc = 0;
  const int sign[4] = {-1, 1, -1, 1};
  for (int skip = 0; skip < 4; ++skip) {
    V3 m[3];
    int w = 0;
    for (int i = 0; i < 4; ++i)
      if (i != skip) m[w++] = r[i];
    acc += (i128)sign[skip] * l[skip] * det3(m[0], m[1], m[2]);
  }
  return acc;
}

inline Q4Ball q4_ball(const mhgp::P3& a, const mhgp::P3& b, const mhgp::P3& c,
                      const mhgp::P3& d) {
  Q4Ball out;
  const V3 u = sub(b, a), v = sub(c, a), w = sub(d, a);
  const i128 det = det3(u, v, w);
  if (det == 0) return out;
  out.ok = true;
  out.orient = det > 0 ? 1 : -1;
  // Le centre est a + t avec 2 M t = q, M = [u;v;w] en lignes,
  // q = (|u|^2,|v|^2,|w|^2). Les barycentriques lambda resolvent M^T l = t.
  // Le signe de lambda_i est celui de (adj(M^T) adj(M) q)_i / (2 det^2), et
  // 2 det^2 > 0 : les numerateurs suffisent.
  const i64 qn[3] = {dot(u, u), dot(v, v), dot(w, w)};
  auto adj_mul = [](const V3 rows[3], const i128 in[3], i128 out3[3]) {
    // adj(M) = cofacteurs transposes ; (adj(M) y)_i = sum_j C_ji y_j.
    const V3& p = rows[0];
    const V3& q2 = rows[1];
    const V3& r2 = rows[2];
    const i128 c00 = (i128)q2.y * r2.z - (i128)q2.z * r2.y;
    const i128 c01 = -((i128)q2.x * r2.z - (i128)q2.z * r2.x);
    const i128 c02 = (i128)q2.x * r2.y - (i128)q2.y * r2.x;
    const i128 c10 = -((i128)p.y * r2.z - (i128)p.z * r2.y);
    const i128 c11 = (i128)p.x * r2.z - (i128)p.z * r2.x;
    const i128 c12 = -((i128)p.x * r2.y - (i128)p.y * r2.x);
    const i128 c20 = (i128)p.y * q2.z - (i128)p.z * q2.y;
    const i128 c21 = -((i128)p.x * q2.z - (i128)p.z * q2.x);
    const i128 c22 = (i128)p.x * q2.y - (i128)p.y * q2.x;
    out3[0] = c00 * in[0] + c10 * in[1] + c20 * in[2];
    out3[1] = c01 * in[0] + c11 * in[1] + c21 * in[2];
    out3[2] = c02 * in[0] + c12 * in[1] + c22 * in[2];
  };
  const V3 rows[3] = {u, v, w};
  const V3 cols[3] = {V3{u.x, v.x, w.x}, V3{u.y, v.y, w.y}, V3{u.z, v.z, w.z}};
  const i128 qi[3] = {(i128)qn[0], (i128)qn[1], (i128)qn[2]};
  i128 tnum[3];
  adj_mul(rows, qi, tnum);  // t = tnum / (2 det)
  i128 lnum[3];
  adj_mul(cols, tnum, lnum);  // lambda = lnum / (2 det^2)
  const i128 den = 2 * det * det;
  const i128 l0 = den - lnum[0] - lnum[1] - lnum[2];
  out.positive = lnum[0] > 0 && lnum[1] > 0 && lnum[2] > 0 && l0 > 0;
  return out;
}

}  // namespace judge

// ---------------------------------------------------------------------------
// Utilitaires de sortie
// ---------------------------------------------------------------------------
double percentile_of(std::vector<double> v, double q) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  // Rang le plus proche, contractuel : pas d'interpolation.
  std::size_t rank = (std::size_t)((double)v.size() * q + 0.9999999);
  if (rank == 0) rank = 1;
  if (rank > v.size()) rank = v.size();
  return v[rank - 1];
}

}  // namespace

int main(int argc, char** argv) {
  int n = 4000, coord = 0, grid_m = 4, kmin = 10, max_neighbours = 4096, threads = 0;
  i64 seed = 20260812;
  std::string mode = "profile";
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  Injections inject;
  i64 min_anchors = 0, min_activations = 0, min_uncertified_report = -1, min_far_pairs = 0;
  int judge_arity = 4, judge_census = 0;
  i64 min_q4 = 0;

  auto integer = [](const char* text, i64* value) {
    const char* last = text + std::strlen(text);
    return std::from_chars(text, last, *value).ec == std::errc();
  };
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto suffix = [&](const char* key) -> const char* {
      const std::size_t len = std::strlen(key);
      return arg.compare(0, len, key) == 0 ? argv[i] + len : nullptr;
    };
    i64 v = 0;
    if (const char* s = suffix("--points=")) { if (!integer(s, &v)) return 2; n = (int)v; }
    else if (const char* s2 = suffix("--coord=")) { if (!integer(s2, &v)) return 2; coord = (int)v; }
    else if (const char* s3 = suffix("--seed=")) { if (!integer(s3, &v)) return 2; seed = v; }
    else if (const char* s4 = suffix("--grid=")) { if (!integer(s4, &v)) return 2; grid_m = (int)v; }
    else if (const char* s5 = suffix("--kmin=")) { if (!integer(s5, &v)) return 2; kmin = (int)v; }
    else if (const char* s6 = suffix("--max-neighbours=")) {
      if (!integer(s6, &v)) return 2;
      max_neighbours = (int)v;
    } else if (const char* s7 = suffix("--threads=")) {
      if (!integer(s7, &v)) return 2;
      threads = (int)v;
    } else if (const char* s8 = suffix("--min-anchors=")) {
      if (!integer(s8, &v)) return 2;
      min_anchors = v;
    } else if (const char* s9 = suffix("--min-activations=")) {
      if (!integer(s9, &v)) return 2;
      min_activations = v;
    } else if (const char* sa = suffix("--max-uncertified=")) {
      if (!integer(sa, &v)) return 2;
      min_uncertified_report = v;
    } else if (const char* sb = suffix("--min-far-pairs=")) {
      if (!integer(sb, &v)) return 2;
      min_far_pairs = v;
    } else if (const char* sq = suffix("--min-q4=")) {
      if (!integer(sq, &v)) return 2;
      min_q4 = v;
    } else if (const char* sj = suffix("--judge-census=")) {
      if (!integer(sj, &v)) return 2;
      judge_census = (int)v;
    } else if (const char* sc = suffix("--judge-arity=")) {
      if (!integer(sc, &v) || v < 1 || v > 4) return 2;
      judge_arity = (int)v;
    } else if (const char* sm = suffix("--mode=")) {
      mode = sm;
    } else if (const char* sf = suffix("--family=")) {
      const std::string f = sf;
      if (f == "uniform") family = mhgp3v::CloudFamily::kUniform;
      else if (f == "terrain") family = mhgp3v::CloudFamily::kTerrain;
      else if (f == "scanline_single_pass") family = mhgp3v::CloudFamily::kScanlineSinglePass;
      else if (f == "scanline_overlap_multiecho")
        family = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
      else return 2;
    } else if (const char* si = suffix("--inject=")) {
      const std::string what = si;
      if (what == "cap-drop-strictness") inject.cap_drop_strictness = true;
      else if (what == "cap-nine") inject.cap_nine = true;
      else if (what == "cap-corner-any") inject.cap_corner_any = true;
      else if (what == "cap-half-radius") inject.cap_half_radius = true;
      else if (what == "census-drop-ties") inject.census_drop_ties = true;
      else if (what == "rho-min-corner") inject.rho_min_corner = true;
      else if (what == "rho-first-corner") inject.rho_first_corner = true;
      else if (what == "rho-kth-short") inject.rho_kth_short = true;
      else if (what == "locate-drop-boundary") inject.locate_drop_boundary = true;
      else if (what == "insphere-sign-flip") inject.insphere_sign_flip = true;
      else return 2;
    } else {
      std::printf("REFUS : argument inconnu %s\n", arg.c_str());
      return 2;
    }
  }
  if (n < 5 || grid_m < 1 || grid_m > 24 || kmin < 1 || max_neighbours < kmin) {
    std::printf("REFUS : parametres hors domaine\n");
    return 2;
  }
  if (mode != "fixture" && mode != "profile" && mode != "census" && mode != "directional") {
    std::printf("REFUS : mode inconnu %s\n", mode.c_str());
    return 2;
  }
  if (threads <= 0) threads = (int)std::thread::hardware_concurrency();
  if (threads <= 0) threads = 1;
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(family, n);

  const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(family, n, coord, seed);
  {
    // LE VECTEUR NUL N'A PAS DE DIRECTION. Deux positions colocalisees
    // rendraient `locate_cells` muet et feraient disparaitre silencieusement
    // une cible. Le profil u16 refuse les doublons : on le VERIFIE ici.
    std::vector<std::array<i64, 3>> keys;
    keys.reserve(pts.size());
    for (const mhgp::P3& p : pts) keys.push_back({p.x, p.y, p.z});
    std::sort(keys.begin(), keys.end());
    if (std::adjacent_find(keys.begin(), keys.end()) != keys.end()) {
      std::printf("ECHEC : positions colocalisees dans le nuage — direction nulle,"
                  " hors du profil u16 a positions deux a deux distinctes\n");
      return 3;
    }
  }
  if ((int)pts.size() != n) {
    std::printf("ECHEC : le generateur a publie %d points au lieu de %d\n", (int)pts.size(), n);
    return 3;
  }
  {
    // FIXTURE PERMANENTE DE L'ORIENTATION q4. Le determinant InSphere
    // developpe sur la colonne des normes est de signe OPPOSE a `orient3d`
    // pour un point strictement interieur. La regle `==` a ete livree une
    // fois : elle faisait passer q4 de 202 a 3 activations a n=70. Ce
    // contre-exemple minimal est gravé et exercé a chaque execution.
    const mhgp::P3 ta{100, 100, 100}, tb{100, -100, -100}, tc{-100, 100, -100},
        td{-100, -100, 100};
    const mhgp::P3 centre{0, 0, 0}, loin{1000, 0, 0};
    const judge::Q4Ball tb4 = judge::q4_ball(ta, tb, tc, td);
    const i128 din = judge::det4_insphere(ta, tb, tc, td, centre);
    const i128 dout = judge::det4_insphere(ta, tb, tc, td, loin);
    auto strict_in = [&](i128 det) {
      if (det == 0) return false;
      return inject.insphere_sign_flip ? ((det > 0) == (tb4.orient > 0))
                                       : ((det > 0) != (tb4.orient > 0));
    };
    const bool in_ok = strict_in(din);
    const bool out_ok = !strict_in(dout);
    if (!tb4.ok || !tb4.positive || !in_ok || !out_ok) {
      if (inject.insphere_sign_flip) {
        std::printf("OK : mutant tue par la fixture d'orientation q4 — le centre du"
                    " tetraedre n'est plus declare interieur\n");
        return 4;
      }
      std::printf("ECHEC : la fixture d'orientation q4 est violee"
                  " (support positif=%d, centre interieur=%d, point lointain exterieur=%d)\n",
                  (int)tb4.positive, (int)in_ok, (int)out_ok);
      return 3;
    }
    if (inject.insphere_sign_flip) {
      std::printf("ECHEC : MUTANT SURVIVANT — la fixture d'orientation q4 ne l'a pas vu\n");
      return 3;
    }
  }
  const DirGrid grid = make_dir_grid(grid_m);
  mhgp3v::MortonLbvh tree;
  tree.build(pts, 8);

  std::printf("famille=%s n=%d coord=%d graine=%lld grille=octaedre_m%d"
              " (%d sommets, %d cellules) kmin=%d voisins_max=%d threads=%d mode=%s\n",
              mhgp3v::cloud_family_name(family), n, coord, seed, grid_m,
              (int)grid.vertices.size(), (int)grid.cells.size(), kmin, max_neighbours, threads,
              mode.c_str());

  // -------------------------------------------------------------------------
  // Passe commune : certificat par ancre.
  // -------------------------------------------------------------------------
  std::vector<int> take_of((std::size_t)n, 0);
  std::vector<i64> radius2_of((std::size_t)n, 0);
  std::atomic<i64> corner_tests{0};
  std::atomic<int> next_anchor{0};
  const auto t_start = std::chrono::steady_clock::now();
  if (mode != "directional") {
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
      pool.emplace_back([&]() {
        std::vector<Neighbour> nbrs;
        CertifyScratch scratch;
        i64 local_tests = 0;
        for (;;) {
          const int a = next_anchor.fetch_add(1);
          if (a >= n) break;
          const int want = max_neighbours < n - 1 ? max_neighbours : n - 1;
          knn(tree, pts, a, want, &nbrs);
          const std::size_t take = certified_take(pts[(std::size_t)a], nbrs, grid, kmin, &scratch,
                                                  pts, inject, &local_tests);
          take_of[(std::size_t)a] = (int)take;
          radius2_of[(std::size_t)a] = take > 0 ? nbrs[take - 1].d2 : 0;
        }
        corner_tests.fetch_add(local_tests);
      });
    for (std::thread& th : pool) th.join();
  }
  const auto t_cert = std::chrono::steady_clock::now();

  i64 certified = 0, uncertified = 0, sum_take = 0, max_take = 0;
  std::vector<double> takes;
  takes.reserve((std::size_t)n);
  for (int a = 0; a < n; ++a) {
    if (take_of[(std::size_t)a] > 0) {
      ++certified;
      sum_take += take_of[(std::size_t)a];
      if (take_of[(std::size_t)a] > max_take) max_take = take_of[(std::size_t)a];
      takes.push_back((double)take_of[(std::size_t)a]);
    } else {
      ++uncertified;
    }
  }
  const double cert_s = std::chrono::duration<double>(t_cert - t_start).count();
  if (mode != "directional")
  std::printf("certificat : %lld ancres certifiees, %lld non certifiees (%.4f %%),"
              " M* moyen=%.2f p50=%.0f p95=%.0f p99=%.0f max=%lld,"
              " %lld tests de sommet, %.3f s (%d threads, machine de mesure)\n",
              certified, uncertified, 100.0 * (double)uncertified / (double)n,
              certified ? (double)sum_take / (double)certified : 0.0, percentile_of(takes, 0.50),
              percentile_of(takes, 0.95), percentile_of(takes, 0.99), max_take,
              (i64)corner_tests.load(), cert_s, threads);

  if (min_anchors > 0 && certified < min_anchors) {
    std::printf("ECHEC : plancher d'ancres certifiees %lld non atteint (%lld)\n", min_anchors,
                certified);
    return 3;
  }
  if (min_uncertified_report >= 0 && uncertified > min_uncertified_report) {
    std::printf("ECHEC : %lld ancres non certifiees au-dela du plafond %lld\n", uncertified,
                min_uncertified_report);
    return 3;
  }

  // -------------------------------------------------------------------------
  // Mode `fixture` : le juge exhaustif confronte le theoreme sur les 3 arites.
  // -------------------------------------------------------------------------
  if (mode == "fixture") {
    if (n > 400 && judge_arity > 1) {
      std::printf("REFUS : le juge exhaustif par arites est borne a 400 points\n");
      return 2;
    }
    // -----------------------------------------------------------------------
    // JUGE A — la contraposee DIRECTE du theoreme, sur toutes les paires
    // lointaines et pas seulement sur les activations : pour toute ancre
    // certifiee au rayon r et tout point y a distance >= r, la boule de
    // diametre [x,y] doit contenir au moins kmin interieurs stricts. C'est la
    // clause que le certificat achete ; elle est testable exactement et elle
    // est bien plus sensible qu'un balayage d'activations.
    // -----------------------------------------------------------------------
    i64 far_pairs = 0, far_violations = 0;
    for (int a = 0; a < n; ++a) {
      if (take_of[(std::size_t)a] == 0) continue;
      const i64 r2 = radius2_of[(std::size_t)a];
      const mhgp::P3& px = pts[(std::size_t)a];
      for (int y = 0; y < n; ++y) {
        if (y == a) continue;
        const i64 dx = pts[(std::size_t)y].x - px.x, dy = pts[(std::size_t)y].y - px.y,
                  dz = pts[(std::size_t)y].z - px.z;
        if (dx * dx + dy * dy + dz * dz < r2) continue;
        ++far_pairs;
        int inside = 0;
        for (int z = 0; z < n && inside < kmin; ++z) {
          if (z == a || z == y) continue;
          if (judge::inside_q2(px, pts[(std::size_t)y], pts[(std::size_t)z])) ++inside;
        }
        if (inside < kmin) ++far_violations;
      }
    }
    std::printf("juge A (paires lointaines) : %lld paires a distance >= r,"
                " %lld violations du seuil de %d interieurs\n",
                far_pairs, far_violations, kmin);
    i64 acts2 = 0, acts3 = 0, acts4 = 0, checked = 0, violations = 0;
    // Pour chaque activation (support, boule), le theoreme exige que TOUT le
    // support et TOUS les interieurs soient a distance^2 < R2(x) de chaque
    // point de support certifie.
    auto verify = [&](const std::vector<int>& support, const std::vector<int>& interior) {
      for (int x : support) {
        const i64 r2 = radius2_of[(std::size_t)x];
        if (take_of[(std::size_t)x] == 0) continue;  // ancre non certifiee : hors portee
        ++checked;
        const mhgp::P3& px = pts[(std::size_t)x];
        auto far = [&](int z) {
          const i64 dx = pts[(std::size_t)z].x - px.x, dy = pts[(std::size_t)z].y - px.y,
                    dz = pts[(std::size_t)z].z - px.z;
          return dx * dx + dy * dy + dz * dz >= r2;
        };
        for (int z : support)
          if (z != x && far(z)) { ++violations; return; }
        for (int z : interior)
          if (far(z)) { ++violations; return; }
      }
    };
    std::vector<int> interior;
    if (judge_arity >= 2)
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j) {
        interior.clear();
        for (int z = 0; z < n && (int)interior.size() <= kmin; ++z) {
          if (z == i || z == j) continue;
          if (judge::inside_q2(pts[(std::size_t)i], pts[(std::size_t)j], pts[(std::size_t)z]))
            interior.push_back(z);
        }
        if ((int)interior.size() > kmin - 1) continue;  // p >= 10 : tombstone
        ++acts2;
        verify({i, j}, interior);
      }
    if (judge_arity >= 3)
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        for (int k = j + 1; k < n; ++k) {
          const judge::Q3Ball ball =
              judge::q3_ball(pts[(std::size_t)i], pts[(std::size_t)j], pts[(std::size_t)k]);
          if (!ball.ok || !ball.positive) continue;
          interior.clear();
          for (int z = 0; z < n && (int)interior.size() <= kmin; ++z) {
            if (z == i || z == j || z == k) continue;
            if (judge::inside_q3(ball, pts[(std::size_t)i], pts[(std::size_t)z]))
              interior.push_back(z);
          }
          if ((int)interior.size() > kmin - 2) continue;  // p >= 9 : tombstone en q3
          ++acts3;
          verify({i, j, k}, interior);
        }
    if (judge_arity >= 4)
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        for (int k = j + 1; k < n; ++k)
          for (int l = k + 1; l < n; ++l) {
            const judge::Q4Ball ball =
                judge::q4_ball(pts[(std::size_t)i], pts[(std::size_t)j], pts[(std::size_t)k],
                               pts[(std::size_t)l]);
            if (!ball.ok || !ball.positive) continue;
            interior.clear();
            for (int z = 0; z < n && (int)interior.size() <= kmin; ++z) {
              if (z == i || z == j || z == k || z == l) continue;
              const i128 det = judge::det4_insphere(
                  pts[(std::size_t)i], pts[(std::size_t)j], pts[(std::size_t)k],
                  pts[(std::size_t)l], pts[(std::size_t)z]);
              // ORIENTATION : verifiee hors bande sur un tetraedre de
              // circumcentre l'origine — le centre doit etre interieur, un
              // point lointain exterieur. Le determinant developpe ci-dessus
              // est de signe OPPOSE a `orient` pour un point interieur ; la
              // regle `==` etait fausse et surdeclarait les exterieurs.
              const bool strictly_in = inject.insphere_sign_flip
                                           ? ((det > 0) == (ball.orient > 0))
                                           : ((det > 0) != (ball.orient > 0));
              if (det != 0 && strictly_in) interior.push_back(z);
            }
            if ((int)interior.size() > kmin - 3) continue;  // p >= 8 : tombstone en q4
            ++acts4;
            verify({i, j, k, l}, interior);
          }
    std::printf("juge exhaustif : q2=%lld q3=%lld q4=%lld activations retenues,"
                " %lld verifications de support, %lld violations\n",
                acts2, acts3, acts4, checked, violations);
    if (min_q4 > 0 && acts4 < min_q4) {
      std::printf("ECHEC : plancher q4 %lld non atteint (%lld) — l'orientation ou le"
                  " support positif ont regresse\n",
                  min_q4, acts4);
      return 3;
    }
    const i64 total_acts = acts2 + acts3 + acts4;
    if (min_activations > 0 && total_acts < min_activations) {
      std::printf("ECHEC : plancher d'activations %lld non atteint (%lld)\n", min_activations,
                  total_acts);
      return 3;
    }
    if (checked == 0 && far_pairs == 0) {
      std::printf("ECHEC : aucune verification exercee (vert par vacuite)\n");
      return 3;
    }
    if (min_far_pairs > 0 && far_pairs < min_far_pairs) {
      std::printf("ECHEC : plancher de paires lointaines %lld non atteint (%lld)\n", min_far_pairs,
                  far_pairs);
      return 3;
    }
    if (inject.rho_min_corner || inject.rho_first_corner || inject.rho_kth_short ||
        inject.locate_drop_boundary) {  // NOLINT
      std::printf("REFUS : injection rho-*/locate-* sans effet en mode fixture ;"
                  " elle appartient au mode directionnel\n");
      return 2;
    }
    const bool mutated = inject.cap_drop_strictness || inject.cap_nine || inject.cap_corner_any ||
                         inject.cap_half_radius || inject.insphere_sign_flip;
    const i64 total_violations = violations + far_violations;
    if (total_violations > 0) {
      if (mutated) {
        std::printf("OK : mutant tue — %lld violations (%lld paires lointaines,"
                    " %lld supports d'activation)\n",
                    total_violations, far_violations, violations);
        return 4;
      }
      std::printf("ECHEC : le theoreme de localite est viole %lld fois (%lld paires"
                  " lointaines, %lld supports d'activation)\n",
                  total_violations, far_violations, violations);
      return 1;
    }
    if (mutated) {
      std::printf("ECHEC : MUTANT SURVIVANT — l'injection n'a produit aucune violation\n");
      return 3;
    }
    std::printf("OK : localite certifiee — juge A sur %lld paires lointaines,"
                " juge B sur %lld supports d'activation, aucun contre-exemple\n",
                far_pairs, checked);
    return 0;
  }

  // -------------------------------------------------------------------------
  // Mode `census` : la TAILLE DE SORTIE q2 exacte via la localite certifiee.
  // -------------------------------------------------------------------------
  if (mode == "census") {
    if (uncertified > 0) {
      std::printf("REFUS : %lld ancres non certifiees — le census q2 ne peut pas etre"
                  " declare complet ; augmenter --max-neighbours ou traiter la"
                  " frontiere par la voie duale\n",
                  uncertified);
      return 3;
    }
    std::atomic<i64> emitted{0}, tomb{0}, tested{0}, interior_tests{0};
    std::atomic<int> cursor{0};
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
      pool.emplace_back([&]() {
        std::vector<Neighbour> nbrs;
        i64 le = 0, lt = 0, ls = 0, li = 0;
        for (;;) {
          const int a = cursor.fetch_add(1);
          if (a >= n) break;
          const i64 r2 = radius2_of[(std::size_t)a];
          const int want = max_neighbours < n - 1 ? max_neighbours : n - 1;
          knn(tree, pts, a, want, &nbrs);
          const mhgp::P3& px = pts[(std::size_t)a];
          // Fenetre exacte : tous les points a d^2 < r2 (les egalites sont
          // hors des interieurs stricts mais restent des cibles possibles).
          std::size_t window = 0;
          while (window < nbrs.size() &&
                 (inject.census_drop_ties ? nbrs[window].d2 < r2 : nbrs[window].d2 <= r2))
            ++window;
          for (std::size_t j = 0; j < window; ++j) {
            if (nbrs[j].d2 >= r2) continue;  // diametre >= r : deja tombstone
            const int y = nbrs[j].id;
            if (y < a) continue;  // owner canonique : toutes les ancres sont certifiees
            ++ls;
            const mhgp::P3& py = pts[(std::size_t)y];
            int inside = 0;
            for (std::size_t z = 0; z < window && inside < kmin; ++z) {
              if (nbrs[z].id == y) continue;
              ++li;
              if (judge::inside_q2(px, py, pts[(std::size_t)nbrs[z].id])) ++inside;
            }
            if (inside < kmin) ++le; else ++lt;
          }
        }
        emitted.fetch_add(le);
        tomb.fetch_add(lt);
        tested.fetch_add(ls);
        interior_tests.fetch_add(li);
      });
    for (std::thread& th : pool) th.join();
    const double census_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const i64 all_pairs = (i64)n * (i64)(n - 1) / 2;
    std::printf("census q2 EXACT : %lld activations (p<=%d), %lld tombstones,"
                " %lld paires candidates sur %lld paires totales (%.6f %%),"
                " %lld tests d'interiorite, %.3f s\n",
                (i64)emitted.load(), kmin - 1, (i64)tomb.load(), (i64)tested.load(), all_pairs,
                100.0 * (double)tested.load() / (double)all_pairs, (i64)interior_tests.load(),
                census_s);
    std::printf("densite de sortie q2 : %.4f activations par point,"
                " %.4f paires candidates par point\n",
                (double)emitted.load() / (double)n, (double)tested.load() / (double)n);
    if (min_activations > 0 && (i64)emitted.load() < min_activations) {
      std::printf("ECHEC : plancher d'activations %lld non atteint (%lld)\n", min_activations,
                  (i64)emitted.load());
      return 3;
    }
    if (inject.census_drop_ties) {
      std::printf("NOTE : injection census-drop-ties active — la fenetre exclut les"
                  " egalites ; ce mode n'est pas une mesure recevable\n");
    }
    return 0;
  }

  // -------------------------------------------------------------------------
  // Mode `directional` : la mesure qui decide. Rayon PAR CELLULE, masse de
  // candidats q2 reellement retenue, cellules ouvertes, et — quand la lane est
  // fermee — le census q2 EXACT, c'est-a-dire la taille de sortie.
  // -------------------------------------------------------------------------
  if (mode == "directional") {
    std::atomic<i64> scanned{0}, cell_tests{0}, candidates{0}, open_cells{0},
        anchors_with_open{0}, emitted{0}, tombs{0}, interior_tests{0}, unbounded{0}, full_scans{0};
    std::atomic<int> cursor{0};
    std::vector<int> cand_of((std::size_t)n, 0);
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
      pool.emplace_back([&]() {
        std::vector<Neighbour> nbrs;
        std::vector<Rho2> radii;
        std::vector<std::vector<Rho2>> heaps;
        std::vector<int> hit;
        i64 lsc = 0, lct = 0, lca = 0, lop = 0, lawo = 0, lem = 0, ltb = 0, lit = 0, lub = 0, lfs = 0;
        for (;;) {
          const int a = cursor.fetch_add(1);
          if (a >= n) break;
          const int want = max_neighbours < n - 1 ? max_neighbours : n - 1;
          knn(tree, pts, a, want, &nbrs);
          const mhgp::P3& px = pts[(std::size_t)a];
          compute_cell_radii(px, nbrs, grid, kmin, pts, &radii, &heaps, &lsc, &lct, inject);
          i64 anchor_open = 0;
          for (const Rho2& r : radii)
            if (r.den == 0) ++anchor_open;
          lop += anchor_open;
          // La fenetre kNN ne ferme la lane que si TOUT rayon de cellule y
          // tient. Une cellule ouverte n'a aucune borne ; une cellule fermee
          // au-dela du dernier voisin lu perdrait des candidats sans le dire.
          // Dans les deux cas on bascule sur l'univers complet : le cout est
          // celui du cone reel, jamais un cap.
          bool beyond_window = anchor_open > 0;
          if (!nbrs.empty())
            for (const Rho2& r : radii)
              if (r.den != 0 && !within_rho(nbrs.back().d2, r)) { beyond_window = true; break; }
          const bool full_scan = beyond_window && (int)nbrs.size() < n - 1;
          if (anchor_open > 0) ++lawo;
          if (full_scan) ++lfs;
          const int universe = full_scan ? n : (int)nbrs.size();
          auto uid = [&](int t) { return full_scan ? t : nbrs[(std::size_t)t].id; };
          auto ud2 = [&](int t) {
            if (!full_scan) return nbrs[(std::size_t)t].d2;
            const mhgp::P3& q = pts[(std::size_t)t];
            const i64 dx = q.x - px.x, dy = q.y - px.y, dz = q.z - px.z;
            return dx * dx + dy * dy + dz * dz;
          };
          // Masse candidate : y retenu si d_y^2 <= max des r_c^2 des cellules
          // fermees dont le triangle contient sa direction (conservateur).
          i64 local_cand = 0;
          for (int t = 0; t < universe; ++t) {
            const int yid = uid(t);
            if (yid == a) continue;
            const i64 dy2 = ud2(t);
            const mhgp::P3& py = pts[(std::size_t)yid];
            const i64 s[3] = {py.x - px.x, py.y - px.y, py.z - px.z};
            locate_cells(s, grid_m, grid, grid.cells_at, &hit, inject.locate_drop_boundary);
            bool keep = false;
            for (int c : hit)
              if (within_rho(dy2, radii[(std::size_t)c])) { keep = true; break; }
            if (!keep) continue;
            ++local_cand;
            if (yid < a) continue;  // owner canonique
            // Classification exacte : tous les interieurs de la boule de
            // diametre [x,y] sont a distance < d_y. L'univers balaye les
            // contient tous des que la lane est close.
            if (!full_scan && dy2 > nbrs.back().d2) { ++lub; continue; }
            int inside = 0;
            for (int u = 0; u < universe && inside < kmin; ++u) {
              const int zid = uid(u);
              if (zid == a || zid == yid) continue;
              if (ud2(u) >= dy2) { if (!full_scan) break; else continue; }
              ++lit;
              if (judge::inside_q2(px, py, pts[(std::size_t)zid])) ++inside;
            }
            if (inside < kmin) ++lem; else ++ltb;
          }
          cand_of[(std::size_t)a] = (int)local_cand;
          lca += local_cand;
        }
        scanned.fetch_add(lsc);
        cell_tests.fetch_add(lct);
        candidates.fetch_add(lca);
        open_cells.fetch_add(lop);
        anchors_with_open.fetch_add(lawo);
        emitted.fetch_add(lem);
        tombs.fetch_add(ltb);
        interior_tests.fetch_add(lit);
        unbounded.fetch_add(lub);
        full_scans.fetch_add(lfs);
      });
    for (std::thread& th : pool) th.join();
    const double sec =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const i64 all_pairs = (i64)n * (i64)(n - 1) / 2;
    std::vector<double> cands;
    cands.reserve((std::size_t)n);
    for (int a = 0; a < n; ++a) cands.push_back((double)cand_of[(std::size_t)a]);
    std::printf("directionnel : %lld points lus, %lld tests cellule-point,"
                " %lld cellules ouvertes sur %lld (%lld ancres concernees),"
                " %.3f s (%d threads, machine de mesure)\n",
                (i64)scanned.load(), (i64)cell_tests.load(), (i64)open_cells.load(),
                (i64)n * (i64)grid.cells.size(), (i64)anchors_with_open.load(), sec, threads);
    std::printf("masse candidate q2 : %lld arcs orientes (%.2f par ancre, p50=%.0f"
                " p95=%.0f p99=%.0f max=%.0f) contre %lld paires de C(n,2)"
                " — rapport %.8f\n",
                (i64)candidates.load(), (double)candidates.load() / (double)n,
                percentile_of(cands, 0.50), percentile_of(cands, 0.95),
                percentile_of(cands, 0.99), percentile_of(cands, 1.00), all_pairs,
                (double)candidates.load() / (2.0 * (double)all_pairs));
    std::printf("census q2 : %lld activations (p<=%d), %lld tombstones,"
                " %lld tests d'interiorite, %lld ancres en balayage complet"
                " (cellule ouverte), %lld ancres a lane NON CLOSE\n",
                (i64)emitted.load(), kmin - 1, (i64)tombs.load(), (i64)interior_tests.load(),
                (i64)full_scans.load(), (i64)unbounded.load());
    std::printf("densite de sortie q2 : %.4f activations par point\n",
                (double)emitted.load() / (double)n);
    // LE JUGE DU CENSUS — enumeration exhaustive de C(n,2) avec le predicat
    // diametral direct, sans calotte, sans cellule, sans rayon. Toute perte
    // d'activation par un filtre non conservateur est vue ici.
    // Les injections `cap-*` visent le certificat GLOBAL (modes fixture et
    // profile). Elles n'ont aucun effet sur le chemin par cellule : les
    // accepter ici ferait passer un mutant pour survivant alors qu'il n'est
    // simplement pas exerce. La campagne le REFUSE avant tout verdict.
    if (inject.cap_drop_strictness || inject.cap_nine || inject.cap_corner_any ||
        inject.cap_half_radius) {
      std::printf("REFUS : injection cap-* sans effet en mode directionnel ;"
                  " elle appartient au mode fixture\n");
      return 2;
    }
    const bool mutated_dir = inject.rho_min_corner || inject.rho_first_corner ||
                             inject.rho_kth_short || inject.locate_drop_boundary;
    if (judge_census) {
      if (n > 4000) {
        std::printf("REFUS : le juge exhaustif du census est borne a 4000 points\n");
        return 2;
      }
      std::atomic<i64> ref{0};
      std::atomic<int> jc{0};
      std::vector<std::thread> jpool;
      for (int t = 0; t < threads; ++t)
        jpool.emplace_back([&]() {
          i64 local = 0;
          for (;;) {
            const int i = jc.fetch_add(1);
            if (i >= n) break;
            for (int j = i + 1; j < n; ++j) {
              int inside = 0;
              for (int z = 0; z < n && inside < kmin; ++z) {
                if (z == i || z == j) continue;
                if (judge::inside_q2(pts[(std::size_t)i], pts[(std::size_t)j],
                                     pts[(std::size_t)z]))
                  ++inside;
              }
              if (inside < kmin) ++local;
            }
          }
          ref.fetch_add(local);
        });
      for (std::thread& th : jpool) th.join();
      const i64 reference = (i64)ref.load(), got = (i64)emitted.load();
      std::printf("juge du census : exhaustif=%lld directionnel=%lld ecart=%lld\n", reference,
                  got, got - reference);
      if (reference == 0) {
        std::printf("ECHEC : juge vide (vert par vacuite)\n");
        return 3;
      }
      if (got != reference) {
        if (mutated_dir) {
          std::printf("OK : mutant tue — le census directionnel differe de %lld\n",
                      got - reference);
          return 4;
        }
        std::printf("ECHEC : le census directionnel ne reproduit pas le juge exhaustif\n");
        return 1;
      }
      if (mutated_dir) {
        std::printf("ECHEC : MUTANT SURVIVANT — census identique au juge exhaustif\n");
        return 3;
      }
      std::printf("OK : census directionnel EXACT contre le juge exhaustif\n");
    } else if (mutated_dir) {
      std::printf("REFUS : une injection exige --judge-census=1\n");
      return 2;
    }
    if ((i64)unbounded.load() > 0)
      std::printf("NOTE : %lld paires candidates depassent la fenetre kNN ; le census"
                  " q2 ci-dessus est INCOMPLET et n'est pas une mesure recevable\n",
                  (i64)unbounded.load());
    if (min_activations > 0 && (i64)emitted.load() < min_activations) {
      std::printf("ECHEC : plancher d'activations %lld non atteint (%lld)\n", min_activations,
                  (i64)emitted.load());
      return 3;
    }
    return 0;
  }

  // -------------------------------------------------------------------------
  // Mode `profile` : seule la mesure de M* et du cout du certificat.
  // -------------------------------------------------------------------------
  std::printf("profil : le travail local certifie est M* ; la generation par ancre"
              " coute O(sum M*) etats, soit %lld etats au total contre %lld paires"
              " de l'enumeration C(n,2) (rapport %.6f)\n",
              sum_take, (i64)n * (i64)(n - 1) / 2,
              (double)sum_take / ((double)n * (double)(n - 1) / 2.0));
  if (min_anchors > 0 && certified < min_anchors) return 3;
  return 0;
}
