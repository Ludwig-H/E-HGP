"""Audit consumer of MHGP11PTS1; exact closed-plateau projections, no EOM.

The native export is an explicit prerequisite: complete strong-ball populations
and their *closed* FULL owner, from one Cloud/coordinate system. This module does
not rebuild FULL, infer missing incidences, or claim a statistical selection rule.
"""
from array import array
from collections import Counter
from dataclasses import dataclass
from fractions import Fraction
import heapq
import mmap
from pathlib import Path
import struct
import sys

MAGIC = b"MHGP11PTS1"
NONE = (1 << 32) - 1
WORD = struct.Struct("<Q")


def need(condition, message):
    if not condition:
        raise ValueError(message)


def date_json(value):
    if value is None:
        return None
    if isinstance(value, Fraction):
        return str(value.numerator) + "/" + str(value.denominator)
    return value


@dataclass
class Order:
    k: int
    births: int
    root: int
    parent: array
    rank: array
    begin: array
    cardinal: array
    edges: array
    core_node: array
    core_date: array
    first_node: array
    first_rank: array
    record_offsets: array
    data: object

    @property
    def count(self):
        return len(self.parent)

    def children(self, node):
        begin = self.begin[node]
        for pos in range(begin, begin + self.cardinal[node]):
            yield self.edges[pos]

    def records(self):
        for offset in self.record_offsets:
            node, rank, cardinal = struct.unpack_from("<QQQ", self.data, offset)
            start = offset + 24
            yield node, rank, (WORD.unpack_from(self.data, start + 8*i)[0]
                               for i in range(cardinal))

    def node_events(self, levels):
        # Births have canonical centre order; fusion nodes have level order.
        births = sorted(range(self.births), key=lambda i: (self.rank[i], i))
        yield from heapq.merge(((levels[self.rank[i]], i) for i in births),
                               ((levels[self.rank[i]], i)
                                for i in range(self.births, self.count)))


@dataclass
class Dump:
    bits: int
    kmax: int
    points: array  # XYZ, original PointId, interleaved u64
    levels: list
    orders: dict
    backing: object

    @property
    def sites(self):
        return len(self.points) // 4

    def close(self):
        if isinstance(self.backing, mmap.mmap):
            self.backing.close()


def load(path):
    """Read and validate the native audit format (a path or an in-memory bytes).

    Forest arrays are compact; record populations remain in the read-only
    backing mapping. Label arrays supplied to analyse() are already in SiteIdx
    order; original PointId is retained here for the runner's row mapping.
    """
    if isinstance(path, (bytes, bytearray)):
        data = bytes(path)
    else:
        with Path(path).open("rb") as stream:
            data = mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ)
    cursor, size = 10, len(data)

    def word():
        nonlocal cursor
        need(cursor + 8 <= size, "truncated word")
        result = WORD.unpack_from(data, cursor)[0]
        cursor += 8
        return result

    def exact():
        negative, limbs = word(), word()
        need(negative in (0, 1) and 1 <= limbs <= 64, "integer header")
        need(cursor + 8*limbs <= size, "truncated integer")
        value = sum(word() << (64*i) for i in range(limbs))
        need(not negative or value > 0, "negative zero")
        return -value if negative else value

    def words(count, narrow=False):
        nonlocal cursor
        need(0 <= count <= (size-cursor)//8, "array exceeds payload")
        result = array("Q")
        result.frombytes(data[cursor:cursor+8*count])
        if sys.byteorder != "little":
            result.byteswap()
        cursor += 8*count
        if narrow:
            need(all(x <= NONE for x in result), "u32 word exceeds domain")
            return array("I", result)
        return result

    try:
        need(size >= 42 and data[:10] == MAGIC, "points signature/size")
        bits, kmax, sites, level_count = word(), word(), word(), word()
        need(bits in (18, 21, 24) and 1 <= kmax <= min(12, sites) and
             0 < sites < NONE and 0 < level_count < NONE, "points header")
        points = words(4*sites)
        need(all(points[4*i+j] < 1 << bits for i in range(sites)
                 for j in range(3)), "point coordinate domain")
        need(len(set(points[3::4])) == sites, "duplicate original PointId")
        levels = []
        for _ in range(level_count):
            numerator, denominator = exact(), exact()
            need(numerator >= 0 and denominator > 0, "level signs")
            value = Fraction(numerator, denominator)
            need(not levels or levels[-1] < value, "levels not strictly sorted")
            levels.append(value)
        need(levels[0] == 0, "missing zero level")
        orders = {}
        for expected in range(1, kmax+1):
            k, births, count, edge_count, root = (word() for _ in range(5))
            need(k == expected and 0 < births <= count < NONE and
                 edge_count == count-1 and root < count, "order header")
            raw = words(4*count, True)
            parent, rank, begin, cardinal = (raw[i::4] for i in range(4))
            edges = words(edge_count, True)
            need(all(r < level_count for r in rank), "node level rank")
            need(all(rank[i] <= rank[i+1] for i in range(births, count-1)),
                 "fusion levels not sorted")
            need(parent[root] == NONE and sum(p == NONE for p in parent) == 1,
                 "forest root")
            seen = bytearray(count)
            for node in range(count):
                need(begin[node]+cardinal[node] <= edge_count,
                     "child range exceeds forest")
                need((cardinal[node] == 0) == (node < births), "birth/merge shape")
                need(node < births or cardinal[node] >= 2, "unary FULL merge")
                p = parent[node]
                need(p == NONE or node < p < count and rank[node] <= rank[p],
                     "parent topology/date")
                for pos in range(begin[node], begin[node]+cardinal[node]):
                    child = edges[pos]
                    need(child < node and parent[child] == node and not seen[child],
                         "child reciprocity/duplicate")
                    seen[child] = 1
            need(all(seen[i] or i == root for i in range(count)), "missing child")
            raw = words(2*sites)
            need(all(x < count for x in raw[::2]), "core node domain")
            core_node, core_date = array("I", raw[::2]), raw[1::2]
            raw = words(2*sites, True)
            first_node, first_rank = raw[::2], raw[1::2]
            need(all(x < count for x in first_node) and
                 all(x < level_count for x in first_rank), "first-cover domain")
            for i in range(sites):
                for node, date in ((core_node[i], Fraction(core_date[i])),
                                   (first_node[i], levels[first_rank[i]])):
                    p = parent[node]
                    need(levels[rank[node]] <= date and
                         (p == NONE or date < levels[rank[p]]),
                         "anchor is not a closed living node")
            records, offsets = word(), array("Q")
            need(records <= (size-cursor)//24, "strong count exceeds payload")
            record_ranks = array("I")
            earliest = array("I", [NONE])*sites
            first_matches = bytearray(sites)
            for _ in range(records):
                offsets.append(cursor)
                node, event_rank, cardinality = word(), word(), word()
                need(node < count and event_rank < level_count and
                     0 < cardinality <= sites, "strong record domain")
                p = parent[node]
                need(rank[node] <= event_rank and
                     (p == NONE or event_rank < rank[p]),
                     "strong owner is not closed/living")
                population = words(cardinality, True)
                need(all(i < sites for i in population) and
                     len(set(population)) == cardinality, "strong population")
                for i in population:
                    if event_rank < earliest[i]:
                        earliest[i] = event_rank
                        first_matches[i] = int(first_node[i] == node)
                    elif event_rank == earliest[i] and first_node[i] == node:
                        first_matches[i] = 1
                record_ranks.append(event_rank)
            need(earliest == first_rank and all(first_matches),
                 "first-cover not an earliest strong witness")
            if any(record_ranks[i] > record_ranks[i+1] for i in range(records-1)):
                offsets = array("Q", sorted(offsets,
                    key=lambda off: WORD.unpack_from(data, off+8)[0]))
            orders[k] = Order(k, births, root, parent, rank, begin, cardinal,
                              edges, core_node, core_date, first_node, first_rank,
                              offsets, data)
        need(cursor == size, "trailing payload")
        return Dump(bits, kmax, points, levels, orders, data)
    except Exception:
        if isinstance(data, mmap.mmap):
            data.close()
        raise


class HierarchyScorer:
    """Shared point DSU and IoU judge; observe() is ONE closed plateau.

    Targets are nonnegative labels. Background -1 contributes to valid_size;
    void -2 contributes only to total_size and geometric transmission thresholds.
    activate precedes union; all unions of a date precede its single observe.
    Leaves are SiteIdx 0..n-1; internal nodes are atomic multifusions. No labels
    enter the projection: they are used only for post-hoc diagnostic IoU.
    """
    def __init__(self, labels):
        need(all(type(x) is int and x >= -2 for x in labels), "label domain")
        self.labels = list(labels)
        n = len(labels)
        need(0 < n < NONE, "scorer size")
        self.parent = array("I", range(n))
        self.size = array("I", [1])*n
        self.valid = array("I", (int(x != -2) for x in labels))
        self.minimum = array("I", range(n))
        self.active = bytearray(n)
        self.overlap = [{x: 1} if x >= 0 else {} for x in labels]
        self.target_sizes = dict(Counter(x for x in labels if x >= 0))
        self.best = {x: None for x in self.target_sizes}
        self.top = array("I", range(n))
        self.parts = {}
        self.touched = set()
        self.entry = [None]*n
        self.tree_height = [None]*n
        self.tree_parent = array("I", [NONE])*n
        self.tree_begin = array("I", [0])*n
        self.tree_count = array("I", [0])*n
        self.tree_edges = array("I")
        self.last_height = None
        self.plateaus = self.unions = self.activations = 0

    def find(self, node):
        parent = self.parent
        root = node
        while parent[root] != root:
            root = parent[root]
        while parent[node] != node:
            nxt = parent[node]
            parent[node] = root
            node = nxt
        return root

    def activate(self, site, height):
        need(0 <= site < len(self.labels), "point activation index")
        if not self.active[site]:
            self.active[site] = 1
            self.entry[site] = self.tree_height[site] = height
            self.touched.add(self.find(site))
            self.activations += 1

    def union(self, a, b):
        need(self.active[a] and self.active[b], "union of inactive point")
        a, b = self.find(a), self.find(b)
        if a == b:
            return a
        if (self.size[a], -self.minimum[a]) < (self.size[b], -self.minimum[b]):
            a, b = b, a
        left = self.parts.pop(a, None)
        right = self.parts.pop(b, None)
        if left is None:
            left = [self.top[a]]
        if right is None:
            right = [self.top[b]]
        left.extend(right)
        self.parts[a] = left
        self.parent[b] = a
        self.size[a] += self.size[b]
        self.valid[a] += self.valid[b]
        self.minimum[a] = min(self.minimum[a], self.minimum[b])
        if len(self.overlap[a]) < len(self.overlap[b]):
            self.overlap[a], self.overlap[b] = self.overlap[b], self.overlap[a]
        for target, value in self.overlap[b].items():
            self.overlap[a][target] = self.overlap[a].get(target, 0) + value
        self.overlap[b] = {}
        self.touched.add(a)
        self.unions += 1
        return a

    def observe(self, height):
        need(self.last_height is None or self.last_height < height,
             "observe must be once per strictly increasing closed plateau")
        roots = {self.find(i) for i in self.touched}
        for root in sorted(roots, key=lambda i: self.minimum[i]):
            parts = self.parts.pop(root, None)
            if parts is not None:
                node = len(self.tree_height)
                self.tree_height.append(height)
                self.tree_parent.append(NONE)
                self.tree_begin.append(len(self.tree_edges))
                self.tree_count.append(len(parts))
                for child in sorted(parts):
                    need(self.tree_parent[child] == NONE, "point child reused")
                    self.tree_parent[child] = node
                    self.tree_edges.append(child)
                self.top[root] = node
            for target, intersection in self.overlap[root].items():
                denominator = self.target_sizes[target] + self.valid[root] - intersection
                old = self.best[target]
                if old is None or intersection*old["denominator"] > old["intersection"]*denominator:
                    self.best[target] = dict(intersection=intersection,
                        denominator=denominator, valid_size=self.valid[root],
                        total_size=self.size[root], tree_node=self.top[root],
                        height=height, witness_site=self.minimum[root])
        need(not self.parts, "unpublished point merges")
        self.touched.clear()
        self.last_height = height
        self.plateaus += 1

    def finish(self):
        need(not self.touched and not self.parts, "finish before closed observation")
        best = {}
        for target in sorted(self.best):
            value = self.best[target]
            if value is None:
                best[str(target)] = dict(iou=0.0, iou_exact="0/1", target_size=self.target_sizes[target],
                                         tree_node=None, height=None)
            else:
                entry = dict(value)
                ratio = Fraction(entry["intersection"], entry.pop("denominator"))
                entry.update(iou=float(ratio), iou_exact=date_json(ratio),
                             target_size=self.target_sizes[target], height=date_json(entry["height"]))
                best[str(target)] = entry
        tree = dict(leaves=len(self.labels), height=[date_json(x) for x in self.tree_height],
                    parent=list(self.tree_parent), child_begin=list(self.tree_begin),
                    child_count=list(self.tree_count), children=list(self.tree_edges),
                    roots=sorted(self.top[i] for i in range(len(self.labels))
                                 if self.parent[i] == i),
                    active_roots=sorted(self.top[i] for i in range(len(self.labels))
                                        if self.parent[i] == i and self.active[i]))
        pairs = {}
        selected = [(target, value["tree_node"]) for target, value in best.items()
                    if value["tree_node"] is not None]
        for pos, (target, node) in enumerate(selected):
            for other, second in selected[pos+1:]:
                ancestor = tree_lca(tree, node, second)
                pairs[target + ":" + other] = None if ancestor is None else tree["height"][ancestor]
        return dict(best_iou=best, selected_group_fusion_heights=pairs,
                    entry_dates=[date_json(x) for x in self.entry], tree=tree,
                    partition_completion="inactive_singletons",
                    metric_policy="active_components_only; void -2 excluded from IoU",
                    inactive_sites=len(self.labels)-self.activations,
                    work=dict(plateaus=self.plateaus, unions=self.unions,
                              activated=self.activations, tree_nodes=len(self.tree_height)))


def tree_lca(tree, a, b):
    ancestors = set()
    while a != NONE:
        ancestors.add(a)
        a = tree["parent"][a]
    while b != NONE:
        if b in ancestors:
            return b
        b = tree["parent"][b]
    return None


def members(tree, node):
    """Recover the immutable SiteIdx witness of a compact point-tree node."""
    stack, output = [node], []
    while stack:
        item = stack.pop()
        if item < tree["leaves"]:
            output.append(item)
        else:
            begin, count = tree["child_begin"][item], tree["child_count"][item]
            stack.extend(tree["children"][begin:begin+count])
    return sorted(output)


def _lca(order, a, b, depths):
    while depths[a] > depths[b]:
        a = order.parent[a]
    while depths[b] > depths[a]:
        b = order.parent[b]
    while a != b:
        a, b = order.parent[a], order.parent[b]
    return a


def _first_lca(order, levels, sites):
    # Linear storage; no O(V log V) lifting table. Pairwise ties may climb deep
    # trees, an explicit audit cost rather than a promised massive algorithm.
    depths = array("I", [0])*order.count
    for node in range(order.count-1, -1, -1):
        p = order.parent[node]
        if p != NONE:
            depths[node] = depths[p] + 1
    owners = array("I", [NONE])*sites
    for node, rank, population in order.records():
        for site in population:
            if rank == order.first_rank[site]:
                owners[site] = node if owners[site] == NONE else _lca(order, owners[site], node, depths)
    need(all(node != NONE for node in owners), "missing first-cover owner")
    return sorted((max(levels[order.first_rank[i]], levels[order.rank[node]]), node, i)
                  for i, node in enumerate(owners))


def _sweep(order, levels, labels, threshold, events):
    scorer = HierarchyScorer(labels)
    payload = {}  # FULL node -> a distinct set of <m sites, or a qualified anchor.
    nodes = iter(order.node_events(levels))
    records = iter(events)
    nxt_node, nxt_record = next(nodes, None), next(records, None)
    work = dict(full_nodes=0, records=0, incidences=0, maximum_pending=0)

    def add(current, site, height):
        if isinstance(current, int):
            scorer.activate(site, height)
            scorer.union(current, site)
            return current
        if current is None:
            current = set()
        current.add(site)
        if len(current) < threshold:
            work["maximum_pending"] = max(work["maximum_pending"], len(current))
            return current
        anchor = min(current)
        for i in current:
            scorer.activate(i, height)
        for i in current:
            scorer.union(anchor, i)
        return anchor

    def combine(left, right, height):
        if right is None:
            return left
        if left is None:
            return right
        if isinstance(right, int):
            if isinstance(left, int):
                scorer.union(left, right)
                return left
            left, right = right, left
        if isinstance(right, set):
            for site in right:
                left = add(left, site, height)
            return left
        raise ValueError("invalid FULL payload")

    while nxt_node is not None or nxt_record is not None:
        height = min(item[0] for item in (nxt_node, nxt_record) if item is not None)
        while nxt_node is not None and nxt_node[0] == height:
            _, node = nxt_node
            current = None
            for child in order.children(node):
                current = combine(current, payload.pop(child, None), height)
            if current is not None:
                payload[node] = current
            work["full_nodes"] += 1
            nxt_node = next(nodes, None)
        while nxt_record is not None and nxt_record[0] == height:
            _, node, population = nxt_record
            current = payload.get(node)
            for site in population:
                current = add(current, site, height)
                work["incidences"] += 1
            if current is not None:
                payload[node] = current
            work["records"] += 1
            nxt_record = next(records, None)
        scorer.observe(height)
    result = scorer.finish()
    result["coverage_work"] = work
    return result


def analyse(data, labels, orders=None, thresholds=None):
    """Analyse qualified covers and explicitly named fixed-anchor baselines.

    Heights are squared radii, rendered as exact n/d strings. Default thresholds
    are distinct values {3,k+1,20}; they are transmission sizes, never mcs.
    Core/first-cover entry dates are retained separately from point merge dates.
    """
    need(len(labels) == data.sites, "labels must be in SiteIdx order")
    chosen = [k for k in (2, 3, 5, 10) if k <= data.kmax] if orders is None else list(orders)
    need(all(k in data.orders for k in chosen), "requested order absent")
    result = dict(schema="ehgp.audit.qualified_cover.v1", bits=data.bits, kmax=data.kmax,
                  sites=data.sites, height_units="squared_grid_radius", void_label=-2,
                  background_label=-1, orders={})
    for k in chosen:
        order = data.orders[k]
        values = sorted(set((3, k+1, 20) if thresholds is None else thresholds))
        need(all(type(m) is int and m >= 1 for m in values), "threshold domain")
        answer = dict(qualified={})
        for m in values:
            events = ((data.levels[rank], node, population)
                      for node, rank, population in order.records())
            answer["qualified"][str(m)] = _sweep(order, data.levels, labels, m, events)
        core = sorted((Fraction(order.core_date[i]), order.core_node[i], i)
                      for i in range(data.sites))
        first = sorted((data.levels[order.first_rank[i]], order.first_node[i], i)
                       for i in range(data.sites))
        lca = _first_lca(order, data.levels, data.sites)
        for name, entries in (("core", core), ("first_cover_canonical", first),
                              ("first_cover_lca_all_ties", lca)):
            answer[name] = _sweep(order, data.levels, labels, 1,
                                 ((height, node, (site,)) for height, node, site in entries))
        result["orders"][str(k)] = answer
    return result
