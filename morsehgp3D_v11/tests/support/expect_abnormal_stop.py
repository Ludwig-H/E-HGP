"""Juge une terminaison par signal avec le statut reel du processus (POSIX).

La sortie de la sonde est transmise telle quelle, mais ne decide jamais le verdict. Aucun shell intermediaire :
un programme absent, non executable ou dont l'interprete manque leve OSError, sans simuler un signal.
Codes : 0 signal constate ; 1 terminaison normale, quel que soit son code ; 2 usage ou lancement impossible.
Le delai et l'arret des descendants restent ceux de la porte CTest ; aucun groupe detache n'est cree ici.
"""
import os
import subprocess
import sys


def main(argv):
    if not argv or not os.path.isabs(argv[0]):
        print('abnormal_stop_verdict usage')
        return 2
    try:
        completed = subprocess.run(argv, check=False)
    except OSError as error:
        print('abnormal_stop_verdict lancement_impossible')
        print(str(error), file=sys.stderr)
        return 2
    if completed.returncode >= 0:
        print('abnormal_stop_verdict code %d' % completed.returncode)
        return 1
    print('abnormal_stop_verdict conforme signal %d' % -completed.returncode)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
