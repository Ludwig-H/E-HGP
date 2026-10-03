"""Profilage G4 de FULL K1..5 (developpement, dev_snapshot) : perf a W1 et W48, variantes ISA a W48.

Aucune decision du produit ne change : les memes sources sont construites avec des drapeaux differents ;
l'identite des sorties est controlee par l'empreinte sha256 du dump de chaque variante et trame.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')
ARGS_TAIL = ['5', '16', '256', '0', '4294967295', '8589934592']


def run(cmd, out_dir, name, timeout, env=None, cwd=None):
    t0 = time.monotonic()
    with open(out_dir / (name + '.stdout'), 'wb') as o, open(out_dir / (name + '.stderr'), 'wb') as e:
        try:
            p = subprocess.run(cmd, stdout=o, stderr=e, timeout=timeout, env=env, cwd=cwd, check=False)
            code = p.returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
    return {'name': name, 'cmd': cmd, 'code': code, 'seconds': round(time.monotonic() - t0, 3)}


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 22), b''):
            h.update(chunk)
    return h.hexdigest()


def full_line(stdout_path):
    for line in Path(stdout_path).read_text(errors='replace').splitlines():
        try:
            j = json.loads(line)
        except ValueError:
            continue
        if j.get('phase') == 'full':
            return {k: j.get(k) for k in ('status', 'wall_ns', 'index_ns', 'domain_ns', 'forest_ns', 'cpu_seconds',
                                         'peak_reserved_bytes', 'phases')}
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--src', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--data', required=True)
    ap.add_argument('--work', required=True)
    ap.add_argument('--reps', type=int, default=3)
    a = ap.parse_args()
    out = Path(a.out); out.mkdir(parents=True, exist_ok=True)
    work = Path(a.work); work.mkdir(parents=True, exist_ok=True)
    report = {'schema': 'ehgp.v11.claude_profile.v1', 'steps': [], 'builds': {}, 'timings': [], 'identity': {}}

    def save():
        (out / 'profile_report.json').write_text(json.dumps(report, indent=1) + '\n')

    src = Path(a.src) / 'morsehgp3D_v11'
    variants = {'prof': ['-DCMAKE_CXX_FLAGS=-fno-omit-frame-pointer -g'], 'base': [],
                'v3': ['-DMHGP11_MARCH=x86-64-v3'], 'v4': ['-DMHGP11_MARCH=x86-64-v4']}
    for name, flags in variants.items():
        bdir = work / ('b_' + name)
        cfg = ['cmake', '-S', str(src), '-B', str(bdir), '-DCMAKE_BUILD_TYPE=Release', '-DMHGP11_COORD_BITS=21'] + flags
        report['steps'].append(run(cfg, out, 'cfg_' + name, 300)); save()
        report['steps'].append(run(['cmake', '--build', str(bdir), '-j', '48', '--target', 'mhgp11_full_bench'],
                                   out, 'build_' + name, 900)); save()
        exe = bdir / 'mhgp11_full_bench'
        report['builds'][name] = {'flags': flags, 'exists': exe.exists(),
                                  'sha256': sha256(exe) if exe.exists() else None}
        save()
    # perf
    perf = shutil.which('perf')
    if perf is None:
        kern = subprocess.run(['uname', '-r'], capture_output=True, text=True).stdout.strip()
        report['steps'].append(run(['sudo', '-n', 'apt-get', 'update', '-qq'], out, 'apt_update', 300))
        for pk in (['linux-tools-common', 'linux-tools-' + kern], ['linux-tools-common', 'linux-tools-gcp']):
            report['steps'].append(run(['sudo', '-n', 'env', 'DEBIAN_FRONTEND=noninteractive', 'apt-get',
                                        'install', '-y', '-qq'] + pk, out, 'apt_' + pk[-1], 400))
            perf = shutil.which('perf')
            if perf:
                break
        save()
    report['perf'] = perf
    if perf:
        run(['sudo', '-n', 'sysctl', '-w', 'kernel.perf_event_paranoid=-1'], out, 'paranoid', 30)
        run(['sudo', '-n', 'sysctl', '-w', 'kernel.kptr_restrict=0'], out, 'kptr', 30)
    data = Path(a.data)
    exe = work / 'b_prof' / 'mhgp11_full_bench'
    for w in (1, 48):
        base = [str(exe), str(data / 'lidar_ng00.u32le'), str(data / 'lidar_ng00.ids.u32le'), '/dev/null'] + \
            ARGS_TAIL + [str(w), '16379']
        if perf:
            pdata = work / ('perf_w%d.data' % w)
            cmd = [perf, 'record', '-F', '1999' if w == 1 else '499', '-g', '-o', str(pdata), '--'] + base
            report['steps'].append(run(cmd, out, 'perf_w%d' % w, 600)); save()
            for kind, extra in (('self', ['--no-children', '--sort', 'symbol', '--percent-limit', '0.2']),
                                ('children', ['--children', '--sort', 'symbol', '--percent-limit', '0.8'])):
                rep = run([perf, 'report', '-i', str(pdata), '--stdio', '-g', 'none'] + extra, out,
                          'report_w%d_%s' % (w, kind), 600)
                report['steps'].append(rep)
            caller = run([perf, 'report', '-i', str(pdata), '--stdio', '--no-children', '--sort', 'symbol',
                          '--percent-limit', '2', '-g', 'caller,0.5,callee,function,percent'], out,
                         'report_w%d_callers' % w, 600)
            report['steps'].append(caller)
        else:
            report['steps'].append(run(base, out, 'plain_w%d' % w, 600))
        save()
    # temps et identite par variante
    for rep in range(a.reps):
        for name in ('base', 'v3', 'v4'):
            exe = work / ('b_' + name) / 'mhgp11_full_bench'
            if not exe.exists():
                continue
            for frame in FRAMES:
                dump = work / ('dump_%s_%s.bin' % (name, frame))
                target = str(dump) if rep == 0 else '/dev/null'
                cmd = [str(exe), str(data / (frame + '.u32le')), str(data / (frame + '.ids.u32le')), target] + \
                    ARGS_TAIL + ['48', '16379']
                tag = 't_%s_%s_r%d' % (name, frame, rep)
                st = run(cmd, out, tag, 300)
                st['full'] = full_line(out / (tag + '.stdout'))
                if rep == 0 and dump.exists():
                    st['dump_sha256'] = sha256(dump)
                    report['identity'].setdefault(frame, {})[name] = st['dump_sha256']
                    dump.unlink()
                report['timings'].append(st)
                save()
    report['status'] = 'done'
    save()


if __name__ == '__main__':
    main()
