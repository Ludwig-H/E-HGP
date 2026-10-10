#!/usr/bin/env python3
"""B3b : admission des primaires closes ; sources/JSONL uniquement, aucun natif/cloud."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import subprocess
import sys
import tarfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'
SOURCE = '81b0883d11df14e4b63d82e85b6539d6abc7bf92'
BEFORE = 'aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(data):
    return dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(a):
    s = a.session
    needed = ['receipt.json', 'DONE', 'preflight.json', 'package/plan.json', 'package/plan.sh',
              'package/package.tar.gz', 'results/results.tar.gz']
    need(all((s / n).is_file() for n in needed), 'session not closed: required primary absent')
    pre = json.loads((s / 'preflight.json').read_text())
    rec = json.loads((s / 'receipt.json').read_text())
    plan = json.loads((s / 'package/plan.json').read_text())
    need(pre['commit'] == rec['commit'] == SOURCE and pre['source_kind'] == 'commit', 'source declaration')
    for name, key in [('package/package.tar.gz', 'package_sha256'), ('package/plan.json', 'plan_sha256')]:
        need(pin((s / name).read_bytes())['sha256'] == pre[key] == rec[key], 'package/plan pin')
    common_path = HERE.parent / 'session_a6c_admission/check.py'
    common = load('a6c_source_helper', common_path)
    after, source = common.source(a.repo, s / 'package/package.tar.gz', SOURCE)
    before_path = Path(pre['data_dir']) / 'v12_src_aa6338ee8.tar.gz'
    before, baseline = common.source(a.repo, before_path, BEFORE)
    arms_name = PREFIX + 'microbancs/mes_t2d_b3/bras_t2d_b3.json'
    arms = json.loads(source[arms_name])
    need(arms['base'] == BEFORE[:9], 'arm base')
    reconstructions = {}
    for name, arm in arms['bras'].items():
        product = {n: b for n, b in baseline.items() if n.startswith(PREFIX + 'src/')}
        changes = 0
        for rel, spec in arm['fichiers'].items():
            n = PREFIX + rel
            need(pin(product[n])['sha256'] == spec['sha256_avant'], 'arm preimage')
            text = product[n].decode()
            for sub in spec['substitutions']:
                need(text.count(sub['cherche']) == 1, 'unique arm substitution')
                text = text.replace(sub['cherche'], sub['remplace'], 1)
                changes += 1
            product[n] = text.encode()
            need(pin(product[n])['sha256'] == spec['sha256_apres'], 'arm postimage')
        if name == 'apres':
            need(product == {n: b for n, b in source.items() if n.startswith(PREFIX + 'src/')},
                 'measured after arm is packaged product')
        inventory = ''.join(pin(b)['sha256'] + '  ' + n + '\n' for n, b in sorted(product.items())).encode()
        reconstructions[name] = dict(files=len(arm['fichiers']), substitutions=changes,
                                    product_inventory_sha256=pin(inventory)['sha256'])
    argv = plan['commands'][1]['argv']
    config = {opt: argv[argv.index(opt) + 1] for opt in ['--fils', '--passes', '--processus', '--avant-sha256']}
    need(config == {'--fils': '48', '--passes': '8', '--processus': '10',
                    '--avant-sha256': before['archive']['sha256']}, 'planned decisive config')
    need('tout' in argv and not any(x in argv for x in ['--essai', '--cache', '--sequentiel']), 'default full route')
    declared = {d['name']: d for d in pre['data_files']}
    need(declared[before_path.name]['sha256'] == before['archive']['sha256'] and
         declared[before_path.name]['size'] == before['archive']['bytes'], 'baseline input metadata')
    reader = load('b3b_primaries', HERE / 'admit.py')
    result = reader.read(a.repo, s)
    fs = reader.files((s / 'results/results.tar.gz').read_bytes())
    need(fs['results/plan.sh'] == (s / 'package/plan.sh').read_bytes() and
         pin(fs['results/plan.sh'])['sha256'] == pre['worker_plan_sha256'], 'worker plan bytes')
    worker = dict(x.split('=', 1) for x in fs['results/worker.txt'].decode().splitlines() if '=' in x)
    need(worker['source'] == 'commit:' + SOURCE and worker['package_sha256'] == pre['package_sha256'] and
         worker['plan_sha256'] == pre['worker_plan_sha256'] and worker['status'] == 'completed' and
         worker['fatal'] == '' and worker['interrupted'] == '0', 'worker closure')
    with tarfile.open(s / 'package/package.tar.gz') as t:
        worker_source = t.extractfile('gcp-migration/v12_worker.sh').read()
    need(worker_source == subprocess.check_output(['git', '-C', str(a.repo), 'show',
                    SOURCE + ':gcp-migration/v12_worker.sh']), 'packaged worker exact Git')
    report_name = reader.ROOT + 'rapport_t2d_b3.json'
    report = json.loads(fs[report_name])
    build_config = ['CMAKE_BUILD_TYPE:STRING=Release', 'MHGP12_COORD_BITS:STRING=21',
                    'MHGP12_ENABLE_CUDA:BOOL=ON']
    configs = report['construction']['cmake']
    need(set(configs) == {'avant', 'cles', 'balayage', 'transfert', 'apres'}, 'five FULL build configurations')
    for options in configs.values():
        need(set(build_config) <= set(options), 'FULL build configuration')
        need(options == configs['avant'], 'same FULL build options')
    need(not any(b.startswith(b'\x7fELF') for b in fs.values()), 'ELF physical return scope')
    old = a.repo / PREFIX / 'receipts/audit_reponses_20261008/session_t2db3_admission/admit.py'
    capture = dict(source_git=SOURCE, before_git=BEFORE,
        primaries={n: pin((s / n).read_bytes()) for n in needed},
        helpers={str(p.relative_to(a.repo)) if p.is_relative_to(a.repo) else
                 PREFIX + 'receipts/audit_reponses_20261010/session_a6c_admission/check.py': pin(p.read_bytes())
                 for p in [common_path, old]},
        sources=dict(before=before, after=after), reconstructed_arms=reconstructions,
        worker_source=pin(worker_source), configuration=config, report=pin(fs[report_name]),
        worker={k: worker[k] for k in ['status', 'commands_total', 'commands_ok', 'interrupted',
                                     'started_epoch', 'ended_epoch']},
        full_profile=build_config, physical_elf_returned=0, native_calls=0, cloud_calls=0)
    return capture, result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    args.repo, args.session = args.repo.resolve(), args.session.resolve()
    cap, result = run(args)
    for name, value in [('capture.json', cap), ('results.json', result)]:
        dest = HERE / name
        if args.write:
            dest.write_text(json.dumps(value, ensure_ascii=False, separators=(',', ':')) + '\n')
        else:
            need(json.loads(dest.read_text()) == value, 'stored ' + name)
    print(json.dumps({k: result[k] for k in ['verdicts', 'native_journals', 'decisive_warm_passes',
                                          'strict_evidence_admission_complete']}))
