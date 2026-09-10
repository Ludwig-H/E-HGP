"""Publish captured results without binaries or duplicate mutant source trees."""
import difflib
import hashlib
import json
from pathlib import Path
import shutil

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
DEST = ROOT / "morsehgp3D_v7/receipts/witness_front_20260910"
HEADER = "morsehgp3D_v7/src/pipeline/witness_front.hpp"
KINDS = ("o2", "san", "index_binding", "generation_reserved", "singleton_stale_work")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy(source, relative):
    target = DEST / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)


DEST.mkdir(parents=True, exist_ok=False)
pins = json.loads((BASE / "source_before.json").read_text())
if {name: sha(ROOT / name) for name in pins} != pins:
    raise RuntimeError("live source drift before publication")
for name, digest in pins.items():
    if sha(BASE / "snapshot" / name) != digest:
        raise RuntimeError("snapshot changed")
    copy(BASE / "snapshot" / name, "source_snapshot/" + name)
copy(BASE / "source_before.json", "capture/source_before.json")
copy(BASE / "record.py", "capture/record.py")
copy(BASE / "publish.py", "capture/publish.py")
for path in sorted((BASE / "logs").iterdir()):
    copy(path, "capture/logs/" + path.name)
for kind in KINDS:
    for suffix in ("_sources_before.json", "_sources_after.json", "_compiled_sources.json", "_binary.sha256", ".d"):
        copy(BASE / (kind + suffix), "capture/" + kind + suffix)
    if kind not in ("o2", "san"):
        copy(BASE / kind / HEADER, "mutants/" + kind + "/witness_front.hpp")
        old = (BASE / "snapshot" / HEADER).read_text().splitlines(keepends=True)
        new = (BASE / kind / HEADER).read_text().splitlines(keepends=True)
        patch = "".join(difflib.unified_diff(old, new, fromfile="a/" + HEADER, tofile="b/" + HEADER))
        (DEST / "mutants" / kind / "change.patch").write_text(patch)
copy(BASE / "verify_template.py", "verify.py")
copy(BASE / "README_template.md", "README.md")
manifest = {str(path.relative_to(DEST)): sha(path) for path in sorted(DEST.rglob("*")) if path.is_file()}
(DEST / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps({"files": len(manifest) + 1, "bytes": sum(path.stat().st_size for path in DEST.rglob("*") if path.is_file()), "manifest_sha256": sha(DEST / "manifest.json")}))
