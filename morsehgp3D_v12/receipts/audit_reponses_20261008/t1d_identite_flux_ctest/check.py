#!/usr/bin/env python3
"""Vrai selftest Python et contrat de ligne CTest ; aucun CMake/moteur."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
C = json.loads((HERE / 'capture.json').read_text())
GATE = 'morsehgp3D_v12/tests/catalogue/tests.cmake'
PILOT = 'morsehgp3D_v12/bench/g4_catalogue_t1d.py'


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def run(repo):
    def blob(path):
        return subprocess.check_output(['git', '-C', str(repo), 'show', C['pin'] + ':' + path])
    patch = blob(C['prerequisite']['path'])
    need(hashlib.sha256(patch).hexdigest() == C['prerequisite']['sha256'], 'patch identite')
    with tempfile.TemporaryDirectory(prefix='audit-t1d-ctest-') as tmp:
        root = Path(tmp)
        tree = root / 'tree'
        for path, digest in C['sources'].items():
            data = blob(path)
            need(hashlib.sha256(data).hexdigest() == digest, path)
            target = tree / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)

        def apply(path):
            for option in ('--check', None):
                subprocess.run(['git', 'apply', *([option] if option else []), str(path)], cwd=tree, check=True)

        def expected():
            rows = re.findall(r'LINE "(juge_g4_t1d_ok injections=\d+)"', (tree / GATE).read_text())
            need(len(rows) == 1, 'attente CTest unique')
            return rows[0]

        def selftest(count):
            outputs = []
            for flags in ([], ['-O']):
                p = subprocess.run([sys.executable, '-B', '-S', *flags, str(tree / PILOT), '--selftest-judge'],
                                   capture_output=True, text=True, timeout=45)
                need(p.returncode == 0 and not p.stderr, 'selftest code/stderr')
                need(p.stdout == 'juge_g4_t1d_ok injections=%d\n' % count, 'selftest ligne')
                outputs.append({'flags': flags, 'code': p.returncode, 'stdout': p.stdout, 'stderr': p.stderr})
            return outputs

        old_line = expected()
        need(old_line == 'juge_g4_t1d_ok injections=36', 'socle36')
        before = selftest(36)
        prefile = root / 'identite.patch'
        prefile.write_bytes(patch)
        apply(prefile)
        need(expected() == old_line, 'proposition initiale touche CMake')
        after = selftest(46)
        rejected = all(old_line not in p['stdout'].splitlines() for p in after)
        need(rejected, 'defaut non reproduit')
        gate_before = (tree / GATE).read_bytes()
        apply(HERE / 'proposition.patch')
        new_line = expected()
        need(new_line == 'juge_g4_t1d_ok injections=46', 'nouvelle attente')
        matched = all(new_line in p['stdout'].splitlines() for p in after)
        need(matched, 'ligne corrigee absente')
        for path, digest in C['post_python'].items():
            need(hashlib.sha256((tree / path).read_bytes()).hexdigest() == digest, 'postimage ' + path)
        result = {'pin': C['pin'], 'before': before, 'after_identity_proposal': after,
                  'line_before': old_line, 'line_after': new_line,
                  'old_expected_line_absent': rejected, 'new_expected_line_present': matched,
                  'gate_pre_sha256': hashlib.sha256(gate_before).hexdigest(),
                  'gate_post_sha256': hashlib.sha256((tree / GATE).read_bytes()).hexdigest(),
                  'native_or_ctest_executed': False}
    frozen = HERE / 'results.json'
    if frozen.exists():
        need(json.loads(frozen.read_text()) == result, 'resultats differents')
    return result


if __name__ == '__main__':
    print(json.dumps(run(Path(sys.argv[1] if len(sys.argv) > 1 else '/workspaces/E-HGP')),
                     ensure_ascii=False, sort_keys=True, indent=2))
