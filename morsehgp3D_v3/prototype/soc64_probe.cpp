// MorseHGP3D v3 — PORTE DU CERTIFICAT CORRELE `SOC64` / `CORNER512`.
//
// Specification : PROPOSITION.md section 6.2 et section 10 (fixtures
// permanentes « SOC64 succes axial et faux-echec du produit relaxe »,
// « CORNER512 coin omis, egalite, axe degenere et largeur 70 bits »).
// Ordre impose par audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md
// section 4.1 : cette ablation vient AVANT toute promotion des cages.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=ablation_counter_only,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CETTE PORTE JUGE, ET AVEC QUELLE AUTORITE
//
// Le sujet est `soc64_rect.hpp`. L'autorite est l'ENUMERATION DU RESEAU ENTIER
// du rectangle : `exhaustive_all_lane` visite tous les triples (a,b,z) de
// A x B x C et rend la lane `ALL` exacte sur les points de la grille. Elle est
// bornee par un cap explicite, elle n'est jamais sur un chemin mesure, et elle
// ne partage avec le sujet que le predicat ponctuel — qui est precisement ce
// que les trois ecritures de `spindle_cone.hpp` recertifient deja entre elles.
//
// La chaine de soliditie exigee est un ORDRE, pas une egalite :
//
//     soc64  <=  corner512  <=  exhaustif
//
// La premiere inegalite dit que la relaxation en produit ne peut que perdre.
// La seconde dit que l'enveloppe CONTINUE ne peut que perdre contre les seuls
// points STOCKES. Toute violation de l'un ou l'autre est un desaccord, code 1.
//
// Les egalites sont interdites comme critere : une porte qui exigerait
// `soc64 == exhaustif` refuserait le fail-open, qui est la propriete voulue.
//
// ---------------------------------------------------------------------------
// CODES DE SORTIE
//   0  accord
//   1  desaccord du juge
//   2  campagne refusee avant tout calcul
//   3  plancher de couverture ou fixture gravee violee
//   4  mutant survivant
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "soc64_rect.hpp"

namespace {

using mhgp3v::soc::Box;
using mhgp3v::soc::SocMutant;
using mhgp3v::soc::SocStats;
using mhgp3v::soc::corner512_all_lane;
using mhgp3v::soc::exhaustive_all_lane;
using mhgp3v::soc::lane_of_et;
using mhgp3v::soc::scalar_extrema_all_lane;
using mhgp3v::soc::soc64_all_lane;
using mhgp3v::soc::soc_mutant_name;

using mhgp3v::cone::kLaneNone;
using mhgp3v::cone::kLaneQ2;
using mhgp3v::cone::kLaneQ3;
using mhgp3v::cone::kLaneQ4;

// Le cap du reseau. Les fixtures gravees restent toutes tres en dessous ; une
// fixture qui le depasserait serait REFUSEE, jamais silencieusement sautee.
constexpr long long kLatticeCap = 200000;

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

const char* lane_name(int lane) {
  switch (lane) {
    case kLaneNone: return "NONE";
    case kLaneQ2: return "q2";
    case kLaneQ3: return "q3";
    case kLaneQ4: return "q4";
    default: return "?";
  }
}

// ---------------------------------------------------------------------------
// LES FIXTURES GRAVEES. Coordonnees exactes, verdicts attendus exacts, et pour
// chacune le mutant qu'elle DOIT tuer. Toutes vivent dans [0,65535]^3.
// ---------------------------------------------------------------------------
struct Fixture {
  const char* nom;
  Box a, b, c;
  int attendu_exhaustif;  // lane ALL exacte sur le reseau
  int attendu_soc64;      // verdict relaxe attendu
  int attendu_corner512;  // verdict sur l'enveloppe continue
  int attendu_scalaire;   // verdict du classifieur a extrema separes
  const char* pourquoi;
};

Box bx(long long x0, long long x1, long long y0, long long y1, long long z0, long long z1) {
  Box b{};
  b.lo[0] = x0; b.hi[0] = x1;
  b.lo[1] = y0; b.hi[1] = y1;
  b.lo[2] = z0; b.hi[2] = z1;
  return b;
}
Box pt(long long x, long long y, long long z) { return bx(x, x, y, y, z, z); }

std::vector<Fixture> engraved_fixtures() {
  std::vector<Fixture> f;

  // F1 — LA FIXTURE AXIALE EXIGEE PAR L'AUDIT. Toutes les differences sont
  // colinearies positives : e = (1..100,0,0) et t = (1..100,0,0), donc
  // H^2 = E X exactement et 3H^2 > EX partout. `SOC64` ferme q4 ; le
  // classifieur a extrema separes prend Hmin=1 contre Emax Xmax = 1e8 et
  // n'atteint meme pas q3. C'est la refutation directe du raccourci
  // « scalaire rouge, donc cages ».
  f.push_back({"soc64-axial",
               bx(0, 99, 100, 100, 100, 100),
               bx(101, 200, 100, 100, 100, 100),
               pt(100, 100, 100),
               kLaneQ4, kLaneQ4, kLaneQ4, kLaneQ2,
               "differences colineaires positives : SOC64 ferme, scalaire echoue"});

  // F2 — LE FAUX-ECHEC DU PRODUIT RELAXE. `A` et `B` sont des points, `C` une
  // boite : les vrais couples verifient e + t = b - a, contrainte que la
  // relaxation en produit PERD. Le couple relaxe e=(80,45,0), t=(80,-45,0)
  // exigerait b = z + t = (1160+..) hors de `B` : il n'existe pas. Il donne
  // 3H^2 = 57 421 875 < E X = 70 980 625, donc `SOC64` rend q3 alors que le
  // rectangle REEL est q4. La reponse doit rester UNKNOWN, jamais NONE.
  f.push_back({"soc64-faux-echec-relaxe",
               pt(1000, 1000, 1000),
               pt(1200, 1000, 1000),
               bx(1080, 1120, 955, 1045, 1000, 1000),
               kLaneQ4, kLaneQ3, kLaneQ4, kLaneQ2,
               "le produit relaxe perd e+t=b-a : SOC64 sous-estime, CORNER512 non"});

  // F3 — LA FRONTIERE EXACTE DE q4. e=(1,1,1), t=(1,0,0) donne H=1, E=3, X=1 :
  // 3H^2 = 3 = E X. L'egalite est HORS du cone ouvert, la lane est donc q3
  // strictement. Le mutant `accept-equality` la promeut en q4 ; le mutant
  // `swap-coeff` teste 4H^2=4 > 3 et la promeut aussi.
  f.push_back({"soc64-frontiere-q4",
               pt(1000, 1000, 1000),
               pt(1002, 1001, 1001),
               pt(1001, 1001, 1001),
               kLaneQ3, kLaneQ3, kLaneQ3, kLaneQ3,
               "3H^2 = E X exactement : l'egalite reste hors du cone ouvert"});

  // F4 — LE CONE OPPOSE. b est du cote de a : H = -200 < 0, donc AUCUNE lane.
  // Le mutant sans positivite ne regarde que H^2 et certifie q4.
  f.push_back({"soc64-cone-oppose",
               pt(1000, 1000, 1000),
               pt(990, 1000, 1000),
               pt(1010, 1000, 1000),
               kLaneNone, kLaneNone, kLaneNone, kLaneNone,
               "H<0 : le temoin est derriere l'ancre, aucune lane"});

  // F5 — LA LARGEUR 68 BITS. e = t = (32768,32768,32768) colineaires :
  // E = X = 3*32768^2 = 3 221 225 472 et E X = 1,0376e19 > 2^63. Un mutant qui
  // reste en i64 y calcule un produit enroule. Le vrai verdict est q4 puisque
  // H^2 = E X exactement.
  f.push_back({"soc64-largeur-68-bits",
               pt(0, 0, 0),
               pt(65535, 65535, 65535),
               pt(32768, 32768, 32768),
               kLaneQ4, kLaneQ4, kLaneQ4, kLaneQ4,
               "E X = 1,04e19 depasse i64 : la promotion 128 bits est obligatoire"});

  // F5b — L'ENROULEMENT QUI DECIDE. La fixture precedente ne suffit PAS a tuer
  // le mutant i64 : elle est colineaire, donc H^2 et E X y enroulent du meme
  // cote et l'ordre survit par accident. Il faut un couple presque orthogonal
  // et large : e=(65534,0,0), t=(1,65535,65535) donne H=65534, donc
  // 3H^2 = 1,29e10 tandis que E X = 3,689e19. La verite est q2. En i64, E X
  // s'enroule a -3,38e15, devient plus petit que 3H^2, et le mutant certifie
  // q4 sur une paire qui n'a aucun temoin. C'est la contradiction exacte, pas
  // un ecart numerique.
  f.push_back({"soc64-enroulement-i64",
               pt(0, 0, 0),
               pt(65535, 65535, 65535),
               pt(65534, 0, 0),
               kLaneQ2, kLaneQ2, kLaneQ2, kLaneQ2,
               "E X = 3,69e19 s'enroule en negatif : le mutant i64 certifie q4"});

  // F6 — LA DIFFERENCE DE MINKOWSKI, ET LA DIAGONALE. `A` et `C` sont deux
  // segments en y opposes : Ebox = C (-) A = {100} x [-100,100] x {0} et
  // Tbox = B (-) C = {100} x [-50,50] x {0}. Le couple (e_y=+100, t_y=-50)
  // echoue ; il n'est PAS diagonal, et il disparait si l'on ecrit
  // [lo-lo, hi-hi] au lieu de [lo-hi, hi-lo] — cette ecriture fausse
  // effondre Ebox sur un point et certifie q4 a tort.
  f.push_back({"soc64-minkowski-et-diagonale",
               bx(1000, 1000, 950, 1050, 1000, 1000),
               pt(1200, 1000, 1000),
               bx(1100, 1100, 950, 1050, 1000, 1000),
               kLaneQ2, kLaneQ2, kLaneQ2, kLaneQ2,
               "le couple fautif est hors diagonale et hors Minkowski errone"});

  // F7 — LE COIN OMIS. Avec e = (-100,0,0) et Tbox = [-50,-30] x [-5,40]^2,
  // la condition q4 s'ecrit 2 t_x^2 > t_y^2 + t_z^2. Le SEUL coin qui la viole
  // est (t_x=-30, t_y=40, t_z=40), c'est-a-dire l'indice 7 : 1800 < 3200. Les
  // sept autres passent. Omettre ce coin certifie q4 a tort.
  f.push_back({"soc64-coin-sept",
               pt(1100, 1000, 1000),
               bx(950, 970, 995, 1040, 995, 1040),
               pt(1000, 1000, 1000),
               kLaneQ2, kLaneQ2, kLaneQ2, kLaneQ2,
               "seul le coin d'indice 7 de Tbox viole q4"});

  // F8 — LA CONTRE-FIXTURE DE SEUIL. Elle vient du contre-audit
  // `AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md` section
  // 2.4. Le couple central `e=(1,1,1)`, `t=(1,0,0)` rend q3 ; le coin
  // `t=(-1,0,0)` rend `H=-1`, donc la vraie lane minimale est `NONE`. Sous
  // `floor=q4`, la version precedente rendait `q3` — une affirmation fausse.
  // Elle est verifiee separement, avec les deux seuils, par `run_seuils`.
  f.push_back({"soc64-seuil-sous-plancher",
               pt(0, 0, 0),
               bx(0, 4, 1, 1, 1, 1),
               pt(1, 1, 1),
               kLaneNone, kLaneNone, kLaneNone, kLaneNone,
               "sous floor=q4 le retour n'est pas une lane : il vaut UNKNOWN"});

  return f;
}

// ---------------------------------------------------------------------------
// ISOMETRIES DU RESEAU. Permutation d'axes, reflexions et translation ne
// changent aucun des trois verdicts : le predicat ne depend que de produits
// scalaires. Une porte qui ne le verifie pas laisse passer un classifieur qui
// privilegie un axe.
// ---------------------------------------------------------------------------
struct Isometry {
  int perm[3];
  int sign[3];
  long long shift[3];
};

Box apply_iso(const Box& b, const Isometry& iso) {
  Box out{};
  for (int i = 0; i < 3; ++i) {
    const int src = iso.perm[i];
    const long long lo = iso.sign[i] * b.lo[src] + iso.shift[i];
    const long long hi = iso.sign[i] * b.hi[src] + iso.shift[i];
    out.lo[i] = (lo < hi) ? lo : hi;
    out.hi[i] = (lo < hi) ? hi : lo;
  }
  return out;
}

// ---------------------------------------------------------------------------
// GENERATEUR DETERMINISTE. Aucun `rand()` : la porte doit etre reproductible a
// la graine pres, sur toute plateforme.
// ---------------------------------------------------------------------------
struct Rng {
  unsigned long long s;
  explicit Rng(unsigned long long seed) : s(seed * 6364136223846793005ULL + 1442695040888963407ULL) {}
  unsigned long long next() {
    s ^= s << 13; s ^= s >> 7; s ^= s << 17;
    return s;
  }
  long long in(long long lo, long long hi) {
    if (hi <= lo) return lo;
    return lo + (long long)(next() % (unsigned long long)(hi - lo + 1));
  }
};

Box random_box(Rng* rng, long long span, long long width_max) {
  Box b{};
  for (int i = 0; i < 3; ++i) {
    const long long w = rng->in(0, width_max);
    const long long lo = rng->in(0, span - w);
    b.lo[i] = lo;
    b.hi[i] = lo + w;
  }
  return b;
}

struct Coverage {
  long long rectangles = 0;
  long long soc64_all_q4 = 0;
  long long corner512_all_q4 = 0;
  long long gain_sur_scalaire = 0;   // soc64 > scalaire
  long long perte_sur_corner512 = 0; // corner512 > soc64 : le fail-open observe
  long long exhaustifs = 0;
};

}  // namespace

int main(int argc, char** argv) {
  bool fixtures_only = false;
  bool selftest = false;
  long long seed = 1;
  long long rounds = 20000;
  long long span = 65535;
  SocMutant mutant = SocMutant::kNone;
  long long min_soc64_all = 0;
  long long min_corner512_all = 0;
  long long min_gain_scalaire = 0;
  long long min_perte_relaxe = 0;
  long long min_exhaustifs = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const std::size_t eq = arg.find('=');
    const std::string key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
    const std::string val = (eq == std::string::npos) ? std::string() : arg.substr(eq + 1);
    long long parsed = 0;
    if (key == "--fixtures") { fixtures_only = true; }
    else if (key == "--selftest") { selftest = true; }
    else if (key == "--seed") { if (!parse_ll(val.c_str(), &parsed)) refuse("--seed invalide"); seed = parsed; }
    else if (key == "--rounds") { if (!parse_ll(val.c_str(), &parsed)) refuse("--rounds invalide"); rounds = parsed; }
    else if (key == "--span") { if (!parse_ll(val.c_str(), &parsed)) refuse("--span invalide"); span = parsed; }
    else if (key == "--min-soc64-all") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-soc64-all invalide"); min_soc64_all = parsed; }
    else if (key == "--min-corner512-all") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-corner512-all invalide"); min_corner512_all = parsed; }
    else if (key == "--min-gain-scalaire") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-gain-scalaire invalide"); min_gain_scalaire = parsed; }
    else if (key == "--min-perte-relaxe") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-perte-relaxe invalide"); min_perte_relaxe = parsed; }
    else if (key == "--min-exhaustifs") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-exhaustifs invalide"); min_exhaustifs = parsed; }
    else if (key == "--inject") {
      if (val == "soc-no-positivity") mutant = SocMutant::kNoPositivity;
      else if (val == "soc-accept-equality") mutant = SocMutant::kAcceptEquality;
      else if (val == "soc-swap-coeff") mutant = SocMutant::kSwapCoeff;
      else if (val == "soc-narrow-i64") mutant = SocMutant::kNarrowI64;
      else if (val == "soc-diagonal-only") mutant = SocMutant::kDiagonalOnly;
      else if (val == "soc-seven-targets") mutant = SocMutant::kSevenTargets;
      else if (val == "soc-minkowski-same") mutant = SocMutant::kMinkowskiSame;
      else refuse("--inject inconnu");
    } else {
      refuse("argument inconnu");
    }
  }

  if (!fixtures_only && !selftest) refuse("choisir --fixtures ou --selftest");
  if (span < 16 || span > 65535) refuse("--span hors du profil u16 [16,65535]");
  if (rounds < 0) refuse("--rounds negatif");
  // Un plancher sans sa campagne est refuse AVANT tout calcul : sinon un
  // plancher inatteignable passerait pour satisfait dans le mode fixtures.
  if (fixtures_only && (min_soc64_all > 0 || min_corner512_all > 0 || min_gain_scalaire > 0 ||
                        min_perte_relaxe > 0 || min_exhaustifs > 0))
    refuse("les planchers de couverture exigent --selftest");

  const std::vector<Fixture> fixtures = engraved_fixtures();

  // -------------------------------------------------------------------------
  // 1. FIXTURES GRAVEES, SUJET SAIN. Chaque verdict attendu est confronte au
  // calcul ET a l'enumeration du reseau.
  // -------------------------------------------------------------------------
  int refutees = 0;
  for (const Fixture& f : fixtures) {
    const int ex = exhaustive_all_lane(f.a, f.b, f.c, kLatticeCap);
    if (ex < 0) {
      std::fprintf(stderr, "REFUS : fixture %s depasse le cap de reseau %lld\n", f.nom, kLatticeCap);
      return 2;
    }
    const int s64 = soc64_all_lane(f.a, f.b, f.c, mutant, kLaneQ2, nullptr);
    const int c512 = corner512_all_lane(f.a, f.b, f.c, mutant, kLaneQ2, nullptr);
    const int sca = scalar_extrema_all_lane(f.a, f.b, f.c, nullptr);
    if (mutant == SocMutant::kNone) {
      if (ex != f.attendu_exhaustif || s64 != f.attendu_soc64 || c512 != f.attendu_corner512 ||
          sca != f.attendu_scalaire) {
        std::fprintf(stderr,
                     "FIXTURE REFUTEE %s : exhaustif=%s (attendu %s) soc64=%s (attendu %s) "
                     "corner512=%s (attendu %s) scalaire=%s (attendu %s) — %s\n",
                     f.nom, lane_name(ex), lane_name(f.attendu_exhaustif), lane_name(s64),
                     lane_name(f.attendu_soc64), lane_name(c512), lane_name(f.attendu_corner512),
                     lane_name(sca), lane_name(f.attendu_scalaire), f.pourquoi);
        ++refutees;
      }
      // L'ORDRE DE SOLIDITE. Il est verifie sur chaque fixture, pas seulement
      // sur les tirages : une fixture est aussi un temoin de l'inegalite.
      if (!(s64 <= c512 && c512 <= ex)) {
        std::fprintf(stderr,
                     "ORDRE VIOLE %s : soc64=%s corner512=%s exhaustif=%s\n", f.nom,
                     lane_name(s64), lane_name(c512), lane_name(ex));
        ++refutees;
      }
    }
  }

  if (mutant == SocMutant::kNone && refutees != 0) {
    std::fprintf(stderr, "REFUS : %d fixture(s) gravee(s) refutee(s)\n", refutees);
    return 3;
  }

  // -------------------------------------------------------------------------
  // 2. LE MUTANT DOIT MOURIR SUR UNE FIXTURE. Un mutant qui passerait toutes
  // les fixtures gravees ne serait pas couvert : c'est un code 4.
  // -------------------------------------------------------------------------
  if (mutant != SocMutant::kNone) {
    long long tues = 0;
    for (const Fixture& f : fixtures) {
      const int ex = exhaustive_all_lane(f.a, f.b, f.c, kLatticeCap);
      if (ex < 0) { std::fprintf(stderr, "REFUS : cap de reseau\n"); return 2; }
      const int s64 = soc64_all_lane(f.a, f.b, f.c, mutant, kLaneQ2, nullptr);
      const int c512 = corner512_all_lane(f.a, f.b, f.c, mutant, kLaneQ2, nullptr);
      // La faute observable est TOUJOURS la meme : le mutant certifie une lane
      // que le reseau ne porte pas. Le sens de l'inegalite est ce qui compte,
      // pas l'ecart numerique.
      if (s64 > ex || c512 > ex) {
        std::printf("mutant %s tue par %s : soc64=%s corner512=%s > exhaustif=%s\n",
                    soc_mutant_name(mutant), f.nom, lane_name(s64), lane_name(c512),
                    lane_name(ex));
        ++tues;
      }
    }
    if (tues == 0) {
      std::fprintf(stderr,
                   "MUTANT SURVIVANT : %s n'est refute par aucune fixture gravee\n",
                   soc_mutant_name(mutant));
      return 4;
    }
    std::printf("mutant %s mort sur %lld fixture(s)\n", soc_mutant_name(mutant), tues);
    return 0;
  }

  if (fixtures_only) {
    // ---- LA SEMANTIQUE DU SEUIL, GRAVEE.
    //
    // Sous `floor=q4`, un retour inferieur au seuil ne doit JAMAIS etre lu
    // comme une lane exacte. Sur la fixture `soc64-seuil-sous-plancher`, le
    // pretest central rend q3 et sort ; la vraie lane minimale est `NONE`.
    // Le contrat exige donc `kUnknownBelowFloor`, et il exige aussi que le
    // meme rectangle rende `NONE` — valeur exacte — sous `floor=q2`.
    {
      const Fixture& f = fixtures.back();
      const int haut = soc64_all_lane(f.a, f.b, f.c, SocMutant::kNone, kLaneQ4, nullptr);
      const int bas = soc64_all_lane(f.a, f.b, f.c, SocMutant::kNone, kLaneQ2, nullptr);
      const int chaut = corner512_all_lane(f.a, f.b, f.c, SocMutant::kNone, kLaneQ4, nullptr);
      if (haut != mhgp3v::soc::kUnknownBelowFloor || bas != kLaneNone ||
          chaut != mhgp3v::soc::kUnknownBelowFloor) {
        std::fprintf(stderr,
                     "FIXTURE REFUTEE soc64-seuil : floor=q4 rend %d (attendu %d), "
                     "floor=q2 rend %d (attendu %d), corner512 floor=q4 rend %d\n",
                     haut, mhgp3v::soc::kUnknownBelowFloor, bas, kLaneNone, chaut);
        return 3;
      }
    }
    // L'EQUIVARIANCE EST PARTIE DES FIXTURES. Vingt-quatre rotations signees et
    // permutations d'axes, plus une translation : aucun verdict ne bouge.
    long long compares = 0;
    for (const Fixture& f : fixtures) {
      const int s64 = soc64_all_lane(f.a, f.b, f.c, SocMutant::kNone, kLaneQ2, nullptr);
      const int c512 = corner512_all_lane(f.a, f.b, f.c, SocMutant::kNone, kLaneQ2, nullptr);
      const int perms[6][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
      for (int p = 0; p < 6; ++p) {
        for (int m = 0; m < 8; ++m) {
          Isometry iso{};
          for (int i = 0; i < 3; ++i) {
            iso.perm[i] = perms[p][i];
            iso.sign[i] = (m & (1 << i)) ? -1 : 1;
            // La translation ramene dans [0,65535] : +65535 si l'axe est
            // reflechi, sinon zero. Les differences sont inchangees.
            iso.shift[i] = (iso.sign[i] < 0) ? 65535 : 0;
          }
          const Box a2 = apply_iso(f.a, iso), b2 = apply_iso(f.b, iso), c2 = apply_iso(f.c, iso);
          const int s2 = soc64_all_lane(a2, b2, c2, SocMutant::kNone, kLaneQ2, nullptr);
          const int c2l = corner512_all_lane(a2, b2, c2, SocMutant::kNone, kLaneQ2, nullptr);
          ++compares;
          if (s2 != s64 || c2l != c512) {
            std::fprintf(stderr,
                         "EQUIVARIANCE VIOLEE %s : perm=%d%d%d signes=%d — soc64 %s->%s "
                         "corner512 %s->%s\n",
                         f.nom, perms[p][0], perms[p][1], perms[p][2], m, lane_name(s64),
                         lane_name(s2), lane_name(c512), lane_name(c2l));
            return 1;
          }
        }
      }
    }
    std::printf("fixtures accord=OUI refutees=0 gravees=%zu equivariances=%lld\n", fixtures.size(),
                compares);
    return 0;
  }

  // -------------------------------------------------------------------------
  // 3. SELFTEST : L'ORDRE DE SOLIDITE SUR DES RECTANGLES TIRES.
  //
  // Deux regimes. Les PETITES boites sont jugees par enumeration complete du
  // reseau. Les GRANDES le sont par echantillonnage : si `corner512` certifie
  // la lane q, alors TOUT triple tire doit porter au moins q. L'echantillon ne
  // prouve pas l'inclusion, mais il la REFUTE des qu'elle est fausse — et
  // c'est exactement le role d'un juge.
  // -------------------------------------------------------------------------
  Rng rng((unsigned long long)seed);
  Coverage cov{};
  long long desaccords = 0;

  for (long long r = 0; r < rounds; ++r) {
    const bool petit = ((r & 1) == 0);
    const long long wmax = petit ? 2 : (long long)(4 + (rng.next() % 400));
    const Box a = random_box(&rng, span, wmax);
    const Box b = random_box(&rng, span, wmax);
    const Box c = random_box(&rng, span, wmax);

    SocStats st{};
    const int s64 = soc64_all_lane(a, b, c, SocMutant::kNone, kLaneQ2, &st);
    const int c512 = corner512_all_lane(a, b, c, SocMutant::kNone, kLaneQ2, &st);
    const int sca = scalar_extrema_all_lane(a, b, c, &st);
    ++cov.rectangles;
    if (s64 == kLaneQ4) ++cov.soc64_all_q4;
    if (c512 == kLaneQ4) ++cov.corner512_all_q4;
    if (s64 > sca) ++cov.gain_sur_scalaire;
    if (c512 > s64) ++cov.perte_sur_corner512;

    if (s64 > c512) {
      std::fprintf(stderr, "DESACCORD : soc64=%s > corner512=%s\n", lane_name(s64),
                   lane_name(c512));
      if (++desaccords > 20) break;
      continue;
    }
    // Le classifieur scalaire est SUR : il ne peut jamais depasser `corner512`,
    // qui est exact sur l'enveloppe continue dont il tire ses extrema.
    if (sca > c512) {
      std::fprintf(stderr, "DESACCORD : scalaire=%s > corner512=%s\n", lane_name(sca),
                   lane_name(c512));
      if (++desaccords > 20) break;
      continue;
    }

    if (petit) {
      const int ex = exhaustive_all_lane(a, b, c, kLatticeCap);
      if (ex < 0) continue;  // cap : la boite tiree est trop grosse, sans verdict
      ++cov.exhaustifs;
      if (c512 > ex) {
        std::fprintf(stderr, "DESACCORD : corner512=%s > exhaustif=%s\n", lane_name(c512),
                     lane_name(ex));
        if (++desaccords > 20) break;
      }
    } else {
      // Echantillonnage de triples interieurs.
      for (int k = 0; k < 12; ++k) {
        mhgp::i64 pa[3], pb[3], pc[3], e[3], t[3];
        for (int i = 0; i < 3; ++i) {
          pa[i] = rng.in(a.lo[i], a.hi[i]);
          pb[i] = rng.in(b.lo[i], b.hi[i]);
          pc[i] = rng.in(c.lo[i], c.hi[i]);
          e[i] = pc[i] - pa[i];
          t[i] = pb[i] - pc[i];
        }
        const int lane = lane_of_et(e, t, SocMutant::kNone, nullptr);
        if (lane < c512) {
          std::fprintf(stderr,
                       "DESACCORD : corner512=%s certifie mais un triple porte %s\n",
                       lane_name(c512), lane_name(lane));
          if (++desaccords > 20) break;
        }
      }
    }
  }

  std::printf(
      "selftest accord=%s rectangles=%lld soc64_all_q4=%lld corner512_all_q4=%lld "
      "gain_sur_scalaire=%lld perte_relaxe=%lld exhaustifs=%lld desaccords=%lld\n",
      desaccords == 0 ? "OUI" : "NON", cov.rectangles, cov.soc64_all_q4, cov.corner512_all_q4,
      cov.gain_sur_scalaire, cov.perte_sur_corner512, cov.exhaustifs, desaccords);

  if (desaccords != 0) return 1;

  // -------------------------------------------------------------------------
  // 4. PLANCHERS. Sans eux, une campagne qui ne certifie jamais rien serait
  // verte. `--min-perte-relaxe` est le plancher le plus important : il exige
  // d'avoir OBSERVE le fail-open, donc d'avoir mesure le prix de la relaxation.
  // -------------------------------------------------------------------------
  struct Plancher { const char* nom; long long valeur; long long seuil; };
  const Plancher planchers[] = {
      {"soc64_all_q4", cov.soc64_all_q4, min_soc64_all},
      {"corner512_all_q4", cov.corner512_all_q4, min_corner512_all},
      {"gain_sur_scalaire", cov.gain_sur_scalaire, min_gain_scalaire},
      {"perte_relaxe", cov.perte_sur_corner512, min_perte_relaxe},
      {"exhaustifs", cov.exhaustifs, min_exhaustifs},
  };
  for (const Plancher& p : planchers) {
    if (p.valeur < p.seuil) {
      std::fprintf(stderr, "REFUS : plancher %s non atteint (%lld < %lld)\n", p.nom, p.valeur,
                   p.seuil);
      return 3;
    }
  }
  return 0;
}
