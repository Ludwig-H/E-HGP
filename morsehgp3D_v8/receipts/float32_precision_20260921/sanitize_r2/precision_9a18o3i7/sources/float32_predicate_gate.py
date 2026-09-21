#!/usr/bin/env python3
"""Independent Fraction oracle for the exact binary32 q2 primitive, not FULL."""
from __future__ import annotations

import argparse
from copy import deepcopy
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
FIELDS = ("queries", "exact_queries", "filter_queries", "filter_accepts", "filter_fallbacks",
          "exact_terms", "filter_axis_products", "filter_interval_additions")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def value(word):
    return Fraction.from_float(struct.unpack("<f", struct.pack("<I", word))[0])


def sign(words):
    a, b, z = ([value(x) for x in words[offset:offset+3]] for offset in (0, 3, 6))
    power = sum((zz-aa)*(zz-bb) for aa, bb, zz in zip(a, b, z, strict=True))
    return (power > 0)-(power < 0)


def cases():
    result = []
    def add(a, b, z):
        result.append(tuple(map(bits, (*a, *b, *z))))
    add((-1,0,0), (1,0,0), (0,0,0))
    add((-1,0,0), (1,0,0), (0,1,0))
    add((-1,0,0), (1,0,0), (0,2,0))
    add((0,0,0), (0,0,0), (0,0,0))
    result.extend(((0,0,0, 2,0,0, 1,0,0),
                   (0,0,0, 1,0,0, 0x80000001,0,0),
                   (0x7f7fffff, bits(1),0, 0xff7fffff,bits(-1),0, 0x7f7fffff,0,0),
                   (0x80000000,0,0, 0,0x80000000,0, 0,0,0x80000000)))
    for exp in (-120, -60, 0, 60, 120):
        scale = 2.0**exp
        a, b = (-scale,0,0), (scale,0,0)
        for word in (bits(scale)-1, bits(scale), bits(scale)+1):
            result.append(tuple(map(bits, (*a, *b))) + (0,word,0))
    rng = random.Random(0x8F322026)
    def finite_word():
        while True:
            word = rng.getrandbits(32)
            if word & 0x7f800000 != 0x7f800000:
                return word
    for _ in range(1200):
        words = tuple(finite_word() for _ in range(9))
        result.extend((words, words[3:6]+words[:3]+words[6:], words[:6]+words[:3]))
    for _ in range(300):
        a = tuple(rng.randint(-100000, 100000) for _ in range(3))
        b = tuple(rng.randint(-100000, 100000) for _ in range(3))
        z = tuple((aa+bb)/2 for aa, bb in zip(a,b,strict=True))
        add(a,b,z)
    require(len(result) == 3923, "binary32 fixture inventory changed")
    return result


def validate_rows(rows, inputs):
    require(type(rows) is list and len(rows) == len(inputs), "binary32 result count")
    signs, accepts, fallbacks = {-1:0, 0:0, 1:0}, 0, 0
    for row, words in zip(rows, inputs, strict=True):
        require(type(row) is dict and set(row) == {"filtered", "exact", "filtered_work", "exact_work"},
                "binary32 result shape")
        expected = sign(words)
        require(type(row["filtered"]) is type(row["exact"]) is int and
                row["filtered"] == row["exact"] == expected, "binary32 sign differs from Fraction oracle")
        signs[expected] += 1
        for name in ("filtered_work", "exact_work"):
            work = row[name]
            require(type(work) is dict and set(work) == set(FIELDS) and all(
                type(x) is int and 0 <= x < 1 << 64 for x in work.values()), "binary32 work shape")
            require(work["queries"] == 1 and work["exact_terms"] == 12*work["exact_queries"],
                    "binary32 exact work ledger")
            if name == "exact_work":
                require(work["exact_queries"] == 1 and all(work[key] == 0 for key in FIELDS if key.startswith("filter_")),
                        "binary32 exact path used filter")
            else:
                require(work["filter_queries"] == 1 and work["filter_accepts"] + work["filter_fallbacks"] == 1 and
                        work["exact_queries"] == work["filter_fallbacks"], "binary32 filter partition")
                require(0 < work["filter_axis_products"] <= 3 and
                        0 < work["filter_interval_additions"] <= 3, "binary32 filter paid work absent")
                accepts += work["filter_accepts"]
                fallbacks += work["filter_fallbacks"]
    require(all(signs.values()) and accepts > 0 and fallbacks > 0, "binary32 oracle/filter non-vacuity")
    return dict(cases=len(inputs), signs={str(k):v for k,v in signs.items()},
                filtered_accepts=accepts, exact_fallbacks=fallbacks)


def payload(inputs):
    return "".join(" ".join(f"{word:08x}" for word in words)+"\n" for words in inputs).encode()


def execute(binary, output, name, arguments, data=None):
    """Direct-to-file native streams survive errors and interrupted collection."""
    record = dict(command=[str(binary), *arguments], exit_code=None)
    try:
        with (output / (name+".stdout")).open("xb") as stdout, (output / (name+".stderr")).open("xb") as stderr:
            child = subprocess.run(record["command"], input=data, stdout=stdout, stderr=stderr, check=False)
        record["exit_code"] = child.returncode
    except BaseException as error:
        record["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        (output / (name+".json")).write_text(json.dumps(record, sort_keys=True)+"\n")
    require(record["exit_code"] == 0 and not (output / (name+".stderr")).read_bytes(),
            "binary32 native command failed; streams retained: " + name)
    return (output / (name+".stdout")).read_bytes()


def read_transcript(output, binary):
    require({p.name for p in output.iterdir()} == {"input.hex", "probe.stdout", "probe.stderr", "probe.json",
            "selftest.stdout", "selftest.stderr", "selftest.json"}, "binary32 native transcript inventory")
    require(not any(p.is_symlink() or not p.is_file() for p in output.iterdir()), "binary32 transcript file shape")
    inputs = cases()
    require((output / "input.hex").read_bytes() == payload(inputs), "binary32 native input differs")
    for name, arguments in (("probe", []), ("selftest", ["--selftest"])):
        record = json.loads((output / (name+".json")).read_bytes())
        require(record == dict(command=[str(binary), *arguments], exit_code=0) and
                not (output / (name+".stderr")).read_bytes(), "binary32 native command/streams differ")
    stdout = (output / "probe.stdout").read_bytes()
    result = validate_rows([json.loads(line) for line in stdout.splitlines()], inputs)
    native_test = json.loads((output / "selftest.stdout").read_bytes())
    require(native_test.get("status") == "passed" and native_test.get("tests") == 49 and
            native_test.get("rounding_modes") == 4, "binary32 native selftest lacks checks")
    return dict(**result, native_selftest=native_test,
                input_sha256=hashlib.sha256(payload(inputs)).hexdigest(),
                output_sha256=hashlib.sha256(stdout).hexdigest())


def run(binary, output):
    inputs = cases()
    data = payload(inputs)
    binary_hash = hashlib.sha256(binary.read_bytes()).hexdigest()
    output.mkdir(parents=True, exist_ok=False)
    (output / "input.hex").write_bytes(data)
    stdout = execute(binary, output, "probe", [], data)
    rows = [json.loads(line) for line in stdout.splitlines()]
    result = validate_rows(rows, inputs)
    # Mutate one of each sign, remove a row and corrupt work; positive
    # population thresholds stay separate from these rejection checks.
    mutations = []
    for expected in (-1,0,1):
        altered = deepcopy(rows)
        index = next(i for i,row in enumerate(rows) if row["exact"] == expected)
        altered[index]["filtered"] = 1 if expected != 1 else -1
        mutations.append(altered)
    mutations.append(rows[:-1])
    altered = deepcopy(rows)
    altered[0]["exact_work"]["exact_terms"] += 1
    mutations.append(altered)
    rejected = 0
    for altered in mutations:
        try:
            validate_rows(altered, inputs)
        except ValueError:
            rejected += 1
    require(rejected == 5, "binary32 oracle mutation survived")
    execute(binary, output, "selftest", ["--selftest"])
    transcript = read_transcript(output, binary)
    require(all(transcript[key] == item for key, item in result.items()), "binary32 transcript differs from gate")
    require(hashlib.sha256(binary.read_bytes()).hexdigest() == binary_hash, "binary32 probe changed during gate")
    return dict(schema="mhgp8_float32_predicate_gate_v1", status="passed", **transcript,
                rejected_mutations=rejected,
                binary=str(binary), binary_sha256=binary_hash,
                gate_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                scope="binary32_q2_power_primitive_not_pipeline_or_FULL", optimized=bool(sys.flags.optimize))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args.binary.resolve(), args.output.resolve()), sort_keys=True))


if __name__ == "__main__":
    main()
