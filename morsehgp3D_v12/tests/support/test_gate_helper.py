"""Porte de l'aide mhgp12_gate.py elle-meme : codes de finish, refus des signaux et des delais, donnees absentes.

    python3 test_gate_helper.py     -> 0 conforme, 1 desaccord, 3 plancher
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mhgp12_gate  # noqa: E402

FLOOR = 28


def binary_output(gate, python):
    script = 'import os; os.write(1, bytes([255])); os.write(2, bytes([254]))'
    done = mhgp12_gate.run([python, '-c', script])
    gate.check_eq((done.code, done.stdout, done.stderr), (0, r'\xff', r'\xfe'), 'octets invalides visibles')
    raw = mhgp12_gate.run([python, '-c', script], text=False)
    gate.check_eq((raw.stdout, raw.stderr), (b'\xff', b'\xfe'), 'mode octets sans transformation')
    slow = mhgp12_gate.run([python, '-c', script + '; import time; time.sleep(30)'], timeout=0.5)
    gate.check_eq((slow.code, slow.timed_out), (None, True), 'delai apres sortie binaire')
    gate.check_eq((slow.stdout, slow.stderr), (r'\xff', r'\xfe'), 'sorties partielles textuelles')
    with tempfile.TemporaryDirectory() as folder:
        stub = os.path.join(folder, 'ctest')
        with open(stub, 'w') as handle:
            handle.write('#!' + python + '\nimport os, sys\n'
                         'report = sys.argv[sys.argv.index("--output-junit") + 1]\n'
                         'with open(report, "wb") as f:\n'
                         ' f.write(b\'<testsuite><testcase name="mhgp12_binary" status="fail">'
                         '<system-out>\\xff</system-out></testcase></testsuite>\')\n'
                         'os.write(1, b"\\xff\\nrun_expect_verdict code\\n")\nsys.exit(8)\n')
        os.chmod(stub, 0o700)
        issue = mhgp12_gate.run_ctest_gate(stub, folder, 'mhgp12_binary')
        gate.check_eq(issue.status, 'echec', 'JUnit contenant un octet invalide : statut conserve')
        gate.check_eq(issue.verdict, 'code', 'verdict conserve apres octet invalide')
        gate.check(r'\xff' in issue.output, 'octet invalide conserve dans le diagnostic')


def silent_finish(gate, floor):
    """finish sans ses lignes : les echecs voulus des portes internes ne doivent pas salir la sortie."""
    out = sys.stdout
    sys.stdout = open(os.devnull, 'w')
    try:
        return gate.finish(floor)
    finally:
        sys.stdout.close()
        sys.stdout = out


def silent_check(gate, ok):
    out = sys.stdout
    sys.stdout = open(os.devnull, 'w')
    try:
        return gate.check(ok, 'controle interne')
    finally:
        sys.stdout.close()
        sys.stdout = out


def main():
    gate = mhgp12_gate.Gate('gate_helper')
    python = sys.executable
    binary_output(gate, python)

    # finish : 0, 1 (un echec), 3 (plancher), et l'echec prime sur le plancher
    inner = mhgp12_gate.Gate('interne')
    inner.check(True, 'vrai')
    gate.check_eq(silent_finish(inner, 1), 0, 'finish conforme')
    gate.check_eq(silent_finish(inner, 2), 3, 'finish sous le plancher')
    gate.check_eq(silent_check(inner, False), False, 'check rend faux')
    gate.check_eq(silent_finish(inner, 1), 1, 'finish avec un echec')
    gate.check_eq(silent_finish(inner, 99), 1, "l'echec prime sur le plancher")
    gate.check_eq((inner.checks, inner.failures), (2, 1), 'comptes')

    # run : code exact, sorties, signal, delai
    done = mhgp12_gate.run([python, '-c', 'import sys; print("bonjour"); sys.exit(3)'])
    gate.check_eq(done.code, 3, 'code exact')
    gate.check_eq(done.stdout, 'bonjour\n', 'sortie standard')
    gate.check_eq((done.signal, done.timed_out), (0, False), 'ni signal ni delai')
    killed = mhgp12_gate.run([python, '-c', 'import os, signal; os.kill(os.getpid(), signal.SIGKILL)'])
    gate.check_eq(killed.code, None, 'un signal ne rend pas de code')
    gate.check_eq(killed.signal, 9, 'numero du signal')
    gate.check_eq(killed.describe(), 'signal 9', 'description du signal')
    slow = mhgp12_gate.run([python, '-c', 'import time; time.sleep(30)'], timeout=0.5)
    gate.check_eq((slow.code, slow.timed_out), (None, True), 'delai depasse')

    # expect_code : un signal ou un autre code est un echec de la porte appelante
    inner = mhgp12_gate.Gate('interne')
    out = sys.stdout
    sys.stdout = open(os.devnull, 'w')
    try:
        mhgp12_gate.expect_code(inner, [python, '-c', 'import sys; sys.exit(2)'], 2, 'code 2')
        mhgp12_gate.expect_code(inner, [python, '-c', 'import sys; sys.exit(0)'], 2, 'code 0 au lieu de 2')
        mhgp12_gate.expect_code(inner, [python, '-c', 'import os; os.kill(os.getpid(), 9)'], 0, 'signal')
    finally:
        sys.stdout.close()
        sys.stdout = out
    gate.check_eq((inner.checks, inner.failures), (3, 2), 'expect_code : un conforme, deux echecs')

    # donnees : variable absente, dossier absent, dossier present
    saved = os.environ.pop(mhgp12_gate.DATA_ENV, None)
    try:
        gate.check_eq(mhgp12_gate.data_dir(), None, 'variable absente')
        os.environ[mhgp12_gate.DATA_ENV] = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'absent')
        gate.check_eq(mhgp12_gate.data_dir(), None, 'dossier absent')
        here = os.path.dirname(os.path.abspath(__file__))
        os.environ[mhgp12_gate.DATA_ENV] = here
        gate.check_eq(mhgp12_gate.data_dir(), here, 'dossier present')
        gate.check_eq(mhgp12_gate.require_data_dir(), here, 'require_data_dir rend le dossier')
        del os.environ[mhgp12_gate.DATA_ENV]
        script = ('import sys; sys.path.insert(0, %r); import mhgp12_gate; mhgp12_gate.require_data_dir()' % here)
        env = dict(os.environ)
        skipped = mhgp12_gate.run([python, '-c', script], env=env)
        gate.check_eq(skipped.code, 2, 'require_data_dir sans donnees : code 2')
        gate.check_eq(skipped.stdout, 'donnees absentes : la variable MHGP12_DATA_DIR ne nomme pas un dossier\n',
                      'message de donnees absentes, sans jeton de saut')
    finally:
        os.environ.pop(mhgp12_gate.DATA_ENV, None)
        if saved is not None:
            os.environ[mhgp12_gate.DATA_ENV] = saved

    # meme comportement sous -O : la porte elle-meme ne depend pas de __debug__
    gate.check_eq(mhgp12_gate.OK + mhgp12_gate.DISAGREEMENT + mhgp12_gate.REFUSAL + mhgp12_gate.FLOOR
                  + mhgp12_gate.MUTANT_KILLED, 10, 'codes 0 a 4')
    return gate.finish(FLOOR)


if __name__ == '__main__':
    sys.exit(main())
