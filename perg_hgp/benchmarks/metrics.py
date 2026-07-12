"""Clustering metrics with explicit noise and oracle semantics."""

from __future__ import annotations

from typing import Any

import numpy as np


def label_summary(labels: np.ndarray) -> dict[str, Any]:
    values = np.asarray(labels, dtype=np.int64)
    unique, counts = np.unique(values, return_counts=True)
    noise_count = int(counts[unique == -1].sum())
    cluster_counts = counts[unique >= 0]
    largest = sorted((int(value) for value in cluster_counts), reverse=True)[:20]
    return {
        "n_labels": int(values.size),
        "n_clusters_excluding_noise": int(np.count_nonzero(unique >= 0)),
        "noise_count": noise_count,
        "noise_fraction": noise_count / max(1, int(values.size)),
        "cluster_size_min": None if cluster_counts.size == 0 else int(cluster_counts.min()),
        "cluster_size_median": (
            None if cluster_counts.size == 0 else float(np.median(cluster_counts))
        ),
        "cluster_size_max": None if cluster_counts.size == 0 else int(cluster_counts.max()),
        "largest_cluster_sizes": largest,
    }


def clustering_scores(
    truth: np.ndarray | None, prediction: np.ndarray
) -> dict[str, Any]:
    """Return standard ARI and a foreground-only diagnostic."""

    if truth is None:
        return {
            "ground_truth_available": False,
            "ARI": None,
            "ARI_non_noise_truth": None,
        }
    from sklearn.metrics import adjusted_rand_score

    expected = np.asarray(truth)
    observed = np.asarray(prediction)
    if expected.shape != observed.shape:
        raise ValueError(
            f"truth/prediction shape mismatch: {expected.shape} != {observed.shape}"
        )
    foreground = expected >= 0
    ari = float(adjusted_rand_score(expected, observed))
    foreground_ari = (
        None
        if int(foreground.sum()) < 2
        else float(adjusted_rand_score(expected[foreground], observed[foreground]))
    )
    return {
        "ground_truth_available": True,
        "ARI": ari,
        "ARI_non_noise_truth": foreground_ari,
        "ground_truth_non_noise_count": int(foreground.sum()),
        "assigned_fraction_on_non_noise_truth": (
            None
            if not np.any(foreground)
            else float(np.mean(observed[foreground] >= 0))
        ),
    }

