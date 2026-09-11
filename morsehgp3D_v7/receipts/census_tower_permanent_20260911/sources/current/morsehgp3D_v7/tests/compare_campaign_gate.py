"""Bounded adversarial tests of the paired runner, using explicitly fake bins.

No timings emitted by these fixtures are scientific measurements. Source
snapshots are controlled stubs for lifecycle tests; real snapshot hashing is
tested separately against temporary source files. Guards remain active in -O.
"""

from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import time


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


RUNNER = Path(__file__).resolve().parents[1] / "bench" / "compare_v6_v7.py"
SPEC = importlib.util.spec_from_file_location("compare_campaign", RUNNER)
require(SPEC is not None and SPEC.loader is not None, "runner import spec")
runner = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runner)


def fixture(label: str, digest_seed: str = "fixture", *, kmax: int = 10,
            threads: int = 2, serial: bool = False, s: int = 8) -> str:
    digests = [hashlib.sha256(f"{digest_seed}:{k}".encode()).hexdigest() for k in range(1, kmax + 1)]
    digest_all = hashlib.sha256(b"mhgp4-digest-v1:all" + "".join(digests).encode()).hexdigest()
    lines = [
        f"payload=mhgp{label[1:]}-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
        "backend=cpu_reference",
        ("tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11" if kmax == 10 else
         "tower_scope=prefix_k5 smax_requested=6 smax_effective=6 (K = 1..5, prefixe exact de l'objet complet)"),
        f"forest_layout=csr forest_storage_kind=csr_facet_keys_v1 csr_fallback=0 ordres_publies={kmax} ordres_storage_conformes={kmax}",
    ]
    if label == "v7":
        lines.append("forest_semantics=verified_events_only public_status=not_claimed require_exact=false")
    lines.append(
        f"famille=uniform n=200 coord=200 s={s} smax={kmax + 1} seed=3 threads={threads} emis=30 boules_uniques=20 "
        f"mortes_profondeur=1 survivantes=19 census_int=10 census_shell=20 evenements={2 * kmax} "
        f"facettes={198 + 2 * kmax} fusions={kmax} deltas={kmax} noeuds={198 + 4 * kmax}"
    )
    if serial:
        lines.append("temps_fold_mur_ms=0.5 (etages A et B, fold_inflight=1, fold_join=1, pic_mesure_en_vol=1)")
    for k in range(1, kmax + 1):
        facets = 200 if k == 1 else 2
        lines.append(f"cardinalites K={k} evenements=2 facettes={facets} deltas=1 attachements=1 fusions=1 noeuds={facets + 2}")
        lines.append(f"digest_forest_K{k}={digests[k - 1]}")
    lines.extend([f"digest_all={digest_all}", "temps_mur_ms=1.0 (fake fixture)"])
    return "\n".join(lines) + "\n"


def parser_gate() -> int:
    text = fixture("v7")
    usage = "\tMaximum resident set size (kbytes): 100\n\tExit status: 0\n"

    def parse(stdout: str, stderr: str = "", timing: str = usage) -> dict:
        return runner.parse_success(stdout, stderr, timing, label="v7", family="uniform",
                                    n=200, seed=3, threads=2, wall_seconds=1.0)

    require(len(parse(text)["digests"]) == 11, "positive parser fixture")
    corruptions = [
        (text + next(line for line in text.splitlines() if line.startswith("digest_all=")) + "\n", "", usage),
        (text.replace("digest_all=", "digest_all_extra="), "", usage),
        (text.replace("digest_forest_K10=", "digest_forest_K11="), "", usage),
        (text.replace("digest_all=", "digest_all=0"), "", usage),
        (text.replace("digest_forest_K1=", "digest_forest_K1=" + "0" * 64 + "\nother="), "", usage),
        (text.replace("seed=3", "seed=4"), "", usage),
        (text.replace("threads=2", "threads=3"), "", usage),
        (text.replace("coord=200", "coord=65537"), "", usage),
        (text.replace("survivantes=19", "survivantes=20"), "", usage),
        (text.replace("cardinalites K=1 ", "cardinalites K=2 "), "", usage),
        (text.replace("facettes=218", "facettes=219"), "", usage),
        (text.replace("temps_mur_ms=1.0", "temps_mur_ms=nan"), "", usage),
        (text.replace("temps_mur_ms=1.0", "temps_mur_ms=0.0"), "", usage),
        (text.replace("temps_mur_ms=1.0", "temps_mur_ms=2000.0"), "", usage),
        (text + "temps_mur_ms=1.0\n", "", usage),
        (text.replace("backend=cpu_reference", "backend=gpu"), "", usage),
        (text.replace("csr_fallback=0", "csr_fallback=1"), "", usage),
        (text.replace("not_claimed", "exact"), "", usage),
        (text + "memory_budget_scope=partial\n", "", usage),
        (text, "warning\n", usage),
        (text, "", "\tExit status: 0\n"),
        (text, "", usage + usage),
        (text, "", usage.replace("kbytes): 100", "kbytes): 0")),
        (text, "", usage.replace("status: 0", "status: 3")),
    ]
    for index, arguments in enumerate(corruptions):
        try:
            parse(*arguments)
        except RuntimeError:
            continue
        raise RuntimeError(f"parser corruption survived: {index}")
    return len(corruptions)


def fake_binary(path: Path, label: str, behavior: str, *, kmax: int = 10,
                threads: int = 2, serial: bool = False) -> None:
    payload = fixture(label, "different" if behavior == "divergent" else "fixture",
                      kmax=kmax, threads=threads, serial=serial)
    if behavior == "internal_work":
        payload = payload.replace("emis=30", "emis=31")
    source = f"#!{sys.executable}\nimport os, sys, time\n"
    if behavior == "timeout":
        # Exercise descendants as well as the top-level timed command.
        source += "if os.fork() == 0:\n    time.sleep(30)\n    sys.exit(0)\ntime.sleep(30)\n"
    elif behavior == "refusal":
        source += "sys.stderr.write('REFUS: fixture only\\n')\nsys.exit(3)\n"
    elif behavior == "refusal_publishes":
        source += f"sys.stdout.write({payload!r})\nsys.exit(3)\n"
    else:
        source += "separation = next(a.split('=', 1)[1] for a in sys.argv[1:] if a.startswith('--s='))\n"
        source += f"payload = {payload!r}.replace(' s=8 ', ' s=' + separation + ' ')\n"
        if behavior == "cross_s_divergent":
            changed = fixture(label, "s12-different", kmax=kmax, threads=threads, serial=serial, s=12)
            source += f"if separation == '12': payload = {changed!r}\n"
        if behavior == "cross_s_work":
            source += "payload = payload.replace('emis=30', 'emis=' + str(30 + int(separation)))\n"
        source += "sys.stdout.write(payload)\n"
    path.write_text(source)
    path.chmod(0o700)


def snapshot_gate(root: Path) -> None:
    for version in ("morsehgp3D_v6", "morsehgp3D_v7"):
        (root / version / "src").mkdir(parents=True)
        (root / version / "src" / "test.hpp").write_text("first\n")
        (root / version / "CMakeLists.txt").write_text("fixture\n")
    (root / "morsehgp3D_v7" / "bench").mkdir()
    (root / "morsehgp3D_v7" / "bench" / "compare_v6_v7.py").write_text("fixture\n")
    before = runner.source_snapshot(root)
    require(len(before) == 5, "nonvacuous source hash manifest")
    (root / "morsehgp3D_v7" / "src" / "test.hpp").write_text("second\n")
    require(before != runner.source_snapshot(root), "source byte mutation")
    (root / "morsehgp3D_v7" / "src" / "new.hpp").write_text("new\n")
    require(len(runner.source_snapshot(root)) == 6, "source addition")


def campaign_gate(root: Path, name: str, behavior: str, *, drift_at: int = 0,
                  kmax: int = 10, serial: bool = False, reference_version: str = "v6",
                  wire_mismatch: bool = False, separations: tuple[int, ...] = (8,)) -> None:
    case = root / name
    case.mkdir()
    reference, candidate = case / "reference", case / "candidate"
    threads = 1 if serial else 2
    common = {"kmax": kmax, "threads": threads, "serial": serial}
    fake_binary(reference, "v6" if wire_mismatch else reference_version,
                behavior if behavior.startswith("cross_s_") else "normal", **common)
    fake_binary(candidate, "v7", behavior, **common)
    output = case / "receipt"
    calls = 0

    def snapshots(_: Path) -> dict:
        nonlocal calls
        calls += 1
        return {"fixture_source": "changed" if drift_at and calls >= drift_at else "fixed"}

    old_argv, old_snapshot = sys.argv, runner.source_snapshot
    sys.argv = [str(RUNNER), "--reference", str(reference), "--candidate", str(candidate),
                "--output", str(output), "--sizes", "200", "--families", "uniform",
                "--seeds", "3", "--threads", "2", "--timeout", "1", "--address-limit-gib", "2"]
    if kmax != 10:
        sys.argv += ["--kmax", str(kmax)]
    if serial:
        sys.argv += ["--serial-stages"]
    if reference_version != "v6":
        sys.argv += ["--reference-version", reference_version]
    if separations != (8,):
        sys.argv += ["--separations", *map(str, separations)]
    runner.source_snapshot = snapshots
    failed = False
    try:
        with redirect_stdout(io.StringIO()):
            runner.main()
    except RuntimeError:
        failed = True
    finally:
        sys.argv, runner.source_snapshot = old_argv, old_snapshot
    summary = json.loads((output / "summary.json").read_text())
    records = json.loads((output / "runs.json").read_text())
    expected_success = behavior in ("normal", "internal_work", "cross_s_work") and not drift_at and not wire_mismatch
    require(failed != expected_success, f"campaign exit: {name}")
    require(summary["status"] == ("completed" if expected_success else "invalid"), f"campaign status: {name}")
    require(summary["all_objects_equal"] == expected_success, f"campaign object claim: {name}")
    require(summary["performance_qualification"] == "not_claimed", "no performance promotion")
    metadata = json.loads((output / "metadata.json").read_text())
    require(metadata["serial_stages_requested"] == serial and
            metadata["strict_single_thread_qualification"] == "not_claimed", "requested scheduling is not strict thread proof")
    require(metadata["binary_roles"]["reference"]["wire_version"] == reference_version and
            metadata["binary_roles"]["candidate"]["wire_version"] == "v7", "roles and real wire versions")
    require(metadata["source_binary_binding"] == "source_hashes_and_binary_hashes_only_build_not_attested" and
            metadata["time_scope"] == "external_process_including_digest_not_warm_e2e", "provenance and digest time scope")
    for record in records:
        require(record["version"] == (reference_version if record["role"] == "reference" else "v7"), "record wire role")
        require(record["s"] in separations and f"--s={record['s']}" in record["command"] and
                f"--smax={kmax + 1}" in record["command"] and f"--threads={threads}" in record["command"], "matrix command identity")
        if serial:
            require("--fold-inflight=1" in record["command"] and "--fold-join=1" in record["command"], "serialized stage command")
        else:
            require("--fold-inflight=2" in record["command"] and "--fold-join=0" in record["command"], "unchanged default scheduling command")
    if expected_success:
        require(len(records) == 2 * len(separations) and summary["cross_separation_comparisons"] == len(separations) - 1,
                "complete noncolliding separation matrix")
        require(len(list(output.glob("*.out"))) == len(records), "role/separation output collision")
    hashes = json.loads((output / "hashes.json").read_text())
    require(len(hashes) >= 4 and all(runner.sha256(output / file) == digest for file, digest in hashes.items()),
            f"terminal hashes: {name}")
    if drift_at:
        require(not summary["source_stable"] and all(record["status"] == "invalid" for record in records),
                "unstable source invalidates prior records")
    elif behavior == "timeout":
        require(len(records) == 2 and records[1]["timed_out"] and records[1]["returncode"] != 0,
                "timeout cannot publish success")
    elif behavior == "refusal":
        require(records[1]["returncode"] == 3 and records[1]["status"] == "failed", "refusal status")
    elif behavior == "refusal_publishes":
        require(records[1]["status"] == "invalid" and records[1]["error"] == "refusal_published_stdout",
                "refusal output contract")
    elif behavior == "divergent":
        require(all(record["status"] == "invalid" and record["error"] == "paired_object_divergence"
                    for record in records), "paired divergence")
    elif behavior == "cross_s_divergent":
        require(len(records) == 6 and all(record["status"] == "invalid" and
                record["error"] == "cross_separation_object_divergence" for record in records), "cross-separation divergence")
    elif wire_mismatch:
        require(records[0]["status"] == "invalid", "wrong reference wire version rejected")


def extended_parser_gate() -> int:
    usage = "\tMaximum resident set size (kbytes): 100\n\tExit status: 0\n"
    rejected = 0
    for kmax in (5, 10):
        for label in ("v6", "v7"):
            for separation in (8, 10, 12):
                for serial in (False, True):
                    threads = 1 if serial else 2
                    text = fixture(label, kmax=kmax, threads=threads, serial=serial, s=separation)
                    def parse(data: str) -> dict:
                        return runner.parse_success(data, "", usage, label=label, family="uniform", n=200,
                                                    seed=3, threads=threads, wall_seconds=1.0, kmax=kmax,
                                                    serial_stages_requested=serial, s=separation)
                    require(len(parse(text)["digests"]) == kmax + 1, "extended positive parser")
                    wrong_version = "7" if label == "v6" else "6"
                    bad = [text.replace(f"payload=mhgp{label[1:]}", f"payload=mhgp{wrong_version}"),
                           text.replace(f" s={separation} ", " s=9 "),
                           text.replace(f"cardinalites K={kmax} ", f"cardinalites K={kmax + 1} "),
                           text + "digest_forest_K11=" + "0" * 64 + "\n",
                           text.replace("digest_all=", "digest_all=0")]
                    if kmax == 5:
                        bad += [text.replace("prefix_k5", "profile_complete_k10"),
                                text.replace("smax_requested=6", "smax_requested=11"),
                                text.replace("ordres_publies=5", "ordres_publies=10"),
                                text + "digest_forest_K6=" + "0" * 64 + "\n"]
                    if serial:
                        bad += [text.replace("fold_join=1", "fold_join=0"),
                                text.replace("fold_inflight=1", "fold_inflight=2"),
                                text.replace("pic_mesure_en_vol=1", "pic_mesure_en_vol=2"),
                                text.replace("temps_fold_mur_ms=", "missing_fold_mur_ms="),
                                text.replace("threads=1", "threads=2")]
                    for changed in bad:
                        try:
                            parse(changed)
                        except RuntimeError:
                            rejected += 1
                        else:
                            raise RuntimeError("extended parser corruption survived")
    return rejected


def argument_gate(root: Path) -> int:
    rejected = 0
    old_argv = sys.argv
    reference, candidate = root / 'argument-reference', root / 'argument-candidate'
    fake_binary(reference, 'v6', 'normal')
    fake_binary(candidate, 'v7', 'normal')
    cases = ((['--kmax', '6'], 'argument --kmax'),
             (['--reference-version', 'v8'], 'argument --reference-version'),
             (['--separations', '8', '8'], 'duplicate matrix coordinates'),
             (['--separations', '7'], 'argument --separations'),
             (['--kmax', '5', '--sizes', '5'], 'K1..5 requires n >= 6'))
    for index, (bad, expected) in enumerate(cases):
        output = root / f"bad-args-{index}"
        sys.argv = [str(RUNNER), '--reference', str(reference), '--candidate', str(candidate),
                    '--output', str(output), '--sizes', '200', '--families', 'uniform',
                    '--seeds', '3', '--threads', '2', '--timeout', '1', *bad]
        errors = io.StringIO()
        try:
            with redirect_stdout(io.StringIO()), redirect_stderr(errors):
                runner.main()
        except RuntimeError as error:
            require(expected in str(error), 'wrong causal argument rejection')
            rejected += 1
        except SystemExit as error:
            require(error.code == 2 and expected in errors.getvalue(), 'wrong argparse rejection')
            rejected += 1
        else:
            raise RuntimeError("invalid matrix argument survived")
        finally:
            sys.argv = old_argv
        require(not output.exists(), "invalid argument must reject before output creation")
    return rejected


def descendant_gate(root: Path) -> None:
    source = root / "descendant.py"
    pid_file = root / "descendant.pid"
    source.write_text(
        "import os, pathlib, time\n"
        "child = os.fork()\n"
        "if child == 0:\n"
        "    time.sleep(30)\n"
        "else:\n"
        f"    pathlib.Path({str(pid_file)!r}).write_text(str(child))\n"
        "    time.sleep(30)\n"
    )
    rc, timeout, elapsed = runner.run_process([sys.executable, str(source)], root / "child.out",
                                               root / "child.err", timeout=1, address_limit_gib=2)
    require(timeout and rc != 0 and elapsed < 5 and pid_file.exists(), "bounded descendant fixture")
    pid = int(pid_file.read_text())
    # Zombies have no running program and cannot be reaped by this non-parent.
    status = Path(f"/proc/{pid}/stat")
    for _ in range(50):
        try:
            state = status.read_text().rsplit(")", 1)[1].split()[0]
        except FileNotFoundError:
            return
        if state == "Z":
            return
        time.sleep(0.01)
    raise RuntimeError("timeout descendant remains running")


def main() -> None:
    corruptions = parser_gate()
    extended = extended_parser_gate()
    with tempfile.TemporaryDirectory(prefix="mhgp7-compare-gate-") as directory:
        root = Path(directory)
        snapshot_gate(root / "source")
        argument_rejections = argument_gate(root)
        for name, behavior, drift_at in (
            ("completed", "normal", 0), ("internal_work", "internal_work", 0),
            ("refusal", "refusal", 0), ("refusal_publishes", "refusal_publishes", 0),
            ("timeout", "timeout", 0), ("divergent", "divergent", 0),
            ("drift_before", "normal", 2), ("drift_during", "normal", 3),
        ):
            campaign_gate(root, name, behavior, drift_at=drift_at)
        for kmax in (5, 10):
            for version in ("v6", "v7"):
                campaign_gate(root, f"serial_k{kmax}_{version}", "normal", kmax=kmax,
                              serial=True, reference_version=version, separations=(8, 10, 12))
        campaign_gate(root, "k5_parallel", "normal", kmax=5)
        campaign_gate(root, "wire_mismatch", "normal", reference_version="v7", wire_mismatch=True)
        campaign_gate(root, "separation_work", "cross_s_work", separations=(8, 10, 12))
        campaign_gate(root, "separation_divergence", "cross_s_divergent", separations=(8, 10, 12))
        descendant_gate(root)
    print(f"compare_campaign_gate=passed parser_rejections={corruptions} extended_rejections={extended} "
          f"argument_rejections={argument_rejections} campaigns=16 descendants=1 fake_metrics_only=true")


if __name__ == "__main__":
    main()
