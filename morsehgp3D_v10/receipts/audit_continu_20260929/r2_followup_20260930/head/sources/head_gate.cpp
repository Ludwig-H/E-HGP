// Porte de la tete (29-30 septembre 2026) : validation du dendrogramme et des parametres, domaine numerique conjoint,
// juge par definition des etiquettes, regle d'appartenance de la racine d'allow_single_cluster.
//
//  1. validate(d) : fixtures gravees, chacune mutation d'un seul champ d'un dendrogramme valide (accepte), refusees
//     avec leur raison exacte et sans lecture hors bornes (porte jouee aussi sous ASan+UBSan). Constats H1 de
//     l'audit independant : rang de point hors de level, arete parent absente du CSR, niveau NaN ; et voisins :
//     arete dupliquee, parent hors bornes, child_off non monotone, niveaux infini ou negatif.
//  2. validate(ClusterParams) (constat H3) : z non fini, nul, negatif ou > kMaxScaleExponent, mcs = 0, selection
//     inconnue refuses (parameter_out_of_range) ; bornes incluses acceptees. Syntaxe stricte de parse_z.
//  3. Peignes des deux audits (audit continu : n points en echelle, mcs = 1 ; audit independant : deux points par
//     feuille, mcs = 2), EOM et feuilles, avec et sans allow_single : juge par definition. Le compteur
//     Clustering::ancestor_steps n'est qu'imprime (diagnostic) ; la porte causale du cout est mhgp10_head_complexity.
//  4. Juge par definition : premier ancetre retenu par remontee explicite (lui compris), desactivation EOM par les
//     retenus initiaux des ancetres, regle de la racine ecrite comme sklearn _do_labelling ; labels, selected et
//     cluster_label identiques sur des dendrogrammes valides aleatoires (N-aires, chaines, plateaux, composantes
//     sans point, niveau nul au rang 0) et sur les petits peignes. Les cas hors domaine numerique (juge du domaine
//     du 6) doivent etre refuses.
//  5. allow_single_cluster (constat H2) : hierarchies K = 1 de points alignes, etiquettes gravees de scikit-learn
//     1.9.1 (HDBSCAN(min_samples=1, min_cluster_size=3, kd_tree)), egalite de partitions, bruit fixe ; et la
//     divergence volontaire feuilles + allow_single sans scission (head.hpp), gravee a sa valeur v10.
//  6. Domaine numerique conjoint (constat H3 ; CONTRE_AUDIT_TETE_BANCS, CONTRE_AUDIT_TETE_NUMERIQUE_20260929) :
//     les cinq cas de l'audit continu, la sonde de l'audit independant, les cas du verificateur du premier tour,
//     une fixture par regle (1 a 4) et leurs bornes exactes, gravees : refus numeric_domain, sorties vides, ou
//     acceptation avec selection egale au juge analytique et sorties finies. Puis un juge du domaine ecrit par
//     definition (masses par remontee explicite) sur des dendrogrammes aleatoires aux niveaux extremes (nul, sous-
//     normaux, minuscules, enormes) et aux poids jusqu'a 2^32 - 1 : meme decision que cluster() et validate(d, p) ;
//     sur les cas acceptes, sorties finies et etiquettes egales au juge du 4.
//  7. Auto-protection : cluster() et condense() sur un dendrogramme ou des parametres invalides rendent la raison
//     de validate, sans aucune sortie.
// Codes : 0 conforme (ligne head_gate_ok), 1 desaccord d'un juge, 3 plancher de couverture non atteint.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "head/head.hpp"

using namespace mhgp10;

namespace {

int failures = 0;
bool floor_missed = false;

void expect(bool ok, const std::string& what) {
  if (!ok) {
    ++failures;
    if (failures <= 40) std::printf("ECHEC %s\n", what.c_str());
  }
}

void floor_at_least(u64 got, u64 want, const char* what) {
  if (got < want) {
    floor_missed = true;
    std::printf("PLANCHER %s : %llu < %llu\n", what, static_cast<unsigned long long>(got),
                static_cast<unsigned long long>(want));
  }
}

// Egalite de partitions, bruit (-1) fixe, etiquettes a permutation pres.
bool same_partition(const std::vector<i32>& a, const std::vector<i32>& b) {
  if (a.size() != b.size()) return false;
  std::map<i32, i32> ab, ba;
  for (size_t i = 0; i < a.size(); ++i) {
    if ((a[i] < 0) != (b[i] < 0)) return false;
    if (a[i] < 0) continue;
    if (ab.emplace(a[i], b[i]).first->second != b[i]) return false;
    if (ba.emplace(b[i], a[i]).first->second != a[i]) return false;
  }
  return true;
}

// Construit le CSR depuis des listes d'enfants.
void set_children(PointDendrogram& d, const std::vector<std::vector<u32>>& ch) {
  d.child_off.assign(d.nodes() + 1, 0);
  d.child_val.clear();
  for (u32 v = 0; v < d.nodes(); ++v) {
    for (u32 c : ch[v]) d.child_val.push_back(c);
    d.child_off[v + 1] = static_cast<u32>(d.child_val.size());
  }
}

// Sorties vides : refus sans payload (jamais un prefixe).
bool empty_payload(const Clustering& cl) {
  const CondensedTree& t = cl.tree;
  return cl.label.empty() && cl.selected.empty() && cl.cluster_label.empty() && t.parent.empty() &&
         t.birth.empty() && t.stability.empty() && t.mass.empty() && t.point_cluster.empty() &&
         t.point_lambda.empty() && t.node_cluster.empty();
}

// Sorties finies et coherentes d'un cas accepte : naissances, stabilites >= 0, lambdas de sortie >= naissance de
// leur cluster.
bool finite_payload(const Clustering& cl) {
  const CondensedTree& t = cl.tree;
  for (double b : t.birth)
    if (!std::isfinite(b) || b < 0) return false;
  for (double s : t.stability)
    if (!std::isfinite(s) || s < 0) return false;
  for (size_t x = 0; x < t.point_lambda.size(); ++x) {
    const double l = t.point_lambda[x];
    if (!std::isfinite(l)) return false;
    if (t.point_cluster[x] != kNone && l < t.birth[t.point_cluster[x]]) return false;
  }
  return true;
}

// ---------------------------------------------------------------- 1. validate(d)

// Base valide : feuilles 0, 1, 2 (rang 0), noeud 3 = {0, 1} au rang 1, racine 4 = {3, 2} au rang 2 ; cinq points,
// dont un entre au rang de son parent (duree de vie nulle) et un attache a la racine au dernier rang.
PointDendrogram base_dendrogram() {
  PointDendrogram d;
  d.level = {1.0, 4.0, 9.0};
  d.node_rank = {0, 0, 0, 1, 2};
  d.parent = {3, 3, 4, 4, kNone};
  d.child_off = {0, 0, 0, 0, 2, 4};
  d.child_val = {0, 1, 3, 2};
  d.point_node = {0, 1, 2, 3, 4};
  d.point_rank = {0, 1, 0, 2, 2};
  d.point_weight = {1, 1, 1, 1, 2};
  return d;
}

// Fixture minimale de l'audit independant (evidence_head_validate.cpp) : une racine, un point.
PointDendrogram audit_single() {
  PointDendrogram d;
  d.level = {1.0};
  d.node_rank = {0};
  d.child_off = {0, 0};
  d.parent = {kNone};
  d.point_node = {0};
  d.point_rank = {0};
  d.point_weight = {1};
  return d;
}

struct Mutation {
  const char* name;
  bool audit_base;  // true : mutation de audit_single(), sinon de base_dendrogram()
  Reason reason;
  void (*apply)(PointDendrogram&);
};

const Mutation kMutations[] = {
    // H1, fixtures exactes de l'audit independant
    {"audit rang du point hors de level (racine)", true, Reason::csr_bounds,
     [](PointDendrogram& d) { d.point_rank[0] = 1; }},
    {"audit arete parent absente du CSR", true, Reason::csr_bounds,
     [](PointDendrogram& d) {
       d.level = {1.0, 4.0};
       d.node_rank = {0, 1};
       d.parent = {1, kNone};
       d.child_off = {0, 0, 0};
     }},
    {"audit niveau NaN", true, Reason::rank_order,
     [](PointDendrogram& d) { d.level[0] = std::numeric_limits<double>::quiet_NaN(); }},
    // bornes et CSR
    {"rang du point = |level| (racine)", false, Reason::csr_bounds, [](PointDendrogram& d) { d.point_rank[4] = 3; }},
    {"arete parent absente du CSR", false, Reason::csr_bounds,
     [](PointDendrogram& d) {
       d.child_off = {0, 0, 0, 0, 2, 3};
       d.child_val = {0, 1, 3};
     }},
    {"arete dupliquee dans le CSR", false, Reason::csr_bounds,
     [](PointDendrogram& d) {
       d.child_off = {0, 0, 0, 0, 2, 5};
       d.child_val = {0, 1, 3, 2, 3};
     }},
    {"parent hors bornes", false, Reason::csr_bounds, [](PointDendrogram& d) { d.parent[2] = 7; }},
    {"child_off non monotone", false, Reason::csr_bounds,
     [](PointDendrogram& d) { d.child_off = {0, 0, 0, 0, 5, 4}; }},
    {"child_off[0] non nul", false, Reason::csr_bounds, [](PointDendrogram& d) { d.child_off[0] = 1; }},
    {"taille de parent", false, Reason::csr_bounds, [](PointDendrogram& d) { d.parent.pop_back(); }},
    {"taille de point_rank", false, Reason::csr_bounds, [](PointDendrogram& d) { d.point_rank.pop_back(); }},
    {"rang de noeud = |level|", false, Reason::csr_bounds, [](PointDendrogram& d) { d.node_rank[4] = 3; }},
    {"composante d'attache hors bornes", false, Reason::csr_bounds, [](PointDendrogram& d) { d.point_node[0] = 5; }},
    {"poids nul", false, Reason::csr_bounds, [](PointDendrogram& d) { d.point_weight[0] = 0; }},
    {"parent de soi-meme hors CSR", false, Reason::csr_bounds,
     [](PointDendrogram& d) {
       d.parent[2] = 2;
       d.child_off = {0, 0, 0, 0, 2, 3};
       d.child_val = {0, 1, 3};
     }},
    {"deux racines", false, Reason::root_count,
     [](PointDendrogram& d) {
       d.parent[3] = kNone;
       d.child_off = {0, 0, 0, 0, 2, 3};
       d.child_val = {0, 1, 2};
     }},
    // niveaux
    {"niveau NaN", false, Reason::rank_order,
     [](PointDendrogram& d) { d.level[1] = std::numeric_limits<double>::quiet_NaN(); }},
    {"niveau infini", false, Reason::rank_order,
     [](PointDendrogram& d) { d.level[2] = std::numeric_limits<double>::infinity(); }},
    {"niveau negatif", false, Reason::rank_order, [](PointDendrogram& d) { d.level[0] = -1.0; }},
    {"niveaux non strictement croissants", false, Reason::rank_order, [](PointDendrogram& d) { d.level[2] = 4.0; }},
    // ordre
    {"enfant d'indice superieur a son parent", false, Reason::rank_order,
     [](PointDendrogram& d) {
       d.node_rank = {0, 0, 0, 2, 1};
       d.parent = {4, 4, 3, kNone, 3};
       d.child_off = {0, 0, 0, 0, 2, 4};
       d.child_val = {4, 2, 0, 1};
       d.point_node = {0, 1, 2, 4, 3};
     }},
    {"enfant cree apres son parent (rang)", false, Reason::rank_order,
     [](PointDendrogram& d) { d.node_rank[0] = 2; }},
    {"point entre avant sa composante", false, Reason::rank_order, [](PointDendrogram& d) { d.point_rank[3] = 0; }},
    {"point entre apres la fusion de sa composante", false, Reason::rank_order,
     [](PointDendrogram& d) { d.point_rank[0] = 2; }},
};

void test_validate() {
  expect(validate(base_dendrogram()).ok(), "validate : dendrogramme de base refuse");
  expect(validate(audit_single()).ok(), "validate : fixture minimale de l'audit refusee");
  u32 refused = 0;
  for (const Mutation& mu : kMutations) {
    PointDendrogram d = mu.audit_base ? audit_single() : base_dendrogram();
    mu.apply(d);
    const Outcome o = validate(d);
    const bool ok = !o.ok() && o.reason == mu.reason;
    refused += ok;
    expect(ok, std::string("validate(") + mu.name + ") : " + std::string(reason_name(o.reason)) + ", attendu " +
                   std::string(reason_name(mu.reason)));
  }
  std::printf("validate : %u/%zu fixtures refusees avec leur raison\n", refused, std::size(kMutations));
  floor_at_least(std::size(kMutations), 24, "fixtures de validate");
}

// ---------------------------------------------------------------- 2. validate(ClusterParams), parse_z

void test_params() {
  const double inf = std::numeric_limits<double>::infinity();
  auto mk = [](u64 mcs, double z, Selection s) {
    ClusterParams p;
    p.min_cluster_size = mcs;
    p.z = z;
    p.selection = s;
    return p;
  };
  struct Case {
    const char* name;
    ClusterParams p;
    bool ok;
  };
  const Case cases[] = {
      {"defaut", ClusterParams{}, true},
      {"z = 1", mk(5, 1.0, Selection::eom), true},
      {"z = 3, feuilles", mk(5, 3.0, Selection::leaf), true},
      {"z = 1e-3", mk(5, 1e-3, Selection::eom), true},
      {"z = kMaxScaleExponent", mk(5, kMaxScaleExponent, Selection::eom), true},
      {"mcs = 1", mk(1, 1.0, Selection::eom), true},
      {"z = NaN", mk(5, std::numeric_limits<double>::quiet_NaN(), Selection::eom), false},
      {"z = +inf", mk(5, inf, Selection::eom), false},
      {"z = -inf", mk(5, -inf, Selection::eom), false},
      {"z = 0", mk(5, 0.0, Selection::eom), false},
      {"z = -0", mk(5, -0.0, Selection::eom), false},
      {"z = -1", mk(5, -1.0, Selection::eom), false},
      {"z juste au-dessus de la borne", mk(5, std::nextafter(kMaxScaleExponent, inf), Selection::eom), false},
      {"mcs = 0", mk(0, 1.0, Selection::eom), false},
      {"selection inconnue", mk(5, 1.0, static_cast<Selection>(2)), false},
  };
  u32 right = 0;
  for (const Case& c : cases) {
    const Outcome o = validate(c.p);
    const bool ok = c.ok ? o.ok() : (!o.ok() && o.reason == Reason::parameter_out_of_range);
    right += ok;
    expect(ok, std::string("validate(ClusterParams ") + c.name + ") : " + std::string(reason_name(o.reason)));
  }
  std::printf("parametres : %u/%zu cas conformes\n", right, std::size(cases));
  // syntaxe decimale stricte de z : jeton entier, sans signe, sans blanc, sans hexadecimal ni mot
  struct Token {
    const char* text;
    bool ok;
    double value;
  };
  const Token tokens[] = {
      {"1", true, 1.0},         {"16", true, 16.0},     {"2.5", true, 2.5},       {"0.001", true, 0.001},
      {"5.", true, 5.0},        {".5", true, 0.5},      {"1e-05", true, 1e-5},    {"3E+2", true, 300.0},
      {"1e400", true, inf},     {"1e-320", true, 1e-320}, {"", false, 0},         {".", false, 0},
      {"e5", false, 0},         {"1e", false, 0},       {"1e+", false, 0},        {"-1", false, 0},
      {"+1", false, 0},         {" 3", false, 0},       {"3 ", false, 0},         {"0x10", false, 0},
      {"nan", false, 0},        {"inf", false, 0},      {"infinity", false, 0},   {"3abc", false, 0},
      {"1.2.3", false, 0},      {"1e5.0", false, 0},    {"1,5", false, 0},
  };
  u32 good = 0;
  for (const Token& t : tokens) {
    double z = -7.0;
    const bool got = parse_z(t.text, z);
    const bool ok = got == t.ok && (!t.ok || z == t.value) && (t.ok || z == -7.0);
    good += ok;
    expect(ok, std::string("parse_z(\"") + t.text + "\")");
  }
  std::printf("parse_z : %u/%zu jetons conformes\n", good, std::size(tokens));
}

// ---------------------------------------------------------------- 6a. juge du domaine numerique, par definition

enum class Rule { none, zero_merge, zero_alive, lambda_range, mass_ceiling };

const char* rule_name(Rule r) {
  switch (r) {
    case Rule::none: return "dans le domaine";
    case Rule::zero_merge: return "(1) fusion au niveau 0";
    case Rule::zero_alive: return "(2) composante vivante nee a 0";
    case Rule::lambda_range: return "(3) lambda non fini ou non normal";
    case Rule::mass_ceiling: return "(4) M * lambda_max >= 2^1000";
  }
  return "?";
}

// M * lambda_max >= 2^1000, exactement, autrement que le produit : lambda_max = m * 2^(e - 53) (m entier < 2^53) ;
// produit M * m sur deux mots de 64 bits par moities de 32 bits (ecole), puis nombre de bits compare a 1053 - e.
bool at_or_above_ceiling(u64 total, double lmax) {
  if (lmax == 0 || total == 0) return false;
  int e = 0;
  const double f = std::frexp(lmax, &e);
  const u64 m = static_cast<u64>(f * 9007199254740992.0);  // f * 2^53, exact
  const u64 a = total >> 32, b = total & 0xFFFFFFFFu, c = m >> 32, dd = m & 0xFFFFFFFFu;
  const u64 bd = b * dd, ad = a * dd, bc = b * c, ac = a * c;
  const u64 mid = (bd >> 32) + (ad & 0xFFFFFFFFu) + (bc & 0xFFFFFFFFu);
  const u64 lo = (bd & 0xFFFFFFFFu) | (mid << 32);
  const u64 hi = ac + (ad >> 32) + (bc >> 32) + (mid >> 32);
  int bits = 0;
  for (u64 w = hi ? hi : lo; w; w >>= 1) ++bits;
  if (hi) bits += 64;
  return bits > 1053 - e;  // produit >= 2^(1053 - e)
}

// Regles de head.hpp, ecrites autrement que le produit : masse d'un noeud = somme des poids des points dont il est
// un ancetre (remontee explicite), enfants comptes par le tableau parent, lambda recalcule a chaque usage, plafond
// compare exactement par at_or_above_ceiling. Rend la premiere regle violee dans l'ordre 1, 2, 3, 4.
Rule judge_domain(const PointDendrogram& d, const ClusterParams& p, double* lmax_out = nullptr) {
  const u32 n = d.nodes();
  std::vector<u64> mass(n, 0), kids(n, 0);
  u64 total = 0;
  for (u32 x = 0; x < d.points(); ++x) {
    total += d.point_weight[x];
    for (u32 v = d.point_node[x]; v != kNone; v = d.parent[v]) mass[v] += d.point_weight[x];
  }
  for (u32 v = 0; v < n; ++v)
    if (d.parent[v] != kNone) ++kids[d.parent[v]];
  auto level_of = [&](u32 r) { return d.level[r]; };
  for (u32 v = 0; v < n; ++v)
    if (kids[v] >= 2 && level_of(d.node_rank[v]) == 0.0) return Rule::zero_merge;
  auto alive = [&](u32 v) { return d.parent[v] == kNone || mass[v] >= p.min_cluster_size; };
  for (u32 v = 0; v < n; ++v)
    if (alive(v) && level_of(d.node_rank[v]) == 0.0) return Rule::zero_alive;
  std::vector<u32> ranks;
  for (u32 v = 0; v < n; ++v)
    if (alive(v) && kids[v] > 0) ranks.push_back(d.node_rank[v]);
  for (u32 x = 0; x < d.points(); ++x)
    if (alive(d.point_node[x])) ranks.push_back(d.point_rank[x]);
  double lmax = 0;
  for (u32 r : ranks) {
    const double l = std::pow(level_of(r), -p.z / 2);
    if (!(std::isfinite(l) && l >= std::numeric_limits<double>::min())) return Rule::lambda_range;
    lmax = std::max(lmax, l);
  }
  if (lmax_out) *lmax_out = lmax;
  if (at_or_above_ceiling(total, lmax)) return Rule::mass_ceiling;
  return Rule::none;
}

// ---------------------------------------------------------------- 4. juge par definition

struct Judged {
  std::vector<i32> label, cluster_label;
  std::vector<u32> selected;
  bool deactivated = false;  // un retenu initial a ete desactive par un ancetre retenu
  bool root_rule = false;    // regle d'appartenance de la racine appliquee
  bool root_removed = false; // ... et elle a mis au moins un point au bruit
  bool root_kids = false;    // ... sur une racine retenue qui a des enfants (seuil = naissance des enfants)
};

Judged judge(const PointDendrogram& d, const ClusterParams& p, const CondensedTree& t) {
  Judged j;
  const u32 m = static_cast<u32>(t.parent.size());
  std::vector<std::vector<u32>> kids(m);  // par balayage, indices croissants
  for (u32 c = 0; c < m; ++c)
    for (u32 k = 0; k < m; ++k)
      if (t.parent[k] == c) kids[c].push_back(k);
  std::vector<unsigned char> chosen(m, 0);
  if (p.selection == Selection::eom) {
    // R(c) = max(E(c), somme des R des enfants), egalite -> parent, racine exclue sauf allow_single_cluster
    std::vector<double> best(m, 0.0);
    for (u32 c = m; c-- > 0;) {
      double sub = 0;
      for (u32 k : kids[c]) sub += best[k];
      if (kids[c].empty()) {
        best[c] = t.stability[c];
        chosen[c] = 1;
      } else if (t.parent[c] == kNone && !p.allow_single_cluster) {
        best[c] = sub;
      } else if (sub > t.stability[c]) {
        best[c] = sub;
      } else {
        best[c] = t.stability[c];
        chosen[c] = 1;
      }
    }
    // retenu = retenu a son niveau et aucun ancetre strict retenu a son niveau (remontee complete)
    const std::vector<unsigned char> initial = chosen;
    for (u32 c = 0; c < m; ++c) {
      if (!initial[c]) continue;
      for (u32 a = t.parent[c]; a != kNone; a = t.parent[a])
        if (initial[a]) {
          chosen[c] = 0;
          j.deactivated = true;
          break;
        }
    }
  } else {
    for (u32 c = 0; c < m; ++c) chosen[c] = kids[c].empty();
  }
  for (u32 c = 0; c < m; ++c)
    if (t.parent[c] == kNone && !p.allow_single_cluster) chosen[c] = 0;
  std::vector<i32> id(m, -1);
  for (u32 c = 0; c < m; ++c)
    if (chosen[c]) {
      id[c] = static_cast<i32>(j.selected.size());
      j.selected.push_back(c);
    }
  auto first_chosen = [&](u32 c) {
    for (u32 a = c; a != kNone; a = t.parent[a])
      if (chosen[a]) return id[a];
    return -1;
  };
  j.cluster_label.resize(m);
  for (u32 c = 0; c < m; ++c) j.cluster_label[c] = first_chosen(c);
  j.label.assign(d.points(), -1);
  for (u32 x = 0; x < d.points(); ++x)
    if (t.point_cluster[x] != kNone) j.label[x] = first_chosen(t.point_cluster[x]);
  // sklearn _do_labelling (cluster_selection_epsilon = 0) : un seul cluster retenu, la racine, et allow_single ;
  // tous les points se rattachent alors a la racine (aucun autre cluster retenu). Seuil = max des valeurs des lignes
  // de parent racine du condense (points qui en sortent : lambda de sortie ; clusters enfants : lambda de naissance).
  if (p.allow_single_cluster && j.selected.size() == 1 && t.parent[j.selected[0]] == kNone) {
    const u32 root = j.selected[0];
    double threshold = -std::numeric_limits<double>::infinity();
    for (u32 x = 0; x < d.points(); ++x)
      if (t.point_cluster[x] == root) threshold = std::max(threshold, t.point_lambda[x]);
    for (u32 c = 0; c < m; ++c)
      if (t.parent[c] == root) {
        threshold = std::max(threshold, t.birth[c]);
        j.root_kids = true;
      }
    j.root_rule = true;
    for (u32 x = 0; x < d.points(); ++x) {
      const i32 lab = t.point_lambda[x] >= threshold ? id[root] : -1;
      j.root_removed |= lab < 0;
      j.label[x] = lab;
    }
  }
  return j;
}

struct JudgeStats {
  u64 cases = 0, deep = 0, deactivated = 0, root_rule = 0, root_removed = 0, root_kids = 0;
  u64 refused[5] = {0, 0, 0, 0, 0};  // par regle du juge du domaine
};

// Un cas : decision du juge du domaine ; refus -> numeric_domain et sorties vides (cluster et validate(d, p)) ;
// acceptation -> sorties finies et etiquettes egales au juge par definition.
bool check_against_judge(const PointDendrogram& d, const ClusterParams& p, JudgeStats& s, const std::string& what) {
  const Clustering cl = cluster(d, p);
  const Rule rule = judge_domain(d, p);
  const Outcome vo = validate(d, p);
  if (rule != Rule::none) {
    const bool ok = cl.outcome.reason == Reason::numeric_domain && vo.reason == Reason::numeric_domain &&
                    empty_payload(cl);
    expect(ok, "domaine : " + what + " : " + rule_name(rule) + " attendu, rendu " +
                   std::string(reason_name(cl.outcome.reason)) + " / " + std::string(reason_name(vo.reason)));
    ++s.refused[static_cast<int>(rule)];
    return ok;
  }
  if (!cl.outcome.ok() || !vo.ok()) {
    expect(false, "domaine : " + what + " : accepte par le juge, refuse " + std::string(reason_name(cl.outcome.reason)) +
                      " / " + std::string(reason_name(vo.reason)));
    return false;
  }
  const Judged j = judge(d, p, cl.tree);
  const bool ok = finite_payload(cl) && cl.label == j.label && cl.selected == j.selected &&
                  cl.cluster_label == j.cluster_label;
  expect(ok, "juge par definition : " + what);
  ++s.cases;
  s.deep += cl.tree.parent.size() >= 5;
  s.deactivated += j.deactivated;
  s.root_rule += j.root_rule;
  s.root_removed += j.root_removed;
  s.root_kids += j.root_kids && j.root_removed;
  return ok;
}

// Dendrogramme valide aleatoire : feuilles, fusions N-aires (1 a 4 enfants : chaines a un enfant comprises),
// plateaux (fusion au rang de son enfant le plus haut), composantes sans point, points attaches dans l'intervalle
// de vie de leur composante, poids 1 a 3, niveau nul possible au rang 0 (jamais pour une fusion).
PointDendrogram random_dendrogram(std::mt19937_64& g) {
  auto uni = [&](u32 n) { return static_cast<u32>(g() % n); };
  PointDendrogram d;
  const u32 leaves = 1 + uni(14);
  std::vector<std::vector<u32>> ch;
  std::vector<u32> roots;
  for (u32 i = 0; i < leaves; ++i) {
    d.node_rank.push_back(uni(3));
    d.parent.push_back(kNone);
    ch.emplace_back();
    roots.push_back(i);
  }
  u32 chains = uni(3);
  while (roots.size() > 1 || (chains > 0 && uni(2) == 0)) {
    u32 k = 1 + uni(std::min<u32>(4, static_cast<u32>(roots.size())));
    if (k == 1 && roots.size() > 1 && (chains == 0 || uni(3) != 0)) k = 2;
    if (k == 1) --chains;
    for (u32 i = 0; i < k; ++i) std::swap(roots[i], roots[i + uni(static_cast<u32>(roots.size()) - i)]);
    const u32 v = d.nodes();
    u32 r = 0;
    for (u32 i = 0; i < k; ++i) r = std::max(r, d.node_rank[roots[i]]);
    r = std::max<u32>(1, r + (uni(3) == 0 ? 0 : 1 + uni(2)));
    d.node_rank.push_back(r);
    d.parent.push_back(kNone);
    ch.emplace_back();
    for (u32 i = 0; i < k; ++i) {
      ch[v].push_back(roots[i]);
      d.parent[roots[i]] = v;
    }
    roots.erase(roots.begin(), roots.begin() + k);
    roots.push_back(v);
  }
  set_children(d, ch);
  u32 top = 0;
  for (u32 r : d.node_rank) top = std::max(top, r);
  const u32 nl = top + 1 + uni(3);
  d.level.resize(nl);
  d.level[0] = uni(3) == 0 ? 0.0 : 0.25 * (1 + uni(4));
  for (u32 i = 1; i < nl; ++i) d.level[i] = d.level[i - 1] + 0.25 * (1 + uni(8));
  const u32 pts = uni(2 * leaves + 4);
  for (u32 x = 0; x < pts; ++x) {
    const u32 v = uni(d.nodes());
    const u32 lo = d.node_rank[v], hi = d.parent[v] == kNone ? nl - 1 : d.node_rank[d.parent[v]];
    d.point_node.push_back(v);
    d.point_rank.push_back(lo + uni(hi - lo + 1));
    d.point_weight.push_back(1 + uni(3));
  }
  return d;
}

void test_judge_random() {
  std::mt19937_64 g(20260929);
  JudgeStats s;
  const u64 mcs_set[] = {1, 2, 3, 4, 6};
  const double z_set[] = {0.5, 1.0, 2.0, 3.0};
  u32 invalid = 0;
  for (u32 it = 0; it < 7000; ++it) {
    const PointDendrogram d = random_dendrogram(g);
    if (!validate(d).ok()) {
      ++invalid;
      continue;
    }
    for (int sel = 0; sel < 2; ++sel)
      for (int single = 0; single < 2; ++single) {
        ClusterParams p;
        p.min_cluster_size = mcs_set[g() % 5];
        p.z = z_set[g() % 4];
        p.selection = sel ? Selection::leaf : Selection::eom;
        p.allow_single_cluster = single != 0;
        check_against_judge(d, p, s, "aleatoire " + std::to_string(it));
      }
  }
  expect(invalid == 0, "generateur : " + std::to_string(invalid) + " dendrogrammes aleatoires refuses par validate");
  std::printf("juge aleatoire : %llu cas acceptes, %llu a >= 5 clusters, %llu desactivations EOM, regle de la racine "
              "%llu (bruit %llu, racine avec enfants %llu) ; refus du domaine (2) %llu\n",
              static_cast<unsigned long long>(s.cases), static_cast<unsigned long long>(s.deep),
              static_cast<unsigned long long>(s.deactivated), static_cast<unsigned long long>(s.root_rule),
              static_cast<unsigned long long>(s.root_removed), static_cast<unsigned long long>(s.root_kids),
              static_cast<unsigned long long>(s.refused[2]));
  floor_at_least(s.cases, 20000, "cas acceptes du juge aleatoire");
  floor_at_least(s.deep, 5000, "cas a >= 5 clusters");
  floor_at_least(s.deactivated, 500, "desactivations EOM");
  floor_at_least(s.root_rule, 500, "applications de la regle de la racine");
  floor_at_least(s.root_removed, 200, "regle de la racine mettant des points au bruit");
  floor_at_least(s.root_kids, 20, "regle de la racine, racine retenue avec enfants");
  floor_at_least(s.refused[2], 500, "refus (2) du domaine (niveau 0 vivant)");
}

// ---------------------------------------------------------------- 3. peignes des audits

// Audit continu (head_ladder.cpp) : n points ; feuille x = point x (rang 0) ; noeud n fusionne les feuilles 0 et 1
// au rang 1, noeud v > n fusionne v - 1 et la feuille v - n + 1 au rang v - n + 1.
PointDendrogram ladder(u32 n) {
  PointDendrogram d;
  const u32 nn = 2 * n - 1;
  d.level.resize(n);
  for (u32 r = 0; r < n; ++r) d.level[r] = r + 1;
  d.node_rank.assign(nn, 0);
  d.parent.assign(nn, kNone);
  d.point_node.resize(n);
  d.point_rank.assign(n, 0);
  d.point_weight.assign(n, 1);
  for (u32 x = 0; x < n; ++x) d.point_node[x] = x;
  std::vector<std::vector<u32>> ch(nn);
  for (u32 v = n; v < nn; ++v) {
    const u32 a = v == n ? 0 : v - 1, b = v - n + 1;
    d.node_rank[v] = v - n + 1;
    d.parent[a] = d.parent[b] = v;
    ch[v] = {a, b};
  }
  set_children(d, ch);
  return d;
}

// Audit independant (evidence_head_complexity.cpp) : q + 1 feuilles de deux points (rang 0) ; noeud interne i
// fusionne le precedent (ou la feuille 0) et la feuille i + 1 au rang i + 1.
PointDendrogram comb(u32 q) {
  PointDendrogram d;
  const u32 n = 2 * q + 1;
  d.node_rank.assign(n, 0);
  d.parent.assign(n, kNone);
  d.level.push_back(1.0);
  for (u32 i = 1; i <= q; ++i) d.level.push_back(double(i + 1));
  for (u32 leaf = 0; leaf <= q; ++leaf)
    for (u32 j = 0; j < 2; ++j) {
      d.point_node.push_back(leaf);
      d.point_rank.push_back(0);
      d.point_weight.push_back(1);
    }
  std::vector<std::vector<u32>> ch(n);
  for (u32 v = q + 1; v < n; ++v) {
    const u32 i = v - q - 1, left = i == 0 ? 0 : v - 1, right = i + 1;
    d.node_rank[v] = i + 1;
    ch[v] = {left, right};
    d.parent[left] = d.parent[right] = v;
  }
  set_children(d, ch);
  return d;
}

void test_combs() {
  struct Family {
    const char* name;
    PointDendrogram (*make)(u32);
    u32 size;
    u64 mcs;
  };
  const Family families[] = {{"echelle de l'audit continu", ladder, 2000, 1},
                             {"peigne de l'audit independant", comb, 1024, 2}};
  JudgeStats s;
  u32 runs = 0;
  for (const Family& f : families) {
    const PointDendrogram d = f.make(f.size);
    expect(validate(d).ok(), std::string(f.name) + " : peigne refuse par validate");
    for (int sel = 0; sel < 2; ++sel)
      for (int single = 0; single < 2; ++single) {
        ClusterParams p;
        p.min_cluster_size = f.mcs;
        p.selection = sel ? Selection::leaf : Selection::eom;
        p.allow_single_cluster = single != 0;
        const std::string tag = std::string(f.name) + (sel ? " feuilles" : " eom") + (single ? " allow_single" : "");
        const Clustering cl = cluster(d, p);
        // le peigne est bien profond : une chaine de clusters de longueur ~ la taille
        expect(cl.tree.parent.size() >= f.size, tag + " : condense trop petit pour juger la profondeur");
        // diagnostic seulement (compteur tenu par le code sous test) : la porte du cout est mhgp10_head_complexity
        std::printf("%s : m = %zu clusters, pas vers le parent (diagnostic) %llu\n", tag.c_str(),
                    cl.tree.parent.size(), static_cast<unsigned long long>(cl.ancestor_steps));
        check_against_judge(d, p, s, tag);
        ++runs;
      }
  }
  floor_at_least(runs, 8, "configurations de peigne");
}

// ---------------------------------------------------------------- 5. allow_single_cluster

// Hierarchie K = 1 de sites alignes (abscisses croissantes) : feuille i = site i ne au niveau 0, fusions donnees
// comme (rang, enfants) ; niveaux = carres des demi-distances de fusion.
PointDendrogram line_k1(u32 sites, const std::vector<double>& levels,
                        const std::vector<std::pair<u32, std::vector<u32>>>& merges) {
  PointDendrogram d;
  d.level = levels;
  std::vector<std::vector<u32>> ch(sites);
  for (u32 s = 0; s < sites; ++s) {
    d.node_rank.push_back(0);
    d.parent.push_back(kNone);
    d.point_node.push_back(s);
    d.point_rank.push_back(0);
    d.point_weight.push_back(1);
  }
  for (const auto& [rank, kids] : merges) {
    const u32 v = d.nodes();
    d.node_rank.push_back(rank);
    d.parent.push_back(kNone);
    ch.push_back(kids);
    for (u32 c : kids) d.parent[c] = v;
  }
  set_children(d, ch);
  return d;
}

void test_allow_single() {
  // x = 0..7, 50, 100 : {0..7} a (1/2)^2, + 50 a (43/2)^2, + 100 a (50/2)^2 (fixture de l'audit independant, H2)
  const PointDendrogram line10 =
      line_k1(10, {0.0, 0.25, 462.25, 625.0}, {{1, {0, 1, 2, 3, 4, 5, 6, 7}}, {2, {10, 8}}, {3, {11, 9}}});
  // x = 0..3, 1000..1003 : deux groupes, racine a (997/2)^2
  const PointDendrogram two =
      line_k1(8, {0.0, 0.25, 248502.25}, {{1, {0, 1, 2, 3}}, {1, {4, 5, 6, 7}}, {2, {8, 9}}});
  // x = 0, 1, 2, 4, 5, 6, 20 : deux groupes a (1/2)^2, fusion a 1, + 20 a (14/2)^2 ; la racine l'emporte en EOM
  const PointDendrogram kids =
      line_k1(7, {0.0, 0.25, 1.0, 49.0}, {{1, {0, 1, 2}}, {1, {3, 4, 5}}, {2, {7, 8}}, {3, {9, 6}}});
  struct Case {
    const char* name;
    const PointDendrogram* d;
    Selection sel;
    bool single;
    std::vector<i32> expected;
  };
  const Selection eom = Selection::eom, leaf = Selection::leaf;
  const std::vector<i32> noise10(10, -1);
  // etiquettes de scikit-learn 1.9.1
  const Case cases[] = {
      {"ligne10 eom allow_single", &line10, eom, true, {0, 0, 0, 0, 0, 0, 0, 0, -1, -1}},
      {"ligne10 eom racine exclue", &line10, eom, false, noise10},
      {"ligne10 feuilles racine exclue", &line10, leaf, false, noise10},
      {"deux groupes eom allow_single", &two, eom, true, {0, 0, 0, 0, 1, 1, 1, 1}},
      {"deux groupes eom", &two, eom, false, {0, 0, 0, 0, 1, 1, 1, 1}},
      {"deux groupes feuilles allow_single", &two, leaf, true, {0, 0, 0, 0, 1, 1, 1, 1}},
      {"racine avec enfants eom allow_single", &kids, eom, true, {0, 0, 0, 0, 0, 0, -1}},
      {"racine avec enfants eom", &kids, eom, false, {0, 0, 0, 1, 1, 1, -1}},
      {"racine avec enfants feuilles allow_single", &kids, leaf, true, {0, 0, 0, 1, 1, 1, -1}},
  };
  // divergence volontaire (head.hpp) : feuilles + allow_single sans scission, la racine est la seule feuille et est
  // retenue avec la regle d'appartenance ; scikit-learn 1.9.1 rend dix -1 (sa racine est ecrasee dans _get_clusters)
  const Case divergences[] = {
      {"ligne10 feuilles allow_single (v10, sklearn : dix -1)", &line10, leaf, true, {0, 0, 0, 0, 0, 0, 0, 0, -1, -1}},
  };
  u32 right = 0;
  for (const Case& c : cases) {
    expect(validate(*c.d).ok(), std::string(c.name) + " : fixture refusee par validate");
    ClusterParams p;
    p.min_cluster_size = 3;
    p.selection = c.sel;
    p.allow_single_cluster = c.single;
    const Clustering cl = cluster(*c.d, p);
    const bool ok = same_partition(cl.label, c.expected);
    right += ok;
    std::string got;
    for (i32 v : cl.label) got += std::to_string(v) + " ";
    expect(ok, std::string(c.name) + " : etiquettes " + got + "differentes de sklearn 1.9.1");
  }
  u32 kept = 0;
  for (const Case& c : divergences) {
    ClusterParams p;
    p.min_cluster_size = 3;
    p.selection = c.sel;
    p.allow_single_cluster = c.single;
    const Clustering cl = cluster(*c.d, p);
    const bool ok = same_partition(cl.label, c.expected) && cl.selected == std::vector<u32>{0};
    kept += ok;
    std::string got;
    for (i32 v : cl.label) got += std::to_string(v) + " ";
    expect(ok, std::string(c.name) + " : etiquettes " + got + "differentes de la semantique v10");
  }
  std::printf("allow_single_cluster : %u/%zu fixtures egales a sklearn 1.9.1, %u/%zu divergences gravees tenues\n",
              right, std::size(cases), kept, std::size(divergences));
}

// ---------------------------------------------------------------- 6b. domaine numerique : fixtures gravees

// Deux feuilles d'un point de poids w chacune, fusionnees par la racine (audit continu, joint_head_probe.cpp) ;
// zero_merge : la racine au rang 0 (fusion au niveau 0).
PointDendrogram two_leaves(double leaf_level, double merge_level, u32 w, bool zero_merge = false) {
  PointDendrogram d;
  d.level = {leaf_level, merge_level};
  d.node_rank = {0, 0, zero_merge ? 0u : 1u};
  d.parent = {2, 2, kNone};
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.point_node = {0, 1};
  d.point_rank = {0, 0};
  d.point_weight = {w, w};
  return d;
}

// Sonde de l'audit independant (positive_levels.cpp) : deux feuilles de deux points, niveaux {1e-300, 2e-300}.
PointDendrogram positive_levels() {
  PointDendrogram d;
  d.level = {1e-300, 2e-300};
  d.node_rank = {0, 0, 1};
  d.parent = {2, 2, kNone};
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.point_node = {0, 0, 1, 1};
  d.point_rank = {0, 0, 0, 0};
  d.point_weight = {1, 1, 1, 1};
  return d;
}

// Une seule composante (racine) au rang 0 et ses points, tous entres au rang 0.
PointDendrogram single_root(double level, std::vector<u32> weights) {
  PointDendrogram d;
  d.level = {level};
  d.node_rank = {0};
  d.parent = {kNone};
  d.child_off = {0, 0};
  for (u32 w : weights) {
    d.point_node.push_back(0);
    d.point_rank.push_back(0);
    d.point_weight.push_back(w);
  }
  return d;
}

// Regle (1) seule : feuilles a, b nees au niveau 0 (un point chacune) fusionnees au niveau 0 en v, legere
// (masse 2 < mcs = 3) ; feuille c (trois points) au niveau 1 ; racine {v, c} au niveau 4. Sans (1), le calcul
// serait fini (v sort au lambda de la racine) : le refus est une decision de domaine, pas une consequence de (2).
PointDendrogram light_zero_merge() {
  PointDendrogram d;
  d.level = {0.0, 1.0, 4.0};
  d.node_rank = {0, 0, 0, 1, 2};
  d.parent = {2, 2, 4, 4, kNone};
  d.child_off = {0, 0, 0, 2, 2, 4};
  d.child_val = {0, 1, 2, 3};
  d.point_node = {0, 1, 3, 3, 3};
  d.point_rank = {0, 0, 1, 1, 1};
  d.point_weight = {1, 1, 1, 1, 1};
  return d;
}

// Rang consomme ou non : racine (rang 2, niveau 4) de trois feuilles ; A (rang 0, niveau 1e-300, un point de
// poids 1) est legere et sort au lambda de la racine ; B et C (rang 1, niveau 1, un point de poids w chacune) sont
// vivantes. Le niveau 1e-300 n'est pas consomme : accepte malgre M * lambda(1e-300) > 2^1000.
PointDendrogram unused_tiny_level(u32 w) {
  PointDendrogram d;
  d.level = {1e-300, 1.0, 4.0};
  d.node_rank = {0, 1, 1, 2};
  d.parent = {3, 3, 3, kNone};
  d.child_off = {0, 0, 0, 0, 3};
  d.child_val = {0, 1, 2};
  d.point_node = {0, 1, 2};
  d.point_rank = {0, 1, 1};
  d.point_weight = {1, w, w};
  return d;
}

// Masse portee par des sous-arbres legers : deux feuilles (un point de poids w chacune, legeres pour mcs > w)
// fusionnees par la racine ; aucun point attache a une composante vivante.
PointDendrogram light_mass(double leaf_level, double root_level, u32 w) { return two_leaves(leaf_level, root_level, w); }

struct Fixture {
  const char* name;
  PointDendrogram d;
  u64 mcs;
  double z;
  Selection sel;
  bool single;
  Rule rule;                   // regle attendue (none : accepte)
  std::vector<u32> selected;   // accepte : clusters retenus par le juge analytique (indices du condense)
  std::vector<i32> labels;     // accepte : etiquettes (partition, bruit fixe)
};

void test_numeric_domain_fixtures() {
  const u32 wmax = std::numeric_limits<u32>::max();
  const Selection eom = Selection::eom, leaf = Selection::leaf;
  std::vector<Fixture> fx;
  // ---- audit continu (CONTRE_AUDIT_TETE_NUMERIQUE_20260929, joint_head_probe.cpp) : deux feuilles de masse w,
  // racine autorisee, mcs = w. Juge analytique : R = 2 w lambda_fusion, S = 2 w (lambda_feuille - lambda_fusion) ;
  // les enfants l'emportent ssi L_fusion > 4 L_feuille (z = 1) ou L_fusion > 2 L_feuille (z = 2).
  // 0,75 < 4 * 0,25 : la racine, retenue, garde ses deux points (seuil lambda(0,75) <= lambda(0,25)).
  fx.push_back({"audit continu u18_range_z1", two_leaves(0.25, 0.75, wmax), wmax, 1.0, eom, true, Rule::none, {0},
                {0, 0}});
  // 0,75 > 2 * 0,25 : les deux feuilles.
  fx.push_back({"audit continu u18_range_z2", two_leaves(0.25, 0.75, wmax), wmax, 2.0, eom, true, Rule::none, {1, 2},
                {0, 1}});
  // meme arbre en selection par feuilles : les deux feuilles du condense, quel que soit z.
  fx.push_back({"audit continu u18_range_z1, feuilles", two_leaves(0.25, 0.75, wmax), wmax, 1.0, leaf, true,
                Rule::none, {1, 2}, {0, 1}});
  // lambda = 1e300 et 1,1e299, M = 2 : M * lambda_max = 2e300 < 2^1000 ; 9e-300 > 2e-300 : les deux feuilles.
  fx.push_back({"audit continu finite_lambda_unweighted_z2", two_leaves(1e-300, 9e-300, 1), 1, 2.0, eom, true,
                Rule::none, {1, 2}, {0, 1}});
  // meme arbre, w = 2^32 - 1 : M * lambda_max ~ 8,6e309 >= 2^1000 (le produit r1 debordait et rendait la racine).
  fx.push_back({"audit continu finite_lambda_weighted_z2", two_leaves(1e-300, 9e-300, wmax), wmax, 2.0, eom, true,
                Rule::mass_ceiling, {}, {}});
  // fusion au rang 0 (niveau 0), poids 2, mcs = 2 : NaN par inf - inf en r1.
  fx.push_back({"audit continu zero_merge_z1", two_leaves(0.0, 0.25, 2, true), 2, 1.0, eom, true, Rule::zero_merge, {},
                {}});
  // ---- audit independant (CONTRE_AUDIT_TETE_BANCS, positive_levels.cpp) : z = 16, lambda(1e-300) = 1e2400 = inf.
  fx.push_back({"audit independant positive_levels", positive_levels(), 2, 16.0, eom, true, Rule::lambda_range, {},
                {}});
  // ---- verificateur du premier tour (h1_edges.cpp) : paire de base, mcs = 1
  fx.push_back({"verificateur level0_split_z1", two_leaves(0.0, 0.0, 1, true), 1, 1.0, eom, false, Rule::zero_merge,
                {}, {}});
  fx.back().d.level = {0.0};
  fx.push_back({"verificateur tiny_levels_z16", two_leaves(1e-300, 2e-300, 1), 1, 16.0, eom, false, Rule::lambda_range,
                {}, {}});
  fx.push_back({"verificateur subnormal_levels_z2", two_leaves(5e-324, 1e-323, 1), 1, 2.0, eom, false,
                Rule::lambda_range, {}, {}});
  fx.push_back({"verificateur huge_levels_z16", two_leaves(1e300, 2e300, 1), 1, 16.0, eom, false, Rule::lambda_range,
                {}, {}});
  // niveau -0 : traite comme 0 ; feuilles vivantes (mcs = 1) nees a 0.
  fx.push_back({"verificateur minus_zero_level", two_leaves(-0.0, 4.0, 1), 1, 1.0, eom, false, Rule::zero_alive, {},
                {}});
  // ---- une fixture par regle et ses bornes
  fx.push_back({"(1) fusion legere au niveau 0", light_zero_merge(), 3, 1.0, eom, false, Rule::zero_merge, {}, {}});
  // (2) feuilles nees a 0 et vivantes (masse 1 = mcs), fusion positive : K = 1 et mcs = 1.
  fx.push_back({"(2) feuilles vivantes nees a 0 (K = 1, mcs = 1)", two_leaves(0.0, 1.0, 1), 1, 1.0, eom, false,
                Rule::zero_alive, {}, {}});
  // (2) borne : masse = mcs - 1, feuilles legeres, leurs points sortent a lambda(1) = 1 dans la racine.
  fx.push_back({"(2) feuilles legeres nees a 0 (mcs = 2)", two_leaves(0.0, 1.0, 1), 2, 1.0, eom, true, Rule::none,
                {0}, {0, 0}});
  // (2) racine nee a 0 : toujours vivante, quelle que soit sa masse (ici 1 < mcs = 5).
  fx.push_back({"(2) racine nee a 0, legere", single_root(0.0, {1}), 5, 1.0, eom, true, Rule::zero_alive, {}, {}});
  // (3) borne basse : z = 2, lambda = L^-1 ; L = 2^1022 -> 2^-1022 (normal) accepte ; 2^1023 -> sous-normal refuse.
  fx.push_back({"(3) lambda = 2^-1022 (normal)", single_root(std::ldexp(1.0, 1022), {1}), 1, 2.0, eom, true,
                Rule::none, {0}, {0}});
  fx.push_back({"(3) lambda = 2^-1023 (sous-normal)", single_root(std::ldexp(1.0, 1023), {1}), 1, 2.0, eom, true,
                Rule::lambda_range, {}, {}});
  // (3) borne haute sans point : racine de deux enfants vides, niveau 1e-300, z = 16.
  {
    PointDendrogram d = two_leaves(1e-300, 2e-300, 1);
    d.point_node.clear();
    d.point_rank.clear();
    d.point_weight.clear();
    fx.push_back({"(3) lambda infini sans aucun point", d, 1, 16.0, eom, false, Rule::lambda_range, {}, {}});
  }
  // (4) bornes exactes : un point de poids 1, L = 2^-1000 (z = 2) -> M * lambda = 2^1000 refuse ; 2^-999 accepte.
  fx.push_back({"(4) M * lambda = 2^1000", single_root(std::ldexp(1.0, -1000), {1}), 1, 2.0, eom, true,
                Rule::mass_ceiling, {}, {}});
  fx.push_back({"(4) M * lambda = 2^999", single_root(std::ldexp(1.0, -999), {1}), 1, 2.0, eom, true, Rule::none,
                {0}, {0}});
  // (4) masse ponderee : deux points de poids 2^31 (M = 2^32) ; lambda = 2^968 refuse, 2^967 accepte.
  fx.push_back({"(4) M = 2^32, lambda = 2^968", single_root(std::ldexp(1.0, -968), {1u << 31, 1u << 31}), 1, 2.0, eom,
                true, Rule::mass_ceiling, {}, {}});
  fx.push_back({"(4) M = 2^32, lambda = 2^967", single_root(std::ldexp(1.0, -967), {1u << 31, 1u << 31}), 1, 2.0, eom,
                true, Rule::none, {0}, {0, 0}});
  // (4) comparaison exacte : lambda = 1 / (9 * 2^-1000) arrondi par defaut (0x1.c71c71c71c71cp+996), M = 9 :
  // M * lambda < 2^1000 exactement, alors que le produit flottant 9 * lambda s'arrondit a 2^1000 (juge aleatoire).
  fx.push_back({"(4) M = 9, lambda = arrondi(2^1000 / 9)", single_root(9 * std::ldexp(1.0, -1000), {9}), 1, 2.0, eom,
                true, Rule::none, {0}, {0}});
  // (4) masse portee par des feuilles legeres (poids 2^32 - 2, mcs = 2^32 - 1) : M = 2^33 - 4 ; seule la racine est
  // vivante, lambda(racine) = 2^968 -> M * lambda ~ 2^1001 refuse (une masse des seules composantes vivantes, le
  // nombre de points ou le poids maximal passeraient sous 2^1000).
  fx.push_back({"(4) masse des sous-arbres legers",
                light_mass(std::ldexp(1.0, -969), std::ldexp(1.0, -968), wmax - 1), wmax, 2.0, eom, true,
                Rule::mass_ceiling, {}, {}});
  // rang non consomme (z = 2 : lambda(1e-300) = 1e300, M * 1e300 > 2^1000) : accepte ; EOM racine exclue : B et C
  // retenues, le point de A (sorti de la racine) au bruit.
  fx.push_back({"rang minuscule non consomme", unused_tiny_level(wmax), wmax, 2.0, eom, false, Rule::none, {1, 2},
                {-1, 0, 1}});

  u32 right = 0, refused = 0, accepted = 0;
  for (const Fixture& f : fx) {
    ClusterParams p;
    p.min_cluster_size = f.mcs;
    p.z = f.z;
    p.selection = f.sel;
    p.allow_single_cluster = f.single;
    bool ok = validate(f.d).ok() && validate(p).ok();
    const Rule judged = judge_domain(f.d, p);
    const Clustering cl = cluster(f.d, p);
    const Outcome vo = validate(f.d, p);
    const CondensedTree ct = condense(f.d, p);
    ok = ok && judged == f.rule;
    if (f.rule != Rule::none) {
      ok = ok && cl.outcome.reason == Reason::numeric_domain && vo.reason == Reason::numeric_domain &&
           ct.outcome.reason == Reason::numeric_domain && cl.outcome.status() == Status::unsupported_degeneracy &&
           empty_payload(cl) && ct.parent.empty() && ct.point_lambda.empty();
      refused += ok;
    } else {
      ok = ok && cl.outcome.ok() && vo.ok() && ct.outcome.ok() && finite_payload(cl) && cl.selected == f.selected &&
           same_partition(cl.label, f.labels);
      accepted += ok;
    }
    right += ok;
    std::string got;
    for (i32 v : cl.label) got += std::to_string(v) + " ";
    expect(ok, std::string("domaine : ") + f.name + " : juge " + rule_name(judged) + ", attendu " + rule_name(f.rule) +
                   ", cluster " + std::string(reason_name(cl.outcome.reason)) + ", validate(d, p) " +
                   std::string(reason_name(vo.reason)) + ", etiquettes " + got);
  }
  std::printf("domaine numerique : %u/%zu fixtures gravees conformes (%u refus, %u acceptations au juge analytique)\n",
              right, fx.size(), refused, accepted);
  floor_at_least(fx.size(), 24, "fixtures du domaine numerique");
}

// ---------------------------------------------------------------- 6c. domaine numerique : juge aleatoire

// Dendrogramme aleatoire aux valeurs extremes : meme forme que random_dendrogram, fusions possibles au rang 0,
// niveaux base * (i + 1) sur une base tiree parmi normale, minuscule, sous-normale, enorme (eventuellement 0 au
// rang 0), poids 1 a 3 ou proches de 2^31 et de 2^32 - 1.
PointDendrogram extreme_dendrogram(std::mt19937_64& g) {
  auto uni = [&](u32 n) { return static_cast<u32>(g() % n); };
  PointDendrogram d = random_dendrogram(g);
  if (uni(4) == 0)  // fusions au rang 0 : une fusion dont tous les enfants sont nes au rang 0 y descend (enfants
                    // d'indice inferieur : un seul balayage suffit)
    for (u32 v = 0; v < d.nodes(); ++v) {
      if (d.child_off[v] == d.child_off[v + 1]) continue;
      bool low = true;
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) low = low && d.node_rank[d.child_val[j]] == 0;
      if (low) d.node_rank[v] = 0;
    }
  // attaches ramenees dans l'intervalle de vie de leur composante
  for (u32 x = 0; x < d.points(); ++x) {
    const u32 v = d.point_node[x];
    const u32 lo = d.node_rank[v];
    const u32 hi = d.parent[v] == kNone ? static_cast<u32>(d.level.size() - 1) : d.node_rank[d.parent[v]];
    d.point_rank[x] = std::min(std::max(d.point_rank[x], lo), hi);
  }
  const double bases[] = {0.25, 1.0, 1e-300, 5e-324, 1e-160, 1e150, 1e300, std::ldexp(1.0, -1000),
                          std::ldexp(1.0, 1000)};
  const double base = bases[uni(std::size(bases))];
  const bool zero = uni(3) == 0;
  for (u32 i = 0; i < d.level.size(); ++i) d.level[i] = base * (i + 1);
  if (zero) d.level[0] = 0.0;
  const u32 weights[] = {1, 2, 3, (1u << 31) - 1, 1u << 31, std::numeric_limits<u32>::max()};
  const bool heavy = uni(3) == 0;
  for (u32& w : d.point_weight) w = heavy ? weights[uni(std::size(weights))] : 1 + uni(3);
  return d;
}

void test_numeric_domain_random() {
  std::mt19937_64 g(20260930);
  JudgeStats s;
  const double z_set[] = {0.5, 1.0, 2.0, 3.0, 16.0};
  u32 invalid = 0, big_lambda = 0;
  for (u32 it = 0; it < 6000; ++it) {
    const PointDendrogram d = extreme_dendrogram(g);
    if (!validate(d).ok()) {
      ++invalid;
      continue;
    }
    u64 total = 0;
    for (u32 w : d.point_weight) total += w;
    for (int sel = 0; sel < 2; ++sel)
      for (int single = 0; single < 2; ++single) {
        ClusterParams p;
        const u64 mcs_set[] = {1, 2, 3, u64(1) << 31, total > 1 ? total - 1 : 1, total + 1};
        p.min_cluster_size = mcs_set[g() % std::size(mcs_set)];
        p.z = z_set[g() % std::size(z_set)];
        p.selection = sel ? Selection::leaf : Selection::eom;
        p.allow_single_cluster = single != 0;
        double lmax = 0;
        if (judge_domain(d, p, &lmax) == Rule::none && lmax > 1e200) ++big_lambda;
        check_against_judge(d, p, s, "extreme " + std::to_string(it));
      }
  }
  expect(invalid == 0, "generateur extreme : " + std::to_string(invalid) + " dendrogrammes refuses par validate");
  std::printf("domaine aleatoire : %llu cas acceptes (dont %u a lambda_max > 1e200), refus (1) %llu, (2) %llu, "
              "(3) %llu, (4) %llu\n",
              static_cast<unsigned long long>(s.cases), big_lambda, static_cast<unsigned long long>(s.refused[1]),
              static_cast<unsigned long long>(s.refused[2]), static_cast<unsigned long long>(s.refused[3]),
              static_cast<unsigned long long>(s.refused[4]));
  floor_at_least(s.cases, 5000, "cas acceptes du juge du domaine");
  floor_at_least(big_lambda, 200, "cas acceptes a lambda_max > 1e200");
  for (int r = 1; r <= 4; ++r) floor_at_least(s.refused[r], 200, rule_name(static_cast<Rule>(r)));
}

// ---------------------------------------------------------------- 7. auto-protection de condense et cluster

void test_self_protection() {
  u32 right = 0, total = 0;
  for (const Mutation& mu : kMutations) {
    PointDendrogram d = mu.audit_base ? audit_single() : base_dendrogram();
    mu.apply(d);
    ClusterParams p;
    p.min_cluster_size = 1;
    const Clustering cl = cluster(d, p);
    const CondensedTree ct = condense(d, p);
    const bool ok = cl.outcome.reason == mu.reason && ct.outcome.reason == mu.reason && empty_payload(cl) &&
                    ct.parent.empty() && validate(d, p).reason == mu.reason;
    right += ok;
    ++total;
    expect(ok, std::string("cluster(") + mu.name + ") : " + std::string(reason_name(cl.outcome.reason)));
  }
  ClusterParams bad;
  bad.z = std::numeric_limits<double>::quiet_NaN();
  const Clustering cl = cluster(base_dendrogram(), bad);
  const bool ok = cl.outcome.reason == Reason::parameter_out_of_range && empty_payload(cl);
  right += ok;
  ++total;
  expect(ok, "cluster(z = NaN) : " + std::string(reason_name(cl.outcome.reason)));
  std::printf("auto-protection : %u/%u appels invalides refuses sans sortie\n", right, total);
}

}  // namespace

int main() {
  test_validate();
  test_params();
  test_combs();
  test_judge_random();
  test_allow_single();
  test_numeric_domain_fixtures();
  test_numeric_domain_random();
  test_self_protection();
  if (failures) {
    std::printf("head_gate : %d echecs\n", failures);
    return 1;
  }
  if (floor_missed) return 3;
  std::printf("head_gate_ok\n");
  return 0;
}
