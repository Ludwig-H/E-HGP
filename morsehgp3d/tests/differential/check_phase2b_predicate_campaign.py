#!/usr/bin/env python3
"""Smoke-test Phase 2B GPU campaign transactions with an exact fake runner."""

from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
from types import SimpleNamespace
from typing import Any, NoReturn, Sequence


FAKE_GPU = r'''#!/usr/bin/env python3
import json
import os
import subprocess
import sys

SCHEMAS = {
    "compare_squared_distances": "morsehgp3d.phase2b.distance_filter.v1",
    "orientation_3d": "morsehgp3d.phase2b.orientation_3d_filter.v1",
    "power_bisector_side": "morsehgp3d.phase2b.power_bisector_side_filter.v1",
}

lines = sys.stdin.read().splitlines()
if not lines or any(not line for line in lines):
    print("fake GPU rejected an empty batch", file=sys.stderr)
    raise SystemExit(1)
parsed = []
for line in lines:
    replay, command = line.split(" ", 1)
    parsed.append((int(replay), command, command.split(" ", 1)[0]))
predicates = {row[2] for row in parsed}
if len(predicates) != 1:
    print("fake GPU rejected a non-homogeneous batch", file=sys.stderr)
    raise SystemExit(1)
cpu = subprocess.run(
    [os.environ["MORSEHGP3D_FAKE_CPU"], "--multiprecision-only", "--decision-only", "--batch"],
    input="".join(command + "\n" for _, command, _ in parsed),
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    check=False,
)
if cpu.returncode or cpu.stderr:
    print("fake GPU exact backend failed", file=sys.stderr)
    raise SystemExit(1)
oracle = [json.loads(line) for line in cpu.stdout.splitlines()]
if len(oracle) != len(parsed):
    print("fake GPU exact backend changed cardinality", file=sys.stderr)
    raise SystemExit(1)
flip = os.environ.get("MORSEHGP3D_FAKE_FLIP_FIRST") == "1"
emit_known = os.environ.get("MORSEHGP3D_FAKE_GPU_KNOWN") == "1"
bad_json = os.environ.get("MORSEHGP3D_FAKE_GPU_BAD_JSON", "")
if bad_json not in ("", "noncanonical", "duplicate"):
    print("fake GPU received an invalid JSON mutation mode", file=sys.stderr)
    raise SystemExit(1)
fallback_mode = os.environ.get(
    "MORSEHGP3D_FAKE_GPU_UNKNOWN_STAGE", "cpu_multiprecision"
)
allowed_fallback_stages = ("fp64_filtered", "expansion", "cpu_multiprecision")
if fallback_mode != "cycle" and fallback_mode not in allowed_fallback_stages:
    print("fake GPU received an invalid fallback stage", file=sys.stderr)
    raise SystemExit(1)
known_count = 0
unknown_count = 0
unknown_zeros = 0
fallback_stage_counts = {stage: 0 for stage in allowed_fallback_stages}
for index, ((replay, command, predicate), exact) in enumerate(zip(parsed, oracle, strict=True)):
    sign = exact["sign"]
    if flip and index == 0:
        sign = "negative" if sign != "negative" else "positive"
    known = emit_known and index % 2 == 0 and sign != "zero"
    known_count += known
    unknown_count += not known
    unknown_zeros += not known and sign == "zero"
    fallback_stage = (
        allowed_fallback_stages[index % len(allowed_fallback_stages)]
        if fallback_mode == "cycle"
        else fallback_mode
    )
    if not known:
        fallback_stage_counts[fallback_stage] += 1
    decision = json.dumps({
        "certification_stage": "fp64_filtered" if known else fallback_stage,
        "gpu_filter_sign": sign if known else "unknown",
        "kind": "decision",
        "predicate": predicate,
        "replay_command": command,
        "replay_id": replay,
        "sign": sign,
    }, ensure_ascii=True, separators=(",", ":"), sort_keys=True)
    if index == 0 and bad_json == "noncanonical":
        decision = " " + decision
    elif index == 0 and bad_json == "duplicate":
        decision = decision[:-1] + ',"sign":"positive"}'
    print(decision)
count = len(parsed)
predicate = parsed[0][2]
print(json.dumps({
    "audit_gpu_signs": False,
    "counters": {
        "async_fallback_batches": 1 if unknown_count else 0,
        "cpu_expansion_certified": fallback_stage_counts["expansion"],
        "cpu_fp64_filtered_certified": fallback_stage_counts["fp64_filtered"],
        "cpu_multiprecision_certified": fallback_stage_counts["cpu_multiprecision"],
        "exact_zeros": unknown_zeros,
        "gpu_fp64_certified": known_count,
        "gpu_inputs": count,
        "gpu_known_audited": 0,
        "gpu_unknown_forwarded": unknown_count,
        "remaining_unknown": 0,
    },
    "kind": "summary",
    "schema": SCHEMAS[predicate],
}, ensure_ascii=True, separators=(",", ":"), sort_keys=True))
'''


def fail(message: str) -> NoReturn:
    raise AssertionError(message)


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    )


def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_canonical(path: Path) -> dict[str, Any]:
    content = path.read_text(encoding="ascii")
    if not content.endswith("\n") or content.count("\n") != 1:
        fail(f"{path.name} is not one canonical JSON row")
    value = json.loads(content, object_pairs_hook=reject_duplicates)
    if not isinstance(value, dict) or content != canonical_json(value) + "\n":
        fail(f"{path.name} is not canonical JSON")
    return value


def run(
    command: Sequence[str],
    *,
    environment: dict[str, str],
    expected_status: int = 0,
    timeout: int = 180,
) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        list(command),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
        timeout=timeout,
        env=environment,
    )
    if completed.returncode != expected_status:
        fail(
            f"campaign returned {completed.returncode}, expected {expected_status}: "
            f"stdout={completed.stdout!r}, stderr={completed.stderr!r}"
        )
    if expected_status == 0 and completed.stderr:
        fail(f"successful campaign emitted stderr: {completed.stderr!r}")
    if expected_status != 0 and (
        completed.stdout
        or not completed.stderr.startswith("Phase 2B GPU sign campaign failed closed:")
    ):
        fail("a failed campaign leaked output or lost its closed diagnostic")
    return completed


def git(repository: Path, *arguments: str) -> None:
    completed = subprocess.run(
        ["git", "-C", str(repository), *arguments],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if completed.returncode:
        fail(f"temporary Git command failed: {completed.stderr!r}")


def make_repository(source: Path, destination: Path) -> None:
    (destination / "morsehgp3d/tools").mkdir(parents=True)
    (destination / "morsehgp3d/tests/campaigns").mkdir(parents=True)
    shutil.copy2(
        source / "morsehgp3d/tools/predicate_campaign.py",
        destination / "morsehgp3d/tools/predicate_campaign.py",
    )
    shutil.copy2(
        source / "morsehgp3d/tests/campaigns/phase2a_predicates_v1.json",
        destination / "morsehgp3d/tests/campaigns/phase2a_predicates_v1.json",
    )
    git(destination, "init", "--quiet")
    git(destination, "config", "user.email", "phase2b-smoke@example.invalid")
    git(destination, "config", "user.name", "Phase 2B smoke")
    git(destination, "add", ".")
    git(destination, "commit", "--quiet", "-m", "frozen generator")


def campaign_command(
    tool: Path,
    gpu: Path,
    cpu: Path,
    checkpoint: Path,
    repository: Path,
    *,
    campaign: str = "both",
    max_chunks: int | None = None,
    cases_per_predicate: int = 100,
    chunk_size: int = 60,
    verify_only: bool = False,
) -> list[str]:
    result = [
        sys.executable,
        str(tool),
        str(gpu),
        str(cpu),
        str(checkpoint),
        "--campaign",
        campaign,
        "--repository",
        str(repository),
        "--smoke-cases-per-predicate",
        str(cases_per_predicate),
        "--chunk-size-base-cases",
        str(chunk_size),
        "--timeout-seconds",
        "60",
    ]
    if max_chunks is not None:
        result.extend(("--max-chunks", str(max_chunks)))
    if verify_only:
        result.append("--verify-only")
    return result


def tree_snapshot(root: Path) -> tuple[tuple[str, str, str], ...]:
    result: list[tuple[str, str, str]] = []
    for path in sorted(root.rglob("*")):
        relative = path.relative_to(root).as_posix()
        if path.is_symlink():
            result.append((relative, "symlink", os.readlink(path)))
        elif path.is_dir():
            result.append((relative, "directory", ""))
        elif path.is_file():
            result.append((relative, "file", hashlib.sha256(path.read_bytes()).hexdigest()))
        else:
            result.append((relative, "other", ""))
    return tuple(result)


def expect_value_error(call: Any, diagnostic: str) -> None:
    try:
        call()
    except ValueError as error:
        if diagnostic not in str(error):
            fail(f"guard lost diagnostic {diagnostic!r}: {error}")
    else:
        fail(f"guard accepted a mutation requiring {diagnostic!r}")


def audit_count_guards(module: Any) -> None:
    counts = module.empty_counts()
    counts["base_case_count"] = 3
    counts["metamorphic_sign_count"] = 3
    counts["metamorphisms_verified"] = 3
    counts["by_predicate"] = {predicate: 2 for predicate in module.PREDICATES}
    counts["by_sign"]["positive"] = 6
    counts["cpu_multiprecision_certified"] = 6
    counts["gpu_unknown_forwarded"] = 6
    counts["gpu_unknown_by_cpu_stage"]["cpu_multiprecision"] = 6
    counts["total_sign_count"] = 6
    for metamorphism in module.METAMORPHISMS[:3]:
        counts["by_metamorphism"][metamorphism] = 1
    counts["by_metamorphic_relation"]["same_sign"] = 2
    counts["by_metamorphic_relation"]["opposite_sign"] = 1
    module.validate_counts(counts, "guard baseline")

    for mutation, diagnostic in (
        (("exact_zeros",), "exact-zero counts"),
        (("cpu_multiprecision_certified",), "exact CPU path"),
        (("gpu_unknown_forwarded",), "GPU tri-state counts"),
        (
            ("gpu_unknown_by_cpu_stage", "cpu_multiprecision"),
            "GPU fallback-stage counts",
        ),
        (("by_metamorphism", module.METAMORPHISMS[0]), "metamorphism counts"),
        (
            ("by_metamorphic_relation", "same_sign"),
            "metamorphic relation counts",
        ),
    ):
        forged = json.loads(json.dumps(counts))
        if len(mutation) == 1:
            forged[mutation[0]] += 1
        else:
            forged[mutation[0]][mutation[1]] += 1
        expect_value_error(
            lambda forged=forged: module.validate_counts(forged, "forged counts"),
            diagnostic,
        )

    forged_zero = json.loads(json.dumps(counts))
    forged_zero["by_sign"]["positive"] = 0
    forged_zero["by_sign"]["zero"] = 6
    forged_zero["exact_zeros"] = 6
    forged_zero["gpu_fp64_certified"] = 6
    forged_zero["gpu_unknown_forwarded"] = 0
    forged_zero["gpu_unknown_by_cpu_stage"]["cpu_multiprecision"] = 0
    expect_value_error(
        lambda: module.validate_counts(forged_zero, "forged zero counts"),
        "GPU-known sign",
    )


def audit_strict_scalar_schemas(module: Any) -> None:
    case = SimpleNamespace(
        base_index=0,
        command=lambda: "compare_squared_distances 0 0 0 1 0 0 2 0 0",
        predicate="compare_squared_distances",
        relation="base",
    )
    boolean_counters = {
        "cpu_multiprecision_certified": True,
        "exact_zeros": False,
        "expansion_certified": False,
        "fp32_proposals": False,
        "fp64_filtered_certified": False,
        "remaining_unknown": False,
    }
    expect_value_error(
        lambda: module.validate_cpu_row(
            {
                "certification_stage": "cpu_multiprecision",
                "counters": boolean_counters,
                "predicate": "compare_squared_distances",
                "sign": "positive",
            },
            case,
            "phase2a",
            "positive",
        ),
        "must be an integer",
    )

    roots = {name: "a" * 64 for name in ("corpus", "oracle", "cpu", "gpu")}
    module.validate_roots(roots, "valid roots")
    for forged_root in ("+" + "a" * 63, "-" + "a" * 63, "A" * 64):
        forged = {**roots, "gpu": forged_root}
        expect_value_error(
            lambda forged=forged: module.validate_roots(forged, "forged roots"),
            "non-SHA256 root",
        )


def audit_aggregate_provenance(module: Any) -> None:
    roots = {
        "corpus": "1" * 64,
        "oracle": "2" * 64,
        "cpu": "3" * 64,
        "gpu": "4" * 64,
    }
    certificates = {
        name: (
            {
                "campaign": name,
                "identities": {"shared": True},
                "ordered_roots": {
                    **roots,
                    "corpus": ("1" if name == "phase2a" else "5") * 64,
                },
                "repository": {"git_head": "a" * 40},
                "scope": "smoke",
                "total_sign_count": 1,
            },
            f"{name}\n".encode("ascii"),
        )
        for name in module.CAMPAIGNS
    }
    module.aggregate_for(certificates, "smoke")
    for field, diagnostic in (
        ("identities", "runtime identities"),
        ("repository", "repository provenance"),
    ):
        forged = {
            name: (json.loads(json.dumps(value[0])), value[1])
            for name, value in certificates.items()
        }
        forged["phase2b"][0][field] = {"forged": True}
        expect_value_error(
            lambda forged=forged: module.aggregate_for(forged, "smoke"),
            diagnostic,
        )


def audit_power_guard(module: Any) -> None:
    base = {
        "predicate": "power_bisector_side",
        "points": (("0000000000000000",) * 3,),
        "r_ids": (0,),
        "q_ids": (0,),
    }
    for witness, diagnostic in (
        ((2**53 + 1, 0, 0, 1), "representable"),
        ((2, 0, 0, 2), "reduced"),
        ((0, 0, 0, 0), "representable"),
        ((True, 0, 0, 1), "representable"),
    ):
        try:
            module.validate_gpu_case(SimpleNamespace(**base, witness=witness))
        except ValueError as error:
            if diagnostic not in str(error):
                fail(f"power guard lost diagnostic {diagnostic!r}: {error}")
        else:
            fail(f"power guard accepted invalid witness {witness!r}")


def audit_isolated_phase2b_root_guard(module: Any, root: Path) -> None:
    base_count = module.EXPECTED_PRODUCTION_BASE_CASES
    metamorphic_count = module.EXPECTED_PRODUCTION_METAMORPHIC_SIGNS
    total = module.EXPECTED_PRODUCTION_TOTAL_SIGNS
    counts = module.empty_counts()
    counts["base_case_count"] = base_count
    counts["metamorphic_sign_count"] = metamorphic_count
    counts["metamorphisms_verified"] = metamorphic_count
    counts["by_predicate"] = {predicate: total // 3 for predicate in module.PREDICATES}
    counts["by_metamorphism"][module.METAMORPHISMS[0]] = metamorphic_count
    counts["by_metamorphic_relation"]["same_sign"] = metamorphic_count
    counts["by_sign"]["positive"] = total
    counts["cpu_multiprecision_certified"] = total
    counts["gpu_unknown_forwarded"] = total
    counts["gpu_unknown_by_cpu_stage"]["cpu_multiprecision"] = total
    counts["total_sign_count"] = total
    frozen_roots = {"corpus": "1" * 64, "oracle": "2" * 64}
    manifest = {
        "base_case_count": base_count,
        "campaign": "phase2b",
        "counts": counts,
        "identities": {},
        "ordered_roots": {
            "corpus": frozen_roots["corpus"],
            "oracle": "3" * 64,
            "cpu": "4" * 64,
            "gpu": "5" * 64,
        },
        "repository": {},
        "runtime_milliseconds": 0,
        "scope": "production",
        "seed": "4257325f47505532",
        "state": "complete",
    }
    certificate_dir = root / "isolated-root-certificate"
    certificate_dir.mkdir()
    try:
        module.publish_certificate(certificate_dir, manifest, frozen_roots)
    except ValueError as error:
        if "frozen Phase 2A root" not in str(error):
            fail(f"isolated Phase 2B root guard lost its diagnostic: {error}")
    else:
        fail("an isolated Phase 2B certificate reused the frozen Phase 2A root")

    truncated = json.loads(json.dumps(manifest))
    truncated_counts = truncated["counts"]
    truncated_counts["metamorphic_sign_count"] = 0
    truncated_counts["metamorphisms_verified"] = 0
    truncated_counts["by_metamorphism"] = {
        name: 0 for name in module.METAMORPHISMS
    }
    truncated_counts["by_metamorphic_relation"] = {
        name: 0 for name in module.METAMORPHIC_RELATIONS
    }
    truncated_counts["by_predicate"] = {
        "compare_squared_distances": base_count // 3,
        "orientation_3d": base_count // 3,
        "power_bisector_side": base_count // 3,
    }
    truncated_counts["by_sign"]["positive"] = base_count
    truncated_counts["cpu_multiprecision_certified"] = base_count
    truncated_counts["gpu_unknown_forwarded"] = base_count
    truncated_counts["gpu_unknown_by_cpu_stage"]["cpu_multiprecision"] = base_count
    truncated_counts["total_sign_count"] = base_count
    expect_value_error(
        lambda: module.certificate_for(truncated, frozen_roots),
        "frozen base and metamorphic scale",
    )


def load_tool(path: Path) -> Any:
    import importlib.util

    spec = importlib.util.spec_from_file_location("phase2b_campaign_smoke_module", path)
    if spec is None or spec.loader is None:
        fail("campaign module could not be loaded")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaign_tool", type=Path)
    parser.add_argument("cpu_replay", type=Path)
    parser.add_argument(
        "--source", type=Path, default=Path(__file__).resolve().parents[3]
    )
    options = parser.parse_args(arguments)
    for path, label in (
        (options.campaign_tool, "campaign tool"),
        (options.cpu_replay, "CPU replay"),
    ):
        if not path.is_file():
            parser.error(f"{label} does not exist: {path}")

    try:
        module = load_tool(options.campaign_tool.resolve())
        audit_power_guard(module)
        audit_count_guards(module)
        audit_strict_scalar_schemas(module)
        audit_aggregate_provenance(module)
        with tempfile.TemporaryDirectory(prefix="morsehgp3d-phase2b-campaign-") as root_name:
            root = Path(root_name)
            audit_isolated_phase2b_root_guard(module, root)
            repository = root / "repository"
            repository.mkdir()
            make_repository(options.source.resolve(), repository)
            fake_gpu = root / "fake_gpu.py"
            fake_gpu.write_text(FAKE_GPU, encoding="utf-8")
            fake_gpu.chmod(fake_gpu.stat().st_mode | stat.S_IXUSR)
            environment = {
                **os.environ,
                "MORSEHGP3D_FAKE_GPU_BAD_JSON": "",
                "MORSEHGP3D_FAKE_CPU": str(options.cpu_replay.resolve()),
                "PYTHONDONTWRITEBYTECODE": "1",
            }
            locked_checkpoint = root / "locked-checkpoint"
            lock_path = locked_checkpoint.parent / f".{locked_checkpoint.name}.lock"
            lock_descriptor = os.open(lock_path, os.O_CREAT | os.O_RDWR, 0o600)
            try:
                fcntl.flock(lock_descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
                locked_run = run(
                    campaign_command(
                        options.campaign_tool.resolve(),
                        fake_gpu,
                        options.cpu_replay.resolve(),
                        locked_checkpoint,
                        repository,
                        campaign="phase2b",
                        cases_per_predicate=20,
                        chunk_size=60,
                    ),
                    environment=environment,
                    expected_status=1,
                )
                if "another writer holds the POSIX campaign lock" not in locked_run.stderr:
                    fail("a concurrent writer did not encounter the campaign lock")
            finally:
                os.close(lock_descriptor)

            checkpoint = root / "checkpoint"
            partial = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    checkpoint,
                    repository,
                    max_chunks=1,
                ),
                environment=environment,
            )
            partial_value = json.loads(partial.stdout)
            if partial_value.get("state") != "running" or partial_value.get(
                "phase_gate_closed"
            ) is not False:
                fail("the partial campaign claimed completion or a phase gate")
            incomplete_verification = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )
            if "read-only verification requires a complete campaign shard" not in (
                incomplete_verification.stderr
            ):
                fail("read-only verification accepted an incomplete checkpoint")

            completed = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    checkpoint,
                    repository,
                ),
                environment=environment,
            )
            aggregate = json.loads(completed.stdout)
            if aggregate.get("total_sign_count") != 660:
                fail("the two smoke campaigns did not aggregate 660 signs")
            if set(aggregate.get("campaigns", {})) != {"phase2a", "phase2b"}:
                fail("the aggregate omitted a campaign certificate")
            if aggregate.get("phase_gate_closed") is not False or aggregate.get(
                "qualification_claimed"
            ) is not False:
                fail("the aggregate overclaimed Phase 2B qualification")
            certificates = {
                name: load_canonical(checkpoint / name / "result.json")
                for name in ("phase2a", "phase2b")
            }
            if any(value["total_sign_count"] != 330 for value in certificates.values()):
                fail("a smoke shard did not contain 300 base and 30 metamorphic signs")
            if any(value["counts"]["wrong_sign"] for value in certificates.values()):
                fail("a smoke certificate retained a contradiction")
            if any(
                value["counts"]["metamorphisms_verified"]
                != value["counts"]["metamorphic_sign_count"]
                for value in certificates.values()
            ):
                fail("a smoke certificate omitted a metamorphic relation check")
            for value in certificates.values():
                counts = value["counts"]
                if (
                    sum(counts["by_metamorphism"].values())
                    != counts["metamorphic_sign_count"]
                    or sum(counts["by_metamorphic_relation"].values())
                    != counts["metamorphic_sign_count"]
                    or counts["exact_zeros"] != counts["by_sign"]["zero"]
                    or sum(counts["gpu_unknown_by_cpu_stage"].values())
                    != counts["gpu_unknown_forwarded"]
                ):
                    fail("a smoke certificate lost detailed counter closure")
            if certificates["phase2a"]["ordered_roots"]["corpus"] == certificates[
                "phase2b"
            ]["ordered_roots"]["corpus"]:
                fail("the additional seed did not produce a distinct corpus")

            before_verification = tree_snapshot(checkpoint)
            verified = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
            )
            if json.loads(verified.stdout) != aggregate:
                fail("read-only verification did not return the committed aggregate")
            if tree_snapshot(checkpoint) != before_verification:
                fail("read-only verification modified the checkpoint copy")

            mutated_gpu = root / "mutated-fake-gpu.py"
            shutil.copy2(fake_gpu, mutated_gpu)
            with mutated_gpu.open("a", encoding="utf-8") as stream:
                stream.write("\n# executable identity mutation\n")
            executable_mutation = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    mutated_gpu,
                    options.cpu_replay.resolve(),
                    checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )
            if "campaign checkpoint mismatch: identities" not in (
                executable_mutation.stderr
            ):
                fail("verification accepted a mutated GPU executable")

            for campaign_name in ("phase2a", "phase2b"):
                single_checkpoint = root / f"single-{campaign_name}-checkpoint"
                single_run = run(
                    campaign_command(
                        options.campaign_tool.resolve(),
                        fake_gpu,
                        options.cpu_replay.resolve(),
                        single_checkpoint,
                        repository,
                        campaign=campaign_name,
                        cases_per_predicate=20,
                        chunk_size=60,
                    ),
                    environment=environment,
                )
                single_aggregate = json.loads(single_run.stdout)
                if (
                    single_aggregate.get("additional_campaign_basis")
                    != "single_campaign_only"
                    or set(single_aggregate.get("campaigns", {})) != {campaign_name}
                ):
                    fail("a single-campaign aggregate claimed two-corpus evidence")
                single_before = tree_snapshot(single_checkpoint)
                single_verified = run(
                    campaign_command(
                        options.campaign_tool.resolve(),
                        fake_gpu,
                        options.cpu_replay.resolve(),
                        single_checkpoint,
                        repository,
                        campaign=campaign_name,
                        cases_per_predicate=20,
                        chunk_size=60,
                        verify_only=True,
                    ),
                    environment=environment,
                )
                if json.loads(single_verified.stdout) != single_aggregate:
                    fail("single-campaign verification returned a different aggregate")
                if tree_snapshot(single_checkpoint) != single_before:
                    fail("single-campaign verification modified its checkpoint")
                single_root = single_checkpoint / "result.json"
                forged_single = load_canonical(single_root)
                forged_single["total_sign_count"] += 1
                single_root.write_text(
                    canonical_json(forged_single) + "\n", encoding="ascii"
                )
                run(
                    campaign_command(
                        options.campaign_tool.resolve(),
                        fake_gpu,
                        options.cpu_replay.resolve(),
                        single_checkpoint,
                        repository,
                        campaign=campaign_name,
                        cases_per_predicate=20,
                        chunk_size=60,
                        verify_only=True,
                    ),
                    environment=environment,
                    expected_status=1,
                )

            known_checkpoint = root / "known-checkpoint"
            known_environment = {
                **environment,
                "MORSEHGP3D_FAKE_GPU_KNOWN": "1",
                "MORSEHGP3D_FAKE_GPU_UNKNOWN_STAGE": "cycle",
            }
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    known_checkpoint,
                    repository,
                    campaign="phase2b",
                    cases_per_predicate=20,
                    chunk_size=60,
                ),
                environment=known_environment,
            )
            known_certificate = load_canonical(
                known_checkpoint / "phase2b" / "result.json"
            )
            if (
                known_certificate["counts"]["gpu_fp64_certified"] == 0
                or known_certificate["counts"]["gpu_unknown_forwarded"] == 0
                or any(
                    value == 0
                    for value in known_certificate["counts"][
                        "gpu_unknown_by_cpu_stage"
                    ].values()
                )
            ):
                fail("the smoke did not cover GPU-known and every fallback stage")

            for json_mode, diagnostic in (
                ("noncanonical", "emitted non-canonical JSON"),
                ("duplicate", "duplicate JSON key: sign"),
            ):
                malformed_run = run(
                    campaign_command(
                        options.campaign_tool.resolve(),
                        fake_gpu,
                        options.cpu_replay.resolve(),
                        root / f"{json_mode}-gpu-json-checkpoint",
                        repository,
                        campaign="phase2b",
                        cases_per_predicate=20,
                        chunk_size=60,
                    ),
                    environment={
                        **environment,
                        "MORSEHGP3D_FAKE_GPU_BAD_JSON": json_mode,
                    },
                    expected_status=1,
                )
                if diagnostic not in malformed_run.stderr:
                    fail(f"the GPU {json_mode} JSON guard lost its diagnostic")

            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    root / "invalid-chunk-checkpoint",
                    repository,
                    campaign="phase2b",
                    chunk_size=7,
                ),
                environment=environment,
                expected_status=1,
            )

            orphan_checkpoint = root / "orphan-checkpoint"
            orphan_chunk = orphan_checkpoint / "phase2a/chunks/chunk-00000000.json"
            orphan_chunk.parent.mkdir(parents=True)
            orphan_chunk.write_text("orphan\n", encoding="ascii")
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    orphan_checkpoint,
                    repository,
                    max_chunks=1,
                    cases_per_predicate=20,
                    chunk_size=60,
                ),
                environment=environment,
                expected_status=1,
            )

            bounded_checkpoint = root / "bounded-checkpoint"
            bounded_run = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    bounded_checkpoint,
                    repository,
                    max_chunks=1,
                    cases_per_predicate=20,
                    chunk_size=60,
                ),
                environment=environment,
            )
            bounded_value = json.loads(bounded_run.stdout)
            if bounded_value.get("campaigns_complete") != ["phase2a"] or (
                bounded_checkpoint / "phase2b"
            ).exists():
                fail("the transaction runner exceeded the exact --max-chunks bound")

            referenced_orphan = root / "referenced-orphan-checkpoint"
            shutil.copytree(checkpoint, referenced_orphan)
            shutil.copy2(
                referenced_orphan / "phase2a/chunks/chunk-00000000.json",
                referenced_orphan / "phase2a/chunks/chunk-99999999.json",
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    referenced_orphan,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )

            temporary_file_checkpoint = root / "temporary-file-checkpoint"
            shutil.copytree(checkpoint, temporary_file_checkpoint)
            (temporary_file_checkpoint / "phase2a/.checkpoint.json.stale.tmp").write_text(
                "stale\n", encoding="ascii"
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    temporary_file_checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )

            temporary_directory_checkpoint = root / "temporary-directory-checkpoint"
            shutil.copytree(checkpoint, temporary_directory_checkpoint)
            (
                temporary_directory_checkpoint
                / "phase2a/chunks/.phase2b-chunk-stale"
            ).mkdir()
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    temporary_directory_checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )

            forged_certificate_checkpoint = root / "forged-certificate-checkpoint"
            shutil.copytree(checkpoint, forged_certificate_checkpoint)
            forged_certificate_path = (
                forged_certificate_checkpoint / "phase2a/result.json"
            )
            forged_certificate = load_canonical(forged_certificate_path)
            forged_certificate["total_sign_count"] += 1
            forged_certificate_path.write_text(
                canonical_json(forged_certificate) + "\n", encoding="ascii"
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    forged_certificate_checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )

            forged_aggregate_checkpoint = root / "forged-aggregate-checkpoint"
            shutil.copytree(checkpoint, forged_aggregate_checkpoint)
            forged_aggregate_path = forged_aggregate_checkpoint / "result.json"
            forged_aggregate = load_canonical(forged_aggregate_path)
            forged_aggregate["total_sign_count"] += 1
            forged_aggregate_path.write_text(
                canonical_json(forged_aggregate) + "\n", encoding="ascii"
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    forged_aggregate_checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )

            tampered_checkpoint = root / "tampered-checkpoint"
            shutil.copytree(checkpoint, tampered_checkpoint)
            tampered_chunk = tampered_checkpoint / "phase2a/chunks/chunk-00000000.json"
            tampered_chunk.write_bytes(tampered_chunk.read_bytes() + b" ")
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    tampered_checkpoint,
                    repository,
                ),
                environment=environment,
                expected_status=1,
            )

            divergent_environment = {
                **environment,
                "MORSEHGP3D_FAKE_FLIP_FIRST": "1",
            }
            failed_checkpoint = root / "failed-checkpoint"
            divergent_run = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    failed_checkpoint,
                    repository,
                    campaign="phase2b",
                ),
                environment=divergent_environment,
                expected_status=1,
            )
            for replay_field in (
                '"base_index":',
                '"campaign":"phase2b"',
                '"expected_sign":',
                '"actual_sign":',
                '"predicate":',
                '"relation":',
                '"replay_command":',
                '"replay_id":',
            ):
                if replay_field not in divergent_run.stderr:
                    fail(
                        "a GPU contradiction lost its permanent-fixture replay field: "
                        + replay_field
                    )
            failed_manifest = load_canonical(
                failed_checkpoint / "phase2b" / "checkpoint.json"
            )
            if failed_manifest["next_base_index"] != 0 or failed_manifest["chunks"]:
                fail("a divergent transaction advanced its checkpoint")

            forged_complete = root / "forged-complete-checkpoint"
            shutil.copytree(failed_checkpoint, forged_complete)
            forged_manifest_path = forged_complete / "phase2b/checkpoint.json"
            forged_manifest = load_canonical(forged_manifest_path)
            forged_manifest["state"] = "complete"
            forged_manifest_path.write_text(
                canonical_json(forged_manifest) + "\n", encoding="ascii"
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    forged_complete,
                    repository,
                    campaign="phase2b",
                ),
                environment=environment,
                expected_status=1,
            )

            forged_roots = root / "forged-roots-checkpoint"
            shutil.copytree(failed_checkpoint, forged_roots)
            forged_roots_path = forged_roots / "phase2b/checkpoint.json"
            forged_roots_manifest = load_canonical(forged_roots_path)
            forged_roots_manifest["ordered_roots"]["gpu"] = "0" * 64
            forged_roots_path.write_text(
                canonical_json(forged_roots_manifest) + "\n", encoding="ascii"
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    forged_roots,
                    repository,
                    campaign="phase2b",
                ),
                environment=environment,
                expected_status=1,
            )

            forged_runtime = root / "forged-runtime-checkpoint"
            shutil.copytree(failed_checkpoint, forged_runtime)
            forged_runtime_path = forged_runtime / "phase2b/checkpoint.json"
            forged_runtime_manifest = load_canonical(forged_runtime_path)
            forged_runtime_manifest["runtime_milliseconds"] = "invalid"
            forged_runtime_path.write_text(
                canonical_json(forged_runtime_manifest) + "\n", encoding="ascii"
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    forged_runtime,
                    repository,
                    campaign="phase2b",
                ),
                environment=environment,
                expected_status=1,
            )

            forged_algorithm = root / "forged-algorithm-checkpoint"
            shutil.copytree(failed_checkpoint, forged_algorithm)
            forged_algorithm_path = forged_algorithm / "phase2b/checkpoint.json"
            forged_algorithm_manifest = load_canonical(forged_algorithm_path)
            forged_algorithm_manifest["ordered_root_algorithm"] = "sha256-unknown"
            forged_algorithm_path.write_text(
                canonical_json(forged_algorithm_manifest) + "\n", encoding="ascii"
            )
            run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    forged_algorithm,
                    repository,
                    campaign="phase2b",
                ),
                environment=environment,
                expected_status=1,
            )

            metamorphic_repository = root / "metamorphic-repository"
            metamorphic_repository.mkdir()
            make_repository(options.source.resolve(), metamorphic_repository)
            metamorphic_generator = (
                metamorphic_repository / "morsehgp3d/tools/predicate_campaign.py"
            )
            with metamorphic_generator.open("a", encoding="utf-8") as stream:
                stream.write(
                    "\ndef validate_metamorphic_sign(*_args, **_kwargs):\n"
                    "    raise ValueError('metamorphic relation sentinel')\n"
                )
            git(metamorphic_repository, "add", ".")
            git(
                metamorphic_repository,
                "commit",
                "--quiet",
                "-m",
                "inject metamorphic sentinel",
            )
            metamorphic_run = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    root / "metamorphic-checkpoint",
                    metamorphic_repository,
                    campaign="phase2b",
                    cases_per_predicate=20,
                    chunk_size=60,
                ),
                environment=environment,
                expected_status=1,
            )
            for replay_field in (
                '"base_index":',
                '"base_replay_command":',
                '"base_sign":',
                '"campaign":"phase2b"',
                '"cause":"metamorphic relation sentinel"',
                '"predicate":',
                '"relation":',
                '"replay_command":',
                '"replay_id":',
                '"transformed_sign":',
            ):
                if replay_field not in metamorphic_run.stderr:
                    fail(
                        "a metamorphic contradiction lost its fixture replay field: "
                        + replay_field
                    )

            repository_mutation = repository / "provenance-mutation.txt"
            repository_mutation.write_text(
                "committed after campaign completion\n", encoding="ascii"
            )
            git(repository, "add", repository_mutation.name)
            git(
                repository,
                "commit",
                "--quiet",
                "-m",
                "mutate repository provenance",
            )
            repository_verification = run(
                campaign_command(
                    options.campaign_tool.resolve(),
                    fake_gpu,
                    options.cpu_replay.resolve(),
                    checkpoint,
                    repository,
                    verify_only=True,
                ),
                environment=environment,
                expected_status=1,
            )
            if "campaign checkpoint mismatch: repository" not in (
                repository_verification.stderr
            ):
                fail("verification accepted changed repository provenance")
    except (AssertionError, OSError, subprocess.TimeoutExpired, ValueError) as error:
        print(f"Phase 2B campaign smoke failed: {error}", file=sys.stderr)
        return 1

    print("Phase 2B campaign smoke passed: 660 signs, resume, roots and fail-closed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
