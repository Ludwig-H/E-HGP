"""Sonde adverse deterministe : un second signal traite a l'entree de close_group, avant le masquage.

CPython 3.12 execute les gestionnaires Python aux points de controle de la boucle d'evaluation : RESUME d'une fonction
et retour d'un appel de fonction C. Dans close_group, les premiers points sont RESUME, le retour de hasattr(...) et le
retour de signal.pthread_sigmask(...) (un signal arrive juste avant le masquage reste a traiter). On simule ce second
signal comme la porte du correcteur simule le signal du lancement : on appelle scale_run.terminate(SIGTERM) depuis
le premier appel a pthread_sigmask (proxy du module signal), c'est-a-dire au point de controle qui suit hasattr.

Deux chemins : (1) SIGTERM pendant l'appel (Terminated leve dans communicate, puis close_group) ; (2) delai
(TimeoutExpired, puis close_group). On juge si l'enfant et le petit-enfant sont vivants au retour de run_json.

  python3 signal_dans_close_group.py <scale_run.py>
Code 0 si la sonde conclut ; ligne RESULTAT par chemin.
"""
import importlib.util
import json
import os
import signal as real_signal
import subprocess
import sys
import tempfile
import time

SLEEPER = r'''
import json, os, subprocess, sys, time
grand = subprocess.Popen([sys.executable, '-S', '-c', 'import time; time.sleep(60)'], stdin=subprocess.DEVNULL,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
with open(sys.argv[1] + '.tmp', 'w') as f:
    json.dump(dict(pids=[os.getpid(), grand.pid]), f)
os.replace(sys.argv[1] + '.tmp', sys.argv[1])
fd = os.open(os.devnull, os.O_RDWR)
for target in (0, 1, 2):
    os.dup2(fd, target)
time.sleep(60)
'''


def alive(pid):
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    try:
        with open('/proc/%d/stat' % pid) as f:
            return f.read().split(') ')[1].split()[0] != 'Z'
    except OSError:
        return False


def load(path):
    spec = importlib.util.spec_from_file_location('scale_run_sonde', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Proxy:
    """Module signal dont le premier pthread_sigmask execute d'abord le gestionnaire de scale_run (second signal)."""

    def __init__(self, module):
        self.module = module
        self.fired = 0

    def __getattr__(self, name):
        return getattr(real_signal, name)

    def pthread_sigmask(self, how, mask):
        if not self.fired:
            self.fired += 1
            self.module.terminate(real_signal.SIGTERM, None)  # point de controle apres hasattr : leve Terminated
        return real_signal.pthread_sigmask(how, mask)


def case(path, tmp, which):
    module = load(path)
    link = os.path.join(tmp, 'py_' + which)
    os.symlink(os.path.realpath(sys.executable), link)
    marker = os.path.join(tmp, which + '.json')
    module.signal = Proxy(module)
    raised = None
    if which == 'pendant_appel':
        module.TIMEOUT = 60.0
        real_popen = subprocess.Popen

        class First(real_popen):  # premier signal : pendant le lancement (differe puis leve sous garde)
            def __init__(self, *a, **k):
                super().__init__(*a, **k)
                t = time.monotonic() + 20
                while not os.path.exists(marker) and time.monotonic() < t:
                    time.sleep(0.01)
                module.terminate(real_signal.SIGTERM, None)
        subprocess.Popen = First
        try:
            module.run_json([link, '-S', '-B', '-c', SLEEPER, marker])
        except BaseException as exc:
            raised = exc
        finally:
            subprocess.Popen = real_popen
    else:
        module.TIMEOUT = 2.0
        try:
            module.run_json([link, '-S', '-B', '-c', SLEEPER, marker])
        except BaseException as exc:
            raised = exc
    try:
        with open(marker) as f:
            pids = json.load(f)['pids']
    except (OSError, ValueError):
        pids = []
    survivors = [p for p in pids if alive(p)]
    for p in pids:  # nettoyage par PID
        try:
            os.kill(p, real_signal.SIGKILL)
        except ProcessLookupError:
            pass
    print('RESULTAT %s' % json.dumps(dict(chemin=which, exception=repr(raised), pids=pids, survivants=survivors,
                                          second_signal_simule=module.signal.fired)), flush=True)
    return survivors


def main():
    path = sys.argv[1]
    with tempfile.TemporaryDirectory(prefix='verif-bancs-cg-') as tmp:
        s1 = case(path, tmp, 'pendant_appel')
        s2 = case(path, tmp, 'delai')
    print('SURVIVANTS pendant_appel=%d delai=%d' % (len(s1), len(s2)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
