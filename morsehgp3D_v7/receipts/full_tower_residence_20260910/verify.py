"""Read-only entrypoint to the sealed residence capture, including under -O."""
import hashlib
import json
from pathlib import Path
import runpy

HERE = Path(__file__).resolve().parent
CAPTURE = HERE / "capture"
PIN = "84b439e0309c6beda2c0e7414477be403b5a364e384ab5b3cd717fba40196960"
if hashlib.sha256((CAPTURE / "manifest.json").read_bytes()).hexdigest() != PIN:
    raise RuntimeError("residence manifest pin mismatch")
for name,digest in json.loads((CAPTURE / "manifest.json").read_text()).items():
    relative = Path(name)
    if relative.is_absolute() or ".." in relative.parts:
        raise RuntimeError("unsafe residence manifest path")
    if hashlib.sha256((CAPTURE / relative).read_bytes()).hexdigest() != digest:
        raise RuntimeError("residence capture file mismatch: " + name)
runpy.run_path(str(CAPTURE / "verify.py"),run_name="__main__")
