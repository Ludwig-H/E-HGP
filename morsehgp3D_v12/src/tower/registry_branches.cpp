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
// bornee par les representants des cellules retenues. Raccourci (levier R1, critere prouve par l'auditeur Codex,
// recu registre_classe_unique du 8 octobre) : si la classe M de la cellule retenue t n'a que t pour contributrice,
// ses branches sont exactement les enfants (tries, distincts) de cette classe ; c'est le cas si et seulement si
// q == d + 1, ou d est le nombre d'evenements de t (son bloc de event_cell) et q l'arite de sa classe z = cell_node[t].
// Ces lignes directes ne relisent aucun representant (longueur de travail nulle, aucun pointeur dans branch_nodes,
// alloue seulement si une ligne generale existe) ; les autres gardent les requetes, le tri et l'unicite. branch_reads
// compte les representants reellement relus (Q_g, lignes generales).
//
// Deux passes paralleles par morceaux de lignes, ecritures disjointes. Deux admissions : le travail et les lignes
// avant la premiere passe (12 R + 8 (R + 1) + 4 Q_g + 4 R), puis la sortie (branches ET decalages de leur CSR,
// 4 A + 8 (R + 1), recu audit_registre_branches_20261007) apres le comptage, quand tout le reste de l'etage est
// deja alloue. Etapes par ordre (lignes, passe 1 par morceaux, decalages et sortie, passe 2, liberation) partagees
// par run_registry et la Session recouverte (pipeline.cpp, admission unique bornee par registry_bytes_bound).
#include <algorithm>

#include "sched/sched.hpp"
#include "tower/forest_internal.hpp"

namespace mhgp12::tower::detail {
namespace {

// Bloc d'evenements [begin, end) de la cellule retenue t = event_cell[begin] (blocs consecutifs) ; reads : nombre de
// representants a relire, 0 pour une ligne directe (q == d + 1). Le meme calcul sert a l'admission de run_registry et
// aux decalages de prepare_rows. Controles locaux (ne remplacent pas la validation de T et M) : cellule et noeud dans
// leur domaine, noeud de fusion, arite entre 2 et naissances, au moins deux representants.
Outcome plan_row(const ForestInput& in, const OrderForest& f, u64 begin, u64& end, u64& reads) noexcept {
  const u32 t = f.event_cell[begin];
  end = begin + 1;
  while (end < f.event_cell.size() && f.event_cell[end] == t) ++end;
  if (t >= in.cell_ball.size() || t >= f.cell_node.size()) return fail(Reason::tower_invariant);
  const u32 z = f.cell_node[t];
  if (z < f.births || z >= f.nodes()) return fail(Reason::tower_invariant);
  const u64 q = f.children.off[u64{z} + 1] - f.children.off[z];
  const u64 n = in.rep_offsets[u64{t} + 1] - in.rep_offsets[t];
  if (q < 2 || q > f.births || n < 2) return fail(Reason::tower_invariant);
  reads = q == end - begin + 1 ? 0 : n;
  return {};
}

// Passe 1 d'une ligne : noeud a la coupe ouverte de chaque representant, tri, doublons retires ; nombre de branches.
// Ligne directe (longueur de travail nulle) : arite de sa classe, sans relire de representant ni former de pointeur
// dans branch_nodes.
Outcome collect_row(const ForestInput& in, const OrderForest& f, OrderWork& w, u64 j, ForestWork& c) noexcept {
  const u32 t = f.retained_cell[j], r = f.retained_rank[j];
  if (r == 0) return fail(Reason::tower_invariant);
  if (w.branch_off[j + 1] == w.branch_off[j]) {
    const u32 z = f.cell_node[t];
    w.branch_count[j] = static_cast<u32>(f.children.off[u64{z} + 1] - f.children.off[z]);  // q <= naissances (plan_row)
    c.branches += w.branch_count[j];
    return {};
  }
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

}  // namespace

// Lignes de l'ordre i : cellules retenues (blocs consecutifs de event_cell) ; decalages des seuls representants a
// relire (longueur nulle : ligne directe) ; branch_nodes alloue seulement si une ligne generale existe.
Outcome prepare_rows(BuildState& s, u32 i, u64& reads) noexcept {
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
  for (u64 e = 0, j = 0; e < f.event_cell.size(); ++j) {
    u64 end = 0, count = 0;
    MHGP12_TRY(plan_row(in, f, e, end, count));
    const u32 t = f.event_cell[e];
    if (j > 0 && t <= f.retained_cell[j - 1]) return fail(Reason::tower_invariant);
    f.retained_cell[j] = t;
    f.retained_ball[j] = idx(in.cell_ball[t]);
    f.retained_rank[j] = idx(in.cell_rank[t]);
    w.branch_off[j + 1] = w.branch_off[j] + count;
    e = end;
  }
  reads = w.branch_off[rows];
  if (reads > 0) MHGP12_TRY(w.branch_nodes.allocate(reads, s.budget));
  if (rows > 0) MHGP12_TRY(w.branch_count.allocate(rows, s.budget));
  return {};
}

Outcome collect_rows(BuildState& s, u32 i, u64 begin, u64 end, ForestWork& counters) noexcept {
  for (u64 j = begin; j < end; ++j)
    MHGP12_TRY(collect_row(s.inputs[i], s.forests.orders[i], s.work[i], j, counters));
  return {};
}

// Decalages exacts des branches de l'ordre i (sommes prefixes), puis reservation de la sortie.
Outcome place_rows(BuildState& s, u32 i) noexcept {
  OrderForest& f = s.forests.orders[i];
  OrderWork& w = s.work[i];
  const u64 rows = f.retained_cell.size();
  MHGP12_TRY(f.branches.off.allocate(rows + 1, s.budget));
  f.branches.off[0] = 0;
  for (u64 j = 0; j < rows; ++j) f.branches.off[j + 1] = f.branches.off[j] + w.branch_count[j];
  if (f.branches.off[rows] > 0) MHGP12_TRY(f.branches.val.allocate(f.branches.off[rows], s.budget));
  return {};
}

void fill_rows(BuildState& s, u32 i, u64 begin, u64 end) noexcept {
  OrderForest& f = s.forests.orders[i];
  const OrderWork& w = s.work[i];
  for (u64 j = begin; j < end; ++j) {
    const u32* from = nullptr;
    if (w.branch_off[j + 1] == w.branch_off[j]) {  // ligne directe : enfants tries de la classe (ordre canonique)
      const u32 z = f.cell_node[f.retained_cell[j]];
      from = f.children.val.data() + f.children.off[z];
    } else {
      from = w.branch_nodes.data() + w.branch_off[j];
    }
    std::copy(from, from + w.branch_count[j], f.branches.val.data() + f.branches.off[j]);
  }
}

void close_rows(BuildState& s, u32 i) noexcept {
  s.work[i].branch_nodes.reset();
  s.work[i].branch_off.reset();
  s.work[i].branch_count.reset();
}

u64 registry_bytes_bound(const ForestInput& in, u64 events) noexcept {
  const u64 rows = events, reads = in.targets.size();
  // lignes (cellule, boule, rang), decalages, noeuds relus, comptes ; sortie : branches et decalages de leur CSR
  return 12 * rows + 8 * (rows + 1) + 4 * reads + 4 * rows + 4 * reads + 8 * (rows + 1);
}

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

Outcome branch_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& run = *static_cast<BranchRun*>(context);
  BuildState& s = run.state;
  Outcome out;
  for (u64 k = begin; k < end; ++k) {
    BranchTask& task = run.tasks[k];
    Stopwatch watch;
    if (!run.fill) out = merge(out, collect_rows(s, task.order, task.begin, task.end, run.counters[k]));
    else fill_rows(s, task.order, task.begin, task.end);
    task.ns += watch.nanoseconds();
  }
  return out;
}

}  // namespace

Outcome run_registry(BuildState& s, sched::Pool& pool) noexcept {
  Stopwatch watch;
  u64 bytes = 0, tasks_count = 0;
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    u64 rows = 0, reads = 0;
    const OrderForest& f = s.forests.orders[i];
    for (u64 e = 0; e < f.event_cell.size();) {
      u64 end = 0, count = 0;
      MHGP12_TRY(plan_row(s.inputs[i], f, e, end, count));
      ++rows;
      reads += count;
      e = end;
    }
    bytes += 12 * rows + 8 * (rows + 1) + 4 * reads + 4 * rows;  // lignes, decalages, noeuds relus (Q_g), comptes
    tasks_count += (rows + kBranchChunk - 1) / kBranchChunk;
  }
  MHGP12_TRY(admit_stage(s, kStageR, bytes + tasks_count * (sizeof(BranchTask) + sizeof(ForestWork))));
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    u64 reads = 0;
    MHGP12_TRY(prepare_rows(s, static_cast<u32>(i), reads));
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
  for (u64 i = 0; i < s.inputs.size(); ++i) MHGP12_TRY(place_rows(s, static_cast<u32>(i)));
  run.fill = true;
  MHGP12_TRY(pool.parallel_for(tasks_count, 1, &run, branch_body));
  for (u64 k = 0; k < tasks_count; ++k) {
    ForestWork& c = s.counters[tasks[k].order];
    c.branch_reads += counters[k].branch_reads;
    c.branches += counters[k].branches;
    s.physical[tasks[k].order].registry_ns += tasks[k].ns;
  }
  for (u64 i = 0; i < s.inputs.size(); ++i) close_rows(s, static_cast<u32>(i));
  s.registry_ns = watch.nanoseconds();
  return {};
}

}  // namespace mhgp12::tower::detail
