// Juge d'Euler a K+2 et de restriction J1 du catalogue (docs/MATHEMATIQUES.md, paragraphe 8 ; docs/CATALOGUE.md,
// section du juge d'Euler). Hors produit : lit un Catalogue construit et son Cloud sans rien modifier ; entiers exacts
// seulement, aucun flottant. Mecanisme repris de la v9 (docs/PROVENANCE.md) : euler_add et coquilles etendues de
// src/chain/tower_chain.cpp, note de l'auditeur C du 23 septembre 2026 et sa contrelecture par le nerf.
//
// Boule b de centre c, p interieurs stricts, coquille U de m sites ; ordre k, t = k - p. Si 1 <= t <= m :
//   e_k(b) = somme, sur les parties A de U avec c dans conv(A) et |A| >= t, de (-1)^(|A|-t) C(|A|-1, t-1),
// sinon e_k(b) = 0. Identite J3 : n [k = 1] + somme_b e_k(b) = 1 pour 1 <= k <= n.
// c est dans conv(A) si et seulement si A contient un support minimal de b : paire de milieu c, triangle strictement
// aigu coplanaire avec c, tetraedre contenant strictement c (Caratheodory ; un support minimal est affinement
// independant et c est dans l'interieur relatif de son enveloppe). Coquille reguliere (m = qmin) : A = U seulement,
// e_k = (-1)^(q-t) C(q-1, t-1). Une boule de contribution non nulle a l'ordre k a p <= k-1 et qmin <= 4 : Cat_{K'}
// les contient toutes pour k <= K'-2. Ordres verifiables de Cat_{K'} : 1..min(K'-2, n).
// Le juge est necessaire, pas suffisant : deux omissions de contributions opposees se compensent (fixtures gravees
// de tests/catalogue/euler_limits.py). Il ne certifie jamais la completude du catalogue.
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <optional>
#include <span>

#include "catalogue/catalogue.hpp"
#include "sched/sched.hpp"

namespace mhgp11::bench::euler {

inline constexpr int kMaxOrder = 12;   // ordres du catalogue : CatalogueParams::kmax <= 12
inline constexpr u32 kMaxShell = 24;   // coquille etendue jugee ; au-dela, refus explicite du juge (2^24 parties)
inline constexpr u64 kNoBall = ~u64{0};
inline constexpr u64 kGrain = 2048;    // boules par tranche du Pool

// C(a, b) pour 0 <= b <= a <= kMaxShell ; le plus grand, C(24, 12) = 2704156, tient en i64.
inline constexpr auto kBinomial = [] {
  std::array<std::array<i64, kMaxShell + 1>, kMaxShell + 1> c{};
  for (u32 a = 0; a <= kMaxShell; ++a) {
    c[a][0] = 1;
    for (u32 b = 1; b <= a; ++b) c[a][b] = c[a - 1][b - 1] + c[a - 1][b];
  }
  return c;
}();

// Ecart local d'une boule ; la premiere (plus petit indice) est publiee avec le nombre total.
enum class Fault : u8 { none, structure, degenerate, level, census, not_minimal, canonical };

inline const char* fault_name(Fault fault) noexcept {
  switch (fault) {
    case Fault::none: return "none";
    case Fault::structure: return "structure";
    case Fault::degenerate: return "support_degenere";
    case Fault::level: return "niveau";
    case Fault::census: return "recensement";
    case Fault::not_minimal: return "support_non_minimal";
    case Fault::canonical: return "support_canonique";
  }
  return "inconnu";
}

// Sommes exactes par ordre (indice k, la case 0 est inutilisee) et compteurs d'une passe ; une case par ouvrier.
struct Totals {
  std::array<i128, kMaxOrder + 1> regular{}, extended{};
  u64 balls = 0, omitted = 0, regular_balls = 0, extended_balls = 0, max_shell = 0;
  u64 supports = 0, census_sites = 0, faults = 0, first_fault_ball = kNoBall;
  std::array<u64, kMaxShell + 1> extended_by_shell{};
  std::array<u64, 5> by_qmin{};
  Fault first_fault = Fault::none;
  u32 refused_shell = 0;  // > kMaxShell : refus du juge avant tout calcul
};

inline void merge(Totals& into, const Totals& part) noexcept {
  for (int k = 0; k <= kMaxOrder; ++k) {
    into.regular[k] += part.regular[k];
    into.extended[k] += part.extended[k];
  }
  into.balls += part.balls;
  into.omitted += part.omitted;
  into.regular_balls += part.regular_balls;
  into.extended_balls += part.extended_balls;
  into.max_shell = std::max(into.max_shell, part.max_shell);
  into.supports += part.supports;
  into.census_sites += part.census_sites;
  into.faults += part.faults;
  if (part.first_fault_ball < into.first_fault_ball) {
    into.first_fault_ball = part.first_fault_ball;
    into.first_fault = part.first_fault;
  }
  for (u32 s = 0; s <= kMaxShell; ++s) into.extended_by_shell[s] += part.extended_by_shell[s];
  for (u32 q = 0; q < 5; ++q) into.by_qmin[q] += part.by_qmin[q];
}

// Ajoute e_k(b), k = 1..orders, d'apres counts[s] : parties de U de cardinal s dont l'enveloppe contient c.
// |e_k(b)| <= 2^(2m) et au plus 2^32 boules : les sommes i128 ne debordent pas.
inline void add_contributions(const std::array<u64, kMaxShell + 1>& counts, u32 p, u32 m, int orders,
                              std::array<i128, kMaxOrder + 1>& sums) noexcept {
  for (int k = 1; k <= orders; ++k) {
    const i64 t = i64{k} - i64{p};
    if (t < 1 || t > i64{m}) continue;
    i128 e = 0;
    for (u32 s = static_cast<u32>(t); s <= m; ++s) {
      const i128 term = i128{kBinomial[s - 1][t - 1]} * static_cast<i128>(counts[s]);
      e += (i64{s} - t) % 2 == 0 ? term : -term;
    }
    sums[k] += e;
  }
}

// Fermeture vers le haut (transformee de zeta en OU) des masques de supports marques dans words[0..2^(m-6)), puis
// decompte par cardinal : counts[s] = nombre de parties A de U, |A| = s, qui contiennent un support minimal.
inline void closure_counts(std::span<u64> words, u32 m, std::array<u64, kMaxShell + 1>& counts) noexcept {
  static constexpr std::array<u64, 6> kLow{0x5555555555555555ull, 0x3333333333333333ull, 0x0F0F0F0F0F0F0F0Full,
                                           0x00FF00FF00FF00FFull, 0x0000FFFF0000FFFFull, 0x00000000FFFFFFFFull};
  static constexpr auto kWeight = [] {
    std::array<u64, 7> weight{};
    for (u32 b = 0; b < 64; ++b) weight[std::popcount(b)] |= u64{1} << b;
    return weight;
  }();
  const u64 used = m > 6 ? u64{1} << (m - 6) : 1;
  for (u32 i = 0; i < std::min<u32>(m, 6); ++i)
    for (u64 w = 0; w < used; ++w) words[w] |= (words[w] & kLow[i]) << (1u << i);
  for (u32 i = 6; i < m; ++i)
    for (u64 w = 0; w < used; ++w)
      if (((w >> (i - 6)) & 1) != 0) words[w] |= words[w ^ (u64{1} << (i - 6))];
  counts.fill(0);
  for (u64 w = 0; w < used; ++w)
    if (words[w] != 0)
      for (u32 r = 0; r <= 6; ++r) counts[std::popcount(w) + r] += std::popcount(words[w] & kWeight[r]);
}

struct Support {
  std::array<u32, 4> at{};  // positions dans la coquille triee
  u32 q = 0;                // 0 : aucun support trouve
};

// Supports minimaux de la coquille, marques dans words (masques de positions). L'enumeration suit le cardinal puis
// l'ordre lexicographique des positions, donc des SiteIdx : le premier support trouve est le support canonique S*.
inline Outcome mark_supports(const num::Sphere& sphere, std::span<const num::Point> u, std::span<u64> words,
                             u64& supports, Support& first) noexcept {
  const u32 m = static_cast<u32>(u.size());
  auto mark = [&](u32 mask, Support found) {
    words[mask >> 6] |= u64{1} << (mask & 63);
    ++supports;
    if (first.q == 0) first = found;
  };
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      if (num::is_midpoint(sphere, u[i], u[j])) mark((1u << i) | (1u << j), {{i, j, 0, 0}, 2});
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k) {
        if (!num::strictly_acute(u[i], u[j], u[k])) continue;
        const auto plane = num::orientation(u[i], u[j], u[k], sphere);
        if (!plane.ok()) return plane.outcome();
        if (plane.value() == 0) mark((1u << i) | (1u << j) | (1u << k), {{i, j, k, 0}, 3});
      }
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k)
        for (u32 l = k + 1; l < m; ++l) {
          const auto inside = num::strictly_inside(sphere, u[i], u[j], u[k], u[l]);
          if (!inside.ok()) return inside.outcome();
          if (inside.value()) mark((1u << i) | (1u << j) | (1u << k) | (1u << l), {{i, j, k, l}, 4});
        }
  return {};
}

inline Result<num::Point> site_point(const Cloud& cloud, SiteIdx site) noexcept {
  const u32 i = idx(site);
  return num::Point::make(cloud.x()[i], cloud.y()[i], cloud.z()[i]);
}

// Forme de la boule b : bornes du CSR, rang, admission, support croissant inclus dans U, I et U croissants disjoints.
inline bool well_formed(const Cloud& cloud, const Catalogue& cat, u64 b) noexcept {
  const auto& ball = cat.balls_data()[b];
  const auto off = cat.population_offsets();
  const u32 n = cloud.sites(), q = ball.qmin;
  if (q < 2 || q > 4 || ball.m < q || ball.p + q > u32{cat.kmax()} + 1) return false;
  if (off[b] > off[b + 1] || off[b + 1] > cat.population().size() || off[b + 1] - off[b] != u64{ball.p} + ball.m)
    return false;
  if (idx(ball.rank) == 0 || idx(ball.rank) >= cat.levels().size()) return false;
  for (u32 j = 0; j < 4; ++j) {
    const u32 s = idx(ball.support[j]);
    if (j < q ? (s >= n || (j > 0 && s <= idx(ball.support[j - 1]))) : s != kNone) return false;
  }
  const BallIdx id = make_id<BallIdx>(static_cast<u32>(b));
  const auto inner = cat.interior(id), shell = cat.shell(id);
  for (auto list : {inner, shell})
    for (u64 i = 0; i < list.size(); ++i)
      if (idx(list[i]) >= n || (i > 0 && idx(list[i]) <= idx(list[i - 1]))) return false;
  u64 i = 0, j = 0, found = 0;
  while (i < inner.size() && j < shell.size()) {  // I et U disjoints
    if (inner[i] == shell[j]) return false;
    if (idx(inner[i]) < idx(shell[j])) ++i;
    else ++j;
  }
  for (u64 s = 0; s < shell.size() && found < q; ++s) found += shell[s] == ball.support[found] ? 1 : 0;
  return found == q;
}

struct Job {
  const Cloud& cloud;
  const Catalogue& cat;
  std::span<const u8> omitted;  // vide : aucune omission
  std::span<Totals> partial;    // une case par ouvrier
  std::span<u64> masks;         // `words` mots par ouvrier
  u64 words = 0;
};

// Juge une boule : forme, sphere refaite depuis S*, niveau exact, signes de puissance de I et U, minimalite de S*
// (coquille reguliere) ou supports, qmin et S* recalcules (coquille etendue), puis contributions exactes.
inline Outcome judge_ball(Job& job, u64 b, u32 worker) noexcept {
  Totals& out = job.partial[worker];
  if (!job.omitted.empty() && job.omitted[b] != 0) {
    ++out.omitted;
    return {};
  }
  ++out.balls;
  auto fault = [&](Fault kind) {
    ++out.faults;
    if (b < out.first_fault_ball) {
      out.first_fault_ball = b;
      out.first_fault = kind;
    }
    return Outcome{};
  };
  const auto& cat = job.cat;
  const auto& ball = cat.balls_data()[b];
  if (!well_formed(job.cloud, cat, b)) return fault(Fault::structure);
  const u32 q = ball.qmin, p = ball.p, m = ball.m;
  std::array<num::Point, 4> v{};
  for (u32 j = 0; j < q; ++j) {
    const auto point = site_point(job.cloud, ball.support[j]);
    if (!point.ok()) return point.outcome();
    v[j] = point.value();
  }
  const auto made = q == 2 ? num::Sphere::through(v[0], v[1])
                  : q == 3 ? num::Sphere::through(v[0], v[1], v[2]) : num::Sphere::through(v[0], v[1], v[2], v[3]);
  if (!made.ok()) return made.outcome();
  if (!made.value()) return fault(Fault::degenerate);
  const num::Sphere& sphere = *made.value();
  if (num::compare(sphere.level(), cat.levels()[idx(ball.rank)]) != 0) return fault(Fault::level);
  const BallIdx id = make_id<BallIdx>(static_cast<u32>(b));
  const auto shell = cat.shell(id);
  std::array<num::Point, kMaxShell> u{};
  for (int pass = 0; pass < 2; ++pass) {
    const auto list = pass == 0 ? cat.interior(id) : shell;
    for (u64 i = 0; i < list.size(); ++i) {
      const auto point = site_point(job.cloud, list[i]);
      if (!point.ok()) return point.outcome();
      const auto side = num::side(sphere, point.value());
      if (!side.ok()) return side.outcome();
      if (side.value() != (pass == 0 ? -1 : 0)) return fault(Fault::census);
      if (pass == 1 && m <= kMaxShell) u[i] = point.value();
    }
  }
  out.census_sites += u64{p} + m;
  std::array<u64, kMaxShell + 1> counts{};
  if (m == q) {
    const auto inside = q == 4 ? num::strictly_inside(sphere, v[0], v[1], v[2], v[3]) : Result<bool>(true);
    if (!inside.ok()) return inside.outcome();
    const bool minimal = q == 2 ? num::is_midpoint(sphere, v[0], v[1])
                       : q == 3 ? num::strictly_acute(v[0], v[1], v[2]) : inside.value();
    if (!minimal) return fault(Fault::not_minimal);
    counts[q] = 1;
    ++out.regular_balls;
    add_contributions(counts, p, m, cat.kmax(), out.regular);
  } else {
    if (m > kMaxShell) return fault(Fault::structure);  // exclu par la pre-passe de euler_totals
    const auto words = job.masks.subspan(worker * job.words, job.words);
    const u64 used = m > 6 ? u64{1} << (m - 6) : 1;
    std::fill_n(words.begin(), used, u64{0});
    Support first;
    MHGP11_TRY(mark_supports(sphere, std::span<const num::Point>(u.data(), m), words, out.supports, first));
    bool canonical = first.q == q;
    for (u32 j = 0; canonical && j < q; ++j) canonical = shell[first.at[j]] == ball.support[j];
    if (!canonical) return fault(Fault::canonical);
    closure_counts(words, m, counts);
    ++out.extended_balls;
    ++out.extended_by_shell[m];
    add_contributions(counts, p, m, cat.kmax(), out.extended);
  }
  ++out.by_qmin[q];
  out.max_shell = std::max<u64>(out.max_shell, m);
  return {};
}

inline Outcome euler_chunk(void* context, u64 begin, u64 end, u32 worker) noexcept {
  auto& job = *static_cast<Job*>(context);
  for (u64 b = begin; b < end; ++b) MHGP11_TRY(judge_ball(job, b, worker));
  return {};
}

// Contributions de toutes les boules non omises, ordres 1..cat.kmax(). Pre-passe : la plus grande coquille etendue
// fixe les brouillons ; au-dela de kMaxShell, refus du juge (totals.refused_shell) sans aucun calcul. Les sommes
// entieres ne dependent ni du nombre de fils ni de l'ordre des tranches.
inline Outcome euler_totals(const Cloud& cloud, const Catalogue& cat, std::span<const u8> omitted, sched::Pool* pool,
                            MemoryBudget& budget, Totals& totals) noexcept {
  totals = Totals{};
  u32 widest = 0;
  for (u64 b = 0; b < cat.balls(); ++b) {
    const auto& ball = cat.balls_data()[b];
    if (ball.m > ball.qmin && (omitted.empty() || omitted[b] == 0)) widest = std::max(widest, ball.m);
  }
  if (widest > kMaxShell) {
    totals.refused_shell = widest;
    return {};
  }
  const u32 workers = pool == nullptr ? 1 : pool->size();
  const u64 words = widest > 6 ? u64{1} << (widest - 6) : 1;
  MHGP11_TRY(budget.admit(u64{workers} * (sizeof(Totals) + words * sizeof(u64))));
  Buffer<Totals> partial;
  Buffer<u64> masks;
  MHGP11_TRY(partial.allocate(workers, budget));
  MHGP11_TRY(masks.allocate(u64{workers} * words, budget));
  std::fill(partial.begin(), partial.end(), Totals{});
  Job job{cloud, cat, omitted, partial.span(), masks.span(), words};
  if (pool != nullptr) MHGP11_TRY(pool->parallel_for(cat.balls(), kGrain, &job, euler_chunk));
  else MHGP11_TRY(euler_chunk(&job, 0, cat.balls(), 0));
  for (const auto& part : partial) merge(totals, part);
  return {};
}

// ---- J1, restriction : Cat_K = filtre p + qmin <= K + 1 de Cat_{K'}, apres recalcul des rangs ----

enum class Mismatch : u8 { none, missing, extra, fields, level, rank, raw, levels_table, large_order };

inline const char* mismatch_name(Mismatch kind) noexcept {
  switch (kind) {
    case Mismatch::none: return "none";
    case Mismatch::missing: return "absente_de_cat_k";
    case Mismatch::extra: return "en_trop_dans_cat_k";
    case Mismatch::fields: return "champs";
    case Mismatch::level: return "niveau";
    case Mismatch::rank: return "rang";
    case Mismatch::raw: return "niveau_brut";
    case Mismatch::levels_table: return "table_des_niveaux";
    case Mismatch::large_order: return "ordre_de_cat_k2";
  }
  return "inconnu";
}

struct Restriction {
  u64 filtered = 0, compared = 0, missing = 0, extra = 0, fields = 0, raw_checks = 0, raw_recomputed = 0;
  u64 first_large = kNoBall, first_small = kNoBall;
  Mismatch first = Mismatch::none;
  u64 mismatches() const noexcept { return missing + extra + fields; }
  void record(Mismatch kind, u64 large, u64 small) noexcept {
    if (kind == Mismatch::missing) ++missing;
    else if (kind == Mismatch::extra) ++extra;
    else ++fields;
    if (first == Mismatch::none) {
      first = kind;
      first_large = large;
      first_small = small;
    }
  }
};

template <class T>
bool same_integer(const T& a, const T& b) noexcept {
  const auto x = num::to_wide(a), y = num::to_wide(b);
  return x.neg == y.neg && x.words == y.words;
}

// Egalite BRUTE (numerateur et denominateur non reduits), distincte de l'egalite exacte num::compare == 0.
inline bool same_raw(const num::Level& a, const num::Level& b) noexcept {
  return same_integer(a.numerator(), b.numerator()) && same_integer(a.denominator(), b.denominator());
}

// Niveau brut de la presentation S* (le meme pour tout ordre des sommets : formules symetriques de num) ; vide si
// le support est degenere (ecart deja vu par la passe d'Euler).
inline Result<std::optional<num::Level>> support_level(const Cloud& cloud, const CatalogueBall& ball) noexcept {
  std::array<num::Point, 4> v{};
  for (u32 j = 0; j < ball.qmin; ++j) {
    const auto point = site_point(cloud, ball.support[j]);
    if (!point.ok()) return point.outcome();
    v[j] = point.value();
  }
  const auto made = ball.qmin == 2 ? num::Sphere::through(v[0], v[1])
                  : ball.qmin == 3 ? num::Sphere::through(v[0], v[1], v[2])
                                   : num::Sphere::through(v[0], v[1], v[2], v[3]);
  if (!made.ok()) return made.outcome();
  if (!made.value()) return std::optional<num::Level>{};
  return std::optional<num::Level>{made.value()->level()};
}

// Rangs de Cat_{K'} : denses depuis 1, croissants avec le niveau exact, support croissant a niveau egal ; levels()[0]
// est le niveau nul brut.
inline void check_large_order(const Catalogue& large, Restriction& out) noexcept {
  const auto balls = large.balls_data();
  const auto levels = large.levels();
  for (u64 i = 0; i < balls.size(); ++i) {
    const u32 rank = idx(balls[i].rank), previous = i == 0 ? 0 : idx(balls[i - 1].rank);
    bool ok = rank >= 1 && rank < levels.size() && (rank == previous || rank == previous + 1);
    if (ok && i > 0 && rank == previous) ok = balls[i - 1].support < balls[i].support;
    if (ok && i > 0 && rank == previous + 1) ok = num::compare(levels[previous], levels[rank]) < 0;
    if (!ok) out.record(Mismatch::large_order, i, kNoBall);
  }
  const u32 last = balls.empty() ? 0 : idx(balls.back().rank);
  if (levels.empty() || u64{last} + 1 != levels.size() || !same_raw(levels[0], num::Level{}))
    out.record(Mismatch::large_order, kNoBall, kNoBall);
}

// Champs d'une paire appariee (meme niveau exact et meme S*) : p, m, qmin, listes I et U.
inline bool same_fields(const Catalogue& small, u64 j, const Catalogue& large, u64 i) noexcept {
  const auto& a = small.balls_data()[j];
  const auto& b = large.balls_data()[i];
  if (a.p != b.p || a.m != b.m || a.qmin != b.qmin) return false;
  const auto id_a = make_id<BallIdx>(static_cast<u32>(j)), id_b = make_id<BallIdx>(static_cast<u32>(i));
  const auto ia = small.interior(id_a), ib = large.interior(id_b), ua = small.shell(id_a), ub = large.shell(id_b);
  return std::equal(ia.begin(), ia.end(), ib.begin(), ib.end()) &&
         std::equal(ua.begin(), ua.end(), ub.begin(), ub.end());
}

// Jointure ordonnee de Cat_K (small) et du filtre p + qmin <= K + 1 de Cat_{K'} (large), cle (niveau exact, S*) :
// une boule filtree sans partenaire est absente de Cat_K, une boule de Cat_K sans partenaire est en trop. Les rangs
// sont recalcules sur le filtre (rang dense des niveaux exacts). Sans omission, le rang stocke de chaque boule de
// Cat_K doit egaler ce rang, le niveau de la table au debut de chaque rang doit etre BRUT-egal a celui de la
// presentation S* (recalcule si la premiere boule de ce niveau dans Cat_{K'} est hors du filtre), et la table a
// exactement (rangs + 1) niveaux. Avec omissions (fixtures du harnais), seules les cles et les champs comptent.
inline Outcome restriction(const Cloud& cloud, const Catalogue& small, const Catalogue& large,
                           std::span<const u8> omit_small, std::span<const u8> omit_large, Restriction& out) noexcept {
  out = Restriction{};
  const bool omissions = !omit_small.empty() || !omit_large.empty();
  if (!omissions) check_large_order(large, out);
  const u32 limit = u32{small.kmax()} + 1;
  const auto sb = small.balls_data(), lb = large.balls_data();
  const auto sl = small.levels(), ll = large.levels();
  u64 i = 0, j = 0, previous = kNoBall;  // previous : derniere boule filtree de Cat_{K'}
  u32 rank = 0;
  for (;;) {
    while (i < lb.size() && ((!omit_large.empty() && omit_large[i] != 0) || lb[i].p + lb[i].qmin > limit)) ++i;
    while (j < sb.size() && !omit_small.empty() && omit_small[j] != 0) ++j;
    if (i == lb.size() && j == sb.size()) break;
    if (i < lb.size() && idx(lb[i].rank) >= ll.size()) {  // rang hors table : ecart, jamais une lecture hors borne
      out.record(Mismatch::large_order, i++, kNoBall);
      continue;
    }
    if (j < sb.size() && idx(sb[j].rank) >= sl.size()) {
      out.record(Mismatch::rank, kNoBall, j++);
      continue;
    }
    int order = i < lb.size() ? -1 : 1;  // < 0 : la boule filtree i vient d'abord
    if (i < lb.size() && j < sb.size()) {
      order = num::compare(ll[idx(lb[i].rank)], sl[idx(sb[j].rank)]);
      if (order == 0) order = lb[i].support < sb[j].support ? -1 : (sb[j].support < lb[i].support ? 1 : 0);
    }
    if (order > 0) {
      out.record(Mismatch::extra, kNoBall, j++);
      continue;
    }
    ++out.filtered;
    const bool starts = previous == kNoBall || num::compare(ll[idx(lb[previous].rank)], ll[idx(lb[i].rank)]) != 0;
    if (starts) ++rank;
    previous = i;
    if (order < 0) {
      out.record(Mismatch::missing, i++, kNoBall);
      continue;
    }
    ++out.compared;
    if (!same_fields(small, j, large, i)) out.record(Mismatch::fields, i, j);
    if (!omissions && idx(sb[j].rank) != rank) out.record(Mismatch::rank, i, j);
    if (!omissions && idx(sb[j].rank) == rank && starts) {
      ++out.raw_checks;
      if (i == 0 || lb[i - 1].rank != lb[i].rank) {
        if (!same_raw(sl[rank], ll[idx(lb[i].rank)])) out.record(Mismatch::raw, i, j);
      } else {
        ++out.raw_recomputed;
        const auto raw = support_level(cloud, lb[i]);
        if (!raw.ok()) return raw.outcome();
        if (!raw.value() || !same_raw(sl[rank], *raw.value())) out.record(Mismatch::raw, i, j);
      }
    }
    ++i;
    ++j;
  }
  if (!omissions && (sl.empty() || sl.size() != u64{rank} + 1 || !same_raw(sl[0], num::Level{})))
    out.record(Mismatch::levels_table, kNoBall, kNoBall);
  return {};
}

}  // namespace mhgp11::bench::euler
