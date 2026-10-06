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

template <class Sink>
struct Leaf {
  const Input& in;
  Counts& c;
  Sink& sink;
  u32 m = 0;
  u32 P[kMaxSites][3];
  u64 dom[kMaxSites], domby[kMaxSites], nbr[kMaxSites];
  u64 live[3][kMaxSites];
  u64 masks[5];
  u32 prefix[4];
  u32 seen[kSeenWords];
  u32 interior[kMaxSites], shell[kMaxSites], shell_local[kMaxSites];
  bool unresolved = false;

  MHGP11_LEAF_HD Leaf(const Input& input, Counts& counts, Sink& out) : in(input), c(counts), sink(out) {}

  MHGP11_LEAF_HD void prepare() {
    m = in.m;
    for (u32 i = 0; i < m; ++i) {
      const u32 s = in.sites[i];
      P[i][0] = in.x[s]; P[i][1] = in.y[s]; P[i][2] = in.z[s];
      dom[i] = domby[i] = nbr[i] = 0;
      live[0][i] = live[1][i] = live[2][i] = 0;
    }
    for (int q = 0; q < 5; ++q) masks[q] = 0;
    for (u32 w = 0; w < kSeenWords; ++w) seen[w] = 0;
    c.dominance_tests += u64(m) * (m - 1) / 2;
    // Forme affine de la difference des distances sur la fermeture ; sommes < 12*2^(2B) en i64.
    for (u32 i = 0; i < m; ++i)
      for (u32 j = i + 1; j < m; ++j) {
        i64 base = 0, cmin = 0, cmax = 0;
        for (int axis = 0; axis < 3; ++axis) {
          const i64 delta = i64(P[j][axis]) - i64(P[i][axis]);
          base += i64(P[j][axis]) * P[j][axis] - i64(P[i][axis]) * P[i][axis];
          cmin += (delta > 0 ? in.lo[axis] : in.hi[axis]) * delta;
          cmax += (delta > 0 ? in.hi[axis] : in.lo[axis]) * delta;
        }
        if (base - 2 * cmin < 0) {  // j domine i
          dom[i] |= u64(1) << j;
          domby[j] |= u64(1) << i;
        } else if (base - 2 * cmax > 0) {  // i domine j
          dom[j] |= u64(1) << i;
          domby[i] |= u64(1) << j;
        } else {
          nbr[i] |= u64(1) << j;
          nbr[j] |= u64(1) << i;
        }
      }
  }

  MHGP11_LEAF_HD void live_rows() {
    for (u32 x = 0; x < m; ++x)
      for (u64 rest = nbr[x]; rest != 0; rest &= rest - 1) {
        const u32 y = ctz(rest);
        const int weight = static_cast<int>(popc(dom[x] | dom[y]));
        for (int q = 2; q <= 4; ++q)
          if (weight <= in.kmax + 1 - q) live[q - 2][x] |= u64(1) << y;
      }
  }

  MHGP11_LEAF_HD static u32 rank(u32 i, u32 j, u32 k) { return k * (k - 1) * (k - 2) / 6 + j * (j - 1) / 2 + i; }

  // J2, droites des faces contenant le dernier site ; cache simule pour les seuls compteurs.
  MHGP11_LEAF_HD bool lines_possible(int q) {
    const u32 last = prefix[q - 1];
    for (int j = 0; j + 2 < q; ++j)
      for (int k = j + 1; k + 1 < q; ++k) {
        ++c.region_line_tests;
        const u32 a = prefix[j], b = prefix[k];
        bool hit = false;
        if (in.cache) {
          const u32 r = rank(a, b, last);
          hit = ((seen[r >> 5] >> (r & 31)) & 1u) != 0;
          seen[r >> 5] |= 1u << (r & 31);
        }
        if (hit) ++c.region_line_cache_hits; else ++c.region_line_evaluations;
        if (center_line_meets(P[a], P[b], P[last], in.lo, in.hi) != kIntersects) {
          ++c.region_line_rejects;
          return false;
        }
      }
    return true;
  }

  MHGP11_LEAF_HD void q2() {
    const u32* a = P[prefix[0]];
    const u32* b = P[prefix[1]];
    Center s;
    for (int j = 0; j < 3; ++j) { s.anchor[j] = a[j]; s.n[j] = i128(i64(b[j]) - i64(a[j])); }
    s.d = 2; s.arity = 2; s.q3_power = false; s.orient = global_orientation(s.d, s.n);
    if (center_in_box(s, in.lo, in.hi)) census_and_emit(2, s);
  }

  MHGP11_LEAF_HD void q3() {
    const u32* a = P[prefix[0]];
    const u32* b = P[prefix[1]];
    const u32* cc = P[prefix[2]];
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
    const u32* p[4] = {P[prefix[0]], P[prefix[1]], P[prefix[2]], P[prefix[3]]};
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
        if (midpoint(s, P[shell_local[i]], P[shell_local[j]])) {
          support[0] = shell[i]; support[1] = shell[j]; support[2] = support[3] = kNoSite;
          qmin = 2;
          return true;
        }
    for (u32 i = 0; i < count; ++i)
      for (u32 j = i + 1; j < count; ++j)
        for (u32 k = j + 1; k < count; ++k) {
          const u32* a = P[shell_local[i]];
          const u32* b = P[shell_local[j]];
          const u32* cc = P[shell_local[k]];
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
            const u32* p[4] = {P[shell_local[i]], P[shell_local[j]], P[shell_local[k]], P[shell_local[l]]};
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
      inside |= dom[prefix[j]];
      outside |= domby[prefix[j]];
    }
    if ((inside & outside) != 0) { unresolved = true; return; }  // leaf.cpp : catalogue_invariant
    u32 p = 0, count = 0, cursor = 0;
    for (u32 i = 0; i < m; ++i) {
      ++c.census_tests;
      int relation = 0;
      if (cursor < q && i == prefix[cursor]) {
        ++cursor;
      } else if ((inside >> i) & 1u) {
        relation = -1;
      } else if ((outside >> i) & 1u) {
        relation = 1;
      } else if (!side(s, P[i], relation)) {
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

  template <int Depth>
  MHGP11_LEAF_HD void extend(u64 candidates, u64 logical) {
    constexpr int q = Depth + 1;
    const int threshold = in.kmax + 1 - q;
    if (threshold < 0) return;
    while (candidates != 0) {
      const u32 i = ctz(candidates);
      candidates &= candidates - 1;
      ++c.prefixes;
      prefix[Depth] = i;
      const u64 mask = masks[Depth] | dom[i];
      masks[q] = mask;
      const u32 count = popc(mask);
      if (count > static_cast<u32>(threshold)) continue;  // G3
      if constexpr (q >= 3) {
        if (!lines_possible(q)) continue;
      }
      if constexpr (q == 4) q4();
      else if constexpr (q == 3) q3();
      else if constexpr (q == 2) q2();
      if (unresolved) return;
      if constexpr (q < 4) {
        u64 next = 0, next_logical = 0;
        if (in.kmax + 1 - (q + 1) >= 0) {
          next_logical = logical & (~u64(0) << (i + 1)) & nbr[i];
          if (count > static_cast<u32>(in.kmax - q)) {
            c.prefixes += popc(next_logical);
            continue;
          }
          next = candidates;
          for (int j = 0; j < q; ++j) next &= live[q - 1][prefix[j]];
          c.prefixes += popc(next_logical) - popc(next);
        }
        extend<Depth + 1>(next, next_logical);
        if (unresolved) return;
      }
    }
  }
};

// Une feuille : kOk et ses emissions et compteurs, ou kUnresolved (compteurs et emissions a jeter).
template <class Sink>
MHGP11_LEAF_HD u32 run_leaf(const Input& in, Counts& counts, Sink& sink) {
  if (in.m == 0 || in.m > kMaxSites || in.kmax < 1) return kUnresolved;
  Leaf<Sink> leaf(in, counts, sink);
  leaf.prepare();
  leaf.live_rows();
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

}  // namespace mhgp11::leaf_device
