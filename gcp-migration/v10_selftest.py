#!/usr/bin/env python3
"""Autotest hors ligne du protocole de session G4 v10 (v10_session.py + v10_worker.sh).

Aucun appel GCP reel, aucun ssh reel. Un faux gcloud a etat (dossier temporaire, en tete du PATH et
passe par --gcloud) emule la cible, OS Login, start/stop, compute ssh (commandes executees localement
dans un faux « distant » dont HOME est un dossier du scenario), compute scp et /run/nologin (tout
SSH/SCP refuse des l'arret invite moins 300 s) ; de faux ssh/scp echouent s'ils sont appeles
directement ; CLOUDSDK_CONFIG pointe vers un dossier vide. Les VRAIS start_and_verify.sh et
stop_and_verify.sh tournent contre ce faux gcloud, et la garde invitee reelle est executee avec un faux
`shutdown`. Des « kits » distants remplacent python3 (paquets epingles presents, pip absent, pip
installable), ctest (vert par vacuite), tar (bloque dans l'emballage) ou stat (archive geante). Un faux
depot Git (origine nue locale) porte le protocole v10 et un mini morsehgp3D_v10 reellement construit
par cmake et g++ locaux.

Fixtures permanentes : revue adverse 1 (T1 STAGING concurrent, console morte, T3, T4, T5, T6 + --recover,
rupture de stock, start en echec mais VM demarree) et revue adverse 2 (disque de session PLEIN avant
l'arret sur un vrai tmpfs sous `unshare -Urm`, generation perimee face a un demarrage etranger en
STAGING pour --recover et pour la fermeture ordinaire, --recover pendant un start_and_verify orphelin,
/run/nologin, resultats evinces ou tronques, decompression bornee, derive de la configuration gcloud,
OS Login desactive, cle ajoutee puis delai depasse, DONE impossible a ecrire), revue adverse 3 (RUNNING
portant notre generation apres un arret posterieur, dossier d'execution $TMPDIR PLEIN puis rempli de
nouveau, --recover face a de simples observateurs, hoquets de l'API describe, ~/.local casse, DONE
perime, dossier de session absent), branches overdue / abandon / secours / worker mort, ctest vacant ou
avec tests desactives, residu de groupe, refus.

  python3 gcp-migration/v10_selftest.py        # code 0 si tout passe (environ 20 min)
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest

HERE = Path(__file__).resolve().parent
PROTOCOL = ('start_and_verify.sh', 'stop_and_verify.sh', 'v10_session.py', 'v10_worker.sh')
TARGET = {'project': 'devpod-gpu-exploration', 'zone': 'us-central1-b',
          'instance': 'ehgp-v7-4fa0e0789a7d5bb06b787d35'}
PLAN_SCHEMA = 'ehgp.v10.session_plan.v1'
READ_ONLY = {('config', 'get-value'), ('compute', 'instances', 'describe'), ('compute', 'instances', 'list'),
             ('compute', 'os-login', 'describe-profile')}
TRICKY_ARG = "a b 'c' $(touch PWNED) `touch PWNED2` {x} \\n"
BACKSLASH_NAME = 'back\\nslash.txt'
PINNED = 'hdbscan=0.8.44,numpy=2.2.6,scikit-learn=1.7.2,scipy=1.15.3'
OLD_GENERATION = '2026-09-28T09:00:00.000000-07:00'

FAKE_GCLOUD = r'''
import datetime
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

STATE = os.environ['FAKE_GCP_STATE']
ARGS = sys.argv[1:]
with open(os.environ['FAKE_GCP_LOG'], 'a') as stream:
    stream.write(json.dumps(ARGS) + '\n')


def load():
    with open(STATE) as stream:
        return json.load(stream)


def save(state):
    with open(STATE + '.tmp', 'w') as stream:
        json.dump(state, stream)
    os.replace(STATE + '.tmp', STATE)


def done(code=0, out='', err=''):
    sys.stdout.write(out)
    sys.stderr.write(err)
    sys.exit(code)


def rfc(epoch):
    zone = datetime.timezone(datetime.timedelta(hours=-7))
    return datetime.datetime.fromtimestamp(epoch, zone).isoformat(timespec='microseconds')


def session_deadline():
    """Echeance invitee D telle que la session la lit (offset d'autotest compris) ; None si non armee."""
    try:
        with open(os.environ['FAKE_SCHEDULED']) as stream:
            text = stream.read()
    except OSError:
        return None
    return int(re.search(r'^USEC=([0-9]+)$', text, re.M).group(1)) / 1e6 - modes.get('session_schedule_offset', 0)


state = load()
modes = state['modes']
positional = [a for a in ARGS if not a.startswith('-')]
options = {}
for item in ARGS:
    if item.startswith('--') and '=' in item:
        key, value = item[2:].split('=', 1)
        options.setdefault(key, value)
inst, zone, project = state['instance'], state['zone'], state['project']
head = tuple(positional[:3])
command = options.get('command', '')

# -- evenements adverses ------------------------------------------------------------------------
if positional[:4] == ['compute', 'os-login', 'ssh-keys', 'add'] and modes.get('concurrent_staging'):
    # Une autre session lance la cible : STAGING, lastStartTimestamp pas encore materialise.
    state['status'] = 'STAGING'
    save(state)
if head == ('compute', 'os-login', 'describe-profile') and modes.get('profile_sleep'):
    time.sleep(modes['profile_sleep'])
if head == ('compute', 'instances', 'start') and modes.get('start_sleep'):
    time.sleep(modes['start_sleep'])
if head == ('compute', 'instances', 'describe') and options.get('format') == 'value(status)' and \
        state.get('preempt_countdown'):
    state['preempt_countdown'] -= 1
    if state['preempt_countdown'] == 0:
        state.update(status='TERMINATED', preempted=True, last_stop=rfc(time.time()))
    save(state)
if head == ('compute', 'instances', 'describe') and modes.get('foreign_start_after_preempt') and \
        state.get('preempted') and state['status'] == 'TERMINATED' and not state.get('foreign_staging'):
    # Demarrage d'une autre session apres la preemption : STAGING, lastStartTimestamp encore le notre.
    state.update(status='STAGING', foreign_staging=True)
    save(state)
if head == ('compute', 'instances', 'describe') and modes.get('foreign_running_after_preempt') and \
        state.get('preempted') and state['status'] == 'TERMINATED' and not state.get('foreign_running'):
    # Autre session deja RUNNING apres la preemption : lastStartTimestamp porte encore le notre
    # (generation pas encore materialisee), seul lastStopTimestamp trahit l'arret intermediaire.
    state.update(status='RUNNING', foreign_running=True)
    save(state)
if positional[:2] == ['compute', 'ssh'] and 'ALIVE' in command:
    state['polls_seen'] = state.get('polls_seen', 0) + 1
    if state['polls_seen'] == modes.get('flip_after_polls'):
        state['last_start'] = rfc(time.time() + 1)
        state['foreign_generation'] = state['last_start']
    if state['polls_seen'] == modes.get('preempt_after_polls'):
        state.update(status='TERMINATED', preempted=True, last_stop=rfc(time.time()))
    if state['polls_seen'] == modes.get('fill_session_after_polls') and not state.get('session_filled'):
        # Disque de session PLEIN : copie du contenu dans un tmpfs monte par-dessus, puis ballast
        # jusqu'a ENOSPC (exige `unshare -Urm` ; aucun privilege reel).
        _session = os.environ['FAKE_SESSION_DIR']
        _keep = tempfile.mkdtemp(dir=os.environ['FAKE_REMOTE_ROOT'])
        shutil.copytree(_session, _keep + '/s', symlinks=True)
        subprocess.run(['mount', '-t', 'tmpfs', '-o', 'size=%dk,mode=700' % modes.get('fill_kib', 16384), 'tmpfs',
                        _session], check=True)
        shutil.copytree(_keep + '/s', _session, symlinks=True, dirs_exist_ok=True)
        subprocess.run(['dd', 'if=/dev/zero', 'of=' + _session + '/ballast', 'bs=4k'], capture_output=True)
        state['session_filled'] = True
    if state['polls_seen'] == modes.get('fill_run_dir_after_polls') and not state.get('run_dir_filled'):
        # Dossier d'execution ($TMPDIR) PLEIN : meme montage tmpfs, sur le dossier d'execution de la session.
        _run = os.environ['FAKE_RUN_DIR']
        _keep = tempfile.mkdtemp(dir=os.environ['FAKE_REMOTE_ROOT'])
        shutil.copytree(_run, _keep + '/r', symlinks=True)
        subprocess.run(['mount', '-t', 'tmpfs', '-o', 'size=%dk,mode=700' % modes.get('fill_kib', 16384), 'tmpfs',
                        _run], check=True)
        shutil.copytree(_keep + '/r', _run, symlinks=True, dirs_exist_ok=True)
        subprocess.run(['dd', 'if=/dev/zero', 'of=' + _run + '/ballast', 'bs=4k'], capture_output=True)
        state['run_dir_filled'] = True
    save(state)
if state.get('session_filled') and modes.get('refill_after_reserve_release') and \
        not state.get('session_refilled') and \
        not os.path.exists(os.path.join(os.environ['FAKE_SESSION_DIR'], 'host', 'reserve.bin')):
    # La reserve vient d'etre liberee : un autre ecrivain reprend aussitot la place (disque de nouveau plein).
    subprocess.run(['dd', 'if=/dev/zero', 'of=' + os.environ['FAKE_SESSION_DIR'] + '/ballast2', 'bs=4k'],
                   capture_output=True)
    state['session_refilled'] = True
    save(state)
if state.get('run_dir_filled') and modes.get('refill_after_reserve_release') and \
        not state.get('run_dir_refilled') and \
        not os.path.exists(os.path.join(os.environ['FAKE_RUN_DIR'], 'reserve.bin')):
    # Idem pour le dossier d'execution : sa reserve liberee est aussitot reprise.
    subprocess.run(['dd', 'if=/dev/zero', 'of=' + os.environ['FAKE_RUN_DIR'] + '/ballast2', 'bs=4k'],
                   capture_output=True)
    state['run_dir_refilled'] = True
    save(state)
if state.get('foreign_generation') and state['last_start'] == state['foreign_generation'] and \
        positional[:2] in (['compute', 'ssh'], ['compute', 'scp']):
    with open(STATE + '.foreign', 'a') as stream:
        stream.write(json.dumps((command or 'scp')[:80]) + '\n')
if positional[:2] == ['compute', 'ssh'] and 'setsid nohup' in command:
    state['worker_launch_seen'] = True
    if modes.get('json_failures_after') == 'launch' and not state.get('json_failures_armed'):
        state.update(json_failures_left=modes.get('json_failures', 0), json_failures_armed=True)
    save(state)
if positional[:2] in (['compute', 'ssh'], ['compute', 'scp']) and not command.startswith('sudo -n bash -c '):
    # /run/nologin : shutdown -P le cree 5 min avant l'arret invite ; pam_nologin refuse alors tout SSH.
    _deadline = session_deadline()
    if _deadline is not None and time.time() >= _deadline - 300:
        with open(STATE + '.nologin', 'a') as stream:
            stream.write(json.dumps([round(time.time() - _deadline, 1), (command or 'scp')[:60]]) + '\n')
        done(255, err='System is going down.\nConnection closed by remote host (pam_nologin, emule)\n')


def field(name):
    running = state['status'] == 'RUNNING'
    values = {
        'status': state['status'], 'scheduling.instanceTerminationAction': 'STOP',
        'scheduling.automaticRestart': 'False', 'scheduling.maxRunDuration.seconds': str(state['max_run']),
        'labels.project': 'e-hgp', 'machineType.basename()': 'g4-standard-48',
        'scheduling.onHostMaintenance': 'TERMINATE', 'scheduling.provisioningModel': 'SPOT',
        'lastStartTimestamp': state['last_start'],
        'resourceStatus.scheduling.terminationTimestamp': state.get('termination', '') if running else '',
        'scheduling.terminationTimestamp': '', 'terminationTimestamp': ''}
    return values[name]


if positional[:2] == ['config', 'get-value']:
    active = os.environ.get('CLOUDSDK_CORE_PROJECT') or project
    if modes.get('active_project_drifts') and state.get('worker_launch_seen') and \
            not os.environ.get('CLOUDSDK_CORE_PROJECT'):
        active = 'autre-projet'   # un autre agent a change la configuration gcloud partagee
    done(0, {'project': active, 'account': 'selftest@example.invalid'}[positional[2]] + '\n')

if head == ('compute', 'instances', 'list'):
    if options.get('format', '') == 'value(name)':
        done(0, inst + '\n' if options.get('filter') == 'name=' + inst else '')
    done(0, '%s,%s,%s\n' % (inst, zone, state['status']))

if head == ('compute', 'instances', 'describe'):
    if positional[3] != inst or options.get('zone') != zone or options.get('project') != project:
        done(1, err='ERROR: instance inconnue\n')
    fmt = options.get('format', '')
    if fmt == 'json' and state.get('json_failures_left', 0) > 0:
        # Hoquet de l'API : describe --format=json illisible N fois d'affilee (les lectures value() des
        # scripts gardes ne sont pas touchees).
        state['json_failures_left'] -= 1
        save(state)
        done(1, err='ERROR: (gcloud.compute.instances.describe) 503 Service Unavailable (simule)\n')
    if fmt == 'json':
        status = state['status']
        if state.get('stopped_by_fake') and modes.get('json_status_after_stop'):
            status = modes['json_status_after_stop']
        base = 'https://www.googleapis.com/compute/v1/projects/%s/zones/%s' % (project, zone)
        value = {'name': inst, 'zone': base, 'selfLink': base + '/instances/' + inst, 'status': status,
                 'labels': {'project': 'e-hgp'}, 'machineType': base + '/machineTypes/g4-standard-48',
                 'lastStartTimestamp': state['last_start'], 'lastStopTimestamp': state.get('last_stop'),
                 'metadata': {'items': [{'key': 'enable-oslogin', 'value': modes.get('oslogin_metadata', 'TRUE')},
                                        {'key': 'startup-script', 'value': '#!/bin/bash'}]},
                 'scheduling': {'automaticRestart': False, 'instanceTerminationAction': 'STOP',
                                'onHostMaintenance': 'TERMINATE', 'provisioningModel': 'SPOT',
                                'maxRunDuration': {'seconds': str(state['max_run'])}}}
        done(0, json.dumps(value) + '\n')
    done(0, field(re.fullmatch(r'value\((.*)\)', fmt).group(1)) + '\n')

if positional[:4] == ['compute', 'os-login', 'ssh-keys', 'add']:
    if modes.get('oslogin_add_fails'):
        done(1, err='ERROR: (gcloud.compute.os-login.ssh-keys.add) refus simule\n')
    with open(options['key-file']) as stream:
        key = stream.read().strip()
    minutes = int(re.fullmatch(r'([0-9]+)m', options['ttl']).group(1))
    fingerprint = 'fp%d' % (len(state['keys']) + state.get('removed_keys', 0) + 1)
    state['keys'][fingerprint] = {'key': key, 'expirationTimeUsec': str(int((time.time() + minutes * 60) * 1e6)),
                                  'fingerprint': fingerprint}
    save(state)
    if modes.get('oslogin_add_applied_then_fails'):
        done(1, err='ERROR: delai depasse cote client (cle pourtant inscrite)\n')
    done(0, json.dumps({'loginProfile': {'sshPublicKeys': state['keys']}}) + '\n')

if positional[:4] == ['compute', 'os-login', 'ssh-keys', 'remove']:
    with open(options['key-file']) as stream:
        key = stream.read().split()[:2]
    before = len(state['keys'])
    state['keys'] = {k: v for k, v in state['keys'].items() if v['key'].split()[:2] != key}
    state['removed_keys'] = state.get('removed_keys', 0) + before - len(state['keys'])
    save(state)
    done(0 if before != len(state['keys']) else 1)

if head == ('compute', 'os-login', 'describe-profile'):
    done(0, json.dumps({'name': 'selftest', 'sshPublicKeys': state['keys']}) + '\n')

if head == ('compute', 'instances', 'start'):
    if state['status'] != 'TERMINATED':
        done(1, err='ERROR: la cible n est pas TERMINATED\n')
    now = time.time()
    if modes.get('start_stockout'):
        done(1, err='ERROR: (gcloud.compute.instances.start) ZONE_RESOURCE_POOL_EXHAUSTED_WITH_DETAILS\n')
    state.update(status='RUNNING', last_start=rfc(now), termination=rfc(now + state['max_run']),
                 starts=state.get('starts', 0) + 1, preempt_countdown=modes.get('preempt_countdown'))
    save(state)
    if modes.get('start_fails_but_runs'):
        done(1, err='ERROR: delai depasse (simule)\n')
    done(0)

if head == ('compute', 'instances', 'stop'):
    if modes.get('stop_fails'):
        done(1, err='ERROR: (gcloud.compute.instances.stop) echec simule\n')
    for path in modes.get('readonly_on_stop', []):
        os.chmod(path, 0o500)
    state.update(status='TERMINATED', stopped_by_fake=True, stops=state.get('stops', 0) + 1)
    if not modes.get('json_status_after_stop'):
        state['last_stop'] = rfc(time.time())
    save(state)
    done(0)


def registered(key_file):
    with open(key_file + '.pub') as stream:
        fields = stream.read().split()
    return any(record['key'].split()[:2] == fields[:2] for record in state['keys'].values())


def remote_env():
    env = dict(os.environ)
    env['TMPDIR'] = os.environ['FAKE_REMOTE_ROOT']
    env['HOME'] = os.environ['FAKE_REMOTE_HOME']
    env['FAKE_PY_MODE'] = modes.get('python_mode', 'present')
    kits = [os.path.join(os.environ['FAKE_KITS'], name) for name in ['python'] + modes.get('kits', [])]
    env['PATH'] = os.pathsep.join(kits + [env['PATH']])
    return env


if positional[:2] == ['compute', 'ssh']:
    if state['status'] != 'RUNNING':
        done(255, err='ssh: connect to host: Connection refused\n')
    if not registered(options['ssh-key-file']):
        done(255, err='Permission denied (publickey).\n')
    if modes.get('ssh_fail_substring') and modes['ssh_fail_substring'] in command:
        done(255, err='Connection closed by remote host (simule)\n')
    if command.startswith('sudo -n bash -c '):
        parts = shlex.split(command)
        env = dict(os.environ, PATH=os.environ['FAKE_BIN'] + os.pathsep + os.environ['PATH'])
        result = subprocess.run(['bash', '-c', parts[4]] + parts[5:] + [os.environ['FAKE_SCHEDULED']],
                                env=env, capture_output=True, text=True)
        done(result.returncode, result.stdout, result.stderr)
    if command == 'sudo -n cat /run/systemd/shutdown/scheduled':
        with open(os.environ['FAKE_SCHEDULED']) as stream:
            text = stream.read()
        usec = int(re.search(r'^USEC=([0-9]+)$', text, re.M).group(1))
        usec -= int(modes.get('session_schedule_offset', 0) * 1e6)
        if modes.get('guest_changed_after_launch') and state.get('worker_launch_seen'):
            usec += 1000000
        done(0, re.sub(r'^USEC=[0-9]+$', 'USEC=%d' % usec, text, flags=re.M) + 'WALL_MESSAGE=Coupe-circuit E-HGP\n')
    result = subprocess.run(['bash', '-c', command], env=remote_env(), capture_output=True, text=True)
    if modes.get('ssh_break_after_substring') and modes['ssh_break_after_substring'] in command:
        done(255, err='Connection reset by peer (simule, apres execution)\n')
    done(result.returncode, result.stdout, result.stderr)

if positional[:2] == ['compute', 'scp']:
    if state['status'] != 'RUNNING':
        done(1, err='scp: connexion refusee\n')
    if not registered(options['ssh-key-file']):
        done(1, err='Permission denied (publickey).\n')
    if modes.get('json_failures_after') == 'results_download' and not state.get('json_failures_armed') and \
            any(p.startswith(inst + ':') and p.endswith('/results.tar.gz') for p in positional[2:]):
        state.update(json_failures_left=modes.get('json_failures', 0), json_failures_armed=True)
        save(state)
    paths = [p[len(inst) + 1:] if p.startswith(inst + ':') else p for p in positional[2:]]
    sources, destination = paths[:-1], paths[-1]
    for source in sources:
        if not os.path.exists(source):
            done(1, err='scp: %s: No such file or directory\n' % source)
        target = os.path.join(destination, os.path.basename(source)) if destination.endswith('/') else destination
        shutil.copyfile(source, target)
    done(0)

done(2, err='ERROR: appel gcloud inattendu dans l autotest : %r\n' % (ARGS,))
'''

FAKE_SHUTDOWN = r'''#!/usr/bin/env bash
# Faux shutdown : ecrit le calendrier systemd dans FAKE_SCHEDULED (jamais d'arret reel).
case "${1:-}" in
  -c) rm -f "${FAKE_SCHEDULED}"; exit 0 ;;
  -P) minutes="${2#+}"
      printf 'USEC=%s\nWARN_WALL=1\nMODE=poweroff\n' "$((($(date +%s) + minutes * 60) * 1000000))" \
        > "${FAKE_SCHEDULED}"
      exit 0 ;;
esac
exit 1
'''

FAKE_FORBIDDEN = r'''#!/usr/bin/env bash
# ssh/scp reels interdits dans l'autotest : tout appel direct est journalise et echoue.
printf '%s %s\n' "$(basename "$0")" "$*" >> "${FAKE_FORBIDDEN_LOG}"
exit 97
'''

KIT_PYTHON = r'''#!/usr/bin/env bash
# Faux python3 distant : repond au controle de versions du worker selon FAKE_PY_MODE
# (present | absent_pip | installable) ; tout le reste passe au vrai interpreteur.
real="__REAL_PYTHON__"
mode="${FAKE_PY_MODE:-present}"
# pip --user installe sous PYTHONUSERBASE (defaut ~/.local), seul site utilisateur lu par Python ; un
# site casse par une installation interrompue (.fake_broken) fait echouer le controle, meme apres pip.
mark_dir="${PYTHONUSERBASE:-${HOME}/.local}"
mark="${mark_dir}/.fake_pip_installed"
broken=0
if [ -e "${mark_dir}/.fake_broken" ]; then broken=1; fi
ok() { printf 'hdbscan=0.8.44,numpy=2.2.6,scikit-learn=1.7.2,scipy=1.15.3\n'; exit 0; }
if [ "${1:-}" = "-" ]; then
  script="$(cat)"
  case "${script}" in
    *EHGP_V10_PYTHON_CHECK*)
      case "${mode}" in
        present) ok ;;
        installable) if [ -e "${mark}" ] && [ "${broken}" = 0 ]; then ok; fi ;;
      esac
      echo "ModuleNotFoundError: No module named 'hdbscan' (autotest)" >&2
      exit 1 ;;
  esac
  printf '%s\n' "${script}" | exec "${real}" -
fi
if [ "${1:-}" = "-m" ] && [ "${2:-}" = "pip" ]; then
  if [ "${mode}" = "absent_pip" ]; then echo "/usr/bin/python3: No module named pip (autotest)" >&2; exit 1; fi
  case "${3:-}" in
    --version) echo "pip 22.0.2 (autotest)"; exit 0 ;;
    freeze) echo "numpy==2.2.6"; exit 0 ;;
    install) printf '%s\n' "$*" >> "${HOME}/pip_calls.txt"; mkdir -p "${mark_dir}"; : > "${mark}"; exit 0 ;;
  esac
fi
exec "${real}" "$@"
'''

KIT_CTEST_VACUOUS = r'''#!/usr/bin/env bash
# ctest qui ignore --no-tests=error : « No tests were found!!! » et code 0 (vert par vacuite).
echo "Test project $(pwd)"
echo "No tests were found!!!"
exit 0
'''

KIT_TAR_HANG = r'''#!/usr/bin/env bash
# tar bloque dans l'emballage du worker (results.tar.gz.partial) jusqu'a FAKE_TAR_RELEASE.
for a in "$@"; do
  case "${a}" in
    *results.tar.gz.partial) while [ ! -e "${FAKE_TAR_RELEASE}" ]; do sleep 0.2; done ;;
  esac
done
exec /usr/bin/tar "$@"
'''

KIT_STAT_BIG = r'''#!/usr/bin/env bash
# stat qui annonce une archive de resultats geante.
case "$*" in
  *results.tar.gz*|*salvage.tar.gz*) echo "SIZE=5000000000"; exit 0 ;;
esac
exec /usr/bin/stat "$@"
'''

MINI_CMAKE = '''cmake_minimum_required(VERSION 3.22)
project(mhgp10_selftest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)
add_executable(mhgp10_hello cli/hello.cpp)
add_executable(mhgp10_fail cli/fail.cpp)
add_executable(mhgp10_broken EXCLUDE_FROM_ALL cli/broken.cpp)
enable_testing()
add_test(NAME mhgp10_hello_gate COMMAND mhgp10_hello)
set_tests_properties(mhgp10_hello_gate PROPERTIES LABELS gate)
add_test(NAME mhgp10_disabled_gate COMMAND mhgp10_hello)
set_tests_properties(mhgp10_disabled_gate PROPERTIES LABELS offgate DISABLED TRUE)
'''

HELLO_CPP = r'''#include <cstdio>
#include <fstream>

int main(int argc, char** argv) {
  std::printf("hello argc=%d\n", argc);
  for (int i = 1; i < argc; ++i) {
    std::printf("arg[%d]=%s\n", i, argv[i]);
  }
  if (argc >= 2) {
    std::ifstream in(argv[1], std::ios::binary | std::ios::ate);
    std::printf("bytes=%lld\n", static_cast<long long>(in.tellg()));
  }
  if (argc >= 3) {
    std::ofstream out(argv[2]);
    out << "hello\n";
  }
  return 0;
}
'''

FAIL_CPP = r'''#include <cstdio>

int main() {
  std::fprintf(stderr, "echec volontaire (autotest v10)\n");
  return 3;
}
'''

BROKEN_CPP = '#error "cible volontairement cassee (autotest v10)"\n'

CHECK_PY = '''import hashlib
import os
import sys

with open(sys.argv[1], 'rb') as stream:
    digest = hashlib.sha256(stream.read()).hexdigest()
with open(sys.argv[2], 'w') as stream:
    stream.write(digest + '\\n')
print('check', digest, 'build_env', bool(os.environ.get('V10_BUILD')))
'''

SLEEP_PY = '''import sys
import time

time.sleep(float(sys.argv[1]))
'''

IGNORE_TERM_PY = '''import signal
import subprocess
import sys
import time

signal.signal(signal.SIGTERM, signal.SIG_IGN)
subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(120)'])
time.sleep(float(sys.argv[1]))
'''

NOISY_PY = '''import sys

sys.stdout.write('x' * int(sys.argv[1]))
'''

WRITE_FILES_PY = '''import os
import sys

with open(os.path.join(sys.argv[1], 'big.bin'), 'wb') as stream:
    stream.write(os.urandom(int(sys.argv[2])))
with open(os.path.join(sys.argv[1], 'small.txt'), 'w') as stream:
    stream.write('petit resultat\\n')
'''

ZEROS_PY = '''import os
import sys
import time

with open(os.path.join(sys.argv[1], 'zeros.bin'), 'wb') as stream:
    stream.write(bytes(int(sys.argv[2])))
os.rename(os.path.join(sys.argv[1], 'zeros.bin'), os.path.join(sys.argv[1], 'zeros.done'))
time.sleep(float(sys.argv[3]))
'''


class SelftestFailure(Exception):
    pass


def check(condition, message):
    if not condition:
        raise SelftestFailure(message)


def git(repo, *args):
    result = subprocess.run(['git', '-C', str(repo), '-c', 'user.name=selftest', '-c',
                             'user.email=selftest@example.invalid', '-c', 'commit.gpgsign=false', '-c',
                             'core.hooksPath=/dev/null', '-c', 'init.defaultBranch=main', *args],
                            capture_output=True, text=True, stdin=subprocess.DEVNULL)
    check(result.returncode == 0, 'git %s : %s' % (' '.join(args), result.stderr))
    return result.stdout.strip()


def write(path, text, mode=None):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    if mode is not None:
        path.chmod(mode)


def wait_for(predicate, seconds, message, process=None):
    end = time.monotonic() + seconds
    while not predicate():
        check(time.monotonic() < end, 'delai depasse : ' + message)
        check(process is None or process.poll() is None or predicate(), 'processus termine avant : ' + message)
        time.sleep(0.1)


def unshare_available():
    try:
        return subprocess.run(['unshare', '-Urm', 'true'], capture_output=True, timeout=30).returncode == 0
    except (OSError, subprocess.SubprocessError):
        return False


BASE = ORIGIN = REPO = COMMIT = DATA = None


def setUpModule():
    global BASE, ORIGIN, REPO, COMMIT, DATA
    BASE = Path(tempfile.mkdtemp(prefix='ehgp-v10-selftest-'))
    (BASE / 'tmp').mkdir()
    ORIGIN = BASE / 'origin.git'
    subprocess.run(['git', 'init', '-q', '--bare', '-b', 'main', str(ORIGIN)], check=True)
    REPO = BASE / 'repo'
    subprocess.run(['git', 'init', '-q', '-b', 'main', str(REPO)], check=True)
    (REPO / 'gcp-migration').mkdir()
    for name in PROTOCOL:
        shutil.copyfile(HERE / name, REPO / 'gcp-migration' / name)
        (REPO / 'gcp-migration' / name).chmod(0o755)
    root = REPO / 'morsehgp3D_v10'
    write(root / 'CMakeLists.txt', MINI_CMAKE)
    write(root / 'cli' / 'hello.cpp', HELLO_CPP)
    write(root / 'cli' / 'fail.cpp', FAIL_CPP)
    write(root / 'cli' / 'broken.cpp', BROKEN_CPP)
    write(root / 'python' / 'check.py', CHECK_PY)
    write(root / 'python' / 'sleep.py', SLEEP_PY)
    write(root / 'python' / 'ignore_term.py', IGNORE_TERM_PY)
    write(root / 'python' / 'noisy.py', NOISY_PY)
    write(root / 'python' / 'write_files.py', WRITE_FILES_PY)
    write(root / 'python' / 'zeros.py', ZEROS_PY)
    git(REPO, 'add', '-A')
    git(REPO, 'commit', '-q', '-m', 'autotest v10')
    git(REPO, 'remote', 'add', 'origin', str(ORIGIN))
    git(REPO, 'push', '-q', 'origin', 'main')
    git(REPO, 'fetch', '-q', 'origin')
    COMMIT = git(REPO, 'rev-parse', 'HEAD')
    DATA = BASE / 'data'
    DATA.mkdir()
    (DATA / 'a.u32le').write_bytes(bytes(range(256)) * 12)
    (DATA / 'b.u32le').write_bytes(b'\x01\x00\x00\x00' * 300)


def tearDownModule():
    if BASE is not None and os.environ.get('V10_SELFTEST_KEEP') != '1':
        subprocess.run(['chmod', '-R', 'u+rwx', str(BASE)], capture_output=True)
        shutil.rmtree(BASE, ignore_errors=True)


def kill_leftovers(marker):
    """Tue les processus residuels d'un scenario (cmdline contenant son dossier)."""
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit() or int(entry.name) == os.getpid():
            continue
        try:
            cmdline = (entry / 'cmdline').read_bytes().replace(b'\0', b' ').decode(errors='replace')
        except OSError:
            continue
        if marker in cmdline:
            try:
                os.kill(int(entry.name), signal.SIGKILL)
            except OSError:
                pass


class Cloud:
    """Faux cloud d'un scenario : etat, journal, faux binaires, kits distants, faux distant."""

    def __init__(self, directory, status='TERMINATED', max_run=3600, **modes):
        self._paths(directory)
        self.bin.mkdir(parents=True)
        for name in ('remote', 'remote_home', 'cloudsdk'):
            (directory / name).mkdir()
        write(self.bin / 'gcloud', '#!' + sys.executable + '\n' + FAKE_GCLOUD, 0o755)
        write(self.bin / 'shutdown', FAKE_SHUTDOWN, 0o755)
        write(self.bin / 'ssh', FAKE_FORBIDDEN, 0o755)
        write(self.bin / 'scp', FAKE_FORBIDDEN, 0o755)
        kits = directory / 'kits'
        write(kits / 'python' / 'python3', KIT_PYTHON.replace('__REAL_PYTHON__', sys.executable), 0o755)
        write(kits / 'ctest_vacuous' / 'ctest', KIT_CTEST_VACUOUS, 0o755)
        write(kits / 'tar_hang' / 'tar', KIT_TAR_HANG, 0o755)
        write(kits / 'big_results' / 'stat', KIT_STAT_BIG, 0o755)
        self.state_path.write_text(json.dumps({
            'project': TARGET['project'], 'zone': TARGET['zone'], 'instance': TARGET['instance'],
            'status': status, 'max_run': max_run, 'last_start': '2026-09-20T08:00:00.000000-07:00',
            'last_stop': '2026-09-20T07:00:00.000000-07:00',
            'keys': {}, 'modes': modes}))
        self.sessions.mkdir()

    def _paths(self, directory):
        self.dir = directory
        self.bin = directory / 'fakebin'
        self.state_path = directory / 'state.json'
        self.log_path = directory / 'gcloud.log'
        self.forbidden_log = directory / 'forbidden.log'
        self.release = directory / 'tar_release'
        self.sessions = directory / 'sessions'
        self.session = self.sessions / 's1'

    @classmethod
    def attach(cls, directory):
        cloud = cls.__new__(cls)
        cloud._paths(Path(directory))
        return cloud

    def env(self):
        env = dict(os.environ)
        env.update(PATH=str(self.bin) + os.pathsep + os.environ.get('PATH', ''), TMPDIR=str(BASE / 'tmp'),
                   CLOUDSDK_CONFIG=str(self.dir / 'cloudsdk'), FAKE_GCP_STATE=str(self.state_path),
                   FAKE_GCP_LOG=str(self.log_path), FAKE_REMOTE_ROOT=str(self.dir / 'remote'),
                   FAKE_REMOTE_HOME=str(self.dir / 'remote_home'), FAKE_BIN=str(self.bin),
                   FAKE_KITS=str(self.dir / 'kits'), FAKE_SCHEDULED=str(self.dir / 'scheduled'),
                   FAKE_FORBIDDEN_LOG=str(self.forbidden_log), FAKE_TAR_RELEASE=str(self.release),
                   FAKE_SESSION_DIR=str(self.session), FAKE_RUN_DIR=str(self.run_dir()))
        env.pop('CLOUDSDK_CORE_PROJECT', None)
        env.pop('CLOUDSDK_CORE_ACCOUNT', None)
        return env

    def run_dir(self):
        digest = hashlib.sha256(str(self.session).encode()).hexdigest()[:12]
        return BASE / 'tmp' / 'ehgp-v10-runs' / ('s1-' + digest)

    def calls(self):
        if not self.log_path.exists():
            return []
        return [json.loads(line) for line in self.log_path.read_text().splitlines()]

    def verbs(self):
        result = []
        for argv in self.calls():
            positional = [a for a in argv if not a.startswith('-')]
            if positional[:2] == ['config', 'get-value']:
                result.append(('config', 'get-value'))
            elif positional[:3] == ['compute', 'os-login', 'ssh-keys']:
                result.append(tuple(positional[:4]))
            elif positional[:2] in (['compute', 'ssh'], ['compute', 'scp']):
                result.append(tuple(positional[:2]))
            else:
                result.append(tuple(positional[:3]))
        return result

    def state(self):
        return json.loads(self.state_path.read_text())

    def set_modes(self, **modes):
        state = self.state()
        state['modes'].update(modes)
        self.state_path.write_text(json.dumps(state))

    def set_state(self, **values):
        state = self.state()
        state.update(values)
        self.state_path.write_text(json.dumps(state))

    def argv(self, plan, execute=True, wait=True, repo=None, commit=None, max_run=3600, data=None, child=False,
             extra=()):
        plan_path = self.dir / 'plan.json'
        plan_path.write_text(json.dumps(plan) if not isinstance(plan, str) else plan)
        argv = [sys.executable, str((repo or REPO) / 'gcp-migration' / 'v10_session.py'),
                '--commit', commit or COMMIT, '--plan', str(plan_path), '--data', str(data or DATA),
                '--session-dir', str(self.session), '--max-run-seconds', str(max_run),
                '--gcloud', str(self.bin / 'gcloud'), '--sessions-root', str(self.sessions),
                '--poll-seconds', '0.2', '--poll-window', '2', *extra]
        if execute:
            argv.append('--execute')
            if child:
                argv.append('--child')
            elif wait:
                argv.append('--wait')
        return argv

    def run(self, plan, execute=True, prefix=(), **kwargs):
        result = subprocess.run([*prefix, *self.argv(plan, execute, **kwargs)], env=self.env(), capture_output=True,
                                text=True, stdin=subprocess.DEVNULL, timeout=1200)
        try:
            output = json.loads(result.stdout)
        except ValueError:
            output = None
        return result.returncode, output, result.stderr

    def recover(self, wait=None):
        argv = [sys.executable, str(REPO / 'gcp-migration' / 'v10_session.py'), '--recover', '--session-dir',
                str(self.session), '--gcloud', str(self.bin / 'gcloud')]
        if wait is not None:
            argv += ['--recover-wait', str(wait)]
        result = subprocess.run(argv, env=self.env(), capture_output=True, text=True, timeout=600)
        try:
            output = json.loads(result.stdout)
        except ValueError:
            output = None
        return result.returncode, output, result.stderr

    def popen(self, plan, **kwargs):
        return subprocess.Popen(self.argv(plan, **kwargs), env=self.env(), stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True, stdin=subprocess.DEVNULL)

    def child_pid(self):
        wait_for(lambda: (self.session / 'launch.json').exists(), 60, 'launch.json')
        return json.loads((self.session / 'launch.json').read_text())['pid']

    def receipt(self):
        return json.loads((self.session / 'receipt.json').read_text())

    def remote_dirs(self):
        return sorted((self.dir / 'remote_home' / 'ehgp-v10').glob('ehgp-v10.*'))

    def results(self):
        return self.session / 'results' / 'extracted' / 'results'

    def rows(self, receipt, name):
        return [row for row in receipt.get('host_commands', []) if row['name'] == name]

    def nologin_refusals(self):
        path = Path(str(self.state_path) + '.nologin')
        return path.read_text().splitlines() if path.exists() else []


def plan_of(*commands, **extra):
    value = {'schema': PLAN_SCHEMA, 'commands': [
        {'name': name, 'argv': list(argv), 'timeout_seconds': timeout} for name, argv, timeout in commands]}
    value.update(extra)
    return value


HELLO = ('hello', ['./mhgp10_hello', '{data}/a.u32le', '{out}/hello.txt', TRICKY_ARG], 60)
HELLO_MANIFEST = ('hello_manifest', ['./mhgp10_hello', '{data}/b.u32le', '{out}/MANIFEST.sha256'], 60)
HELLO_BACKSLASH = ('hello_backslash', ['./mhgp10_hello', '{data}/b.u32le', '{out}/' + BACKSLASH_NAME], 60)
CHECK = ('check', ['python3', '{src}/morsehgp3D_v10/python/check.py', '{data}/a.u32le', '{out}/check.txt'], 60)
CTEST_GATE = ('gates', ['ctest', '--no-tests=error', '-L', '^gate$'], 120)


def sleeper(seconds, timeout=120):
    return ('sleeper', ['python3', '{src}/morsehgp3D_v10/python/sleep.py', str(seconds)], timeout)


def prepared_session(cloud, generation, lifecycle_state='targeted_running'):
    """Dossier de session fabrique a la main (reprise) : scripts gardes du depot, handoff et cycle de vie."""
    host = cloud.session / 'host'
    (host / 'logs').mkdir(parents=True)
    cloud.session.chmod(0o700)
    for name in ('start_and_verify.sh', 'stop_and_verify.sh'):
        shutil.copyfile(REPO / 'gcp-migration' / name, host / name)
        (host / name).chmod(0o700)
    (host / 'lifecycle.txt').write_text(
        'schema=e-hgp.lifecycle-state.v1\nstate=%s\nproject=%s\nzone=%s\ninstance=%s\ngeneration=%s\n' % (
            lifecycle_state, TARGET['project'], TARGET['zone'], TARGET['instance'], generation))
    (host / 'handoff.json').write_text(json.dumps(
        {'guest_shutdown_minutes': 45, 'instance': TARGET['instance'], 'last_start_timestamp': generation,
         'project': TARGET['project'], 'schema': 'e-hgp.start-handoff.v3', 'status': 'targeted_running',
         'zone': TARGET['zone']}, sort_keys=True, separators=(',', ':')) + '\n')


def namespace_run(request):
    """Execute DANS un espace de noms `unshare -Urm` : lance la session et rend un verdict JSON lu de
    l'interieur (le tmpfs monte sur la session n'est visible que la)."""
    global BASE, REPO, COMMIT, DATA
    BASE, REPO, COMMIT, DATA = (Path(request['base']), Path(request['repo']), request['commit'],
                                Path(request['data']))
    cloud = Cloud.attach(request['cloud'])
    code, output, err = cloud.run(request['plan'])
    verdict = {'code': code, 'output': output, 'stderr_tail': err[-2000:], 'state': cloud.state(),
               'done': None, 'receipt': None, 'df_free': None}
    for place, directory in (('session', cloud.session), ('run_dir', cloud.run_dir())):
        try:
            verdict['done'] = (directory / 'DONE').read_text().strip()
            verdict['done_place'] = place
            break
        except OSError:
            pass
    for place, directory in (('session', cloud.session), ('run_dir', cloud.run_dir())):
        try:
            verdict['receipt'] = json.loads((directory / 'receipt.json').read_text())
            verdict['receipt_place'] = place
            break
        except (OSError, ValueError):
            pass
    try:
        verdict['df_free'] = shutil.disk_usage(cloud.session).free
    except OSError:
        pass
    kill_leftovers(str(cloud.dir))
    return verdict


class V10SessionSelftest(unittest.TestCase):
    maxDiff = None

    def setUp(self):
        self.base = Path(tempfile.mkdtemp(prefix='scenario-', dir=BASE))
        self.clouds = []

    def tearDown(self):
        for cloud in self.clouds:
            cloud.release.touch()
        time.sleep(0.5)
        kill_leftovers(str(self.base))
        subprocess.run(['chmod', '-R', 'u+rwx', str(self.base), str(BASE / 'tmp')], capture_output=True)

    def cloud(self, name='c', **kwargs):
        cloud = Cloud(self.base / name, **kwargs)
        self.clouds.append(cloud)
        return cloud

    # -- aides d'assertion -------------------------------------------------------------------

    def assert_no_forbidden(self, cloud):
        check(not cloud.forbidden_log.exists(), 'ssh/scp reels appeles : ' +
              (cloud.forbidden_log.read_text() if cloud.forbidden_log.exists() else ''))
        check(not cloud.nologin_refusals(), 'SSH/SCP tente apres /run/nologin : %r' % cloud.nologin_refusals())

    def assert_read_only(self, cloud):
        verbs = cloud.verbs()
        check(all(verb in READ_ONLY for verb in verbs), 'commande GCP mutante en lecture seule : %r' % verbs)

    def assert_no_stop(self, cloud, receipt):
        check(not cloud.rows(receipt, 'guarded_stop'), 'stop_and_verify.sh appele sans generation prouvee')
        check(('compute', 'instances', 'stop') not in cloud.verbs(), 'instances stop emis')

    def assert_certified_stop(self, cloud, receipt, gce_stops=1):
        state = cloud.state()
        check(receipt['targeted_shutdown_certified'] is True, 'arret cible non certifie : %r' % receipt['errors'])
        check(state['status'] == 'TERMINATED', 'la fausse cible n est pas TERMINATED')
        check(receipt['closing_generation'] == state['last_start'], 'generation de fermeture differente')
        stops = cloud.rows(receipt, 'guarded_stop')
        check(len(stops) == gce_stops, 'stop_and_verify.sh appele %d fois, %d attendu' % (len(stops), gce_stops))
        for stop in stops:
            check(stop['argv'][1:] == ['--yes', '--expected-last-start-timestamp', receipt['closing_generation']],
                  'arret non cible : %r' % stop['argv'])
        check(receipt['observed_before_stop']['lastStartTimestamp'] == receipt['closing_generation'],
              'describe prealable a l\'arret absent')
        check(cloud.verbs().count(('compute', 'instances', 'stop')) == gce_stops, 'nombre de stops GCE')
        verbs = cloud.verbs()
        if ('compute', 'instances', 'stop') in verbs:
            last_stop = len(verbs) - 1 - verbs[::-1].index(('compute', 'instances', 'stop'))
            check(not any(v in (('compute', 'ssh'), ('compute', 'scp')) for v in verbs[last_stop:]),
                  'ssh/scp apres l arret')
        check(not (cloud.session / 'key').exists(), 'cle privee non effacee')
        self.assert_no_forbidden(cloud)

    # -- refus sans mutation --------------------------------------------------------------------

    def test_dry_run_is_read_only(self):
        cloud = self.cloud()
        before = cloud.state()
        code, output, err = cloud.run(plan_of(HELLO, CHECK, CTEST_GATE), execute=False)
        check(code == 0 and output['status'] == 'dry_run_ok', 'dry-run refuse : %r %s' % (output, err))
        check(output['gcp_mutations'] == 'none' and output['commit'] == COMMIT, 'rapport de dry-run')
        check(output['guest_shutdown_minutes'] == 45 and output['os_login_ttl_minutes'] == 65, 'budget de garde')
        check(output['budget']['closing_reserve_seconds'] == 660 and
              output['budget']['ssh_cutoff_before_guest_seconds'] == 360, 'reserve de fermeture avant nologin')
        check(output['plan']['needs_python'] is True and output['plan']['needs_python_deduced'] is True,
              'needs_python doit etre deduit des commandes python3/ctest')
        check(output['python_pins'] == {'numpy': '2.2.6', 'scipy': '1.15.3', 'scikit-learn': '1.7.2',
                                        'hdbscan': '0.8.44'}, 'versions Python epinglees')
        check(output['run_dir'].startswith(str(BASE / 'tmp')), 'dossier d\'execution sous $TMPDIR')
        check(len(output['steps']) >= 8, 'le dry-run doit imprimer les etapes prevues')
        check(cloud.calls(), 'le dry-run doit relire la cible')
        self.assert_read_only(cloud)
        self.assert_no_forbidden(cloud)
        check(cloud.state() == before, 'le dry-run a modifie la fausse cible')
        check(not any(cloud.sessions.iterdir()), 'le dry-run a cree un dossier de session')

    def test_unpushed_commit_is_refused(self):
        clone = self.base / 'clone'
        subprocess.run(['git', 'clone', '-q', str(ORIGIN), str(clone)], check=True)
        write(clone / 'morsehgp3D_v10' / 'NOTE.txt', 'commit local non pousse\n')
        git(clone, 'add', '-A')
        git(clone, 'commit', '-q', '-m', 'non pousse')
        local = git(clone, 'rev-parse', 'HEAD')
        cloud = self.cloud()
        for execute in (False, True):
            code, output, _ = cloud.run(plan_of(HELLO), execute=execute, repo=clone, commit=local)
            check(code == 2 and 'origin/main' in output['reason'], 'commit non pousse accepte : %r' % output)
        check(cloud.calls() == [], 'appel GCP avant le refus du commit')
        check(not any(cloud.sessions.iterdir()), 'dossier de session cree malgre le refus')

    def test_controller_must_match_commit(self):
        clone = self.base / 'clone'
        subprocess.run(['git', 'clone', '-q', str(ORIGIN), str(clone)], check=True)
        with open(clone / 'gcp-migration' / 'v10_session.py', 'a') as stream:
            stream.write('# modification locale non committee\n')
        cloud = self.cloud()
        code, output, _ = cloud.run(plan_of(HELLO), repo=clone)
        check(code == 2 and 'controleur' in output['reason'], 'controleur non committe accepte : %r' % output)
        check(cloud.calls() == [], 'appel GCP avant le refus du controleur')

    def test_guard_scripts_are_pinned(self):
        clone = self.base / 'clone'
        subprocess.run(['git', 'clone', '-q', str(ORIGIN), str(clone)], check=True)
        with open(clone / 'gcp-migration' / 'stop_and_verify.sh', 'a') as stream:
            stream.write('# modification non revue\n')
        git(clone, 'commit', '-q', '-am', 'script garde modifie')
        git(clone, 'push', '-q', 'origin', 'HEAD:main')
        cloud = self.cloud()
        code, output, _ = cloud.run(plan_of(HELLO), repo=clone, commit=git(clone, 'rev-parse', 'HEAD'))
        check(code == 2 and 'epingle' in output['reason'], 'script garde modifie accepte : %r' % output)
        check(cloud.calls() == [], 'appel GCP avant le refus du script garde')

    def test_invalid_plans_are_refused_before_any_gcp_call(self):
        cases = {
            'binaire_inconnu': plan_of(('x', ['./mhgp10_absent'], 10)),
            'donnee_absente': plan_of(('x', ['./mhgp10_hello', '{data}/absent.u32le'], 10)),
            'script_non_suivi': plan_of(('x', ['python3', '{src}/morsehgp3D_v10/python/absent.py'], 10)),
            'commande_libre': plan_of(('x', ['bash', '-c', 'true'], 10)),
            'nom_duplique': plan_of(('x', ['./mhgp10_hello'], 10), ('x', ['./mhgp10_hello'], 10)),
            'delai_invalide': plan_of(('x', ['./mhgp10_hello'], 0)),
            'cle_inconnue': plan_of(HELLO, extra_key=True),
            'schema': dict(plan_of(HELLO), schema='autre'),
            'hors_build_targets': plan_of(HELLO, build_targets=['mhgp10_fail']),
            'plafond_trop_petit': plan_of(HELLO, results_cap_bytes=1000),
            'plafond_trop_grand': plan_of(HELLO, results_cap_bytes=2 ** 31),
            'json_duplique': '{"schema": "%s", "schema": "%s", "commands": []}' % (PLAN_SCHEMA, PLAN_SCHEMA),
            'ctest_sans_no_tests': plan_of(('t', ['ctest', '-L', 'scale8000'], 10)),
            'ctest_no_tests_valeur_de_R': plan_of(('t', ['ctest', '-R', '--no-tests=error', '-L', 'x'], 10)),
            'ctest_no_tests_ignore': plan_of(('t', ['ctest', '--no-tests=error', '--no-tests=ignore'], 10)),
            'ctest_script': plan_of(('t', ['ctest', '--no-tests=error', '-S', '{data}/a.u32le'], 10)),
            'ctest_dashboard': plan_of(('t', ['ctest', '--no-tests=error', '-D', 'Experimental'], 10)),
            'ctest_build_and_test': plan_of(('t', ['ctest', '--no-tests=error', '--build-and-test', 'a', 'b'], 10)),
            'ctest_test_dir': plan_of(('t', ['ctest', '--no-tests=error', '--test-dir', '{data}'], 10)),
            'ctest_quiet': plan_of(('t', ['ctest', '--no-tests=error', '-Q'], 10)),
            'ctest_repeat': plan_of(('t', ['ctest', '--no-tests=error', '--repeat', 'until-pass:9'], 10)),
            'ctest_liste': plan_of(('t', ['ctest', '--no-tests=error', '-N'], 10)),
        }
        for label, plan in cases.items():
            cloud = self.cloud(label)
            code, output, _ = cloud.run(plan, execute=False)
            check(code == 2 and output['status'] == 'refused', 'plan invalide accepte (%s) : %r' % (label, output))
            check(cloud.calls() == [], 'appel GCP avant le refus du plan (%s)' % label)
        colon = self.base / 'da:ta'
        shutil.copytree(DATA, colon)
        cloud = self.cloud('deux_points')
        code, output, _ = cloud.run(plan_of(HELLO), execute=False, data=colon)
        check(code == 2 and '«' in output['reason'] and cloud.calls() == [], 'chemin --data avec « : » accepte')

    def test_max_run_mismatch_is_refused(self):
        cloud = self.cloud()
        code, output, _ = cloud.run(plan_of(HELLO), execute=False, max_run=7200)
        check(code == 2 and 'allowlist' in output['reason'], 'maxRunDuration different accepte : %r' % output)
        self.assert_read_only(cloud)

    def test_target_not_terminated_or_without_oslogin_is_refused(self):
        cloud = self.cloud(status='RUNNING')
        for execute in (False, True):
            code, output, _ = cloud.run(plan_of(HELLO), execute=execute)
            check(code == 2 and 'TERMINATED' in output['reason'], 'cible RUNNING acceptee : %r' % output)
        self.assert_read_only(cloud)
        check(not any(cloud.sessions.iterdir()), 'dossier de session cree pour une cible RUNNING')
        cloud = self.cloud('sans_oslogin', oslogin_metadata='FALSE')
        code, output, _ = cloud.run(plan_of(HELLO))
        check(code == 2 and 'enable-oslogin' in output['reason'], 'cible sans OS Login acceptee : %r' % output)
        self.assert_read_only(cloud)

    # -- chemin nominal ------------------------------------------------------------------------

    def test_completed_session(self):
        cloud = self.cloud()
        plan = plan_of(HELLO, HELLO_MANIFEST, HELLO_BACKSLASH, CHECK, CTEST_GATE)
        code, output, err = cloud.run(plan)
        receipt = cloud.receipt()
        check(code == 0 and output['status'] == 'completed',
              'session non conforme : %r\n%s' % (output, (cloud.session / 'session.stderr').read_text()[-3000:]))
        check(receipt['errors'] == [] and receipt['closure'] == 'stopped', 'recu : %r' % receipt['errors'])
        self.assert_certified_stop(cloud, receipt)
        check(receipt['recovery_command'] == (
            'CLOUDSDK_CORE_PROJECT=devpod-gpu-exploration CLOUDSDK_CORE_ACCOUNT=selftest@example.invalid '
            'GCP_PROJECT_ID=devpod-gpu-exploration GCP_ZONE=us-central1-b GCP_INSTANCE_NAME='
            'ehgp-v7-4fa0e0789a7d5bb06b787d35 %s --yes --expected-last-start-timestamp %s' % (
                cloud.session / 'host' / 'stop_and_verify.sh', receipt['closing_generation'])),
            'commande de reprise inexacte : %r' % receipt['recovery_command'])
        recovery = (cloud.session / 'host' / 'RECOVERY.txt').read_text()
        check('Lire d\'abord' in recovery and 'SEULEMENT si' in recovery and '--recover' in recovery and
              'ET lastStopTimestamp absent ou anterieur' in recovery and 'CLOUDSDK_CORE_ACCOUNT=' in recovery,
              'RECOVERY.txt sans describe prealable')
        start = cloud.rows(receipt, 'guarded_start')[0]
        check(start['argv'][1:4] == ['--yes', '--guest-shutdown-minutes', '45'], 'garde invitee de 45 min')
        verbs = cloud.verbs()
        adds = [argv for argv in cloud.calls() if argv[:4] == ['compute', 'os-login', 'ssh-keys', 'add']]
        check(len(adds) == 1 and '--ttl=65m' in adds[0], 'inscription OS Login bornee a 65 min')
        check(verbs.index(('compute', 'os-login', 'ssh-keys', 'add')) < verbs.index(('compute', 'instances', 'start'))
              < verbs.index(('compute', 'instances', 'stop')) < verbs.index(('compute', 'os-login', 'ssh-keys',
                                                                             'remove')),
              'ordre cle -> demarrage -> arret -> retrait de cle')
        check(receipt['oslogin_key_removed'] is True and cloud.state()['keys'] == {}, 'cle OS Login non retiree')
        check(cloud.session.stat().st_mode & 0o777 == 0o700, 'session en mode 700')
        check((cloud.session / 'DONE').read_text().strip() == '0', 'sentinelle DONE')
        check(receipt['receipt_path'] == str(cloud.session / 'receipt.json'), 'recu hors du dossier de session')
        check(receipt['reserve_released'] and not (cloud.session / 'host' / 'reserve.bin').exists(), 'reserve')
        run_logs = cloud.run_dir() / 'logs'
        check(list(run_logs.glob('*_guarded_stop.stdout')) and
              list((cloud.session / 'host' / 'logs').glob('*_guarded_stop.stdout')),
              'journaux du dossier d\'execution ou miroir absents')
        remote = cloud.remote_dirs()
        check(len(remote) == 1 and receipt['remote_directory'] == str(remote[0]), 'repertoire distant hors /tmp')
        results = cloud.results()
        hello = (results / 'cmd' / '000_hello' / 'stdout').read_text()
        check('arg[3]=' + TRICKY_ARG + '\n' in hello, 'argument transmis non litteral : %r' % hello)
        check(not list(self.base.rglob('PWNED*')), 'un argument du plan a ete interprete par un shell')
        check((results / 'cmd' / '001_hello_manifest' / 'files' / 'MANIFEST.sha256').is_file(),
              'sortie nommee MANIFEST.sha256 perdue')
        check((results / 'cmd' / '002_hello_backslash' / 'files' / BACKSLASH_NAME).is_file(),
              'nom avec barre oblique inverse perdu')
        digest = hashlib.sha256((DATA / 'a.u32le').read_bytes()).hexdigest()
        check((results / 'cmd' / '003_check' / 'files' / 'check.txt').read_text().strip() == digest, 'script Python')
        check('100% tests passed, 0 tests failed out of 1' in (results / 'cmd' / '004_gates' / 'stdout').read_text(),
              'ctest n a execute aucune porte')
        meta = (results / 'cmd' / '000_hello' / 'meta.txt').read_text()
        check('group_closed=1' in meta and 'time_txt=present' in meta and 'streams_truncated=0' in meta, 'meta')
        for name in ('nproc', 'lscpu', 'loadavg_start', 'loadavg_end', 'gxx', 'cmake', 'tool_paths', 'pip_freeze'):
            check((results / 'env' / (name + '.txt')).stat().st_size > 0, 'releve d environnement vide : ' + name)
        worker = receipt['worker']
        check(worker['status'] == 'completed' and worker['pip'] == 'already_present' and
              worker['python_versions'] == PINNED and worker['overflow_files'] == '0' and
              worker['truncated_streams'] == '0', 'worker : %r' % worker)
        check(worker['commit'] == COMMIT and worker['generation'] == receipt['generation'], 'provenance du worker')
        check('mhgp10_hello' in receipt['provenance']['binaries_sha256'], 'sha256 des binaires absent')
        check(receipt['results_verified'] and [r['status'] for r in receipt['commands']] == ['ok'] * 5, 'statuts')
        check('NON comparables' in receipt['comparability'], 'avertissement de comparabilite absent')
        check(receipt['guest_guard_intact'] is True, 'garde invitee non relue')

    # -- echecs distants : l'arret est toujours certifie -----------------------------------------

    def test_worker_failures_still_stop(self):
        cloud = self.cloud()
        plan = plan_of(('fail', ['./mhgp10_fail'], 30),
                       ('ignore_term', ['python3', '{src}/morsehgp3D_v10/python/ignore_term.py', '60'], 2),
                       ('scale', ['ctest', '--no-tests=error', '-L', 'scale8000'], 60), HELLO)
        code, output, _ = cloud.run(plan)
        receipt = cloud.receipt()
        check(code == 3 and output['status'] == 'failed_remote', 'echec distant mal classe : %r' % output)
        self.assert_certified_stop(cloud, receipt)
        rows = [(r['name'], r['status'], r['exit_code']) for r in receipt['commands']]
        check(rows[0] == ('fail', 'failed', '3') and rows[1][:2] == ('ignore_term', 'timeout') and
              rows[1][2] in ('124', '137') and rows[2][:2] == ('scale', 'failed') and rows[3] == ('hello', 'ok', '0'),
              'statuts des commandes apres echecs : %r' % rows)
        meta = (cloud.results() / 'cmd' / '001_ignore_term' / 'meta.txt').read_text()
        check('time_txt=present' in meta and 'residual_group_killed=1' in meta and 'group_closed=1' in meta,
              'commande coupee : time.txt ou fermeture du groupe : %s' % meta)
        wall = float(meta.split('wall_seconds=')[1].split()[0])
        check(wall < 20, 'temps mural gonfle par la grace de fermeture du groupe : %s' % wall)
        check(receipt['worker_exit_code'] == 1 and receipt['results_verified'], 'resultats rapatries malgre l echec')

    def test_ctest_vacuous_or_disabled_is_not_ok(self):
        cloud = self.cloud(kits=['ctest_vacuous'])
        code, output, _ = cloud.run(plan_of(CTEST_GATE))
        receipt = cloud.receipt()
        check(code == 3 and [r['status'] for r in receipt['commands']] == ['vacuous'],
              'ctest sans test accepte : %r' % output)
        self.assert_certified_stop(cloud, receipt)
        cloud = self.cloud('desactive')
        code, output, _ = cloud.run(plan_of(('gates', ['ctest', '--no-tests=error', '-L', 'gate'], 120)))
        receipt = cloud.receipt()
        stdout = (cloud.results() / 'cmd' / '000_gates' / 'stdout').read_text()
        check(code == 3 and [r['status'] for r in receipt['commands']] == ['vacuous'] and
              'The following tests did not run' in stdout, 'test ctest desactive accepte : %r' % output)
        self.assert_certified_stop(cloud, receipt)

    def test_build_failure_still_stops(self):
        cloud = self.cloud()
        code, output, _ = cloud.run(plan_of(('broken', ['./mhgp10_broken'], 30), build_targets=['mhgp10_broken']))
        receipt = cloud.receipt()
        check(code == 3 and receipt['worker']['build'] == 'failed', 'echec de construction mal classe : %r' % output)
        check([r['status'] for r in receipt['commands']] == ['skipped_build'], 'commande non sautee')
        self.assert_certified_stop(cloud, receipt)

    def test_ssh_launch_failures(self):
        broken = self.cloud('broken', ssh_fail_substring='setsid nohup')
        code, output, _ = broken.run(plan_of(HELLO))
        receipt = broken.receipt()
        check(code == 3 and receipt['worker_launched'] is False, 'coupure SSH au lancement mal classee : %r' % output)
        self.assert_certified_stop(broken, receipt)
        after = self.cloud('after', ssh_break_after_substring='setsid nohup')
        code, output, _ = after.run(plan_of(HELLO))
        receipt = after.receipt()
        check(code == 0 and receipt['worker_launch_recovered'] is True,
              'worker lance malgre la coupure SSH non recupere : %r' % output)
        self.assert_certified_stop(after, receipt)

    def test_pip_absent_is_reported_and_binaries_still_run(self):
        cloud = self.cloud(python_mode='absent_pip')
        code, output, _ = cloud.run(plan_of(HELLO, CHECK))
        receipt = cloud.receipt()
        check(code == 3 and receipt['worker']['pip'] == 'failed', 'pip absent non signale : %r' % output)
        check(receipt['commands'][0]['status'] == 'ok', 'binaire non execute apres l echec pip')
        self.assert_certified_stop(cloud, receipt)

    def test_pip_installs_pinned_wheels(self):
        cloud = self.cloud(python_mode='installable')
        code, output, _ = cloud.run(plan_of(CHECK))
        receipt = cloud.receipt()
        check(code == 0 and receipt['worker']['pip'] == 'installed', 'installation epinglee : %r' % output)
        calls = (cloud.dir / 'remote_home' / 'pip_calls.txt').read_text()
        check(all(pin in calls for pin in ('numpy==2.2.6', 'scipy==1.15.3', 'scikit-learn==1.7.2', 'hdbscan==0.8.44'))
              and '--user' in calls and '--only-binary=:all:' in calls and '--no-cache-dir' in calls,
              'pip non epingle : %s' % calls)
        self.assert_certified_stop(cloud, receipt)

    def test_guest_guard_change_is_reported(self):
        cloud = self.cloud(guest_changed_after_launch=True)
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 3 and receipt['guest_guard_intact'] is False, 'garde invitee modifiee non vue : %r' % output)
        self.assert_certified_stop(cloud, receipt)

    def test_results_above_cap_stay_on_vm(self):
        cloud = self.cloud(kits=['big_results'])
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 3 and any('plafond' in error for error in receipt['errors']), 'archive geante acceptee')
        check(not cloud.rows(receipt, 'download'), 'telechargement malgre le plafond')
        self.assert_certified_stop(cloud, receipt)

    def test_results_overflow_is_never_completed(self):
        cloud = self.cloud()
        plan = plan_of(('noisy', ['python3', '{src}/morsehgp3D_v10/python/noisy.py', str(3 * 2 ** 20)], 60),
                       ('files', ['python3', '{src}/morsehgp3D_v10/python/write_files.py', '{out}', str(3 * 2 ** 20)],
                        60), results_cap_bytes=4 * 2 ** 20)
        code, output, _ = cloud.run(plan)
        receipt = cloud.receipt()
        worker = receipt['worker']
        check(code == 3 and output['status'] == 'failed_remote' and worker['status'] == 'overflow' and
              worker['overflow_files'] == '1' and worker['truncated_streams'] == '1',
              'debordement publie comme conforme : %r' % worker)
        summary = receipt['remote_summary']
        check(summary['overflow_files'] == '1' and summary['truncated_streams'] == '1' and
              any('big.bin' in line for line in summary['overflow']['evicted']) and
              any('000_noisy/stdout' in line for line in summary['overflow']['truncated_streams']),
              'debordement absent du resume : %r' % summary)
        files = cloud.results() / 'cmd' / '001_files' / 'files'
        check((files / 'small.txt').is_file() and not (files / 'big.bin').exists(), 'petit fichier evince')
        stdout = (cloud.results() / 'cmd' / '000_noisy' / 'stdout').read_bytes()
        check(len(stdout) <= 2 ** 20 + 200 and b'octets tronques' in stdout, 'stdout non tronque')
        check(receipt['results_verified'] is True, 'resultats non verifies')
        self.assert_certified_stop(cloud, receipt)

    def test_worker_killed_without_exit_file_uses_salvage(self):
        cloud = self.cloud()
        process = cloud.popen(plan_of(sleeper(60)))
        wait_for(lambda: cloud.remote_dirs() and (cloud.remote_dirs()[0] / 'results' / 'cmd' / '000_sleeper' /
                                                   'argv.txt').exists(), 300, 'commande lancee', process)
        os.kill(int((cloud.remote_dirs()[0] / 'worker.pid').read_text()), signal.SIGKILL)
        out, err = process.communicate(timeout=600)
        receipt = cloud.receipt()
        check(process.returncode == 3 and receipt['worker_outcome'] == 'died_without_exit_file' and
              receipt['results_archive'] == 'salvage' and 'unverified_worker' in receipt and
              not receipt['results_verified'], 'worker mort mal traite : %s' % out)
        self.assert_certified_stop(cloud, receipt)

    def test_salvage_expansion_is_bounded_before_extraction(self):
        cloud = self.cloud()
        plan = plan_of(('zeros', ['python3', '{src}/morsehgp3D_v10/python/zeros.py', '{out}', str(8 * 2 ** 20), '60'],
                        120), results_cap_bytes=4 * 2 ** 20)
        process = cloud.popen(plan)
        wait_for(lambda: cloud.remote_dirs() and (cloud.remote_dirs()[0] / 'results' / 'cmd' / '000_zeros' / 'files' /
                                                   'zeros.done').exists(), 300, 'fichier de zeros ecrit', process)
        os.kill(int((cloud.remote_dirs()[0] / 'worker.pid').read_text()), signal.SIGKILL)
        out, err = process.communicate(timeout=600)
        receipt = cloud.receipt()
        check(process.returncode == 3 and receipt['results_archive'] == 'salvage' and
              receipt['results_bytes'] < 2 ** 20 and receipt['results_expanded_bytes'] >= 8 * 2 ** 20 and
              any('extraction refusee' in error for error in receipt['errors']) and
              not (cloud.session / 'results' / 'extracted').exists(), 'decompression non bornee : %s' % out)
        self.assert_certified_stop(cloud, receipt)

    def test_overdue_worker_is_aborted_then_salvaged_before_nologin(self):
        cloud = self.cloud(kits=['tar_hang'], session_schedule_offset=1880)
        code, output, _ = cloud.run(plan_of(sleeper(5, 30)))
        receipt = cloud.receipt()
        check(code == 3 and receipt['worker_outcome'] == 'overdue' and receipt.get('worker_aborted') is True and
              receipt['results_archive'] == 'salvage' and not receipt['results_verified'],
              'branche overdue/abandon/secours : %r' % {k: receipt.get(k) for k in (
                  'worker_outcome', 'worker_aborted', 'results_archive', 'errors')})
        cutoff = receipt['ssh_cutoff_epoch']
        late = [row['name'] for row in receipt['host_commands']
                if row['name'] in ('abort_worker', 'results_hash', 'salvage', 'download', 'download_worker_log')
                and row['started_epoch'] > cutoff]
        check(not late, 'SSH/SCP apres D - 360 : %r' % late)
        self.assert_certified_stop(cloud, receipt)

    # -- generation : jamais d'arret sans preuve ni sur un demarrage etranger ---------------------

    def test_t1_concurrent_staging_is_never_stopped(self):
        cloud = self.cloud(concurrent_staging=True)
        code, output, _ = cloud.run(plan_of(HELLO), wait=False)
        check(code == 0 and output['status'] == 'launched', 'lanceur detache : %r' % output)
        wait_for(lambda: (cloud.session / 'DONE').exists(), 300, 'sentinelle DONE')
        receipt = cloud.receipt()
        check((cloud.session / 'DONE').read_text().strip() == '2' and receipt['status'] == 'failed_before_start' and
              receipt['closure'] == 'no_start_requested' and receipt['start_may_have_been_requested'] is False,
              'STAGING concurrent mal classe : %r' % receipt['closure'])
        self.assert_no_stop(cloud, receipt)
        check(cloud.state()['status'] == 'STAGING' and cloud.state().get('starts', 0) == 0, 'cible etrangere touchee')
        check(receipt['oslogin_key_removed'] is True, 'cle OS Login non retiree')

    def test_stockout_generation_unknown_is_uncertified_without_stop(self):
        cloud = self.cloud(start_stockout=True)
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 74 and receipt['closure'] == 'generation_unknown' and
              receipt['start_may_have_been_requested'] is True and 'NE JAMAIS' in receipt['recovery'] and
              'operations list' in receipt['recovery'], 'rupture de stock : %r' % output)
        self.assert_no_stop(cloud, receipt)
        check(cloud.state()['keys'] == {} and not (cloud.session / 'key').exists(), 'cle non retiree en 74')

    def test_start_failed_but_running_is_uncertified_without_stop(self):
        cloud = self.cloud(start_fails_but_runs=True)
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 74 and receipt['closure'] == 'generation_unknown' and
              receipt['observed_after']['status'] == 'RUNNING', 'start en echec mais VM demarree : %r' % output)
        self.assert_no_stop(cloud, receipt)

    def test_t4_preemption_during_start_is_certified_without_stop(self):
        cloud = self.cloud(preempt_countdown=2)
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 2 and receipt['targeted_shutdown_certified'] is True and receipt['start_certified'] is False and
              receipt['closure'] == 'already_terminated', 'preemption pendant le start : %r' % output)
        check(not cloud.rows(receipt, 'guarded_stop') and cloud.state()['status'] == 'TERMINATED', 'fermeture')

    def test_t5_foreign_generation_during_worker_is_not_stopped(self):
        cloud = self.cloud(flip_after_polls=2)
        code, output, _ = cloud.run(plan_of(sleeper(20)))
        receipt = cloud.receipt()
        check(code == 75 and receipt['status'] == 'foreign_generation_active' and
              receipt['closure'] == 'own_generation_ended_foreign_active' and 'NE PAS' in receipt['recovery'],
              'generation etrangere : %r' % output)
        self.assert_no_stop(cloud, receipt)
        foreign = Path(str(cloud.state_path) + '.foreign')
        sent = foreign.read_text().splitlines() if foreign.exists() else []
        check(len(sent) <= 1, 'SSH/SCP envoyes a la generation etrangere : %r' % sent)
        for name in ('abort_worker', 'results_hash', 'download', 'salvage'):
            check(not cloud.rows(receipt, name), name + ' vers la generation etrangere')

    def test_preemption_during_worker_stops_ssh(self):
        cloud = self.cloud(preempt_after_polls=2)
        code, output, _ = cloud.run(plan_of(sleeper(20)))
        receipt = cloud.receipt()
        check(code == 3 and receipt['worker_outcome'] == 'target_lost' and receipt['closure'] == 'already_terminated'
              and receipt['retrieval'] == 'skipped_target_lost', 'preemption pendant le worker : %r' % output)
        self.assert_certified_stop(cloud, receipt, gce_stops=0)

    def test_stale_generation_foreign_staging_after_preemption_is_not_stopped(self):
        cloud = self.cloud(preempt_after_polls=2, foreign_start_after_preempt=True)
        code, output, _ = cloud.run(plan_of(sleeper(20)))
        receipt = cloud.receipt()
        check(code == 75 and receipt['closure'] == 'start_in_flight_same_generation' and
              receipt['observed_before_stop']['status'] == 'STAGING', 'demarrage etranger arrete : %r' % output)
        self.assert_no_stop(cloud, receipt)
        check(cloud.state()['status'] == 'STAGING', 'le demarrage etranger a ete touche')

    def test_recover_stale_generation_never_stops_a_foreign_start(self):
        cloud = self.cloud(status='STAGING')
        cloud.set_state(last_start=OLD_GENERATION)
        prepared_session(cloud, OLD_GENERATION)
        code, output, err = cloud.recover(wait=2)
        check(code == 75 and output['closure'] == 'start_in_flight_same_generation' and
              ('compute', 'instances', 'stop') not in cloud.verbs() and cloud.state()['status'] == 'STAGING',
              'reprise sur generation perimee : %s %s' % (output, err[-500:]))
        done = self.cloud('termine', status='TERMINATED')
        done.set_state(last_start=OLD_GENERATION)
        prepared_session(done, OLD_GENERATION)
        code, output, _ = done.recover(wait=2)
        check(code == 0 and output['closure'] == 'already_terminated' and
              ('compute', 'instances', 'stop') not in done.verbs(), 'deja arrete : %r' % output)
        absent = self.cloud('absent')
        code, output, _ = absent.recover(wait=1)
        check(code == 2 and output['status'] == 'session_absente' and absent.calls() == [],
              'session absente : %r' % output)

    # -- signaux, console morte, controleur tue, disque plein ------------------------------------

    def test_interrupted_session_stops_before_retrieval(self):
        cloud = self.cloud()
        process = cloud.popen(plan_of(sleeper(60)))
        pid = cloud.child_pid()
        wait_for(lambda: cloud.remote_dirs() and (cloud.remote_dirs()[0] / 'results' / 'cmd' / '000_sleeper' /
                                                   'argv.txt').exists(), 300, 'commande lancee', process)
        os.kill(pid, signal.SIGTERM)
        started = time.monotonic()
        out, err = process.communicate(timeout=600)
        receipt = cloud.receipt()
        check(process.returncode == 3 and receipt['retrieval'] == 'skipped_host_interrupted' and
              any('signal hote' in e for e in receipt['errors']), 'interruption : %s' % out)
        check(time.monotonic() - started < 60, 'arret trop tardif apres l interruption')
        self.assert_certified_stop(cloud, receipt)

    def test_t3_sigterm_during_start_is_deferred(self):
        cloud = self.cloud(start_sleep=4)
        process = cloud.popen(plan_of(HELLO))
        pid = cloud.child_pid()
        wait_for(lambda: ('compute', 'instances', 'start') in cloud.verbs(), 120, 'instances start', process)
        os.kill(pid, signal.SIGTERM)
        out, err = process.communicate(timeout=600)
        receipt = cloud.receipt()
        start_err = next((cloud.session / 'host' / 'logs').glob('*_guarded_start.stderr')).read_text()
        check(process.returncode == 3 and receipt['start_certified'] is True and 'URGENCE' not in start_err,
              'start signale : %s' % start_err[-500:])
        check((cloud.dir / 'scheduled').exists(), 'garde invitee jamais armee')
        self.assert_certified_stop(cloud, receipt)

    def test_t6_controller_killed_during_start_then_recover(self):
        cloud = self.cloud(start_sleep=4)
        stale = cloud.run_dir()
        stale.mkdir(mode=0o700, parents=True, exist_ok=True)
        (stale / 'DONE').write_text('0\n')   # sentinelle perimee d'une session precedente au meme chemin
        process = cloud.popen(plan_of(HELLO))
        pid = cloud.child_pid()
        wait_for(lambda: ('compute', 'instances', 'start') in cloud.verbs(), 120, 'instances start', process)
        os.kill(pid, signal.SIGKILL)
        out, err = process.communicate(timeout=120)
        check(process.returncode == 70 and 'child_died_without_sentinel' in out and 'NE RIEN relancer' in err,
              'lanceur : %s %s' % (out, err[-300:]))
        marks = cloud.session / 'host' / 'guardmarks' / 'double_guard_verified'
        wait_for(marks.exists, 120, 'start_and_verify orphelin termine')
        check((cloud.dir / 'scheduled').exists() and cloud.state()['status'] == 'RUNNING',
              'start orphelin tue par SIGPIPE (garde invitee non armee)')
        code, output, err = cloud.recover(wait=60)
        check(code == 0 and cloud.state()['status'] == 'TERMINATED' and output['closure'] == 'stopped' and
              cloud.verbs().count(('compute', 'instances', 'stop')) == 1, 'reprise : %s %s' % (output, err[-500:]))
        recovery = json.loads(next(cloud.session.glob('recovery_*.json')).read_text())
        check(recovery['closing_generation'] == cloud.state()['last_start'] and
              recovery['targeted_shutdown_certified'] is True and recovery['observed_before_stop']['status'] ==
              'RUNNING', 'recu de reprise')

    def test_r4_recover_refuses_while_orphan_start_is_alive(self):
        cloud = self.cloud(profile_sleep=8)
        process = cloud.popen(plan_of(HELLO))
        pid = cloud.child_pid()
        wait_for(lambda: ('compute', 'os-login', 'describe-profile') in cloud.verbs(), 120, 'describe-profile',
                 process)
        os.kill(pid, signal.SIGKILL)
        out, err = process.communicate(timeout=120)
        check(process.returncode == 70, 'lanceur : %s' % out)
        check(not (cloud.session / 'host' / 'lifecycle.txt').exists(), 'fixture : cycle de vie deja ecrit')
        code, output, err = cloud.recover(wait=2)
        check(code == 76 and output['status'] == 'session_alive' and
              any('start_and_verify.sh' in p['cmdline'] for p in output['processes']) and
              ('compute', 'instances', 'stop') not in cloud.verbs(),
              '--recover a conclu pendant un start orphelin : %s %s' % (output, err[-500:]))
        marks = cloud.session / 'host' / 'guardmarks' / 'double_guard_verified'
        wait_for(marks.exists, 180, 'start_and_verify orphelin termine')
        check(cloud.state()['status'] == 'RUNNING' and cloud.state().get('starts') == 1, 'orphelin : VM demarree')
        code, output, err = cloud.recover(wait=60)
        check(code == 0 and output['closure'] == 'stopped' and cloud.state()['status'] == 'TERMINATED' and
              cloud.verbs().count(('compute', 'instances', 'stop')) == 1, 'reprise apres l orphelin : %s' % output)

    def _dead_console_child(self, cloud, plan, sig):
        cloud.session.mkdir(mode=0o700)
        read_end, write_end = os.pipe()
        reader = subprocess.Popen(['cat'], stdin=read_end, stdout=subprocess.DEVNULL)
        os.close(read_end)
        process = subprocess.Popen(cloud.argv(plan, child=True), env=cloud.env(), stdout=write_end,
                                   stderr=write_end, stdin=subprocess.DEVNULL)
        os.close(write_end)
        wait_for(lambda: cloud.remote_dirs() and (cloud.remote_dirs()[0] / 'results' / 'cmd' / '000_sleeper' /
                                                   'argv.txt').exists(), 300, 'commande lancee', process)
        reader.kill()
        reader.wait()
        process.send_signal(sig)
        process.wait(timeout=600)
        return process.returncode

    def test_dead_console_then_sigint_still_stops(self):
        cloud = self.cloud()
        code = self._dead_console_child(cloud, plan_of(sleeper(60)), signal.SIGINT)
        receipt = cloud.receipt()
        check(code == 3 and receipt['console_write_failures'] > 0, 'console morte : code %s' % code)
        self.assert_certified_stop(cloud, receipt)
        check((cloud.session / 'DONE').read_text().strip() == '3', 'sentinelle DONE')

    def test_dead_console_stop_failure_returns_74(self):
        cloud = self.cloud(stop_fails=True)
        code = self._dead_console_child(cloud, plan_of(sleeper(60)), signal.SIGHUP)
        receipt = cloud.receipt()
        check(code == 74 and receipt['status'] == 'shutdown_uncertified' and cloud.rows(receipt, 'guarded_stop'),
              'code 74 perdu avec une console morte : %s' % code)

    def _full_disk_verdict(self, cloud):
        if not unshare_available():
            self.skipTest('unshare -Urm indisponible : fixture tmpfs plein non executable ici')
        request = {'base': str(BASE), 'repo': str(REPO), 'commit': COMMIT, 'data': str(DATA), 'cloud': str(cloud.dir),
                   'plan': plan_of(sleeper(30))}
        result = subprocess.run(['unshare', '-Urm', sys.executable, str(Path(__file__).resolve()), '--namespace-run',
                                 json.dumps(request)], capture_output=True, text=True, timeout=900,
                                env=dict(os.environ, TMPDIR=str(BASE / 'tmp')))
        check(result.returncode == 0, 'execution sous unshare : %s' % result.stderr[-2000:])
        verdict = json.loads(result.stdout)
        state = verdict['state']
        check((state.get('session_filled') or state.get('run_dir_filled')) and state.get('stops') == 1 and
              state['status'] == 'TERMINATED',
              'disque plein : pas d\'instances stop : %r' % {k: state.get(k) for k in (
                  'session_filled', 'run_dir_filled', 'stops')})
        return verdict, verdict['receipt'] or {}

    def test_session_disk_full_before_stop(self):
        cloud = self.cloud(fill_session_after_polls=2, fill_kib=16384)
        verdict, receipt = self._full_disk_verdict(cloud)
        check(verdict['code'] == 3 and verdict['done'] == '3' and verdict['done_place'] == 'session' and
              verdict['receipt_place'] == 'session' and receipt.get('targeted_shutdown_certified') is True
              and receipt.get('worker_outcome') == 'local_disk_low' and
              receipt.get('retrieval') == 'skipped_local_disk_low' and receipt.get('reserve_released') is True,
              'disque plein : %r' % {k: verdict.get(k) for k in ('code', 'done', 'done_place', 'df_free',
                                                                'stderr_tail')})

    def test_session_disk_refilled_after_reserve_release(self):
        cloud = self.cloud(fill_session_after_polls=2, fill_kib=16384, refill_after_reserve_release=True)
        verdict, receipt = self._full_disk_verdict(cloud)
        check(verdict['state'].get('session_refilled') and verdict['code'] == 3 and verdict['done'] == '3' and
              verdict['receipt_place'] == 'run_dir' and receipt.get('receipt_path', '').startswith(str(BASE / 'tmp'))
              and receipt.get('targeted_shutdown_certified') is True and receipt.get('closure') == 'stopped',
              'disque de nouveau plein a l\'arret : %r' % {k: verdict.get(k) for k in (
                  'code', 'done', 'done_place', 'receipt_place', 'stderr_tail')})

    def test_done_unwritable_propagates_the_real_code(self):
        cloud = self.cloud()
        cloud.set_modes(readonly_on_stop=[str(cloud.session), str(cloud.run_dir())])
        code, output, err = cloud.run(plan_of(HELLO))
        check(code == 0 and output['done_missing'] is True and output['exit_code'] == 0 and
              cloud.state()['stops'] == 1, 'code reel non propage sans DONE : %s %s' % (output, err[-300:]))

    def test_second_session_is_refused_while_one_runs(self):
        cloud = self.cloud()
        process = cloud.popen(plan_of(sleeper(20)))
        cloud.child_pid()
        wait_for(lambda: cloud.remote_dirs(), 300, 'premiere session lancee', process)
        argv = cloud.argv(plan_of(HELLO))
        argv[argv.index('--session-dir') + 1] = str(cloud.sessions / 's2')
        env = dict(cloud.env(), FAKE_GCP_LOG=str(cloud.dir / 'second_session.log'))
        second = subprocess.run(argv, env=env, capture_output=True, text=True, timeout=300)
        output = json.loads(second.stdout)
        check(second.returncode == 2 and 'autre session v10' in output['reason'] and
              not (cloud.sessions / 's2').exists(), 'seconde session : %r' % output)
        check(not (cloud.dir / 'second_session.log').exists(), 'la seconde session a appele gcloud')
        out, err = process.communicate(timeout=600)
        check(process.returncode == 0, 'premiere session : %s' % out)

    # -- revue adverse 3 ----------------------------------------------------------------------------

    def test_running_stale_generation_after_a_stop_is_never_stopped(self):
        cloud = self.cloud(preempt_after_polls=2, foreign_running_after_preempt=True)
        code, output, _ = cloud.run(plan_of(sleeper(20)))
        receipt = cloud.receipt()
        check(code == 75 and receipt['closure'] == 'stopped_after_generation_foreign_active' and
              receipt['observed_before_stop']['status'] == 'RUNNING' and
              receipt['observed_before_stop']['lastStartTimestamp'] == receipt['closing_generation'] and
              (receipt.get('target_lost') or {}).get('observed_by') == 'recertify_poll' and
              'NE PAS' in receipt['recovery'], 'RUNNING perime arrete : %r' % output)
        self.assert_no_stop(cloud, receipt)
        check(cloud.state()['status'] == 'RUNNING', 'la VM etrangere a ete touchee')
        other = self.cloud('reprise', status='RUNNING')
        other.set_state(last_start=OLD_GENERATION, last_stop='2026-09-28T09:30:00.000000-07:00')
        prepared_session(other, OLD_GENERATION)
        code, output, err = other.recover(wait=2)
        check(code == 75 and output['closure'] == 'stopped_after_generation_foreign_active' and
              ('compute', 'instances', 'stop') not in other.verbs() and other.state()['status'] == 'RUNNING',
              'reprise sur RUNNING perime : %s %s' % (output, err[-300:]))

    def test_tmpdir_full_still_stops(self):
        cloud = self.cloud(fill_run_dir_after_polls=2, fill_kib=8192, refill_after_reserve_release=True)
        verdict, receipt = self._full_disk_verdict(cloud)
        stops = [row for row in receipt.get('host_commands', []) if row.get('name') == 'guarded_stop']
        check(verdict['code'] == 3 and verdict['done'] == '3' and verdict['receipt_place'] == 'session' and
              verdict['state'].get('run_dir_filled') and verdict['state'].get('run_dir_refilled') and
              receipt.get('targeted_shutdown_certified') is True and receipt.get('closure') == 'stopped' and
              receipt.get('worker_outcome') == 'local_disk_low' and len(stops) == 1 and
              stops[0].get('output_via_pipes') is True and stops[0].get('exit_code') == 0,
              'dossier d\'execution plein : %r' % {k: verdict.get(k) for k in ('code', 'done', 'receipt_place',
                                                                              'stderr_tail')})

    def test_recover_ignores_observers_but_not_the_session(self):
        cloud = self.cloud()
        process = cloud.popen(plan_of(sleeper(60)))
        pid = cloud.child_pid()
        wait_for(lambda: cloud.remote_dirs() and (cloud.remote_dirs()[0] / 'results' / 'cmd' / '000_sleeper' /
                                                   'argv.txt').exists(), 300, 'commande lancee', process)
        code, output, _ = cloud.recover(wait=1)
        check(code == 76 and any(p['pid'] == pid for p in output['processes']),
              'processus de session vivant ignore : %r' % output)
        os.kill(pid, signal.SIGKILL)
        out, _ = process.communicate(timeout=120)
        check(process.returncode == 70, 'lanceur : %s' % out)
        quiet = {'stdin': subprocess.DEVNULL, 'stdout': subprocess.DEVNULL, 'stderr': subprocess.DEVNULL}
        observers = [
            subprocess.Popen(['tail', '-f', str(cloud.session / 'session.stderr')], **quiet),
            subprocess.Popen(['bash', '-c', 'until [ -f "$1/DONE" ]; do sleep 1; done', 'attente', str(cloud.session)],
                             **quiet),
            subprocess.Popen(['bash', '-c', 'while :; do sleep 1; done', 'sonde',
                              '--ssh-key-file=%s/key' % cloud.session], **quiet)]
        try:
            time.sleep(0.5)
            code, output, err = cloud.recover(wait=2)
        finally:
            for observer in observers:
                observer.kill()
                observer.wait()
        ignored = {p['pid'] for p in (output or {}).get('ignored_processes') or []}
        check(code == 0 and output['closure'] == 'stopped' and all(o.pid in ignored for o in observers) and
              cloud.verbs().count(('compute', 'instances', 'stop')) == 1 and cloud.state()['status'] == 'TERMINATED',
              'observateurs bloquants : %s %s' % (output, err[-300:]))

    def test_describe_hiccups_are_retried(self):
        for label, trigger, failures in (('sondage', 'launch', 6), ('fermeture', 'results_download', 4)):
            cloud = self.cloud(label, json_failures_after=trigger, json_failures=failures)
            code, output, _ = cloud.run(plan_of(sleeper(8)))
            receipt = cloud.receipt()
            state = cloud.state()
            check(code == 0 and receipt['closure'] == 'stopped' and state.get('json_failures_armed') and
                  state.get('json_failures_left') == 0, 'hoquet d\'API (%s) : %r' % (label, output))
            self.assert_certified_stop(cloud, receipt)

    def test_pip_is_isolated_from_a_broken_home_local(self):
        cloud = self.cloud(python_mode='installable')
        broken = cloud.dir / 'remote_home' / '.local'
        broken.mkdir()
        (broken / '.fake_broken').write_text('installation pip interrompue\n')
        code, output, _ = cloud.run(plan_of(CHECK))
        receipt = cloud.receipt()
        worker = receipt.get('worker') or {}
        base = worker.get('python_user_base', '')
        check(code == 0 and worker.get('pip') == 'installed' and base.endswith('/pyuser') and
              base.startswith(str(cloud.dir / 'remote_home' / 'ehgp-v10') + '/'),
              'pip dependant de ~/.local : %r' % worker)
        self.assert_certified_stop(cloud, receipt)

    def test_gcloud_config_drift_does_not_break_stop(self):
        cloud = self.cloud(active_project_drifts=True)
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 0 and receipt['status'] == 'completed', 'derive de configuration gcloud : %r' % output)
        self.assert_certified_stop(cloud, receipt)

    def test_oslogin_add_applied_then_failed_is_removed_without_stop(self):
        cloud = self.cloud(oslogin_add_applied_then_fails=True)
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 2 and receipt['closure'] == 'no_start_requested' and receipt['gcp_mutation_phase_entered'] and
              receipt['oslogin_key_removed'] is True and cloud.state()['keys'] == {}, 'echec OS Login : %r' % output)
        self.assert_no_stop(cloud, receipt)

    def test_stop_failure_is_shutdown_uncertified(self):
        cloud = self.cloud(stop_fails=True)
        code, output, _ = cloud.run(plan_of(HELLO))
        receipt = cloud.receipt()
        check(code == 74 and output['status'] == 'shutdown_uncertified' and
              receipt['recovery_command'].endswith('--expected-last-start-timestamp ' + receipt['closing_generation'])
              and 'GCP_INSTANCE_NAME=ehgp-v7-4fa0e0789a7d5bb06b787d35' in receipt['recovery_command']
              and 'SEULEMENT si' in receipt['recovery'], 'arret en echec accepte : %r' % output)
        check(receipt['targeted_shutdown_certified'] is False and cloud.state()['keys'] == {},
              'arret declare certifie ou cle OS Login laissee')

    def test_terminated_is_required_after_stop(self):
        cloud = self.cloud(json_status_after_stop='RUNNING')
        code, output, _ = cloud.run(plan_of(HELLO))
        check(code == 74 and output['status'] == 'shutdown_uncertified', 'relecture non TERMINATED acceptee : %r' % output)


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--namespace-run':
        print(json.dumps(namespace_run(json.loads(sys.argv[2]))))
        sys.exit(0)
    program = unittest.main(exit=False, verbosity=2)
    sys.exit(0 if program.result.wasSuccessful() else 1)
