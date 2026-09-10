#!/usr/bin/env python3
"""Seal this local, private capture without modifying active files."""
import difflib
import json
from pathlib import Path

from verify import BASE, check, sha

summary = check()
target = BASE / "both_final"
pins = {str(p.relative_to(target)): sha(p) for p in sorted(target.rglob("*")) if p.is_file()}
(BASE / "both_final_sources_after.json").write_text(json.dumps(pins, indent=2) + "\n")
(BASE / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
diff = []
for name in ("full_ball_tower.hpp", "full_coverage_certificate.hpp"):
    relative = Path("morsehgp3D_v7/src/forest") / name
    diff.extend(difflib.unified_diff((BASE / "baseline" / relative).read_text().splitlines(keepends=True),
                                   (BASE / "both_final" / relative).read_text().splitlines(keepends=True),
                                   fromfile="a/" + str(relative), tofile="b/" + str(relative)))
(BASE / "minimal_delta.patch").write_text("".join(diff))
suffixes = {".cpp", ".hpp", ".py", ".md", ".json", ".stdout", ".stderr", ".sha256", ".patch"}
manifest = {str(p.relative_to(BASE)): sha(p) for p in sorted(BASE.rglob("*"))
            if p.is_file() and p.suffix in suffixes and p.name != "manifest.json"}
(BASE / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(f"sealed {len(manifest)} files: manifest SHA256 {sha(BASE / 'manifest.json')}")
