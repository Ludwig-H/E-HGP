// MorseHGP3D v5 — signature canonique de l'objet produit.
//
// Deux objets differents peuvent partager toutes leurs cardinalites : les
// campagnes hors juge comparent des digests SHA-256 d'une serialisation
// VERSIONNEE (petit-boutiste, largeurs fixes). Le format `mhgp4-digest-v1`
// est reproduit A L'IDENTIQUE : c'est la porte de conformite v4 ≡ v5
// (receipts/conformite_v4/digests_v4.txt), calculee par la v4 sur les memes
// entrees. Contenu :
//   balls  : boules post-RLE (cle primitive, representant de niveau, arite) ;
//   forest : par K, facet_keys, final_canon_fid, deltas ;
//   all    : chainage ordonne des hex des K.
#pragma once

#include <string>
#include <vector>

#include "../core/sha256.hpp"
#include "../forest/fold.hpp"
#include "candidates.hpp"

namespace mhgp5 {

namespace digest_detail {
struct Writer {
  Sha256 h;
  void tag(const char* t) { h.update(t, std::strlen(t)); }  // format v4 : sans longueur
  void u8v(u8 v) { h.update(&v, 1); }
  void u32v(u32 v) {
    u8 b[4];
    for (int i = 0; i < 4; ++i) b[i] = (u8)(v >> (8 * i));
    h.update(b, 4);
  }
  void u64v(u64 v) { h.u64le(v); }
  void i128v(i128 v) { h.i128le(v); }
  void facet(const FacetKey& f) {
    u8v(f.k);
    for (int i = 0; i < kFacetMaxK; ++i) u32v(f.p[(size_t)i]);
  }
  void level(const ExactLevel& l) {
    for (int i = 0; i < 3; ++i) u64v(l.num[i]);
    i128v(l.den);
  }
};
}  // namespace digest_detail

inline std::string digest_balls_v4(const std::vector<BallCandidate>& cands) {
  digest_detail::Writer d;
  d.tag("mhgp4-digest-v1:balls");
  d.u64v((u64)cands.size());
  for (const BallCandidate& c : cands) {
    d.i128v(c.key.a);
    for (int i = 0; i < 3; ++i) d.i128v(c.key.b[i]);
    d.i128v(c.key.c);
    d.level(c.level);
    d.u8v(c.arity);
  }
  return d.h.hex();
}

inline std::string digest_forest_v4(u32 K, const ForestResult& r) {
  digest_detail::Writer d;
  d.tag("mhgp4-digest-v1:forest");
  d.u32v(K);
  d.u64v((u64)r.facet_keys.size());
  for (const FacetKey& f : r.facet_keys) d.facet(f);
  d.u64v((u64)r.final_canon_fid.size());
  for (const u32 v : r.final_canon_fid) d.u32v(v);
  d.u64v((u64)r.deltas.size());
  for (const ComponentDelta& cd : r.deltas) {
    d.u64v(cd.batch);
    d.level(cd.level);
    d.facet(cd.output);
    d.u64v((u64)cd.parents.size());
    for (const FacetKey& f : cd.parents) d.facet(f);
    d.u64v((u64)cd.born.size());
    for (const FacetKey& f : cd.born) d.facet(f);
  }
  return d.h.hex();
}

// Chainage des hex par K croissant (le digest « all » du format v4).
struct DigestAll {
  digest_detail::Writer d;
  DigestAll() { d.tag("mhgp4-digest-v1:all"); }
  void add(const std::string& hex) { d.h.update(hex.data(), hex.size()); }
  std::string hex() { return d.h.hex(); }
};

}  // namespace mhgp5
