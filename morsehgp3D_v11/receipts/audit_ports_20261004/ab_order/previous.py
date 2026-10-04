"""Banc A/B G4 de developpement (dev_snapshot) : portes, TSan cible, mutants choisis, chronos apparies, verdict.

Variantes : `base` (archive de la source de reference fournie dans les donnees, base_src.tar.gz) et `new` (arbre
envoye). Chaque prise FULL ecrit son dump, hache puis efface ; toutes les empreintes d'une trame doivent egaler celle
de la premiere prise de base. Les commandes tournent dans leur propre groupe de processus : un delai tue le groupe
entier et la quiescence est verifiee avant de continuer. Le verdict final n'est `conforme` que si chaque construction,
porte, campagne de mutants, prise et identite attendue est presente et reussie ; sinon `refus` (code 1).
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import tarfile
import time

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')
ARGS_TAIL = ['5', '16', '256', '0', '4294967295', '8589934592']


def run(cmd, out_dir, name, timeout, cwd=None):
    t0 = time.monotonic()
    with open(out_dir / (name + '.stdout'), 'wb') as o, open(out_dir / (name + '.stderr'), 'wb') as e:
        proc = subprocess.Popen(cmd, stdout=o, stderr=e, cwd=cwd, start_new_session=True)
        try:
            code = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            code = 'timeout'
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait()
    quiet = True
    try:
        os.killpg(proc.pid, 0)  # descendants encore vivants dans le groupe prive
        quiet = False
        os.killpg(proc.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    return {'name': name, 'cmd': [str(c) for c in cmd], 'code': code, 'quiet': quiet,
            'seconds': round(time.monotonic() - t0, 3)}


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 22), b''):
            h.update(chunk)
    return h.hexdigest()


def lines(stdout_path):
    out = {}
    for line in Path(stdout_path).read_text(errors='replace').splitlines():
        try:
            j = json.loads(line)
        except ValueError:
            continue
        if isinstance(j, dict) and 'phase' in j:
            out[j['phase']] = j
    return out


def summary(stdout_path):
    got = lines(stdout_path)
    full, domain = got.get('full', {}), got.get('domain', {})
    keep = {k: full.get(k) for k in ('status', 'reason', 'wall_ns', 'index_ns', 'domain_ns', 'forest_ns',
                                     'cpu_seconds', 'peak_reserved_bytes', 'phases')}
    keep['exit'] = got.get('exit', {}).get('status')
    keep['domain_detail'] = {k: domain.get(k) for k in ('single_pass_ns', 'prefix_ns', 'sort_ns', 'level_scan_ns',
                                                        'assembly_ns', 'compact_ns', 'catalogue_balls')}
    keep['orders'] = [{'k': o.get('k'), 'timings': o.get('timings'),
                       'work': {w: (o.get('work') or {}).get(w) for w in
                                ('descent_steps', 'population_hits', 'catalogue_hits', 'census_calls',
                                 'census_point_tests', 'part_meb_presentations', 'traces')}}
                      for o in full.get('orders') or []]
    return keep


def ctest_summary(path):
    text = Path(path).read_text(errors='replace')
    tail = [l.strip() for l in text.splitlines() if 'tests passed' in l]
    return tail[-1] if tail else ''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--src', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--data', required=True)
    ap.add_argument('--work', required=True)
    ap.add_argument('--reps', type=int, default=5)
    ap.add_argument('--mode', default='16379')
    ap.add_argument('--tests', default='')        # expression reguliere ctest (-R) ; vide : aucune porte
    ap.add_argument('--labels-exclude', default='^(mutant|long)$')
    ap.add_argument('--min-tests', type=int, default=1)
    ap.add_argument('--tsan', default='')         # expression reguliere ctest jouee sous TSan (setarch -R)
    ap.add_argument('--min-tsan', type=int, default=1)
    ap.add_argument('--mutants', default='')      # module:id,id;module:id
    ap.add_argument('--w1', action='store_true')
    ap.add_argument('--perf-new', action='store_true')
    a = ap.parse_args()
    out = Path(a.out); out.mkdir(parents=True, exist_ok=True)
    work = Path(a.work); work.mkdir(parents=True, exist_ok=True)
    data = Path(a.data)
    report = {'schema': 'ehgp.v11.claude_ab.v2', 'steps': [], 'builds': {}, 'tests': {}, 'tsan': {}, 'mutants': {},
              'timings': [], 'identity': {}, 'status': 'running', 'verdict': None, 'refusals': []}

    def save():
        (out / 'ab_report.json').write_text(json.dumps(report, indent=1) + '\n')

    def step(cmd, name, timeout):
        s = run(cmd, out, name, timeout)
        report['steps'].append(s)
        save()
        return s

    sources = {'new': Path(a.src) / 'morsehgp3D_v11'}
    archive = data / 'base_src.tar.gz'
    if archive.exists():
        base_root = work / 'base_src'
        if base_root.exists():
            shutil.rmtree(base_root)
        base_root.mkdir(parents=True)
        with tarfile.open(archive) as t:
            t.extractall(base_root)
        sources['base'] = base_root / 'morsehgp3D_v11'
        report['base_archive_sha256'] = sha256(archive)
    report['sources'] = {k: str(v) for k, v in sources.items()}

    for name, src in sources.items():
        bdir = work / ('b_' + name)
        step(['cmake', '-S', str(src), '-B', str(bdir), '-DCMAKE_BUILD_TYPE=Release', '-DMHGP11_COORD_BITS=21'],
             'cfg_' + name, 300)
        target = [] if (name == 'new' and a.tests) else ['--target', 'mhgp11_full_bench']
        step(['cmake', '--build', str(bdir), '-j', '48'] + target, 'build_' + name, 1500)
        exe = bdir / 'mhgp11_full_bench'
        report['builds'][name] = {'exists': exe.exists(), 'sha256': sha256(exe) if exe.exists() else None}
        save()

    if a.tests:
        s = step(['ctest', '--test-dir', str(work / 'b_new'), '-j', '48', '--output-on-failure', '--timeout', '900',
                  '-R', a.tests, '-LE', a.labels_exclude], 'ctest_new', 2400)
        report['tests'] = {'code': s['code'], 'summary': ctest_summary(out / 'ctest_new.stdout')}

    if a.tsan:
        bdir = work / 'b_tsan'
        step(['cmake', '-S', str(sources['new']), '-B', str(bdir), '-DCMAKE_BUILD_TYPE=Release',
              '-DMHGP11_COORD_BITS=21', '-DMHGP11_TSAN=ON'], 'cfg_tsan', 300)
        step(['cmake', '--build', str(bdir), '-j', '48'], 'build_tsan', 2400)
        arch = subprocess.run(['uname', '-m'], capture_output=True, text=True).stdout.strip() or 'x86_64'
        s = step(['setarch', arch, '-R', 'ctest', '--test-dir', str(bdir), '-j', '16', '--output-on-failure',
                  '--timeout', '1500', '-R', a.tsan], 'ctest_tsan', 3000)
        report['tsan'] = {'code': s['code'], 'summary': ctest_summary(out / 'ctest_tsan.stdout')}

    for spec in [x for x in a.mutants.split(';') if x]:
        module, ids = spec.split(':')
        idlist = [i for i in ids.split(',') if i]
        src = sources['new']
        report_path = out / ('mutants_' + module + '.json')
        s = step(['python3', str(src / 'tests/mutants/run_mutants.py'), '--manifest',
                  str(src / 'tests/mutants' / (module + '.json')), '--source', str(src),
                  '--work', str(work / ('mut_' + module)), '--jobs', '24', '--only', ','.join(idlist),
                  '--floor', str(len(idlist)), '--report', str(report_path)], 'mutants_' + module, 2400)
        killed = []
        if report_path.exists():
            got = json.loads(report_path.read_text())
            killed = sorted(m['id'] for m in got.get('mutants', []) if m.get('verdict') == 'TUE')
        report['mutants'][module] = {'code': s['code'], 'ids': idlist, 'killed': killed}
        save()

    variants = [v for v in ('base', 'new') if v in sources and report['builds'].get(v, {}).get('exists')]
    plans = [('48', r) for r in range(a.reps)] + ([('1', 0)] if a.w1 else [])
    for workers, rep in plans:
        for frame in FRAMES:
            for variant in (variants if rep % 2 == 0 else list(reversed(variants))):
                exe = work / ('b_' + variant) / 'mhgp11_full_bench'
                dump = work / ('dump_%s_%s.bin' % (variant, frame))
                cmd = [str(exe), str(data / (frame + '.u32le')), str(data / (frame + '.ids.u32le')), str(dump)]
                name = 't_%s_%s_w%s_r%d' % (variant, frame, workers, rep)
                s = run(cmd + ARGS_TAIL + [workers, a.mode], out, name, 600)
                s.update(variant=variant, frame=frame, workers=workers, rep=rep,
                         summary=summary(out / (name + '.stdout')))
                s['dump_sha256'] = sha256(dump) if dump.exists() else None
                if dump.exists():
                    dump.unlink()
                report['timings'].append(s)
                if variant == 'base' and workers == '48' and rep == 0 and s['dump_sha256']:
                    report['identity'][frame] = s['dump_sha256']
                save()

    if a.perf_new and shutil.which('perf') and report['builds'].get('new', {}).get('exists'):
        exe = work / 'b_new' / 'mhgp11_full_bench'
        data_file = work / 'perf_new.data'
        step(['sudo', '-n', 'sysctl', '-w', 'kernel.perf_event_paranoid=-1'], 'paranoid', 60)
        step(['perf', 'record', '-F', '1999', '-g', '-o', str(data_file), '--', str(exe),
              str(data / 'lidar_ng00.u32le'), str(data / 'lidar_ng00.ids.u32le'), '/dev/null'] + ARGS_TAIL + ['1', a.mode],
             'perf_new', 600)
        step(['perf', 'report', '-i', str(data_file), '--no-children', '--stdio', '--percent-limit', '0.2'],
             'perf_new_self', 600)

    # Verdict : tout ce qui etait demande est present et reussi, chaque sortie egale la reference de sa trame.
    refusals = report['refusals']
    need = lambda ok, why: None if ok else refusals.append(why)  # noqa: E731
    for s in report['steps']:
        if s['name'].startswith(('cfg_', 'build_', 'ctest_', 'mutants_')):
            need(s['code'] == 0 and s['quiet'], 'step %s code %s quiet %s' % (s['name'], s['code'], s['quiet']))
    for v in sources:
        need(report['builds'].get(v, {}).get('exists'), 'missing build ' + v)
    if a.tests:
        t = report['tests']
        need(t.get('code') == 0 and t.get('summary', '').startswith('100% tests passed'), 'ctest not all passed')
        count = int(t['summary'].split('out of')[-1]) if 'out of' in t.get('summary', '') else 0
        need(count >= a.min_tests, 'ctest count %d < %d' % (count, a.min_tests))
    if a.tsan:
        t = report['tsan']
        need(t.get('code') == 0 and t.get('summary', '').startswith('100% tests passed'), 'tsan not all passed')
        count = int(t['summary'].split('out of')[-1]) if 'out of' in t.get('summary', '') else 0
        need(count >= a.min_tsan, 'tsan count %d < %d' % (count, a.min_tsan))
    for module, m in report['mutants'].items():
        need(m['code'] == 0 and sorted(m['ids']) == m['killed'], 'mutants %s not all killed' % module)
    for frame in FRAMES:
        need(frame in report['identity'], 'no reference output for ' + frame)
    expected = {(f, w, v) for (w, _) in plans for f in FRAMES for v in variants}
    seen = {}
    for s in report['timings']:
        key = (s['frame'], s['workers'], s['variant'])
        seen[key] = seen.get(key, 0) + 1
        ok = (s['code'] == 0 and s['quiet'] and s['summary'].get('status') == 'ok' and
              s['summary'].get('exit') == 'ok' and s['dump_sha256'] == report['identity'].get(s['frame']))
        need(ok, 'run %s code %s status %s dump %s' % (s['name'], s['code'], s['summary'].get('status'),
                                                      (s['dump_sha256'] or '')[:12]))
    for key in expected:
        want = sum(1 for (w, _) in plans if w == key[1])
        need(seen.get(key, 0) == want, 'run count %s %d != %d' % (key, seen.get(key, 0), want))
    report['verdict'] = 'conforme' if not refusals else 'refus'
    report['status'] = 'done'
    save()
    print('ab_verdict', report['verdict'], 'refusals', len(refusals))
    return 0 if not refusals else 1


if __name__ == '__main__':
    raise SystemExit(main())
