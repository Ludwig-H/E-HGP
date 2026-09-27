"""Synthetic scheduler gates: no actual process, native export or 34-scene run.

Popen, process groups and computation are mocked at their ownership boundary.
Temporary ledgers are explicitly synthetic and never scientific receipts.
"""
from contextlib import ExitStack, contextmanager, redirect_stdout
from copy import deepcopy
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import run as scheduler


def write(path,value):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(value,sort_keys=True)+'\n')


def rows(case):
    result = []
    for minimum in (20,50):
        for exponent in (1,2):
            for method in scheduler.PLAN['quality']['methods']:
                if method != 'hdbscan_standard' or exponent == 1:
                    result.append(dict(case=case['id'],n=case['n'],k=5,min_cluster_size=minimum,
                                       exp_z=exponent,method=method,synthetic_test_only=True))
    return result


def prepared_fixture(root):
    source = root/'fixture_source'; source.write_text('synthetic source pin\n')
    qualification_pin = root/'qualification_source'; qualification_pin.write_text('synthetic qualification pin\n')
    native = root/'not_a_native_binary'; native.write_text('synthetic not executable\n')
    plan_path = root/'plan.json'; write(plan_path,scheduler.PLAN)
    manifest = dict(schema='mhgp9_synthetic_cluster_input_manifest_v1',plan=deepcopy(scheduler.PLAN),
                    plan_sha256=scheduler.sha(plan_path),cases=[])
    for expected in scheduler.PLAN['cases']:
        case = deepcopy(expected); case.update(n=case['spec']['n'],groups=case['spec']['groups'])
        names = dict(original_npy='original.npy',points_npy='points.npy',points_u32le='points.u32le',
                     labels_json='labels.json',parameters_json='parameters.json')
        case['files'] = {}
        for key,filename in names.items():
            path = root/case['id']/filename
            write(path,{'synthetic_test_only':True,'role':key})
            case[key] = str(path); case['files'][str(path)] = scheduler.sha(path)
        case['prepared_sha256'] = {key:case['files'][case[key]] for key in ('points_u32le','points_npy','labels_json')}
        write(root/case['id']/'case.json',case)
        manifest['cases'].append(case)
    manifest_path = root/'manifest.json'; write(manifest_path,manifest)
    pins = {str(source):scheduler.sha(source)}
    receipt = dict(schema='mhgp9_synthetic_preparation_receipt_v1',status='completed',
                   manifest=str(manifest_path),manifest_sha256=scheduler.sha(manifest_path),
                   sources_before=pins,sources_after=pins)
    write(root/'receipt.json',receipt)
    return manifest_path,manifest,source,qualification_pin,native


class Harness:
    def __init__(self,root,**options):
        self.root = root; self.output = root/'capture'; self.options = options
        self.source = root/'runtime_source'; self.source.write_text('synthetic source\n')
        self.pins = {str(self.source):scheduler.sha(self.source)}
        self.manifest = dict(cases=[dict(deepcopy(case),n=case['spec']['n'],files={}) for case in scheduler.PLAN['cases']])
        self.manifest_path = root/'manifest.json'; write(self.manifest_path,self.manifest)
        self.started=[]; self.closed=[]; self.live=set(); self.maximum_live=0; self.streams=[]
        self.interrupted=False; self.source_calls=0

    def sources(self):
        self.source_calls += 1
        if self.options.get('source_closure_raises') and self.source_calls > 1:
            raise OSError('synthetic source disappeared at closure')
        if self.options.get('source_closure_changes') and self.source_calls > 1:
            return {str(self.source):'changed'}
        return dict(self.pins)

    def popen(self,argv,**kwargs):
        index = len(self.started)
        self.streams.extend((kwargs['stdout'],kwargs['stderr']))
        if self.options.get('launch_fails_at') == index:
            raise OSError('synthetic Popen launch error')
        if kwargs['start_new_session'] is not True:
            raise AssertionError('worker must own its new process group')
        if not all(kwargs['env'][key] == value for key,value in scheduler.THREAD_ENV.items()):
            raise AssertionError('thread environment')
        spec = scheduler.read(Path(argv[-1])); case = spec['case']
        folder = Path(spec['output']); folder.mkdir()
        unit = dict(schema='mhgp9_synthetic_clustering_unit_v1',status='completed',case=case['id'],
                    n=case['n'],k=5,rows=rows(case),sources_before=self.pins,sources_after=self.pins,
                    artifacts={},synthetic_test_only=True)
        if index == self.options.get('unit_wrong_case_at'):
            unit['case'] = 'wrong-owned-case'
        if index == self.options.get('unit_missing_row_at'):
            unit['rows'].pop()
        if index == self.options.get('unit_bad_grid_at'):
            unit['rows'][-1] = deepcopy(unit['rows'][0])
        if index == self.options.get('unit_failed_at'):
            unit['status'] = 'failed'; unit['error'] = 'synthetic worker receipt failure'
        write(folder/'receipt.json',unit)
        kwargs['stdout'].write(b'synthetic worker output, not scientific evidence\n')
        process = FakeProcess(self,index,case['id'])
        self.started.append(process); self.live.add(process.pid)
        self.maximum_live = max(self.maximum_live,len(self.live))
        return process

    def close_group(self,process):
        self.closed.append(process.pid)
        if self.options.get('cleanup_fails_at') == process.index:
            raise OSError('synthetic cleanup error')
        self.live.discard(process.pid)
        if process.returncode is None:
            process.returncode = -2
        return dict(pgid=process.pid,status='closed',returncode=process.returncode,signals=[],
                    residual_group_before_cleanup=False)

    @contextmanager
    def deferred(self):
        yield
        if self.options.get('interrupt_on_deferred_exit') and not self.interrupted:
            self.interrupted = True
            raise KeyboardInterrupt('synthetic deferred interrupt after Popen registration')

    def run(self,workers=2):
        with ExitStack() as stack:
            stack.enter_context(patch.object(scheduler,'sources',side_effect=self.sources))
            stack.enter_context(patch.object(scheduler,'preflight',return_value=(self.manifest,self.pins)))
            stack.enter_context(patch.object(scheduler,'enable_subreaper'))
            stack.enter_context(patch.object(scheduler,'defer_signals',side_effect=self.deferred))
            stack.enter_context(patch.object(scheduler,'close_group',side_effect=self.close_group))
            stack.enter_context(patch.object(scheduler.subprocess,'Popen',side_effect=self.popen))
            stack.enter_context(patch.object(scheduler.time,'sleep'))
            stack.enter_context(redirect_stdout(io.StringIO()))
            scheduler.run(self.manifest_path,self.output,workers)


class FakeProcess:
    def __init__(self,harness,index,case):
        self.harness,self.index,self.case = harness,index,case
        self.pid = 800000+index; self.returncode = None; self.polls = 0

    def poll(self):
        self.polls += 1
        if self.harness.options.get('interrupt_poll_at') == self.index and not self.harness.interrupted:
            self.harness.interrupted = True
            raise KeyboardInterrupt('synthetic first interruption')
        if self.index == 0 and self.polls == 1 and self.harness.options.get('out_of_order'):
            return None
        self.returncode = 7 if self.harness.options.get('worker_fails_at') == self.index else 0
        return self.returncode

    def wait(self):
        if self.returncode is None:
            self.returncode = 7 if self.harness.options.get('worker_fails_at') == self.index else 0
        return self.returncode


class RunTests(unittest.TestCase):
    def test_grid_612_34_quality_no_growth(self):
        grid = scheduler.expected_grid()
        self.assertEqual(len(grid),612)
        quality = [case for case in scheduler.PLAN['cases'] if case['phase'] == 'quality']
        self.assertEqual(len(quality),34)
        self.assertEqual({row[0] for row in grid},{case['id'] for case in quality})
        self.assertEqual(sum(row[-1] == 'hdbscan_standard' for row in grid),68)
        self.assertTrue(all(row[3] == 1 for row in grid if row[-1] == 'hdbscan_standard'))
        self.assertEqual(len([case for case in scheduler.PLAN['cases'] if case['phase'] == 'growth']),9)

    def test_sources_exclude_new_tests(self):
        pins = scheduler.sources()
        self.assertNotIn(str(Path(__file__).resolve()),pins)
        for name in ('run.py','unit_pipeline.py','prepare.py','plan.py','synthetic_data.py','PLAN.md'):
            self.assertIn(str(scheduler.HERE/name),pins)

    def test_preflight_live_pins_and_mutations(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-preflight-') as directory:
            root=Path(directory); path,manifest,source,qualified,native=prepared_fixture(root)
            with patch.object(scheduler,'validate_qualification',return_value={str(qualified):scheduler.sha(qualified)}),\
                 patch.object(scheduler,'NATIVE',native),patch.object(scheduler,'NATIVE_SHA',scheduler.sha(native)):
                returned,pins = scheduler.preflight(path)
                self.assertEqual(returned,manifest)
                self.assertEqual(pins[str(source)],scheduler.sha(source))
                self.assertIn(str(native),pins)
                self.assertEqual(len([case for case in returned['cases'] if case['phase']=='quality']),34)
                source.write_text('changed')
                with self.assertRaisesRegex(ValueError,'LIVE pin changed'):
                    scheduler.preflight(path)

    def test_preflight_refuses_divergent_declared_spec(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-plan-mutant-') as directory:
            root=Path(directory); path,manifest,source,qualified,native=prepared_fixture(root)
            manifest['cases'][0]['spec']['separation'] = 999
            write(root/manifest['cases'][0]['id']/'case.json',manifest['cases'][0]);write(path,manifest)
            receipt=scheduler.read(root/'receipt.json');receipt['manifest_sha256']=scheduler.sha(path);write(root/'receipt.json',receipt)
            with patch.object(scheduler,'validate_qualification',return_value={str(qualified):scheduler.sha(qualified)}),\
                 patch.object(scheduler,'NATIVE',native),patch.object(scheduler,'NATIVE_SHA',scheduler.sha(native)):
                with self.assertRaises(ValueError):
                    scheduler.preflight(path)

    def test_worker_uses_prepared_pin_alias_without_mutating_case(self):
        import unit_pipeline
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-worker-boundary-') as directory:
            root=Path(directory);_,manifest,source,qualified,native=prepared_fixture(root)
            case=manifest['cases'][0]; before=deepcopy(case)
            source_pins={str(source):scheduler.sha(source)}
            spec_path=root/'worker.spec.json'
            write(spec_path,dict(case=case,output=str(root/'unit'),native_binary=str(native),source_pins=source_pins))

            def compute(received,output,binary):
                self.assertEqual(received['prepared_sha256'],
                    {key:case['files'][case[key]] for key in ('points_u32le','points_npy','labels_json')})
                self.assertEqual(received['id'],case['id'])
                self.assertEqual(received['spec'],case['spec'])
                self.assertEqual(output,root/'unit');self.assertEqual(binary,native)
                return dict(schema=unit_pipeline.SCHEMA,status='completed',case=case['id'],n=case['n'],k=5,rows=rows(case))

            with patch.object(unit_pipeline,'run_unit',side_effect=compute) as invoked,\
                 patch.object(scheduler,'NATIVE_SHA',scheduler.sha(native)):
                scheduler.worker(spec_path)
                self.assertEqual(invoked.call_count,1)
            self.assertEqual(case,before)
            self.assertEqual(scheduler.read(spec_path)['case'],before)

    def test_success_ledger_all_tasks_two_workers_out_of_order(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-scheduler-') as directory:
            harness=Harness(Path(directory),out_of_order=True);harness.run()
            receipt=scheduler.read(harness.output/'receipt.json')
            self.assertEqual(receipt['status'],'completed')
            self.assertEqual(len(receipt['rows']),612);self.assertEqual(len(receipt['units']),34)
            self.assertEqual(len(receipt['worker_commands']),34)
            self.assertEqual(len(receipt['events']),68);self.assertEqual(len(receipt['cleanup']),34)
            self.assertEqual(harness.maximum_live,2);self.assertEqual(harness.live,set())
            expected=[case['id'] for case in harness.manifest['cases'] if case['phase']=='quality']
            self.assertEqual([unit['case'] for unit in receipt['units']],expected)
            self.assertEqual(receipt['worker_commands'][0]['case'],expected[1])
            self.assertFalse(receipt['growth_executed'])
            self.assertTrue(all(stream.closed for stream in harness.streams))
            scheduler.check_pins(receipt['artifacts'])

    def test_worker_failure_stops_new_launches_closes_other_group(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-worker-fail-') as directory:
            harness=Harness(Path(directory),worker_fails_at=0)
            with self.assertRaisesRegex(ValueError,'worker failed'):
                harness.run()
            receipt=scheduler.read(harness.output/'receipt.json')
            self.assertEqual(receipt['status'],'failed');self.assertEqual(len(harness.started),2)
            self.assertEqual(receipt['rows'],[]);self.assertEqual(receipt['units'],[])
            self.assertEqual(receipt['worker_commands'][0]['returncode'],7)
            self.assertEqual(set(harness.closed),{process.pid for process in harness.started})
            self.assertEqual(harness.live,set());self.assertTrue(all(stream.closed for stream in harness.streams))

    def test_interrupt_after_registration_is_owned(self):
        for options in (dict(interrupt_poll_at=0),dict(interrupt_on_deferred_exit=True)):
            with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-interrupt-') as directory:
                harness=Harness(Path(directory),**options)
                with self.assertRaises(KeyboardInterrupt):
                    harness.run()
                receipt=scheduler.read(harness.output/'receipt.json')
                self.assertEqual(receipt['status'],'failed');self.assertIn('KeyboardInterrupt',receipt['error'])
                self.assertEqual(set(harness.closed),{process.pid for process in harness.started})
                self.assertEqual(harness.live,set());self.assertTrue(all(stream.closed for stream in harness.streams))

    def test_cleanup_failure_preserves_first_interrupt_and_closes_others(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-cleanup-fail-') as directory:
            harness=Harness(Path(directory),interrupt_poll_at=0,cleanup_fails_at=0)
            with self.assertRaisesRegex(KeyboardInterrupt,'first interruption'):
                harness.run()
            receipt=scheduler.read(harness.output/'receipt.json')
            self.assertIn('first interruption',receipt['error'])
            self.assertEqual(receipt['status'],'failed');self.assertEqual(len(receipt['cleanup_errors']),1)
            self.assertEqual(set(harness.closed),{process.pid for process in harness.started})
            self.assertTrue(all(stream.closed for stream in harness.streams))

    def test_source_hash_exception_preserves_first_worker_failure(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-source-fail-') as directory:
            harness=Harness(Path(directory),worker_fails_at=0,source_closure_raises=True)
            with self.assertRaisesRegex(ValueError,'worker failed'):
                harness.run()
            receipt=scheduler.read(harness.output/'receipt.json')
            self.assertEqual(receipt['status'],'failed');self.assertIn('worker failed',receipt['error'])
            self.assertTrue('closure_errors' in receipt or 'closure_error' in receipt)

    def test_unit_schema_case_and_grid_refusals(self):
        for option in ('unit_wrong_case_at','unit_missing_row_at','unit_bad_grid_at','unit_failed_at'):
            with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-unit-mutant-') as directory:
                harness=Harness(Path(directory),**{option:0})
                with self.assertRaises(ValueError):
                    harness.run()
                receipt=scheduler.read(harness.output/'receipt.json')
                self.assertEqual(receipt['status'],'failed')
                self.assertEqual(receipt['units'],[])
                self.assertEqual(len(harness.started),2)

    def test_launch_failure_and_no_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-launch-fail-') as directory:
            harness=Harness(Path(directory),launch_fails_at=1)
            with self.assertRaisesRegex(OSError,'Popen launch error'):
                harness.run()
            receipt=scheduler.read(harness.output/'receipt.json')
            self.assertEqual(receipt['status'],'failed');self.assertEqual(harness.live,set())
            self.assertTrue(all(stream.closed for stream in harness.streams))
            before=(harness.output/'receipt.json').read_bytes()
            with self.assertRaisesRegex(ValueError,'NEW'):
                harness.run()
            self.assertEqual((harness.output/'receipt.json').read_bytes(),before)

    def test_source_closure_change_refuses_success_and_worker_limits(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-closure-') as directory:
            harness=Harness(Path(directory),source_closure_changes=True)
            with self.assertRaises(ValueError):
                harness.run()
            receipt=scheduler.read(harness.output/'receipt.json')
            self.assertEqual(receipt['status'],'failed');self.assertEqual(len(receipt['rows']),612)
            self.assertTrue('closure_errors' in receipt or 'closure_error' in receipt,'closure refusal recorded')
        for workers in (0,3,True,1.0):
            with tempfile.TemporaryDirectory(prefix='mhgp9-synthetic-worker-count-') as directory:
                harness=Harness(Path(directory))
                with self.assertRaises(ValueError):
                    harness.run(workers)
                self.assertFalse(harness.output.exists())


if __name__ == '__main__':
    unittest.main()
