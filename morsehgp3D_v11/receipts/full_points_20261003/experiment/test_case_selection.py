#!/usr/bin/env python3
"""Pure AST runner regression: whole-scene selection, no numpy/fit/native/GCP."""
import argparse
import ast
from contextlib import redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
CHECKS = 0


def check(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise RuntimeError(message)


def rejected(action, message):
    try:
        action()
    except ValueError:
        check(True, message)
    else:
        check(False, message)


class Mask(list):
    def sum(self):
        return sum(self)


class Array(list):
    def __eq__(self, scalar):
        return Mask(value == scalar for value in self)

    def copy(self):
        return Array(self)

    def __setitem__(self, key, value):
        if isinstance(key, Mask):
            for i, selected in enumerate(key):
                if selected:
                    super().__setitem__(i, value)
        else:
            super().__setitem__(key, value)

    def reshape(self, rows, columns):
        check(len(self) == rows * columns, "mock reshape retains every input element")
        return [Array(self[i*columns:(i+1)*columns]) for i in range(rows)]


class XYZ(list):
    def __add__(self, shift):
        return XYZ(tuple(value + shift for value in point) for point in self)


class PureArrays:
    def __init__(self):
        self.reads = []

    def fromfile(self, path, dtype):
        self.reads.append(Path(path).name)
        data = Path(path).read_bytes()
        check(len(data) % 4 == 0 and dtype in ("<u4", "<i4"), "exact full binary words")
        return Array(struct.unpack("<" + ("I" if dtype == "<u4" else "i") * (len(data)//4), data))

    def arange(self, count, dtype):
        check(dtype == "<u4", "synthetic ID type unchanged")
        return list(range(count))


def main():
    source_path = HERE / "run_experiment.py"
    source = source_path.read_text()
    module = ast.parse(source, filename=str(source_path))
    names = {"need", "select_cases", "digest", "save", "main"}
    bodies = [node for node in module.body if isinstance(node, ast.FunctionDef) and node.name in names]
    check(len(bodies) == len(names), "actual runner implementations extracted")
    arrays, calls, gate_calls, generated = PureArrays(), [], [], []
    environment = dict(argparse=argparse, hashlib=hashlib, json=json, Path=Path, np=arrays, HERE=HERE)
    environment["read_gate"] = lambda probe, gate: gate_calls.append((probe, gate)) or {"mock_only": True}

    def one_case(args, name, xyz, ids, labels, metadata, config):
        check(len(xyz) == len(ids) == len(labels), "whole selected scene reaches one_case")
        calls.append(dict(name=name, xyz=[tuple(point) for point in xyz], ids=list(ids),
                          labels=list(labels), metadata=metadata))

    environment["one_case"] = one_case
    exec(compile(ast.Module(body=bodies, type_ignores=[]), str(source_path), "exec"), environment)
    select = environment["select_cases"]
    available = ["a", "b", "c"]
    check(select(available, None) == available, "default all unchanged")
    check(select(available, ["c", "a"]) == ["a", "c"], "manifest order retained")
    for wanted in ([], [""], [" "], [" a"], ["a", "a"], ["unknown"], ["a", "unknown"]):
        rejected(lambda: select(available, wanted), "invalid filter refused")
    rejected(lambda: select([], None), "empty inventory refused")
    rejected(lambda: select(["a", "a"], None), "duplicate inventory refused")

    with tempfile.TemporaryDirectory(prefix="ehgp-cases-pure-") as temporary:
        base = Path(temporary)
        data = base / "data"
        data.mkdir()
        scenes, expected = [], {}
        for index, name in enumerate(("01_a", "02_b", "03_c"), start=3):
            points = [(i, 2*i, 3*i) for i in range(index)]
            ids = [100+i for i in range(index)]
            labels = [i % 3 for i in range(index)]
            files = []
            for role, words in (("sites", [v for point in points for v in point]),
                                ("ids", ids), ("truth", [0]*index + labels)):
                filename = name + "_" + role + ".u32le"
                payload = struct.pack("<" + "I"*len(words), *words)
                (data / filename).write_bytes(payload)
                files.append(dict(role=role, name=filename, bytes=len(payload),
                                  sha256=hashlib.sha256(payload).hexdigest()))
            scene = dict(name=name, n=index, files=files,
                         targets=[dict(sites=labels.count(i)) for i in range(3)])
            scenes.append(scene)
            expected["zoltan_"+name] = (points, ids, labels)
        manifest = data / "manifest.json"
        manifest.write_text(json.dumps(dict(scenes=scenes)))
        config = dict(synthetic=[dict(family="toy", seed=1), dict(family="toy", seed=2)],
                      generator_source="mock_only",
                      zoltan_manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest())
        config_path = base / "campaign.json"
        config_path.write_text(json.dumps(config))
        environment["CONFIG"] = config_path
        config_hash = hashlib.sha256(config_path.read_bytes()).hexdigest()
        run_number = 0

        def run(mode, requested):
            nonlocal run_number
            run_number += 1
            directory = base / str(run_number)
            args = ["runner", "--probe", "mock-only", "--gate", "unused", "--mode", mode,
                    "--work", str(directory/"work"), "--out", str(directory/"out"), "--data", str(data)]
            if requested is not None:
                for name in requested:
                    args.extend(("--case", name))
            calls.clear(); arrays.reads.clear(); generated.clear(); gate_calls.clear()
            with patch.object(sys, "argv", args), redirect_stdout(io.StringIO()):
                environment["main"]()
            selection = json.loads((directory/"out"/"selection.json").read_text())
            check(selection["config_sha256"] == config_hash, "global config pin unchanged")
            check(json.loads((directory/"out"/"campaign.json").read_text()) == config,
                  "full campaign copied unchanged")
            check(selection["requested_cases"] == requested, "requested scope recorded exactly")
            check(selection["selected_cases"] == [call["name"] for call in calls], "executed scope recorded")
            return selection

        selection = run("zoltan", None)
        check(selection["available_cases"] == list(expected), "complete manifest inventory recorded")
        check(selection["input_manifest_sha256"] == config["zoltan_manifest_sha256"], "global manifest pin retained")
        check(len(arrays.reads) == 9, "default reads all three whole scenes")
        for call in calls:
            points, ids, labels = expected[call["name"]]
            check(call["xyz"] == points and call["ids"] == ids and
                  list(call["labels"]) == labels, "all scene points, IDs and labels preserved")
        run("zoltan", ["zoltan_03_c", "zoltan_01_a"])
        check([call["name"] for call in calls] == ["zoltan_01_a", "zoltan_03_c"], "selection before one_case")
        check(len(arrays.reads) == 6 and not any(name.startswith("02_b") for name in arrays.reads),
              "unselected payloads not loaded")
        run("zoltan", ["zoltan_02_b"])
        check(len(calls) == 1 and len(calls[0]["ids"]) == 4, "single whole scene retained")
        for wanted in (["unknown"], ["zoltan_02_b", "zoltan_02_b"], [""]):
            rejected(lambda: run("zoltan", wanted), "CLI filter refused before execution")
            check(not calls and not arrays.reads and not gate_calls, "invalid CLI performs no scene or gate work")

        def generate(spec):
            generated.append(spec["seed"])
            return [(0, 1, 2), (3, 4, 5), (6, 7, 8)], [0, 1, 2], {}

        def quantize(points, millimetre, bits):
            check(millimetre == 0.001 and bits == 21, "synthetic quantization unchanged")
            return XYZ(points), "1/1000"

        with patch.dict(sys.modules, vendor_scenes=SimpleNamespace(generate=generate, quantize=quantize)):
            run("synthetic", ["toy_2"])
            check(generated == [2] and calls[0]["xyz"] == [(2, 3, 4), (5, 6, 7), (8, 9, 10)] and
                  calls[0]["ids"] == [0, 1, 2], "selection before generation; original full translation/IDs")
            run("synthetic", None)
            check(generated == [1, 2], "synthetic default all unchanged")
            rejected(lambda: run("synthetic", ["unknown"]), "unknown synthetic scene refused")
            check(not generated and not calls, "unknown synthetic scene never generated")
        check(hashlib.sha256(config_path.read_bytes()).hexdigest() == config_hash and
              hashlib.sha256(manifest.read_bytes()).hexdigest() == config["zoltan_manifest_sha256"],
              "global fixture config and manifest remained unchanged")
    check(source_path.read_text() == source, "source stable during pure regression")
    print(json.dumps(dict(status="pass", checks=CHECKS,
                          scope="actual runner AST with pure array/generator/case mocks; no native, fit or GCP"),
                     sort_keys=True))


if __name__ == "__main__":
    main()
