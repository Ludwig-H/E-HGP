// MorseHGP3D v3 — SUJET : CREDITS CELLULAIRES, DECISION PAR INTERVALLE.
//
// Specification : audits/AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md, section
// « Deblocage mathematique prioritaire : credits cellulaires sans triples ».
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUI CHANGE PAR RAPPORT AU SUJET DE GROUPES
//
// Le sujet `conic_groups` formait `C(m,3)` triples par PAIRE. Celui-ci ne forme
// aucun triple par paire : il calcule, PAR CELLULE, l'evenement d'activation
// entier `X_s = floor(T ||s||^2 / m_C(s)) + 1` de chaque site, empile les
// credits disjoints par ordre d'activation, et lit le `h`-ieme seuil. Toute
// cible de la cellule dont la hauteur atteint ce seuil est fermee SANS etre
// examinee. La decision porte sur un intervalle de hauteurs, comme le cutoff de
// dominance — et c'est la seule forme qui puisse ensuite se factoriser en
// rectangles `A x B`.
//
// ---------------------------------------------------------------------------
// LE TEST D'INCLUSION CONIQUE, ET POURQUOI IL EST ICI EXHAUSTIF
//
// `C` est incluse dans `cone(G)` si et seulement si ses TROIS rayons y sont.
// Par Caratheodory, `r` appartient a `cone(G)` si et seulement s'il appartient
// au cone d'un sous-ensemble d'au plus trois membres. Ce sujet enumere donc ces
// sous-ensembles, ce qui est EXACT et borne par le cap de pool. La version
// projective par enveloppe convexe de Jarvis — sans division, orientations par
// `det(s_i,s_j,s_k)` — est le successeur qui supprime cette enumeration ; elle
// n'est pas ecrite ici, et ce sujet mesure d'abord ce que le certificat FERME.
//
// CODES DE SORTIE : 1 desaccords du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/cell_credits.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/spindle_cone_oracle.hpp"

namespace {

using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::credits::CreditMutant;

namespace cr = mhgp3v::credits;
namespace dom = mhgp3v::dominance;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(2);
}
[[noreturn]] void violate(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(3);
}

constexpr int kNeed[3] = {10, 9, 8};


struct Ledger {
  long long anchors = 0;
  long long cells_active = 0;         // couples (ancre, cellule) a pool non vide
  long long activations = 0;          // sites actives, tous couples confondus
  long long cone_tests = 0;           // tests d'inclusion conique
  long long credits_emitted = 0;
  long long credits_hwm = 0;
  long long closed_directed[3] = {0, 0, 0};
  long long residual_directed[3] = {0, 0, 0};
  long long carrier_rank[4] = {0, 0, 0, 0};  // 1, 2 ou 3 membres par rayon
};

// `r` appartient-il au cone positif de `G` ? Rend le carrier minimal trouve,
// de rang un, deux ou trois. Un carrier de rang un ou deux est LEGITIME : le
// contre-audit l'indique explicitement.
bool ray_in_cone(const std::vector<std::array<i64, 3>>& pool, const std::vector<int>& avail,
                 const i64 r[3], std::vector<int>* carrier, Ledger* led) {
  carrier->clear();
  const int m = (int)avail.size();
  // Rang un : `r` est un multiple positif d'un membre.
  for (int i = 0; i < m; ++i) {
    const i64* s = pool[(std::size_t)avail[(std::size_t)i]].data();
    ++led->cone_tests;
    const i64 cx = s[1] * r[2] - s[2] * r[1];
    const i64 cy = s[2] * r[0] - s[0] * r[2];
    const i64 cz = s[0] * r[1] - s[1] * r[0];
    if (cx == 0 && cy == 0 && cz == 0 && cr::dot3(s, r) > 0) {
      carrier->push_back(avail[(std::size_t)i]);
      ++led->carrier_rank[1];
      return true;
    }
  }
  // Rang deux : `r` est dans le secteur plan de deux membres.
  for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j) {
      const i64* a = pool[(std::size_t)avail[(std::size_t)i]].data();
      const i64* b = pool[(std::size_t)avail[(std::size_t)j]].data();
      ++led->cone_tests;
      if (cr::det3(a, b, r) != 0) continue;  // `r` hors du plan de `a`,`b`
      // Coordonnees dans le plan : signes des determinants avec la normale.
      const i64 nx = a[1] * b[2] - a[2] * b[1];
      const i64 ny = a[2] * b[0] - a[0] * b[2];
      const i64 nz = a[0] * b[1] - a[1] * b[0];
      const i64 nrm[3] = {nx, ny, nz};
      if (nx == 0 && ny == 0 && nz == 0) continue;  // colineaires
      const i64 ca = cr::det3(nrm, a, r);
      const i64 cb = cr::det3(nrm, r, b);
      if (ca >= 0 && cb >= 0) {
        carrier->push_back(avail[(std::size_t)i]);
        carrier->push_back(avail[(std::size_t)j]);
        ++led->carrier_rank[2];
        return true;
      }
    }
  // Rang trois : les signes de Cramer, exactement comme le certificat de groupe.
  for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j)
      for (int k = j + 1; k < m; ++k) {
        const i64* a = pool[(std::size_t)avail[(std::size_t)i]].data();
        const i64* b = pool[(std::size_t)avail[(std::size_t)j]].data();
        const i64* c = pool[(std::size_t)avail[(std::size_t)k]].data();
        ++led->cone_tests;
        const i64 det = cr::det3(a, b, c);
        if (det == 0) continue;  // rang deficient : fail-open
        const i64 d1 = cr::det3(r, b, c), d2 = cr::det3(a, r, c), d3 = cr::det3(a, b, r);
        const bool ok = (det > 0) ? (d1 >= 0 && d2 >= 0 && d3 >= 0)
                                  : (d1 <= 0 && d2 <= 0 && d3 <= 0);
        if (!ok) continue;
        carrier->push_back(avail[(std::size_t)i]);
        carrier->push_back(avail[(std::size_t)j]);
        carrier->push_back(avail[(std::size_t)k]);
        ++led->carrier_rank[3];
        return true;
      }
  return false;
}

struct Options {
  long long n = 300;
  long long coord = -1;
  long long seed = 3;
  long long smax = 11;
  long long bank = 64;
  long long pool_cap = 16;
  bool hull = true;
  long long judge_sample = 0;
  CloudFamily family = CloudFamily::kEightClusters;
  CreditMutant mutant = CreditMutant::kNone;
  long long min_credits = 0;
  long long min_closed_q4 = 0;
  long long min_residuel = 0;
  long long min_rank_two = 0;
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
  bool selftest = false;
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
    else if (key == "--bank") { if (!parse_ll(val.c_str(), &p)) refuse("--bank invalide"); opt.bank = p; }
    else if (key == "--pool") { if (!parse_ll(val.c_str(), &p)) refuse("--pool invalide"); opt.pool_cap = p; }
    else if (key == "--ablation-caratheodory") { opt.hull = false; }
    else if (key == "--judge-echantillon") { if (!parse_ll(val.c_str(), &p)) refuse("--judge-echantillon invalide"); opt.judge_sample = p; }
    else if (key == "--min-credits") { if (!parse_ll(val.c_str(), &p)) refuse("--min-credits invalide"); opt.min_credits = p; }
    else if (key == "--min-closed-q4") { if (!parse_ll(val.c_str(), &p)) refuse("--min-closed-q4 invalide"); opt.min_closed_q4 = p; }
    else if (key == "--min-residuel") { if (!parse_ll(val.c_str(), &p)) refuse("--min-residuel invalide"); opt.min_residuel = p; }
    else if (key == "--min-rang-deux") { if (!parse_ll(val.c_str(), &p)) refuse("--min-rang-deux invalide"); opt.min_rank_two = p; }
    else if (key == "--selftest") { selftest = true; }
    else if (key == "--family") {
      if (val == "uniform") opt.family = CloudFamily::kUniform;
      else if (val == "terrain") opt.family = CloudFamily::kTerrain;
      else if (val == "eight_clusters") opt.family = CloudFamily::kEightClusters;
      else if (val == "scanline_single_pass") opt.family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") opt.family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else if (key == "--inject") {
      if (val == "credit-activation-frontiere") opt.mutant = CreditMutant::kActivationOff;
      else if (val == "credit-un-seul-rayon") opt.mutant = CreditMutant::kOneRayOnly;
      else if (val == "credit-ids-partages") opt.mutant = CreditMutant::kShareIds;
      else if (val == "credit-sans-positivite") opt.mutant = CreditMutant::kIgnorePositive;
      else refuse("--inject inconnu");
    } else {
      refuse("argument inconnu");
    }
  }

  // ---- SELFTEST DES RAYONS ------------------------------------------------
  //
  // « Chaque rayon appartient a sa propre cellule » serait FAUX, et c'est une
  // erreur que j'ai d'abord ecrite : les rayons sont des SOMMETS PARTAGES entre
  // cellules adjacentes, et la convention half-open en attribue chacun a une
  // seule. Le rayon `(3,1,1)` est ainsi un sommet de `U00`, de `D10` et de
  // `U11`, et la classification le donne a `U11`.
  //
  // La propriete correcte porte sur l'INTERIEUR : la somme des trois rayons est
  // strictement interieure au cone simplicial, donc elle doit se classer dans
  // sa propre cellule. C'est ce qui lie la table de rayons a la classification,
  // signes et permutation compris, sur les 432 cellules.
  if (selftest) {
    long long bad = 0;
    for (int c = 0; c < dom::kCells; ++c) {
      i64 rays[3][3];
      cr::cell_rays(c, rays);
      i64 mid[3] = {0, 0, 0};
      for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) mid[k] += rays[j][k];
        if (dom::height(rays[j][0], rays[j][1], rays[j][2]) != cr::kRayHeight) ++bad;
      }
      if (dom::cell_of(mid[0], mid[1], mid[2]) != c) ++bad;
      // Les trois rayons doivent etre affinement independants.
      if (cr::det3(rays[0], rays[1], rays[2]) == 0) ++bad;
      // Et chaque rayon doit etre dans la FERMETURE : son produit scalaire avec
      // la normale interieure de chaque face est positif ou nul.
      for (int j = 0; j < 3; ++j) {
        const int u = (j + 1) % 3, v = (j + 2) % 3;
        i64 nrm[3] = {rays[u][1] * rays[v][2] - rays[u][2] * rays[v][1],
                      rays[u][2] * rays[v][0] - rays[u][0] * rays[v][2],
                      rays[u][0] * rays[v][1] - rays[u][1] * rays[v][0]};
        const i64 sgn_in = cr::dot3(nrm, rays[j]);
        if (sgn_in == 0) ++bad;
        for (int j2 = 0; j2 < 3; ++j2) {
          const i64 d = cr::dot3(nrm, rays[j2]);
          if ((sgn_in > 0 && d < 0) || (sgn_in < 0 && d > 0)) ++bad;
        }
      }
    }
    if (bad != 0) {
      std::fprintf(stderr, "REFUS : %lld violations de la table de rayons\n", bad);
      return 3;
    }
    // L'evenement d'activation doit etre le PREMIER entier qui satisfait (H2).
    i64 rays[3][3];
    cr::cell_rays(0, rays);
    long long checked = 0;
    for (i64 x = 1; x <= 40; ++x)
      for (i64 y = 0; y <= 12; ++y)
        for (i64 z = 0; z <= 12; ++z) {
          const i64 s[3] = {x, y, z};
          const i64 X = cr::activation_height(rays, s);
          if (X < 0) continue;
          const i64 m = cr::cell_margin(rays, s);
          if ((X - 1) * m > (i64)cr::kRayHeight * cr::dot3(s, s)) {
            std::fprintf(stderr, "REFUS : activation non minimale\n");
            return 3;
          }
          if (X * m <= (i64)cr::kRayHeight * cr::dot3(s, s)) {
            std::fprintf(stderr, "REFUS : activation insuffisante\n");
            return 3;
          }
          ++checked;
        }
    // EQUIVALENCE ENVELOPPE / CARATHEODORY. La marche de Jarvis remplace une
    // enumeration exacte : elle doit rendre EXACTEMENT les memes verdicts. Le
    // test tire des pools entiers, verifie que chaque membre est bien dans le
    // demi-espace `w > 0`, puis compare les deux chemins rayon par rayon.
    unsigned long long rng = 0x9E3779B97F4A7C15ULL;
    auto nxt = [&rng](i64 lo, i64 hi) {
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      return lo + (i64)((rng >> 17) % (unsigned long long)(hi - lo + 1));
    };
    long long pools = 0, agree = 0, inside = 0;
    Ledger sink{};
    for (int it = 0; it < 4000; ++it) {
      const int c = (int)nxt(0, dom::kCells - 1);
      i64 rr[3][3];
      cr::cell_rays(c, rr);
      const i64 wsum[3] = {rr[0][0] + rr[1][0] + rr[2][0], rr[0][1] + rr[1][1] + rr[2][1],
                           rr[0][2] + rr[1][2] + rr[2][2]};
      const int m = (int)nxt(1, 12);
      std::vector<std::array<i64, 3>> pl;
      std::vector<i64> fl;
      std::vector<int> av;
      for (int i = 0; i < m; ++i) {
        i64 sv[3];
        for (int k = 0; k < 3; ++k) sv[k] = nxt(-40, 40);
        if (cr::cell_margin(rr, sv) <= 0) continue;  // hors du pool actif
        av.push_back((int)pl.size());
        pl.push_back({sv[0], sv[1], sv[2]});
        fl.push_back(sv[0]); fl.push_back(sv[1]); fl.push_back(sv[2]);
      }
      if (av.empty()) continue;
      ++pools;
      std::vector<int> hl((std::size_t)av.size() + 2, 0);
      long long tests = 0;
      const int hs = cr::projective_hull(fl.data(), av.data(), (int)av.size(), wsum, hl.data(),
                                         &tests);
      for (int j = 0; j < 3; ++j) {
        int cbuf[3], csz = 0;
        const bool by_hull =
            cr::ray_in_hull(fl.data(), av.data(), hl.data(), hs, rr[j], cbuf, &csz, &tests);
        std::vector<int> car;
        const bool by_brute = ray_in_cone(pl, av, rr[j], &car, &sink);
        if (by_hull != by_brute) {
          std::fprintf(stderr,
                       "DESACCORD ENVELOPPE : cellule %d rayon %d, enveloppe=%d brute=%d, "
                       "pool de %zu membres\n",
                       c, j, (int)by_hull, (int)by_brute, av.size());
          return 1;
        }
        ++agree;
        if (by_hull) ++inside;
      }
    }
    if (inside < 200) {
      std::fprintf(stderr, "REFUS : porte vide — seulement %lld inclusions sur %lld\n", inside,
                   agree);
      return 3;
    }
    std::printf("selftest accord=OUI cellules=%d rayons_valides=%d activations_verifiees=%lld "
                "pools=%lld accords_enveloppe=%lld inclusions=%lld\n",
                dom::kCells, dom::kCells * 3, checked, pools, agree, inside);
    return 0;
  }

  if (opt.smax < 4 || opt.smax > 34) refuse("--smax hors du domaine d'enveloppe [4, 34]");
  if (opt.n < 16 || opt.n > 4000) refuse("--points hors bornes [16, 4000]");
  if (opt.bank < 8 || opt.bank > 256) refuse("--bank hors bornes [8, 256]");
  // LE CAP DE POOL EST LA CONTRAINTE LIANTE, ET C'EST MESURE : un credit
  // consomme un a trois `PointId`, donc un pool de seize n'en produit jamais
  // plus de cinq disjoints, la ou q4 en exige huit. Le cap doit valoir au moins
  // trois fois le seuil de la lane la plus exigeante.
  if (opt.pool_cap < 3 || opt.pool_cap > 48) refuse("--pool hors bornes [3, 48]");
  if (opt.mutant != CreditMutant::kNone && opt.judge_sample <= 0)
    refuse("un mutant sans juge ne prouve rien : --inject exige --judge-echantillon");
  if (opt.coord < 0) opt.coord = mhgp3v::cloud_family_default_coord(opt.family, (int)opt.n);
  if (opt.coord < 2 || opt.coord > 65536) refuse("--coord hors bornes");

  const std::vector<P3> pts =
      mhgp3v::make_family_cloud(opt.family, (int)opt.n, (int)opt.coord, opt.seed);
  if ((long long)pts.size() != opt.n)
    refuse("le generateur n'a pas rendu la cardinalite demandee");
  const int n = (int)pts.size();

  Ledger led{};
  std::vector<std::array<int, 3>> sample;  // (a, b, lane) du controle
  long long closed_seen = 0;

  // Seuils de fermeture par (ancre, cellule) et par lane.
  std::vector<std::array<i64, 3>> thresh((std::size_t)dom::kCells);

  std::vector<std::pair<i64, int>> byd;
  std::vector<std::array<i64, 3>> pool;
  std::vector<i64> pool_X;
  std::vector<int> avail, carrier, used_ids;

  for (int a = 0; a < n; ++a) {
    ++led.anchors;
    // Banque des `M` plus proches : les credits utiles viennent des sites de
    // plus petite activation, donc des plus proches.
    byd.clear();
    for (int z = 0; z < n; ++z) {
      if (z == a) continue;
      const i64 dx = (i64)pts[(std::size_t)z].x - (i64)pts[(std::size_t)a].x;
      const i64 dy = (i64)pts[(std::size_t)z].y - (i64)pts[(std::size_t)a].y;
      const i64 dz = (i64)pts[(std::size_t)z].z - (i64)pts[(std::size_t)a].z;
      byd.push_back({dx * dx + dy * dy + dz * dz, z});
    }
    std::sort(byd.begin(), byd.end());
    const int mbank = (int)std::min<long long>(opt.bank, (long long)byd.size());

    for (int c = 0; c < dom::kCells; ++c)
      thresh[(std::size_t)c] = {-1, -1, -1};

    for (int c = 0; c < dom::kCells; ++c) {
      i64 rays[3][3];
      cr::cell_rays(c, rays);
      // Pool actif de la cellule : sites de marge positive, tries par activation.
      pool.clear();
      pool_X.clear();
      std::vector<std::pair<i64, int>> order;
      for (int t = 0; t < mbank; ++t) {
        const int z = byd[(std::size_t)t].second;
        const i64 s[3] = {(i64)pts[(std::size_t)z].x - (i64)pts[(std::size_t)a].x,
                          (i64)pts[(std::size_t)z].y - (i64)pts[(std::size_t)a].y,
                          (i64)pts[(std::size_t)z].z - (i64)pts[(std::size_t)a].z};
        const i64 X = cr::activation_height(rays, s, opt.mutant);
        if (X < 0) continue;
        order.push_back({X, (int)pool.size()});
        pool.push_back({s[0], s[1], s[2]});
        pool_X.push_back(X);
        if ((long long)pool.size() >= opt.pool_cap) break;
      }
      if (pool.empty()) continue;
      ++led.cells_active;
      led.activations += (long long)pool.size();
      std::sort(order.begin(), order.end());

      // Empilement glouton : on ajoute les sites par activation croissante et
      // on extrait un credit des que les TROIS rayons sont couverts.
      avail.clear();
      used_ids.clear();
      std::vector<i64> credit_X;
      std::vector<i64> flat;
      flat.reserve(pool.size() * 3);
      for (const auto& v : pool) { flat.push_back(v[0]); flat.push_back(v[1]); flat.push_back(v[2]); }
      const i64 wsum[3] = {rays[0][0] + rays[1][0] + rays[2][0],
                           rays[0][1] + rays[1][1] + rays[2][1],
                           rays[0][2] + rays[1][2] + rays[2][2]};
      std::vector<int> hull((std::size_t)opt.pool_cap + 2, 0);
      for (std::size_t k = 0; k < order.size() && (int)credit_X.size() < kNeed[0]; ++k) {
        avail.push_back(order[k].second);
        std::vector<int> union_ids;
        bool all = true;
        // MUTANT : un seul rayon fait passer la cellule entiere. Un groupe qui
        // ne contient qu'une direction representative ne certifie jamais toute
        // la cellule.
        const int rays_to_check = (opt.mutant == CreditMutant::kOneRayOnly) ? 1 : 3;
        int hsize = 0;
        if (opt.hull)
          hsize = cr::projective_hull(flat.data(), avail.data(), (int)avail.size(), wsum,
                                      hull.data(), &led.cone_tests);
        for (int j = 0; j < rays_to_check && all; ++j) {
          if (opt.hull) {
            int cbuf[3], csz = 0;
            if (!cr::ray_in_hull(flat.data(), avail.data(), hull.data(), hsize, rays[j], cbuf,
                                 &csz, &led.cone_tests)) { all = false; break; }
            ++led.carrier_rank[csz];
            carrier.assign(cbuf, cbuf + csz);
          } else if (!ray_in_cone(pool, avail, rays[j], &carrier, &led)) {
            all = false;
            break;
          }
          for (int id : carrier)
            if (std::find(union_ids.begin(), union_ids.end(), id) == union_ids.end())
              union_ids.push_back(id);
        }
        if (!all) continue;
        i64 xg = 0;
        for (int id : union_ids) xg = std::max(xg, pool_X[(std::size_t)id]);
        credit_X.push_back(xg);
        ++led.credits_emitted;
        // MUTANT : les credits partagent leurs `PointId`. Deux credits servis
        // par le meme site ne donnent qu'UN interieur.
        if (opt.mutant != CreditMutant::kShareIds) {
          std::vector<int> rest;
          for (int id : avail)
            if (std::find(union_ids.begin(), union_ids.end(), id) == union_ids.end())
              rest.push_back(id);
          avail.swap(rest);
        }
      }
      if ((long long)credit_X.size() > led.credits_hwm) led.credits_hwm = (long long)credit_X.size();
      std::sort(credit_X.begin(), credit_X.end());
      for (int q = 0; q < 3; ++q)
        if ((int)credit_X.size() >= kNeed[q])
          thresh[(std::size_t)c][(std::size_t)q] = credit_X[(std::size_t)kNeed[q] - 1];
    }

    // Ledger : chaque cible est comparee au seuil de SA cellule.
    for (int b = 0; b < n; ++b) {
      if (b == a) continue;
      const int c = dom::cell_of(pts[(std::size_t)a], pts[(std::size_t)b]);
      const i64 x = dom::height((i64)pts[(std::size_t)b].x - (i64)pts[(std::size_t)a].x,
                                (i64)pts[(std::size_t)b].y - (i64)pts[(std::size_t)a].y,
                                (i64)pts[(std::size_t)b].z - (i64)pts[(std::size_t)a].z);
      for (int q = 0; q < 3; ++q) {
        const i64 th = (c >= 0) ? thresh[(std::size_t)c][(std::size_t)q] : -1;
        if (th >= 0 && x >= th) {
          ++led.closed_directed[(std::size_t)q];
          if (opt.judge_sample > 0) {
            ++closed_seen;
            if ((long long)sample.size() < opt.judge_sample)
              sample.push_back(std::array<int, 3>{a, b, q});
          }
        } else {
          ++led.residual_directed[(std::size_t)q];
        }
      }
    }
  }

  std::printf("CellCreditsReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=proposition_math_non_recue "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%d coord=%lld seed=%lld smax=%lld banque=%lld pool=%lld inject=%s\n",
              mhgp3v::cloud_family_name(opt.family), n, opt.coord, opt.seed, opt.smax, opt.bank,
              opt.pool_cap, cr::credit_mutant_name(opt.mutant));
  std::printf("travail ancres=%lld cellules_actives=%lld activations=%lld tests_cone=%lld\n",
              led.anchors, led.cells_active, led.activations, led.cone_tests);
  std::printf("credits emis=%lld hwm=%lld rang1=%lld rang2=%lld rang3=%lld\n",
              led.credits_emitted, led.credits_hwm, led.carrier_rank[1], led.carrier_rank[2],
              led.carrier_rank[3]);
  const long long ordered = (long long)n * ((long long)n - 1);
  for (int q = 0; q < 3; ++q) {
    std::printf("lane q%d ferme=%lld residuel=%lld\n", q + 2, led.closed_directed[(std::size_t)q],
                led.residual_directed[(std::size_t)q]);
    if (led.closed_directed[(std::size_t)q] + led.residual_directed[(std::size_t)q] != ordered)
      violate("identite de masse dirigee violee");
  }

  // ---- LE FALSIFICATEUR DU CERTIFICAT -----------------------------------
  //
  // Un credit ne fournit AUCUN temoin universel : il fournit un interieur par
  // sphere, potentiellement different a chaque sphere. Le juge ponctuel ne peut
  // donc pas confirmer une fermeture, et l'utiliser comme tel serait une faute.
  //
  // Ce qui EST refutable, c'est la conclusion : si la lane `q` est fermee pour
  // `(a,b)`, alors CHAQUE sphere admissible doit porter au moins `kNeed[q]`
  // interieurs stricts. Les centres admissibles sont `(a+b)/2 + t` avec
  // `t.d = 0`; on en echantillonne le long d'une base entiere du plan, avec des
  // magnitudes couvrant plusieurs ordres et les deux signes. Une seule sphere
  // deficitaire refute le certificat.
  int disagreements = 0;
  if (opt.judge_sample > 0) {
    static const int kMag[] = {0, 1, -1, 5, -5, 37, -37, 401, -401};
    long long spheres = 0, wrong = 0, printed = 0, worst = -1;
    for (const auto& s3 : sample) {
      const int a = s3[0], b = s3[1], lane = s3[2];
      const i64 d[3] = {(i64)pts[(std::size_t)b].x - (i64)pts[(std::size_t)a].x,
                        (i64)pts[(std::size_t)b].y - (i64)pts[(std::size_t)a].y,
                        (i64)pts[(std::size_t)b].z - (i64)pts[(std::size_t)a].z};
      int axis = 0;
      for (int k = 1; k < 3; ++k)
        if ((d[k] < 0 ? -d[k] : d[k]) < (d[axis] < 0 ? -d[axis] : d[axis])) axis = k;
      i64 ax[3] = {0, 0, 0};
      ax[axis] = 1;
      const i64 e1[3] = {d[1] * ax[2] - d[2] * ax[1], d[2] * ax[0] - d[0] * ax[2],
                         d[0] * ax[1] - d[1] * ax[0]};
      const i64 e2[3] = {d[1] * e1[2] - d[2] * e1[1], d[2] * e1[0] - d[0] * e1[2],
                         d[0] * e1[1] - d[1] * e1[0]};
      for (int base = 0; base < 2; ++base) {
        const i64* ee = (base == 0) ? e1 : e2;
        for (int mi = 0; mi < (int)(sizeof(kMag) / sizeof(kMag[0])); ++mi) {
          if (base == 1 && kMag[mi] == 0) continue;
          const i64 t[3] = {ee[0] * kMag[mi], ee[1] * kMag[mi], ee[2] * kMag[mi]};
          long long interior = 0;
          for (int z = 0; z < n; ++z) {
            if (z == a || z == b) continue;
            const i64 sz[3] = {(i64)pts[(std::size_t)z].x - (i64)pts[(std::size_t)a].x,
                               (i64)pts[(std::size_t)z].y - (i64)pts[(std::size_t)a].y,
                               (i64)pts[(std::size_t)z].z - (i64)pts[(std::size_t)a].z};
            const mhgp::i128 power = (mhgp::i128)cr::dot3(sz, sz) - (mhgp::i128)cr::dot3(d, sz) -
                                     2 * (mhgp::i128)cr::dot3(sz, t);
            if (power < 0) ++interior;
          }
          ++spheres;
          if (worst < 0 || interior < worst) worst = interior;
          if (interior < (long long)kNeed[lane]) {
            ++wrong;
            if (printed++ < 8)
              std::fprintf(stderr,
                           "DESACCORD q%d : (%d,%d) est ferme, mais une sphere admissible n'a "
                           "que %lld interieurs stricts pour un seuil de %d\n",
                           lane + 2, a, b, interior, kNeed[lane]);
          }
        }
      }
    }
    std::printf("falsificateur paires_tirees=%zu spheres=%lld interieurs_min=%lld desaccords=%lld "
                "accord=%s\n",
                sample.size(), spheres, worst, wrong, wrong == 0 ? "OUI" : "NON");
    disagreements = (int)wrong;
  }
  if (false) {
    std::vector<int> xyz;
    xyz.reserve((std::size_t)n * 3);
    for (const P3& p : pts) { xyz.push_back((int)p.x); xyz.push_back((int)p.y); xyz.push_back((int)p.z); }
    long long wrong = 0, printed = 0;
    for (const auto& s3 : sample) {
      long long cnt[3];
      mhgp3v::cone_oracle::count_witnesses(xyz, n, s3[0], s3[1], cnt);
      // Un credit ne fournit PAS un temoin universel : il fournit un interieur
      // par sphere, potentiellement different a chaque sphere. Le juge ponctuel
      // ne peut donc pas confirmer la fermeture — il peut seulement mesurer ce
      // que le certificat ponctuel voyait deja. Ce compteur est une MESURE de
      // complementarite, pas une validation.
      if (cnt[s3[2]] < (long long)kNeed[s3[2]]) {
        ++wrong;
        if (printed++ < 4)
          std::fprintf(stderr,
                       "COMPLEMENTAIRE q%d : (%d,%d) ferme par credits, le ponctuel n'y compte "
                       "que %lld temoins universels\n",
                       s3[2] + 2, s3[0], s3[1], cnt[s3[2]]);
      }
    }
    std::printf("juge_echantillon tires=%zu sur %lld fermetures complementaires=%lld "
                "temoins_evalues=%lld\n",
                sample.size(), closed_seen, wrong,
                mhgp3v::cone_oracle::last_witness_evaluations());
  }

  if (opt.mutant != CreditMutant::kNone) {
    if (disagreements > 0) {
      std::fprintf(stderr, "MUTANT TUE (falsificateur) : %s\n",
                   cr::credit_mutant_name(opt.mutant));
      return 4;
    }
    violate("MUTANT SURVIVANT : la porte est vacueuse pour cette injection");
  }
  if (disagreements > 0) return 1;

  if (led.credits_emitted < opt.min_credits) {
    std::fprintf(stderr, "REFUS : plancher de credits (%lld < %lld)\n", led.credits_emitted,
                 opt.min_credits);
    return 3;
  }
  if (led.closed_directed[2] < opt.min_closed_q4) {
    std::fprintf(stderr, "REFUS : plancher de fermeture q4 (%lld < %lld)\n",
                 led.closed_directed[2], opt.min_closed_q4);
    return 3;
  }
  if (led.residual_directed[2] < opt.min_residuel) {
    std::fprintf(stderr, "REFUS : plancher de residuel q4 (%lld < %lld)\n",
                 led.residual_directed[2], opt.min_residuel);
    return 3;
  }
  if (led.carrier_rank[2] < opt.min_rank_two) {
    std::fprintf(stderr, "REFUS : plancher de carriers de rang deux (%lld < %lld)\n",
                 led.carrier_rank[2], opt.min_rank_two);
    return 3;
  }
  return 0;
}
