#!/usr/bin/env python3
"""A6c : sources publiques, metadonnees et JSONL ; aucun moteur, cloud ou payload."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
import tarfile
import tempfile
from types import SimpleNamespace

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'
SOURCE = 'aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66'
BEFORE = '8a0716e7470197c95953b38d79f26b8d8f2379fc'
PUBLIC = '9ffe6bd17e1ed52d28054f6bef92622e295947ba'
BASE = 'results/cmd/001_t2da6c_pilote/files/t2da6c/'
RECEIPT = PREFIX + 'receipts/g4_a6c_20261010/'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(b):
    return dict(bytes=len(b), sha256=hashlib.sha256(b).hexdigest())


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def source(repo, archive, commit):
    scopes = [PREFIX + p for p in ['src/', 'bench/', 'tests/', 'cmake/', 'microbancs/', 'CMakeLists.txt']]
    names = git(repo, 'ls-tree', '-r', '--name-only', commit, '--', *scopes).decode().splitlines()
    with tarfile.open(archive) as t:
        members = {m.name: m for m in t if m.isfile() and any(m.name.startswith(p) if p.endswith('/')
                   else m.name == p for p in scopes)}
        need(set(members) == set(names), 'complete source inventory')
        blobs = {n: t.extractfile(members[n]).read() for n in names}
    batch = subprocess.check_output(
        ['git', '-C', str(repo), 'cat-file', '--batch'],
        input=('\n'.join(commit + ':' + n for n in names) + '\n').encode())
    at = 0
    for n in names:
        end = batch.index(b'\n', at)
        size = int(batch[at:end].split()[2])
        at = end + 1
        need(batch[at:at + size] == blobs[n], 'source exact Git ' + n)
        at += size + 1
    need(at == len(batch), 'source batch complete')
    inventory = ''.join(pin(blobs[n])['sha256'] + '  ' + n + '\n' for n in sorted(names)).encode()
    return dict(commit=commit, files=len(names), inventory_sha256=pin(inventory)['sha256'],
                archive=pin(archive.read_bytes())), blobs


def run(a):
    prior = a.repo / PREFIX / 'receipts/audit_reponses_20261008'
    helpers = [prior / 'a6_retrait_qualification/check.py',
               prior / 'session_r1_admission/check.py']
    archive_helper = load('archive_a6c', helpers[0])
    reader = load('independent_a6c', HERE / 'admit.py')
    s = a.session
    rec = json.loads((s / 'receipt.json').read_text())
    pre = json.loads((s / 'preflight.json').read_text())
    plan = json.loads((s / 'package/plan.json').read_text())
    raw = (s / 'results/results.tar.gz').read_bytes()
    fs, arc = archive_helper.archive(raw)
    need(rec['commit'] == pre['commit'] == SOURCE and rec['results_sha256'] == pin(raw)['sha256']
         and rec['results_bytes'] == len(raw), 'result/source identity')
    need(rec['status'] == 'completed' and rec['worker_exit_code'] == 0 and
         rec['results_verified'] is True and not rec['errors'] and
         int((s / 'DONE').read_text()) == 0, 'successful outer execution')
    need(rec['targeted_shutdown_certified'] is True and rec['stop_exit_code'] == 0 and
         rec['observed_before_stop']['status'] == 'RUNNING' and
         rec['observed_after']['status'] == 'TERMINATED', 'certified session shutdown')
    names = ['socle_ctest', 't2da6c_pilote', 'lidar_ctest', 'mutants_tour']
    need(arc['commands'] == rec['commands'] and [c['name'] for c in rec['commands']] == names and
         all(c['exit_code'] == '0' and c['status'] == 'ok' for c in rec['commands']), 'commands closure')
    need([c['name'] for c in plan['commands']] == names, 'plan commands')
    for n, key in [('package/plan.json', 'plan_sha256'), ('package/package.tar.gz', 'package_sha256')]:
        need(pin((s / n).read_bytes())['sha256'] == pre[key] == rec[key], 'package/plan')
    need(fs['results/plan.sh'] == (s / 'package/plan.sh').read_bytes() and
         pin(fs['results/plan.sh'])['sha256'] == pre['worker_plan_sha256'], 'executed worker plan')
    worker = dict(x.split('=', 1) for x in fs['results/worker.txt'].decode().splitlines() if '=' in x)
    need(worker['source'] == 'commit:' + SOURCE and worker['plan_sha256'] == pre['worker_plan_sha256']
         and worker['package_sha256'] == pre['package_sha256'] and worker['status'] == 'completed'
         and worker['fatal'] == '' and worker['interrupted'] == '0', 'worker closure')
    after, sources = source(a.repo, s / 'package/package.tar.gz', SOURCE)
    before, _ = source(a.repo, Path(pre['data_dir']) / 'v12_src_8a0716e74.tar.gz', BEFORE)
    argv = plan['commands'][1]['argv']
    opts = {n: argv[argv.index(n) + 1] for n in ['--fils', '--tours', '--tours-grandes', '--passes', '--avant-sha256']}
    need(opts == {'--fils': '48', '--tours': '5', '--tours-grandes': '6', '--passes': '10',
                  '--avant-sha256': before['archive']['sha256']} and '--essai' not in argv, 'planned configuration')
    report = json.loads(fs[BASE + 'rapport_t2d_a6c.json'])
    published = json.loads(git(a.repo, 'show', PUBLIC + ':' + RECEIPT +
                               'resultats/cmd/001_t2da6c_pilote/files/t2da6c/rapport_t2d_a6c.json'))
    pub_fields = ['schema', 'cohorte_demandee', 'regle', 'decision_chaine', 'grandes', 'identite', 'ng', 'jugement']
    need(all(report[k] == published[k] for k in pub_fields), 'published numeric report')
    selected = {n: b for n, b in fs.items() if n.startswith(BASE) and n.endswith(('.jsonl', '.jsonl.err'))}
    need(sum(n.endswith('.jsonl') for n in selected) == 85 and len(selected) == 170, 'journal inventory')
    for n, b in selected.items():
        need(git(a.repo, 'show', PUBLIC + ':' + RECEIPT + 'resultats/' + n.removeprefix('results/')) == b,
             'published raw journal')
    with tempfile.TemporaryDirectory(prefix='audit-a6c-closed-') as td:
        tmp = Path(td)
        source_pins = {}
        for n in ['microbancs/mes_t2d_a6c/pilote_t2d_a6c.py', 'microbancs/outils/banc_full.py',
                  'microbancs/outils/lecteur_full.py']:
            target = tmp / 'source' / PREFIX / n
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(sources[PREFIX + n])
            source_pins[n] = pin(target.read_bytes())
        pilot = load('closed_a6c', tmp / 'source' / PREFIX / 'microbancs/mes_t2d_a6c/pilote_t2d_a6c.py')
        # Read ONLY the public bundle manifest, never coordinate/ID members.
        manifest_archive = Path(pre['data_dir']) / 'g4_kitti_v12set_xyz.tar'
        config = pilot.plan_demande(SimpleNamespace(archive_v12set=str(manifest_archive), fils=48,
                                     passes=10, tours=5, tours_grandes=6, essai=False))
        need(len(config['cas']) == 37 and sum(n > 60000 for _, n in config['cas']) == 21,
             'external manifest cohort')
        protocol = dict(source_pins=source_pins, rule=pilot.REGLE_T2D_A6C, cohort=config,
                        source_packages=dict(before=before, after=after))
        (tmp / 'protocol.json').write_text(json.dumps(protocol))
        returned = tmp / 'returned'
        for n, b in {**selected, BASE + 'rapport_t2d_a6c.json': fs[BASE + 'rapport_t2d_a6c.json']}.items():
            rel = Path(n.removeprefix(BASE))
            need(not rel.is_absolute() and '..' not in rel.parts, 'journal path')
            target = returned / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b)
        results = reader.run(a.repo, returned, tmp / 'protocol.json', tmp / 'source')
    need(results['verdict'] == 'adopte', 'independent verdict')
    gates = {}
    for i, name, expected in [(0, 'socle_ctest', 755), (2, 'lidar_ctest', 7), (3, 'mutants_tour', 1)]:
        out = fs['results/cmd/%03d_%s/stdout' % (i, name)].decode()
        counts = {k: len(re.findall(r'Test\s+#\d+:.*?\.\.\.\s+' + v + r'\b', out)) for k, v in
                  [('passed', 'Passed'), ('failed', r'\*\*\*Failed'), ('skipped', r'\*\*\*Skipped')]}
        need(counts == dict(passed=expected, failed=0, skipped=0), 'CTest gate count')
        gates[name] = counts
    mutant = json.loads(sources[PREFIX + 'tests/mutants/tower.json'])
    need(mutant['plancher'] == len(mutant['mutants']) == 72, '72 tower mutants/floor')
    need(not any('mutant' in n and n.endswith('.json') for n in fs), 'individual mutant reports absent')
    native_cmake = sources[PREFIX + 'CMakeLists.txt'].decode()
    mutant_profile = native_cmake.split('set(mhgp12_mutant_profile ', 1)[1].split('foreach(unit', 1)[0]
    need('MHGP12_ENABLE_CUDA' not in mutant_profile and
         'option(MHGP12_ENABLE_CUDA' in native_cmake, 'mutant CUDA flag not propagated')
    need(not any(b.startswith(b'\x7fELF') for b in fs.values()), 'no returned ELF file')
    cmake = report['construction']['binaires']
    need(cmake['avant']['cmake'] == cmake['apres']['cmake'], 'same cache options')
    capture = dict(source_git=SOURCE, before_git=BEFORE, public_git=PUBLIC,
        primaries={n: pin((s / n).read_bytes()) for n in ['receipt.json', 'DONE', 'preflight.json',
            'package/plan.json', 'package/plan.sh', 'package/package.tar.gz', 'results/results.tar.gz']},
        helpers={str(p.relative_to(a.repo)): pin(p.read_bytes()) for p in helpers},
        archive=arc, sources=dict(before=before, after=after), rule=protocol['rule'], cohort=config,
        source_pins=source_pins, report=pin(fs[BASE + 'rapport_t2d_a6c.json']),
        journals=dict(processes=85, stderr_empty=85,
            inventory_sha256=pin(''.join(pin(b)['sha256'] + '  ' + n + '\n' for n, b in sorted(selected.items())).encode())['sha256']),
        gates=gates, mutants=dict(manifest=pin(sources[PREFIX + 'tests/mutants/tower.json']),
            declared=72, floor=72, aggregate_ctest_passed=True, individual_reports_returned=False,
            cuda_enable_forwarded=False),
        closure={k: rec[k] for k in ['status', 'worker_exit_code', 'results_verified',
            'targeted_shutdown_certified', 'stop_exit_code', 'reserve_released', 'guest_guard_intact']},
        worker={k: worker[k] for k in ['status', 'commands_total', 'commands_ok', 'interrupted',
                                     'started_epoch', 'ended_epoch']},
        build_options=cmake['apres']['cmake'], physical_elf_returned=0, native_calls=0, cloud_calls=0)
    return capture, results


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, required=True)
    p.add_argument('--session', type=Path, required=True)
    p.add_argument('--write', action='store_true')
    a = p.parse_args()
    a.repo, a.session = a.repo.resolve(), a.session.resolve()
    capture, result = run(a)
    for name, value in [('capture.json', capture), ('results.json', result)]:
        path = HERE / name
        if a.write:
            path.write_text(json.dumps(value, ensure_ascii=False, separators=(',', ':')) + '\n')
        else:
            need(json.loads(path.read_text()) == value, 'stored ' + name)
    print(json.dumps({k: result[k] for k in ['processes', 'full_passes', 'decisive_warm_passes',
        'identity_passes', 'verdict', 'worker_float_differences']}))


if __name__ == '__main__':
    main()
