// MorseHGP3D v6 — fold d'un ordre K : de la liste d'evenements a la foret.
//
// Objet (docs/MATHEMATIQUES.md § 5) : pour K fixe, les sommets sont les
// facettes (K-uplets tries de PointId), chaque evenement σ = T ∪ I (|T| = q,
// |I| = d, K = q + d − 1, niveau exact) unionne ses K+1 facettes. La foret
// est le K-MST elague (Theoreme 5), rendue en macro-lots :
//   - LOTS : plages de niveaux SEMANTIQUEMENT egaux (`same_exact_level`,
//     jamais l'egalite de representation — mutant `repr-ties`) ; un lot
//     n'est jamais scinde en chronologie binaire (mutant `binary-ties`) ;
//   - ROLES : une facette σ∖{v} est ACTIVE (nee strictement avant) ou un
//     ATTACHEMENT (nait au niveau) ; les enfants d'un nœud sont les racines
//     pre-lot des ACTIVES seulement (mutant `attach-prebatch`) ;
//   - DELTAS : par racine post-lot touchee, parents = canoniques pre-lot
//     distincts, born = facettes nees (attachement ∧ ¬active) ; emis des que
//     parents != 1 ou born non vide : naissance, croissance, (multi)fusion
//     (mutant `drop-nonmerge` : fusions seules) ;
//   - CANONIQUE = plus petite FacetKey de la composante = min-fid, les fid
//     etant attribues en ordre de FacetKey croissante (mutant
//     `canonical-is-uf-root`) ;
//   - INVARIANTS mesures (toujours 0) : attachement deja vu dans un lot
//     anterieur, facette active et attachement au meme lot, partition
//     dense non triee / non idempotente.
//
// Internement en streaming : table d'adressage ouvert dimensionnee UNE fois
// sur le majorant des incidences (jamais de rehachage), appartenance par
// comparaison EXACTE de cle, puis tri des cles UNIQUES seules — le tri final
// est la seule autorite d'ordre, l'empreinte n'entre dans aucune sortie.
//
// Garde de capacite transactionnelle : evenements <= UINT32_MAX,
// Σ(q+d) <= INT32_MAX, lots < UINT32_MAX — refus AVANT toute allocation,
// jamais une troncature.
#pragma once

#include <algorithm>
#include <atomic>
#include <array>
#include <string>
#include <vector>

#include <chrono>

#include "../core/mutants.hpp"
#include "../lanes/keys.hpp"
#include "../lanes/level.hpp"
#include "../parallel/pool.hpp"
#include "../parallel/sort.hpp"

namespace mhgp6 {

struct ForestEvent {
  u8 q = 0;               // |T| <= 11
  u8 d = 0;               // |I| <= 9
  u16 active_mask = 0;    // bit t : σ∖{T[t]} active
  PointId support[11] = {};
  PointId interior[9] = {};
  ExactLevel level{};
};

struct ComponentDelta {
  u64 batch = 0;
  ExactLevel level{};
  FacetKey output;
  std::vector<FacetKey> parents;  // tries
  std::vector<FacetKey> born;     // triees
};

#if defined(MHGP6_PROFILE_LIVENESS) && !defined(MHGP6_PROFILE_REDUCE)
#error "MHGP6_PROFILE_LIVENESS exige MHGP6_PROFILE_REDUCE (la sonde n'existe pas seule)"
#endif

#ifdef MHGP6_PROFILE_REDUCE
// PROFIL DU REDUCE (§ 5.10 de REPONSE_AUDITEURS_MULTICPU_V6) — record PAR K,
// stocke dans ForestResult et imprime par run.hpp APRES run_pipeline (aucune
// impression du profil dans les chronos ; %.3f ; somme + residuel aux memes
// bornes ; intervalles pour rendre VISIBLES les recouvrements
// reduction/reduction et A/reduction — chaque fenetre est un temps mur LOCAL
// a un K, leur somme est un cumul qui peut depasser le mur du fold et n'en
// est JAMAIS soustraite). HORIZONS EXACTS (7e contre-lecture 9041c191) :
// begin est pris apres le deplacement initial et le test de refus ; end
// avant les destructeurs restants — liberation_ms ne couvre QUE ev_fid et
// FidState ; init_ms ne couvre PAS la croissance dynamique de scratch (elle
// tombe dans post_remplissage) ; les colonnes d'internement sont des
// fenetres locales SELECTIVES, jamais un bilan exhaustif des temporaires.
// Le mur de reference vient d'un Release NON instrumente ; ces colonnes ne
// sont qu'une ATTRIBUTION. La sonde de vivacite (TROIS parcours des
// incidences : precompte, activation, decrement) n'existe que sous
// MHGP6_PROFILE_LIVENESS (a definir EN PLUS de MHGP6_PROFILE_REDUCE) : sa
// pollution cache/mur ne contamine plus l'attribution par defaut.
struct ReduceProfile {
  // prepare (etage intern) : fenetres de l'internement des facettes.
  double intern_empreintes_ms = 0, intern_diffusion_ms = 0, intern_tri_ms = 0, intern_fusion_ms = 0,
         intern_remap_ms = 0;
  // reduce : initialisation (allocations FidState + warmup de prefetch,
  // mesuree des l'entree — scratch grandit plus tard), fenetres par lot, fin.
  double init_ms = 0, touch_ms = 0, pre_ms = 0, unite_ms = 0, post_remplissage_ms = 0,
         materialisation_tri_copie_ms = 0, liveness_ms = 0, partition_ms = 0, liberation_ms = 0;
  std::chrono::steady_clock::time_point begin{}, end{};
  // Champs remplis par run.hpp (jamais par le fold) : intervalles de l'etage
  // A (preparation) — la concurrence A/B se LIT dans la trace —, duree du
  // digest par K, et le drapeau join du run.
  std::chrono::steady_clock::time_point a_begin{}, a_end{};
  double duree_digest_foret_k_ms = 0;
  double somme() const {
    return init_ms + touch_ms + pre_ms + unite_ms + post_remplissage_ms +
           materialisation_tri_copie_ms + liveness_ms + partition_ms + liberation_ms;
  }
#ifdef MHGP6_PROFILE_LIVENESS
  // Vivacite : PIC INTRA-LOT (fids du lot actives PUIS releve PUIS
  // decrementation des derniers contacts — l'ancien releve post-extinction
  // pouvait publier zero sur un lot au pic eleve) ET frontiere inter-lots.
  u64 live_peak_intra = 0, live_frontier_max = 0;
  double live_frontier_mean_pct = 0;
#endif
};
#endif

struct ForestResult {
  std::string refusal;                 // non vide = refus AVANT allocation
  u64 facets = 0, fusions = 0, batches = 0, new_attachments = 0;
  u64 attach_violations = 0, birth_violations = 0, partition_violations = 0;
  u64 nodes = 0;                       // deltas a >= 2 parents (vue derivee)
  std::vector<FacetKey> facet_keys;    // fid -> FacetKey, strictement croissante
  std::vector<u32> final_canon_fid;    // fid -> plus petit fid de sa composante
  // Le payload des EVENEMENTS VERIFIES (forest_semantics=verified_events_only,
  // proof_basis=gabriel_positive_connectivity), de portee horizontale : il ne
  // porte PAS les incidences silencieuses du contrat Gamma (cofaces non-Gabriel
  // attachant une arete a un niveau que le flot brut ne voit pas ; fixture
  // gabriel-point-set-counterexample-5-points-v1). Ce n'est donc pas un payload
  // hierarchique complet, et il ne peut pas porter require_exact=true.
  std::vector<ComponentDelta> deltas;
  std::vector<ExactLevel> batch_levels;
  u64 workers = 0;  // ouvriers reellement crees (max sur les phases paralleles)
  double t_sort_ms = 0, t_intern_ms = 0, t_merge_ms = 0, t_reduce_ms = 0, t_partition_ms = 0;
#ifdef MHGP6_PROFILE_REDUCE
  ReduceProfile profile;  // rempli par prepare/reduce, imprime a la publication (run.hpp)
#endif
};

namespace fold_detail {

inline FacetKey facet_minus(const ForestEvent& e, int drop_support, int drop_interior) {
  FacetKey f;
  for (int t = 0; t < (int)e.q; ++t)
    if (t != drop_support) f.p[f.k++] = e.support[t];
  for (int t = 0; t < (int)e.d; ++t)
    if (t != drop_interior) f.p[f.k++] = e.interior[t];
  for (u8 t = 1; t < f.k; ++t) {  // insertion (k <= 10)
    const PointId v = f.p[t];
    u8 w = t;
    for (; w > 0 && f.p[w - 1] > v; --w) f.p[w] = f.p[w - 1];
    f.p[w] = v;
  }
  return f;
}

inline bool facet_less_k(const FacetKey& x, const FacetKey& y) {
  if (x.k != y.k) return x.k < y.k;
  for (u8 i = 0; i < x.k; ++i)
    if (x.p[i] != y.p[i]) return x.p[i] < y.p[i];
  return false;
}

inline bool facet_equal_k(const FacetKey& x, const FacetKey& y) {
  if (x.k != y.k) return false;
  for (u8 i = 0; i < x.k; ++i)
    if (x.p[i] != y.p[i]) return false;
  return true;
}

#ifdef MHGP6_TESTING
// STRESS DE COLLISION (jamais compile en production : `MHGP6_TESTING` n'est
// pose que sur les cibles de tests/). Force l'empreinte d'adressage a une
// CONSTANTE : toutes les facettes tombent dans la meme partition et le meme
// domicile de la table d'internement. Ce n'est pas un mutant — ce n'est pas
// un defaut : l'appartenance etant tranchee par comparaison EXACTE de cle,
// la sortie doit etre INCHANGEE, et c'est ce que verifie la porte des
// fixtures (docs/ECHELLE.md § 8 bis, « frontieres externes avec hachage
// constant »). A n'employer que sur des fixtures : le sondage lineaire y
// devient quadratique.
inline bool& fold_hash_constant() {
  static bool b = false;
  return b;
}
#endif

// Empreinte d'adressage seulement : l'appartenance est tranchee par la cle.
inline u64 facet_fingerprint(const FacetKey& f) {
#ifdef MHGP6_TESTING
  if (fold_hash_constant()) return 0;
#endif
  u64 h = 0x9E3779B97F4A7C15ull ^ (u64)f.k;
  for (u8 i = 0; i < f.k; ++i) h = (h + (u64)f.p[i]) * 0x9E3779B97F4A7C15ull;
  h ^= h >> 31;
  h *= 0xBF58476D1CE4E5B9ull;
  h ^= h >> 27;
  h *= 0x94D049BB133111EBull;
  h ^= h >> 31;
  return h;
}

struct FacetIntern {
  static constexpr u64 kTag = 0xFFFFFFFF00000000ull;
  std::vector<u64> table;                      // empreinte<<32 | tid+1 ; 0 = vide
  std::vector<std::pair<FacetKey, u32>> pool;  // tid -> (cle, tid)
  size_t mask = 0;
  explicit FacetIntern(size_t incidences) {
    size_t cap = 1024;
    while (cap < incidences * 2 + 2) cap <<= 1;
    table.assign(cap, 0);
    mask = cap - 1;
    pool.reserve(incidences);
  }
  void prefetch(u64 h) const {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(&table[(size_t)h & mask], 1, 1);
#else
    (void)h;
#endif
  }
  u32 intern_hashed(const FacetKey& f, u64 h) {
    const u64 tag = h & kTag;
    size_t i = (size_t)h & mask;
    while (table[i] != 0) {
      const u32 tid = (u32)(table[i] & 0xFFFFFFFFull) - 1;
      if ((table[i] & kTag) == tag && facet_equal_k(pool[(size_t)tid].first, f)) return tid;
      i = (i + 1) & mask;
    }
    const u32 tid = (u32)pool.size();
    pool.push_back({f, tid});
    table[i] = tag | (u64)(tid + 1);
    return tid;
  }
};


}  // namespace fold_detail

// Garde de capacite transactionnelle, decidable AVANT toute allocation sur
// les seuls comptes (evenements <= UINT32_MAX ; Σ(q+d) <= INT32_MAX pour les
// fid de l'union-find i32 ; lots <= evenements < UINT32_MAX, sentinelle des
// tables a epoque).
inline bool fold_capacity_ok(u64 events, u64 incidences, std::string* why) {
  if (events >= (u64)UINT32_MAX / 11) { *why = "resource_exhausted/requires_tiling : evenements >= (2^32-1)/11 (positions d'incidence u32)"; return false; }
  if (incidences > (u64)INT32_MAX) { *why = "resource_exhausted/requires_tiling : incidences > 2^31-1"; return false; }
  return true;
}

// `order` : permutation des evenements triee par niveau exact (calculee par
// l'appelant, qui peut la partager avec le rendu).
inline std::vector<u32> sort_events_by_level(const std::vector<ForestEvent>& events, int threads = 1,
                                             u64* workers = nullptr) {
  std::vector<u32> order(events.size());
  for (size_t i = 0; i < order.size(); ++i) order[i] = (u32)i;
  // Tri STABLE (les ex aequo d'un lot gardent l'ordre d'entree : l'ordre des
  // unions et l'emission des deltas en dependent) — parallele, identique a
  // std::stable_sort par contrat (porte mhgp6_parallel_sort_gate).
  const size_t w = parallel_stable_sort(
      order.begin(), order.end(),
      [&](u32 x, u32 y) { return compare_exact_level(events[x].level, events[y].level) < 0; }, threads);
  if (workers) *workers = std::max(*workers, (u64)w);
  return order;
}

// PARTITIONNEMENT PAR EMPREINTE (parallelisation du fold, session G4 du
// 27 aout : le fold sequentiel pesait 115 s sur 219 a uniform 50k) :
// 64 partitions FIXES par les six bits hauts de l'empreinte — independantes du
// nombre de fils, donc les pools et les tid ne dependent que de l'ordre des
// evenements ; les fid finaux viennent du tri global des cles uniques et ne
// dependent de rien d'autre : la sortie est bit-identique a 1 fil et a N fils.
inline constexpr int kFoldPartitionBits = 6;
inline constexpr size_t kFoldPartitions = (size_t)1 << kFoldPartitionBits;

// ETAT PREPARE d'un fold : tout ce qui est parallele (tri des evenements,
// lots, internement partitionne, fusion, remap). La reduction (union-find a
// lots, deltas, partition finale) est sequentielle et vit dans
// `reduce_fold`, pour pouvoir etre PIPELINEE avec la preparation de l'ordre
// suivant (run.hpp). Non copiable ; `events` est non possede : l'appelant le
// garde vivant jusqu'a la fin de la reduction.
struct FoldPrepared {
  const std::vector<ForestEvent>* events = nullptr;
  ForestResult r;  // refusal, workers, chronos de preparation, facets
  std::vector<u32> order;
  std::vector<std::pair<size_t, size_t>> batches;
  std::vector<FacetKey> keys;
  std::vector<u32> ev_fid;
  bool mutants[6] = {false, false, false, false, false, false};  // binary, repr, attach_pre, drop_nonmerge, canon_root, no_detector
};

// VALIDATION STRUCTURELLE (P0 de l'audit 9762daaf), distincte de la garde de
// capacite et AVANT toute allocation ou tri : q in [2, 11], d <= 9, q + d <= 11,
// un meme K = q + d - 1 dans l'appel, identifiants distincts (support et
// interieur), active_mask borne aux q bits. Un evenement hors contrat est un
// refus `invalid_input`, jamais une ecriture hors des dix cases de FacetKey.
inline bool fold_event_ok(const ForestEvent& ev, int K) {
  if (ev.q < 2 || ev.q > 11 || ev.d > 9 || (int)ev.q + (int)ev.d > 11) return false;
  if ((int)ev.q + (int)ev.d - 1 != K) return false;
  if ((u32)ev.active_mask >= (1u << ev.q)) return false;
  PointId ids[20];
  int n = 0;
  for (int t = 0; t < (int)ev.q; ++t) ids[n++] = ev.support[t];
  for (int t = 0; t < (int)ev.d; ++t) ids[n++] = ev.interior[t];
  for (int a = 0; a < n; ++a)
    for (int b = a + 1; b < n; ++b)
      if (ids[a] == ids[b]) return false;
  return true;
}

inline bool validate_fold_events(const std::vector<ForestEvent>& events, int threads, std::string* why) {
  // BALAYAGE SEQUENTIEL : aucune allocation ni fil avant que le contrat ne
  // soit verifie (la revendication « avant toute allocation » est ainsi
  // vraie ; cout mesure : ~60 ns par evenement). `threads` reste dans la
  // signature pour les appelants ; il n'est pas utilise ici.
  (void)threads;
  if (events.empty()) return true;
  const int K = (int)events[0].q + (int)events[0].d - 1;
  for (size_t i = 0; i < events.size(); ++i) {
    const ForestEvent& ev = events[i];
    const bool level_ok = ev.level.den > 0;  // contrat ExactLevel : den > 0
    if (fold_event_ok(ev, K) && level_ok) continue;
    *why = "invalid_input : evenement " + std::to_string(i) + " hors contrat (q=" + std::to_string((int)ev.q) +
           ", d=" + std::to_string((int)ev.d) + ", K attendu=" + std::to_string(K) +
           (level_ok ? "" : ", niveau den <= 0") +
           ") : q in [2,11], d <= 9, q+d <= 11, K constant, identifiants distincts, masque < 2^q, den > 0";
    return false;
  }
  return true;
}

inline FoldPrepared prepare_fold(const std::vector<ForestEvent>& events, int threads = 1) {
  using namespace fold_detail;
  FoldPrepared fp;
  fp.events = &events;
  ForestResult& r = fp.r;
  const bool m_binary = MHGP6_MUTANT("binary-ties");
  const bool m_repr = MHGP6_MUTANT("repr-ties");
  fp.mutants[0] = m_binary;
  fp.mutants[1] = m_repr;
  fp.mutants[2] = MHGP6_MUTANT("attach-prebatch");
  fp.mutants[3] = MHGP6_MUTANT("drop-nonmerge");
  fp.mutants[4] = MHGP6_MUTANT("canonical-is-uf-root");
  fp.mutants[5] = MHGP6_MUTANT("attach-detector-disabled");
  auto tmark = std::chrono::steady_clock::now();
  const auto mark = [&](double* out) {
    const auto now = std::chrono::steady_clock::now();
    *out += std::chrono::duration<double, std::milli>(now - tmark).count();
    tmark = now;
  };
  // Validation structurelle puis garde de capacite, AVANT toute allocation.
  if (!validate_fold_events(events, threads, &r.refusal)) return fp;
  u64 total_recs = 0;
  for (const ForestEvent& ev : events) total_recs += (u64)ev.q + ev.d;
  if (!fold_capacity_ok((u64)events.size(), total_recs, &r.refusal)) return fp;
  fp.order = sort_events_by_level(events, threads, &r.workers);
  const std::vector<u32>& order = fp.order;
  const auto evt = [&](size_t i) -> const ForestEvent& { return events[(size_t)order[i]]; };
  mark(&r.t_sort_ms);

  // Lots.
  std::vector<std::pair<size_t, size_t>>& batches = fp.batches;
  for (size_t b0 = 0; b0 < events.size();) {
    size_t b1 = b0 + 1;
    while (b1 < events.size() && !m_binary) {
      const bool same = m_repr ? (evt(b1).level == evt(b0).level) : same_exact_level(evt(b1).level, evt(b0).level);
      if (!same) break;
      ++b1;
    }
    batches.push_back({b0, b1});
    b0 = b1;
  }

  // ---- Internement partitionne.
  // Passe 1 (parallele par tranche d'evenements) : empreintes et positions,
  // comptees par partition. Passe 2 : diffusion en ordre (partition, tranche).
  // Passe 3 (parallele par partition) : table privee, tid locaux, ev_fid =
  // partition << 26 | tid. Passe 4 (parallele par partition) : tri des cles
  // uniques. Passe 5 : fusion k-aire des 64 listes triees -> fid globaux.
  // Passe 6 (parallele par tranche) : remap des ev_fid.
  const size_t ne = events.size();
#ifdef MHGP6_PROFILE_REDUCE
  double pi[6] = {0, 0, 0, 0, 0, 0};
  auto pim = std::chrono::steady_clock::now();
  const auto pitick = [&](int i) { const auto now = std::chrono::steady_clock::now(); pi[i] += std::chrono::duration<double, std::milli>(now - pim).count(); pim = now; };
#else
  const auto pitick = [](int) {};
#endif
  std::vector<FacetKey>& keys = fp.keys;
  fp.ev_fid.assign(ne * 11, 0);
  std::vector<u32>& ev_fid = fp.ev_fid;
  std::vector<u8> ev_part(ne * 11, 0);  // partition de chaque enregistrement (temporaire de prepare_fold)
  {
    struct Rec {
      u64 h;
      u32 pos;
    };
    const size_t T = planned_workers(ne, threads);
    const size_t chunk = T <= 1 ? std::max<size_t>(ne, 1) : std::max<size_t>(1, (ne + 8 * T - 1) / (8 * T));
    const size_t nchunks = ne == 0 ? 0 : (ne + chunk - 1) / chunk;
    std::vector<std::vector<Rec>> crec(nchunks);
    std::vector<std::vector<u32>> ccount(nchunks, std::vector<u32>(kFoldPartitions, 0));
    size_t created = parallel_items(nchunks, (int)T, [&](size_t c, size_t) {
      const size_t e0 = c * chunk, e1 = std::min(ne, e0 + chunk);
      std::vector<Rec>& out = crec[c];
      std::vector<u32>& cnt = ccount[c];
      size_t inc = 0;
      for (size_t e = e0; e < e1; ++e) inc += (size_t)evt(e).q + evt(e).d;
      out.reserve(inc);
      for (size_t e = e0; e < e1; ++e) {
        const ForestEvent& ev = evt(e);
        for (int s = 0; s < (int)ev.q; ++s) {
          const u64 h = facet_fingerprint(facet_minus(ev, s, -1));
          out.push_back(Rec{h, (u32)(e * 11 + (size_t)s)});
          ++cnt[(size_t)(h >> (64 - kFoldPartitionBits))];
        }
        for (int z = 0; z < (int)ev.d; ++z) {
          const u64 h = facet_fingerprint(facet_minus(ev, -1, z));
          out.push_back(Rec{h, (u32)(e * 11 + (size_t)(ev.q + z))});
          ++cnt[(size_t)(h >> (64 - kFoldPartitionBits))];
        }
      }
    });
    r.workers = std::max(r.workers, (u64)created);
    // Offsets : base par partition, puis par (partition, tranche).
    pitick(0);
    std::vector<size_t> pbase(kFoldPartitions + 1, 0);
    for (size_t p = 0; p < kFoldPartitions; ++p) {
      size_t tot = 0;
      for (size_t c = 0; c < nchunks; ++c) tot += ccount[c][p];
      pbase[p + 1] = pbase[p] + tot;
    }
    std::vector<std::vector<size_t>> coff(nchunks, std::vector<size_t>(kFoldPartitions, 0));
    for (size_t p = 0; p < kFoldPartitions; ++p) {
      size_t off = pbase[p];
      for (size_t c = 0; c < nchunks; ++c) {
        coff[c][p] = off;
        off += ccount[c][p];
      }
    }
    std::vector<Rec> parts((size_t)total_recs);
    created = parallel_items(nchunks, (int)T, [&](size_t c, size_t) {
      std::vector<size_t> off = coff[c];
      for (const Rec& rc : crec[c]) parts[off[(size_t)(rc.h >> (64 - kFoldPartitionBits))]++] = rc;
      std::vector<Rec>().swap(crec[c]);
    });
    r.workers = std::max(r.workers, (u64)created);
    // Passe 3 : internement par partition (table privee), tid locaux.
    pitick(1);
    std::vector<std::vector<std::pair<FacetKey, u32>>> pools(kFoldPartitions);
    const size_t TP = planned_workers(kFoldPartitions, threads);
    created = parallel_items(kFoldPartitions, (int)TP, [&](size_t p, size_t) {
      const size_t b = pbase[p], e = pbase[p + 1];
      if (b == e) return;
      FacetIntern in(e - b);
      for (size_t i = b; i < e; ++i) {
        const Rec& rc = parts[i];
        const size_t ev_i = rc.pos / 11, slot = rc.pos % 11;
        const ForestEvent& ev = evt(ev_i);
        const FacetKey f = slot < ev.q ? facet_minus(ev, (int)slot, -1) : facet_minus(ev, -1, (int)(slot - ev.q));
        // Temporaire {partition, tid} en deux tableaux : injectif sans borne
        // sur le nombre de facettes par partition (P1 de l'audit 9762daaf).
        ev_fid[rc.pos] = in.intern_hashed(f, rc.h);
        ev_part[rc.pos] = (u8)p;
      }
      std::vector<u64>().swap(in.table);
      pools[p].swap(in.pool);
      // Passe 4 : tri des cles uniques de la partition ; second = tid.
      std::sort(pools[p].begin(), pools[p].end(),
                [](const std::pair<FacetKey, u32>& x, const std::pair<FacetKey, u32>& y) {
                  return facet_less_k(x.first, y.first);
                });
    });
    r.workers = std::max(r.workers, (u64)created);
    pitick(2);
    std::vector<Rec>().swap(parts);
    mark(&r.t_intern_ms);
    // Passe 5 : fusion k-aire des partitions triees -> keys globales et
    // g_of[p][tid] = fid global. Les cles sont distinctes entre partitions
    // (empreintes differentes) : aucune egalite a departager. PARALLELE PAR
    // RANGS DE VALEURS : des separateurs pris dans la plus grosse partition
    // decoupent l'ordre total en R rangs ; chaque rang fusionne (tas) les 64
    // sous-listes [lower_bound(sep_t), lower_bound(sep_{t+1})) — les bornes
    // par partition et les offsets de sortie sont connus avant la fusion.
    size_t total_unique = 0, biggest = 0;
    for (size_t p = 0; p < kFoldPartitions; ++p) {
      total_unique += pools[p].size();
      if (pools[p].size() > pools[biggest].size()) biggest = p;
    }
    keys.resize(total_unique);
    std::vector<std::vector<u32>> g_of(kFoldPartitions);
    for (size_t p = 0; p < kFoldPartitions; ++p) g_of[p].resize(pools[p].size());
    {
      const size_t R = std::max<size_t>(1, std::min<size_t>(planned_workers(total_unique, threads) * 2, pools[biggest].size()));
      // Bornes lo[t][p] : debut du rang t dans la partition p (lo[R][p] = fin).
      std::vector<std::vector<size_t>> lo(R + 1, std::vector<size_t>(kFoldPartitions, 0));
      for (size_t p = 0; p < kFoldPartitions; ++p) lo[R][p] = pools[p].size();
      for (size_t t = 1; t < R; ++t) {
        const FacetKey& sep = pools[biggest][t * pools[biggest].size() / R].first;
        for (size_t p = 0; p < kFoldPartitions; ++p) {
          const auto it = std::lower_bound(pools[p].begin(), pools[p].end(), sep,
                                           [](const std::pair<FacetKey, u32>& a, const FacetKey& k) {
                                             return facet_less_k(a.first, k);
                                           });
          lo[t][p] = (size_t)(it - pools[p].begin());
        }
      }
      std::vector<size_t> out_off(R + 1, 0);
      for (size_t t = 0; t < R; ++t) {
        size_t n = 0;
        for (size_t p = 0; p < kFoldPartitions; ++p) n += lo[t + 1][p] - lo[t][p];
        out_off[t + 1] = out_off[t] + n;
      }
      const size_t created_m = parallel_items(R, threads, [&](size_t t, size_t) {
        std::vector<size_t> cur(kFoldPartitions);
        for (size_t p = 0; p < kFoldPartitions; ++p) cur[p] = lo[t][p];
        std::vector<u32> heap;
        const auto less_p = [&](u32 a, u32 b) {
          return facet_less_k(pools[b][cur[b]].first, pools[a][cur[a]].first);  // min-heap
        };
        for (u32 p = 0; p < (u32)kFoldPartitions; ++p)
          if (cur[p] < lo[t + 1][p]) heap.push_back(p);
        std::make_heap(heap.begin(), heap.end(), less_p);
        for (size_t g = out_off[t]; g < out_off[t + 1]; ++g) {
          std::pop_heap(heap.begin(), heap.end(), less_p);
          const u32 p = heap.back();
          heap.pop_back();
          keys[g] = pools[p][cur[p]].first;
          g_of[p][(size_t)pools[p][cur[p]].second] = (u32)g;
          if (++cur[p] < lo[t + 1][p]) {
            heap.push_back(p);
            std::push_heap(heap.begin(), heap.end(), less_p);
          }
        }
      });
      r.workers = std::max(r.workers, (u64)created_m);
    }
    pitick(3);
    for (size_t p = 0; p < kFoldPartitions; ++p) std::vector<std::pair<FacetKey, u32>>().swap(pools[p]);
    // Passe 6 : remap parallele.
    created = parallel_ranges(ne * 11, threads, [&](size_t b, size_t e, size_t) {
      for (size_t i = b; i < e; ++i) {
        const size_t ev_i = i / 11, slot = i % 11;
        const ForestEvent& ev = evt(ev_i);
        if (slot >= (size_t)ev.q + ev.d) continue;
        ev_fid[i] = g_of[(size_t)ev_part[i]][(size_t)ev_fid[i]];
      }
    });
    r.workers = std::max(r.workers, (u64)created);
#ifdef MHGP6_PROFILE_REDUCE
    // § 5.10 : plus AUCUNE impression avant les mark — les fenetres intern
    // sont stockees dans le record et imprimees a la publication ordonnee.
    pitick(4);
    r.profile.intern_empreintes_ms += pi[0];
    r.profile.intern_diffusion_ms += pi[1];
    r.profile.intern_tri_ms += pi[2];
    r.profile.intern_fusion_ms += pi[3];
    r.profile.intern_remap_ms += pi[4];
#endif
    mark(&r.t_merge_ms);
  }
  r.facets = keys.size();
  return fp;
}

// Etat par facette PACKE sur 32 octets (sizeof fige — ni ligne de cache ni
// alignement prouves) : le reduce est sequentiel ; l'HYPOTHESE DE
// DIMENSIONNEMENT (jamais un diagnostic mesure) est la latence memoire — a 200 k points, 56 M facettes
// par ordre, ~6 facettes touchees par evenement dans plusieurs tableaux
// distincts. Un etat compact par facette et une prefetch glissante (fenetre
// kReduceAhead evenements)
// recouvrent ces defauts. SEMANTIQUE INCHANGEE : memes racines (la racine de
// `first` absorbe), meme compression par moitie, memes epoques, meme ordre
// des deltas (racines triees) — le digest v5 est bit-identique (conformites,
// banc a signature identique).
struct FidState {
  i32 parent;
  u32 canon;
  u32 role_epoch, pre_epoch, post_epoch;
  u32 pre_canon, post_slot;
  u8 role_bits, seen;
  u8 pad_[2];
};
static_assert(sizeof(FidState) == 32, "FidState : 32 octets (taille figee du DSU chaud)");

inline constexpr size_t kReduceAhead = 8;

inline void reduce_prefetch(const void* p) {
#if defined(__GNUC__) || defined(__clang__)
  __builtin_prefetch(p, 1, 3);
#else
  (void)p;
#endif
}

inline ForestResult reduce_fold(FoldPrepared&& fp) {
  using namespace fold_detail;
  ForestResult r = std::move(fp.r);
  if (!r.refusal.empty()) return r;
#ifdef MHGP6_PROFILE_REDUCE
  // § 5.10 : le chronometre du profil demarre DES L'ENTREE — la fenetre init
  // couvre les allocations produit (FidState, scratch, deltas.reserve) ET le
  // warmup de prefetch, plus rien ne fuit dans la premiere fenetre touch.
  //   0 touch, 1 pre, 2 unite, 3 post_remplissage, 4 materialisation_tri_
  //   copie, 5 bookkeeping vivacite (LIVENESS seulement), 6 init,
  //   7 partition finale, 8 liberation.
  double pt[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  auto pm = std::chrono::steady_clock::now();
  r.profile.begin = pm;
  const auto ptick = [&](int i) { const auto now = std::chrono::steady_clock::now(); pt[i] += std::chrono::duration<double, std::milli>(now - pm).count(); pm = now; };
#else
  const auto ptick = [](int) {};
#endif
  const std::vector<ForestEvent>& events = *fp.events;
  const std::vector<u32>& order = fp.order;
  const auto evt = [&](size_t i) -> const ForestEvent& { return events[(size_t)order[i]]; };
  const std::vector<std::pair<size_t, size_t>>& batches = fp.batches;
  std::vector<FacetKey>& keys = fp.keys;
  std::vector<u32>& ev_fid = fp.ev_fid;
  const bool m_attach_pre = fp.mutants[2], m_drop_nonmerge = fp.mutants[3], m_canon_root = fp.mutants[4],
             m_no_detector = fp.mutants[5];
  // SONDES D'ABLATION (2 septembre, arbre § 5.10 : decomposer la fenetre
  // materialisation_tri_copie AVANT d'ecrire un palier) — trois retraits
  // isoles, chacun change l'objet (conformite code 4), constante false hors
  // MHGP6_TESTING : (a) copie profonde scratch -> r.deltas retiree ; (b) tris
  // des parents/nes (44 o) retires ; (c) lecture aleatoire keys[fid] du
  // remplissage remplacee par une cle CHAUDE (copie locale de keys[0]) — le
  // prefetch de keys[] reste arme, l'ablation n'isole que la lecture.
  const bool abl_sans_copie = MHGP6_MUTANT("ablation-mat-sans-copie"),
             abl_sans_tris = MHGP6_MUTANT("ablation-mat-sans-tris"),
             abl_cle_factice = MHGP6_MUTANT("ablation-post-cle-factice");
  const FacetKey cle_factice = keys.empty() ? FacetKey{} : keys[0];
  auto tmark = std::chrono::steady_clock::now();
  const auto mark = [&](double* out) {
    const auto now = std::chrono::steady_clock::now();
    *out += std::chrono::duration<double, std::milli>(now - tmark).count();
    tmark = now;
  };
  const size_t nfid = keys.size();
  r.facets = nfid;

  std::vector<FidState> st(nfid);
  for (size_t i = 0; i < nfid; ++i) st[i] = FidState{(i32)i, (u32)i, UINT32_MAX, UINT32_MAX, UINT32_MAX, 0, 0, 0, 0, {0, 0}};
  const auto find = [&](i32 v) -> i32 {
    while (st[(size_t)v].parent != v) {
      st[(size_t)v].parent = st[(size_t)st[(size_t)v].parent].parent;
      v = st[(size_t)v].parent;
    }
    return v;
  };
  const auto unite_canon = [&](i32 a, i32 b) {
    const i32 ra = find(a), rb = find(b);
    if (ra == rb) return false;
    const u32 mn = m_canon_root ? st[(size_t)ra].canon : std::min(st[(size_t)ra].canon, st[(size_t)rb].canon);
    st[(size_t)rb].parent = ra;
    st[(size_t)ra].canon = mn;
    return true;
  };
  constexpr u8 kActive = 1, kAttach = 2;
  std::vector<u32> touched;
  std::vector<i32> pre_list, post_list;
  std::vector<ComponentDelta> scratch;
  const size_t ne = order.size();
  r.deltas.reserve(batches.size());
  const auto prefetch_event = [&](size_t e) {
    const ForestEvent& pv = evt(e);
    const u32* f = &ev_fid[e * 11];
    for (int s = 0; s < (int)pv.q + (int)pv.d; ++s) {
      reduce_prefetch(&st[(size_t)f[s]]);
      reduce_prefetch(&keys[(size_t)f[s]]);  // clef copiee dans parents/born
    }
  };
  for (size_t e = 0; e < std::min(kReduceAhead, ne); ++e) {
    reduce_prefetch(&events[(size_t)order[e]]);
    prefetch_event(e);
  }
  ptick(6);  // initialisation : allocations produit + warmup de prefetch (fermee AVANT la sonde)
#if defined(MHGP6_PROFILE_REDUCE) && defined(MHGP6_PROFILE_LIVENESS)
  // SONDE DE VIVACITE (opt-in SEPARE, § 5.10 : deux balayages de toutes les
  // incidences + bookkeeping par lot polluent caches et mur — jamais dans
  // l'attribution par defaut) : fraction de facettes VIVANTES au fil des
  // lots, pour le dimensionnement d'un fold streame a etat borne.
  std::vector<u32> remaining(nfid, 0);
  for (size_t e = 0; e < ne; ++e) {
    const ForestEvent& ev = evt(e);
    for (int t = 0; t < (int)ev.q + (int)ev.d; ++t) ++remaining[ev_fid[e * 11 + (size_t)t]];
  }
  std::vector<u8> alive_flag(nfid, 0);
  u64 live = 0, live_peak_intra = 0, live_frontier_max = 0, live_sum = 0;
  ptick(5);  // le pre-balayage de vivacite est SA fenetre, jamais l'init
#endif
  for (size_t b = 0; b < batches.size(); ++b) {
    const size_t e0 = batches[b].first, e1 = batches[b].second;
    touched.clear();
    const auto touch = [&](u32 fid, u8 bit) {
      FidState& f = st[(size_t)fid];
      if (f.role_epoch != (u32)b) {
        f.role_epoch = (u32)b;
        f.role_bits = 0;
        touched.push_back(fid);
      }
      f.role_bits |= bit;
    };
    // Fenetre de prefetch GLOBALE (les lots sont le plus souvent d'un seul
    // evenement : une fenetre par lot ne recouvrirait rien).
    for (size_t e = e0; e < e1; ++e) {
      if (e + kReduceAhead < ne) {
        reduce_prefetch(&events[(size_t)order[e + kReduceAhead]]);
        prefetch_event(e + kReduceAhead);
      }
      const ForestEvent& ev = evt(e);
      for (int s = 0; s < (int)ev.q; ++s) touch(ev_fid[e * 11 + (size_t)s], ((ev.active_mask >> s) & 1u) ? kActive : kAttach);
      for (int z = 0; z < (int)ev.d; ++z) touch(ev_fid[e * 11 + (size_t)(ev.q + z)], kAttach);
    }
    ptick(0);
    for (const u32 fid : touched) {
      const FidState& f = st[(size_t)fid];
      const bool active = f.role_bits & kActive;
      const bool attach = f.role_bits & kAttach;
      if (!m_no_detector) {
        if (attach && f.seen) ++r.attach_violations;
        if (attach && active) ++r.birth_violations;
      }
      if (attach && !active) ++r.new_attachments;
    }
    pre_list.clear();
    for (const u32 fid : touched)
      if (m_attach_pre || (st[(size_t)fid].role_bits & kActive)) {
        const i32 pr = find((i32)fid);
        FidState& fr = st[(size_t)pr];
        if (fr.pre_epoch != (u32)b) {
          fr.pre_epoch = (u32)b;
          fr.pre_canon = fr.canon;
          pre_list.push_back(pr);
        }
      }
    std::sort(pre_list.begin(), pre_list.end());
    ptick(1);
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      i32 first = -1;
      for (int s = 0; s < (int)ev.q; ++s) {
        const i32 v = (i32)ev_fid[e * 11 + (size_t)s];
        if (first < 0) first = v;
        else if (unite_canon(first, v)) ++r.fusions;
      }
      for (int z = 0; z < (int)ev.d; ++z)
        if (unite_canon(first, (i32)ev_fid[e * 11 + (size_t)(ev.q + z)])) ++r.fusions;
    }
    ptick(2);
    post_list.clear();
    const auto post_of = [&](i32 rt) -> ComponentDelta& {
      FidState& fr = st[(size_t)rt];
      if (fr.post_epoch != (u32)b) {
        fr.post_epoch = (u32)b;
        fr.post_slot = (u32)post_list.size();
        post_list.push_back(rt);
        if (scratch.size() < post_list.size()) scratch.emplace_back();
        ComponentDelta& cd = scratch[post_list.size() - 1];
        cd.parents.clear();
        cd.born.clear();
        return cd;
      }
      return scratch[fr.post_slot];
    };
    for (const i32 pr : pre_list)
      post_of(find(pr)).parents.push_back(abl_cle_factice ? cle_factice : keys[st[(size_t)pr].pre_canon]);
    for (const u32 fid : touched) {
      const u8 bits = st[(size_t)fid].role_bits;
      if ((bits & kAttach) && !(bits & kActive))
        post_of(find((i32)fid)).born.push_back(abl_cle_factice ? cle_factice : keys[fid]);
    }
    ptick(3);
    std::sort(post_list.begin(), post_list.end());
    for (const i32 rt : post_list) {
      ComponentDelta& cd = scratch[st[(size_t)rt].post_slot];
      if (!abl_sans_tris) {
        std::sort(cd.parents.begin(), cd.parents.end());
        std::sort(cd.born.begin(), cd.born.end());
      }
      if (cd.parents.size() >= 2) ++r.nodes;
      if (cd.parents.size() == 1 && cd.born.empty()) continue;  // continuation
      if (m_drop_nonmerge && cd.parents.size() < 2) continue;
      cd.batch = (u64)b;
      cd.level = evt(e0).level;
      cd.output = keys[st[(size_t)find(rt)].canon];
      if (!abl_sans_copie)
        r.deltas.push_back(cd);  // copie : le deplacement (mesure) ne fait que deplacer les allocations vers le scratch
    }
    r.batch_levels.push_back(evt(e0).level);
    for (const u32 fid : touched) st[(size_t)fid].seen = 1;
    ++r.batches;
    ptick(4);
#if defined(MHGP6_PROFILE_REDUCE) && defined(MHGP6_PROFILE_LIVENESS)
    // DEUX PHASES (§ 5.10 : l'ancien releve post-extinction pouvait publier
    // zero sur un lot dont chaque facette avait son dernier contact, malgre
    // un pic intra-lot eleve) : activer d'abord TOUS les fids du lot,
    // relever le pic INTRA-LOT, puis decrementer les derniers contacts et
    // relever la FRONTIERE inter-lots — les deux valeurs sont publiees.
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      for (int t = 0; t < (int)ev.q + (int)ev.d; ++t) {
        const u32 fid = ev_fid[e * 11 + (size_t)t];
        if (!alive_flag[fid]) { alive_flag[fid] = 1; ++live; }
      }
    }
    live_peak_intra = std::max(live_peak_intra, live);
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      for (int t = 0; t < (int)ev.q + (int)ev.d; ++t) {
        const u32 fid = ev_fid[e * 11 + (size_t)t];
        if (--remaining[fid] == 0) { alive_flag[fid] = 2; --live; }
      }
    }
    live_frontier_max = std::max(live_frontier_max, live);
    live_sum += live;
    ptick(5);  // bookkeeping de la sonde : isole, ne fuit plus dans le pt[0] suivant
#endif
  }
  mark(&r.t_reduce_ms);
#ifdef MHGP6_PROFILE_REDUCE
  pm = std::chrono::steady_clock::now();  // re-armement post-mark (plus aucune impression ici)
#endif
  r.final_canon_fid.resize(nfid);
  for (size_t fid = 0; fid < nfid; ++fid) {
    const u32 c = st[(size_t)find((i32)fid)].canon;
    r.final_canon_fid[fid] = c;
    if (c > (u32)fid || r.final_canon_fid[(size_t)c] != c) ++r.partition_violations;
  }
  for (size_t fid = 1; fid < nfid; ++fid)
    if (!(keys[fid - 1] < keys[fid])) ++r.partition_violations;
  ptick(7);  // partition finale
  std::vector<u32>().swap(ev_fid);
  std::vector<FidState>().swap(st);
  ptick(8);  // liberation des arenes (hors chrono reduce, mesuree quand meme)
  r.facet_keys = std::move(keys);
  mark(&r.t_partition_ms);
#ifdef MHGP6_PROFILE_REDUCE
  // § 5.10 : AUCUNE impression ici — le record est rempli apres l'arret de
  // tous les chronometres et imprime par run.hpp a la publication ordonnee
  // (avec K, somme, residuel, intervalles debut/fin, inflight et pic).
  r.profile.touch_ms = pt[0];
  r.profile.pre_ms = pt[1];
  r.profile.unite_ms = pt[2];
  r.profile.post_remplissage_ms = pt[3];
  r.profile.materialisation_tri_copie_ms = pt[4];
  r.profile.liveness_ms = pt[5];
  r.profile.init_ms = pt[6];
  r.profile.partition_ms = pt[7];
  r.profile.liberation_ms = pt[8];
  r.profile.end = std::chrono::steady_clock::now();
#ifdef MHGP6_PROFILE_LIVENESS
  r.profile.live_peak_intra = live_peak_intra;
  r.profile.live_frontier_max = live_frontier_max;
  r.profile.live_frontier_mean_pct =
      nfid && !batches.empty() ? 100.0 * (double)live_sum / ((double)nfid * (double)batches.size()) : 0.0;
#endif
#endif
  return r;
}

// Le fold complet, sequentiel de bout en bout du point de vue de l'appelant.
inline ForestResult build_forest(const std::vector<ForestEvent>& events, int threads = 1) {
  return reduce_fold(prepare_fold(events, threads));
}

}  // namespace mhgp6
