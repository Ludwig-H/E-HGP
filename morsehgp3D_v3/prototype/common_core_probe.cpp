// MorseHGP3D v3 — SUJET `counter-only` : COEUR COMMUN DE JUNG PAR BLOCS.
//
// Specification : audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md
// section 5, corrigee par la reponse Q7 de
// audits/AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LE THEOREME, ET SA CORRECTION
//
// Soient deux blocs `A`, `B` de boules englobantes certifiees `(c_A,R_A)` et
// `(c_B,R_B)`, `S=R_A+R_B`, `d=||c_B-c_A||` et `m_0=(c_A+c_B)/2`. Pour toute
// paire `a` dans `A`, `b` dans `B`, le milieu de `ab` est a distance au plus
// `S/2` de `m_0` et sa longueur verifie `D >= d-S`.
//
// Si `ab` est l'arete maximale d'un support positif q3 ou q4, la geometrie de
// Jung donne une epaisseur minimale de la circumboule superieure a `D/4` autour
// du milieu. Donc, si `d > 3S`,
//
//   R_AB = (d - 3S)/4 > 0    et    B°(m_0, R_AB)  inclus dans  B°_{a,b}
//
// pour TOUTES les paires du bloc a la fois : tout `z` du coeur verifie
// ||z-(a+b)/2|| < (d-3S)/4 + S/2 = (d-S)/4 <= D/4.
//
// ---------------------------------------------------------------------------
// CE QUE Q7 CORRIGE, ET QUI CHANGE TOUT
//
// `d > 3S` construit un coeur. Il ne fournit AUCUN temoin. Le bloc n'est ferme
// que si une requete y certifie `h = smax+1-q` `PointId` DISTINCTS et
// strictement interieurs — huit pour q4, neuf pour q3 a `smax=11`.
//
// Deux amas serres avec un vide central ont donc un coeur parfaitement defini
// et parfaitement VIDE : le bloc reste entierement ouvert. C'est pourquoi ce
// certificat, contrairement aux deux autres, ne vise pas le mur des amas. Le
// ledger le mesure au lieu de l'esperer : `coeurs_vides` compte exactement ces
// blocs.
//
// ---------------------------------------------------------------------------
// TOUT EST ENTIER
//
// `R_A` est majore par le demi-diametre de l'AABB, arrondi SUPERIEUREMENT par
// racine entiere ; `d` est minore par racine entiere INFERIEURE. La separation
// testee est donc `d_lb > 3(R_A+R_B)`, plus stricte que la condition reelle.
// L'appartenance au coeur emploie des coordonnees DOUBLEES pour eviter le
// demi-entier de `m_0` :
//
//   ||2z - (c_A+c_B)||^2 < (2 R_AB)^2.
//
// Largeurs u16 : `||2z-(c_A+c_B)||^2 <= 3*(4*65535)^2 = 2,1e11` et
// `(2R_AB)^2 <= 1,8e10`. Tout tient dans `i64`.
//
// CODES DE SORTIE : 1 desaccords du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"
#include "prototype/spindle_cone_oracle.hpp"

namespace {

using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::LbvhNode;
using mhgp3v::MortonLbvh;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(2);
}
[[noreturn]] void violate(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(3);
}

constexpr int kNeed[3] = {10, 9, 8};

enum class CoreMutant {
  kNone,
  // `separation-deux` et `compte-sans-distinction` ont ete RETIRES : le
  // contre-audit montre qu'ils restent SOUND. Le premier arrete plus tot en
  // gardant le numerateur `d-3S`, donc il perd des descendants sans jamais
  // fermer a tort ; le second retirait la soustraction de deux, qui etait
  // elle-meme injustifiee. Un mutant sound n'est pas un mutant.
  kBoundaryInside,  // accepte l'egalite : le bord du coeur devient interieur
  kFloorRadius,     // arrondit le rayon du coeur vers le HAUT
};

const char* core_mutant_name(CoreMutant m) {
  switch (m) {
    case CoreMutant::kNone: return "none";
    case CoreMutant::kBoundaryInside: return "coeur-bord-interieur";
    case CoreMutant::kFloorRadius: return "coeur-rayon-arrondi-haut";
  }
  return "?";
}

// Racine entiere par defaut et par exces, exactes.
i64 isqrt_floor(i64 v) {
  if (v <= 0) return 0;
  i64 r = (i64)std::sqrt((double)v);
  while (r > 0 && r * r > v) --r;
  while ((r + 1) * (r + 1) <= v) ++r;
  return r;
}
i64 isqrt_ceil(i64 v) {
  const i64 r = isqrt_floor(v);
  return (r * r == v) ? r : r + 1;
}

struct Ledger {
  long long node_pairs_visited = 0;
  long long blocks_emitted = 0;      // blocs bien separes
  long long blocks_split = 0;
  long long core_queries = 0;
  long long core_point_tests = 0;
  // TROIS COMPTEURS DISTINCTS, PAS UN SEUL.
  //
  // Une premiere version incrementait `cores_empty` des que `occupants-2 < 8`.
  // Elle comptait donc des coeurs SOUS-PLEINS sous une soustraction de deux
  // injustifiee, et le chiffre `2 306/2 306` ne prouvait aucun vide central. Le
  // contre-audit l'a refuse a juste titre : le vide et la sous-plenitude sont
  // deux faits differents et doivent etre publies separement.
  long long occupancy_zero = 0;       // coeur reellement VIDE
  long long underfull[3] = {0, 0, 0}; // coeur non vide mais sous le seuil de lane
  long long closed_directed[3] = {0, 0, 0};
  long long residual_directed[3] = {0, 0, 0};
  long long block_mass_closed[3] = {0, 0, 0};
  long long core_occupancy_hwm = 0;
  long long block_mass_hwm = 0;
  long long pair_mass_covered = 0;   // doit valoir exactement C(n,2)
  long long pair_multiplicity_bad = 0;  // paires vues != 1 fois
};

struct Options {
  long long n = 400;
  long long coord = -1;
  long long seed = 3;
  long long smax = 11;
  long long leaf = 8;
  bool judge = false;
  CloudFamily family = CloudFamily::kUniform;
  CoreMutant mutant = CoreMutant::kNone;
  long long min_blocks = 0;
  long long min_closed_q4 = 0;
  long long min_cores_empty = 0;
  long long min_residuel = 0;
};

bool parse_ll(const char* s, long long* out) {
  if (s == nullptr || *s == '\0') return false;
  errno = 0;
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (end == nullptr || *end != '\0') return false;
  if (errno == ERANGE) return false;
  *out = v;
  return true;
}

// Boule englobante certifiee d'un noeud : centre entier double, rayon majore.
struct Ball {
  i64 c2[3] = {0, 0, 0};  // centre DOUBLE, pour rester entier
  i64 r = 0;              // majorant entier du rayon
};

Ball ball_of(const LbvhNode& nd) {
  Ball b{};
  i64 half2 = 0;
  for (int k = 0; k < 3; ++k) {
    b.c2[k] = nd.lo[k] + nd.hi[k];
    const i64 e = nd.hi[k] - nd.lo[k];
    half2 += e * e;
  }
  // Rayon = demi-diagonale = sqrt(somme e^2)/2, majore : ceil(sqrt(somme e^2)/2)
  // se calcule exactement par ceil(sqrt(somme e^2)) puis division haute par 2.
  const i64 diag = isqrt_ceil(half2);
  b.r = (diag + 1) / 2;
  return b;
}

}  // namespace

int main(int argc, char** argv) {
  Options opt{};
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const std::size_t eq = arg.find('=');
    const std::string key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
    const std::string val = (eq == std::string::npos) ? std::string() : arg.substr(eq + 1);
    long long p = 0;
    if (key == "--points") { if (!parse_ll(val.c_str(), &p)) refuse("--points invalide"); opt.n = p; }
    else if (key == "--coord") { if (!parse_ll(val.c_str(), &p)) refuse("--coord invalide"); opt.coord = p; }
    else if (key == "--seed") { if (!parse_ll(val.c_str(), &p)) refuse("--seed invalide"); opt.seed = p; }
    else if (key == "--smax") { if (!parse_ll(val.c_str(), &p)) refuse("--smax invalide"); opt.smax = p; }
    else if (key == "--leaf") { if (!parse_ll(val.c_str(), &p)) refuse("--leaf invalide"); opt.leaf = p; }
    else if (key == "--min-blocs") { if (!parse_ll(val.c_str(), &p)) refuse("--min-blocs invalide"); opt.min_blocks = p; }
    else if (key == "--min-closed-q4") { if (!parse_ll(val.c_str(), &p)) refuse("--min-closed-q4 invalide"); opt.min_closed_q4 = p; }
    else if (key == "--min-coeurs-vides") { if (!parse_ll(val.c_str(), &p)) refuse("--min-coeurs-vides invalide"); opt.min_cores_empty = p; }
    else if (key == "--min-residuel") { if (!parse_ll(val.c_str(), &p)) refuse("--min-residuel invalide"); opt.min_residuel = p; }
    else if (key == "--judge") { opt.judge = true; }
    else if (key == "--family") {
      if (val == "uniform") opt.family = CloudFamily::kUniform;
      else if (val == "terrain") opt.family = CloudFamily::kTerrain;
      else if (val == "eight_clusters") opt.family = CloudFamily::kEightClusters;
      else if (val == "scanline_single_pass") opt.family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") opt.family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else if (key == "--inject") {
      if (val == "coeur-bord-interieur") opt.mutant = CoreMutant::kBoundaryInside;
      else if (val == "coeur-rayon-arrondi-haut") opt.mutant = CoreMutant::kFloorRadius;
      else refuse("--inject inconnu");
    } else {
      refuse("argument inconnu");
    }
  }

  if (opt.smax < 4 || opt.smax > 34) refuse("--smax hors du domaine d'enveloppe [4, 34]");
  if (opt.n < 16 || opt.n > 4000) refuse("--points hors bornes [16, 4000]");
  if (opt.leaf < 1 || opt.leaf > 256) refuse("--leaf hors bornes");
  if (opt.mutant != CoreMutant::kNone && !opt.judge)
    refuse("un mutant sans juge ne prouve rien : --inject exige --judge");
  if (opt.judge && opt.n > 1200) refuse("le juge par paire est borne a --points <= 1200");
  if (opt.coord < 0) opt.coord = mhgp3v::cloud_family_default_coord(opt.family, (int)opt.n);
  if (opt.coord < 2 || opt.coord > 65536) refuse("--coord hors bornes");

  const std::vector<P3> pts =
      mhgp3v::make_family_cloud(opt.family, (int)opt.n, (int)opt.coord, opt.seed);
  if ((long long)pts.size() != opt.n)
    refuse("le generateur n'a pas rendu la cardinalite demandee");
  const int n = (int)pts.size();

  MortonLbvh tree;
  tree.build(pts, (int)opt.leaf);

  Ledger led{};
  std::vector<std::vector<unsigned char>> closed(3);
  for (int q = 0; q < 3; ++q) closed[(std::size_t)q].assign((std::size_t)n * (std::size_t)n, 0);

  // Decomposition en blocs : recursion sur les couples de noeuds, avec split du
  // plus gros tant que la separation n'est pas certifiee.
  // MULTIPLICITE PAR `PairId`. L'identite scalaire `somme = C(n,2)` est
  // NECESSAIRE mais pas suffisante : une omission peut etre compensee par un
  // doublon et le total resterait juste. Le compteur par paire tranche.
  std::vector<unsigned char> seen((std::size_t)n * (std::size_t)n, 0);
  auto mark_block = [&](const LbvhNode& na2, const LbvhNode& nb2, bool same) {
    for (int ta = na2.begin; ta < na2.end; ++ta)
      for (int tb = (same ? ta + 1 : nb2.begin); tb < nb2.end; ++tb) {
        const int u = tree.order[(std::size_t)ta], v = tree.order[(std::size_t)tb];
        if (u == v) continue;
        const std::size_t k = (std::size_t)std::min(u, v) * (std::size_t)n + (std::size_t)std::max(u, v);
        if (seen[k] != 0) ++led.pair_multiplicity_bad;
        seen[k] = 1;
      }
  };

  std::vector<std::pair<int, int>> stack;
  stack.push_back({0, 0});
  while (!stack.empty()) {
    const auto [ia, ib] = stack.back();
    stack.pop_back();
    ++led.node_pairs_visited;
    const LbvhNode& na = tree.nodes[(std::size_t)ia];
    const LbvhNode& nb = tree.nodes[(std::size_t)ib];
    const long long ma = na.end - na.begin, mb = nb.end - nb.begin;
    if (ma <= 0 || mb <= 0) continue;

    const Ball ba = ball_of(na), bb = ball_of(nb);
    i64 d2x4 = 0;  // ||c_B - c_A||^2 en coordonnees doublees, donc 4*d^2
    for (int k = 0; k < 3; ++k) {
      const i64 e = bb.c2[k] - ba.c2[k];
      d2x4 += e * e;
    }
    const i64 d_lb = isqrt_floor(d2x4) / 2;  // minorant entier de d
    const i64 S = ba.r + bb.r;
    const bool separated = (ia != ib) && (d_lb > 3 * S);

    const long long block_pairs = (ia == ib) ? (ma * (ma - 1) / 2) : (ma * mb);
    if (!separated) {
      const bool a_leaf = na.left < 0, b_leaf = nb.left < 0;
      // LE CAS `A == B` DOIT SE SCINDER DES DEUX COTES A LA FOIS.
      //
      // Une premiere version poussait `(gauche, A)` et `(droite, A)` : les blocs
      // produits etaient IMBRIQUES, donc la decomposition ne partitionnait plus
      // les paires et le ledger cessait d'etre une identite — `masse_bloc` et
      // fermetures dirigees divergeaient sans qu'aucun invariant ne l'annonce.
      // La recursion correcte d'une decomposition bien separee traite `A x A`
      // par `(A_g,A_g)`, `(A_d,A_d)` et `(A_g,A_d)`, ce qui couvre chaque paire
      // non ordonnee EXACTEMENT une fois.
      if (ia == ib) {
        if (a_leaf) { led.pair_mass_covered += block_pairs; mark_block(na, na, true); continue; }
        ++led.blocks_split;
        stack.push_back({na.left, na.left});
        stack.push_back({na.right, na.right});
        stack.push_back({na.left, na.right});
        continue;
      }
      if (a_leaf && b_leaf) {
        led.pair_mass_covered += block_pairs;
        mark_block(na, nb, false);
        continue;
      }
      ++led.blocks_split;
      if (!a_leaf && (b_leaf || ma >= mb)) {
        stack.push_back({na.left, ib});
        stack.push_back({na.right, ib});
      } else {
        stack.push_back({ia, nb.left});
        stack.push_back({ia, nb.right});
      }
      continue;
    }

    ++led.blocks_emitted;
    led.pair_mass_covered += block_pairs;
    mark_block(na, nb, ia == ib);
    if (ma * mb > led.block_mass_hwm) led.block_mass_hwm = ma * mb;

    // Rayon du coeur, arrondi vers le BAS pour rester conservateur : le mutant
    // l'arrondit vers le haut et fait entrer des points qui n'y sont pas.
    const i64 num = d_lb - 3 * S;
    if (num <= 0) { ++led.occupancy_zero; continue; }
    const i64 r_core = (opt.mutant == CoreMutant::kFloorRadius) ? ((num + 3) / 4) : (num / 4);
    const i64 two_r = 2 * r_core;
    const i64 lim = two_r * two_r;
    const i64 m0x2[3] = {ba.c2[0] + bb.c2[0], ba.c2[1] + bb.c2[1], ba.c2[2] + bb.c2[2]};
    // `m0` double vaut (c_A+c_B)/2 en coordonnees doublees, donc `m0x2/2`.
    // Pour rester entier on compare ||4z - m0x2||^2 a (4 r_core)^2.
    ++led.core_queries;
    long long occupants = 0;
    for (int z = 0; z < n; ++z) {
      ++led.core_point_tests;
      i64 acc = 0;
      for (int k = 0; k < 3; ++k) {
        const i64 c = (k == 0 ? 4 * (i64)pts[(std::size_t)z].x
                              : (k == 1 ? 4 * (i64)pts[(std::size_t)z].y
                                        : 4 * (i64)pts[(std::size_t)z].z)) -
                      m0x2[k];
        acc += c * c;
      }
      const bool inside = (opt.mutant == CoreMutant::kBoundaryInside) ? (acc <= 4 * lim)
                                                                     : (acc < 4 * lim);
      if (!inside) continue;
      // Les endpoints potentiels du bloc ne peuvent pas se compter eux-memes.
      const int rank = 0;
      (void)rank;
      ++occupants;
    }
    if (occupants > led.core_occupancy_hwm) led.core_occupancy_hwm = occupants;

    if (occupants == 0) ++led.occupancy_zero;
    for (int q = 0; q < 3; ++q) {
      // LES ENDPOINTS SONT STRICTEMENT HORS DU COEUR, ET C'EST DEMONTRABLE :
      // pour `a` dans `A`, la distance de `a` a `m_0` vaut au moins
      // `d/2 - R_A >= d/2 - S`, tandis que le rayon du coeur vaut
      // `(d-3S)/4`. Or `d/2 - S - (d-3S)/4 = (d-S)/4 > 0` des que `d > 3S`.
      // La soustraction de deux d'une premiere version etait donc injustifiee
      // et retiree : elle ne protegeait de rien et faussait le compte.
      const long long usable = occupants;
      if (usable < kNeed[q]) { ++led.underfull[(std::size_t)q]; continue; }
      led.block_mass_closed[(std::size_t)q] += ma * mb;
      for (int ta = na.begin; ta < na.end; ++ta)
        for (int tb = nb.begin; tb < nb.end; ++tb) {
          const int a = tree.order[(std::size_t)ta], b = tree.order[(std::size_t)tb];
          if (a == b) continue;
          closed[(std::size_t)q][(std::size_t)a * (std::size_t)n + (std::size_t)b] = 1;
          closed[(std::size_t)q][(std::size_t)b * (std::size_t)n + (std::size_t)a] = 1;
        }
    }
  }

  // IDENTITE DE COUVERTURE. La decomposition doit visiter chaque paire non
  // ordonnee exactement une fois, repartie entre blocs emis et blocs terminaux
  // non separes. Sans cette verification, une recursion imbriquee compterait
  // deux fois et personne ne le verrait.
  if (led.pair_mass_covered != (long long)n * ((long long)n - 1) / 2)
    violate("la decomposition ne partitionne pas les paires non ordonnees");
  if (led.pair_multiplicity_bad != 0)
    violate("une paire non ordonnee est couverte plus d'une fois");
  {
    long long unseen = 0;
    for (int u = 0; u < n; ++u)
      for (int v = u + 1; v < n; ++v)
        if (seen[(std::size_t)u * (std::size_t)n + (std::size_t)v] == 0) ++unseen;
    if (unseen != 0) violate("une paire non ordonnee n'est couverte par aucun bloc");
  }

  for (int q = 0; q < 3; ++q) {
    long long c = 0;
    for (int a = 0; a < n; ++a)
      for (int b = 0; b < n; ++b)
        if (a != b && closed[(std::size_t)q][(std::size_t)a * (std::size_t)n + (std::size_t)b]) ++c;
    led.closed_directed[(std::size_t)q] = c;
    led.residual_directed[(std::size_t)q] = (long long)n * (n - 1) - c;
  }

  std::printf("CommonCoreReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=proposition_math_non_recue "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%d coord=%lld seed=%lld smax=%lld leaf=%lld inject=%s\n",
              mhgp3v::cloud_family_name(opt.family), n, opt.coord, opt.seed, opt.smax, opt.leaf,
              core_mutant_name(opt.mutant));
  std::printf("blocs visites=%lld emis=%lld splits=%lld masse_hwm=%lld paires_couvertes=%lld/%lld\n",
              led.node_pairs_visited, led.blocks_emitted, led.blocks_split, led.block_mass_hwm,
              led.pair_mass_covered, (long long)n * ((long long)n - 1) / 2);
  std::printf("coeur requetes=%lld tests_points=%lld vides=%lld sous_pleins_q2=%lld "
              "sous_pleins_q3=%lld sous_pleins_q4=%lld occupation_hwm=%lld\n",
              led.core_queries, led.core_point_tests, led.occupancy_zero,
              led.underfull[0], led.underfull[1], led.underfull[2], led.core_occupancy_hwm);
  for (int q = 0; q < 3; ++q)
    std::printf("lane q%d ferme=%lld residuel=%lld masse_bloc_fermee=%lld\n", q + 2,
                led.closed_directed[(std::size_t)q], led.residual_directed[(std::size_t)q],
                led.block_mass_closed[(std::size_t)q]);

  int disagreements = 0;
  if (opt.judge) {
    std::vector<int> xyz;
    xyz.reserve((std::size_t)n * 3);
    for (const P3& p : pts) { xyz.push_back((int)p.x); xyz.push_back((int)p.y); xyz.push_back((int)p.z); }
    long long wrong[3] = {0, 0, 0}, agreed[3] = {0, 0, 0}, printed = 0;
    for (int q = 0; q < 3; ++q)
      for (int a = 0; a < n; ++a)
        for (int b = 0; b < n; ++b) {
          if (a == b) continue;
          if (!closed[(std::size_t)q][(std::size_t)a * (std::size_t)n + (std::size_t)b]) continue;
          long long c[3];
          mhgp3v::cone_oracle::count_witnesses(xyz, n, a, b, c);
          if (c[q] < (long long)kNeed[q]) {
            ++wrong[q];
            if (printed++ < 8)
              std::fprintf(stderr,
                           "DESACCORD q%d : le bloc ferme (%d,%d) mais le juge n'y compte que "
                           "%lld temoins universels pour un seuil de %d\n",
                           q + 2, a, b, c[q], kNeed[q]);
          } else {
            ++agreed[q];
          }
        }
    std::printf("juge accord=%lld/%lld/%lld desaccords=%lld/%lld/%lld accord=%s\n", agreed[0],
                agreed[1], agreed[2], wrong[0], wrong[1], wrong[2],
                (wrong[0] + wrong[1] + wrong[2]) == 0 ? "OUI" : "NON");
    disagreements = (int)(wrong[0] + wrong[1] + wrong[2]);
  }

  if (opt.mutant != CoreMutant::kNone) {
    if (disagreements > 0) {
      std::fprintf(stderr, "MUTANT TUE (juge) : %s\n", core_mutant_name(opt.mutant));
      return 4;
    }
    violate("MUTANT SURVIVANT : la porte est vacueuse pour cette injection");
  }
  if (disagreements > 0) return 1;

  if (led.blocks_emitted < opt.min_blocks) {
    std::fprintf(stderr, "REFUS : plancher de blocs separes (%lld < %lld)\n", led.blocks_emitted,
                 opt.min_blocks);
    return 3;
  }
  if (led.closed_directed[2] < opt.min_closed_q4) {
    std::fprintf(stderr, "REFUS : plancher de fermeture q4 (%lld < %lld)\n",
                 led.closed_directed[2], opt.min_closed_q4);
    return 3;
  }
  // LE COEUR VIDE EST UNE MESURE, PAS UN ECHEC. Q7 impose de le compter : deux
  // amas separes ont un coeur parfaitement defini et parfaitement vide.
  if (led.underfull[2] < opt.min_cores_empty) {
    std::fprintf(stderr, "REFUS : plancher de coeurs sous-pleins q4 (%lld < %lld)\n",
                 led.underfull[2], opt.min_cores_empty);
    return 3;
  }
  if (led.residual_directed[2] < opt.min_residuel) {
    std::fprintf(stderr, "REFUS : plancher de residuel q4 (%lld < %lld)\n",
                 led.residual_directed[2], opt.min_residuel);
    return 3;
  }
  return 0;
}
