"""Regression (29 septembre 2026, audit continu timeout/README.md) : au delai, scale_run.run_json tuait /usr/bin/time
mais laissait vivre le calcul, son enfant ; un calcul declare arrete pouvait partager la machine avec les mesures
suivantes. La porte lance de vrais processus qui dorment, flux fermes, et exige :
  - delai de run_json : code -9, enfant ET petit-enfant tues et recoltes avant le retour ;
  - signal recu entre fork et la garde d'un appel : differe, puis appel tue et recolte (Terminated propage) ;
  - appel normal : JSON natif (stdout brut) conserve octet pour octet ;
  - `scale_run.py run` avec de faux binaires : ligne ok et les deux appels natifs dans <out>.calls.jsonl ; compteurs
    de boules differents entre l'appel catalogue et l'appel tour -> statut balls_mismatch et code 3 ; delai de la
    tour -> statut tower_timeout, calcul tue ; SIGTERM (SIGINT) pendant un appel -> code 143 (130), calcul tue et
    recolte.
Aucun calcul geometrique. Python nu (ni numpy ni scipy) : la porte tourne aussi sur la VM G4 (label fast). Aucun
assert : elle tient sous python3 -O. Tout processus survivant est tue par son PID a la fin.

  python3 test_scale_run_timeout.py <dossier de build>   -> code 0 si conforme, 1 sinon
"""
import csv
import importlib.util
import json
import os
import signal
import subprocess
import sys
import tempfile
import time

sys.dont_write_bytecode = True  # scale_run.py est importe en processus : aucun __pycache__ dans l'arbre source
HERE = os.path.dirname(os.path.abspath(__file__))
SCALE_RUN = os.path.normpath(os.path.join(HERE, '..', '..', 'bench', 'scaling', 'scale_run.py'))
PYTHON = os.path.realpath(sys.executable)
DELAY = 3.0  # delai impose aux appels qui dorment (s)
SLACK = 20.0  # marge de retour, fermeture du groupe comprise, sur une machine chargee (s)

# Enfant qui dort : lance un petit-enfant qui dort, publie les deux PID, ferme ses flux (fin des tubes sans fin du
# processus, comme la sonde de l'audit), puis dort.
SLEEPER = r'''
import json, os, subprocess, sys, time
grand = subprocess.Popen([sys.executable, '-S', '-c', 'import time; time.sleep(120)'], stdin=subprocess.DEVNULL,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
with open(sys.argv[1] + '.tmp', 'w') as f:
    json.dump(dict(pids=[os.getpid(), grand.pid]), f)
os.replace(sys.argv[1] + '.tmp', sys.argv[1])
fd = os.open(os.devnull, os.O_RDWR)
for target in (0, 1, 2):
    os.dup2(fd, target)
time.sleep(120)
'''

NATIVE = '{"status":"ok","balls":7,"catalogue_s":0.2520,"tower_s":0.1000}'

# Faux binaires : memes noms et memes arguments que les vrais, JSON de la meme forme. FAKE_SLEEP (tour seulement) :
# publie le PID puis dort, pour les delais et SIGTERM.
FAKE = r'''#!%s -S
import json, os, sys, time
tower = os.path.basename(sys.argv[0]) == 'mhgp10_tower'
marker = os.environ.get('FAKE_SLEEP', '')
if tower and marker:
    with open(marker + '.tmp', 'w') as f:
        json.dump(dict(pids=[os.getpid()]), f)
    os.replace(marker + '.tmp', marker)
    time.sleep(120)
if tower:
    print('{"status":"ok","n":10,"K":5,"balls":%%s,"catalogue_s":0.1250,"tower_s":0.2520,'
          '"orders":[{"k":1,"nodes":3,"steps":4},{"k":5,"nodes":5,"steps":6}]}' %% os.environ['FAKE_TOWER_BALLS'])
else:
    print('{"status":"ok","n":10,"sites":10,"K":5,"balls":%%s,"nodes":11,"leaves":6,"sum_m":40,"judged":9,'
          '"quad_tests":2,"triple_tests":1}' %% os.environ['FAKE_CAT_BALLS'])
''' % PYTHON

SPAWNED = []  # PID publies par les processus de la porte : tues par PID a la fin, quoi qu'il arrive
MARKERS = []  # marqueurs ou ces PID sont publies, relus a la fin meme si un cas a leve une exception


def alive(pid):
    """Vrai tant que le PID existe, zombie non recolte compris."""
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def new_marker(path):
    MARKERS.append(path)
    return path


def read_pids(marker):
    try:
        with open(marker) as f:
            pids = json.load(f)['pids']
    except (OSError, ValueError, KeyError):
        return []
    SPAWNED.extend(pids)
    return pids


def field(rec, key, index):
    """Enregistrement de run_json (dict) ; l'ancien run_json rendait un tuple (code, json, mur, cpu, rss)."""
    if isinstance(rec, dict):
        return rec.get(key)
    return rec[index] if index is not None and index < len(rec) else None


def load_scale_run():
    spec = importlib.util.spec_from_file_location('scale_run_sous_test', SCALE_RUN)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check(results, name, ok, detail):
    results.append(ok)
    print('%-34s %s  %s' % (name, 'ok' if ok else 'ECHEC', detail), flush=True)


def case_delay(tmp, module, results):
    link = os.path.join(tmp, 'python_porte')  # le fichier de temps de run_json va a cote de cmd[0]
    os.symlink(PYTHON, link)
    marker = new_marker(os.path.join(tmp, 'dort.json'))
    module.TIMEOUT = DELAY
    t0 = time.monotonic()
    rec = module.run_json([link, '-S', '-B', '-c', SLEEPER, marker])
    elapsed = time.monotonic() - t0
    pids = read_pids(marker)
    survivors = [p for p in pids if alive(p)]
    leftovers = [f for f in os.listdir(tmp) if '.time.' in f]
    ok = (field(rec, 'code', 0) == -9 and len(pids) == 2 and not survivors and elapsed < DELAY + SLACK and
          not leftovers)
    check(results, 'delai_enfant_et_petit_enfant', ok,
          'code=%s retour=%.2fs pids=%s survivants=%s fichiers_temps=%s' % (field(rec, 'code', 0), elapsed, pids,
                                                                            survivors, leftovers))


def group_alive(pgid):
    try:
        os.killpg(pgid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def case_launch_signal(tmp, module, results):
    """SIGTERM recu entre fork et la garde de l'appel (simule : le gestionnaire de `run` est appele des la creation du
    processus). Le signal doit etre differe, puis leve sous garde : appel tue et recolte, Terminated propage."""
    link = os.path.join(tmp, 'python_lancement')
    os.symlink(PYTHON, link)
    marker = new_marker(os.path.join(tmp, 'lancement.json'))
    real = subprocess.Popen
    launched = []

    class Interrupted(real):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            launched.append(self)
            module.terminate(signal.SIGTERM, None)

    module.TIMEOUT = 60.0
    raised = None
    subprocess.Popen = Interrupted
    try:
        module.run_json([link, '-S', '-B', '-c', SLEEPER, marker])
    except BaseException as exc:  # Terminated derive de BaseException
        raised = exc
    finally:
        subprocess.Popen = real
    proc = launched[0] if launched else None
    closed = proc is not None and proc.returncode is not None and not group_alive(proc.pid)
    pids = read_pids(marker)
    survivors = [p for p in pids if alive(p)]
    for p in launched:  # ancien comportement : l'appel survit, on le tue ici
        if p.returncode is None:
            p.kill()
            p.wait()
    ok = type(raised).__name__ == 'Terminated' and closed and not survivors
    check(results, 'signal_pendant_le_lancement', ok,
          'exception=%r groupe_ferme=%s survivants=%s' % (raised, closed, survivors))


def case_native(tmp, module, results):
    link = os.path.join(tmp, 'python_normal')
    os.symlink(PYTHON, link)
    module.TIMEOUT = 60.0
    rec = module.run_json([link, '-S', '-c', 'print(%r)' % NATIVE])
    data = field(rec, 'json', 1) or {}
    stdout = field(rec, 'stdout', None)
    timed = os.path.exists('/usr/bin/time')
    measured = not timed or (field(rec, 'cpu_s', 3) is not None and field(rec, 'max_rss_kb', 4) is not None)
    ok = (field(rec, 'code', 0) == 0 and data.get('balls') == 7 and stdout == NATIVE + '\n' and measured and
          not [f for f in os.listdir(tmp) if '.time.' in f])
    check(results, 'appel_normal_json_natif', ok,
          'code=%s balls=%s stdout_natif=%s mesures=%s' % (field(rec, 'code', 0), data.get('balls'),
                                                          stdout == NATIVE + '\n', measured))


def fake_build(tmp):
    build = os.path.join(tmp, 'build')
    data = os.path.join(tmp, 'data')
    os.makedirs(build)
    os.makedirs(data)
    for name in ('mhgp10_catalogue', 'mhgp10_tower'):
        path = os.path.join(build, name)
        with open(path, 'w') as f:
            f.write(FAKE)
        os.chmod(path, 0o755)
    with open(os.path.join(data, 'syn_uniform_space_x1.u32le'), 'wb') as f:
        f.write(bytes(12 * 10))
    return build, data


def scale_run(build, data, out, env_extra, extra=()):
    env = dict(os.environ, FAKE_CAT_BALLS='7', FAKE_TOWER_BALLS='7')
    env.update(env_extra)
    return subprocess.Popen([PYTHON, '-S', '-B', SCALE_RUN, 'run', '--build', build, '--data', data, '--out', out,
                             '--k', '5', '--threads', '1'] + list(extra), stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True, env=env, start_new_session=True)


def finish(proc, limit):
    try:
        out, err = proc.communicate(timeout=limit)
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGKILL)
        out, err = proc.communicate()
    try:
        os.killpg(proc.pid, signal.SIGKILL)  # reste eventuel de la session de scale_run (ancien comportement)
    except ProcessLookupError:
        pass
    return proc.returncode, out, err


def read_rows(path):
    try:
        with open(path, newline='') as f:
            return list(csv.DictReader(f))
    except OSError:
        return []


def read_calls(path):
    try:
        with open(path) as f:
            return [json.loads(line) for line in f if line.strip()]
    except (OSError, ValueError):
        return []


def case_cli(tmp, results):
    build, data = fake_build(tmp)
    out = os.path.join(tmp, 'ok.csv')
    code, _, err = finish(scale_run(build, data, out, {}), 60)
    rows, calls = read_rows(out), read_calls(out + '.calls.jsonl')
    row = rows[0] if len(rows) == 1 else {}
    tower = [c for c in calls if c.get('call') == 'tower']
    ok = (code == 0 and row.get('status') == 'ok' and row.get('balls') == '7' and row.get('tower_nodes_all') == '8'
          and row.get('tower_steps_all') == '10' and [c.get('call') for c in calls] == ['catalogue', 'tower'] and
          all(c.get('code') == 0 and c.get('file') == 'syn_uniform_space_x1.u32le' and c.get('k') == 5
              for c in calls) and
          len(tower) == 1 and '"tower_s":0.2520,' in tower[0].get('stdout', ''))
    check(results, 'run_ligne_ok_appels_natifs', ok,
          'code=%s statut=%s appels=%s %s' % (code, row.get('status'), [c.get('call') for c in calls],
                                               err.strip()[-160:]))

    out = os.path.join(tmp, 'ecart.csv')
    code, _, err = finish(scale_run(build, data, out, dict(FAKE_TOWER_BALLS='8')), 60)
    rows, calls = read_rows(out), read_calls(out + '.calls.jsonl')
    row = rows[0] if len(rows) == 1 else {}
    ok = code == 3 and row.get('status') == 'balls_mismatch' and len(calls) == 2
    check(results, 'run_compteurs_de_boules_egaux', ok,
          'code=%s statut=%s appels=%d %s' % (code, row.get('status'), len(calls), err.strip()[-160:]))

    out = os.path.join(tmp, 'delai.csv')
    marker = new_marker(os.path.join(tmp, 'tour_delai.json'))
    t0 = time.monotonic()
    code, _, err = finish(scale_run(build, data, out, dict(FAKE_SLEEP=marker), ('--timeout', str(DELAY))),
                          DELAY + SLACK + 30)
    elapsed = time.monotonic() - t0
    pids = read_pids(marker)
    survivors = [p for p in pids if alive(p)]
    rows, calls = read_rows(out), read_calls(out + '.calls.jsonl')
    row = rows[0] if len(rows) == 1 else {}
    ok = (code == 0 and row.get('status') == 'tower_timeout' and len(pids) == 1 and not survivors and
          [(c.get('call'), c.get('timed_out'), c.get('code')) for c in calls] == [('catalogue', False, 0),
                                                                                  ('tower', True, -9)])
    check(results, 'run_delai_tour_tuee', ok,
          'code=%s statut=%s pids=%s survivants=%s duree=%.2fs %s' % (code, row.get('status'), pids, survivors,
                                                                      elapsed, err.strip()[-160:]))

    for sig, name in ((signal.SIGTERM, 'run_sigterm_appel_tue'), (signal.SIGINT, 'run_sigint_appel_tue')):
        out = os.path.join(tmp, name + '.csv')
        marker = new_marker(os.path.join(tmp, name + '.json'))
        proc = scale_run(build, data, out, dict(FAKE_SLEEP=marker))
        deadline = time.monotonic() + 60
        while not os.path.exists(marker) and proc.poll() is None and time.monotonic() < deadline:
            time.sleep(0.02)
        started = os.path.exists(marker)
        if started:
            os.kill(proc.pid, sig)
        code, _, err = finish(proc, SLACK + 30)
        pids = read_pids(marker)
        survivors = [p for p in pids if alive(p)]
        ok = started and code == 128 + sig and len(pids) == 1 and not survivors
        check(results, name, ok, 'code=%s pids=%s survivants=%s %s' % (code, pids, survivors, err.strip()[-160:]))


def main():
    if len(sys.argv) != 2:
        print('usage : test_scale_run_timeout.py <dossier de build>')
        return 2
    results = []
    # dispositions connues pour les scale_run lances (un signal ignore ici le serait aussi par eux)
    signal.signal(signal.SIGTERM, signal.SIG_DFL)
    signal.signal(signal.SIGINT, signal.default_int_handler)
    with tempfile.TemporaryDirectory(prefix='mhgp10-scale-run-porte-') as tmp:
        try:
            try:
                module = load_scale_run()
            except Exception as exc:
                module = None
                check(results, 'chargement_scale_run', False, 'exception %r' % exc)
            for case in (case_delay, case_launch_signal, case_native) if module is not None else ():
                sub = os.path.join(tmp, case.__name__)
                os.makedirs(sub)
                try:
                    case(sub, module, results)
                except Exception as exc:  # une API absente ou cassee est un echec, pas un crash de la porte
                    check(results, case.__name__, False, 'exception %r' % exc)
            sub = os.path.join(tmp, 'cli')
            os.makedirs(sub)
            try:
                case_cli(sub, results)
            except Exception as exc:
                check(results, 'case_cli', False, 'exception %r' % exc)
        finally:
            for path in MARKERS:  # relus ici, avant l'effacement du dossier, meme apres une exception
                read_pids(path)
            for pid in SPAWNED:  # nettoyage par PID, jamais par nom
                try:
                    os.kill(pid, signal.SIGKILL)
                except (ProcessLookupError, PermissionError):
                    pass
    failures = results.count(False)
    print('scale_run_timeout_ok' if results and not failures else 'ECHECS %d' % failures)
    return 1 if failures or not results else 0


if __name__ == '__main__':
    sys.exit(main())
