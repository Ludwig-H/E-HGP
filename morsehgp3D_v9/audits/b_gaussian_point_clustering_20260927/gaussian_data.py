#!/usr/bin/env python3
"""Predeclared, paired Gaussian scenes. No clustering, fitting or score tuning."""
from __future__ import annotations

import argparse
from fractions import Fraction
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
QUANTIZER = HERE.parent / "b_point_hierarchy_k_20260927" / "datasets.py"
QUANTIZER_SHA256 = "7ff3d93677f51eabe9c1bea0f56a62bbcd53ce531c3a49bdfafeab29ab14f53c"
N = 1200
SEEDS = (2026092711, 2026092712, 2026092713)
SEPARATIONS = (8, 4, 2)
COMMUNITIES = (2, 4, 8, 16)


def plan_cases():
    cases = []
    for regime, groups, seeds in (("spherical", COMMUNITIES, SEEDS),
                                   ("anisotropic", (8,), SEEDS[:2]),
                                   ("unbalanced", (8,), SEEDS[:2])):
        for g in groups:
            for delta in SEPARATIONS:
                for index, seed in enumerate(seeds, 1):
                    cases.append(dict(id=f"{regime}_g{g}_d{delta}_s{index}", regime=regime,
                        communities=g, separation=delta, seed=seed, seed_index=index,
                        split="development" if index == 1 else "evaluation", n=N, dimension=3))
    return cases


PLAN = dict(schema="gaussian-point-clustering-plan-v1", n=N, cases=plan_cases(),
    k=[5, 10], min_cluster_size=[10, 20, 50, 100], exp_z=[1, 2],
    primary=dict(k=5, min_cluster_size=20, exp_z=1),
    source_quantizer_sha256=QUANTIZER_SHA256,
    selection="fixed before any clustering scores; retain every prescribed realization",
    pairing="same unit centers, rotations, standardized normal draws and labels across delta for each regime/G/seed",
    layout="farthest-first cubic grid {-1,0,1}^3; lexicographic ties; centered; minimum distance 1; seeded proper rotation",
    injected_noise=False, truth="generating component labels 1..G; Gaussian overlap is not mislabeled noise",
    map_scope="known-parameter pointwise Gaussian MAP diagnostic; not clustering baseline or ARI bound")


def _quantizer():
    if hashlib.sha256(QUANTIZER.read_bytes()).hexdigest() != QUANTIZER_SHA256:
        raise ValueError("pinned prior quantizer source changed")
    spec = importlib.util.spec_from_file_location("gaussian_pinned_quantizer", QUANTIZER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def canonical_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n").encode()


def proper_rotation(seed, g, stream):
    rng = np.random.Generator(np.random.PCG64(np.random.SeedSequence([seed, g, stream])))
    q, r = np.linalg.qr(rng.normal(size=(3, 3)))
    q = q @ np.diag(np.where(np.diag(r) < 0, -1., 1.))
    if np.linalg.det(q) < 0:
        q[:, -1] *= -1
    return q


def unit_layout(g):
    if type(g) is not int or g not in COMMUNITIES:
        raise ValueError("G must be one of 2,4,8,16")
    grid = list(itertools.product((-1, 0, 1), repeat=3))
    chosen = [grid[0]]
    while len(chosen) < g:
        def distance(candidate):
            return min(sum((a-b)**2 for a,b in zip(candidate, prior)) for prior in chosen)
        remaining = [point for point in grid if point not in chosen]
        # max retains the first (lexicographic) point when integer distances tie.
        chosen.append(max(remaining, key=distance))
    minimum_squared = min(sum((a-b)**2 for a,b in zip(x,y))
                          for i,x in enumerate(chosen) for y in chosen[:i])
    centers = np.asarray(chosen, dtype=np.float64)
    centers -= centers.mean(axis=0)
    return centers / np.sqrt(minimum_squared)


def generate(case):
    g, seed, delta, regime = (case[k] for k in ("communities", "seed", "separation", "regime"))
    if regime not in ("spherical", "anisotropic", "unbalanced") or delta not in SEPARATIONS:
        raise ValueError("unplanned regime or separation")
    if seed not in SEEDS or (regime != "spherical" and (g != 8 or seed not in SEEDS[:2])):
        raise ValueError("unplanned seed or stress G")
    common_rotation = proper_rotation(seed, g, 1)
    centers = unit_layout(g) @ common_rotation.T
    means = float(delta) * centers
    weights = np.asarray([4 if i % 2 == 0 else 1 for i in range(g)] if regime == "unbalanced" else [1]*g)
    counts = N * weights // weights.sum()
    if int(counts.sum()) != N or np.any(N * weights % weights.sum()):
        raise ValueError("prescribed weights must yield exact integer class sizes")
    labels = np.repeat(np.arange(1, g+1, dtype=np.int64), counts)
    rng = np.random.Generator(np.random.PCG64(np.random.SeedSequence([seed, g, 0])))
    normals = rng.normal(size=(N, 3))
    transforms, covariance, rotations = [], [], []
    for j in range(g):
        rotation = proper_rotation(seed, g, 100+j) if regime == "anisotropic" else np.eye(3)
        transform = rotation @ np.diag([2., 1., .5]) if regime == "anisotropic" else np.eye(3)
        rotations.append(rotation)
        transforms.append(transform)
        covariance.append(transform @ transform.T)
    points = np.empty((N, 3), dtype=np.float64)
    for j in range(g):
        selected = labels == j+1
        points[selected] = means[j] + normals[selected] @ transforms[j].T
    distances = [float(np.linalg.norm(means[i]-means[j])) for i in range(g) for j in range(i)]
    parameters = dict(communities=g, n=N, regime=regime, seed=seed, separation=delta,
        means=means.tolist(), covariances=np.asarray(covariance).tolist(),
        priors=(counts/N).tolist(), true_counts=counts.tolist(),
        common_rotation=common_rotation.tolist(), component_rotations=np.asarray(rotations).tolist(),
        unit_centers=centers.tolist(), minimum_mean_distance_observed=min(distances),
        separation_units="reference isotropic standard deviation 1; not anisotropic Mahalanobis distance",
        standard_deviations=[2., 1., .5] if regime == "anisotropic" else [1., 1., 1.],
        truth_labels="positive component IDs 1..G", injected_noise_count=0,
        numpy_version=np.__version__, generator="numpy.PCG64/SeedSequence streams [seed,G,0/1/100+j]",
        standardized_draws_sha256=hashlib.sha256(np.asarray(normals, dtype="<f8").tobytes()).hexdigest())
    return points, labels, parameters


def bayes_map(points, parameters):
    """Pointwise MAP with prescribed model priors/covariances, never fitted."""
    means = np.asarray(parameters["means"])
    covariance = np.asarray(parameters["covariances"])
    priors = np.asarray(parameters["priors"])
    scores = np.empty((len(points), len(means)), dtype=np.float64)
    for j, (mean, cov, prior) in enumerate(zip(means, covariance, priors)):
        sign, logdet = np.linalg.slogdet(cov)
        if sign <= 0 or prior <= 0:
            raise ValueError("invalid known Gaussian parameters")
        difference = np.asarray(points) - mean
        maha = np.einsum("ni,ij,nj->n", difference, np.linalg.inv(cov), difference)
        scores[:, j] = np.log(prior) - .5 * (3*np.log(2*np.pi) + logdet + maha)
    if not np.isfinite(scores).all():
        raise ValueError("nonfinite MAP score")
    return 1 + np.argmax(scores, axis=1)


def map_diagnostic(points, q, labels, parameters, grid):
    original = bayes_map(points, parameters)
    # Reconstruct physical coordinates; the competing algorithms use exact q.
    origin = np.asarray([float(Fraction(x)) for x in grid["origin_exact"]])
    reconstructed = origin + q.astype(np.float64) * float(Fraction(grid["step_exact"]))
    prepared = bayes_map(reconstructed, parameters)
    g = parameters["communities"]
    confusion = np.zeros((g, g), dtype=np.int64)
    for actual, predicted in zip(labels, original):
        confusion[actual-1, predicted-1] += 1
    return dict(scope=PLAN["map_scope"], predictions_original=original.tolist(),
        predictions_reconstructed_grid=prepared.tolist(),
        empirical_accuracy_original=float(np.mean(original == labels)),
        empirical_accuracy_reconstructed_grid=float(np.mean(prepared == labels)),
        grid_prediction_changes=int(np.count_nonzero(original != prepared)),
        confusion_rows_truth_columns_map=confusion.tolist(),
        exact_score_tie_policy="lowest positive component ID", fitted=False,
        caution="finite-sample model-aware diagnostic, not theoretical Bayes error or an upper bound on ARI")


def prepare(root):
    import io
    quantizer = _quantizer()
    root = Path(root).resolve()
    root.mkdir(parents=True, exist_ok=True)
    quantizer.write_once(root / "plan.json", canonical_bytes(PLAN))
    manifest = dict(schema="gaussian-point-clustering-datasets-v1", complete=True,
        plan=PLAN, plan_sha256=quantizer.digest(root / "plan.json"),
        numpy_version=np.__version__, quantizer_source=str(QUANTIZER),
        quantizer_source_sha256=QUANTIZER_SHA256, cases=[], manifest_path=str(root / "manifest.json"))
    for case in PLAN["cases"]:
        points, labels, parameters = generate(case)
        q, truth, grid = quantizer.quantize(points, labels)
        diagnostic = map_diagnostic(points, q, truth, parameters, grid)
        destination = root / "prepared" / case["id"]
        destination.mkdir(parents=True, exist_ok=True)
        paths = dict(points_u32le=destination / "points.u32le", points_npy=destination / "points.npy",
            labels_json=destination / "labels.json", parameters_json=destination / "parameters.json",
            bayes_map_json=destination / "bayes_map.json")
        stream = io.BytesIO()
        np.save(stream, q.astype(np.float64), allow_pickle=False)
        payloads = dict(points_u32le=q.tobytes(), points_npy=stream.getvalue(),
                        labels_json=canonical_bytes(truth.tolist()),
                        parameters_json=canonical_bytes(parameters), bayes_map_json=canonical_bytes(diagnostic))
        for key, path in paths.items():
            quantizer.write_once(path, payloads[key])
        entry = dict(case, name=case["id"], collection="synthetic_gaussian", source_url=None,
            source_sha256=hashlib.sha256(np.asarray(points, dtype="<f8").tobytes() +
                                         np.asarray(labels, dtype="<i8").tobytes()).hexdigest(),
            source_hash_encoding="C-order little-endian float64 XYZ then int64 positive truth labels",
            quantization=grid, noise_label=-1, noise_count=0, true_counts=parameters["true_counts"],
            bayes_map_accuracy_original=diagnostic["empirical_accuracy_original"],
            bayes_map_accuracy_reconstructed_grid=diagnostic["empirical_accuracy_reconstructed_grid"],
            **{key:str(path) for key,path in paths.items()})
        entry["prepared_sha256"] = {key:quantizer.digest(paths[key])
                                    for key in ("points_u32le", "points_npy", "labels_json")}
        entry["diagnostic_sha256"] = {key:quantizer.digest(paths[key])
                                      for key in ("parameters_json", "bayes_map_json")}
        quantizer.write_once(destination / "case.json", canonical_bytes(entry))
        manifest["cases"].append(entry)
    quantizer.write_once(root / "manifest.json", canonical_bytes(manifest))
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    result = prepare(args.root)
    print(json.dumps(dict(manifest=result["manifest_path"], cases=len(result["cases"]),
                         points=sum(c["n"] for c in result["cases"])), sort_keys=True))


if __name__ == "__main__":
    main()
