"""Controles du pilote G4 : signal tardif et budget des sondes de sanitizer.

Le vrai main/juge tourne ; seuls l'ordonnanceur et les appels externes sont remplaces. Aucun compilateur,
CTest ou appel cloud dans cette porte. Les delais sont verifies sur une horloge simulee, sans attente.
"""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import signal
import sys
import tempfile
import threading
from types import SimpleNamespace
from unittest import mock

import mhgp11_gate

FLOOR = 29


class FakeSteps:
    """Programme des issues et durees ; tout lancement de plus que prevu est un echec du test."""

    def __init__(self, deadline, outcomes):
        self.deadline, self.now = deadline, 0
        self.outcomes, self.calls = list(outcomes), []
        self.abort = threading.Event()

    def remaining(self):
        return self.deadline - self.now

    def run(self, name, argv, cwd, env, log, timeout):
        self.calls.append((name, timeout))
        if not self.outcomes:
            raise RuntimeError('lancement non prevu : ' + name)
        status, duration, aborted = self.outcomes.pop(0)
        self.now += duration
        if aborted:
            self.abort.set()
        return {'status': status}


def late_signal(gate, matrix, root, signum):
    source = root / ('source_%d' % signum)
    (source / 'morsehgp3D_v11').mkdir(parents=True)
    (source / 'morsehgp3D_v11' / 'CMakeLists.txt').write_text('# source factice, jamais configuree\n')
    plan = source / 'matrix.json'
    plan.write_text(json.dumps({'schema': matrix.MATRIX_SCHEMA, 'source_dir': 'morsehgp3D_v11',
                               'budget_seconds': 30, 'configurations': [{'name': 'finished'}]}))
    handlers = {}

    def schedule(configurations, budget, context, on_result):
        result = {'name': 'finished', 'status': 'ok', 'conforming': True, 'tests': {'selected': 1, 'passed': 1}}
        on_result(result)
        if signum:
            handlers[signum](signum, None)  # toutes les configurations sont deja terminees
        return [result]

    out = root / ('out_%d' % signum)
    argv = ['--src', str(source), '--out', str(out), '--work', str(root / ('work_%d' % signum)),
            '--matrix', str(plan)]
    with mock.patch.object(matrix, 'schedule', schedule), \
            mock.patch.object(matrix, 'ctest_version', return_value=(3, 22)), \
            mock.patch.object(matrix, 'first_line', return_value='outil factice'), \
            mock.patch.object(matrix.signal, 'signal', side_effect=lambda sig, handler: handlers.update({sig: handler})), \
            contextlib.redirect_stdout(io.StringIO()):
        code = matrix.main(argv)
    summary = json.loads((out / 'matrix' / 'summary.json').read_text())
    expected = 1 if signum else 0
    gate.check_eq(code, expected, 'code du vrai main apres signal %d' % signum)
    gate.check_eq(summary['exit_code'], expected, 'code du resume coherent avec main')
    gate.check_eq(summary['conforming'], not bool(signum), 'signal interdit le resume conforme')
    gate.check(summary['complete'], 'resume final ecrit meme apres interruption')
    gate.check_eq(summary['signals'], [signum] if signum else [], 'signal conserve')
    gate.check_eq(summary['statuses'], {'finished': 'ok'}, 'travail termine conserve sans reinterpretation')


def sanitizer_budget(gate, matrix, work):
    config = {'compiler': 'compilateur_jamais_lance', 'sanitizer': 'address,undefined'}
    # La premiere borne vient de la matrice, la seconde de la configuration ; la compilation paie aussi le budget.
    for overall, deadline, outcomes, expected in [
            (9, 20, [('ok', 4, False), ('ok', 3, False), ('timeout', 2, False)],
             [('sanitizer_compile', 9), ('native', 5), ('native', 2)]),
            (100, 6, [('ok', 4, False), ('timeout', 2, False)],
             [('sanitizer_compile', 6), ('native', 2)])]:
        steps = FakeSteps(overall, outcomes)
        with mock.patch.object(matrix.time, 'monotonic', side_effect=lambda: steps.now):
            wrapper, reason = matrix.sanitizer_wrapper(config, SimpleNamespace(steps=steps), work, {},
                                                       work / 'probe.log', deadline)
        gate.check_eq(wrapper, None, 'sanitizer non qualifie apres budget epuise')
        gate.check(bool(reason), 'raison conservee')
        gate.check_eq(steps.calls, expected, 'chaque essai borne, aucun repli hors delai')
    steps = FakeSteps(100, [('ok', 1, False), ('failed', 1, True)])
    with mock.patch.object(matrix.time, 'monotonic', side_effect=lambda: steps.now):
        wrapper, _ = matrix.sanitizer_wrapper(config, SimpleNamespace(steps=steps), work, {}, work / 'probe.log', 100)
    gate.check_eq(wrapper, None, 'signal pendant un essai : non qualifie')
    gate.check_eq([name for name, _ in steps.calls], ['sanitizer_compile', 'native'], 'aucun repli apres signal')
    steps = FakeSteps(100, [('ok', 1, False)] * 4)
    with mock.patch.object(matrix.time, 'monotonic', side_effect=lambda: steps.now):
        wrapper, reason = matrix.sanitizer_wrapper(config, SimpleNamespace(steps=steps), work, {},
                                                   work / 'probe.log', 100)
    gate.check_eq((wrapper, reason), ((), ''), 'temoin positif : trois essais natifs conformes')
    gate.check_eq(steps.calls, [('sanitizer_compile', 100), ('native', 30), ('native', 30), ('native', 30)],
                  'plafonds conserves quand le temps suffit')


def main():
    if len(sys.argv) != 2:
        print('usage : test_g4_matrix.py <g4_matrix.py>')
        return 2
    spec = importlib.util.spec_from_file_location('matrix_under_test', sys.argv[1])
    matrix = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(matrix)
    gate = mhgp11_gate.Gate('g4_matrix')
    with tempfile.TemporaryDirectory() as folder:
        root = Path(folder)
        for signum in (0, signal.SIGTERM, signal.SIGINT):
            late_signal(gate, matrix, root, signum)
        sanitizer_budget(gate, matrix, root)
        config = {'name': 'no_probe', 'probes': [{'name': 'absent', 'executable': 'mhgp11_probe_absent'}]}
        context = SimpleNamespace(work=root, out=root, environment=lambda config, threads: {})
        with contextlib.redirect_stdout(io.StringIO()):
            probes = matrix.run_probes(config, {}, context, 1)
        gate.check_eq((probes[0]['status'], probes[0]['isolation']), ('absent', 'not_certified'),
                      'aucune certification implicite de l isolation des sondes')
    return gate.finish(FLOOR)


if __name__ == '__main__':
    sys.exit(main())
