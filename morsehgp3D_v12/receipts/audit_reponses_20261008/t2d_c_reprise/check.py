#!/usr/bin/env python3
"""Relecture statique des copies externes epinglees ; aucun temoin ni moteur."""
import argparse
import ast
import hashlib
import json
from pathlib import Path


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def function(text, name):
    nodes = [n for n in ast.parse(text).body if isinstance(n, ast.FunctionDef) and n.name == name]
    require(len(nodes) == 1, name)
    return ast.dump(nodes[0], include_attributes=False)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", type=Path, required=True)
    p.add_argument("--replay", type=Path, required=True)
    a = p.parse_args()
    c = json.loads(Path(__file__).with_name("capture.json").read_text())
    sources, references = {}, {}
    for pins, root, dest in [(c["source_pins"], a.source, sources), (c["reference_pins"], a.replay, references)]:
        for rel, pin in pins.items():
            b = (root / rel).read_bytes()
            require(sha(b) == pin, "source alteree : " + rel)
            dest[rel] = b.decode()
    for row in c["ast_comparisons"]:
        x = function(sources[row["source"]], row["function"])
        y = function(references[row["reference"]], row["function"])
        require(x == y and sha(x.encode()) == row["ast_sha256"], "AST altere")
    for row in c["causal_excerpts"]:
        lines = sources[row["source"]].splitlines()
        lo = row["first_line"] - 1
        require("\n".join(lines[lo:lo + len(row["text"].splitlines())]) == row["text"], "extrait altere")
    print(json.dumps({"sources": len(sources), "references": len(references),
                      "ast_unchanged": [r["function"] for r in c["ast_comparisons"]],
                      "causal_excerpts": len(c["causal_excerpts"])}, sort_keys=True))


if __name__ == "__main__":
    main()
