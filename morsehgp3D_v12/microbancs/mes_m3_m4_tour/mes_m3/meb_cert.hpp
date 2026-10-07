// Coeur de LEV-MEB-CERT (MES-M3), partage par mhgp12_mes_m3 et par la mesure de resolution de mhgp12_vidage.
// Hors produit. Voir mes_m3.cpp pour le contrat ; les predicats exacts sont ceux de la v11 (src/num), lies.
#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
#include <vector>

#include "cloud/cloud.hpp"
#include "tower/meb.hpp"
#include "../common/format.hpp"
#include "welzl_proposal.hpp"

namespace mhgp12 {
namespace mebcert {
using namespace ::mhgp11;
namespace d = ::mhgp12::dump;

#ifdef MHGP12_MUTANT_T1_SANS_S_DANS_F
inline constexpr bool kMutantSansSDansF = true;
#else
inline constexpr bool kMutantSansSDansF = false;
#endif

// ---- Catalogue lu et table S* -> boule ----------------------------------------------------------------------------
struct Cat {
  u32 sites = 0, balls = 0;
  const u32* xyz = nullptr;
  const d::BallRec* rec = nullptr;
  const u64* off = nullptr;
  const u32* val = nullptr;
  std::span<const u32> interior(u32 b) const { return {val + off[b], rec[b].p}; }
  std::span<const u32> shell(u32 b) const { return {val + off[b] + rec[b].p, rec[b].m}; }
};

class SupportTable {
 public:
  void build(const d::BallRec* rec, u32 balls) {
    rec_ = rec;
    u64 capacity = 16;
    while (capacity < 2 * u64{balls}) capacity *= 2;
    slots_.assign(capacity, 0);
    mask_ = capacity - 1;
    for (u32 b = 0; b < balls; ++b) {
      u64 at = hash(rec[b].sstar) & mask_;
      while (slots_[at] != 0) {
        if (same(rec[slots_[at] - 1].sstar, rec[b].sstar))
          throw std::runtime_error("table S* : support en double (catalogue invalide)");
        at = (at + 1) & mask_;
      }
      slots_[at] = b + 1;
    }
  }
  // Egalite des quatre SiteIdx (completes par kNone). Rend kNone si absent.
  u32 find(const std::array<u32, 4>& key) const {
    for (u64 at = hash(key.data()) & mask_;; at = (at + 1) & mask_) {
      const u32 slot = slots_[at];
      if (slot == 0) return d::kNone;
      if (same(rec_[slot - 1].sstar, key.data())) return slot - 1;
    }
  }

 private:
  static u64 hash(const u32* k) {
    u64 h = 0x9E3779B97F4A7C15ull;
    for (int i = 0; i < 4; ++i) {
      h ^= u64{k[i]} + 0x632BE59BD9B4E019ull + (h << 6) + (h >> 2);
      h *= 0xff51afd7ed558ccdull;
    }
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ull;
    return h ^ (h >> 33);
  }
  static bool same(const u32* a, const u32* b) { return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]; }
  const d::BallRec* rec_ = nullptr;
  std::vector<u32> slots_;
  u64 mask_ = 0;
};

struct Ctx {
  const Cloud& cloud;
  const Cat& cat;
  const SupportTable& table;
  const std::vector<num::Point>& points;  // num::Point de chaque SiteIdx, prepare une fois
  // Variante nommee de la proposition : false = repere local (translation par le premier site, defaut) ;
  // true = coordonnees absolues, port litteral de la v10. Aucune decision n'en depend.
  bool absolute_frame = false;
};

struct Part {
  std::array<u32, kMaxPart> id{};
  u32 k = 0;
};

// ---- Voie nouvelle --------------------------------------------------------------------------------------------------
enum Route : u8 { kT1 = 0, kCertTable, kCertCensus, kFallbackTable, kFallbackCensus, kRouteCount, kNeedsFallback };
inline const char* const kRouteNames[kRouteCount] = {"t1", "cert_table", "cert_census", "repli_table", "repli_census"};
enum Why : u8 {
  kNoReason = 0,
  kProposalFailed,   // proposition flottante en echec (degenerescence, tours epuises)
  kSNotInF,          // S n'est pas dans F (inclusion d'indices) : LEM-T1 et le certificat sont inapplicables
  kCertDegenerate,   // support propose affinement dependant
  kCertNotStrict,    // coordonnee barycentrique nulle ou negative (triangle droit ou obtus, sommet hors du tetraedre)
  kCertOutside,      // un site de F est strictement hors de la sphere proposee
  kCertArith,        // refus arithmetique d'un predicat exact (jamais attendu)
  kWhyCount
};
inline const char* const kWhyNames[kWhyCount] = {"aucune", "proposition_echouee", "s_hors_de_f", "support_degenere",
                                          "barycentre_non_strict", "site_exterieur", "refus_arithmetique"};

struct NewOut {
  u8 route = kNeedsFallback, why = kNoReason;
  bool canonicalized = false;  // support propose valide mais non canonique (canonisation l'a change)
  u32 ball = d::kNone;
  std::array<u32, 4> support{d::kNone, d::kNone, d::kNone, d::kNone};
  u8 arity = 0;
  u64 sink = 0;  // materialisation de la sphere (voie census), lue pour ne pas etre eliminee
};

inline bool sorted_subset(const u32* a, u32 na, const u32* b, u32 nb) {
  u32 j = 0;
  for (u32 i = 0; i < na; ++i) {
    if (i > 0 && a[i] <= a[i - 1]) return false;  // multiensemble : pas de repetition dans S
    while (j < nb && b[j] < a[i]) ++j;
    if (j == nb || b[j] != a[i]) return false;
    ++j;
  }
  return true;
}

inline bool part_in_population(const Ctx& c, const Part& f, u32 b) {
  const auto inner = c.cat.interior(b), shell = c.cat.shell(b);
  for (u32 i = 0; i < f.k; ++i)
    if (!std::binary_search(inner.begin(), inner.end(), f.id[i]) &&
        !std::binary_search(shell.begin(), shell.end(), f.id[i]))
      return false;
  return true;
}

// Support minimal canonique parmi Z (sites de F sur la sphere, SiteIdx croissants) : cardinal minimal, puis premier
// dans l'ordre lexicographique. Predicats exacts de la v11 sur le centre certifie.
template <class Center>
int canonical_support(const Center& center, const u32* z, const num::Point* zp, u32 nz, std::array<u32, 4>& out) {
  for (u32 i = 0; i < nz; ++i)
    for (u32 j = i + 1; j < nz; ++j)
      if (num::is_midpoint(center, zp[i], zp[j])) {
        out = {z[i], z[j], d::kNone, d::kNone};
        return 2;
      }
  for (u32 i = 0; i < nz; ++i)
    for (u32 j = i + 1; j < nz; ++j)
      for (u32 l = j + 1; l < nz; ++l) {
        if (num::classify_triangle(zp[i], zp[j], zp[l]) != num::TriangleKind::strict) continue;
        auto o = num::orientation(zp[i], zp[j], zp[l], center);
        if (!o.ok()) return -1;
        if (o.value() == 0) {
          out = {z[i], z[j], z[l], d::kNone};
          return 3;
        }
      }
  for (u32 i = 0; i < nz; ++i)
    for (u32 j = i + 1; j < nz; ++j)
      for (u32 l = j + 1; l < nz; ++l)
        for (u32 m = l + 1; m < nz; ++m) {
          auto in = num::strictly_inside(center, zp[i], zp[j], zp[l], zp[m]);
          if (!in.ok()) return -1;
          if (in.value()) {
            out = {z[i], z[j], z[l], z[m]};
            return 4;
          }
        }
  return 0;
}

inline u64 level_bits(const num::Level& level) {
  const auto wide = num::to_wide(level.numerator());
  return wide.words[0] ^ (wide.words[1] << 1);
}
// Materialisation du niveau exact pour le census (voie cert_census) ; q2 : la Sphere est deja materialisee.
inline u64 materialize_sink(const num::Sphere& sphere, NewOut&) { return level_bits(sphere.level()); }
template <class Candidate>
u64 materialize_sink(const Candidate& candidate, NewOut& out) {
  auto sphere = candidate.materialize();
  if (!sphere.ok()) {
    out.route = kNeedsFallback;
    out.why = kCertArith;
    return 0;
  }
  return level_bits(sphere.value().level());
}

// Fin du certificat exact : cotes de tous les sites de F, canonisation, table ; materialisation si census.
template <class Center>
void finish_certificate(const Ctx& c, const Part& f, const num::Point* fp, const Center& center,
                        const std::array<u32, 4>& s, int q, NewOut& out) {
  std::array<u32, kMaxPart> z{};
  std::array<num::Point, kMaxPart> zp{};
  u32 nz = 0;
  for (u32 i = 0; i < f.k; ++i) {
    auto side = num::side(center, fp[i]);
    if (!side.ok()) {
      out.why = kCertArith;
      return;
    }
    if (side.value() > 0) {
      out.why = kCertOutside;
      return;
    }
    if (side.value() == 0) {
      z[nz] = f.id[i];
      zp[nz] = fp[i];
      ++nz;
    }
  }
  std::array<u32, 4> canonical = s;
  int arity = q;
  if (nz != static_cast<u32>(q)) {
    arity = canonical_support(center, z.data(), zp.data(), nz, canonical);
    if (arity <= 0) {
      out.why = arity < 0 ? kCertArith : kCertNotStrict;
      return;
    }
    out.canonicalized = canonical != s;
  }
  out.support = canonical;
  out.arity = static_cast<u8>(arity);
  out.ball = c.table.find(canonical);
  if (out.ball != d::kNone) {
    out.route = kCertTable;
    return;
  }
  out.route = kCertCensus;  // la sphere sert au census (materialisation du niveau, comme la v11)
  out.sink = materialize_sink(center, out);
}

// Tri croissant des q premiers identifiants d'un support (q dans [2, 4]) par insertion, a bornes explicites : meme
// resultat que std::sort, sans le faux positif -Warray-bounds de GCC 13 qu'engendre std::sort sur ce tableau de
// quatre cases quand certify est expansee dans une boucle de resolution (chemin des plus de seize elements).
inline void sort_support(std::array<u32, 4>& s, int q) {
  const int n = q < 4 ? q : 4;
  for (int i = 1; i < n; ++i)
    for (int j = i; j > 0 && s[j - 1] > s[j]; --j) std::swap(s[j - 1], s[j]);
}

// Certification d'un support propose S (indices de sites, quelconques) pour la partie F.
inline NewOut certify(const Ctx& c, const Part& f, const num::Point* fp, std::array<u32, 4> s, int q) {
  NewOut out;
  if (q < 2 || q > 4) {
    out.why = kCertDegenerate;
    return out;
  }
  sort_support(s, q);
  for (int i = q; i < 4; ++i) s[i] = d::kNone;
  // Correctif LEM-T1 (CST-0101) : S dans F, inclusion de multiensembles d'indices de sites. Echec : repli exact.
  if (!kMutantSansSDansF && !sorted_subset(s.data(), static_cast<u32>(q), f.id.data(), f.k)) {
    out.why = kSNotInF;
    return out;
  }
  const u32 b = c.table.find(s);
  if (b != d::kNone && part_in_population(c, f, b)) {
    out.route = kT1;
    out.ball = b;
    out.support = s;
    out.arity = static_cast<u8>(q);
    return out;
  }
  // Points du support (dans F : S dans F a ete verifie ; le mutant lit les points du nuage).
  std::array<num::Point, 4> sp{};
  for (int i = 0; i < q; ++i) sp[i] = c.points[s[i]];
  if (q == 2) {
    auto sphere = num::Sphere::through(sp[0], sp[1]);
    if (!sphere.ok()) {
      out.why = kCertArith;
      return out;
    }
    if (!sphere.value()) {
      out.why = kCertDegenerate;
      return out;
    }
    finish_certificate(c, f, fp, *sphere.value(), s, q, out);
    return out;
  }
  if (q == 3) {
    const auto kind = num::classify_triangle(sp[0], sp[1], sp[2]);
    if (kind == num::TriangleKind::degenerate) {
      out.why = kCertDegenerate;
      return out;
    }
    if (kind == num::TriangleKind::non_strict) {
      out.why = kCertNotStrict;
      return out;
    }
    auto candidate = num::Q3Candidate::through(sp[0], sp[1], sp[2]);
    if (!candidate.ok()) {
      out.why = kCertArith;
      return out;
    }
    if (!candidate.value()) {
      out.why = kCertDegenerate;
      return out;
    }
    finish_certificate(c, f, fp, *candidate.value(), s, q, out);
    return out;
  }
  auto candidate = num::Q4Candidate::through(sp[0], sp[1], sp[2], sp[3]);
  if (!candidate.ok()) {
    out.why = kCertArith;
    return out;
  }
  if (!candidate.value()) {
    out.why = kCertDegenerate;
    return out;
  }
  if (!candidate.value()->q4_presentation_strictly_inside()) {
    out.why = kCertNotStrict;
    return out;
  }
  finish_certificate(c, f, fp, *candidate.value(), s, q, out);
  return out;
}

inline std::array<SiteIdx, kMaxPart> as_sites(const Part& f) {
  std::array<SiteIdx, kMaxPart> ids{};
  for (u32 i = 0; i < f.k; ++i) ids[i] = SiteIdx{f.id[i]};
  return ids;
}

// Repli exact : bounded_meb de la v11 (la reference elle-meme), puis table.
inline NewOut fallback(const Ctx& c, const Part& f, u8 why) {
  NewOut out;
  out.why = why;
  const auto ids = as_sites(f);
  auto meb = bounded_meb(c.cloud, {ids.data(), f.k});
  if (!meb.ok()) throw std::runtime_error("repli : bounded_meb refuse");
  const auto support = meb.value().support();
  for (std::size_t i = 0; i < support.size(); ++i) out.support[i] = idx(support[i]);
  out.arity = static_cast<u8>(support.size());
  out.ball = c.table.find(out.support);
  out.route = out.ball != d::kNone ? kFallbackTable : kFallbackCensus;
  return out;
}

struct Proposal {
  bool ok = false;
  std::array<u32, 4> s{};
  int q = 0;
};

inline Proposal propose(const Ctx& c, const Part& f) {
  DWelzl w;
  const u32 o = f.id[0];
  const double ox = c.absolute_frame ? 0.0 : double(c.cloud.x()[o]);
  const double oy = c.absolute_frame ? 0.0 : double(c.cloud.y()[o]);
  const double oz = c.absolute_frame ? 0.0 : double(c.cloud.z()[o]);
  for (u32 i = 0; i < f.k; ++i) {
    const u32 s = f.id[i];
    w.p[i][0] = double(c.cloud.x()[s]) - ox;
    w.p[i][1] = double(c.cloud.y()[s]) - oy;
    w.p[i][2] = double(c.cloud.z()[s]) - oz;
  }
  const DBall ball = w.run(static_cast<int>(f.k));
  Proposal p;
  if (!w.ok || ball.nr < 2 || ball.nr > 4) return p;
  p.ok = true;
  p.q = ball.nr;
  for (int i = 0; i < ball.nr; ++i) p.s[i] = f.id[ball.R[i]];
  return p;
}

inline NewOut new_path(const Ctx& c, const Part& f) {
  std::array<num::Point, kMaxPart> fp{};
  for (u32 i = 0; i < f.k; ++i) fp[i] = c.points[f.id[i]];
  const Proposal p = propose(c, f);
  if (!p.ok) return fallback(c, f, kProposalFailed);
  NewOut out = certify(c, f, fp.data(), p.s, p.q);
  if (out.route == kNeedsFallback) return fallback(c, f, out.why);
  return out;
}

// Variante nommee « mere puis Welzl » : la proposition est d'abord le S* de la boule du pas precedent (la mere : cellule
// de la trace pour la premiere partie, B de la partie precedente sinon). Par la descente stricte, S*(mere) n'est jamais
// dans F, alors que F est toujours dans P_mere : c'est exactement le contre-exemple CST-0101, a chaque pas. Avec le test
// S dans F, la proposition est ecartee (raison s_hors_de_f) et DWelzl prend le relais ; sans lui (mutant), LEM-T1 rend
// la mere, fausse. Rend aussi si la mere a ete ecartee par S dans F.
inline NewOut new_path_mother(const Ctx& c, const Part& f, u32 mother, bool& mother_rejected) {
  mother_rejected = false;
  if (mother != d::kNone) {
    std::array<num::Point, kMaxPart> fp{};
    for (u32 i = 0; i < f.k; ++i) fp[i] = c.points[f.id[i]];
    const auto& rec = c.cat.rec[mother];
    const std::array<u32, 4> s{rec.sstar[0], rec.sstar[1], rec.sstar[2], rec.sstar[3]};
    NewOut out = certify(c, f, fp.data(), s, static_cast<int>(rec.q));
    if (out.route != kNeedsFallback) return out;
    mother_rejected = out.why == kSNotInF;
  }
  return new_path(c, f);
}

// ---- Reference ------------------------------------------------------------------------------------------------------
struct RefOut {
  u32 ball = d::kNone;
  std::array<u32, 4> support{d::kNone, d::kNone, d::kNone, d::kNone};
  u8 arity = 0;
  u64 presentations = 0;
};

inline RefOut reference(const Ctx& c, const Part& f) {
  RefOut out;
  const auto ids = as_sites(f);
  auto meb = bounded_meb(c.cloud, {ids.data(), f.k});
  if (!meb.ok()) throw std::runtime_error("reference : bounded_meb refuse");
  const auto support = meb.value().support();
  for (std::size_t i = 0; i < support.size(); ++i) out.support[i] = idx(support[i]);
  out.arity = static_cast<u8>(support.size());
  out.ball = c.table.find(out.support);
  out.presentations = meb.value().ledger().presentations;
  return out;
}

// ---- Juge d'identite (hors chronometre) -----------------------------------------------------------------------------
inline Result<num::Sphere> sphere_through(const Ctx& c, const std::array<u32, 4>& s, u8 q) {
  const auto& p = c.points;
  Result<std::optional<num::Sphere>> made = fail(Reason::arithmetic_invariant);
  if (q == 2) made = num::Sphere::through(p[s[0]], p[s[1]]);
  else if (q == 3) made = num::Sphere::through(p[s[0]], p[s[1]], p[s[2]]);
  else if (q == 4) made = num::Sphere::through(p[s[0]], p[s[1]], p[s[2]], p[s[3]]);
  if (!made.ok()) return made.outcome();
  if (!made.value()) return fail(Reason::arithmetic_invariant);
  return *made.value();
}

struct Verdict {
  bool sphere = true, ball = true, support = true;
  u32 shell_in_f = 0;  // sites de F sur la sphere de reference
};

inline Verdict judge(const Ctx& c, const Part& f, const NewOut& n, const RefOut& r) {
  Verdict v;
  const auto ids = as_sites(f);
  auto meb = bounded_meb(c.cloud, {ids.data(), f.k});
  auto mine = sphere_through(c, n.support, n.arity);
  if (!meb.ok() || !mine.ok()) {
    v.sphere = false;
    return v;
  }
  const auto& ref = meb.value().sphere();
  v.sphere = num::compare_centers(ref, mine.value()) == 0 && num::compare(ref.level(), mine.value().level()) == 0;
  v.ball = n.ball == r.ball;
  v.support = n.arity == r.arity && n.support == r.support;
  for (u32 i = 0; i < f.k; ++i) {
    auto side = num::side(ref, c.points[f.id[i]]);
    if (side.ok() && side.value() == 0) ++v.shell_in_f;
  }
  return v;
}

}  // namespace mebcert
}  // namespace mhgp12
