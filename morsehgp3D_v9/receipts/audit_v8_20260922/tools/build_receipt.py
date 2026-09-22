#!/usr/bin/env python3
"""Construit le reçu d'audit v8 de l'ouverture v9 (morsehgp3D_v9/receipts/audit_v8_20260922/).

Entrées : journaux CTest des deux constructions neuves du commit audité, inventaire épinglé (inventory.py),
lecteurs de reçus rejoués. Sortie : CHECKS.json, INVENTORY.json, logs/, SHA256SUMS. Aucun assert (python3 -O).
"""
import hashlib, json, re, shutil, subprocess, sys
from pathlib import Path

LOGS = Path("/workspaces/E-HGP/build/v9-audit-logs")
HEAD_WT = Path("/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/v8_head")
OUT = Path(sys.argv[1])


class Failure(RuntimeError):
    pass


def require(cond, msg):
    if not cond:
        raise Failure(msg)


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def ctest_summary(text):
    tests = {}
    for m in re.finditer(r"Test +#(\d+): (\S+) \.+\s*(\*\*\*)?(Passed|Failed|Not Run \(Disabled\)|\S+)\s+([\d.]+) sec", text):
        tests[m.group(2)] = dict(number=int(m.group(1)), status="disabled" if "Disabled" in m.group(4) else m.group(4).lower(), seconds=float(m.group(5)))
    m = re.search(r"(\d+)% tests passed, (\d+) tests failed out of (\d+)", text)
    require(m is not None, "résumé CTest absent")
    total = re.search(r"Total Test time \(real\) = +([\d.]+) sec", text)
    return dict(tests=tests, failed=int(m.group(2)), executed=int(m.group(3)), total_seconds=float(total.group(1)) if total else None)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "logs").mkdir(exist_ok=True)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=HEAD_WT, text=True).strip()
    require(commit.startswith("12294241"), f"worktree audité inattendu : {commit}")
    require(subprocess.check_output(["git", "status", "--porcelain"], cwd=HEAD_WT, text=True).strip() == "", "worktree audité sale")
    head = ctest_summary((LOGS / "head_ctest.log").read_text())
    mut = ctest_summary((LOGS / "mut_ctest.log").read_text())
    require(head["failed"] == 0 and mut["failed"] == 0, "échec CTest")
    passed = {n for n, t in head["tests"].items() if t["status"] == "passed"} | {n for n, t in mut["tests"].items() if t["status"] == "passed"}
    registered = set(head["tests"])
    disabled_everywhere = sorted(n for n in registered if n not in passed)
    require(len(registered) == 132 and len(passed) == 129, f"décomptes inattendus : {len(registered)} / {len(passed)}")
    require(disabled_everywhere == ["mhgp8_q34_indexed_witness_mutations", "mhgp8_q34_spatial_gate", "mhgp8_q34_spatial_gate_optimized"], f"désactivés : {disabled_everywhere}")
    readers = []
    for receipt, variant in (("ground_baseline_20260921", "full"), ("ground_phase1_20260921", "full"), ("ground_phase1_20260921", "only")):
        cmd = ["python3", "-B", "-O", "morsehgp3D_v8/bench/run_ground_baseline.py", "read", "--output", f"morsehgp3D_v8/receipts/{receipt}", "--variant", variant]
        out = subprocess.run(cmd, cwd=HEAD_WT, capture_output=True, text=True)
        require(out.returncode == 0, f"lecteur en échec : {receipt} {variant}")
        readers.append(dict(command=" ".join(cmd), exit=out.returncode, stdout=json.loads(out.stdout)))
    require(subprocess.check_output(["git", "status", "--porcelain"], cwd=HEAD_WT, text=True).strip() == "", "un lecteur a écrit dans le worktree audité")
    compiler = subprocess.check_output(["c++", "--version"], text=True).splitlines()[0]
    cmake = subprocess.check_output(["cmake", "--version"], text=True).splitlines()[0]
    checks = dict(
        schema="mhgp9_audit_v8_checks_v1", phase="exploration_v9_hors_registre", public_status="not_claimed", gcp_used=False,
        audited_commit=commit, host=subprocess.check_output(["uname", "-srm"], text=True).strip(), cpus=subprocess.check_output(["nproc"], text=True).strip(),
        compiler=compiler, cmake=cmake, boost_root="build/v7_boost_gate/extracted/usr (Boost 1.83.0, non versionné)",
        builds=[
            dict(role="suite complète hors de l'arbre canonique", build_dir="build/v9-audit-v8-head-12294241", configure="-DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=…",
                 command="ctest -j 3 --output-on-failure", executed=head["executed"], failed=head["failed"], total_seconds=head["total_seconds"],
                 not_run=sorted(n for n, t in head["tests"].items() if t["status"] == "disabled"), log="logs/head_ctest.log"),
            dict(role="mutations dans l'arbre de build canonique <worktree>/build/", build_dir="<worktree audité>/build/v9-audit-mutations",
                 configure="-DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=…", command="ctest -L mutation --output-on-failure", executed=mut["executed"],
                 failed=mut["failed"], total_seconds=mut["total_seconds"], not_run=sorted(n for n, t in mut["tests"].items() if t["status"] == "disabled"),
                 log="logs/mut_ctest.log")],
        union=dict(registered=len(registered), passed=len(passed), disabled_by_construction=disabled_everywhere,
                   reasons={"mhgp8_q34_indexed_witness_mutations": "site de mutation non unique depuis 2629a536 (CMakeLists.txt)",
                            "mhgp8_q34_spatial_gate": "n'est active que dans le build épinglé build/v8_q4_seed_cells_r2_20260921",
                            "mhgp8_q34_spatial_gate_optimized": "idem"}),
        readers=readers,
        lenses=dict(count=12, verification="une contre-vérification adversariale par lentille, puis une critique de complétude",
                    reports="morsehgp3D_v9/docs/audit_v8/"))
    (OUT / "CHECKS.json").write_text(json.dumps(checks, indent=1, ensure_ascii=False, sort_keys=True) + "\n")
    for name in ("head_configure.log", "head_ctest.log", "mut_configure.log", "mut_ctest.log"):
        text = (LOGS / name).read_text().replace(str(HEAD_WT), "<worktree audité>")
        (OUT / "logs" / name).write_text(text)
    inv = subprocess.run(["python3", "-B", "-O", str(LOGS / "tools/inventory.py"), "--head-worktree", str(HEAD_WT), "--output", str(OUT / "INVENTORY.json")],
                         capture_output=True, text=True)
    require(inv.returncode == 0, f"inventaire en échec : {inv.stderr[-300:]}")
    (OUT / "tools").mkdir(exist_ok=True)
    for name in ("inventory.py", "build_receipt.py"):
        shutil.copyfile(LOGS / "tools" / name, OUT / "tools" / name)
    files = sorted(p for p in OUT.rglob("*") if p.is_file() and p.name != "SHA256SUMS")
    (OUT / "SHA256SUMS").write_text("".join(f"{sha(p)}  {p.relative_to(OUT)}\n" for p in files))
    print(json.dumps(dict(status="passed", registered=len(registered), passed=len(passed), readers=len(readers), inventory=inv.stdout.strip(), files=len(files))))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Failure as f:
        print(json.dumps(dict(status="failed", error=str(f))))
        sys.exit(1)
