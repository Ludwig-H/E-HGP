"""Necessary MEB-work bounds on six closed singleton captures; no engine."""

from __future__ import annotations

from copy import deepcopy
from fractions import Fraction
import hashlib
import json
from math import comb
from pathlib import Path


HERE = Path(__file__).resolve().parent
V7 = HERE.parent
BASE = V7 / "receipts/full_gabriel_singleton_mono_20260905/paired"
HEADERS = {
    "old": "13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627",
    "new": "21b77d29a4ba2bca453b602a8faa4564a978f4ba71af5167c164faae4ef0e1a5",
}
MEB = "f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76"


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def derive(row: dict) -> dict:
    require(row["outcome"] == "complete_relative", "completed order required")
    require(row["alias_policy"] == "lazy_first_c_strict_resolutions_v1", "lazy contract")
    names = ("k", "portal_requests", "chain_steps", "meb_calls", "geometry_meb_calls", "meb_supports")
    require(all(type(row[n]) is int and row[n] >= 0 for n in names), "integer counts")
    k, p, c, m, gm, s = (row[n] for n in names)
    require(1 <= k <= 10 and m == gm == p + c, "one initial MEB plus one per descent")
    require(row["terminal_direct"] == p, "closed portal requests")
    require(k != 1 or m == s == 0, "K1 has no MEB")
    # Every successful local call tries supports in arity order 2, 3, 4.
    low = p * (comb(k, 2) + comb(k, 3)) + c * (comb(k + 1, 2) + comb(k + 1, 3))
    q4max = p * comb(k, 4) + c * comb(k + 1, 4)
    require(m <= s <= low + q4max, "enumeration support range")
    excess = max(0, s - low)
    per_call_max = comb(k + 1, 4)
    calls_min = (excess + per_call_max - 1) // per_call_max if excess else 0
    require(calls_min <= m, "terminal q4 population bound")
    return {"k": k, "initial_calls_nK": p, "descent_calls_nKplus1": c,
            "meb_calls": m, "supports_tried": s,
            "q2_q3_support_upper_bound": low, "q4_support_lower_bound": excess,
            "q4_terminal_call_lower_bound": calls_min,
            "q4_support_lower_fraction": str(Fraction(excess, s)) if s else None,
            "mean_supports_per_call": str(Fraction(s, m)) if m else None}


def main() -> None:
    pins, attempts, reference, sample = {}, [], None, None

    def read(path: Path) -> bytes:
        raw = path.read_bytes()
        pins[str(path.relative_to(V7))] = hashlib.sha256(raw).hexdigest()
        return raw

    files = sorted(BASE.glob("*/n8000*.raw.txt"))
    require(len(files) == 6, "six closed attempts")
    for raw_path in files:
        stem = str(raw_path)[:-len(".raw.txt")]
        receipt = json.loads(read(Path(stem + ".receipt.json")))
        raw = read(raw_path)
        require(receipt["status"] == "completed" and receipt["error"] is None
                and receipt["exit_code"] == 0, "closed successful attempt")
        require(receipt["streams"][raw_path.name] ==
                {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}, "raw binding")
        rows = [json.loads(line) for line in raw.splitlines() if line.startswith(b"{")]
        orders = [r for r in rows if r.get("type") == "order"]
        terminal = [r for r in rows if r.get("type") == "terminal"]
        require(orders == receipt["orders"] and len(terminal) == 1
                and terminal[0] == receipt["terminal"], "raw receipt equality")
        require(terminal[0]["complete_requested_horizontal_orders"] is True
                and terminal[0]["exit_code"] == 0 and len(orders) == 10, "complete ten orders")
        before = json.loads(read(Path(stem + ".sources_before.json")))
        after = json.loads(read(Path(stem + ".sources_after.json")))
        require(before == after, "source capture stability")
        arm = "old" if "_old_" in raw_path.parent.name else "new"
        require(before["files"]["morsehgp3D_v7/src/forest/full_gabriel.hpp"] == HEADERS[arm]
                and before["files"]["morsehgp3D_v7/src/forest/silent_incidence.hpp"] == MEB,
                "caller cardinalities and MEB enumeration premises")
        result = [derive(r) for r in orders]
        require([r["k"] for r in result] == list(range(1, 11)), "order inventory")
        if reference is None:
            reference, sample = result, orders[-1]
        else:
            require(reference == result, "same MEB work across six arms")
        attempts.append(str(raw_path.relative_to(V7)))
    mutations = []
    for name, field, value in [
        ("missing_call", "meb_calls", sample["meb_calls"] - 1),
        ("impossible_support_count", "meb_supports", 10**12),
        ("refused_prefix", "outcome", "resource_exhausted"),
        ("boolean_count", "chain_steps", True),
    ]:
        bad = deepcopy(sample)
        bad[field] = value
        try:
            derive(bad)
        except ValueError:
            mutations.append(name)
        else:
            raise ValueError("data corruption survived: " + name)
    print(json.dumps({"status": "passed", "scope": "Necessary MEB count bounds only; no timing, whole packet or geometric qualification",
                      "attempts": attempts, "successful_orders": 60,
                      "work_per_order_identical_across_arms": reference,
                      "data_corruptions_refuted": mutations, "input_pins": pins,
                      "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                      "public_status": "not_claimed", "gcp": "not_used"}, indent=2))


if __name__ == "__main__":
    main()
