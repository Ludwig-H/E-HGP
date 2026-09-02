// MorseHGP3D v6 — PORTE DU CSR DE FacetKey (palier KeyCSR, GO exploratoire
// REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902 ; plan csr_6, fixtures csr_5).
//
// Le stockage `csr_facet_keys_v1` doit produire EXACTEMENT le meme objet que
// le stockage classique (vecteurs de ComponentDelta). Preuves exercees :
//   --fixtures     : 13 fixtures gravees aux coordonnees exactes (born-only,
//                    parents-only, continuation, multi-parents S5, plusieurs
//                    racines par lot S2, foret vide, S1 inter-segments et ses
//                    variantes de representation, R2/R2b K=2 reel) ; le bras
//                    CLASSIQUE est compare au texte grave, aux compteurs graves
//                    et aux pins de digest (un ecart = derive du classique, code
//                    1, jamais un regravage) ; le bras CSR est compare au
//                    classique par `first_divergence` (lecteur TIERS), par le
//                    rendu, par le digest, et REJOUE (catalogue + deltas ->
//                    partition, seconde autorite) sur les deux bras ; 1 et 4
//                    fils. Sous --inject=<csr-*> : code 4 SEULEMENT si la
//                    premiere divergence sur la fixture annoncee porte le champ
//                    annonce (--expect-divergence=), sinon 1.
//   --offsets      : scenes unitaires du validateur d'offsets (message exact).
//   --overflow     : gardes de capacite (plafond d'append abaisse, majorant
//                    abaisse), refus AVANT toute vue, pipeline refuse sans
//                    callback ; mutant csr-guard-skip = refus attendu ABSENT ;
//                    mutant csr-inject-bad-alloc = bad_alloc d'arene capture
//                    dans le fold (resource_exhausted, payload vide, zero
//                    callback), jamais une exception hors du fold.
//   --copy-alias   : ForestResult copie/deplace autonome (arene possedee),
//                    copie prise dans on_forest relue APRES run_pipeline.
//   --pipeline     : matrice fils {1,T} x inflight {1,2} x join {0,1} x layout
//                    sur les familles de conformite (petites tailles), temoin
//                    complet par K contre la reference classique, rejeu csr.
//   --pipeline-refus --inject=csr-offset-* : invariant_violated, zero callback,
//                    provisoires vides (sans --inject : refus 2).
// Planchers (code 3) : --min-fixtures, --min-deltas, --min-continuations
// (alias --min-continuation-lots), --min-multi-root-lots, --min-vides,
// --min-refus (refus amont sous csr : kind csr, payload vide), --min-orders,
// --min-keys. Codes : 0 conforme ; 1 desaccord (y compris une
// divergence sur un champ non annonce) ; 2 refus ; 3 plancher ; 4 mutant tue.
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/run.hpp"
#include "forest_witness.hpp"

#include <utility>

// DENT DE COMPILATION (retour auditeur, couture de duree de vie) : delta(i) ET
// for_each_delta sont ref-qualifies const& ; l'appel sur un ForestResult
// TEMPORAIRE est ill-formed (surcharges const&& supprimees) — une vue prise sur
// le temporaire pendrait. Concepts DEPENDANTS : la substitution echoue au lieu
// d'etre une erreur dure, et l'assertion negative tue toute regression.
template <typename R>
concept DeltaOnRef = requires(R&& r) { std::forward<R>(r).delta(size_t{0}); };
template <typename R>
concept ForEachDeltaOnRef = requires(R&& r) {
  std::forward<R>(r).for_each_delta([](const mhgp6::ComponentDeltaView&) {});
};
static_assert(DeltaOnRef<const mhgp6::ForestResult&>, "delta(i) doit rester appelable sur lvalue");
static_assert(!DeltaOnRef<mhgp6::ForestResult>, "delta(i) sur temporaire doit etre refuse");
static_assert(ForEachDeltaOnRef<const mhgp6::ForestResult&>, "for_each_delta doit rester appelable sur lvalue");
static_assert(!ForEachDeltaOnRef<mhgp6::ForestResult>, "for_each_delta sur temporaire doit etre refuse");

using namespace mhgp6;

namespace {

u64 g_bad = 0;

void bad(const std::string& what) {
  ++g_bad;
  std::printf("DESACCORD : %s\n", what.c_str());
}

// ---- Helpers de fixtures (portes de morsehgp3D_v5/tests/fold_fixtures_gate.cpp, pin bc66ade7).
ExactLevel lvl(u64 n, u64 d = 1) { return ExactLevel{{n, 0, 0}, (i128)d}; }

// pair(a, b, mask, num/den) : K = 1, q = 2, d = 0 ; bit t qualifie σ∖{support[t]}.
ForestEvent pair(PointId a, PointId b, u16 mask, u64 num, u64 den = 1) {
  ForestEvent e;
  e.q = 2;
  e.d = 0;
  e.active_mask = mask;
  e.support[0] = a;
  e.support[1] = b;
  e.level = lvl(num, den);
  return e;
}

ForestEvent ev2(std::vector<PointId> sup, std::vector<PointId> in, u16 mask, u64 num, u64 den = 1) {
  ForestEvent e;
  e.q = (u8)sup.size();
  e.d = (u8)in.size();
  e.active_mask = mask;
  for (size_t i = 0; i < sup.size(); ++i) e.support[i] = sup[i];
  for (size_t i = 0; i < in.size(); ++i) e.interior[i] = in[i];
  e.level = lvl(num, den);
  return e;
}

std::string key_str(const FacetKey& f) {
  std::string s = "{";
  for (u8 i = 0; i < f.k; ++i) {
    if (i) s += ",";
    s += std::to_string((unsigned long long)f.p[i]);
  }
  return s + "}";
}

std::string level_str(const ExactLevel& l) {
  return std::to_string((unsigned long long)l.num[0]) + "/" + std::to_string((unsigned long long)(u64)l.den);
}

// Rendu texte par l'ACCESSEUR (les deux stockages) : `b=<batch> out={..} par=[..] born=[..]\n`.
std::string render(const ForestResult& r) {
  std::string s;
  r.for_each_delta([&](const ComponentDeltaView& d) {
    s += "b=" + std::to_string((unsigned long long)d.batch) + " out=" + key_str(d.output) + " par=[";
    for (size_t i = 0; i < d.parents.size(); ++i) s += (i ? "," : "") + key_str(d.parents[i]);
    s += "] born=[";
    for (size_t i = 0; i < d.born.size(); ++i) s += (i ? "," : "") + key_str(d.born[i]);
    s += "]\n";
  });
  return s;
}

std::string counters(const ForestResult& r) {
  std::string s = "facets=" + std::to_string((unsigned long long)r.facets) +
                  " fusions=" + std::to_string((unsigned long long)r.fusions) +
                  " batches=" + std::to_string((unsigned long long)r.batches) +
                  " new_att=" + std::to_string((unsigned long long)r.new_attachments) +
                  " nodes=" + std::to_string((unsigned long long)r.nodes) + " viol=" +
                  std::to_string((unsigned long long)r.attach_violations) + "/" +
                  std::to_string((unsigned long long)r.birth_violations) + "/" +
                  std::to_string((unsigned long long)r.partition_violations) + "/" +
                  std::to_string((unsigned long long)r.storage_violations) + " canon=[";
  for (size_t i = 0; i < r.final_canon_fid.size(); ++i)
    s += (i ? "," : "") + std::to_string((unsigned long long)r.final_canon_fid[i]);
  s += "] levels=[";
  for (size_t i = 0; i < r.batch_levels.size(); ++i) s += (i ? "," : "") + level_str(r.batch_levels[i]);
  return s + "]";
}

// ---- REJEU INDEPENDANT « catalogue + deltas -> partition » (porte de
// morsehgp3D_v5/tests/delta_replay_gate.cpp, pin bc66ade7 ; lu par la vue).
// UF frais indexe par facet_keys (strictement croissant verifie), fid_of par
// lower_bound ; par delta : output == min(parents ∪ nes) ; chaque nee une
// seule fois et jamais deja portee ; chaque parent = canonique d'un bloc
// vivant ou singleton implicite (jamais ne, jamais uni) ; apres le delta les
// parents ne sont plus vivants, l'output l'est ; partition reconstruite ==
// final_canon_fid fid par fid. Retourne le nombre de desaccords.
struct ReplayUF {
  std::vector<u32> parent, canon;
  explicit ReplayUF(size_t n) : parent(n), canon(n) {
    for (size_t i = 0; i < n; ++i) { parent[i] = (u32)i; canon[i] = (u32)i; }
  }
  u32 find(u32 v) {
    while (parent[v] != v) { parent[v] = parent[parent[v]]; v = parent[v]; }
    return v;
  }
  void unite(u32 a, u32 b) {
    a = find(a);
    b = find(b);
    if (a == b) return;
    parent[b] = a;
    canon[a] = std::min(canon[a], canon[b]);
  }
};

u32 fid_of(const std::vector<FacetKey>& keys, const FacetKey& k) {
  const auto it = std::lower_bound(keys.begin(), keys.end(), k);
  if (it == keys.end() || !(*it == k)) return UINT32_MAX;
  return (u32)(it - keys.begin());
}

u64 replay(const ForestResult& r) {
  const std::vector<FacetKey>& keys = r.facet_keys;
  const size_t nf = keys.size();
  for (size_t i = 1; i < nf; ++i)
    if (!(keys[i - 1] < keys[i])) return 1;
  ReplayUF uf(nf);
  std::vector<u8> born_count(nf, 0), alive_root(nf, 0);
  u64 badc = 0;
  r.for_each_delta([&](const ComponentDeltaView& d) {
    FacetKey mn;
    bool has = false;
    for (const FacetKey& p : d.parents) if (!has || p < mn) { mn = p; has = true; }
    for (const FacetKey& b : d.born) if (!has || b < mn) { mn = b; has = true; }
    if (!has || !(mn == d.output)) { ++badc; return; }
    u32 first = UINT32_MAX;
    for (const FacetKey& b : d.born) {
      const u32 f = fid_of(keys, b);
      if (f == UINT32_MAX) { ++badc; continue; }
      if (born_count[f]++ != 0) ++badc;  // nee deux fois
      if (alive_root[f]) ++badc;         // nee alors qu'un bloc la porte deja
      if (first == UINT32_MAX) first = f; else uf.unite(first, f);
    }
    for (const FacetKey& p : d.parents) {
      const u32 f = fid_of(keys, p);
      if (f == UINT32_MAX) { ++badc; continue; }
      const u32 rt = uf.find(f);
      const bool live_block = alive_root[f] && uf.canon[rt] == f;
      const bool implicit_singleton = !alive_root[f] && born_count[f] == 0 && rt == f && uf.canon[rt] == f;
      if (!live_block && !implicit_singleton) ++badc;
      if (first == UINT32_MAX) first = f; else uf.unite(first, f);
    }
    for (const FacetKey& p : d.parents) {
      const u32 f = fid_of(keys, p);
      if (f != UINT32_MAX) alive_root[f] = 0;
    }
    const u32 fo = fid_of(keys, d.output);
    if (fo == UINT32_MAX || uf.canon[uf.find(fo)] != fo) ++badc;
    else alive_root[fo] = 1;
  });
  if (r.final_canon_fid.size() != nf) return badc + 1;
  u64 diff = 0;
  for (size_t f = 0; f < nf; ++f)
    if (uf.canon[uf.find((u32)f)] != r.final_canon_fid[f]) ++diff;
  return badc + diff;
}

// ---- FIXTURES GRAVEES (csr_5 : re-derivees machine au HEAD 98e406ce ; les
// digests sont des pins du bras CLASSIQUE).
struct Fixture {
  const char* name;
  u32 K;
  std::vector<ForestEvent> events;
  const char* render_expected;
  const char* counters_expected;
  const char* digest_pin;
};

std::vector<Fixture> fixtures() {
  std::vector<Fixture> fx;
  // F1 born-only.
  fx.push_back({"F1", 1, {pair(1, 2, 0b00, 1)},
                "b=0 out={1} par=[] born=[{1},{2}]\n",
                "facets=2 fusions=1 batches=1 new_att=2 nodes=0 viol=0/0/0/0 canon=[0,0] levels=[1/1]",
                "c59f73653b33ad998ad1932b9444c6810d0075d27e172e13813fb4752b9ba8ba"});
  // F2 parents-only (deux parents actifs, aucun ne).
  fx.push_back({"F2", 1, {pair(1, 2, 0, 1), pair(3, 4, 0, 1), pair(1, 3, 0b11, 2)},
                "b=0 out={1} par=[] born=[{1},{2}]\n"
                "b=0 out={3} par=[] born=[{3},{4}]\n"
                "b=1 out={1} par=[{1},{3}] born=[]\n",
                "facets=4 fusions=3 batches=2 new_att=4 nodes=1 viol=0/0/0/0 canon=[0,0,0,0] levels=[1/1,2/1]",
                "198807971f850169de51fa3d99547660c9f29313bf29e66cae3497a7cf447668"});
  // F2b : support inverse a E2, sortie identique.
  fx.push_back({"F2b", 1, {pair(1, 2, 0, 1), pair(3, 4, 0, 1), pair(3, 1, 0b11, 2)},
                "b=0 out={1} par=[] born=[{1},{2}]\n"
                "b=0 out={3} par=[] born=[{3},{4}]\n"
                "b=1 out={1} par=[{1},{3}] born=[]\n",
                "facets=4 fusions=3 batches=2 new_att=4 nodes=1 viol=0/0/0/0 canon=[0,0,0,0] levels=[1/1,2/1]",
                "198807971f850169de51fa3d99547660c9f29313bf29e66cae3497a7cf447668"});
  // F3 continuation : un seul delta, batch_levels de taille 2.
  fx.push_back({"F3", 1, {pair(1, 2, 0, 1), pair(2, 3, 0, 1), pair(1, 3, 0b11, 2)},
                "b=0 out={1} par=[] born=[{1},{2},{3}]\n",
                "facets=3 fusions=2 batches=2 new_att=3 nodes=0 viol=0/0/0/0 canon=[0,0,0] levels=[1/1,2/1]",
                "ce352cec492200470b64034fda452bd3a2852f5b33f80407cb64faee233e6e40"});
  // F4 multi-parents S5 : ordre de pre_list != ordre des canons.
  fx.push_back({"F4", 1,
                {pair(1, 9, 0, 1), pair(3, 4, 0, 1), pair(5, 6, 0, 1), pair(1, 3, 0b11, 2), pair(3, 5, 0b11, 2)},
                "b=0 out={3} par=[] born=[{3},{4}]\n"
                "b=0 out={5} par=[] born=[{5},{6}]\n"
                "b=0 out={1} par=[] born=[{1},{9}]\n"
                "b=1 out={1} par=[{1},{3},{5}] born=[]\n",
                "facets=6 fusions=5 batches=2 new_att=6 nodes=1 viol=0/0/0/0 canon=[0,0,0,0,0,0] levels=[1/1,2/1]",
                "628b7f559414186ceaab29ce7ac379dc909cf12c5bd4c7302629700c946c8c57"});
  // F4-min : deux parents pousses [{3},{1}] avant tri.
  fx.push_back({"F4min", 1, {pair(1, 9, 0, 1), pair(3, 4, 0, 1), pair(1, 3, 0b11, 2)},
                "b=0 out={3} par=[] born=[{3},{4}]\n"
                "b=0 out={1} par=[] born=[{1},{9}]\n"
                "b=1 out={1} par=[{1},{3}] born=[]\n",
                "facets=4 fusions=3 batches=2 new_att=4 nodes=1 viol=0/0/0/0 canon=[0,0,0,0] levels=[1/1,2/1]",
                "8218e851a1e6d18ba275a6a4262a4d65d2eb9269a65d939c5e286a9dd223f8b9"});
  // F5 S2 : deux racines post dans un lot, ordre par racine UF != ordre par output.
  fx.push_back({"F5", 1, {pair(1, 9, 0, 1), pair(3, 4, 0, 1)},
                "b=0 out={3} par=[] born=[{3},{4}]\n"
                "b=0 out={1} par=[] born=[{1},{9}]\n",
                "facets=4 fusions=2 batches=1 new_att=4 nodes=0 viol=0/0/0/0 canon=[0,1,1,0] levels=[1/1]",
                "d2f620c7a11bf70ab16febaf92bf60549182eeda6d2113beef309608467f0bf8"});
  // F6 foret vide.
  fx.push_back({"F6", 1, {}, "",
                "facets=0 fusions=0 batches=0 new_att=0 nodes=0 viol=0/0/0/0 canon=[] levels=[]",
                "726fbd697d80a1917b9b85c6f772969429bbb12f85f88b5df64ce1b2f4306319"});
  // S1 inter-segments : output {5} FIGE au lot 0, canon final {1}.
  fx.push_back({"S1", 1, {pair(5, 6, 0b00, 1), pair(5, 1, 0b10, 2), pair(6, 2, 0b10, 2)},
                "b=0 out={5} par=[] born=[{5},{6}]\n"
                "b=1 out={1} par=[{5}] born=[{1},{2}]\n",
                "facets=4 fusions=3 batches=2 new_att=4 nodes=0 viol=0/0/0/0 canon=[0,0,0,0] levels=[1/1,2/1]",
                "080e5dd4e825293d6d7fe5f6aec78380a89cb7bd691e744cd226f3f8a43bd3ce"});
  // S1a : E2 a 4/2 (meme lot par same_exact_level), sortie identique a S1.
  fx.push_back({"S1a", 1, {pair(5, 6, 0b00, 1), pair(5, 1, 0b10, 2), pair(6, 2, 0b10, 4, 2)},
                "b=0 out={5} par=[] born=[{5},{6}]\n"
                "b=1 out={1} par=[{5}] born=[{1},{2}]\n",
                "facets=4 fusions=3 batches=2 new_att=4 nodes=0 viol=0/0/0/0 canon=[0,0,0,0] levels=[1/1,2/1]",
                "080e5dd4e825293d6d7fe5f6aec78380a89cb7bd691e744cd226f3f8a43bd3ce"});
  // S1b : E2(4/2) AVANT E1(2/1) : meme texte, level = REPRESENTATION du premier evenement du lot.
  fx.push_back({"S1b", 1, {pair(5, 6, 0b00, 1), pair(6, 2, 0b10, 4, 2), pair(5, 1, 0b10, 2)},
                "b=0 out={5} par=[] born=[{5},{6}]\n"
                "b=1 out={1} par=[{5}] born=[{1},{2}]\n",
                "facets=4 fusions=3 batches=2 new_att=4 nodes=0 viol=0/0/0/0 canon=[0,0,0,0] levels=[1/1,4/2]",
                "e46018388ee0074ff539c8b7f224babf083a1c711363caff7ff2a2f5c8ef3b2b"});
  // R2 : encodage REEL K=2 (regime regulier, mask = 2^q - 1).
  fx.push_back({"R2", 2, {ev2({1, 2}, {3}, 0b11, 1)},
                "b=0 out={1,2} par=[{1,3},{2,3}] born=[{1,2}]\n",
                "facets=3 fusions=2 batches=1 new_att=1 nodes=1 viol=0/0/0/0 canon=[0,0,0] levels=[1/1]",
                "fa7a3335400c320fe72d6d27d22b8235cddf7dbdae5d0c805469119567a5f8b1"});
  fx.push_back({"R2b", 2, {ev2({1, 2}, {3}, 0b11, 1), ev2({2, 4}, {3}, 0b11, 2)},
                "b=0 out={1,2} par=[{1,3},{2,3}] born=[{1,2}]\n"
                "b=1 out={1,2} par=[{1,2},{3,4}] born=[{2,4}]\n",
                "facets=5 fusions=4 batches=2 new_att=2 nodes=2 viol=0/0/0/0 canon=[0,0,0,0,0] levels=[1/1,2/1]",
                "03bd088e268199ca9303f4ca82d428e2ddc3253cd908f2173b06b3cf780ee47a"});
  return fx;
}

// Table mutant -> fixture annoncee -> champ annonce (first_divergence) -> sous-chaine exigee du message.
struct MutantSpec {
  const char* mutant;
  const char* fixture;
  const char* field;
  const char* message;  // "" = aucune exigence de message
};
inline constexpr MutantSpec kMutantTable[] = {
    {"csr-order-by-output", "F5", "delta[0].output", ""},
    {"csr-keep-continuation", "F3", "delta_count", ""},
    {"csr-stale-level", "F2", "delta[2].level", ""},
    {"csr-stale-output", "S1", "delta[0].output", ""},
    {"csr-unsorted-born", "F1", "delta[0].born[0]", ""},
    {"csr-unsorted-parents", "F2", "delta[2].parents[0]", ""},
    {"csr-drop-delta", "F2", "delta_count", ""},
    {"csr-dup-delta", "F2", "delta_count", ""},
    {"csr-shift-offset", "F2", "delta[0].born.size", ""},
    {"csr-offset-hole", "F2", "storage_violations", "premier offset non nul"},
    {"csr-offset-overlap", "F2", "storage_violations", "non monotones (chevauchement)"},
    {"csr-offset-end", "F2", "storage_violations", "dernier offset != taille d'arene"},
    {"csr-offset-domain", "F2", "storage_violations", "hors domaine"},
    {"csr-guard-skip", "F4", "storage_message", ""},               // mode --overflow seulement
    {"csr-inject-bad-alloc", "F4", "storage_message", "allocation"},  // mode --overflow seulement
};

bool overflow_only(const std::string& name) { return name == "csr-guard-skip" || name == "csr-inject-bad-alloc"; }

const MutantSpec* mutant_spec(const std::string& name) {
  for (const MutantSpec& m : kMutantTable)
    if (name == m.mutant) return &m;
  return nullptr;
}

bool csr_payload_empty(const ForestResult& r) {
  return r.delta_meta.empty() && r.parents_off.empty() && r.born_off.empty() && r.parents_keys.empty() &&
         r.born_keys.empty();
}

// Verifications STRUCTURELLES d'un ForestResult csr sain (offsets, arenes, compteurs).
void check_csr_structure(const ForestResult& c, const char* name) {
  const std::string n = name;
  if (c.storage_kind != ForestStorageKind::kCsrFacetKeysV1) bad(n + " : storage_kind != csr");
  if (c.storage_violations != 0) bad(n + " : storage_violations != 0");
  if (!c.storage_message.empty()) bad(n + " : storage_message non vide : " + c.storage_message);
  if (!c.deltas.empty()) bad(n + " : deltas classiques non vides sous csr");
  if (c.parents_off.size() != c.delta_meta.size() + 1 || c.born_off.size() != c.delta_meta.size() + 1)
    bad(n + " : nombre d'offsets");
  else {
    if (c.parents_off[0] != 0 || c.born_off[0] != 0) bad(n + " : premier offset non nul");
    if ((size_t)c.parents_off.back() != c.parents_keys.size() || (size_t)c.born_off.back() != c.born_keys.size())
      bad(n + " : dernier offset != taille d'arene");
  }
  if (c.parents_keys.size() != c.keys_parents || c.born_keys.size() != c.keys_born)
    bad(n + " : arenes != compteurs keys_parents/keys_born");
}

struct Floors {
  u64 fixtures = 0, deltas = 0, continuations = 0, multi_root = 0, vides = 0, refus = 0, orders = 0, keys = 0;
};

// Refus AMONT sous csr (validate_fold_events, ici deux identifiants egaux) :
// meme refus que le classique, kind csr SIGNE, payload vide — jamais un
// repli fantome pour un appelant qui compterait « kind construit != route
// demandee » sur une entree refusee.
void check_refus_csr(Floors* got) {
  const std::vector<ForestEvent> ev = {pair(1, 1, 0, 1)};
  const ForestResult c = build_forest(ev, 1, ForestLayout::kClassic);
  const ForestResult s = build_forest(ev, 1, ForestLayout::kCsr);
  if (c.refusal.empty() || s.refusal.empty()) {
    bad("refus amont : scene non refusee (vacue)");
    return;
  }
  ++got->refus;
  const std::string d = witness::first_divergence(c, s);
  if (!d.empty()) bad("refus amont : classique != csr sur " + d);
  if (c.storage_kind != ForestStorageKind::kVectorComponentDeltaV1) bad("refus amont classique : storage_kind != classique");
  if (s.storage_kind != ForestStorageKind::kCsrFacetKeysV1) bad("refus amont sous csr : storage_kind != csr (repli fantome)");
  if (s.delta_count() != 0 || !csr_payload_empty(s) || s.storage_violations != 0 || !s.storage_message.empty())
    bad("refus amont sous csr : payload non vide ou message parasite");
}

// ---- --fixtures.
int run_fixtures(const std::string& inject, const std::string& expect_field, const Floors& min) {
  const std::vector<Fixture> fx = fixtures();
  Floors got;
  const MutantSpec* spec = inject.empty() ? nullptr : mutant_spec(inject);
  if (!inject.empty() && (!spec || overflow_only(inject))) {
    std::printf("REFUS : mutant %s hors cible du mode --fixtures\n", inject.c_str());
    return 2;
  }
  if (inject.empty()) check_refus_csr(&got);
  std::string observed_on_announced;
  bool announced_seen = false;
  const ForestResult* announced_csr = nullptr;
  ForestResult announced_csr_copy;
  for (const Fixture& f : fx) {
    ++got.fixtures;
    const ForestResult c1 = build_forest(f.events, 1, ForestLayout::kClassic);
    const ForestResult c4 = build_forest(f.events, 4, ForestLayout::kClassic);
    const std::string n = f.name;
    // (a) Bras CLASSIQUE contre le texte grave, les compteurs graves, le pin.
    if (render(c1) != f.render_expected)
      bad(n + " : rendu classique != fixture gravee\n--- obtenu ---\n" + render(c1) + "--- attendu ---\n" + f.render_expected);
    if (counters(c1) != f.counters_expected)
      bad(n + " : compteurs classiques != graves\n--- obtenu ---\n" + counters(c1) + "\n--- attendu ---\n" + f.counters_expected);
    const std::string dg1 = digest_forest_v4(f.K, c1);
    if (dg1 != f.digest_pin) bad(n + " : digest classique != pin (derive du classique) : " + dg1);
    if (!c1.refusal.empty()) bad(n + " : refus classique : " + c1.refusal);
    if (c1.storage_kind != ForestStorageKind::kVectorComponentDeltaV1 || !csr_payload_empty(c1) || c1.storage_violations ||
        !c1.storage_message.empty())
      bad(n + " : stockage classique pollue");
    {
      const std::string d = witness::first_divergence(c1, c4);
      if (!d.empty()) bad(n + " : classique 1 fil != 4 fils sur " + d);
    }
    if (c1.keys_parents + c1.keys_born != [&] { u64 s = 0; c1.for_each_delta([&](const ComponentDeltaView& v) { s += v.parents.size() + v.born.size(); }); return s; }())
      bad(n + " : keys_parents+keys_born != cles des deltas emis (classique)");
    got.deltas += c1.delta_count();
    {
      std::vector<u64> batches_seen;
      std::vector<u64> per_batch;
      c1.for_each_delta([&](const ComponentDeltaView& v) {
        if (std::find(batches_seen.begin(), batches_seen.end(), v.batch) == batches_seen.end()) {
          batches_seen.push_back(v.batch);
          per_batch.push_back(1);
        } else {
          ++per_batch[(size_t)(std::find(batches_seen.begin(), batches_seen.end(), v.batch) - batches_seen.begin())];
        }
      });
      got.continuations += c1.batches - batches_seen.size();
      for (const u64 k : per_batch) if (k >= 2) ++got.multi_root;
      if (c1.facets == 0 && c1.batches == 0) ++got.vides;
    }
    // (c) Rejeu sur le bras classique.
    if (replay(c1) != 0) bad(n + " : rejeu catalogue+deltas != partition (classique)");
    // (b) Bras CSR.
    const ForestResult s1 = build_forest(f.events, 1, ForestLayout::kCsr);
    if (inject.empty()) {
      const ForestResult s4 = build_forest(f.events, 4, ForestLayout::kCsr);
      check_csr_structure(s1, f.name);
      check_csr_structure(s4, f.name);
      const std::string d = witness::first_divergence(c1, s1);
      if (!d.empty()) bad(n + " : classique != csr sur " + d);
      const std::string d4 = witness::first_divergence(s1, s4);
      if (!d4.empty()) bad(n + " : csr 1 fil != 4 fils sur " + d4);
      if (render(s1) != render(c1)) bad(n + " : rendu csr != rendu classique");
      if (digest_forest_v4(f.K, s1) != dg1) bad(n + " : digest csr != digest classique");
      if (s1.batch_levels != c1.batch_levels || s1.batches != c1.batches) bad(n + " : batch_levels/batches csr != classique");
      if (replay(s1) != 0) bad(n + " : rejeu catalogue+deltas != partition (csr)");
      if (c1.facets == 0 && (s1.parents_off != std::vector<u32>{0} || s1.born_off != std::vector<u32>{0}))
        bad(n + " : foret vide csr : offsets != {0}");
    } else if (n == spec->fixture) {
      announced_seen = true;
      observed_on_announced = witness::first_divergence(c1, s1);
      announced_csr_copy = s1;
      announced_csr = &announced_csr_copy;
    }
  }
  std::printf("fold_csr_gate --fixtures fixtures=%llu deltas=%llu continuations=%llu multi_root=%llu vides=%llu refus=%llu desaccords=%llu\n",
              (unsigned long long)got.fixtures, (unsigned long long)got.deltas, (unsigned long long)got.continuations,
              (unsigned long long)got.multi_root, (unsigned long long)got.vides, (unsigned long long)got.refus,
              (unsigned long long)g_bad);
  if (got.fixtures < min.fixtures || got.deltas < min.deltas || got.continuations < min.continuations ||
      got.multi_root < min.multi_root || got.vides < min.vides || (inject.empty() && got.refus < min.refus)) {
    std::printf("PLANCHER\n");
    return 3;
  }
  if (inject.empty()) return g_bad ? 1 : 0;
  // Verdict sous mutant : le bras classique doit etre reste intact (sinon le
  // site a fui hors de la branche csr : 1), et la premiere divergence sur la
  // fixture annoncee doit porter EXACTEMENT le champ annonce.
  if (g_bad) {
    std::printf("mutant %s : le bras CLASSIQUE a bouge (site partage) — code 1\n", inject.c_str());
    return 1;
  }
  const std::string expected = expect_field.empty() ? std::string(spec->field) : expect_field;
  if (expected != spec->field) {
    std::printf("mutant %s : champ annonce %s != table %s — code 1\n", inject.c_str(), expected.c_str(), spec->field);
    return 1;
  }
  if (!announced_seen) {
    std::printf("mutant %s : fixture annoncee %s absente — code 1\n", inject.c_str(), spec->fixture);
    return 1;
  }
  std::printf("mutant %s sur %s : premiere divergence = '%s' (annonce '%s')\n", inject.c_str(), spec->fixture,
              observed_on_announced.c_str(), expected.c_str());
  if (observed_on_announced != expected) return 1;
  if (std::strlen(spec->message) != 0) {
    // Mutants d'offset : refus AVANT vue — violation unique, payload vide, message grave.
    const ForestResult& s = *announced_csr;
    if (s.storage_violations != 1 || s.delta_count() != 0 || !csr_payload_empty(s) ||
        s.storage_message.find(spec->message) == std::string::npos) {
      std::printf("mutant %s : refus avant vue non conforme (violations=%llu deltas=%zu vide=%d message='%s')\n",
                  inject.c_str(), (unsigned long long)s.storage_violations, s.delta_count(), csr_payload_empty(s) ? 1 : 0,
                  s.storage_message.c_str());
      return 1;
    }
  }
  return 4;
}

// ---- --offsets : scenes unitaires du validateur (message exact exige).
int run_offsets() {
  struct Scene {
    std::vector<u32> off;
    size_t n_meta, arena;
    const char* expect;  // nullptr = accepte
  };
  const Scene scenes[] = {
      {{0, 5}, 2, 5, "nombre d'offsets != deltas + 1"},
      {{1, 2, 5}, 2, 5, "premier offset non nul (trou en tete d'arene)"},
      {{0, 9, 5}, 2, 5, "offset hors domaine (> taille d'arene)"},
      {{0, 3, 2, 5}, 3, 5, "offsets non monotones (chevauchement)"},
      {{0, 2, 4}, 2, 5, "dernier offset != taille d'arene (fin inexacte, queue orpheline)"},
      {{0, 2, 2, 5}, 3, 5, nullptr},
      {{0}, 0, 0, nullptr},
  };
  u64 scenes_run = 0;
  for (const Scene& s : scenes) {
    ++scenes_run;
    std::string why;
    const bool ok = fold_detail::csr_offsets_ok(s.off, s.n_meta, s.arena, "scene", &why);
    if (s.expect == nullptr) {
      if (!ok) bad("scene d'offsets acceptable refusee : " + why);
    } else {
      const std::string expected = std::string("invariant stockage csr : scene : ") + s.expect;
      if (ok) bad(std::string("scene d'offsets refusable acceptee (") + s.expect + ")");
      else if (why != expected) bad("message inexact : '" + why + "' attendu '" + expected + "'");
    }
  }
  // F2 csr sain : les deux arenes acceptees.
  const std::vector<Fixture> fx = fixtures();
  for (const Fixture& f : fx) {
    if (std::strcmp(f.name, "F2") != 0) continue;
    const ForestResult s = build_forest(f.events, 1, ForestLayout::kCsr);
    std::string why;
    check_csr_structure(s, "F2");
    if (!fold_detail::csr_offsets_ok(s.parents_off, s.delta_meta.size(), s.parents_keys.size(), "parents", &why) ||
        !fold_detail::csr_offsets_ok(s.born_off, s.delta_meta.size(), s.born_keys.size(), "nes", &why))
      bad("F2 csr sain refuse par le validateur : " + why);
    if (s.born_off != std::vector<u32>{0, 2, 4, 4} || s.parents_off != std::vector<u32>{0, 0, 0, 2})
      bad("F2 csr : offsets != graves ({0,2,4,4} nes, {0,0,0,2} parents)");
  }
  std::printf("fold_csr_gate --offsets scenes=%llu desaccords=%llu\n", (unsigned long long)scenes_run, (unsigned long long)g_bad);
  if (scenes_run < 7) return 3;
  return g_bad ? 1 : 0;
}

const Fixture* find_fixture(const std::vector<Fixture>& fx, const char* name) {
  for (const Fixture& f : fx)
    if (std::strcmp(f.name, name) == 0) return &f;
  return nullptr;
}

// ---- --overflow : gardes de capacite (plafond d'append, majorant), refus avant vue, pipeline,
// panne d'allocation injectee (csr-inject-bad-alloc). Sous --inject, le champ annonce
// (--expect-divergence=) est juge contre la table comme en --fixtures (sinon 1).
int run_overflow(const std::string& inject, const std::string& expect_field, int threads) {
  const MutantSpec* spec = inject.empty() ? nullptr : mutant_spec(inject);
  if (!inject.empty() && (!spec || !overflow_only(inject))) {
    std::printf("REFUS : mutant %s hors cible du mode --overflow\n", inject.c_str());
    return 2;
  }
  if (spec) {
    const std::string expected = expect_field.empty() ? std::string(spec->field) : expect_field;
    if (expected != spec->field) {
      std::printf("mutant %s : champ annonce %s != table %s — code 1\n", inject.c_str(), expected.c_str(), spec->field);
      return 1;
    }
  }
  const std::vector<Fixture> fx = fixtures();
  const Fixture* f4 = find_fixture(fx, "F4");
  const Fixture* f5 = find_fixture(fx, "F5");
  if (!f4 || !f5) return 2;
  if (spec && std::strcmp(spec->fixture, f4->name) != 0) {
    std::printf("mutant %s : fixture annoncee %s != F4 — code 1\n", inject.c_str(), spec->fixture);
    return 1;
  }
  const ForestResult c4 = build_forest(f4->events, 1, ForestLayout::kClassic);
  // Le bras CLASSIQUE reste ancre au texte et au pin sous tout mutant (site exclusif).
  if (render(c4) != f4->render_expected || digest_forest_v4(f4->K, c4) != f4->digest_pin) {
    std::printf("F4 : le bras CLASSIQUE a bouge (site partage ou derive) — code 1\n");
    return 1;
  }
  if (inject == "csr-inject-bad-alloc") {
    // Scene I : F4 csr sous plafonds nominaux, bad_alloc injecte au 4e delta APRES
    // l'append des parents (queue orpheline de 3 parents) : refus
    // resource_exhausted porte par storage_message, zero violation, payload vide.
    const ForestResult s4 = build_forest(f4->events, 1, ForestLayout::kCsr);
    const std::string d = witness::first_divergence(c4, s4);
    std::printf("mutant %s sur %s : premiere divergence = '%s' (annonce '%s')\n", inject.c_str(), spec->fixture, d.c_str(),
                spec->field);
    if (d != spec->field) return 1;
    if (s4.storage_message.find(spec->message) == std::string::npos || s4.storage_violations != 0 ||
        s4.delta_count() != 0 || !csr_payload_empty(s4) || s4.storage_kind != ForestStorageKind::kCsrFacetKeysV1) {
      std::printf("mutant %s : refus non conforme (violations=%llu deltas=%zu vide=%d message='%s') — code 1\n",
                  inject.c_str(), (unsigned long long)s4.storage_violations, s4.delta_count(), csr_payload_empty(s4) ? 1 : 0,
                  s4.storage_message.c_str());
      return 1;
    }
    // Scene I' : pipeline uniform 400 csr -> resource_exhausted, zero callback, provisoires vides.
    const std::vector<InputPoint> in =
        make_family_input(CloudFamily::kUniform, 400, cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
    RunOptions o;
    o.threads = threads;
    o.digest = true;
    o.forest_layout = ForestLayout::kCsr;
    std::atomic<u64> callbacks{0}, phases{0};
    o.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
    o.on_fold_phase = [&](u64, FoldPhase) { ++phases; };
    const RunResult rr = run_pipeline(in, o);
    if (rr.status != PipelineStatus::kResourceExhausted) bad("scene I' : statut != resource_exhausted : " + rr.message);
    if (rr.message.find(spec->message) == std::string::npos) bad("scene I' : message sans '" + std::string(spec->message) + "' : " + rr.message);
    if (callbacks.load() != 0) bad("scene I' : callbacks publies apres bad_alloc");
    if (phases.load() == 0) bad("scene I' : aucune phase de fold observee (scene vacue)");
    if (!rr.digest_all.empty() || !rr.digest_forest.empty() || !rr.cards.empty() || rr.total_deltas != 0 ||
        !rr.forest_storage.empty() || rr.csr_fallback != 0 || rr.forest_storage_conformes != 0)
      bad("scene I' : provisoires non vides apres bad_alloc");
    std::printf("mutant csr-inject-bad-alloc : bad_alloc capture dans le fold (statut='%s', callbacks=%llu) — %s\n",
                rr.message.c_str(), (unsigned long long)callbacks.load(), g_bad ? "survivant" : "tue");
    return g_bad ? 1 : 4;
  }
  // Scene A : plafond d'APPEND abaisse a 5 cles par arene (F4 : 6 nes -> refus au 3e delta du lot 0 ; F5 : 4 nes -> accepte).
  bool refus_a_present = false;
  {
    fold_detail::csr_keys_cap_for_tests() = 5;
    const ForestResult s4 = build_forest(f4->events, 1, ForestLayout::kCsr);
    const ForestResult s5 = build_forest(f5->events, 1, ForestLayout::kCsr);
    fold_detail::csr_keys_cap_for_tests() = (size_t)UINT32_MAX;
    refus_a_present = !s4.storage_message.empty();
    if (inject.empty()) {
      if (s4.storage_message.find("plafond de cles") == std::string::npos)
        bad("scene A : F4 sous cap 5 : refus 'plafond de cles' absent (message='" + s4.storage_message + "')");
      if (s4.storage_violations != 0) bad("scene A : F4 : violation structurelle au lieu d'un refus de capacite");
      if (s4.delta_count() != 0 || !csr_payload_empty(s4)) bad("scene A : F4 : payload non vide apres refus");
      if (s4.storage_kind != ForestStorageKind::kCsrFacetKeysV1) bad("scene A : F4 : storage_kind != csr");
      check_csr_structure(s5, "scene A F5");
      const ForestResult c5 = build_forest(f5->events, 1, ForestLayout::kClassic);
      const std::string d = witness::first_divergence(c5, s5);
      if (!d.empty()) bad("scene A : F5 sous cap 5 diverge du classique sur " + d);
    } else {
      // csr-guard-skip : le refus attendu est ABSENT et l'objet est complet (9 cles sous cap 5).
      const std::string d = witness::first_divergence(c4, s4);
      if (refus_a_present || s4.storage_violations != 0 || !d.empty() || s4.delta_count() != 4) {
        std::printf("mutant csr-guard-skip : refus present ou objet incomplet (message='%s', div='%s') — survivant\n",
                    s4.storage_message.c_str(), d.c_str());
        return 1;
      }
    }
  }
  // Scene B : MAJORANT abaisse a 8 (F4 : Σ(q+d) = 10 -> refus avant toute reserve ; F5 : 4 -> accepte).
  {
    fold_detail::csr_majorant_cap_for_tests() = 8;
    const ForestResult s4 = build_forest(f4->events, 1, ForestLayout::kCsr);
    const ForestResult s5 = build_forest(f5->events, 1, ForestLayout::kCsr);
    fold_detail::csr_majorant_cap_for_tests() = (size_t)UINT32_MAX;
    if (s4.storage_message.find("majorant") == std::string::npos || s4.storage_message.find("plafond de cles") == std::string::npos)
      bad("scene B : F4 sous majorant 8 : refus 'majorant' absent (message='" + s4.storage_message + "')");
    if (s4.storage_violations != 0 || s4.delta_count() != 0 || !csr_payload_empty(s4)) bad("scene B : F4 : payload non vide ou violation");
    if (!s5.storage_message.empty() || s5.delta_count() != 2) bad("scene B : F5 sous majorant 8 refuse a tort");
  }
  if (!inject.empty()) {
    std::printf("mutant %s sur %s : refus attendu ABSENT sous cap 5 (F4 complete, 9 cles), champ '%s' — tue\n", inject.c_str(),
                spec->fixture, spec->field);
    return g_bad ? 1 : 4;
  }
  // Scene C : pipeline uniform 400 csr sous cap 5 -> resource_exhausted, zero callback, provisoires vides.
  {
    const std::vector<InputPoint> in =
        make_family_input(CloudFamily::kUniform, 400, cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
    RunOptions o;
    o.threads = threads;
    o.digest = true;
    o.forest_layout = ForestLayout::kCsr;
    std::atomic<u64> callbacks{0};
    o.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
    fold_detail::csr_keys_cap_for_tests() = 5;
    const RunResult rr = run_pipeline(in, o);
    fold_detail::csr_keys_cap_for_tests() = (size_t)UINT32_MAX;
    if (rr.status != PipelineStatus::kResourceExhausted) bad("scene C : statut != resource_exhausted : " + rr.message);
    if (rr.message.find("plafond de cles") == std::string::npos) bad("scene C : message sans 'plafond de cles' : " + rr.message);
    if (callbacks.load() != 0) bad("scene C : callbacks publies apres refus de stockage");
    if (!rr.digest_all.empty() || !rr.digest_forest.empty() || !rr.cards.empty() || rr.total_deltas != 0 ||
        !rr.forest_storage.empty() || rr.csr_fallback != 0 || rr.forest_storage_conformes != 0)
      bad("scene C : provisoires non vides apres refus");
  }
  std::printf("fold_csr_gate --overflow scenes=3 desaccords=%llu\n", (unsigned long long)g_bad);
  return g_bad ? 1 : 0;
}

// ---- --copy-alias : ForestResult autonome (copie, deplacement, copie prise dans on_forest).
int run_copy_alias(int threads) {
  const std::vector<Fixture> fx = fixtures();
  const Fixture* f4 = find_fixture(fx, "F4");
  if (!f4) return 2;
  {
    ForestResult a = build_forest(f4->events, 1, ForestLayout::kCsr);
    ForestResult b = a;
    const std::string ra = render(a);
    a.parents_keys.assign(a.parents_keys.size(), FacetKey{});
    a.delta_meta.clear();
    a = ForestResult{};
    if (render(b) != ra || ra != f4->render_expected) bad("copie : rendu de la copie altere par la destruction de l'original");
    if (digest_forest_v4(1, b) != f4->digest_pin) bad("copie : digest de la copie != pin F4");
    check_csr_structure(b, "copie");
    ForestResult c(std::move(b));
    if (render(c) != ra) bad("deplacement : rendu altere");
    if (digest_forest_v4(1, c) != f4->digest_pin) bad("deplacement : digest != pin F4");
    if (replay(c) != 0) bad("deplacement : rejeu en desaccord");
  }
  // Pipeline : copies prises dans on_forest, relues APRES run_pipeline (originaux et evenements detruits).
  {
    const std::vector<InputPoint> in =
        make_family_input(CloudFamily::kUniform, 400, cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
    RunOptions o;
    o.threads = threads;
    o.digest = true;
    o.forest_layout = ForestLayout::kCsr;
    std::mutex m;
    std::vector<ForestResult> copies;
    std::vector<std::string> dg;
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult& fr) {
      std::lock_guard<std::mutex> lk(m);
      if (copies.size() <= K) { copies.resize(K + 1); dg.resize(K + 1); }
      copies[K] = fr;
      dg[K] = digest_forest_v4((u32)K, fr);
    };
    const RunResult rr = run_pipeline(in, o);
    if (rr.status != PipelineStatus::kCompleteRegular) { std::printf("REFUS : %s\n", rr.message.c_str()); return 2; }
    u64 total = 0;
    if (copies.size() != rr.kmax_eff + 1) bad("pipeline : nombre de copies != kmax_eff");
    for (u64 K = 1; K < copies.size() && K <= rr.kmax_eff; ++K) {
      const ForestResult& c = copies[K];
      if (c.storage_kind != ForestStorageKind::kCsrFacetKeysV1) bad("pipeline : copie K non csr");
      if (digest_forest_v4((u32)K, c) != rr.digest_forest[K] || dg[K] != rr.digest_forest[K])
        bad("pipeline : digest de la copie != digest publie (K=" + std::to_string(K) + ")");
      if (replay(c) != 0) bad("pipeline : rejeu de la copie en desaccord (K=" + std::to_string(K) + ")");
      (void)render(c);
      total += c.delta_count();
    }
    if (total == 0) { std::printf("PLANCHER : aucune delta dans les copies\n"); return 3; }
    std::printf("fold_csr_gate --copy-alias deltas_copies=%llu desaccords=%llu\n", (unsigned long long)total, (unsigned long long)g_bad);
  }
  return g_bad ? 1 : 0;
}

// ---- --pipeline : matrice fils x inflight x join x layout, temoin complet contre la reference classique.
int run_pipeline_matrix(int T, const Floors& min, const std::string& only_family, int only_n) {
  struct Cloud { CloudFamily f; int n; };
  std::vector<Cloud> clouds = {
      {CloudFamily::kUniform, 400}, {CloudFamily::kUniform, 2000}, {CloudFamily::kTerrain, 400},
      {CloudFamily::kEightClusters, 400}, {CloudFamily::kScanlineSinglePass, 400},
      {CloudFamily::kCollinearSeven, 600}, {CloudFamily::kTwoLines, 200}};
  if (!only_family.empty()) {  // une seule famille/taille (tailles d'interet, label scale*)
    CloudFamily f = CloudFamily::kUniform;
    if (!parse_cloud_family(only_family.c_str(), &f) || only_n < 2) { std::printf("REFUS : famille/taille\n"); return 2; }
    clouds = {Cloud{f, only_n}};
  }
  Floors got;
  u64 cells = 0;
  std::vector<int> fils = {1};
  if (T != 1) fils.push_back(T);
  for (const auto& c : clouds) {
    const char* fam = cloud_family_name(c.f);
    const std::vector<InputPoint> in = make_family_input(c.f, c.n, cloud_family_default_coord(c.f, c.n), 3);
    std::vector<ForestResult> ref;
    RunOptions ro;
    ro.threads = 1;
    ro.fold_inflight = 1;
    ro.fold_join_before_next_k = false;
    ro.digest = true;
    ro.forest_layout = ForestLayout::kClassic;
    std::mutex m;
    ro.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult& fr) {
      std::lock_guard<std::mutex> lk(m);
      if (ref.size() <= K) ref.resize(K + 1);
      ref[K] = fr;
    };
    const RunResult rref = run_pipeline(in, ro);
    if (rref.status != PipelineStatus::kCompleteRegular) { std::printf("REFUS %s : %s\n", fam, rref.message.c_str()); return 2; }
    got.orders += rref.kmax_eff;
    for (u64 K = 1; K <= rref.kmax_eff && K < ref.size(); ++K) {
      got.deltas += ref[K].delta_count();
      got.keys += ref[K].keys_parents + ref[K].keys_born;
    }
    for (const int th : fils)
      for (const int infl : {1, 2})
        for (const int join : {0, 1})
          for (const ForestLayout lay : {ForestLayout::kClassic, ForestLayout::kCsr}) {
            ++cells;
            const std::string cell = std::string(fam) + " n=" + std::to_string(c.n) + " fils=" + std::to_string(th) +
                                     " inflight=" + std::to_string(infl) + " join=" + std::to_string(join) +
                                     " layout=" + forest_layout_name(lay);
            RunOptions o;
            o.threads = th;
            o.fold_inflight = infl;
            o.fold_join_before_next_k = join != 0;
            o.digest = true;
            o.forest_layout = lay;
            std::vector<std::string> divs;
            std::vector<u64> replays;
            std::mutex cm;
            o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult& fr) {
              std::lock_guard<std::mutex> lk(cm);
              if (K >= ref.size()) { divs.push_back("K hors reference"); return; }
              const std::string d = witness::first_divergence(ref[K], fr);
              if (!d.empty()) divs.push_back("K=" + std::to_string(K) + " " + d);
              if (lay == ForestLayout::kCsr) {
                if (fr.storage_kind != ForestStorageKind::kCsrFacetKeysV1) divs.push_back("K=" + std::to_string(K) + " storage_kind != csr");
                if (fr.parents_keys.size() != fr.keys_parents || fr.born_keys.size() != fr.keys_born)
                  divs.push_back("K=" + std::to_string(K) + " offset_dernier != cles");
                replays.push_back(replay(fr));
              }
            };
            const RunResult rr = run_pipeline(in, o);
            if (rr.status != PipelineStatus::kCompleteRegular) { bad(cell + " : statut " + rr.message); continue; }
            if (rr.digest_forest != rref.digest_forest || rr.digest_all != rref.digest_all) bad(cell + " : digests != reference");
            if (rr.cards != rref.cards) bad(cell + " : cartes != reference");
            if (rr.total_events != rref.total_events || rr.total_facets != rref.total_facets ||
                rr.total_fusions != rref.total_fusions || rr.total_deltas != rref.total_deltas || rr.total_nodes != rref.total_nodes)
              bad(cell + " : totaux != reference");
            for (const std::string& d : divs) bad(cell + " : temoin " + d);
            for (const u64 rp : replays) if (rp) bad(cell + " : rejeu csr en desaccord");
            if (lay == ForestLayout::kCsr) {
              if (rr.csr_fallback != 0 || rr.forest_storage_conformes != rr.kmax_eff) bad(cell + " : csr_fallback ou conformes");
              if (replays.size() != rr.kmax_eff) bad(cell + " : rejeu absent sur un K");
              for (u64 K = 1; K <= rr.kmax_eff && K < rr.forest_storage.size(); ++K) {
                const RunResult::ForestStorageStats& s = rr.forest_storage[K];
                if (s.kind != (u8)ForestStorageKind::kCsrFacetKeysV1 || s.parents_size != s.keys_parents ||
                    s.born_size != s.keys_born || s.deltas != rr.cards[K].deltas || !s.bytes_exact)
                  bad(cell + " : signature de stockage K=" + std::to_string(K));
              }
            } else if (rr.csr_fallback != 0 || rr.forest_storage_conformes != rr.kmax_eff) {
              bad(cell + " : classique non conforme a la demande");
            }
          }
  }
  std::printf("fold_csr_gate --pipeline cellules=%llu ordres=%llu deltas=%llu cles=%llu desaccords=%llu\n",
              (unsigned long long)cells, (unsigned long long)got.orders, (unsigned long long)got.deltas,
              (unsigned long long)got.keys, (unsigned long long)g_bad);
  if (got.orders < min.orders || got.deltas < min.deltas || got.keys < min.keys) { std::printf("PLANCHER\n"); return 3; }
  return g_bad ? 1 : 0;
}

// ---- --pipeline-refus --inject=csr-offset-* : refus transactionnel, zero callback, provisoires vides.
int run_pipeline_refus(const std::string& inject, int threads) {
  if (inject.empty() || inject.rfind("csr-offset-", 0) != 0) {
    std::printf("REFUS : --pipeline-refus exige --inject=csr-offset-*\n");
    return 2;
  }
  const std::vector<InputPoint> in =
      make_family_input(CloudFamily::kUniform, 400, cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
  RunOptions o;
  o.threads = threads;
  o.digest = true;
  o.forest_layout = ForestLayout::kCsr;
  std::atomic<u64> callbacks{0}, phases{0};
  o.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
  o.on_fold_phase = [&](u64, FoldPhase) { ++phases; };
  const RunResult rr = run_pipeline(in, o);
  if (rr.status != PipelineStatus::kInvariantViolated) bad("statut != invariant_violated : " + rr.message);
  if (rr.message.find("stockage csr") == std::string::npos) bad("message sans 'stockage csr' : " + rr.message);
  if (callbacks.load() != 0) bad("callbacks publies apres violation de stockage");
  if (phases.load() == 0) bad("aucune phase de fold observee (scene vacue)");
  if (!rr.digest_all.empty() || !rr.digest_forest.empty() || !rr.cards.empty() || rr.total_deltas != 0 ||
      !rr.forest_storage.empty() || rr.csr_fallback != 0 || rr.forest_storage_conformes != 0)
    bad("provisoires non vides apres violation de stockage");
  std::printf("fold_csr_gate --pipeline-refus mutant=%s statut=%s callbacks=%llu desaccords=%llu\n", inject.c_str(),
              rr.message.c_str(), (unsigned long long)callbacks.load(), (unsigned long long)g_bad);
  return g_bad ? 1 : 4;
}

bool parse_u64(const char* s, u64* out) {
  i64 v = 0;
  if (!parse_i64_exact(s, &v) || v < 0) return false;
  *out = (u64)v;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::string mode, inject, expect_field, only_family;
  bool inject_given = false;
  int threads = 4, only_n = 0;
  Floors min;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return a.compare(0, l, prefix) == 0 ? a.c_str() + l : nullptr;
    };
    bool ok = true;
    if (a == "--fixtures" || a == "--offsets" || a == "--overflow" || a == "--copy-alias" || a == "--pipeline" ||
        a == "--pipeline-refus") {
      if (!mode.empty()) { std::printf("REFUS : un seul mode\n"); return 2; }
      mode = a.substr(2);
    } else if (const char* s = val("--inject=")) { inject = s; inject_given = true; }
    else if (const char* s = val("--expect-divergence=")) expect_field = s;
    else if (const char* s = val("--threads=")) { i64 v = 0; ok = parse_i64_exact(s, &v) && v >= 1 && v <= 1024; threads = (int)v; }
    else if (const char* s = val("--family=")) only_family = s;
    else if (const char* s = val("--n=")) { i64 v = 0; ok = parse_i64_exact(s, &v) && v >= 2 && v <= 2147483647; only_n = (int)v; }
    else if (const char* s = val("--min-fixtures=")) ok = parse_u64(s, &min.fixtures);
    else if (const char* s = val("--min-deltas=")) ok = parse_u64(s, &min.deltas);
    else if (const char* s = val("--min-continuations=")) ok = parse_u64(s, &min.continuations);
    else if (const char* s = val("--min-continuation-lots=")) ok = parse_u64(s, &min.continuations);
    else if (const char* s = val("--min-multi-root-lots=")) ok = parse_u64(s, &min.multi_root);
    else if (const char* s = val("--min-vides=")) ok = parse_u64(s, &min.vides);
    else if (const char* s = val("--min-refus=")) ok = parse_u64(s, &min.refus);
    else if (const char* s = val("--min-orders=")) ok = parse_u64(s, &min.orders);
    else if (const char* s = val("--min-keys=")) ok = parse_u64(s, &min.keys);
    else { std::printf("REFUS : argument inconnu %s\n", a.c_str()); return 2; }
    if (!ok) { std::printf("REFUS : valeur invalide %s\n", a.c_str()); return 2; }
  }
  if (mode.empty()) { std::printf("REFUS : mode requis (--fixtures --offsets --overflow --copy-alias --pipeline --pipeline-refus)\n"); return 2; }
  if (inject_given && inject.empty()) { std::printf("REFUS : --inject= vide\n"); return 2; }
  // mutants_enable AVANT tout run (cache statique par site).
  if (!inject.empty() && !mutants_enable(inject)) { std::printf("REFUS : mutant inconnu %s\n", inject.c_str()); return 2; }
  int rc = 2;
  if (mode == "fixtures") rc = run_fixtures(inject, expect_field, min);
  else if (mode == "offsets") rc = inject.empty() ? run_offsets() : 2;
  else if (mode == "overflow") rc = run_overflow(inject, expect_field, threads);
  else if (mode == "copy-alias") rc = inject.empty() ? run_copy_alias(threads) : 2;
  else if (mode == "pipeline") rc = inject.empty() ? run_pipeline_matrix(threads, min, only_family, only_n) : 2;
  else if (mode == "pipeline-refus") rc = run_pipeline_refus(inject, threads);
  if (rc == 0) std::printf("fold_csr_gate --%s OK\n", mode.c_str());
  return rc;
}
