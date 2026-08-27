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
// Serialiseur TAMPONNE : les millions de champs de quelques octets sont
// accumules dans un tampon de 64 Ko avant d'alimenter SHA-256 (13 s -> ~1 s a
// uniform 8000 sur le meme flux d'octets ; le format est inchange).
struct Writer {
  Sha256 h;
  u8 buf[1 << 16];
  size_t n = 0;
  ~Writer() = default;
  void flush() {
    if (n) h.update(buf, n);
    n = 0;
  }
  void put(const void* p, size_t k) {
    if (n + k > sizeof(buf)) flush();
    std::memcpy(buf + n, p, k);
    n += k;
  }
  void tag(const char* t) { put(t, std::strlen(t)); }  // format v4 : sans longueur
  void u8v(u8 v) { put(&v, 1); }
  void u32v(u32 v) {
    u8 b[4];
    for (int i = 0; i < 4; ++i) b[i] = (u8)(v >> (8 * i));
    put(b, 4);
  }
  void u64v(u64 v) {
    u8 b[8];
    for (int i = 0; i < 8; ++i) b[i] = (u8)(v >> (8 * i));
    put(b, 8);
  }
  void i128v(i128 v) {
    const u128 u = (u128)v;
    u64v((u64)u);
    u64v((u64)(u >> 64));
  }
  void facet(const FacetKey& f) {
    u8 b[1 + 4 * kFacetMaxK];
    b[0] = f.k;
    for (int i = 0; i < kFacetMaxK; ++i)
      for (int j = 0; j < 4; ++j) b[1 + 4 * i + j] = (u8)(f.p[(size_t)i] >> (8 * j));
    put(b, sizeof(b));
  }
  void level(const ExactLevel& l) {
    for (int i = 0; i < 3; ++i) u64v(l.num[i]);
    i128v(l.den);
  }
  std::string hex() {
    flush();
    return h.hex();
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
  return d.hex();
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
  return d.hex();
}

// Chainage des hex par K croissant (le digest « all » du format v4).
struct DigestAll {
  digest_detail::Writer d;
  DigestAll() { d.tag("mhgp4-digest-v1:all"); }
  void add(const std::string& hex) { d.put(hex.data(), hex.size()); }
  std::string hex() { return d.hex(); }
};

}  // namespace mhgp5
