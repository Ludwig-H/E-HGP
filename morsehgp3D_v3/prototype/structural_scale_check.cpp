// MorseHGP3D v3 — LA VERIFICATION STRUCTURELLE A L'ECHELLE (plan de Louis).
//
// Aux tailles sans verite exhaustive, l'exactitude se controle par des
// invariants de structure. Celui-ci est une EGALITE, pas une borne :
//
//   THEOREME (k=1 == single-linkage, INSENSIBLE A LA TRONCATURE).
//   La partition de couverture du fold a l'ordre 1 egale le single-linkage a
//   chaque niveau. Sens direct : deux points d'une meme boule de niveau L
//   sont a distance <= 2*rayon, donc merges par le single-linkage au plus a
//   L. Sens reciproque : chaque arete EMST est une arete de GABRIEL (boule
//   diametrale vide), donc sa paire a un sature de RANG 2 — jamais censuree
//   par smax — et son generateur fusionne les deux composantes exactement au
//   niveau (d/2)^2. Le multiset des niveaux de fusion k=1 (avec multiplicite
//   arite-1) doit donc EGALER le multiset des niveaux (d_e/2)^2 des aretes
//   EMST, et la somme des arites-1 doit valoir n-1.
//
// RENFORCEMENT (reponse auditeur 20260811) : le multiensemble des seuls
// niveaux de fusion ne distingue pas deux dendrogrammes ayant les memes
// poids. La porte compare donc AUSSI la PARTITION CANONIQUE des PointId,
// avant (stricte) et apres (fermee) chaque niveau exact : deux DSU — la
// couverture k=1 rejouee des lots du catalogue, et le single-linkage rejoue
// des aretes EMST — avancent sur la fusion des niveaux exacts des deux cotes,
// chaque composante etant etiquetee par son plus petit PointId. L'egalite est
// verifiee sur tous les points touches du lot (suffisant par recurrence : une
// divergence de composantes se manifeste toujours en un point touche), puis
// par un balayage final complet. Cout O(masse alpha(n)) : praticable a 50 k.
//
// L'EMST est calcule ICI, exactement, par Prim sur les distances carrees
// entieres (i128) — O(n^2), sans dependance, praticable en verification
// jusqu'a 50 k. La comparaison de niveaux n'invente aucune arithmetique :
// le niveau EMST est fabrique par `mhgp::sphere2(x, y)` et compare au
// representant du record par `mhgp::sphere_cmp_beta`.
//
// Codes : 0 OK ; 1 refutation structurelle ; 2 CLI ; 3 nuage/plancher.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"
#include "prototype/saturated_fold_faceowner.hpp"

namespace {

// DSU a etiquette canonique : chaque racine porte le PLUS PETIT PointId de sa
// composante — jamais un numero de racine (contrat des audits).
struct LabelledDsu {
  std::vector<int> parent;
  std::vector<int> minimum;
  explicit LabelledDsu(int n) : parent((std::size_t)n), minimum((std::size_t)n) {
    for (int v = 0; v < n; ++v) {
      parent[(std::size_t)v] = v;
      minimum[(std::size_t)v] = v;
    }
  }
  int find(int a) {
    while (parent[(std::size_t)a] != a) {
      parent[(std::size_t)a] = parent[(std::size_t)parent[(std::size_t)a]];
      a = parent[(std::size_t)a];
    }
    return a;
  }
  void unite(int a, int b) {
    int ra = find(a), rb = find(b);
    if (ra == rb) return;
    parent[(std::size_t)ra] = rb;
    minimum[(std::size_t)rb] = std::min(minimum[(std::size_t)rb], minimum[(std::size_t)ra]);
  }
  int label(int a) { return minimum[(std::size_t)find(a)]; }
};

}  // namespace

int main(int argc, char** argv) {
  int n = 200, coord = 0, smax = 11;
  long long seed = 20260810;
  int force_shift = 0;   // mutant : decale un niveau de fusion — doit rougir
  int force_early = 0;   // mutant : une union un lot trop tot — la partition rougit
  auto integer = [](const char* text, long long* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
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
    else if (!strcmp(argv[i], "--seed")) wide = &seed;
    else if (!strcmp(argv[i], "--force-shift-fusion")) target = &force_shift;
    else if (!strcmp(argv[i], "--force-merge-early")) target = &force_early;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { std::printf("ECHEC : valeur entiere invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  if (n < 5 || n > 100000 || coord < 0 || coord > 65536 || smax < 2 ||
      smax > mhgp::kMaxRank || force_shift < 0 || force_shift > 1 || force_early < 0 ||
      force_early > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  if (coord == 0) {
    const double c = std::cbrt((double)n * 1000.0);
    coord = (int)std::max(4.0, std::min(65536.0, c));
  }
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);
  std::vector<mhgp::P3> pts;
  {
    std::set<long long> keys;
    for (int guard = 0; (int)pts.size() < n && guard < 200 * n; ++guard) {
      mhgp::P3 q{};
      q.x = (mhgp::i32)pick(rng);
      q.y = (mhgp::i32)pick(rng);
      q.z = (mhgp::i32)pick(rng);
      const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
      if (!keys.insert(key).second) continue;
      pts.push_back(q);
    }
  }
  if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }

  // LE FOLD a l'ordre 1 (forme face-owner, la moins chere a k=1 : etoiles).
  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const auto t0 = std::chrono::steady_clock::now();
  const mhgp::Catalogue catalogue = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
  const auto t1 = std::chrono::steady_clock::now();
  if (status != mhgp3v::CloudStatus::kOk) {
    std::printf("ECHEC : statut nuage %s\n", mhgp3v::cloud_status_name(status));
    return 3;
  }
  mhgp3v::FaceOwnerReceipt receipt;
  const mhgp3v::SaturatedFold fold = mhgp3v::build_saturated_fold_faceowner(
      catalogue, 1, /*keep_partitions=*/false, &receipt, /*enforce_event_guard=*/false);
  const auto t2 = std::chrono::steady_clock::now();
  if (!fold.ok) { std::printf("ECHEC : fold refuse : %s\n", fold.refusal); return 3; }

  // L'EMST EXACT par Prim sur d^2 en i128 — independant de tout le pipeline.
  std::vector<unsigned char> in_tree((std::size_t)n, 0);
  std::vector<mhgp::i128> best((std::size_t)n);
  std::vector<int> best_from((std::size_t)n, 0);
  const auto squared = [&](int a, int b) {
    const mhgp::i128 dx = pts[(std::size_t)a].x - pts[(std::size_t)b].x;
    const mhgp::i128 dy = pts[(std::size_t)a].y - pts[(std::size_t)b].y;
    const mhgp::i128 dz = pts[(std::size_t)a].z - pts[(std::size_t)b].z;
    return dx * dx + dy * dy + dz * dz;
  };
  for (int v = 1; v < n; ++v) best[(std::size_t)v] = squared(0, v);
  in_tree[0] = 1;
  std::vector<std::pair<int, int>> emst_edges;
  for (int step = 1; step < n; ++step) {
    int chosen = -1;
    for (int v = 0; v < n; ++v)
      if (!in_tree[(std::size_t)v] && (chosen < 0 || best[(std::size_t)v] < best[(std::size_t)chosen]))
        chosen = v;
    in_tree[(std::size_t)chosen] = 1;
    emst_edges.push_back({best_from[(std::size_t)chosen], chosen});
    for (int v = 0; v < n; ++v)
      if (!in_tree[(std::size_t)v]) {
        const mhgp::i128 d = squared(chosen, v);
        if (d < best[(std::size_t)v]) {
          best[(std::size_t)v] = d;
          best_from[(std::size_t)v] = chosen;
        }
      }
  }
  const auto t3 = std::chrono::steady_clock::now();

  // LES DEUX MULTISETS DE NIVEAUX, compares par sphere_cmp_beta : cote fold,
  // chaque fusion k=1 pese arite-1 ; cote EMST, chaque arete pese un, au
  // niveau EXACT de sa paire (sphere2 — aucune nouvelle arithmetique).
  std::vector<mhgp::Sphere> fold_levels;
  long long arity_sum = 0;
  for (const mhgp3v::GammaEventRecord& record : fold.orders[0].gamma_records) {
    if (record.type != 2) continue;
    const long long arity = (long long)record.strict_witnesses.size();
    arity_sum += arity - 1;
    for (long long copy = 0; copy + 1 < arity; ++copy)
      fold_levels.push_back(
          catalogue.spheres[(std::size_t)record.level_representative].sph);
  }
  if (force_shift == 1 && !fold_levels.empty())
    fold_levels.back() = mhgp::sphere2(pts[0], pts[(std::size_t)(n - 1)]);
  std::vector<mhgp::Sphere> emst_levels;
  for (const std::pair<int, int>& edge : emst_edges)
    emst_levels.push_back(
        mhgp::sphere2(pts[(std::size_t)edge.first], pts[(std::size_t)edge.second]));
  const auto by_level = [](const mhgp::Sphere& a, const mhgp::Sphere& b) {
    return mhgp::sphere_cmp_beta(a, b) < 0;
  };
  std::sort(fold_levels.begin(), fold_levels.end(), by_level);
  std::sort(emst_levels.begin(), emst_levels.end(), by_level);

  std::printf("provenance : --points %d --coord %d --smax %d --seed %lld\n", n, coord, smax,
              seed);
  std::printf("temps      : catalogue %.3f s, fold k=1 %.3f s, EMST exact %.3f s\n",
              std::chrono::duration<double>(t1 - t0).count(),
              std::chrono::duration<double>(t2 - t1).count(),
              std::chrono::duration<double>(t3 - t2).count());
  std::printf("structure  : fusions k=1 (arites-1)=%lld — aretes EMST=%zu\n", arity_sum,
              emst_edges.size());
  if (arity_sum != (long long)emst_edges.size() ||
      fold_levels.size() != emst_levels.size()) {
    std::printf("REFUTATION : la somme des arites-1 ne vaut pas n-1\n");
    return 1;
  }
  for (std::size_t i = 0; i < fold_levels.size(); ++i)
    if (mhgp::sphere_cmp_beta(fold_levels[i], emst_levels[i]) != 0) {
      std::printf("REFUTATION : niveau de fusion %zu divergent du niveau EMST\n", i);
      return 1;
    }

  // LA PARTITION CANONIQUE PAR NIVEAU EXACT (renforcement 20260811) : la
  // couverture k=1 rejouee des lots du catalogue CONTRE le single-linkage
  // rejoue des aretes EMST, sur la fusion des niveaux exacts des deux cotes.
  // L'egalite des etiquettes min-PointId est exigee sur tous les points
  // touches du lot, en strict PUIS en ferme, et par un balayage final.
  long long level_batches = 0;
  {
    std::vector<int> a_order(catalogue.spheres.size());
    for (std::size_t s = 0; s < a_order.size(); ++s) a_order[s] = (int)s;
    std::sort(a_order.begin(), a_order.end(), [&](int x, int y) {
      const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                          catalogue.spheres[(std::size_t)y].sph);
      if (c != 0) return c < 0;
      return x < y;
    });
    std::vector<std::pair<mhgp::Sphere, std::pair<int, int>>> b_events;
    b_events.reserve(emst_edges.size());
    for (const std::pair<int, int>& edge : emst_edges)
      b_events.push_back({mhgp::sphere2(pts[(std::size_t)edge.first],
                                        pts[(std::size_t)edge.second]),
                          edge});
    std::sort(b_events.begin(), b_events.end(),
              [](const std::pair<mhgp::Sphere, std::pair<int, int>>& a,
                 const std::pair<mhgp::Sphere, std::pair<int, int>>& b) {
                const int c = mhgp::sphere_cmp_beta(a.first, b.first);
                if (c != 0) return c < 0;
                return a.second < b.second;
              });
    LabelledDsu subject(n), truth(n);
    if (force_early == 1 && !b_events.empty()) {
      // MUTANT : la fusion la plus tardive de l'EMST est unie dans le sujet
      // DES AVANT le premier lot — la partition doit rougir a un lot touche.
      subject.unite(b_events.back().second.first, b_events.back().second.second);
    }
    std::size_t ia = 0, ib = 0;
    std::vector<int> touched;
    while (ia < a_order.size() || ib < b_events.size()) {
      const bool take_a =
          ib >= b_events.size() ||
          (ia < a_order.size() &&
           mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)a_order[ia]].sph,
                                 b_events[ib].first) <= 0);
      const mhgp::Sphere level =
          take_a ? catalogue.spheres[(std::size_t)a_order[ia]].sph : b_events[ib].first;
      std::size_t ja = ia, jb = ib;
      while (ja < a_order.size() &&
             mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)a_order[ja]].sph, level) == 0)
        ++ja;
      while (jb < b_events.size() && mhgp::sphere_cmp_beta(b_events[jb].first, level) == 0)
        ++jb;
      touched.clear();
      for (std::size_t a = ia; a < ja; ++a) {
        const mhgp::CriticalSphere& sphere = catalogue.spheres[(std::size_t)a_order[a]];
        for (mhgp::i32 t = 0; t < sphere.rank; ++t)
          touched.push_back((int)catalogue.members[(std::size_t)(sphere.members_begin + t)]);
      }
      for (std::size_t b = ib; b < jb; ++b) {
        touched.push_back(b_events[b].second.first);
        touched.push_back(b_events[b].second.second);
      }
      for (int x : touched)
        if (subject.label(x) != truth.label(x)) {
          std::printf("REFUTATION : partition STRICTE divergente au lot de niveau %lld"
                      " (point %d : composante %d contre %d)\n", level_batches, x,
                      subject.label(x), truth.label(x));
          return 1;
        }
      for (std::size_t a = ia; a < ja; ++a) {
        const mhgp::CriticalSphere& sphere = catalogue.spheres[(std::size_t)a_order[a]];
        for (mhgp::i32 t = 1; t < sphere.rank; ++t)
          subject.unite((int)catalogue.members[(std::size_t)sphere.members_begin],
                        (int)catalogue.members[(std::size_t)(sphere.members_begin + t)]);
      }
      for (std::size_t b = ib; b < jb; ++b)
        truth.unite(b_events[b].second.first, b_events[b].second.second);
      for (int x : touched)
        if (subject.label(x) != truth.label(x)) {
          std::printf("REFUTATION : partition FERMEE divergente au lot de niveau %lld"
                      " (point %d : composante %d contre %d)\n", level_batches, x,
                      subject.label(x), truth.label(x));
          return 1;
        }
      ++level_batches;
      ia = ja;
      ib = jb;
    }
    for (int v = 0; v < n; ++v)
      if (subject.label(v) != truth.label(v)) {
        std::printf("REFUTATION : partition finale divergente (point %d)\n", v);
        return 1;
      }
  }
  std::printf("OK : la foret k=1 EGALE le single-linkage — multiset des niveaux de fusion"
              " == multiset des (d/2)^2 EMST, et partition canonique des PointId egale en"
              " strict ET en ferme sur %lld lots de niveau exact, meme sous famille"
              " tronquee (aretes EMST = aretes de Gabriel, rang 2, jamais censurees)\n",
              level_batches);
  return 0;
}
