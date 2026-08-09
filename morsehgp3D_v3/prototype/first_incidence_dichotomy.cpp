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

// ---------------------------------------------------------------------------
// LA PORTE RÉGULIÈRE EST UNE PROPRIÉTÉ DE LA FACETTE, PAS UNE MOYENNE DE CAMPAGNE
// ---------------------------------------------------------------------------
//
// Mon premier compteur d'égalité extérieure ne s'incrémentait que si la liste
// d'intrus stricts était VIDE. L'audit le réfute au chiffre près sur cinq
// points : pour F = {0,1,2}, le point 3 est un intrus strict et le point 4 un
// extérieur exactement sur la coquille, et le compteur rendait zéro. Une égalité
// extérieure est une égalité extérieure, qu'un intrus l'accompagne ou non.
//
// Trois autres obligations manquaient :
//   * un support essentiel est minimal pour l'INCLUSION, tous cardinaux
//     confondus. Comparer les seuls sous-ensembles du cardinal rendu laisse
//     passer une paire antipodale et un triangle réalisant la même boule.
//   * tout point de F hors du support doit être STRICTEMENT intérieur, sinon la
//     coquille de F elle-même est dégénérée et u_F reste ambigu.
//   * une panne de primitive dans l'AUTORITÉ doit rendre un statut distinct, pas
//     laisser filer la facette.
struct Regularity {
  bool outside_equality = false;
  bool ambiguous_support = false;
  bool shell_outside_support = false;
  bool primitive_failed = false;
  bool unsupported_size = false;  // énumération des sous-ensembles hors domaine
  int essential = 0;              // nombre de supports essentiels, tous cardinaux
  bool regular() const {
    return !outside_equality && !ambiguous_support && !shell_outside_support &&
           !primitive_failed && !unsupported_size;
  }
};

// Un support S de F est un sous-ensemble dont la miniboule ENFERME F avec le
// même beta : par unicité de la boule englobante minimale, elle EST alors B_F.
// L'essentialité est la minimalité pour l'inclusion, indépendante du cardinal.
static Regularity facet_regularity(const std::vector<P3>& pts, const std::vector<i32>& facet,
                                   const mhgp::Sphere& ball) {
  Regularity reg;
  const int n = (int)pts.size();
  const int m = (int)facet.size();
  for (i32 z = 0; z < n; ++z) {
    if (std::binary_search(facet.begin(), facet.end(), z)) continue;
    if (mhgp::sphere_side(ball, pts[(std::size_t)z]) == 0) reg.outside_equality = true;
  }
  const mhgp::MiniballResult mb = mhgp::miniball_of(pts, facet.data(), m);
  if (!mb.ok) { reg.primitive_failed = true; return reg; }
  for (i32 x : facet) {
    if (mhgp::sphere_side(ball, pts[(std::size_t)x]) != 0) continue;
    bool in_support = false;
    for (int i = 0; i < mb.n_support; ++i) if (mb.support[i] == x) in_support = true;
    if (!in_support) reg.shell_outside_support = true;
  }
  if (m > 12) { reg.unsupported_size = true; return reg; }
  std::vector<unsigned> realisers;
  for (unsigned mask = 1; mask < (1u << m); ++mask) {
    std::vector<i32> subset;
    for (int i = 0; i < m; ++i) if (mask & (1u << i)) subset.push_back(facet[(std::size_t)i]);
    mhgp::Sphere candidate{};
    // FAIL-CLOSED. Un `continue` sur une panne de primitive pouvait faire manquer
    // une realisation, donc declarer unique un support qui ne l'est pas.
    if (!miniball_of_set(pts, subset, &candidate)) { reg.primitive_failed = true; return reg; }
    if (mhgp::sphere_cmp_beta(candidate, ball) != 0) continue;
    bool encloses = true;
    for (i32 x : facet)
      if (mhgp::sphere_side(candidate, pts[(std::size_t)x]) > 0) encloses = false;
    if (encloses) realisers.push_back(mask);
  }
  for (unsigned mask : realisers) {
    bool minimal = true;
    for (unsigned other : realisers)
      if (other != mask && (other & mask) == other) minimal = false;
    if (minimal) ++reg.essential;
  }
  if (reg.essential != 1) reg.ambiguous_support = true;
  return reg;
}

static std::vector<i32> strict_intruders_of(const std::vector<P3>& pts,
                                            const std::vector<i32>& set,
                                            const mhgp::Sphere& ball) {
  std::vector<i32> out;
  for (i32 z = 0; z < (i32)pts.size(); ++z) {
    if (std::binary_search(set.begin(), set.end(), z)) continue;
    if (mhgp::sphere_side(ball, pts[(std::size_t)z]) < 0) out.push_back(z);
  }
  return out;
}

// BUDGET DÉMONTRÉ. Chaque pas de la descente fait STRICTEMENT décroître beta sur
// des sous-ensembles de cardinal fixe, donc la chaîne est plus courte que leur
// nombre. `n*n+16` n'était ni démontré, ni transplantable : à 50 000 points le
// produit `int*int` déborde. Ici le calcul sature en 64 bits.
static long long binomial_saturated(int n, int k) {
  if (k < 0 || k > n) return 0;
  const long long kCap = 1LL << 40;
  long long value = 1;
  for (int i = 1; i <= k; ++i) {
    value = value * (long long)(n - k + i);
    if (value >= kCap) return kCap;
    value /= (long long)i;
  }
  return value < 1 ? 1 : value;
}

// beta = |num|^2 / den^2, comparé à la fraction EXACTE p/q. Les fixtures ont des
// coordonnées minuscules ; toute magnitude qui sortirait du domaine sûr est un
// échec, pas un silence.
static bool beta_equals(const mhgp::Sphere& s, long long p, long long q) {
  const mhgp::i128 kBound = (mhgp::i128)1 << 40;
  if (s.den <= 0 || s.den >= kBound || q <= 0) return false;
  if (s.nx >= kBound || s.nx <= -kBound || s.ny >= kBound || s.ny <= -kBound ||
      s.nz >= kBound || s.nz <= -kBound) return false;
  const mhgp::i128 num = s.nx * s.nx + s.ny * s.ny + s.nz * s.nz;
  return num * (mhgp::i128)q == (mhgp::i128)p * s.den * s.den;
}

// Univers exhaustif des cofaces directes, et le cœur D_k qu'il engendre. Réservé
// aux fixtures : le coût est celui de tous les (k+1)-sous-ensembles.
static std::map<std::vector<i32>, mhgp::Sphere> direct_cofaces_of(const std::vector<P3>& pts,
                                                                 int k) {
  std::map<std::vector<i32>, mhgp::Sphere> out;
  const int n = (int)pts.size();
  if (n > 24 || k + 1 > n) return out;
  std::vector<int> choose((std::size_t)(k + 1));
  for (int i = 0; i <= k; ++i) choose[(std::size_t)i] = i;
  while (true) {
    std::vector<i32> coface;
    for (int i = 0; i <= k; ++i) coface.push_back((i32)choose[(std::size_t)i]);
    mhgp::Sphere sphere{};
    if (gabriel_open(pts, coface, &sphere)) out.emplace(coface, sphere);
    int i = k;
    while (i >= 0 && choose[(std::size_t)i] == n - (k + 1) + i) --i;
    if (i < 0) break;
    ++choose[(std::size_t)i];
    for (int j = i + 1; j <= k; ++j) choose[(std::size_t)j] = choose[(std::size_t)j - 1] + 1;
  }
  return out;
}

static std::set<std::vector<i32>> core_facets_of(const std::vector<P3>& pts, int k) {
  std::set<std::vector<i32>> out;
  for (const auto& entry : direct_cofaces_of(pts, k))
    for (std::size_t drop = 0; drop < entry.first.size(); ++drop) {
      std::vector<i32> facet;
      for (std::size_t i = 0; i < entry.first.size(); ++i)
        if (i != drop) facet.push_back(entry.first[i]);
      out.insert(facet);
    }
  return out;
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
  // Support essentiel UNIQUE, minimal pour l'INCLUSION et tous cardinaux
  // confondus : deux realisations rendraient u_F ambigu, donc le choix canonique
  // de l'attache aussi.
  long long ambiguous_support = 0;
  // Un point de F sur la coquille mais hors du support rend la coquille de F
  // elle-meme degeneree.
  long long shell_outside_support = 0;
  // Panne de primitive dans l'AUTORITE : fail-closed, statut distinct.
  long long authority_primitive_failed = 0;
  // Facettes a >= 2 intrus stricts REFUSEES parce que hors porte reguliere.
  long long attachment_unsupported = 0;
  // DESCENTE LOCALE DU CARRIER. Tant que la facette a au moins deux intrus
  // stricts, remplacer min U_H par min J_H fait STRICTEMENT decroitre beta ; les
  // branches a un intrus et sans intrus terminent dans le coeur. Seul le `find`
  // final de cette clef coeur dans la partition anterieure reste global.
  long long descent_runs = 0;
  long long descent_steps = 0;
  long long descent_steps_max = 0;
  long long descent_not_strict = 0;
  long long descent_terminal_outside_core = 0;
  long long descent_witness_not_below = 0;
  long long descent_terminal_empty = 0;
  long long descent_terminal_single = 0;
  // RECU du terminal : une coface DIRECTE engagee, de niveau < a_F. Une
  // appartenance au coeur tous niveaux ne dit rien sur la DATE.
  long long descent_receipts = 0;
  long long descent_receipt_missing = 0;
  long long descent_receipt_not_below = 0;
  // Sorties non concluantes, chacune nommee : epuisement de budget et refus hors
  // domaine ne sont pas le meme evenement qu'un terminal hors coeur.
  long long descent_budget_exhausted = 0;
  long long descent_unsupported = 0;
  long long descent_broken = 0;
  long long attachment_target_not_smaller = 0;
  long long attachment_target_outside_core = 0;
  long long disagreements = 0;
};

// ---------------------------------------------------------------------------
// DESCENTE LOCALE DU CARRIER, RÉAUTHENTIFIÉE À CHAQUE PAS ET AVEC REÇU
// ---------------------------------------------------------------------------
//
// Tant que la facette courante a au moins deux intrus STRICTS, remplacer
// min U_H par min J_H fait strictement décroître beta. La chaîne s'arrête sur une
// facette à un intrus — sa première incidence est sa propre boule — ou sans
// intrus, où le témoin transporté prouve qu'une coface directe arrive avant a_F.
//
// Trois exigences que ma première version n'honorait pas :
//   * le théorème n'est démontré que sous la porte régulière. Un descendant hors
//     domaine doit être un REFUS, pas un désaccord scientifique.
//   * le reçu doit ENGAGER une coface directe et son niveau < a_F. Une
//     appartenance au cœur tous niveaux ne dit rien sur la date.
//   * l'épuisement du budget est un statut propre, distinct d'un terminal hors
//     cœur, et le budget est démontré.
struct Receipt {
  std::vector<i32> terminal;   // la facette du coeur atteinte
  std::vector<i32> coface;     // la coface DIRECTE engagee, identite comprise
  long long steps = 0;
  mhgp::Sphere level{};        // son niveau, qui doit etre < a_F
  bool has_level = false;
  bool branch_empty = false;
};

static void descend_to_core(const std::vector<P3>& pts, const std::vector<i32>& target,
                            const mhgp::Sphere& cutoff,
                            const std::set<std::vector<i32>>& truth_facets,
                            const std::map<std::vector<i32>, mhgp::Sphere>& truth_direct,
                            Dichotomy* out, Receipt* receipt = nullptr) {
  const int n = (int)pts.size();
  const long long budget = binomial_saturated(n, (int)target.size());
  enum class End { kBudget, kSingle, kEmpty, kUnsupported, kBroken };
  End end = End::kBudget;
  std::vector<i32> current = target;
  long long steps = 0;
  ++out->descent_runs;
  while (steps <= budget) {
    mhgp::Sphere ball{};
    if (!miniball_of_set(pts, current, &ball)) { end = End::kBroken; break; }
    const Regularity here = facet_regularity(pts, current, ball);
    if (here.primitive_failed) { end = End::kBroken; break; }
    if (!here.regular()) { end = End::kUnsupported; break; }
    const std::vector<i32> intruders = strict_intruders_of(pts, current, ball);
    if (intruders.size() < 2) {
      end = intruders.empty() ? End::kEmpty : End::kSingle;
      break;
    }
    const mhgp::MiniballResult mb = mhgp::miniball_of(pts, current.data(), (int)current.size());
    if (!mb.ok) { end = End::kBroken; break; }
    std::vector<i32> next;
    for (i32 x : current) if (x != mb.support[0]) next.push_back(x);
    next.push_back(intruders.front());
    std::sort(next.begin(), next.end());
    mhgp::Sphere next_ball{};
    if (!miniball_of_set(pts, next, &next_ball) ||
        mhgp::sphere_cmp_beta(next_ball, ball) >= 0) {
      ++out->descent_not_strict;
      ++out->disagreements;
      end = End::kBroken;
      break;
    }
    current = next;
    ++steps;
  }
  out->descent_steps += steps;
  out->descent_steps_max = std::max(out->descent_steps_max, steps);
  if (end == End::kBudget) { ++out->descent_budget_exhausted; ++out->disagreements; return; }
  if (end == End::kUnsupported) { ++out->descent_unsupported; return; }
  // Une panne de primitive DANS la descente ne peut pas etre un simple compteur :
  // elle est fail-closed comme toutes les autres.
  if (end == End::kBroken) { ++out->descent_broken; ++out->disagreements; return; }
  if (end == End::kEmpty) ++out->descent_terminal_empty;
  else ++out->descent_terminal_single;
  if (truth_facets.find(current) == truth_facets.end()) {
    ++out->descent_terminal_outside_core;
    ++out->disagreements;
  }
  bool have = false;
  mhgp::Sphere best{};
  std::vector<i32> best_coface;
  for (i32 x = 0; x < n; ++x) {
    if (std::binary_search(current.begin(), current.end(), x)) continue;
    std::vector<i32> coface = current;
    coface.push_back(x);
    std::sort(coface.begin(), coface.end());
    const auto it = truth_direct.find(coface);
    if (it == truth_direct.end()) continue;
    if (!have || mhgp::sphere_cmp_beta(it->second, best) < 0) {
      best = it->second;
      best_coface = coface;
      have = true;
    }
  }
  if (!have) { ++out->descent_receipt_missing; ++out->disagreements; }
  else if (mhgp::sphere_cmp_beta(best, cutoff) >= 0) {
    ++out->descent_receipt_not_below;
    ++out->disagreements;
  } else {
    ++out->descent_receipts;
  }
  // LE RECU EST SERIALISE, et pas seulement compte : le terminal, la longueur de
  // la chaine, l'IDENTITE de la coface directe engagee et son niveau. Un compteur
  // ne permet a personne de rejouer ce qui a ete conclu.
  if (receipt != nullptr) {
    receipt->terminal = current;
    receipt->coface = best_coface;
    receipt->steps = steps;
    receipt->level = best;
    receipt->has_level = have;
    receipt->branch_empty = (end == End::kEmpty);
  }
}

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
    const Regularity reg = facet_regularity(pts, facet, facet_ball);
    if (reg.outside_equality) ++out->outside_equalities;
    if (reg.ambiguous_support) ++out->ambiguous_support;
    if (reg.shell_outside_support) ++out->shell_outside_support;
    if (reg.primitive_failed || reg.unsupported_size) {
      ++out->authority_primitive_failed;
      ++out->disagreements;
    }
    const std::vector<i32> strict_intruders = strict_intruders_of(pts, facet, facet_ball);
    if (strict_intruders.empty()) ++out->intruders_zero;
    else if (strict_intruders.size() == 1) ++out->intruders_one;
    else if (!reg.regular() || !facet_mb.ok) {
      // Hors porte reguliere le theoreme n'est pas demontre : la facette est
      // REFUSEE, elle ne compte pas comme candidate et rien n'en est conclu.
      ++out->intruders_many;
      ++out->attachment_unsupported;
    } else {
      ++out->intruders_many;
      ++out->attachment_candidates;
      out->attachment_cominimisers += (long long)decided.size();
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

      // Le temoin W_0 = T_F union {w_F}, ou w_F est le second intrus strict.
      // L'essentialite de u_F donne beta(W_0) < a_F, et sans lui la branche
      // sans intrus ne prouverait pas que la premiere coface arrive avant a_F.
      {
        std::vector<i32> witness = target;
        witness.push_back(strict_intruders[1]);
        std::sort(witness.begin(), witness.end());
        mhgp::Sphere witness_ball{};
        if (!miniball_of_set(pts, witness, &witness_ball) ||
            mhgp::sphere_cmp_beta(witness_ball, facet_ball) >= 0) {
          ++out->descent_witness_not_below;
          ++out->disagreements;
        }
      }

      descend_to_core(pts, target, facet_ball, truth_facets, truth_direct, out);
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
  int min_descent = 0, min_steps = 0, min_terminal_empty = 0, min_terminal_single = 0;
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
    else if (!strcmp(argv[i], "--min-descent")) target = &min_descent;
    else if (!strcmp(argv[i], "--min-descent-steps")) target = &min_steps;
    else if (!strcmp(argv[i], "--min-terminal-empty")) target = &min_terminal_empty;
    else if (!strcmp(argv[i], "--min-terminal-single")) target = &min_terminal_single;
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
      k >= npoints || min_closed < 0 || min_empty < 0 || min_internal < 0 ||
      min_attachments < 0 || min_descent < 0 || min_steps < 0 || min_terminal_empty < 0 ||
      min_terminal_single < 0) {
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

  // FIXTURE DE L'EGALITE EXTERIEURE MIXTE. Mon compteur ne s'incrementait que si
  // la liste d'intrus stricts etait VIDE ; l'audit le refute sur ces cinq points.
  // Pour F = {0,1,2} le point 3 est un intrus STRICT et le point 4 un exterieur
  // exactement sur la coquille : l'ancienne version rendait zero et aurait appele
  // cette campagne reguliere. La fixture exige les deux a la fois.
  {
    const std::vector<P3> five{pt(3, 9, 13), pt(4, 9, 2), pt(10, 2, 14), pt(4, 6, 3),
                              pt(1, 9, 11)};
    const std::vector<i32> facet{0, 1, 2};
    mhgp::Sphere ball{};
    int faults = 0;
    if (!miniball_of_set(five, facet, &ball)) ++faults;
    const int expect[5] = {-1, 0, 0, -1, 0};
    for (i32 z = 0; z < 5 && !faults; ++z) {
      const int side = mhgp::sphere_side(ball, five[(std::size_t)z]);
      const int sign = side < 0 ? -1 : (side > 0 ? 1 : 0);
      if (sign != expect[(std::size_t)z]) {
        printf("[fixture egalite mixte] signe %d attendu %d pour le point %d\n", sign,
               expect[(std::size_t)z], z);
        ++faults;
      }
    }
    const mhgp::MiniballResult mb = mhgp::miniball_of(five, facet.data(), 3);
    if (!mb.ok || mb.n_support != 2 || mb.support[0] != 1 || mb.support[1] != 2) ++faults;
    mhgp::Sphere coface_ball{};
    if (!gabriel_open(five, std::vector<i32>{0, 1, 2, 3}, &coface_ball)) ++faults;
    const Regularity reg = facet_regularity(five, facet, ball);
    const std::vector<i32> intruders = strict_intruders_of(five, facet, ball);
    if (!reg.outside_equality || intruders.size() != 1 || intruders.front() != 3) ++faults;
    if (reg.regular()) ++faults;
    if (faults) {
      printf("[fixture egalite mixte] %d faute(s) : egalite=%d intrus=%zu reguliere=%d\n", faults,
             reg.outside_equality ? 1 : 0, intruders.size(), reg.regular() ? 1 : 0);
      return 1;
    }
    printf("[fixture egalite mixte] egalite exterieure ET intrus strict simultanes :"
           " la facette est REFUSEE, pas comptee candidate\n");
  }

  // FIXTURE DES DEUX SUPPORTS ESSENTIELS DE CARDINAUX DIFFERENTS. Sur la sphere de
  // centre (5,5,5) et de rayon 5, la paire antipodale et le triangle realisent la
  // MEME boule. Un enumerateur limite au cardinal rendu annonce une realisation
  // unique ; la minimalite pour l'inclusion en voit deux.
  {
    const std::vector<P3> six{pt(0, 5, 5), pt(10, 5, 5), pt(5, 10, 5), pt(5, 2, 9), pt(5, 2, 1)};
    const std::vector<i32> facet{0, 1, 2, 3, 4};
    mhgp::Sphere ball{};
    int faults = 0;
    if (!miniball_of_set(six, facet, &ball)) ++faults;
    for (i32 x : facet)
      if (mhgp::sphere_side(ball, six[(std::size_t)x]) != 0) ++faults;
    mhgp::Sphere pair_ball{}, triangle_ball{};
    if (!miniball_of_set(six, std::vector<i32>{0, 1}, &pair_ball) ||
        !miniball_of_set(six, std::vector<i32>{2, 3, 4}, &triangle_ball) ||
        mhgp::sphere_cmp_beta(pair_ball, ball) != 0 ||
        mhgp::sphere_cmp_beta(triangle_ball, ball) != 0) ++faults;
    const Regularity reg = facet_regularity(six, facet, ball);
    if (!reg.ambiguous_support || !reg.shell_outside_support || reg.regular()) ++faults;
    if (faults) {
      printf("[fixture deux supports] %d faute(s) : ambigu=%d coquille hors support=%d\n", faults,
             reg.ambiguous_support ? 1 : 0, reg.shell_outside_support ? 1 : 0);
      return 1;
    }
    printf("[fixture deux supports] paire antipodale et triangle realisent la meme boule :"
           " support ambigu detecte tous cardinaux confondus\n");
  }

  // FIXTURE DU CUBE u16 : SIX SUPPORTS MINIMAUX POUR UNE SEULE BOULE.
  // L'audit annonçait d'abord quatre paires antipodales, puis a corrige en six.
  // Mon predicat compte les supports essentiels — minimaux pour l'inclusion, tous
  // cardinaux confondus — donc il peut verifier ce compte au lieu de le croire :
  // les quatre diagonales, plus les DEUX tetraedres de parite, dont aucune face ni
  // aucune arete ne realise la boule.
  {
    std::vector<P3> cube;
    for (int x = 0; x < 2; ++x) for (int y = 0; y < 2; ++y) for (int z = 0; z < 2; ++z)
      cube.push_back(pt(2 * x, 2 * y, 2 * z));
    std::vector<i32> all;
    for (i32 i = 0; i < 8; ++i) all.push_back(i);
    mhgp::Sphere ball{};
    int faults = 0;
    if (!miniball_of_set(cube, all, &ball)) ++faults;
    for (i32 x : all) if (mhgp::sphere_side(ball, cube[(std::size_t)x]) != 0) ++faults;
    const Regularity reg = facet_regularity(cube, all, ball);
    if (reg.essential != 6 || !reg.ambiguous_support) ++faults;
    if (faults) {
      printf("[fixture cube u16] %d faute(s) : supports essentiels=%d (attendu 6)\n", faults,
             reg.essential);
      return 1;
    }
    printf("[fixture cube u16] %d supports minimaux pour une seule boule : quatre diagonales"
           " et deux tetraedres de parite — le compte de l'audit est verifie\n", reg.essential);
  }

  // FIXTURE E5 — LA BRANCHE SANS INTRUS ET SON TEMOIN. beta(F=AC)=33/2,
  // beta(T=CD)=9/2, J_T vide, et la coface temoin CDE est de niveau 162/25 < 33/2.
  // Sans le temoin, J_T vide ne dirait RIEN sur la date de la premiere coface.
  {
    const std::vector<P3> e5{pt(0, 0, 7), pt(0, 9, 6), pt(1, 4, 0), pt(0, 0, 1), pt(4, 1, 2)};
    const std::vector<i32> facet{0, 2}, target{2, 3}, witness{2, 3, 4};
    mhgp::Sphere a_f{}, t_ball{}, w_ball{};
    int faults = 0;
    if (!miniball_of_set(e5, facet, &a_f) || !beta_equals(a_f, 33, 2)) ++faults;
    if (!miniball_of_set(e5, target, &t_ball) || !beta_equals(t_ball, 9, 2)) ++faults;
    if (!miniball_of_set(e5, witness, &w_ball) || !beta_equals(w_ball, 162, 25)) ++faults;
    if (!strict_intruders_of(e5, target, t_ball).empty()) ++faults;
    if (mhgp::sphere_cmp_beta(w_ball, a_f) >= 0) ++faults;
    const std::set<std::vector<i32>> core = core_facets_of(e5, 2);
    std::map<std::vector<i32>, mhgp::Sphere> direct2 = direct_cofaces_of(e5, 2);
    if (core.find(target) == core.end()) ++faults;
    Dichotomy probe;
    Receipt receipt;
    descend_to_core(e5, target, a_f, core, direct2, &probe, &receipt);
    if (probe.descent_terminal_empty != 1 || probe.descent_receipts != 1 ||
        probe.descent_steps != 0 || probe.disagreements != 0) ++faults;
    // L'IDENTITE de la coface engagee, et non un compteur : le recu doit nommer
    // CDE, avec son niveau exact et la branche dont il provient.
    if (receipt.terminal != target || receipt.coface != witness || receipt.steps != 0 ||
        !receipt.has_level || !receipt.branch_empty || !beta_equals(receipt.level, 162, 25))
      ++faults;
    if (faults) {
      printf("[fixture E5] %d faute(s) : terminal vide=%lld recus=%lld etapes=%lld"
             " desaccords=%lld coface engagee={", faults, probe.descent_terminal_empty,
             probe.descent_receipts, probe.descent_steps, probe.disagreements);
      for (std::size_t i = 0; i < receipt.coface.size(); ++i)
        printf("%s%d", i ? "," : "", receipt.coface[i]);
      printf("}\n");
      return 1;
    }
    printf("[fixture E5] branche sans intrus : T=CD est dans D_2 et son recu engage");
    for (std::size_t i = 0; i < receipt.coface.size(); ++i)
      printf("%s%d", i ? "," : " {", receipt.coface[i]);
    printf("} de niveau 162/25 < 33/2\n");
  }

  // FIXTURE DES SEPT POINTS — LE LOOKUP IMMEDIAT ECHOUE, LA DESCENTE ABOUTIT.
  // A l'ordre trois, F = {0,1,6} est dans D_3 mais sa cible brute T = {1,2,6}
  // n'y est PAS : chercher T dans la partition anterieure echouerait. La descente
  // canonique, elle, atteint le coeur avec un recu.
  {
    const std::vector<P3> seven{pt(0, 400, 275), pt(600, 100, 324), pt(0, 200, 294),
                                pt(600, 600, 271), pt(500, 0, 276), pt(200, 0, 301),
                                pt(300, 600, 320)};
    const std::vector<i32> facet{0, 1, 6}, target{1, 2, 6};
    const std::set<std::vector<i32>> core = core_facets_of(seven, 3);
    std::map<std::vector<i32>, mhgp::Sphere> direct3 = direct_cofaces_of(seven, 3);
    mhgp::Sphere a_f{};
    int faults = 0;
    if (!miniball_of_set(seven, facet, &a_f)) ++faults;
    if (core.find(facet) == core.end()) ++faults;
    if (core.find(target) != core.end()) ++faults;
    Dichotomy probe;
    Receipt receipt;
    descend_to_core(seven, target, a_f, core, direct3, &probe, &receipt);
    if (probe.descent_receipts != 1 || probe.descent_steps < 1 || probe.disagreements != 0)
      ++faults;
    // Le terminal atteint doit etre DANS le coeur, la coface engagee le contenir,
    // et son niveau etre strictement sous le cutoff.
    if (core.find(receipt.terminal) == core.end() || !receipt.has_level ||
        receipt.coface.size() != receipt.terminal.size() + 1 ||
        mhgp::sphere_cmp_beta(receipt.level, a_f) >= 0) ++faults;
    if (faults) {
      printf("[fixture sept points] %d faute(s) : F dans le coeur=%d, T dans le coeur=%d,"
             " etapes=%lld recus=%lld desaccords=%lld\n", faults,
             core.find(facet) != core.end() ? 1 : 0, core.find(target) != core.end() ? 1 : 0,
             probe.descent_steps, probe.descent_receipts, probe.disagreements);
      return 1;
    }
    printf("[fixture sept points] T={1,2,6} est hors de D_3 : le lookup brut echoue,"
           " la descente atteint le coeur en %lld pas avec recu\n", probe.descent_steps);
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
  const bool regular = (total.outside_equalities == 0 && total.ambiguous_support == 0 &&
                        total.shell_outside_support == 0 &&
                        total.authority_primitive_failed == 0);
  printf("porte regul.   : egalites exterieures=%lld  supports ambigus=%lld"
         "  coquille hors support=%lld  primitive en panne=%lld  refus d'attache=%lld"
         "  -> campagne %s\n", total.outside_equalities, total.ambiguous_support,
         total.shell_outside_support, total.authority_primitive_failed,
         total.attachment_unsupported,
         regular ? "DANS le domaine regulier" : "HORS domaine regulier (candidats seulement)");
  printf("descente       : executions=%lld  etapes=%lld (moyenne %.2f, max %lld)"
         "  non stricte=%lld  terminal hors coeur=%lld  temoin non inferieur=%lld\n",
         total.descent_runs, total.descent_steps,
         total.descent_runs ? (double)total.descent_steps / (double)total.descent_runs : 0.0,
         total.descent_steps_max, total.descent_not_strict,
         total.descent_terminal_outside_core, total.descent_witness_not_below);
  printf("               : terminal sans intrus=%lld  a un intrus=%lld  RECUS=%lld"
         "  recu absent=%lld  recu non inferieur=%lld  budget epuise=%lld"
         "  refus=%lld  primitive=%lld\n", total.descent_terminal_empty,
         total.descent_terminal_single, total.descent_receipts, total.descent_receipt_missing,
         total.descent_receipt_not_below, total.descent_budget_exhausted,
         total.descent_unsupported, total.descent_broken);
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
  // PLANCHERS DE LA DESCENTE. Sans eux, supprimer le bloc de descente laisse les
  // CTests verts : un plancher sur les executions ne suffit pas, il faut au moins
  // un PAS effectif, les deux branches terminales et un recu.
  if (total.descent_runs < min_descent || total.descent_steps < min_steps ||
      total.descent_terminal_empty < min_terminal_empty ||
      total.descent_terminal_single < min_terminal_single ||
      (min_descent > 0 && total.descent_receipts < min_descent)) {
    printf("ECHEC : plancher de descente non atteint — executions %lld/%d, etapes %lld/%d,"
           " terminal vide %lld/%d, terminal un intrus %lld/%d, recus %lld (refus %lld)\n",
           total.descent_runs, min_descent, total.descent_steps, min_steps,
           total.descent_terminal_empty, min_terminal_empty, total.descent_terminal_single,
           min_terminal_single, total.descent_receipts, total.descent_unsupported);
    return 3;
  }
  if (require_regular && !regular) {
    printf("ECHEC : --require-regular exige zero egalite exterieure et zero support ambigu\n");
    return 3;
  }
  // Une campagne qui se DECLARE reguliere ne peut pas contenir de descendant
  // refuse : le refus signifie precisement que le theoreme n'y est pas demontre.
  if (require_regular && (total.descent_unsupported != 0 || total.descent_broken != 0)) {
    printf("ECHEC : --require-regular exige zero refus et zero panne dans la descente"
           " (refus=%lld panne=%lld)\n", total.descent_unsupported, total.descent_broken);
    return 3;
  }
  if (total.disagreements == 0 && failures == 0) {
    printf("OK : source ouverte, univers de facettes, lambda(F) et M(F) tous concordants\n");
    return 0;
  }
  return 1;
}
