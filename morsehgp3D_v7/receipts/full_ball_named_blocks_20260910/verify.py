"""Read-only preparation verifier. No 50k execution and no cloud operations."""
import hashlib
import json
from pathlib import Path
import runpy
import tempfile

CAPTURE = Path(__file__).resolve().parent / "capture"
PIN = "46a3f36cbebe795d1aebb9ce6a2381a1bf38106698fa357a6975079bb76f066f"
if hashlib.sha256((CAPTURE / "manifest.json").read_bytes()).hexdigest() != PIN:
    raise RuntimeError("named-block manifest pin mismatch")
manifest = json.loads((CAPTURE / "manifest.json").read_text())
mapping = json.loads((CAPTURE / "storage_map.json").read_text())
if mapping != {name:name + ".source" for name in manifest if name.endswith(".md")}:
    raise RuntimeError("historical Markdown storage map mismatch")
expected = {mapping.get(name,name) for name in manifest} | {"manifest.json","storage_map.json"}
actual = {str(path.relative_to(CAPTURE)) for path in CAPTURE.rglob("*") if path.is_file()}
if actual != expected:
    raise RuntimeError("stored capture inventory mismatch")
for name,digest in manifest.items():
    relative = Path(name)
    if relative.is_absolute() or ".." in relative.parts:
        raise RuntimeError("unsafe named-block manifest path")
    if hashlib.sha256((CAPTURE / mapping.get(name,name)).read_bytes()).hexdigest() != digest:
        raise RuntimeError("named-block capture file mismatch: " + name)
# The unchanged original reader expects the logical paths in its pinned
# manifest. Restore only verified bytes into a fresh disposable directory;
# historical Markdown never becomes navigation in the active repository.
with tempfile.TemporaryDirectory(prefix="mhgp7_named_blocks_receipt_") as spelling:
    logical = Path(spelling)
    for name in [*manifest,"manifest.json"]:
        target = logical / name
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((CAPTURE / mapping.get(name,name)).read_bytes())
    runpy.run_path(str(logical / "verify.py"),run_name="__main__")
