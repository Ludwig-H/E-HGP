// MorseHGP3D v3 — SUJET `counter-only` : GROUPES CONIQUES DE TROIS TEMOINS.
//
// Specification : audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md
// section 4 (deuxieme des trois voies mises en concurrence).
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LA QUESTION QUE CE SUJET TRANCHE
//
// Un temoin PONCTUEL doit etre interieur a toute sphere admissible. Un GROUPE de
// trois sites peut couvrir tout le pinceau sans qu'aucun de ses membres ne le
// fasse seul. La question est donc quantitative et une seule mesure y repond :
//
//   combien de candidatures qu'AUCUN temoin ponctuel ne ferme
//   sont fermees par des groupes disjoints ?
//
// C'est le nombre `ferme_par_groupes_seulement`. S'il est nul, la machinerie
// collective ne vaut pas son cout et la voie s'arrete ici.
//
// ---------------------------------------------------------------------------
// L'EMPAQUETAGE, ET POURQUOI IL EST PLANAIRE
//
// `d` appartient au cone positif de `s_1,s_2,s_3` si et seulement si, dans le
// plan orthogonal a `d`, l'origine est combinaison positive non triviale de
// leurs projections. En effet, si `d = somme alpha_i s_i` avec `alpha_i >= 0`,
// la projection donne `0 = somme alpha_i p_i`; reciproquement, comme (H2)
// impose `d.s_i > ||s_i||^2 >= 0`, toute combinaison positive non triviale
// annulant les `p_i` se remet a l'echelle pour rendre la composante selon `d`.
//
// La condition planaire est classique : les trois projections ne tiennent dans
// AUCUN demi-plan ouvert, c'est-a-dire que leurs trois ecarts angulaires
// cycliques valent au plus `pi`. L'empaquetage trie donc les membres
// admissibles par angle — comparateur entier, sans arctangente — et forme les
// triples a pas regulier `(i, i+m/3, i+2m/3)`. Ce choix est canonique : il ne
// depend que de l'ordre angulaire, donc du seul nuage. Chaque triple est
// ensuite verifie par le test exact de Cramer ; un empaquetage glouton n'est
// PAS complet, et son echec ne prouve donc jamais l'absence de groupes.
//
// CODES DE SORTIE : 1 desaccords du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/cloud_families.hpp"
#include "prototype/conic_groups.hpp"
#include "prototype/spindle_cone_oracle.hpp"

namespace {

using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::conic::GroupMutant;

namespace cg = mhgp3v::conic;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(2);
}
[[noreturn]] void violate(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(3);
}

// Seuils a smax=11 : dix groupes pour q2, neuf pour q3, huit pour q4.
constexpr int kNeed[3] = {10, 9, 8};

struct Ledger {
  long long pairs = 0;
  long long admissible_members = 0;   // sites verifiant (H2)
  long long triples_tested = 0;
  long long triples_accepted = 0;
  long long singular_triples = 0;     // rang deficient : fail-open
  long long closed_groups[3] = {0, 0, 0};
  long long closed_pointwise[3] = {0, 0, 0};
  long long closed_groups_only[3] = {0, 0, 0};
  long long closed_pointwise_only[3] = {0, 0, 0};
  long long groups_hwm = 0;
  long long spheres_tested = 0;      // centres admissibles echantillonnes
  long long min_interior_seen = -1;  // pire compte d'interieurs observe
};

// Comparateur angulaire ENTIER dans le plan, sans arctangente : demi-plan
// d'abord, produit vectoriel ensuite.
struct Planar {
  i64 u = 0, v = 0;
  int id = 0;
};

inline int half_of(const Planar& p) {
  return (p.v > 0 || (p.v == 0 && p.u > 0)) ? 0 : 1;
}

inline bool angle_less(const Planar& a, const Planar& b) {
  const int ha = half_of(a), hb = half_of(b);
  if (ha != hb) return ha < hb;
  const mhgp::i128 cross = (mhgp::i128)a.u * b.v - (mhgp::i128)a.v * b.u;
  if (cross != 0) return cross > 0;
  return a.id < b.id;  // ex aequo : plus petit PointId, canonique
}

struct Options {
  long long n = 120;
  long long coord = -1;
  long long seed = 3;
  long long smax = 11;
  bool judge = false;
  CloudFamily family = CloudFamily::kEightClusters;
  GroupMutant mutant = GroupMutant::kNone;
  long long min_groups_only = 0;
  long long min_triples = 0;
  long long min_pointwise_only = 0;
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

}  // namespace

int main(int argc, char** argv) {
  Options opt{};
  bool fixture = false;
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
    else if (key == "--min-groupes-seuls") { if (!parse_ll(val.c_str(), &p)) refuse("--min-groupes-seuls invalide"); opt.min_groups_only = p; }
    else if (key == "--min-triples") { if (!parse_ll(val.c_str(), &p)) refuse("--min-triples invalide"); opt.min_triples = p; }
    else if (key == "--min-ponctuel-seul") { if (!parse_ll(val.c_str(), &p)) refuse("--min-ponctuel-seul invalide"); opt.min_pointwise_only = p; }
    else if (key == "--judge") { opt.judge = true; }
    else if (key == "--fixture") { fixture = true; }
    else if (key == "--family") {
      if (val == "uniform") opt.family = CloudFamily::kUniform;
      else if (val == "terrain") opt.family = CloudFamily::kTerrain;
      else if (val == "eight_clusters") opt.family = CloudFamily::kEightClusters;
      else if (val == "scanline_single_pass") opt.family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") opt.family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else if (key == "--inject") {
      if (val == "groupe-espace-lineaire") opt.mutant = GroupMutant::kLinearSpan;
      else if (val == "groupe-egalite-puissance") opt.mutant = GroupMutant::kAcceptEquality;
      else if (val == "groupe-pointid-reutilise") opt.mutant = GroupMutant::kReusePointId;
      else if (val == "groupe-axe-seul") opt.mutant = GroupMutant::kAxisOnly;
      else if (val == "groupe-determinant-nul") opt.mutant = GroupMutant::kSingularOk;
      else refuse("--inject inconnu");
    } else {
      refuse("argument inconnu");
    }
  }

  // ---- FIXTURE GRAVEE DU CONTRE-AUDIT ------------------------------------
  // a=(100,100,100), b=(120,100,100), s1=(2,3,0), s2=(2,-3,3), s3=(2,-3,-3),
  // lambda=(1/2,1/4,1/4). Le barycentre pondere des trois `s_i` est parallele a
  // `d`, donc le groupe couvre tout le pinceau. AUCUN des trois n'est pourtant
  // un temoin universel q3 ou q4 : c'est exactement le cas que le certificat
  // ponctuel manque.
  if (fixture) {
    const i64 d[3] = {20, 0, 0};
    const i64 s1[3] = {2, 3, 0}, s2[3] = {2, -3, 3}, s3[3] = {2, -3, -3};
    // Le barycentre : (1/2)s1 + (1/4)s2 + (1/4)s3 = (2,0,0), parallele a d.
    i64 bary[3];
    for (int k = 0; k < 3; ++k) bary[k] = 2 * s1[k] + s2[k] + s3[k];  // *4
    const bool parallele = (bary[1] == 0 && bary[2] == 0 && bary[0] > 0);
    const bool couvre = cg::group_covers(d, s1, s2, s3);
    // Le certificat ponctuel : chaque `s_i` est-il universel q4 ? Le juge
    // independant le dit, sur le nuage a trois points {a, b, z_i}.
    int universels = 0;
    for (const i64* s : {s1, s2, s3}) {
      const std::vector<int> xyz = {100, 100, 100, 120, 100, 100,
                                    (int)(100 + s[0]), (int)(100 + s[1]), (int)(100 + s[2])};
      long long c[3];
      mhgp3v::cone_oracle::count_witnesses(xyz, 3, 0, 1, c);
      if (c[2] > 0) ++universels;  // lane q4
    }
    const bool ok = parallele && couvre && universels == 0;
    std::printf("fixture groupe-amas parallele=%d couvre=%d universels_q4=%d %s\n",
                (int)parallele, (int)couvre, universels, ok ? "OK" : "FAUX");
    if (!ok) {
      std::fprintf(stderr, "REFUS : la fixture gravee du groupe conique est refutee\n");
      return 3;
    }
    // Le mutant `espace lineaire` doit accepter un triple dont `d` n'est PAS
    // dans le cone positif : (2,3,0), (2,-3,3), (-9,0,0) engendrent l'espace,
    // mais le troisieme pointe a l'oppose de `d`.
    const i64 t3[3] = {-9, 0, 0};
    const bool ref = cg::cone_contains(s1, s2, t3, d, GroupMutant::kNone);
    const bool mut = cg::cone_contains(s1, s2, t3, d, GroupMutant::kLinearSpan);
    if (ref || !mut) {
      std::fprintf(stderr, "REFUS : le mutant d'espace lineaire est inerte sur sa fixture\n");
      return 3;
    }
    std::printf("fixtures accord=OUI refutees=0\n");
    return 0;
  }

  if (opt.smax < 4 || opt.smax > 34) refuse("--smax hors du domaine d'enveloppe [4, 34]");
  if (opt.n < 12 || opt.n > 400) refuse("--points hors bornes [12, 400]");
  if (opt.mutant != GroupMutant::kNone && !opt.judge)
    refuse("un mutant sans juge ne prouve rien : --inject exige --judge");
  if (opt.coord < 0) opt.coord = mhgp3v::cloud_family_default_coord(opt.family, (int)opt.n);
  if (opt.coord < 2 || opt.coord > 65536) refuse("--coord hors bornes");

  const std::vector<P3> pts =
      mhgp3v::make_family_cloud(opt.family, (int)opt.n, (int)opt.coord, opt.seed);
  if ((long long)pts.size() != opt.n)
    refuse("le generateur n'a pas rendu la cardinalite demandee");
  const int n = (int)pts.size();

  std::vector<int> xyz;
  xyz.reserve((std::size_t)n * 3);
  for (const P3& p : pts) { xyz.push_back((int)p.x); xyz.push_back((int)p.y); xyz.push_back((int)p.z); }

  Ledger led{};
  std::vector<Planar> members;
  std::vector<i64> sx((std::size_t)n * 3);
  int disagreements = 0;
  long long printed = 0;

  for (int a = 0; a < n; ++a) {
    for (int b = 0; b < n; ++b) {
      if (b == a) continue;
      ++led.pairs;
      const i64 d[3] = {(i64)pts[(std::size_t)b].x - (i64)pts[(std::size_t)a].x,
                        (i64)pts[(std::size_t)b].y - (i64)pts[(std::size_t)a].y,
                        (i64)pts[(std::size_t)b].z - (i64)pts[(std::size_t)a].z};
      // Base entiere du plan orthogonal a `d` : `e1 = d x axe`, l'axe choisi
      // etant celui de plus petite composante en valeur absolue, ce qui rend
      // `e1` non nul ; puis `e2 = d x e1`.
      int axis = 0;
      for (int k = 1; k < 3; ++k)
        if ((d[k] < 0 ? -d[k] : d[k]) < (d[axis] < 0 ? -d[axis] : d[axis])) axis = k;
      i64 ax[3] = {0, 0, 0};
      ax[axis] = 1;
      const i64 e1[3] = {d[1] * ax[2] - d[2] * ax[1], d[2] * ax[0] - d[0] * ax[2],
                         d[0] * ax[1] - d[1] * ax[0]};
      const i64 e2[3] = {d[1] * e1[2] - d[2] * e1[1], d[2] * e1[0] - d[0] * e1[2],
                         d[0] * e1[1] - d[1] * e1[0]};

      members.clear();
      for (int z = 0; z < n; ++z) {
        if (z == a || z == b) continue;
        const i64 s[3] = {(i64)pts[(std::size_t)z].x - (i64)pts[(std::size_t)a].x,
                          (i64)pts[(std::size_t)z].y - (i64)pts[(std::size_t)a].y,
                          (i64)pts[(std::size_t)z].z - (i64)pts[(std::size_t)a].z};
        if (!cg::member_admissible(d, s, opt.mutant)) continue;
        ++led.admissible_members;
        Planar p{};
        p.u = cg::dot3(s, e1);
        p.v = cg::dot3(s, e2);
        p.id = z;
        members.push_back(p);
        sx[(std::size_t)z * 3 + 0] = s[0];
        sx[(std::size_t)z * 3 + 1] = s[1];
        sx[(std::size_t)z * 3 + 2] = s[2];
      }
      std::sort(members.begin(), members.end(), angle_less);
      const int m = (int)members.size();
      int groups = 0;
      std::vector<unsigned char> used((std::size_t)m, 0);
      const int third = m / 3;
      for (int i = 0; i < third && groups < kNeed[0]; ++i) {
        const int i1 = i, i2 = i + third, i3 = i + 2 * third;
        // MUTANT : la reutilisation d'un `PointId` entre deux groupes. Deux
        // groupes servis par le meme point ne donnent qu'UN interieur, pas deux.
        if (opt.mutant != GroupMutant::kReusePointId &&
            (used[(std::size_t)i1] || used[(std::size_t)i2] || used[(std::size_t)i3]))
          continue;
        const i64* g1 = &sx[(std::size_t)members[(std::size_t)i1].id * 3];
        const i64* g2 = &sx[(std::size_t)members[(std::size_t)i2].id * 3];
        const i64* g3 = &sx[(std::size_t)members[(std::size_t)i3].id * 3];
        ++led.triples_tested;
        if (cg::det3(g1, g2, g3) == 0) ++led.singular_triples;
        if (!cg::group_covers(d, g1, g2, g3, opt.mutant)) continue;
        ++led.triples_accepted;
        used[(std::size_t)i1] = used[(std::size_t)i2] = used[(std::size_t)i3] = 1;
        ++groups;
      }
      if (groups > led.groups_hwm) led.groups_hwm = groups;

      long long cnt[3];
      mhgp3v::cone_oracle::count_witnesses(xyz, n, a, b, cnt);
      for (int q = 0; q < 3; ++q) {
        const bool by_groups = groups >= kNeed[q];
        const bool by_points = cnt[q] >= (long long)kNeed[q];
        if (by_groups) ++led.closed_groups[(std::size_t)q];
        if (by_points) ++led.closed_pointwise[(std::size_t)q];
        if (by_groups && !by_points) ++led.closed_groups_only[(std::size_t)q];
        if (by_points && !by_groups) ++led.closed_pointwise_only[(std::size_t)q];
        // LE JUGE. Un groupe garantit UN interieur par sphere admissible, donc
        // `groups` interieurs distincts dans CHAQUE sphere. La lane est alors
        // morte, et le juge independant doit compter au moins `kNeed[q]`
        // temoins... NON : il compte les temoins UNIVERSELS, et un groupe n'en
        // fournit aucun. La verification correcte est donc l'inverse : le juge
        // ne peut pas REFUTER une fermeture par groupes, il peut seulement
        // confirmer que le certificat ponctuel, lui, ne la voyait pas.
        (void)q;
      }
      // ---- LE FALSIFICATEUR DU THEOREME LUI-MEME -----------------------
      //
      // Le juge de Cramer ci-dessous ne verifie que l'hypothese (H1). Il ne
      // teste PAS la conclusion : « `G` groupes disjoints donnent `G` interieurs
      // distincts dans CHAQUE sphere admissible ». Cette conclusion est le
      // theoreme, et elle doit pouvoir etre refutee.
      //
      // Les centres admissibles sont exactement `c = (a+b)/2 + t` avec `t.d=0`.
      // On en echantillonne le long de la base entiere `e1`,`e2` du plan, avec
      // des magnitudes couvrant plusieurs ordres — y compris negatives et tres
      // grandes, ou la sphere devient un demi-espace. Pour chacune, le nombre de
      // points STRICTEMENT interieurs est compte ; il doit valoir au moins
      // `groups`. Une seule violation refute le theoreme ou son implementation.
      //
      // La puissance de `z` au centre `c = a + d/2 + t` vaut
      // `||s||^2 - d.s - 2 s.t`, entierement entiere. Interieur strict signifie
      // qu'elle est negative.
      if (opt.judge && groups > 0) {
        static const int kMag[] = {0, 1, -1, 3, -3, 11, -11, 97, -97, 1009, -1009};
        for (int base = 0; base < 2; ++base) {
          const i64* e = (base == 0) ? e1 : e2;
          for (int mi = 0; mi < (int)(sizeof(kMag) / sizeof(kMag[0])); ++mi) {
            if (base == 1 && kMag[mi] == 0) continue;  // t=0 deja teste
            const i64 t[3] = {e[0] * kMag[mi], e[1] * kMag[mi], e[2] * kMag[mi]};
            long long interior = 0;
            for (int z = 0; z < n; ++z) {
              if (z == a || z == b) continue;
              const i64 sz[3] = {(i64)pts[(std::size_t)z].x - (i64)pts[(std::size_t)a].x,
                                 (i64)pts[(std::size_t)z].y - (i64)pts[(std::size_t)a].y,
                                 (i64)pts[(std::size_t)z].z - (i64)pts[(std::size_t)a].z};
              const mhgp::i128 power = (mhgp::i128)cg::dot3(sz, sz) -
                                       (mhgp::i128)cg::dot3(d, sz) -
                                       2 * (mhgp::i128)cg::dot3(sz, t);
              if (power < 0) ++interior;
            }
            ++led.spheres_tested;
            if (led.min_interior_seen < 0 || interior < led.min_interior_seen)
              led.min_interior_seen = interior;
            if (interior < groups) {
              ++disagreements;
              if (printed++ < 8)
                std::fprintf(stderr,
                             "DESACCORD : (%d,%d) porte %d groupes disjoints, mais une sphere "
                             "admissible n'a que %lld interieurs stricts\n",
                             a, b, groups, interior);
            }
          }
        }
      }

      // Verification exacte de ce qui EST refutable : chaque triple accepte
      // doit reellement couvrir. Elle est rejouee ici par une seconde ecriture
      // du test conique, en resolvant `d = somme alpha_i s_i` et en verifiant
      // les signes ainsi que l'identite vectorielle.
      if (opt.judge && groups > 0) {
        for (int i = 0; i < third; ++i) {
          const int i1 = i, i2 = i + third, i3 = i + 2 * third;
          if (i3 >= m) break;
          const i64* g1 = &sx[(std::size_t)members[(std::size_t)i1].id * 3];
          const i64* g2 = &sx[(std::size_t)members[(std::size_t)i2].id * 3];
          const i64* g3 = &sx[(std::size_t)members[(std::size_t)i3].id * 3];
          if (!cg::group_covers(d, g1, g2, g3, opt.mutant)) continue;
          const i64 det = cg::det3(g1, g2, g3);
          if (det == 0) {
            ++disagreements;
            if (printed++ < 8)
              std::fprintf(stderr, "DESACCORD : triple accepte de determinant nul (%d,%d)\n", a, b);
            continue;
          }
          const i64 a1 = cg::det3(d, g2, g3), a2 = cg::det3(g1, d, g3), a3 = cg::det3(g1, g2, d);
          // Identite vectorielle : a1*g1 + a2*g2 + a3*g3 == det * d.
          for (int k = 0; k < 3; ++k) {
            const mhgp::i128 lhs = (mhgp::i128)a1 * g1[k] + (mhgp::i128)a2 * g2[k] +
                                   (mhgp::i128)a3 * g3[k];
            if (lhs != (mhgp::i128)det * d[k]) {
              ++disagreements;
              if (printed++ < 8)
                std::fprintf(stderr, "DESACCORD : identite de Cramer violee sur (%d,%d)\n", a, b);
              break;
            }
          }
          const bool signs_ok = (det > 0) ? (a1 >= 0 && a2 >= 0 && a3 >= 0)
                                          : (a1 <= 0 && a2 <= 0 && a3 <= 0);
          if (!signs_ok) {
            ++disagreements;
            if (printed++ < 8)
              std::fprintf(stderr,
                           "DESACCORD : triple accepte hors du cone positif sur (%d,%d)\n", a, b);
          }
        }
      }
    }
  }

  std::printf("ConicGroupsReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=proposition_math_non_recue "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%d coord=%lld seed=%lld smax=%lld inject=%s\n",
              mhgp3v::cloud_family_name(opt.family), n, opt.coord, opt.seed, opt.smax,
              cg::group_mutant_name(opt.mutant));
  std::printf("travail paires=%lld membres_admissibles=%lld triples_testes=%lld "
              "triples_acceptes=%lld triples_singuliers=%lld groupes_hwm=%lld\n",
              led.pairs, led.admissible_members, led.triples_tested, led.triples_accepted,
              led.singular_triples, led.groups_hwm);
  std::printf("falsificateur spheres_testees=%lld interieurs_min_observes=%lld\n",
              led.spheres_tested, led.min_interior_seen);
  for (int q = 0; q < 3; ++q)
    std::printf("lane q%d groupes=%lld ponctuel=%lld groupes_seuls=%lld ponctuel_seul=%lld\n",
                q + 2, led.closed_groups[(std::size_t)q], led.closed_pointwise[(std::size_t)q],
                led.closed_groups_only[(std::size_t)q],
                led.closed_pointwise_only[(std::size_t)q]);
  std::printf("juge desaccords=%d accord=%s\n", disagreements,
              disagreements == 0 ? "OUI" : "NON");

  if (opt.mutant != GroupMutant::kNone) {
    if (disagreements > 0) {
      std::fprintf(stderr, "MUTANT TUE (juge) : %s\n", cg::group_mutant_name(opt.mutant));
      return 4;
    }
    violate("MUTANT SURVIVANT : la porte est vacueuse pour cette injection");
  }
  if (disagreements > 0) return 1;

  if (led.closed_groups_only[2] < opt.min_groups_only) {
    std::fprintf(stderr, "REFUS : plancher de fermetures par groupes seuls (%lld < %lld)\n",
                 led.closed_groups_only[2], opt.min_groups_only);
    return 3;
  }
  if (led.triples_accepted < opt.min_triples) {
    std::fprintf(stderr, "REFUS : plancher de triples acceptes (%lld < %lld)\n",
                 led.triples_accepted, opt.min_triples);
    return 3;
  }
  if (led.closed_pointwise_only[2] < opt.min_pointwise_only) {
    std::fprintf(stderr, "REFUS : plancher de fermetures ponctuelles seules (%lld < %lld)\n",
                 led.closed_pointwise_only[2], opt.min_pointwise_only);
    return 3;
  }
  return 0;
}
