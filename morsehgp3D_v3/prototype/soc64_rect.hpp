// MorseHGP3D v3 — SOC64 / CORNER512 : LE CERTIFICAT `ALL` CORRELE D'UN RECTANGLE.
//
// Specification : PROPOSITION.md section 6.2 ; ordre experimental impose par
// audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md section 4.1 et
// audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md section 5.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=ablation_counter_only,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CE FICHIER N'EST PAS
//
// Il ne remplace pas `spindle_cone.hpp`. Celui-la fixe UN temoin `z` et UNE
// ancre `a`, puis certifie une boite de cibles `b` par ses huit coins. Ici les
// TROIS objets sont des boites : `A` d'ancres, `B` de cibles, `C` de temoins.
// C'est le seul niveau ou la question posee par le contre-audit se pose : le
// classifieur `JungSpindleRect-v0` combine des EXTREMA SEPARES de `D`, `V`, `T`
// sur ce meme rectangle, et son gain mesure est de trois centiemes de point.
// La question n'est donc pas « le rectangle est-il un mauvais objet » mais
// « la perte vient-elle de la DECORRELATION des extrema, ou d'une absence
// reelle de coeur commun ». Ce fichier repond exactement a la premiere moitie.
//
// ---------------------------------------------------------------------------
// LE PREDICAT, ET SA SYMETRIE
//
// Avec e = z-a, t = b-z, H = e.t, E = |e|^2, X = |t|^2 :
//
//   q2 : H > 0                    q3 : H > 0 et 4 H^2 > E X
//   q4 : H > 0 et 3 H^2 > E X
//
// Ces trois predicats ne dependent QUE du couple (e,t), et ils y sont
// SYMETRIQUES : echanger e et t laisse H, puis E X, inchanges. Cette symetrie
// n'est pas une coquetterie, c'est ce qui rend l'argument ci-dessous valable
// des DEUX cotes sans le reecrire.
//
// A `t` fixe, l'ensemble admissible en `e` est
//   { e : e.t > 0, c (e.t)^2 > |e|^2 |t|^2 },  c = 4 (q3), 3 (q4),
// c'est-a-dire { e : angle(e,t) < theta_c }, un CONE CIRCULAIRE OUVERT donc
// CONVEXE. Par symetrie, a `e` fixe, l'ensemble admissible en `t` l'est aussi.
//
// ---------------------------------------------------------------------------
// SOC64 : L'ARGUMENT DE CONVEXITE EN DEUX TEMPS
//
// Poser les differences de Minkowski
//   Ebox = C (-) A = [C.lo - A.hi, C.hi - A.lo]
//   Tbox = B (-) C = [B.lo - C.hi, B.hi - C.lo]
// Tout triple reel (a,b,z) de A x B x C donne (e,t) dans Ebox x Tbox. La
// reciproque est FAUSSE — e et t partagent le meme `z` — et c'est precisement
// la relaxation qui rend le test fail-open.
//
// Supposons les 64 couples (coin de Ebox, coin de Tbox) admissibles.
//   1. Fixons un coin `t_k` de Tbox. Les huit coins de Ebox sont admissibles
//      pour ce `t_k`, et l'ensemble admissible en `e` est convexe : il contient
//      donc l'enveloppe convexe de ces coins, c'est-a-dire Ebox entier.
//   2. Fixons maintenant `e` QUELCONQUE dans Ebox. Par l'etape 1 applique aux
//      huit coins de Tbox, les huit couples (e, t_k) sont admissibles. Or
//      l'ensemble admissible en `t` est convexe : il contient Tbox entier.
// Donc Ebox x Tbox est entierement admissible, donc a fortiori A x B x C.
//
// L'implication ne se renverse pas : un echec des 64 couples ne dit RIEN du
// rectangle reel. La reponse est `UNKNOWN`, jamais `NONE`. La fixture gravee
// `soc64-faux-echec` du probe le materialise.
//
// ---------------------------------------------------------------------------
// CORNER512 : L'EXACTITUDE SUR L'ENVELOPPE CONTINUE
//
// Le meme predicat est separement convexe en `a`, en `b` et en `z` :
//   - a `b,z` fixes, e = z-a est affine en `a` et la condition sur `e` est un
//     cone convexe ;
//   - a `a,z` fixes, t = b-z est affine en `b`, meme argument ;
//   - a `a,b` fixes, le spindle en `z` est convexe (lemme du noeud spindle).
// Un argument en TROIS temps, identique au precedent, donne donc : les 512
// triples de coins de A x B x C admissibles <=> A x B x C admissible EN TANT
// QU'ENVELOPPE CONTINUE. C'est une equivalence, pas une suffisance.
//
// Un coin FICTIF — un sommet de boite ou aucun `PointId` ne vit — qui echoue ne
// prouve donc rien sur les points reellement stockes : l'echec reste
// `AABB_envelope_not_all`, jamais `NONE`. Ce fichier n'ecrit aucun `NONE`.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Coordonnees dans [0,65535], donc chaque composante de `e` et de `t` vit dans
// [-65535, 65535] et chaque composante des differences de Minkowski aussi.
//   |H|   <= 3 * 65535^2         = 1,288e10   -> 34 bits   (i64 exact)
//   E, X  <= 3 * 65535^2         = 1,288e10   -> 34 bits   (i64 exact)
//   E * X <= 9 * 65535^4         = 1,659e20   -> 68 bits   (i128 obligatoire)
//   4 H^2 <= 36 * 65535^4        = 6,636e20   -> 70 bits   (i128 obligatoire)
// Aucune quantite ne depasse 71 bits : `i128` suffit a TOUT ce fichier, et la
// preuve tient en une ligne parce que rien n'est jamais divise. Le mutant
// `kNarrowI64` reste en i64 : `E*X` y deborde des la fixture large, il doit
// mourir.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "spindle_cone.hpp"

namespace mhgp3v {
namespace soc {

using mhgp::i128;
using mhgp::i64;

using cone::Box;
using cone::box_corner;
using cone::kLaneNone;
using cone::kLaneQ2;
using cone::kLaneQ3;
using cone::kLaneQ4;

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse UNE decision exacte du certificat correle. Ils vivent
// dans le predicat, pas dans le probe : une faute qui n'est pas dans l'objet
// juge ne prouve rien sur lui.
// ---------------------------------------------------------------------------
enum class SocMutant {
  kNone,
  kNoPositivity,   // oublie H > 0 : le cone oppose (b du cote de a) est accepte
  kAcceptEquality, // >= au lieu de > : la frontiere fermee entre dans le cone
  kSwapCoeff,      // confond les coefficients 3 (q4) et 4 (q3)
  kNarrowI64,      // ne promeut pas avant multiplication : E*X deborde
  kDiagonalOnly,   // 8 couples (coin k, coin k) au lieu des 64 : la relaxation
                   // n'est plus un produit, l'argument de convexite tombe
  kSevenTargets,   // omet le huitieme coin de Tbox
  kMinkowskiSame,  // forme C (-) A avec [lo-lo, hi-hi] : ce n'est plus un
                   // sur-ensemble des differences reelles
};

inline const char* soc_mutant_name(SocMutant m) {
  switch (m) {
    case SocMutant::kNone: return "none";
    case SocMutant::kNoPositivity: return "soc-no-positivity";
    case SocMutant::kAcceptEquality: return "soc-accept-equality";
    case SocMutant::kSwapCoeff: return "soc-swap-coeff";
    case SocMutant::kNarrowI64: return "soc-narrow-i64";
    case SocMutant::kDiagonalOnly: return "soc-diagonal-only";
    case SocMutant::kSevenTargets: return "soc-seven-targets";
    case SocMutant::kMinkowskiSame: return "soc-minkowski-same";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// COMPTEURS. Le jalon est COUNTER-ONLY : il ne change aucun fate, il mesure.
// `pairs` compte les couples (e,t) reellement evalues, `early` les sorties
// anticipees, `wide` les multiplications 128 bits.
// ---------------------------------------------------------------------------
struct SocStats {
  long long calls = 0;
  long long pairs = 0;
  long long early = 0;
  long long wide = 0;
};

// ---------------------------------------------------------------------------
// LA VALEUR RENDUE SOUS UN SEUIL N'EST PAS UNE LANE.
//
// Le contre-audit `AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`
// section 2.4 a raison : avec `floor > q2`, une sortie anticipee prouve
// seulement « le seuil est impossible », jamais « la lane exacte vaut ceci ».
// Sa contre-fixture u16 le montre en quatre points :
//
//   A={(0,0,0)}, C={(1,1,1)}, B=[0,4] x {1} x {1}, floor=q4
//
// Le couple central `e=(1,1,1)`, `t=(1,0,0)` rend q3 et fait sortir tout de
// suite ; mais le coin `t=(-1,0,0)` donne `H=-1`, donc la vraie lane minimale
// est `NONE`. Rendre `q3` serait une affirmation fausse.
//
// La regle exacte est desormais : la valeur rendue est la lane `ALL` du produit
// relaxe SI ET SEULEMENT SI l'enumeration a ete complete, ou si elle vaut
// `kLaneNone` — qui est le minimum du treillis et ne peut donc pas etre
// ameliore. Dans tous les autres cas la fonction rend `kUnknownBelowFloor`.
// Un appelant qui teste `>= kLaneQ4` reste correct sans changement, puisque
// `kUnknownBelowFloor` est strictement negatif.
// `constexpr int` et non un enum : les lanes viennent d'un enum anonyme de
// `cone::`, et melanger deux enums anonymes dans un `?:` est refuse par
// `-Werror=enum-compare`. Le type doit rester `int`, celui des lanes.
constexpr int kUnknownBelowFloor = -1;

// ---------------------------------------------------------------------------
// DIFFERENCE DE MINKOWSKI EXACTE : X (-) Y = { x - y : x dans X, y dans Y }.
// C'est la boite [X.lo - Y.hi, X.hi - Y.lo]. Le mutant `kMinkowskiSame` ecrit
// [X.lo - Y.lo, X.hi - Y.hi], qui n'est plus un sur-ensemble des differences.
// ---------------------------------------------------------------------------
MHGP_HD inline Box box_minkowski_diff(const Box& x, const Box& y,
                                      SocMutant mu = SocMutant::kNone) {
  Box out{};
  for (int i = 0; i < 3; ++i) {
    if (mu == SocMutant::kMinkowskiSame) {
      out.lo[i] = x.lo[i] - y.lo[i];
      out.hi[i] = x.hi[i] - y.hi[i];
      if (out.lo[i] > out.hi[i]) { const i64 s = out.lo[i]; out.lo[i] = out.hi[i]; out.hi[i] = s; }
    } else {
      out.lo[i] = x.lo[i] - y.hi[i];
      out.hi[i] = x.hi[i] - y.lo[i];
    }
  }
  return out;
}

// ---------------------------------------------------------------------------
// PREDICAT PONCTUEL SUR LE COUPLE (e,t). Rend la lane la PLUS FORTE satisfaite.
// L'emboitement C4 < C3 < C2 rend ce seul entier suffisant.
//
// Cette fonction est l'autorite arithmetique du fichier : tout le reste ne fait
// que choisir les couples a lui soumettre.
// ---------------------------------------------------------------------------
MHGP_HD inline int lane_of_et(const i64 e[3], const i64 t[3],
                              SocMutant mu = SocMutant::kNone,
                              SocStats* st = nullptr) {
  if (st != nullptr) ++st->pairs;
  const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
  if (mu != SocMutant::kNoPositivity) {
    const bool positive = (mu == SocMutant::kAcceptEquality) ? (h >= 0) : (h > 0);
    if (!positive) return kLaneNone;
  }
  const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
  const i64 x2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];

  int c3 = 4, c4 = 3;
  if (mu == SocMutant::kSwapCoeff) { c3 = 3; c4 = 4; }

  if (mu == SocMutant::kNarrowI64) {
    // MUTANT : `e2 * x2` atteint 1,66e20 sous u16 et deborde i64 des la
    // fixture large. La decision devient un signe enroule.
    //
    // L'ENROULEMENT EST EXPLICITE, ET C'EST UNE REPARATION. Un mutant qui
    // provoque un debordement SIGNE est un comportement indefini : sous UBSan
    // il meurt par diagnostic et non par contradiction, et le contre-audit de
    // `ball_event.hpp:290` a montre que ce vert-la n'est pas causal. Le calcul
    // passe donc par `unsigned long long`, dont l'arithmetique modulaire est
    // definie, puis reconvertit — conversion elle-meme definie depuis C++20,
    // qui impose le complement a deux. La faute reste EXACTEMENT la meme
    // — un produit tronque a 64 bits — mais elle est deterministe et
    // observable sous tout sanitizer.
    typedef unsigned long long u64;
    const i64 hh = (i64)((u64)h * (u64)h);
    const i64 ex = (i64)((u64)e2 * (u64)x2);
    if ((i64)((u64)c4 * (u64)hh) > ex) return kLaneQ4;
    if ((i64)((u64)c3 * (u64)hh) > ex) return kLaneQ3;
    return kLaneQ2;
  }

  const i128 hh = (i128)h * (i128)h;
  const i128 ex = (i128)e2 * (i128)x2;
  if (st != nullptr) st->wide += 2;
  if (mu == SocMutant::kAcceptEquality) {
    if ((i128)c4 * hh >= ex) return kLaneQ4;
    if ((i128)c3 * hh >= ex) return kLaneQ3;
    return kLaneQ2;
  }
  if ((i128)c4 * hh > ex) return kLaneQ4;
  if ((i128)c3 * hh > ex) return kLaneQ3;
  return kLaneQ2;
}

// ---------------------------------------------------------------------------
// SOC64. Rend la lane la plus forte q telle que le PRODUIT RELAXE
// Ebox x Tbox est entierement dans C_q ; c'est une CONDITION SUFFISANTE pour
// que le rectangle reel A x B x C le soit.
//
// `floor` est une sortie anticipee, pas un relachement : des que le minimum
// courant tombe sous `floor`, la reponse exacte ne peut plus l'atteindre. Toute
// reponse >= floor reste donc la lane relaxee exacte. L'appelant qui ne
// s'interesse qu'a q4 passe `floor = kLaneQ4` et paie en general un seul
// couple.
//
// Le premier couple evalue est (centre de Ebox, centre de Tbox) : le produit
// relaxe est inclus dans le cone seulement si ce couple y est, donc sa lane
// MAJORE la reponse. Un seul predicat elimine ainsi la grande majorite des
// taches.
// ---------------------------------------------------------------------------
MHGP_HD inline int soc64_all_lane(const Box& a, const Box& b, const Box& c,
                                  SocMutant mu = SocMutant::kNone,
                                  int floor = kLaneQ2, SocStats* st = nullptr) {
  if (st != nullptr) ++st->calls;
  const Box ebox = box_minkowski_diff(c, a, mu);
  const Box tbox = box_minkowski_diff(b, c, mu);

  i64 ec[3], tc[3];
  for (int i = 0; i < 3; ++i) {
    ec[i] = (ebox.lo[i] + ebox.hi[i]) / 2;
    tc[i] = (tbox.lo[i] + tbox.hi[i]) / 2;
  }
  const int centre = lane_of_et(ec, tc, mu, st);
  if (centre < floor) {
    if (st != nullptr) ++st->early;
    // `kLaneNone` est le minimum du treillis : l'annoncer est exact. Toute
    // autre valeur sous le seuil n'a pas ete confrontee aux coins.
    return (centre == kLaneNone) ? kLaneNone : kUnknownBelowFloor;
  }

  const int nt = (mu == SocMutant::kSevenTargets) ? 7 : 8;
  int best = kLaneQ4;
  if (mu == SocMutant::kDiagonalOnly) {
    // MUTANT : seuls les huit couples DIAGONAUX (coin k, coin k). Le domaine
    // teste n'est plus un produit de boites, et l'argument de convexite en deux
    // temps ne s'applique plus : le mutant certifie des rectangles ouverts.
    for (int k = 0; k < 8; ++k) {
      i64 e[3], t[3];
      box_corner(ebox, k, e);
      box_corner(tbox, k, t);
      const int lane = lane_of_et(e, t, mu, st);
      if (lane < best) best = lane;
      if (best < floor) {
        if (st != nullptr) ++st->early;
        return (best == kLaneNone) ? kLaneNone : kUnknownBelowFloor;
      }
    }
    return best;
  }
  for (int ke = 0; ke < 8; ++ke) {
    i64 e[3];
    box_corner(ebox, ke, e);
    for (int kt = 0; kt < nt; ++kt) {
      i64 t[3];
      box_corner(tbox, kt, t);
      const int lane = lane_of_et(e, t, mu, st);
      if (lane < best) best = lane;
      if (best < floor) {
        if (st != nullptr) ++st->early;
        return (best == kLaneNone) ? kLaneNone : kUnknownBelowFloor;
      }
    }
  }
  return best;
}

// ---------------------------------------------------------------------------
// CORNER512. Rend la lane la plus forte q telle que l'ENVELOPPE CONTINUE
// A x B x C est entierement dans C_q. C'est une equivalence sur l'enveloppe,
// donc un certificat strictement plus fort que SOC64 — au prix de 512 couples
// au lieu de 64. Il ne devient jamais `NONE` sur les points stockes.
// ---------------------------------------------------------------------------
MHGP_HD inline int corner512_all_lane(const Box& a, const Box& b, const Box& c,
                                      SocMutant mu = SocMutant::kNone,
                                      int floor = kLaneQ2, SocStats* st = nullptr) {
  if (st != nullptr) ++st->calls;
  i64 ac[3], bc[3], cc[3];
  for (int i = 0; i < 3; ++i) {
    ac[i] = (a.lo[i] + a.hi[i]) / 2;
    bc[i] = (b.lo[i] + b.hi[i]) / 2;
    cc[i] = (c.lo[i] + c.hi[i]) / 2;
  }
  {
    i64 e[3], t[3];
    for (int i = 0; i < 3; ++i) { e[i] = cc[i] - ac[i]; t[i] = bc[i] - cc[i]; }
    const int centre = lane_of_et(e, t, mu, st);
    if (centre < floor) {
      if (st != nullptr) ++st->early;
      return (centre == kLaneNone) ? kLaneNone : kUnknownBelowFloor;
    }
  }
  int best = kLaneQ4;
  for (int ka = 0; ka < 8; ++ka) {
    i64 pa[3];
    box_corner(a, ka, pa);
    for (int kb = 0; kb < 8; ++kb) {
      i64 pb[3];
      box_corner(b, kb, pb);
      for (int kc = 0; kc < 8; ++kc) {
        i64 pc[3];
        box_corner(c, kc, pc);
        i64 e[3], t[3];
        for (int i = 0; i < 3; ++i) { e[i] = pc[i] - pa[i]; t[i] = pb[i] - pc[i]; }
        const int lane = lane_of_et(e, t, mu, st);
        if (lane < best) best = lane;
        if (best < floor) {
        if (st != nullptr) ++st->early;
        return (best == kLaneNone) ? kLaneNone : kUnknownBelowFloor;
      }
      }
    }
  }
  return best;
}

// ---------------------------------------------------------------------------
// REFERENCE EXHAUSTIVE, POUR LES SEULES FIXTURES GRAVEES. Elle enumere tous les
// triples ENTIERS du rectangle et rend la lane ALL exacte sur les points de la
// grille. Son cout est le produit des volumes : le probe refuse au-dela d'un
// cap explicite. Elle n'est jamais sur un chemin mesure.
// ---------------------------------------------------------------------------
inline long long rect_lattice_volume(const Box& x) {
  long long v = 1;
  for (int i = 0; i < 3; ++i) {
    const long long w = x.hi[i] - x.lo[i] + 1;
    if (w <= 0) return 0;
    v *= w;
  }
  return v;
}

// `cap` borne le nombre de triples enumeres ; -1 est rendu si le cap est
// depasse, ce qui est un REFUS et non un resultat.
inline int exhaustive_all_lane(const Box& a, const Box& b, const Box& c, long long cap) {
  const long long va = rect_lattice_volume(a);
  const long long vb = rect_lattice_volume(b);
  const long long vc = rect_lattice_volume(c);
  if (va <= 0 || vb <= 0 || vc <= 0 || cap <= 0) return -1;
  // Preflight EXACT du produit, sans jamais former le produit lui-meme.
  if (va > cap / vb) return -1;
  const long long vab = va * vb;
  if (vab > cap / vc) return -1;
  int best = kLaneQ4;
  for (i64 ax = a.lo[0]; ax <= a.hi[0]; ++ax)
   for (i64 ay = a.lo[1]; ay <= a.hi[1]; ++ay)
    for (i64 az = a.lo[2]; az <= a.hi[2]; ++az)
     for (i64 bx = b.lo[0]; bx <= b.hi[0]; ++bx)
      for (i64 by = b.lo[1]; by <= b.hi[1]; ++by)
       for (i64 bz = b.lo[2]; bz <= b.hi[2]; ++bz)
        for (i64 cx = c.lo[0]; cx <= c.hi[0]; ++cx)
         for (i64 cy = c.lo[1]; cy <= c.hi[1]; ++cy)
          for (i64 cz = c.lo[2]; cz <= c.hi[2]; ++cz) {
            const i64 e[3] = {cx - ax, cy - ay, cz - az};
            const i64 t[3] = {bx - cx, by - cy, bz - cz};
            const int lane = lane_of_et(e, t, SocMutant::kNone, nullptr);
            if (lane < best) best = lane;
            if (best == kLaneNone) return best;
          }
  return best;
}

// ---------------------------------------------------------------------------
// LE CLASSIFIEUR SCALAIRE DE REFERENCE — CELUI QUE LA MESURE DOIT DEPASSER.
//
// Il reproduit la logique `JungSpindleRect-v0` : extrema SEPARES de H, de E et
// de X sur les memes boites relaxees, puis test unique. Il est SUR (tout
// rectangle qu'il certifie est certifie), mais il ignore la correlation entre
// H et le produit E X. Il vit ici pour que la comparaison SOC64 / scalaire
// mesure exactement la meme entree, sans dependre du probe.
//
//   Hmin  = min sur Ebox x Tbox de e.t          (exact, axe par axe)
//   Emax  = max sur Ebox de |e|^2               (exact, axe par axe)
//   Xmax  = max sur Tbox de |t|^2               (exact, axe par axe)
//   q4 <= Hmin > 0 et 3 Hmin^2 > Emax Xmax
// ---------------------------------------------------------------------------
MHGP_HD inline int scalar_extrema_all_lane(const Box& a, const Box& b, const Box& c,
                                           SocStats* st = nullptr) {
  if (st != nullptr) ++st->calls;
  const Box ebox = box_minkowski_diff(c, a);
  const Box tbox = box_minkowski_diff(b, c);
  i64 hmin = 0, emax = 0, xmax = 0;
  for (int i = 0; i < 3; ++i) {
    // e_i t_i est bilineaire : son minimum sur un produit d'intervalles est
    // atteint sur un couple de bornes.
    const i64 p1 = ebox.lo[i] * tbox.lo[i];
    const i64 p2 = ebox.lo[i] * tbox.hi[i];
    const i64 p3 = ebox.hi[i] * tbox.lo[i];
    const i64 p4 = ebox.hi[i] * tbox.hi[i];
    i64 m = p1;
    if (p2 < m) m = p2;
    if (p3 < m) m = p3;
    if (p4 < m) m = p4;
    hmin += m;
    const i64 ea = (ebox.lo[i] < 0 ? -ebox.lo[i] : ebox.lo[i]);
    const i64 eb = (ebox.hi[i] < 0 ? -ebox.hi[i] : ebox.hi[i]);
    const i64 em = (ea > eb) ? ea : eb;
    emax += em * em;
    const i64 ta = (tbox.lo[i] < 0 ? -tbox.lo[i] : tbox.lo[i]);
    const i64 tb = (tbox.hi[i] < 0 ? -tbox.hi[i] : tbox.hi[i]);
    const i64 tm = (ta > tb) ? ta : tb;
    xmax += tm * tm;
  }
  if (hmin <= 0) return kLaneNone;
  const i128 hh = (i128)hmin * (i128)hmin;
  const i128 ex = (i128)emax * (i128)xmax;
  if (st != nullptr) st->wide += 2;
  if (3 * hh > ex) return kLaneQ4;
  if (4 * hh > ex) return kLaneQ3;
  return kLaneQ2;
}

}  // namespace soc
}  // namespace mhgp3v
