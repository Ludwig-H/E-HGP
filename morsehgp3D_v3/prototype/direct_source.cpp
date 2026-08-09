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

std::string forest_signature(const mhgp::Forest& forest, const mhgp::Catalogue& cat,
                             const std::vector<int>& rank, i32 node) {
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
  for (i32 c = n.first_child; c >= 0; c = forest.nodes[(std::size_t)c].next_sibling)
    children.push_back(forest_signature(forest, cat, rank, c));
  std::sort(children.begin(), children.end());
  out += "[";
  for (const std::string& child : children) out += child;
  out += "])";
  return out;
}

std::string forest_digest(const mhgp::Forest& forest, const mhgp::Catalogue& cat) {
  const std::vector<int> rank = beta_ranks(cat);
  std::vector<std::string> roots;
  for (i32 r : forest.roots) roots.push_back(forest_signature(forest, cat, rank, r));
  std::sort(roots.begin(), roots.end());
  std::string out = "ordre=" + std::to_string(forest.order) +
                    " naissances=" + std::to_string(forest.births) +
                    " fusions=" + std::to_string(forest.merge_events) +
                    " tues=" + std::to_string(forest.killed) +
                    " bras=" + std::to_string(forest.unresolved_arms) +
                    " censures=" + std::to_string(forest.censored_events) +
                    " autoritaire=" + std::to_string((int)forest.authoritative) +
                    " noeuds=" + std::to_string(forest.nodes.size()) +
                    " racines=" + std::to_string(forest.roots.size()) + " ";
  for (const std::string& r : roots) out += r;
  return out;
}

enum class Mode { kJudge, kMeasure, kCover };

}  // namespace

int main(int argc, char** argv) {
  int n = 60, coord = 400, smax = 8, clouds = 4, leaf = 0, judge = 1, cover_only = 0;
  int both_directions = 0, forest_orders = 0;
  long long seed = 20260810, cell_cap = 4000000;
  long long min_clouds = 0, min_emitted = 0, min_windowed = 0, min_candidates = 0;
  long long min_forest_nodes = 0;
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
    else if (!strcmp(argv[i], "--cell-cap")) wide = &cell_cap;
    else if (!strcmp(argv[i], "--min-clouds")) wide = &min_clouds;
    else if (!strcmp(argv[i], "--min-emitted")) wide = &min_emitted;
    else if (!strcmp(argv[i], "--min-windowed")) wide = &min_windowed;
    else if (!strcmp(argv[i], "--min-candidates")) wide = &min_candidates;
    else if (!strcmp(argv[i], "--min-forest-nodes")) wide = &min_forest_nodes;
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
      forest_orders > mhgp::kMaxRank) {
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
  const Mode mode = cover_only == 1 ? Mode::kCover : (judge == 1 ? Mode::kJudge : Mode::kMeasure);
  if (mode != Mode::kJudge && forest_orders > 0) {
    printf("ECHEC : la foret ne se juge que sous --judge 1\n");
    return 2;
  }
  if (mode != Mode::kJudge && both_directions == 1) {
    printf("ECHEC : le mutant --force-both-directions n'a de sens que sous --judge 1\n");
    return 2;
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);

  SourceCounters totals[5];
  long long decided = 0, refused_status = 0, mismatches = 0;
  long long cover_tests = 0, neighbour_tests = 0;
  long long degree_max = 0, degree_sum = 0, degree_samples = 0;
  long long locator_clamps = 0, duplicate_emissions = 0;
  long long forests_compared = 0, forest_faults = 0, forest_nodes = 0, forest_roots = 0;
  unsigned long long csr_bytes_high_water = 0, cover_bytes_high_water = 0;
  double reference_seconds = 0, source_seconds = 0;
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
    if (mode == Mode::kJudge) {
      const auto r0 = std::chrono::steady_clock::now();
      truth = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
      reference_seconds +=
          std::chrono::duration<double>(std::chrono::steady_clock::now() - r0).count();
      if (status != mhgp3v::CloudStatus::kOk) {
        printf("  nuage %d : statut %s, ignore\n", c, mhgp3v::cloud_status_name(status));
        ++refused_status;
        continue;
      }
    }

    const auto s0 = std::chrono::steady_clock::now();
    long long side = leaf;
    if (side <= 0) {
      const double volume = (double)coord * coord * coord;
      side = (long long)std::max(1.0, std::cbrt(volume * (double)(smax + 1) / (double)n));
    }

    std::map<std::vector<i32>, Produced> produced;
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
      Cover cover;
      if (!build_cover(pts, q, smax, side, cell_cap, &cover)) {
        printf("ECHEC : arithmetique du cover d'arite %d hors domaine au nuage %d\n", q, c);
        return 3;
      }
      cover_tests += cover.distance_tests;
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
    source_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - s0).count();

    if (mode != Mode::kJudge) { ++decided; continue; }

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
      expected[key] = {&sphere, all};
    }
    long long missing = 0, extra = 0, differing = 0, member_faults = 0;
    for (const auto& entry : expected)
      if (produced.find(entry.first) == produced.end()) ++missing;
    for (const auto& entry : produced) {
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
             " %lld differentes, %lld pools de membres faux (verite %zu, source %zu)\n", c,
             missing, extra, differing, member_faults, expected.size(), produced.size());
      mismatches += missing + extra + differing + member_faults;
    }

    // LA FORÊT DES K ARBRES, BOUT EN BOUT. Le catalogue n'est que la moitié du
    // contrat; ce que le projet doit produire est la forêt des niveaux de
    // densité. On la construit depuis LA SOURCE et depuis LA RÉFÉRENCE, puis on
    // compare des signatures récursives canoniques — jamais des indices.
    if (forest_orders > 0) {
      mhgp::Catalogue source_catalogue;
      for (const auto& entry : produced) {
        mhgp::CriticalSphere sphere = entry.second.sphere;
        sphere.members_begin = (i32)source_catalogue.members.size();
        source_catalogue.members.insert(source_catalogue.members.end(),
                                        entry.second.members.begin(),
                                        entry.second.members.end());
        source_catalogue.spheres.push_back(sphere);
      }
      for (int k = 1; k <= forest_orders; ++k) {
        const mhgp::Forest a = mhgp::build_forest(pts, source_catalogue, k);
        const mhgp::Forest b = mhgp::build_forest(pts, truth, k);
        ++forests_compared;
        forest_nodes += (long long)a.nodes.size();
        forest_roots += (long long)a.roots.size();
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
    ++decided;
  }

  if (decided == 0) { printf("ECHEC : aucun nuage decide\n"); return 3; }
  const char* mode_name = mode == Mode::kJudge ? "juge"
                        : (mode == Mode::kMeasure ? "mesure" : "cover");
  printf("provenance : --clouds %d --points %d --coord %d --smax %d --seed %lld --leaf %d"
         " --judge %d --cover-only %d --cell-cap %lld --force-both-directions %d --forest %d"
         "  [mode %s]\n", clouds, n, coord, smax, seed, leaf, judge, cover_only, cell_cap,
         both_directions, forest_orders, mode_name);
  printf("nuages     : decides=%lld  refuses pour statut=%lld\n", decided, refused_status);
  for (int q = 2; q <= 4; ++q)
    for (int o = 0; o < 4; ++o)
      if (outcome_count[q][o] != 0)
        printf("cover q=%d  : %s = %lld nuage(s)\n", q,
               cover_outcome_name((CoverOutcome)o), outcome_count[q][o]);
  printf("cout       : tests cover=%lld  tests voisinage=%lld  degre max=%lld  degre moyen=%.1f\n",
         cover_tests, neighbour_tests, degree_max,
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
  printf("temps      : reference=%.3f s  source=%.3f s  (diagnostics, pas un SLO)\n",
         reference_seconds, source_seconds);

  struct Floor { const char* name; u128 value; long long required; };
  const Floor floors[] = {
      {"nuages decides", (u128)decided, min_clouds},
      {"spheres emises", all_emitted, min_emitted},
      {"candidats", all_candidates, min_candidates},
      {"refus par la fenetre", all_window, min_windowed},
      {"noeuds de foret", (u128)forest_nodes, min_forest_nodes},
  };
  for (const Floor& f : floors)
    if (f.value < (u128)f.required) {
      printf("ECHEC : plancher « %s » non atteint — %s/%lld\n", f.name,
             u128_text(f.value).c_str(), f.required);
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
  if (duplicate_emissions != 0) {
    printf("ECHEC : %lld emission(s) en double — une sphere doit etre produite exactement une"
           " fois, depuis l'ancre de son support canonique\n", duplicate_emissions);
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
           " canoniques, memes rangs, memes niveaux exacts, memes pools de membres, et memes"
           " %lld forets de k=1 a %d, signatures recursives comprises\n", forests_compared,
           forest_orders);
  else
    printf("OK : accord relatif complet avec le catalogue ferme — memes coquilles, memes"
           " supports canoniques, memes rangs, memes niveaux exacts, memes pools de membres\n");
  return 0;
}
