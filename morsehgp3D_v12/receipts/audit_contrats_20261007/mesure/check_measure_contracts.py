#!/usr/bin/env python3
"""Rejoue les témoins légers sur les lecteurs et documents strictement épinglés.

Tout le FULL généré est synthétique : un site choisi dans ce script. Aucun
fichier de nuage réel ni archive de données n'est ouvert.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import types

PIN = "fc1f913ce325b04922b897137a9507153b129894"
ROOT = Path(__file__).resolve().parents[4]
SOURCE_HASHES = {}


def source(path):
    value = subprocess.check_output(["git", "show", f"{PIN}:{path}"], cwd=ROOT)
    SOURCE_HASHES[path] = hashlib.sha256(value).hexdigest()
    return value


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def module(name, path):
    result = types.ModuleType(name)
    sys.modules[name] = result
    exec(compile(source(path), f"{PIN}:{path}", "exec"), result.__dict__)
    return result


def words(*values):
    return struct.pack("<" + "Q" * len(values), *values)


def integer(value):
    # Canonical positive one-limb integer, sufficient for this tiny fixture.
    need(0 <= value < 2**64, "fixture integer domain")
    return words(0, 1, value)


def one_site_full(xyz, bits):
    # One order, one birth at radius zero, no edges or verticals, root zero.
    # The one site's stable PointId is 7 in every translation/profile.
    return (b"MHGP11FUL1" + words(bits, 1, 1, 1) + words(*xyz, 1, 7)
            + words(1, 1, 1, 0, 0) + words(2**32-1, 0, 0)
            + integer(0) + integer(1)
            + b"".join(integer(value) for value in xyz) + integer(1))


def main():
    for path in (
        "morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md",
        "morsehgp3D_v12/docs/DECISIONS.md",
        "morsehgp3D_v12/docs/ARCHITECTURE.md",
        "morsehgp3D_v12/docs/MESURE.md",
        "morsehgp3D_v12/docs/PLAN.md",
        "morsehgp3D_v12/docs/PROVENANCE.md",
    ):
        source(path)
    module("catalogue_semantic", "morsehgp3D_v11/bench/catalogue_semantic.py")
    reader = module("full_semantic", "morsehgp3D_v11/bench/full_semantic.py")
    samples = []
    for name, xyz, bits in (
        ("original_u21", (0, 0, 0), 21),
        ("translate_u21", (1, 0, 0), 21),
        ("same_input_u24", (0, 0, 0), 24),
    ):
        raw = one_site_full(xyz, bits)
        decoded = reader.decode(raw, bits, 1, 1)
        samples.append(dict(name=name, xyz=list(xyz), point_id=7, accepted=True,
                            decoded=decoded))
    original, translated, cross_profile = (row["decoded"] for row in samples)
    need(original["sha256"] != translated["sha256"], "translation unexpectedly preserves digest")
    need(original["sha256"] == cross_profile["sha256"], "positive control: same input cross-profile")
    need(original["orders"] == translated["orders"], "fixture topology differs")
    try:
        reader.decode(one_site_full((0, 0, 0), 32), 32, 1, 1)
    except ValueError as error:
        profile32 = dict(accepted=False, reason=str(error))
    else:
        raise ValueError("expected legacy reader to reject B=32")

    # Countermodels of two different adoption predicates, not measurements.
    models = []
    for baseline, candidate_original, candidate_translated in ((100,120,120),(100,100,104)):
        widening = candidate_original / baseline
        translation = candidate_translated / candidate_original
        models.append(dict(
            kind="synthetic_countermodel_not_a_measurement",
            u21_build_original_ms=baseline,
            candidate_build_original_ms=candidate_original,
            candidate_build_translated_ms=candidate_translated,
            widening_ratio=widening, translation_ratio=translation,
            d6_accepts=widening < 1.03,
            numerical_section8_accepts=translation < 1.03))
    need(all(row["d6_accepts"] != row["numerical_section8_accepts"] for row in models),
         "countermodels must disagree")

    # Identify archived measured modes without reading/republishing point data.
    reports = []
    base = "morsehgp3D_v11/receipts/developpement_20261007/filtre_g1_avx2/claudeg1/"
    for name in ("gpu_ab_report_ab_k5_16_cpu.json", "gpu_ab_report_ab_k5_24_gpu.json",
                 "gpu_ab_report_ab_k10_24_gpu.json"):
        data = json.loads(source(base + name))
        reports.append(dict(path=base+name, schema=data["schema"],
                            kmax=data["kmax"], leaf=data["leaf"], workers=data["workers"],
                            modes=data["modes"], bench_sha256=data["bench_sha256"],
                            verdict=data.get("verdict"), reps=data["reps"],
                            warm_passes=data["warm_passes"]))
    result = dict(schema="ehgp.v12.audit_measure_contracts.v1", pin=PIN,
                  source_sha256=SOURCE_HASHES,
                  CST_0206=dict(translation_fixture=samples,
                                same_topology=True, same_levels=True,
                                translated_semantic_digest_equal=False,
                                same_input_cross_profile_digest_equal=True,
                                legacy_reader_u32=profile32),
                  CST_0207=dict(countermodels=models),
                  measured_v11_modes=reports,
                  scope="synthetic codec fixture and document predicates; no v12 engine, no GCP")
    print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
