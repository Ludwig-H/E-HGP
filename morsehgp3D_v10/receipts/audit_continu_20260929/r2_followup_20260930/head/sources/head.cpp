#include "head/head.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace mhgp10 {

namespace {

// Etat prepare par la garde du domaine numerique conjoint, consomme par la condensation.
struct Prepared {
  std::vector<u64> mass;    // masse de chaque sous-arbre (u64 exact : au plus (2^32 - 1)^2)
  std::vector<double> lam;  // rang -> lambda, calcule pour les seuls rangs consommes (les autres ne sont jamais lus)
  u32 root = kNone;
};

// Plafond de (4) : M * lambda_max < 2^kCeilingLog2 (head.hpp).
constexpr int kCeilingLog2 = 1000;

// (4), comparaison exacte : lmax = f * 2^e (frexp, f dans [1/2, 1)), mantisse entiere m = f * 2^53 < 2^53 ; alors
// M * lmax < 2^1000 <=> M * m < 2^(1053 - e), et M * m < 2^117 tient dans u128. lmax = 0 (aucun rang consomme) : vrai.
bool below_ceiling(u64 total, double lmax) {
  int e = 0;
  const u64 m = static_cast<u64>(std::ldexp(std::frexp(lmax, &e), 53));
  const int shift = kCeilingLog2 + 53 - e;
  return shift >= 128 || u128(total) * m < (u128(1) << shift);
}

// validate(d), validate(p), puis domaine numerique conjoint (head.hpp, regles 1 a 4). Lineaire.
Outcome prepare(const PointDendrogram& d, const ClusterParams& p, Prepared& s) {
  Outcome o = validate(d);
  if (!o.ok()) return o;
  o = validate(p);
  if (!o.ok()) return o;
  const u32 n = d.nodes();
  s.mass.assign(n, 0);
  u64 total = 0;
  for (u32 x = 0; x < d.points(); ++x) {
    s.mass[d.point_node[x]] += d.point_weight[x];
    total += d.point_weight[x];
  }
  for (u32 v = 0; v < n; ++v) {  // enfants d'indice inferieur a leur parent (validate(d)) : masses completes
    if (d.parent[v] != kNone) s.mass[d.parent[v]] += s.mass[v];
    else s.root = v;
  }
  // composante gardee vivante par la condensation : la racine, ou une masse >= mcs (la masse croit vers la racine,
  // donc tous ses ancetres sont vivants)
  auto alive = [&](u32 v) { return v == s.root || s.mass[v] >= p.min_cluster_size; };
  std::vector<u8> used(d.level.size(), 0);
  for (u32 v = 0; v < n; ++v) {
    const bool zero = d.level[d.node_rank[v]] == 0;  // -0 compris
    const u32 kids = d.child_off[v + 1] - d.child_off[v];
    MHGP10_CHECK(!(zero && kids >= 2), numeric_domain);  // (1) fusion au niveau 0
    if (!alive(v)) continue;
    MHGP10_CHECK(!zero, numeric_domain);  // (2) composante vivante nee au niveau 0
    if (kids > 0) used[d.node_rank[v]] = 1;  // lambda de sa fusion (naissances, sorties des enfants legers)
  }
  for (u32 x = 0; x < d.points(); ++x)
    if (alive(d.point_node[x])) used[d.point_rank[x]] = 1;  // lambda d'entree : le point sort a son propre niveau
  // (3) lambda fini et normal sur tout rang consomme ; (4) M * lambda_max < 2^1000, exactement. Apres (2), tout rang
  // consomme a un niveau strictement positif (niveaux strictement croissants, rang d'un point >= rang de sa
  // composante).
  s.lam.assign(d.level.size(), 0.0);
  const double e = -0.5 * p.z;
  double lmax = 0;
  for (u64 r = 0; r < d.level.size(); ++r) {
    if (!used[r]) continue;
    const double l = std::pow(d.level[r], e);
    MHGP10_CHECK(std::isnormal(l), numeric_domain);
    s.lam[r] = l;
    lmax = std::max(lmax, l);
  }
  MHGP10_CHECK(below_ceiling(total, lmax), numeric_domain);
  return Outcome{};
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Entier decimal sans signe, sans depassement de u64.
bool parse_u64(const std::string& s, u64& v) {
  if (s.empty()) return false;
  v = 0;
  for (char c : s) {
    if (!is_digit(c)) return false;
    const u64 digit = static_cast<u64>(c - '0');
    if (v > (std::numeric_limits<u64>::max() - digit) / 10) return false;
    v = 10 * v + digit;
  }
  return true;
}

// Taille maximale d'un fichier de configurations.
constexpr size_t kMaxConfigsBytes = size_t(1) << 20;

}  // namespace

Outcome validate(const ClusterParams& p) {
  MHGP10_CHECK(p.min_cluster_size >= 1, parameter_out_of_range);
  MHGP10_CHECK(std::isfinite(p.z) && p.z > 0 && p.z <= kMaxScaleExponent, parameter_out_of_range);
  MHGP10_CHECK(p.selection == Selection::eom || p.selection == Selection::leaf, parameter_out_of_range);
  return Outcome{};
}

Outcome validate(const PointDendrogram& d, const ClusterParams& p) {
  Prepared s;
  return prepare(d, p, s);
}

bool parse_z(const std::string& s, double& z) {
  size_t i = 0, digits = 0;
  while (i < s.size() && is_digit(s[i])) ++i, ++digits;
  if (i < s.size() && s[i] == '.') {
    ++i;
    while (i < s.size() && is_digit(s[i])) ++i, ++digits;
  }
  if (digits == 0) return false;
  if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
    ++i;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
    size_t exp_digits = 0;
    while (i < s.size() && is_digit(s[i])) ++i, ++exp_digits;
    if (exp_digits == 0) return false;
  }
  if (i != s.size()) return false;
  char* end = nullptr;
  const double v = std::strtod(s.c_str(), &end);
  if (end != s.c_str() + s.size()) return false;
  z = v;
  return true;
}

Outcome read_configs(const std::string& path, std::vector<ClusterParams>& out) {
  out.clear();
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return fail(Reason::parameter_out_of_range);
  std::string text;
  char buf[4096];
  bool ok = true;
  size_t got;
  while (ok && (got = std::fread(buf, 1, sizeof buf, f)) > 0) {
    text.append(buf, got);
    ok = text.size() <= kMaxConfigsBytes;
  }
  ok = ok && !std::ferror(f);
  std::fclose(f);
  std::vector<ClusterParams> list;
  std::vector<std::string> tok;
  for (size_t pos = 0; ok && pos < text.size();) {
    size_t eol = text.find('\n', pos);
    if (eol == std::string::npos) eol = text.size();
    size_t end = eol;
    if (end > pos && text[end - 1] == '\r') --end;  // fin de ligne \r\n
    tok.clear();
    for (size_t i = pos; i < end;) {
      if (text[i] == ' ' || text[i] == '\t') {
        ++i;
        continue;
      }
      size_t j = i;
      while (j < end && text[j] != ' ' && text[j] != '\t') ++j;
      tok.emplace_back(text, i, j - i);
      i = j;
    }
    pos = eol + 1;
    if (tok.empty()) continue;  // ligne blanche
    ClusterParams q;
    ok = tok.size() == 4 && parse_u64(tok[0], q.min_cluster_size) && parse_z(tok[1], q.z) &&
         (tok[2] == "eom" || tok[2] == "leaf") && (tok[3] == "0" || tok[3] == "1");
    if (!ok) break;
    q.selection = tok[2] == "eom" ? Selection::eom : Selection::leaf;
    q.allow_single_cluster = tok[3] == "1";
    ok = validate(q).ok();
    list.push_back(q);
  }
  if (!ok || list.empty()) return fail(Reason::parameter_out_of_range);
  out.swap(list);
  return Outcome{};
}

CondensedTree condense(const PointDendrogram& d, const ClusterParams& p) {
  CondensedTree t;
  Prepared s;
  t.outcome = prepare(d, p, s);
  if (!t.outcome.ok()) return t;  // aucun calcul flottant, tables vides
  const u32 n = d.nodes();
  const std::vector<u64>& mass = s.mass;
  const u32 root = s.root;
  // points directement attaches a chaque noeud (CSR)
  std::vector<u32> att_off(n + 1, 0), att(d.points());
  for (u32 x = 0; x < d.points(); ++x) ++att_off[d.point_node[x] + 1];
  for (u32 v = 0; v < n; ++v) att_off[v + 1] += att_off[v];
  {
    std::vector<u32> fill(att_off.begin(), att_off.end() - 1);
    for (u32 x = 0; x < d.points(); ++x) att[fill[d.point_node[x]]++] = x;
  }

  t.point_cluster.assign(d.points(), kNone);
  t.node_cluster.assign(n, kNone);
  t.point_lambda.assign(d.points(), 0.0);
  auto new_cluster = [&](u32 parent, double birth, u64 m) {
    t.parent.push_back(parent);
    t.birth.push_back(birth);
    t.stability.push_back(0.0);
    t.mass.push_back(m);
    return static_cast<u32>(t.parent.size() - 1);
  };
  // sortie de tous les points d'un sous-arbre au lambda donne
  std::vector<u32> stack;
  auto drop_subtree = [&](u32 v, u32 c, double lam) {
    stack.assign(1, v);
    while (!stack.empty()) {
      const u32 u = stack.back();
      stack.pop_back();
      t.node_cluster[u] = c;
      for (u32 j = att_off[u]; j < att_off[u + 1]; ++j) {
        const u32 x = att[j];
        t.point_cluster[x] = c;
        t.point_lambda[x] = lam;
        t.stability[c] += d.point_weight[x] * (lam - t.birth[c]);
      }
      for (u32 j = d.child_off[u]; j < d.child_off[u + 1]; ++j) stack.push_back(d.child_val[j]);
    }
  };
  // travail : (noeud, cluster courant) ; seuls les noeuds vivants (racine, masse >= mcs) y entrent, et leurs lambdas
  // sont ceux de la table preparee (rangs consommes, finis et normaux)
  std::vector<std::pair<u32, u32>> work;
  work.push_back({root, new_cluster(kNone, 0.0, mass[root])});
  std::vector<u32> big;
  while (!work.empty()) {
    auto [v, c] = work.back();
    work.pop_back();
    t.node_cluster[v] = c;
    // points attaches directement : sortent a leur niveau d'entree
    for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) {
      const u32 x = att[j];
      const double lam = s.lam[d.point_rank[x]];
      t.point_cluster[x] = c;
      t.point_lambda[x] = lam;
      t.stability[c] += d.point_weight[x] * (lam - t.birth[c]);
    }
    if (d.child_off[v] == d.child_off[v + 1]) continue;
    const double lam = s.lam[d.node_rank[v]];
    big.clear();
    for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j)
      if (mass[d.child_val[j]] >= p.min_cluster_size) big.push_back(d.child_val[j]);
    if (big.size() >= 2) {
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
        const u32 u = d.child_val[j];
        if (mass[u] >= p.min_cluster_size) {
          t.stability[c] += double(mass[u]) * (lam - t.birth[c]);
          work.push_back({u, new_cluster(c, lam, mass[u])});
        } else {
          drop_subtree(u, c, lam);
        }
      }
    } else {
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
        const u32 u = d.child_val[j];
        if (big.size() == 1 && u == big[0]) work.push_back({u, c});
        else drop_subtree(u, c, lam);
      }
    }
  }
  return t;
}

Clustering cluster(const PointDendrogram& d, const ClusterParams& p) {
  Clustering out;
  out.tree = condense(d, p);
  out.outcome = out.tree.outcome;
  if (!out.outcome.ok()) return out;  // refus : sorties vides
  const CondensedTree& t = out.tree;
  const u32 m = static_cast<u32>(t.parent.size());
  u64& steps = out.ancestor_steps;
  // enfants de chaque cluster (les enfants ont un indice superieur au parent : le condense cree le cluster 0, la
  // racine, puis chaque cluster depuis son parent deja cree)
  std::vector<std::vector<u32>> kids(m);
  for (u32 c = 1; c < m; ++c, ++steps) kids[t.parent[c]].push_back(c);
  std::vector<unsigned char> chosen(m, 0);
  if (p.selection == Selection::eom) {
    std::vector<double> best(m, 0.0);
    for (u32 c = m; c-- > 0;) {
      double sub = 0;
      for (u32 k : kids[c]) sub += best[k];
      const bool is_root = t.parent[c] == kNone;
      if (kids[c].empty()) {
        best[c] = t.stability[c];
        chosen[c] = 1;
      } else if (is_root && !p.allow_single_cluster) {
        best[c] = sub;
      } else if (sub > t.stability[c]) {
        best[c] = sub;
      } else {
        best[c] = t.stability[c];
        chosen[c] = 1;
      }
    }
    // un cluster retenu desactive ses descendants : « un ancetre strict est retenu » se propage du parent a
    // l'enfant en une passe (parents avant enfants). covered[a] || chosen[a] (deja desactive) vaut
    // covered[a] || retenu initial de a : meme resultat que la remontee jusqu'au premier ancetre retenu.
    std::vector<unsigned char> covered(m, 0);
    for (u32 c = 1; c < m; ++c, ++steps) {
      const u32 a = t.parent[c];
      covered[c] = covered[a] || chosen[a];
      if (covered[c]) chosen[c] = 0;
    }
  } else {
    for (u32 c = 0; c < m; ++c) chosen[c] = kids[c].empty();
  }
  if (!p.allow_single_cluster && m > 0) chosen[0] = 0;
  std::vector<i32> id(m, -1);
  for (u32 c = 0; c < m; ++c)
    if (chosen[c]) {
      id[c] = static_cast<i32>(out.selected.size());
      out.selected.push_back(c);
    }
  // etiquette du premier ancetre retenu (lui compris), propagee du parent a l'enfant ; un point lit celle du
  // cluster d'ou il sort
  out.cluster_label.assign(m, -1);
  for (u32 c = 0; c < m; ++c) {
    if (chosen[c]) {
      out.cluster_label[c] = id[c];
    } else if (c > 0) {
      out.cluster_label[c] = out.cluster_label[t.parent[c]];
      ++steps;
    }
  }
  out.label.assign(d.points(), -1);
  for (u32 x = 0; x < d.points(); ++x)
    if (t.point_cluster[x] != kNone) out.label[x] = out.cluster_label[t.point_cluster[x]];
  // allow_single_cluster, racine seule retenue (elle desactive tous ses descendants) : regle d'appartenance de la
  // racine de scikit-learn. Seuil = plus haut lambda des lignes de la racine dans le condense.
  if (m > 0 && chosen[0]) {
    double threshold = -std::numeric_limits<double>::infinity();
    for (u32 x = 0; x < d.points(); ++x)
      if (t.point_cluster[x] == 0 && t.point_lambda[x] > threshold) threshold = t.point_lambda[x];
    for (u32 c : kids[0])
      if (t.birth[c] > threshold) threshold = t.birth[c];
    for (u32 x = 0; x < d.points(); ++x)
      if (!(t.point_lambda[x] >= threshold)) out.label[x] = -1;
  }
  return out;
}

}  // namespace mhgp10
