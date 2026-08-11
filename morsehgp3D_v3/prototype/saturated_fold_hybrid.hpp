// MorseHGP3D v3 — LE FOLD HYBRIDE PRODUIT (note solution hybride + note
// certificat par miniboules supprimees).
//
// La forme produit sans P_post ni enumeration des faces : deux chemins, juges
// par l'oracle face-owner.
//
//   1. FAST PATH principal-support. Le certificat (note miniboules) : u
//      appartient a TOUT support de B ssi la miniboule de M prive de u est
//      STRICTEMENT sous B — verifie ici avec la primitive exacte
//      `mhgp::miniball_of`, sans LP ni reconstruction de coquille. Sous ce
//      bit, le theoreme des q attaches donne : pour l'ordre k, les racines
//      distinctes des q carriers Sat(S_u), S_u = (U \ {u}) ∪ T avec T les
//      k-q+1 plus petits identifiants de M \ U (T est LIBRE pour la
//      connexite ; ce choix canonique rend les carriers reproductibles), sont
//      EXACTEMENT les composantes strictes touchees par B. Au plus quatre
//      miniboules et quatre lookups par (generateur, ordre).
//   2. FALLBACK demand-driven sous coquille multi-support : trie canonique
//      des combinaisons de points de M, postings intersectees du plus rare au
//      plus frequent, coupure quand la liste ne contient plus aucune racine
//      exterieure nouvelle (l'intersection est le CERTIFICAT D'ABSENCE — les
//      incidents d'une extension sont inclus dans ceux du prefixe), carrier
//      unique par racine nouvelle aux feuilles de profondeur k.
//
// Les generateurs q > k+1 (non-evenement) sont REDONDANTS pour la connexite
// sous famille complete (theoreme 2 recu) : une seule attache par first_k(M)
// suffit — leurs faces sont deja une seule composante a leur niveau.
//
// LE LOOKUP EXACT d'une boule : le tuple brut Sphere{base,num,den} n'est PAS
// une cle (refutation f2e78fa). La cle canonique est le CENTRE REDUIT
// (C=base*den+num divise par pgcd(C, den), den>0) + l'egalite exacte de
// niveau `sphere_cmp_beta` — deux supports de la meme boule la partagent.
//
// CE FOLD EXIGE LA PRETENTION DE FAMILLE COMPLETE : le theoreme des q
// attaches, la reduction q>k+1 et le refus d'un lookup manquant n'ont pas de
// sens sur une sous-famille. Le mode partiel reste le join par postings.
#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "mhgp/miniball.hpp"
#include "prototype/prefix_index.hpp"
#include "prototype/saturated_fold.hpp"

namespace mhgp3v {

struct HybridReceipt {
  long long principal_generators = 0, fallback_generators = 0;
  long long redundant_generators = 0;   // q > k+1, une attache (somme sur k)
  long long certificates_verified = 0, certificates_failed = 0;
  long long fast_lookups_tried = 0, fast_lookups_found = 0;
  long long trie_nodes = 0, trie_cut_empty = 0, trie_cut_known = 0;
  long long trie_leaves = 0, postings_scanned = 0;
  long long attaches = 0, unions_attempted = 0, unions_done = 0;
  // Le fallback prefixe--prefixe (note index), quand il est actif.
  PrefixIndexReceipt prefix;
  bool identities_ok = false;
};

struct HybridMutants {
  bool force_principal = false;   // court-circuiter le certificat (refute !)
  bool raw_ball_key = false;      // cle brute base/num/den (refutee !)
  PrefixIndexMutants prefix;      // mutants du fallback prefixe--prefixe
};

// Le centre reduit : C = base*den + num, divise par pgcd(|C|, den), den > 0.
struct HybridBallKey {
  mhgp::i128 x = 0, y = 0, z = 0, den = 0;
  bool operator==(const HybridBallKey& other) const {
    return x == other.x && y == other.y && z == other.z && den == other.den;
  }
};

struct HybridBallKeyHash {
  std::size_t operator()(const HybridBallKey& key) const {
    const auto fold_in = [](std::size_t seed, mhgp::i128 value) {
      const unsigned long long low = (unsigned long long)(unsigned __int128)value;
      const unsigned long long high =
          (unsigned long long)(((unsigned __int128)value) >> 64);
      seed ^= low + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
      seed ^= high + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
      return seed;
    };
    std::size_t seed = 1469598103934665603ULL;
    seed = fold_in(seed, key.x);
    seed = fold_in(seed, key.y);
    seed = fold_in(seed, key.z);
    seed = fold_in(seed, key.den);
    return seed;
  }
};

inline mhgp::i128 hybrid_gcd(mhgp::i128 a, mhgp::i128 b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) {
    const mhgp::i128 r = a % b;
    a = b;
    b = r;
  }
  return a;
}

inline HybridBallKey hybrid_ball_key(const mhgp::Sphere& sphere, bool raw_mutant) {
  HybridBallKey key;
  if (raw_mutant) {
    // MUTANT : la representation relative brute — deux supports de la meme
    // boule donnent des cles differentes, le lookup manque ou double.
    key.x = (mhgp::i128)sphere.base.x * 1000000007 + sphere.nx;
    key.y = (mhgp::i128)sphere.base.y * 1000000007 + sphere.ny;
    key.z = (mhgp::i128)sphere.base.z * 1000000007 + sphere.nz;
    key.den = sphere.den;
    return key;
  }
  mhgp::i128 cx = (mhgp::i128)sphere.base.x * sphere.den + sphere.nx;
  mhgp::i128 cy = (mhgp::i128)sphere.base.y * sphere.den + sphere.ny;
  mhgp::i128 cz = (mhgp::i128)sphere.base.z * sphere.den + sphere.nz;
  mhgp::i128 den = sphere.den;
  mhgp::i128 g = hybrid_gcd(hybrid_gcd(hybrid_gcd(cx, cy), cz), den);
  if (g == 0) g = 1;
  key.x = cx / g;
  key.y = cy / g;
  key.z = cz / g;
  key.den = den / g;
  return key;
}

// L'ORDRE GLOBAL D'ACTIVATION et ses lots de niveau exact : tri par
// (sphere_cmp_beta, indice catalogue), lots = plages d'egalite exacte. La
// POSITION dans cet ordre est l'ActivationId v1 de la possession canonique.
inline void hybrid_level_batches(const mhgp::Catalogue& catalogue, std::vector<int>* by_level,
                                 std::vector<std::pair<std::size_t, std::size_t>>* batches) {
  const std::size_t count = catalogue.spheres.size();
  by_level->resize(count);
  for (std::size_t s = 0; s < count; ++s) (*by_level)[s] = (int)s;
  std::sort(by_level->begin(), by_level->end(), [&](int x, int y) {
    const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                        catalogue.spheres[(std::size_t)y].sph);
    if (c != 0) return c < 0;
    return x < y;
  });
  batches->clear();
  std::size_t cursor = 0;
  while (cursor < count) {
    std::size_t batch_end = cursor + 1;
    while (batch_end < count &&
           mhgp::sphere_cmp_beta(
               catalogue.spheres[(std::size_t)(*by_level)[cursor]].sph,
               catalogue.spheres[(std::size_t)(*by_level)[batch_end]].sph) == 0)
      ++batch_end;
    batches->push_back({cursor, batch_end});
    cursor = batch_end;
  }
}

// LE CERTIFICAT DE SUPPORT PRINCIPAL (note miniboules supprimees) : u
// obligatoire ssi miniball(M \ {u}) est STRICTEMENT sous B ; U principal ssi
// tous ses points sont obligatoires. Partage entre le fold et la sonde de
// masse pour que le masque hybride de la sonde soit CELUI du fold, pas une
// reimplementation divergente.
template <class PointArray>
inline void hybrid_principal_certificates(const PointArray& pts, const mhgp::Catalogue& catalogue,
                                          const std::vector<std::vector<mhgp::i32>>& members,
                                          bool force_principal, std::vector<char>* principal,
                                          long long* verified, long long* failed) {
  const std::size_t count = catalogue.spheres.size();
  principal->assign(count, 0);
  std::vector<mhgp::i32> scratch;
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = catalogue.spheres[s];
    if (force_principal) {   // MUTANT : refute par la cosphere
      (*principal)[s] = 1;
      continue;
    }
    if ((int)members[s].size() <= (int)sphere.n_support) {
      // M == U (coquille = support) : aucun support alternatif possible.
      (*principal)[s] = 1;
      ++(*verified);
      continue;
    }
    bool all_obligatory = true;
    for (int u = 0; u < (int)sphere.n_support && all_obligatory; ++u) {
      scratch.clear();
      for (mhgp::i32 x : members[s])
        if (x != sphere.support[u]) scratch.push_back(x);
      const mhgp::MiniballResult removed =
          mhgp::miniball_of(pts, scratch.data(), (int)scratch.size());
      if (!removed.ok ||
          mhgp::sphere_cmp_beta(removed.sph, sphere.sph) >= 0)
        all_obligatory = false;
    }
    (*principal)[s] = all_obligatory ? 1 : 0;
    if (all_obligatory)
      ++(*verified);
    else
      ++(*failed);
  }
}

// LA DECISION DE REQUETE du fold hybride, partagee avec la sonde : un
// generateur est une requete fallback ssi il n'est ni redondant (q > k+1 en
// lot solo), ni une naissance rang k, ni un fast principal en lot solo. La
// meme chaine, dans le meme ordre, que le corps du fold.
inline bool hybrid_is_fallback_query(int rank, int k, int support_size, bool is_principal,
                                     bool solo_batch, bool force_principal) {
  if (support_size > k + 1 && (solo_batch || force_principal)) return false;
  if (rank == k) return false;
  if ((is_principal && solo_batch) || force_principal) return false;
  return true;
}

// `prefix_fallback` remplace le fallback demand-driven par l'index exact
// prefixe--prefixe de la note : memes attaches semantiques (unir M a toute
// racine d'un N partageant au moins k points), candidats par prefixes
// r-k+1 sous l'ordre des identifiants puis recertification |M∩N| >= k —
// aucun trie de faces, aucun scan des postings complets. La possession
// canonique (en-tete de prefix_index.hpp) garde chaque paire une fois.
// `prefix_pair_ledger` est une CAPABILITY DE TEST (reponse auditeur Q2) : le
// multiensemble canonique (ordre, lot, min(GeneratorId), max(GeneratorId))
// des paires possedees et recertifiees AVANT toute union DSU — l'idempotence
// des unions rend deux directions indiscernables, le ledger non. Il ne doit
// jamais devenir un flux persistant du produit.
using HybridPairLedger = std::vector<std::array<int, 4>>;

// `factorise_exaequo` (reponse auditeur 20260811, Q3 « factorisation stricte
// des ex aequo ») : sous la PRETENTION DE FAMILLE COMPLETE et l'unicite du
// handle par boule exacte, deux generateurs canoniques distincts M != N de
// MEME niveau exact avec |M∩N| >= k possedent un carrier strict commun
// Sat(F), F ⊆ M∩N de taille k : l'egalite beta(F) = alpha ferait de B_M et
// B_N deux boules minimales couvrant F, donc la MEME boule par unicite de la
// miniboule — contradiction avec la distinction des handles. Aucune arete
// nouveau--nouveau du lot n'est alors essentielle : chaque nouveau se relie
// aux racines STRICTES gelees qu'il touche (le carrier est un generateur
// anterieur avec |M ∩ Sat(F)| >= k, donc trouve par la requete prefixe sur
// les lots anterieurs), et le quotient biparti nouveaux--racines strictes
// ferme exactement le lot. Le mode saute donc TOUS les candidats du lot
// courant ; la source shallow seule ne donne PAS cette fermeture (fixture
// des deux triangles prives du carrier AB) — le differentiel des formes est
// le juge en campagne bornee, le contrat Gate D l'est a l'echelle.
//
// `prefix_all` (reponse auditeur 20260811, ordre recommande §2) : le join
// prefixe RELATIF a toute `GeneratorTable` recue — toutes les requetes, aucun
// certificat principal, aucun theoreme des q attaches, donc AUCUNE pretention
// de famille complete. Il separe la correction du join de la completude de la
// source : exact relativement a la table, jamais un claim sur l'amont.
template <class PointArray>
inline SaturatedFold build_saturated_fold_hybrid(
    const PointArray& pts, int point_count, const mhgp::Catalogue& catalogue,
    int maximum_order, bool keep_partitions, HybridReceipt* receipt,
    bool enforce_event_guard = false, HybridMutants mutants = {},
    bool prefix_fallback = false, HybridPairLedger* prefix_pair_ledger = nullptr,
    bool prefix_all = false, bool factorise_exaequo = false) {
  SaturatedFold fold;
  fold.maximum_order = maximum_order;
  if (receipt != nullptr) *receipt = HybridReceipt{};
  if (maximum_order < 1 || maximum_order > mhgp::kMaxRank) {
    fold.refusal = "ordre maximal hors contrat";
    return fold;
  }
  if (prefix_all && !prefix_fallback) {
    fold.refusal = "prefix-all exige le fallback prefixe";
    return fold;
  }
  if (factorise_exaequo && (!prefix_fallback || prefix_all)) {
    fold.refusal = "la factorisation des ex aequo exige le fallback prefixe sous"
                   " pretention de famille complete";
    return fold;
  }
  const std::size_t count = catalogue.spheres.size();

  // 1. MEMBRES et SUPPORTS verifies — identifiants BRUTS conserves (ils
  // indexent les points pour les miniboules exactes), bornes par le nuage.
  std::vector<std::vector<mhgp::i32>> members(count);
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = catalogue.spheres[s];
    if (sphere.members_begin < 0 || sphere.rank < 0 ||
        (std::size_t)sphere.members_begin + (std::size_t)sphere.rank >
            catalogue.members.size()) {
      fold.refusal = "tranche de pool hors catalogue";
      return fold;
    }
    members[s].assign(catalogue.members.begin() + sphere.members_begin,
                      catalogue.members.begin() + sphere.members_begin + sphere.rank);
    for (std::size_t t = 0; t < members[s].size(); ++t) {
      if (members[s][t] < 0 || members[s][t] >= point_count) {
        fold.refusal = "membre hors du nuage";
        return fold;
      }
      if (t > 0 && members[s][t - 1] >= members[s][t]) {
        fold.refusal = "membres non tries ou dupliques";
        return fold;
      }
    }
    const int support_count = (int)sphere.n_support;
    if (support_count < 1 ||
        support_count > std::min(mhgp::kMaxSupport, (int)sphere.rank)) {
      fold.refusal = "support hors contrat : cardinal invalide";
      return fold;
    }
    for (int u = 0; u < support_count; ++u) {
      if (u > 0 && sphere.support[u - 1] >= sphere.support[u]) {
        fold.refusal = "support hors contrat : non trie ou duplique";
        return fold;
      }
      if (!std::binary_search(members[s].begin(), members[s].end(), sphere.support[u])) {
        fold.refusal = "support hors contrat : point hors des membres";
        return fold;
      }
    }
  }

  // 2. LOTS par niveau exact, et INDEX DE BOULES par cle canonique : centre
  // reduit -> candidats, egalite finale par `sphere_cmp_beta` exact.
  // L'ActivationId v1 de la possession canonique est la POSITION dans
  // l'ordre global — jamais un numero de racine DSU.
  std::vector<int> by_level;
  std::vector<std::pair<std::size_t, std::size_t>> batches;
  hybrid_level_batches(catalogue, &by_level, &batches);
  std::vector<int> activation_of((std::size_t)count);
  for (std::size_t i = 0; i < count; ++i)
    activation_of[(std::size_t)by_level[i]] = (int)i;
  HybridReceipt out;
  std::unordered_map<HybridBallKey, std::vector<int>, HybridBallKeyHash> ball_index;
  for (std::size_t s = 0; s < count; ++s)
    ball_index[hybrid_ball_key(catalogue.spheres[s].sph, mutants.raw_ball_key)]
        .push_back((int)s);
  const auto lookup_ball = [&](const mhgp::Sphere& sphere) -> int {
    ++out.fast_lookups_tried;
    const auto bucket = ball_index.find(hybrid_ball_key(sphere, mutants.raw_ball_key));
    if (bucket == ball_index.end()) return -1;
    for (int candidate : bucket->second)
      if (mhgp::sphere_cmp_beta(sphere, catalogue.spheres[(std::size_t)candidate].sph) == 0) {
        ++out.fast_lookups_found;
        return candidate;
      }
    return -1;
  };

  // 3. LE CERTIFICAT DE SUPPORT PRINCIPAL — l'aide partagee avec la sonde de
  // masse : primitives exactes existantes, memes compteurs. En mode
  // prefix-all il n'y a ni fast path ni redondants : aucune miniboule payee.
  std::vector<char> principal;
  if (prefix_all)
    principal.assign(count, 0);
  else
    hybrid_principal_certificates(pts, catalogue, members, mutants.force_principal, &principal,
                                  &out.certificates_verified, &out.certificates_failed);

  // 4. UN ORDRE A LA FOIS : attaches (fast path, redondants, fallback) puis
  // rejeu du lot — capture d'epoque, marquage q_min, records par temoin,
  // identiques aux quatre formes recues.
  fold.orders.resize((std::size_t)maximum_order);
  const int K = maximum_order;
  struct OrderState {
    std::vector<int> parent;
    std::vector<long long> node_of_root;
    std::vector<std::set<mhgp::i32>> coverage;
    std::set<int> live_roots;
    std::vector<long long> touch_epoch;
    std::vector<long long> staged_node;
    std::vector<std::size_t> staged_cov;
    std::vector<std::vector<mhgp::i32>> witness;
    std::vector<std::vector<mhgp::i32>> staged_witness;
    long long next_node = 0;
    int find(int a) {
      while (parent[(std::size_t)a] != a) {
        parent[(std::size_t)a] = parent[(std::size_t)parent[(std::size_t)a]];
        a = parent[(std::size_t)a];
      }
      return a;
    }
  };
  for (int k = 1; k <= K; ++k) {
    SaturatedOrderFold& order = fold.orders[(std::size_t)(k - 1)];
    OrderState st;
    st.parent.resize(count);
    for (std::size_t s = 0; s < count; ++s) st.parent[s] = (int)s;
    st.node_of_root.assign(count, -1);
    st.coverage.resize(count);
    st.touch_epoch.assign(count, -1);
    st.staged_node.assign(count, -1);
    st.staged_cov.assign(count, 0);
    st.witness.resize(count);
    st.staged_witness.resize(count);
    // Postings par point pour le fallback demand-driven, OU index
    // prefixe--prefixe : generateurs actifs (rang >= k), le lot courant STAGE
    // avant toute recherche, publies a la fermeture. `staged_epoch` grave le
    // lot logique de chaque generateur et `is_query_now` le masque de
    // requetes du lot courant — les deux ingredients de la possession.
    std::vector<std::vector<int>> postings;
    PrefixIndex prefix_index;
    std::vector<long long> staged_epoch;
    std::vector<char> is_query_now;
    if (prefix_fallback) {
      prefix_index.reset(point_count);
      staged_epoch.assign(count, -1);
      is_query_now.assign(count, 0);
      if (mutants.prefix.future_visible) {
        // MUTANT : les lots futurs sont visibles d'avance — les requetes
        // unissent trop tot, le differentiel des formes le voit.
        for (std::size_t s = 0; s < count; ++s)
          if ((int)members[s].size() >= k)
            out.prefix.entries +=
                prefix_stage(&prefix_index, (int)s, members[s], k, mutants.prefix);
      }
    } else {
      postings.assign((std::size_t)point_count, {});
    }

    for (std::size_t batch = 0; batch < batches.size(); ++batch) {
      const long long epoch = (long long)batch + 1;
      const bool solo_batch = batches[batch].second - batches[batch].first == 1;
      const auto touch = [&](int root) {
        if (st.touch_epoch[(std::size_t)root] != epoch) {
          st.touch_epoch[(std::size_t)root] = epoch;
          st.staged_node[(std::size_t)root] = st.node_of_root[(std::size_t)root];
          st.staged_cov[(std::size_t)root] = st.coverage[(std::size_t)root].size();
          st.staged_witness[(std::size_t)root] = st.witness[(std::size_t)root];
        }
      };
      std::vector<int> touched;
      std::vector<std::pair<int, int>> events;
      std::vector<int> batch_generators;
      for (std::size_t b = batches[batch].first; b < batches[batch].second; ++b) {
        const int m = by_level[b];
        if ((int)members[(std::size_t)m].size() < k) continue;
        const int support_size = (int)catalogue.spheres[(std::size_t)m].n_support;
        touch(m);
        st.coverage[(std::size_t)m].insert(members[(std::size_t)m].begin(),
                                           members[(std::size_t)m].end());
        st.witness[(std::size_t)m].assign(members[(std::size_t)m].begin(),
                                          members[(std::size_t)m].begin() + k);
        st.live_roots.insert(m);
        touched.push_back(m);
        batch_generators.push_back(m);
        if (support_size <= k + 1) events.push_back({m, support_size});
        // STAGING : le lot entier est visible avant la premiere recherche —
        // les generateurs futurs restent invisibles (contrat de la note). Le
        // masque de requetes du lot est fige ICI, avant tout travail.
        if (prefix_fallback) {
          staged_epoch[(std::size_t)m] = epoch;
          is_query_now[(std::size_t)m] =
              prefix_all
                  ? ((int)members[(std::size_t)m].size() > k ? 1 : 0)
                  : (hybrid_is_fallback_query((int)members[(std::size_t)m].size(), k,
                                              support_size, principal[(std::size_t)m] != 0,
                                              solo_batch, mutants.force_principal)
                         ? 1
                         : 0);
          // FACTORISATION DES EX AEQUO : le lot est stage a sa CLOTURE — les
          // requetes du lot ne lisent que les lots anterieurs, aucun candidat
          // nouveau--nouveau n'apparait (le carrier strict fait la connexite).
          if (!mutants.prefix.future_visible && !mutants.prefix.stage_query_sequentially &&
              !factorise_exaequo)
            out.prefix.entries +=
                prefix_stage(&prefix_index, m, members[(std::size_t)m], k, mutants.prefix);
        } else {
          for (mhgp::i32 x : members[(std::size_t)m]) postings[(std::size_t)x].push_back(m);
        }
      }
      // PREFLIGHT H_Q (audit 8df7ac8) : les degres sont figes a la cloture du
      // staging du lot ; la somme q_x*d_x est la masse EXACTE que les requetes
      // du lot liront. L'identite contre `hits` est un invariant a refus.
      if (prefix_fallback)
        for (int m : batch_generators)
          if (is_query_now[(std::size_t)m] != 0)
            out.prefix.predicted_hits += prefix_predicted_hits(
                prefix_index, members[(std::size_t)m], k, mutants.prefix);

      const auto unite = [&](int a, int b) {
        ++out.unions_attempted;
        int ra = st.find(a), rb = st.find(b);
        touch(ra);
        touch(rb);
        if (ra == rb) return;
        ++out.unions_done;
        if (st.coverage[(std::size_t)ra].size() > st.coverage[(std::size_t)rb].size())
          std::swap(ra, rb);
        st.coverage[(std::size_t)rb].insert(st.coverage[(std::size_t)ra].begin(),
                                            st.coverage[(std::size_t)ra].end());
        std::set<mhgp::i32>().swap(st.coverage[(std::size_t)ra]);
        st.parent[(std::size_t)ra] = rb;
        if (st.witness[(std::size_t)ra] < st.witness[(std::size_t)rb])
          st.witness[(std::size_t)rb] = st.witness[(std::size_t)ra];
        st.live_roots.erase(ra);
        touched.push_back(rb);
        touched.push_back(ra);
      };

      // PRUDENCE D'EX AEQUO : le theoreme des q attaches ne parle que des
      // composantes STRICTES ; dans un lot a plusieurs generateurs, une face
      // partagee peut avoir son carrier DANS le lot, hors des completions de
      // support. Tant que ce cas n'est pas recu separement, les lots d'ex
      // aequo passent au fallback exact — le generique n'en a presque pas,
      // la grille saturee en a partout et le fallback y est deja requis.
      for (int m : batch_generators) {
        if (prefix_fallback && mutants.prefix.stage_query_sequentially)
          // MUTANT : stager au tour de traitement au lieu du lot entier —
          // une requete precoce perd ses candidats tardifs du meme lot, et
          // le preflight fige a la cloture du staging ne colle plus.
          out.prefix.entries +=
              prefix_stage(&prefix_index, m, members[(std::size_t)m], k, mutants.prefix);
        const mhgp::CriticalSphere& sphere = catalogue.spheres[(std::size_t)m];
        const int q = (int)sphere.n_support;
        const int rank = (int)members[(std::size_t)m].size();
        if (!prefix_all && q > k + 1 && (solo_batch || mutants.force_principal)) {
          // REDONDANT (theoreme 2, famille complete) : ses k-faces sont deja
          // UNE composante a son niveau — une attache par first_k suffit.
          ++out.redundant_generators;
          const mhgp::MiniballResult carrier_ball = mhgp::miniball_of(
              pts, members[(std::size_t)m].data(), k);
          const int carrier = carrier_ball.ok ? lookup_ball(carrier_ball.sph) : -1;
          if (carrier < 0) {
            fold.refusal = "lookup manquant sous pretention de famille complete";
            return fold;
          }
          ++out.attaches;
          unite(m, carrier);
          continue;
        }
        if (rank == k) {
          // L'unique k-face est M : naissance sans lookup strict (theoreme).
          ++out.principal_generators;
          continue;
        }
        if (!prefix_all &&
            ((principal[(std::size_t)m] != 0 && solo_batch) || mutants.force_principal)) {
          // FAST PATH : les q attaches S_u = (U \ {u}) ∪ T.
          ++out.principal_generators;
          // T = les k-q+1 plus petits identifiants de M \ U — VIDE pour la
          // coface q = k+1 (le test de taille se fait AVANT de pousser : la
          // cible zero ne doit collecter personne).
          std::vector<mhgp::i32> tail;
          for (mhgp::i32 x : members[(std::size_t)m]) {
            if ((int)tail.size() >= k - q + 1) break;
            bool in_support = false;
            for (int u = 0; u < q; ++u)
              if (sphere.support[u] == x) { in_support = true; break; }
            if (!in_support) tail.push_back(x);
          }
          for (int u = 0; u < q; ++u) {
            std::vector<mhgp::i32> face;
            for (int v = 0; v < q; ++v)
              if (v != u) face.push_back(sphere.support[v]);
            face.insert(face.end(), tail.begin(), tail.end());
            if ((int)face.size() != k) continue;   // q = k+1 et T vide : taille k
            std::sort(face.begin(), face.end());
            const mhgp::MiniballResult carrier_ball =
                mhgp::miniball_of(pts, face.data(), (int)face.size());
            const int carrier = carrier_ball.ok ? lookup_ball(carrier_ball.sph) : -1;
            if (carrier < 0) {
              fold.refusal = "lookup manquant sous pretention de famille complete";
              return fold;
            }
            ++out.attaches;
            unite(m, carrier);
          }
          continue;
        }
        ++out.fallback_generators;
        if (prefix_fallback) {
          // FALLBACK PREFIXE--PREFIXE (note index) : candidats par prefixes
          // r-k+1 sous l'ordre des identifiants, recertification exacte
          // |M∩N| >= k sur les MEMBRES DU GENERATEUR — jamais sur une
          // projection DSU (le mutant project-root-first grave ce piege) —
          // puis union ; les doublons de composante sont idempotents.
          if (is_query_now[(std::size_t)m] == 0) {
            fold.refusal = "masque de requetes incoherent avec la decision du fold";
            return fold;
          }
          std::vector<int> candidates;
          prefix_query(prefix_index, members[(std::size_t)m], k, &candidates, &out.prefix,
                       mutants.prefix);
          for (int candidate : candidates) {
            if (candidate == m) continue;
            // POSSESSION CANONIQUE : M garde N ssi N est d'un lot anterieur,
            // ou non-requete du lot courant, ou ActivationId(N) < ActivationId(M).
            // Une paire Q--Q est ainsi possedee UNE fois par l'ActivationId le
            // plus grand ; Q--R reste a la requete meme si le R est posterieur.
            if (!mutants.prefix.double_query_pair &&
                staged_epoch[(std::size_t)candidate] == epoch &&
                is_query_now[(std::size_t)candidate] != 0 &&
                activation_of[(std::size_t)candidate] > activation_of[(std::size_t)m])
              continue;
            ++out.prefix.candidate_pairs_after_filter;
            bool certified;
            if (mutants.prefix.skip_recertification) {
              certified = true;
            } else if (mutants.prefix.project_root_first) {
              // MUTANT : compter l'intersection contre la COUVERTURE de la
              // racine — des membres issus separement de plusieurs
              // generateurs d'une meme racine certifient a tort.
              const std::set<mhgp::i32>& cover =
                  st.coverage[(std::size_t)st.find(candidate)];
              int common = 0;
              for (mhgp::i32 x : members[(std::size_t)m])
                if (cover.count(x) != 0 && ++common >= k) break;
              certified = common >= k;
            } else {
              certified = prefix_recertify(members[(std::size_t)m],
                                           members[(std::size_t)candidate], k);
            }
            if (!certified) {
              ++out.prefix.false_candidates;
              continue;
            }
            ++out.prefix.recertified_true;
            if (prefix_pair_ledger != nullptr)
              prefix_pair_ledger->push_back({k, (int)batch, std::min(m, candidate),
                                             std::max(m, candidate)});
            ++out.attaches;
            unite(m, candidate);
          }
          continue;
        }
        // FALLBACK demand-driven : trie canonique des combinaisons de M,
        // postings intersectees du plus rare au plus frequent, coupure par
        // certificat d'absence, un carrier par racine nouvelle aux feuilles.
        std::vector<mhgp::i32> ordered = members[(std::size_t)m];
        std::sort(ordered.begin(), ordered.end(), [&](mhgp::i32 a, mhgp::i32 b) {
          const std::size_t pa = postings[(std::size_t)a].size();
          const std::size_t pb = postings[(std::size_t)b].size();
          if (pa != pb) return pa < pb;
          return a < b;
        });
        // Toute attache unit dans LA composante de m : une racine est
        // nouvelle ssi elle differe de find(m) — aucune liste d'atteintes a
        // entretenir, les racines bougeant a chaque union.
        struct Frame {
          std::size_t next_point;
          std::vector<int> incident;
        };
        const auto roots_outside = [&](const std::vector<int>& incident) {
          std::set<int> fresh;
          const int mine = st.find(m);
          for (int candidate : incident) {
            const int root = st.find(candidate);
            if (root != mine) fresh.insert(root);
          }
          return fresh;
        };
        std::vector<std::pair<Frame, int>> stack;   // (cadre, profondeur)
        {
          Frame root_frame;
          root_frame.next_point = 0;
          stack.push_back({std::move(root_frame), 0});
        }
        while (!stack.empty()) {
          Frame frame = std::move(stack.back().first);
          const int depth = stack.back().second;
          stack.pop_back();
          for (std::size_t p = frame.next_point; p < ordered.size(); ++p) {
            if ((int)(ordered.size() - p) < k - depth) break;
            ++out.trie_nodes;
            std::vector<int> narrowed;
            if (depth == 0) {
              // Les postings sont appendues en ordre d'activation, pas d'
              // identifiant : trier UNE fois a la racine du trie rend toutes
              // les intersections descendantes valides.
              narrowed = postings[(std::size_t)ordered[p]];
              std::sort(narrowed.begin(), narrowed.end());
              out.postings_scanned += (long long)narrowed.size();
            } else {
              std::vector<int> sorted_px = postings[(std::size_t)ordered[p]];
              std::sort(sorted_px.begin(), sorted_px.end());
              out.postings_scanned += (long long)sorted_px.size();
              std::set_intersection(frame.incident.begin(), frame.incident.end(),
                                    sorted_px.begin(), sorted_px.end(),
                                    std::back_inserter(narrowed));
            }
            if (narrowed.empty()) {
              ++out.trie_cut_empty;
              continue;
            }
            std::set<int> fresh = roots_outside(narrowed);
            if (fresh.empty()) {
              ++out.trie_cut_known;
              continue;
            }
            if (depth + 1 == k) {
              ++out.trie_leaves;
              for (int root : fresh) {
                ++out.attaches;
                unite(m, root);
              }
              // Les composantes unies sont absorbees par find(m) : les
              // prefixes suivants ne verront que des racines encore neuves.
              continue;
            }
            Frame child;
            child.next_point = p + 1;
            child.incident = std::move(narrowed);
            stack.push_back({std::move(child), depth + 1});
          }
        }
      }

      if (prefix_fallback) {
        if (factorise_exaequo && !mutants.prefix.future_visible &&
            !mutants.prefix.stage_query_sequentially)
          for (int m : batch_generators)
            out.prefix.entries +=
                prefix_stage(&prefix_index, m, members[(std::size_t)m], k, mutants.prefix);
        for (int m : batch_generators) is_query_now[(std::size_t)m] = 0;
      }
      if (touched.empty()) continue;
      std::map<int, std::set<long long>> strict_of;
      std::map<int, std::size_t> strict_cov_of;
      std::map<int, std::vector<std::vector<mhgp::i32>>> strict_witnesses_of;
      std::set<int> finals;
      for (int r : touched) {
        const int final_root = st.find(r);
        finals.insert(final_root);
        if (st.touch_epoch[(std::size_t)r] == epoch && st.staged_node[(std::size_t)r] >= 0) {
          strict_of[final_root].insert(st.staged_node[(std::size_t)r]);
          strict_witnesses_of[final_root].push_back(st.staged_witness[(std::size_t)r]);
          strict_cov_of.emplace(final_root, st.staged_cov[(std::size_t)r]);
        }
      }
      for (int root : finals) {
        const auto it = strict_of.find(root);
        const std::size_t strict = it == strict_of.end() ? 0 : it->second.size();
        if (strict == 0) {
          ++order.births;
          st.node_of_root[(std::size_t)root] = st.next_node++;
        } else if (strict == 1) {
          const std::size_t before = strict_cov_of[root];
          if (st.coverage[(std::size_t)root].size() > before)
            ++order.coverage_growth_batches;
          else
            ++order.silent_generator_batches;
          st.node_of_root[(std::size_t)root] = *it->second.begin();
        } else {
          ++order.fusions;
          st.node_of_root[(std::size_t)root] = st.next_node++;
        }
      }
      std::map<int, int> marked_minimum_support;
      std::map<int, std::vector<int>> marked_generators;
      for (const std::pair<int, int>& event : events) {
        const int root = st.find(event.first);
        const auto it = marked_minimum_support.find(root);
        if (it == marked_minimum_support.end())
          marked_minimum_support.emplace(root, event.second);
        else
          it->second = std::min(it->second, event.second);
        marked_generators[root].push_back(event.first);
      }
      long long births_here = 0, continuations_here = 0, multifusions_here = 0;
      std::vector<GammaEventRecord> batch_records;
      for (const auto& entry : marked_minimum_support) {
        const auto it = strict_of.find(entry.first);
        const std::size_t strict = it == strict_of.end() ? 0 : it->second.size();
        GammaEventRecord record;
        record.level_representative = by_level[batches[batch].first];
        record.closed_witness = st.witness[(std::size_t)entry.first];
        for (int marker : marked_generators[entry.first])
          record.marking_saturations.push_back(members[(std::size_t)marker]);
        std::sort(record.marking_saturations.begin(), record.marking_saturations.end());
        record.marking_saturations.erase(
            std::unique(record.marking_saturations.begin(), record.marking_saturations.end()),
            record.marking_saturations.end());
        const auto witnesses = strict_witnesses_of.find(entry.first);
        if (witnesses != strict_witnesses_of.end())
          for (const std::vector<mhgp::i32>& strict_witness : witnesses->second)
            record.strict_witnesses.push_back(strict_witness);
        std::sort(record.strict_witnesses.begin(), record.strict_witnesses.end());
        record.strict_witnesses.erase(
            std::unique(record.strict_witnesses.begin(), record.strict_witnesses.end()),
            record.strict_witnesses.end());
        if (strict == 0) {
          ++births_here;
          record.type = 0;
          if (entry.second > k) {
            ++order.event_guard_violations;
            if (enforce_event_guard) {
              fold.refusal = "garde d'evenement violee : naissance marquee sans support q<=k";
              return fold;
            }
          }
        } else if (strict == 1) {
          ++continuations_here;
          record.type = 1;
        } else {
          ++multifusions_here;
          record.type = 2;
        }
        batch_records.push_back(std::move(record));
      }
      std::sort(batch_records.begin(), batch_records.end(),
                [](const GammaEventRecord& a, const GammaEventRecord& b) {
                  return a.closed_witness < b.closed_witness;
                });
      order.gamma_records.insert(order.gamma_records.end(), batch_records.begin(),
                                 batch_records.end());
      order.gamma_births += births_here;
      order.gamma_continuations += continuations_here;
      order.gamma_multifusions += multifusions_here;
      order.level_representative.push_back(by_level[batches[batch].first]);
      order.gamma_birth_at_level.push_back(births_here);
      order.gamma_continuation_at_level.push_back(continuations_here);
      order.gamma_multifusion_at_level.push_back(multifusions_here);
      if (keep_partitions) {
        FoldPartition partition;
        for (int root : st.live_roots)
          partition.push_back(std::vector<mhgp::i32>(st.coverage[(std::size_t)root].begin(),
                                                     st.coverage[(std::size_t)root].end()));
        std::sort(partition.begin(), partition.end());
        order.closed_partitions.push_back(std::move(partition));
      }
    }
  }
  if (out.attaches != out.unions_attempted) {
    fold.refusal = "identite hybride violee : attaches != unions tentees";
    return fold;
  }
  // L'IDENTITE DE PREFLIGHT : les hits reellement lus doivent egaler la
  // prediction aux degres figes a la cloture du staging de chaque lot. Un
  // staging pendant la phase de requetes — le mutant sequentiel — la casse.
  if (prefix_fallback && out.prefix.predicted_hits != out.prefix.hits) {
    fold.refusal = "identite prefixe violee : hits lus != hits prevus au staging";
    return fold;
  }
  // L'IDENTITE DE MASSE (reponse auditeur Q3) : les entrees posees doivent
  // egaler exactement la somme L(r,K) des membres — duplicate-posting peut
  // respecter le preflight (la prediction relit le posting duplique), il
  // meurt ici.
  if (prefix_fallback) {
    long long expected_mass = 0;
    for (std::size_t s = 0; s < count; ++s) {
      const long long r = (long long)members[s].size();
      const long long m = std::min((long long)K, r);
      expected_mass += m * (r + 1) - m * (m + 1) / 2;
    }
    if (out.prefix.entries != expected_mass) {
      fold.refusal = "identite de masse violee : entrees != somme L(r,K)";
      return fold;
    }
  }
  out.identities_ok = true;
  if (receipt != nullptr) *receipt = out;
  fold.ok = true;
  return fold;
}

}  // namespace mhgp3v
