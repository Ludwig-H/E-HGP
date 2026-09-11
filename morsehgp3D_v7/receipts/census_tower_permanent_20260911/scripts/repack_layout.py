#!/usr/bin/env python3
"""Create-only physical relayout; no qualified or logical byte is changed."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
REL = "sources/current/morsehgp3D_v7/bench/COMPARE_MONO.md"
TASKS = {
    "census_tower_permanent_20260911": "f26e11a64ac44c6cc8f617b7e3b670b86fe75427a3aad5370705e8a98114f953",
    "full_ball_batch_permanent_20260911": "6396a594daeab25f409e1f2a5ce812b29ec7bc599e4d7328632f02106533365c",
}


def need(good, reason):
    if not good:
        raise RuntimeError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def files(root):
    need(not any(p.is_symlink() for p in root.rglob("*")), "symlink in source")
    return {p.relative_to(root).as_posix(): p.read_bytes() for p in root.rglob("*") if p.is_file()}


def main():
    need(not (HERE / "packets").exists() and not (HERE / "capture_r1").exists(), "create-only destination")
    snapshots = {}
    for name, pin in TASKS.items():
        snapshot = files(REPO / "morsehgp3D_v7/receipts" / name)
        need(sha(snapshot["manifest.json"]) == pin, "old manifest pin")
        old = json.loads(snapshot["manifest.json"])
        need(set(snapshot) == set(old["files"]) | {"manifest.json"}, "old physical closure")
        for path, entry in old["files"].items():
            need(sha(snapshot[path]) == entry["sha256"] and len(snapshot[path]) == entry["size"], "old physical pin")
        snapshots[name] = snapshot
    out = HERE / "capture_r1"
    out.mkdir()
    report = {"status": "running", "scope": "physical_layout_only", "commands": [], "packets": {}}

    def save():
        (out / "receipt.json").write_bytes(encoded(report))

    def check(root, label, optimize=False, extract=None):
        argv = ["python3", "-B"] + (["-O"] if optimize else []) + [str(root / "verify.py")]
        if extract is not None:
            argv += ["--extract", str(extract)]
        with (out / (label + ".stdout")).open("xb") as stdout, (out / (label + ".stderr")).open("xb") as stderr:
            status = subprocess.run(argv, stdout=stdout, stderr=stderr, check=False).returncode
        report["commands"].append({"label": label, "argv": argv, "exit_code": status,
            "expected_exit_code": 0, "stdout_sha256": sha((out / (label + ".stdout")).read_bytes()),
            "stderr_sha256": sha((out / (label + ".stderr")).read_bytes())})
        save()
        need(status == 0, "reader failure: " + label)

    save()
    try:
        for name, snapshot in snapshots.items():
            source = REPO / "morsehgp3D_v7/receipts" / name
            destination = HERE / "packets" / name
            check(source, name + "_old_normal")
            old_mapping = json.loads(snapshot["storage_map.json"])
            mapping = copy.deepcopy(old_mapping)
            renamed = []
            for logical, entry in mapping.items():
                if entry["storage"] == REL:
                    entry["storage"] = REL + ".source"
                    renamed.append(logical)
            need(len(renamed) == 3 and all(p.endswith("/bench/COMPARE_MONO.md") for p in renamed), "exact three logical aliases")
            updated = {path: data for path, data in snapshot.items()
                       if path not in {"manifest.json", "storage_map.json", "README.md", REL}}
            updated[REL + ".source"] = snapshot[REL]
            updated["storage_map.json"] = encoded(mapping)
            updated["provenance/layout_v1/manifest.json"] = snapshot["manifest.json"]
            updated["provenance/layout_v1/storage_map.json"] = snapshot["storage_map.json"]
            updated["provenance/layout_v1/README.md.source"] = snapshot["README.md"]
            revision = {"scope": "physical_layout_only", "qualified_bytes_changed": False,
                "logical_paths_changed": False, "logical_pins_changed": False,
                "capture_manifest_sha256": sha(snapshot["capture_manifest.json"]),
                "previous_manifest_sha256": TASKS[name],
                "previous_storage_map_sha256": sha(snapshot["storage_map.json"]),
                "reader_sha256_unchanged": sha(snapshot["verify.py"]),
                "physical_rename": {"from": REL, "to": REL + ".source"}, "logical_aliases": sorted(renamed)}
            updated["provenance/layout_v2.json"] = encoded(revision)
            updated["scripts/repack_layout.py"] = Path(__file__).read_bytes()
            note = ("\n## Révision physique du stockage documentaire\n\n"
                "La copie qualifiée `sources/current/morsehgp3D_v7/bench/COMPARE_MONO.md` est stockée physiquement "
                "sous `COMPARE_MONO.md.source`, avec exactement les mêmes octets. Ce snapshot documentaire n'est pas "
                "une documentation autonome dont les liens relatifs auraient été relocalisés. "
                "Les trois chemins logiques restent inchangés dans `storage_map.json` ; l'extraction rétablit le nom `.md`. "
                "Aucune source, commande, sortie, attente ou observation qualifiée n'est modifiée. "
                "Le manifeste de capture et le lecteur sont byte-identiques à la version précédente.\n\n"
                "L'ancien manifeste physique, son stockage et son README sont conservés sous `provenance/layout_v1/`. "
                "Le nouveau manifeste désigne seulement ce nouveau conditionnement ; il ne remplace ni ne réinterprète "
                "le hash historique `" + TASKS[name] + "`. Les chemins historiques restent attribuables via cette provenance. "
                "Les détails du renommage sont dans `provenance/layout_v2.json`. Cette révision n'ajoute aucun résultat "
                "géométrique, CTest, CUDA ou de performance.\n")
            updated["README.md"] = snapshot["README.md"] + note.encode()
            need(updated["capture_manifest.json"] == snapshot["capture_manifest.json"] and
                 updated["verify.py"] == snapshot["verify.py"] and set(mapping) == set(old_mapping), "unchanged qualification authority")
            for logical, old_entry in old_mapping.items():
                entry = mapping[logical]
                need({k: v for k, v in entry.items() if k != "storage"} ==
                     {k: v for k, v in old_entry.items() if k != "storage"}, "unchanged logical metadata")
                need(updated[entry["storage"]] == snapshot[old_entry["storage"]], "unchanged logical bytes")
            manifest = copy.deepcopy(json.loads(snapshot["manifest.json"]))
            manifest["files"] = {path: {"sha256": sha(data), "size": len(data)} for path, data in sorted(updated.items())}
            manifest["physical_layout_revision"] = 2
            manifest["previous_manifest_sha256"] = TASKS[name]
            updated["manifest.json"] = encoded(manifest)
            destination.mkdir(parents=True, exist_ok=False)
            for path, data in updated.items():
                write(destination / path, data)
            check(destination, name + "_new_normal")
            check(destination, name + "_new_optimized", True)
            extracted = out / (name + "_extracted")
            check(destination, name + "_new_extract", True, extracted)
            extracted_files = files(extracted)
            need(set(extracted_files) == set(mapping), "extraction logical coverage")
            for logical, entry in old_mapping.items():
                need(extracted_files[logical] == snapshot[entry["storage"]], "extraction byte identity")
            spec = importlib.util.spec_from_file_location("repo_check_docs", REPO / "tools/check_docs.py")
            docs = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(docs)
            markdown = sorted(destination.rglob("*.md"))
            errors = [error for path in markdown for error in docs.validate(path)]
            need(not errors, "private physical Markdown errors: " + repr(errors))
            need(files(source) == snapshot, "source packet changed during relayout")
            report["packets"][name] = {**revision, "destination": str(destination),
                "new_manifest_sha256": sha(updated["manifest.json"]), "logical_files": len(mapping),
                "physical_files": len(updated), "private_markdown_checked": len(markdown),
                "extracted_bytes_identical": True, "existing_receipt_unchanged": True}
            save()
        report["status"] = "passed"
    except BaseException as error:
        report["status"] = "failed"
        report["error"] = str(error)
        raise
    finally:
        save()
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
