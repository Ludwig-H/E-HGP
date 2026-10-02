#!/usr/bin/env python3
"""Matrice des configurations de morsehgp3D_v11, executee SUR LA VM G4 (Python 3.10 nu, aucun module tiers).

Une commande du plan de session (gcp-migration/v11_session.py, default_build = false) la lance :

  python3 {src}/morsehgp3D_v11/tools/g4_matrix.py --src {src} --out {out} --data {data} \
      --work {build}/matrix [--only cfg,...] [--threads N] [--budget-seconds S] [--dry-run]

Pour chaque configuration de g4_matrix.json : dossier de construction PROPRE, `cmake` (configuration),
`cmake --build` (-j, en continuant apres une erreur pour que les portes des autres modules tournent quand
meme), liste des portes selectionnees (`ctest --show-only=json-v1`), `ctest` (JUnit si CTest >= 3.21),
puis analyse. Les configurations independantes tournent en parallele dans la limite d'un budget de fils ;
chacune a son delai et l'ensemble a une echeance (--budget-seconds) : ce qui n'a pas pu tourner est dit.
Les sondes de mesure (« probes ») tournent a la fin, une par une, quand plus rien d'autre ne tourne.

Sorties : {out}/matrix/<configuration>/ (journaux BORNES : configure.log, build.log, ctest.log,
LastTest.log, junit.xml ; tests.json ; result.json) et {out}/matrix/summary.json, reecrit apres chaque
configuration : un resume partiel existe meme si la commande est coupee. Les dossiers de construction et
les journaux complets restent sous --work, sur la VM.

Vert par vacuite refuse : zero porte selectionnee ou executee est un echec ; toute porte selectionnee mais
non executee (sans resultat, « Not Run », desactivee, sautee) est listee et rend la configuration non
conforme ; les planchers de g4_matrix.json (min_tests, require_labels) sont verifies.

Codes de sortie : 0 toutes les configurations demandees sont conformes (une configuration facultative dont
l'outil manque est relevee « absent » : ce n'est pas un echec) ; 1 au moins un echec (configuration,
construction, porte, delai, exigence manquante, interruption) ; 2 refus avant tout calcul (arguments,
matrice) ; 3 aucun echec, mais un plancher viole (vacuite, porte non executee, rien n'a tourne).

Les processus lances restent dans le groupe de processus de la commande (aucun setsid) : la fermeture de
groupe du worker de session les couvre tous, meme si ce script est tue. Une etape qui depasse son delai
est tuee avec toute sa descendance (gel par SIGSTOP jusqu'a stabilite, puis SIGKILL).
"""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import sys
import threading
import time
import xml.etree.ElementTree as ElementTree

MATRIX_SCHEMA = 'ehgp.v11.g4_matrix.v1'
SUMMARY_SCHEMA = 'ehgp.v11.g4_matrix_summary.v1'
NAME_RE = re.compile(r'[a-z0-9][a-z0-9_]{0,31}')
LABEL_RE = re.compile(r'[A-Za-z0-9_]{1,32}')
RESULT_RE = re.compile(r'^\s*\d+/\d+ Test\s+#\d+: (\S+) \.*\s*(Passed|\*\*\*\S.*?)\s+(\d+(?:\.\d+)?) sec\s*$')
TOTAL_RE = re.compile(r'^(\d+)% tests passed, (\d+) tests failed out of (\d+)\s*$', re.M)
# Options de ctest que le script fixe lui-meme, ou qui changent sa nature (tableau de bord, script, repetition,
# listage) : une matrice ne peut ni les redefinir ni les affaiblir. Comparaison par prefixe.
CTEST_RESERVED = ('--no-tests', '--show-only', '--test-dir', '--parallel', '--timeout', '--output-junit',
                  '--quiet', '--repeat', '--script', '--dashboard', '--build-', '--test-action', '--test-model',
                  '--extra-submit', '--submit', '-N', '-j', '-Q', '-S', '-D', '-T', '-M')
MIN_CONFIGURATION_SECONDS = 20      # sous ce reste de budget, une configuration n'est pas lancee
SANITIZER_SOURCE = 'int main() { return 0; }\n'
EXIT_OK, EXIT_FAILURE, EXIT_REFUSAL, EXIT_FLOOR = 0, 1, 2, 3
FAILURE_STATUSES = ('configure_failed', 'build_failed', 'list_failed', 'failed', 'timeout', 'requirement_missing',
                    'not_run_deadline', 'interrupted', 'internal_error')
FLOOR_STATUSES = ('vacuous', 'incomplete', 'floor_violated')
LIMITS = {'log_bytes': 1 << 20, 'excerpt_lines': 40, 'excerpt_line_chars': 400, 'failures_listed': 100}
CONFIGURATION_DEFAULTS = {
    'description': '', 'compiler': 'g++', 'optional': False, 'build': True, 'sanitizer': '', 'cmake_options': [],
    'ctest_args': [], 'threads': 4, 'ctest_parallel': 0, 'timeout_seconds': 900, 'test_timeout_seconds': 300,
    'min_tests': 1, 'require_labels': [], 'require_labels_if_data': [], 'env': {}, 'probes': []}
THREADS_TOKEN = '{threads}'         # dans cmake_options et env : remplace par le nombre de fils alloues
PROBE_DEFAULTS = {'args': [], 'timeout_seconds': 120}


class Refusal(Exception):
    """Refus avant tout calcul ; le message dit pourquoi."""


def need(ok, reason):
    if not ok:
        raise Refusal(reason)


def utc_now():
    return time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())


def say(message):
    sys.stdout.write('[g4_matrix %s] %s\n' % (time.strftime('%H:%M:%S', time.gmtime()), message))
    sys.stdout.flush()


# ---------------------------------------------------------------------------------------------
# Matrice (g4_matrix.json) : lecture stricte, aucune cle inconnue, valeurs par defaut explicites.

def strict_json(text):
    def pairs(items):
        result = {}
        for key, value in items:
            need(key not in result, 'cle JSON dupliquee : ' + key)
            result[key] = value
        return result
    try:
        return json.loads(text, object_pairs_hook=pairs)
    except ValueError as error:
        raise Refusal('JSON invalide : %s' % error) from error


def strings(value, pattern=None):
    return (type(value) is list and all(type(item) is str and item and '\0' not in item for item in value) and
            (pattern is None or all(pattern.fullmatch(item) for item in value)))


def positive(value, maximum):
    return type(value) is int and 1 <= value <= maximum


def validate_probe(where, value):
    need(type(value) is dict and set(value) <= {'name', 'executable', 'args', 'timeout_seconds'} and
         {'name', 'executable'} <= set(value), where + 'sonde : cles name, executable [, args, timeout_seconds]')
    probe = dict(PROBE_DEFAULTS, **value)
    need(type(probe['name']) is str and NAME_RE.fullmatch(probe['name']), where + 'sonde : nom [a-z0-9_]')
    need(type(probe['executable']) is str and re.fullmatch(r'mhgp11(_[A-Za-z0-9_]+)?', probe['executable']),
         where + 'sonde : executable de la forme mhgp11 ou mhgp11_*')
    need(strings(probe['args']) and positive(probe['timeout_seconds'], 3600), where + 'sonde : args ou delai')
    return probe


def validate_configuration(value):
    need(type(value) is dict and type(value.get('name')) is str and NAME_RE.fullmatch(value['name']),
         'configuration : objet avec un nom [a-z0-9_]{1,32}')
    where = 'configuration %s : ' % value['name']
    unknown = set(value) - set(CONFIGURATION_DEFAULTS) - {'name'}
    need(not unknown, where + 'cles inconnues : ' + ', '.join(sorted(unknown)))
    config = dict(CONFIGURATION_DEFAULTS, **value)
    need(type(config['description']) is str and len(config['description']) <= 400, where + 'description')
    need(type(config['compiler']) is str and re.fullmatch(r'[A-Za-z0-9_+.-]{1,32}', config['compiler']),
         where + 'compiler : nom d\'outil cherche dans le PATH')
    need(type(config['optional']) is bool and type(config['build']) is bool, where + 'optional et build : booleens')
    need(config['sanitizer'] in ('', 'address,undefined', 'thread'),
         where + 'sanitizer : "", "address,undefined" ou "thread"')
    need(strings(config['cmake_options']) and all(item.startswith('-D') for item in config['cmake_options']),
         where + 'cmake_options : liste de -DNOM=VALEUR')
    need(strings(config['ctest_args']), where + 'ctest_args : liste de chaines')
    for arg in config['ctest_args']:
        need(not any(arg.startswith(option) for option in CTEST_RESERVED),
             where + 'option ctest reservee au script : ' + arg)
    need(positive(config['threads'], 1024) and type(config['ctest_parallel']) is int and
         0 <= config['ctest_parallel'] <= 1024, where + 'threads >= 1 ; ctest_parallel >= 0 (0 : autant que threads)')
    need(positive(config['timeout_seconds'], 28800) and positive(config['test_timeout_seconds'], 28800),
         where + 'delais entiers entre 1 et 28800 s')
    need(positive(config['min_tests'], 100000), where + 'min_tests >= 1 (zero porte est toujours un echec)')
    need(strings(config['require_labels'], LABEL_RE) and strings(config['require_labels_if_data'], LABEL_RE),
         where + 'require_labels : liste de labels')
    need(type(config['env']) is dict and all(type(k) is str and re.fullmatch(r'[A-Z][A-Z0-9_]{0,63}', k) and
                                             type(v) is str and '\0' not in v for k, v in config['env'].items()),
         where + 'env : objet NOM -> chaine')
    need(type(config['probes']) is list, where + 'probes : liste')
    config['probes'] = [validate_probe(where, probe) for probe in config['probes']]
    need(len({probe['name'] for probe in config['probes']}) == len(config['probes']), where + 'sondes en double')
    return config


def load_matrix(path):
    try:
        text = Path(path).read_text()
    except OSError as error:
        raise Refusal('matrice illisible : %s' % error) from error
    value = strict_json(text)
    allowed = {'schema', 'note', 'source_dir', 'budget_seconds', 'thread_budget', 'limits', 'configurations'}
    need(type(value) is dict and set(value) <= allowed, 'matrice : cles permises ' + ', '.join(sorted(allowed)))
    need(value.get('schema') == MATRIX_SCHEMA, 'schema de matrice attendu : ' + MATRIX_SCHEMA)
    need(type(value.get('source_dir')) is str and re.fullmatch(r'[A-Za-z0-9_]{1,64}', value['source_dir']),
         'source_dir : nom du dossier des sources sous --src')
    need(positive(value.get('budget_seconds'), 28800), 'budget_seconds entier entre 1 et 28800')
    budget = value.get('thread_budget', 0)
    need(type(budget) is int and 0 <= budget <= 1024, 'thread_budget : entier >= 0 (0 = fils de la machine)')
    limits = dict(LIMITS)
    given = value.get('limits', {})
    need(type(given) is dict and set(given) <= set(LIMITS) and all(positive(v, 1 << 30) for v in given.values()),
         'limits : ' + ', '.join(sorted(LIMITS)))
    limits.update(given)
    need(limits['excerpt_lines'] <= 40, 'limits.excerpt_lines : 40 lignes au plus par echec')
    configurations = value.get('configurations')
    need(type(configurations) is list and configurations, 'configurations : liste non vide')
    configurations = [validate_configuration(item) for item in configurations]
    names = [config['name'] for config in configurations]
    need(len(set(names)) == len(names), 'noms de configuration en double')
    return {'source_dir': value['source_dir'], 'budget_seconds': value['budget_seconds'], 'thread_budget': budget,
            'limits': limits, 'configurations': configurations}


# ---------------------------------------------------------------------------------------------
# Processus : les enfants restent dans le groupe de la commande ; une etape en depassement est tuee
# avec toute sa descendance.

def process_parents():
    """{pid: ppid} des processus lisibles dans /proc."""
    table = {}
    for name in os.listdir('/proc'):
        if not name.isdigit():
            continue
        try:
            with open('/proc/%s/stat' % name) as stream:
                text = stream.read()
            table[int(name)] = int(text[text.rindex(')') + 2:].split()[1])
        except (OSError, ValueError, IndexError):
            continue
    return table


def process_tree(root):
    children = {}
    for pid, parent in process_parents().items():
        children.setdefault(parent, []).append(pid)
    found, pending = {root}, [root]
    while pending:
        for child in children.get(pending.pop(), []):
            if child not in found:
                found.add(child)
                pending.append(child)
    return found


def kill_tree(root):
    """Tue `root` et toute sa descendance. Gel d'abord (SIGSTOP), repete jusqu'a ce qu'aucun processus nouveau
    n'apparaisse : un processus gele ne cree plus d'enfant et n'en abandonne plus a init ; puis SIGKILL de
    l'ensemble (SIGKILL atteint aussi un processus gele)."""
    frozen = set()
    for _ in range(64):
        fresh = process_tree(root) - frozen
        if not fresh:
            break
        for pid in fresh:
            try:
                os.kill(pid, signal.SIGSTOP)
            except OSError:
                pass
        frozen |= fresh
    for pid in frozen:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass


class Steps:
    """Lance les etapes ; l'evenement `abort` (signal recu) les fait toutes tuer dans la demi-seconde."""

    def __init__(self, deadline):
        self.deadline, self.abort = deadline, threading.Event()

    def remaining(self):
        return self.deadline - time.monotonic()

    def run(self, name, argv, cwd, env, log, timeout):
        argv = [str(item) for item in argv]
        row = {'name': name, 'argv': argv, 'timeout_seconds': round(max(0.0, timeout), 1), 'exit_code': None,
               'seconds': 0.0}
        if self.abort.is_set():
            return dict(row, status='interrupted')
        if timeout < 1:
            return dict(row, status='timeout', timed_out=True)
        started = time.monotonic()
        with open(log, 'wb') as stream:
            try:
                process = subprocess.Popen(argv, cwd=str(cwd), env=env, stdin=subprocess.DEVNULL, stdout=stream,
                                           stderr=subprocess.STDOUT)
            except OSError as error:
                return dict(row, status='not_started', error=str(error))
            while True:
                try:
                    process.wait(timeout=0.5)
                    break
                except subprocess.TimeoutExpired:
                    if self.abort.is_set() or time.monotonic() - started >= timeout:
                        row['timed_out'] = not self.abort.is_set()
                        kill_tree(process.pid)
                        process.wait()
                        break
        row.update(exit_code=process.returncode, seconds=round(time.monotonic() - started, 3))
        if row.get('timed_out'):
            row['status'] = 'timeout'
        elif self.abort.is_set() and process.returncode != 0:
            row['status'] = 'interrupted'
        else:
            row['status'] = 'ok' if process.returncode == 0 else 'failed'
        return row


# ---------------------------------------------------------------------------------------------
# Journaux bornes et extraits.

def bounded_copy(source, destination, cap):
    """Copie au plus `cap` octets de `source` (tete 1/4, marqueur, queue 3/4) ; rend la taille d'origine."""
    try:
        size = os.path.getsize(source)
    except OSError:
        return None
    with open(source, 'rb') as stream, open(destination, 'wb') as sink:
        if size <= cap:
            shutil.copyfileobj(stream, sink)
        else:
            head = cap // 4
            sink.write(stream.read(head))
            sink.write(('\n[... g4_matrix : %d octets omis ; journal complet sous --work, sur la VM ...]\n' %
                        (size - cap)).encode())
            stream.seek(size - (cap - head))
            sink.write(stream.read())
    return size


def read_text(path, cap=1 << 26):
    try:
        with open(path, 'rb') as stream:
            return stream.read(cap).decode(errors='replace')
    except OSError:
        return ''


def excerpt(text, limits):
    """Fin de la sortie d'une porte : au plus excerpt_lines lignes non vides, chacune bornee."""
    lines = [line[:limits['excerpt_line_chars']] for line in text.splitlines() if line.strip()]
    return lines[-limits['excerpt_lines']:]


def build_excerpt(text, limits):
    """Extrait d'un journal de configuration ou de construction, au plus excerpt_lines lignes : d'abord les
    premieres lignes d'erreur, puis un separateur, puis la fin du journal."""
    lines = [line for line in text.splitlines() if line.strip()]
    half = limits['excerpt_lines'] // 2
    pattern = re.compile(r'\b(error|Error|ERROR)\b|\*\*\*|undefined reference')
    errors = [line for line in lines if pattern.search(line)][:max(0, half - 1)]
    return [line[:limits['excerpt_line_chars']] for line in errors + ['[...]'] + lines[-half:]][
        -limits['excerpt_lines']:]


# ---------------------------------------------------------------------------------------------
# Lecture des resultats de ctest.

def parse_test_list(text):
    """Portes selectionnees, d'apres `ctest --show-only=json-v1` : nom, labels, desactivee."""
    value = json.loads(text)
    tests = []
    for item in value.get('tests', []):
        labels, disabled = [], False
        for entry in item.get('properties', []):
            if entry.get('name') == 'LABELS':
                raw = entry.get('value') or []
                labels = sorted(raw if isinstance(raw, list) else str(raw).split(';'))
            elif entry.get('name') == 'DISABLED':
                disabled = bool(entry.get('value'))
        tests.append({'name': str(item['name']), 'labels': labels, 'disabled': disabled})
    return tests


def parse_ctest_output(text, names):
    """{nom: (etat, secondes, detail)} d'apres les lignes de resultat de ctest ; les noms inconnus sont
    ignores (une porte peut imprimer la sortie d'un ctest interne) et un nom vu avec deux etats differents
    est « ambiguous »."""
    results = {}
    for line in text.splitlines():
        match = RESULT_RE.match(line)
        if not match or match.group(1) not in names:
            continue
        name, word, seconds = match.group(1), match.group(2), float(match.group(3))
        if word == 'Passed':
            state = 'passed'
        elif word.startswith('***Timeout'):
            state = 'timeout'
        elif word.startswith('***Exception'):
            state = 'exception'
        elif word.startswith('***Not Run'):
            state = 'disabled' if 'Disabled' in word else 'not_run'
        elif word.startswith('***Skipped'):
            state = 'skipped'
        else:
            state = 'failed'
        if name in results and results[name][0] != state:
            state = 'ambiguous'
        results[name] = (state, seconds, word.lstrip('*'))
    return results


def parse_junit(path):
    """{nom: {'state', 'seconds', 'message', 'output'}} d'apres le rapport JUnit de ctest ; None s'il manque
    ou s'il est illisible (ctest tue, CTest < 3.21, ou Python sans le module pyexpat : l'analyse retombe
    alors sur la sortie de ctest et sur LastTest.log)."""
    try:
        root = ElementTree.parse(str(path)).getroot()
    except Exception:   # noqa: B902 -- OSError, ParseError, ImportError (pyexpat absent) : meme repli
        return None
    cases = {}
    for case in root.iter('testcase'):
        failure, skipped, output = case.find('failure'), case.find('skipped'), case.find('system-out')
        status = case.get('status', '')
        if status == 'run' and failure is None and skipped is None:
            state = 'passed'
        elif status == 'fail' or failure is not None:
            state = 'failed'
        elif status == 'disabled':
            state = 'disabled'
        else:
            state = 'not_run'
        note = failure if failure is not None else skipped
        try:
            seconds = float(case.get('time') or 0)
        except ValueError:
            seconds = 0.0
        cases[str(case.get('name'))] = {'state': state, 'seconds': seconds,
                                        'message': note.get('message', '') if note is not None else '',
                                        'output': (output.text or '') if output is not None else ''}
    return cases


def last_test_outputs(text):
    """{nom: sortie} d'apres LastTest.log (repli quand le rapport JUnit manque)."""
    outputs = {}
    pattern = re.compile(r'^\d+/\d+ Test: (\S+)\n.*?^Output:\n-{10,}\n(.*?)^<end of output>$', re.M | re.S)
    for match in pattern.finditer(text):
        outputs[match.group(1)] = match.group(2)
    return outputs


def judge_tests(selected, ctest_step, stdout, junit, last_log, limits, config, data_given):
    """Verdict d'une configuration d'apres les portes selectionnees et ce que ctest en dit."""
    names = {test['name'] for test in selected}
    seen = parse_ctest_output(stdout, names)
    outputs = last_test_outputs(last_log) if junit is None else {}
    tests, failures, not_run, passed_labels = [], [], [], {}
    for test in selected:
        name = test['name']
        state, seconds, detail = seen.get(name, (None, 0.0, ''))
        report = junit.get(name) if junit is not None else None
        if state is None and report is not None:
            state, seconds, detail = report['state'], report['seconds'], report['message']
        elif state == 'passed' and report is not None and report['state'] != 'passed':
            state, detail = 'ambiguous', 'sortie de ctest et rapport JUnit en desaccord'
        if state is None:
            state, detail = 'no_result', 'aucun resultat (ctest coupe, ou porte jamais lancee)'
        tests.append({'name': name, 'state': state, 'seconds': seconds, 'labels': test['labels']})
        if state == 'passed':
            for label in test['labels']:
                passed_labels[label] = passed_labels.get(label, 0) + 1
            continue
        output = report['output'] if report is not None else outputs.get(name, '')
        entry = {'test': name, 'state': state, 'detail': detail, 'seconds': seconds, 'labels': test['labels'],
                 'excerpt': excerpt(output, limits)}
        if state in ('failed', 'timeout', 'exception', 'ambiguous'):
            failures.append(entry)
        else:
            not_run.append(entry)
    total = TOTAL_RE.search(stdout)
    counts = {'selected': len(selected), 'passed': sum(1 for test in tests if test['state'] == 'passed'),
              'failed': len(failures), 'not_run': len(not_run),
              'ctest_total': int(total.group(3)) if total else None,
              'ctest_failed': int(total.group(2)) if total else None}
    labels = list(config['require_labels']) + (list(config['require_labels_if_data']) if data_given else [])
    missing_labels = sorted(label for label in set(labels) if passed_labels.get(label, 0) == 0)
    if ctest_step['status'] in ('timeout', 'interrupted', 'not_started'):
        status, reason = ctest_step['status'], 'ctest : ' + ctest_step['status']
        status = 'failed' if status == 'not_started' else status
    elif not selected or counts['passed'] + counts['failed'] == 0:
        status, reason = 'vacuous', 'aucune porte selectionnee ou executee'
    elif failures:
        status, reason = 'failed', '%d porte(s) en echec' % len(failures)
    elif ctest_step['exit_code'] != 0:
        status, reason = 'failed', 'ctest a rendu le code %s (%d porte(s) non executee(s))' % (
            ctest_step['exit_code'], len(not_run))
    elif not_run:
        status, reason = 'incomplete', '%d porte(s) selectionnee(s) non executee(s)' % len(not_run)
    elif counts['ctest_total'] != len(selected) or counts['ctest_failed'] != 0:
        status, reason = 'incomplete', 'ligne de total de ctest absente ou differente des portes selectionnees'
    elif len(selected) < config['min_tests']:
        status, reason = 'floor_violated', '%d porte(s), plancher %d' % (len(selected), config['min_tests'])
    elif missing_labels:
        status, reason = 'floor_violated', 'aucune porte passee pour le(s) label(s) : ' + ', '.join(missing_labels)
    else:
        status, reason = 'ok', ''
    listed = limits['failures_listed']
    return {'status': status, 'reason': reason, 'tests': counts, 'passed_labels': passed_labels,
            'failures': failures[:listed], 'not_run': not_run[:listed],
            'slowest': sorted(({'test': t['name'], 'seconds': t['seconds']} for t in tests),
                              key=lambda item: -item['seconds'])[:10]}


# ---------------------------------------------------------------------------------------------
# Une configuration.

def first_line(argv):
    try:
        result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                timeout=60)
    except (OSError, subprocess.SubprocessError):
        return 'absent'
    lines = result.stdout.decode(errors='replace').strip().splitlines()
    return lines[0] if result.returncode == 0 and lines else 'absent'


def ctest_version():
    match = re.search(r'(\d+)\.(\d+)', first_line(['ctest', '--version']))
    return (int(match.group(1)), int(match.group(2))) if match else (0, 0)


class Context:
    def __init__(self, args, matrix, steps):
        self.src = Path(args.src).absolute() / matrix['source_dir']
        self.out = Path(args.out).absolute() / 'matrix'
        self.work = Path(args.work).absolute()
        self.data = Path(args.data).absolute() if args.data else None
        self.limits, self.steps = matrix['limits'], steps
        # Rapport JUnit : CTest >= 3.21. L'essai a sec ne lance rien, pas meme `ctest --version`.
        self.junit = True if args.dry_run else ctest_version() >= (3, 21)

    def environment(self, config, threads):
        env = dict(os.environ)
        if self.data is not None:
            env['MHGP11_DATA_DIR'] = str(self.data)
        env.update({key: value.replace(THREADS_TOKEN, str(threads)) for key, value in config['env'].items()})
        return env


def commands(config, context, threads, generator='Unix Makefiles', wrapper=()):
    """Commandes d'une configuration (partagees par l'execution et par l'essai a sec)."""
    build = context.work / config['name'] / 'build'
    parallel = config['ctest_parallel'] or threads
    keep_going = ['-k', '0'] if 'Ninja' in generator else ['-k']
    junit = ['--output-junit', str(context.work / config['name'] / 'junit.xml')] if context.junit else []
    options = [option.replace(THREADS_TOKEN, str(threads)) for option in config['cmake_options']]
    return {
        'configure': ['cmake', '-S', str(context.src), '-B', str(build),
                      '-DCMAKE_CXX_COMPILER=' + config['compiler']] + options,
        'build': ['cmake', '--build', str(build), '-j', str(threads), '--'] + keep_going,
        'list': ['ctest', '--show-only=json-v1'] + config['ctest_args'],
        'test': list(wrapper) + ['ctest', '--no-tests=error', '--timeout', str(config['test_timeout_seconds']),
                                 '-j', str(parallel)] + config['ctest_args'] + junit}


def sanitizer_wrapper(config, context, work, env, log):
    """Verifie que le sanitizer de la configuration s'execute sur cette machine. Rend (prefixe, raison) :
    prefixe () ou (`setarch`, ARCH, `-R`) ; prefixe None et une raison quand la bibliotheque manque ou ne
    s'execute pas.

    Avec une forte randomisation de l'espace d'adressage (noyaux recents), un binaire ThreadSanitizer echoue
    ALEATOIREMENT au demarrage (« unexpected memory mapping ») : mesure dans le codespace, 6 reussites sur 40
    en natif, 40 sur 40 sous `setarch -R`. Une execution native reussie ne prouve donc rien. Pour `thread`,
    le prefixe setarch est pris des qu'il fonctionne ; pour les autres sanitizers, il n'est pris que si l'une
    de trois executions natives echoue."""
    source, binary = work / 'sanitizer_probe.cpp', work / 'sanitizer_probe'
    source.write_text(SANITIZER_SOURCE)
    step = context.steps.run('sanitizer_compile', [config['compiler'], '-std=c++20', '-O1',
                                                   '-fsanitize=' + config['sanitizer'], '-o', binary, source],
                             work, env, log, min(120, context.steps.remaining()))
    if step['status'] != 'ok':
        return None, 'compilation avec -fsanitize=%s impossible (%s)' % (config['sanitizer'], step['status'])
    prefix = ('setarch', os.uname().machine, '-R')

    def runs(label, argv, count):   # executions natives sans fichier core : `ulimit -c 0`
        return all(context.steps.run(label, argv, work, env, '%s.%s%d' % (log, label, index), 30)['status'] == 'ok'
                   for index in range(count))
    native = ['bash', '-c', 'ulimit -c 0; exec "$0"', binary]
    if config['sanitizer'] == 'thread':
        if runs('setarch', list(prefix) + [binary], 1):
            return prefix, ''
        if runs('native', native, 5):
            return (), ''
    else:
        if runs('native', native, 3):
            return (), ''
        if runs('setarch', list(prefix) + [binary], 1):
            return prefix, ''
    return None, 'programme compile avec -fsanitize=%s inexecutable, meme sous setarch -R' % config['sanitizer']


def keep_log(context, config, name, result):
    """Copie bornee d'un journal de --work vers {out} ; la taille d'origine est notee si elle est tronquee."""
    source = context.work / config['name'] / name
    size = bounded_copy(source, context.out / config['name'] / name, context.limits['log_bytes'])
    if size is not None and size > context.limits['log_bytes']:
        result.setdefault('truncated_logs', {})[name] = size


def run_configuration(config, context, threads):
    name = config['name']
    started = time.monotonic()
    deadline = min(started + config['timeout_seconds'], context.steps.deadline)
    work, out = context.work / name, context.out / name
    result = {'name': name, 'description': config['description'], 'status': None, 'conforming': False, 'reason': '',
              'threads': threads, 'optional': config['optional'], 'compiler': config['compiler'],
              'cmake_options': config['cmake_options'], 'ctest_args': config['ctest_args'], 'steps': [],
              'started_utc': utc_now()}

    def finish(status, reason='', **extra):
        result.update(status=status, reason=reason, conforming=status == 'ok',
                      seconds=round(time.monotonic() - started, 3), **extra)
        try:
            (out / 'result.json').write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
        except OSError:
            pass
        return result

    def left():
        return deadline - time.monotonic()

    out.mkdir(parents=True, exist_ok=True)
    if context.steps.abort.is_set():
        return finish('interrupted', 'signal recu avant le lancement')
    if left() < MIN_CONFIGURATION_SECONDS:
        return finish('not_run_deadline', 'budget de temps epuise avant le lancement')
    path = shutil.which(config['compiler'])
    if path is None:
        return finish('absent' if config['optional'] else 'requirement_missing',
                      'outil absent du PATH : ' + config['compiler'])
    result['compiler_path'], result['compiler_version'] = path, first_line([config['compiler'], '--version'])
    shutil.rmtree(work, ignore_errors=True)
    (work / 'build').mkdir(parents=True)
    env = context.environment(config, threads)
    wrapper = ()
    if config['sanitizer']:
        wrapper, reason = sanitizer_wrapper(config, context, work, env, work / 'sanitizer.log')
        keep_log(context, config, 'sanitizer.log', result)
        if wrapper is None:
            return finish('absent' if config['optional'] else 'requirement_missing', reason)
        result['wrapper'] = list(wrapper)
    plan = commands(config, context, threads)
    step = context.steps.run('configure', plan['configure'], work, env, work / 'configure.log', left())
    result['steps'].append(step)
    keep_log(context, config, 'configure.log', result)
    if step['status'] != 'ok':
        status = step['status'] if step['status'] in ('timeout', 'interrupted') else 'configure_failed'
        return finish(status, 'configuration CMake : ' + step['status'],
                      excerpt=build_excerpt(read_text(work / 'configure.log'), context.limits))
    built = True
    if config['build']:
        cache = read_text(work / 'build' / 'CMakeCache.txt')
        generator = re.search(r'^CMAKE_GENERATOR:INTERNAL=(.*)$', cache, re.M)
        plan = commands(config, context, threads, generator.group(1) if generator else '', wrapper)
        step = context.steps.run('build', plan['build'], work, env, work / 'build.log', left())
        result['steps'].append(step)
        keep_log(context, config, 'build.log', result)
        if step['status'] in ('timeout', 'interrupted'):
            return finish(step['status'], 'construction : ' + step['status'],
                          excerpt=build_excerpt(read_text(work / 'build.log'), context.limits))
        built = step['status'] == 'ok'
        if not built:
            result['build_excerpt'] = build_excerpt(read_text(work / 'build.log'), context.limits)
    else:
        plan = commands(config, context, threads, '', wrapper)
    step = context.steps.run('list', plan['list'], work / 'build', env, work / 'tests.raw.json', min(120, left()))
    result['steps'].append(step)
    try:
        selected = parse_test_list(read_text(work / 'tests.raw.json')) if step['status'] == 'ok' else None
    except (ValueError, KeyError, TypeError, AttributeError):
        selected = None
    if selected is None:
        keep_log(context, config, 'tests.raw.json', result)
        status = step['status'] if step['status'] in ('timeout', 'interrupted') else 'list_failed'
        return finish(status if built else 'build_failed', 'liste des portes illisible (ctest --show-only=json-v1)')
    (out / 'tests.json').write_text(json.dumps(selected, indent=1, sort_keys=True) + '\n')
    step = context.steps.run('test', plan['test'], work / 'build', env, work / 'ctest.log', left())
    result['steps'].append(step)
    temporary = work / 'build' / 'Testing' / 'Temporary'
    last = temporary / 'LastTest.log' if (temporary / 'LastTest.log').exists() else temporary / 'LastTest.log.tmp'
    if last.exists():
        shutil.copyfile(last, work / 'LastTest.log')
    for log in ('ctest.log', 'LastTest.log', 'junit.xml'):
        keep_log(context, config, log, result)
    verdict = judge_tests(selected, step, read_text(work / 'ctest.log'), parse_junit(work / 'junit.xml'),
                          read_text(work / 'LastTest.log'), context.limits, config, context.data is not None)
    result.update({key: verdict[key] for key in ('tests', 'passed_labels', 'failures', 'not_run', 'slowest')})
    if not built:
        return finish('build_failed', 'construction en echec ; portes : %s' % (verdict['reason'] or 'conformes'))
    return finish(verdict['status'], verdict['reason'])


def run_probes(config, result, context, threads):
    """Sondes de mesure d'une configuration construite ; jamais une condition de conformite."""
    rows = []
    work, out = context.work / config['name'], context.out / config['name']
    env = context.environment(config, threads)
    for probe in config['probes']:
        row = {'name': probe['name'], 'executable': probe['executable']}
        found = sorted(path for path in (work / 'build').rglob(probe['executable'])
                       if path.is_file() and os.access(path, os.X_OK)) if (work / 'build').is_dir() else []
        if not found:
            row['status'] = 'absent'
        elif context.steps.remaining() < MIN_CONFIGURATION_SECONDS:
            row['status'] = 'not_run_deadline'
        else:
            log = 'probe_%s.log' % probe['name']
            step = context.steps.run('probe_' + probe['name'], list(result.get('wrapper', [])) + [found[0]] +
                                     probe['args'], work / 'build', env, work / log,
                                     min(probe['timeout_seconds'], context.steps.remaining()))
            bounded_copy(work / log, out / log, context.limits['log_bytes'])
            row.update(status=step['status'], exit_code=step['exit_code'], seconds=step['seconds'], log=log)
        rows.append(row)
        say('%s : sonde %s : %s' % (config['name'], probe['name'], row['status']))
    return rows


# ---------------------------------------------------------------------------------------------
# Ordonnancement, resume, essai a sec.

def schedule(configurations, budget, context, on_result):
    """Lance les configurations dans l'ordre de la matrice, tant que la somme des fils alloues tient dans le
    budget (une configuration plus gourmande que le budget tourne seule, ramenee au budget)."""
    condition = threading.Condition()
    pending, running, results = list(configurations), {}, {}

    def worker(config, threads):
        try:
            result = run_configuration(config, context, threads)
        except Exception as error:   # noqa: B902 -- une erreur interne ne doit pas arreter les autres configurations
            result = {'name': config['name'], 'status': 'internal_error', 'conforming': False,
                      'reason': '%s: %s' % (type(error).__name__, error), 'optional': config['optional'],
                      'threads': threads}
            try:
                (context.out / config['name']).mkdir(parents=True, exist_ok=True)
                (context.out / config['name'] / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
            except OSError:
                pass
        with condition:
            results[config['name']] = result
            del running[config['name']]
            on_result(result)
            condition.notify_all()

    with condition:
        while pending or running:
            used = sum(running.values())
            for config in list(pending):
                threads = min(config['threads'], budget)
                if used + threads <= budget or not running:
                    pending.remove(config)
                    running[config['name']] = threads
                    used += threads
                    say('%s : lancement (%d fils ; en cours : %s)' % (config['name'], threads, ', '.join(running)))
                    threading.Thread(target=worker, args=(config, threads), daemon=True).start()
            condition.wait(0.5)
    return [results[config['name']] for config in configurations]


def exit_code_of(results):
    statuses = [result['status'] for result in results]
    if any(status in FAILURE_STATUSES for status in statuses):
        return EXIT_FAILURE
    if any(status in FLOOR_STATUSES for status in statuses) or 'ok' not in statuses:
        return EXIT_FLOOR
    return EXIT_OK


def write_summary(path, header, results, complete):
    value = dict(header, complete=complete, ended_utc=utc_now() if complete else None,
                 configurations=results, exit_code=exit_code_of(results) if complete else None,
                 conforming=complete and exit_code_of(results) == EXIT_OK,
                 statuses={result['name']: result['status'] for result in results})
    temporary = Path(str(path) + '.partial')
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
    os.replace(temporary, path)


def dry_run(matrix, selection, context, budget):
    say('essai a sec : %d configuration(s), budget %d fils, echeance %d s ; rien n\'est lance (--output-junit '
        'seulement si CTest >= 3.21 ; -k devient -k 0 avec Ninja)' % (
            len(selection), budget, matrix['budget_seconds']))
    for config in selection:
        threads = min(config['threads'], budget)
        present = shutil.which(config['compiler']) is not None
        print('[%s] fils=%d delai=%ds compilateur=%s%s' % (
            config['name'], threads, config['timeout_seconds'], config['compiler'],
            '' if present else (' (absent ici : configuration relevee « absent »)' if config['optional'] else
                                ' (absent ici : exigence manquante, echec)')))
        plan = commands(config, context, threads, wrapper=('[setarch ARCH -R si necessaire]',)
                        if config['sanitizer'] == 'thread' else ())
        env = {key: value.replace(THREADS_TOKEN, str(threads)) for key, value in config['env'].items()}
        if context.data is not None:
            env.setdefault('MHGP11_DATA_DIR', str(context.data))
        if env:
            print('  env       : ' + ' '.join('%s=%s' % (key, shlex.quote(value))
                                              for key, value in sorted(env.items())))
        if config['sanitizer']:
            print('  sanitizer : %s -std=c++20 -O1 -fsanitize=%s (programme d\'une ligne, compile puis execute)' % (
                config['compiler'], config['sanitizer']))
        for step in ('configure', 'build', 'list', 'test'):
            if step == 'build' and not config['build']:
                continue
            print('  %-9s : %s' % (step, ' '.join(shlex.quote(str(arg)) for arg in plan[step])))
        for probe in config['probes']:
            print('  probe     : %s %s (a la fin, seule sur la machine, si l\'executable existe)' % (
                probe['executable'], ' '.join(shlex.quote(arg) for arg in probe['args'])))
    return EXIT_OK


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--src', required=True, help='racine du paquet ({src}) : contient morsehgp3D_v11/')
    parser.add_argument('--out', required=True, help='dossier de sortie ({out}) : matrix/ y est cree')
    parser.add_argument('--data', help='dossier des donnees ({data}), exporte dans MHGP11_DATA_DIR')
    parser.add_argument('--work', help='dossier des constructions (defaut : ./g4_matrix_work)')
    parser.add_argument('--only', help='configurations a executer, separees par des virgules (defaut : toutes)')
    parser.add_argument('--matrix', help='matrice JSON (defaut : g4_matrix.json a cote de ce script)')
    parser.add_argument('--threads', type=int, help='budget de fils (defaut : matrice, sinon fils de la machine)')
    parser.add_argument('--budget-seconds', type=int, help='echeance de l\'ensemble (defaut : matrice)')
    parser.add_argument('--dry-run', action='store_true', help='afficher les commandes sans rien lancer')
    args = parser.parse_args(argv)
    args.work = args.work or str(Path.cwd() / 'g4_matrix_work')
    try:
        matrix = load_matrix(args.matrix or Path(__file__).resolve().parent / 'g4_matrix.json')
        names = [config['name'] for config in matrix['configurations']]
        wanted = names if args.only is None else [item for item in args.only.split(',')]
        need(wanted and all(item in names for item in wanted) and len(set(wanted)) == len(wanted),
             '--only : configurations connues, sans doublon : ' + ', '.join(names))
        selection = [config for config in matrix['configurations'] if config['name'] in wanted]
        budget = args.threads if args.threads is not None else matrix['thread_budget'] or os.cpu_count() or 1
        need(1 <= budget <= 1024, '--threads : entier entre 1 et 1024')
        if args.budget_seconds is not None:
            need(1 <= args.budget_seconds <= 28800, '--budget-seconds : entier entre 1 et 28800')
            matrix['budget_seconds'] = args.budget_seconds
        steps = Steps(time.monotonic() + matrix['budget_seconds'])
        context = Context(args, matrix, steps)
        need((context.src / 'CMakeLists.txt').is_file(), 'sources absentes : %s/CMakeLists.txt' % context.src)
        need(context.data is None or context.data.is_dir(), '--data : dossier absent : %s' % context.data)
        if args.dry_run:
            return dry_run(matrix, selection, context, budget)
        need(not context.out.exists() or not any(context.out.iterdir()),
             'sortie deja presente : %s (un dossier {out} neuf est exige)' % context.out)
        context.out.mkdir(parents=True, exist_ok=True)
        context.work.mkdir(parents=True, exist_ok=True)
    except Refusal as error:
        sys.stderr.write('g4_matrix : refus : %s\n' % error)
        return EXIT_REFUSAL

    received = []

    def on_signal(signum, _frame):   # aucune entree-sortie ici : le fil principal peut etre en train d'ecrire
        received.append(signum)
        steps.abort.set()
    for signum in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
        signal.signal(signum, on_signal)
    header = {'schema': SUMMARY_SCHEMA, 'started_utc': utc_now(), 'requested': wanted, 'thread_budget': budget,
              'budget_seconds': matrix['budget_seconds'], 'data_dir': str(context.data) if context.data else None,
              'host': {'nproc': os.cpu_count(), 'gxx': first_line(['g++', '--version']),
                       'clangxx': first_line(['clang++', '--version']), 'cmake': first_line(['cmake', '--version']),
                       'ctest': first_line(['ctest', '--version']), 'python': sys.version.split()[0],
                       'junit': context.junit}}
    summary, done = context.out / 'summary.json', []

    def on_result(result):
        done.append(result)
        say('%s : %s%s (%s s)' % (result['name'], result['status'],
                                  ' -- ' + result['reason'] if result.get('reason') else '', result.get('seconds')))
        write_summary(summary, header, done, complete=False)
    write_summary(summary, header, done, complete=False)
    results = schedule(selection, budget, context, on_result)
    for config, result in zip(selection, results):
        if config['probes'] and result.get('steps') and not steps.abort.is_set():
            result['probes'] = run_probes(config, result, context, budget)
            (context.out / config['name'] / 'result.json').write_text(json.dumps(result, indent=2, sort_keys=True) +
                                                                      '\n')
    if received:
        header['signals'] = received
        say('signal(aux) %s recu(s) : etapes en cours tuees' % received)
    write_summary(summary, header, results, complete=True)
    code = exit_code_of(results)
    for result in results:
        counts = result.get('tests') or {}
        print('%-16s %-20s portes %s/%s%s' % (result['name'], result['status'], counts.get('passed', '-'),
                                              counts.get('selected', '-'),
                                              '  ' + result['reason'] if result.get('reason') else ''))
        for failure in (result.get('failures') or []) + (result.get('not_run') or []):
            print('    %-12s %s' % (failure['state'], failure['test']))
    say('code de sortie %d ; resume : %s' % (code, summary))
    return code


if __name__ == '__main__':
    sys.exit(main())
