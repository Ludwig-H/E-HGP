#!/usr/bin/env python3
"""Create-only mechanical adaptation of the closed census packet tools."""
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent / "v7_census_tower_permanent_publish_20260911"


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError("adaptation anchor drift: " + old[:80])
    return text.replace(old, new)


def main():
    publish = (SOURCE / "publish.py").read_text()
    publish = publish.replace("v7_census_tower_permanent_20260911", "v7_batch_permanent_20260911")
    publish = publish.replace("permanent_census_CTest_CPU83f1_without_callback", "permanent_batch_CTest_CPU83f1")
    publish = publish.replace('"callback_supplied": False', '"callback_supplied": True')
    publish = replace_once(publish, '"incidental_pins_no_bytes": incidental,',
                          '"incidental_pins_no_bytes": incidental, "product_instrumentation": False,')
    publish = replace_once(publish, '"scripts/publish.py", Path(__file__).read_bytes())',
                          '"scripts/publish.py", Path(__file__).read_bytes())\n    write(TARGET / "scripts/prepare.py", (HERE / "prepare.py").read_bytes())')
    verify = (SOURCE / "verify.py").read_text()
    verify = verify.replace('TARGET = "mhgp7_census_tower_gate"', 'TARGET = "mhgp7_full_ball_batch_gate"')
    begin = verify.index('CAUSES = {')
    end = verify.index('\n\n\ndef need(', begin)
    verify = verify[:begin] + '''KINDS = ["cpu1", "cpu4", "rejects", "bad_argument", "missing_argument"]
CANDIDATES = ["full_ball_batch_gate.cpp", "full_ball_batch_geometry.hpp", "full_ball_batch_owner.hpp"]
NOMINAL = {"batch_calls": 49, "batch_requests": 103, "direct_terminals": 103,
           "diagnostic_reference_MEB_calls": 119, "payload_checks": 5704, "work_checks": 442,
           "product_instrumentation": False, "status": "passed_batch_callback"}
FULL = {"status": "passed", "checks": 298742, "clouds": 34, "orders": 150,
        "facets": 33870, "cuts": 3916, "vertical_checks": 87230, "same_radius_steps": 4,
        "authority": "bounded_independent_Gram_Gamma_not_WSPD_completeness"}
''' + verify[end:]
    verify = verify.replace("permanent_census_CTest_CPU83f1_without_callback", "permanent_batch_CTest_CPU83f1")
    verify = verify.replace('captured["callback_supplied"] is False', 'captured["callback_supplied"] is True and captured["product_instrumentation"] is False')
    verify = verify.replace('overlay["callback_supplied"] is False', 'overlay["callback_supplied"] is True and overlay["product_instrumentation"] is False')
    verify = verify.replace('overlay["new_helpers"]', 'overlay["new_product_helpers"]')
    verify = verify.replace('origin["source_judge_pins"]', 'origin["source_helper_pins"]')
    verify = verify.replace('for source in ("census_tower_gate.cpp", "census_tower_oracle.hpp"):', 'for source in CANDIDATES:')
    verify = verify.replace('"/tests/census_tower_gate.cpp"', '"/tests/full_ball_batch_gate.cpp"')
    verify = verify.replace('"tests/census_tower_gate.cpp.o.d"', '"tests/full_ball_batch_gate.cpp.o.d"')
    verify = verify.replace('("census_tower_gate.cpp", "census_tower_oracle.hpp", "full_ball_tower_gate.cpp",\n             "full_ball_tower.hpp", "pipeline/generate.hpp")',
                            '("full_ball_batch_gate.cpp", "full_ball_batch_geometry.hpp", "full_ball_batch_owner.hpp",\n             "full_ball_tower_gate.cpp", "full_ball_tower.hpp", "local_plateau_oracle.hpp")')
    verify = verify.replace('"mhgp7_census_tower_"', '"mhgp7_full_ball_batch_"')
    verify = verify.replace('"eleven intended CTests"', '"five intended CTests"')
    verify = verify.replace('len(sections) == 11 and sum("Test Passed." in section for section in sections) == 11, "eleven physical CTest passes"',
                            'len(sections) == 5 and sum("Test Passed." in section for section in sections) == 5, "five physical CTest passes"')
    verify = replace_once(verify, 'expected = 1 if kind in CAUSES else 2 if kind in ("bad_argument", "missing_argument") else 0',
                          'expected = 2 if kind in ("bad_argument", "missing_argument") else 0')
    verify = replace_once(verify, 'argument = "--unknown" if kind == "bad_argument" else "" if kind == "missing_argument" else "--" + kind.replace("_", "-")',
                          'argument = {"cpu1": "--selftest-1", "cpu4": "--selftest-4", "rejects": "--rejects",\n                        "bad_argument": "--unknown", "missing_argument": ""}[kind]')
    verify = replace_once(verify, 'cause = "cause=" + CAUSES[kind] if kind in CAUSES else ""', 'cause = ""')
    begin = verify.index('        need(results[0] == {')
    end = verify.index('        summaries[name] = results', begin)
    verify = verify[:begin] + '''        for result in (results[0], results[2]):
            need(all(result.get(key) == value for key, value in FULL.items()), "independent FULL physical results")
        need(results[1] == {**NOMINAL, "workers": 1} and results[3] == {**NOMINAL, "workers": 4},
             "two actual callbacks direct terminal and work nonvacuities")
        need(results[4] == {"status": "passed_batch_rejections", "cases": 65, "prefix_cases": 32,
             "known_paid": 44, "product_instrumentation": False}, "actual 65 callback refusals")
        need(not any("MHGP7_TESTING" in arg for arg in args), "no product testing macro")
''' + verify[end:]
    begin = verify.index('    return {"status": "passed", "captures": 2,')
    end = verify.index('\n\n\ndef main():', begin)
    verify = verify[:begin] + '''    need(not incidental_seen, "no captured bytecode")
    return {"status": "passed", "captures": 2, "commands": 10, "CTests_each": 5,
            "FULL_orders_per_worker": 150, "FULL_clouds_per_worker": 34,
            "batch_calls_per_worker": 49, "direct_terminals_per_worker": 103,
            "diagnostic_reference_MEB_calls_per_worker": 119,
            "rejections_each": 65, "prefix_rejections_each": 32, "known_paid_rejections_each": 44,
            "incidental_bytecode_pins": 0, "callback_supplied": True, "product_instrumentation": False,
            "geometry_executed_by_reader": False, "device_executed": False, "gcp_used": False}
''' + verify[end:]
    for name, data in (("publish.py", publish), ("verify.py", verify)):
        with (HERE / name).open("x") as output:
            output.write(data)


if __name__ == "__main__":
    main()
