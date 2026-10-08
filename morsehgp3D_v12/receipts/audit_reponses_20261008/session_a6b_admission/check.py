#!/usr/bin/env python3
"""A6b : archives, sources et JSONL locaux uniquement ; aucun natif/cloud/payload."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
import tarfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'
SOURCE = 'f2c106d93f1c835f129cef60e4e5e65ae185bd65'
BASE = 'results/cmd/001_t2da6b_pilote/files/t2da6b/'
INNER = 'results/cmd/000_archive_a6b/files/a6b_archive/'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(b):
    return dict(bytes=len(b), sha256=hashlib.sha256(b).hexdigest())


def load(name, p):
    spec = importlib.util.spec_from_file_location(name, p)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def closed_worker(fs, pre):
    d = dict(line.split('=', 1) for line in fs['results/worker.txt'].decode().splitlines()
             if '=' in line)
    need(d['source'] == 'commit:' + SOURCE and
         d['plan_sha256'] == pre['worker_plan_sha256'] and
         d['package_sha256'] == pre['package_sha256'], 'worker source/plan/package')
    return {k: d[k] for k in ('schema', 'status', 'fatal', 'commands_total', 'commands_ok',
                             'interrupted', 'source', 'plan_sha256', 'package_sha256',
                             'started_epoch', 'ended_epoch')}


def run(a):
    pub = a.repo / PREFIX / 'receipts/audit_reponses_20261008'
    prior = pub / 'a6b_reprise_sans_resultats'
    protocol = json.loads((prior / 'protocol.json').read_text())
    prior_cap = json.loads((prior / 'capture.json').read_text())
    helpers = [prior / 'check.py', prior / 'capture.json', prior / 'protocol.json',
               pub / 'a6_retrait_qualification/check.py',
               pub / 'session_t1d_admission/check.py', pub / 'session_r1_admission/check.py']
    # The earlier observation remains historical; this only replays its closed primaries.
    cmd = [sys.executable, '-B', '-S'] + (['-O'] if sys.flags.optimize else [])
    output = subprocess.check_output(cmd + [str(prior / 'check.py'), '--repo', str(a.repo),
                                           '--session', str(a.first)])
    old_closure = json.loads(output)
    need(old_closure['sources_exact'] and old_closure['targeted_shutdown_certified'],
         'prior closed source/stop observation')
    helper = load('archive_helper', pub / 'a6_retrait_qualification/check.py')
    common = load('source_helper', pub / 'session_t1d_admission/check.py')
    reader = load('a6b_independent', HERE / 'admit.py')
    primaries = {}
    closures = {}
    archives = {}
    contents = {}
    pres = {}
    for label, session in [('second', a.second), ('recovery', a.recovery)]:
        r = json.loads((session / 'receipt.json').read_text())
        pre = json.loads((session / 'preflight.json').read_text())
        pres[label] = pre
        blob = (session / 'results/results.tar.gz').read_bytes()
        fs, info = helper.archive(blob)
        need(r['commit'] == pre['commit'] == SOURCE and
             r['results_sha256'] == pin(blob)['sha256'] and r['results_bytes'] == len(blob),
             'result archive identity')
        need(r['status'] == 'completed' and r['worker_exit_code'] == 0 and
             r['results_verified'] is True and not r['errors'] and
             int((session / 'DONE').read_text()) == 0, 'successful outer execution')
        need(r['targeted_shutdown_certified'] is True and r['stop_exit_code'] == 0 and
             r['observed_before_stop']['status'] == 'RUNNING' and
             r['observed_after']['status'] == 'TERMINATED', 'certified stop transition')
        need(info['commands'] == r['commands'] and
             all(c['exit_code'] == '0' and c['status'] == 'ok' for c in r['commands']),
             'outer command closure')
        need(pin((session / 'package/plan.json').read_bytes())['sha256'] ==
             pre['plan_sha256'] == r['plan_sha256'], 'outer plan')
        need(pin((session / 'package/package.tar.gz').read_bytes())['sha256'] ==
             pre['package_sha256'] == r['package_sha256'], 'outer package')
        need(fs['results/plan.sh'] == (session / 'package/plan.sh').read_bytes(),
             'executed outer plan bytes')
        info['worker'] = closed_worker(fs, pre)
        info['worker_exit_code'] = 0
        need(info['worker']['status'] == 'completed' and
             info['worker']['fatal'] == '' and info['worker']['interrupted'] == '0',
             'outer worker complete')
        primaries[label] = {n: pin((session / n).read_bytes()) for n in
            ['receipt.json', 'DONE', 'preflight.json', 'package/plan.json',
             'package/plan.sh', 'package/package.tar.gz', 'results/results.tar.gz']}
        closures[label] = {k: r[k] for k in ['status', 'worker_exit_code', 'results_verified',
            'targeted_shutdown_certified', 'stop_exit_code', 'stop_attempts',
            'reserve_released', 'guest_guard_intact']}
        closures[label].update(done=0, before='RUNNING', after='TERMINATED', errors_count=0)
        archives[label] = info
        contents[label] = fs
    need([c['name'] for c in archives['second']['commands']] ==
         [c['name'] for c in protocol['commands']], 'second command cohort')
    need([c['name'] for c in archives['recovery']['commands']] == ['archive_a6b'],
         'recovery command only')
    for n in ['package/plan.json', 'package/plan.sh', 'package/package.tar.gz']:
        need((a.second / n).read_bytes() == (a.first / n).read_bytes(),
             'same first/second source and protocol')
    first_pre = json.loads((a.first / 'preflight.json').read_text())
    need(pres['second']['data_files'] == first_pre['data_files'], 'same declared inputs')
    recovered = contents['recovery']
    first_blob = recovered[INNER + 'results.tar.gz']
    first_fs, first_info = helper.archive(first_blob)
    recorded = recovered[INNER + 'SHA256SUMS'].decode().strip().split()
    need(recorded == [pin(first_blob)['sha256'], 'results.tar.gz'], 'recovered original hash')
    need(int(recovered[INNER + 'worker.exit']) == 1, 'original worker exit')
    need(first_fs['results/plan.sh'] == (a.first / 'package/plan.sh').read_bytes(),
         'original executed plan bytes')
    first_info['worker'] = closed_worker(first_fs, first_pre)
    first_info['worker_exit_code'] = 1
    need(first_info['worker']['status'] == 'failed' and first_info['worker']['fatal'] == '' and
         first_info['worker']['commands_total'] == '4' and
         first_info['worker']['commands_ok'] == '3' and
         first_info['worker']['interrupted'] == '0', 'original worker completion')
    need([c['name'] for c in first_info['commands']] ==
         [c['name'] for c in protocol['commands']] and
         [c['exit_code'] for c in first_info['commands']] == ['0', '3', '0', '0'],
         'original pilot refusal, other commands successful')
    archives['first'] = first_info
    contents['first'] = first_fs
    recovery_source, _ = common.source(a.repo, a.recovery / 'package/package.tar.gz', SOURCE)
    need(recovery_source['inventory_sha256'] ==
         protocol['source_packages']['after']['inventory_sha256'], 'recovery native source')
    a.snapshot.mkdir(parents=True, exist_ok=True)
    with tarfile.open(a.first / 'package/package.tar.gz') as t:
        for name, expected in protocol['source_pins'].items():
            b = t.extractfile(PREFIX + name).read()
            need(pin(b) == expected, 'closed pilot source')
            target = a.snapshot / 'source' / PREFIX / name
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                need(target.read_bytes() == b, 'source snapshot changed')
            else:
                target.write_bytes(b)
    results = {}
    gates = {}
    inventories = {}
    for label in ['first', 'second']:
        fs = contents[label]
        selected = {n: b for n, b in fs.items() if n.startswith(BASE) and
                    (n.endswith(('.jsonl', '.jsonl.err')) or n == BASE + 'rapport_t2d_a6b.json')}
        need(sum(n.endswith('.jsonl') for n in selected) == 85, '85 native journals')
        out = a.snapshot / label / 'returned'
        for n, b in selected.items():
            rel = Path(n[len(BASE):])
            need(not rel.is_absolute() and '..' not in rel.parts, 'journal path')
            target = out / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                need(target.read_bytes() == b, 'journal snapshot changed')
            else:
                target.write_bytes(b)
        results[label] = reader.run(a.repo, out, prior / 'protocol.json', a.snapshot / 'source')
        need(results[label]['verdict'] == ('refuse' if label == 'first' else 'rejete'),
             'replayed verdict')
        gates[label] = {}
        for index, name, expected in [(0, 'socle_ctest', 754), (2, 'lidar_ctest', 7),
                                      (3, 'mutants_tour', 1)]:
            b = fs['results/cmd/%03d_%s/stdout' % (index, name)].decode()
            counts = {key: len(re.findall(r'Test\s+#\d+:.*?\.\.\.\s+' + pat + r'\b', b))
                for key, pat in [('passed', 'Passed'), ('skipped', r'\*\*\*Skipped'),
                                 ('failed', r'\*\*\*Failed')]}
            need(counts == dict(passed=expected, skipped=0, failed=0), 'CTest gate closure')
            gates[label][name] = counts
        inventories[label] = dict(report=pin(fs[BASE + 'rapport_t2d_a6b.json']),
            journals_sha256=pin(''.join(pin(b)['sha256'] + '  ' + n + '\n'
                                       for n, b in sorted(selected.items())).encode())['sha256'])
    capture = dict(source_git=SOURCE, before_git=protocol['before_git'],
        helpers={str(p.relative_to(a.repo)): pin(p.read_bytes()) for p in helpers},
        primaries=primaries, closures=closures, archives=archives,
        original_source_packages=protocol['source_packages'], recovery_sources=recovery_source,
        rule=protocol['rule'], cohort=protocol['cohort'], gates=gates,
        inventories=inventories, original_absence_observation=prior_cap['observation_utc'])
    return capture, results


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ['repo', 'first', 'second', 'recovery', 'snapshot']:
        p.add_argument('--' + name, required=True, type=Path)
    p.add_argument('--write', action='store_true')
    a = p.parse_args()
    for name in ['repo', 'first', 'second', 'recovery', 'snapshot']:
        setattr(a, name, getattr(a, name).resolve())
    capture, result = run(a)
    for name, obj in [('capture.json', capture), ('results.json', result)]:
        path = HERE / name
        if a.write:
            path.write_text(json.dumps(obj, ensure_ascii=False, separators=(',', ':')) + '\n')
        else:
            need(obj == json.loads(path.read_text()), 'stored ' + name)
    print(json.dumps({label: {k: r[k] for k in ['cohort_admitted', 'processes', 'full_passes',
        'decisive_warm_passes', 'verdict', 'worker_float_differences']}
        for label, r in result.items()}))


if __name__ == '__main__':
    main()
