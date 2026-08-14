// MorseHGP3D v3 — PORTE DIFFERENTIELLE DE `Q4SeedAxisTopR4`.
//
// Specification : PROPOSITION.md section 7.2. Reception demandee par
// audits/AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CETTE PORTE JUGE, ET AVEC QUEL JUGE
//
// Le SUJET est `q4axis::select_axis_topr4` et son replay de census. Le JUGE est
// exhaustif et vit sur le determinant InSphere d'ordre quatre de
// `corner8_ball.hpp` : `insphere_j<0`, `==0`, `>0` classe interieur, shell et
// exterieur. Le sujet vit sur la puissance AFFINE `A_z - tau B_z` le long de
// l'axe. Aucune algebre n'est partagee : une faute commune ne se compense pas.
//
// QUATRE CHOSES SONT JUGEES, PAS UNE.
//   1. la SELECTION : tout apex shallow est-il dans `First_k union Last_k` ?
//   2. les IDENTITES : `I_B` et `U_B` reconstruits sont-ils les vraies listes
//      triees de `PointId`, et non des cardinaux ?
//   3. la MORT PAR GAPS : le verdict `MORT_GAP` du sujet coincide-t-il avec la
//      profondeur minimale exhaustive sur `J_f` ?
//   4. l'EXACT-ONCE : le multiensemble global des `SupportKey` q4 produits par
//      `Lane4` egale-t-il celui du brute force `C(n,4)`, sans manque ni doublon,
//      avec les memes owners et les memes provenances primaires ?
//
// ---------------------------------------------------------------------------
// L'INDEPENDANCE DES LANES EST UNE PROPRIETE TESTEE, PAS UNE CONVENTION
//
// `Lane4` construit ses `Q4Seed3` depuis son propre univers. Le mode
// `--exact-once` le verifie en publiant `seeds_rang_q3_mort` : le nombre de
// prefixes generateurs dont la miniboule PROPRE porte plus de `smax-2`
// interieurs, donc morts pour q3, et qui produisent pourtant un q4 pertinent.
// Si ce compteur est nul sur toutes les familles, la fixture d'independance
// n'est pas exercee et le plancher `--min-q3-morts` refuse la campagne.
//
// ---------------------------------------------------------------------------
// CODES DE SORTIE
//   0  accord
//   1  desaccord du juge (apex hors selection, identites fausses, gap faux,
//      manque ou doublon exact-once)
//   2  campagne refusee avant calcul (option, domaine, cap)
//   3  plancher de couverture viole (vert par vacuite)
//   4  mutant tue
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <string>
#include <thread>
#include <vector>

#include "cloud_families.hpp"
#include "corner8_ball.hpp"
#include "q4seed_axis_topr4.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;
using mhgp3v::q4axis::Mutant;
using mhgp3v::q4axis::Profil;
using mhgp3v::q4axis::SeedVerdict;

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

// `ab` est-elle l'arete maximale canonique de `id[0..k)` ? A egalite, le plus
// petit couple de vrais `PointId` gagne.
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
// LE JUGE : `insphere_j` signe, trois classes, vraies identites.
// ---------------------------------------------------------------------------
struct Verdict {
  bool positif = false;      // bien centre
  bool owner = false;        // `ab` est l'arete maximale canonique
  std::vector<int> I_B, U_B;
};

Verdict juge_tetra(const std::vector<Pt>& P, int ia, int ib, int ix, int iy) {
  Verdict v;
  const int id[4] = {ia, ib, ix, iy};
  const i64* q[4];
  for (int t = 0; t < 4; ++t) q[t] = P[(size_t)id[t]].c;
  const i128 O = mhgp3v::c8::orient3d(q[0], q[1], q[2], q[3]);
  if (O == 0) return v;
  if (!mhgp3v::c8::bien_centre(q[0], q[1], q[2], q[3])) return v;
  v.positif = true;
  v.owner = arete_owner(P, id, 4, 0, 1);
  for (int t = 0; t < 4; ++t) v.U_B.push_back(id[t]);
  for (size_t z = 0; z < P.size(); ++z) {
    const int zi = (int)z;
    if (zi == ia || zi == ib || zi == ix || zi == iy) continue;
    const i128 J = mhgp3v::c8::insphere_j(q[0], q[1], q[2], q[3], P[z].c);
    if (J == 0) { v.U_B.push_back(zi); continue; }
    const bool dedans = ((O > 0) != (J > 0));
    if (dedans) v.I_B.push_back(zi);
  }
  std::sort(v.I_B.begin(), v.I_B.end());
  std::sort(v.U_B.begin(), v.U_B.end());
  return v;
}

// Profondeur minimale EXHAUSTIVE sur `J_f` : le juge de la mort par gaps.
long long profondeur_min_exhaustive(const mhgp3v::q4axis::SeedAxis& f,
                                    const std::vector<Pt>& P,
                                    const std::vector<int>& sites) {
  if (!f.jung_ouverte) return -1;
  auto pw = [&](int i) { return mhgp3v::q4axis::site_power(f, P[(size_t)i].c); };
  auto profondeur = [&](const mhgp3v::q4axis::SitePower* tau, int bord) -> long long {
    long long d = 0;
    for (int z : sites) {
      const auto p = pw(z);
      if (p.B == 0) { if (p.A < 0) ++d; continue; }
      int c;
      if (bord != 0) c = mhgp3v::q4axis::cmp_racine_bout(p, f.T2, bord);
      else c = mhgp3v::q4axis::cmp_racines(p, *tau);
      if (p.B > 0) { if (c < 0) ++d; } else { if (c > 0) ++d; }
    }
    return d;
  };
  long long best = profondeur(nullptr, -1);
  const long long droite = profondeur(nullptr, +1);
  if (droite < best) best = droite;
  for (int z : sites) {
    const auto p = pw(z);
    if (p.B == 0) continue;
    if (mhgp3v::q4axis::cmp_racine_bout(p, f.T2, -1) < 0) continue;
    if (mhgp3v::q4axis::cmp_racine_bout(p, f.T2, +1) > 0) continue;
    const long long d = profondeur(&p, 0);
    if (d < best) best = d;
  }
  return best;
}

// ---------------------------------------------------------------------------
// UN `Q4Seed3` : selection, identites, gaps.
// ---------------------------------------------------------------------------
struct Bilan {
  bool exploree = false;
  long long morts_degen = 0, morts_T2 = 0, morts_perm = 0, morts_gap = 0, ouverts = 0;
  long long candidats = 0, groupes = 0, shallow = 0;
  long long manquants = 0, bornes = 0, identites_fausses = 0, gaps_faux = 0;
  long long debordes = 0, degeneres = 0, cand_max = 0;
  long long fate_exact = 0, fate_cap = 0, fate_degen = 0, fate_hors = 0;
  long long doublons_bruts = 0, refus_abusifs = 0, ids_max = 0;
};

void bilan_seed(const std::vector<Pt>& P, int ia, int ib, int ix, int r4,
                Mutant mut, std::vector<int>* scratch, Bilan* out,
                Profil profil = Profil::kRelevantGp) {
  const auto f = mhgp3v::q4axis::seed_axis(P[(size_t)ia].c, P[(size_t)ib].c, P[(size_t)ix].c);
  scratch->clear();
  for (size_t z = 0; z < P.size(); ++z) {
    const int zi = (int)z;
    if (zi != ia && zi != ib && zi != ix) scratch->push_back(zi);
  }
  auto pw = [&](int i) { return mhgp3v::q4axis::site_power(f, P[(size_t)i].c); };
  const auto sel = mhgp3v::q4axis::select_axis_topr4(f, scratch->data(),
                                                     (int)scratch->size(), pw, r4, mut);
  out->exploree = true;
  switch (sel.verdict) {
    case SeedVerdict::kMortDegeneree: ++out->morts_degen; return;
    case SeedVerdict::kMortT2: ++out->morts_T2; return;
    case SeedVerdict::kMortPermanents: ++out->morts_perm; break;
    case SeedVerdict::kMortGap: ++out->morts_gap; break;
    case SeedVerdict::kDebordement: ++out->debordes; return;
    case SeedVerdict::kOuvert: ++out->ouverts; break;
  }
  const long long cand = sel.n_entrants + sel.n_sortants;
  out->candidats += cand;
  if (cand > out->cand_max) out->cand_max = cand;
  // La borne porte sur les GROUPES de racines, pas sur les sites.
  long long groupes = 0;
  {
    const int* tab[2] = {sel.entrants, sel.sortants};
    const int cnt[2] = {sel.n_entrants, sel.n_sortants};
    for (int side = 0; side < 2; ++side)
      for (int t = 0; t < cnt[side]; ++t) {
        bool deja = false;
        for (int u = 0; u < t && !deja; ++u)
          deja = (mhgp3v::q4axis::cmp_racines(pw(tab[side][u]), pw(tab[side][t])) == 0);
        if (!deja) ++groupes;
      }
  }
  out->groupes += groupes;
  if (sel.verdict != SeedVerdict::kMortPermanents && groupes > 2 * (r4 - sel.p))
    ++out->bornes;

  // JUGE DE LA MORT PAR GAPS : le verdict typé du sujet contre la profondeur
  // minimale exhaustive. Aucun des deux ne derive de l'autre.
  const long long dmin = profondeur_min_exhaustive(f, P, *scratch);
  const bool morte_reelle = (dmin >= r4);
  const bool morte_sujet = (sel.verdict == SeedVerdict::kMortGap ||
                            sel.verdict == SeedVerdict::kMortPermanents);
  if (morte_sujet != morte_reelle) ++out->gaps_faux;

  // SWEEP EXHAUSTIVE : selection puis IDENTITES.
  const int seed3[3] = {ia, ib, ix};
  for (int y : *scratch) {
    const Verdict v = juge_tetra(P, ia, ib, ix, y);
    if (!v.positif || !v.owner || (long long)v.I_B.size() >= r4) continue;
    ++out->shallow;
    bool dedans = false;
    for (int t = 0; t < sel.n_entrants && !dedans; ++t) dedans = (sel.entrants[t] == y);
    for (int t = 0; t < sel.n_sortants && !dedans; ++t) dedans = (sel.sortants[t] == y);
    if (!dedans) { ++out->manquants; continue; }
    const auto c = mhgp3v::q4axis::census_replay(sel, y, seed3, pw, mut, profil);
    switch (c.fate) {
      case mhgp3v::q4axis::CensusFate::kExact: ++out->fate_exact; break;
      case mhgp3v::q4axis::CensusFate::kPendingCap: ++out->fate_cap; break;
      case mhgp3v::q4axis::CensusFate::kUnsupportedDegeneracy: ++out->fate_degen; break;
      case mhgp3v::q4axis::CensusFate::kHorsDomaine: ++out->fate_hors; break;
    }
    if (c.degenere) ++out->degeneres;
    if (c.fate != mhgp3v::q4axis::CensusFate::kExact) {
      // UN FATE N'EST PAS UN ECHEC, mais il ne doit pas etre ABUSIF : si le
      // census exhaustif est trivial — quatre IDs de shell, aucun ex aequo — le
      // sujet n'avait aucune raison de refuser.
      if (v.U_B.size() == 4) ++out->refus_abusifs;
      continue;
    }
    // L'UNICITE BRUTE EST VERIFIEE AVANT toute canonicalisation : trier puis
    // `unique` masquerait un doublon accidentel d'ID.
    std::vector<int> IB(c.interieur, c.interieur + c.n_interieur);
    std::vector<int> UB(c.shell, c.shell + c.n_shell);
    if ((int)UB.size() > out->ids_max) out->ids_max = (long long)UB.size();
    {
      std::vector<int> t = UB;
      std::sort(t.begin(), t.end());
      if (std::unique(t.begin(), t.end()) != t.end()) ++out->doublons_bruts;
      std::vector<int> ti = IB;
      std::sort(ti.begin(), ti.end());
      if (std::unique(ti.begin(), ti.end()) != ti.end()) ++out->doublons_bruts;
    }
    std::sort(IB.begin(), IB.end());
    std::sort(UB.begin(), UB.end());
    if (IB != v.I_B || UB != v.U_B) ++out->identites_fausses;
  }
}

// ---------------------------------------------------------------------------
// FIXTURES SHARP.
// ---------------------------------------------------------------------------
std::vector<Pt> seed_sharp() {
  return {Pt{{125, 100, 100}}, Pt{{93, 124, 100}}, Pt{{93, 76, 100}}};
}

int fixture_16(int r4, Mutant mut) {
  std::vector<Pt> P = seed_sharp();
  for (int h = -33; h <= -26; ++h) P.push_back(Pt{{100, 100, 100 + h}});
  for (int h = 26; h <= 33; ++h) P.push_back(Pt{{100, 100, 100 + h}});
  if (P.size() != 19) refuse("fixture-16 mal construite");
  std::vector<int> scratch;
  Bilan b;
  bilan_seed(P, 1, 2, 0, r4, mut, &scratch, &b);
  long long hist[16] = {0}, total = 0;
  for (size_t y = 3; y < P.size(); ++y) {
    const Verdict v = juge_tetra(P, 1, 2, 0, (int)y);
    if (!v.positif || !v.owner || (long long)v.I_B.size() >= r4) continue;
    ++total;
    ++hist[v.I_B.size()];
  }
  std::printf("fixture16 : r4=%d owner=EdgeKey(1,2) q4=%lld candidats=%lld groupes=%lld"
              " manquants=%lld identites_fausses=%lld gaps_faux=%lld degeneres=%lld hist=",
              r4, total, b.candidats, b.groupes, b.manquants, b.identites_fausses,
              b.gaps_faux, b.degeneres);
  for (int i = 0; i < 8; ++i) std::printf("%s%lld", i ? "," : "", hist[i]);
  std::printf("\n");
  const bool nominal = (total == 16) && (b.candidats == 16) && (b.groupes == 16) &&
                       (b.manquants == 0) && (b.identites_fausses == 0) &&
                       (b.gaps_faux == 0) && (b.degeneres == 0) && (b.debordes == 0) &&
                       [&] { for (int i = 0; i < 8; ++i) if (hist[i] != 2) return false;
                             return true; }();
  if (mut != Mutant::kNone) {
    if (!nominal) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::q4axis::mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::q4axis::mutant_name(mut));
    return 1;
  }
  if (!nominal) { std::fprintf(stderr, "DESACCORD: fixture 16 non nominale\n"); return 1; }
  return 0;
}

int fixture_mort_16(int r4, Mutant mut) {
  std::vector<Pt> P = seed_sharp();
  for (int h = -24; h <= -17; ++h) P.push_back(Pt{{100, 100, 100 + h}});
  for (int h = 17; h <= 24; ++h) P.push_back(Pt{{100, 100, 100 + h}});
  if (P.size() != 19) refuse("fixture-mort-16 mal construite");
  std::vector<int> sites;
  for (size_t z = 3; z < P.size(); ++z) sites.push_back((int)z);
  const auto f = mhgp3v::q4axis::seed_axis(P[1].c, P[2].c, P[0].c);
  const long long dmin = profondeur_min_exhaustive(f, P, sites);
  long long dmin_sans_un = 1 << 30;
  for (size_t k = 3; k < P.size(); ++k) {
    std::vector<Pt> Q;
    std::vector<int> S;
    for (size_t i = 0; i < P.size(); ++i) if (i != k) Q.push_back(P[i]);
    for (size_t z = 3; z < Q.size(); ++z) S.push_back((int)z);
    const auto g = mhgp3v::q4axis::seed_axis(Q[1].c, Q[2].c, Q[0].c);
    const long long d = profondeur_min_exhaustive(g, Q, S);
    if (d < dmin_sans_un) dmin_sans_un = d;
  }
  auto pw = [&](int i) { return mhgp3v::q4axis::site_power(f, P[(size_t)i].c); };
  const auto sel = mhgp3v::q4axis::select_axis_topr4(f, sites.data(), (int)sites.size(),
                                                     pw, r4, mut);
  std::vector<int> scratch;
  Bilan b;
  bilan_seed(P, 1, 2, 0, r4, mut, &scratch, &b);
  // LE DOMAINE DE LA PREUVE EST EXERCE ICI, ET NULLE PART AILLEURS. Sur un seed
  // `MORT_GAP` aucun apex n'est pertinent : le replay doit rendre
  // `HORS_DOMAINE`, jamais un census. Sans cette assertion la precondition
  // resterait un commentaire.
  const char* fate_mort = "AUCUN";
  if (sel.n_entrants + sel.n_sortants > 0) {
    const int apex = sel.n_entrants ? sel.entrants[0] : sel.sortants[0];
    const int seed3[3] = {1, 2, 0};
    fate_mort = mhgp3v::q4axis::census_fate_name(
        mhgp3v::q4axis::census_replay(sel, apex, seed3, pw, mut).fate);
  }
  std::printf("fixture_mort16 : r4=%d profondeur_min_Jf=%lld retrait_un_min=%lld"
              " verdict=%s minoree=%d shallow=%lld gaps_faux=%lld fate_apex=%s\n",
              r4, dmin, dmin_sans_un, mhgp3v::q4axis::verdict_name(sel.verdict),
              sel.profondeur_min_minoree, b.shallow, b.gaps_faux, fate_mort);
  const bool nominal = (dmin == 8) && (dmin_sans_un == 7) && (b.shallow == 0) &&
                       (b.gaps_faux == 0) && (sel.verdict == SeedVerdict::kMortGap) &&
                       (sel.profondeur_min_minoree == 8) &&
                       (std::string(fate_mort) == "HORS_DOMAINE");
  if (mut != Mutant::kNone) {
    if (!nominal) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::q4axis::mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::q4axis::mutant_name(mut));
    return 1;
  }
  if (!nominal) { std::fprintf(stderr, "DESACCORD: la mort par gaps n'est pas typee\n"); return 1; }
  return 0;
}

int fixture_jung_tendu(int r4, Mutant mut) {
  // Tetraedre REGULIER : il sature Jung a l'egalite, donc la racine de son apex
  // tombe exactement sur le bout de `J_f`. Plus un temoin `(98,100,104)`
  // cospherique du meme bout : c'est un SHELL, jamais un interieur.
  std::vector<Pt> P = {Pt{{100, 100, 100}}, Pt{{100, 110, 110}},
                       Pt{{110, 100, 110}}, Pt{{110, 110, 100}},
                       Pt{{98, 100, 104}}};
  const auto f = mhgp3v::q4axis::seed_axis(P[0].c, P[1].c, P[2].c);
  const auto py = mhgp3v::q4axis::site_power(f, P[3].c);
  const int bout = mhgp3v::q4axis::cmp_racine_bout(py, f.T2, +1);
  std::vector<int> scratch;
  Bilan b1, b2;
  // Le temoin cospherique fait de ce cas un PLATEAU declare : sous
  // `RelevantGP` il rendrait `unsupported_degeneracy`, ce qui est correct mais
  // n'exercerait plus les identites. La fixture `--fixture-plateau` teste
  // l'autre profil.
  bilan_seed(P, 0, 1, 2, r4, mut, &scratch, &b1, Profil::kPlateau);
  bilan_seed(P, 0, 1, 3, r4, mut, &scratch, &b2, Profil::kPlateau);
  std::printf("fixture_jung_tendu : T2=%lld bout_apex=%d s1=%lld m1=%lld i1=%lld"
              " s2=%lld m2=%lld i2=%lld degeneres=%lld\n",
              (long long)f.T2, bout, b1.shallow, b1.manquants, b1.identites_fausses,
              b2.shallow, b2.manquants, b2.identites_fausses,
              b1.degeneres + b2.degeneres);
  const bool nominal = (f.T2 > 0) && (bout == 0) && (b1.shallow == 1) &&
                       (b1.manquants == 0) && (b1.identites_fausses == 0) &&
                       (b2.shallow == 1) && (b2.manquants == 0) &&
                       (b2.identites_fausses == 0);
  if (mut != Mutant::kNone) {
    if (!nominal) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::q4axis::mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::q4axis::mutant_name(mut));
    return 1;
  }
  if (!nominal) { std::fprintf(stderr, "DESACCORD: le bout ferme n'est pas respecte\n"); return 1; }
  return 0;
}

int fixture_t2(int r4) {
  // `C=150` degres : au-dela de `125,26`, donc `T2<0`. La branche est
  // INATTEIGNABLE sous acuite owner, elle reste testee sur un prefixe obtus.
  std::vector<Pt> Q = {Pt{{0, 0, 0}}, Pt{{1000, 0, 0}}, Pt{{500, 67, 0}},
                       Pt{{500, 30, 400}}};
  const auto fo = mhgp3v::q4axis::seed_axis(Q[0].c, Q[1].c, Q[2].c);
  const bool obtus = !mhgp3v::q4axis::seed_aigu(Q[0].c, Q[1].c, Q[2].c);
  std::vector<Pt> E = {Pt{{0, 0, 0}}, Pt{{1000, 0, 0}}, Pt{{500, 866, 0}}};
  const auto fe = mhgp3v::q4axis::seed_axis(E[0].c, E[1].c, E[2].c);
  const bool aigu = mhgp3v::q4axis::seed_aigu(E[0].c, E[1].c, E[2].c);
  std::vector<int> scratch;
  Bilan b;
  bilan_seed(Q, 0, 1, 2, r4, Mutant::kNone, &scratch, &b);
  std::printf("fixture_t2 : obtus=%d T2_obtus_negatif=%d aigu=%d T2_aigu_positif=%d"
              " verdict_obtus_mort_T2=%lld candidats=%lld\n",
              (int)obtus, (int)(fo.T2 < 0), (int)aigu, (int)(fe.T2 > 0),
              b.morts_T2, b.candidats);
  if (!obtus || fo.T2 >= 0 || !aigu || fe.T2 <= 0 || b.morts_T2 != 1 || b.candidats != 0) {
    std::fprintf(stderr, "DESACCORD: la branche T2 n'est pas exercee des deux cotes\n");
    return 1;
  }
  return 0;
}


// LA CONTRE-FIXTURE DE L'AUDITEUR : quatre-vingt-dix-sept IDs de MEME racine.
// Le `Q4Seed3` aigu owner est `(96,108,100)`, `(108,96,100)`, `(92,96,100)` ;
// quarante-neuf vrais `PointId` distincts occupent `(100,100,110)` et
// quarante-huit `(100,100,92)`. Tous partagent la racine axiale, donc le shell
// exact d'un apex vaut `3 + 97 = 100` IDs.
//
// L'ancien buffer avait pour capacite `32+64+3=99` et cessait SILENCIEUSEMENT
// d'ecrire au 99e slot : il rendait une liste fausse sans overflow ni compte
// requis. La capacite prouvee vaut desormais `3+32+2*64=163`, le compte VOULU
// est publie meme tronque, et le profil decide du fate.
int fixture_plateau(int r4, Mutant mut) {
  std::vector<Pt> P = {Pt{{96, 108, 100}}, Pt{{108, 96, 100}}, Pt{{92, 96, 100}}};
  for (int t = 0; t < 49; ++t) P.push_back(Pt{{100, 100, 110}});
  for (int t = 0; t < 48; ++t) P.push_back(Pt{{100, 100, 92}});
  const auto f = mhgp3v::q4axis::seed_axis(P[0].c, P[1].c, P[2].c);
  const bool aigu = mhgp3v::q4axis::seed_aigu(P[0].c, P[1].c, P[2].c);
  std::vector<int> sites;
  for (size_t z = 3; z < P.size(); ++z) sites.push_back((int)z);
  auto pw = [&](int i) { return mhgp3v::q4axis::site_power(f, P[(size_t)i].c); };
  const auto sel = mhgp3v::q4axis::select_axis_topr4(f, sites.data(), (int)sites.size(),
                                                     pw, r4, mut);
  const int seed3[3] = {0, 1, 2};
  int apex = -1;
  if (sel.n_entrants > 0) apex = sel.entrants[0];
  else if (sel.n_sortants > 0) apex = sel.sortants[0];
  int n_shell_plateau = 0, requis_plateau = 0, requis_int_plateau = 0;
  const char* fate_gp = "AUCUN";
  const char* fate_pl = "AUCUN";
  if (apex >= 0) {
    const auto gp = mhgp3v::q4axis::census_replay(sel, apex, seed3, pw, mut,
                                                  Profil::kRelevantGp);
    const auto pl = mhgp3v::q4axis::census_replay(sel, apex, seed3, pw, mut,
                                                  Profil::kPlateau);
    fate_gp = mhgp3v::q4axis::census_fate_name(gp.fate);
    fate_pl = mhgp3v::q4axis::census_fate_name(pl.fate);
    n_shell_plateau = pl.n_shell;
    requis_plateau = pl.requis_shell;
    requis_int_plateau = pl.requis_interieur;
  }
  std::printf("fixture_plateau : aigu=%d verdict=%s entrants=%d sortants=%d"
              " fate_relevant_gp=%s fate_plateau=%s n_shell=%d requis_shell=%d"
              " requis_interieur=%d\n",
              (int)aigu, mhgp3v::q4axis::verdict_name(sel.verdict),
              sel.n_entrants, sel.n_sortants, fate_gp, fate_pl,
              n_shell_plateau, requis_plateau, requis_int_plateau);
  const bool nominal = aigu && (sel.verdict == SeedVerdict::kOuvert) && (apex >= 0) &&
                       (std::string(fate_gp) == "UNSUPPORTED_DEGENERACY") &&
                       (std::string(fate_pl) == "EXACT") &&
                       (n_shell_plateau == 100) && (requis_plateau == 100) &&
                       (requis_int_plateau == 0);
  if (mut != Mutant::kNone) {
    if (!nominal) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::q4axis::mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::q4axis::mutant_name(mut));
    return 1;
  }
  if (!nominal) {
    std::fprintf(stderr, "DESACCORD: le plateau de 97 racines egales n'est ni"
                 " refuse sous RelevantGP ni complet sous plateau\n");
    return 1;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// CAMPAGNES.
// ---------------------------------------------------------------------------
std::vector<Pt> nuage(const std::string& family, long long n, long long coord,
                      long long seed) {
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
  return P;
}

int campagne(const std::string& family, long long n, long long coord, long long seed,
             int r4, long long threads, Mutant mut, long long min_seeds,
             long long min_shallow, Profil profil) {
  if (n < 5 || n > 400) refuse("la campagne exhaustive est bornee a 400 points");
  const std::vector<Pt> P = nuage(family, n, coord, seed);
  const int m = (int)P.size();
  std::vector<Bilan> acc((size_t)threads);
  auto worker = [&](long long tid) {
    Bilan& A = acc[(size_t)tid];
    std::vector<int> scratch;
    for (int ia = (int)tid; ia < m; ia += (int)threads)
      for (int ib = ia + 1; ib < m; ++ib)
        for (int ix = 0; ix < m; ++ix) {
          if (ix == ia || ix == ib) continue;
          const int id[3] = {ia, ib, ix};
          if (!arete_owner(P, id, 3, 0, 1)) continue;
          if (!mhgp3v::q4axis::seed_aigu(P[(size_t)ia].c, P[(size_t)ib].c,
                                         P[(size_t)ix].c)) continue;
          bilan_seed(P, ia, ib, ix, r4, mut, &scratch, &A, profil);
        }
  };
  std::vector<std::thread> th;
  for (long long t = 0; t < threads; ++t) th.emplace_back(worker, t);
  for (auto& t : th) t.join();
  Bilan g;
  for (const Bilan& a : acc) {
    g.morts_degen += a.morts_degen; g.morts_T2 += a.morts_T2;
    g.morts_perm += a.morts_perm; g.morts_gap += a.morts_gap; g.ouverts += a.ouverts;
    g.candidats += a.candidats; g.groupes += a.groupes; g.shallow += a.shallow;
    g.manquants += a.manquants; g.bornes += a.bornes;
    g.identites_fausses += a.identites_fausses; g.gaps_faux += a.gaps_faux;
    g.debordes += a.debordes; g.degeneres += a.degeneres;
    g.fate_exact += a.fate_exact; g.fate_cap += a.fate_cap;
    g.fate_degen += a.fate_degen; g.fate_hors += a.fate_hors;
    g.doublons_bruts += a.doublons_bruts; g.refus_abusifs += a.refus_abusifs;
    if (a.ids_max > g.ids_max) g.ids_max = a.ids_max;
    if (a.cand_max > g.cand_max) g.cand_max = a.cand_max;
  }
  const long long seeds = g.morts_T2 + g.morts_perm + g.morts_gap + g.ouverts;
  std::printf("q4seed_axis_topr4 : famille=%s n=%d r4=%d seeds=%lld morts_T2=%lld"
              " morts_perm=%lld morts_gap=%lld ouverts=%lld candidats=%lld groupes=%lld"
              " cand_max=%lld shallow=%lld manquants=%lld bornes=%lld"
              " identites_fausses=%lld gaps_faux=%lld degeneres=%lld debordes=%lld"
              " fate_exact=%lld fate_cap=%lld fate_degen=%lld fate_hors=%lld"
              " doublons_bruts=%lld refus_abusifs=%lld ids_max=%lld\n",
              family.c_str(), m, r4, seeds, g.morts_T2, g.morts_perm, g.morts_gap,
              g.ouverts, g.candidats, g.groupes, g.cand_max, g.shallow, g.manquants,
              g.bornes, g.identites_fausses, g.gaps_faux, g.degeneres, g.debordes,
              g.fate_exact, g.fate_cap, g.fate_degen, g.fate_hors,
              g.doublons_bruts, g.refus_abusifs, g.ids_max);
  const long long fautes = g.manquants + g.bornes + g.identites_fausses +
                           g.gaps_faux + g.doublons_bruts + g.refus_abusifs;
  if (mut != Mutant::kNone) {
    if (fautes > 0 || g.debordes > 0) {
      std::printf("mutant_killed=1 %s\n", mhgp3v::q4axis::mutant_name(mut));
      return 4;
    }
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mhgp3v::q4axis::mutant_name(mut));
    return 1;
  }
  if (seeds < min_seeds) {
    std::fprintf(stderr, "PLANCHER: %lld Q4Seed3 aigus owner < %lld\n", seeds, min_seeds);
    return 3;
  }
  if (g.shallow < min_shallow) {
    std::fprintf(stderr, "PLANCHER: %lld apex shallow < %lld\n", g.shallow, min_shallow);
    return 3;
  }
  if (fautes > 0) {
    std::fprintf(stderr, "DESACCORD: manquants=%lld bornes=%lld identites=%lld"
                 " gaps=%lld doublons=%lld refus_abusifs=%lld\n",
                 g.manquants, g.bornes, g.identites_fausses, g.gaps_faux,
                 g.doublons_bruts, g.refus_abusifs);
    return 1;
  }
  if (g.debordes > 0) {
    std::fprintf(stderr, "PLANCHER: %lld debordements de capacite\n", g.debordes);
    return 3;
  }
  // LA BORNE PORTE SUR LES GROUPES, PAS SUR LES SITES : un plateau valide peut
  // depasser `2 r4` IDs sans contredire le theoreme. C'est `bornes` — calcule
  // sur les groupes — qui decide, jamais `cand_max`.
  return 0;
}

// L'EXACT-ONCE GLOBAL. `Lane4` construit ses deux `Q4Seed3` possibles, applique
// la provenance primaire — le plus petit vrai `PointId` aigu — et le
// multiensemble produit doit egaler celui du brute force `C(n,4)`.
int exact_once(const std::string& family, long long n, long long coord, long long seed,
               int r4, long long min_q4, long long min_q3_morts) {
  if (n < 5 || n > 200) refuse("l'exact-once exhaustif est borne a 200 points");
  const std::vector<Pt> P = nuage(family, n, coord, seed);
  const int m = (int)P.size();
  const int r3 = r4 + 1;   // `smax-2` : le seuil de mort de la lane q3

  // --- 1. Lane4 : prefixes owner aigus, selection, provenance primaire.
  std::map<std::vector<int>, int> produits;
  long long emissions = 0, q3_morts_utiles = 0;
  std::vector<int> scratch;
  for (int ia = 0; ia < m; ++ia)
    for (int ib = ia + 1; ib < m; ++ib)
      for (int ix = 0; ix < m; ++ix) {
        if (ix == ia || ix == ib) continue;
        const int id3[3] = {ia, ib, ix};
        if (!arete_owner(P, id3, 3, 0, 1)) continue;
        if (!mhgp3v::q4axis::seed_aigu(P[(size_t)ia].c, P[(size_t)ib].c, P[(size_t)ix].c))
          continue;
        const auto f = mhgp3v::q4axis::seed_axis(P[(size_t)ia].c, P[(size_t)ib].c,
                                                 P[(size_t)ix].c);
        scratch.clear();
        for (int z = 0; z < m; ++z)
          if (z != ia && z != ib && z != ix) scratch.push_back(z);
        auto pw = [&](int i) { return mhgp3v::q4axis::site_power(f, P[(size_t)i].c); };
        const auto sel = mhgp3v::q4axis::select_axis_topr4(f, scratch.data(),
                                                           (int)scratch.size(), pw, r4);
        if (sel.verdict == SeedVerdict::kMortDegeneree ||
            sel.verdict == SeedVerdict::kMortT2 ||
            sel.verdict == SeedVerdict::kMortPermanents ||
            sel.verdict == SeedVerdict::kDebordement) continue;
        for (int pass = 0; pass < 2; ++pass) {
          const int cnt = pass ? sel.n_sortants : sel.n_entrants;
          const int* tab = pass ? sel.sortants : sel.entrants;
          for (int t = 0; t < cnt; ++t) {
            const int iy = tab[t];
            const Verdict v = juge_tetra(P, ia, ib, ix, iy);
            if (!v.positif || !v.owner || (long long)v.I_B.size() >= r4) continue;
            // PROVENANCE PRIMAIRE : le plus petit vrai `PointId` parmi les
            // prefixes AIGUS adjacents a l'arete owner. Le sujet la decide, elle
            // n'est pas choisie a la main.
            const bool ix_aigu = true;   // teste plus haut
            const bool iy_aigu = mhgp3v::q4axis::seed_aigu(P[(size_t)ia].c,
                                                           P[(size_t)ib].c,
                                                           P[(size_t)iy].c);
            int primaire = ix;
            if (iy_aigu && iy < ix) primaire = iy;
            (void)ix_aigu;
            if (primaire != ix) continue;
            std::vector<int> cle = {ia, ib, ix, iy};
            std::sort(cle.begin(), cle.end());
            ++produits[cle];
            ++emissions;
            // INDEPENDANCE DES LANES, MESUREE : la miniboule PROPRE du prefixe
            // est-elle deja morte pour q3 ?
            long long dq3 = 0;
            for (int z = 0; z < m; ++z) {
              if (z == ia || z == ib || z == ix) continue;
              const auto p = pw(z);
              if (p.B == 0) { if (p.A < 0) ++dq3; continue; }
            }
            // Profondeur q3 exacte : la miniboule du triangle est le parametre
            // `tau=0`, donc `A_z<0`.
            dq3 = 0;
            for (int z = 0; z < m; ++z) {
              if (z == ia || z == ib || z == ix) continue;
              if (pw(z).A < 0) ++dq3;
            }
            if (dq3 >= r3) ++q3_morts_utiles;
          }
        }
      }

  // --- 2. Brute force `C(n,4)`.
  std::map<std::vector<int>, int> attendus;
  for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j)
      for (int k = j + 1; k < m; ++k)
        for (int l = k + 1; l < m; ++l) {
          const i64* q[4] = {P[(size_t)i].c, P[(size_t)j].c, P[(size_t)k].c, P[(size_t)l].c};
          if (mhgp3v::c8::orient3d(q[0], q[1], q[2], q[3]) == 0) continue;
          if (!mhgp3v::c8::bien_centre(q[0], q[1], q[2], q[3])) continue;
          long long ins = 0;
          for (int z = 0; z < m; ++z) {
            if (z == i || z == j || z == k || z == l) continue;
            if (mhgp3v::c8::interieur_strict(q[0], q[1], q[2], q[3], P[(size_t)z].c)) ++ins;
          }
          if (ins >= r4) continue;
          attendus[{i, j, k, l}] = 1;
        }

  long long manque = 0, doublon = 0, surplus = 0;
  for (const auto& kv : attendus) if (!produits.count(kv.first)) ++manque;
  for (const auto& kv : produits) {
    if (kv.second > 1) ++doublon;
    if (!attendus.count(kv.first)) ++surplus;
  }
  std::printf("exact_once : famille=%s n=%d r4=%d attendus=%zu produits=%zu"
              " emissions=%lld manque=%lld doublon=%lld surplus=%lld"
              " seeds_rang_q3_mort=%lld\n",
              family.c_str(), m, r4, attendus.size(), produits.size(), emissions,
              manque, doublon, surplus, q3_morts_utiles);
  if ((long long)attendus.size() < min_q4) {
    std::fprintf(stderr, "PLANCHER: %zu q4 attendus < %lld\n", attendus.size(), min_q4);
    return 3;
  }
  if (q3_morts_utiles < min_q3_morts) {
    std::fprintf(stderr, "PLANCHER: %lld prefixes q3-morts utiles < %lld —"
                 " l'independance des lanes n'est pas exercee\n",
                 q3_morts_utiles, min_q3_morts);
    return 3;
  }
  if (manque || doublon || surplus) {
    std::fprintf(stderr, "DESACCORD: manque=%lld doublon=%lld surplus=%lld\n",
                 manque, doublon, surplus);
    return 1;
  }
  return 0;
}

Mutant mutant_de(const std::string& s) {
  if (s == "a8-cap-moins-un") return Mutant::kCapMoinsUn;
  if (s == "a8-k-fixe-r4-moins-un") return Mutant::kKFixeR4MoinsUn;
  if (s == "a8-signe-b-inverse") return Mutant::kSigneBInverse;
  if (s == "a8-bouts-ouverts") return Mutant::kBoutsOuverts;
  if (s == "a8-permanence-large") return Mutant::kPermanenceLarge;
  if (s == "a8-oublie-b-zero") return Mutant::kOublieBZero;
  if (s == "a8-abs-avant-tri") return Mutant::kAbsAvantTri;
  if (s == "a8-shell-compte-interieur") return Mutant::kShellCompteInterieur;
  if (s == "a8-gap-large") return Mutant::kGapLarge;
  refuse("mutant inconnu : " + s);
}

}  // namespace

int main(int argc, char** argv) {
  std::string family, mode;
  long long n = 60, coord = 0, seed = 1, smax = 11, threads = 0;
  long long min_seeds = 1, min_shallow = 1, min_q4 = 1, min_q3_morts = 0;
  Mutant mut = Mutant::kNone;
  Profil profil = Profil::kRelevantGp;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a == "--fixture-16" || a == "--fixture-mort-16" || a == "--fixture-jung-tendu" ||
        a == "--fixture-t2" || a == "--fixture-plateau" || a == "--sweep" ||
        a == "--exact-once") mode = a;
    else if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = atoll(val("--points=").c_str());
    else if (a.rfind("--coord=", 0) == 0) coord = atoll(val("--coord=").c_str());
    else if (a.rfind("--seed=", 0) == 0) seed = atoll(val("--seed=").c_str());
    else if (a.rfind("--smax=", 0) == 0) smax = atoll(val("--smax=").c_str());
    else if (a.rfind("--threads=", 0) == 0) threads = atoll(val("--threads=").c_str());
    else if (a.rfind("--min-seeds=", 0) == 0) min_seeds = atoll(val("--min-seeds=").c_str());
    else if (a.rfind("--min-shallow=", 0) == 0) min_shallow = atoll(val("--min-shallow=").c_str());
    else if (a.rfind("--min-q4=", 0) == 0) min_q4 = atoll(val("--min-q4=").c_str());
    else if (a.rfind("--min-q3-morts=", 0) == 0) min_q3_morts = atoll(val("--min-q3-morts=").c_str());
    else if (a.rfind("--profil=", 0) == 0) {
      const std::string v = val("--profil=");
      if (v == "relevant_gp") profil = Profil::kRelevantGp;
      else if (v == "plateau") profil = Profil::kPlateau;
      else refuse("profil inconnu : " + v);
    }
    else if (a.rfind("--inject=", 0) == 0) mut = mutant_de(val("--inject="));
    else refuse("option inconnue : " + a);
  }
  if (mode.empty())
    refuse("--fixture-16, --fixture-mort-16, --fixture-jung-tendu, --fixture-t2,"
           " --fixture-plateau, --sweep ou --exact-once est exige");
  // `r4 = smax - 3` est le PREMIER nombre d'interieurs qui rejette un q4.
  if (smax < 4 || smax > 18) refuse("--smax hors domaine");
  const int r4 = (int)smax - 3;
  if (threads <= 0) threads = (long long)std::thread::hardware_concurrency();
  if (threads <= 0) threads = 1;
  if (mode == "--fixture-16") return fixture_16(r4, mut);
  if (mode == "--fixture-mort-16") return fixture_mort_16(r4, mut);
  if (mode == "--fixture-jung-tendu") return fixture_jung_tendu(r4, mut);
  if (mode == "--fixture-t2") return fixture_t2(r4);
  if (mode == "--fixture-plateau") return fixture_plateau(r4, mut);
  if (family.empty()) refuse("--sweep et --exact-once exigent --family");
  if (mode == "--exact-once")
    return exact_once(family, n, coord, seed, r4, min_q4, min_q3_morts);
  return campagne(family, n, coord, seed, r4, threads, mut, min_seeds, min_shallow,
                  profil);
}
