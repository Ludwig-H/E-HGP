// MorseHGP3D v3 — PROTOTYPE DE SOURCE DIRECTE, sans arrangement ni mosaïque.
//
// Tout le chemin précédent construit le catalogue en PARCOURANT l'arrangement
// relevé : il visite $V$ sommets pour produire une sortie beaucoup plus petite.
// Le profileur mesure environ 1 270 sommets par point à $n=800$ et à densité
// fixe, contre 300 sphères par point. Aucun débit ne rattrape ce facteur, et
// c'est pourquoi la route passe ailleurs.
//
// Ce prototype-ci n'énumère aucun sommet. Il énumère directement les SUPPORTS,
// chacun depuis son ancre $p=\min U$, dans un voisinage dont la complétude est
// un théorème. Le mot « certifiée » n'est pas employé : ce que ce fichier
// établit est un ACCORD RELATIF à `mhgp::sphere_*` et `mhgp::miniball_of`, qu'il
// partage avec la référence, sur les campagnes exercées.
//
// ---------------------------------------------------------------------------
// LE LEMME DE RAYON, ET POURQUOI IL FERME LE VOISINAGE
// ---------------------------------------------------------------------------
//
// Pour $q\in\{2,3,4\}$ poser $t_q=s_{\max}-q+1$. On subdivise la boîte du nuage
// en feuilles à bornes entières, et chaque feuille authentifie $t_q$ PointId
// distincts $W_C$ tels que, pour tout $w\in W_C$,
//
//   somme_i max{ (w_i - C_i^-)^2, (w_i - C_i^+)^2 } < Q_q,
//
// c'est-à-dire : $w$ est à distance carrée strictement inférieure à $Q_q$ du coin
// le PLUS ÉLOIGNÉ de la feuille — donc de n'importe quel point de la feuille.
//
// Soit alors $B_U$ une miniboule propre de support $q$ vérifiant
// $q+\lvert I\rvert\leq s_{\max}$. Son centre appartient à $\mathrm{conv}(U)$,
// donc à la boîte, donc à une feuille $C$. Si $\beta(U)\geq Q_q$, les $t_q$
// témoins de $C$ sont tous à distance carrée $<Q_q\leq\beta$ du centre, donc tous
// STRICTEMENT intérieurs, donc $\lvert I\rvert\geq t_q$ et
// $q+\lvert I\rvert\geq s_{\max}+1$ : contradiction. Donc $\beta<Q_q$.
//
// Pour $p\in U$, tout $x$ de la boule fermée vérifie alors
// $\lVert x-p\rVert^2\leq4\beta<4Q_q$. La liste exacte
// $N_q(p)=\{x\neq p:\mathrm{dist}^2(x,p)<4Q_q\}$ contient donc le support entier,
// l'intérieur entier et la coquille entière.
//
// ---------------------------------------------------------------------------
// L'ORDRE N'EST PAS CIRCULAIRE, ET C'EST LE POINT DÉLICAT
// ---------------------------------------------------------------------------
//
// Le lemme conclut $\beta<Q_q$ SOUS l'hypothèse $q+\lvert I\rvert\leq s_{\max}$,
// et c'est justement $\lvert I\rvert$ que le census doit calculer. Employer
// $N_q(p)$ pour faire ce census avant d'avoir établi $\beta<Q_q$ serait circulaire.
//
// La banque de témoins brise le cercle, et dans le bon sens :
//
//   * tous les $t_q$ témoins strictement intérieurs  ->  `AboveInteriorWindow`,
//     et le candidat est REFUSÉ sans census : $\lvert I\rvert\geq t_q$ suffit ;
//   * un témoin non strictement intérieur            ->  $\beta\leq\mathrm{dist}^2<Q_q$,
//     PROUVÉ avant le census, qui devient alors global et complet dans $N_q(p)$.
//
// ---------------------------------------------------------------------------
// CE QUE L'AUDIT A CASSÉ, ET QUI EST RÉPARÉ ICI
// ---------------------------------------------------------------------------
//
// Le premier palier de ce fichier avait quatre défauts que l'audit
// `AUDIT_SOURCE_DIRECTE_24AD3D37.md` a reproduits, et ils étaient tous du même
// genre : le programme AFFIRMAIT plus que ce qu'il vérifiait.
//
//   * `--judge 0` imprimait quand même « rend exactement le catalogue fermé ».
//     Une exactitude annoncée en l'absence d'oracle est pire qu'un silence. Les
//     trois modes sont maintenant EXCLUSIFS, la combinaison incohérente est
//     refusée, et seul le mode jugé a le droit de conclure.
//   * la map de sortie était indexée par la coquille et l'affectation ÉCRASAIT
//     les doublons : un mutant qui retire la restriction `z > p` émettait 126
//     fois au lieu de 56 sans qu'aucun compteur ne bouge. L'unicité est
//     maintenant reçue, et le mutant vit dans le binaire sous
//     `--force-both-directions`.
//   * le payload jetait `members` après en avoir pris la taille : deux sorties de
//     même rang et d'intérieurs différents étaient indiscernables. Le pool
//     ordonné complet est maintenant construit ET comparé.
//   * `n < t_q` et le plafond de cellules sortaient sur un message générique. Ce
//     sont maintenant des STATUTS TYPÉS avec repli racine réellement appliqué.
//
// Deux bornes annoncées étaient aussi fausses. La dernière cellule nominale peut
// dépasser le maximum du nuage, donc la distance brute au coin n'est pas bornée
// par $3\cdot65535^2$ mais par $3(2\cdot65535)^2<2^{36}$. Et `bound_t` en `u64`
// déborde silencieusement dès $n\approx13\,500$ : les compteurs de preuve sont
// passés en 128 bits non signés.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"

using mhgp::P3;
using mhgp::i32;
using mhgp::i128;

namespace {

using u128 = unsigned __int128;

// LES COMPTEURS DE PREUVE NE PEUVENT PAS DÉBORDER EN SILENCE. Dans un graphe
// complet à l'arité quatre, `bound_t` vaut `(n-1) * C(n,4)` : il dépasse
// `UINT64_MAX` dès `n = 13 468` et vaut 7,23 fois la capacité à `n = 20 000`.
std::string u128_text(u128 v) {
  if (v == 0) return "0";
  char buffer[48];
  int i = (int)sizeof buffer;
  buffer[--i] = '\0';
  while (v > 0) { buffer[--i] = (char)('0' + (int)(v % 10)); v /= 10; }
  return std::string(buffer + i);
}

u128 binomial(long long m, int k) {
  if (m < k) return 0;
  u128 r = 1;
  for (int i = 0; i < k; ++i) r = r * (u128)(m - i) / (u128)(i + 1);
  return r;
}

// ---------------------------------------------------------------------------
// LE COVER
// ---------------------------------------------------------------------------
enum class CoverOutcome {
  kGridCertified = 0,       // la grille demandée certifie toutes les feuilles
  kRootFallback,            // la grille dépasse la racine : la racine est reprise
  kSmallCloudDirect,        // n < t_q : la banque ne peut jamais conclure, on l'ôte
  kCellCapExceeded,         // la grille demandée est refusée, repli racine appliqué
};

const char* cover_outcome_name(CoverOutcome o) {
  switch (o) {
    case CoverOutcome::kGridCertified: return "grille_certifiee";
    case CoverOutcome::kRootFallback: return "repli_racine";
    case CoverOutcome::kSmallCloudDirect: return "petit_nuage_direct";
    case CoverOutcome::kCellCapExceeded: return "plafond_cellules";
  }
  return "inconnu";
}

struct Cover {
  CoverOutcome outcome = CoverOutcome::kGridCertified;
  int arity = 0;
  int witnesses_per_leaf = 0;     // t_q, ou zéro quand la banque est ôtée
  long long leaf_side = 0;
  long long lo[3] = {0, 0, 0};
  int cells[3] = {1, 1, 1};
  long long q_value = 0;          // Q_q EFFECTIF, celui que le lemme emploie
  long long q_root = 0;
  std::vector<i32> witnesses;
  long long distance_tests = 0;
  std::vector<long long> per_leaf_effective;   // les Q effectifs, un par feuille
};

// La distance carrée du point `w` au coin le plus éloigné de la feuille.
//
// La dernière cellule NOMINALE dépasse le maximum du nuage — la grille pave la
// boîte par excès —, donc l'écart par axe peut atteindre deux fois l'étendue et
// non une. La borne sûre est `3 * (2 * 65535)^2 < 2^36`, très à l'aise en `i64`.
long long farthest_corner_distance2(const P3& w, const long long lo[3], long long side) {
  long long total = 0;
  const long long c[3] = {w.x, w.y, w.z};
  for (int d = 0; d < 3; ++d) {
    const long long a = c[d] - lo[d];
    const long long b = c[d] - (lo[d] + side);
    const long long da = a < 0 ? -a : a;
    const long long db = b < 0 ? -b : b;
    const long long m = da > db ? da : db;
    total += m * m;
  }
  return total;
}

// Le repli racine est EXACT sans certificat de coin : centre et point sont tous
// deux dans la boîte, donc leur distance carrée est au plus la somme des carrés
// des spans, strictement sous `Q_root`. Les témoins ne servent alors qu'à la
// fenêtre, et n'importe quels `t` PointId distincts conviennent.
void install_root_cover(const std::vector<P3>& pts, Cover* out, CoverOutcome outcome) {
  out->outcome = outcome;
  out->cells[0] = out->cells[1] = out->cells[2] = 1;
  out->leaf_side = 0;
  out->q_value = out->q_root;
  out->per_leaf_effective.assign(1, out->q_root);
  const int t = out->witnesses_per_leaf;
  out->witnesses.assign((std::size_t)t, -1);
  for (int i = 0; i < t && i < (int)pts.size(); ++i) out->witnesses[(std::size_t)i] = (i32)i;
}

bool build_cover(const std::vector<P3>& pts, int arity, int s_max, long long leaf_side,
                 long long cell_cap, Cover* out) {
  const int n = (int)pts.size();
  out->arity = arity;
  const int t_q = s_max - arity + 1;
  if (t_q < 1) return false;

  long long hi[3];
  out->lo[0] = pts[0].x; out->lo[1] = pts[0].y; out->lo[2] = pts[0].z;
  hi[0] = pts[0].x; hi[1] = pts[0].y; hi[2] = pts[0].z;
  for (const P3& p : pts) {
    const long long c[3] = {p.x, p.y, p.z};
    for (int d = 0; d < 3; ++d) {
      if (c[d] < out->lo[d]) out->lo[d] = c[d];
      if (c[d] > hi[d]) hi[d] = c[d];
    }
  }
  long long span2 = 0;
  for (int d = 0; d < 3; ++d) span2 += (hi[d] - out->lo[d]) * (hi[d] - out->lo[d]);
  out->q_root = span2 + 1;

  // `n < t_q` N'EST PAS UNE ERREUR. La note le dit : la banque ne peut alors
  // jamais conclure, puisque `|I| <= n - q < t_q`. On l'ôte, et le voisinage
  // racine est exact. C'est un statut, pas un échec.
  if (n < t_q) {
    out->witnesses_per_leaf = 0;
    install_root_cover(pts, out, CoverOutcome::kSmallCloudDirect);
    return true;
  }
  out->witnesses_per_leaf = t_q;

  if (leaf_side < 1) leaf_side = 1;
  long long total_cells = 1;
  bool capped = false;
  int cells[3] = {1, 1, 1};
  for (int d = 0; d < 3; ++d) {
    const long long extent = hi[d] - out->lo[d];
    const long long count = extent / leaf_side + 1;
    if (count > cell_cap || total_cells > cell_cap / count) { capped = true; break; }
    cells[d] = (int)count;
    total_cells *= count;
  }
  if (capped) {
    // LE PLAFOND EST UN STATUT, PAS UNE SORTIE. Le repli racine est toujours
    // disponible et il est exact; il ne prouve simplement aucun SLO.
    install_root_cover(pts, out, CoverOutcome::kCellCapExceeded);
    return true;
  }
  for (int d = 0; d < 3; ++d) out->cells[d] = cells[d];
  out->leaf_side = leaf_side;

  out->witnesses.assign((std::size_t)total_cells * (std::size_t)t_q, -1);
  out->per_leaf_effective.assign((std::size_t)total_cells, 0);
  out->q_value = 1;
  std::vector<std::pair<long long, i32>> ranked;
  for (int cz = 0; cz < out->cells[2]; ++cz)
    for (int cy = 0; cy < out->cells[1]; ++cy)
      for (int cx = 0; cx < out->cells[0]; ++cx) {
        const long long leaf_lo[3] = {out->lo[0] + (long long)cx * leaf_side,
                                      out->lo[1] + (long long)cy * leaf_side,
                                      out->lo[2] + (long long)cz * leaf_side};
        ranked.clear();
        ranked.reserve((std::size_t)n);
        for (i32 z = 0; z < n; ++z)
          ranked.push_back({farthest_corner_distance2(pts[(std::size_t)z], leaf_lo, leaf_side), z});
        out->distance_tests += n;
        // Les `t` plus petits, ex aequo départagés par PointId : le cover est
        // canonique, il ne dépend d'aucun ordre d'itération.
        std::partial_sort(ranked.begin(), ranked.begin() + t_q, ranked.end());
        const long long leaf_index = ((long long)cz * out->cells[1] + cy) * out->cells[0] + cx;
        long long local = 1;
        for (int t = 0; t < t_q; ++t) {
          out->witnesses[(std::size_t)(leaf_index * t_q + t)] = ranked[(std::size_t)t].second;
          if (ranked[(std::size_t)t].first + 1 > local) local = ranked[(std::size_t)t].first + 1;
        }
        out->per_leaf_effective[(std::size_t)leaf_index] = local;
        if (local > out->q_value) out->q_value = local;
      }
  // LA VALIDATION AVANT MULTIPLICATION, comme la note l'exige. Si la grille
  // demande plus que la racine, la racine est meilleure ET plus simple : on la
  // reprend, et les `Q` par feuille publiés deviennent EFFECTIFS.
  if (out->q_value < 1 || out->q_value > out->q_root) {
    install_root_cover(pts, out, CoverOutcome::kRootFallback);
    return true;
  }
  if (out->q_value > (long long)1 << 40) return false;   // 4*Q doit rester exact
  out->outcome = CoverOutcome::kGridCertified;
  return true;
}

long long floor_div(i128 a, i128 b) {
  i128 q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
  return (long long)q;
}

// La feuille d'un centre RATIONNEL, en arithmétique exacte. Le centre vaut
// `base + num/den` avec `den > 0`. Sur un support BIEN CENTRÉ le centre est dans
// l'enveloppe convexe du support, donc dans la boîte : le clamp ne doit jamais
// servir, et s'il sert c'est une violation d'invariant, pas un repli.
long long locate_leaf(const Cover& cover, const mhgp::Sphere& s, long long* steps,
                      bool* clamped) {
  ++*steps;
  if (cover.cells[0] == 1 && cover.cells[1] == 1 && cover.cells[2] == 1) return 0;
  const i128 base[3] = {(i128)s.base.x, (i128)s.base.y, (i128)s.base.z};
  const i128 num[3] = {s.nx, s.ny, s.nz};
  long long cell[3];
  for (int d = 0; d < 3; ++d) {
    const i128 relative = (base[d] - (i128)cover.lo[d]) * s.den + num[d];
    long long k = floor_div(relative, s.den * (i128)cover.leaf_side);
    if (k < 0) { k = 0; *clamped = true; }
    if (k >= cover.cells[d]) { k = cover.cells[d] - 1; *clamped = true; }
    cell[d] = k;
  }
  return (cell[2] * cover.cells[1] + cell[1]) * cover.cells[0] + cell[0];
}

// ---------------------------------------------------------------------------
// LES VOISINAGES
// ---------------------------------------------------------------------------
//
// `a` est la plus grande puissance de deux avec `a*a <= Q`. Deux points d'une
// même cellule de pas `a` sont à distance carrée `< 3 a^2 < 4 Q`; et comme
// `Q < 4 a^2`, tout voisin est dans l'un des `9^3` décalages. Les décalages dont
// la distance AABB minimale atteint déjà `4Q` sont rejetés exactement.
struct Neighbourhoods {
  std::vector<int> begin;
  std::vector<i32> ids;
  long long distance_tests = 0;
  long long degree_max = 0;
  unsigned long long bytes = 0;
};

void build_neighbourhoods(const std::vector<P3>& pts, long long q_value, Neighbourhoods* out) {
  const int n = (int)pts.size();
  long long a = 1;
  while ((a * 2) * (a * 2) <= q_value) a *= 2;
  const long long radius2 = 4 * q_value;

  long long lo[3] = {pts[0].x, pts[0].y, pts[0].z};
  for (const P3& p : pts) {
    const long long c[3] = {p.x, p.y, p.z};
    for (int d = 0; d < 3; ++d) if (c[d] < lo[d]) lo[d] = c[d];
  }
  auto cell_of = [&](const P3& p, long long c[3]) {
    c[0] = (p.x - lo[0]) / a; c[1] = (p.y - lo[1]) / a; c[2] = (p.z - lo[2]) / a;
  };
  std::map<std::array<long long, 3>, std::vector<i32>> buckets;
  for (i32 z = 0; z < n; ++z) {
    long long c[3];
    cell_of(pts[(std::size_t)z], c);
    buckets[{c[0], c[1], c[2]}].push_back(z);
  }

  out->begin.assign((std::size_t)n + 1, 0);
  std::vector<std::vector<i32>> lists((std::size_t)n);
  for (i32 p = 0; p < n; ++p) {
    long long c[3];
    cell_of(pts[(std::size_t)p], c);
    std::vector<i32>& list = lists[(std::size_t)p];
    for (int dz = -4; dz <= 4; ++dz)
      for (int dy = -4; dy <= 4; ++dy)
        for (int dx = -4; dx <= 4; ++dx) {
          long long floor2 = 0;
          const int delta[3] = {dx, dy, dz};
          for (int d = 0; d < 3; ++d) {
            const long long m = (delta[d] < 0 ? -delta[d] : delta[d]);
            const long long gap = m > 0 ? (m - 1) * a : 0;
            floor2 += gap * gap;
          }
          if (floor2 >= radius2) continue;
          const auto it = buckets.find({c[0] + dx, c[1] + dy, c[2] + dz});
          if (it == buckets.end()) continue;
          for (i32 z : it->second) {
            if (z == p) continue;
            ++out->distance_tests;
            const long long ex = (long long)pts[(std::size_t)z].x - pts[(std::size_t)p].x;
            const long long ey = (long long)pts[(std::size_t)z].y - pts[(std::size_t)p].y;
            const long long ez = (long long)pts[(std::size_t)z].z - pts[(std::size_t)p].z;
            if (ex * ex + ey * ey + ez * ez < radius2) list.push_back(z);
          }
        }
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    if ((long long)list.size() > out->degree_max) out->degree_max = (long long)list.size();
  }
  int total = 0;
  for (i32 p = 0; p < n; ++p) {
    out->begin[(std::size_t)p] = total;
    total += (int)lists[(std::size_t)p].size();
  }
  out->begin[(std::size_t)n] = total;
  out->ids.reserve((std::size_t)total);
  for (i32 p = 0; p < n; ++p)
    out->ids.insert(out->ids.end(), lists[(std::size_t)p].begin(), lists[(std::size_t)p].end());
  out->bytes = (unsigned long long)out->ids.size() * sizeof(i32) +
               (unsigned long long)out->begin.size() * sizeof(int);
}

// La sortie du prototype. Le pool ordonné COMPLET des membres en fait partie :
// sans lui, deux payloads de même rang et d'intérieurs différents sont
// indiscernables, et l'audit l'a dit.
struct Produced {
  mhgp::CriticalSphere sphere{};
  std::vector<i32> members;
};

struct SourceCounters {
  u128 candidates = 0;
  u128 bank_tests = 0;
  u128 census_tests = 0;
  u128 not_proper = 0;
  u128 above_window = 0;
  u128 complete = 0;
  u128 rank_above_smax = 0;
  u128 non_canonical_support = 0;
  u128 emitted = 0;              // ÉMISSIONS, pas entrées distinctes
  u128 locator_steps = 0;
  u128 bound_c = 0;
  u128 bound_t = 0;
};

// ---------------------------------------------------------------------------
// LA FORÊT, ET POURQUOI ELLE NE SE COMPARE PAS PAR INDICES
// ---------------------------------------------------------------------------
//
// Le catalogue n'est que la moitié du contrat : ce que le projet doit produire
// est la FORÊT des arbres de niveaux de densité, pour k = 1..K. `build_forest`
// la construit depuis un catalogue, en triant les événements par comparaison
// EXACTE des niveaux rationnels.
//
// Mais `ForestNode::source` est un indice DANS le catalogue, et l'audit
// `AUDIT_CONTRAT_CATALOGUE_FORET_ORDER_K_CF9374` a montré que cet indice dépend
// du générateur. Comparer deux forêts par leurs indices comparerait donc l'ordre
// d'énumération, pas la structure.
//
// La signature ci-dessous est récursive et canonique : le type du nœud, les
// MEMBRES triés de la sphère qui le porte — invariants par renumérotation du
// catalogue —, puis le multiensemble trié des signatures de ses enfants. Deux
// forêts sont identiques si et seulement si leurs multiensembles de signatures
// de racines coïncident, et cela ne suppose aucun ordre commun.
// LE RANG EXACT DU NIVEAU. `ForestNode::source` d'une MULTIFUSION est, par
// contrat de `build_forest`, « la plus petite PAR INDEX des sphères de rang k+1
// du lot » : cet indice dépend du générateur, et l'audit
// `AUDIT_CONTRAT_CATALOGUE_FORET_ORDER_K_CF9374` l'avait déjà dit. Deux forêts
// structurellement identiques peuvent donc nommer un représentant différent pour
// le même événement de fusion — c'est exactement ce que la première version de
// cette signature a détecté, et ce n'était pas une divergence.
//
// Ce qui EST canonique dans une multifusion, c'est son NIVEAU. On le rend
// comparable sans réduire une fraction sur 180 bits : on trie toutes les sphères
// du catalogue par `sphere_cmp_beta` et on donne à chaque niveau distinct son
// rang. Les deux catalogues étant déjà prouvés égaux par le différentiel, les
// rangs se correspondent terme à terme.
std::vector<int> beta_ranks(const mhgp::Catalogue& cat) {
  std::vector<int> order((std::size_t)cat.spheres.size());
  for (std::size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    return mhgp::sphere_cmp_beta(cat.spheres[(std::size_t)a].sph,
                                 cat.spheres[(std::size_t)b].sph) < 0;
  });
  std::vector<int> rank((std::size_t)cat.spheres.size(), 0);
  int current = 0;
  for (std::size_t i = 0; i < order.size(); ++i) {
    if (i > 0 && mhgp::sphere_cmp_beta(cat.spheres[(std::size_t)order[i]].sph,
                                       cat.spheres[(std::size_t)order[i - 1]].sph) != 0)
      ++current;
    rank[(std::size_t)order[i]] = current;
  }
  return rank;
}

// LA RÉCURSION EST BORNÉE, PARCE QU'UN CYCLE EST POSSIBLE. `build_forest` ne
// promet pas l'acyclicité au juge, et une signature récursive naïve boucle
// jusqu'à épuiser la pile. On marque le chemin courant : un nœud déjà sur ce
// chemin rend un jeton `CYCLE`, qui se propage et rougit l'empreinte.
std::string forest_signature(const mhgp::Forest& forest, const mhgp::Catalogue& cat,
                             const std::vector<int>& rank, i32 node,
                             std::vector<char>* on_path) {
  if (node < 0 || node >= (i32)forest.nodes.size()) return "(HORS_PLAGE)";
  if ((*on_path)[(std::size_t)node]) return "(CYCLE)";
  (*on_path)[(std::size_t)node] = 1;
  const mhgp::ForestNode& n = forest.nodes[(std::size_t)node];
  std::string out = "(";
  out += std::to_string(n.kind);
  out += ":";
  if (n.source >= 0 && n.source < (i32)cat.spheres.size()) {
    out += "b";
    out += std::to_string(rank[(std::size_t)n.source]);
    // Une NAISSANCE porte un minimum de rang k, dont l'ensemble de membres est
    // canonique : on le grave. Une MULTIFUSION n'a que son niveau.
    if (n.kind == 0) {
      const mhgp::CriticalSphere& s = cat.spheres[(std::size_t)n.source];
      std::vector<i32> members(cat.members.begin() + s.members_begin,
                               cat.members.begin() + s.members_begin + s.rank);
      std::sort(members.begin(), members.end());
      out += "=";
      for (i32 z : members) { out += std::to_string(z); out += ","; }
    }
  } else {
    out += "sans_source";
  }
  std::vector<std::string> children;
  // La garde `forest_structure` en amont garantit des listes de frères finies
  // et en plage : le test `(*on_path)[c]` d'ici était MORT — la récursion remet
  // le drapeau à zéro en sortant, un cycle de frères rebouclait sans fin.
  for (i32 c = n.first_child; c >= 0 && c < (i32)forest.nodes.size();
       c = forest.nodes[(std::size_t)c].next_sibling)
    children.push_back(forest_signature(forest, cat, rank, c, on_path));
  std::sort(children.begin(), children.end());
  out += "[";
  for (const std::string& child : children) out += child;
  out += "])";
  (*on_path)[(std::size_t)node] = 0;
  return out;
}

// LES NŒUDS INACCESSIBLES ET LES LIENS `parent` NE SONT PAS DANS LA SIGNATURE
// RÉCURSIVE, et l'audit l'a dit : une signature qui part des racines ne voit
// jamais un nœud que personne n'atteint, ni un `parent` incohérent avec la liste
// des enfants. On les vérifie séparément, et le résultat entre dans l'empreinte.
//
// LE VALIDATEUR EST TOTAL, ET C'EST UN CONTRAT D'ENTREE, PAS UNE PROMESSE.
// L'audit a montré que la version précédente BOUCLAIT sur un cycle de frères et
// SORTAIT DU TABLEAU sur un enfant hors plage : elle lisait `nodes[c]` avant
// tout contrôle de plage, et sa marche de frères n'était pas bornée. Chaque
// visite est maintenant contrôlée avant déréférencement, la marche de frères
// est bornée par le nombre de nœuds, et chaque faute est comptée sous son nom.
struct ForestStructure {
  long long unreachable = 0, multiply_reached = 0, parent_faults = 0,
            child_count_faults = 0, out_of_range = 0, sibling_overflows = 0;
  // Le réaudit a relevé que « total » était exagéré tant que `source` et la
  // tranche de pool n'étaient pas reçus, ni la profondeur : ils le sont.
  long long source_faults = 0, member_slice_faults = 0;
  long long max_depth = 0;   // reçue, pas une faute : borne la récursion aval
  bool sound() const {
    return unreachable == 0 && multiply_reached == 0 && parent_faults == 0 &&
           child_count_faults == 0 && out_of_range == 0 && sibling_overflows == 0 &&
           source_faults == 0 && member_slice_faults == 0;
  }
  std::string text() const {
    return " inaccessibles=" + std::to_string(unreachable) +
           " atteints_plusieurs_fois=" + std::to_string(multiply_reached) +
           " parents_faux=" + std::to_string(parent_faults) +
           " comptes_enfants_faux=" + std::to_string(child_count_faults) +
           " hors_plage=" + std::to_string(out_of_range) +
           " freres_cycliques=" + std::to_string(sibling_overflows) +
           " sources_fausses=" + std::to_string(source_faults) +
           " tranches_pool_fausses=" + std::to_string(member_slice_faults) +
           " profondeur_max=" + std::to_string(max_depth);
  }
};

ForestStructure forest_structure(const mhgp::Forest& forest, const mhgp::Catalogue& cat) {
  ForestStructure s;
  const i32 size = (i32)forest.nodes.size();
  std::vector<int> visits((std::size_t)size, 0);
  std::vector<std::pair<i32, long long>> stack;
  for (i32 r : forest.roots) stack.push_back({r, 1});
  while (!stack.empty()) {
    const auto [node, depth] = stack.back();
    stack.pop_back();
    if (node < 0 || node >= size) { ++s.out_of_range; continue; }
    ++visits[(std::size_t)node];
    if (visits[(std::size_t)node] > 1) continue;   // un cycle s'arrête ici, et se compte
    s.max_depth = std::max(s.max_depth, depth);
    const mhgp::ForestNode& n = forest.nodes[(std::size_t)node];
    // `source` n'est jamais -1 dans une forêt publiée, et il doit désigner une
    // sphère dont la tranche de pool tient dans le catalogue : c'est ce que la
    // signature récursive LIT, elle ne peut pas le supposer.
    if (n.source < 0 || n.source >= (i32)cat.spheres.size()) {
      ++s.source_faults;
    } else {
      const mhgp::CriticalSphere& sphere = cat.spheres[(std::size_t)n.source];
      if (sphere.members_begin < 0 || sphere.rank < 0 ||
          (std::size_t)sphere.members_begin + (std::size_t)sphere.rank > cat.members.size())
        ++s.member_slice_faults;
    }
    i32 counted = 0;
    long long walked = 0;
    for (i32 c = n.first_child; c != -1;) {
      if (c < 0 || c >= size) { ++s.out_of_range; break; }
      // Une liste de frères plus longue que la forêt entière PROUVE un cycle :
      // la marche s'arrête là, au lieu de pousser la pile sans fin.
      if (++walked > (long long)size) { ++s.sibling_overflows; break; }
      if (forest.nodes[(std::size_t)c].parent != node) ++s.parent_faults;
      ++counted;
      stack.push_back({c, depth + 1});
      c = forest.nodes[(std::size_t)c].next_sibling;
    }
    if (counted != n.n_children) ++s.child_count_faults;
  }
  for (int v : visits) {
    if (v == 0) ++s.unreachable;
    if (v > 1) ++s.multiply_reached;
  }
  for (i32 r : forest.roots)
    if (r >= 0 && r < size && forest.nodes[(std::size_t)r].parent != -1) ++s.parent_faults;
  return s;
}

std::string forest_digest(const mhgp::Forest& forest, const mhgp::Catalogue& cat) {
  // LA SIGNATURE RECURSIVE N'EST LANCEE QUE SUR UNE STRUCTURE SAINE. Elle
  // partage les hypothèses que le validateur vérifie — listes de frères finies,
  // enfants dans la plage — et les re-vérifier récursivement coûterait un
  // parcours exponentiel sur une entrée adverse. Une forêt malformée rend une
  // empreinte qui NOMME ses fautes, jamais une boucle ni une lecture hors
  // tableau.
  const ForestStructure structure = forest_structure(forest, cat);
  if (!structure.sound())
    return "STRUCTURE_INVALIDE ordre=" + std::to_string(forest.order) + structure.text();
  const std::vector<int> rank = beta_ranks(cat);
  std::vector<char> on_path((std::size_t)forest.nodes.size(), 0);
  std::vector<std::string> roots;
  for (i32 r : forest.roots) roots.push_back(forest_signature(forest, cat, rank, r, &on_path));
  std::sort(roots.begin(), roots.end());
  std::string out = "ordre=" + std::to_string(forest.order) +
                    " naissances=" + std::to_string(forest.births) +
                    " fusions=" + std::to_string(forest.merge_events) +
                    " tues=" + std::to_string(forest.killed) +
                    " bras=" + std::to_string(forest.unresolved_arms) +
                    " censures=" + std::to_string(forest.censored_events) +
                    " autoritaire=" + std::to_string((int)forest.authoritative) +
                    " noeuds=" + std::to_string(forest.nodes.size()) +
                    " racines=" + std::to_string(forest.roots.size()) +
                    structure.text() + " ";
  for (const std::string& r : roots) out += r;
  return out;
}

enum class Mode { kJudge, kMeasure, kCover };

// LES DEUX FAUTES DE L'AUDIT, EN FIXTURES PERMANENTES. Le cycle de frères
// faisait BOUCLER l'ancien contrôle, l'enfant hors plage le faisait LIRE HORS
// DU TABLEAU (overflow ASan). L'auto-test exige que le validateur termine, que
// chaque faute soit comptée sous son nom, que l'empreinte d'une forêt malformée
// se déclare STRUCTURE_INVALIDE sans lancer la récursion, et qu'une forêt saine
// reste saine.
int forest_validator_selftest() {
  // Un catalogue minimal d'UNE sphere valide : le validateur recoit desormais
  // `source` et la tranche de pool, le temoin sain doit donc porter une source
  // legitime — et les temoins fautifs une source valide AUSSI, pour que chaque
  // fixture n'exerce QUE sa faute.
  mhgp::Catalogue tiny_catalogue;
  {
    mhgp::CriticalSphere sphere{};
    sphere.support[0] = 0;
    for (int i = 1; i < mhgp::kMaxSupport; ++i) sphere.support[i] = -1;
    sphere.n_support = 1;
    sphere.rank = 1;
    sphere.members_begin = 0;
    tiny_catalogue.members = {0};
    tiny_catalogue.spheres.push_back(sphere);
  }

  mhgp::Forest sibling_cycle;
  sibling_cycle.order = 1;
  sibling_cycle.nodes.resize(2);
  sibling_cycle.nodes[0].first_child = 1;
  sibling_cycle.nodes[0].n_children = 1;
  sibling_cycle.nodes[0].source = 0;
  sibling_cycle.nodes[1].parent = 0;
  sibling_cycle.nodes[1].next_sibling = 1;   // frère de lui-même : cycle
  sibling_cycle.nodes[1].source = 0;
  sibling_cycle.roots = {0};
  const ForestStructure cycle_structure = forest_structure(sibling_cycle, tiny_catalogue);
  if (cycle_structure.sibling_overflows == 0) {
    printf("ECHEC : le cycle de freres n'est pas compte (%s)\n",
           cycle_structure.text().c_str());
    return 2;
  }
  if (forest_digest(sibling_cycle, tiny_catalogue).rfind("STRUCTURE_INVALIDE", 0) != 0) {
    printf("ECHEC : l'empreinte du cycle de freres ne se declare pas invalide\n");
    return 2;
  }

  mhgp::Forest out_of_range;
  out_of_range.order = 1;
  out_of_range.nodes.resize(1);
  out_of_range.nodes[0].first_child = 7;     // enfant hors du tableau
  out_of_range.nodes[0].n_children = 1;
  out_of_range.nodes[0].source = 0;
  out_of_range.roots = {0};
  const ForestStructure range_structure = forest_structure(out_of_range, tiny_catalogue);
  if (range_structure.out_of_range == 0) {
    printf("ECHEC : l'enfant hors plage n'est pas compte (%s)\n",
           range_structure.text().c_str());
    return 2;
  }
  if (forest_digest(out_of_range, tiny_catalogue).rfind("STRUCTURE_INVALIDE", 0) != 0) {
    printf("ECHEC : l'empreinte de l'enfant hors plage ne se declare pas invalide\n");
    return 2;
  }

  // LES DEUX FAUTES QUE LE REAUDIT A NOMMEES : une source hors du catalogue
  // (`source` n'est jamais -1 dans une foret publiee), et une tranche de pool
  // qui sort du tableau des membres.
  mhgp::Forest bad_source;
  bad_source.order = 1;
  bad_source.nodes.resize(1);
  bad_source.roots = {0};   // source = -1 par defaut : faute
  const ForestStructure source_structure = forest_structure(bad_source, tiny_catalogue);
  if (source_structure.source_faults == 0) {
    printf("ECHEC : la source hors catalogue n'est pas comptee (%s)\n",
           source_structure.text().c_str());
    return 2;
  }
  mhgp::Catalogue bad_slice_catalogue = tiny_catalogue;
  bad_slice_catalogue.spheres[0].rank = 9;   // la tranche sort du pool
  mhgp::Forest slice_probe;
  slice_probe.order = 1;
  slice_probe.nodes.resize(1);
  slice_probe.nodes[0].source = 0;
  slice_probe.roots = {0};
  const ForestStructure slice_structure = forest_structure(slice_probe, bad_slice_catalogue);
  if (slice_structure.member_slice_faults == 0) {
    printf("ECHEC : la tranche de pool hors tableau n'est pas comptee (%s)\n",
           slice_structure.text().c_str());
    return 2;
  }

  // LE TEMOIN SAIN : deux noeuds, une racine, liens et sources coherents, et sa
  // profondeur est RECUE. Sans lui, un validateur qui compterait des fautes
  // partout resterait vert ici.
  mhgp::Forest sound;
  sound.order = 1;
  sound.nodes.resize(2);
  sound.nodes[0].first_child = 1;
  sound.nodes[0].n_children = 1;
  sound.nodes[0].source = 0;
  sound.nodes[1].parent = 0;
  sound.nodes[1].source = 0;
  sound.roots = {0};
  const ForestStructure sound_structure = forest_structure(sound, tiny_catalogue);
  if (!sound_structure.sound() || sound_structure.max_depth != 2) {
    printf("ECHEC : la foret saine est comptee fautive ou sa profondeur fausse (%s)\n",
           sound_structure.text().c_str());
    return 2;
  }
  printf("OK : validateur de foret — cycle de freres, enfant hors plage, source hors"
         " catalogue et tranche de pool comptes ; empreintes declarees invalides ;"
         " temoin sain accepte, profondeur recue\n");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && !strcmp(argv[1], "--selftest-forest-validator"))
    return forest_validator_selftest();
  int n = 60, coord = 400, smax = 8, clouds = 4, leaf = 0, judge = 1, cover_only = 0;
  int both_directions = 0, forest_orders = 0, drop_member = 0, build_order = 0;
  int shell_order = 0;
  long long seed = 20260810, cell_cap = 4000000;
  long long min_clouds = 0, min_emitted = 0, min_windowed = 0, min_candidates = 0;
  long long min_forest_nodes = 0, min_cover_tests = 0, min_lane_cover_tests = 0;
  auto integer = [](const char* text, long long* value) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(first, last, magnitude);
    if (r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    long long* wide = nullptr;
    if (!strcmp(argv[i], "--points")) target = &n;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--clouds")) target = &clouds;
    else if (!strcmp(argv[i], "--leaf")) target = &leaf;
    else if (!strcmp(argv[i], "--judge")) target = &judge;
    else if (!strcmp(argv[i], "--cover-only")) target = &cover_only;
    else if (!strcmp(argv[i], "--force-both-directions")) target = &both_directions;
    else if (!strcmp(argv[i], "--forest")) target = &forest_orders;
    else if (!strcmp(argv[i], "--force-drop-member")) target = &drop_member;
    else if (!strcmp(argv[i], "--build-order")) target = &build_order;
    else if (!strcmp(argv[i], "--force-shell-order")) target = &shell_order;
    else if (!strcmp(argv[i], "--cell-cap")) wide = &cell_cap;
    else if (!strcmp(argv[i], "--min-clouds")) wide = &min_clouds;
    else if (!strcmp(argv[i], "--min-emitted")) wide = &min_emitted;
    else if (!strcmp(argv[i], "--min-windowed")) wide = &min_windowed;
    else if (!strcmp(argv[i], "--min-candidates")) wide = &min_candidates;
    else if (!strcmp(argv[i], "--min-forest-nodes")) wide = &min_forest_nodes;
    else if (!strcmp(argv[i], "--min-cover-tests")) wide = &min_cover_tests;
    else if (!strcmp(argv[i], "--min-lane-cover-tests")) wide = &min_lane_cover_tests;
    else if (!strcmp(argv[i], "--seed")) {
      if (!has) { printf("ECHEC : --seed invalide\n"); return 2; }
      ++i; seed = value; continue;
    } else { printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { printf("ECHEC : valeur invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  if (n < 5 || n > 20000 || coord < 4 || coord > 65536 || smax < 2 || smax > mhgp::kMaxRank ||
      clouds < 1 || clouds > 2000 || leaf < 0 || leaf > 65536 || judge < 0 || judge > 1 ||
      cover_only < 0 || cover_only > 1 || both_directions < 0 || both_directions > 1 ||
      cell_cap < 1 || cell_cap > 100000000LL || forest_orders < 0 ||
      forest_orders > mhgp::kMaxRank || drop_member < 0 || drop_member > 1 ||
      build_order < 0 || build_order > 1 || shell_order < 0 || shell_order > 1) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // LES TROIS MODES SONT EXCLUSIFS, ET LA COMBINAISON INCOHERENTE EST REFUSEE.
  // `--cover-only 1 --judge 1` demandait un jugement en sautant l'enumeration :
  // le binaire imprimait des spheres manquantes, ignorait les planchers et
  // sortait zero.
  if (cover_only == 1 && judge == 1) {
    printf("ECHEC : --cover-only 1 ne peut pas juger ; passer --judge 0\n");
    return 2;
  }
  // LES PLANCHERS D'ENUMERATION N'ONT AUCUNE SEMANTIQUE EN MODE COVER. Aucun
  // candidat, aucune sphere, aucune fenetre ni aucune foret n'existent dans ce
  // mode : un tel plancher serait STRUCTURELLEMENT rouge, et son acceptation une
  // branche morte. La combinaison est refusee AVANT, pas diagnostiquee apres.
  if (cover_only == 1 &&
      (min_emitted > 0 || min_windowed > 0 || min_candidates > 0 || min_forest_nodes > 0)) {
    printf("ECHEC : planchers d'enumeration incompatibles avec --cover-only 1 ; seuls"
           " --min-clouds et --min-cover-tests ont une semantique en mode cover\n");
    return 2;
  }
  const Mode mode = cover_only == 1 ? Mode::kCover : (judge == 1 ? Mode::kJudge : Mode::kMeasure);
  // UNE FORET D'ORDRE k LIT LES SPHERES DE RANG k ET k+1. Demander k = s_max
  // rendrait donc des ordres TRONQUES en les qualifiant : c'est refuse.
  if (forest_orders > 0 && forest_orders + 1 > smax) {
    printf("ECHEC : --forest %d exige s_max >= %d ; une foret d'ordre k lit les rangs k et"
           " k+1\n", forest_orders, forest_orders + 1);
    return 2;
  }
  if (mode != Mode::kJudge && forest_orders > 0) {
    printf("ECHEC : la foret ne se juge que sous --judge 1\n");
    return 2;
  }
  // LE MUTANT DE MEMBRE TRONQUE `members` SANS TOUCHER `rank`. Assembler un
  // `mhgp::Catalogue` avec cette incohérence ferait lire `build_forest` hors des
  // bornes du pool : la combinaison est refusée AVANT, pas diagnostiquée après.
  if (drop_member == 1 && forest_orders > 0) {
    printf("ECHEC : --force-drop-member rend le pool incoherent avec le rang ; il ne peut pas"
           " etre combine a --forest\n");
    return 2;
  }
  if (mode != Mode::kJudge && drop_member == 1) {
    printf("ECHEC : le mutant --force-drop-member n'a de sens que sous --judge 1\n");
    return 2;
  }
  if (mode != Mode::kJudge && both_directions == 1) {
    printf("ECHEC : le mutant --force-both-directions n'a de sens que sous --judge 1\n");
    return 2;
  }
  // LE MUTANT D'ORDRE DE COQUILLE assemble le catalogue public dans l'ordre
  // d'iteration de la map — la faute que la sonde d'audit observait. Le quotient
  // semantique reste vert; c'est la porte de payload public qui doit le tuer.
  if (mode != Mode::kJudge && shell_order == 1) {
    printf("ECHEC : le mutant --force-shell-order n'a de sens que sous --judge 1\n");
    return 2;
  }
  // L'ORDRE DE CONSTRUCTION EST UN PARAMETRE DE CAMPAGNE CHRONO. Il n'existe
  // que la ou DEUX constructions coexistent : le mode juge.
  if (mode != Mode::kJudge && build_order != 0) {
    printf("ECHEC : --build-order n'a de sens que sous --judge 1 ; il n'y a qu'une seule"
           " construction dans les autres modes\n");
    return 2;
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);

  SourceCounters totals[5];
  long long decided = 0, refused_status = 0, mismatches = 0;
  long long cover_tests = 0, neighbour_tests = 0, cover_tests_lane[5] = {};
  long long judge_comparisons = 0;
  long long clouds_reference_first = 0, clouds_source_first = 0;
  long long degree_max = 0, degree_sum = 0, degree_samples = 0;
  // CES TROIS COMPTEURS RESTENT EN 64 BITS, ET C'EST BORNE, PAS OUBLIE. Chacun
  // est majoré par le nombre de candidats, lui-même au plus
  // `n * C(n-1, 3) <= 2,7e16` sous le plafond CLI de 20 000 points — très en
  // dessous de `2^63`. C'est `bound_t = somme d * C(d+, q-1)`, qui atteint
  // 5,3e20 au même plafond, qui exigeait les 128 bits.
  long long locator_clamps = 0, duplicate_emissions = 0;
  long long forests_compared = 0, forest_faults = 0, forest_nodes = 0, forest_roots = 0;
  unsigned long long csr_bytes_high_water = 0, cover_bytes_high_water = 0;
  double reference_seconds = 0, source_seconds = 0, judge_seconds = 0, refusal_seconds = 0;
  double source_fold_seconds = 0, reference_fold_seconds = 0;
  unsigned long long source_payload_bytes_high_water = 0, reference_payload_bytes_high_water = 0;
  const auto catalogue_bytes = [](const mhgp::Catalogue& catalogue) {
    return (unsigned long long)catalogue.spheres.size() * sizeof(mhgp::CriticalSphere) +
           (unsigned long long)catalogue.members.size() * sizeof(i32);
  };
  const auto forest_bytes = [](const mhgp::Forest& forest) {
    return (unsigned long long)forest.nodes.size() * sizeof(mhgp::ForestNode) +
           (unsigned long long)forest.roots.size() * sizeof(i32);
  };
  std::vector<long long> leaf_q[5];
  long long outcome_count[5][4] = {};

  for (int c = 0; c < clouds; ++c) {
    std::vector<P3> pts;
    {
      std::set<long long> keys;
      for (int guard = 0; (int)pts.size() < n && guard < 200 * n; ++guard) {
        P3 q{};
        q.x = (i32)pick(rng); q.y = (i32)pick(rng); q.z = (i32)pick(rng);
        const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
        if (!keys.insert(key).second) continue;
        pts.push_back(q);
      }
    }
    if ((int)pts.size() < n) { printf("ECHEC : nuage %d non genere\n", c); return 3; }

    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    mhgp::Catalogue truth;
    mhgp::Catalogue source_catalogue;
    std::vector<mhgp::Forest> source_forests, reference_forests;
    std::map<std::vector<i32>, Produced> produced;
    double cloud_reference_seconds = 0, cloud_source_seconds = 0;
    double cloud_reference_fold = 0, cloud_source_fold = 0;
    // L'ORDRE EXECUTE EST OBSERVE, PAS DEDUIT DE L'OPTION : le reaudit a releve
    // que la ligne imprimee repetait `--build-order`, si bien qu'un mutant
    // forcant la mauvaise branche restait vert. Chaque bloc note son passage.
    int executed_first = 0;   // 0 = aucun, 1 = reference, 2 = source

    // LA REFERENCE PRODUIT SON PAYLOAD ENTIER SOUS SON PROPRE CHRONO : catalogue
    // puis forets. Le juge n'y est plus — l'audit chrono a etabli que soustraire
    // deux modes CLI n'isole rien, seule la separation des horloges DANS LE MEME
    // MODE est recevable.
    auto run_reference = [&]() {
      if (executed_first == 0) executed_first = 1;
      const auto r0 = std::chrono::steady_clock::now();
      truth = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
      if (status == mhgp3v::CloudStatus::kOk)
        for (int k = 1; k <= forest_orders; ++k) {
          const auto f0 = std::chrono::steady_clock::now();
          reference_forests.push_back(mhgp::build_forest(pts, truth, k));
          cloud_reference_fold +=
              std::chrono::duration<double>(std::chrono::steady_clock::now() - f0).count();
        }
      cloud_reference_seconds +=
          std::chrono::duration<double>(std::chrono::steady_clock::now() - r0).count();
    };

    // LA SOURCE AUSSI : enumeration, assemblage du catalogue public, forets.
    // Un echec arithmetique du cover rend false et la campagne echoue ferme.
    auto run_source = [&]() -> bool {
    if (executed_first == 0) executed_first = 2;
    const auto s0 = std::chrono::steady_clock::now();
    long long side = leaf;
    if (side <= 0) {
      const double volume = (double)coord * coord * coord;
      side = (long long)std::max(1.0, std::cbrt(volume * (double)(smax + 1) / (double)n));
    }

    if (mode != Mode::kCover) {
      for (i32 p = 0; p < n; ++p) {
        Produced entry;
        entry.sphere.support[0] = p;
        for (int t = 1; t < mhgp::kMaxSupport; ++t) entry.sphere.support[t] = -1;
        entry.sphere.n_support = 1;
        entry.sphere.rank = 1;
        entry.sphere.sph = mhgp::sphere1(pts[(std::size_t)p]);
        entry.sphere.beta = 0.0;
        entry.members = {p};
        if (!produced.emplace(std::vector<i32>{p}, entry).second) ++duplicate_emissions;
        ++totals[1].emitted;
      }
    }

    for (int q = 2; q <= 4; ++q) {
      // UN SUPPORT D'ARITE q A UN RANG AU MOINS q. Si q > s_max la lane est VIDE
      // par contrat, pas en echec : `t_q = s_max - q + 1` y serait nul ou negatif
      // et le cover echouait sur un message generique.
      if (q > smax) continue;
      Cover cover;
      if (!build_cover(pts, q, smax, side, cell_cap, &cover)) {
        printf("ECHEC : arithmetique du cover d'arite %d hors domaine au nuage %d\n", q, c);
        return false;
      }
      cover_tests += cover.distance_tests;
      cover_tests_lane[q] += cover.distance_tests;
      ++outcome_count[q][(int)cover.outcome];
      for (long long value : cover.per_leaf_effective) leaf_q[q].push_back(value);
      cover_bytes_high_water = std::max<unsigned long long>(
          cover_bytes_high_water,
          (unsigned long long)cover.witnesses.size() * sizeof(i32) +
              (unsigned long long)cover.per_leaf_effective.size() * sizeof(long long));

      Neighbourhoods nb;
      build_neighbourhoods(pts, cover.q_value, &nb);
      neighbour_tests += nb.distance_tests;
      degree_max = std::max(degree_max, nb.degree_max);
      degree_sum += (long long)nb.ids.size();
      degree_samples += n;
      csr_bytes_high_water = std::max(csr_bytes_high_water, nb.bytes);

      SourceCounters& counters = totals[q];
      // LES MASSES COMBINADIQUES SE CALCULENT DEPUIS LES DEGRES SEULS. Elles
      // n'ont pas besoin de l'enumeration qu'elles servent a predire, et le mode
      // cout doit pouvoir les publier sans la lancer.
      for (i32 p = 0; p < n; ++p) {
        long long forward_degree = 0;
        for (int t = nb.begin[(std::size_t)p]; t < nb.begin[(std::size_t)p + 1]; ++t)
          if (nb.ids[(std::size_t)t] > p) ++forward_degree;
        const long long degree = nb.begin[(std::size_t)p + 1] - nb.begin[(std::size_t)p];
        const u128 choose = binomial(forward_degree, q - 1);
        counters.bound_c += choose;
        counters.bound_t += (u128)degree * choose;
      }
      if (mode == Mode::kCover) continue;

      std::vector<i32> forward, support(4, -1), members, shell;
      for (i32 p = 0; p < n; ++p) {
        forward.clear();
        for (int t = nb.begin[(std::size_t)p]; t < nb.begin[(std::size_t)p + 1]; ++t) {
          const i32 z = nb.ids[(std::size_t)t];
          // LE MUTANT : sans la restriction `z > p`, chaque support est enumere
          // `q` fois. La map ecrasait les doublons et le differentiel restait
          // vert; l'unicite est desormais RECUE et le mutant meurt.
          if (both_directions == 1 || z > p) forward.push_back(z);
        }
        if ((int)forward.size() < q - 1) continue;
        std::vector<int> idx((std::size_t)(q - 1));
        for (int t = 0; t < q - 1; ++t) idx[(std::size_t)t] = t;
        for (;;) {
          ++counters.candidates;
          support[0] = p;
          for (int t = 0; t < q - 1; ++t)
            support[(std::size_t)(t + 1)] = forward[(std::size_t)idx[(std::size_t)t]];

          mhgp::Sphere sph{};
          bool proper = false;
          bool distinct = true;
          for (int t = 1; t < q; ++t) if (support[(std::size_t)t] == p) distinct = false;
          if (distinct) {
            if (q == 2) {
              sph = mhgp::sphere2(pts[(std::size_t)support[0]], pts[(std::size_t)support[1]]);
              proper = true;
            } else if (q == 3) {
              proper = mhgp::sphere3(pts[(std::size_t)support[0]], pts[(std::size_t)support[1]],
                                     pts[(std::size_t)support[2]], &sph) &&
                       mhgp::well_centered3(pts[(std::size_t)support[0]],
                                            pts[(std::size_t)support[1]],
                                            pts[(std::size_t)support[2]]);
            } else {
              proper = mhgp::sphere4(pts[(std::size_t)support[0]], pts[(std::size_t)support[1]],
                                     pts[(std::size_t)support[2]], pts[(std::size_t)support[3]],
                                     &sph) &&
                       mhgp::well_centered4(sph, pts[(std::size_t)support[0]],
                                            pts[(std::size_t)support[1]],
                                            pts[(std::size_t)support[2]],
                                            pts[(std::size_t)support[3]]);
            }
          }

          bool census_allowed = false;
          if (proper) {
            if (cover.witnesses_per_leaf == 0) {
              // La banque est ôtée : `n < t_q`, la fenêtre ne peut pas conclure.
              census_allowed = true;
            } else {
              long long steps = 0;
              bool clamped = false;
              const long long leaf_index = locate_leaf(cover, sph, &steps, &clamped);
              counters.locator_steps += (u128)steps;
              if (clamped) ++locator_clamps;
              bool all_interior = true;
              for (int t = 0; t < cover.witnesses_per_leaf; ++t) {
                const i32 w =
                    cover.witnesses[(std::size_t)(leaf_index * cover.witnesses_per_leaf + t)];
                if (w < 0) { all_interior = false; break; }
                ++counters.bank_tests;
                if (mhgp::sphere_side(sph, pts[(std::size_t)w]) >= 0) { all_interior = false; break; }
              }
              census_allowed = !all_interior;
              if (all_interior) ++counters.above_window;
            }
          }

          if (!proper) ++counters.not_proper;
          else if (census_allowed) {
            ++counters.complete;
            members.clear();
            shell.clear();
            members.push_back(p);
            shell.push_back(p);
            for (int t = nb.begin[(std::size_t)p]; t < nb.begin[(std::size_t)p + 1]; ++t) {
              const i32 z = nb.ids[(std::size_t)t];
              ++counters.census_tests;
              const int side_of = mhgp::sphere_side(sph, pts[(std::size_t)z]);
              if (side_of > 0) continue;
              members.push_back(z);
              if (side_of == 0) shell.push_back(z);
            }
            std::sort(members.begin(), members.end());
            std::sort(shell.begin(), shell.end());
            if ((int)members.size() > smax) ++counters.rank_above_smax;
            else {
              std::vector<i32> by_coordinate = shell;
              std::sort(by_coordinate.begin(), by_coordinate.end(), [&](i32 x, i32 y) {
                const P3& u = pts[(std::size_t)x];
                const P3& w = pts[(std::size_t)y];
                if (u.x != w.x) return u.x < w.x;
                if (u.y != w.y) return u.y < w.y;
                if (u.z != w.z) return u.z < w.z;
                return x < y;
              });
              const mhgp::MiniballResult canonical =
                  mhgp::miniball_of(pts, by_coordinate.data(), (int)by_coordinate.size());
              bool same = canonical.ok && canonical.n_support == q;
              if (same)
                for (int t = 0; t < q; ++t) {
                  bool found = false;
                  for (int u = 0; u < canonical.n_support; ++u)
                    if (canonical.support[u] == support[(std::size_t)t]) found = true;
                  if (!found) same = false;
                }
              if (!same) ++counters.non_canonical_support;
              else {
                Produced entry;
                for (int t = 0; t < mhgp::kMaxSupport; ++t)
                  entry.sphere.support[t] = t < canonical.n_support ? canonical.support[t] : -1;
                entry.sphere.n_support = canonical.n_support;
                entry.sphere.rank = (int)members.size();
                entry.sphere.sph = canonical.sph;
                entry.sphere.beta = mhgp::sphere_beta(canonical.sph);
                entry.members = members;
                // LE MUTANT DE MEMBRE. Sans lui, rien ne prouve que le
                // comparateur de pools est encore vivant : il pourrait ne
                // comparer que des listes vides.
                if (drop_member == 1 && entry.members.size() > 1) entry.members.pop_back();
                // L'UNICITE EST RECUE, PAS SUPPOSEE. Une seconde insertion sur la
                // meme coquille est un DOUBLON compte, jamais un ecrasement.
                if (!produced.emplace(shell, entry).second) ++duplicate_emissions;
                ++counters.emitted;
              }
            }
          }

          int t = q - 2;
          while (t >= 0 && idx[(std::size_t)t] == (int)forward.size() - (q - 1) + t) --t;
          if (t < 0) break;
          ++idx[(std::size_t)t];
          for (int u = t + 1; u < q - 1; ++u) idx[(std::size_t)u] = idx[(std::size_t)(u - 1)] + 1;
        }
      }
    }
    if (mode == Mode::kJudge) {
      // L'ASSEMBLAGE DU CATALOGUE PUBLIC EST DE LA PRODUCTION, PAS DU JUGEMENT :
      // il reste dans le chrono source, comme les forets qui le lisent.
      //
      // L'ORDRE PUBLIC EST CELUI DE LA REFERENCE : lexicographique sur les
      // QUATRE cases du support, queue -1, pool reconstruit dans cet ordre. La
      // map est clef par COQUILLE — son ordre d'iteration n'est pas le contrat,
      // et l'assembler tel quel donnait les 3 762 positions et 4 435 indices
      // `ForestNode::source` divergents de la sonde d'audit.
      std::vector<const Produced*> ordered;
      ordered.reserve(produced.size());
      for (const auto& entry : produced) ordered.push_back(&entry.second);
      if (shell_order == 0)
        std::sort(ordered.begin(), ordered.end(),
                  [](const Produced* x, const Produced* y) {
                    for (int t = 0; t < mhgp::kMaxSupport; ++t)
                      if (x->sphere.support[t] != y->sphere.support[t])
                        return x->sphere.support[t] < y->sphere.support[t];
                    return false;
                  });
      for (const Produced* entry : ordered) {
        mhgp::CriticalSphere sphere = entry->sphere;
        sphere.members_begin = (i32)source_catalogue.members.size();
        source_catalogue.members.insert(source_catalogue.members.end(),
                                        entry->members.begin(),
                                        entry->members.end());
        source_catalogue.spheres.push_back(sphere);
      }
      for (int k = 1; k <= forest_orders; ++k) {
        const auto f0 = std::chrono::steady_clock::now();
        source_forests.push_back(mhgp::build_forest(pts, source_catalogue, k));
        cloud_source_fold +=
            std::chrono::duration<double>(std::chrono::steady_clock::now() - f0).count();
      }
    }
    cloud_source_seconds +=
        std::chrono::duration<double>(std::chrono::steady_clock::now() - s0).count();
    return true;
    };

    if (mode != Mode::kJudge) {
      if (!run_source()) return 3;
      source_seconds += cloud_source_seconds;
      ++decided;
      continue;
    }
    if (build_order == 1) {
      if (!run_source()) return 3;
      run_reference();
      if (status != mhgp3v::CloudStatus::kOk) {
        // La source a deja tourne : purger ses compteurs serait silencieux, les
        // garder romprait « seuls les nuages decides ». Refus ferme, nomme.
        printf("ECHEC : nuage %d refuse (statut %s) sous --build-order 1 — les compteurs"
               " agreges ne couvriraient plus les seuls nuages decides ; relancer en ordre"
               " reference d'abord ou changer la graine\n", c,
               mhgp3v::cloud_status_name(status));
        return 3;
      }
    } else {
      run_reference();
      if (status != mhgp3v::CloudStatus::kOk) {
        // UN NUAGE REFUSE N'EST FACTURE A AUCUN DES DEUX CHRONOS. L'audit a
        // montre qu'il etait facture a la reference puis saute par la source :
        // la comparaison n'agregeait pas les memes decisions.
        printf("  nuage %d : statut %s, ignore — %.3f s au compte des refus\n", c,
               mhgp3v::cloud_status_name(status), cloud_reference_seconds);
        ++refused_status;
        refusal_seconds += cloud_reference_seconds;
        continue;
      }
      if (!run_source()) return 3;
    }
    // L'ordre observe doit etre celui du contrat, et il est compte : un mutant
    // qui force la mauvaise branche echoue ICI, pas dans un libelle.
    const int expected_first = build_order == 1 ? 2 : 1;
    if (executed_first != expected_first) {
      printf("ECHEC : ordre execute au nuage %d (%s d'abord) contraire a --build-order %d\n",
             c, executed_first == 1 ? "reference" : "source", build_order);
      return 3;
    }
    if (executed_first == 1) ++clouds_reference_first; else ++clouds_source_first;
    reference_seconds += cloud_reference_seconds;
    source_seconds += cloud_source_seconds;
    reference_fold_seconds += cloud_reference_fold;
    source_fold_seconds += cloud_source_fold;

    // LE JUGE A SA PROPRE HORLOGE, ET ELLE COMMENCE ICI. Tout ce qui suit —
    // differentiel des catalogues, empreintes de forets, comparaisons — est du
    // travail de qualification : il n'existe pas dans un chemin produit et n'est
    // facture a AUCUN des deux chronos.
    const auto j0 = std::chrono::steady_clock::now();

    // LE DIFFERENTIEL. La clef est la coquille, comme la deduplication de la
    // reference; mais le POOL DE MEMBRES est compare lui aussi, sans quoi deux
    // payloads de meme rang et d'interieurs differents seraient indiscernables.
    std::map<std::vector<i32>, std::pair<const mhgp::CriticalSphere*, std::vector<i32>>> expected;
    for (const auto& sphere : truth.spheres) {
      std::vector<i32> key, all;
      for (int t = 0; t < sphere.rank; ++t) {
        const i32 z = truth.members[(std::size_t)(sphere.members_begin + t)];
        all.push_back(z);
        if (mhgp::sphere_side(sphere.sph, pts[(std::size_t)z]) == 0) key.push_back(z);
      }
      std::sort(key.begin(), key.end());
      std::sort(all.begin(), all.end());
      // L'UNICITE DE LA COQUILLE DE L'AUTORITE EST ASSERTEE, pas supposée : deux
      // sphères de référence partageant une coquille rendraient la map muette.
      if (!expected.emplace(key, std::make_pair(&sphere, all)).second) {
        printf("[nuage %d] la REFERENCE porte deux spheres de meme coquille\n", c);
        ++mismatches;
      }
    }
    long long missing = 0, extra = 0, differing = 0, member_faults = 0;
    // CHAQUE COMPARAISON DU JUGE EST COMPTEE : le reaudit a montre que
    // `judge_seconds > 0` recevait l'accumulation du timer, pas l'execution des
    // comparaisons — un mutant supprimant le differentiel en gardant l'horloge
    // restait plausible. Le plancher porte sur ce compteur.
    judge_comparisons += (long long)expected.size();
    for (const auto& entry : expected)
      if (produced.find(entry.first) == produced.end()) ++missing;
    for (const auto& entry : produced) {
      ++judge_comparisons;
      const auto it = expected.find(entry.first);
      if (it == expected.end()) { ++extra; continue; }
      const mhgp::CriticalSphere& a = entry.second.sphere;
      const mhgp::CriticalSphere& b = *it->second.first;
      bool bad = a.n_support != b.n_support || a.rank != b.rank ||
                 mhgp::sphere_cmp_beta(a.sph, b.sph) != 0;
      for (int t = 0; !bad && t < a.n_support; ++t)
        if (a.support[t] != b.support[t]) bad = true;
      if (bad) { ++differing; continue; }
      if (entry.second.members != it->second.second) ++member_faults;
    }
    if (missing || extra || differing || member_faults) {
      printf("[nuage %d] SOURCE != CATALOGUE : %lld manquantes, %lld surnumeraires,"
             " %lld differentes, %lld listes de membres fausses (verite %zu, source %zu)\n", c,
             missing, extra, differing, member_faults, expected.size(), produced.size());
      mismatches += missing + extra + differing + member_faults;
    }

    // LA FORÊT DES K ARBRES, BOUT EN BOUT. Le catalogue n'est que la moitié du
    // contrat; ce que le projet doit produire est la forêt des niveaux de
    // densité. Les forets sont déjà construites, chacune sous son chrono; ici
    // on ne fait que comparer des signatures récursives canoniques — jamais des
    // indices.
    if (forest_orders > 0) {
      for (int k = 1; k <= forest_orders; ++k) {
        const mhgp::Forest& a = source_forests[(std::size_t)(k - 1)];
        const mhgp::Forest& b = reference_forests[(std::size_t)(k - 1)];
        ++forests_compared;
        forest_nodes += (long long)a.nodes.size();
        forest_roots += (long long)a.roots.size();
        // LA SANTE STRUCTURELLE EST EXIGEE DE CHAQUE COTE, PAS SEULEMENT LEUR
        // EGALITE : deux forets identiquement malformees comparaient egal et
        // restaient vertes (audit). Une faute d'un seul cote est un desaccord.
        const ForestStructure sa = forest_structure(a, source_catalogue);
        const ForestStructure sb = forest_structure(b, truth);
        if (!sa.sound() || !sb.sound()) {
          printf("[nuage %d] FORET k=%d STRUCTURELLEMENT INVALIDE\n  source    :%s\n"
                 "  reference :%s\n", c, k, sa.text().c_str(), sb.text().c_str());
          ++forest_faults;
          ++mismatches;
        }
        const std::string da = forest_digest(a, source_catalogue);
        const std::string db = forest_digest(b, truth);
        if (da != db) {
          printf("[nuage %d] FORET k=%d DIFFERENTE\n  source    : %s\n  reference : %s\n",
                 c, k, da.c_str(), db.c_str());
          ++forest_faults;
          ++mismatches;
        }
      }
    }

    // LE PAYLOAD PUBLIC, CHAMP A CHAMP. Le quotient semantique laissait 3 762
    // positions catalogue, 14 579 positions pool et 4 435 indices
    // `ForestNode::source` divergents passer au vert — sonde d'audit chrono.
    // L'ordre canonique etant maintenant celui de la reference DES DEUX COTES,
    // l'egalite est exigee sur les octets publics : spheres (representation
    // exacte comprise), pool concatene, offsets, forets et indices `source`.
    // `beta` est compare BIT a bit : les deux cotes projettent le meme
    // rationnel, une difference d'ULP denoncerait une representation divergente.
    long long payload_faults = 0;
    if (source_catalogue.spheres.size() != truth.spheres.size() ||
        source_catalogue.members.size() != truth.members.size()) {
      ++payload_faults;
    } else {
      for (std::size_t i = 0; i < truth.spheres.size(); ++i) {
        ++judge_comparisons;
        const mhgp::CriticalSphere& a = source_catalogue.spheres[i];
        const mhgp::CriticalSphere& b = truth.spheres[i];
        bool bad = a.n_support != b.n_support || a.rank != b.rank ||
                   a.members_begin != b.members_begin ||
                   a.sph.base.x != b.sph.base.x || a.sph.base.y != b.sph.base.y ||
                   a.sph.base.z != b.sph.base.z || a.sph.nx != b.sph.nx ||
                   a.sph.ny != b.sph.ny || a.sph.nz != b.sph.nz ||
                   a.sph.den != b.sph.den || a.sph.support != b.sph.support ||
                   memcmp(&a.beta, &b.beta, sizeof(double)) != 0;
        for (int t = 0; t < mhgp::kMaxSupport; ++t)
          if (a.support[t] != b.support[t]) bad = true;
        if (bad) ++payload_faults;
      }
      if (source_catalogue.members != truth.members) ++payload_faults;
      // LES DIAGNOSTICS DU `Catalogue` FONT PARTIE DU PAYLOAD ENTIER (residu
      // releve par l'audit live). Aucun des deux generateurs v3 ne les remplit :
      // l'egalite exige qu'ils restent TOUS deux vides — un cote qui se mettrait
      // a en publier romprait le contrat, et cette porte le dirait.
      if (source_catalogue.neighbourhood_size != truth.neighbourhood_size ||
          source_catalogue.growth_rounds != truth.growth_rounds ||
          source_catalogue.certified != truth.certified ||
          source_catalogue.candidate_pairs != truth.candidate_pairs ||
          source_catalogue.candidate_triples != truth.candidate_triples ||
          source_catalogue.candidate_quads != truth.candidate_quads ||
          source_catalogue.degenerate_shells != truth.degenerate_shells ||
          source_catalogue.shell_anomalies != truth.shell_anomalies)
        ++payload_faults;
    }
    for (std::size_t f = 0; f < source_forests.size(); ++f) {
      ++judge_comparisons;
      const mhgp::Forest& a = source_forests[f];
      const mhgp::Forest& b = reference_forests[f];
      bool bad = a.order != b.order || a.nodes.size() != b.nodes.size() ||
                 a.roots != b.roots || a.births != b.births ||
                 a.merge_events != b.merge_events || a.killed != b.killed ||
                 a.unresolved_arms != b.unresolved_arms ||
                 a.censored_events != b.censored_events ||
                 a.authoritative != b.authoritative;
      if (!bad)
        for (std::size_t t = 0; t < a.nodes.size(); ++t) {
          const mhgp::ForestNode& x = a.nodes[t];
          const mhgp::ForestNode& y = b.nodes[t];
          if (memcmp(&x.beta, &y.beta, sizeof(double)) != 0 || x.parent != y.parent ||
              x.first_child != y.first_child || x.next_sibling != y.next_sibling ||
              x.n_children != y.n_children || x.kind != y.kind || x.source != y.source) {
            bad = true;
            break;
          }
        }
      if (bad) ++payload_faults;
    }
    if (payload_faults != 0) {
      printf("[nuage %d] PAYLOAD PUBLIC DIFFERENT : %lld structure(s) en desaccord — ordre"
             " canonique, pool, offsets, forets ou indices `source`\n", c, payload_faults);
      mismatches += payload_faults;
    }
    judge_seconds +=
        std::chrono::duration<double>(std::chrono::steady_clock::now() - j0).count();

    // LES OCTETS COMPARABLES DES DEUX COTES : payload public par nuage, en
    // haute-eau. Spheres, pool de membres, noeuds et racines des forets.
    unsigned long long source_payload = catalogue_bytes(source_catalogue);
    unsigned long long reference_payload = catalogue_bytes(truth);
    for (const mhgp::Forest& f : source_forests) source_payload += forest_bytes(f);
    for (const mhgp::Forest& f : reference_forests) reference_payload += forest_bytes(f);
    source_payload_bytes_high_water =
        std::max(source_payload_bytes_high_water, source_payload);
    reference_payload_bytes_high_water =
        std::max(reference_payload_bytes_high_water, reference_payload);
    ++decided;
  }

  if (decided == 0) { printf("ECHEC : aucun nuage decide\n"); return 3; }
  const char* mode_name = mode == Mode::kJudge ? "juge"
                        : (mode == Mode::kMeasure ? "mesure" : "cover");
  printf("provenance : --clouds %d --points %d --coord %d --smax %d --seed %lld --leaf %d"
         " --judge %d --cover-only %d --cell-cap %lld --force-both-directions %d --forest %d"
         " --build-order %d --force-shell-order %d  [mode %s]\n", clouds, n, coord, smax,
         seed, leaf, judge, cover_only, cell_cap, both_directions, forest_orders,
         build_order, shell_order, mode_name);
  printf("nuages     : decides=%lld  refuses pour statut=%lld\n", decided, refused_status);
  for (int q = 2; q <= 4; ++q)
    for (int o = 0; o < 4; ++o)
      if (outcome_count[q][o] != 0)
        printf("cover q=%d  : %s = %lld nuage(s)\n", q,
               cover_outcome_name((CoverOutcome)o), outcome_count[q][o]);
  printf("cout       : tests cover=%lld (lanes q2/q3/q4 = %lld/%lld/%lld)  tests"
         " voisinage=%lld  degre max=%lld  degre moyen=%.1f\n",
         cover_tests, cover_tests_lane[2], cover_tests_lane[3], cover_tests_lane[4],
         neighbour_tests, degree_max,
         degree_samples ? (double)degree_sum / (double)degree_samples : 0.0);
  printf("octets     : CSR high-water=%llu  banque+dispersion high-water=%llu"
         "  (aucun cap recu, aucun SLO)\n", csr_bytes_high_water, cover_bytes_high_water);
  for (int q = 2; q <= 4; ++q) {
    std::vector<long long>& values = leaf_q[q];
    std::sort(values.begin(), values.end());
    const long long lo = values.empty() ? 0 : values.front();
    const long long mid = values.empty() ? 0 : values[values.size() / 2];
    const long long hi = values.empty() ? 0 : values.back();
    // TOUTES LES FEUILLES DE TOUS LES NUAGES, valeurs EFFECTIVES. La version
    // precedente publiait le maximum des medianes par nuage, et melangeait des
    // coins nominaux d'avant repli avec un maximum d'apres repli : la « mediane »
    // pouvait depasser le « maximum ».
    printf("lane q=%d   : Q effectifs sur %zu feuilles — min=%lld mediane=%lld max=%lld"
           "   rayons %.1f / %.1f / %.1f\n", q, values.size(), lo, mid, hi,
           2.0 * std::sqrt((double)lo), 2.0 * std::sqrt((double)mid), 2.0 * std::sqrt((double)hi));
    printf("           : C_q=%s  T_q=%s  H_q<=%s\n", u128_text(totals[q].bound_c).c_str(),
           u128_text(totals[q].bound_t).c_str(),
           u128_text(totals[q].bound_t + (u128)(smax - q + 1) * totals[q].bound_c).c_str());
    if (mode == Mode::kCover) continue;
    printf("           : candidats=%s  non propres=%s  fenetre=%s  completes=%s\n",
           u128_text(totals[q].candidates).c_str(), u128_text(totals[q].not_proper).c_str(),
           u128_text(totals[q].above_window).c_str(), u128_text(totals[q].complete).c_str());
    printf("           : rang>s_max=%s  support non canonique=%s  EMISES=%s"
           "  banque=%s  census=%s  locator=%s\n",
           u128_text(totals[q].rank_above_smax).c_str(),
           u128_text(totals[q].non_canonical_support).c_str(),
           u128_text(totals[q].emitted).c_str(), u128_text(totals[q].bank_tests).c_str(),
           u128_text(totals[q].census_tests).c_str(), u128_text(totals[q].locator_steps).c_str());
  }
  u128 all_emitted = 0, all_candidates = 0, all_window = 0;
  for (int q = 1; q <= 4; ++q) {
    all_emitted += totals[q].emitted;
    all_candidates += totals[q].candidates;
    all_window += totals[q].above_window;
  }
  if (mode != Mode::kCover) {
    printf("total      : emises=%s (dont %s singletons)  candidats=%s  fenetre=%s\n",
           u128_text(all_emitted).c_str(), u128_text(totals[1].emitted).c_str(),
           u128_text(all_candidates).c_str(), u128_text(all_window).c_str());
    printf("unicite    : emissions=%s  doublons=%lld  clamps du locator=%lld\n",
           u128_text(all_emitted).c_str(), duplicate_emissions, locator_clamps);
  }
  if (forest_orders > 0)
    printf("foret      : %lld forets comparees (k=1..%d)  %lld noeuds  %lld racines"
           "  %lld differentes\n", forests_compared, forest_orders, forest_nodes,
           forest_roots, forest_faults);
  // LE PAYLOAD PUBLIC CANONIQUE, LES SEULS NUAGES DECIDES, LE MEME PROCESSUS,
  // LA MEME MACHINE. Les deux cotes serialisent le MEME contrat public — ordre
  // lexicographique sur le support, pool reconstruit, offsets — et le juge
  // l'exige champ a champ; le libelle etait faux tant que seul le quotient
  // semantique etait compare (audit chrono). Les masses de chaque cote ne sont
  // PAS commensurables : un sommet d'arrangement et un candidat de clique ne
  // coutent pas la meme chose.
  if (refused_status > 0)
    printf("refus      : %lld nuage(s) au statut non kOk — %.3f s au compte des refus, HORS"
           " des deux chronos\n", refused_status, refusal_seconds);
  if (mode == Mode::kJudge) {
    printf("octets     : buffers dynamiques du payload par nuage, haute-eau — spheres,"
           " pool et forets SEULEMENT, pas tout l'objet public — reference=%llu"
           "  source=%llu\n",
           reference_payload_bytes_high_water, source_payload_bytes_high_water);
    if (forest_orders > 0)
      printf("temps      : PAYLOAD PUBLIC CANONIQUE (catalogue + %d forets), nuages"
             " DECIDES seulement — reference %.3f s (dont fold %.3f) contre source %.3f s"
             " (dont fold %.3f), rapport %.2f ; juge %.3f s (%lld comparaisons) HORS des"
             " deux chronos\n",
             forest_orders, reference_seconds, reference_fold_seconds, source_seconds,
             source_fold_seconds,
             source_seconds > 0.0 ? reference_seconds / source_seconds : 0.0,
             judge_seconds, judge_comparisons);
    else
      printf("temps      : catalogue seul, nuages DECIDES seulement — reference=%.3f s"
             "  source=%.3f s  rapport %.2f ; juge %.3f s (%lld comparaisons) HORS des"
             " deux chronos\n",
             reference_seconds, source_seconds,
             source_seconds > 0.0 ? reference_seconds / source_seconds : 0.0,
             judge_seconds, judge_comparisons);
    printf("           : ordre OBSERVE par nuage — reference d'abord sur %lld, source"
           " d'abord sur %lld (tout ecart au contrat --build-order est un ECHEC par"
           " nuage) ; le juge a sa propre horloge, fermee AVANT ces lignes; ce rapport"
           " est un diagnostic sur ces nuages, pas un SLO\n",
           clouds_reference_first, clouds_source_first);
  } else if (mode == Mode::kCover)
    // AUCUNE REFERENCE N'EXISTE DANS CE MODE : le libelle « catalogue seul —
    // reference=0.000 » etait un rapport inexploitable, releve par l'audit.
    printf("temps      : MODE COVER — cover, voisinages et masses combinadiques en %.3f s ;"
           " aucune reference n'est construite, aucun rapport n'a de sens\n", source_seconds);
  else
    // « reference=0 » signifiait « non executee », pas « chronometree a zero » :
    // le mode mesure n'imprime plus que ce qu'il execute.
    printf("temps      : MODE MESURE — source=%.3f s ; aucune reference n'est executee dans"
           " ce mode\n", source_seconds);

  struct Floor { const char* name; u128 value; long long required; };
  const Floor floors[] = {
      {"nuages decides", (u128)decided, min_clouds},
      {"spheres emises", all_emitted, min_emitted},
      {"candidats", all_candidates, min_candidates},
      {"refus par la fenetre", all_window, min_windowed},
      {"noeuds de foret", (u128)forest_nodes, min_forest_nodes},
      {"tests de cover", (u128)cover_tests, min_cover_tests},
  };
  for (const Floor& f : floors)
    if (f.value < (u128)f.required) {
      printf("ECHEC : plancher « %s » non atteint — %s/%lld\n", f.name,
             u128_text(f.value).c_str(), f.required);
      return 3;
    }
  // LE PLANCHER AGREGE NE PROUVE PAS L'EXERCICE DES TROIS LANES : omettre une
  // lane entiere laissait 137 200 tests sur 205 800 et le seuil agrege vert
  // (audit live). Ce plancher-ci vaut PAR lane non vide du contrat.
  if (min_lane_cover_tests > 0)
    for (int q = 2; q <= 4; ++q)
      if (q <= smax && cover_tests_lane[q] < min_lane_cover_tests) {
        printf("ECHEC : plancher « tests de cover lane q=%d » non atteint — %lld/%lld\n",
               q, cover_tests_lane[q], min_lane_cover_tests);
        return 3;
      }
  // UN CLAMP DU LOCATOR EST UNE VIOLATION D'INVARIANT. Sur un support bien
  // centre le centre appartient a l'enveloppe convexe du support, donc a la
  // boite : le clamp masquerait une faute au lieu d'echouer ferme.
  if (locator_clamps != 0) {
    printf("ECHEC : %lld clamp(s) du locator — le centre d'un support bien centre est dans la"
           " boite, ce clamp masque une violation d'invariant\n", locator_clamps);
    return 3;
  }
  // L'IDENTITE `candidats == C_q` EST EXIGEE, pas observee. Une mutation qui
  // saute ou duplique seulement des candidats NON EMISSIBLES conserverait la
  // sortie, les planchers et le differentiel : ce compteur est le seul temoin.
  // Elle ne vaut que sous la regle d'ancre intacte — le mutant bidirectionnel
  // enumere volontairement chaque support q fois — et que dans les MODES QUI
  // ENUMERENT : le mode cover saute l'enumeration par contrat, comparer son zero
  // a la masse analytique etait la regression relevee par l'audit.
  if (mode != Mode::kCover && both_directions == 0)
    for (int q = 2; q <= 4; ++q)
      if (q <= smax && totals[q].candidates != totals[q].bound_c) {
        printf("ECHEC : lane q=%d a evalue %s candidats pour C_q=%s — l'enumeration"
               " combinadique et sa masse ne coincident pas\n", q,
               u128_text(totals[q].candidates).c_str(), u128_text(totals[q].bound_c).c_str());
        return 3;
      }
  if (duplicate_emissions != 0) {
    printf("ECHEC : %lld emission(s) en double — une sphere doit etre produite exactement une"
           " fois, depuis l'ancre de son support canonique\n", duplicate_emissions);
    return 3;
  }
  // UN CHRONO NUL SUR UNE CAMPAGNE DECIDEE EST UNE MUTATION, PAS UNE MESURE.
  // L'audit a montre qu'annuler l'accumulation du timer source laissait 14/14
  // portes vertes; ce garde tue ce mutant dans TOUTES les campagnes : la
  // moindre lane fait des dizaines de microsecondes, un steady_clock ne peut
  // pas rendre zero dessus.
  if (source_seconds <= 0.0) {
    printf("ECHEC : chrono source nul apres %lld nuage(s) decide(s) — accumulation morte\n",
           decided);
    return 3;
  }
  if (mode == Mode::kJudge && reference_seconds <= 0.0) {
    printf("ECHEC : chrono reference nul apres %lld nuage(s) decide(s) — accumulation"
           " morte\n", decided);
    return 3;
  }
  // LE JUGE DOIT AVOIR TOURNE, ET C'EST SON COMPTEUR DE COMPARAISONS QUI LE
  // PROUVE — le reaudit a montre qu'une horloge non nulle ne recevait que
  // l'accumulation du timer. Tout nuage decide porte au moins ses n singletons
  // dans la verite : zero comparaison apres un nuage decide est une mutation.
  if (mode == Mode::kJudge && judge_seconds <= 0.0) {
    printf("ECHEC : chrono juge nul apres %lld nuage(s) decide(s) — le differentiel n'a"
           " pas tourne\n", decided);
    return 3;
  }
  if (mode == Mode::kJudge && judge_comparisons < decided * (long long)n) {
    printf("ECHEC : %lld comparaison(s) de juge pour %lld nuage(s) decide(s) de %d points —"
           " le differentiel n'a pas couvert la verite\n", judge_comparisons, decided, n);
    return 3;
  }
  if (refused_status != 0) {
    printf("ECHEC : %lld nuage(s) au statut non kOk\n", refused_status);
    return 3;
  }
  // SEUL LE MODE JUGE A LE DROIT DE CONCLURE. Les deux autres MESURENT.
  if (mode == Mode::kCover) {
    printf("\nMODE COVER : cover, voisinages et masses combinadiques seulement."
           " Aucun candidat, aucune sphere, AUCUNE EXACTITUDE N'EST AFFIRMEE\n");
    return 0;
  }
  if (mode == Mode::kMeasure) {
    printf("\nMODE MESURE : la source a produit %s spheres et n'a ete comparee a AUCUN oracle."
           " AUCUNE EXACTITUDE N'EST AFFIRMEE\n", u128_text(all_emitted).c_str());
    return 0;
  }
  printf("\n%lld desaccords\n", mismatches);
  if (mismatches != 0) return 1;
  if (forest_orders > 0)
    printf("OK : accord relatif complet, catalogue ET foret — memes coquilles, memes supports"
           " canoniques, memes rangs, memes niveaux exacts, memes listes de membres par"
           " coquille, memes"
           " %lld forets de k=1 a %d signatures recursives comprises, et PAYLOAD PUBLIC"
           " IDENTIQUE champ a champ : ordre canonique, pool, offsets, indices `source`\n",
           forests_compared, forest_orders);
  else
    printf("OK : accord relatif complet avec le catalogue ferme — memes coquilles, memes"
           " supports canoniques, memes rangs, memes niveaux exacts, memes listes de membres"
           " par coquille, et PAYLOAD PUBLIC IDENTIQUE champ a champ\n");
  return 0;
}
