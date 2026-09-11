// MorseHGP3D v6 — TEMOINS de ForestResult partages par les portes (tests/ seulement).
//
// (1) `digest_callback_witness` : projection SEMANTIQUE d'un callback on_forest
//     (evenements recus + ForestResult hors workers/chronos/stockage), serialisee
//     par le Writer canonique — comparee INTRA-PROCESSUS seulement (selftest,
//     fenetre (d) de caps-refus). Passe par l'accesseur agnostique
//     `ForestResult::delta(i)` : les deux stockages donnent la meme sequence.
// (2) `first_divergence` : comparateur classique/CSR exige par les auditeurs
//     (REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902 § « Porte semantique ») —
//     lecteur TIERS (ni Writer, ni digest_callback, ni render, ni meme
//     l'accesseur ForestResult::delta : il lit les deux stockages A CRU,
//     meta/offsets/arenes d'un cote, vecteurs de ComponentDelta de l'autre —
//     un bug de l'accesseur commun ne peut pas l'aveugler), qui rend le NOM
//     GRAVE du premier champ different ("" = aucune divergence). Ordre grave :
//     1 refusal ; 2 storage_violations, storage_message ; 3 facets, fusions,
//     batches, new_attachments, attach/birth/partition_violations, nodes,
//     keys_parents, keys_born ; 4 facet_keys.size puis facet_keys[i] ;
//     5 final_canon_fid.size, final_canon_fid[i] ; 6 batch_levels.size,
//     batch_levels[i] (egalite de REPRESENTATION num[0..2], den) ; 7 delta_count ;
//     8 par i : delta[i].batch, .level, .output, .parents.size, .parents[j],
//     .born.size, .born[j]. Exclus : storage_kind, workers, t_*_ms, profile,
//     capacites, csr_capacity_growths. Cles comparees par k puis les dix cases
//     (jamais memcmp).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../src/forest/fold.hpp"
#include "../src/pipeline/digest.hpp"

namespace mhgp7 {
namespace witness {

inline std::string digest_callback_witness(u64 k, const std::vector<ForestEvent>& ev, const ForestResult& fr) {
  digest_detail::Writer d;
  d.tag("cbK");
  d.u64v(k);
  d.u64v((u64)ev.size());
  for (const ForestEvent& e : ev) {
    d.u8v(e.q);
    d.u8v(e.d);
    d.u32v((u32)e.active_mask);
    for (int i = 0; i < 11; ++i) d.u32v(e.support[i]);
    for (int i = 0; i < 9; ++i) d.u32v(e.interior[i]);
    d.level(e.level);
  }
  d.tag("fr");
  d.u64v(fr.facets);
  d.u64v(fr.fusions);
  d.u64v(fr.batches);
  d.u64v(fr.new_attachments);
  d.u64v(fr.attach_violations);
  d.u64v(fr.birth_violations);
  d.u64v(fr.partition_violations);
  d.u64v(fr.nodes);
  d.u64v(fr.storage_violations);  // projection semantique du stockage (jamais kind/offsets/capacites)
  d.u64v(fr.keys_parents);
  d.u64v(fr.keys_born);
  d.u64v((u64)fr.facet_keys.size());
  for (const FacetKey& f : fr.facet_keys) d.facet(f);
  d.u64v((u64)fr.final_canon_fid.size());
  for (const u32 c : fr.final_canon_fid) d.u32v(c);
  const size_t nd = fr.delta_count();
  d.u64v((u64)nd);
  for (size_t i = 0; i < nd; ++i) {
    const ComponentDeltaView v = fr.delta(i);
    d.u64v(v.batch);
    d.level(v.level);
    d.facet(v.output);
    d.u64v((u64)v.parents.size());
    for (const FacetKey& p : v.parents) d.facet(p);
    d.u64v((u64)v.born.size());
    for (const FacetKey& b : v.born) d.facet(b);
  }
  d.u64v((u64)fr.batch_levels.size());
  for (const ExactLevel& l : fr.batch_levels) d.level(l);
  return d.hex();
}

namespace detail {
inline bool key_same(const FacetKey& x, const FacetKey& y) {
  if (x.k != y.k) return false;
  for (int i = 0; i < kFacetMaxK; ++i)
    if (x.p[(size_t)i] != y.p[(size_t)i]) return false;
  return true;
}
inline bool level_same_repr(const ExactLevel& x, const ExactLevel& y) {
  return x.num[0] == y.num[0] && x.num[1] == y.num[1] && x.num[2] == y.num[2] && x.den == y.den;
}
inline std::string idx(const char* base, size_t i) { return std::string(base) + "[" + std::to_string(i) + "]"; }

// LECTURE BRUTE d'un delta, sans l'accesseur ForestResult::delta. Une plage
// csr incoherente (offsets hors domaine ou non monotones) donne n = SIZE_MAX :
// divergence sur `.range` sans jamais lire hors de l'arene.
struct RawDelta {
  u64 batch = 0;
  ExactLevel level{};
  FacetKey output;
  const FacetKey* parents = nullptr;
  size_t n_parents = 0;
  const FacetKey* born = nullptr;
  size_t n_born = 0;
};
inline void raw_range(const std::vector<FacetKey>& arena, const std::vector<u32>& off, size_t i, const FacetKey** p,
                      size_t* n) {
  *p = nullptr;
  *n = SIZE_MAX;
  if (i + 1 >= off.size()) return;
  const size_t b = off[i], e = off[i + 1];
  if (b > e || e > arena.size()) return;
  *n = e - b;
  *p = arena.empty() ? nullptr : arena.data() + b;
}
inline size_t raw_count(const ForestResult& r) {
  return r.storage_kind == ForestStorageKind::kCsrFacetKeysV1 ? r.delta_meta.size() : r.deltas.size();
}
inline RawDelta raw_delta(const ForestResult& r, size_t i) {
  RawDelta d;
  if (r.storage_kind == ForestStorageKind::kCsrFacetKeysV1) {
    const DeltaMeta& m = r.delta_meta[i];
    d.batch = (u64)m.batch;
    d.level = m.level;
    d.output = m.output;
    raw_range(r.parents_keys, r.parents_off, i, &d.parents, &d.n_parents);
    raw_range(r.born_keys, r.born_off, i, &d.born, &d.n_born);
  } else {
    const ComponentDelta& cd = r.deltas[i];
    d.batch = cd.batch;
    d.level = cd.level;
    d.output = cd.output;
    d.parents = cd.parents.data();
    d.n_parents = cd.parents.size();
    d.born = cd.born.data();
    d.n_born = cd.born.size();
  }
  return d;
}
}  // namespace detail

inline std::string first_divergence(const ForestResult& a, const ForestResult& b) {
  using namespace detail;
  if (a.refusal != b.refusal) return "refusal";
  if (a.storage_violations != b.storage_violations) return "storage_violations";
  if (a.storage_message != b.storage_message) return "storage_message";
  if (a.facets != b.facets) return "facets";
  if (a.fusions != b.fusions) return "fusions";
  if (a.batches != b.batches) return "batches";
  if (a.new_attachments != b.new_attachments) return "new_attachments";
  if (a.attach_violations != b.attach_violations) return "attach_violations";
  if (a.birth_violations != b.birth_violations) return "birth_violations";
  if (a.partition_violations != b.partition_violations) return "partition_violations";
  if (a.nodes != b.nodes) return "nodes";
  if (a.keys_parents != b.keys_parents) return "keys_parents";
  if (a.keys_born != b.keys_born) return "keys_born";
  if (a.facet_keys.size() != b.facet_keys.size()) return "facet_keys.size";
  for (size_t i = 0; i < a.facet_keys.size(); ++i)
    if (!key_same(a.facet_keys[i], b.facet_keys[i])) return idx("facet_keys", i);
  if (a.final_canon_fid.size() != b.final_canon_fid.size()) return "final_canon_fid.size";
  for (size_t i = 0; i < a.final_canon_fid.size(); ++i)
    if (a.final_canon_fid[i] != b.final_canon_fid[i]) return idx("final_canon_fid", i);
  if (a.batch_levels.size() != b.batch_levels.size()) return "batch_levels.size";
  for (size_t i = 0; i < a.batch_levels.size(); ++i)
    if (!level_same_repr(a.batch_levels[i], b.batch_levels[i])) return idx("batch_levels", i);
  const size_t na = raw_count(a), nb = raw_count(b);
  if (na != nb) return "delta_count";
  for (size_t i = 0; i < na; ++i) {
    const RawDelta va = raw_delta(a, i), vb = raw_delta(b, i);
    const std::string di = idx("delta", i);
    if (va.batch != vb.batch) return di + ".batch";
    if (!level_same_repr(va.level, vb.level)) return di + ".level";
    if (!key_same(va.output, vb.output)) return di + ".output";
    if (va.n_parents == SIZE_MAX || vb.n_parents == SIZE_MAX) return di + ".parents.range";
    if (va.n_parents != vb.n_parents) return di + ".parents.size";
    for (size_t j = 0; j < va.n_parents; ++j)
      if (!key_same(va.parents[j], vb.parents[j])) return di + "." + idx("parents", j);
    if (va.n_born == SIZE_MAX || vb.n_born == SIZE_MAX) return di + ".born.range";
    if (va.n_born != vb.n_born) return di + ".born.size";
    for (size_t j = 0; j < va.n_born; ++j)
      if (!key_same(va.born[j], vb.born[j])) return di + "." + idx("born", j);
  }
  return std::string();
}

}  // namespace witness
}  // namespace mhgp7

