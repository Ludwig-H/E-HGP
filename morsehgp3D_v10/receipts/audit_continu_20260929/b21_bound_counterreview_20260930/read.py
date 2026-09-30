from pathlib import Path
import hashlib
import json
import subprocess
import sys

def need(ok, label):
    if not ok:
        raise RuntimeError(label)

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def unique(items):
    out = {}
    for k, v in items:
        need(k not in out, "duplicate JSON key")
        out[k] = v
    return out

def parse(raw):
    return json.loads(raw, object_pairs_hook=unique)

root = Path(__file__).resolve().parent
names = {"bounds.py", "capture.json", "README.md", "manifest.json", "read.py"}
need({p.name for p in root.iterdir()} == names, "exact archive inventory")
files = {n: (root / n).read_bytes() for n in names}
need(sha(files["manifest.json"]) == "c3bf17f5186a1521a5ef590b62177ed1989037bf9aff020e01685c0de2bd32ee", "manifest pin")
mf = parse(files["manifest.json"])
need(set(mf) == {"schema", "files"} and mf["schema"] == 1, "manifest schema")
need(set(mf["files"]) == {"bounds.py", "capture.json", "README.md"}, "manifest inventory")
for name, digest in mf["files"].items():
    need(sha(files[name]) == digest, "file hash " + name)
cap = parse(files["capture.json"])
need(cap["scope"] == "pure_rational_bound_model_not_native_not_gcp", "scope")
need(cap["commit"] == "5dd83b5c68919d87ada067204d19ee4edb166859", "private engine version")
need(cap["script_sha256"] == sha(files["bounds.py"]), "script provenance")
need(len(cap["source_pins"]) == 4 and len(cap["commands"]) == 2, "nonempty inventories")
out = []
for i, row in enumerate(cap["commands"]):
    options = ["-B"] + (["-O"] if i else [])
    need(row["argv"] == ["python3"] + options + ["bounds.py"], "captured argv")
    need(row["exit_code"] == 0, "captured code")
    stdout = row["stdout"]
    need("echecs : 0 ; constats : 3\n" in stdout, "scope-aware summary")
    need(sum(s.startswith("CONSTAT ") for s in stdout.splitlines()) == 3, "documentary gaps retained")
    need("ECHEC" not in stdout, "no failed model assertion")
    for b in range(1, 22):
        need("ok     B%d : m >= erreur conjointe " % b in stdout, "joint bound")
        need("ok     B%d : I3 sous conv : m >= " % b in stdout, "MEB bound")
    replay = subprocess.run([sys.executable] + options + ["-c", files["bounds.py"].decode()],
                            capture_output=True, text=True, timeout=30)
    need(replay.returncode == 0 and replay.stderr == "" and replay.stdout == stdout, "exact frozen replay")
    out.append(stdout)
need(out[0] == out[1], "normal/-O identity")
need({p.name for p in root.iterdir()} == names, "no new entries")
need(all((root / n).read_bytes() == data for n, data in files.items()), "archive unchanged")
print('{"status":"PASS","model_modes":2,"documentary_gaps":3,"native":false,"gcp":false}')
