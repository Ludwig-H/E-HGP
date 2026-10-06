// Condensation au critere A (tranche S10) : port explicite de condense (bench/points_flat.py:647-750).
//
// Par plateau croissant, les blocs touches sont ceux crees au plateau (indices croissants), puis ceux qui recoivent
// des entrees sans y etre crees (ordre du premier site entrant). Pour un bloc B : parts = ses enfants s'il est cree au
// plateau, sinon B ; masse = somme des masses des parts + entrees ; gros si masse >= mcs. Un gros bloc sans cluster
// vivant parmi ses parts fait naitre un cluster ; avec un seul, celui-ci continue ; avec deux ou plus, ils meurent et
// un parent nait (N-aire). Les sites en attente des petites parts et les entrees rejoignent le cluster au plateau.
// Ce qui change : listes d'attente chainees par site (concatenation en O(1)) au lieu de listes Python copiees ;
// jonctions consignees puis rangees en CSR par cluster ; forme de l'arbre controlee (head_invariant).
#include <algorithm>

#include "head/internal.hpp"

namespace mhgp11::head::detail {

namespace {

// CSR des indices 0..n-1 ranges par cle (cles < rows), stable : ordre croissant des indices dans chaque ligne.
Outcome by_key(std::span<const u32> key, u64 rows, MemoryBudget& budget, Csr<u32>& out) noexcept {
  MHGP11_TRY(out.off.allocate(rows + 1, budget));
  std::fill(out.off.begin(), out.off.end(), u64{0});
  u64 used = 0;
  for (const u32 k : key) {
    if (k == kNone) continue;
    if (k >= rows) return fail(Reason::head_invariant);
    ++out.off[k + 1];
    ++used;
  }
  for (u64 r = 0; r < rows; ++r) out.off[r + 1] += out.off[r];
  MHGP11_TRY(out.val.allocate(used, budget));
  Buffer<u64> cursor;
  MHGP11_TRY(cursor.allocate(rows, budget));
  std::copy_n(out.off.begin(), rows, cursor.begin());
  for (u64 i = 0; i < key.size(); ++i)
    if (key[i] != kNone) out.val[cursor[key[i]]++] = static_cast<u32>(i);
  return {};
}

Outcome check_shape(const TreeView& t) noexcept {
  const u64 plateaus = t.plateau_t.size(), blocks = t.block_plateau.size(), n = t.site_block.size();
  MHGP11_CHECK(t.plateau_m.size() == plateaus && t.plateau_q.size() == plateaus, head_invariant);
  MHGP11_CHECK(t.block_parent.size() == blocks && t.site_plateau.size() == n && t.site_ids.size() == n,
               head_invariant);
  MHGP11_CHECK(plateaus < kNone && blocks < kNone && n < kNone, head_invariant);
  for (u64 b = 0; b < blocks; ++b) {
    MHGP11_CHECK(t.block_plateau[b] < plateaus, head_invariant);
    const u32 up = t.block_parent[b];
    MHGP11_CHECK(up == kNone || (up > b && up < blocks && t.block_plateau[up] > t.block_plateau[b]), head_invariant);
  }
  for (u64 s = 0; s < n; ++s) {
    MHGP11_CHECK(t.site_block[s] < blocks && t.site_plateau[s] < plateaus, head_invariant);
    MHGP11_CHECK(t.site_plateau[s] >= t.block_plateau[t.site_block[s]], head_invariant);
    const u32 up = t.block_parent[t.site_block[s]];
    MHGP11_CHECK(up == kNone || t.site_plateau[s] < t.block_plateau[up], head_invariant);
  }
  return {};
}

struct Log {
  u32 cluster, plateau;
  u64 count;
};

// Etat de la condensation : CSR par plateau, masses, clusters et listes d'attente par bloc, journal des jonctions.
struct Condenser {
  Condenser(const TreeView& t, u32 m, MemoryBudget& b, Condensed& o) noexcept : tree(t), mcs(m), budget(b), out(o) {}
  const TreeView& tree;
  u32 mcs;
  MemoryBudget& budget;
  Condensed& out;
  u64 plateaus = 0, blocks = 0, n = 0, most_joins = 0, logged = 0;
  u32 count = 0;
  Csr<u32> created, entering, kids;
  Buffer<u64> mass, fresh_len;
  Buffer<u32> clus, pend_head, pend_tail, pend_len, next_site, mark, touched_extra, fresh_head, fresh_tail;
  Buffer<Log> log;

  Outcome prepare() noexcept {
    plateaus = tree.plateau_t.size();
    blocks = tree.block_plateau.size();
    n = tree.site_block.size();
    // Clusters : au plus un par bloc, plus la racine virtuelle ; jonctions : au plus une par (bloc, plateau touche).
    const u64 most_clusters = blocks + 1;
    most_joins = blocks + n;
    MHGP11_TRY(budget.admit(8 * (plateaus + 1) + 4 * blocks + 8 * (blocks + 1) + 4 * n +       // CSR par plateau
                            8 * (blocks + 1) + 4 * blocks +                                    // enfants des blocs
                            blocks * (8 + 4 + 4 + 4 + 8) + 4 * n + 4 * plateaus + 8 * plateaus +  // etat
                            most_clusters * (4 + 4 + 8) + sizeof(Log) * most_joins + 4 * n));
    MHGP11_TRY(by_key(tree.block_plateau, plateaus, budget, created));
    MHGP11_TRY(by_key(tree.site_plateau, plateaus, budget, entering));
    MHGP11_TRY(by_key(tree.block_parent, blocks, budget, kids));
    for (Buffer<u32>* b : {&clus, &pend_head, &pend_tail, &pend_len, &mark, &fresh_head, &fresh_tail})
      MHGP11_TRY(b->allocate(blocks, budget));
    MHGP11_TRY(mass.allocate(blocks, budget));
    MHGP11_TRY(fresh_len.allocate(blocks, budget));
    MHGP11_TRY(next_site.allocate(n, budget));
    MHGP11_TRY(touched_extra.allocate(n, budget));
    std::fill(mass.begin(), mass.end(), u64{0});
    for (Buffer<u32>* b : {&clus, &pend_head, &pend_tail, &mark}) std::fill(b->begin(), b->end(), kNone);
    std::fill(pend_len.begin(), pend_len.end(), 0u);
    MHGP11_TRY(out.parent.allocate(most_clusters, budget));
    MHGP11_TRY(out.top.allocate(most_clusters, budget));
    MHGP11_TRY(out.size.allocate(most_clusters, budget));
    MHGP11_TRY(out.first.allocate(n, budget));
    std::fill(out.first.begin(), out.first.end(), kNone);
    return log.allocate(most_joins, budget);
  }

  u32 birth() noexcept {
    out.parent[count] = kNone;
    out.top[count] = kNone;
    out.size[count] = 0;
    return count++;
  }

  Outcome join(u32 c, u32 plateau, u64 k) noexcept {
    if (k == 0) return {};
    MHGP11_CHECK(logged < most_joins, head_invariant);
    log[logged++] = {c, plateau, k};
    out.size[c] += k;
    return {};
  }

  // Entrees du plateau p, groupees par bloc dans l'ordre du premier site entrant ; rend le nombre de blocs entrants
  // qui ne sont pas crees a p (touched_extra).
  u64 gather(u32 p) noexcept {
    u64 extra = 0;
    for (const u32 s : entering.row(p)) {
      const u32 b = tree.site_block[s];
      if (mark[b] != p) {
        mark[b] = p;
        fresh_head[b] = fresh_tail[b] = kNone;
        fresh_len[b] = 0;
        if (tree.block_plateau[b] != p) touched_extra[extra++] = b;
      }
      next_site[s] = kNone;
      if (fresh_tail[b] == kNone) fresh_head[b] = s;
      else next_site[fresh_tail[b]] = s;
      fresh_tail[b] = s;
      ++fresh_len[b];
    }
    return extra;
  }

  // Bloc B touche au plateau p : parts = ses enfants s'il y est cree, sinon B lui-meme.
  Outcome touch(u32 B, u32 p, std::span<const u32> parts) noexcept {
    const bool fresh_here = mark[B] == p;
    u64 m_new = fresh_here ? fresh_len[B] : 0;
    u32 big_first = kNone, bigs = 0;
    // newcomers : attentes des petites parts (dans l'ordre des parts), puis les entrees.
    u32 head = kNone, tail = kNone;
    u64 newcomers = 0;
    for (const u32 x : parts) {
      m_new += mass[x];
      if (clus[x] != kNone) {
        if (bigs++ == 0) big_first = clus[x];
      } else if (pend_head[x] != kNone) {
        if (tail == kNone) head = pend_head[x];
        else next_site[tail] = pend_head[x];
        tail = pend_tail[x];
        newcomers += pend_len[x];
      }
      pend_head[x] = pend_tail[x] = kNone;
      pend_len[x] = 0;
    }
    if (fresh_here && fresh_head[B] != kNone) {
      if (tail == kNone) head = fresh_head[B];
      else next_site[tail] = fresh_head[B];
      tail = fresh_tail[B];
      newcomers += fresh_len[B];
    }
    if (m_new < mcs) {
      MHGP11_CHECK(bigs == 0, head_invariant);  // masse decroissante
      mass[B] = m_new;
      clus[B] = kNone;
      pend_head[B] = head;
      pend_tail[B] = tail;
      pend_len[B] = static_cast<u32>(newcomers);
      return {};
    }
    u32 c = kNone;
    if (bigs == 0) {
      c = birth();
      MHGP11_TRY(join(c, p, newcomers));
    } else if (bigs == 1) {
      c = big_first;
      MHGP11_TRY(join(c, p, newcomers));
    } else {
      c = birth();
      for (const u32 x : parts) {
        if (clus[x] == kNone) continue;
        out.parent[clus[x]] = c;
        out.top[clus[x]] = p;
      }
      MHGP11_TRY(join(c, p, m_new));
    }
    for (u32 s = head; s != kNone; s = (s == tail ? kNone : next_site[s])) out.first[s] = c;
    mass[B] = m_new;
    clus[B] = c;
    return {};
  }

  // Racine virtuelle d'une foret, puis jonctions en CSR par cluster (ordre de consignation) et enfants.
  Outcome finish() noexcept {
    u32 roots = 0;
    for (u32 c = 0; c < count; ++c) roots += out.parent[c] == kNone;
    if (roots >= 2) {
      const u32 v = birth();
      for (u32 c = 0; c < v; ++c)
        if (out.parent[c] == kNone) out.parent[c] = v;
    }
    out.count = count;
    MHGP11_TRY(out.join_off.allocate(u64{count} + 1, budget));
    std::fill(out.join_off.begin(), out.join_off.end(), u64{0});
    for (u64 j = 0; j < logged; ++j) ++out.join_off[log[j].cluster + 1];
    for (u32 c = 0; c < count; ++c) out.join_off[c + 1] += out.join_off[c];
    MHGP11_TRY(out.join_plateau.allocate(logged, budget));
    MHGP11_TRY(out.join_count.allocate(logged, budget));
    {
      Buffer<u64> cursor;
      MHGP11_TRY(cursor.allocate(count, budget));
      std::copy_n(out.join_off.begin(), count, cursor.begin());
      for (u64 j = 0; j < logged; ++j) {
        const u64 at = cursor[log[j].cluster]++;
        out.join_plateau[at] = log[j].plateau;
        out.join_count[at] = log[j].count;
      }
    }
    return by_key(std::span<const u32>(out.parent.data(), count), count, budget, out.children);
  }
};

}  // namespace

Outcome condense(const TreeView& tree, u32 mcs, MemoryBudget& budget, Condensed& out) noexcept {
  if (mcs < 2) return fail(Reason::parameter_out_of_range);
  MHGP11_TRY(check_shape(tree));
  Condenser state(tree, mcs, budget, out);
  MHGP11_TRY(state.prepare());
  for (u32 p = 0; p < state.plateaus; ++p) {
    const u64 extra = state.gather(p);
    const auto made = state.created.row(p);
    for (const u32 B : made) MHGP11_TRY(state.touch(B, p, state.kids.row(B)));
    for (u64 i = 0; i < extra; ++i) {
      const u32 B = state.touched_extra[i];
      MHGP11_TRY(state.touch(B, p, std::span<const u32>(&state.touched_extra[i], 1)));
    }
  }
  return state.finish();
}

}  // namespace mhgp11::head::detail
