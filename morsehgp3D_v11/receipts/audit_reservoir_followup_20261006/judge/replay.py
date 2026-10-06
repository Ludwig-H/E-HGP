#!/usr/bin/env python3
"""Execute the pinned Python judge with stdlib mocked processes; no native/cloud call."""
import argparse
import contextlib
import difflib
import hashlib
import io
import json
import sys
import tempfile
import types
from pathlib import Path
from unittest.mock import patch

PIN = '59509bbc816646f7bda0c77a3b948f57c79a8b9c'
SOURCE = 'gpu_sanitizer_59509bbc.py'
ROOT = Path(__file__).resolve().parent

def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)


def fixed(source):
    before = "if run.returncode != 0 or entry['status'] != 'ok' or digest is None:"
    after = "if run.returncode != 0 or entry['status'] != 'ok' or digest is None or entry['work'] is None:"
    need(source.count(before) == 1, 'initial guard anchor')
    source = source.replace(before, after)
    before = "        if run.returncode != 0 or not clean or not entry['same_dump']:\n"
    after = ("        entry['status'] = got.get('exit', {}).get('status')\n"
             "        entry['same_work'] = got.get('domain', {}).get('catalogue_work') == seen[(name, '10', 'cpu')]['work']\n"
             "        if run.returncode != 0 or not clean or not entry['same_dump'] or entry['status'] != 'ok' or not entry['same_work']:\n")
    need(source.count(before) == 1, 'instrumented guard anchor')
    return source.replace(before, after)


def case(source, mutation):
    module = types.ModuleType('pinned_gpu_sanitizer_judge')
    exec(compile(source, SOURCE, 'exec'), module.__dict__)
    calls = []
    with tempfile.TemporaryDirectory(prefix='mhgp11-audit-sanitizer-') as temp:
        root = Path(temp)
        executable = root / 'bench.stub'
        executable.write_bytes(b'not-an-executable; subprocess.run is mocked')
        out = root / 'out'

        def run(argv, **kwargs):
            wrapped = '--tool' in argv
            args = argv[argv.index(str(executable)):]
            name = Path(args[1]).stem
            k, mode = args[4], int(args[11])
            calls.append({'cloud':name,'k':k,'mode':mode,'instrumented':wrapped})
            payload = ('audit-protocol-stub:%s:K%s' % (name,k)).encode()
            if mutation == 'changed_dump' and not wrapped and mode == 81915:
                payload += b':different'
            Path(args[3]).write_bytes(payload)
            batch = {'fill_jobs':1 if mode == 212987 else 0,
                     'spare_record_chunks':1 if mode == 81915 else 0,
                     'spare_population_chunks':1 if mode == 81915 else 0,
                     'unresolved':1 if name == 'B' and mode == 81915 else 0}
            domain = {'phase':'domain','leaf_batch':batch,'catalogue_work':{'prefixes':17,'emitted':3}}
            if mutation == 'missing_work' or (mutation == 'instrumented_missing_work' and wrapped):
                del domain['catalogue_work']
            if mutation == 'instrumented_different_work' and wrapped:
                domain['catalogue_work'] = {'prefixes':18,'emitted':3}
            status = 'refused' if mutation == 'instrumented_bad_status' and wrapped else 'ok'
            stdout = json.dumps(domain)+'\n'+json.dumps({'phase':'exit','status':status})+'\n'
            code = 9 if mutation == 'process_nonzero' and not wrapped else 0
            if mutation == 'instrumented_nonzero' and wrapped:
                code = 9
            stderr = ''
            if wrapped and mutation != 'missing_clean_marker':
                tool = argv[argv.index('--tool')+1]
                stderr = 'RACECHECK SUMMARY: 0 hazards\n' if tool == 'racecheck' else 'ERROR SUMMARY: 0 errors\n'
            return types.SimpleNamespace(returncode=code,stdout=stdout,stderr=stderr)

        args = ['gpu_sanitizer.py','--bench',str(executable),'--out',str(out),'--sanitizer','mock-compute-sanitizer']
        with patch.object(module.subprocess,'run',side_effect=run), patch.object(sys,'argv',args), contextlib.redirect_stdout(io.StringIO()):
            code = module.main()
        report = json.loads(out.joinpath('gpu_sanitizer.json').read_text())
        return {'exit_code':code,'verdict':report['verdict'],'mock_process_calls':len(calls),
                'ordinary_runs':len(report['runs']),'instrumented_runs':len(report['sanitizer']),
                'ordinary_work_absent':sum(row['work'] is None for row in report['runs']),
                'instrumented_pairs':[[row['tool'],row['cloud']] for row in report['sanitizer']]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',action='store_true')
    args = parser.parse_args()
    source = ROOT.joinpath(SOURCE).read_text()
    proposal = fixed(source)
    proposed_diff = ''.join(difflib.unified_diff(source.splitlines(keepends=True),proposal.splitlines(keepends=True),
         fromfile='a/morsehgp3D_v11/bench/gpu_sanitizer.py',tofile='b/morsehgp3D_v11/bench/gpu_sanitizer.py'))
    if args.capture:
        ROOT.joinpath('fix_proposal.patch').write_text(proposed_diff)
    else:
        need(ROOT.joinpath('fix_proposal.patch').read_text() == proposed_diff, 'patch changed')
    mutations = ('valid','missing_work','instrumented_missing_work','instrumented_different_work',
                 'instrumented_bad_status','process_nonzero','changed_dump','instrumented_nonzero','missing_clean_marker')
    original = {mutation:case(source,mutation) for mutation in mutations}
    repaired = {mutation:case(proposal,mutation) for mutation in mutations}
    need(all(original[m]['exit_code'] == 0 for m in mutations[:5]), 'original counterexample verdicts')
    need(all(original[m]['exit_code'] == 1 for m in mutations[5:]), 'original negative controls')
    need(repaired['valid']['exit_code'] == 0 and all(repaired[m]['exit_code'] == 1 for m in mutations[1:]),
         'proposed guard positive/negative cases')
    need(original['missing_work']['ordinary_work_absent'] == 12, 'missing ordinary ledger count')
    result = {'schema':'audit_gpu_sanitizer_missing_ledger_v1','source_pin':PIN,
              'source_path':'morsehgp3D_v11/bench/gpu_sanitizer.py',
              'source_sha256':hashlib.sha256(source.encode()).hexdigest(),
              'proposal_sha256':hashlib.sha256(proposal.encode()).hexdigest(),
              'native_executions':0,'cloud_actions':0,'processes_fully_mocked':True,
              'original':original,'proposal':repaired,
              'scope':{'instrumented_pairs':original['valid']['instrumented_pairs'],
                       'profiles':'The judge assumes an externally supplied CUDA/u21 bench; this mock qualifies no binary/tool/profile.'},
              'limits':['Synthetic protocol stub dumps are not native outputs or geometric evidence.',
                        'The original judge function main is executed; subprocess.run is always mocked.',
                        'No GPU/Sanitizer execution, no deployed source or historical receipt changed.']}
    text = json.dumps(result,indent=2,sort_keys=True)+'\n'
    if args.capture:
        ROOT.joinpath('summary.json').write_text(text)
    else:
        need(ROOT.joinpath('summary.json').read_text()==text,'summary changed')
    print(text,end='')
