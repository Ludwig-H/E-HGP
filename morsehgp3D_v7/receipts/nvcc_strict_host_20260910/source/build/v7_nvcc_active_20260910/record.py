#!/usr/bin/env python3
"""Portable strict-NVCC compile evidence. CUDA binaries are never executed."""
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import shutil
import subprocess
import time


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    repo = root.parents[1]
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    packet = output / "packet"
    source = packet / "source"
    source.mkdir(parents=True)
    streams = packet / "streams"
    streams.mkdir()
    commands = []
    before = {}
    binaries = {}
    receipt = dict(status="running", claim="local_NVCC_compile_link_only", device_executed=False,
                   GCP="not_used_by_this_agent", public_status="not_claimed", commands=commands, binaries=binaries)

    def save() -> None:
        (packet / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")

    def run(name: str, argv: list[str], expected: int = 0) -> None:
        start = time.monotonic()
        with (streams / (name + ".stdout")).open("xb") as out, (streams / (name + ".stderr")).open("xb") as err:
            result = subprocess.run(argv, cwd=repo, stdout=out, stderr=err, check=False)
        row = dict(name=name, argv=argv, returncode=result.returncode, expected=expected, wall_s=time.monotonic() - start)
        for stream in ("stdout", "stderr"):
            path = streams / (name + "." + stream)
            row[stream] = dict(path=str(path.relative_to(packet)), bytes=path.stat().st_size, sha256=sha(path))
        commands.append(row)
        save()
        print(name, result.returncode, f'{row["wall_s"]:.3f}s', "expected", expected, flush=True)
        if result.returncode != expected:
            raise RuntimeError(f"{name}: code {result.returncode}, expected {expected}")

    try:
        run("head", ["git", "rev-parse", "HEAD"])
        run("worktree", ["git", "status", "--porcelain=v1", "--untracked-files=no"])
        run("gxx_version", ["/usr/bin/g++", "--version"])
        toolkit = repo / "build/v7_nvcc_pedantic_20260910/toolkit"
        nvcc = toolkit / "bin/nvcc"
        run("nvcc_version", [str(nvcc), "--version"])
        active_helper = repo / "morsehgp3D_v7/bench/nvcc_strict_host.py"
        helper = output / "nvcc_strict_host.py"
        shutil.copy2(active_helper, helper)
        helper.chmod(0o700)
        flags = ["-O3", "-DNDEBUG", "-std=c++20", "-arch=sm_120", "-fmad=false", "--expt-relaxed-constexpr",
                 "-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror,-pthread", "-ccbin", str(helper), "-L" + str(toolkit / "lib")]
        gate = Path("morsehgp3D_v7/tests/census_route_device_gate.cu")
        probe = Path("morsehgp3D_v7/bench/full_ball_tower_probe.cpp")
        plans = [("gate", gate, []), ("probe", probe, ["-DMHGP7_FULL_BALL_CUDA=1", "-x", "cu"])]
        paths = {Path(__file__).resolve(), root / "verify.py", active_helper}
        fixture_base = repo / "build/v7_nvcc_pedantic_20260910"
        fixture_names = ("strict_minimal.cu", "strict_bad.cu", "strict_bad_pp.cu", "fixture.cudafe1.cpp", "strict_include.hpp")
        paths.update(fixture_base / name for name in fixture_names)
        paths.add(fixture_base / "redistrib_12.9.1.json")
        for name, path, extra in plans:
            run("dependencies_" + name, [str(nvcc), *flags, *extra, "-MM", "-MT", "target", str(repo / path)])
            text = (streams / ("dependencies_" + name + ".stdout")).read_text().replace("\\\n", " ")
            for item in shlex.split(text.split(":", 1)[1]):
                resolved = Path(item).resolve()
                if resolved.is_relative_to(repo / "morsehgp3D_v7"):
                    paths.add(resolved)
        for path in sorted(paths):
            relative = path.relative_to(repo)
            before[str(relative)] = sha(path)
            target = source / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
            if sha(target) != before[str(relative)]:
                raise RuntimeError("snapshot mismatch")
        receipt.update(sources_before=before, nvcc_sha256=sha(nvcc), gxx_sha256=sha(Path("/usr/bin/g++").resolve()),
                       helper_sha256=sha(helper), toolkit_metadata="source/build/v7_nvcc_pedantic_20260910/redistrib_12.9.1.json")
        if sha(helper) != before[str(active_helper.relative_to(repo))]:
            raise RuntimeError("helper executable not pinned")
        frozen_fixtures = source / fixture_base.relative_to(repo)
        common = [str(nvcc), *flags]
        for name in ("strict_bad.cu", "strict_bad_pp.cu"):
            run("reject_" + name, [*common, "-c", str(frozen_fixtures / name), "-o", str(output / (name + ".o"))], 1)
        run("minimal_compile_link", [*common, str(frozen_fixtures / "strict_minimal.cu"), "-o", str(output / "minimal_cuda")])
        binaries["minimal_cuda"] = sha(output / "minimal_cuda")
        generated = str(frozen_fixtures / "fixture.cudafe1.cpp")
        run("include_define_compile", [str(helper), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-DMHGP7_COMMAND_DEFINE=7", "-c", "-x", "c++", generated, "-o", str(output / "include_define.o")])
        run("include_define_link", ["/usr/bin/g++", str(output / "include_define.o"), "-o", str(output / "include_define")])
        run("include_define_host_run", [str(output / "include_define")])
        run("reject_unknown_phase", [str(helper), "-E", "-x", "c++", generated, "-o", str(output / "unknown.ii")], 2)
        run("reject_truncated_output", [str(helper), "-c", generated, "-o"], 2)
        run("reject_truncated_language", [str(helper), "-c", generated, "-o", str(output / "truncated.o"), "-x"], 2)
        for name, path, extra in plans:
            binary = output / name
            run("compile_link_" + name, [*common, *extra, "-MMD", "-MF", str(output / (name + ".d")),
                                        str(source / path), "-o", str(binary)])
            binaries[name] = sha(binary)
        receipt["status"] = "completed"
    except BaseException as error:
        receipt.update(status="failed", error=type(error).__name__ + ": " + str(error))
    finally:
        after = {path: sha(repo / path) for path in before}
        receipt.update(sources_after=after, sources_stable=before == after,
                       snapshot_stable=all(sha(source / path) == digest for path, digest in before.items()),
                       binaries_after={name: sha(output / name) for name in binaries})
        if not receipt["sources_stable"] or not receipt["snapshot_stable"] or binaries != receipt["binaries_after"]:
            receipt.update(status="failed", stability_error="source or binary changed")
        save()
        shutil.copy2(root / "verify.py", packet / "verify.py")
        manifest = {str(path.relative_to(packet)): dict(bytes=path.stat().st_size, sha256=sha(path))
                    for path in sorted(packet.rglob("*")) if path.is_file()}
        (packet / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return 0 if receipt["status"] == "completed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
