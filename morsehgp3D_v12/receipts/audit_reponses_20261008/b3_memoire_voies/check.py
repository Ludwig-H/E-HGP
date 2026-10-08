#!/usr/bin/env python3
"""Read pinned source/public metadata; apply arm substitutions in memory only."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    need(len(sys.argv) == 2, "usage: check.py DEPOT_GIT")
    repo, here = Path(sys.argv[1]).resolve(), Path(__file__).resolve().parent
    cap = json.loads((here / "capture.json").read_text())

    def git(commit, path):
        return subprocess.check_output(["git", "show", commit + ":" + path], cwd=repo)

    source = {}
    for path, expected in cap["source_pins_under_morsehgp3D_v12"].items():
        data = git(cap["b3_git"], "morsehgp3D_v12/" + path)
        need(sha(data) == expected, "source pin " + path)
        source[path] = data.decode()
    arms = json.loads(source["microbancs/mes_t2d_b3/bras_t2d_b3.json"])
    need(arms["base"] == cap["base_git"][:9], "arm base")
    post = {}
    substitution_count = 0
    for name, arm in arms["bras"].items():
        post[name] = {}
        for path, record in arm["fichiers"].items():
            data = git(cap["base_git"], "morsehgp3D_v12/" + path)
            need(sha(data) == record["sha256_avant"], "arm preimage")
            text = data.decode()
            for change in record["substitutions"]:
                need(text.count(change["cherche"]) == 1, "substitution occurrence")
                text = text.replace(change["cherche"], change["remplace"])
                substitution_count += 1
            need(sha(text.encode()) == record["sha256_apres"], "arm postimage")
            post[name][path] = text
    transfer, keys = post["transfert"], post["cles"]
    need(set(keys) - set(transfer) == {"src/catalogue/table.cpp"}, "transfer lookup ablation")
    need(set(transfer) - set(keys) == set(), "unexpected transfer change")
    need(all(text == keys[path] == post["apres"][path] for path, text in transfer.items()), "shared retention bodies")
    need(all(text == source[path] for path, text in transfer.items()), "transfer product retention")
    complete = source["src/catalogue/finish_driver.hpp"]
    need("take_segment(b, a.table.keys[ct], out.table_keys, in.balls" in complete, "complete export")
    sliced = source["src/catalogue/finish_slices.hpp"]
    body = sliced.split("Outcome slice_take(", 1)[1].split("// Une tranche", 1)[0]
    need("table_keys" not in body and "segments[4]" in body, "no sliced key export")
    need("host_table(out.balls.span(), in.sites, out.table_offsets, out.table_values, out.table_keys" in sliced,
         "sliced host reconstruction")
    need("keys.allocate(n, budget)" in source["src/catalogue/slices.cpp"], "host key buffer")
    names = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", cap["b3_git"], "--"] +
                                    cap["public_result_roots"], cwd=repo, text=True).splitlines()
    names = sorted(p for p in names if p.endswith(".jsonl") or p.endswith("/rapport_b.json"))
    metadata = {p: git(cap["b3_git"], p) for p in names}
    inventory = "".join(sha(data) + "  " + path + "\n" for path, data in metadata.items())
    need(len(metadata) == cap["public_result_files"] and sha(inventory.encode()) == cap["public_result_inventory_sha256"],
         "public metadata inventory")
    all_keys, full_rows, raw_files = set(), 0, 0

    def visit(value):
        if isinstance(value, dict):
            for key, val in value.items():
                all_keys.add(key)
                visit(val)
        elif isinstance(value, list):
            for val in value:
                visit(val)

    for path, data in metadata.items():
        if path.endswith(".jsonl"):
            raw_files += 1
            for line in data.splitlines():
                row = json.loads(line)
                full_rows += row.get("phase") == "full"
                if row.get("phase") == "full":
                    need(row.get("status") == "ok", "FULL row status")
                visit(row)
        else:
            visit(json.loads(data))
    count_fields = sorted(all_keys & {"balls", "ball_count", "n_balls", "boules", "balles", "emitted", "incidences", "ledger"})
    need(not count_fields, "new count field needs review")
    result = dict(schema="b3-memory-paths-result-v1", source_git=cap["b3_git"],
                  source_files=len(source), arm_files=sum(len(x) for x in post.values()),
                  exact_substitutions=substitution_count, transfer_common_files=len(transfer),
                  transfer_sliced_reconstruction=True, transfer_direct_lookup=False,
                  key_storage_bytes_per_ball=16, complete_device_extra_key_D2H_bytes_per_ball=16,
                  sliced_device_extra_key_D2H_bytes_per_ball=0,
                  cpu_complete_key_copy=True, total_peak_delta_inferred=False,
                  public_metadata_files=len(metadata), public_raw_files=raw_files,
                  public_full_rows=full_rows, catalogue_count_fields=count_fields,
                  per_scene_overhead_computed=False, native_runs=0)
    rendered = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    saved = here / "results.json"
    if saved.exists():
        need(saved.read_text() == rendered, "result changed")
    print(rendered, end="")


if __name__ == "__main__":
    main()
