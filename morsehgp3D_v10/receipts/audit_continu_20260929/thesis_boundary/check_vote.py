#!/usr/bin/env python3
"""Exact abstract face-tree check; not a geometric FULL implementation."""
import json
from fractions import Fraction


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def run() -> dict[str, object]:
    # Positive fixed face scores, the normalization of thesis section 9.1.
    faces = [
        ("xa", ("x", "a"), 3, "A"),
        ("xb", ("x", "b"), 3, "B"),
        ("xc", ("x", "c"), 4, "C"),
        ("yd", ("y", "d"), 1, "A"),
        ("ye", ("y", "e"), 1, "B"),
        ("yf", ("y", "f"), 8, "C"),
    ]
    points = sorted({point for _, ends, _, _ in faces for point in ends})
    totals = {
        point: sum(score for _, ends, score, _ in faces if point in ends)
        for point in points
    }
    masses = {
        name: sum(Fraction(score, totals[point]) for point in ends)
        for name, ends, score, _ in faces
    }
    require(sum(masses.values()) == len(points), "unit point mass lost")

    def vote(
        coarsen: dict[str, str],
    ) -> tuple[dict[str, str], dict[str, dict[str, str]]]:
        labels, scores = {}, {}
        for point in points:
            votes = {}
            for _, ends, score, branch in faces:
                if point in ends:
                    target = coarsen[branch]
                    votes[target] = votes.get(target, Fraction(0)) + Fraction(
                        score, totals[point]
                    )
            maximum = max(votes.values())
            winners = sorted(key for key, value in votes.items() if value == maximum)
            require(len(winners) == 1, "fixture must not depend on tie-breaking")
            labels[point] = winners[0]
            scores[point] = {key: str(value) for key, value in sorted(votes.items())}
        return labels, scores

    early, early_votes = vote({"A": "A", "B": "B", "C": "C"})
    late, late_votes = vote({"A": "AB", "B": "AB", "C": "C"})
    require(early["x"] == early["y"] == "C", "unexpected first partition")
    require(late["x"] == "AB" and late["y"] == "C", "split not exercised")
    # Face clusters only merged. Point clusters, recomputed independently, split.
    violation = {"pair": ["x", "y"], "early": [early["x"], early["y"]],
                 "late": [late["x"], late["y"]]}
    fixed = {point: {"A": "AB", "B": "AB", "C": "C"}[label]
             for point, label in early.items()}
    for left in points:
        for right in points:
            require(early[left] != early[right] or fixed[left] == fixed[right],
                    "fixed anchors did not preserve nesting")
    return {
        "status": "pass",
        "scope": "abstract_positive_face_weights_not_a_geometric_FULL_receipt",
        "point_count": len(points), "face_count": len(faces),
        "total_face_mass": str(sum(masses.values())),
        "early_votes": early_votes, "late_votes": late_votes,
        "early_labels": early, "late_labels": late,
        "nesting_violation_recomputed_vote": violation,
        "fixed_anchors_nested": True,
    }


if __name__ == "__main__":
    print(json.dumps(run(), indent=2, sort_keys=True))
