// MorseHGP3D v3 — PORTE DIFFERENTIELLE DE `AxisTop8`.
//
// Specification : theoreme `FaceAxisExtremalCompletion-8` de
// audits/AUDIT_DEBLOCAGE_SOURCE_SHALLOW_SANS_DELAUNAY_20260814.md, tranche
// explicitement demandee a Claude.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CETTE PORTE JUGE, ET AVEC QUEL JUGE
//
// Le SUJET est `axis8::select_top8` : a face aigue owner fixee, il pretend que
// tout apex q4 shallow est parmi au plus `16-2p` racines extremales, et il
// reconstruit le census sans second balayage.
//
// Le JUGE est une SWEEP EXHAUSTIVE : pour chaque site `y`, il forme directement
// le tetraedre, decide la positivite par `c8::bien_centre` (Cramer) et compte
// les interieurs par `c8::interieur_strict` (determinant InSphere d'ordre
// quatre). Les deux chemins ne partagent AUCUNE algebre : le sujet vit sur la
// puissance affine `A_z - tau B_z` le long de l'axe, le juge sur un
// determinant `4x4`. Une faute commune ne peut pas se compenser.
//
// Le desaccord juge : un apex shallow trouve par la sweep et ABSENT de la
// selection refute le theoreme. C'est le code de sortie `1`.
//
// ---------------------------------------------------------------------------
// LES DEUX FIXTURES SHARP
//
// `--fixture-16` grave que la borne seize ne peut pas etre abaissee a quinze :
// dix-neuf points, seize tetraedres bien centres, deux a chaque profondeur de
// zero a sept, un seul owner et un seul carrier primaire.
//
// `--fixture-mort-16` grave que le certificat de mort est lui aussi sharp : sur
// tout le segment de Jung la profondeur minimale vaut exactement huit, et
// retirer n'importe lequel des seize temoins fait tomber un bout a sept. Un
// noyau de mort universel de taille au plus quinze est donc impossible.
//
// ---------------------------------------------------------------------------
// CODES DE SORTIE
//   0  accord
//   1  desaccord du juge (apex shallow hors selection, census faux, borne cassee)
//   2  campagne refusee avant calcul (option, domaine, cap)
//   3  plancher de couverture viole (vert par vacuite)
//   4  mutant tue
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "axis_top8.hpp"
#include "cloud_families.hpp"
#include "corner8_ball.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;
using mhgp3v::axis8::A8Mutant;

struct Pt {
  i64 c[3] = {0, 0, 0};
};

[[noreturn]] void refuse(const std::string& raison) {
  std::fprintf(stderr, "REFUS: %s\n", raison.c_str());
  std::exit(2);
}

i64 d2(const Pt& a, const Pt& b) {
  i64 s = 0;
  for (int i = 0; i < 3; ++i) { const i64 e = a.c[i] - b.c[i]; s += e * e; }
  return s;
}

// `ab` est-elle l'arete maximale canonique de l'ensemble `id` ? A egalite, le
// plus petit couple de vrais `PointId` gagne.
bool arete_owner(const std::vector<Pt>& P, const int* id, int k, int ia, int ib) {
  const i64 D2 = d2(P[(size_t)id[ia]], P[(size_t)id[ib]]);
  const int lo1 = std::min(id[ia], id[ib]), hi1 = std::max(id[ia], id[ib]);
  for (int u = 0; u < k; ++u)
    for (int v = u + 1; v < k; ++v) {
      if ((u == ia && v == ib) || (u == ib && v == ia)) continue;
      const i64 e = d2(P[(size_t)id[u]], P[(size_t)id[v]]);
      if (e > D2) return false;
      if (e == D2) {
        const int lo2 = std::min(id[u], id[v]), hi2 = std::max(id[u], id[v]);
        if (lo2 < lo1 || (lo2 == lo1 && hi2 < hi1)) return false;
      }
    }
  return true;
}

// ---------------------------------------------------------------------------
// LE JUGE : determinant InSphere, aucune algebre partagee avec l'axe.
// ---------------------------------------------------------------------------
struct Verdict {
  bool valide = false;      // bien centre, owner respecte
  long long profondeur = 0;
  long long shell_extra = 0;
};

Verdict juge_tetra(const std::vector<Pt>& P, int ia, int ib, int ix, int iy,
                   long long seuil) {
  Verdict v;
  const int id[4] = {ia, ib, ix, iy};
  const i64* q[4];
  for (int t = 0; t < 4; ++t) q[t] = P[(size_t)id[t]].c;
  if (mhgp3v::c8::orient3d(q[0], q[1], q[2], q[3]) == 0) return v;
  if (!mhgp3v::c8::bien_centre(q[0], q[1], q[2], q[3])) return v;
  if (!arete_owner(P, id, 4, 0, 1)) return v;
  v.valide = true;
  for (size_t z = 0; z < P.size(); ++z) {
    const int zi = (int)z;
    if (zi == ia || zi == ib || zi == ix || zi == iy) continue;
    if (mhgp3v::c8::interieur_strict(q[0], q[1], q[2], q[3], P[z].c)) ++v.profondeur;
  }
  (void)seuil;
  return v;
}

// ---------------------------------------------------------------------------
// UNE FACE : selection extremale, puis confrontation a la sweep.
// ---------------------------------------------------------------------------
struct FaceBilan {
  bool exploree = false;
  bool morte_T2 = false, morte_perm8 = false, morte_gap = false;
  int p = 0, candidats = 0, groupes = 0, shallow = 0;
  int manquants = 0;          // apex shallow ABSENT de la selection : refutation
  int borne_cassee = 0;       // GROUPES de racines > 16 - 2p
  int census_faux = 0;        // `I_B` reconstruit != census exhaustif
  int deborde = 0;            // concurrence au-dela de la capacite : refus
};

FaceBilan face_bilan(const std::vector<Pt>& P, int ia, int ib, int ix,
                     long long seuil, A8Mutant mut, std::vector<int>* scratch) {
  const int mort = (int)seuil + 1;
  FaceBilan bil;
  const i64* a = P[(size_t)ia].c;
  const i64* b = P[(size_t)ib].c;
  const i64* x = P[(size_t)ix].c;
  const auto f = mhgp3v::axis8::face_axis(a, b, x);
  if (!f.reguliere) return bil;
  bil.exploree = true;

  scratch->clear();
  for (size_t z = 0; z < P.size(); ++z) {
    const int zi = (int)z;
    if (zi == ia || zi == ib || zi == ix) continue;
    scratch->push_back(zi);
  }
  auto pw = [&](int i) { return mhgp3v::axis8::site_power(f, P[(size_t)i].c); };
  const auto sel =
      mhgp3v::axis8::select_top8(f, scratch->data(), (int)scratch->size(), pw, mut, mort);
  bil.deborde = sel.debordement ? 1 : 0;
  bil.p = sel.p;
  if (sel.face_morte) {
    if (f.T2 <= 0) bil.morte_T2 = true; else bil.morte_perm8 = true;
  }
  bil.candidats = sel.n_entrants + sel.n_sortants;
  // LA BORNE PORTE SUR LES GROUPES DE RACINES, PAS SUR LES SITES. En cas de
  // concurrence — plusieurs sites cospheriques au meme parametre — un groupe
  // compte une fois ; l'audit le dit explicitement, et un comptage par site
  // ferait faussement croire a une violation.
  {
    const int* tab[2] = {sel.entrants, sel.sortants};
    const int cnt[2] = {sel.n_entrants, sel.n_sortants};
    for (int side = 0; side < 2; ++side)
      for (int t = 0; t < cnt[side]; ++t) {
        const auto pt = pw(tab[side][t]);
        bool deja = false;
        for (int u = 0; u < t && !deja; ++u)
          deja = (mhgp3v::axis8::cmp_racines(pw(tab[side][u]), pt) == 0);
        if (!deja) ++bil.groupes;
      }
  }
  if (!sel.face_morte && bil.groupes > 2 * (mort - sel.p)) bil.borne_cassee = 1;

  // SWEEP EXHAUSTIVE : tous les sites sont des apex candidats.
  for (int y : *scratch) {
    const Verdict v = juge_tetra(P, ia, ib, ix, y, seuil);
    if (!v.valide || v.profondeur > seuil) continue;
    ++bil.shallow;
    bool dedans = false;
    for (int t = 0; t < sel.n_entrants && !dedans; ++t) dedans = (sel.entrants[t] == y);
    for (int t = 0; t < sel.n_sortants && !dedans; ++t) dedans = (sel.sortants[t] == y);
    if (!dedans) { ++bil.manquants; continue; }
    // LE CENSUS RECONSTRUIT. L'audit exige que le meme replay redonne `I_B`
    // sans second balayage global : les permanents, plus les entrants retenus
    // strictement anterieurs, plus les sortants retenus strictement posterieurs.
    // Une egalite est un shell et ne credite jamais l'interieur.
    const auto py = pw(y);
    long long dk = sel.p;
    for (int t = 0; t < sel.n_entrants; ++t) {
      const int z = sel.entrants[t];
      if (z == y) continue;
      if (mhgp3v::axis8::cmp_racines(pw(z), py) < 0) ++dk;
    }
    for (int t = 0; t < sel.n_sortants; ++t) {
      const int z = sel.sortants[t];
      if (z == y) continue;
      if (mhgp3v::axis8::cmp_racines(pw(z), py) > 0) ++dk;
    }
    if (dk != v.profondeur) ++bil.census_faux;
  }
  if (bil.shallow == 0 && !sel.face_morte) bil.morte_gap = true;
  return bil;
}

// ---------------------------------------------------------------------------
// LES DEUX FIXTURES SHARP.
// ---------------------------------------------------------------------------
std::vector<Pt> fixture_face() {
  return {Pt{{125, 100, 100}}, Pt{{93, 124, 100}}, Pt{{93, 76, 100}}};
}

int fixture_16(A8Mutant mut) {
  std::vector<Pt> P = fixture_face();
  for (int h = -33; h <= -26; ++h) P.push_back(Pt{{100, 100, 100 + h}});
  for (int h = 26; h <= 33; ++h) P.push_back(Pt{{100, 100, 100 + h}});
  if (P.size() != 19) refuse("fixture-16 mal construite");

  // L'arete owner est `p1p2` (carre 2304) ; la face primaire porte `p0`, plus
  // petit `PointId` de carrier.
  std::vector<int> scratch;
  const auto bil = face_bilan(P, 1, 2, 0, 7, mut, &scratch);
  long long hist[8] = {0};
  long long total = 0, extra_shell = 0;
  for (size_t y = 3; y < P.size(); ++y) {
    const Verdict v = juge_tetra(P, 1, 2, 0, (int)y, 7);
    if (!v.valide || v.profondeur > 7) continue;
    ++total;
    ++hist[v.profondeur];
    extra_shell += v.shell_extra;
  }
  std::printf("fixture16 : owner=EdgeKey(1,2) carrier_primaire=0 q4=%lld"
              " candidats=%d groupes=%d p=%d borne=%d manquants=%d census_faux=%d"
              " deborde=%d extra_shell=%lld hist=",
              total, bil.candidats, bil.groupes, bil.p, 16 - 2 * bil.p, bil.manquants,
              bil.census_faux, bil.deborde, extra_shell);
  for (int i = 0; i < 8; ++i) std::printf("%s%lld", i ? "," : "", hist[i]);
  std::printf("\n");

  if (mut != A8Mutant::kNone) {
    const bool tue = (total != 16) || (bil.manquants > 0) || (bil.candidats < 16) ||
                     (bil.borne_cassee != 0) || (bil.census_faux != 0) || (bil.deborde != 0);
    if (tue) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::axis8::a8_mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::axis8::a8_mutant_name(mut));
    return 1;
  }
  if (total != 16) { std::fprintf(stderr, "DESACCORD: q4=%lld attendu 16\n", total); return 1; }
  for (int i = 0; i < 8; ++i)
    if (hist[i] != 2) { std::fprintf(stderr, "DESACCORD: hist[%d]=%lld attendu 2\n", i, hist[i]); return 1; }
  if (bil.manquants != 0) { std::fprintf(stderr, "DESACCORD: %d apex shallow hors selection\n", bil.manquants); return 1; }
  if (bil.candidats != 16) { std::fprintf(stderr, "DESACCORD: candidats=%d attendu 16\n", bil.candidats); return 1; }
  if (bil.borne_cassee) { std::fprintf(stderr, "DESACCORD: borne 16-2p cassee\n"); return 1; }
  return 0;
}

// Profondeur minimale sur `J_f`, calculee EXHAUSTIVEMENT par balayage des
// racines : c'est le juge du certificat de mort.
long long profondeur_min_sur_Jf(const std::vector<Pt>& P, int ia, int ib, int ix) {
  const auto f = mhgp3v::axis8::face_axis(P[(size_t)ia].c, P[(size_t)ib].c, P[(size_t)ix].c);
  if (!f.jung_ouverte) return -1;
  std::vector<int> sites;
  for (size_t z = 0; z < P.size(); ++z) {
    const int zi = (int)z;
    if (zi != ia && zi != ib && zi != ix) sites.push_back(zi);
  }
  // Profondeur aux deux bouts et juste apres chaque racine interne.
  long long best = -1;
  auto profondeur_en = [&](const mhgp3v::axis8::SitePower& tau, int bord) -> long long {
    // `bord` = -1 : bout gauche ; +1 : bout droit ; 0 : au parametre `tau`.
    long long d = 0;
    for (int z : sites) {
      const auto p = mhgp3v::axis8::site_power(f, P[(size_t)z].c);
      if (p.B == 0) { if (p.A < 0) ++d; continue; }
      int c;
      if (bord != 0) {
        c = mhgp3v::axis8::cmp_racine_bout(p, f.T2, bord);   // signe de alpha - bord*taumax
        if (p.B > 0) { if (c < 0) ++d; } else { if (c > 0) ++d; }
      } else {
        c = mhgp3v::axis8::cmp_racines(p, tau);              // signe de alpha_z - tau
        if (p.B > 0) { if (c < 0) ++d; } else { if (c > 0) ++d; }
      }
    }
    return d;
  };
  mhgp3v::axis8::SitePower zero;
  best = profondeur_en(zero, -1);
  const long long droite = profondeur_en(zero, +1);
  if (droite < best) best = droite;
  for (int z : sites) {
    const auto p = mhgp3v::axis8::site_power(f, P[(size_t)z].c);
    if (p.B == 0) continue;
    if (mhgp3v::axis8::cmp_racine_bout(p, f.T2, -1) < 0) continue;
    if (mhgp3v::axis8::cmp_racine_bout(p, f.T2, +1) > 0) continue;
    const long long d = profondeur_en(p, 0);
    if (d < best) best = d;
  }
  return best;
}

int fixture_mort_16(A8Mutant mut) {
  std::vector<Pt> base = fixture_face();
  for (int h = -24; h <= -17; ++h) base.push_back(Pt{{100, 100, 100 + h}});
  for (int h = 17; h <= 24; ++h) base.push_back(Pt{{100, 100, 100 + h}});
  if (base.size() != 19) refuse("fixture-mort-16 mal construite");

  const long long dmin = profondeur_min_sur_Jf(base, 1, 2, 0);
  long long dmin_sans_min = 1 << 30;
  for (size_t k = 3; k < base.size(); ++k) {
    std::vector<Pt> Q;
    for (size_t i = 0; i < base.size(); ++i) if (i != k) Q.push_back(base[i]);
    const long long d = profondeur_min_sur_Jf(Q, 1, 2, 0);
    if (d < dmin_sans_min) dmin_sans_min = d;
  }
  // Aucun apex shallow : la face est morte pour q4 malgre `p=0`.
  std::vector<int> scratch;
  const auto bil = face_bilan(base, 1, 2, 0, 7, mut, &scratch);
  std::printf("fixture_mort16 : profondeur_min_Jf=%lld retrait_un_min=%lld p=%d"
              " candidats=%d shallow=%d manquants=%d\n",
              dmin, dmin_sans_min, bil.p, bil.candidats, bil.shallow, bil.manquants);

  if (mut != A8Mutant::kNone) {
    const bool tue = (dmin != 8) || (dmin_sans_min != 7) || (bil.shallow != 0) ||
                     (bil.manquants > 0) || (bil.census_faux != 0);
    if (tue) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::axis8::a8_mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::axis8::a8_mutant_name(mut));
    return 1;
  }
  if (dmin != 8) { std::fprintf(stderr, "DESACCORD: profondeur_min_Jf=%lld attendu 8\n", dmin); return 1; }
  if (dmin_sans_min != 7) { std::fprintf(stderr, "DESACCORD: retrait_un_min=%lld attendu 7\n", dmin_sans_min); return 1; }
  if (bil.shallow != 0) { std::fprintf(stderr, "DESACCORD: %d apex shallow sur une face morte\n", bil.shallow); return 1; }
  return 0;
}


// Le tetraedre REGULIER realise la borne de Jung a l'EGALITE : la racine de son
// apex tombe exactement sur le bout de `J_f`. C'est la seule fixture qui separe
// un bout ferme d'un bout ouvert, et elle tue `a8-bouts-larges`.
int fixture_jung_tendu(A8Mutant mut) {
  // Tetraedre regulier, plus UN temoin cospherique du bout : `w` est a distance
  // carree `75` du circumcentre `(105,105,105)`, donc exactement SUR la sphere
  // de parametre `+tau_max`, et du cote negatif du plan de face `x+y-z=100`.
  // Il est donc un SORTANT dont la racine sature le bout droit, jamais un
  // permanent : le mutant `a8-permanence-large` le credite a tort et fausse le
  // census. Un shell n'est jamais un interieur.
  std::vector<Pt> P = {Pt{{100, 100, 100}}, Pt{{100, 110, 110}},
                       Pt{{110, 100, 110}}, Pt{{110, 110, 100}},
                       Pt{{98, 100, 104}}};
  const auto f = mhgp3v::axis8::face_axis(P[0].c, P[1].c, P[2].c);
  // Toutes les aretes valent 200 ; `T2 = D(G-2(E-F)^2)` doit etre STRICTEMENT
  // positif et la racine de l'apex doit saturer `2 tau^2 = T2`.
  const auto py = mhgp3v::axis8::site_power(f, P[3].c);
  const int bout = mhgp3v::axis8::cmp_racine_bout(py, f.T2, +1);
  std::vector<int> scratch;
  // Les DEUX faces owner sont exercees : `(0,1,2)` a `B_apex>0` et sa racine au
  // bout DROIT, `(0,1,3)` a `B_apex<0` et sa racine au bout GAUCHE. Les deux
  // orientations sont donc couvertes.
  const auto bil = face_bilan(P, 0, 1, 2, 7, mut, &scratch);
  const auto bil2 = face_bilan(P, 0, 1, 3, 7, mut, &scratch);
  const Verdict v = juge_tetra(P, 0, 1, 2, 3, 7);
  std::printf("fixture_jung_tendu : T2=%lld bout_apex=%d valide=%d profondeur=%lld"
              " candidats=%d shallow=%d manquants=%d census_faux=%d"
              " face2_candidats=%d face2_shallow=%d face2_manquants=%d\n",
              (long long)f.T2, bout, (int)v.valide, v.profondeur, bil.candidats,
              bil.shallow, bil.manquants, bil.census_faux,
              bil2.candidats, bil2.shallow, bil2.manquants);
  if (mut != A8Mutant::kNone) {
    const bool tue = (bil.manquants > 0) || (bil.candidats < 1) || (bil.census_faux != 0) ||
                     (bil2.manquants > 0) || (bil2.candidats < 1) || (bil2.census_faux != 0);
    if (tue) { std::printf("mutant_killed=1 %s\n", mhgp3v::axis8::a8_mutant_name(mut)); return 4; }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::axis8::a8_mutant_name(mut));
    return 1;
  }
  if (f.T2 <= 0) { std::fprintf(stderr, "DESACCORD: T2=%lld attendu > 0\n", (long long)f.T2); return 1; }
  if (bout != 0) { std::fprintf(stderr, "DESACCORD: la racine de l'apex ne sature pas le bout\n"); return 1; }
  if (!v.valide || v.profondeur != 0) { std::fprintf(stderr, "DESACCORD: apex regulier non valide\n"); return 1; }
  if (bil.shallow != 1) { std::fprintf(stderr, "DESACCORD: shallow=%d attendu 1\n", bil.shallow); return 1; }
  if (bil.manquants != 0) { std::fprintf(stderr, "DESACCORD: apex de bout perdu\n"); return 1; }
  if (bil2.manquants != 0 || bil2.candidats < 1) {
    std::fprintf(stderr, "DESACCORD: apex de bout perdu sur la face miroir\n");
    return 1;
  }
  return 0;
}

// Une face OWNER STRICTEMENT AIGUE a toujours `T2 > 0`. Preuve : `T2 > 0` vaut
// `sin^2 C > 2/3` ou `C` est l'angle oppose a l'arete owner ; comme celle-ci est
// maximale, `C` est le plus grand angle, donc `C >= 60 deg`, et l'acuite donne
// `C < 90 deg`, d'ou `sin^2 C >= 3/4 > 2/3`. La branche `T2 <= 0` est donc
// INATTEIGNABLE sous acuite owner. Cette fixture le grave des deux cotes : un
// triangle obtus a `C > 125,26 deg` est bien refuse, et la campagne verifie que
// `mortes_T2` reste nul sur les faces aigues.
int fixture_t2_positif() {
  // `C` vaut 150 degres : obtus au-dela de 125,26 deg, donc `T2 < 0`.
  std::vector<Pt> Q = {Pt{{0, 0, 0}}, Pt{{1000, 0, 0}}, Pt{{500, 67, 0}}};
  const auto fo = mhgp3v::axis8::face_axis(Q[0].c, Q[1].c, Q[2].c);
  const bool obtus = !mhgp3v::axis8::face_aigue(Q[0].c, Q[1].c, Q[2].c);
  // Equilateral : le cas extremal `C = 60 deg`, ou `T2` est le plus petit
  // possible sous acuite.
  std::vector<Pt> E = {Pt{{0, 0, 0}}, Pt{{1000, 0, 0}}, Pt{{500, 866, 0}}};
  const auto fe = mhgp3v::axis8::face_axis(E[0].c, E[1].c, E[2].c);
  const bool aigu = mhgp3v::axis8::face_aigue(E[0].c, E[1].c, E[2].c);
  // La face obtuse doit etre REFUSEE par la selection : `T2 <= 0` n'admet
  // aucune completion q4 strictement positive. Le mutant qui accepte `T2 = 0`
  // se mesure ici, la ou la branche est atteignable ; sous acuite owner elle
  // ne l'est jamais.
  std::vector<Pt> Qo = Q;
  Qo.push_back(Pt{{500, 30, 400}});
  std::vector<int> sc;
  const auto bo = face_bilan(Qo, 0, 1, 2, 7, A8Mutant::kNone, &sc);
  std::printf("fixture_t2_positif : obtus=%d T2_obtus_negatif=%d aigu=%d T2_aigu_positif=%d"
              " obtuse_morte_T2=%d obtuse_candidats=%d\n",
              (int)obtus, (int)(fo.T2 < 0), (int)aigu, (int)(fe.T2 > 0),
              (int)bo.morte_T2, bo.candidats);
  if (!bo.morte_T2 || bo.candidats != 0) {
    std::fprintf(stderr, "DESACCORD: la face T2<0 n'est pas refusee\n");
    return 1;
  }
  if (!obtus || fo.T2 >= 0) { std::fprintf(stderr, "DESACCORD: la face obtuse n'a pas T2<0\n"); return 1; }
  if (!aigu || fe.T2 <= 0) { std::fprintf(stderr, "DESACCORD: la face aigue n'a pas T2>0\n"); return 1; }
  return 0;
}

// ---------------------------------------------------------------------------
// CAMPAGNE DIFFERENTIELLE SUR NUAGE.
// ---------------------------------------------------------------------------
int campagne(const std::string& family, long long n, long long coord, long long seed,
             long long seuil, long long threads, A8Mutant mut,
             long long min_faces, long long min_shallow) {
  if (n < 5 || n > 400) refuse("la campagne exhaustive est bornee a 400 points");
  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "two_lines") fam = mhgp3v::CloudFamily::kTwoLines;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho")
    fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else refuse("famille inconnue : " + family);
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(fam, (int)n);
  const auto brut = mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
  std::vector<Pt> P;
  P.reserve(brut.size());
  for (const auto& q : brut) P.push_back(Pt{{q.x, q.y, q.z}});
  const int m = (int)P.size();

  struct Acc {
    long long faces = 0, aigues = 0, mortes_T2 = 0, mortes_perm8 = 0, mortes_gap = 0;
    long long candidats = 0, groupes = 0, shallow = 0, manquants = 0, bornes = 0;
    long long census = 0;
    long long cand_max = 0, debordes = 0;
  };
  std::vector<Acc> acc((size_t)threads);
  auto worker = [&](long long tid) {
    Acc& A = acc[(size_t)tid];
    std::vector<int> scratch;
    for (int ia = (int)tid; ia < m; ia += (int)threads)
      for (int ib = ia + 1; ib < m; ++ib)
        for (int ix = 0; ix < m; ++ix) {
          if (ix == ia || ix == ib) continue;
          const int id[3] = {ia, ib, ix};
          ++A.faces;
          if (!arete_owner(P, id, 3, 0, 1)) continue;
          if (!mhgp3v::axis8::face_aigue(P[(size_t)ia].c, P[(size_t)ib].c, P[(size_t)ix].c))
            continue;
          ++A.aigues;
          const auto b = face_bilan(P, ia, ib, ix, seuil, mut, &scratch);
          if (!b.exploree) continue;
          A.mortes_T2 += b.morte_T2;
          A.mortes_perm8 += b.morte_perm8;
          A.mortes_gap += b.morte_gap;
          A.candidats += b.candidats;
          A.groupes += b.groupes;
          A.debordes += b.deborde;
          A.shallow += b.shallow;
          A.manquants += b.manquants;
          A.bornes += b.borne_cassee;
          A.census += b.census_faux;
          if (b.candidats > A.cand_max) A.cand_max = b.candidats;
        }
  };
  std::vector<std::thread> th;
  for (long long t = 0; t < threads; ++t) th.emplace_back(worker, t);
  for (auto& t : th) t.join();
  Acc g;
  for (const Acc& a : acc) {
    g.faces += a.faces; g.aigues += a.aigues; g.mortes_T2 += a.mortes_T2;
    g.mortes_perm8 += a.mortes_perm8; g.mortes_gap += a.mortes_gap;
    g.candidats += a.candidats; g.groupes += a.groupes; g.shallow += a.shallow;
    g.manquants += a.manquants; g.bornes += a.bornes; g.debordes += a.debordes;
    g.census += a.census;
    if (a.cand_max > g.cand_max) g.cand_max = a.cand_max;
  }
  std::printf("axis_top8_juge : famille=%s n=%d seuil=%lld faces=%lld aigues_owner=%lld"
              " mortes_T2=%lld mortes_perm8=%lld mortes_gap=%lld candidats=%lld"
              " groupes=%lld cand_max=%lld shallow=%lld manquants=%lld"
              " bornes_cassees=%lld census_faux=%lld debordes=%lld\n",
              family.c_str(), m, seuil, g.faces, g.aigues, g.mortes_T2, g.mortes_perm8,
              g.mortes_gap, g.candidats, g.groupes, g.cand_max, g.shallow, g.manquants,
              g.bornes, g.census, g.debordes);

  if (mut != A8Mutant::kNone) {
    if (g.manquants > 0 || g.bornes > 0 || g.census > 0) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::axis8::a8_mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::axis8::a8_mutant_name(mut));
    return 1;
  }
  if (g.aigues < min_faces) {
    std::fprintf(stderr, "PLANCHER: %lld faces aigues owner < %lld\n", g.aigues, min_faces);
    return 3;
  }
  if (g.shallow < min_shallow) {
    std::fprintf(stderr, "PLANCHER: %lld apex shallow < %lld\n", g.shallow, min_shallow);
    return 3;
  }
  if (g.manquants > 0) {
    std::fprintf(stderr, "DESACCORD: %lld apex shallow hors selection\n", g.manquants);
    return 1;
  }
  if (g.bornes > 0) {
    std::fprintf(stderr, "DESACCORD: %lld faces depassent 16-2p groupes\n", g.bornes);
    return 1;
  }
  if (g.census > 0) {
    std::fprintf(stderr, "DESACCORD: %lld census reconstruits faux\n", g.census);
    return 1;
  }
  if (g.debordes > 0) {
    std::fprintf(stderr, "PLANCHER: %lld faces en debordement de capacite\n", g.debordes);
    return 3;
  }
  return 0;
}

A8Mutant mutant_de(const std::string& s) {
  if (s == "a8-cap15") return A8Mutant::kCap15;
  if (s == "a8-k-fixe-7") return A8Mutant::kKFixe7;
  if (s == "a8-signe-b-inverse") return A8Mutant::kSigneBInverse;
  if (s == "a8-bouts-ouverts") return A8Mutant::kBoutsOuverts;
  if (s == "a8-permanence-large") return A8Mutant::kPermanenceLarge;
  if (s == "a8-oublie-b-zero") return A8Mutant::kOublieBZero;
  if (s == "a8-abs-avant-tri") return A8Mutant::kAbsAvantTri;
  refuse("mutant inconnu : " + s);
}

}  // namespace

int main(int argc, char** argv) {
  std::string family, mode;
  long long n = 60, coord = 0, seed = 1, seuil = 7, threads = 0;
  long long min_faces = 1, min_shallow = 1;
  A8Mutant mut = A8Mutant::kNone;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a == "--fixture-16" || a == "--fixture-mort-16" || a == "--sweep" ||
             a == "--fixture-jung-tendu" || a == "--fixture-t2-positif") mode = a;
    else if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = atoll(val("--points=").c_str());
    else if (a.rfind("--coord=", 0) == 0) coord = atoll(val("--coord=").c_str());
    else if (a.rfind("--seed=", 0) == 0) seed = atoll(val("--seed=").c_str());
    else if (a.rfind("--seuil=", 0) == 0) seuil = atoll(val("--seuil=").c_str());
    else if (a.rfind("--threads=", 0) == 0) threads = atoll(val("--threads=").c_str());
    else if (a.rfind("--min-faces=", 0) == 0) min_faces = atoll(val("--min-faces=").c_str());
    else if (a.rfind("--min-shallow=", 0) == 0) min_shallow = atoll(val("--min-shallow=").c_str());
    else if (a.rfind("--inject=", 0) == 0) mut = mutant_de(val("--inject="));
    else refuse("option inconnue : " + a);
  }
  if (mode.empty())
    refuse("--fixture-16, --fixture-mort-16, --fixture-jung-tendu, --fixture-t2-positif"
           " ou --sweep est exige");
  if (seuil < 0 || seuil > 15) refuse("--seuil hors domaine");
  if (threads <= 0) threads = (long long)std::thread::hardware_concurrency();
  if (threads <= 0) threads = 1;
  if (mode == "--fixture-16") return fixture_16(mut);
  if (mode == "--fixture-mort-16") return fixture_mort_16(mut);
  if (mode == "--fixture-jung-tendu") return fixture_jung_tendu(mut);
  if (mode == "--fixture-t2-positif") return fixture_t2_positif();
  if (family.empty()) refuse("--sweep exige --family");
  return campagne(family, n, coord, seed, seuil, threads, mut, min_faces, min_shallow);
}
