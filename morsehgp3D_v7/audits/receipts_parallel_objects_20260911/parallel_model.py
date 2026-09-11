"""Bounded model: immutable dated forests and independent occurrence queries.

The sorted union-find constructor is a TEST ORACLE, not a delivered parallel
forest producer. The parallel structure model uses synchronous double buffers;
this Python witness does not launch threads, execute product code or time work.
"""

from __future__ import annotations

from bisect import bisect_left
from dataclasses import dataclass
from fractions import Fraction as F
import importlib.util
import json
from pathlib import Path
import sys


SOURCE = Path(__file__).resolve().parents[1] / "receipts_filtered_graph_20260911/graph_model.py"
SPEC = importlib.util.spec_from_file_location("mhgp7_audit_filtered_graph", SOURCE)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("cannot load the pinned, independent graph model")
G = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = G
SPEC.loader.exec_module(G)
Hub = G.Hub
require = G.require


@dataclass(frozen=True)
class Node:
    level: F
    children: tuple[int, ...]
    leaves: tuple[str, ...]


@dataclass(frozen=True)
class Forest:
    nodes: tuple[Node, ...]
    parent: tuple[int, ...]
    jumps: tuple[tuple[int, ...], ...]
    leaf_ids: tuple[tuple[str, int], ...]


@dataclass(frozen=True)
class HeavyChains:
    chains: tuple[tuple[int, ...], ...]
    node_chain: tuple[int, ...]
    position: tuple[int, ...]


def leaf_id(forest: Forest, leaf: str) -> int:
    # String names keep this witness readable. A product occurrence stores
    # its integer leaf id directly; the witness resolves names by binary search.
    index = bisect_left(forest.leaf_ids, (leaf, -1))
    require(index < len(forest.leaf_ids) and forest.leaf_ids[index][0] == leaf,
            "unknown leaf id")
    return forest.leaf_ids[index][1]


def heavy_chains(forest: Forest) -> HeavyChains:
    """Sequential MODEL builder; storage has three integer entries per node."""
    size = []
    heavy = []
    for node in forest.nodes:
        size.append(1 + sum(size[c] for c in node.children))
        heavy.append(min(node.children, key=lambda c: (-size[c], c)) if node.children else None)
    chains = []
    node_chain = [-1] * len(forest.nodes)
    position = [-1] * len(forest.nodes)
    pending = [i for i, parent in enumerate(forest.parent) if i == parent]
    while pending:
        node_id = pending.pop()
        chain = []
        while node_id is not None:
            node_chain[node_id] = len(chains)
            position[node_id] = len(chain)
            chain.append(node_id)
            pending.extend(c for c in forest.nodes[node_id].children if c != heavy[node_id])
            node_id = heavy[node_id]
        chains.append(tuple(chain))
    require(all(i >= 0 for i in node_chain + position), "heavy chains lose a node")
    return HeavyChains(tuple(chains), tuple(node_chain), tuple(position))


def chain_query(forest: Forest, layout: HeavyChains, leaf: str,
                cut: F, closed: bool) -> tuple[int | None, int, int]:
    """O(log N) query, O(N) shared layout; no observable artificial root."""
    node_id = leaf_id(forest, leaf)
    light_steps = binary_steps = 0
    if not active(forest.nodes[node_id].level, cut, closed):
        return None, light_steps, binary_steps
    while True:
        chain = layout.chains[layout.node_chain[node_id]]
        head = chain[0]
        if not active(forest.nodes[head].level, cut, closed):
            # Dates decrease from head toward descendants. The current node
            # is active, so this single final search always has an answer.
            lo, hi = 0, layout.position[node_id]
            while lo < hi:
                mid = (lo + hi) // 2
                binary_steps += 1
                if active(forest.nodes[chain[mid]].level, cut, closed):
                    hi = mid
                else:
                    lo = mid + 1
            return chain[lo], light_steps, binary_steps
        above = forest.parent[head]
        if above == head or not active(forest.nodes[above].level, cut, closed):
            return head, light_steps, binary_steps
        node_id = above
        light_steps += 1


def active(level: F, cut: F, closed: bool) -> bool:
    return level <= cut if closed else level < cut


def validate_forest(nodes: tuple[Node, ...], parent: tuple[int, ...]) -> None:
    require(len(nodes) == len(parent), "parent cardinal mismatch")
    for i, node in enumerate(nodes):
        require(bool(node.leaves), "empty public component")
        require(len(node.children) != 1, "unary public event")
        require(0 <= parent[i] < len(nodes), "invalid parent")
        if node.children:
            require(all(0 <= c < i for c in node.children), "non-topological children")
            require(all(parent[c] == i for c in node.children), "parent mismatch")
            # Born leaves may share a fusion date, but equal-date internal
            # nodes must be contracted into a single simultaneous multifusion.
            require(all(nodes[c].level <= node.level for c in node.children), "time reversal")
            require(all(not nodes[c].children or nodes[c].level < node.level
                        for c in node.children), "uncontracted simultaneous fusion")
            merged = tuple(sorted(x for c in node.children for x in nodes[c].leaves))
            require(merged == node.leaves and len(set(merged)) == len(merged),
                    "component identities lost or duplicated")
        if parent[i] != i:
            require(i in nodes[parent[i]].children, "unlinked parent")


def build_oracle(hubs: tuple[Hub, ...]) -> Forest:
    """SEQUENTIAL TEST ORACLE: sorted lots, union-find, one node per multifusion."""
    births, edges, _ = G.reduce_graph(hubs)
    nodes = [Node(level, (), (name,)) for name, level in sorted(births.items())]
    leaf_ids = tuple((node.leaves[0], i) for i, node in enumerate(nodes))
    ids = dict(leaf_ids)
    uf = {name: name for name in births}
    component = {name: ids[name] for name in births}
    parent = list(range(len(nodes)))

    def root(name: str) -> str:
        while uf[name] != name:
            name = uf[name]
        return name

    for level in sorted({date for _, _, date in edges}):
        lot = [(u, v) for u, v, date in edges if date == level]
        old = {root(v): component[root(v)] for pair in lot for v in pair}
        for u, v in lot:
            a, b = root(u), root(v)
            if a != b:
                uf[b] = a
        groups: dict[str, set[int]] = {}
        for old_root, node_id in old.items():
            groups.setdefault(root(old_root), set()).add(node_id)
        for representative, children_set in sorted(groups.items()):
            children = tuple(sorted(children_set))
            if len(children) == 1:
                component[representative] = children[0]
                continue
            leaves = tuple(sorted(x for c in children for x in nodes[c].leaves))
            node_id = len(nodes)
            nodes.append(Node(level, children, leaves))
            parent.append(node_id)
            for child in children:
                parent[child] = node_id
            component[representative] = node_id
    immutable_nodes, immutable_parent = tuple(nodes), tuple(parent)
    validate_forest(immutable_nodes, immutable_parent)
    # Every round reads only a complete old buffer and creates a new immutable
    # buffer: no worker consumes a partly updated same-round pointer.
    jumps = [immutable_parent]
    for _ in range(max(0, len(nodes).bit_length() - 1)):
        previous = jumps[-1]
        following = tuple(previous[previous[i]] for i in range(len(nodes)))
        jumps.append(following)
    return Forest(immutable_nodes, immutable_parent, tuple(jumps), leaf_ids)


def query(forest: Forest, leaf: str, cut: F, closed: bool) -> int | None:
    """One independent occurrence query; all shared forest storage is immutable."""
    node_id = leaf_id(forest, leaf)
    if not active(forest.nodes[node_id].level, cut, closed):
        return None
    for jump in reversed(forest.jumps):
        ancestor = jump[node_id]
        if active(forest.nodes[ancestor].level, cut, closed):
            node_id = ancestor
    return node_id


def tree_signature(hubs: tuple[Hub, ...], forest: Forest,
                   cut: F, closed: bool, lose_contribution_date: bool = False):
    _, _, reps = G.reduce_graph(hubs)
    coverage: dict[tuple[str, ...], set[int]] = {}
    anchors = {}
    for leaf, _ in forest.leaf_ids:
        node_id = query(forest, leaf, cut, closed)
        if node_id is not None:
            coverage.setdefault(forest.nodes[node_id].leaves, set())
    for hub in hubs:
        admitted = active(hub.level, cut, closed)
        if lose_contribution_date:
            admitted = query(forest, reps[hub.name], cut, closed) is not None
        if not admitted:
            continue
        node_id = query(forest, reps[hub.name], cut, closed)
        require(node_id is not None, "active occurrence has no active representative")
        label = forest.nodes[node_id].leaves
        anchors[hub.name] = label
        coverage[label].update(hub.points)
    return tuple(sorted((label, tuple(sorted(points))) for label, points in coverage.items())), anchors


def cuts_for(hubs: tuple[Hub, ...]) -> list[F]:
    levels = sorted({h.level for h in hubs})
    return sorted(set(levels + [levels[0] - 1, levels[-1] + 1]
                      + [(a + b) / 2 for a, b in zip(levels, levels[1:])]))


def check_case(hubs: tuple[Hub, ...]) -> tuple[Forest, dict[str, int]]:
    forest = build_oracle(hubs)
    layout = heavy_chains(forest)
    raw_births, raw_edges = G.graph(hubs)
    births, edges, _ = G.reduce_graph(hubs)
    counts = {"cut_checks": 0, "leaf_queries": 0, "inactive_leaf_queries": 0,
              "dated_occurrence_checks": 0, "jump_entries": sum(map(len, forest.jumps)),
              "heavy_layout_integer_entries": 3 * len(forest.nodes),
              "heavy_queries": 0, "heavy_light_steps": 0, "heavy_binary_steps": 0}
    for cut in cuts_for(hubs):
        for closed in (False, True):
            bfs = G.partition(births, edges, cut, closed)
            for leaf, _ in forest.leaf_ids:
                node_id = query(forest, leaf, cut, closed)
                observed = None if node_id is None else frozenset(forest.nodes[node_id].leaves)
                require(observed == bfs.get(leaf), "ancestor query differs from independent BFS")
                chain_answer, light_steps, binary_steps = chain_query(forest, layout, leaf, cut, closed)
                require(chain_answer == node_id, "heavy-chain query differs from table and BFS")
                require(light_steps <= len(forest.nodes).bit_length(), "too many light edges")
                require(binary_steps <= len(forest.nodes).bit_length(), "too many binary steps")
                counts["heavy_queries"] += 1
                counts["heavy_light_steps"] += light_steps
                counts["heavy_binary_steps"] += binary_steps
                counts["leaf_queries"] += 1
                counts["inactive_leaf_queries"] += node_id is None
            expected = G.signatures(hubs, raw_births, raw_edges,
                                    {h.name: h.name for h in hubs}, cut, closed)
            require(tree_signature(hubs, forest, cut, closed) == expected,
                    "dated payload or hub admission differs from raw graph BFS")
            counts["cut_checks"] += 1
            counts["dated_occurrence_checks"] += len(hubs)
    return forest, counts


def check_verticals(tower: tuple[tuple[Hub, ...], ...], forests: tuple[Forest, ...]) -> dict[str, int]:
    """All horizontal forests already exist; requests do not await other verticals."""
    counts = {"vertical_nodes": 0, "vertical_leaf_witnesses": 0,
              "vertical_nonfinal_images": 0, "vertical_equal_level_merges": 0}
    for lower, upper, lower_tree, upper_tree in zip(tower, tower[1:], forests, forests[1:]):
        lower_layout = heavy_chains(lower_tree)
        raw_births, raw_edges = G.graph(lower)
        _, _, lower_reps = G.reduce_graph(lower)
        lower_minima = {h.name for h in lower if not h.terminals}
        for node in upper_tree.nodes:
            bfs = G.partition(raw_births, raw_edges, node.level, True)
            # Every descendant witness must yield the same image; only one
            # chosen descendant is needed by the actual occurrence query.
            expected = {tuple(sorted(bfs[leaf] & lower_minima)) for leaf in node.leaves}
            require(len(expected) == 1, "upper component lacks a well-defined lower image")
            for leaf in node.leaves:
                answer = query(lower_tree, lower_reps[leaf], node.level, True)
                require(answer is not None and lower_tree.nodes[answer].leaves in expected,
                        "single descendant gives wrong vertical image")
                require(chain_query(lower_tree, lower_layout, lower_reps[leaf], node.level, True)[0]
                        == answer, "heavy-chain vertical image differs")
                counts["vertical_leaf_witnesses"] += 1
            first = node.leaves[0]
            answer = query(lower_tree, lower_reps[first], node.level, True)
            require(answer is not None, "missing vertical image")
            counts["vertical_nonfinal_images"] += lower_tree.parent[answer] != answer
            counts["vertical_equal_level_merges"] += bool(lower_tree.nodes[answer].children) and lower_tree.nodes[answer].level == node.level
            counts["vertical_nodes"] += 1
    return counts


def run() -> dict[str, object]:
    line3 = (
        (Hub("A", F(0), points=(0,)), Hub("B", F(0), points=(1,)),
         Hub("C", F(0), points=(2,)), Hub("AB", F(1), ("A", "B")),
         Hub("BC", F(1), ("B", "C"))),
        (Hub("AB", F(1), points=(0, 1)), Hub("BC", F(1), points=(1, 2)),
         Hub("ABC", F(4), ("AB", "BC"))),
        (Hub("ABC", F(4), points=(0, 1, 2)),),
    )
    # Collinear coordinates 0, 2, 10, 12: squared interval radii are
    # 1 (AB, CD), 16 (BC), 25 (ABC, BCD), 36 (ABCD).
    line4 = (
        (Hub("A", F(0), points=(0,)), Hub("B", F(0), points=(1,)),
         Hub("C", F(0), points=(2,)), Hub("D", F(0), points=(3,)),
         Hub("AB", F(1), ("A", "B")), Hub("CD", F(1), ("C", "D")),
         Hub("BC", F(16), ("B", "C"))),
        (Hub("AB", F(1), points=(0, 1)), Hub("CD", F(1), points=(2, 3)),
         Hub("BC", F(16), points=(1, 2)),
         Hub("ABC", F(25), ("AB", "BC")), Hub("BCD", F(25), ("BC", "CD"))),
        (Hub("ABC", F(25), points=(0, 1, 2)), Hub("BCD", F(25), points=(1, 2, 3)),
         Hub("ABCD", F(36), ("ABC", "BCD"))),
        (Hub("ABCD", F(36), points=(0, 1, 2, 3)),),
    )
    growth = (Hub("ABC", F(16), points=(0, 1, 2)),
              Hub("ABCZ", F(25), ("ABC",), (0, 1, 2, 3)))
    overlap = (Hub("a", F(0), points=(0, 1)), Hub("b", F(0), points=(1, 2)))
    singleton = (Hub("only_point", F(0), points=(0,)),)
    # A deterministic comb exercises a depth far exceeding one jump round.
    deep = [Hub(f"L{i:02}", F(0)) for i in range(35)]
    previous = "L00"
    for i in range(1, 35):
        name = f"M{i:02}"
        deep.append(Hub(name, F(i), (previous, f"L{i:02}")))
        previous = name
    cases = (*line3, *line4, growth, overlap, singleton, tuple(deep))
    forests = []
    counts: dict[str, int] = {}
    for hubs in cases:
        forest, case_counts = check_case(hubs)
        forests.append(forest)
        for key, value in case_counts.items():
            counts[key] = counts.get(key, 0) + value
    vertical = check_verticals(line3, tuple(forests[:3]))
    for key, value in check_verticals(line4, tuple(forests[3:7])).items():
        vertical[key] += value
    require(vertical["vertical_nonfinal_images"] >= 2, "vertical fixture only sees final roots")
    require(vertical["vertical_equal_level_merges"] >= 2, "vertical equality fixture vacuous")
    require(counts["inactive_leaf_queries"] > 0, "birth/future fixture vacuous")
    require(counts["heavy_light_steps"] > 0 and counts["heavy_binary_steps"] > 0,
            "heavy-chain traversal or final binary search is vacuous")
    deep_tree = forests[-1]
    at_end = query(deep_tree, "L00", F(34), True)
    require(at_end is not None and len(deep_tree.nodes[at_end].leaves) == 35,
            "deep ancestor query stopped early")
    require(query(deep_tree, "L00", F(34), False) != at_end, "deep open cut vacuous")

    mutants = []
    first = forests[0]
    require(query(first, "A", F(1), False) != query(first, "A", F(1), True),
            "closed-for-open mutant survived")
    mutants.append("replace_open_cut_with_closed")
    branch = forests[3]
    proper = query(branch, "A", F(1), True)
    root = query(branch, "A", F(100), True)
    require(proper != root, "final-root mutant survived")
    mutants.append("replace_dated_image_with_final_root")
    require(tree_signature(growth, forests[7], F(25), False, True)
            != tree_signature(growth, forests[7], F(25), False),
            "undated-contribution mutant survived")
    mutants.append("lose_unary_contribution_date")
    # This binary refinement has correct cut partitions, but false public
    # parents: the simultaneous A/B/C event must have three pre-lot parents.
    binary = (Node(F(0), (), ("A",)), Node(F(0), (), ("B",)), Node(F(0), (), ("C",)),
              Node(F(1), (0, 1), ("A", "B")), Node(F(1), (3, 2), ("A", "B", "C")))
    try:
        validate_forest(binary, (3, 3, 4, 4, 4))
    except ValueError as error:
        require(str(error) == "uncontracted simultaneous fusion", "wrong binary-mutant cause")
    else:
        raise ValueError("binary equality mutant survived")
    require(len(first.nodes[-1].children) == 3, "positive simultaneous multifusion missing")
    mutants.append("leave_equal_date_binary_nodes_uncontracted")
    require(query(branch, "C", F(1), True) != proper,
            "wrong vertical descendant mutant survived")
    mutants.append("use_lower_leaf_outside_upper_descendants")
    require(len(tree_signature(overlap, forests[8], F(0), True)[0]) == 2,
            "overlapping point payloads coalesced component identities")
    require(len(forests[9].nodes) == 1 and forests[9].parent == (0,), "n=1 failed")
    require(len(forests[2].nodes) == len(forests[6].nodes) == 1, "terminal K=n failed")
    return {"status": "passed_parallel_object_model", "cases": len(cases), **counts, **vertical,
            "maximum_tested_depth": 34, "maximum_jump_rounds": len(deep_tree.jumps),
            "simultaneous_three_parent_fusion": True, "overlap_keeps_two_components": True,
            "mutants_rejected": mutants, "constructor": "sequential_sorted_dsu_test_oracle",
            "jump_construction": "synchronous_double_buffer_model",
            "heavy_chain_construction": "sequential_model_not_parallel_product",
            "heavy_layout": "linear_logical_integer_storage_not_measured_bytes",
            "verticals_consume": "completed_horizontal_forests_only",
            "geometry_source": "bounded_collinear_fixture_dates_not_product_census",
            "parallel_threads_executed": False, "product_executed": False,
            "performance_claim": False, "public_status": "not_claimed", "gcp_used": False}


if __name__ == "__main__":
    print(json.dumps(run(), sort_keys=True, separators=(",", ":")))
