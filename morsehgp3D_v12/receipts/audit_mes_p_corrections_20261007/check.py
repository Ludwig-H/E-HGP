#!/usr/bin/env python3
"""Audit MES-P borne : JSON inventes, aucun calcul HGP ni build. Sortie JSON deterministe.

python3 -B check.py [--source-dir snapshot] [--require-common]
Le dernier drapeau exige le contrat de cohorte, sans masquer le defaut dans la capture historique.
"""
import argparse
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
import pathlib
import re
import subprocess
import sys
import tempfile
from unittest import mock


def require(value, message):
    if not value:
        raise RuntimeError(message)


def take(name, n, k, threads, rate):
    return dict(nuage=name, sites=n, k=k, fils=threads, chaud=n*rate/1e6, code=0)


def failed(rows, condition):
    result = copy.deepcopy(rows)
    for row in result:
        if condition(row):
            row.update(code='expire', chaud=None)
    return result


def render(source, rows, directory):
    path = directory / 'mes_p.json'
    path.write_text(json.dumps(dict(prises=rows)), encoding='utf-8')
    argv = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    proc = subprocess.run(argv + [str(source), str(path)], capture_output=True, text=True, timeout=10)
    require(proc.returncode == 0, proc.stderr)
    lines = proc.stdout.splitlines()
    slopes = [line for line in lines if line.startswith('Droite des moindres carres')]
    cohorts = [line for line in lines if re.match(r'^\| \d+ fils \|', line)]
    synth = [line for line in lines if line.startswith('| synth_')]
    failures = [line for line in lines if '| expire |' in line]
    return dict(slopes=slopes, cohorts=cohorts, synthetics=synth, failures=failures,
                stdout_sha256=hashlib.sha256(proc.stdout.encode()).hexdigest())


def analyser_checks(source, directory):
    base = [take('bout_n%d' % n, n, 5, f, rate)
            for f, rate in [(1, 100), (4, 60), (48, 10)] for n in (100, 200)]
    good = render(source, base, directory)
    require(len(good['slopes']) == 3, 'trois droites attendues')
    for f, rate in [(1, 100), (4, 60), (48, 10)]:
        require(any('K = 5, %d fils)' % f in line and '%.2f µs par site' % rate in line
                    for line in good['slopes']), 'pente du regime')
    require([line.replace('| -0.00 |', '| 0.00 |') for line in good['cohorts']] ==
                              ['| 1 fils | 2 | 0 | 0.00 | 100.00 |',
                               '| 4 fils | 2 | 0 | 0.00 | 60.00 |',
                               '| 48 fils | 2 | 0 | 0.00 | 10.00 |'], 'cohorte sans echec')
    partial = render(source, failed(base, lambda t: t['fils'] == 1 and t['sites'] == 200), directory)
    require(partial['cohorts'] == ['| 1 fils | 1 | 0 | - | - |', '| 4 fils | 1 | 1 | - | - |',
                                  '| 48 fils | 1 | 1 | - | - |'], 'cohorte apres un echec')
    require(len(partial['failures']) == 1 and '| 5 | 1 | 200 | expire |' in partial['failures'][0],
            'echec avec regime affiche')
    absent = render(source, failed(base, lambda t: t['fils'] == 1), directory)
    expected = ['| 1 fils | 0 | 0 | - | - |', '| 4 fils | 0 | 2 | - | - |',
                '| 48 fils | 0 | 2 | - | - |']
    absent['expected_cohorts'] = expected
    absent['common_contract_pass'] = absent['cohorts'] == expected
    all_failed = render(source, failed(base, lambda t: True), directory)
    all_failed['expected_cohorts'] = ['| %d fils | 0 | 0 | - | - |' % f for f in (1, 4, 48)]
    all_failed['common_contract_pass'] = all_failed['cohorts'] == all_failed['expected_cohorts']
    extras = [take('bout_n%d' % n, n, 10, 4, 800) for n in (100, 200)]
    extras += [take('synth_lattice_n100', 100, 5, 1, 2000),
               take('synth_lattice_n100', 100, 5, 4, 4000),
               take('synth_lattice_n100', 100, 10, 4, 8000),
               take('synth_sphere_n100', 100, 5, 4, 12000)]
    isolation = render(source, base + extras, directory)
    require(len(isolation['slopes']) == 4 and all(line in isolation['slopes'] for line in good['slopes']),
            'isolation K et familles')
    require(any('K = 10, 4 fils)' in line and '800.00 µs par site' in line
                for line in isolation['slopes']), 'pente K10')
    require(isolation['synthetics'] == ['| synth_lattice | 5 | 1 | 100 : 200.0 |',
                                       '| synth_lattice | 5 | 4 | 100 : 400.0 |',
                                       '| synth_lattice | 10 | 4 | 100 : 800.0 |',
                                       '| synth_sphere | 5 | 4 | 100 : 1200.0 |'], 'isolation synthetiques')
    return dict(all_good=good, one_failed=partial, whole_regime_failed=absent,
                all_failed=all_failed, k_threads_family_isolation=isolation)


def pilot_checks(source, directory):
    spec = importlib.util.spec_from_file_location('audit_pilot', source)
    pilot = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(pilot)
    output = b'{"phase":"cloud","sites":100}\n'
    output += b''.join(json.dumps(dict(phase='pass', wall_ns=ns, domain_ns=800000000,
                                     forest_ns=900000000)).encode() + b'\n'
                       for ns in (3000000, 1000000, 2000000))
    records = []
    for name, rc, timeout in [('success', 0, False), ('failure', 3, False), ('timeout', -9, True)]:
        proc = mock.Mock(pid=123456, returncode=rc)
        proc.communicate.side_effect = ([subprocess.TimeoutExpired(['fake'], 1), (output, b'')]
                                        if timeout else [(output, b'')])
        with mock.patch.object(pilot.subprocess, 'Popen', return_value=proc) as popen, \
             mock.patch.object(pilot.os, 'killpg') as killpg, \
             mock.patch.object(pilot.time, 'monotonic', side_effect=[1.0, 1.25]):
            record = pilot.run_cloud('/audit/fake', str(directory), str(directory), name, 5, 4, 3, 1)
            require(popen.call_args.kwargs['start_new_session'] is True, 'nouveau groupe attendu')
            require(record['froid'] == .003 and record['passes'] == [.003, .001, .002], 'wall_ns prioritaire')
            require(record['chaud'] == (.0015 if name == 'success' else None), 'chaud succes seulement')
            require(record['code'] == ('expire' if timeout else rc), 'code conserve')
            if timeout:
                killpg.assert_called_once_with(123456, pilot.signal.SIGKILL)
                require(proc.communicate.call_count == 2, 'collecte apres arret')
            else:
                killpg.assert_not_called()
            records.append(record)
    manifest = directory / 'bundle_manifest.json'
    manifest.write_text(json.dumps(dict(cases=[dict(name='synth_lattice_n100')])))
    with mock.patch.object(pilot, 'build') as build, contextlib.redirect_stderr(io.StringIO()):
        code = pilot.main(['pilot', '--v11-build', '/audit/fake', '--donnees', str(directory),
                           '--sortie', str(directory/'out'), '--exclure', 'synth_lattice'])
        require(code == 2 and not build.called, 'selection vide refusee avant build')
    return dict(mocked_processes=records, selection_empty_code=code,
                scope='Popen/killpg doubles: controle du protocole, pas preuve systeme de terminaison')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', type=pathlib.Path, default=pathlib.Path(__file__).parent/'snapshot')
    parser.add_argument('--require-common', action='store_true')
    args = parser.parse_args()
    sources = {name: args.source_dir / name for name in ('analyse_p.py', 'pilote_p.py')}
    hashes = {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in sources.items()}
    with tempfile.TemporaryDirectory(prefix='audit-mesp-json-') as tmp:
        directory = pathlib.Path(tmp)
        analyser = analyser_checks(sources['analyse_p.py'], directory)
        pilot = pilot_checks(sources['pilote_p.py'], directory)
    require(hashes == {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in sources.items()},
            'sources modifiees pendant lecture')
    complete = all(analyser[key]['common_contract_pass'] for key in ('whole_regime_failed', 'all_failed'))
    print(json.dumps(dict(source_sha256=hashes, analyser=analyser, pilot=pilot,
                         cst_0238_common_cohort_contract_pass=complete), sort_keys=True, indent=2))
    return 1 if args.require_common and not complete else 0


if __name__ == '__main__':
    sys.exit(main())
