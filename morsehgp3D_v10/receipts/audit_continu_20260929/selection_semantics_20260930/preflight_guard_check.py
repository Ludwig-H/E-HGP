"""Bounded exact audit; only selected AST definitions are executed."""
import argparse
import ast
import hashlib
import json
import math
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace as NS

PINS = {
    "selection.py": "509acaa1170c1bcf3a3cfb97eb6220db3a908b0b7669c75efad78183e84bc80e",
    "dev_scenes.py": "626b0dc9ca27c9aae4a5a184188e012d394ff05d52b4480f3729cebee0fad03c",
    "participation.py": "47acfd0c482455f9b14fbfaa9e90a9f5ff02627dd15453c3dc99a8651b696549",
    "scale.py": "7445aec285ce649b32e6b31c62dca96448f4f5b19b0b305b455d545462b2f294",
    "MEMO_MASSES_SELECTION_20260930.md": "3fd53faa94d2b1775a7e38f2832b3eb118ecf8b71b7e072263998e9111d0d001",
}
NAMES = {"NodeStats", "hard_stats", "condense", "eom", "depth_of", "unselect", "ancestor_selected"}

def require(ok, why):
    if not ok:
        raise ValueError(why)

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def field(n):
    return isinstance(n, ast.Attribute) and isinstance(n.value, ast.Name)

def inspect_sources(root):
    data = {}
    for name, pin in PINS.items():
        p = root / "snapshots" / name
        require(not p.is_symlink() and p.is_file(), "snapshot regular file: " + name)
        require(sha(p) == pin, "snapshot hash: " + name)
        data[name] = p.read_text()
    parsed = ast.parse(data["selection.py"])
    nodes = [n for n in parsed.body if isinstance(n, (ast.FunctionDef, ast.ClassDef)) and n.name in NAMES]
    require(len(nodes) == len(NAMES) and {n.name for n in nodes} == NAMES, "exact AST inventory")
    fn = {n.name: n for n in nodes}
    require(any(field(n) and n.value.id == "a" and n.attr == "phi_date" for n in ast.walk(fn["hard_stats"])),
            "hard_stats original date use")
    require(any(field(n) and n.value.id == "stats" and n.attr == "end_mass" for n in ast.walk(fn["condense"])),
            "condense final-mass use")
    require(any(field(n) and n.value.id == "stats" and n.attr == "stab" for n in ast.walk(fn["eom"])),
            "eom wholelife use")
    dev = ast.parse(data["dev_scenes.py"])
    blocks = [n for n in dev.body if isinstance(n, ast.FunctionDef) and n.name == "selection_block"]
    require(len(blocks) == 1, "selection_block unique")
    block = blocks[0]
    loops = [n for n in ast.walk(block) if isinstance(n, ast.For) and ast.dump(n.target, include_attributes=False) ==
             ast.dump(ast.parse("rname, (atts, _phi)", mode="eval").body, include_attributes=False)]
    require(len(loops) == 1, "real hard-method loop")
    loop = loops[0]
    require(isinstance(loop.iter, ast.Call) and field(loop.iter.func) and loop.iter.func.value.id == "hard"
            and loop.iter.func.attr == "items", "hard.items loop")
    calls = [n for n in ast.walk(loop) if isinstance(n, ast.Call) and field(n.func)
             and n.func.value.id == "SE" and n.func.attr == "hard_stats"]
    require(len(calls) == 1 and [ast.dump(a, include_attributes=False) for a in calls[0].args] ==
            [ast.dump(ast.Name(id=v, ctx=ast.Load()), include_attributes=False) for v in ("T", "atts", "phi")],
            "actual hard_stats(T, atts, phi) call")
    require(not any(isinstance(n, ast.Name) and n.id == "_phi" and isinstance(n.ctx, ast.Load)
                    for n in ast.walk(loop)), "original phi ignored in hard loop")
    require("La condensation ne lit que les masses de fin de vie" in data["MEMO_MASSES_SELECTION_20260930.md"],
            "declared structural semantics present")
    ns = {"Fraction": F, "cmp": lambda a, b, soft=False: (a > b) - (a < b)}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), "pinned_selection_AST", "exec"), ns)
    return ns, {"executed": sorted(NAMES), "selection_block_AST_checked_only": True,
                "phi_orig_ignored": True, "structural_final_mass_declared": True}

class Tree:
    birth = list(map(F, [1, 16, 25, 100, 1600]))
    parent = [2, 2, 4, 4, -1]
    children = [[], [], [0, 1], [], [2, 3]]
    root = 4
    death = [F(25), F(25), F(1600), F(1600), None]
    def __len__(self):
        return len(self.birth)

class ExactSquarePhi:
    """Exact on this fixture's perfect-square rational beta, no Decimal runtime."""
    def __init__(self, z):
        self.z = z
    def zero(self):
        return F(0)
    def __call__(self, beta):
        if beta is None:
            return F(0)
        beta = F(beta)
        a, b = math.isqrt(beta.numerator), math.isqrt(beta.denominator)
        require(a * a == beta.numerator and b * b == beta.denominator, "square rational beta")
        return F(b, a) ** self.z

def departure_score(birth_lambda, exits, mcs):
    """Independent ascending-lambda sweep; departures are simultaneous cohorts."""
    alive, previous, area = len(exits), birth_lambda, F(0)
    trace = []
    for lam in sorted(set(exits)):
        require(lam >= birth_lambda, "departure before birth")
        part = alive * (lam - previous)
        area += part
        after = alive - exits.count(lam)
        trace.append({"lambda": str(lam), "before": alive, "after": after, "area": str(part)})
        alive, previous = after, lam
        if alive < mcs:
            return area, lam, trace
    raise ValueError("unterminated finite cohort fixture")

def evaluate(root):
    ns, guards = inspect_sources(root)
    T, phi = Tree(), ExactSquarePhi(1)
    pairs = [(1, 0), (4, 0), (9, 0), (16, 1), (16, 1), (100, 3), (100, 3)] * 3
    atts = [NS(owner=o, beta=F(b), phi_date=phi(b)) for b, o in pairs]
    stats = ns["hard_stats"](T, atts, phi)
    clusters = ns["condense"](T, stats, 5)
    chosen, scores = ns["eom"](T, stats, clusters)
    tops = [clusters[i]["top"] for i in chosen]
    require(stats.end_mass == [9, 6, 15, 6, 21], "API21 end masses")
    require(stats.stab == list(map(F, ["37/10", "3/10", "21/8", "9/20", "21/40"])), "API21 wholelife scores")
    require(tops == [3, 0, 1], "pinned traversal order")
    A, endA, traceA = departure_score(F(1, 5), [F(1), F(1, 2), F(1, 3)] * 3, 5)
    B, endB, traceB = departure_score(F(1, 5), [F(1, 4)] * 6, 5)
    R, C = 15 * (F(1, 5) - F(1, 40)), 6 * (F(1, 10) - F(1, 40))
    require(A == F(11, 5) and B == F(3, 10) and R == F(21, 8) and C == F(9, 20)
            and R > A + B and stats.stab[0] - A == F(3, 2), "standard departure oracle")
    cases = []
    for oldz, newz, beta, expected_got, expected_right in [
        (2, 3, 4, F(121, 500), F(117, 1000)),
        (3, 2, 9, F(-2, 675), F(16, 225)),
        (1, 1, 9, F(2, 15), F(2, 15)),
        (2, 2, 4, F(21, 100), F(21, 100)),
        (3, 3, 9, F(98, 3375), F(98, 3375)),
        (4, 4, 9, F(544, 50625), F(544, 50625)),
    ]:
        oldphi, newphi = ExactSquarePhi(oldz), ExactSquarePhi(newz)
        a = NS(owner=0, beta=F(beta), phi_date=oldphi(beta))
        got = ns["hard_stats"](T, [a], newphi).stab[0]
        right = newphi(beta) - newphi(T.death[0])
        fixed = ns["hard_stats"](T, [NS(owner=0, beta=F(beta), phi_date=newphi(beta))], newphi).stab[0]
        require(got == expected_got and right == expected_right and fixed == right, "exact scale case")
        require((got == right and got > 0) if oldz == newz else got != right, "scale positive controls")
        cases.append({"z_attach": oldz, "z_eom": newz, "beta": str(beta), "death": "25",
                      "observed": str(got), "correct": str(right), "converted_input_control": str(fixed)})
    # Check the historical wrong-order expectation itself without rerunning its original command.
    require(tops != [3, 1, 0] and set(tops) == {0, 1, 3}, "preflight order, not mathematical error")
    return {
        "pins": PINS, "guards": guards,
        "API21": {"n": len(atts), "mcs": 5, "z": 1, "birth": list(map(str, T.birth)), "parent": T.parent,
                  "attachments": [[str(b), o] for b, o in pairs],
                  "end_mass": list(map(str, stats.end_mass)), "wholelife_stab": list(map(str, stats.stab)),
                  "clusters": clusters, "chosen_tops_traversal": tops,
                  "standard_scores": {"A": str(A), "B": str(B), "R": str(R), "C": str(C)},
                  "standard_chosen_tops": [2, 3], "A_cut_lambda": str(endA), "B_cut_lambda": str(endB),
                  "A_departures": traceA, "B_departures": traceB, "A_overcount": "3/2"},
        "scale_cases": cases,
        "scope": {"abstract_tree_API": True, "Fraction_exact": True, "native": False,
                  "HDBSCAN_run": False, "Decimal_runtime_qualified": False, "GCP": False,
                  "fractional_final_mass_semantics_refuted": False},
    }

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--archive", type=Path, default=Path(__file__).resolve().parent)
    args = ap.parse_args()
    root = args.archive
    before = {name: sha(root / "snapshots" / name) for name in PINS}
    out = evaluate(root)
    require(before == {name: sha(root / "snapshots" / name) for name in PINS}, "after source hashes")
    print(json.dumps(out, sort_keys=True, separators=(",", ":")))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
