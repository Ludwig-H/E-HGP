#!/usr/bin/env python3
"""Reader-only JSON-boundary fault injections; not C++ producer mutants."""
from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import sys

import verify


def main() -> None:
    target = str(Path(__file__).resolve().parent)
    original_json = verify.Files.json
    original_argv = sys.argv
    accepted = []

    def nominal(name: str):
        sys.argv = ["verify.py", target]
        with contextlib.redirect_stdout(io.StringIO()) as output:
            verify.main()
        result = json.loads(output.getvalue())
        verify.need(result["status"] == "verified_static_s_factors_closed", "positive_reader")
        accepted.append(name)

    nominal("closed_three_factors")
    cases = [
        ("payload_changed", "stdout", lambda row: row.update(payload_digest="0" * 64),
         "closed_8k_input_and_full_payload"),
        ("contract_promoted", "stdout", lambda row: row.update(contract_qualified=True),
         "bounded_authority"),
        ("static_orders_empty", "stdout", lambda row: row.update(static_orders=[]),
         "all_ten_static_orders"),
        ("MEB_work_zeroed", "stdout", lambda row: row.update(resolver_meb_calls=0),
         "MEB_initial_and_descent_identity"),
        ("sample_not_closed", "receipt.json", lambda row: row.update(status="running"),
         "closed_capture_receipt"),
        ("comparison_lies", "comparison.json", lambda row: row.update(static_orders_matched=False),
         "independent_recomputed_comparison"),
    ]
    rejected = []
    try:
        for name, file_name, inject, expected in cases:
            def changed_json(self, path, requested="n8000_s10_static4/" + file_name,
                             mutate=inject):
                result = original_json(self, path)
                if path == requested:
                    mutate(result)
                return result

            verify.Files.json = changed_json
            sys.argv = ["verify.py", target]
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    verify.main()
            except ValueError as error:
                verify.need(str(error) == expected, "wrong_reader_rejection:" + name + ":" + str(error))
                rejected.append(dict(name=name, reason=str(error)))
            else:
                raise ValueError("reader_fault_survived:" + name)
    finally:
        verify.Files.json = original_json
        sys.argv = original_argv
    print(json.dumps(dict(status="reader_json_boundary_faults_rejected", positive=accepted,
                          rejected=rejected, Cpp_producer_mutants=False, public_status="not_claimed")))


if __name__ == "__main__":
    main()
