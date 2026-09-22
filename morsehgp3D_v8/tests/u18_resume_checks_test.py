#!/usr/bin/env python3
"""Pure simulated-build tests: no compiler, CMake, CTest or pinned build launch."""
from __future__ import annotations

import base64
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
import xml.etree.ElementTree as ET

BENCH = Path(__file__).resolve().parents[1] / "bench"
sys.path.insert(0, str(BENCH))
spec = importlib.util.spec_from_file_location("u18_resume_runner_under_test", BENCH / "run_u18_resume_checks.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
import run_q34_affine_checks as affine


def record(command, payload=""):
    return dict(command=command, exit_code=0, stdout=payload, stderr="",
                stdout_base64=base64.b64encode(payload.encode()).decode(), stderr_base64="")


def scale_row(n):
    return dict(schema="mhgp8_q4_saturating_atlas_scale_v1",
        scope="one_supplied_edge_empty_q4_stream_not_pipeline", n=n, kmax=10, outputs=0,
        prepare_ms=1,baseline_ms=1,saturated_ms=0.5,baseline_tests=12,saturated_total_tests=4,
        baseline_node_visits=12,saturated_total_node_visits=4,baseline_frontier_copies=0,
        saturated_total_frontier_copies=0,unvisited_site_mass=n-12,certificates=1,
        baseline_peak_build_bytes=128,saturated_peak_build_bytes=96)


class ReceiptTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mhgp8-u18-reader-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.build, self.capture = self.root / "build", self.root / "capture"
        self.build.mkdir(); self.capture.mkdir()
        self.compiler = self.root / "c++"
        self.source, self.header = self.root / "source.cpp", self.root / "header.hpp"
        self.compiler.write_text("simulated compiler v1")
        self.source.write_text("// simulated source")
        self.header.write_text("// simulated header")
        self.scan = self.capture / "dependencies"; self.scan.mkdir()
        self.dep_dir = self.build / "CMakeFiles/target.dir"; self.dep_dir.mkdir(parents=True)
        for name in ("CMakeCache.txt", "CTestTestfile.cmake", "mhgp8_fake_gate", "libmhgp8_p0.a"):
            (self.build / name).write_text(name)
        for name in ("flags.make", "link.txt"):
            (self.dep_dir / name).write_text(name)
        entry = dict(directory=str(self.build), file=str(self.source),
                     arguments=[str(self.compiler), "-std=c++20", "-O3", "-MD", "-MF", "old.d",
                                "-MT", "old.o", "-o", "source.o", "-c", str(self.source)])
        runner.write_json(self.build / "compile_commands.json", [entry])
        dep_text = f"target: {self.source} \\\n {self.header}\n"
        (self.dep_dir / "source.cpp.o.d").write_text(dep_text)
        (self.scan / "unit_0000.d").write_text(dep_text)
        plan = runner.scan_plan(self.build, self.compiler, self.scan)
        scanned = record(plan[0]["command"])
        scanned.update(cwd=plan[0]["cwd"], source=plan[0]["source"])
        runner.write_json(self.scan / "unit_0000.json", scanned)
        scan_state = dict(status="passed", compiler_sha256=runner.sha(self.compiler),
                          compile_commands_sha256=runner.sha(self.build / "compile_commands.json"),
                          commands=[dict(name="unit_0000", sha256=runner.sha(self.scan / "unit_0000.json"))],
                          dependencies=runner.pins([self.source,self.header]),
                          artifacts=runner.pins(self.scan.iterdir()))
        runner.write_json(self.scan / "SCAN.json", scan_state)
        self.names = ["mhgp8_u18_numeric_domain_gate", "mhgp8_q4_saturating_atlas_gate"] + [f"old_{i}" for i in range(130)] + sorted(runner.DISABLED)
        suite = ET.Element("testsuite")
        for name in self.names:
            case = ET.SubElement(suite, "testcase", name=name, status="notrun" if name in runner.DISABLED else "run")
            if name in runner.DISABLED:
                ET.SubElement(case, "skipped", message="known authority exclusion")
        ET.ElementTree(suite).write(self.capture / "CTEST.xml")
        commands = runner.command_plan(self.build,self.capture,self.compiler,False,2,self.root)
        manifest = dict(schema=runner.SCHEMA, public_status="not_claimed", gcp_used=False,
                        source_sha256=runner.pins([self.source]), compiler=str(self.compiler),
                        compiler_sha256=runner.sha(self.compiler), build=str(self.build), plan=commands,
                        sanitize=False,jobs=2,boost=str(self.root))
        runner.write_json(self.capture / "MANIFEST.json", manifest)
        records = []
        for name, command in commands:
            payload = json.dumps(dict(tests=[dict(name=x) for x in self.names])) if name == "inventory" else "compiler v1\n"
            if name.startswith("scale_"): payload=json.dumps(scale_row(int(command[-1])))
            runner.write_json(self.capture / (name + ".json"), record(command, payload))
            records.append(dict(name=name, path=name+".json", sha256=runner.sha(self.capture / (name + ".json"))))
        state = dict(schema=runner.SCHEMA, status="passed", manifest_sha256=runner.sha(self.capture / "MANIFEST.json"),
                     commands=records, compiled=runner.compiled(self.build), test_names=self.names,
                     precompiled_dependencies=scan_state["dependencies"], dependency_scan_sha256=runner.sha(self.scan / "SCAN.json"),
                     ctest_sha256=runner.sha(self.capture / "CTEST.xml"),
                     ctest=runner.judge_xml(self.capture / "CTEST.xml", self.names),
                     scales={f"scale_{n}":scale_row(n) for n in (8000,16000,32000)})
        runner.write_json(self.capture / "COMPLETION.json", state)
        patcher = mock.patch.object(runner, "sources", return_value={self.source})
        patcher.start(); self.addCleanup(patcher.stop)

    def state_change(self, change):
        path=self.capture / "COMPLETION.json"
        state=runner.load(path); change(state); runner.write_json(path,state)

    def assert_rejected(self):
        with self.assertRaises(runner.require.__globals__["InvalidReceipt"]):
            runner.read(self.capture)

    def test_simulated_baseline_passes(self):
        self.assertEqual(runner.read(self.capture)["passed"],132)

    def test_sanitizer_exclusions_are_explicit(self):
        path = self.capture / "CTEST.xml"
        tree = ET.parse(path)
        names = list(self.names)
        for name in sorted(runner.SAN_DISABLED - runner.DISABLED):
            names.append(name)
            case = ET.SubElement(tree.getroot(), "testcase", name=name, status="notrun")
            ET.SubElement(case, "skipped", message="Release-only mutation linker")
        tree.write(path)
        self.assertEqual(runner.judge_xml(path,names,True)["passed"],132)
        with self.assertRaises(runner.require.__globals__["InvalidReceipt"]):
            runner.judge_xml(path,names,False)

    def test_compiler_change_rejected(self):
        self.compiler.write_text("different compiler")
        self.assert_rejected()

    def test_dropped_output_inventory_rejected(self):
        self.state_change(lambda state: state["compiled"].update(outputs={}))
        self.assert_rejected()

    def test_dropped_dependency_inventory_rejected(self):
        self.state_change(lambda state: state["compiled"].update(dependencies={}))
        self.assert_rejected()

    def test_extra_output_rejected(self):
        (self.build / "mhgp8_unrecorded").write_text("new binary")
        self.assert_rejected()

    def test_changed_header_rejected(self):
        self.header.write_text("changed header")
        self.assert_rejected()

    def test_source_inventory_omission_rejected(self):
        path=self.capture / "MANIFEST.json"
        manifest=runner.load(path); manifest["source_sha256"]={}; runner.write_json(path,manifest)
        self.state_change(lambda state: state.update(manifest_sha256=runner.sha(path)))
        self.assert_rejected()

    def test_duplicate_xml_name_rejected_even_rehashed(self):
        path=self.capture / "CTEST.xml"
        tree=ET.parse(path); tree.findall(".//testcase")[-1].set("name",self.names[-2]); tree.write(path)
        self.state_change(lambda state: state.update(ctest_sha256=runner.sha(path)))
        self.assert_rejected()

    def test_xml_renamed_test_rejected_even_rehashed(self):
        path=self.capture / "CTEST.xml"
        tree=ET.parse(path); tree.findall(".//testcase")[-1].set("name","unregistered"); tree.write(path)
        self.state_change(lambda state: state.update(ctest_sha256=runner.sha(path)))
        self.assert_rejected()

    def test_failed_xml_rejected_even_rehashed(self):
        path=self.capture / "CTEST.xml"
        tree=ET.parse(path); ET.SubElement(tree.findall(".//testcase")[0],"failure"); tree.write(path)
        self.state_change(lambda state: state.update(ctest_sha256=runner.sha(path)))
        self.assert_rejected()

    def test_precompile_inventory_omission_rejected(self):
        self.state_change(lambda state: state.update(precompiled_dependencies={}))
        self.assert_rejected()

    def test_ctest_command_omission_rejected_even_rehashed(self):
        path=self.capture / "MANIFEST.json"
        manifest=runner.load(path); manifest["plan"]=[p for p in manifest["plan"] if p[0]!="ctest"]
        runner.write_json(path,manifest)
        def mutate(state):
            state["manifest_sha256"]=runner.sha(path)
            state["commands"]=[p for p in state["commands"] if p["name"]!="ctest"]
        self.state_change(mutate); self.assert_rejected()

    def test_scale_summary_corruption_rejected(self):
        self.state_change(lambda state: state["scales"]["scale_8000"].update(outputs=1))
        self.assert_rejected()

    def test_scale_wrong_n_and_bool_rejected(self):
        invalid=runner.require.__globals__["InvalidReceipt"]
        for field,value in (("n",8001),("outputs",False),("certificates",0),("baseline_ms",float("nan"))):
            row=scale_row(8000); row[field]=value
            with self.assertRaises(invalid): runner.judge_scale(row,8000)

    def test_scan_unit_omission_rejected_even_rehashed(self):
        path=self.scan / "SCAN.json"
        state=runner.load(path); state["commands"]=[]; runner.write_json(path,state)
        self.state_change(lambda state: state.update(dependency_scan_sha256=runner.sha(path)))
        self.assert_rejected()

    def test_compiler_change_during_read_rejected(self):
        original=runner.judge_xml
        def changing(*args):
            result=original(*args); self.compiler.write_text("changed during replay"); return result
        with mock.patch.object(runner,"judge_xml",side_effect=changing): self.assert_rejected()

    def test_raw_proof_change_during_read_rejected(self):
        original=runner.judge_xml
        def changing(*args):
            result=original(*args); (self.capture / "compiler.json").write_text("changed during replay"); return result
        with mock.patch.object(runner,"judge_xml",side_effect=changing): self.assert_rejected()

    def test_scan_command_strips_object_and_dep_output(self):
        command=runner.scan_plan(self.build,self.compiler,self.scan)[0]["command"]
        self.assertNotIn("-c",command); self.assertNotIn("-o",command); self.assertNotIn("-MD",command)
        self.assertEqual(command.count("-MF"),1); self.assertEqual(command.count("-MT"),1)
        self.assertIn(str(self.source),command); self.assertIn("-M",command)

    def test_scan_rejects_different_driver(self):
        entry=json.loads((self.build / "compile_commands.json").read_text())[0]
        entry["arguments"][0]=str(self.source)
        runner.write_json(self.build / "compile_commands.json",[entry])
        with self.assertRaises(runner.require.__globals__["InvalidReceipt"]):
            runner.scan_plan(self.build,self.compiler,self.scan)

    def test_failed_preprocessor_raw_is_preserved(self):
        output=self.root / "failed_scan"
        def fail(command,_environment,_cwd,result):
            result.update(record(command,"partial preprocessing output")); result["exit_code"]=7
        with mock.patch.object(runner,"invoke",side_effect=fail):
            with self.assertRaises(runner.require.__globals__["InvalidReceipt"]):
                runner.scan(self.build,self.compiler,output)
        state=runner.load(output / "SCAN.json")
        self.assertEqual(state["status"],"failed")
        self.assertEqual(runner.load(output / "unit_0000.json")["exit_code"],7)
        self.assertEqual(runner.load(output / "unit_0000.json")["stdout"],"partial preprocessing output")


class AffineReaderTests(unittest.TestCase):
    """Synthetic parser fixtures, not historical/native qualifications."""
    def row(self, u18=True):
        value={field:1 for field in (*affine.base.SEARCH_GATE_FIELDS,*affine.SEARCH_GATE_ADDED)}
        value.update(schema="mhgp8_q34_witness_search_gate_v1",status="PASS",parallel_calls=4,
            peak_stack=55 if u18 else 49,extreme_queries=15,invalid_inputs=11,
            mode_parallel_calls=4,mode_invalid_inputs=2,local_exclusion_cases=2,
            nonpositive_minimum_cases=2,mixed_terminal_cases=2,bounds_calls=3,
            mode_singletons=1,mode_rectangles=2,singleton_queries=20,wide_predicate_cases=30,oracle_sites=40)
        if u18: value.update(extreme18_queries=15,wide18_predicate_cases=20)
        return value

    def validate(self,row):
        affine.validate_gate(row,"mhgp8_q34_witness_search_gate")

    def test_affine_two_exact_inventories(self):
        self.validate(self.row()); self.validate(self.row(False))

    def test_affine_missing_u18_fields_rejected(self):
        for removed in (("extreme18_queries",),("wide18_predicate_cases",),affine.SEARCH_GATE_U18_ADDED):
            row=self.row()
            for field in removed: del row[field]
            with self.assertRaises(affine.InvalidReceipt): self.validate(row)

    def test_affine_u18_floors_and_types_rejected(self):
        for field in affine.SEARCH_GATE_U18_ADDED:
            for value in (0,-1,True,1.0,"15",1<<64):
                row=self.row(); row[field]=value
                with self.assertRaises(affine.InvalidReceipt): self.validate(row)
        for field,value in (("extreme18_queries",14),("wide18_predicate_cases",31),
                            ("oracle_sites",29),("singleton_queries",14)):
            row=self.row(); row[field]=value
            with self.assertRaises(affine.InvalidReceipt): self.validate(row)

    def test_affine_width_inventory_cannot_mix(self):
        for u18,stack in ((False,55),(True,49)):
            row=self.row(u18); row["peak_stack"]=stack
            with self.assertRaises(affine.InvalidReceipt): self.validate(row)

    def test_affine_unknown_field_rejected(self):
        row=self.row(); row["unknown18"]=1
        with self.assertRaises(affine.InvalidReceipt): self.validate(row)

    def pair_row(self,u18=True):
        row={field:1 for field in affine.PAIR_GATE_FIELDS}
        n=320 if u18 else 268
        row.update(schema="mhgp8_q34_pair_bounds_gate_v1",status="PASS",queries=n,corner_evaluations=8*n,
            sample_evaluations=27*n,singleton_boxes=72,nondegenerate_boxes=n-72,reversed_pairs=n,
            extreme_cases=50,invalid_inputs=6,parallel_calls=4,wide_xi=72,wide_h_squares=98)
        if u18: row.update(extreme_cases_u18=52,wide_xi_u18=42,wide_h_squares_u18=48)
        return row

    def ball_row(self,u18=True):
        row={field:1 for field in affine.base.BALL_GATE_FIELDS}
        row.update(schema="mhgp8_q3_ball_census_gate_v1",status="PASS",calls=2,max_shell=30,
            singleton_cases=4 if u18 else 3,extreme_cases=5,wide_linear_squares=3,parallel_calls=4)
        if u18: row.update(wide_linear_squares_u18=3,extreme_cases_u18=5)
        return row

    def global_row(self,extended=True):
        row={field:1 for field in (*affine.edge.GLOBAL_GATE_FIELDS,*affine.base.GLOBAL_GATE_ADDED,*affine.GLOBAL_GATE_ADDED)}
        row.update(schema="mhgp8_wspd_q34_gate_v1",status="PASS",candidates=2,max_shell=30,
            allocation_failures=4,parallel_calls=4,parallel_join_checks=2,parallel_empty_calls=8,
            bounds_parallel_calls=24,bounds_invalid_inputs=4,bounds_inactive_calls=6,bounds_callback_failures=2,
            bounds_allocation_failures=4,bounds_shared_calls=4,bounds_mode_calls=2,bounds_work_checks=2)
        if extended:
            row.update(seed_cell_calls=10,seed_cell_live_calls=5,seed_cell_joined_calls=5,seed_cell_work_checks=10,
                seed_cell_parallel_calls=12,seed_cell_inactive_calls=6,seed_cell_invalid_inputs=12,
                seed_cell_callback_failures=2,seed_cell_allocation_failures=4,seed_cell_shared_calls=4,
                seed_cell_owner_resets=1,seed_cell_parallel_failures=1,atlas_rejections=1,atlas_lane_skips=0,
                atlas_locations=2,atlas_outside=1,task_sharing_calls=1,task_ranges=1,task_splits=1,task_refusals=1)
        return row

    def test_other_affine_gate_historical_and_current_inventories(self):
        for factory,name in ((self.pair_row,"q34_pair_bounds"),(self.ball_row,"q3_ball_census"),
                             (self.global_row,"wspd_q34")):
            for extended in (False,True): affine.validate_gate(factory(extended),f"mhgp8_{name}_gate")

    def test_other_affine_gate_new_fields_nonvacuous(self):
        groups=((self.pair_row,"q34_pair_bounds",affine.PAIR_GATE_U18_ADDED),
                (self.ball_row,"q3_ball_census",affine.base.BALL_GATE_U18_ADDED),
                (self.global_row,"wspd_q34",[*affine.GLOBAL_SEED_CELL_ADDED,*affine.GLOBAL_ATLAS_TASK_ADDED]))
        for factory,name,fields in groups:
            for field in fields:
                for value in ((True,-1) if field=="atlas_lane_skips" else (0,True,-1)):
                    row=factory(); row[field]=value
                    with self.assertRaises(affine.InvalidReceipt): affine.validate_gate(row,f"mhgp8_{name}_gate")
                row=factory(); del row[field]
                with self.assertRaises(affine.InvalidReceipt): affine.validate_gate(row,f"mhgp8_{name}_gate")

    def test_other_affine_gate_counts_cannot_silently_downgrade(self):
        for factory,name,fields in ((self.pair_row,"q34_pair_bounds",affine.PAIR_GATE_U18_ADDED),
                                   (self.ball_row,"q3_ball_census",affine.base.BALL_GATE_U18_ADDED)):
            row=factory()
            for field in fields: del row[field]
            with self.assertRaises(affine.InvalidReceipt): affine.validate_gate(row,f"mhgp8_{name}_gate")

    def test_other_affine_gate_fixture_counts_rejected(self):
        for factory,name,field,value in ((self.pair_row,"q34_pair_bounds","extreme_cases_u18",51),
            (self.pair_row,"q34_pair_bounds","wide_xi_u18",73),
            (self.ball_row,"q3_ball_census","wide_linear_squares_u18",2),
            (self.ball_row,"q3_ball_census","extreme_cases_u18",4),
            (self.global_row,"wspd_q34","atlas_locations",1),
            (self.global_row,"wspd_q34","task_splits",2),
            (self.global_row,"wspd_q34","seed_cell_calls",9)):
            row=factory(); row[field]=value
            with self.assertRaises(affine.InvalidReceipt): affine.validate_gate(row,f"mhgp8_{name}_gate")


if __name__ == "__main__":
    suite=unittest.TestSuite(unittest.defaultTestLoader.loadTestsFromTestCase(case)
                             for case in (ReceiptTests,AffineReaderTests))
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    print(json.dumps(dict(schema="mhgp8_u18_resume_checks_test_v1",status="passed" if result.wasSuccessful() else "failed",
                          tests=result.testsRun,failures=len(result.failures),errors=len(result.errors),optimized=not __debug__),sort_keys=True))
    raise SystemExit(0 if result.wasSuccessful() else 1)
