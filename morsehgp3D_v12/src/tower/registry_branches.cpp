// Etage R : hyperaretes retenues par Kruskal et leurs branches ouvertes ant(b) (CONTRAT_TOUR.md, paragraphe 5 ;
// ARCHITECTURE.md, paragraphe 4.4). Une cellule est retenue si le noyau y a uni au moins deux composantes ; ses branches
// sont les noeuds vivants a la coupe OUVERTE de son rang r qui contiennent ses representants, y compris ceux qu'une
// autre cellule du meme plateau a deja reunis : les evenements binaires, les attaches et la foret ne les determinent
// pas (recu de l'auditeur Codex audit_t1b_tour_prepublication_20261007, temoins {0,1} puis {0,2} ou {1,2}).
//
// Variante de raccord apres M, admise par ce recu : pour chaque representant, un temoin de naissance immuable l (sa
// naissance ; pour une cible << cellule >>, la plus petite naissance du sommet laisse par la cellule cible, de rang
// strictement inferieur), puis component_at(l, r - 1) : les rangs sont entiers, la coupe fermee r - 1 est la coupe
// ouverte r, et rang(l) <= r - 1 tient par la date de LEM-T3. Aucune barriere par plateau, aucune descente de G.
// Comptage puis reservation exacte : la sortie A (somme des branches) n'est pas bornee par naissances - 1 (sept
// naissances au meme plateau, cellules {0,1}, {0,1,2}, ..., {0,..,6} : six evenements, 27 branches) ; elle est
// bornee par les representants relus. Deux passes paralleles par morceaux de lignes, ecritures disjointes. Deux
// admissions : le travail et les lignes avant la premiere passe, puis la sortie (branches ET decalages de leur CSR,
// recu audit_registre_branches_20261007) apres le comptage, quand tout le reste de l'etage est deja alloue.
#include <algorithm>

#include "sched/sched.hpp"
#include "tower/forest_internal.hpp"

namespace mhgp12::tower::detail {
namespace {

inline constexpr u64 kBranchChunk = 2048;  // lignes (cellules retenues) par morceau

struct BranchTask {
  u32 order = 0;
  u64 begin = 0, end = 0, ns = 0;
};
struct BranchRun {
  BuildState& state;
  std::span<BranchTask> tasks;
  std::span<ForestWork> counters;
  bool fill;
};

// Lignes de l'ordre i : cellules retenues (blocs consecutifs de event_cell) ; decalages de leurs representants.
Outcome prepare_rows(BuildState& s, u64 i, u64& reads) noexcept {
  OrderForest& f = s.forests.orders[i];
  OrderWork& w = s.work[i];
  const ForestInput& in = s.inputs[i];
  u64 rows = 0;
  for (u64 e = 0; e < f.event_cell.size(); ++e) rows += e == 0 || f.event_cell[e] != f.event_cell[e - 1];
  MHGP12_TRY(f.retained_cell.allocate(rows, s.budget));
  MHGP12_TRY(f.retained_ball.allocate(rows, s.budget));
  MHGP12_TRY(f.retained_rank.allocate(rows, s.budget));
  MHGP12_TRY(w.branch_off.allocate(rows + 1, s.budget));
  w.branch_off[0] = 0;
  for (u64 e = 0, j = 0; e < f.event_cell.size(); ++e) {
    if (e > 0 && f.event_cell[e] == f.event_cell[e - 1]) continue;
    const u32 t = f.event_cell[e];
    if (t >= in.cell_ball.size() || (j > 0 && t <= f.retained_cell[j - 1])) return fail(Reason::tower_invariant);
    f.retained_cell[j] = t;
    f.retained_ball[j] = idx(in.cell_ball[t]);
    f.retained_rank[j] = idx(in.cell_rank[t]);
    w.branch_off[j + 1] = w.branch_off[j] + (in.rep_offsets[t + 1] - in.rep_offsets[t]);
    ++j;
  }
  reads = w.branch_off[rows];
  if (rows > 0) {
    MHGP12_TRY(w.branch_nodes.allocate(reads, s.budget));
    MHGP12_TRY(w.branch_count.allocate(rows, s.budget));
  }
  return {};
}

// Passe 1 d'une ligne : noeud a la coupe ouverte de chaque representant, tri, doublons retires ; nombre de branches.
Outcome collect_row(const ForestInput& in, const OrderForest& f, OrderWork& w, u64 j, ForestWork& c) noexcept {
  const u32 t = f.retained_cell[j], r = f.retained_rank[j];
  if (r == 0) return fail(Reason::tower_invariant);
  u32* row = w.branch_nodes.data() + w.branch_off[j];
  u64 n = 0;
  for (u64 p = in.rep_offsets[t]; p < in.rep_offsets[t + 1]; ++p) {
    const u32 target = in.targets[p], index = target_index(target);
    const u32 witness = target_is_cell(target) ? f.minleaf[f.cell_node[index]] : f.birth_node[index];
    auto open = component_at(f, witness, r - 1);
    if (!open.ok()) return open.outcome();
    row[n++] = open.value();
  }
  std::sort(row, row + n);
  const u64 distinct = static_cast<u64>(std::unique(row, row + n) - row);
  if (distinct < 2) return fail(Reason::tower_invariant);  // une cellule retenue unit au moins deux composantes
  w.branch_count[j] = static_cast<u32>(distinct);
  c.branch_reads += n;
  c.branches += distinct;
  return {};
}

Outcome branch_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& run = *static_cast<BranchRun*>(context);
  BuildState& s = run.state;
  Outcome out;
  for (u64 k = begin; k < end; ++k) {
    BranchTask& task = run.tasks[k];
    const ForestInput& in = s.inputs[task.order];
    OrderForest& f = s.forests.orders[task.order];
    OrderWork& w = s.work[task.order];
    Stopwatch watch;
    for (u64 j = task.begin; j < task.end && out.ok(); ++j) {
      if (!run.fill) {
        out = merge(out, collect_row(in, f, w, j, run.counters[k]));
      } else {
        const u32* from = w.branch_nodes.data() + w.branch_off[j];
        std::copy(from, from + w.branch_count[j], f.branches.val.data() + f.branches.off[j]);
      }
    }
    task.ns += watch.nanoseconds();
  }
  return out;
}

// Decalages exacts des branches de l'ordre i (sommes prefixes), puis reservation de la sortie.
Outcome place_rows(BuildState& s, u64 i) noexcept {
  OrderForest& f = s.forests.orders[i];
  OrderWork& w = s.work[i];
  const u64 rows = f.retained_cell.size();
  MHGP12_TRY(f.branches.off.allocate(rows + 1, s.budget));
  f.branches.off[0] = 0;
  for (u64 j = 0; j < rows; ++j) f.branches.off[j + 1] = f.branches.off[j] + w.branch_count[j];
  if (f.branches.off[rows] > 0) MHGP12_TRY(f.branches.val.allocate(f.branches.off[rows], s.budget));
  return {};
}

}  // namespace

Outcome run_registry(BuildState& s, sched::Pool& pool) noexcept {
  Stopwatch watch;
  u64 bytes = 0, tasks_count = 0;
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    u64 rows = 0;
    const OrderForest& f = s.forests.orders[i];
    for (u64 e = 0; e < f.event_cell.size(); ++e) rows += e == 0 || f.event_cell[e] != f.event_cell[e - 1];
    u64 reads = 0;
    for (u64 e = 0; e < f.event_cell.size(); ++e)
      if (e == 0 || f.event_cell[e] != f.event_cell[e - 1])
        reads += s.inputs[i].rep_offsets[f.event_cell[e] + 1] - s.inputs[i].rep_offsets[f.event_cell[e]];
    bytes += 12 * rows + 8 * (rows + 1) + 4 * reads + 4 * rows;  // lignes, decalages, noeuds relus, comptes
    tasks_count += (rows + kBranchChunk - 1) / kBranchChunk;
  }
  MHGP12_TRY(admit_stage(s, kStageR, bytes + tasks_count * (sizeof(BranchTask) + sizeof(ForestWork))));
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    u64 reads = 0;
    MHGP12_TRY(prepare_rows(s, i, reads));
  }
  Buffer<BranchTask> tasks;
  Buffer<ForestWork> counters;
  if (tasks_count > 0) {
    MHGP12_TRY(tasks.allocate(tasks_count, s.budget));
    MHGP12_TRY(counters.allocate(tasks_count, s.budget));
  }
  for (u64 i = 0, k = 0; i < s.inputs.size(); ++i) {
    const u64 rows = s.forests.orders[i].retained_cell.size();
    for (u64 b = 0; b < rows; b += kBranchChunk)
      tasks[k++] = BranchTask{static_cast<u32>(i), b, std::min(rows, b + kBranchChunk), 0};
  }
  for (u64 k = 0; k < tasks_count; ++k) counters[k] = ForestWork{};
  BranchRun run{s, tasks.span(), counters.span(), false};
  MHGP12_TRY(pool.parallel_for(tasks_count, 1, &run, branch_body));
  u64 output = 0, output_offsets = 0;
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    output_offsets += 8 * (s.forests.orders[i].retained_cell.size() + 1);
    for (u64 j = 0; j < s.forests.orders[i].retained_cell.size(); ++j) output += s.work[i].branch_count[j];
  }
  // sortie exacte : 4 octets par branche, et les decalages de la CSR (8 octets par ligne, plus un, meme sans ligne)
  MHGP12_TRY(admit_stage(s, kStageR, 4 * output + output_offsets));
  for (u64 i = 0; i < s.inputs.size(); ++i) MHGP12_TRY(place_rows(s, i));
  run.fill = true;
  MHGP12_TRY(pool.parallel_for(tasks_count, 1, &run, branch_body));
  for (u64 k = 0; k < tasks_count; ++k) {
    ForestWork& c = s.counters[tasks[k].order];
    c.branch_reads += counters[k].branch_reads;
    c.branches += counters[k].branches;
    s.physical[tasks[k].order].registry_ns += tasks[k].ns;
  }
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    s.work[i].branch_nodes.reset();
    s.work[i].branch_off.reset();
    s.work[i].branch_count.reset();
  }
  s.registry_ns = watch.nanoseconds();
  return {};
}

}  // namespace mhgp12::tower::detail
