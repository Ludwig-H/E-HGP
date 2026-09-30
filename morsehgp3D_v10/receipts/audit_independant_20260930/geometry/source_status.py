"""Lecture seule des copies de correction ; écrit une capture neuve, sans build ni test."""
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


ROOTS = {
    "produit": Path("/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10"),
    "sitetree_r2": Path("/tmp/mhgp10-r2/sitetree/src/morsehgp3D_v10"),
    "oracles_r2": Path("/tmp/mhgp10-r2/oracles/src/morsehgp3D_v10"),
    "oracles_snapshot_r1": Path("/tmp/mhgp10-r2/oracles/src-r1/morsehgp3D_v10"),
    "entrees_r2": Path("/tmp/mhgp10-r2/entrees_cli/src/morsehgp3D_v10"),
}
FILES = (
    "src/cloud/site_tree.cpp",
    "src/cloud/site_tree.hpp",
    "tests/regression/site_tree_far_center.cpp",
    "src/catalogue/generator.cpp",
    "src/catalogue/catalogue.hpp",
    "cli/mhgp10_catalogue.cpp",
    "tests/oracle/test_catalogue_oracle.py",
    "CMakeLists.txt",
)


def capture(root):
    files = {}
    texts = {}
    for name in FILES:
        path = root / name
        data = path.read_bytes() if path.is_file() else None
        files[name] = hashlib.sha256(data).hexdigest() if data is not None else None
        texts[name] = data.decode() if data is not None else ""
    st = texts["src/cloud/site_tree.cpp"]
    oracle = texts["tests/oracle/test_catalogue_oracle.py"]
    cli = texts["cli/mhgp10_catalogue.cpp"]
    generator = texts["src/catalogue/generator.cpp"]
    return {
        "root": str(root),
        "files": files,
        "textual_observations_not_tests": {
            "sitetree_mentions_fegetround": "fegetround" in st,
            "oracle_uses_frozenset": "frozenset(I)" in oracle and "frozenset(U)" in oracle,
            "oracle_sorts_got_by_rank": "sorted(got, key=lambda t: t['rank'])" in oracle,
            "catalogue_cli_mentions_allow_small_leaf": "--allow-small-leaf" in cli,
            "catalogue_cli_mentions_max_nodes": "--max-nodes" in cli,
            "generator_has_k_plus_margin_guard": "params.leaf_size < u32(params.kmax) + kLeafMargin" in generator,
        },
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    result = {
        "utc": datetime.now(timezone.utc).isoformat(),
        "scope": "lecture de copies par groupe ; aucune extraction commune ni qualification de build",
        "roots": {name: capture(root) for name, root in ROOTS.items()},
    }
    with args.out.open("x") as out:
        json.dump(result, out, indent=2)
        out.write("\n")
    print(json.dumps({"utc": result["utc"], "capture": str(args.out), "roots": list(ROOTS)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
