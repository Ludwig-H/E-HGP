// Feuille J3 : census par vote du warp, support canonique et emission. Port explicite de census() et canonical() de
// microbancs/mes_m2_feuille/include/mhgp12/leaf/leaf_common.hpp (MES-M2), sur le warp de N voies.
//
// Census (lemme R puis vote) : chaque voie s < m classe son site (generateur : contact certifie par la fabrique ;
// dominateur d'un generateur : interieur ; domine : exterieur ; sinon cote exact). Premier evenement de l'ordre
// sequentiel (contrat Q1 de l'auditeur v11) : rejet au (theta+1)-ieme interieur, theta = K+1-q ; census_tests compte
// jusqu'au site d'arret inclus, ou m. Sur l'hote, les voies sont jouees dans l'ordre et s'arretent effectivement a ce
// site (CST-0234, preuve de l'auditeur : les sites suivants ne changent ni le rejet ni aucune emission). Un site a la fois dominateur et domine des generateurs contredirait la propriete
// du centre : invariant (catalogue_invariant de leaf.cpp).
//
// Support canonique de la v12 (CONTRAT_NUMERIQUE.md, paragraphe 4 ; CST-0113) : support de cardinal minimal, puis plus
// petite liste triee des POSITIONS de ses sites dans l'ordre lexicographique des coordonnees, et non plus l'ordre des
// rangs (SiteIdx, rangs de Morton) de la v11. Les sites de la coquille sont ranges par coordonnees (locales : meme
// ordre que les absolues) ; les supports y sont enumeres en ordre lexicographique des indices, paires, puis triangles
// strictement aigus coplanaires au centre, puis tetraedres contenant strictement le centre : le premier succes est le
// minimum (l'application rang -> position est croissante). Chemin froid, hors ligne : coquilles etendues rares.
// L'emission n'a lieu que depuis la presentation egale a S* (comparee comme ensemble de rangs locaux) : toute
// presentation minimale d'une boule admise est visitee dans sa feuille proprietaire (prefixes preserves par G3, J2 et
// le graphe de paires), quelle que soit la convention de S*.
#pragma once

#include "catalogue/leaf_common.hpp"

namespace mhgp12::catalogue_detail {

// Plafond declare de la coquille d'une boule emise (WIT-SPHERE50 : 84 sites cospheriques, refus explicite) : 64 sites,
// la largeur des masques de traces de la tour (tranche T2).
inline constexpr u32 kMaxShell = 64;

MHGP12_HD bool lex_less(const u32* a, const u32* b) {
  if (a[0] != b[0]) return a[0] < b[0];
  if (a[1] != b[1]) return a[1] < b[1];
  return a[2] < b[2];
}

// Rangs locaux d'un support trouve, croissants, kNoLocal au-dela de qmin.
MHGP12_HD void store_support(LocalRank (&support)[4], u32 q, const u32* ranks) {
  u32 sorted[4] = {ranks[0], ranks[1], q > 2 ? ranks[2] : 0u, q > 3 ? ranks[3] : 0u};
  for (u32 i = 1; i < q; ++i)
    for (u32 j = i; j > 0 && sorted[j] < sorted[j - 1]; --j) {
      const u32 t = sorted[j];
      sorted[j] = sorted[j - 1];
      sorted[j - 1] = t;
    }
  for (u32 k = 0; k < 4; ++k) support[k] = k < q ? static_cast<LocalRank>(sorted[k]) : kNoLocal;
}

// Paires de milieu, puis triangles, puis tetraedres, sur la coquille rangee par positions. Faux et statut invariant si
// aucun support (impossible : le centre est dans l'enveloppe de la coquille) ou sur une faute de la politique exacte.
template <u32 N, class A>
MHGP12_HD_COLD bool canonical(LeafCtx<N, A>& X, const Center<A>& s, MaskOf<N> shell, LocalRank (&support)[4],
                              u32& qmin) {
  const auto& P = X.S.P;
  u8 order[N];
  u32 count = 0;
  for (auto rest = shell; simt::any(rest); rest = simt::clear_lowest(rest)) {
    const u32 r = simt::ctz(rest);
    u32 at = count++;
    while (at > 0 && lex_less(P[r], P[order[at - 1]])) {
      order[at] = order[at - 1];
      --at;
    }
    order[at] = static_cast<u8>(r);
  }
  for (u32 i = 0; i < count; ++i)
    for (u32 j = i + 1; j < count; ++j) {
      const Verdict mid = midpoint<A>(s, P[order[i]], P[order[j]]);
      if (mid == kFault) {
        X.status = kLeafInvariant;
        return false;
      }
      if (mid == kYes) {
        const u32 r[2] = {order[i], order[j]};
        store_support(support, 2, r);
        qmin = 2;
        return true;
      }
    }
  for (u32 i = 0; i < count; ++i)
    for (u32 j = i + 1; j < count; ++j)
      for (u32 k = j + 1; k < count; ++k) {
        if (!strictly_acute<A>(P[order[i]], P[order[j]], P[order[k]])) continue;
        int plane = 0;
        if (!center_orientation<A>(P[order[i]], P[order[j]], P[order[k]], s, plane)) {
          X.status = kLeafInvariant;
          return false;
        }
        if (plane == 0) {
          const u32 r[3] = {order[i], order[j], order[k]};
          store_support(support, 3, r);
          qmin = 3;
          return true;
        }
      }
  for (u32 i = 0; i < count; ++i)
    for (u32 j = i + 1; j < count; ++j)
      for (u32 k = j + 1; k < count; ++k)
        for (u32 l = k + 1; l < count; ++l) {
          const u32* p[4] = {P[order[i]], P[order[j]], P[order[k]], P[order[l]]};
          bool inside = false;
          if (!center_inside<A>(s, p, inside)) {
            X.status = kLeafInvariant;
            return false;
          }
          if (inside) {
            const u32 r[4] = {order[i], order[j], order[k], order[l]};
            store_support(support, 4, r);
            qmin = 4;
            return true;
          }
        }
  X.status = kLeafInvariant;  // leaf.cpp : catalogue_invariant
  return false;
}

// Census d'une presentation (generateurs gen[0..q) en rangs locaux croissants, centre c), puis support canonique,
// admission et emission. Appel uniforme ; chaque voie classe le site de son rang.
template <u32 N, class A, class Sink>
MHGP12_HD void census(LeafCtx<N, A>& X, Sink& sink, u32 q, const LocalRank (&gen)[4], const Center<A>& c) {
  using W = simt::Width<N>;
  const auto& L = X.S;
  const u32 m = X.m;
  ++X.judged;
  auto gm = W::empty(), inside = W::empty(), outside = W::empty();
  for (u32 t = 0; t < q; ++t) {
    gm = gm | W::bit(gen[t]);
    inside = inside | L.dom[gen[t]];
    outside = outside | L.domby[gen[t]];
  }
  if (simt::any(inside & outside)) {  // leaf.cpp : catalogue_invariant (centre hors de la boite)
    X.status = kLeafInvariant;
    return;
  }
  simt::Lanes<bool, N> is_in, is_on, fault;
  const u32 theta = static_cast<u32>(X.K + 1) - q;
  u32 inside_seen = 0;  // hote : arret effectif au (theta+1)-ieme interieur (CST-0234) ; appareil : toutes les voies
  MHGP12_LANES(N, s) {
    is_in[s] = is_on[s] = fault[s] = false;
    if (s < m && !simt::serial_stop(inside_seen, theta)) {
      if (simt::test(gm, s)) {
        is_on[s] = true;  // contact certifie par la fabrique
      } else if (simt::test(inside, s)) {
        is_in[s] = true;  // lemme R
      } else if (!simt::test(outside, s)) {
        int r = 0;
        bool healthy_side = true;
        if (q == 2) r = side_q2<A>(L.P[gen[0]], L.P[gen[1]], L.P[s]);
        else healthy_side = side<A>(c, L.P[s], r);
        if (!healthy_side) fault[s] = true;
        else if (r < 0) is_in[s] = true;
        else if (r == 0) is_on[s] = true;
      }
      inside_seen += is_in[s] ? 1u : 0u;
    }
  }
  const auto I = simt::ballot<N>(is_in), C = simt::ballot<N>(is_on);
  if (simt::any(simt::ballot<N>(fault))) {
    X.status = kLeafInvariant;
    return;
  }
  const u32 t = simt::popc(I) > theta ? simt::select_nth<N>(I, theta + 1) : m;
  if (t < m) {
    X.census_tests += t + 1;
    return;  // rejet au (theta+1)-ieme interieur
  }
  X.census_tests += m;
  const u32 p = simt::popc(I), count = simt::popc(C);
  LocalRank support[4] = {gen[0], gen[1], q > 2 ? gen[2] : kNoLocal, q > 3 ? gen[3] : kNoLocal};
  u32 qmin = q;
  if (count != q) {
    if (!canonical<N, A>(X, c, C, support, qmin)) return;
    for (u32 k = 0; k < 4; ++k)
      if (support[k] != (k < q ? gen[k] : kNoLocal)) return;  // S* sera visite dans cette meme feuille
  }
  if (p + qmin > static_cast<u32>(X.K) + 1) return;
  if (count > kMaxShell) {
    X.status = kLeafShellCapacity;
    return;
  }
  if (q == 4) ++X.q4_levels;
  ++X.emitted;
  X.incidences += p + count;
  Emission<N> e;
  for (u32 k = 0; k < 4; ++k) e.support[k] = support[k];
  e.p = static_cast<u8>(p);
  e.qmin = static_cast<u8>(qmin);
  e.q = static_cast<u8>(q);
  e.pad = 0;
  e.m = count;
  e.interior = I;
  e.shell = C;
  sink.emit(e);
}

}  // namespace mhgp12::catalogue_detail
