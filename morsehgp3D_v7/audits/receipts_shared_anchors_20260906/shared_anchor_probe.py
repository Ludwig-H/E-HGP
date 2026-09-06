"""Replay pinned tower histories to check shared anchors and their failure modes."""

from copy import deepcopy
from fractions import Fraction as Q
from pathlib import Path
from typing import Any
import hashlib
import json


AUDIT = Path(__file__).resolve().parent.parent
SOURCE = AUDIT / "receipts_gabriel_vertices_20260906/tower_normal.json"
SOURCE_SHA = "13fbed40cbdb627e3bf578fb8e8783e420c0cbc618d12fcd80b8e047fe031d0d"
FULL = AUDIT / "receipts_gabriel_20260905/full_normal.json"
FULL_SHA = "00911669a9045b0bd466ee991d9972e764c46a0fa941d7ab79cfeca3082e627f"


def require(ok: bool, why: str) -> None:
    if not ok:
        raise ValueError(why)


def main() -> None:
    require(hashlib.sha256(SOURCE.read_bytes()).hexdigest() == SOURCE_SHA,
            "tower_pin")
    tower = json.loads(SOURCE.read_text())["nominal"]
    nodes = {int(k): v["nodes"] for k, v in tower["orders"].items()}
    # Reconstruct historical parent links, not a final compressed successor.
    parent = {k: {p: i for i, n in enumerate(v) for p in n["parents"]}
              for k, v in nodes.items()}

    def active(level: Q, cut: Q, closed: bool) -> bool:
        return level <= cut if closed else level < cut

    def normalize(k: int, token: int, cut: Q, closed: bool) -> int:
        require(type(token) is int and 0 <= token < len(nodes[k]), "token_range")
        require(active(Q(nodes[k][token]["level"]), cut, closed), "token_not_born")
        while token in parent[k]:
            next_token = parent[k][token]
            if not active(Q(nodes[k][next_token]["level"]), cut, closed):
                break
            token = next_token
        return token

    records = []
    for link in tower["vertical_birth_links"]:
        m, label, level = link["upper_order"], link["leaf"], Q(link["level"])
        closed = next(row for row in tower["observations"]
                      if Q(row["level"]) == level and row["side"] == "closed")
        lower = closed["orders"][m - 2]
        boundary = {label.replace(p, "") for p in label}
        group = next(g for g in lower["Gamma"] if boundary <= set(g))
        minima = set(group) & set(tower["orders"][str(m - 1)]["minima"])
        expected = next(r["id"] for r in lower["roots"] if set(r["minima"]) == minima)
        require(expected == link["lower_closed_anchor"], "direct_equals_vertical")
        require(nodes[m][link["upper_birth"]]["minimum"] == label, "upper_birth_binding")
        records.append(dict(label=label, rank=m, level=str(level),
                            upper_birth=link["upper_birth"], lower_order=m - 1,
                            lower_anchor=expected, accepted=True))
    require(len(records) == 7, "shared_record_nonvacuity")

    transport_checks = 0
    for row in tower["observations"]:
        cut, closed = Q(row["level"]), row["side"] == "closed"
        for record in records:
            if not active(Q(record["level"]), cut, closed):
                continue
            m, label = record["rank"], record["label"]
            mapped = next(v for v in row["vertical"]
                          if v["upper_order"] == m and label in v["upper_minima"])
            target = normalize(m - 1, record["lower_anchor"], cut, closed)
            require(nodes[m - 1][target]["minima"] == mapped["lower_minima"],
                    "shared_anchor_at_requested_cut")
            transport_checks += 1

    # A proposed protocol gate: absence is optional; present invalid data refuses.
    def lookup(record: dict[str, Any] | None, k: int, ball_level: Q,
               cut: Q) -> int | None:
        if record is None:
            return None
        require(record["accepted"], "anchor_owner_unaccepted")
        require(record["label"] == "ACD", "anchor_key")
        require(record["lower_order"] == k and record["rank"] == k + 1,
                "anchor_order")
        require(Q(record["level"]) == ball_level, "anchor_level")
        require(ball_level < cut, "anchor_not_prior")
        token = record["lower_anchor"]
        require(normalize(k, token, ball_level, True) == token,
                "anchor_not_closed_root")
        return normalize(k, token, cut, False)

    acd = next(r for r in records if r["label"] == "ACD")
    require(lookup(acd, 2, Q(16), Q(169, 9)) == 4, "J1_shared_positive")
    require(lookup(None, 2, Q(16), Q(169, 9)) is None, "optional_anchor_fallback")
    rejected = {}
    for name, field, value, expected in [
        ("wrong_order", "lower_order", 1, "anchor_order"),
        ("wrong_key", "label", "ABC", "anchor_key"),
        ("unaccepted_owner", "accepted", False, "anchor_owner_unaccepted"),
        ("wrong_level", "level", "13", "anchor_level"),
        ("out_of_range", "lower_anchor", 1000, "token_range"),
    ]:
        bad = deepcopy(acd)
        bad[field] = value
        try:
            lookup(bad, 2, Q(16), Q(169, 9))
        except ValueError as error:
            require(str(error) == expected, "wrong_mutant_rejection")
            rejected[name] = str(error)
        else:
            raise ValueError("corruption_hidden_as_fallback")
    try:
        lookup(acd, 2, Q(16), Q(16))
    except ValueError as error:
        require(str(error) == "anchor_not_prior", "same_lot_rejection")
        rejected["same_lot"] = str(error)
    else:
        raise ValueError("same_lot_accepted")

    cd = next(r for r in records if r["label"] == "CD")
    at_birth = Q(cd["level"])
    correct = normalize(1, cd["lower_anchor"], at_birth, True)
    wrong_order_token = normalize(1, cd["upper_birth"], at_birth, True)
    premature_final = normalize(1, cd["lower_anchor"], Q(100), True)
    require((correct, wrong_order_token, premature_final) == (4, 1, 5),
            "typed_token_and_historical_cut_mutants")
    require(normalize(1, wrong_order_token, Q(100), True) == premature_final,
            "final_cut_alone_masks_wrong_order_token")
    require(normalize(1, cd["lower_anchor"], Q(13), True) == 5,
            "old_anchor_needs_normalization")
    siblings = [r for r in records if r["rank"] == 2 and Q(r["level"]) == at_birth]
    require(len(siblings) == 2 and len({r["lower_anchor"] for r in siblings}) == 1,
            "deduplicating_by_lower_anchor_loses_a_leaf")
    require(set(nodes[1][correct]["minima"]) == {"A", "C", "D"},
            "whole_equal_level_batch_required")
    horizons = []
    for h in range(1, 5):
        shared = [r["label"] for r in records if r["rank"] <= h]
        boundary = [r["label"] for r in records if r["rank"] == h + 1]
        horizons.append(dict(Kmax=h, shared_vertical_records=shared,
                             optional_top_rank_anchors=boundary))
    require([len(h["shared_vertical_records"]) for h in horizons] == [0, 4, 6, 7],
            "horizon_counts")
    # E5 has a genuine no-op lower connection CE but a new upper leaf CE.
    require(hashlib.sha256(FULL.read_bytes()).hexdigest() == FULL_SHA, "E5_pin")
    full = json.loads(FULL.read_text())
    rows = {r["K"]: r for r in full["records"]
            if r["case"] == "E5" and not r["reversed_ids"]}
    upper_leaf = next(e for e in rows[2]["journal"]
                      if e["kind"] == "birth" and e["label"] == [2, 4])
    no_op_level = Q(upper_leaf["level"])
    require(no_op_level == Q(11, 2), "CE_birth")
    require(all(Q(e["level"]) != no_op_level for e in rows[1]["journal"]),
            "no_lower_event_at_upper_birth")
    roots: dict[int, set[int]] = {}
    for event in rows[1]["journal"]:
        if Q(event["level"]) > no_op_level:
            break
        if event["kind"] == "birth":
            roots[event["output"]] = set(event["label"])
        else:
            covered = set().union(*(roots.pop(p) for p in event["parents"]))
            roots[event["output"]] = covered
    require(roots[5] == {2, 3, 4}, "CE_prior_lower_component")
    points = next(c["points"] for c in full["cases"]
                  if c["name"] == "E5" and not c["reversed_ids"])
    powers = {str(i): sum((p[j] - points[2][j]) * (p[j] - points[4][j])
                         for j in range(3)) for i, p in enumerate(points) if i not in (2, 4)}
    require(all(value > 0 for value in powers.values()), "CE_strict_Gabriel")
    require(Q(sum((points[2][j] - points[4][j]) ** 2 for j in range(3)), 4)
            == no_op_level, "CE_geometric_level")
    result = dict(schema="mhgp7-shared-anchor-history-replay-v1", status="passed",
                  source_sha256=SOURCE_SHA, no_op_source_sha256=FULL_SHA,
                  script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  scope="Pinned n4 tower histories, shared-anchor identity and proposed protocol rejection models; no new constructor or C++ run",
                  records=records, transport_checks=transport_checks, horizons=horizons,
                  protocol_rejections=rejected,
                  semantic_mutants=dict(upper_token_as_lower=wrong_order_token,
                                        expected_at_birth=correct,
                                        final_normalization_too_early=premature_final,
                                        distinct_upper_leaves=2, shared_lower_value=1),
                  no_op_witness=dict(case="E5", label="CE", rank=2,
                                     level=str(no_op_level), lower_closed_anchor=5,
                                     upper_birth=upper_leaf["output"],
                                     lower_points=[2, 3, 4], foreign_powers=powers),
                  publication="accepted flag models owner validity; no product allocation failure or shared-memory implementation exercised",
                  engine_calls=0, public_status="not_claimed", gcp="not_used")
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
