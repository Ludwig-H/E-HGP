"""Fixture crossK : deux amas core de la tour exacte se croisent, malgre laminarite par K."""
import json
import os
import struct
import subprocess
import sys
import tempfile

from tower_full_check import parse, ancestor, R


def cut(order, a):
    groups = {}
    for x, v, e in order["points"]:
        if e <= a:
            groups.setdefault(ancestor(order["nodes"], v, a), set()).add(x)
    return sorted((sorted(g) for g in groups.values()))


def main():
    P = [(0, 0, 0), (1, 0, 0), (4, 0, 0), (7, 0, 0)]
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "cross.u32le")
        with open(src, "wb") as f:
            for p in P:
                f.write(struct.pack("<III", *p))
        r = subprocess.run([sys.argv[1], src, "4", "2"], capture_output=True, text=True, timeout=10)
        if r.returncode:
            raise RuntimeError(r.stdout)
        Q, modes = parse(r.stdout)
    a, b = R.Fr(1, 4), R.Fr(36)
    A, B = cut(modes["core"][1], a), cut(modes["core"][4], b)
    want_a, want_b = R.point_partition_gamma(Q, 1, a), R.point_partition_gamma(Q, 4, b)
    if {frozenset(x) for x in A} != set(want_a) or {frozenset(x) for x in B} != set(want_b):
        raise RuntimeError("binary differs from independent Gamma")
    C, D = set((0, 1)), set((1, 2))
    if sorted(C) not in A or sorted(D) not in B or not (C & D and C - D and D - C):
        raise RuntimeError("crossing absent")
    print(json.dumps(dict(status="ok", positions=Q, cuts=[dict(k=1, a="1/4", groups=A),
                                                          dict(k=4, a="36", groups=B)],
                          crossing=dict(first=sorted(C), second=sorted(D))), sort_keys=True))


if __name__ == "__main__":
    main()
