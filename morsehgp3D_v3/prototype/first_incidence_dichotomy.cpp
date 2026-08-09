// MorseHGP3D v3 — PREMIÈRES INCIDENCES DU CŒUR : la dichotomie, mesurée et jugée.
//
// `audits/NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md` retire la source
// silencieuse de la liste des inconnues mathématiques. Pour une facette F du
// cœur, de miniboule fermée B_F et de niveau b_F, en posant
// E_F = (B_F inter X) \ F, la première incidence se décide sans recherche de
// voisinage :
//
//   * BRANCHE FERMÉE, E_F non vide : lambda(F) = b_F et
//     M(F) = { F union {x} : x dans E_F }. Aucune régularité requise.
//   * BRANCHE VIDE : lambda(F) est le minimum des niveaux des cofaces DIRECTES
//     contenant F, et M(F) en est le groupe d'ex æquo, parce que tout minimiseur
//     est de Gabriel au sens OUVERT.
//
// ---------------------------------------------------------------------------
// LA SOURCE DIRECTE EST À VACUITÉ INTÉRIEURE, ET MA PREMIÈRE VERSION ÉTAIT FAUSSE
// ---------------------------------------------------------------------------
//
// J'avais retenu les sphères critiques de rang exactement k+1, c'est-à-dire la
// vacuité FERMÉE. Le théorème demande la vacuité INTÉRIEURE, avec une politique
// explicite des égalités extérieures. L'audit le montre sur cinq points :
//
//     (0,0,0) (0,2,2) (2,0,2) (2,2,0) (0,0,2)
//
// les quatre premiers forment un tétraèdre régulier et le cinquième est sur sa
// sphère. Vérifié ici : la vacuité ouverte compte **cinq** cofaces de Gabriel de
// taille quatre, la vacuité fermée une seule. Ma version en gardait une et en
// omettait quatre — et affichait zéro désaccord, parce que les facettes disparues
// n'étaient jamais soumises à la vérité. Le juge était CIRCULAIRE.
//
// La politique retenue est le DÉVELOPPEMENT des extra-shells. Toute coface de
// Gabriel ouverte Q de cardinal k+1 a pour miniboule une sphère critique B, avec
// I(B) inclus dans Q inclus dans I(B) union S(B) ; donc Q = I union T avec
// T inclus dans S de cardinal k+1-|I|. Énumérer les sphères critiques puis ces
// sous-ensembles T est complet, à condition que le catalogue contienne B, ce que
// `s_max = n` garantit.
//
// ---------------------------------------------------------------------------
// L'UNIVERS DES FACETTES EST JUGÉ INDÉPENDAMMENT
// ---------------------------------------------------------------------------
//
// La vérité n'attend plus les facettes du sujet : elle énumère elle-même toutes
// les cofaces de cardinal k+1 par vacuité ouverte, en déduit son propre univers
// de facettes, et compare les DEUX ensembles avant de comparer lambda et M. Une
// coface omise par le sujet devient donc un désaccord, au lieu d'être invisible.
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/order_k_flats.hpp"

using mhgp::P3;
using mhgp::i32;

static P3 pt(int x, int y, int z) {
  P3 p{};
  p.x = (i32)x; p.y = (i32)y; p.z = (i32)z;
  return p;
}

// Miniboule exacte d'un ensemble. Elle ne passe pas necessairement par tous ses
// points : c'est justement ce qui distingue une coface de son sous-ensemble.
static bool miniball_of_set(const std::vector<P3>& pts, const std::vector<i32>& set,
                            mhgp::Sphere* out) {
  const mhgp::MiniballResult mb = mhgp::miniball_of(pts, set.data(), (int)set.size());
  if (!mb.ok) return false;
  *out = mb.sph;
  return true;
}

// GABRIEL AU SENS OUVERT : aucun point extérieur STRICTEMENT à l'intérieur de la
// miniboule. Les points extérieurs exactement sur la sphère sont permis — c'est
// la politique d'extra-shell développée.
static bool gabriel_open(const std::vector<P3>& pts, const std::vector<i32>& coface,
                         mhgp::Sphere* sphere_out) {
  if (!miniball_of_set(pts, coface, sphere_out)) return false;
  for (i32 z = 0; z < (i32)pts.size(); ++z) {
    if (std::binary_search(coface.begin(), coface.end(), z)) continue;
    if (mhgp::sphere_side(*sphere_out, pts[(std::size_t)z]) < 0) return false;
  }
  return true;
}

struct Dichotomy {
  long long clouds_decided = 0;
  long long direct_cofaces = 0;
  long long direct_missing = 0;
  long long direct_extra = 0;
  long long core_facets = 0;
  long long facets_missing = 0;
  long long facets_extra = 0;
  long long closed_nonempty = 0;
  long long closed_empty = 0;
  long long cominimisers = 0;
  long long cominimisers_max = 0;
  long long facets_from_several_cofaces = 0;
  long long cofaces_from_several_facets = 0;
  long long closed_ball_touched = 0;
  long long index_internal_nodes = 0;
  long long wrong_level = 0;
  long long wrong_set = 0;
  // Attache resolue par facette coeur : classification des intrus STRICTS.
  long long intruders_zero = 0;
  long long intruders_one = 0;
  long long intruders_many = 0;
  long long attachment_candidates = 0;
  long long attachment_cominimisers = 0;
  // PORTE REGULIERE. Une facette dont la boule FERMEE contient un point
  // exterieur alors que la boule OUVERTE n'en contient aucun porte une egalite
  // exterieure : le domaine regulier l'interdit. Le compter est la seule facon
  // de savoir si une campagne a le droit d'appeler ses candidats des attaches.
  long long outside_equalities = 0;
  // Support minimal UNIQUE : deux sous-ensembles de meme cardinal donnant la meme
  // miniboule rendraient u_F ambigu, donc le choix canonique de l'attache aussi.
  long long ambiguous_support = 0;
  long long attachment_target_not_smaller = 0;
  long long attachment_target_outside_core = 0;
  long long disagreements = 0;
};

// Vérité indépendante de lambda(F) et M(F) : balayage de tous les points
// extérieurs, avec le niveau conservé pour être comparé lui aussi.
static void brute_first_incidence(const std::vector<P3>& pts, const std::vector<i32>& facet,
                                  bool* have, mhgp::Sphere* level_out,
                                  std::set<std::vector<i32>>* set_out) {
  const int n = (int)pts.size();
  *have = false;
  set_out->clear();
  for (i32 x = 0; x < n; ++x) {
    if (std::binary_search(facet.begin(), facet.end(), x)) continue;
    std::vector<i32> coface = facet;
    coface.push_back(x);
    std::sort(coface.begin(), coface.end());
    mhgp::Sphere sphere{};
    if (!miniball_of_set(pts, coface, &sphere)) continue;
    if (!*have) { *level_out = sphere; *have = true; set_out->insert(coface); continue; }
    const int cmp = mhgp::sphere_cmp_beta(sphere, *level_out);
    if (cmp < 0) { *level_out = sphere; set_out->clear(); set_out->insert(coface); }
    else if (cmp == 0) set_out->insert(coface);
  }
}

static bool run_cloud(const std::vector<P3>& pts, int k, Dichotomy* out) {
  const int n = (int)pts.size();
  if (k + 1 > n) return true;

  // ------------------------------------------------------------------ SUJET
  // Catalogue complet : s_max = n, sinon une sphère critique portant des points
  // surnuméraires sur sa coquille sort du filtre de rang et ses cofaces de
  // Gabriel ouvertes disparaissent.
  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const mhgp::Catalogue catalogue = mhgp3v::flat_catalogue(pts, n, &st, &status, false, true);
  if (status != mhgp3v::CloudStatus::kOk) {
    // Un statut non `kOk` ne doit pas censurer silencieusement le nuage.
    printf("  STATUT %s : nuage non decide\n", mhgp3v::cloud_status_name(status));
    ++out->disagreements;
    return false;
  }
  ++out->clouds_decided;

  mhgp3v::CertifiedIndex index;
  index.build(pts, 4);
  for (const auto& node : index.nodes)
    if (node.left >= 0) ++out->index_internal_nodes;

  std::map<std::vector<i32>, mhgp::Sphere> direct;
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres) {
    std::vector<i32> interior, shell;
    for (int i = 0; i < sphere.rank; ++i) {
      const i32 z = catalogue.members[(std::size_t)(sphere.members_begin + i)];
      if (mhgp::sphere_side(sphere.sph, pts[(std::size_t)z]) == 0) shell.push_back(z);
      else interior.push_back(z);
    }
    const int want = k + 1 - (int)interior.size();
    if (want < 0 || want > (int)shell.size()) continue;
    // Sous-ensembles T de la coquille : le développement des extra-shells.
    std::vector<int> choose((std::size_t)std::max(0, want));
    for (int i = 0; i < want; ++i) choose[(std::size_t)i] = i;
    while (true) {
      std::vector<i32> coface = interior;
      for (int i = 0; i < want; ++i) coface.push_back(shell[(std::size_t)choose[(std::size_t)i]]);
      std::sort(coface.begin(), coface.end());
      mhgp::Sphere own{};
      if (gabriel_open(pts, coface, &own)) direct.emplace(coface, own);
      if (want == 0) break;
      int i = want - 1;
      while (i >= 0 && choose[(std::size_t)i] == (int)shell.size() - want + i) --i;
      if (i < 0) break;
      ++choose[(std::size_t)i];
      for (int j = i + 1; j < want; ++j) choose[(std::size_t)j] = choose[(std::size_t)j - 1] + 1;
    }
  }
  out->direct_cofaces += (long long)direct.size();

  // ------------------------------------------------------------------ VÉRITÉ
  // Univers indépendant : toutes les cofaces de cardinal k+1 à vacuité ouverte.
  std::map<std::vector<i32>, mhgp::Sphere> truth_direct;
  {
    std::vector<int> choose((std::size_t)(k + 1));
    for (int i = 0; i <= k; ++i) choose[(std::size_t)i] = i;
    while (true) {
      std::vector<i32> coface;
      for (int i = 0; i <= k; ++i) coface.push_back((i32)choose[(std::size_t)i]);
      mhgp::Sphere sphere{};
      if (gabriel_open(pts, coface, &sphere)) truth_direct.emplace(coface, sphere);
      int i = k;
      while (i >= 0 && choose[(std::size_t)i] == n - (k + 1) + i) --i;
      if (i < 0) break;
      ++choose[(std::size_t)i];
      for (int j = i + 1; j <= k; ++j) choose[(std::size_t)j] = choose[(std::size_t)j - 1] + 1;
    }
  }
  for (const auto& entry : truth_direct)
    if (direct.find(entry.first) == direct.end()) {
      ++out->direct_missing;
      ++out->disagreements;
    }
  for (const auto& entry : direct)
    if (truth_direct.find(entry.first) == truth_direct.end()) {
      ++out->direct_extra;
      ++out->disagreements;
    }

  // ---- flux de suppressions : exactement k+1 records par coface directe ----
  struct Record { std::vector<i32> facet; mhgp::Sphere level; const std::vector<i32>* coface; };
  std::vector<Record> stream;
  for (const auto& entry : direct)
    for (std::size_t drop = 0; drop < entry.first.size(); ++drop) {
      std::vector<i32> facet;
      for (std::size_t i = 0; i < entry.first.size(); ++i)
        if (i != drop) facet.push_back(entry.first[i]);
      stream.push_back(Record{facet, entry.second, &entry.first});
    }

  std::map<std::vector<i32>, std::vector<std::size_t>> grouped;
  for (std::size_t i = 0; i < stream.size(); ++i) grouped[stream[i].facet].push_back(i);
  out->core_facets += (long long)grouped.size();

  // L'univers de facettes de la vérité, indépendant du sujet.
  std::set<std::vector<i32>> truth_facets;
  for (const auto& entry : truth_direct)
    for (std::size_t drop = 0; drop < entry.first.size(); ++drop) {
      std::vector<i32> facet;
      for (std::size_t i = 0; i < entry.first.size(); ++i)
        if (i != drop) facet.push_back(entry.first[i]);
      truth_facets.insert(facet);
    }
  for (const auto& facet : truth_facets)
    if (grouped.find(facet) == grouped.end()) { ++out->facets_missing; ++out->disagreements; }
  for (const auto& group : grouped)
    if (truth_facets.find(group.first) == truth_facets.end()) {
      ++out->facets_extra;
      ++out->disagreements;
    }

  std::map<std::vector<i32>, int> coface_provenance;
  for (const auto& group : grouped) {
    const std::vector<i32>& facet = group.first;
    if (group.second.size() > 1) ++out->facets_from_several_cofaces;

    mhgp::Sphere facet_ball{};
    if (!miniball_of_set(pts, facet, &facet_ball)) { ++out->disagreements; continue; }

    std::vector<i32> outside_in_ball;
    index.closed_ball(facet_ball, &out->closed_ball_touched, [&](i32 z) {
      if (!std::binary_search(facet.begin(), facet.end(), z)) outside_in_ball.push_back(z);
    });
    std::sort(outside_in_ball.begin(), outside_in_ball.end());

    std::set<std::vector<i32>> decided;
    bool have_level = false;
    mhgp::Sphere decided_level{};
    if (!outside_in_ball.empty()) {
      ++out->closed_nonempty;
      decided_level = facet_ball;
      have_level = true;
      for (i32 x : outside_in_ball) {
        std::vector<i32> coface = facet;
        coface.push_back(x);
        std::sort(coface.begin(), coface.end());
        decided.insert(coface);
      }
    } else {
      ++out->closed_empty;
      const mhgp::Sphere* best = nullptr;
      for (std::size_t i : group.second)
        if (best == nullptr || mhgp::sphere_cmp_beta(stream[i].level, *best) < 0)
          best = &stream[i].level;
      if (best != nullptr) {
        decided_level = *best;
        have_level = true;
        for (std::size_t i : group.second)
          if (mhgp::sphere_cmp_beta(stream[i].level, *best) == 0)
            decided.insert(*stream[i].coface);
      }
    }
    out->cominimisers += (long long)decided.size();
    out->cominimisers_max = std::max(out->cominimisers_max, (long long)decided.size());
    for (const auto& coface : decided) ++coface_provenance[coface];

    // ------------------------------------------------------------------------
    // UNE ATTACHE RESOLUE PAR FACETTE COEUR.
    //
    // `NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md` montre que sous la porte
    // reguliere il suffit d'une attache canonique par facette ayant au moins
    // DEUX intrus STRICTS — J_F, la boule OUVERTE, distincte de E_F qui decide
    // la branche. Avec z_F = min J_F et u_F = min U_F, la cible locale est
    // T_F = (F \ {u_F}) union {z_F}, et le lemme garantit beta(T_F) < a_F.
    //
    // La cible BRUTE est refutee : T_F peut ne pas appartenir a D_k, et il faut
    // alors viser le carrier strict RESOLU. Ce prototype verifie donc le lemme
    // de descente et MESURE combien de fois la cible brute sort du coeur ; il ne
    // resout rien, faute de reducteur horizontal.
    const mhgp::MiniballResult facet_mb =
        mhgp::miniball_of(pts, facet.data(), (int)facet.size());
    std::vector<i32> strict_intruders;
    for (i32 z = 0; z < n; ++z) {
      if (std::binary_search(facet.begin(), facet.end(), z)) continue;
      if (mhgp::sphere_side(facet_ball, pts[(std::size_t)z]) < 0) strict_intruders.push_back(z);
    }
    if (!outside_in_ball.empty() && strict_intruders.empty()) ++out->outside_equalities;
    if (facet_mb.ok) {
      int realisations = 0;
      const int q = facet_mb.n_support;
      std::vector<int> choose((std::size_t)q);
      for (int i = 0; i < q; ++i) choose[(std::size_t)i] = i;
      const int size = (int)facet.size();
      while (q <= size) {
        std::vector<i32> subset;
        for (int i = 0; i < q; ++i) subset.push_back(facet[(std::size_t)choose[(std::size_t)i]]);
        mhgp::Sphere candidate_ball{};
        if (miniball_of_set(pts, subset, &candidate_ball) &&
            mhgp::sphere_cmp_beta(candidate_ball, facet_ball) == 0) {
          bool covers = true;
          for (i32 z : facet)
            if (mhgp::sphere_side(candidate_ball, pts[(std::size_t)z]) > 0) covers = false;
          if (covers) ++realisations;
        }
        int i = q - 1;
        while (i >= 0 && choose[(std::size_t)i] == size - q + i) --i;
        if (i < 0) break;
        ++choose[(std::size_t)i];
        for (int j = i + 1; j < q; ++j) choose[(std::size_t)j] = choose[(std::size_t)j - 1] + 1;
      }
      if (realisations > 1) ++out->ambiguous_support;
    }
    if (strict_intruders.empty()) ++out->intruders_zero;
    else if (strict_intruders.size() == 1) ++out->intruders_one;
    else {
      ++out->intruders_many;
      ++out->attachment_candidates;
      out->attachment_cominimisers += (long long)decided.size();
      if (facet_mb.ok) {
        const i32 z_f = strict_intruders.front();
        const i32 u_f = facet_mb.support[0];
        std::vector<i32> target;
        for (i32 x : facet) if (x != u_f) target.push_back(x);
        target.push_back(z_f);
        std::sort(target.begin(), target.end());
        mhgp::Sphere target_ball{};
        if (!miniball_of_set(pts, target, &target_ball) ||
            mhgp::sphere_cmp_beta(target_ball, facet_ball) >= 0) {
          ++out->attachment_target_not_smaller;
          ++out->disagreements;                 // le lemme de descente est faux
        }
        if (grouped.find(target) == grouped.end() &&
            truth_facets.find(target) == truth_facets.end())
          ++out->attachment_target_outside_core;
      }
    }

    bool truth_have = false;
    mhgp::Sphere truth_level{};
    std::set<std::vector<i32>> truth_set;
    brute_first_incidence(pts, facet, &truth_have, &truth_level, &truth_set);
    // Le NIVEAU est comparé, pas seulement l'ensemble : un lot correct porté par
    // un mauvais lambda passerait sinon.
    if (truth_have != have_level ||
        (truth_have && mhgp::sphere_cmp_beta(decided_level, truth_level) != 0)) {
      ++out->wrong_level;
      ++out->disagreements;
    }
    if (decided != truth_set) {
      ++out->wrong_set;
      ++out->disagreements;
      printf("  DESACCORD facette {");
      for (std::size_t i = 0; i < facet.size(); ++i) printf("%s%d", i ? "," : "", facet[i]);
      printf("} : dichotomie %zu, verite %zu (branche %s)\n", decided.size(), truth_set.size(),
             outside_in_ball.empty() ? "vide" : "fermee");
    }
  }
  for (const auto& entry : coface_provenance)
    if (entry.second > 1) ++out->cofaces_from_several_facets;
  return true;
}

int main(int argc, char** argv) {
  int clouds = 60, npoints = 20, coord = 22, k = 3;
  long long seed = 4242;
  int min_closed = 0, min_empty = 0, min_internal = 0, min_attachments = 0, require_regular = 0;
  auto integer = [](const char* text, long long* value) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    const bool negative = (*first == '-');
    if (negative) ++first;
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto result = std::from_chars(first, last, magnitude);
    if (result.ec != std::errc{} || result.ptr != last) return false;
    if (magnitude > 1000000000ULL) return false;
    *value = negative ? -(long long)magnitude : (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    if (!strcmp(argv[i], "--clouds")) target = &clouds;
    else if (!strcmp(argv[i], "--points")) target = &npoints;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--k")) target = &k;
    else if (!strcmp(argv[i], "--min-closed")) target = &min_closed;
    else if (!strcmp(argv[i], "--min-empty")) target = &min_empty;
    else if (!strcmp(argv[i], "--min-internal-nodes")) target = &min_internal;
    else if (!strcmp(argv[i], "--min-attachments")) target = &min_attachments;
    else if (!strcmp(argv[i], "--require-regular")) { require_regular = 1; continue; }
    else if (!strcmp(argv[i], "--seed")) {
      if (!has || value < 0) { printf("ECHEC : valeur entiere invalide pour --seed\n"); return 2; }
      ++i;
      seed = value;
      continue;
    } else {
      printf("ECHEC : argument inconnu %s\n", argv[i]);
      return 2;
    }
    if (!has) {
      printf("ECHEC : valeur entiere invalide ou manquante pour %s\n", argv[i]);
      return 2;
    }
    ++i;
    *target = (int)value;
  }
  if (clouds < 1 || npoints < 2 || npoints > 64 || coord < 2 || coord > 65536 || k < 1 ||
      k >= npoints || min_closed < 0 || min_empty < 0 || min_internal < 0 || min_attachments < 0) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }
  {
    const long long capacity = (long long)coord * coord * coord;
    if ((long long)npoints > capacity) {
      printf("ECHEC : campagne impossible, %d points distincts dans %lld positions\n", npoints,
             capacity);
      return 2;
    }
  }

  // FIXTURE REGULIERE DES DIX POINTS. Elle satisfait la porte reguliere — aucun
  // point exterieur sur la coquille d'un triplet, tous les quadruplets
  // independants — et pourtant les TROIS bras immediats de F = {2,8,9} sont hors
  // du coeur. Aucun choix du support supprime ne sauve la cible brute : le
  // resolver du carrier strict est inevitable.
  {
    const std::vector<P3> ten{pt(0, 1, 4), pt(18, 6, 24), pt(38, 4, 17), pt(12, 22, 29),
                              pt(20, 40, 11), pt(22, 5, 24), pt(4, 25, 10), pt(17, 6, 21),
                              pt(15, 31, 6), pt(8, 21, 14)};
    const int m = (int)ten.size();
    std::set<std::vector<i32>> core;
    for (int a = 0; a < m; ++a) for (int b = a + 1; b < m; ++b)
      for (int c = b + 1; c < m; ++c) for (int d = c + 1; d < m; ++d) {
        const std::vector<i32> coface{(i32)a, (i32)b, (i32)c, (i32)d};
        mhgp::Sphere sphere{};
        if (!gabriel_open(ten, coface, &sphere)) continue;
        for (std::size_t drop = 0; drop < 4; ++drop) {
          std::vector<i32> facet;
          for (std::size_t i = 0; i < 4; ++i) if (i != drop) facet.push_back(coface[i]);
          core.insert(facet);
        }
      }
    const std::vector<i32> facet{2, 8, 9};
    const mhgp::MiniballResult facet_mb = mhgp::miniball_of(ten, facet.data(), 3);
    int faults = 0;
    if (core.find(facet) == core.end() || !facet_mb.ok || facet_mb.n_support != 3) ++faults;
    int arms_outside = 0, arms_smaller = 0;
    if (facet_mb.ok) {
      std::vector<i32> intruders;
      for (i32 z = 0; z < m; ++z) {
        if (std::binary_search(facet.begin(), facet.end(), z)) continue;
        if (mhgp::sphere_side(facet_mb.sph, ten[(std::size_t)z]) < 0) intruders.push_back(z);
      }
      if (intruders.size() != 3) ++faults;
      const i32 z_f = intruders.empty() ? -1 : intruders.front();
      for (i32 dropped : facet) {
        std::vector<i32> arm;
        for (i32 x : facet) if (x != dropped) arm.push_back(x);
        if (z_f >= 0) arm.push_back(z_f);
        std::sort(arm.begin(), arm.end());
        mhgp::Sphere arm_ball{};
        if (!miniball_of_set(ten, arm, &arm_ball)) { ++faults; continue; }
        if (mhgp::sphere_cmp_beta(arm_ball, facet_mb.sph) < 0) ++arms_smaller;
        if (core.find(arm) == core.end()) ++arms_outside;
      }
    }
    if (arms_smaller != 3 || arms_outside != 3) ++faults;
    if (faults) {
      printf("[fixture dix points] %d faute(s) : bras strictement plus petits=%d, hors coeur=%d"
             " (attendu 3 et 3)\n", faults, arms_smaller, arms_outside);
      return 1;
    }
    printf("[fixture dix points] les trois bras immediats sont strictement plus petits"
           " ET hors du coeur : la cible brute est refutee sous regularite\n");
  }

  Dichotomy total;
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);
  int failures = 0;
  for (int c = 0; c < clouds; ++c) {
    std::vector<P3> pts;
    for (int guard = 0; (int)pts.size() < npoints && guard < 200 * npoints; ++guard) {
      const P3 q = pt(pick(rng), pick(rng), pick(rng));
      bool seen = false;
      for (const P3& r : pts) if (r.x == q.x && r.y == q.y && r.z == q.z) seen = true;
      if (!seen) pts.push_back(q);
    }
    if ((int)pts.size() < npoints) { printf("ECHEC : nuage %d non genere\n", c); return 3; }
    if (!run_cloud(pts, k, &total)) ++failures;
  }

  // PROVENANCE. La graine et les quatre parametres sont imprimes : j'ai publie
  // une table de trente nuages en l'attribuant aux douze du CTest, et rien dans
  // la sortie ne permettait de le voir.
  printf("provenance : --clouds %d --points %d --coord %d --k %d --seed %lld  (decides %lld)\n",
         clouds, npoints, coord, k, seed, total.clouds_decided);
  printf("source ouverte : cofaces=%lld  manquantes=%lld  surnumeraires=%lld\n",
         total.direct_cofaces, total.direct_missing, total.direct_extra);
  printf("facettes       : univers=%lld  manquantes=%lld  surnumeraires=%lld"
         "  portees par plusieurs cofaces=%lld\n", total.core_facets, total.facets_missing,
         total.facets_extra, total.facets_from_several_cofaces);
  printf("dichotomie     : branche fermee=%lld (%.1f%%)  branche vide=%lld"
         "  niveau faux=%lld  ensemble faux=%lld\n", total.closed_nonempty,
         total.core_facets ? 100.0 * (double)total.closed_nonempty / (double)total.core_facets : 0.0,
         total.closed_empty, total.wrong_level, total.wrong_set);
  printf("co-minimiseurs : total=%lld  moyenne=%.2f  maximum=%lld"
         "  proposes par plusieurs facettes=%lld\n", total.cominimisers,
         total.core_facets ? (double)total.cominimisers / (double)total.core_facets : 0.0,
         total.cominimisers_max, total.cofaces_from_several_facets);
  printf("attache        : intrus stricts 0/1/>=2 = %lld/%lld/%lld  CANDIDATS=%lld"
         "  co-minimiseurs fermes portes=%lld\n", total.intruders_zero,
         total.intruders_one, total.intruders_many, total.attachment_candidates,
         total.attachment_cominimisers);
  printf("               : lemme de descente viole=%lld  cible brute hors coeur=%lld\n",
         total.attachment_target_not_smaller, total.attachment_target_outside_core);
  // Le domaine regulier interdit une egalite exterieure pertinente. Tant que ce
  // compteur n'est pas nul, les objets ci-dessus sont des CANDIDATS locaux, pas
  // des attaches autorisees, et ils ne remplacent rien.
  const bool regular = (total.outside_equalities == 0 && total.ambiguous_support == 0);
  printf("porte regul.   : egalites exterieures sans intrus strict=%lld  supports ambigus=%lld"
         "  -> campagne %s\n", total.outside_equalities, total.ambiguous_support,
         regular ? "DANS le domaine regulier" : "HORS domaine regulier (candidats seulement)");
  printf("index          : points touches=%lld (%.1f par facette)  noeuds internes=%lld\n",
         total.closed_ball_touched,
         total.core_facets ? (double)total.closed_ball_touched / (double)total.core_facets : 0.0,
         total.index_internal_nodes);

  printf("\n%lld desaccords, %d nuages non decides\n", total.disagreements, failures);
  if (total.core_facets == 0) {
    printf("ECHEC : aucune facette, la campagne ne mesure rien\n");
    return 3;
  }
  if (total.closed_nonempty < min_closed || total.closed_empty < min_empty ||
      total.index_internal_nodes < min_internal || total.attachment_candidates < min_attachments) {
    printf("ECHEC : plancher de couverture non atteint — fermee %lld/%d, vide %lld/%d,"
           " noeuds internes %lld/%d, attaches %lld/%d\n", total.closed_nonempty, min_closed,
           total.closed_empty, min_empty, total.index_internal_nodes, min_internal,
           total.attachment_candidates, min_attachments);
    return 3;
  }
  if (require_regular && !regular) {
    printf("ECHEC : --require-regular exige zero egalite exterieure et zero support ambigu\n");
    return 3;
  }
  if (total.disagreements == 0 && failures == 0) {
    printf("OK : source ouverte, univers de facettes, lambda(F) et M(F) tous concordants\n");
    return 0;
  }
  return 1;
}
