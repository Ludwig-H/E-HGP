"""Execute un script Python suivi du depot sous l'environnement Python portable, sur la VM G4 (campagnes dev longues).

La VM n'a ni pip ni numpy : la session televerse l'environnement comme donnee (conteneur `.u32le`, voir
lot_runner.py). Ce lanceur, execute par le python3 systeme, l'extrait dans --work puis relance le script cible sous le
Python portable, un fil par processus pour les bibliotheques numeriques. Les binaires sont ceux que la session a
construits depuis le commit ({build}), donc lies au glibc de la VM.

  python3 pyenv_run.py --env ENV.u32le --work DIR -- SCRIPT.py [arguments du script]
Codes : ceux du script ; 2 si le conteneur est invalide ou le script absent.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from lot_runner import extract  # noqa: E402


def main():
    argv = sys.argv[1:]
    if '--' not in argv:
        print('REFUS : « -- SCRIPT.py ... » attendu', flush=True)
        return 2
    cut = argv.index('--')
    head, target = argv[:cut], argv[cut + 1:]
    opts = dict(zip(head[::2], head[1::2]))
    if set(opts) != {'--env', '--work'} or not target or not os.path.isfile(target[0]):
        print('REFUS : --env ENV --work DIR -- SCRIPT.py attendus', flush=True)
        return 2
    work = os.path.abspath(opts['--work'])
    try:
        extract(opts['--env'], os.path.join(work, 'pyenv'))
    except (OSError, ValueError) as e:
        print('REFUS', e, flush=True)
        return 2
    python = os.path.join(work, 'pyenv', 'python', 'bin', 'python3')
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1', PYTHONNOUSERSITE='1')
    env.pop('PYTHONPATH', None)
    os.execve(python, [python] + target, env)
    return 2


if __name__ == '__main__':
    sys.exit(main())
