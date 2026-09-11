#!/usr/bin/env python3
"""Independent read-only comparison of initial R_K/U_K against static requests."""
import hashlib
import json
from pathlib import Path
import sys
from typing import Any


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise RuntimeError(reason)


def main() -> None:
    here = Path(__file__).resolve().parent
    manifest = json.loads((here / "manifest.json").read_text())
    for name, digest in manifest["files"].items():
        data = (here / name).read_bytes()
        need(hashlib.sha256(data).hexdigest() == digest and not data.startswith(b"\x7fELF"), "file integrity: " + name)

    def js(name: str) -> Any:
        return json.loads((here / name).read_text())

    observed, static = js("observed.stdout"), js("static.stdout")
    for key in ("n", "s", "kmax", "threads", "seed", "coord", "input_digest", "payload_digest", "nodes", "representatives"):
        need(observed[key] == static[key], "paired configuration/payload: " + key)
    need(observed["n"] == 8000 and observed["s"] == 8 and observed["kmax"] == 10 and static["static_threads"] == 4,
         "scope 8k/s8/K10/static4")
    for row in (observed, static):
        need(row["status"] == "completed_relative" and row["public_status"] == "not_claimed" and row["contract_qualified"] is False,
             "relative authority")
    prior = js("observed.receipt.json")
    newer = js("static.receipt.json")
    need(prior["exit_code"] == 0 and prior["sources_stable"] and prior["sources_before"] == prior["sources_after"], "observed closed stable run")
    need(newer["status"] == "completed" and newer["exit_code"] == 0 and newer["sources_and_binary_stable"], "static closed stable run")
    need(js("static.sources_before.json") == js("static.sources_after.json"), "static source stability")
    need((here / "static.binary_before.sha256").read_bytes() == (here / "static.binary_after.sha256").read_bytes(), "static binary stability")
    a = observed["observer"]["rows"]
    b = static["static_orders"]
    need([r["k"] for r in a] == list(range(1, 11)) and [r["K"] for r in b] == list(range(1, 11)), "all ten counters")
    need(a[0]["representative_occurrences"] == 59750 and a[0]["unique_representative_keys"] == 8000,
         "actual K1 representatives are present only in observer")
    need(b[0] == {"K": 1, "requests": 0, "unique": 0, "seeded_unique": 0}, "static K1 intentionally excluded")
    pairs = []
    for x, y in zip(a[1:], b[1:]):
        need(x["k"] == y["K"] and x["representative_occurrences"] == y["requests"] and
             x["unique_representative_keys"] == y["unique"], "per-order exact R/U equality")
        need(0 < y["seeded_unique"] <= y["unique"] < y["requests"], "nonvacuous seed/dedup each order")
        pairs.append({"K": y["K"], "R_observed": x["representative_occurrences"], "R_static": y["requests"],
                      "U_observed": x["unique_representative_keys"], "U_static": y["unique"], "static_seeded_unique": y["seeded_unique"]})
    need(pairs == js("comparison.json")["orders"], "derived comparison replays")
    need(sum(r["R_static"] for r in pairs) == 10396562 and sum(r["U_static"] for r in pairs) == 5176885 and
         sum(r["static_seeded_unique"] for r in pairs) == 2396646, "exact order totals")
    print(json.dumps({"status": "passed", "orders_compared": 9, "R_K2_to_K10": 10396562, "U_K2_to_K10": 5176885,
                      "static_seeded_unique": 2396646, "public_status": "not_claimed", "new_engine_execution": False}))


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print("VERIFY FAILED:", error, file=sys.stderr)
        raise SystemExit(1)
