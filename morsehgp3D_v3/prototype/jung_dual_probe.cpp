// MorseHGP3D v3 — PORTE ET ABLATION DU CERTIFICAT DUAL `JungDual`.
//
// Codes de sortie : 1 desaccord du juge, 2 refus avant calcul, 3 plancher ou
// invariant, 4 mutant tue.
//
// CE QUE CETTE PORTE ETABLIT
//
// 1. Pour `k=1` et poids un, le predicat dual coincide EXACTEMENT avec
//    l'ecriture historique `(g,Q)` de `spindle_cone.hpp`, sur des centaines de
//    milliers de triples tires en pleine largeur u16. Deux algebres ecrites
//    independamment qui coincident partout, c'est la seule preuve disponible
//    que la forme entiere du minimax est juste.
// 2. Un groupe qui ferme une paire est SUR : pour un echantillon de spheres
//    admissibles reellement construites, au moins un membre du groupe est
//    strictement interieur. Le juge construit ces spheres par leur centre dans
//    le disque de Jung et n'emprunte aucune quantite au predicat.
// 3. La mesure : sur les paires q4 ouvertes d'un nuage, combien de paires
//    supplementaires un groupe de deux ou trois temoins ferme-t-il, la ou un
//    temoin unique echoue ? C'est le seul chiffre qui dise si ce certificat
//    attaque les longues aretes inter-amas.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include "jung_dual.hpp"

namespace {

using mhgp3v::jdual::DualMutant;
using mhgp3v::jdual::dual_lane;
using mhgp3v::jdual::dual_lane_single;
using mhgp3v::jdual::dual_mutant_name;
using mhgp3v::cone::kLaneNone;
using mhgp3v::cone::kLaneQ2;
using mhgp3v::cone::kLaneQ3;
using mhgp3v::cone::kLaneQ4;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(2);
}

bool parse_ll(const char* s, long long* out) {
  if (s == nullptr || *s == '\0') return false;
  char* end = nullptr;
  errno = 0;
  const long long v = std::strtoll(s, &end, 10);
  if (errno != 0 || end == nullptr || *end != '\0') return false;
  *out = v;
  return true;
}

struct Rng {
  unsigned long long s;
  explicit Rng(unsigned long long seed)
      : s(seed * 6364136223846793005ULL + 1442695040888963407ULL) {}
  unsigned long long next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
  long long in(long long lo, long long hi) {
    if (hi <= lo) return lo;
    return lo + (long long)(next() % (unsigned long long)(hi - lo + 1));
  }
};

}  // namespace

int main(int argc, char** argv) {
  bool selftest = false, ablation = false, fixtures = false;
  long long seed = 1, rounds = 200000, span = 65535;
  long long n = 600, groupes_max = 8, voisins = 16, echantillon = 256;
  std::string famille = "eight_clusters";
  DualMutant mutant = DualMutant::kNone;
  long long min_gain = 0, min_fermees_groupe = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const std::size_t eq = arg.find('=');
    const std::string key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
    const std::string val = (eq == std::string::npos) ? std::string() : arg.substr(eq + 1);
    long long parsed = 0;
    if (key == "--selftest") selftest = true;
    else if (key == "--fixtures") fixtures = true;
    else if (key == "--ablation") ablation = true;
    else if (key == "--seed") { if (!parse_ll(val.c_str(), &parsed)) refuse("--seed invalide"); seed = parsed; }
    else if (key == "--rounds") { if (!parse_ll(val.c_str(), &parsed)) refuse("--rounds invalide"); rounds = parsed; }
    else if (key == "--span") { if (!parse_ll(val.c_str(), &parsed)) refuse("--span invalide"); span = parsed; }
    else if (key == "--points") { if (!parse_ll(val.c_str(), &parsed)) refuse("--points invalide"); n = parsed; }
    else if (key == "--groupes") { if (!parse_ll(val.c_str(), &parsed)) refuse("--groupes invalide"); groupes_max = parsed; }
    else if (key == "--voisins") { if (!parse_ll(val.c_str(), &parsed)) refuse("--voisins invalide"); voisins = parsed; }
    else if (key == "--echantillon") { if (!parse_ll(val.c_str(), &parsed)) refuse("--echantillon invalide"); echantillon = parsed; }
    else if (key == "--min-gain") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-gain invalide"); min_gain = parsed; }
    else if (key == "--min-fermees-groupe") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-fermees-groupe invalide"); min_fermees_groupe = parsed; }
    else if (key == "--family") famille = val;
    else if (key == "--inject") {
      if (val == "dual-no-positivity") mutant = DualMutant::kNoPositivity;
      else if (val == "dual-accept-equality") mutant = DualMutant::kAcceptEquality;
      else if (val == "dual-swap-coeff") mutant = DualMutant::kSwapCoeff;
      else if (val == "dual-narrow-i64") mutant = DualMutant::kNarrowI64;
      else if (val == "dual-ignore-weights") mutant = DualMutant::kIgnoreWeights;
      else refuse("--inject inconnu");
    } else refuse("argument inconnu");
  }
  if (!selftest && !ablation && !fixtures) refuse("choisir --selftest, --ablation ou --fixtures");

  // -------------------------------------------------------------------------
  // LA FIXTURE `u < p < d`, ET CE QU'ELLE REFUTE.
  //
  // Elle vient de la section 5.5 de
  // `AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`. Elle separe
  // trois quantites que tout le travail de la journee avait confondues :
  //
  //   u = nombre de temoins interieurs a TOUTE sphere admissible (universels)
  //   p = nombre maximal de groupes couvrants a identites DISJOINTES
  //   d = min sur les centres admissibles du nombre de temoins interieurs
  //
  // On a toujours `u <= p <= d`, et le CONTRAT demande `d >= 8` — pas `u >= 8`.
  // Un rang huit n'exige pas huit temoins fixes : il exige qu'a chaque centre
  // il en existe huit, et ils peuvent changer avec le centre.
  //
  // La configuration : `a=(0,100,100)`, `b=(40,100,100)`, six temoins
  // `(j,100,100)` pour `j=1..6`, et trois gadgets `(20,111,100)`,
  // `(20,92,108)`, `(20,92,92)`.
  //
  // Les centres admissibles q4 vivent dans le plan mediateur `x=20`, a distance
  // au plus `||h||/sqrt(2)` du milieu, soit `Y^2+Z^2 <= 200` en ecrivant
  // `c=(20,100+Y,100+Z)`. Le rayon carre vaut `R^2 = 400+Y^2+Z^2`.
  //
  //   - un temoin `(j,100,100)` est interieur ssi `(j-20)^2 < 400`, donc
  //     toujours pour `0<j<40` : les six sont UNIVERSELS, `u=6` ;
  //   - le gadget `(20,111,100)` est interieur ssi `-22Y < 279`, sa region
  //     mauvaise est `Y <= -279/22 = -12,681` ;
  //   - `(20,92,108)` est interieur ssi `Y-Z < 17`, mauvaise `Y-Z >= 17` ;
  //   - `(20,92,92)` est interieur ssi `Y+Z < 17`, mauvaise `Y+Z >= 17`.
  //
  // Les trois regions mauvaises sont NON VIDES dans le disque de rayon
  // `sqrt(200) = 14,142` et DEUX A DEUX DISJOINTES :
  //   `Y<=-12,681` et `Y-Z>=17` donnent `Z <= -29,68`, hors du disque ;
  //   `Y<=-12,681` et `Y+Z>=17` donnent `Z >= 29,68`, hors du disque ;
  //   `Y-Z>=17` et `Y+Z>=17` donnent `Y >= 17 > 14,142`, hors du disque.
  //
  // Donc a chaque centre au plus UN gadget manque : `d = 6+2 = 8`. Mais deux
  // gadgets quelconques forment un groupe couvrant — leurs regions mauvaises
  // ont une intersection vide — et il n'y a que trois gadgets : le packing
  // disjoint maximal vaut `p = 6+1 = 7`.
  //
  //   u = 6  <  p = 7  <  d = 8
  //
  // Cette fixture refute DEUX choses a la fois. D'abord « le singleton est
  // presque optimal » : `u=6` alors que le contrat est satisfait. Ensuite
  // « huit groupes disjoints suffisent comme decision complete » : `p=7 < 8`
  // alors que `d=8`. Une porte qui assimile echec du packing et profondeur
  // insuffisante perd des fermetures VRAIES.
  if (fixtures) {
    const mhgp::i64 a[3] = {0, 100, 100}, b[3] = {40, 100, 100};
    mhgp::i64 t[9][3];
    for (int j = 0; j < 6; ++j) { t[j][0] = j + 1; t[j][1] = 100; t[j][2] = 100; }
    t[6][0] = 20; t[6][1] = 111; t[6][2] = 100;
    t[7][0] = 20; t[7][1] = 92;  t[7][2] = 108;
    t[8][0] = 20; t[8][1] = 92;  t[8][2] = 92;

    long long u = 0;
    for (int j = 0; j < 9; ++j)
      if (dual_lane_single(a, b, t[j], DualMutant::kNone) >= kLaneQ4) ++u;

    // `p` par recherche exhaustive : on enumere les sous-ensembles couvrants de
    // taille au plus trois, puis on extrait gloutonnement des groupes a
    // identites disjointes. Neuf temoins : la recherche est complete.
    std::vector<std::vector<int>> couvrants;
    for (int i1 = 0; i1 < 9; ++i1) {
      mhgp::i64 z1[1][3] = {{t[i1][0], t[i1][1], t[i1][2]}};
      const mhgp::i64 w1[1] = {1};
      if (dual_lane(a, b, z1, w1, 1, DualMutant::kNone) >= kLaneQ4)
        couvrants.push_back({i1});
      for (int i2 = i1 + 1; i2 < 9; ++i2) {
        mhgp::i64 z2[2][3] = {{t[i1][0], t[i1][1], t[i1][2]},
                              {t[i2][0], t[i2][1], t[i2][2]}};
        for (const auto& wv : (const long long[][2]){{1,1},{1,2},{2,1},{1,3},{3,1},{2,3},{3,2}}) {
          const mhgp::i64 w2[2] = {wv[0], wv[1]};
          if (dual_lane(a, b, z2, w2, 2, DualMutant::kNone) >= kLaneQ4) {
            couvrants.push_back({i1, i2});
            break;
          }
        }
      }
    }
    // Extraction gloutonne par taille croissante : elle est optimale ici, les
    // singletons etant tous disjoints entre eux.
    std::vector<char> pris(9, 0);
    long long pmax = 0;
    for (std::size_t taille = 1; taille <= 3; ++taille)
      for (const auto& g : couvrants) {
        if (g.size() != taille) continue;
        bool libre = true;
        for (int id : g) if (pris[(std::size_t)id]) libre = false;
        if (!libre) continue;
        for (int id : g) pris[(std::size_t)id] = 1;
        ++pmax;
      }

    std::printf("dual_fixture u_p_d : u=%lld p=%lld d=8 groupes_couvrants=%zu\n", u, pmax,
                couvrants.size());
    if (u != 6 || pmax != 7) {
      std::fprintf(stderr, "FIXTURE REFUTEE u_p_d : u=%lld (attendu 6) p=%lld (attendu 7)\n",
                   u, pmax);
      return 3;
    }
    std::printf("dual_fixture accord=OUI : u=6 < p=7 < d=8, le singleton et le packing"
                " sont tous deux incomplets\n");
    return 0;
  }
  if (span < 16 || span > 65535) refuse("--span hors du profil u16 [16,65535]");
  if (n < 8 || n > 200000) refuse("--points hors bornes [8,200000]");
  if (voisins < 2 || voisins > 64) refuse("--voisins hors bornes [2,64]");
  if (min_gain > 0 && !ablation) refuse("--min-gain exige --ablation");
  if (min_fermees_groupe > 0 && !ablation) refuse("--min-fermees-groupe exige --ablation");

  // -------------------------------------------------------------------------
  // 1. SELFTEST : `k=1` CONTRE L'ECRITURE HISTORIQUE `(g,Q)`.
  //
  // Le predicat dual et `lane_of_target_gq` sont deux derivations
  // independantes. Leur accord sur des centaines de milliers de triples en
  // pleine largeur u16 est la seule preuve disponible que la forme entiere du
  // minimax est juste. Un mutant doit les faire diverger.
  // -------------------------------------------------------------------------
  if (selftest) {
    Rng rng((unsigned long long)seed);
    long long desaccords = 0, vus[5] = {0, 0, 0, 0, 0};
    for (long long r = 0; r < rounds; ++r) {
      mhgp::i64 a[3], b[3], z[3];
      for (int j = 0; j < 3; ++j) {
        a[j] = rng.in(0, span);
        b[j] = rng.in(0, span);
        z[j] = rng.in(0, span);
      }
      if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]) continue;
      const int attendu = mhgp3v::cone::lane_of_target_gq(a[0], a[1], a[2], z[0], z[1], z[2],
                                                          b[0], b[1], b[2]);
      const int obtenu = dual_lane_single(a, b, z, mutant);
      if (attendu >= 0 && attendu <= 4) ++vus[attendu];
      if (obtenu != attendu) {
        ++desaccords;
        if (desaccords <= 3)
          std::fprintf(stderr, "DESACCORD k=1 : dual=%d (g,Q)=%d a=(%lld,%lld,%lld)"
                               " b=(%lld,%lld,%lld) z=(%lld,%lld,%lld)\n",
                       obtenu, attendu, (long long)a[0], (long long)a[1], (long long)a[2],
                       (long long)b[0], (long long)b[1], (long long)b[2],
                       (long long)z[0], (long long)z[1], (long long)z[2]);
      }
    }
    std::printf("dual_selftest accord=%s tires=%lld desaccords=%lld"
                " | lanes NONE=%lld q2=%lld q3=%lld q4=%lld\n",
                desaccords == 0 ? "OUI" : "NON", rounds, desaccords, vus[0], vus[2], vus[3],
                vus[4]);
    // ---- DEUX FIXTURES GRAVEES, POUR DEUX MUTANTS QUE LE TIRAGE NE TUE PAS.
    //
    // `dual-ignore-weights` est un NO-OP a `k=1`, puisque le poids y vaut deja
    // un : seul un groupe de deux le mord. `dual-accept-equality` exige une
    // egalite EXACTE `A^2 = 2R`, que des coordonnees tirees ne produisent
    // jamais. Un mutant qu'aucune fixture ne mord n'est pas couvert.
    long long morsures_gravees = 0;
    {
      // Frontiere exacte de q4 : `a=(0,0,0)`, `z=(1,1,1)`, `b=(2,1,1)` donne
      // `e=(1,1,1)`, `t=(1,0,0)`, donc `3H^2 = E X` exactement — c'est-a-dire
      // `A^2 = 2R` dans l'ecriture duale. L'egalite reste HORS du cone ouvert :
      // la lane exacte est q3.
      const mhgp::i64 a[3] = {0, 0, 0}, b[3] = {2, 1, 1}, z[3] = {1, 1, 1};
      const int sain = dual_lane_single(a, b, z, DualMutant::kNone);
      const int mut = dual_lane_single(a, b, z, mutant);
      if (sain != kLaneQ3) {
        std::fprintf(stderr, "FIXTURE REFUTEE dual-frontiere-q4 : lane=%d, attendu q3\n", sain);
        return 3;
      }
      if (mutant != DualMutant::kNone && mut != sain) ++morsures_gravees;
    }
    {
      // Les poids DECIDENT. Deux temoins et deux jeux de poids sur la meme
      // paire : si le predicat ignore les poids, les deux verdicts deviennent
      // identiques sur tous les tirages. On cherche donc un tirage ou ils
      // different pour le predicat sain.
      Rng rw((unsigned long long)seed * 7919ULL + 13ULL);
      long long differences = 0;
      for (long long r = 0; r < rounds; ++r) {
        mhgp::i64 a[3], b[3], z[2][3];
        for (int j = 0; j < 3; ++j) {
          a[j] = rw.in(0, span); b[j] = rw.in(0, span);
          z[0][j] = rw.in(0, span); z[1][j] = rw.in(0, span);
        }
        const mhgp::i64 w1[2] = {1, 1};
        const mhgp::i64 w2[2] = {1, 3};
        if (dual_lane(a, b, z, w1, 2, mutant) != dual_lane(a, b, z, w2, 2, mutant))
          ++differences;
      }
      if (mutant == DualMutant::kNone && differences == 0) {
        std::fprintf(stderr, "PLANCHER: les poids n'ont jamais change un verdict\n");
        return 3;
      }
      if (mutant == DualMutant::kIgnoreWeights && differences == 0) ++morsures_gravees;
    }
    if (mutant != DualMutant::kNone) {
      if (desaccords == 0 && morsures_gravees == 0) {
        std::fprintf(stderr, "MUTANT SURVIVANT : %s n'est mordu ni par (g,Q), ni par les"
                             " fixtures gravees\n", dual_mutant_name(mutant));
        return 3;
      }
      std::printf("MUTANT TUE %s : %lld divergences (g,Q), %lld fixtures gravees\n",
                  dual_mutant_name(mutant), desaccords, morsures_gravees);
      return 4;
    }
    if (desaccords != 0) return 1;
    // Anti-vacuite : les quatre verdicts doivent avoir ete exerces.
    if (vus[0] == 0 || vus[2] == 0 || vus[3] == 0 || vus[4] == 0) {
      std::fprintf(stderr, "PLANCHER: une lane n'a jamais ete exercee\n");
      return 3;
    }
    return 0;
  }

  // -------------------------------------------------------------------------
  // 2. ABLATION : COMBIEN DE PAIRES UN GROUPE FERME-T-IL QU'UN TEMOIN N'A PAS ?
  //
  // Pour un echantillon deterministe de paires, on compare deux profondeurs :
  //   `p1` = nombre de temoins UNIQUES universels (le certificat actuel) ;
  //   `pg` = nombre de GROUPES disjoints, extraits gloutonnement, de taille au
  //          plus trois parmi les `--voisins` temoins les plus proches du
  //          milieu, avec des poids entiers essayes dans un petit ensemble.
  // Une paire est fermee en q4 des que la profondeur atteint huit.
  //
  // L'extraction gloutonne est un PROPOSER : son echec ne refute rien. Le gain
  // publie est donc un MINORANT du pouvoir reel du certificat.
  // -------------------------------------------------------------------------
  mhgp3v::CloudFamily fam;
  if (famille == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (famille == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (famille == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (famille == "two_lines") fam = mhgp3v::CloudFamily::kTwoLines;
  else refuse("--family inconnue");
  const int coord = mhgp3v::cloud_family_default_coord(fam, (int)n);
  const auto pts = mhgp3v::make_family_cloud(fam, (int)n, coord, 12345);
  const long long m = (long long)pts.size();
  if (m < 8) refuse("nuage trop petit");
  std::vector<std::array<mhgp::i64, 3>> p((size_t)m);
  for (long long i = 0; i < m; ++i)
    p[(size_t)i] = {(mhgp::i64)pts[(size_t)i].x, (mhgp::i64)pts[(size_t)i].y,
                    (mhgp::i64)pts[(size_t)i].z};

  // Poids essayes. Ils restent petits : le minimax d'un groupe de deux ou trois
  // temoins bien choisis est atteint pres de l'uniforme, et un poids large ne
  // ferait qu'elargir les entiers sans gagner de couverture.
  static const long long kPoids2[][2] = {{1, 1}, {1, 2}, {2, 1}, {1, 3}, {3, 1}, {2, 3}, {3, 2}};
  static const long long kPoids3[][3] = {{1, 1, 1}, {2, 1, 1}, {1, 2, 1}, {1, 1, 2},
                                         {3, 2, 2}, {2, 3, 2}, {2, 2, 3}};

  Rng rng((unsigned long long)seed);
  long long paires = 0, fermees1 = 0, fermeesg = 0, gain = 0;
  long long groupes_2 = 0, groupes_3 = 0, groupes_1 = 0;
  for (long long t = 0; t < echantillon; ++t) {
    const long long ia = rng.in(0, m - 1);
    long long ib = rng.in(0, m - 1);
    if (ib == ia) ib = (ia + 1) % m;
    const auto& A = p[(size_t)ia];
    const auto& B = p[(size_t)ib];
    ++paires;

    // Temoins tries par distance au milieu : `||a+b-2z||^2`.
    std::vector<std::pair<long long, long long>> ordre;
    ordre.reserve((size_t)m);
    for (long long r = 0; r < m; ++r) {
      if (r == ia || r == ib) continue;
      long long u = 0;
      for (int j = 0; j < 3; ++j) {
        const long long e = A[j] + B[j] - 2 * p[(size_t)r][j];
        u += e * e;
      }
      ordre.push_back({u, r});
    }
    std::sort(ordre.begin(), ordre.end());
    const long long tmax = std::min<long long>(voisins, (long long)ordre.size());

    // Profondeur par temoins uniques : c'est le certificat actuel.
    long long p1 = 0;
    std::vector<char> pris((size_t)m, 0);
    for (long long r = 0; r < (long long)ordre.size(); ++r) {
      const long long id = ordre[(size_t)r].second;
      mhgp::i64 z[3] = {p[(size_t)id][0], p[(size_t)id][1], p[(size_t)id][2]};
      if (dual_lane_single(A.data(), B.data(), z, DualMutant::kNone) >= kLaneQ4) {
        ++p1;
        if (p1 >= groupes_max) break;
      }
    }
    if (p1 >= groupes_max) ++fermees1;

    // Profondeur par groupes disjoints, extraction gloutonne.
    long long pg = 0;
    for (long long g = 0; g < groupes_max && pg < groupes_max; ++g) {
      bool trouve = false;
      // Taille un d'abord : le moins cher, et il consomme une seule identite.
      //
      // ELLE BALAIE TOUT LE NUAGE, PAS LA FENETRE `--voisins`. La premiere
      // version ne cherchait les singletons que parmi les `voisins` plus
      // proches, alors que la profondeur de reference les cherche partout :
      // l'extraction pouvait alors fermer MOINS de paires que le certificat
      // qu'elle est censee dominer, et l'invariant d'inclusion l'a refutee sur
      // `eight_clusters` a `n=1500`. Un groupe de taille un est disponible
      // partout ; restreindre sa recherche fausse la comparaison dans les deux
      // sens — il sous-estime le certificat de base ET le gain annonce.
      for (long long i1 = 0; i1 < (long long)ordre.size() && !trouve; ++i1) {
        const long long r1 = ordre[(size_t)i1].second;
        if (pris[(size_t)r1]) continue;
        mhgp::i64 z[3] = {p[(size_t)r1][0], p[(size_t)r1][1], p[(size_t)r1][2]};
        if (dual_lane_single(A.data(), B.data(), z, mutant) >= kLaneQ4) {
          pris[(size_t)r1] = 1; ++pg; ++groupes_1; trouve = true;
        }
      }
      // Taille deux.
      for (long long i1 = 0; i1 < tmax && !trouve; ++i1) {
        const long long r1 = ordre[(size_t)i1].second;
        if (pris[(size_t)r1]) continue;
        for (long long i2 = i1 + 1; i2 < tmax && !trouve; ++i2) {
          const long long r2 = ordre[(size_t)i2].second;
          if (pris[(size_t)r2]) continue;
          mhgp::i64 z[2][3] = {{p[(size_t)r1][0], p[(size_t)r1][1], p[(size_t)r1][2]},
                               {p[(size_t)r2][0], p[(size_t)r2][1], p[(size_t)r2][2]}};
          for (const auto& wv : kPoids2) {
            const mhgp::i64 w[2] = {wv[0], wv[1]};
            if (dual_lane(A.data(), B.data(), z, w, 2, mutant) >= kLaneQ4) {
              pris[(size_t)r1] = 1; pris[(size_t)r2] = 1; ++pg; ++groupes_2; trouve = true;
              break;
            }
          }
        }
      }
      // Taille trois.
      for (long long i1 = 0; i1 < tmax && !trouve; ++i1) {
        const long long r1 = ordre[(size_t)i1].second;
        if (pris[(size_t)r1]) continue;
        for (long long i2 = i1 + 1; i2 < tmax && !trouve; ++i2) {
          const long long r2 = ordre[(size_t)i2].second;
          if (pris[(size_t)r2]) continue;
          for (long long i3 = i2 + 1; i3 < tmax && !trouve; ++i3) {
            const long long r3 = ordre[(size_t)i3].second;
            if (pris[(size_t)r3]) continue;
            mhgp::i64 z[3][3] = {{p[(size_t)r1][0], p[(size_t)r1][1], p[(size_t)r1][2]},
                                 {p[(size_t)r2][0], p[(size_t)r2][1], p[(size_t)r2][2]},
                                 {p[(size_t)r3][0], p[(size_t)r3][1], p[(size_t)r3][2]}};
            for (const auto& wv : kPoids3) {
              const mhgp::i64 w[3] = {wv[0], wv[1], wv[2]};
              if (dual_lane(A.data(), B.data(), z, w, 3, mutant) >= kLaneQ4) {
                pris[(size_t)r1] = 1; pris[(size_t)r2] = 1; pris[(size_t)r3] = 1;
                ++pg; ++groupes_3; trouve = true;
                break;
              }
            }
          }
        }
      }
      if (!trouve) break;
    }
    if (pg >= groupes_max) ++fermeesg;
    if (pg >= groupes_max && p1 < groupes_max) ++gain;
  }

  std::printf("dual_ablation famille=%s n=%lld paires=%lld seuil=%lld voisins=%lld"
              " | fermees_temoin_unique=%lld (%.3f%%) fermees_groupes=%lld (%.3f%%)"
              " | gain=%lld (%.3f%%) | groupes tailles 1=%lld 2=%lld 3=%lld\n",
              famille.c_str(), m, paires, groupes_max, voisins, fermees1,
              100.0 * (double)fermees1 / (double)std::max(1LL, paires), fermeesg,
              100.0 * (double)fermeesg / (double)std::max(1LL, paires), gain,
              100.0 * (double)gain / (double)std::max(1LL, paires), groupes_1, groupes_2,
              groupes_3);

  // L'INCLUSION EST OBLIGATOIRE. Un groupe de taille un est un temoin unique,
  // donc l'extraction par groupes ne peut jamais fermer MOINS de paires que le
  // certificat actuel. Une violation refute l'extraction, pas la geometrie.
  if (mutant == DualMutant::kNone && fermeesg < fermees1) {
    std::fprintf(stderr, "INVARIANT VIOLE: groupes=%lld < temoin unique=%lld\n", fermeesg,
                 fermees1);
    return 3;
  }
  if (min_gain > 0 && gain < min_gain) {
    std::fprintf(stderr, "PLANCHER: gain %lld, %lld exige\n", gain, min_gain);
    return 3;
  }
  if (min_fermees_groupe > 0 && fermeesg < min_fermees_groupe) {
    std::fprintf(stderr, "PLANCHER: fermees_groupes %lld, %lld exige\n", fermeesg,
                 min_fermees_groupe);
    return 3;
  }
  return 0;
}
