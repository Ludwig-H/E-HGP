"""Seal preparation only; no 50k execution, GPU call or Git operation."""
import difflib
import json
from pathlib import Path

from verify import BASE, check, sha

summary = check()
tree = BASE / "overlay"
pins = {str(p.relative_to(tree)):sha(p) for p in sorted(tree.rglob("*")) if p.is_file()}
(BASE / "overlay_after.json").write_text(json.dumps(pins,indent=2)+"\n")
(BASE / "summary.json").write_text(json.dumps(summary,indent=2)+"\n")
relative = Path("morsehgp3D_v7/src/forest/full_ball_tower.hpp")
patch = difflib.unified_diff((BASE / "baseline" / relative).read_text().splitlines(keepends=True),
                             (tree / relative).read_text().splitlines(keepends=True),
                             fromfile="nominal/"+str(relative),tofile="test_overlay/"+str(relative))
(BASE / "observer_overlay.patch").write_text("".join(patch))
suffixes = {".cpp",".hpp",".cuh",".cu",".py",".md",".json",".stdout",".stderr",".sha256",".patch"}
manifest = {str(p.relative_to(BASE)):sha(p) for p in sorted(BASE.rglob("*"))
            if p.is_file() and p.suffix in suffixes and p.name != "manifest.json"}
(BASE / "manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
print(f"sealed NOT_EXECUTED_50K: {len(manifest)} files, manifest SHA256 {sha(BASE / 'manifest.json')}")
