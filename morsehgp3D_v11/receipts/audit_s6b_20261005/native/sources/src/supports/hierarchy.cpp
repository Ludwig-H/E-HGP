// Assemblage de la hierarchie des supports d'ordre K (tranche S6b de la sortie parametree ; docs/SORTIES.md, section
// 6 ; specification, paragraphes 4, 7.3 a 7.5) : postordre de l'arbre d'ordre K, tri des boules de W_K par seaux
// stables, Q_b de chaque boule en deux passes count puis fill a positions fixes, comptes du lemme G en memoire.
//
// Decisions :
//   - plafond : la pre-passe controle TOUTES les coquilles etendues de W_K (check_shell) avant toute allocation et
//     tout calcul de Q_b ; un refus support_shell_capacity vaut pour l'appel entier (apports des auditeurs) ;
//   - admission : la formule de HierarchyAdmission (supports.hpp) couvre exactement les tampons de l'appel ; par fil
//     actif, le brouillon de fermeture (2 Mio a m = 24) et la liste temporaire de supports. Les tampons des fils sont
//     alloues apres les sorties : une formule qui en oublierait un ferait monter le pic avant le refus (porte) ;
//   - positions fixes : la position d'une boule est fixee par le tri avant les passes ; count et fill lisent la meme
//     boule a la meme position (meme BallIdx), quel que soit le fil qui la traite ; les registres sont un par fil,
//     sommes apres la jointure ; aucune sortie ne depend du nombre de fils ;
//   - contre-epreuve : strict_traces = C(m, t) - N_t doit egaler le journal du constructeur (WindowAttachment) ;
//   - ordre des supports : tri explicite (arite, SiteIdx lexicographiques), qui ne change rien a l'enumeration de
//     ball_supports mais fait de l'ordre publie un contrat de l'assemblage ;
//   - gardes sans porte possible (arbre ou rattachement incoherents : build_order les controle deja, I1 a I4) :
//     supports_invariant. Seules la contre-epreuve et le nombre de supports entre les passes protegent un calcul.
#include <algorithm>

#include "sched/sched.hpp"
#include "supports/supports.hpp"

namespace mhgp11::supports {

namespace supports_detail {

struct Assembly {
  static constexpr u64 kGrain = 512;  // boules par tranche ; sans effet sur les sorties

  Assembly(const OrderTree& t, MemoryBudget& b, sched::Pool* p) noexcept : tree(t), budget(b), pool(p) {}
  const OrderTree& tree;
  MemoryBudget& budget;
  sched::Pool* pool;
  SupportHierarchy out;
  Buffer<u32> origin;              // position -> indice dans le rattachement (temporaire)
  Buffer<SupportLedger> ledgers;   // un par fil
  Buffer<u64> scratch;             // brouillons de fermeture, words par fil
  Buffer<Support> lists;           // listes temporaires de supports, list par fil
  HierarchyAdmission sizes;
  u64 words = 0, list = 1;

  Outcome prepass() noexcept;
  Outcome allocate_first() noexcept;
  Outcome postorder() noexcept;
  Outcome sort_balls() noexcept;
  Ball metadata(u64 slot) const noexcept;
  Outcome run(u64 n, sched::Pool::Body body) noexcept;
  Outcome close_counts() noexcept;
  void finish() noexcept {
    out.order_ = tree.order();
    out.root_ = tree.forest().root();
  }
  static Outcome count_body(void* context, u64 begin, u64 end, u32 worker) noexcept;
  static Outcome fill_body(void* context, u64 begin, u64 end, u32 worker) noexcept;

  std::span<Support> list_of(u32 worker) noexcept { return lists.span().subspan(worker * list, list); }
  std::span<u64> words_of(u32 worker) noexcept {
    return words == 0 ? std::span<u64>() : scratch.span().subspan(worker * words, words);
  }
};

// Rattachement coherent (tailles, BallIdx strictement croissants, noeuds de l'arbre, branches) et forme de chaque
// boule ; plafond de toutes les coquilles etendues. Aucune allocation.
Outcome Assembly::prepass() noexcept {
  const WindowAttachment& a = tree.attachment();
  const Catalogue& cat = tree.domain().catalogue();
  const u64 b = a.size(), n = tree.forest().nodes().size();
  if (a.node().size() != b || a.role().size() != b || a.strict_traces().size() != b || a.components().size() != b ||
      a.prior_offsets().size() != b + 1 || a.prior_offsets()[b] != a.prior().size() || n == 0 || n >= kNone)
    return fail(Reason::supports_invariant);
  u64 widest = 0;
  for (u64 i = 0; i < b; ++i) {
    const u32 key = idx(a.balls()[i]);
    if (key >= cat.balls() || (i > 0 && idx(a.balls()[i - 1]) >= key) || idx(a.node()[i]) >= n ||
        a.prior_offsets()[i] > a.prior_offsets()[i + 1])
      return fail(Reason::supports_invariant);
    const CatalogueBall& data = cat.balls_data()[key];
    if (data.p > kMaxInterior || data.qmin < 2 || data.qmin > 4 || data.m < data.qmin)
      return fail(Reason::supports_invariant);
    if (data.m == data.qmin) continue;
    MHGP11_TRY(check_shell(data.m));
    widest = std::max<u64>(widest, data.m);
  }
  sizes.nodes = n;
  sizes.balls = b;
  sizes.prior = a.prior().size();
  sizes.workers = pool == nullptr ? 1 : pool->size();
  sizes.widest = widest;
  words = widest == 0 ? 0 : closure_words(static_cast<u32>(widest));
  list = widest == 0 ? 1 : support_capacity(static_cast<u32>(widest));
  return {};
}

// Premier etage, deja admis : sorties connues, puis temporaires, puis tampons par fil (en dernier).
Outcome Assembly::allocate_first() noexcept {
  const u64 n = sizes.nodes, b = sizes.balls, w = sizes.workers;
  MHGP11_TRY(out.post_.allocate(n, budget));
  MHGP11_TRY(out.size_.allocate(n, budget));
  MHGP11_TRY(out.ball_offsets_.allocate(n + 1, budget));
  MHGP11_TRY(out.balls_.allocate(b, budget));
  MHGP11_TRY(out.support_offsets_.allocate(b + 1, budget));
  MHGP11_TRY(out.prior_offsets_.allocate(b + 1, budget));
  MHGP11_TRY(out.prior_.allocate(sizes.prior, budget));
  MHGP11_TRY(origin.allocate(b, budget));
  MHGP11_TRY(ledgers.allocate(w, budget));
  MHGP11_TRY(scratch.allocate(w * words, budget));
  MHGP11_TRY(lists.allocate(w * list, budget));
  std::fill(ledgers.begin(), ledgers.end(), SupportLedger{});
  return {};
}

// Postordre iteratif depuis la racine, enfants par NodeIdx croissant, sans pile : tant qu'un noeud est sur le chemin,
// post[v] porte le curseur de son prochain enfant ; a sa sortie, son rang. Un parcours de 2N - 1 pas au plus.
Outcome Assembly::postorder() noexcept {
  const OrderForest& forest = tree.forest();
  const auto nodes = forest.nodes();
  const u32 n = static_cast<u32>(nodes.size()), root = idx(forest.root());
  u32* post = out.post_.data();
  u32* size = out.size_.data();
  if (root >= n || nodes[root].parent != NodeIdx{kNone}) return fail(Reason::supports_invariant);
  post[root] = 0;
  size[root] = 1;
  u32 v = root, rank = 0;
  for (u64 steps = 0;; ++steps) {
    if (steps >= 2 * u64{n}) return fail(Reason::supports_invariant);
    const auto kids = forest.children(NodeIdx{v});
    const u32 c = post[v];
    if (c < kids.size()) {
      const u32 w = idx(kids[c]);
      if (w >= n || nodes[w].parent != NodeIdx{v} || (c > 0 && idx(kids[c - 1]) >= w))
        return fail(Reason::supports_invariant);
      post[v] = c + 1;
      post[w] = 0;
      size[w] = 1;
      v = w;
      continue;
    }
    post[v] = rank++;
    if (v == root) break;
    const u32 up = idx(nodes[v].parent);
    size[up] += size[v];
    v = up;
  }
  if (rank != n) return fail(Reason::supports_invariant);
  return {};
}

// Seaux par postordre du noeud de rattachement. Le rattachement est en BallIdx croissants (rangs croissants) : on
// place de la derniere boule a la premiere en decrementant la fin de chaque seau, ce qui garde dans un seau l'ordre
// des BallIdx (tri stable). Puis decalages des branches ; les metadonnees viennent avec la passe count.
Outcome Assembly::sort_balls() noexcept {
  const WindowAttachment& a = tree.attachment();
  const u64 n = sizes.nodes, b = sizes.balls;
  u64* off = out.ball_offsets_.data();
  const u32* post = out.post_.data();
  std::fill(off, off + n + 1, u64{0});
  for (u64 i = 0; i < b; ++i) ++off[u64{post[idx(a.node()[i])]} + 1];
  for (u64 j = 1; j <= n; ++j) off[j] += off[j - 1];
  for (u64 i = b; i-- > 0;) origin[--off[u64{post[idx(a.node()[i])]} + 1]] = static_cast<u32>(i);
  for (u64 j = 0; j < n; ++j) off[j] = off[j + 1];
  off[n] = b;
  u64* prior = out.prior_offsets_.data();
  prior[0] = 0;
  for (u64 slot = 0; slot < b; ++slot) {
    const u32 at = origin[slot];
    prior[slot + 1] = prior[slot] + (a.prior_offsets()[at + 1] - a.prior_offsets()[at]);
  }
  if (prior[b] != sizes.prior) return fail(Reason::supports_invariant);
  return {};
}

// Metadonnees d'une position, lues dans le rattachement et le catalogue (passe count).
Ball Assembly::metadata(u64 slot) const noexcept {
  const WindowAttachment& a = tree.attachment();
  const u32 at = origin[slot];
  const CatalogueBall& data = tree.domain().catalogue().balls_data()[idx(a.balls()[at])];
  Ball ball;
  ball.key = a.balls()[at];
  ball.node = a.node()[at];
  ball.rank = data.rank;
  ball.role = a.role()[at];
  ball.components = a.components()[at];
  ball.p = static_cast<u8>(data.p);
  ball.m = static_cast<u8>(data.m);
  ball.qmin = data.qmin;
  return ball;
}

Outcome Assembly::run(u64 n, sched::Pool::Body body) noexcept {
  if (pool == nullptr) return body(this, 0, n, 0);
  return pool->parallel_for(n, kGrain, this, body);
}

// Passe count : metadonnees de la position, Q_b dans le brouillon du fil, comptes du lemme G, contre-epreuve du
// journal, |Q_b| a sa position.
Outcome Assembly::count_body(void* context, u64 begin, u64 end, u32 worker) noexcept {
  Assembly& s = *static_cast<Assembly*>(context);
  if (worker >= s.sizes.workers) return fail(Reason::supports_invariant);
  const FullDomain& domain = s.tree.domain();
  const auto journal = s.tree.attachment().strict_traces();
  for (u64 slot = begin; slot < end; ++slot) {
    Ball& ball = s.out.balls_[slot];
    ball = s.metadata(slot);
    const auto made = ball_supports(domain, ball.key, s.list_of(worker), s.words_of(worker), &s.ledgers[worker]);
    if (!made.ok()) return made.outcome();
    const auto shape = make_shape(ball.p, ball.m, ball.qmin, s.tree.order());
    if (!shape.ok()) return shape.outcome();
    const auto counts = ball_counts(shape.value(), made.value().closure);
    if (!counts.ok()) return counts.outcome();
    const BallCounts& c = counts.value();
    if (c.strict_traces != journal[s.origin[slot]]) return fail(Reason::supports_invariant);
    ball.kparties_reliees = c.kparties_reliees;
    ball.compressed_parts = c.compressed_parts;
    ball.strict_traces = c.strict_traces;
    ball.cofaces = c.cofaces;
    ball.gabriel_cofaces = c.gabriel_cofaces;
    s.out.support_offsets_[slot + 1] = made.value().count;
  }
  return {};
}

// Sommes des registres apres la jointure, decalages des supports en u64 aux sommes verifiees, second etage admis.
Outcome Assembly::close_counts() noexcept {
  for (const SupportLedger& part : ledgers) out.ledger_.add(part);
  u64* off = out.support_offsets_.data();
  off[0] = 0;
  for (u64 slot = 0; slot < sizes.balls; ++slot) {
    if (off[slot + 1] > Buffer<Support>::kMaxCount - off[slot]) return fail(Reason::memory_budget);
    off[slot + 1] += off[slot];
  }
  const u64 total = off[sizes.balls];
  if (total > Buffer<Support>::kMaxCount / (sizeof(Support) + sizeof(u32))) return fail(Reason::memory_budget);
  MHGP11_TRY(budget.admit(HierarchyAdmission::second(total)));
  MHGP11_TRY(out.supports_.allocate(total, budget));
  MHGP11_TRY(out.support_cofaces_.allocate(total, budget));
  return {};
}

// Passe fill : la meme position redonne la meme boule ; meme nombre de supports qu'en count, ordre publie, incidences
// par support, branches recopiees.
Outcome Assembly::fill_body(void* context, u64 begin, u64 end, u32 worker) noexcept {
  Assembly& s = *static_cast<Assembly*>(context);
  if (worker >= s.sizes.workers) return fail(Reason::supports_invariant);
  const FullDomain& domain = s.tree.domain();
  const WindowAttachment& a = s.tree.attachment();
  const auto before = [](const Support& l, const Support& r) noexcept {
    if (l.arity != r.arity) return l.arity < r.arity;
    for (u32 j = 0; j < l.arity; ++j)
      if (l.sites[j] != r.sites[j]) return idx(l.sites[j]) < idx(r.sites[j]);
    return false;
  };
  const std::span<Support> list = s.list_of(worker);
  for (u64 slot = begin; slot < end; ++slot) {
    const Ball& ball = s.out.balls_[slot];
    const auto made = ball_supports(domain, ball.key, list, s.words_of(worker));
    if (!made.ok()) return made.outcome();
    const u64 first = s.out.support_offsets_[slot], count = s.out.support_offsets_[slot + 1] - first;
    if (made.value().count != count) return fail(Reason::supports_invariant);
    const auto shape = make_shape(ball.p, ball.m, ball.qmin, s.tree.order());
    if (!shape.ok()) return shape.outcome();
    std::sort(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(count), before);
    for (u64 j = 0; j < count; ++j) {
      s.out.supports_[first + j] = list[j];
      s.out.support_cofaces_[first + j] = support_cofaces(shape.value(), list[j].arity);
    }
    const u32 at = s.origin[slot];
    std::copy(a.prior().begin() + static_cast<std::ptrdiff_t>(a.prior_offsets()[at]),
              a.prior().begin() + static_cast<std::ptrdiff_t>(a.prior_offsets()[at + 1]),
              s.out.prior_.begin() + static_cast<std::ptrdiff_t>(s.out.prior_offsets_[slot]));
  }
  return {};
}

}  // namespace supports_detail

Result<SupportHierarchy> build_support_hierarchy(const OrderTree& tree, MemoryBudget& budget, sched::Pool* pool,
                                                 HierarchyTimings* timings) noexcept {
  using supports_detail::Assembly;
  Assembly s(tree, budget, pool);
  Stopwatch watch;
  MHGP11_TRY(s.prepass());
  HierarchyTimings measured;
  measured.workers = s.sizes.workers;
  measured.widest = s.sizes.widest;
  measured.admitted[0] = s.sizes.first();
  MHGP11_TRY(budget.admit(measured.admitted[0]));
  MHGP11_TRY(s.allocate_first());
  MHGP11_TRY(s.postorder());
  MHGP11_TRY(s.sort_balls());
  measured.tree_ns = watch.nanoseconds();
  MHGP11_TRY(s.run(s.sizes.balls, &Assembly::count_body));
  MHGP11_TRY(s.close_counts());
  measured.count_ns = watch.nanoseconds() - measured.tree_ns;
  measured.admitted[1] = HierarchyAdmission::second(s.out.supports().size());
  MHGP11_TRY(s.run(s.sizes.balls, &Assembly::fill_body));
  measured.fill_ns = watch.nanoseconds() - measured.tree_ns - measured.count_ns;
  s.finish();
  if (timings != nullptr) *timings = measured;
  return std::move(s.out);
}

}  // namespace mhgp11::supports
