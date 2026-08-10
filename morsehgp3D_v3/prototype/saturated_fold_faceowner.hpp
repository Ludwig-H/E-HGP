// MorseHGP3D v3 — LE FOLD FACE-OWNER (theoreme de la reponse masse).
//
// La factorisation exacte qui sort du mur P_post : deux generateurs se
// touchent a l'ordre k ssi ils contiennent une meme SIGNATURE de k-face. Le
// graphe biparti generateurs--signatures a les memes composantes que H_k ;
// pour chaque signature F, une ETOILE centree sur owner_k(F) — l'incident de
// niveau exact minimal, tie-break canonique — remplace sa clique a TOUTES les
// coupes : l'owner est actif avant (ou dans le meme lot exact que) chaque
// autre incident, aucune fausse arete n'est creee (chaque branche partage
// reellement F), et le lot d'ex aequo reste atomique. La masse devient
// I = somme_k somme_M C(|M|, k) au lieu de P_post = somme_x C(d_x, 2) —
// mesuree 23,5x plus petite a n=200 (16,4 M contre 385,6 M).
//
// C'est un ORACLE BORNE (les signatures sont materialisees, k <= 6 et
// identifiants denses < 2^21 pour l'empaquetage u128) : la variante produit
// est la recherche a la demande avec coupure par prefixe de postings (§6 de
// la reponse), a differencier contre celui-ci. Le transcript par marquage
// q_min, les temoins de racines et les records sont CEUX DU FOLD RECU —
// memes structures, meme juge.
//
// Les mutants nommes par la porte minimale §7 vivent ici : owner non
// minimal, lot publie trop tot, signature omise, signature doublee, mauvais
// k, filtre q_min illicite en mode partiel. Chacun doit mourir par la porte
// differentielle ou une identite de recu.
#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "prototype/saturated_fold.hpp"

namespace mhgp3v {

struct FaceOwnerReceipt {
  // Par ordre k (indexe k-1) : la masse du profileur §7.1.
  std::vector<long long> generators_k;        // |M| >= k
  std::vector<long long> incidences_k;        // somme C(|M|, k)
  std::vector<long long> unique_signatures_k;
  std::vector<long long> multiplicity_one_k;  // signatures a un seul incident
  std::vector<long long> star_branches_k;     // incidences - signatures
  std::vector<long long> deduplicated_branches_k;   // aretes (owner, M) uniques
  long long incidences_total = 0;
  long long predicted_incidences = 0;   // preflight par binomiales, u128
  long long unions_attempted = 0, unions_done = 0;
  std::vector<long long> rank_histogram;      // [0..kMaxRank]
  bool identities_ok = false;
};

struct FaceOwnerMutants {
  bool owner_not_minimal = false;       // prendre l'incident MAXIMAL
  bool publish_batch_early = false;     // committer boule par boule (atomicite)
  bool omit_first_signature = false;    // premiere signature jamais emise
  bool duplicate_first_signature = false;   // premiere signature emise deux fois
  bool shift_k = false;                 // signatures de taille k-1 pour l'ordre k
  bool qmin_filter_partial = false;     // filtre Sigma_k SANS certificat
  // LE MUTANT DE LA REFUTATION COFACES (audit f2e78fa) : ne garder que les
  // signatures touchant le support canonique en >= q-1 points — sur la
  // cosphere u16 a dix points, il perd NEUF des dix-sept composantes strictes
  // des six-faces, pour n'importe quel support minimal choisi.
  bool support_facet_filter = false;
};

inline SaturatedFold build_saturated_fold_faceowner(
    const mhgp::Catalogue& catalogue, int maximum_order, bool keep_partitions,
    FaceOwnerReceipt* receipt, bool enforce_event_guard = false,
    FaceOwnerMutants mutants = {}, long long memory_budget_bytes = 0) {
  SaturatedFold fold;
  fold.maximum_order = maximum_order;
  if (receipt != nullptr) *receipt = FaceOwnerReceipt{};
  // ORACLE BORNE : k <= 6 pour l'empaquetage des signatures en u128.
  if (maximum_order < 1 || maximum_order > 6) {
    fold.refusal = "ordre maximal hors contrat de l'oracle face-owner";
    return fold;
  }
  const std::size_t count = catalogue.spheres.size();

  // 1. MEMBRES verifies puis COMPRIMES — contrats fail-closed identiques.
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
      if (members[s][t] < 0) {
        fold.refusal = "membre negatif : les signatures indexent par PointId";
        return fold;
      }
      if (t > 0 && members[s][t - 1] >= members[s][t]) {
        fold.refusal = "membres non tries ou dupliques";
        return fold;
      }
    }
  }
  std::vector<mhgp::i32> universe;
  for (const std::vector<mhgp::i32>& generator_members : members)
    universe.insert(universe.end(), generator_members.begin(), generator_members.end());
  std::sort(universe.begin(), universe.end());
  universe.erase(std::unique(universe.begin(), universe.end()), universe.end());
  if (universe.size() >= ((std::size_t)1 << 21)) {
    fold.refusal = "univers trop large pour l'empaquetage des signatures";
    return fold;
  }
  const auto dense_of = [&universe](mhgp::i32 raw) {
    return (mhgp::i32)(std::lower_bound(universe.begin(), universe.end(), raw) -
                       universe.begin());
  };
  for (std::vector<mhgp::i32>& generator_members : members)
    for (mhgp::i32& x : generator_members) x = dense_of(x);

  // 2. LOTS par niveau exact, et rang d'activation canonique (lot, indice).
  std::vector<int> by_level((std::size_t)count);
  for (std::size_t s = 0; s < count; ++s) by_level[s] = (int)s;
  std::sort(by_level.begin(), by_level.end(), [&](int x, int y) {
    const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                        catalogue.spheres[(std::size_t)y].sph);
    if (c != 0) return c < 0;
    return x < y;
  });
  std::vector<int> batch_of((std::size_t)count, 0);
  std::vector<int> activation_rank((std::size_t)count, 0);
  std::vector<std::pair<std::size_t, std::size_t>> batches;
  {
    std::size_t cursor = 0;
    while (cursor < count) {
      std::size_t batch_end = cursor + 1;
      if (!mutants.publish_batch_early)
        while (batch_end < count &&
               mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)by_level[cursor]].sph,
                                     catalogue.spheres[(std::size_t)by_level[batch_end]].sph) ==
                   0)
          ++batch_end;
      for (std::size_t b = cursor; b < batch_end; ++b) {
        batch_of[(std::size_t)by_level[b]] = (int)batches.size();
        activation_rank[(std::size_t)by_level[b]] = (int)b;
      }
      batches.push_back({cursor, batch_end});
      cursor = batch_end;
    }
  }

  // 3. PREFLIGHT : I par binomiales depuis les rangs, en u128, AVANT toute
  // emission — l'identite post-hoc I reel == predit est verifiee a la fin.
  FaceOwnerReceipt out;
  const int K = maximum_order;
  out.generators_k.assign((std::size_t)K, 0);
  out.incidences_k.assign((std::size_t)K, 0);
  out.unique_signatures_k.assign((std::size_t)K, 0);
  out.multiplicity_one_k.assign((std::size_t)K, 0);
  out.star_branches_k.assign((std::size_t)K, 0);
  out.deduplicated_branches_k.assign((std::size_t)K, 0);
  out.rank_histogram.assign((std::size_t)mhgp::kMaxRank + 1, 0);
  {
    using u128 = unsigned __int128;
    const u128 ceiling = (u128)1 << 62;
    u128 total = 0, heaviest_order = 0;
    for (std::size_t s = 0; s < count; ++s) {
      const long long rank = (long long)members[s].size();
      if (rank <= mhgp::kMaxRank) ++out.rank_histogram[(std::size_t)rank];
    }
    for (int k = 1; k <= K; ++k) {
      u128 order_mass = 0;
      for (std::size_t s = 0; s < count; ++s) {
        const long long rank = (long long)members[s].size();
        if (rank < k) continue;
        u128 binomial = 1;
        for (int i = 0; i < k; ++i) binomial = binomial * (u128)(rank - i) / (u128)(i + 1);
        order_mass += binomial;
        if (order_mass >= ceiling) {
          fold.refusal = "preflight : incidences debordent le contrat entier";
          return fold;
        }
      }
      total += order_mass;
      heaviest_order = std::max(heaviest_order, order_mass);
      if (total >= ceiling) {
        fold.refusal = "preflight : incidences debordent le contrat entier";
        return fold;
      }
    }
    out.predicted_incidences = (long long)total;
    // Pic conservateur : les ordres sont traites UN PAR UN — 24 octets par
    // incidence du plus lourd (clef u128 + generateur), plus etats et
    // couvertures comme les autres formes.
    long long mass = 0;
    for (const std::vector<mhgp::i32>& generator_members : members)
      mass += (long long)generator_members.size();
    const u128 peak = heaviest_order * 24 + (u128)mass * 8 +
                      (u128)count * 40 * (u128)K + (u128)mass * 64 * (u128)K;
    if (peak >= ceiling) {
      fold.refusal = "preflight : pic memoire predit deborde le contrat entier";
      return fold;
    }
    if (memory_budget_bytes > 0 && peak > (u128)memory_budget_bytes) {
      if (receipt != nullptr) *receipt = out;
      fold.refusal = "preflight : pic memoire predit au-dessus du budget";
      return fold;
    }
  }

  // 4. EMISSION-TRI-OWNER par ordre : signatures u128, owner = incident de
  // rang d'activation minimal, etoile dedupliquee, aretes par lot du membre.
  std::vector<std::vector<std::pair<std::pair<int, int>, int>>> star_edges(
      (std::size_t)K);   // ((lot d'activation, owner), membre)
  for (int k = 1; k <= K; ++k) {
    using u128 = unsigned __int128;
    const int signature_size = mutants.shift_k ? std::max(1, k - 1) : k;
    std::vector<std::pair<u128, int>> incidences;
    bool first_signature_seen = false;
    for (std::size_t s = 0; s < count; ++s) {
      if ((int)members[s].size() < k) continue;
      ++out.generators_k[(std::size_t)(k - 1)];
      // MUTANT qmin_filter_partial : appliquer Sigma_k sans certificat de
      // completude — illicite, la porte differentielle doit rougir.
      if (mutants.qmin_filter_partial &&
          (int)catalogue.spheres[s].n_support > k + 1)
        continue;
      const std::vector<mhgp::i32>& gen_members = members[s];
      // Support canonique en identifiants DENSES, pour le mutant refute.
      std::vector<mhgp::i32> dense_support;
      if (mutants.support_facet_filter)
        for (int u = 0; u < (int)catalogue.spheres[s].n_support; ++u)
          dense_support.push_back(dense_of(catalogue.spheres[s].support[u]));
      std::vector<int> chosen((std::size_t)signature_size, 0);
      // Enumeration canonique des sous-ensembles de taille signature_size.
      std::vector<std::size_t> index((std::size_t)signature_size);
      for (int i = 0; i < signature_size; ++i) index[(std::size_t)i] = (std::size_t)i;
      const std::size_t rank = gen_members.size();
      if ((std::size_t)signature_size > rank) continue;
      while (true) {
        u128 key = 0;
        for (int i = 0; i < signature_size; ++i)
          key = (key << 21) | (u128)(unsigned)gen_members[index[(std::size_t)i]];
        bool emit = true;
        if (mutants.support_facet_filter) {
          int support_hits = 0;
          for (int i = 0; i < signature_size; ++i)
            for (mhgp::i32 u : dense_support)
              if (gen_members[index[(std::size_t)i]] == u) { ++support_hits; break; }
          if (support_hits < (int)dense_support.size() - 1) emit = false;
        }
        if (mutants.omit_first_signature && !first_signature_seen) emit = false;
        if (emit) {
          incidences.push_back({key, (int)s});
          if (mutants.duplicate_first_signature && !first_signature_seen)
            incidences.push_back({key, (int)s});
        }
        first_signature_seen = true;
        // sous-ensemble suivant
        int position = signature_size - 1;
        while (position >= 0 &&
               index[(std::size_t)position] == rank - (std::size_t)(signature_size - position))
          --position;
        if (position < 0) break;
        ++index[(std::size_t)position];
        for (int i = position + 1; i < signature_size; ++i)
          index[(std::size_t)i] = index[(std::size_t)(i - 1)] + 1;
      }
      static_cast<void>(chosen);
    }
    out.incidences_k[(std::size_t)(k - 1)] = (long long)incidences.size();
    out.incidences_total += (long long)incidences.size();
    std::sort(incidences.begin(), incidences.end());
    // Groupes par signature : owner = rang d'activation minimal (tie-break
    // canonique par l'indice catalogue, deja dans activation_rank).
    std::vector<std::pair<std::pair<int, int>, int>> edges;
    for (std::size_t i = 0; i < incidences.size();) {
      std::size_t j = i;
      int owner = incidences[i].second;
      while (j < incidences.size() && incidences[j].first == incidences[i].first) {
        const int candidate = incidences[j].second;
        const bool better =
            mutants.owner_not_minimal
                ? activation_rank[(std::size_t)candidate] > activation_rank[(std::size_t)owner]
                : activation_rank[(std::size_t)candidate] < activation_rank[(std::size_t)owner];
        if (better) owner = candidate;
        ++j;
      }
      ++out.unique_signatures_k[(std::size_t)(k - 1)];
      if (j - i == 1) ++out.multiplicity_one_k[(std::size_t)(k - 1)];
      for (std::size_t t = i; t < j; ++t) {
        const int member = incidences[t].second;
        if (member == owner) continue;
        ++out.star_branches_k[(std::size_t)(k - 1)];
        edges.push_back({{batch_of[(std::size_t)member], owner}, member});
      }
      i = j;
    }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    out.deduplicated_branches_k[(std::size_t)(k - 1)] = (long long)edges.size();
    star_edges[(std::size_t)(k - 1)] = std::move(edges);
  }

  // 5. REJEU SEQUENTIEL DES LOTS — identique aux autres formes : activation,
  // branches d'etoiles du lot, classification par capture d'epoque, transcript
  // Gamma par marquage, records par temoin.
  fold.orders.resize((std::size_t)K);
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
    std::size_t edge_cursor = 0;
    int find(int a) {
      while (parent[(std::size_t)a] != a) {
        parent[(std::size_t)a] = parent[(std::size_t)parent[(std::size_t)a]];
        a = parent[(std::size_t)a];
      }
      return a;
    }
  };
  std::vector<OrderState> states((std::size_t)K);
  for (OrderState& st : states) {
    st.parent.resize(count);
    for (std::size_t s = 0; s < count; ++s) st.parent[s] = (int)s;
    st.node_of_root.assign(count, -1);
    st.coverage.resize(count);
    st.touch_epoch.assign(count, -1);
    st.staged_node.assign(count, -1);
    st.staged_cov.assign(count, 0);
    st.witness.resize(count);
    st.staged_witness.resize(count);
  }
  long long epoch = 0;
  for (std::size_t batch = 0; batch < batches.size(); ++batch) {
    ++epoch;
    const auto touch = [&](OrderState& st, int root) {
      if (st.touch_epoch[(std::size_t)root] != epoch) {
        st.touch_epoch[(std::size_t)root] = epoch;
        st.staged_node[(std::size_t)root] = st.node_of_root[(std::size_t)root];
        st.staged_cov[(std::size_t)root] = st.coverage[(std::size_t)root].size();
        st.staged_witness[(std::size_t)root] = st.witness[(std::size_t)root];
      }
    };
    std::vector<std::vector<int>> batch_touched((std::size_t)K);
    std::vector<std::vector<std::pair<int, int>>> batch_events((std::size_t)K);
    for (std::size_t b = batches[batch].first; b < batches[batch].second; ++b) {
      const int m = by_level[b];
      const int support_size = (int)catalogue.spheres[(std::size_t)m].n_support;
      for (int k = 1; k <= K; ++k) {
        if ((int)members[(std::size_t)m].size() < k) continue;
        OrderState& st = states[(std::size_t)(k - 1)];
        touch(st, m);
        st.coverage[(std::size_t)m].insert(members[(std::size_t)m].begin(),
                                           members[(std::size_t)m].end());
        st.witness[(std::size_t)m].assign(members[(std::size_t)m].begin(),
                                          members[(std::size_t)m].begin() + k);
        st.live_roots.insert(m);
        batch_touched[(std::size_t)(k - 1)].push_back(m);
        if (support_size <= k + 1)
          batch_events[(std::size_t)(k - 1)].push_back({m, support_size});
      }
    }
    for (int k = 1; k <= K; ++k) {
      OrderState& st = states[(std::size_t)(k - 1)];
      const std::vector<std::pair<std::pair<int, int>, int>>& edges =
          star_edges[(std::size_t)(k - 1)];
      while (st.edge_cursor < edges.size() &&
             (std::size_t)edges[st.edge_cursor].first.first == batch) {
        const int owner = edges[st.edge_cursor].first.second;
        const int member = edges[st.edge_cursor].second;
        ++out.unions_attempted;
        int ro = st.find(owner), rm = st.find(member);
        touch(st, ro);
        touch(st, rm);
        if (ro != rm) {
          ++out.unions_done;
          if (st.coverage[(std::size_t)ro].size() > st.coverage[(std::size_t)rm].size())
            std::swap(ro, rm);
          st.coverage[(std::size_t)rm].insert(st.coverage[(std::size_t)ro].begin(),
                                              st.coverage[(std::size_t)ro].end());
          std::set<mhgp::i32>().swap(st.coverage[(std::size_t)ro]);
          st.parent[(std::size_t)ro] = rm;
          if (st.witness[(std::size_t)ro] < st.witness[(std::size_t)rm])
            st.witness[(std::size_t)rm] = st.witness[(std::size_t)ro];
          st.live_roots.erase(ro);
          batch_touched[(std::size_t)(k - 1)].push_back(rm);
          batch_touched[(std::size_t)(k - 1)].push_back(ro);
        }
        ++st.edge_cursor;
      }
    }
    for (int k = 1; k <= K; ++k) {
      OrderState& st = states[(std::size_t)(k - 1)];
      SaturatedOrderFold& order = fold.orders[(std::size_t)(k - 1)];
      std::vector<int>& touched = batch_touched[(std::size_t)(k - 1)];
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
      for (const std::pair<int, int>& event : batch_events[(std::size_t)(k - 1)]) {
        const int root = st.find(event.first);
        const auto it = marked_minimum_support.find(root);
        if (it == marked_minimum_support.end())
          marked_minimum_support.emplace(root, event.second);
        else
          it->second = std::min(it->second, event.second);
        marked_generators[root].push_back(event.first);
      }
      long long births_here = 0, continuations_here = 0, multifusions_here = 0;
      const auto translated = [&universe](const std::vector<mhgp::i32>& dense) {
        std::vector<mhgp::i32> raw;
        for (mhgp::i32 d : dense) raw.push_back(universe[(std::size_t)d]);
        return raw;
      };
      std::vector<GammaEventRecord> batch_records;
      for (const auto& entry : marked_minimum_support) {
        const auto it = strict_of.find(entry.first);
        const std::size_t strict = it == strict_of.end() ? 0 : it->second.size();
        GammaEventRecord record;
        record.level_representative = by_level[batches[batch].first];
        record.closed_witness = translated(st.witness[(std::size_t)entry.first]);
        for (int marker : marked_generators[entry.first])
          record.marking_saturations.push_back(translated(members[(std::size_t)marker]));
        std::sort(record.marking_saturations.begin(), record.marking_saturations.end());
        record.marking_saturations.erase(
            std::unique(record.marking_saturations.begin(), record.marking_saturations.end()),
            record.marking_saturations.end());
        const auto witnesses = strict_witnesses_of.find(entry.first);
        if (witnesses != strict_witnesses_of.end())
          for (const std::vector<mhgp::i32>& strict_witness : witnesses->second)
            record.strict_witnesses.push_back(translated(strict_witness));
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
        for (int root : st.live_roots) {
          std::vector<mhgp::i32> cluster;
          for (mhgp::i32 dense : st.coverage[(std::size_t)root])
            cluster.push_back(universe[(std::size_t)dense]);
          partition.push_back(std::move(cluster));
        }
        std::sort(partition.begin(), partition.end());
        order.closed_partitions.push_back(std::move(partition));
      }
    }
  }
  for (int k = 1; k <= K; ++k)
    if (states[(std::size_t)(k - 1)].edge_cursor != star_edges[(std::size_t)(k - 1)].size()) {
      fold.refusal = "rejeu incomplet : branches d'etoile non consommees";
      return fold;
    }
  // IDENTITE DE PREFLIGHT : incidences reelles == binomiales predites (sauf
  // sous les mutants d'emission, que la porte tue par le differentiel).
  // L'identite de preflight TUE le doublon d'emission : un duplicata se
  // deduplique dans l'etoile et laisse le differentiel vert — seul le compte
  // des incidences contre les binomiales le voit. Les mutants qui REDUISENT
  // l'emission par construction (omission, decalage, filtres illicites) sont
  // exemptes ici pour mourir par le DIFFERENTIEL, qui prouve la semantique.
  out.identities_ok = out.incidences_total == out.predicted_incidences;
  if (!out.identities_ok && !mutants.omit_first_signature && !mutants.shift_k &&
      !mutants.qmin_filter_partial && !mutants.support_facet_filter) {
    fold.refusal = "identite de preflight violee : incidences != binomiales";
    return fold;
  }
  if (receipt != nullptr) *receipt = out;
  fold.ok = true;
  return fold;
}

}  // namespace mhgp3v
