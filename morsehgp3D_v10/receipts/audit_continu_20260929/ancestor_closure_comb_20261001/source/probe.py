#!/usr/bin/env python3
"""Exact tiny ancestor-closure experiment. Sources are pinned before AST execution."""
import ast
import hashlib
import json
import pathlib
import sys
from fractions import Fraction
from itertools import combinations
from types import SimpleNamespace

sys.dont_write_bytecode = True
BASE = pathlib.Path(__file__).resolve().parent
PINS = {
    "original/fullk.py": "a09a6c85687971f80e0df030effaf4cc112b9641176badf8460296898625a44e",
    "original/frontier_core.py": "86ba984ff7986bdd44a5d53e9b2c7d3f2c0eb847764938cd28259d18439850b7",
}
CASES = [(8, 5, True), (16, 5, True), (32, 5, False), (16, 10, True), (32, 10, False)]


def require(cond, message):
    if not cond:
        raise RuntimeError(message)


def encoded(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()


def sha(data):
    return hashlib.sha256(data).hexdigest()


def q(value):
    value = Fraction(value)
    return str(value.numerator) + "/" + str(value.denominator)


def snapshot_namespace():
    raw = {}
    for name, pin in PINS.items():
        data = (BASE / name).read_bytes()
        require(sha(data) == pin, "source pin: " + name)
        raw[name] = data
    ns = {"Fraction": Fraction, "combinations": combinations, "NONE": (1 << 32) - 1}
    names = {"MathError", "require", "Tree", "gamma_tree", "check_cover", "native_tree"}
    tree = ast.parse(raw["original/fullk.py"])
    selected = [x for x in tree.body if isinstance(x, (ast.ClassDef, ast.FunctionDef)) and x.name in names]
    require(len(selected) == len(names), "fullk extraction inventory")
    exec(compile(ast.Module(body=selected, type_ignores=[]), "original/fullk.py", "exec"), ns)
    tree = ast.parse(raw["original/frontier_core.py"])
    selected = [x for x in tree.body if isinstance(x, ast.FunctionDef) and x.name == "witness_universe"]
    require(len(selected) == 1, "frontier extraction inventory")
    exec(compile(ast.Module(body=selected, type_ignores=[]), "original/frontier_core.py", "exec"), ns)

    class DSU:
        def __init__(self):
            self.parent = {}

        def find(self, x):
            self.parent.setdefault(x, x)
            root = x
            while self.parent[root] != root:
                root = self.parent[root]
            while self.parent[x] != x:
                nxt = self.parent[x]
                self.parent[x] = root
                x = nxt
            return root

        def union(self, x, y):
            x, y = self.find(x), self.find(y)
            if x != y:
                self.parent[y] = x

    # Exact MEB specialization for collinear points. gamma_tree uses element 0 only.
    def meb_1d(points, ids):
        xs = [points[i][0] for i in ids]
        return (Fraction((max(xs) - min(xs)) ** 2, 4),)

    ns["REF"] = SimpleNamespace(DSU=DSU, meb=meb_1d)
    ns["FC"] = lambda: SimpleNamespace(NONE=ns["NONE"], witness_universe=ns["witness_universe"])
    return ns, raw


def analytic_export(n, K, none):
    """Adapter, NOT the native exporter: K-window leaves and increasing K+1-window merges."""
    xs = [j * (j + 1) // 2 for j in range(n)]
    m = n - K + 1
    birth = [Fraction((xs[i + K - 1] - xs[i]) ** 2, 4) for i in range(m)]
    parent, children = [none] * m, [[] for _ in range(m)]
    prefix = 0
    for i in range(m - 1):
        v = len(birth)
        birth.append(Fraction((xs[i + K] - xs[i]) ** 2, 4))
        parent.append(none)
        children.append([prefix, i + 1])
        parent[prefix] = parent[i + 1] = v
        prefix = v

    class Forest:
        def __len__(self):
            return len(birth)

        def level(self, v):
            return birth[v]

        def ancestor(self, v, t, closed):
            require(closed and birth[v] <= t, "analytic ancestor contract")
            while parent[v] != none and birth[parent[v]] <= t:
                v = parent[v]
            return v

    forest = Forest()
    forest.parent, forest.children = parent, children
    balls, ball_node = [], []
    # The catalogue contains populations 2..K+1. Strong coverage selects population K only.
    for pop in range(2, K + 2):
        for i in range(n - pop + 1):
            lvl = Fraction((xs[i + pop - 1] - xs[i]) ** 2, 4)
            ids = tuple(range(i, i + pop))
            ball = SimpleNamespace(index=len(balls), level=lvl, q=2, p=pop - 2,
                                   population=pop, members=lambda ids=ids: ids)
            balls.append(ball)
            ball_node.append(i if pop == K else m + i if pop == K + 1 else none)
    order = SimpleNamespace(k=K, forest=forest, ball_node=ball_node)

    def get_order(k=None):
        require(k is None or k == K, "analytic export order")
        return order

    return SimpleNamespace(sites=[(x, 0, 0) for x in xs], balls=balls, order=get_order)


def digests(T, cover, sites):
    """Node-ID-independent forest and exact coverage signatures; no full relation is published."""
    node = {}
    for v in T.bottomup:
        node[v] = sha(encoded([q(T.birth[v]), sorted(node[c] for c in T.children[v])]))
    forest = sha(encoded(sorted([node[v], q(T.birth[v]),
                                 None if T.death[v] is None else q(T.death[v]),
                                 sorted(node[c] for c in T.children[v])] for v in range(len(T)))))
    covering = sha(encoded([[list(sites[x]), sorted([node[v], q(c)] for v, c in cv.items())]
                            for x, cv in enumerate(cover)]))
    return forest, covering


def run(protocol_bytes):
    protocol = json.loads(protocol_bytes)
    require(protocol["source_pins"] == PINS, "protocol source pins")
    require([(c["n"], c["K"], c["gamma"]) for c in protocol["cases"]] == CASES, "protocol case inventory")
    ns, raw = snapshot_namespace()
    rows = []
    for n, K, exhaustive in CASES:
        export = analytic_export(n, K, ns["NONE"])
        T, cover, info = ns["native_tree"](export)
        H = 2 * (n - K + 1) - 1
        W = K * (n - K + 1)
        D = W + (n - K) * (n + K + 1) // 2
        require((len(T), info["witness_incidences"], sum(map(len, cover))) == (H, W, D),
                "native closure differs from formula")
        forest, covering = digests(T, cover, export.sites)
        row = dict(n=n, K=K, H=H, seeds=W, D=D, catalogue_balls=len(export.balls),
                   weak_inc=sum(map(len, ns["witness_universe"](export, strong=False))),
                   forest_sha=forest, cover_sha=covering, gamma=None)
        if exhaustive:
            Tg, cg, ig = ns["gamma_tree"](export.sites, K)
            gf, gc = digests(Tg, cg, export.sites)
            require((len(Tg), sum(map(len, cg)), gf, gc) == (H, D, forest, covering),
                    "exhaustive Gamma disagreement")
            row["gamma"] = dict(vertices=ig["vertices"], edges=ig["edges"], H=len(Tg),
                                D=sum(map(len, cg)), forest_sha=gf, cover_sha=gc)
        rows.append(row)
    for name, data in raw.items():
        require((BASE / name).read_bytes() == data, "source changed during replay: " + name)
    return dict(schema="ancestor_closure_1d_result_v2", protocol_sha=sha(protocol_bytes),
                sources=PINS, cases=rows)


def main(argv):
    require(len(argv) == 3 and argv[1] == "--protocol", "usage: probe.py --protocol protocol.json")
    require(pathlib.Path(argv[2]).resolve() == BASE / "protocol.json", "package protocol required")
    result = run((BASE / "protocol.json").read_bytes())
    exe = pathlib.Path(sys.executable).resolve(strict=True)
    result["execution"] = dict(path=str(exe), sha256=sha(exe.read_bytes()), version=sys.version,
                               version_info=list(sys.version_info), implementation=sys.implementation.name)
    sys.stdout.buffer.write(encoded(result))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
