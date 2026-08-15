// MorseHGP3D v3 — SUJET `counter-only` : DOMINANCE 432 A CUTOFF PONCTUEL.
//
// Specification : audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md,
// sections 2, 3 et 7 (premiere des trois voies mises en concurrence).
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CE SUJET MESURE
//
// Une seule chose : la MASSE DE CANDIDATURES D'ANCRE MAXIMALE que le cutoff de
// hauteur ferme, par lane, sans jamais examiner une paire individuellement dans
// le chemin de decision. L'identite du ledger est
//
//   maxanchor_closed_directed_q + maxanchor_residual_directed_q = n(n-1),
//
// et la fusion non ordonnee est un OU sur les deux orientations :
//
//   maxanchor_closed_PairId_q + maxanchor_residual_PairId_q = C(n,2).
//
// Le juge est `spindle_cone_oracle.cpp`, unite de traduction independante qui
// n'inclut aucun en-tete de ce sujet et reecrit son arithmetique. Il verifie
// l'inclusion `closed_q inclus dans dead_q` : toute fermeture doit correspondre
// a un vrai compte de temoins universels.
//
// AUCUN TEMPS, AUCUNE PENTE, AUCUN SLO. La boucle de MESURE du ledger est en
// n(n-1) parce qu'elle doit etre exacte a petit `n`; ce n'est PAS l'ordonnance,
// qui decide par intervalle de hauteurs. Les compteurs de travail publies sont
// ceux de la selection top-h, qui est le seul travail reel du sujet.
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

#include "prototype/cloud_families.hpp"
#include "prototype/directional_dominance.hpp"
#include "prototype/spindle_cone_oracle.hpp"

namespace {

using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::dominance::DomMutant;

namespace dom = mhgp3v::dominance;

// LE SEUIL DERIVE DE `smax`, IL N'EST PLUS FIGE.
//
// La CLI accepte `4 <= smax <= 34` mais le sujet employait `{10,9,8}`, qui ne
// vaut qu'a `smax=11`. Le contre-audit le rapporte comme P0 : a `smax=34` le
// juge refute une fermeture q4 que le sujet prononce, parce que le sujet exige
// huit temoins la ou la lane en demande trente et un. Le sujet ET le juge
// emploient donc `h = smax + 1 - q`.
inline int need_of(long long smax, int lane) { return (int)(smax + 1 - (lane + 2)); }

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(2);
}
[[noreturn]] void violate(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(3);
}

struct Ledger {
  long long classifications = 0;      // directions classees dans un sous-cone
  long long topk_insertions = 0;      // insertions reellement effectuees
  long long cells_occupied = 0;       // couples (ancre, cellule) non vides
  long long cells_underfull[3] = {0, 0, 0};  // seuil infini, par lane
  long long closed_directed[3] = {0, 0, 0};
  long long residual_directed[3] = {0, 0, 0};
  long long closed_pairid[3] = {0, 0, 0};
  long long residual_pairid[3] = {0, 0, 0};
  long long zero_direction = 0;       // positions colocalisees : preflight
  std::vector<long long> per_anchor_cells;
};

// Selection top-`kMaxNeed` par cellule, en insertion triee. Le tableau est
// reutilise d'une ancre a l'autre : c'est le seul etat vivant du sujet.
struct TopK {
  i64 tau[dom::kCells][dom::kMaxNeed];
  int count[dom::kCells];
};

void reset_topk(TopK* t) {
  for (int c = 0; c < dom::kCells; ++c) t->count[c] = 0;
}

inline void topk_push(TopK* t, int cell, i64 tau, Ledger* led) {
  int& n = t->count[cell];
  i64* row = t->tau[cell];
  if (n == dom::kMaxNeed && tau >= row[n - 1]) return;
  int pos = (n < dom::kMaxNeed) ? n : dom::kMaxNeed - 1;
  while (pos > 0 && row[pos - 1] > tau) {
    row[pos] = row[pos - 1];
    --pos;
  }
  row[pos] = tau;
  if (n < dom::kMaxNeed) ++n;
  ++led->topk_insertions;
}

struct Options {
  long long n = 200;
  long long coord = -1;
  long long seed = 3;
  long long smax = 11;
  bool judge = false;
  bool cloud_seuil = false;
  bool radial_ablation = false;
  long long judge_sample = 0;
  CloudFamily family = CloudFamily::kUniform;
  DomMutant mutant = DomMutant::kNone;
  long long min_closed_q4 = 0;
  long long min_closed_q2 = 0;
  long long min_residual = 0;
  long long min_cells_occupied = 0;
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
    else if (key == "--min-closed-q4") { if (!parse_ll(val.c_str(), &p)) refuse("--min-closed-q4 invalide"); opt.min_closed_q4 = p; }
    else if (key == "--min-closed-q2") { if (!parse_ll(val.c_str(), &p)) refuse("--min-closed-q2 invalide"); opt.min_closed_q2 = p; }
    else if (key == "--min-residuel") { if (!parse_ll(val.c_str(), &p)) refuse("--min-residuel invalide"); opt.min_residual = p; }
    else if (key == "--min-cellules") { if (!parse_ll(val.c_str(), &p)) refuse("--min-cellules invalide"); opt.min_cells_occupied = p; }
    else if (key == "--judge") { opt.judge = true; }
    else if (key == "--cloud-seuil") { opt.cloud_seuil = true; }
    else if (key == "--ablation-radiale") { opt.radial_ablation = true; }
    else if (key == "--judge-echantillon") { if (!parse_ll(val.c_str(), &p)) refuse("--judge-echantillon invalide"); opt.judge_sample = p; }
    else if (key == "--selftest") { selftest = true; }
    else if (key == "--family") {
      if (val == "uniform") opt.family = CloudFamily::kUniform;
      else if (val == "terrain") opt.family = CloudFamily::kTerrain;
      else if (val == "eight_clusters") opt.family = CloudFamily::kEightClusters;
      else if (val == "scanline_single_pass") opt.family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") opt.family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else if (key == "--inject") {
      if (val == "dom-h-moins-un") opt.mutant = DomMutant::kHMinusOne;
      else if (val == "dom-frontiere-fermee") opt.mutant = DomMutant::kBoundaryClosed;
      else if (val == "dom-cellule-voisine") opt.mutant = DomMutant::kNeighbourCell;
      else if (val == "dom-facteur-deux") opt.mutant = DomMutant::kFactorTwo;
      else refuse("--inject inconnu");
    } else {
      refuse("argument inconnu");
    }
  }

  // ---- SELFTEST DE LA PARTITION ------------------------------------------
  // Les 432 sous-cones doivent PARTITIONNER les directions : chaque direction
  // non nulle recoit exactement une cellule, et deux directions d'une meme
  // cellule verifient cos^2(gamma) >= 9/11. La seconde propriete est le coeur
  // du cutoff : sans elle le certificat radial est faux.
  if (selftest) {
    long long tested = 0, worst_num = 1, worst_den = 1;
    std::vector<std::vector<std::array<i64, 3>>> members((std::size_t)dom::kCells);
    const int span = 9;
    for (i64 x = -span; x <= span; ++x)
      for (i64 y = -span; y <= span; ++y)
        for (i64 z = -span; z <= span; ++z) {
          const int c = dom::cell_of(x, y, z);
          if (x == 0 && y == 0 && z == 0) {
            if (c != -1) { std::fprintf(stderr, "PARTITION FAUSSE : le vecteur nul recoit %d\n", c); return 1; }
            continue;
          }
          if (c < 0 || c >= dom::kCells) {
            std::fprintf(stderr, "PARTITION FAUSSE : (%lld,%lld,%lld) -> %d\n", (long long)x,
                         (long long)y, (long long)z, c);
            return 1;
          }
          members[(std::size_t)c].push_back(std::array<i64, 3>{x, y, z});
          ++tested;
        }
    // Angle maximal a l'interieur de chaque cellule, par produits croises
    // entiers : cos^2 = (u.v)^2 / (|u|^2 |v|^2) doit rester >= 9/11.
    long long non_vides = 0;
    for (int c = 0; c < dom::kCells; ++c) {
      const auto& v = members[(std::size_t)c];
      if (!v.empty()) ++non_vides;
      for (std::size_t i = 0; i < v.size(); ++i)
        for (std::size_t j = i + 1; j < v.size(); ++j) {
          const i64 d = v[i][0] * v[j][0] + v[i][1] * v[j][1] + v[i][2] * v[j][2];
          if (d <= 0) {
            std::fprintf(stderr, "CELLULE %d : produit scalaire non positif\n", c);
            return 1;
          }
          const mhgp::i128 num = (mhgp::i128)d * d * 11;
          const i64 n1 = v[i][0] * v[i][0] + v[i][1] * v[i][1] + v[i][2] * v[i][2];
          const i64 n2 = v[j][0] * v[j][0] + v[j][1] * v[j][1] + v[j][2] * v[j][2];
          const mhgp::i128 den = (mhgp::i128)n1 * n2 * 9;
          if (num < den) {
            std::fprintf(stderr,
                         "CELLULE %d : cos^2 = %lld^2/(%lld*%lld) < 9/11 entre "
                         "(%lld,%lld,%lld) et (%lld,%lld,%lld)\n",
                         c, (long long)d, (long long)n1, (long long)n2, (long long)v[i][0],
                         (long long)v[i][1], (long long)v[i][2], (long long)v[j][0],
                         (long long)v[j][1], (long long)v[j][2]);
            return 1;
          }
        }
    }
    (void)worst_num; (void)worst_den;
    if (non_vides != dom::kCells) {
      std::fprintf(stderr, "REFUS : %lld cellules sur %d sont vides a span=%d\n",
                   (long long)(dom::kCells - non_vides), dom::kCells, span);
      return 3;
    }
    // FIXTURE DE STRICTE INEGALITE, gravee par le contre-audit Q2.
    //   a=(100,100,100), z=a+6*(3,1,1), b=a+11*(3,0,0)
    // donne tau(s)=18, tau(d)=33, donc un rapport de hauteur exactement 11/6 sur
    // le type de cellule `U00`. Le predicat spindle exact y rend
    // H=198 et R=2*198^2 : `2H^2 > R` est FAUX de justesse, et `z` n'est donc
    // PAS temoin universel q4. Le certificat direct doit rendre une marge
    // EXACTEMENT NULLE et refuser. Un `>=` la creditrait.
    const mhgp::i128 marge = dom::cutoff_margin(0, 2, 33, 18);
    const bool ferme = dom::cutoff_closes(0, 2, 33, 18);
    if (marge != 0 || ferme) {
      std::fprintf(stderr,
                   "FIXTURE STRICTE REFUTEE : marge=%lld (attendu 0), ferme=%d (attendu 0)\n",
                   (long long)marge, (int)ferme);
      return 3;
    }
    // Le meme point, une unite de hauteur plus loin, doit fermer : sans cela la
    // fixture ne prouverait que l'inertie du predicat.
    if (!dom::cutoff_closes(0, 2, 34, 18)) {
      std::fprintf(stderr, "FIXTURE STRICTE REFUTEE : x=34 devrait fermer\n");
      return 3;
    }
    std::printf("selftest accord=OUI directions=%lld cellules=%d toutes_non_vides=OUI "
                "cos2_min>=9/11=OUI frontiere_stricte_marge=0\n",
                tested, dom::kCells);
    return 0;
  }

  if (opt.smax < 4 || opt.smax > 34) refuse("--smax hors du domaine d'enveloppe [4, 34]");
  // Le tampon top-h est dimensionne pour `smax=11`. Un `smax` plus grand exige
  // un besoin plus grand que le tampon ne peut porter : REFUS, jamais une
  // troncature silencieuse qui fermerait a huit ce que la lane veut a trente.
  if (need_of(opt.smax, 0) > dom::kMaxNeed)
    refuse("--smax exige un top-h superieur au tampon : refus plutot que troncature");
  if (opt.n < 4 || opt.n > 60000) refuse("--points hors bornes [4, 60000]");
  if (opt.judge && opt.n > 400) refuse("le juge ponctuel exhaustif est borne a --points <= 400");
  if (opt.mutant != DomMutant::kNone && !opt.judge && opt.judge_sample <= 0)
    refuse("un mutant sans juge ne prouve rien : --inject exige --judge ou --judge-echantillon");
  if (opt.judge_sample < 0) refuse("--judge-echantillon negatif");
  if (opt.coord < 0) opt.coord = mhgp3v::cloud_family_default_coord(opt.family, (int)opt.n);
  if (opt.coord < 2 || opt.coord > 65536) refuse("--coord hors bornes");

  // NUAGE GRAVE DU SEUIL DE TEMOINS.
  //
  // Aucune famille generique n'arme le mutant `dom-h-moins-un` : la ou le cutoff
  // ferme, la paire possede en general BIEN PLUS que le seuil de temoins
  // universels, et retirer un temoin au compte ne change donc aucun verdict du
  // juge. Il faut un nuage ou les temoins de la cellule sont les SEULS temoins
  // universels, et ou ils sont exactement `sept`.
  //
  //   a = (0,0,0) ; sept temoins colineaires (10,0,0) .. (70,0,0) ;
  //   la cible b = (500,0,0) ; trois points lointains hors du spindle.
  //
  // Reference : la cellule axiale contient huit entrees — les sept temoins ET
  // la cible elle-meme —, donc tau_8 = 500 et la fermeture exigerait
  // 500 >= 1,843*500. Elle ne ferme pas. C'est d'ailleurs la raison pour
  // laquelle la cible ne peut JAMAIS etre creditee comme temoin : la fermeture
  // demande tau_d >= 1,843 tau_h, or si `b` figurait parmi les `h` plus petites
  // hauteurs on aurait tau_d <= tau_h.
  //
  // Mutant `h-1` : il lit tau_7 = 70, ferme des 500 >= 129, et le juge ne
  // compte que sept temoins universels pour un seuil de huit.
  const std::vector<P3> pts =
      opt.cloud_seuil
          ? std::vector<P3>{P3{0, 0, 0},   P3{10, 0, 0},  P3{20, 0, 0},   P3{30, 0, 0},
                            P3{40, 0, 0}, P3{50, 0, 0},  P3{60, 0, 0},   P3{70, 0, 0},
                            P3{500, 0, 0}, P3{100, 400, 0}, P3{100, 0, 400}, P3{400, 400, 400}}
          : mhgp3v::make_family_cloud(opt.family, (int)opt.n, (int)opt.coord, opt.seed);
  if (!opt.cloud_seuil && (long long)pts.size() != opt.n)
    refuse("le generateur n'a pas rendu la cardinalite demandee");
  const int n = (int)pts.size();

  Ledger led{};
  // DEUX STOCKAGES, ET PAS UN DE PLUS.
  //
  // Le bitset ORDONNE `n^2` ne sert qu'au juge, donc il n'est alloue que sous
  // `--judge`, ou `n <= 400`. Le bitset des `PairId` NON ORDONNES est en
  // revanche necessaire a la fusion OU/ET a toute taille : il est stocke sur le
  // triangle superieur compacte, `n(n-1)/2` BITS par lane — soit 468 Mo a
  // `n=50 000` pour les trois lanes, contre 7,5 Go si l'on gardait un octet par
  // paire ordonnee. C'est cette difference qui avait fait tuer la campagne
  // `n=20 000` par le systeme.
  const long long unordered_pairs = (long long)n * ((long long)n - 1) / 2;
  std::vector<std::vector<unsigned long long>> pair_closed(3);
  for (int q = 0; q < 3; ++q)
    pair_closed[(std::size_t)q].assign((std::size_t)((unordered_pairs + 63) / 64), 0ULL);
  auto pair_index = [n](int a, int b) -> long long {
    const int lo = a < b ? a : b, hi = a < b ? b : a;
    return (long long)lo * (2LL * n - lo - 1) / 2 + (hi - lo - 1);
  };
  auto pair_set = [&](int q, int a, int b) {
    const long long k = pair_index(a, b);
    pair_closed[(std::size_t)q][(std::size_t)(k >> 6)] |= 1ULL << (k & 63);
  };
  auto pair_test = [&](int q, long long k) -> bool {
    return (pair_closed[(std::size_t)q][(std::size_t)(k >> 6)] & (1ULL << (k & 63))) != 0ULL;
  };
  std::vector<std::vector<unsigned char>> closed(3);
  if (opt.judge)
    for (int q = 0; q < 3; ++q) closed[(std::size_t)q].assign((std::size_t)n * (std::size_t)n, 0);
  // Reservoir de fermetures dirigees pour le controle ECHANTILLONNE. Il est
  // borne par `--judge-echantillon` et rempli par pas regulier sur le flux, ce
  // qui le rend deterministe et independant du scheduling.
  // Reservoir ADVERSARIAL : on garde les `K` fermetures de plus petite marge.
  struct Cand { mhgp::i128 margin; int a, b, lane; };
  std::vector<Cand> sample;
  long long closed_seen = 0;
  auto sample_push = [&](mhgp::i128 margin, int a, int b, int lane) {
    if ((long long)sample.size() < opt.judge_sample) {
      sample.push_back(Cand{margin, a, b, lane});
      std::push_heap(sample.begin(), sample.end(),
                     [](const Cand& x, const Cand& y) { return x.margin < y.margin; });
      return;
    }
    if (margin >= sample.front().margin) return;
    std::pop_heap(sample.begin(), sample.end(),
                  [](const Cand& x, const Cand& y) { return x.margin < y.margin; });
    sample.back() = Cand{margin, a, b, lane};
    std::push_heap(sample.begin(), sample.end(),
                   [](const Cand& x, const Cand& y) { return x.margin < y.margin; });
  };

  TopK* top = new TopK();
  TopK* top_ref = new TopK();
  for (int a = 0; a < n; ++a) {
    reset_topk(top);
    reset_topk(top_ref);
    // PASSE 1 : selection top-h par cellule. C'est le seul travail reel.
    for (int z = 0; z < n; ++z) {
      if (z == a) continue;
      const int cell = dom::cell_of(pts[(std::size_t)a], pts[(std::size_t)z], opt.mutant);
      ++led.classifications;
      if (cell < 0) { ++led.zero_direction; continue; }
      const i64 tau = dom::height((i64)pts[(std::size_t)z].x - (i64)pts[(std::size_t)a].x,
                                  (i64)pts[(std::size_t)z].y - (i64)pts[(std::size_t)a].y,
                                  (i64)pts[(std::size_t)z].z - (i64)pts[(std::size_t)a].z);
      topk_push(top, cell, tau, &led);
      if (opt.mutant != DomMutant::kNone) {
        Ledger sink{};
        const int cr = dom::cell_of(pts[(std::size_t)a], pts[(std::size_t)z], DomMutant::kNone);
        if (cr >= 0) topk_push(top_ref, cr, tau, &sink);
      }
    }
    long long occupied = 0;
    for (int c = 0; c < dom::kCells; ++c)
      if (top->count[c] > 0) ++occupied;
    led.cells_occupied += occupied;
    led.per_anchor_cells.push_back(occupied);

    // PASSE 2 : LEDGER. Cette boucle est la MESURE, pas l'ordonnance : le
    // certificat decide par intervalle de hauteurs, et un producteur reel
    // compterait par recherche dichotomique. La boucle exacte est ici pour que
    // le juge puisse comparer des identites a petit `n`.
    for (int b = 0; b < n; ++b) {
      if (b == a) continue;
      const int cell = dom::cell_of(pts[(std::size_t)a], pts[(std::size_t)b], opt.mutant);
      // Cellule de REFERENCE, pour le differentiel. Elle ne coute que sous
      // injection et n'entre dans aucun compteur du sujet.
      const int cell_ref = (opt.mutant == DomMutant::kNone)
                               ? cell
                               : dom::cell_of(pts[(std::size_t)a], pts[(std::size_t)b],
                                              DomMutant::kNone);
      const i64 tau_d = dom::height((i64)pts[(std::size_t)b].x - (i64)pts[(std::size_t)a].x,
                                    (i64)pts[(std::size_t)b].y - (i64)pts[(std::size_t)a].y,
                                    (i64)pts[(std::size_t)b].z - (i64)pts[(std::size_t)a].z);
      for (int q = 0; q < 3; ++q) {
        int need = need_of(opt.smax, q);
        // MUTANT : un temoin de moins suffit. Le seuil devient plus petit, donc
        // plus de cibles sont fermees, avec moins de temoins que la lane exige.
        if (opt.mutant == DomMutant::kHMinusOne) --need;
        int use_cell = cell;
        // MUTANT : le seuil vient de la cellule voisine, dont l'angle avec la
        // cible n'est pas borne par 9/11.
        if (opt.mutant == DomMutant::kNeighbourCell && use_cell >= 0)
          use_cell = (use_cell + 1) % dom::kCells;
        i64 tau_h = -1;
        if (use_cell >= 0) {
          const int have = top->count[use_cell];
          if (have >= need) tau_h = top->tau[use_cell][need - 1];
        }
        if (tau_h < 0 && cell >= 0) ++led.cells_underfull[(std::size_t)q];
        // FERMETURE DE REFERENCE, calculee avec la cellule et le seuil non
        // mutes. Le differentiel `mutant ferme, reference non` est le SEUL
        // ensemble ou une injection peut etre fausse : partout ailleurs le
        // mutant ferme ce que la reference fermait deja, et le juge y donnera
        // toujours raison. Un echantillon uniforme, ou meme un echantillon a
        // plus petite marge, ne rencontre jamais ce differentiel.
        bool ref_closes = true;
        if (opt.mutant != DomMutant::kNone) {
          i64 tau_h_ref = -1;
          if (cell_ref >= 0 && top_ref->count[cell_ref] >= need_of(opt.smax, q))
            tau_h_ref = top_ref->tau[cell_ref][need_of(opt.smax, q) - 1];
          ref_closes = dom::cutoff_closes(cell_ref, q, tau_d, tau_h_ref, DomMutant::kNone,
                                         !opt.radial_ablation);
        }
        if (dom::cutoff_closes(cell, q, tau_d, tau_h, opt.mutant, !opt.radial_ablation)) {
          ++led.closed_directed[(std::size_t)q];
          pair_set(q, a, b);
          if (opt.judge)
            closed[(std::size_t)q][(std::size_t)a * (std::size_t)n + (std::size_t)b] = 1;
          // Sans injection, l'echantillon porte sur TOUTES les fermetures, a
          // plus petite marge. Sous injection il porte sur le seul
          // DIFFERENTIEL : partout ailleurs le mutant ferme ce que la reference
          // fermait deja, et le juge y donnerait toujours raison.
          if (opt.judge_sample > 0 && (opt.mutant == DomMutant::kNone || !ref_closes)) {
            ++closed_seen;
            sample_push(dom::cutoff_margin(use_cell, q, tau_d, tau_h, !opt.radial_ablation), a, b, q);
          }
        } else {
          ++led.residual_directed[(std::size_t)q];
        }
      }
    }
  }
  delete top;
  delete top_ref;

  // Fusion non ordonnee : fermeture par OU des deux orientations, residuel par
  // ET. Le bit est deja pose par l'une ou l'autre orientation, donc le OU est
  // implicite et le comptage se fait par popcount.
  for (int q = 0; q < 3; ++q) {
    long long set = 0;
    for (unsigned long long w : pair_closed[(std::size_t)q]) set += __builtin_popcountll(w);
    led.closed_pairid[(std::size_t)q] = set;
    led.residual_pairid[(std::size_t)q] = unordered_pairs - set;
  }
  (void)pair_test;

  std::sort(led.per_anchor_cells.begin(), led.per_anchor_cells.end());
  auto pct = [&](double p) -> long long {
    if (led.per_anchor_cells.empty()) return 0;
    return led.per_anchor_cells[(std::size_t)(p * (double)(led.per_anchor_cells.size() - 1))];
  };

  std::printf("DirectionalDominanceReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=proposition_math_non_recue "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%d coord=%lld seed=%lld smax=%lld cellules=%d inject=%s\n",
              mhgp3v::cloud_family_name(opt.family), n, opt.coord, opt.seed, opt.smax,
              dom::kCells, dom::dom_mutant_name(opt.mutant));
  std::printf("cutoff forme=%s\n", opt.radial_ablation ? "radiale_tabulee_ablation" : "directe_spindle");
  std::printf("travail classifications=%lld insertions_topk=%lld cellules_occupees=%lld "
              "directions_nulles=%lld\n",
              led.classifications, led.topk_insertions, led.cells_occupied, led.zero_direction);
  std::printf("cellules_par_ancre p50=%lld p90=%lld p99=%lld max=%lld\n", pct(0.50), pct(0.90),
              pct(0.99), led.per_anchor_cells.empty() ? 0 : led.per_anchor_cells.back());
  const long long ordered = (long long)n * ((long long)n - 1);
  const long long unordered = (long long)n * ((long long)n - 1) / 2;
  for (int q = 0; q < 3; ++q) {
    std::printf("lane q%d dirige_ferme=%lld dirige_residuel=%lld PairId_ferme=%lld "
                "PairId_residuel=%lld cellules_sous_pleines=%lld\n",
                q + 2, led.closed_directed[(std::size_t)q], led.residual_directed[(std::size_t)q],
                led.closed_pairid[(std::size_t)q], led.residual_pairid[(std::size_t)q],
                led.cells_underfull[(std::size_t)q]);
    if (led.closed_directed[(std::size_t)q] + led.residual_directed[(std::size_t)q] != ordered)
      violate("identite de masse dirigee violee");
    if (led.closed_pairid[(std::size_t)q] + led.residual_pairid[(std::size_t)q] != unordered)
      violate("identite de masse PairId violee");
  }

  int disagreements = 0;
  // CONTROLE ECHANTILLONNE. Le juge exhaustif est en O(n^3) et ne peut pas
  // atteindre les tailles ou ce certificat mord. Chaque fermeture tiree est
  // rejouee en O(n) par l'unite oracle independante, qui recompte les temoins
  // universels de la lane. UN ECHANTILLON N'EST PAS UNE PREUVE DE COMPLETUDE :
  // il ne peut que REFUTER une fermeture, jamais certifier les autres.
  if (opt.judge_sample > 0) {
    std::vector<int> xyz;
    xyz.reserve((std::size_t)n * 3);
    for (const P3& p : pts) { xyz.push_back((int)p.x); xyz.push_back((int)p.y); xyz.push_back((int)p.z); }
    long long wrong = 0, printed = 0;
    mhgp::i128 marge_max = 0;
    for (const auto& cd : sample) {
      long long c[3];
      mhgp3v::cone_oracle::count_witnesses(xyz, n, cd.a, cd.b, c);
      const long long need = (long long)need_of(opt.smax, cd.lane);
      if (cd.margin > marge_max) marge_max = cd.margin;
      if (c[cd.lane] < need) {
        ++wrong;
        if (printed < 8) {
          std::fprintf(stderr,
                       "DESACCORD q%d : la candidature (%d,%d) est fermee, mais le juge n'y "
                       "compte que %lld temoins universels pour un seuil de %lld\n",
                       cd.lane + 2, cd.a, cd.b, c[cd.lane], need);
          ++printed;
        }
      }
    }
    std::printf("juge_echantillon tires=%zu sur %lld fermetures_differentielles marge_max_tiree=%lld "
                "desaccords=%lld temoins_evalues=%lld accord=%s\n",
                sample.size(), closed_seen, (long long)marge_max, wrong,
                mhgp3v::cone_oracle::last_witness_evaluations(), wrong == 0 ? "OUI" : "NON");
    disagreements += (int)wrong;
  }
  if (opt.judge) {
    std::vector<int> xyz;
    xyz.reserve((std::size_t)n * 3);
    for (const P3& p : pts) { xyz.push_back((int)p.x); xyz.push_back((int)p.y); xyz.push_back((int)p.z); }
    const mhgp3v::cone_oracle::LaneTruth t = mhgp3v::cone_oracle::judge(xyz, n, opt.smax);
    if (t.refused) { std::fprintf(stderr, "REFUS : juge — %s\n", t.refusal); return 2; }
    const std::vector<unsigned char>* verite[3] = {&t.dead_q2, &t.dead_q3, &t.dead_q4};
    long long wrong[3] = {0, 0, 0}, agreed[3] = {0, 0, 0}, printed = 0;
    for (int q = 0; q < 3; ++q)
      for (int a = 0; a < n; ++a)
        for (int b = 0; b < n; ++b) {
          if (b == a) continue;
          const std::size_t k = (std::size_t)a * (std::size_t)n + (std::size_t)b;
          if (closed[(std::size_t)q][k] == 0) continue;
          if ((*verite[q])[k] == 0) {
            if (printed < 8) {
              std::fprintf(stderr,
                           "DESACCORD q%d : la candidature (%d,%d) est fermee par le cutoff "
                           "mais le juge n'y compte pas assez de temoins universels\n",
                           q + 2, a, b);
              ++printed;
            }
            ++wrong[q];
          } else {
            ++agreed[q];
          }
        }
    std::printf("juge accord_q2=%lld accord_q3=%lld accord_q4=%lld desaccords=%lld/%lld/%lld "
                "temoins_evalues=%lld accord=%s\n",
                agreed[0], agreed[1], agreed[2], wrong[0], wrong[1], wrong[2],
                mhgp3v::cone_oracle::last_witness_evaluations(),
                (wrong[0] + wrong[1] + wrong[2]) == 0 ? "OUI" : "NON");
    disagreements = (int)(wrong[0] + wrong[1] + wrong[2]);
  }

  if (opt.mutant != DomMutant::kNone) {
    if (disagreements > 0) {
      std::fprintf(stderr, "MUTANT TUE (juge) : %s\n", dom::dom_mutant_name(opt.mutant));
      return 4;
    }
    violate("MUTANT SURVIVANT : la porte est vacueuse pour cette injection");
  }
  if (disagreements > 0) return 1;

  if (led.closed_directed[2] < opt.min_closed_q4) {
    std::fprintf(stderr, "REFUS : plancher de fermeture q4 (%lld < %lld)\n",
                 led.closed_directed[2], opt.min_closed_q4);
    return 3;
  }
  if (led.closed_directed[0] < opt.min_closed_q2) {
    std::fprintf(stderr, "REFUS : plancher de fermeture q2 (%lld < %lld)\n",
                 led.closed_directed[0], opt.min_closed_q2);
    return 3;
  }
  // UN CERTIFICAT QUI FERME TOUT EST FAUX. Ce plancher-ci porte sur ce qui
  // RESTE : sans lui, un mutant qui fermerait la totalite de la masse passerait
  // tous les autres planchers.
  if (led.residual_directed[2] < opt.min_residual) {
    std::fprintf(stderr, "REFUS : plancher de residuel q4 (%lld < %lld)\n",
                 led.residual_directed[2], opt.min_residual);
    return 3;
  }
  if (led.cells_occupied < opt.min_cells_occupied) {
    std::fprintf(stderr, "REFUS : plancher de cellules occupees (%lld < %lld)\n",
                 led.cells_occupied, opt.min_cells_occupied);
    return 3;
  }
  return 0;
}
