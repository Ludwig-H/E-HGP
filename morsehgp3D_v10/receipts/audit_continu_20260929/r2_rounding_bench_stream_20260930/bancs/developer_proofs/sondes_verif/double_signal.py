"""Sonde adverse : scale_run.py sous l'enveloppe exacte d'une etape du worker G4 v10, puis SIGTERM au groupe de
l'etape (comme on_signal de v10_worker.sh) ou deux SIGTERM directs rapproches.

Enveloppe (v10_worker.sh:209-210) : setsid -w /usr/bin/time -v -o time.txt timeout --foreground --kill-after=10s N cmd.
`timeout` relaie SIGTERM a la commande : scale_run recoit donc deux SIGTERM tres rapproches (le noyau via le groupe,
puis timeout). Le faux mhgp10_tower publie son PID puis dort ; on juge s'il survit a la sortie de scale_run.

  python3 double_signal.py <scale_run.py> <dossier de travail> <mode> <repetitions> [ecart_us]
  mode : groupe (killpg du groupe de l'etape) | direct2 (deux os.kill a ecart_us) | direct1 (un seul os.kill)
         | rafale (SIGTERM repetes 3 ms, espaces de ecart_us)
Sortie : une ligne JSON par repetition, puis un resume. Nettoyage par PID publie (jamais par nom).
"""
import json
import os
import signal
import subprocess
import sys
import time

PY = os.path.realpath(sys.executable)
FAKE = r'''#!%s -S
import json, os, sys, time
tower = os.path.basename(sys.argv[0]) == 'mhgp10_tower'
marker = os.environ.get('FAKE_SLEEP', '')
if tower and marker:
    with open(marker + '.tmp', 'w') as f:
        json.dump(dict(pid=os.getpid(), pgid=os.getpgrp()), f)
    os.replace(marker + '.tmp', marker)
    time.sleep(60)
if tower:
    print('{"status":"ok","balls":7,"catalogue_s":0.1,"tower_s":0.2,"orders":[{"k":5,"nodes":5,"steps":6}]}')
else:
    print('{"status":"ok","balls":7,"nodes":11,"leaves":6,"sum_m":40,"judged":9,"quad_tests":2,"triple_tests":1}')
''' % PY


def alive(pid):
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    try:
        with open('/proc/%d/stat' % pid) as f:
            return f.read().split(') ')[1].split()[0] != 'Z'
    except OSError:
        return False


def setup(work):
    build = os.path.join(work, 'build')
    data = os.path.join(work, 'data')
    os.makedirs(build, exist_ok=True)
    os.makedirs(data, exist_ok=True)
    for name in ('mhgp10_catalogue', 'mhgp10_tower'):
        p = os.path.join(build, name)
        with open(p, 'w') as f:
            f.write(FAKE)
        os.chmod(p, 0o755)
    with open(os.path.join(data, 'syn_uniform_space_x1.u32le'), 'wb') as f:
        f.write(bytes(120))
    return build, data


def one(scale_run, work, build, data, mode, i, gap_us):
    marker = os.path.join(work, 'tour_%s_%d.json' % (mode, i))
    out = os.path.join(work, 'out_%s_%d.csv' % (mode, i))
    env = dict(os.environ, FAKE_SLEEP=marker)
    inner = [PY, '-S', '-B', scale_run, 'run', '--build', build, '--data', data, '--out', out, '--k', '5',
             '--threads', '1']
    if mode == 'groupe':
        argv = ['setsid', '-w', '/usr/bin/time', '-v', '-o', os.path.join(work, 'time_%d.txt' % i), 'timeout',
                '--foreground', '--kill-after=10s', '600'] + inner
    else:
        argv = inner
    proc = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True, env=env, start_new_session=(mode != 'groupe'))
    deadline = time.monotonic() + 30
    while not os.path.exists(marker) and proc.poll() is None and time.monotonic() < deadline:
        time.sleep(0.01)
    tower = None
    if os.path.exists(marker):
        with open(marker) as f:
            tower = json.load(f)
    group = proc.pid
    if mode == 'groupe':
        # le groupe de l'etape : setsid (sans fork, l'appelant n'est pas chef de groupe) -> pgid = proc.pid
        os.killpg(group, signal.SIGTERM)
    elif mode == 'direct2':
        os.kill(proc.pid, signal.SIGTERM)
        if gap_us:
            t = time.perf_counter() + gap_us * 1e-6
            while time.perf_counter() < t:
                pass
        os.kill(proc.pid, signal.SIGTERM)
    elif mode == 'rafale':  # SIGTERM repetes pendant 3 ms, espaces de gap_us
        end = time.perf_counter() + 0.003
        while time.perf_counter() < end:
            try:
                os.kill(proc.pid, signal.SIGTERM)
            except ProcessLookupError:
                break
            t = time.perf_counter() + gap_us * 1e-6
            while time.perf_counter() < t:
                pass
    else:
        os.kill(proc.pid, signal.SIGTERM)
    try:
        so, se = proc.communicate(timeout=25)
    except subprocess.TimeoutExpired:
        so, se = '', 'bloque'
        try:
            os.killpg(group, signal.SIGKILL)
        except ProcessLookupError:
            pass
        proc.wait()
    time.sleep(0.3)
    survived = tower is not None and alive(tower['pid'])
    if tower is not None:  # nettoyage par PID publie
        try:
            os.kill(tower['pid'], signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
    tail = (se or '').strip().splitlines()[-1:] if se else []
    with open(os.path.join(work, 'stderr_%s_%d.txt' % (mode, i)), 'w') as f:
        f.write(se or '')
    return dict(mode=mode, i=i, gap_us=gap_us, code=proc.returncode, tower=tower, tower_survived=survived,
                stderr_tail=tail)


def main():
    scale_run, work, mode, reps = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
    gap_us = float(sys.argv[5]) if len(sys.argv) > 5 else 0.0
    os.makedirs(work, exist_ok=True)
    build, data = setup(work)
    results = []
    for i in range(reps):
        r = one(scale_run, work, build, data, mode, i, gap_us)
        results.append(r)
        print(json.dumps(r), flush=True)
    summ = dict(mode=mode, reps=reps, gap_us=gap_us, survivals=sum(r['tower_survived'] for r in results),
                codes=sorted({r['code'] for r in results}))
    print('RESUME ' + json.dumps(summ), flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
