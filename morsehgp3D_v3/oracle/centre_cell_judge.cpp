// MorseHGP3D v3 — JUGE ARITHMETIQUEMENT INDEPENDANT DE LA SOURCE PAR CELLULES.
//
// L'audit repete depuis le 12 aout que le juge interne du probe partage
// `ball_front.hpp`, ses lifts et sa forme de puissance avec le sujet : deux
// fautes communes s'y compenseraient sans etre vues. Ce juge-ci ne partage avec
// le sujet QUE le generateur de nuages et une ligne de texte.
//
// REPRESENTATION VOLONTAIREMENT DIFFERENTE. Le sujet decide par des
// determinants entiers `i128` — `orient3`, mineurs a colonne de carres, forme de
// puissance liftee. Ce juge n'emploie aucun determinant. Il resout le centre par
// ELIMINATION DE GAUSS en rationnels multiprecision, dans la base affine du
// support :
//
//   c = u_0 + somme_i lambda_i (u_i - u_0),
//   2 (u_i-u_0) . (c-u_0) = ||u_i-u_0||^2   pour i = 1..q-1.
//
// Les coordonnees barycentriques sortent directement du meme systeme :
// `lambda_i > 0` pour tout `i` et `somme lambda_i < 1` equivaut a
// `c dans relint conv(U)`. Le sujet, lui, teste des signes de mineurs. Le rayon
// carre et le census sont ensuite des COMPARAISONS DE RATIONNELS, jamais des
// signes de determinants.
//
// PORTEE. Le juge enumere tous les sous-ensembles de taille deux, trois et
// quatre et fait son census sur TOUT le nuage. Son cout est donc
// `Theta(n^5)` en rationnels : il est borne a de petits nuages et ne peut jamais
// devenir un backend.
//
// Codes de sortie : 0 accord, 1 desaccord, 2 refus avant calcul,
// 3 plancher viole.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "oracle/rational.hpp"
#include "prototype/cloud_families.hpp"

namespace {

using mhgp3v::BigInt;
using mhgp3v::Rational;

Rational rat(long long v) { return Rational(BigInt(v)); }

// Produit scalaire exact de deux differences de sites.
Rational dot(const mhgp::P3& a, const mhgp::P3& o, const mhgp::P3& b, const mhgp::P3& o2) {
  const long long ax = a.x - o.x, ay = a.y - o.y, az = a.z - o.z;
  const long long bx = b.x - o2.x, by = b.y - o2.y, bz = b.z - o2.z;
  return rat(ax * bx + ay * by + az * bz);
}

// ELIMINATION DE GAUSS rationnelle sur un systeme `k x k`, avec pivot partiel
// par nullite. Renvoie faux si la matrice est singuliere : le support est alors
// affinement dependant et n'a pas de circumcentre dans son enveloppe affine.
bool solve(std::vector<std::vector<Rational>> m, std::vector<Rational> b,
           std::vector<Rational>* out) {
  const int k = (int)b.size();
  for (int col = 0; col < k; ++col) {
    int pivot = -1;
    for (int r = col; r < k; ++r)
      if (!m[(std::size_t)r][(std::size_t)col].numerator().is_zero()) { pivot = r; break; }
    if (pivot < 0) return false;
    std::swap(m[(std::size_t)col], m[(std::size_t)pivot]);
    std::swap(b[(std::size_t)col], b[(std::size_t)pivot]);
    for (int r = 0; r < k; ++r) {
      if (r == col) continue;
      if (m[(std::size_t)r][(std::size_t)col].numerator().is_zero()) continue;
      const Rational factor = m[(std::size_t)r][(std::size_t)col] /
                              m[(std::size_t)col][(std::size_t)col];
      for (int c2 = col; c2 < k; ++c2)
        m[(std::size_t)r][(std::size_t)c2] =
            m[(std::size_t)r][(std::size_t)c2] - factor * m[(std::size_t)col][(std::size_t)c2];
      b[(std::size_t)r] = b[(std::size_t)r] - factor * b[(std::size_t)col];
    }
  }
  out->assign((std::size_t)k, Rational());
  for (int i = 0; i < k; ++i)
    (*out)[(std::size_t)i] = b[(std::size_t)i] / m[(std::size_t)i][(std::size_t)i];
  return true;
}

struct Verdict {
  bool ok = false;
  std::vector<int> interior;
  std::vector<int> shell;
};

// Un support est retenu si son circumcentre est dans `relint conv(U)` et si
// `p + q <= smax`. Tout est decide en rationnels.
Verdict judge_support(const std::vector<mhgp::P3>& cloud, const std::vector<int>& ids,
                      int smax) {
  Verdict v;
  const int q = (int)ids.size();
  const mhgp::P3& u0 = cloud[(std::size_t)ids[0]];
  const int k = q - 1;
  std::vector<std::vector<Rational>> m((std::size_t)k,
                                       std::vector<Rational>((std::size_t)k, Rational()));
  std::vector<Rational> b((std::size_t)k, Rational());
  for (int i = 0; i < k; ++i) {
    const mhgp::P3& ui = cloud[(std::size_t)ids[i + 1]];
    for (int j = 0; j < k; ++j) {
      const mhgp::P3& uj = cloud[(std::size_t)ids[j + 1]];
      m[(std::size_t)i][(std::size_t)j] = rat(2) * dot(ui, u0, uj, u0);
    }
    b[(std::size_t)i] = dot(ui, u0, ui, u0);
  }
  std::vector<Rational> lambda;
  if (!solve(m, b, &lambda)) return v;

  // POSITIVITE par les barycentriques, pas par des signes de mineurs.
  Rational sum = rat(0);
  for (const Rational& l : lambda) {
    if (!(rat(0) < l)) return v;
    sum = sum + l;
  }
  if (!(sum < rat(1))) return v;

  // Le centre, coordonnee par coordonnee, en rationnels.
  Rational c[3] = {rat(u0.x), rat(u0.y), rat(u0.z)};
  for (int i = 0; i < k; ++i) {
    const mhgp::P3& ui = cloud[(std::size_t)ids[i + 1]];
    c[0] = c[0] + lambda[(std::size_t)i] * rat(ui.x - u0.x);
    c[1] = c[1] + lambda[(std::size_t)i] * rat(ui.y - u0.y);
    c[2] = c[2] + lambda[(std::size_t)i] * rat(ui.z - u0.z);
  }
  auto dist2 = [&](const mhgp::P3& p) {
    Rational acc = rat(0);
    const Rational d0 = rat(p.x) - c[0];
    const Rational d1 = rat(p.y) - c[1];
    const Rational d2 = rat(p.z) - c[2];
    acc = d0 * d0 + d1 * d1 + d2 * d2;
    return acc;
  };
  const Rational beta = dist2(u0);

  int interior = 0;
  for (int id = 0; id < (int)cloud.size(); ++id) {
    const Rational d = dist2(cloud[(std::size_t)id]);
    if (d < beta) {
      ++interior;
      if (interior + q > smax) return v;
      v.interior.push_back(id);
    } else if (!(beta < d)) {
      v.shell.push_back(id);
    }
  }
  v.ok = true;
  return v;
}

std::string format_line(const std::vector<int>& s, const std::vector<int>& i,
                        const std::vector<int>& u) {
  std::string line = "ID S=";
  for (std::size_t k = 0; k < s.size(); ++k) {
    if (k) line += ",";
    line += std::to_string(s[k]);
  }
  line += " I=";
  for (std::size_t k = 0; k < i.size(); ++k) {
    if (k) line += ",";
    line += std::to_string(i[k]);
  }
  line += " U=";
  for (std::size_t k = 0; k < u.size(); ++k) {
    if (k) line += ",";
    line += std::to_string(u[k]);
  }
  return line;
}

int parse_int(const char* text, bool* ok) {
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  *ok = end != nullptr && *end == '\0' && end != text && value >= 0 && value <= 100000000LL;
  return (int)value;
}

}  // namespace

int main(int argc, char** argv) {
  int points = 30;
  int smax = 6;
  int coord = 0;
  long long seed = 11;
  std::string family = "uniform";
  std::string subject;
  int min_supports = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    bool ok = true;
    if (arg.rfind("--points=", 0) == 0) points = parse_int(arg.substr(9).c_str(), &ok);
    else if (arg.rfind("--smax=", 0) == 0) smax = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--coord=", 0) == 0) coord = parse_int(arg.substr(8).c_str(), &ok);
    else if (arg.rfind("--seed=", 0) == 0) seed = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--family=", 0) == 0) family = arg.substr(9);
    else if (arg.rfind("--subject=", 0) == 0) subject = arg.substr(10);
    else if (arg.rfind("--min-supports=", 0) == 0)
      min_supports = parse_int(arg.substr(15).c_str(), &ok);
    else {
      std::fprintf(stderr, "REFUS argument inconnu: %s\n", arg.c_str());
      return 2;
    }
    if (!ok) {
      std::fprintf(stderr, "REFUS valeur invalide: %s\n", arg.c_str());
      return 2;
    }
  }
  // LE COUT EST `Theta(n^5)` EN RATIONNELS. Ce juge est borne par construction.
  if (points < 4 || points > 45) {
    std::fprintf(stderr, "REFUS : juge rationnel borne a 45 points\n");
    return 2;
  }
  if (smax < 4 || smax > 16) {
    std::fprintf(stderr, "REFUS domaine smax\n");
    return 2;
  }

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho")
    fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else {
    std::fprintf(stderr, "REFUS famille inconnue: %s\n", family.c_str());
    return 2;
  }
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(fam, points);
  const std::vector<mhgp::P3> cloud = mhgp3v::make_family_cloud(fam, points, coord, seed);
  if ((int)cloud.size() != points) {
    std::fprintf(stderr, "REFUS nuage incomplet\n");
    return 2;
  }

  std::set<std::string> truth;
  std::vector<int> ids;
  for (int q = 2; q <= 4; ++q) {
    std::vector<int> pick((std::size_t)q);
    for (int i = 0; i < q; ++i) pick[(std::size_t)i] = i;
    for (;;) {
      ids.assign(pick.begin(), pick.end());
      const Verdict v = judge_support(cloud, ids, smax);
      if (v.ok) truth.insert(format_line(ids, v.interior, v.shell));
      int i = q - 1;
      while (i >= 0 && pick[(std::size_t)i] == points - q + i) --i;
      if (i < 0) break;
      ++pick[(std::size_t)i];
      for (int j = i + 1; j < q; ++j) pick[(std::size_t)j] = pick[(std::size_t)(j - 1)] + 1;
    }
  }

  std::printf("CentreCellIndependentJudge-v1\n");
  std::printf("famille=%s points=%d coord=%d seed=%lld smax=%d\n", family.c_str(), points,
              coord, seed, smax);
  std::printf("methode=elimination_de_gauss_rationnelle_multiprecision\n");
  std::printf("identites_juge=%zu\n", truth.size());

  if ((int)truth.size() < min_supports) {
    std::fprintf(stderr, "PLANCHER viole : %zu<%d identites\n", truth.size(), min_supports);
    return 3;
  }
  if (subject.empty()) return 0;

  std::ifstream in(subject);
  if (!in) {
    std::fprintf(stderr, "REFUS : fichier sujet illisible\n");
    return 2;
  }
  std::set<std::string> mine;
  std::string line;
  bool saw_count = false;
  std::size_t declared = 0;
  while (std::getline(in, line)) {
    if (line.rfind("ID S=", 0) == 0) mine.insert(line);
    else if (line.rfind("IDENTITES=", 0) == 0) {
      saw_count = true;
      declared = (std::size_t)std::strtoull(line.c_str() + 10, nullptr, 10);
    }
  }
  if (!saw_count) {
    std::fprintf(stderr, "REFUS : le sujet n'a pas ferme sa liste d'identites\n");
    return 2;
  }
  if (declared != mine.size()) {
    std::fprintf(stderr, "REFUS : le sujet annonce %zu identites et en ecrit %zu\n", declared,
                 mine.size());
    return 2;
  }

  int missing = 0, spurious = 0;
  for (const std::string& t : truth)
    if (mine.find(t) == mine.end()) ++missing;
  for (const std::string& m : mine)
    if (truth.find(m) == truth.end()) ++spurious;
  std::printf("identites_sujet=%zu manquantes=%d parasites=%d\n", mine.size(), missing,
              spurious);
  if (missing || spurious) {
    std::fprintf(stderr, "DESACCORD du juge independant\n");
    return 1;
  }
  std::printf("ACCORD JUGE INDEPENDANT sur (support, I_B, U_B)\n");
  return 0;
}
