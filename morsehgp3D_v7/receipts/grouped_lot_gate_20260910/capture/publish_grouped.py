"""Seal existing historical runs; never re-run tests, edit live code, or use GCP."""
import difflib
import hashlib
import json
from pathlib import Path
import posixpath
import shlex
import shutil
import time

BASE = Path(__file__).resolve().parent
DEST = BASE.parents[1] / "morsehgp3D_v7/receipts/grouped_lot_gate_20260910"
HEADER = "morsehgp3D_v7/src/forest/full_ball_tower.hpp"
MUTANTS = ("drop_growth_grouped", "drop_inert_grouped", "drop_growth_singleton", "drop_inert_singleton", "strict_radius", "wrong_vertical_cut")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy(source, name):
    target = DEST / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)


def json_read(path):
    return json.loads(path.read_text())


pins = json_read(BASE / "run_r1/sources_before.json")
if pins != json_read(BASE / "run_r2/sources_before.json") or len(pins) != 60:
    raise RuntimeError("historical baseline mismatch")
trees = {"run_r1/baseline": "baseline", "run_r2/baseline": "baseline", "run_r1/drop_growth_grouped": "drop_growth_grouped"}
trees.update({"run_r2/" + name: name for name in MUTANTS})
trees["run_r3_vertical/baseline"] = "wrong_vertical_cut_typed_retry"
before = {tree: {name:sha(BASE / tree / name) for name in pins} for tree in trees}
for tree, values in before.items():
    if {str(path.relative_to(BASE / tree)) for path in (BASE / tree).rglob("*") if path.is_file()} != set(pins):
        raise RuntimeError("archived tree inventory " + tree)
    changed = {name for name in pins if values[name] != pins[name]}
    if changed != (set() if trees[tree] == "baseline" else {HEADER}):
        raise RuntimeError("archive has additional changes " + tree)
if before["run_r1/drop_growth_grouped"] != before["run_r2/drop_growth_grouped"]:
    raise RuntimeError("first failed mutant differs from retry")
DEST.mkdir(parents=True, exist_ok=False)
for name in pins:
    copy(BASE / "run_r1/baseline" / name, "source_snapshot/" + name)
for name in (*MUTANTS, "wrong_vertical_cut_typed_retry"):
    tree = "run_r3_vertical/baseline" if name.endswith("typed_retry") else "run_r2/" + name
    source = BASE / tree / HEADER
    copy(source, "mutants/" + name + "/full_ball_tower.hpp")
    old = (BASE / "run_r1/baseline" / HEADER).read_text().splitlines(keepends=True)
    new = source.read_text().splitlines(keepends=True)
    patch = "".join(difflib.unified_diff(old, new, fromfile="a/" + HEADER, tofile="b/" + HEADER))
    (DEST / "mutants" / name / "change.patch").write_text(patch)
compiled = {}
for run in ("run_r1", "run_r2", "run_r3_vertical"):
    for source in sorted((BASE / run).iterdir()):
        if source.is_file() and source.suffix in (".json", ".py", ".d", ".stdout", ".stderr"):
            copy(source, "capture/" + run + "/" + source.name)
        if source.suffix != ".d":
            continue
        values = {}
        for spelling in shlex.split(source.read_text().replace("\\\n", " ").split(":", 1)[1]):
            normalized = posixpath.normpath(spelling)
            prefix = str(BASE) + "/" + run + "/"
            if not normalized.startswith(prefix):
                raise RuntimeError("external project dependency")
            tree_name, relative = normalized[len(prefix):].split("/", 1)
            tree_key = run + "/" + tree_name
            if tree_key not in trees or relative not in before[tree_key]:
                raise RuntimeError("unknown dependency")
            values[relative] = before[tree_key][relative]
        compiled[run + "/" + source.name] = values
copy(BASE / "publish_grouped.py", "capture/publish_grouped.py")
copy(BASE / "verify_grouped_template.py", "verify.py")
copy(BASE / "README_grouped_template.md", "README.md")
after = {tree: {name:sha(BASE / tree / name) for name in pins} for tree in trees}
if after != before:
    raise RuntimeError("archived source drift while publishing")
(DEST / "capture/archive_at_publication.json").write_text(json.dumps({"epoch": time.time(), "scope": "archived_sources_checked_at_publication_not_historical_after_run", "trees": trees, "before": before, "after": after}, indent=2) + "\n")
(DEST / "capture/compiled_project_closures.json").write_text(json.dumps(compiled, indent=2) + "\n")
manifest = {str(path.relative_to(DEST)):sha(path) for path in sorted(DEST.rglob("*")) if path.is_file()}
(DEST / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps({"files":len(manifest)+1,"bytes":sum(path.stat().st_size for path in DEST.rglob("*") if path.is_file()),"manifest_sha256":sha(DEST / "manifest.json")}))
