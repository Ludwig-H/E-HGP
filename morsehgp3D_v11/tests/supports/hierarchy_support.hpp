// Outils de test de la hierarchie des supports (tranche S6b), hors produit, partages par les portes unitaires et la
// sonde mhgp11_supports_hierarchy_probe : juge des invariants de la specification (paragraphe 8.4) I5, I6 et I11 et de
// la coherence avec le rattachement de la tour, recomptes ici par des binomes et une fermeture brute (masques de
// positions de U_b) independants de counts.hpp et de enumerate.cpp ; empreinte FNV-1a 64 de la hierarchie entiere.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "supports/supports.hpp"

namespace hierarchy_test {
using namespace mhgp11;
using supports::Ball;
using supports::Support;
using supports::SupportHierarchy;

// C(n, k) exact (formule multiplicative : chaque quotient intermediaire est un binome), 0 hors du triangle.
inline u64 choose(i64 n, i64 k) {
  if (n < 0 || k < 0 || k > n) return 0;
  u64 c = 1;
  for (i64 i = 1; i <= k; ++i) c = c * static_cast<u64>(n - k + i) / static_cast<u64>(i);
  return c;
}

struct Totals {
  u64 nodes = 0, balls = 0, births = 0, merges = 0, internals = 0, supports = 0, extended = 0, multiple = 0;
  u64 tetra = 0, prior = 0, kparties = 0, cofaces = 0, incidences = 0, max_shell = 0, closures = 0;
};

inline bool support_less(const Support& l, const Support& r) {
  if (l.arity != r.arity) return l.arity < r.arity;
  for (u32 j = 0; j < l.arity; ++j)
    if (l.sites[j] != r.sites[j]) return idx(l.sites[j]) < idx(r.sites[j]);
  return false;
}

// Postordre : bijection, racine en dernier, et pour chaque noeud ses enfants (NodeIdx croissants) occupent des
// intervalles consecutifs qui remplissent [post - size + 1, post - 1] : c'est la caracterisation du postordre des
// enfants ordonnes, et les intervalles de sous-arbre sont emboites (I11).
inline std::string check_postorder(const OrderForest& forest, const SupportHierarchy& h, std::vector<u32>& by_post) {
  const u64 n = forest.nodes().size();
  const auto post = h.post();
  const auto size = h.subtree_size();
  by_post.assign(n, kNone);
  for (u32 v = 0; v < n; ++v) {
    if (post[v] >= n || by_post[post[v]] != kNone) return "postordre non bijectif";
    by_post[post[v]] = v;
  }
  if (post[idx(forest.root())] != n - 1) return "racine hors du dernier rang";
  for (u32 v = 0; v < n; ++v) {
    if (size[v] == 0 || size[v] > post[v] + 1) return "taille de sous-arbre";
    u64 cursor = post[v] + 1 - size[v], total = 1;
    u32 last = 0;
    bool first = true;
    for (NodeIdx c : forest.children(NodeIdx{v})) {
      if (!first && idx(c) <= last) return "enfants non croissants";
      first = false;
      last = idx(c);
      if (u64{post[idx(c)]} + 1 < size[idx(c)] || post[idx(c)] + 1 - size[idx(c)] != cursor) return "intervalles";
      cursor = u64{post[idx(c)]} + 1;
      total += size[idx(c)];
    }
    if (cursor != post[v] || total != size[v]) return "intervalles de sous-arbre";
  }
  return {};
}

// N_0..N_m de Q_b par masques de positions de U_b (bornes : m <= 20).
inline std::vector<u64> brute_closure(std::span<const SiteIdx> shell, std::span<const Support> supports) {
  const u32 m = static_cast<u32>(shell.size());
  std::vector<u32> masks;
  for (const Support& s : supports) {
    u32 mask = 0;
    for (u32 j = 0; j < s.arity; ++j)
      mask |= u32{1} << static_cast<u32>(std::lower_bound(shell.begin(), shell.end(), s.sites[j]) - shell.begin());
    masks.push_back(mask);
  }
  std::vector<u64> parts(m + 1, 0);
  for (u32 mask = 0; mask < (u32{1} << m); ++mask)
    for (u32 s : masks)
      if ((s & mask) == s) {
        ++parts[static_cast<u32>(__builtin_popcount(mask))];
        break;
      }
  return parts;
}

// I5 et I6 d'une boule : Q_b (S* en tete, sites dans U_b, ordre (arite, SiteIdx)), comptes du lemme G recomptes,
// incidences par support, cas reguliers.
inline std::string check_ball(const FullDomain& domain, Order k, const Ball& ball, std::span<const Support> sup,
                              std::span<const u32> sup_cofaces, Totals& t, u32 closure_limit) {
  const auto& cat = domain.catalogue();
  const auto& data = cat.balls_data()[idx(ball.key)];
  const auto shell = cat.shell(ball.key);
  const i64 p = ball.p, m = ball.m, q = ball.qmin, kk = k, tt = kk - p;
  if (sup.empty() || sup[0].arity != q ||
      !std::equal(sup[0].sites.begin(), sup[0].sites.begin() + q, data.support.begin()))
    return "I5 : S* absent de la tete";
  if (m == q && sup.size() != 1) return "I5 : coquille reguliere a plusieurs supports";
  u64 sum = 0, top = 0;
  for (u64 i = 0; i < sup.size(); ++i) {
    const Support& s = sup[i];
    if (s.arity < 2 || s.arity > 4 || s.arity > m) return "I5 : arite";
    for (u32 j = 0; j < 4; ++j) {
      if (j >= s.arity) {
        if (s.sites[j] != SiteIdx{kNone}) return "I5 : bourrage";
        continue;
      }
      if (j > 0 && idx(s.sites[j - 1]) >= idx(s.sites[j])) return "I5 : sites non croissants";
      if (!std::binary_search(shell.begin(), shell.end(), s.sites[j])) return "I5 : site hors de U_b";
    }
    if (i > 0 && !support_less(sup[i - 1], s)) return "I5 : ordre (arite, SiteIdx)";
    const u64 want = choose(p + m - s.arity, kk + 1 - s.arity);
    if (sup_cofaces[i] != want) return "I6 : cofaces par support";
    sum += want;
    top = std::max(top, want);
    t.tetra += s.arity == 4 ? 1 : 0;
  }
  if (ball.kparties_reliees != choose(p + m, kk) || ball.compressed_parts != choose(m, tt)) return "I6 : K-parties";
  if (!(top <= ball.cofaces && ball.cofaces <= sum)) return "I6 : max_Q <= cofaces <= somme";
  if (m == q && p + q == kk && (ball.kparties_reliees != 1 || ball.cofaces != 0 || ball.strict_traces != 0))
    return "I6 : naissance reguliere";
  if (m == q && p + q == kk + 1 && (ball.kparties_reliees != kk + 1 || ball.cofaces != 1 || ball.strict_traces != q))
    return "I6 : jonction reguliere";
  if (m <= closure_limit) {
    const auto parts = brute_closure(shell, sup);
    u64 cofaces = 0;
    for (i64 j = 0; j <= m; ++j) cofaces += choose(p, kk + 1 - j) * parts[static_cast<u64>(j)];
    const u64 gabriel = tt + 1 <= m ? parts[static_cast<u64>(tt + 1)] : 0;
    const u64 strict = choose(m, tt) - (tt <= m ? parts[static_cast<u64>(tt)] : 0);
    if (ball.strict_traces != strict) return "I5 : strict_traces != C(m, t) - N_t";
    if (ball.cofaces != cofaces || ball.gabriel_cofaces != gabriel) return "I6 : cofaces de la fermeture";
    ++t.closures;
  }
  t.supports += sup.size();
  t.extended += m > q ? 1 : 0;
  t.multiple += sup.size() > 1 ? 1 : 0;
  t.max_shell = std::max<u64>(t.max_shell, static_cast<u64>(m));
  t.kparties += ball.kparties_reliees;
  t.cofaces += ball.cofaces;
  t.incidences += sum;
  return {};
}

// Juge complet : coherence avec le rattachement (chaque boule de W_K une fois, memes noeud, role, composantes,
// branches, journal = strict_traces), I11 (listes propres triees, premiere boule propre au rang du noeud, roles),
// I5 et I6 par boule.
inline std::string check(const OrderTree& tree, const SupportHierarchy& h, Totals& t, u32 closure_limit = 20) {
  const OrderForest& forest = tree.forest();
  const WindowAttachment& a = tree.attachment();
  const auto& cat = tree.domain().catalogue();
  const u64 n = forest.nodes().size(), b = a.size();
  const Order k = tree.order();
  if (h.order() != k || h.root() != forest.root()) return "ordre ou racine";
  if (h.post().size() != n || h.subtree_size().size() != n || h.ball_offsets().size() != n + 1 ||
      h.balls().size() != b || h.support_offsets().size() != b + 1 || h.prior_offsets().size() != b + 1 ||
      h.supports().size() != h.support_cofaces().size() || h.support_offsets()[b] != h.supports().size() ||
      h.prior_offsets()[b] != h.prior().size() || h.support_offsets()[0] != 0 || h.prior_offsets()[0] != 0)
    return "tailles";
  std::vector<u32> by_post;
  if (auto e = check_postorder(forest, h, by_post); !e.empty()) return e;
  const auto off = h.ball_offsets();
  if (off[0] != 0 || off[n] != b) return "decalages des boules";
  std::vector<char> seen(b, 0);
  t.nodes += n;
  for (u64 j = 0; j < n; ++j) {
    if (off[j] > off[j + 1]) return "decalages des boules";
    const u32 v = by_post[j];
    const auto& node = forest.nodes()[v];
    const bool leaf = k == 1 && v < forest.births();
    if (leaf != (off[j] == off[j + 1])) return "I11 : liste propre vide ou feuille portant une boule";
    u64 merges = 0, births = 0;
    for (u64 slot = off[j]; slot < off[j + 1]; ++slot) {
      const Ball& ball = h.balls()[slot];
      const auto found = std::lower_bound(a.balls().begin(), a.balls().end(), ball.key);
      const auto at = static_cast<u64>(found - a.balls().begin());
      if (at >= b || a.balls()[at] != ball.key || seen[at] != 0) return "I1 : boule absente ou en double";
      seen[at] = 1;
      const auto& data = cat.balls_data()[idx(ball.key)];
      if (ball.node != NodeIdx{v} || a.node()[at] != ball.node || a.role()[at] != ball.role ||
          a.components()[at] != ball.components || ball.rank != data.rank || ball.p != data.p || ball.m != data.m ||
          ball.qmin != data.qmin)
        return "rattachement recopie";
      if (ball.strict_traces != a.strict_traces()[at]) return "contre-epreuve du journal";
      if (slot > off[j]) {
        const Ball& prev = h.balls()[slot - 1];
        if (idx(prev.rank) > idx(ball.rank) || (prev.rank == ball.rank && idx(prev.key) >= idx(ball.key)))
          return "I11 : liste propre non triee (rang, BallIdx)";
      } else if (ball.rank != node.rank) {
        return "I11 : premiere boule propre hors du rang du noeud";
      }
      const bool birth = ball.role == BallRole::birth, merge = ball.role == BallRole::merge;
      if (birth != (ball.strict_traces == 0)) return "naissance et traces strictes";
      if (birth && (v >= forest.births() || node.birth_key != idx(ball.key))) return "lemme B : naissance";
      if (ball.rank != node.rank && ball.role != BallRole::internal) return "lemme B : boule tardive non interne";
      if (ball.role == BallRole::internal && ball.components != 1) return "I3 : interne a plusieurs composantes";
      const u64 pb = h.prior_offsets()[slot], pe = h.prior_offsets()[slot + 1];
      const u64 ab = a.prior_offsets()[at], ae = a.prior_offsets()[at + 1];
      if (pe - pb != ae - ab || !std::equal(h.prior().begin() + pb, h.prior().begin() + pe, a.prior().begin() + ab) ||
          (!merge && pe != pb) || (merge && pe - pb != ball.components))
        return "branches";
      merges += merge ? 1 : 0;
      births += birth ? 1 : 0;
      t.births += birth ? 1 : 0;
      t.merges += merge ? 1 : 0;
      t.internals += ball.role == BallRole::internal ? 1 : 0;
      t.prior += pe - pb;
      const u64 sb = h.support_offsets()[slot], se = h.support_offsets()[slot + 1];
      if (se <= sb) return "I5 : Q_b vide";
      if (auto e = check_ball(tree.domain(), k, ball, h.supports().subspan(sb, se - sb),
                              h.support_cofaces().subspan(sb, se - sb), t, closure_limit);
          !e.empty())
        return e;
      ++t.balls;
    }
    if (v >= forest.births() && merges == 0) return "I3 : fusion sans boule de fusion";
    if (births != (v < forest.births() && k >= 2 ? 1u : 0u)) return "lemme B : une naissance par noeud de naissance";
  }
  return {};
}

// FNV-1a 64 de la hierarchie entiere, dans l'ordre des tableaux ; registre compris.
inline u64 fingerprint(const SupportHierarchy& h) {
  u64 hash = 0xcbf29ce484222325ull;
  auto feed = [&](u64 word, int bytes) {
    for (int i = 0; i < bytes; ++i) hash = (hash ^ ((word >> (8 * i)) & 255)) * 0x100000001b3ull;
  };
  feed(h.order(), 1);
  feed(idx(h.root()), 4);
  for (u32 x : h.post()) feed(x, 4);
  for (u32 x : h.subtree_size()) feed(x, 4);
  for (u64 x : h.ball_offsets()) feed(x, 8);
  for (const Ball& x : h.balls()) {
    for (u32 w : {idx(x.key), idx(x.node), idx(x.rank), x.kparties_reliees, x.compressed_parts, x.strict_traces,
                  x.cofaces, x.gabriel_cofaces, x.components})
      feed(w, 4);
    for (u32 w : {static_cast<u32>(x.role), u32{x.p}, u32{x.m}, u32{x.qmin}}) feed(w, 1);
  }
  for (u64 x : h.support_offsets()) feed(x, 8);
  for (const Support& s : h.supports()) {
    for (SiteIdx site : s.sites) feed(idx(site), 4);
    feed(s.arity, 1);
  }
  for (u32 x : h.support_cofaces()) feed(x, 4);
  for (u64 x : h.prior_offsets()) feed(x, 8);
  for (NodeIdx x : h.prior()) feed(idx(x), 4);
  const auto& l = h.ledger();
  for (u64 w : {l.balls, l.regular, l.extended, l.supports, l.midpoint_tests, l.acute_tests, l.orientation_tests,
                l.inside_tests})
    feed(w, 8);
  return hash;
}

// Formule d'admission recalculee ici (HierarchyAdmission, supports.hpp), jamais lue dans le produit.
struct Admission {
  u64 first = 0, retained = 0, per_worker = 0;
};
inline Admission admission(const OrderTree& tree, u64 workers) {
  const auto& cat = tree.domain().catalogue();
  const u64 n = tree.forest().nodes().size(), b = tree.attachment().size(), prior = tree.attachment().prior().size();
  u64 widest = 0;
  for (BallIdx key : tree.attachment().balls()) {
    const auto& d = cat.balls_data()[idx(key)];
    if (d.m > d.qmin) widest = std::max<u64>(widest, d.m);
  }
  const u64 words = widest == 0 ? 0 : (widest <= 6 ? 1 : u64{1} << (widest - 6));
  const u64 list = widest == 0 ? 1 : choose(static_cast<i64>(widest), 2) + choose(static_cast<i64>(widest), 3) +
                                         choose(static_cast<i64>(widest), 4);
  Admission out;
  out.per_worker = sizeof(supports::SupportLedger) + 8 * words + sizeof(Support) * list;
  out.retained = 4 * n + 4 * n + 8 * (n + 1) + sizeof(Ball) * b + 8 * (b + 1) + 8 * (b + 1) + 4 * prior;
  out.first = out.retained + 4 * b + workers * out.per_worker;
  return out;
}

}  // namespace hierarchy_test
