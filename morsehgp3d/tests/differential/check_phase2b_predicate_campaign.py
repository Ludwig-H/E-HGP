#!/usr/bin/env python3
"""Smoke-test Phase 2B GPU campaign transactions with an exact fake runner."""

from __future__ import annotations

import argparse
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
known_count = 0
unknown_count = 0
unknown_zeros = 0
for index, ((replay, command, predicate), exact) in enumerate(zip(parsed, oracle, strict=True)):
    sign = exact["sign"]
    if flip and index == 0:
        sign = "negative" if sign != "negative" else "positive"
    known = emit_known and index % 2 == 0 and sign != "zero"
    known_count += known
    unknown_count += not known
    unknown_zeros += not known and sign == "zero"
    print(json.dumps({
        "certification_stage": "fp64_filtered" if known else "cpu_multiprecision",
        "gpu_filter_sign": sign if known else "unknown",
        "kind": "decision",
        "predicate": predicate,
        "replay_command": command,
        "replay_id": replay,
        "sign": sign,
    }, ensure_ascii=True, separators=(",", ":"), sort_keys=True))
count = len(parsed)
predicate = parsed[0][2]
print(json.dumps({
    "audit_gpu_signs": False,
    "counters": {
        "async_fallback_batches": 1 if unknown_count else 0,
        "cpu_expansion_certified": 0,
        "cpu_fp64_filtered_certified": 0,
        "cpu_multiprecision_certified": unknown_count,
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
    return result


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
    total = module.MINIMUM_ADDITIONAL_SIGNS
    counts = module.empty_counts()
    counts["base_case_count"] = total
    counts["by_predicate"] = {
        "compare_squared_distances": total // 3 + 1,
        "orientation_3d": total // 3,
        "power_bisector_side": total // 3,
    }
    counts["by_sign"]["positive"] = total
    counts["cpu_multiprecision_certified"] = total
    counts["gpu_unknown_forwarded"] = total
    counts["total_sign_count"] = total
    frozen_roots = {"corpus": "1" * 64, "oracle": "2" * 64}
    manifest = {
        "base_case_count": total,
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
                "MORSEHGP3D_FAKE_CPU": str(options.cpu_replay.resolve()),
                "PYTHONDONTWRITEBYTECODE": "1",
            }
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
            if certificates["phase2a"]["ordered_roots"]["corpus"] == certificates[
                "phase2b"
            ]["ordered_roots"]["corpus"]:
                fail("the additional seed did not produce a distinct corpus")

            known_checkpoint = root / "known-checkpoint"
            known_environment = {**environment, "MORSEHGP3D_FAKE_GPU_KNOWN": "1"}
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
            ):
                fail("the smoke did not cover both GPU-known and GPU-unknown counters")

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
            orphan_run = run(
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
            )
            orphan_value = json.loads(orphan_run.stdout)
            if orphan_value.get("campaigns_complete") != ["phase2a"] or (
                orphan_checkpoint / "phase2b"
            ).exists():
                fail("an orphan chunk bypassed the exact --max-chunks bound")

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
            run(
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
            run(
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
    except (AssertionError, OSError, subprocess.TimeoutExpired, ValueError) as error:
        print(f"Phase 2B campaign smoke failed: {error}", file=sys.stderr)
        return 1

    print("Phase 2B campaign smoke passed: 660 signs, resume, roots and fail-closed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
