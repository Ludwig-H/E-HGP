// Boites T0 et listes K-certifiees : preparation possedee puis reprise, sans refiltrer un noeud prepare.
#include "catalogue/internal.hpp"

#include <cstring>

#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {
namespace {

// x-lo dans (-M,M), largeur <=M, M=2^B. Carres <3M^2, termes de dominance <12M^2,
// cles du reservoir <12M^2. La marge 2B+5 couvre chaque somme partielle en i64 aux trois profils.
static_assert(2 * kCoordBits + 5 <= 63, "catalogue : dominance T0 et reservoir en i64");

std::array<i64, 3> coordinates(const Cloud& cloud, SiteIdx site) noexcept {
  const u32 i = idx(site);
  return {cloud.x()[i], cloud.y()[i], cloud.z()[i]};
}

// Reservoir de temoins : les cap premiers sites de parent[begin, end) dans l'ordre (distance au centre de box, rang
// dans parent). Insertion stable ; plein, toute distance >= la derniere est ecartee. Rend leur nombre ; positions[i]
// est le rang dans parent. Une ecriture pour les deux voies : prepare_node (tout le parent) et les tranches.
u32 reservoir(const Cloud& cloud, std::span<const SiteIdx> parent, u64 begin, u64 end, const Box& box, u32 cap,
              std::array<i64, 3 * kMaxOrder>& distances, std::array<u32, 3 * kMaxOrder>& positions) noexcept {
  u32 count = 0;
  for (u64 r = begin; r < end; ++r) {
    const auto x = coordinates(cloud, parent[r]);
    i64 distance = 0;
    for (int axis = 0; axis < 3; ++axis) {
      const i64 delta = 2 * x[axis] - box.lo[axis] - box.hi[axis];
      distance += delta * delta;
    }
    if (count == cap && distance >= distances[count - 1]) continue;
    u32 at = count < cap ? count++ : count - 1;
    while (at > 0 && distances[at - 1] > distance) {
      distances[at] = distances[at - 1];
      positions[at] = positions[at - 1];
      --at;
    }
    distances[at] = distance;
    positions[at] = static_cast<u32>(r);  // rang < 2^32 : sites < kNone
  }
  return count;
}

// Termes du test G1 separes par site : 2*largeur*(x-lo) par axe et |x-lo|^2. Leur difference redonne
// exactement 2*largeur*(x-y) ; memes entiers (<2^(2B+2)), donc memes decisions G1.
struct Terms {
  std::array<i64, 3> scaled;
  i64 square;
};

Terms terms(const std::array<i64, 3>& site, const Box& box) noexcept {
  Terms t{};
  for (int axis = 0; axis < 3; ++axis) {
    const i64 x = site[axis] - box.lo[axis];
    t.scaled[axis] = 2 * (box.hi[axis] - box.lo[axis]) * x;
    t.square += x * x;
  }
  return t;
}

// Test G1 du site x contre les temoins prepares, dans leur ordre, jusqu'a kmax dominateurs stricts : ajoute a tests le
// nombre de temoins examines et rend le nombre de dominateurs trouves (le site est retire s'il en a kmax). Une
// ecriture pour les deux voies, comme le reservoir.
u32 dominators(const Terms& x, std::span<const Terms> witnesses, u32 kmax, u64& tests) noexcept {
  u32 found = 0, i = 0;
  for (; i < witnesses.size() && found < kmax; ++i) {
    const auto& y = witnesses[i];
    const i64 right = std::max<i64>(0, x.scaled[0] - y.scaled[0]) + std::max<i64>(0, x.scaled[1] - y.scaled[1]) +
                      std::max<i64>(0, x.scaled[2] - y.scaled[2]);
    found += x.square - y.square > right ? 1u : 0u;  // G1 : egalite conservee sur la fermeture de la boite.
  }
  tests += i;
  return found;
}

Outcome filter(Run& run, std::span<const SiteIdx> parent, const Box& box, Buffer<SiteIdx>& storage,
               u32& count) noexcept {
  MHGP11_TRY(storage.allocate(parent.size(), run.budget));
  const u32 kmax = static_cast<u32>(run.params.kmax);
  const u32 cap = static_cast<u32>(std::min<u64>(parent.size(), 3u * kmax));
  std::array<i64, 3 * kMaxOrder> distances{};
  std::array<u32, 3 * kMaxOrder> positions{};
  const u32 selected = reservoir(run.cloud, parent, 0, parent.size(), box, cap, distances, positions);
  // Temoins pretraites une fois par noeud ; le compte de tests (meme arret a K dominateurs) est ajoute en fin.
  std::array<Terms, 3 * kMaxOrder> prepared{};
  for (u32 i = 0; i < selected; ++i) prepared[i] = terms(coordinates(run.cloud, parent[positions[i]]), box);
  const auto witnesses = std::span<const Terms>(prepared).first(selected);
  u64 tests = 0;  // <= |parent|*3K < 2^38
  count = 0;
  for (SiteIdx s : parent)
    if (dominators(terms(coordinates(run.cloud, s), box), witnesses, kmax, tests) < kmax) storage[count++] = s;
  return checked_add(run.ledger.filter_tests, tests);
}

Box envelope(const Cloud& cloud, std::span<const SiteIdx> sites) noexcept {
  const auto initial = coordinates(cloud, sites.front());
  Box box{initial, initial};
  for (SiteIdx s : sites) {
    const auto x = coordinates(cloud, s);
    for (int axis = 0; axis < 3; ++axis) {
      box.lo[axis] = std::min(box.lo[axis], x[axis]);
      box.hi[axis] = std::max(box.hi[axis], x[axis]);
    }
  }
  for (i64& high : box.hi) ++high;  // 2^B representable ; garde le centre au maximum exact.
  return box;
}

Outcome process(Run& run, std::span<const SiteIdx> parent, const Box& box, u32 depth) noexcept {
  ReadyNode ready;
  MHGP11_TRY(prepare_node(run, parent, box, depth, ready));
  return ready.count == 0 ? Outcome{} : run_ready(run, ready);
}

}  // namespace

Outcome NodeQuota::claim() noexcept {
  if (limit_ == 0) return {};
  u64 before = claimed_.load(std::memory_order_relaxed);
  while (before < limit_) {
    if (claimed_.compare_exchange_weak(before, before + 1, std::memory_order_relaxed)) return {};
  }
  return fail(Reason::node_budget);
}

Outcome prepare_node(Run& run, std::span<const SiteIdx> parent, const Box& box, u32 depth,
                     ReadyNode& ready) noexcept {
  ready = ReadyNode{};
  if (depth > kMaxDepth) return fail(Reason::catalogue_invariant);
  if (run.quota != nullptr) {
    if (run.quota->limit() != run.params.max_nodes) return fail(Reason::catalogue_invariant);
    MHGP11_TRY(run.quota->claim());
  }
  MHGP11_TRY(checked_add(run.ledger.nodes, 1));
  if (run.quota == nullptr && run.params.max_nodes != 0 && run.ledger.nodes > run.params.max_nodes)
    return fail(Reason::node_budget);
  run.ledger.max_depth = std::max<u64>(run.ledger.max_depth, depth);
  u32 count = 0;
  MHGP11_TRY(filter(run, parent, box, ready.storage, count));
  if (count == 0) return {};
  const auto sites = ready.storage.span().first(count);
  Box adjusted = envelope(run.cloud, sites);
  for (int i = 0; i < 3; ++i) {
    adjusted.lo[i] = std::max(adjusted.lo[i], box.lo[i]);
    adjusted.hi[i] = std::min(adjusted.hi[i], box.hi[i]);
    if (adjusted.lo[i] >= adjusted.hi[i]) return {};
  }
  ready.count = count;
  ready.depth = depth;
  ready.box = adjusted;
  return {};
}

namespace {

// Preparation par tranches (prepare_nodes_chunked). Une tranche : sites [begin, end) du parent d'un noeud. Le
// reservoir du filtre est l'ensemble des cap premiers sites dans l'ordre (distance au centre, rang dans le parent) :
// reservoir() les insere dans l'ordre du parent et ecarte, plein, toute distance >= la derniere. Le reservoir de chaque
// tranche, puis la fusion de ces listes dans l'ordre lexicographique (distance, rang), le rendent exactement.
struct PrepareChunk {
  u32 job, count, kept;
  u64 begin, end, tests;
  std::array<i64, 3 * kMaxOrder> distance;
  std::array<u32, 3 * kMaxOrder> position;  // rang dans le parent, < 2^32 (sites < kNone)
  std::array<i64, 3> lo, hi;                // enveloppe des sites gardes de la tranche, si kept > 0
};
struct PreparedWitnesses {
  u32 selected;
  std::array<Terms, 3 * kMaxOrder> terms;
};

bool before(i64 d, u32 p, i64 e, u32 q) noexcept { return d < e || (d == e && p < q); }

struct ChunkedPrepare {
  const Cloud& cloud;
  std::span<const NodeJob> jobs;
  std::span<PrepareChunk> chunks;
  std::span<PreparedWitnesses> witnesses;
  u32 kmax;

  static Outcome reservoirs(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<ChunkedPrepare*>(context);
    for (u64 t = begin; t < end; ++t) {
      PrepareChunk& c = self.chunks[t];
      const NodeJob& job = self.jobs[c.job];
      const u32 cap = static_cast<u32>(std::min<u64>(job.parent.size(), 3u * self.kmax));
      c.count = reservoir(self.cloud, job.parent, c.begin, c.end, job.box, cap, c.distance, c.position);
    }
    return {};
  }

  static Outcome filters(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<ChunkedPrepare*>(context);
    for (u64 t = begin; t < end; ++t) {
      PrepareChunk& c = self.chunks[t];
      const NodeJob& job = self.jobs[c.job];
      const auto witnesses = std::span<const Terms>(self.witnesses[c.job].terms).first(self.witnesses[c.job].selected);
      SiteIdx* out = job.ready->storage.data() + c.begin;  // les gardes de la tranche, au debut de sa place
      u32 kept = 0;
      u64 tests = 0;
      for (u64 r = c.begin; r < c.end; ++r) {
        const SiteIdx s = job.parent[r];
        const auto xyz = coordinates(self.cloud, s);
        if (dominators(terms(xyz, job.box), witnesses, self.kmax, tests) >= self.kmax) continue;
        for (int axis = 0; axis < 3; ++axis) {
          c.lo[axis] = kept == 0 ? xyz[axis] : std::min(c.lo[axis], xyz[axis]);
          c.hi[axis] = kept == 0 ? xyz[axis] : std::max(c.hi[axis], xyz[axis]);
        }
        out[kept++] = s;
      }
      c.kept = kept;
      c.tests = tests;
    }
    return {};
  }
};

}  // namespace

u64 prepare_chunks(u64 sites, u64 chunk) noexcept { return chunk == 0 ? 0 : (sites + chunk - 1) / chunk; }

Outcome prepare_scratch_bytes(u64 chunks, u64 jobs, u64& bytes) noexcept {
  MHGP11_TRY(add_bytes<PrepareChunk>(bytes, chunks));
  return add_bytes<PreparedWitnesses>(bytes, jobs);
}

Outcome prepare_nodes_chunked(Run& run, sched::Pool& pool, std::span<const NodeJob> jobs, u64 chunk) noexcept {
  if (chunk == 0 || run.params.kmax < 1 || static_cast<u32>(run.params.kmax) > kMaxOrder)
    return fail(Reason::parameter_out_of_range);
  // Pilote, dans l'ordre des noeuds : memes controles, quota, registre et liste que prepare_node.
  u64 chunks = 0;
  for (const NodeJob& job : jobs) {
    if (job.ready == nullptr || job.ledger == nullptr || job.parent.empty()) return fail(Reason::catalogue_invariant);
    *job.ready = ReadyNode{};
    *job.ledger = CatalogueLedger{};
    if (job.depth > kMaxDepth) return fail(Reason::catalogue_invariant);
    if (run.quota != nullptr) {
      if (run.quota->limit() != run.params.max_nodes) return fail(Reason::catalogue_invariant);
      MHGP11_TRY(run.quota->claim());
    }
    job.ledger->nodes = 1;
    if (run.quota == nullptr && run.params.max_nodes != 0 && job.ledger->nodes > run.params.max_nodes)
      return fail(Reason::node_budget);
    job.ledger->max_depth = job.depth;
    MHGP11_TRY(job.ready->storage.allocate(job.parent.size(), run.budget));
    MHGP11_TRY(checked_add(chunks, prepare_chunks(job.parent.size(), chunk)));
  }
  Buffer<PrepareChunk> table;
  Buffer<PreparedWitnesses> prepared;
  MHGP11_TRY(table.allocate(chunks, run.budget));
  MHGP11_TRY(prepared.allocate(jobs.size(), run.budget));
  u64 at = 0;
  for (u32 j = 0; j < jobs.size(); ++j)
    for (u64 b = 0; b < jobs[j].parent.size(); b += chunk) {
      PrepareChunk& c = table[at++];
      c.job = j; c.count = 0; c.kept = 0; c.tests = 0;
      c.begin = b; c.end = std::min<u64>(b + chunk, jobs[j].parent.size());
    }
  if (at != chunks) return fail(Reason::catalogue_invariant);
  const u32 kmax = static_cast<u32>(run.params.kmax);
  ChunkedPrepare work{run.cloud, jobs, table.span(), prepared.span(), kmax};
  MHGP11_TRY(pool.parallel_for(chunks, 1, &work, ChunkedPrepare::reservoirs));
  // Fusion des reservoirs, noeud par noeud : les cap premiers dans l'ordre (distance, rang), puis les termes G1.
  at = 0;
  for (u32 j = 0; j < jobs.size(); ++j) {
    const NodeJob& job = jobs[j];
    const u32 cap = static_cast<u32>(std::min<u64>(job.parent.size(), 3u * kmax));
    std::array<i64, 3 * kMaxOrder> distance{};
    std::array<u32, 3 * kMaxOrder> position{};
    u32 count = 0;
    for (; at < chunks && table[at].job == j; ++at) {
      const PrepareChunk& c = table[at];
      for (u32 e = 0; e < c.count; ++e) {
        const i64 d = c.distance[e];
        const u32 p = c.position[e];
        if (count == cap && !before(d, p, distance[count - 1], position[count - 1])) continue;
        u32 slot = count < cap ? count++ : count - 1;
        while (slot > 0 && before(d, p, distance[slot - 1], position[slot - 1])) {
          distance[slot] = distance[slot - 1];
          position[slot] = position[slot - 1];
          --slot;
        }
        distance[slot] = d;
        position[slot] = p;
      }
    }
    prepared[j].selected = count;
    for (u32 i = 0; i < count; ++i) prepared[j].terms[i] = terms(coordinates(run.cloud, job.parent[position[i]]), job.box);
  }
  MHGP11_TRY(pool.parallel_for(chunks, 1, &work, ChunkedPrepare::filters));
  // Pilote : gardes rendus contigus dans l'ordre des tranches, enveloppe, boite ajustee, comme prepare_node.
  at = 0;
  for (u32 j = 0; j < jobs.size(); ++j) {
    const NodeJob& job = jobs[j];
    SiteIdx* storage = job.ready->storage.data();
    u64 count = 0, tests = 0;
    Box adjusted;
    for (; at < chunks && table[at].job == j; ++at) {
      const PrepareChunk& c = table[at];
      MHGP11_TRY(checked_add(tests, c.tests));
      if (c.kept == 0) continue;
      if (c.begin != count) std::memmove(storage + count, storage + c.begin, c.kept * sizeof(SiteIdx));
      for (int axis = 0; axis < 3; ++axis) {
        adjusted.lo[axis] = count == 0 ? c.lo[axis] : std::min(adjusted.lo[axis], c.lo[axis]);
        adjusted.hi[axis] = count == 0 ? c.hi[axis] : std::max(adjusted.hi[axis], c.hi[axis]);
      }
      count += c.kept;
    }
    MHGP11_TRY(checked_add(job.ledger->filter_tests, tests));
    if (count == 0) continue;
    bool empty = false;
    for (int i = 0; i < 3; ++i) {
      ++adjusted.hi[i];  // comme envelope : 2^B representable
      adjusted.lo[i] = std::max(adjusted.lo[i], job.box.lo[i]);
      adjusted.hi[i] = std::min(adjusted.hi[i], job.box.hi[i]);
      empty = empty || adjusted.lo[i] >= adjusted.hi[i];
    }
    if (empty) continue;
    job.ready->count = static_cast<u32>(count);
    job.ready->depth = job.depth;
    job.ready->box = adjusted;
  }
  return {};
}

bool split_ready(const ReadyNode& ready, const CatalogueParams& params, Box& left, Box& right) noexcept {
  int axis = 0;
  for (int i = 1; i < 3; ++i)
    if (ready.box.hi[i] - ready.box.lo[i] > ready.box.hi[axis] - ready.box.lo[axis]) axis = i;
  const i64 width = ready.box.hi[axis] - ready.box.lo[axis];
  if (ready.count <= params.leaf_size || width <= 1) return false;
  const i64 middle = ready.box.lo[axis] + width / 2;
  left = ready.box;
  right = ready.box;
  left.hi[axis] = middle;
  right.lo[axis] = middle;
  return true;
}

Outcome run_ready(Run& run, const ReadyNode& ready) noexcept {
  if (ready.count == 0 || ready.count > ready.storage.size() || ready.depth > kMaxDepth)
    return fail(Reason::catalogue_invariant);
  Box left, right;
  if (split_ready(ready, run.params, left, right)) {
    MHGP11_TRY(process(run, ready.sites(), left, ready.depth + 1));
    return process(run, ready.sites(), right, ready.depth + 1);
  }
  MHGP11_TRY(checked_add(run.ledger.leaves, 1));
  run.ledger.max_leaf = std::max<u64>(run.ledger.max_leaf, ready.count);
  if (ready.count > run.params.max_leaf) return fail(Reason::wide_leaf);
  return enumerate_leaf(run, ready.sites(), ready.box);
}

Outcome make_root(Run& run, Buffer<SiteIdx>& root, Box& box) noexcept {
  if (run.cloud.sites() == 0) return fail(Reason::empty_input);
  MHGP11_TRY(root.allocate(run.cloud.sites(), run.budget));
  for (u32 i = 0; i < run.cloud.sites(); ++i) root[i] = make_id<SiteIdx>(i);
  box = envelope(run.cloud, root.span());
  return {};
}

Outcome walk(Run& run) noexcept {
  Buffer<SiteIdx> root;
  Box box;
  MHGP11_TRY(make_root(run, root, box));
  // Racine rectangulaire ; somme ceil(log2 largeur)<=3B. Chaque coupe diminue ce potentiel d'au moins un,
  // les ajustements ne l'augmentent pas. Au plus 3B+1 listes filtrees simultanees, plus la liste racine.
  return process(run, root.span(), box, 0);
}

}  // namespace mhgp11::catalogue_detail
