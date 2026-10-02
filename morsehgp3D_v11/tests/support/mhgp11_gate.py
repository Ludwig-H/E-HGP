"""Aide des portes Python de la v11 (hors produit).

Python 3.10 nu : bibliotheque standard seulement. Jamais assert : une porte rend le meme code sous python3 et sous
python3 -O (la porte mhgp11_style refuse toute instruction assert ; cmake/gates.cmake rejoue les portes sous -O).

Codes de sortie des portes (docs/ARCHITECTURE.md, paragraphe 5) :
    0 conforme, 1 desaccord d'un juge, 2 refus avant calcul, 3 plancher ou invariant viole, 4 mutant tue.

Import depuis une porte de tests/<module>/ (aucune variable d'environnement requise) :

    import os, sys
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'support'))
    import mhgp11_gate

Usage :

    gate = mhgp11_gate.Gate('ma_porte')
    result = mhgp11_gate.run([binaire, '--option'], timeout=60)
    gate.check_eq(result.code, 0, 'code de la sonde')
    sys.exit(gate.finish(floor=12))      # 0, 1 (echecs) ou 3 (moins de 12 controles joues)

Donnees LiDAR : mhgp11_gate.data_dir() rend le dossier de la variable MHGP11_DATA_DIR ou None ; require_data_dir()
quitte avec le code 2 s'il manque. Sous CTest, une porte de label lidar est sautee avant meme d'etre lancee
(cmake/run_expect.cmake) ; un programme n'ecrit jamais lui-meme le jeton de saut.

Issue d'une porte CTest : run_ctest_gate joue une porte par son nom exact et lit son issue STRUCTUREE (rapport JUnit
de CTest), jamais le seul code de sortie de ctest : une porte sautee rend 0 comme une porte passee, et une erreur de
chargement de CTest rend 8 comme une porte en echec (audits du 2 octobre 2026).
"""
import os
import re
import subprocess
import sys
import xml.etree.ElementTree

OK = 0
DISAGREEMENT = 1
REFUSAL = 2
FLOOR = 3
MUTANT_KILLED = 4

DATA_ENV = 'MHGP11_DATA_DIR'


class Gate:
    """Compte les controles et les echecs d'une porte ; finish rend son code de sortie."""

    def __init__(self, name):
        self.name = name
        self.checks = 0
        self.failures = 0

    def check(self, ok, what):
        """Compte un controle ; ecrit l'echec et continue. Rend ok."""
        self.checks += 1
        if not ok:
            self.failures += 1
            print('ECHEC %s : %s' % (self.name, what))
            sys.stdout.flush()
        return bool(ok)

    def check_eq(self, got, want, what):
        """Controle d'egalite ; ecrit les deux valeurs en cas d'echec."""
        return self.check(got == want, '%s : obtenu %r, attendu %r' % (what, got, want))

    def finish(self, floor):
        """Ligne finale et code : 1 si un controle a echoue, 3 si moins de `floor` controles, sinon 0."""
        sys.stdout.flush()
        if self.failures:
            print('ECHECS %s %d sur %d controles' % (self.name, self.failures, self.checks))
            return DISAGREEMENT
        if self.checks < floor:
            print('PLANCHER %s : %d controles, au moins %d attendus' % (self.name, self.checks, floor))
            return FLOOR
        print('%s_ok controles=%d' % (self.name, self.checks))
        return OK


class Completed:
    """Issue d'un processus : code (None si arret par signal ou delai), signal (0 si aucun), delai, sorties."""

    def __init__(self, code, signal, timed_out, stdout, stderr):
        self.code = code
        self.signal = signal
        self.timed_out = timed_out
        self.stdout = stdout
        self.stderr = stderr

    def describe(self):
        if self.timed_out:
            return 'delai depasse'
        if self.signal:
            return 'signal %d' % self.signal
        return 'code %d' % self.code


def run(argv, timeout=300, env=None, cwd=None, stdin=None, text=True):
    """Lance un processus et attend sa fin. Un arret par signal n'est jamais un code : code vaut alors None."""
    decoding = {'encoding': 'utf-8', 'errors': 'backslashreplace'} if text else {}
    try:
        done = subprocess.run(argv, capture_output=True, text=text, timeout=timeout, env=env, cwd=cwd, input=stdin,
                              **decoding)
    except subprocess.TimeoutExpired as expired:
        def partial(data):
            raw = data or b''
            return raw.decode('utf-8', errors='backslashreplace') if text and isinstance(raw, bytes) else raw
        return Completed(None, 0, True, partial(expired.stdout), partial(expired.stderr))
    if done.returncode < 0:
        return Completed(None, -done.returncode, False, done.stdout, done.stderr)
    return Completed(done.returncode, 0, False, done.stdout, done.stderr)


def expect_code(gate, argv, expected, what, **options):
    """Lance argv et controle son code exact (un signal ou un delai est un echec). Rend l'issue."""
    result = run(argv, **options)
    gate.check(result.code == expected, '%s : %s, attendu code %d' % (what, result.describe(), expected))
    return result


def data_dir():
    """Dossier des donnees LiDAR (variable MHGP11_DATA_DIR), ou None s'il est absent."""
    path = os.environ.get(DATA_ENV, '')
    return path if path and os.path.isdir(path) else None


def require_data_dir():
    """Dossier des donnees LiDAR ; s'il manque, le dit et quitte avec le code 2 (refus avant calcul)."""
    path = data_dir()
    if path is None:
        print('donnees absentes : la variable %s ne nomme pas un dossier' % DATA_ENV)
        sys.exit(REFUSAL)
    return path


class GateRun:
    """Issue d'une porte CTest.

    status  : 'passe' (executee et passee), 'echec' (executee et en echec), 'sautee' (non executee : Skipped,
              Not Run, Disabled), 'absente' (aucune porte de ce nom, ou rapport de CTest illisible)
    verdict : dernier mot de run_expect.cmake (conforme, code, ligne_absente, arret_anormal, lancement_impossible,
              jeton_usurpe, sautee), ou '' si le script n'a rien dit
    timed_out : CTest a coupe la porte a son delai
    """

    def __init__(self, status, verdict, timed_out, output):
        self.status = status
        self.verdict = verdict
        self.timed_out = timed_out
        self.output = output


def run_ctest_gate(ctest, build, name, timeout=7500, env=None):
    """Joue la porte CTest `name` (nom exact) du build et rend son issue structuree."""
    report = os.path.join(build, 'mhgp11_issue_%s_%d.xml' % (name, os.getpid()))
    if os.path.exists(report):
        os.remove(report)
    done = run([ctest, '--test-dir', build, '-R', '^%s$' % re.escape(name), '--output-on-failure',
                '--no-tests=error', '--output-junit', report], timeout=timeout, env=env)
    output = (done.stdout or '') + (done.stderr or '')
    verdicts = re.findall(r'^run_expect_verdict (\w+)', output, re.M)
    verdict = verdicts[-1] if verdicts else ''
    timed_out = re.search(r'Test +#\d+: %s \.*\*\*\*Timeout' % re.escape(name), output) is not None
    try:
        # A faulty program may emit non-UTF-8 bytes into the escaped system-out element.
        # Preserve those bytes visibly without losing the structured CTest status.
        with open(report, encoding='utf-8', errors='backslashreplace') as handle:
            root = xml.etree.ElementTree.fromstring(handle.read())
        cases = [(case.get('name'), case.get('status'))
                 for case in root.iter('testcase')]
    except (OSError, xml.etree.ElementTree.ParseError):
        cases = []
    finally:
        if os.path.exists(report):
            os.remove(report)
    if len(cases) != 1 or cases[0][0] != name:
        return GateRun('absente', verdict, timed_out, output)
    if cases[0][1] == 'run':  # executee et passee ; un code de ctest non nul contredirait le rapport
        return GateRun('passe' if done.code == 0 else 'absente', verdict, timed_out, output)
    if cases[0][1] == 'fail':
        return GateRun('echec', verdict, timed_out, output)
    return GateRun('sautee', verdict, timed_out, output)
