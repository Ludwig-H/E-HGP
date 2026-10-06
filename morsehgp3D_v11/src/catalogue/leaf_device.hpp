// Feuille du catalogue en source unique hote/appareil (voie GPU, 4 octobre 2026). Port fidele de la voie graphe de
// paires de leaf.cpp (pair_graph actif, m <= 32 sites) : memes prefixes dans le meme ordre, memes decisions G3, J2,
// M3, E4, proprietaire, census par masques (lemme R), support canonique et admission, memes quinze compteurs, memes
// emissions (S*, p, m, qmin, listes I et U en SiteIdx globaux croissants). Le Level n'est pas calcule ici : l'hote le
// tire du support par Sphere::through de meme arite, exactement emission_level.
// Seuls les chemins i128 dont la v11 a prouve la borne sont joues (puissance native q2/q4 et q3 certifiee,
// orientation certifiee, poids q4 dans un cube de cote 2^20, droites J2, proprietaire). Tout autre cas, et tout
// invariant que leaf.cpp refuserait, rend kUnresolved : l'hote rejoue alors la feuille entiere par leaf.cpp, qui
// prend la voie checked/Wide ou rend le meme refus ; rien de la feuille non resolue n'est publie (contrat R7 des
// auditeurs : jamais un debordement ni un refus transforme en rejet geometrique).
// Aucun conteneur, exception, allocation ni flottant ; aucune dependance au reste du depot hors <cstdint>.
#pragma once

#include "catalogue/leaf_device_predicates.hpp"

namespace mhgp11::leaf_device {

// Tables d'une feuille, partageables par les fils qui la calculent ensemble (feuille cooperative, 6 octobre 2026) :
// sites, lignes de dominance et de voisinage, lignes vivantes, cache J2 simule. Remplies par fill_tables (ou par
// lignes, fill_row), lues ensuite seulement, sauf le cache J2 (bit pose par seen_test_set).
struct Tables {
  u32 m = 0;
  u32 P[kMaxSites][3];
  u64 dom[kMaxSites], domby[kMaxSites], nbr[kMaxSites];
  u64 live[3][kMaxSites];
  u32 seen[kSeenWords];
};

// Relation du couple (i, j) sur la fermeture de la boite : -1 si j domine i, +1 si i domine j, 0 sinon (voisins).
// Forme affine de la difference des distances ; sommes < 12*2^(2B) en i64.
MHGP11_LEAF_HD int pair_relation(const Tables& t, const Input& in, u32 i, u32 j) {
  i64 base = 0, cmin = 0, cmax = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const i64 delta = i64(t.P[j][axis]) - i64(t.P[i][axis]);
    base += i64(t.P[j][axis]) * t.P[j][axis] - i64(t.P[i][axis]) * t.P[i][axis];
    cmin += (delta > 0 ? in.lo[axis] : in.hi[axis]) * delta;
    cmax += (delta > 0 ? in.hi[axis] : in.lo[axis]) * delta;
  }
  if (base - 2 * cmin < 0) return -1;
  if (base - 2 * cmax > 0) return 1;
  return 0;
}

// Ligne i des tables (dom, domby, nbr), tous les j : chaque fil peut remplir ses lignes seul. Meme relation que le
// parcours des paires i < j de fill_tables (pair_relation(j, i) = -pair_relation(i, j)).
MHGP11_LEAF_HD void fill_row(Tables& t, const Input& in, u32 i) {
  u64 dom = 0, domby = 0, nbr = 0;
  for (u32 j = 0; j < t.m; ++j) {
    if (j == i) continue;
    const int r = j > i ? pair_relation(t, in, i, j) : -pair_relation(t, in, j, i);
    if (r < 0) dom |= u64(1) << j;
    else if (r > 0) domby |= u64(1) << j;
    else nbr |= u64(1) << j;
  }
  t.dom[i] = dom; t.domby[i] = domby; t.nbr[i] = nbr;
}

// Ligne vivante x (apres toutes les lignes dom et nbr).
MHGP11_LEAF_HD void fill_live(Tables& t, const Input& in, u32 x) {
  u64 live0 = 0, live1 = 0, live2 = 0;
  for (u64 rest = t.nbr[x]; rest != 0; rest &= rest - 1) {
    const u32 y = ctz(rest);
    const int weight = static_cast<int>(popc(t.dom[x] | t.dom[y]));
    if (weight <= in.kmax - 1) live0 |= u64(1) << y;
    if (weight <= in.kmax - 2) live1 |= u64(1) << y;
    if (weight <= in.kmax - 3) live2 |= u64(1) << y;
  }
  t.live[0][x] = live0; t.live[1][x] = live1; t.live[2][x] = live2;
}

MHGP11_LEAF_HD void load_sites(Tables& t, const Input& in, u32 i) {
  const u32 s = in.sites[i];
  t.P[i][0] = in.x[s]; t.P[i][1] = in.y[s]; t.P[i][2] = in.z[s];
}

// Tables completes, sur un seul fil : chaque couple i < j est calcule une seule fois (fill_row, qui recalcule chaque
// couple pour sa ligne, est reserve aux fils d'un warp ; audit d117de397).
MHGP11_LEAF_HD void fill_tables(Tables& t, const Input& in) {
  t.m = in.m;
  for (u32 i = 0; i < t.m; ++i) {
    load_sites(t, in, i);
    t.dom[i] = t.domby[i] = t.nbr[i] = 0;
  }
  for (u32 w = 0; w < kSeenWords; ++w) t.seen[w] = 0;
  for (u32 i = 0; i < t.m; ++i)
    for (u32 j = i + 1; j < t.m; ++j) {
      const int r = pair_relation(t, in, i, j);
      if (r < 0) {  // j domine i
        t.dom[i] |= u64(1) << j;
        t.domby[j] |= u64(1) << i;
      } else if (r > 0) {  // i domine j
        t.dom[j] |= u64(1) << i;
        t.domby[i] |= u64(1) << j;
      } else {
        t.nbr[i] |= u64(1) << j;
        t.nbr[j] |= u64(1) << i;
      }
    }
  for (u32 x = 0; x < t.m; ++x) fill_live(t, in, x);
}

// Bit r du cache J2 simule : vrai s'il etait deja pose. Atomique sur l'appareil (fils d'une meme feuille) ; le nombre
// de succes vaut les tests moins les rangs distincts, independant de l'ordre de visite.
MHGP11_LEAF_HD bool seen_test_set(u32* seen, u32 r) {
  const u32 bit = 1u << (r & 31);
#if defined(__CUDA_ARCH__)
  return (atomicOr(&seen[r >> 5], bit) & bit) != 0;
#else
  const bool hit = (seen[r >> 5] & bit) != 0;
  seen[r >> 5] |= bit;
  return hit;
#endif
}

template <class Sink>
struct Leaf {
  const Input& in;
  Counts& c;
  Sink& sink;
  Tables& t;
  u64 masks[5] = {0, 0, 0, 0, 0};
  u32 prefix[4] = {0, 0, 0, 0};
  u32 interior[kMaxSites], shell[kMaxSites], shell_local[kMaxSites];
  bool unresolved = false;

  MHGP11_LEAF_HD Leaf(const Input& input, Counts& counts, Sink& out, Tables& tables)
      : in(input), c(counts), sink(out), t(tables) {}

  MHGP11_LEAF_HD static u32 rank(u32 i, u32 j, u32 k) { return k * (k - 1) * (k - 2) / 6 + j * (j - 1) / 2 + i; }

  // J2, droites des faces contenant le dernier site ; cache simule pour les seuls compteurs.
  MHGP11_LEAF_HD bool lines_possible(int q) {
    const u32 last = prefix[q - 1];
    for (int j = 0; j + 2 < q; ++j)
      for (int k = j + 1; k + 1 < q; ++k) {
        ++c.region_line_tests;
        const u32 a = prefix[j], b = prefix[k];
        bool hit = false;
        if (in.cache) hit = seen_test_set(t.seen, rank(a, b, last));
        if (hit) ++c.region_line_cache_hits; else ++c.region_line_evaluations;
        if (center_line_meets(t.P[a], t.P[b], t.P[last], in.lo, in.hi) != kIntersects) {
          ++c.region_line_rejects;
          return false;
        }
      }
    return true;
  }

  MHGP11_LEAF_HD void q2() {
    const u32* a = t.P[prefix[0]];
    const u32* b = t.P[prefix[1]];
    Center s;
    for (int j = 0; j < 3; ++j) { s.anchor[j] = a[j]; s.n[j] = i128(i64(b[j]) - i64(a[j])); }
    s.d = 2; s.arity = 2; s.q3_power = false; s.orient = global_orientation(s.d, s.n);
    if (center_in_box(s, in.lo, in.hi)) census_and_emit(2, s);
  }

  MHGP11_LEAF_HD void q3() {
    const u32* a = t.P[prefix[0]];
    const u32* b = t.P[prefix[1]];
    const u32* cc = t.P[prefix[2]];
    if (!strictly_acute(a, b, cc)) return;
    const i64 mid[3][3] = {{i64(a[0]) + b[0], i64(a[1]) + b[1], i64(a[2]) + b[2]},
                           {i64(b[0]) + cc[0], i64(b[1]) + cc[1], i64(b[2]) + cc[2]},
                           {i64(a[0]) + cc[0], i64(a[1]) + cc[1], i64(a[2]) + cc[2]}};
    if (!doubled_envelope_meets(mid, in.lo, in.hi)) return;  // lemme M3
    const Vec u = diff(b, a), v = diff(cc, a), w = cross(u, v);
    const i128 g = i128(w.v[0]) * w.v[0] + i128(w.v[1]) * w.v[1] + i128(w.v[2]) * w.v[2];
    if (g == 0) return;  // impossible pour un triangle strictement aigu ; meme issue que la fabrique
    const i64 uu = dot(u, u), vv = dot(v, v);
    i128 t[3];
    for (int j = 0; j < 3; ++j) t[j] = i128(uu) * v.v[j] - i128(vv) * u.v[j];
    Center s;
    for (int j = 0; j < 3; ++j) s.anchor[j] = a[j];
    s.n[0] = t[1] * w.v[2] - t[2] * w.v[1];
    s.n[1] = t[2] * w.v[0] - t[0] * w.v[2];
    s.n[2] = t[0] * w.v[1] - t[1] * w.v[0];
    s.d = 2 * g; s.arity = 3;
    s.q3_power = q3_global_power(s.d, s.n);
    s.orient = global_orientation(s.d, s.n);
    if (center_in_box(s, in.lo, in.hi)) census_and_emit(3, s);
  }

  MHGP11_LEAF_HD void q4() {
    const u32* p[4] = {t.P[prefix[0]], t.P[prefix[1]], t.P[prefix[2]], t.P[prefix[3]]};
    if (orientation4(p[0], p[1], p[2], p[3]) == 0) return;
    ++c.q4_candidates;
    const i64 twice[4][3] = {{2 * i64(p[0][0]), 2 * i64(p[0][1]), 2 * i64(p[0][2])},
                             {2 * i64(p[1][0]), 2 * i64(p[1][1]), 2 * i64(p[1][2])},
                             {2 * i64(p[2][0]), 2 * i64(p[2][1]), 2 * i64(p[2][2])},
                             {2 * i64(p[3][0]), 2 * i64(p[3][1]), 2 * i64(p[3][2])}};
    if (!doubled_envelope_meets(twice, in.lo, in.hi)) return;  // lemme E4
    const Vec u = diff(p[1], p[0]), v = diff(p[2], p[0]), sv = diff(p[3], p[0]);
    const Vec vs = cross(v, sv), su = cross(sv, u), uv = cross(u, v);
    const i128 det = i128(u.v[0]) * vs.v[0] + i128(u.v[1]) * vs.v[1] + i128(u.v[2]) * vs.v[2];
    if (det == 0) { unresolved = true; return; }  // leaf.cpp : catalogue_invariant
    const i64 uu = dot(u, u), vv = dot(v, v), ss = dot(sv, sv);
    i128 n[3];
    for (int j = 0; j < 3; ++j) n[j] = i128(uu) * vs.v[j] + i128(vv) * su.v[j] + i128(ss) * uv.v[j];
    // Positivite de la presentation (q4_weights.hpp), numerateur BRUT, voie native du cube de cote 2^20.
    for (int j = 0; j < 3; ++j) {
      u32 low = p[0][j], high = p[0][j];
      for (int i = 1; i < 4; ++i) {
        low = p[i][j] < low ? p[i][j] : low;
        high = p[i][j] > high ? p[i][j] : high;
      }
      if (high - low > (u32(1) << 20)) { unresolved = true; return; }  // voie Wide de leaf.cpp
    }
    const Vec face{{vs.v[0] + su.v[0] + uv.v[0], vs.v[1] + su.v[1] + uv.v[1], vs.v[2] + su.v[2] + uv.v[2]}};
    const i128 h = 2 * (det * det);
    const auto scalar = [&](const Vec& normal) { return n[0] * normal.v[0] + n[1] * normal.v[1] + n[2] * normal.v[2]; };
    bool strict = false;
    const i128 w0 = h - scalar(face);
    if (w0 > 0) {
      const i128 w1 = scalar(vs);
      if (w1 > 0) {
        const i128 w2 = scalar(su);
        if (w2 > 0) strict = h - w0 - w1 - w2 > 0;
      }
    }
    Center s;
    for (int j = 0; j < 3; ++j) s.anchor[j] = p[0][j];
    i128 d = 2 * det;
    if (d < 0) {
      d = -d;
      for (int j = 0; j < 3; ++j) n[j] = -n[j];
    }
    for (int j = 0; j < 3; ++j) s.n[j] = n[j];
    s.d = d; s.arity = 4; s.q3_power = false; s.orient = global_orientation(s.d, s.n);
    if (!strict) return;
    if (center_in_box(s, in.lo, in.hi)) census_and_emit(4, s);
  }

  // Support canonique (support.cpp) : paire, puis triangle strictement aigu coplanaire au centre, puis tetraedre.
  MHGP11_LEAF_HD bool canonical(const Center& s, u32 count, u32* support, u32& qmin) {
    for (u32 i = 0; i < count; ++i)
      for (u32 j = i + 1; j < count; ++j)
        if (midpoint(s, t.P[shell_local[i]], t.P[shell_local[j]])) {
          support[0] = shell[i]; support[1] = shell[j]; support[2] = support[3] = kNoSite;
          qmin = 2;
          return true;
        }
    for (u32 i = 0; i < count; ++i)
      for (u32 j = i + 1; j < count; ++j)
        for (u32 k = j + 1; k < count; ++k) {
          const u32* a = t.P[shell_local[i]];
          const u32* b = t.P[shell_local[j]];
          const u32* cc = t.P[shell_local[k]];
          if (!strictly_acute(a, b, cc)) continue;
          int plane = 0;
          if (!center_orientation(a, b, cc, s, plane)) { unresolved = true; return false; }
          if (plane == 0) {
            support[0] = shell[i]; support[1] = shell[j]; support[2] = shell[k]; support[3] = kNoSite;
            qmin = 3;
            return true;
          }
        }
    for (u32 i = 0; i < count; ++i)
      for (u32 j = i + 1; j < count; ++j)
        for (u32 k = j + 1; k < count; ++k)
          for (u32 l = k + 1; l < count; ++l) {
            const u32* p[4] = {t.P[shell_local[i]], t.P[shell_local[j]], t.P[shell_local[k]], t.P[shell_local[l]]};
            bool inside = false;
            if (!center_inside(s, p, inside)) { unresolved = true; return false; }
            if (inside) {
              support[0] = shell[i]; support[1] = shell[j]; support[2] = shell[k]; support[3] = shell[l];
              qmin = 4;
              return true;
            }
          }
    unresolved = true;  // leaf.cpp : catalogue_invariant
    return false;
  }

  MHGP11_LEAF_HD void census_and_emit(u32 q, const Center& s) {
    ++c.judged;
    const u32 threshold = static_cast<u32>(in.kmax + 1) - q;
    u64 inside = 0, outside = 0;
    for (u32 j = 0; j < q; ++j) {
      inside |= t.dom[prefix[j]];
      outside |= t.domby[prefix[j]];
    }
    if ((inside & outside) != 0) { unresolved = true; return; }  // leaf.cpp : catalogue_invariant
    u32 p = 0, count = 0, cursor = 0;
    for (u32 i = 0; i < t.m; ++i) {
      ++c.census_tests;
      int relation = 0;
      if (cursor < q && i == prefix[cursor]) {
        ++cursor;
      } else if ((inside >> i) & 1u) {
        relation = -1;
      } else if ((outside >> i) & 1u) {
        relation = 1;
      } else if (!side(s, t.P[i], relation)) {
        unresolved = true;
        return;
      }
      if (relation < 0) {
        if (p == threshold) return;
        interior[p++] = in.sites[i];
      } else if (relation == 0) {
        shell_local[count] = i;
        shell[count++] = in.sites[i];
      }
    }
    if (count < q || cursor != q) { unresolved = true; return; }
    u32 generated[4] = {kNoSite, kNoSite, kNoSite, kNoSite};
    for (u32 i = 0; i < q; ++i) generated[i] = in.sites[prefix[i]];
    u32 support[4] = {generated[0], generated[1], generated[2], generated[3]};
    u32 qmin = q;
    if (count != q && !canonical(s, count, support, qmin)) return;
    for (int i = 0; i < 4; ++i)
      if (support[i] != generated[i]) return;  // S* sera visite dans cette meme feuille
    if (p + qmin > static_cast<u32>(in.kmax) + 1) return;
    if (q == 4) ++c.q4_levels;  // emission_level(Q4Candidate) materialise et compte
    Ball ball{{support[0], support[1], support[2], support[3]}, p, count, qmin};
    sink.emit(ball, interior, shell);
    ++c.emitted;
    c.incidences += u64(p) + count;
  }

  // Corps de la boucle d'extend pour le site i a la profondeur Depth ; candidates : candidats restants apres i.
  // Sans recursion (Recurse = false), rend les candidats et l'ensemble logique de la profondeur suivante au lieu de
  // les parcourir : la profondeur 0 de la feuille cooperative les distribue en paires. Rend faux si la boucle
  // appelante doit continuer sans descendre.
  template <int Depth, bool Recurse = true>
  MHGP11_LEAF_HD bool extend_one(u32 i, u64 candidates, u64 logical, u64* out_next = nullptr,
                                 u64* out_logical = nullptr) {
    constexpr int q = Depth + 1;
    ++c.prefixes;
    prefix[Depth] = i;
    const u64 mask = masks[Depth] | t.dom[i];
    masks[q] = mask;
    const u32 count = popc(mask);
    if (count > static_cast<u32>(in.kmax + 1 - q)) return false;  // G3
    if constexpr (q >= 3) {
      if (!lines_possible(q)) return false;
    }
    if constexpr (q == 4) q4();
    else if constexpr (q == 3) q3();
    else if constexpr (q == 2) q2();
    if (unresolved) return false;
    if constexpr (q < 4) {
      u64 next = 0, next_logical = 0;
      if (in.kmax + 1 - (q + 1) >= 0) {
        next_logical = logical & (~u64(0) << (i + 1)) & t.nbr[i];
        if (count > static_cast<u32>(in.kmax - q)) {
          c.prefixes += popc(next_logical);
          return false;
        }
        next = candidates;
        for (int j = 0; j < q; ++j) next &= t.live[q - 1][prefix[j]];
        c.prefixes += popc(next_logical) - popc(next);
      }
      if constexpr (Recurse) {
        extend<Depth + 1>(next, next_logical);
      } else {
        *out_next = next;
        *out_logical = next_logical;
      }
      return true;
    }
    return false;
  }

  template <int Depth>
  MHGP11_LEAF_HD void extend(u64 candidates, u64 logical) {
    constexpr int q = Depth + 1;
    if (in.kmax + 1 - q < 0) return;
    while (candidates != 0) {
      const u32 i = ctz(candidates);
      candidates &= candidates - 1;
      extend_one<Depth>(i, candidates, logical);
      if (unresolved) return;
    }
  }
};

// Une feuille : kOk et ses emissions et compteurs, ou kUnresolved (compteurs et emissions a jeter).
template <class Sink>
MHGP11_LEAF_HD u32 run_leaf(const Input& in, Counts& counts, Sink& sink) {
  if (in.m == 0 || in.m > kMaxSites || in.kmax < 1) return kUnresolved;
  Tables tables;
  fill_tables(tables, in);
  counts.dominance_tests += u64(in.m) * (in.m - 1) / 2;
  Leaf<Sink> leaf(in, counts, sink, tables);
  const u64 initial = in.m >= 64 ? ~u64(0) : (u64(1) << in.m) - 1;
  leaf.template extend<0>(initial, initial);
  return leaf.unresolved ? kUnresolved : kOk;
}

// Puits de comptage : nombre de boules et d'incidences d'une feuille (premiere passe compter-puis-ecrire).
struct CountSink {
  u64 balls = 0, incidences = 0;
  MHGP11_LEAF_HD void emit(const Ball& ball, const u32*, const u32*) {
    ++balls;
    incidences += u64(ball.p) + ball.m;
  }
};

// Feuille cooperative (6 octobre 2026 ; note aux auditeurs, section O). La profondeur 0 (sites i) est jouee sur un fil
// et rend les paires (i, j) de la profondeur 1 dans l'ordre du parcours ; chaque paire porte les candidats restants
// apres j et l'ensemble logique de sa profondeur, et son sous-arbre (extend_one<1> puis extend<2>, extend<3>) est
// independant des autres, hors le cache J2 (bit pose par seen_test_set, succes = tests - rangs distincts). Les
// emissions d'un sous-arbre de paire sont contigues dans l'ordre du parcours : leur ordre global est l'ordre des
// paires, et des comptes par paire suivis d'un prefixe placent chaque emission a sa place exacte.
inline constexpr u32 kMaxPairs = kMaxSites * (kMaxSites - 1) / 2;
struct PairTask {
  u32 i = 0, j = 0;
  u64 remaining = 0, logical = 0;  // candidats restants apres j ; ensemble logique de la profondeur 1
};

// Profondeur 0 : memes coupes et memes compteurs qu'extend<0> ; appelle out(k, i, j, remaining, logical, next) pour
// chaque paire k dans l'ordre du parcours (next : candidats de la profondeur 1 sous i, remaining = next au-dela de j) ;
// rend le nombre de paires.
template <class Sink, class Out>
MHGP11_LEAF_HD u32 depth0_pairs(Leaf<Sink>& leaf, Out&& out) {
  const u64 initial = leaf.in.m >= 64 ? ~u64(0) : (u64(1) << leaf.in.m) - 1;
  if (leaf.in.kmax + 1 - 1 < 0) return 0;
  u32 count = 0;
  u64 candidates = initial;
  while (candidates != 0) {
    const u32 i = ctz(candidates);
    candidates &= candidates - 1;
    u64 next = 0, next_logical = 0;
    if (!leaf.template extend_one<0, false>(i, candidates, initial, &next, &next_logical)) continue;
    if (leaf.in.kmax + 1 - 2 < 0) continue;
    for (u64 rest = next; rest != 0; rest &= rest - 1) out(count++, i, ctz(rest), rest & (rest - 1), next_logical, next);
  }
  return count;
}

// Sous-arbre d'une paire, sur un fil : prefixe (i), puis le corps d'extend<1> pour j et sa recursion.
template <class Sink>
MHGP11_LEAF_HD void run_pair(Leaf<Sink>& leaf, const PairTask& pair) {
  leaf.prefix[0] = pair.i;
  leaf.masks[0] = 0;
  leaf.masks[1] = leaf.t.dom[pair.i];
  leaf.template extend_one<1>(pair.j, pair.remaining, pair.logical);
}

// Emulation hote de la feuille cooperative, pour les portes et l'executeur hote : comptage des paires dans l'ordre
// INVERSE (les compteurs ne dependent pas de l'ordre), puis emissions des paires dans l'ordre du parcours (compteurs de
// cette seconde passe jetes, cache J2 remis a zero entre les passes). Memes statut, compteurs et emissions que run_leaf.
struct NullSink {
  MHGP11_LEAF_HD void emit(const Ball&, const u32*, const u32*) {}
};
// order : 0, paires comptees dans l'ordre inverse ; sinon k = (order * step + order) mod n, une permutation quand order
// est premier avec n, l'ordre inverse sinon (portes : ordres normal, inverse et permutes ; audit d117de397).
MHGP11_LEAF_HD u32 gcd(u32 a, u32 b) {
  while (b != 0) {
    const u32 r = a % b;
    a = b; b = r;
  }
  return a;
}
template <class Sink>
MHGP11_LEAF_HD u32 run_leaf_coop(const Input& in, Counts& counts, Sink& sink, u32 order = 0) {
  if (in.m == 0 || in.m > kMaxSites || in.kmax < 1) return kUnresolved;
  Tables tables;
  fill_tables(tables, in);
  counts.dominance_tests += u64(in.m) * (in.m - 1) / 2;
  PairTask pairs[kMaxPairs];
  NullSink none;
  Leaf<NullSink> root(in, counts, none, tables);
  const u32 n = depth0_pairs(root, [&](u32 k, u32 i, u32 j, u64 remaining, u64 logical, u64) {
    pairs[k] = PairTask{i, j, remaining, logical};
  });
  if (root.unresolved) return kUnresolved;
  CountSink tally_sink;
  if (order != 0 && gcd(order, n) != 1) order = 0;
  for (u32 step = 0; step < n; ++step) {
    const u32 k = order == 0 ? n - 1 - step : static_cast<u32>((u64(order) * step + order) % n);
    Leaf<CountSink> lane(in, counts, tally_sink, tables);
    run_pair(lane, pairs[k]);
    if (lane.unresolved) return kUnresolved;
  }
  for (u32 w = 0; w < kSeenWords; ++w) tables.seen[w] = 0;
  Counts discarded;
  for (u32 k = 0; k < n; ++k) {
    Leaf<Sink> lane(in, discarded, sink, tables);
    run_pair(lane, pairs[k]);
    if (lane.unresolved) return kUnresolved;
  }
  return kOk;
}

// Feuille sequentielle ou cooperative selon Input::coop.
template <class Sink>
MHGP11_LEAF_HD u32 run_leaf_any(const Input& in, Counts& counts, Sink& sink) {
  return in.coop ? run_leaf_coop(in, counts, sink) : run_leaf(in, counts, sink);
}

}  // namespace mhgp11::leaf_device
