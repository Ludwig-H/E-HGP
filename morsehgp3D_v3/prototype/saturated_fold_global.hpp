// MorseHGP3D v3 — LA FORME GLOBALE DU JOIN PAR POSTINGS (note §3).
//
// C'est la forme GPU-reprenable et le repli CPU multi-coeurs : le CSR complet
// des postings est construit d'abord, TOUTES les paires triangulaires de
// chaque posting sont emises par chunks (parallelisables — l'emission par lot
// du candidat batch ne parallelise presque rien, les lots etant petits), puis
// triees et reduites en poids exacts w(M,N) = |M inter N|. Chaque arete
// s'active au lot du PLUS TARDIF de ses deux generateurs ; le REJEU des lots
// DSU par niveau est le petit reste sequentiel, identique au protocole Q1.2
// et au transcript Gamma par marquage (theoremes 1--2 recus).
//
// DETERMINISME (contrat a deux digests du repli multi-coeurs, reponse Q4) :
// la table reduite est une fonction PURE de l'entree — les threads n'influent
// que sur l'ordre d'emission, efface par le tri global. Un run a T threads et
// un run a 1 thread rendent le MEME fold bit a bit ; la porte l'exige.
//
// Les recus sont ceux de la forme par lots : occurrences ancien--nouveau
// (paires de lots distincts) et nouveau--nouveau (meme lot), somme des poids,
// P_post, masse des postings, unions tentees/reussies — l'egalite CHAMP A
// CHAMP avec le recu de la forme par lots est exigee par la porte.
#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include "prototype/saturated_fold.hpp"

namespace mhgp3v {

inline SaturatedFold build_saturated_fold_postings_global(
    const mhgp::Catalogue& catalogue, int maximum_order, bool keep_partitions,
    PostingsReceipt* receipt, int threads = 1, bool enforce_event_guard = false,
    bool collect_pairs = false, long long memory_budget_bytes = 0) {
  SaturatedFold fold;
  fold.maximum_order = maximum_order;
  if (receipt != nullptr) *receipt = PostingsReceipt{};
  if (maximum_order < 1 || maximum_order > mhgp::kMaxRank) {
    fold.refusal = "ordre maximal hors contrat";
    return fold;
  }
  if (threads < 1) threads = (int)std::thread::hardware_concurrency();
  if (threads < 1) threads = 1;
  const std::size_t count = catalogue.spheres.size();

  // 1. MEMBRES verifies puis COMPRIMES — memes contrats fail-closed que la
  // forme par lots (tranches, tri, doublons, PointId negatifs, univers dense).
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
        fold.refusal = "membre negatif : les postings indexent par PointId";
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
  const auto dense_of = [&universe](mhgp::i32 raw) {
    return (mhgp::i32)(std::lower_bound(universe.begin(), universe.end(), raw) -
                       universe.begin());
  };
  for (std::vector<mhgp::i32>& generator_members : members)
    for (mhgp::i32& x : generator_members) x = dense_of(x);

  // 2. LOTS par niveau exact : batch_of[generateur] = indice de sa classe.
  std::vector<int> by_level((std::size_t)count);
  for (std::size_t s = 0; s < count; ++s) by_level[s] = (int)s;
  std::sort(by_level.begin(), by_level.end(), [&](int x, int y) {
    const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                        catalogue.spheres[(std::size_t)y].sph);
    if (c != 0) return c < 0;
    return x < y;
  });
  std::vector<int> batch_of((std::size_t)count, 0);
  std::vector<std::pair<std::size_t, std::size_t>> batches;   // [begin, end) dans by_level
  {
    std::size_t cursor = 0;
    while (cursor < count) {
      std::size_t batch_end = cursor + 1;
      while (batch_end < count &&
             mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)by_level[cursor]].sph,
                                   catalogue.spheres[(std::size_t)by_level[batch_end]].sph) == 0)
        ++batch_end;
      for (std::size_t b = cursor; b < batch_end; ++b)
        batch_of[(std::size_t)by_level[b]] = (int)batches.size();
      batches.push_back({cursor, batch_end});
      cursor = batch_end;
    }
  }

  // PREFLIGHT DE LA FORME GLOBALE, AVANT LE CSR (sequence exigee par l'audit
  // 39cf76e : degres u128 -> manifeste -> budget -> CSR -> emission) : cette
  // forme materialise TOUTES les occurrences d'un coup — son pic est en P_post
  // entier, pas en masse de lot. Sans ce refus declare, un P_post au-dessus de
  // la RAM etait un OOM SILENCIEUX (observe a n=200 : 385 M d'occurrences).
  // L'accumulation est ENTIEREMENT en u128 : aucun long long avant la garde.
  PostingsReceipt out;
  long long expected_postings_mass = 0;
  {
    using u128 = unsigned __int128;
    const u128 ceiling = (u128)1 << 62;
    std::vector<long long> degree(universe.size(), 0);
    u128 mass = 0;
    for (const std::vector<mhgp::i32>& generator_members : members) {
      mass += (u128)generator_members.size();
      for (mhgp::i32 x : generator_members) ++degree[(std::size_t)x];
    }
    if (mass >= ceiling) {
      fold.refusal = "preflight : masse des postings deborde le contrat entier";
      return fold;
    }
    u128 predicted = 0;
    std::size_t heaviest = 0;
    for (long long d : degree) {
      predicted += (u128)d * (u128)(d - 1) / 2;
      heaviest = std::max(heaviest, (std::size_t)d);
      if (predicted >= ceiling) {
        fold.refusal = "preflight : P_post deborde le contrat entier";
        return fold;
      }
    }
    const u128 peak = predicted * 32 + mass * 8 +
                      (u128)count * 40 * (u128)maximum_order +
                      mass * 64 * (u128)maximum_order;
    if (peak >= ceiling) {
      fold.refusal = "preflight : pic memoire predit deborde le contrat entier";
      return fold;
    }
    expected_postings_mass = (long long)mass;
    out.predicted_p_post = (long long)predicted;
    out.max_batch_occurrences = (long long)predicted;   // l'emission est UN lot global
    out.predicted_peak_bytes = (long long)peak;
    out.max_posting = heaviest;
    if (receipt != nullptr) {
      receipt->predicted_p_post = out.predicted_p_post;
      receipt->max_batch_occurrences = out.max_batch_occurrences;
      receipt->predicted_peak_bytes = out.predicted_peak_bytes;
    }
    if (memory_budget_bytes > 0 && peak > (u128)memory_budget_bytes) {
      fold.refusal = "preflight : pic memoire predit au-dessus du budget";
      return fold;
    }
  }

  // 3. CSR COMPLET des postings — construit APRES l'admission du preflight.
  std::vector<std::vector<int>> postings(universe.size());
  for (std::size_t s = 0; s < count; ++s)
    for (mhgp::i32 x : members[s]) postings[(std::size_t)x].push_back((int)s);
  for (const std::vector<int>& px : postings) {
    out.postings_mass += (long long)px.size();
    out.p_post += (long long)px.size() * ((long long)px.size() - 1) / 2;
  }
  if (out.p_post != out.predicted_p_post || out.postings_mass != expected_postings_mass) {
    fold.refusal = "identite de preflight violee : CSR != degres predits";
    return fold;
  }

  // 4. EMISSION PAR CHUNKS, parallele : les points sont partages entre les
  // threads par masse triangulaire equilibree ; chaque thread emet dans son
  // buffer local. Le tri global efface l'ordre d'emission — c'est ce qui rend
  // le resultat independant de T (contrat a deux digests).
  std::vector<std::pair<int, int>> occurrences;
  {
    std::vector<std::vector<std::pair<int, int>>> local((std::size_t)threads);
    std::vector<std::thread> workers;
    // Decoupage par prefixe de masse : chaque thread vise ~P_post/T.
    std::vector<std::size_t> stops;
    {
      const long long target = out.p_post / threads + 1;
      long long acc = 0;
      for (std::size_t x = 0; x < postings.size(); ++x) {
        acc += (long long)postings[x].size() * ((long long)postings[x].size() - 1) / 2;
        if (acc >= target && stops.size() + 1 < (std::size_t)threads) {
          stops.push_back(x + 1);
          acc = 0;
        }
      }
      stops.push_back(postings.size());
    }
    std::size_t from = 0;
    for (std::size_t t = 0; t < stops.size(); ++t) {
      const std::size_t to = stops[t];
      workers.push_back(std::thread([&, from, to, t]() {
        std::vector<std::pair<int, int>>& sink = local[t];
        for (std::size_t x = from; x < to; ++x) {
          const std::vector<int>& px = postings[x];
          for (std::size_t i = 0; i < px.size(); ++i)
            for (std::size_t j = i + 1; j < px.size(); ++j)
              sink.push_back({std::min(px[i], px[j]), std::max(px[i], px[j])});
        }
        std::sort(sink.begin(), sink.end());
      }));
      from = to;
    }
    for (std::thread& worker : workers) worker.join();
    for (const std::vector<std::pair<int, int>>& sink : local)
      occurrences.insert(occurrences.end(), sink.begin(), sink.end());
  }
  std::sort(occurrences.begin(), occurrences.end());

  // 5. REDUCTION en poids exacts, et recus d'occurrences par type de lot.
  std::vector<std::pair<std::pair<int, int>, long long>> edges;
  for (std::size_t i = 0; i < occurrences.size();) {
    std::size_t j = i;
    while (j < occurrences.size() && occurrences[j] == occurrences[i]) ++j;
    edges.push_back({occurrences[i], (long long)(j - i)});
    i = j;
  }
  std::vector<std::pair<int, int>>().swap(occurrences);
  for (const auto& edge : edges) {
    out.weight_sum += edge.second;
    ++out.reduced_pairs;
    if (batch_of[(std::size_t)edge.first.first] == batch_of[(std::size_t)edge.first.second])
      out.new_new_occurrences += edge.second;
    else
      out.old_new_occurrences += edge.second;
  }
  out.identities_ok = out.p_post == out.weight_sum && out.postings_mass == expected_postings_mass;
  if (!out.identities_ok) {
    fold.refusal = "identite globale violee : P_post != somme des poids";
    return fold;
  }

  // 6. TRI DES ARETES PAR LOT D'ACTIVATION (le plus tardif des deux), puis clef
  // canonique — l'ordre de rejeu est une fonction pure de l'entree.
  std::vector<std::size_t> edge_order(edges.size());
  for (std::size_t e = 0; e < edges.size(); ++e) edge_order[e] = e;
  std::sort(edge_order.begin(), edge_order.end(), [&](std::size_t a, std::size_t b) {
    const int ba = std::max(batch_of[(std::size_t)edges[a].first.first],
                            batch_of[(std::size_t)edges[a].first.second]);
    const int bb = std::max(batch_of[(std::size_t)edges[b].first.first],
                            batch_of[(std::size_t)edges[b].first.second]);
    if (ba != bb) return ba < bb;
    return edges[a].first < edges[b].first;
  });

  // 7. REJEU SEQUENTIEL DES LOTS : activation, unions, classification par
  // capture d'epoque et transcript Gamma par marquage — identique a la forme
  // par lots, protocole Q1.2.
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
  std::size_t edge_cursor = 0;
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
    std::vector<std::pair<std::pair<int, int>, long long>> batch_pairs;
    while (edge_cursor < edge_order.size()) {
      const auto& edge = edges[edge_order[edge_cursor]];
      const int activation =
          std::max(batch_of[(std::size_t)edge.first.first],
                   batch_of[(std::size_t)edge.first.second]);
      if ((std::size_t)activation != batch) break;
      if (collect_pairs) batch_pairs.push_back(edge);
      const int m = edge.first.first, nid = edge.first.second;
      const long long w = edge.second;
      const long long cap = std::min<long long>(K, w);
      for (int k = 1; (long long)k <= cap; ++k) {
        OrderState& st = states[(std::size_t)(k - 1)];
        if ((int)members[(std::size_t)m].size() < k ||
            (int)members[(std::size_t)nid].size() < k)
          continue;
        ++out.unions_attempted;
        int rm = st.find(m), rn = st.find(nid);
        touch(st, rm);
        touch(st, rn);
        if (rm == rn) continue;
        ++out.unions_done;
        if (st.coverage[(std::size_t)rm].size() > st.coverage[(std::size_t)rn].size())
          std::swap(rm, rn);
        st.coverage[(std::size_t)rn].insert(st.coverage[(std::size_t)rm].begin(),
                                            st.coverage[(std::size_t)rm].end());
        std::set<mhgp::i32>().swap(st.coverage[(std::size_t)rm]);
        st.parent[(std::size_t)rm] = rn;
        if (st.witness[(std::size_t)rm] < st.witness[(std::size_t)rn])
          st.witness[(std::size_t)rn] = st.witness[(std::size_t)rm];
        st.live_roots.erase(rm);
        batch_touched[(std::size_t)(k - 1)].push_back(rn);
        batch_touched[(std::size_t)(k - 1)].push_back(rm);
      }
      ++edge_cursor;
    }
    if (collect_pairs)
      out.pairs.insert(out.pairs.end(), batch_pairs.begin(), batch_pairs.end());

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
  if (edge_cursor != edge_order.size()) {
    fold.refusal = "rejeu incomplet : aretes non consommees";
    return fold;
  }
  if (receipt != nullptr) *receipt = out;
  fold.ok = true;
  return fold;
}

}  // namespace mhgp3v
