#!/usr/bin/env python3
"""Session G4 SPOT gardee et minimale pour morsehgp3D_v10 (reference_cpu). Inerte sans --execute.

  python3 gcp-migration/v10_session.py --commit SHA --plan PLAN.json --data DOSSIER \
      --session-dir /workspaces/.ehgp-sessions/NOM --max-run-seconds S [--execute [--wait]]
  python3 gcp-migration/v10_session.py --recover --session-dir /workspaces/.ehgp-sessions/NOM

Sans --execute : validation complete (commit pousse, protocole committe et epingle, plan, donnees,
tailles, espace disque, budget, cible TERMINATED, OS Login et maxRunDuration par lectures GCP seules)
et impression de ce qui serait fait. Aucune commande GCP mutante, aucun dossier de session ; seul
`git fetch origin` met a jour refs/remotes/origin et FETCH_HEAD.

Avec --execute : la meme validation, puis creation du dossier de session et lancement DETACHE
(nouvelle session Unix, stdout/stderr vers des fichiers, stdin /dev/null) du processus de session ;
le lanceur rend la main aussitot (ou attend la sentinelle DONE avec --wait). C'est l'unique facon
de lancer.

Cycle de vie (repris de tower_session_v9.py, sans exec de modules v7/v9) : cle ed25519 de session
inscrite dans OS Login avec une expiration bornee -> start_and_verify.sh du commit (epingle, jamais
signale) -> recertification RUNNING + generation -> televersement du paquet puis des donnees (sha256
des deux cotes) -> worker detache v10_worker.sh, sonde avec recertification de la cible a chaque
cycle -> rapatriement borne AVANT /run/nologin (arret invite - 360 s) -> fermeture par la generation
PROUVEE : describe d'abord (retente avec un repli progressif), stop_and_verify.sh seulement sur
RUNNING/SUSPENDING de cette generation sans arret posterieur (lastStopTimestamp) ou sur STOPPING,
jamais sur un demarrage etranger ni sur une generation devinee -> relecture TERMINATED
-> retrait de la cle OS Login -> extraction bornee et verification locales.

Disque : les journaux bavards de toutes les commandes hote (scripts gardes compris) vont dans un
dossier d'execution JETABLE sous $TMPDIR (autre systeme de fichiers) ; le dossier de session garde
recu, RECOVERY.txt, handoff, cycle de vie, marques et DONE, et une reserve preallouee y est liberee
juste avant l'arret pour que ces ecritures passent meme disque plein. Dossier d'execution absent ou
presque plein : scripts gardes et commandes critiques passent par des tubes.

Statuts : completed 0 | failed_before_start 2 | failed_remote 3 | shutdown_uncertified 74 |
foreign_generation_active 75 (notre generation est terminee ou un demarrage etranger est en vol :
NE RIEN arreter) ; --recover rend aussi 76 si le processus de session ou un script garde de la
session vit encore, et 2 si le dossier de session n'existe pas.
public_status=not_claimed : une mesure ne certifie rien.
"""
import argparse
import fcntl
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import signal
import stat
import subprocess
import sys
import tarfile
import tempfile
import time
from datetime import datetime, timezone

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
TARGET = {'project': 'devpod-gpu-exploration', 'zone': 'us-central1-b',
          'instance': 'ehgp-v7-4fa0e0789a7d5bb06b787d35'}
MACHINE_TYPE = 'g4-standard-48'
DEFAULT_GCLOUD = '/home/codespace/google-cloud-sdk/bin/gcloud'
DEFAULT_SESSIONS_ROOT = '/workspaces/.ehgp-sessions'
SOURCE_ROOT = 'morsehgp3D_v10'
CONTROLLER = 'gcp-migration/v10_session.py'
WORKER = 'gcp-migration/v10_worker.sh'
START_GUARD = 'gcp-migration/start_and_verify.sh'
STOP_GUARD = 'gcp-migration/stop_and_verify.sh'
# Scripts gardes epingles (memes epingles que full_probe_session_v7.py / tower_session_v9.py) :
# toute modification au commit exige une revue et une nouvelle epingle ici.
GUARD_PINS = {START_GUARD: '73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0',
              STOP_GUARD: 'ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c'}
PLAN_SCHEMA = 'ehgp.v10.session_plan.v1'
WORKER_PLAN_MAGIC = '# ehgp.v10.worker_plan.v2'
WORKER_RESULT_SCHEMA = 'ehgp.v10.worker_result.v3'
RECEIPT_SCHEMA = 'ehgp.v10.session_receipt.v3'
PYTHON_PINS = {'numpy': '2.2.6', 'scipy': '1.15.3', 'scikit-learn': '1.7.2', 'hdbscan': '0.8.44'}
COMPARABILITY = ('VM : g++ 11.4, CMake 3.22.1, Python 3.10 et les paquets epingles ; local : g++ 13.3, '
                 'Python 3.12, numpy/scipy/scikit-learn plus recents. Temps et versions NON comparables '
                 'entre la VM et le codespace.')

MIN_RUN_SECONDS, MAX_RUN_SECONDS = 30, 28800
GCE_GUARD_OVERHEAD = 900       # 300 (reserve GCE) + 120 (systemd) + 480 (armement) : start_and_verify.sh
MIN_GUEST_MINUTES = 5
NOLOGIN_LEAD = 300             # shutdown -P cree /run/nologin 5 min avant l'arret invite D (pam_nologin)
SSH_CUTOFF = NOLOGIN_LEAD + 60  # aucun SSH/SCP ne part apres D - 360
RETRIEVE_BUDGET = 240          # rapatriement complet (recertifications comprises), borne par D - SSH_CUTOFF
HOST_WAIT_GRACE = 60           # attente au-dela de l'echeance du worker
CLOSING_RESERVE = SSH_CUTOFF + RETRIEVE_BUDGET + HOST_WAIT_GRACE   # echeance du worker = D - 660
WORKER_PACK_RESERVE = 120      # identique a PACK_RESERVE_SECONDS de v10_worker.sh
ABORT_GRACE_MAX = 90           # attente de worker.exit apres SIGTERM
MIN_SALVAGE_WINDOW = 100       # temps garde pour l'archive de secours et le telechargement
SETUP_ESTIMATE = 180
UPLOAD_RATE = 10 * 2 ** 20     # estimation (octets/s) pour le budget
UPLOAD_TIMEOUT_RATE = 2 * 2 ** 20
PIP_TIMEOUT = 600              # identique a PIP_TIMEOUT_SECONDS de v10_worker.sh
DEFAULT_BUILD_TIMEOUT = 600
MAX_RESULTS_BYTES = 2 ** 30    # plafond par defaut et maximum (plan : results_cap_bytes)
MIN_RESULTS_BYTES = 2 ** 20
MAX_DATA_FILES = 512
MAX_DATA_BYTES = 8 * 2 ** 30
MAX_PACKAGE_BYTES = 256 * 2 ** 20
LOCAL_FREE_MARGIN = 2 ** 30
RUN_DIR_FREE_MIN = 256 * 2 ** 20
REMOTE_FREE_MARGIN = 2 * 2 ** 30
RESERVE_BYTES = 8 * 2 ** 20    # reserve preallouee du dossier de session, liberee juste avant l'arret
SESSION_FREE_FLOOR = 64 * 2 ** 20  # sous ce seuil pendant le sondage : fermeture anticipee
RUN_DIR_FLOOR = 32 * 2 ** 20   # idem pour le dossier d'execution ($TMPDIR)
RUN_DIR_CRITICAL_FREE = 16 * 2 ** 20  # sous ce seuil libre, scripts gardes et commandes critiques : tubes
RUN_DIR_RESERVE_BYTES = 4 * 2 ** 20   # reserve du dossier d'execution, liberee juste avant l'arret
PATIENCE_DELAYS = (5, 10, 20, 30, 30, 30, 30, 30, 30, 30)   # relectures illisibles : repli progressif
CLOSE_PATIENCE = 270           # fermeture : 11 relectures sur ~4 min (245 s d'attente) avant describe_failed
POLL_PATIENCE = 200            # sondage : 9 relectures sur ~3 min (185 s) avant target_lost, jamais apres `until`
START_TIMEOUT = 2400           # pire cas mesure de start_and_verify.sh ~1870 s ; jamais signale avant
STOP_TIMEOUT = 1200
GUARD_GRACE = 900
RECOVER_WAIT = 60
EXIT_CODES = {'completed': 0, 'failed_before_start': 2, 'failed_remote': 3, 'shutdown_uncertified': 74,
              'foreign_generation_active': 75}
SESSION_ALIVE_CODE = 76
CHILD_DIED_CODE = 70
LAUNCH_FILES = {'launch.json', 'session.stdout', 'session.stderr', 'session.lock'}
LIFECYCLE_STATES = ('start_may_have_been_requested', 'targeted_running', 'targeted_stopping', 'targeted_stopped',
                    'targeted_stop_failed')
FOREIGN_CLOSURES = ('own_generation_ended_foreign_active', 'start_in_flight_same_generation',
                    'stopped_after_generation_foreign_active')
CTEST_FORBIDDEN = ('-S', '--script', '-D', '--dashboard', '--build-', '-T', '--test-action', '-M', '--test-model',
                   '--test-dir', '--extra-submit', '--submit', '-Q', '--quiet', '--repeat', '-N', '--show-only')

NAME_RE = re.compile(r'[a-z0-9][a-z0-9_.-]{0,63}')
BINARY_RE = re.compile(r'\./(mhgp10_[A-Za-z0-9_]+)')
PYTHON_RE = re.compile(r'\{src\}/(' + SOURCE_ROOT + r'/[A-Za-z0-9_./-]+\.py)')
DATA_REF_RE = re.compile(r'\{data\}/([A-Za-z0-9._-]*)')
DATA_NAME_RE = re.compile(r'[A-Za-z0-9][A-Za-z0-9._-]{0,127}')
EXECUTABLE_RE = re.compile(r'add_executable\(\s*(mhgp10_[A-Za-z0-9_]+)')
REMOTE_DIR_RE = re.compile(r'/[A-Za-z0-9._/-]*/ehgp-v10/ehgp-v10\.[A-Za-z0-9]{10}')
SHA_RE = re.compile(r'[0-9a-f]{64}')


class Refusal(Exception):
    """Refus explicite ; le message dit pourquoi."""


class Interrupted(BaseException):
    """Signal recu dans une section interruptible."""


def need(ok, reason):
    if not ok:
        raise Refusal(reason)


def sha_bytes(raw):
    return hashlib.sha256(raw).hexdigest()


def sha_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def strict_json(raw):
    def pairs(items):
        result = {}
        for key, value in items:
            need(key not in result, 'cle JSON dupliquee : ' + key)
            result[key] = value
        return result

    def constant(name):
        raise Refusal('constante JSON interdite : ' + name)
    try:
        return json.loads(raw, object_pairs_hook=pairs, parse_constant=constant)
    except (ValueError, UnicodeDecodeError) as error:
        raise Refusal('JSON invalide : ' + str(error)) from error


def fields(text):
    result = {}
    for line in text.splitlines():
        key, separator, value = line.partition('=')
        need(separator == '=' and key and key not in result, 'ligne cle=valeur invalide : ' + line[:80])
        result[key] = value
    return result


def epoch(value):
    need(type(value) is str and re.fullmatch(
        r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,6})?(?:Z|[+-]\d\d:\d\d)', value), 'horodatage RFC 3339 attendu')
    return datetime.fromisoformat(value.replace('Z', '+00:00')).timestamp()


def write_new(path, data, mode=0o600):
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, mode)
    with os.fdopen(descriptor, 'wb') as stream:
        stream.write(data if isinstance(data, bytes) else data.encode())


def write_atomic(path, data, mode=0o600):
    temporary = Path(str(path) + '.partial')
    try:
        descriptor = os.open(temporary, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, mode)
        with os.fdopen(descriptor, 'wb') as stream:
            stream.write(data if isinstance(data, bytes) else data.encode())
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    except OSError:
        try:
            temporary.unlink()
        except OSError:
            pass
        raise


def json_bytes(value):
    return (json.dumps(value, sort_keys=True, indent=2) + '\n').encode()


def utc_now():
    return datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')


def run_directory(session):
    """Dossier d'execution JETABLE (journaux bavards) : sous $TMPDIR, propre a la session."""
    session = Path(session).absolute()
    return Path(tempfile.gettempdir()) / 'ehgp-v10-runs' / ('%s-%s' % (session.name,
                                                                          sha_bytes(str(session).encode())[:12]))


def make_run_directory(session):
    run_dir = run_directory(session)
    run_dir.parent.mkdir(mode=0o700, exist_ok=True)
    run_dir.mkdir(mode=0o700, exist_ok=True)
    os.chmod(run_dir, 0o700)
    return run_dir


def write_with_fallback(primary, fallback, data):
    """Ecrit dans le dossier de session ; a defaut (ENOSPC...), dans le dossier d'execution."""
    try:
        write_atomic(primary, data)
        return str(primary)
    except OSError:
        try:
            write_atomic(fallback, data)
            return str(fallback)
        except OSError:
            return None


class Console:
    """Journal humain : fichier et stderr ; ne leve JAMAIS (tube mort, EIO, ENOSPC).

    Un flux mort est redirige vers /dev/null des le premier echec : sinon Python retenterait de le
    vider a l'arret et rendrait le code 120 a la place du statut (74 compris)."""

    def __init__(self):
        self.path, self.failures = None, 0

    def _write(self, stream, text):
        try:
            stream.write(text)
            stream.flush()
        except (OSError, ValueError):
            self.failures += 1
            try:
                descriptor = os.open(os.devnull, os.O_WRONLY)
                os.dup2(descriptor, stream.fileno())
                os.close(descriptor)
            except (OSError, ValueError):
                pass

    def log(self, message):
        line = '[v10 %s] %s\n' % (utc_now(), message)
        if self.path is not None:
            try:
                with open(self.path, 'a') as stream:
                    stream.write(line)
            except (OSError, ValueError):
                self.failures += 1
        self._write(sys.stderr, line)

    def emit(self, value):
        self._write(sys.stdout, json.dumps(value, sort_keys=True, indent=2) + '\n')


CONSOLE = Console()
log = CONSOLE.log


class Signals:
    """SIGINT/SIGTERM/SIGHUP enregistres ; Interrupted n'est leve que dans une section interruptible.

    Jamais SIG_IGN : les enfants heritent donc de SIG_DFL, et aucun signal n'est perdu."""

    def __init__(self):
        self.received, self.interruptible, self.raised, self.previous = [], False, False, {}

    def install(self):
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            self.previous[sig] = signal.signal(sig, self._handler)

    def _handler(self, signum, _frame):
        self.received.append(signum)
        if self.interruptible and not self.raised:
            self.raised = True
            raise Interrupted('signal hote %d' % signum)

    def check(self):
        if self.received and not self.raised:
            self.raised = True
            raise Interrupted('signal hote %d (differe)' % self.received[0])


SIGNALS = Signals()


# ---------------------------------------------------------------------------------------------
# Commandes locales : groupe de processus propre, sorties vers des FICHIERS du dossier d'execution
# (jamais un tube du controleur, jamais le disque de session), copie miroir au mieux vers host/logs.

class Runner:
    def __init__(self, logdir, env, prefix='', mirror=None):
        self.logdir, self.env, self.prefix, self.mirror = Path(logdir), env, prefix, mirror
        self.rows, self.count = [], 0

    @staticmethod
    def _group_alive(group):
        try:
            os.killpg(group, 0)
            return True
        except ProcessLookupError:
            return False
        except PermissionError:
            return True

    def _mirror(self, stem):
        if self.mirror is None:
            return
        for suffix in ('.stdout', '.stderr', '.json'):
            try:
                shutil.copyfile(self.logdir / (stem + suffix), Path(self.mirror) / (stem + suffix))
            except OSError:
                pass

    def run(self, name, argv, timeout, guard=False, critical=False, quiet=False, interruptible=True):
        """critical (toujours vrai pour un script garde) : un echec d'ecriture de journal ne leve pas, et la
        sortie passe par des tubes si le dossier d'execution manque ou a moins de RUN_DIR_CRITICAL_FREE
        octets libres (un open() reussi suivi d'ENOSPC tuerait le script sur son premier printf) ; le code
        de retour reste exact."""
        critical = critical or guard
        argv = [str(item) for item in argv]
        self.count += 1
        stem = self.prefix + ('poll' if quiet else '%03d_%s' % (self.count, name))
        row = {'name': name, 'argv': argv, 'timeout_seconds': timeout, 'started_epoch': round(time.time(), 3)}
        if not quiet:
            self.rows.append(row)
        paths = (self.logdir / (stem + '.stdout'), self.logdir / (stem + '.stderr'))
        streams = []
        try:
            self.logdir.mkdir(mode=0o700, parents=True, exist_ok=True)   # jetable : recree s'il a disparu
            starved = shutil.disk_usage(self.logdir).free < RUN_DIR_CRITICAL_FREE
        except OSError:
            starved = True
        if critical and starved:
            row['output_via_pipes'] = True
        else:
            try:
                for path in paths:
                    streams.append(open(path, 'wb'))
            except OSError as error:
                for stream in streams:
                    stream.close()
                streams = []
                if not critical:
                    raise
                row['log_error'] = str(error)
        piped = not streams
        out, err = b'', b''
        saved = SIGNALS.interruptible
        SIGNALS.interruptible = saved and interruptible
        process = None
        try:
            process = subprocess.Popen(argv, stdin=subprocess.DEVNULL,
                                       stdout=subprocess.PIPE if piped else streams[0],
                                       stderr=subprocess.PIPE if piped else streams[1],
                                       env=self.env, start_new_session=True)
            row['pid'] = process.pid
            try:
                if piped:
                    out, err = process.communicate(timeout=timeout)
                else:
                    process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                row['timed_out'] = True
        finally:
            SIGNALS.interruptible = False
            try:
                if process is not None:
                    more_out, more_err = self._close(process, guard, row, piped)
                    out, err = out + more_out, err + more_err
                    row['exit_code'] = process.returncode
            finally:
                SIGNALS.interruptible = saved
                for stream in streams:
                    try:
                        stream.close()
                    except OSError:
                        pass
                row['elapsed_seconds'] = round(time.time() - row['started_epoch'], 3)
        if not piped:
            try:
                out, err = paths[0].read_bytes(), paths[1].read_bytes()
            except OSError as error:
                if not critical:
                    raise
                row['log_error'] = str(error)
        if not quiet:
            try:
                (self.logdir / (stem + '.json')).write_bytes(json_bytes(row))
            except OSError as error:
                if not critical:
                    raise
                row['log_error'] = str(error)
            self._mirror(stem)
        return process.returncode, out.decode(errors='replace'), err.decode(errors='replace')

    def _close(self, process, guard, row, piped):
        """Un script garde ne recoit SIGTERM (leader seul) qu'a expiration de son delai externe."""
        out, err = b'', b''
        if process.poll() is None or self._group_alive(process.pid):
            grace = GUARD_GRACE if guard else 10
            try:
                if guard:
                    if process.poll() is None:
                        process.send_signal(signal.SIGTERM)
                else:
                    os.killpg(process.pid, signal.SIGTERM)
                row['terminated'] = True
            except ProcessLookupError:
                pass
            end = time.monotonic() + grace
            try:
                if piped:
                    out, err = process.communicate(timeout=grace)
                else:
                    process.wait(timeout=grace)
            except subprocess.TimeoutExpired:
                pass
            while self._group_alive(process.pid) and time.monotonic() < end:
                time.sleep(0.05)
            if self._group_alive(process.pid):
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                row['group_killed'] = True
            if process.poll() is None:
                process.kill()
            if piped:
                rest_out, rest_err = process.communicate()
                out, err = out + rest_out, err + rest_err
        process.wait()
        return out, err


# ---------------------------------------------------------------------------------------------
# Validation locale (commune au dry-run, au lanceur et au processus de session).

def git(*args, binary=False, timeout=120, check=True):
    result = subprocess.run(['git', '-C', str(REPO), *args], stdin=subprocess.DEVNULL, capture_output=True,
                            timeout=timeout)
    if check and result.returncode != 0:
        raise Refusal('git ' + args[0] + ' a echoue : ' + result.stderr.decode(errors='replace').strip()[:300])
    return result if not check else (result.stdout if binary else result.stdout.decode())


def resolve_commit(commit, fetch):
    need(re.fullmatch(r'[0-9a-fA-F]{7,40}', commit or ''), '--commit doit etre un SHA hexadecimal de 7 a 40 caracteres')
    top = git('rev-parse', '--show-toplevel').strip()
    need(Path(top).resolve() == REPO, 'le controleur doit etre execute depuis gcp-migration/ du depot')
    if fetch:
        git('fetch', '--quiet', 'origin', timeout=300)
    result = git('rev-parse', '--verify', '--quiet', commit + '^{commit}', check=False)
    need(result.returncode == 0, 'commit inconnu localement : ' + commit)
    full = result.stdout.decode().strip()
    pushed = git('merge-base', '--is-ancestor', full, 'refs/remotes/origin/main', check=False)
    need(pushed.returncode == 0, 'commit ' + full + ' non pousse sur origin/main : refus (git push puis relancer)')
    return full


def commit_file(commit, name):
    result = git('cat-file', '-e', commit + ':' + name, check=False)
    need(result.returncode == 0, 'fichier absent du commit : ' + name)
    return git('show', commit + ':' + name, binary=True)


def tracked_tree(commit):
    """Fichiers suivis de morsehgp3D_v10 au commit ; refuse liens symboliques et sous-modules."""
    raw = git('ls-tree', '-r', '-z', '--full-tree', commit, '--', SOURCE_ROOT, WORKER, binary=True)
    tracked = set()
    for entry in raw.split(b'\0'):
        if not entry:
            continue
        meta, _, name = entry.decode().partition('\t')
        need(meta.split()[0] in ('100644', '100755'), 'entree non reguliere dans le paquet (lien ou sous-module) : ' + name)
        tracked.add(name)
    need(SOURCE_ROOT + '/CMakeLists.txt' in tracked, SOURCE_ROOT + '/CMakeLists.txt absent du commit')
    need(WORKER in tracked, WORKER + ' absent du commit')
    return tracked


def cmake_executables(commit, tracked):
    names = set()
    for name in tracked:
        if name.endswith('CMakeLists.txt') or name.endswith('.cmake'):
            names.update(EXECUTABLE_RE.findall(commit_file(commit, name).decode(errors='replace')))
    return names


def check_ctest(where, argv):
    """Anti vert par vacuite : --no-tests=error obligatoire JUSTE APRES ctest (jamais la valeur d'une
    autre option) ; modes script, tableau de bord, repetition, silence et listage interdits."""
    need(len(argv) >= 2 and argv[1] == '--no-tests=error' and argv.count('--no-tests=error') == 1 and
         all(arg == '--no-tests=error' for arg in argv if arg.startswith('--no-tests')),
         where + 'ctest exige --no-tests=error en premier argument (CTest 3.22 rend 0 sans aucun test)')
    for arg in argv[2:]:
        need(not any(arg.startswith(prefix) for prefix in CTEST_FORBIDDEN),
             where + 'option ctest interdite : ' + arg + ' (-S, -D, --build-and-test, -T, --test-dir, -Q, --repeat, -N...)')


def validate_plan(value, tracked, executables, data_names):
    need(type(value) is dict, 'le plan doit etre un objet JSON')
    allowed = {'schema', 'commands', 'needs_python', 'build_targets', 'build_timeout_seconds', 'results_cap_bytes',
               'note'}
    need(set(value) <= allowed, 'cles de plan inconnues : ' + ', '.join(sorted(set(value) - allowed)))
    need(value.get('schema') == PLAN_SCHEMA, 'schema de plan attendu : ' + PLAN_SCHEMA)
    declared_python = value.get('needs_python', False)
    need(type(declared_python) is bool, 'needs_python doit etre un booleen')
    build_timeout = value.get('build_timeout_seconds', DEFAULT_BUILD_TIMEOUT)
    need(type(build_timeout) is int and 60 <= build_timeout <= 7200, 'build_timeout_seconds entier entre 60 et 7200')
    cap = value.get('results_cap_bytes', MAX_RESULTS_BYTES)
    need(type(cap) is int and MIN_RESULTS_BYTES <= cap <= MAX_RESULTS_BYTES,
         'results_cap_bytes entier entre %d et %d' % (MIN_RESULTS_BYTES, MAX_RESULTS_BYTES))
    note = value.get('note', '')
    need(type(note) is str and len(note) <= 2000, 'note : chaine de 2000 caracteres au plus')
    targets = value.get('build_targets')
    if targets is not None:
        need(type(targets) is list and targets and len(set(map(str, targets))) == len(targets) and
             all(type(t) is str and t in executables for t in targets),
             'build_targets : liste non vide de cibles add_executable(mhgp10_*) du commit, sans doublon')
    commands = value.get('commands')
    need(type(commands) is list and 1 <= len(commands) <= 128, 'commands : liste de 1 a 128 commandes')
    names, normalized, python_used = set(), [], False
    for index, command in enumerate(commands):
        where = 'commande %d : ' % index
        need(type(command) is dict and set(command) == {'name', 'argv', 'timeout_seconds'},
             where + 'cles exactes name, argv, timeout_seconds')
        name, argv, timeout = command['name'], command['argv'], command['timeout_seconds']
        need(type(name) is str and NAME_RE.fullmatch(name) and name not in names,
             where + 'nom unique [a-z0-9][a-z0-9_.-]{0,63} attendu')
        names.add(name)
        need(type(timeout) is int and 1 <= timeout <= MAX_RUN_SECONDS, where + 'timeout_seconds entier entre 1 et 28800')
        need(type(argv) is list and 1 <= len(argv) <= 256 and
             all(type(arg) is str and len(arg) <= 8192 and not set(arg) & set('\0\n\r') for arg in argv),
             where + 'argv : 1 a 256 chaines sans NUL ni saut de ligne')
        head = argv[0]
        binary = BINARY_RE.fullmatch(head)
        if binary:
            need(binary.group(1) in executables, where + 'binaire inconnu du CMake du commit : ' + head)
            need(targets is None or binary.group(1) in targets, where + head + ' absent de build_targets')
        elif head == 'python3':
            script = PYTHON_RE.fullmatch(argv[1]) if len(argv) >= 2 else None
            need(script is not None and script.group(1) in tracked,
                 where + 'python3 exige {src}/' + SOURCE_ROOT + '/<script suivi>.py en second argument')
            python_used = True
        else:
            need(head == 'ctest', where + 'argv[0] doit etre ./mhgp10_*, python3 ou ctest')
            check_ctest(where, argv)
            python_used = True
        for arg in argv:
            for ref in DATA_REF_RE.findall(arg):
                need(ref in data_names, where + 'fichier de donnees absent de --data : {data}/' + ref)
        normalized.append({'name': name, 'argv': list(argv), 'timeout_seconds': timeout})
    return {'schema': PLAN_SCHEMA, 'needs_python': declared_python or python_used,
            'needs_python_deduced': python_used and not declared_python, 'build_timeout_seconds': build_timeout,
            'build_targets': list(targets or []), 'results_cap_bytes': cap, 'commands': normalized, 'note': note}


def results_slack(cap):
    return min(64 * 2 ** 20, cap // 4)


def render_worker_plan(plan):
    quote = shlex.quote
    lines = [WORKER_PLAN_MAGIC,
             'PLAN_NEEDS_PYTHON=' + ('1' if plan['needs_python'] else '0'),
             'PLAN_BUILD_TIMEOUT=' + str(plan['build_timeout_seconds']),
             'PLAN_RESULTS_CAP=' + str(plan['results_cap_bytes']),
             'PLAN_BUILD_TARGETS=(' + ' '.join(quote(t) for t in plan['build_targets']) + ')',
             'PLAN_COUNT=' + str(len(plan['commands']))]
    for index, command in enumerate(plan['commands']):
        lines += ['PLAN_NAME_%d=%s' % (index, quote(command['name'])),
                  'PLAN_TIMEOUT_%d=%d' % (index, command['timeout_seconds']),
                  'PLAN_ARGV_%d=(%s)' % (index, ' '.join(quote(arg) for arg in command['argv']))]
    return ('\n'.join(lines) + '\n').encode()


def validate_data(directory):
    path = Path(directory).absolute()
    need(':' not in str(path), '--data ne doit pas contenir « : » (gcloud compute scp le lirait INSTANCE:CHEMIN)')
    need(path.is_dir() and not path.is_symlink(), '--data doit etre un dossier existant non symbolique')
    entries = sorted(os.listdir(path))
    need(len(entries) <= MAX_DATA_FILES, 'plus de %d fichiers de donnees' % MAX_DATA_FILES)
    files, total = [], 0
    for name in entries:
        need(DATA_NAME_RE.fullmatch(name) and name != 'SHA256SUMS',
             'nom de donnee refuse (dossier plat, [A-Za-z0-9][A-Za-z0-9._-]*, pas SHA256SUMS) : ' + name)
        info = os.lstat(path / name)
        need(stat.S_ISREG(info.st_mode), 'donnee non reguliere ou symbolique : ' + name)
        need(info.st_size > 0, 'donnee vide : ' + name)
        need(not name.endswith('.u32le') or info.st_size % 4 == 0, 'taille .u32le non multiple de 4 : ' + name)
        files.append({'name': name, 'size': info.st_size, 'sha256': sha_file(path / name)})
        total += info.st_size
    need(total <= MAX_DATA_BYTES, 'donnees au-dela de %d octets' % MAX_DATA_BYTES)
    try:
        inside = path.resolve().relative_to(REPO)
    except ValueError:
        inside = None
    if inside is not None:
        tracked = git('ls-files', '-z', '--', str(path.resolve()), binary=True)
        need(not tracked.strip(b'\0'), 'des donnees sont versionnees dans Git : refus (les .u32le restent hors depot)')
    manifest = ''.join('%s  %s\n' % (item['sha256'], item['name']) for item in files).encode()
    return path, files, total, manifest


def build_package(commit, directory):
    package = Path(directory) / 'package.tar.gz'
    git('archive', '--format=tar.gz', '-o', str(package), commit, '--', SOURCE_ROOT, WORKER, timeout=300)
    size = package.stat().st_size
    need(size <= MAX_PACKAGE_BYTES, 'paquet au-dela de %d octets' % MAX_PACKAGE_BYTES)
    with tarfile.open(package, 'r:gz') as archive:
        for member in archive.getmembers():
            name = member.name.rstrip('/')
            parts = PurePosixPath(name)
            need(not parts.is_absolute() and '..' not in parts.parts and (member.isfile() or member.isdir()) and
                 (name in (SOURCE_ROOT, 'gcp-migration', WORKER) or name.startswith(SOURCE_ROOT + '/')),
                 'membre inattendu dans le paquet : ' + member.name)
    return package, sha_file(package), size


def describe_target(runner, gcloud, name, critical=False, timeout=90):
    rc, out, err = runner.run(name, [gcloud, 'compute', 'instances', 'describe', TARGET['instance'],
                                     '--project=' + TARGET['project'], '--zone=' + TARGET['zone'],
                                     '--format=json'], timeout=timeout, critical=critical,
                              interruptible=not critical)
    need(rc == 0, 'lecture de la cible impossible : ' + err.strip()[:300])
    value = strict_json(out)
    need(type(value) is dict, 'description de cible invalide')
    return value


def check_target(value, status, max_run, generation=None):
    zone_path = '/projects/%s/zones/%s/instances/%s' % (TARGET['project'], TARGET['zone'], TARGET['instance'])
    need(value.get('name') == TARGET['instance'] and str(value.get('zone', '')).rsplit('/', 1)[-1] == TARGET['zone'] and
         str(value.get('selfLink', '')).endswith(zone_path) and
         (value.get('labels') or {}).get('project') == 'e-hgp' and
         str(value.get('machineType', '')).rsplit('/', 1)[-1] == MACHINE_TYPE,
         'cible fixe attendue : %s/%s/%s, %s, label project=e-hgp' % (
             TARGET['project'], TARGET['zone'], TARGET['instance'], MACHINE_TYPE))
    scheduling = value.get('scheduling') or {}
    need(scheduling.get('provisioningModel') == 'SPOT' and scheduling.get('instanceTerminationAction') == 'STOP' and
         scheduling.get('onHostMaintenance') == 'TERMINATE' and scheduling.get('automaticRestart') is False,
         'garde statique attendue : SPOT, action STOP, maintenance TERMINATE, automaticRestart=false')
    metadata = {str(item.get('key')): str(item.get('value'))
                for item in ((value.get('metadata') or {}).get('items') or []) if isinstance(item, dict)}
    need(metadata.get('enable-oslogin', '').upper() == 'TRUE',
         'metadonnee d\'instance enable-oslogin=TRUE exigee (sans OS Login, gcloud ecrirait la cle dans le projet)')
    configured = str((scheduling.get('maxRunDuration') or {}).get('seconds', ''))
    need(re.fullmatch(r'[1-9][0-9]{1,4}', configured) and MIN_RUN_SECONDS <= int(configured) <= MAX_RUN_SECONDS,
         'maxRunDuration de la cible absent ou hors [30 s, 8 h] : ' + configured)
    need(int(configured) == max_run,
         'maxRunDuration configure = %s s != --max-run-seconds = %d s ; set_max_run_duration_and_verify.sh '
         "n'admet pas cette cible (allowlist europe-west4) : relancer avec --max-run-seconds %s, ou etendre "
         "d'abord l'allowlist de ce script garde (hors de ce protocole)" % (configured, max_run, configured))
    need(value.get('status') == status, 'etat de la cible = %s, %s exige' % (value.get('status'), status))
    if generation is not None:
        need(value.get('lastStartTimestamp') == generation, 'generation de la cible differente de la session')


def runner_env(gcloud, key=None, account=None):
    """Projet (et compte, une fois lu) FIGES : un changement de configuration gcloud partagee pendant la
    session ne fait echouer ni stop_and_verify.sh ni le retrait de cle."""
    env = dict(os.environ)
    env['PATH'] = str(Path(gcloud).parent) + os.pathsep + env.get('PATH', '')
    env['CLOUDSDK_CORE_DISABLE_PROMPTS'] = '1'
    env['CLOUDSDK_CORE_PROJECT'] = TARGET['project']
    if account:
        env['CLOUDSDK_CORE_ACCOUNT'] = account
    env.update(GCP_PROJECT_ID=TARGET['project'], GCP_ZONE=TARGET['zone'], GCP_INSTANCE_NAME=TARGET['instance'])
    if key is not None:
        env['GCP_SSH_KEY_FILE'] = str(key)
    return env


def preflight(args, report, scratch, child):
    """Toute la validation ; lectures GCP seulement. Rend le contexte d'execution."""
    max_run = args.max_run_seconds
    need(MIN_RUN_SECONDS <= max_run <= MAX_RUN_SECONDS, '--max-run-seconds entre 30 et 28800')
    guest_minutes = min(480, (max_run - GCE_GUARD_OVERHEAD) // 60)
    need(guest_minutes >= MIN_GUEST_MINUTES,
         '--max-run-seconds trop court : arret invite < %d min apres les reserves de start_and_verify.sh' %
         MIN_GUEST_MINUTES)
    ttl_minutes = math.ceil(max_run / 60) + 5
    report.update(max_run_seconds=max_run, guest_shutdown_minutes=guest_minutes, os_login_ttl_minutes=ttl_minutes)
    root = Path(args.sessions_root)
    session = Path(args.session_dir)
    need(root.is_absolute() and root.is_dir() and not root.is_symlink(), 'racine des sessions absente : ' + str(root))
    need(session.name != '.ehgp-v10.lock', 'nom de session reserve')
    need(session.is_absolute() and session.parent == root and DATA_NAME_RE.fullmatch(session.name) and
         ':' not in str(session), '--session-dir doit etre un chemin absolu sans « : », enfant direct de ' + str(root))
    if child:
        need(session.is_dir() and not session.is_symlink() and session.stat().st_mode & 0o777 == 0o700 and
             set(os.listdir(session)) <= LAUNCH_FILES, 'dossier de session du lanceur absent ou deja utilise')
    else:
        need(not os.path.lexists(session), '--session-dir existe deja : ' + str(session))
    commit = resolve_commit(args.commit, not args.no_fetch)
    report['commit'] = commit
    protocol = {name: commit_file(commit, name) for name in (CONTROLLER, WORKER, START_GUARD, STOP_GUARD)}
    need(sha_bytes(protocol[CONTROLLER]) == sha_file(Path(__file__)),
         'le controleur execute differe de ' + CONTROLLER + ' au commit : committer et pousser le protocole')
    for name, pin in GUARD_PINS.items():
        need(sha_bytes(protocol[name]) == pin, 'script garde %s au commit different de son epingle : revue requise' % name)
    report['protocol_sha256'] = {name: sha_bytes(raw) for name, raw in sorted(protocol.items())}
    tracked = tracked_tree(commit)
    executables = cmake_executables(commit, tracked)
    data_dir, data_files, data_total, data_manifest = validate_data(args.data)
    report.update(data_dir=str(data_dir), data_files=data_files, data_bytes=data_total,
                  data_manifest_sha256=sha_bytes(data_manifest))
    plan_path = Path(args.plan)
    need(plan_path.is_file() and plan_path.stat().st_size <= 1 << 20, '--plan : fichier JSON de 1 Mio au plus')
    plan_raw = plan_path.read_bytes()
    plan = validate_plan(strict_json(plan_raw), tracked, executables, {item['name'] for item in data_files})
    worker_plan = render_worker_plan(plan)
    report.update(plan_sha256=sha_bytes(plan_raw), worker_plan_sha256=sha_bytes(worker_plan),
                  python_pins=PYTHON_PINS if plan['needs_python'] else None,
                  plan={key: plan[key] for key in ('needs_python', 'needs_python_deduced', 'build_timeout_seconds',
                                                  'build_targets', 'results_cap_bytes', 'commands')})
    package, package_sha, package_size = build_package(commit, scratch)
    report.update(package_sha256=package_sha, package_bytes=package_size)
    cap = plan['results_cap_bytes']
    free = shutil.disk_usage(root).free
    need(free >= package_size + RESERVE_BYTES + 2 * cap + LOCAL_FREE_MARGIN,
         'espace local insuffisant sous %s : %d octets libres, %d exiges (2 x plafond des resultats + marge)' %
         (root, free, package_size + RESERVE_BYTES + 2 * cap + LOCAL_FREE_MARGIN))
    run_parent = Path(tempfile.gettempdir())
    run_free = shutil.disk_usage(run_parent).free
    need(run_free >= RUN_DIR_FREE_MIN, 'espace insuffisant pour le dossier d\'execution sous %s' % run_parent)
    report.update(local_free_bytes=free, run_dir=str(run_directory(session)),
                  run_dir_same_filesystem=os.stat(run_parent).st_dev == os.stat(root).st_dev)
    if report['run_dir_same_filesystem']:
        report.setdefault('warnings', []).append(
            '$TMPDIR est sur le meme systeme de fichiers que les sessions : seule la reserve protege l\'arret')
    guest_seconds = guest_minutes * 60
    upload_estimate = SETUP_ESTIMATE + math.ceil((package_size + data_total) / UPLOAD_RATE)
    worker_window = guest_seconds - upload_estimate - CLOSING_RESERVE - WORKER_PACK_RESERVE
    fixed = plan['build_timeout_seconds'] + (PIP_TIMEOUT if plan['needs_python'] else 0) + 120
    commands_total = sum(c['timeout_seconds'] for c in plan['commands'])
    need(worker_window - fixed >= 60,
         'budget insuffisant : %d s utiles pour %d s de construction/installation' % (worker_window, fixed))
    need(math.ceil((package_size + data_total) / UPLOAD_TIMEOUT_RATE) < worker_window // 2,
         'donnees trop volumineuses pour la fenetre au debit prudent (%d o/s)' % UPLOAD_TIMEOUT_RATE)
    report['budget'] = {'guest_seconds': guest_seconds, 'upload_estimate_seconds': upload_estimate,
                        'closing_reserve_seconds': CLOSING_RESERVE, 'ssh_cutoff_before_guest_seconds': SSH_CUTOFF,
                        'worker_window_seconds': worker_window, 'build_and_setup_seconds': fixed,
                        'command_timeouts_sum_seconds': commands_total,
                        'oversubscribed': commands_total > worker_window - fixed}
    if report['budget']['oversubscribed']:
        report.setdefault('warnings', []).append(
            'somme des delais > fenetre utile : les dernieres commandes seront coupees ou sautees a l\'echeance')
    gcloud = Path(args.gcloud)
    need(gcloud.is_file() and os.access(gcloud, os.X_OK), 'gcloud introuvable : ' + str(gcloud))
    for tool in ('ssh-keygen', 'timeout', 'python3', 'tar'):
        need(shutil.which(tool, path=runner_env(gcloud)['PATH']) is not None, 'outil local absent : ' + tool)
    logs = Path(scratch) / 'preflight_logs'
    logs.mkdir(exist_ok=True)
    plain = dict(os.environ, PATH=str(gcloud.parent) + os.pathsep + os.environ.get('PATH', ''),
                 CLOUDSDK_CORE_DISABLE_PROMPTS='1')
    reader = Runner(logs, plain)
    rc, out, _ = reader.run('config_project', [gcloud, 'config', 'get-value', 'project'], timeout=60)
    need(rc == 0 and out.strip() == TARGET['project'], 'projet gcloud actif different de ' + TARGET['project'])
    rc, out, _ = reader.run('config_account', [gcloud, 'config', 'get-value', 'account'], timeout=60)
    account = out.strip()
    need(rc == 0 and account not in ('', '(unset)') and '\n' not in account, 'aucun compte gcloud actif')
    runner = Runner(logs, runner_env(gcloud, account=account), prefix='frozen_')
    value = describe_target(runner, gcloud, 'describe_before')
    check_target(value, 'TERMINATED', max_run)
    pre_start = value.get('lastStartTimestamp')
    need(type(pre_start) is str and pre_start, 'lastStartTimestamp avant demarrage absent')
    epoch(pre_start)
    report.update(pre_start_generation=pre_start, gcloud_account=account,
                  gcp_reads=[row['argv'][1:4] for row in reader.rows + runner.rows])
    return dict(commit=commit, protocol=protocol, plan=plan, plan_raw=plan_raw, worker_plan=worker_plan,
                data_dir=data_dir, data_files=data_files, data_total=data_total, data_manifest=data_manifest,
                package=package, package_sha=package_sha, package_size=package_size, max_run=max_run,
                guest_minutes=guest_minutes, ttl_minutes=ttl_minutes, pre_start=pre_start, session=session,
                gcloud=gcloud, account=account)


def planned_steps(context):
    return [
        'creer %s (0700) et le dossier d\'execution jetable %s ; lancer le processus de session DETACHE '
        '(setsid, sorties vers session.stdout/.stderr, verrou session.lock, sentinelle DONE)' % (
            context['session'], run_directory(context['session'])),
        'reserve preallouee de %d o dans la session ; cle ed25519 de session (600) ; os-login ssh-keys add '
        '--ttl=%dm' % (RESERVE_BYTES, context['ttl_minutes']),
        'start_and_verify.sh --yes --guest-shutdown-minutes %d ... (maxRunDuration GCE %d s), jamais signale' %
        (context['guest_minutes'], context['max_run']),
        'recertifier RUNNING + generation ; lire l\'arret invite D ; echeance worker = D - %d s ; aucun SSH apres '
        'D - %d s (/run/nologin a D - %d s)' % (CLOSING_RESERVE, SSH_CUTOFF, NOLOGIN_LEAD),
        'televerser package.tar.gz (%d o), plan.sh, plan.json puis %d fichier(s) de donnees (%d o) ; sha256 verifies '
        'sur la VM ; repertoire distant $HOME/ehgp-v10/ (hors /tmp, anciens builds et donnees elagues)' % (
            context['package_size'], len(context['data_files']), context['data_total']),
        'lancer v10_worker.sh detache (%sconstruction Release -j nproc puis %d commande(s)) ; sondage long avec '
        'recertification de la cible et controle du disque local a chaque cycle' % (
            'Python epingle, ' if context['plan']['needs_python'] else '', len(context['plan']['commands'])),
        'rapatriement borne a %d s et avant D - %d s (garde invitee relue, VM recertifiee, taille <= %d o)' % (
            RETRIEVE_BUDGET, SSH_CUTOFF, context['plan']['results_cap_bytes']),
        'finally : liberer la reserve ; describe ; stop_and_verify.sh --yes --expected-last-start-timestamp '
        '<generation> seulement si RUNNING/SUSPENDING de cette generation sans arret posterieur, ou STOPPING ; '
        'relire TERMINATED ; retirer la '
        'cle OS Login ; puis extraction bornee et verification des resultats',
    ]


# ---------------------------------------------------------------------------------------------
# Generation, fermeture, reprise.

def read_generation(host):
    """(demarrage_peut_avoir_ete_demande, generation|None) d'apres le fichier de passage et le cycle de vie."""
    handoff, lifecycle = host / 'handoff.json', host / 'lifecycle.txt'
    requested = handoff.exists() or lifecycle.exists()
    generations = []
    if handoff.exists():
        record = strict_json(handoff.read_bytes())
        need(type(record) is dict and record.get('schema') == 'e-hgp.start-handoff.v3' and
             all(record.get(k) == v for k, v in TARGET.items()), 'fichier de passage : schema ou cible')
        generations.append(record.get('last_start_timestamp'))
    if lifecycle.exists():
        record = fields(lifecycle.read_text())
        need(record.get('schema') == 'e-hgp.lifecycle-state.v1' and all(record.get(k) == v for k, v in TARGET.items())
             and record.get('state') in LIFECYCLE_STATES, 'enregistrement de cycle de vie : schema, cible ou etat')
        if record.get('generation'):
            generations.append(record['generation'])
    if not generations:
        return requested, None
    need(len(set(generations)) == 1, 'generation ambigue entre fichier de passage et cycle de vie ; jamais devinee')
    epoch(generations[0])
    return requested, generations[0]


def describe_command():
    return ("gcloud compute instances describe %s --project=%s --zone=%s "
            "--format='value(status,lastStartTimestamp,lastStopTimestamp)'" % (
                TARGET['instance'], TARGET['project'], TARGET['zone']))


def recovery_command(host, generation, account=None):
    """Commande d'arret de reprise, projet et compte gcloud FIGES (une configuration partagee qui a derive
    ferait sinon refuser stop_and_verify.sh)."""
    frozen = 'CLOUDSDK_CORE_PROJECT=%s ' % TARGET['project']
    if account:
        frozen += 'CLOUDSDK_CORE_ACCOUNT=%s ' % shlex.quote(account)
    return frozen + 'GCP_PROJECT_ID=%s GCP_ZONE=%s GCP_INSTANCE_NAME=%s %s --yes --expected-last-start-timestamp %s' % (
        TARGET['project'], TARGET['zone'], TARGET['instance'], shlex.quote(str(host / 'stop_and_verify.sh')),
        shlex.quote(generation))


def recovery_text(session, host, generation, account=None):
    return ('1. Lire d\'abord (lecture seule) : %s\n'
            '2. SEULEMENT si l\'etat est RUNNING ou SUSPENDING avec lastStartTimestamp = %s ET lastStopTimestamp '
            'absent ou anterieur a cette generation (STOPPING : attendre et relire) :\n   %s\n'
            '   (TERMINATED avec cette generation : rien a faire ; STAGING/PROVISIONING, autre generation, ou '
            'lastStopTimestamp posterieur a cette generation : demarrage d\'une autre session, NE RIEN arreter.)\n'
            '3. Ou, equivalent et verifie : python3 %s --recover --session-dir %s\n' % (
                describe_command(), generation, recovery_command(host, generation, account), CONTROLLER, session))


def unknown_generation_text(pre_start):
    return ("generation inconnue : NE JAMAIS lancer stop_and_verify.sh sans --expected-last-start-timestamp. "
            "Verifier d'abord qu'aucun processus ne reference la session (--recover le fait), puis lire : %s, et "
            "gcloud compute operations list --project=%s --zones=%s --filter='targetLink~%s AND "
            "operationType=start' (lecture seule). Si TERMINATED et lastStartTimestamp == %s sans operation start "
            "en cours : aucun demarrage n'a eu lieu, relire quelques minutes plus tard pour confirmer. Sinon reprise "
            "humaine : verifier qu'aucune autre session n'utilise la cible avant tout arret par la generation lue." % (
                describe_command(), TARGET['project'], TARGET['zone'], TARGET['instance'],
                pre_start or '<generation d avant demarrage>'))


def observe(runner, gcloud, name, until=None, patience=CLOSE_PATIENCE):
    """Relecture de la cible en lecture seule (mode critique). Illisible (hoquet d'API, 503, coupure) :
    retentee avec un repli progressif (5, 10, 20 puis 30 s) pendant `patience` secondes, et jamais au-dela
    de `until` (horloge monotone) ; None si elle reste illisible."""
    started = time.monotonic()
    for attempt, delay in enumerate((0,) + PATIENCE_DELAYS):
        if delay:
            if time.monotonic() + delay - started > patience:
                return None
            if until is not None and until - time.monotonic() < delay + 20:
                return None
            time.sleep(delay)
        timeout = 90 if until is None else min(90, until - time.monotonic())
        if timeout < 10:
            return None
        try:
            value = describe_target(runner, gcloud, '%s_%d' % (name, attempt + 1), critical=True, timeout=timeout)
        except (Refusal, OSError, subprocess.SubprocessError):
            continue
        return {'status': value.get('status'), 'lastStartTimestamp': value.get('lastStartTimestamp'),
                'lastStopTimestamp': value.get('lastStopTimestamp'), 'name': value.get('name')}
    return None


def stopped_after(observed, generation):
    """Vrai si la cible a ete arretee APRES le debut de la generation (lastStopTimestamp >= G) : un etat
    actif qui porte encore G appartient alors au demarrage d'une autre session (generation pas encore
    materialisee, cf. start_and_verify.sh)."""
    stop = observed.get('lastStopTimestamp')
    return bool(stop) and epoch(stop) >= epoch(generation)


def stop_decision(observed, generation):
    """Decision de fermeture sur une relecture ; jamais d'arret d'une VM qui n'est pas prouvee la notre."""
    if observed is None or observed.get('name') != TARGET['instance']:
        return 'unreadable'
    status = observed.get('status')
    if observed.get('lastStartTimestamp') != generation:
        if status == 'TERMINATED':
            return 'terminated_after_foreign_generation'
        return 'own_generation_ended_foreign_active'
    if status == 'TERMINATED':
        return 'already_terminated'
    if status == 'STOPPING':
        return 'stop'           # arret deja en cours : stop_and_verify.sh attend TERMINATED sans nouvelle mutation
    if status not in ('RUNNING', 'SUSPENDING'):
        return 'start_in_flight_same_generation'
    try:
        if stopped_after(observed, generation):
            return 'stopped_after_generation_foreign_active'
    except (Refusal, ValueError):
        return 'unreadable'
    return 'stop'


def foreign_text(closure, observed, generation):
    if closure == 'own_generation_ended_foreign_active':
        return ('NE PAS arreter : notre generation %s est terminee (une generation plus recente %s existe, etat '
                '%s) ; elle appartient a une autre session.' % (generation, observed.get('lastStartTimestamp'),
                                                                observed.get('status')))
    if closure == 'stopped_after_generation_foreign_active':
        return ('NE PAS arreter : la cible a ete arretee (lastStopTimestamp %s) apres le debut de notre generation '
                '%s ; l\'etat %s qui porte encore cette generation est le demarrage d\'une autre session.' % (
                    observed.get('lastStopTimestamp'), generation, observed.get('status')))
    return ('NE PAS arreter : etat %s avec lastStartTimestamp = %s, c\'est-a-dire un demarrage en vol dont la '
            'generation n\'est pas encore materialisee (autre session). Relire dans quelques minutes : %s' % (
                observed.get('status'), generation, describe_command()))


def close_by_generation(state, runner, host, gcloud, pre_start, release_reserve=None, account=None):
    """Fermeture par la generation PROUVEE, apres un describe en lecture seule (retente avec un repli
    progressif pendant ~4 min tant qu'il est illisible) :

    - aucun fichier de passage ni cycle de vie : aucun demarrage demande, aucun arret ;
    - cycle de vie sans generation : aucun arret devine (74, reprise humaine) ;
    - TERMINATED avec notre generation G : deja arrete, certifie sans stop ;
    - RUNNING/SUSPENDING avec G sans arret posterieur a G (lastStopTimestamp), ou STOPPING avec G :
      stop_and_verify.sh --expected-last-start-timestamp G ; si la relecture ne montre pas TERMINATED,
      un second cycle relecture -> decision -> stop, avec les memes regles ;
    - arret posterieur a G sur un etat actif portant G, STAGING/PROVISIONING (ou tout autre etat) avec G,
      ou une autre generation : demarrage d'une autre session ; NE RIEN arreter (75).
    Fenetre residuelle : entre ce describe et les lectures internes de stop_and_verify.sh (quelques
    secondes). Aucune operation susceptible de lever ne precede l'appel a stop_and_verify.sh."""
    try:
        requested, recorded = read_generation(host)
    except (Refusal, OSError, ValueError) as error:
        requested, recorded = True, None
        state['errors'].append('enregistrements de generation illisibles : %s' % error)
    state['start_may_have_been_requested'] = requested
    known = state.get('generation')
    if recorded is not None and known is not None and recorded != known:
        state.update(closure='conflicting_generations', recovery=unknown_generation_text(pre_start))
        return
    closing = recorded or known
    if closing is None:
        if not requested:
            state.update(closure='no_start_requested', recovery='aucun demarrage demande : rien a arreter')
            return
        state.update(closure='generation_unknown', recovery=unknown_generation_text(pre_start),
                     observed_after=observe(runner, gcloud, 'describe_generation_unknown'))
        return
    state['closing_generation'] = closing
    state['recovery_command'] = recovery_command(host, closing, account)
    if release_reserve is not None:
        release_reserve()
    rc, stops = None, 0
    while True:
        observed = observe(runner, gcloud, 'describe_before_stop' if stops == 0 else 'describe_after_stop%d' % stops)
        state['observed_before_stop' if stops == 0 else 'observed_after'] = observed
        decision = stop_decision(observed, closing)
        if decision != 'stop' or stops == 2:
            break
        rc, _, _ = runner.run('guarded_stop', [host / 'stop_and_verify.sh', '--yes', '--expected-last-start-timestamp',
                                               closing], timeout=STOP_TIMEOUT, guard=True, critical=True,
                              interruptible=False)
        stops += 1
        state.update(stop_exit_code=rc, stop_attempts=stops)
    if decision == 'already_terminated':
        state.update(targeted_shutdown_certified=True, closure='already_terminated' if rc is None else 'stopped')
    elif decision == 'terminated_after_foreign_generation':
        state.update(targeted_shutdown_certified=True, closure=decision)
    elif decision in FOREIGN_CLOSURES:
        state.update(closure=decision, recovery=foreign_text(decision, observed, closing))
    else:
        state.update(closure='describe_failed' if rc is None else 'stop_uncertified',
                     recovery=recovery_text(host.parent, host, closing, account))
    log('fermeture : %s (generation %s, code stop %s)' % (state['closure'], closing, rc))


def closure_status(state):
    closure = state.get('closure')
    if closure == 'no_start_requested':
        return 'failed_before_start'
    if closure in FOREIGN_CLOSURES:
        return 'foreign_generation_active'
    if not state.get('targeted_shutdown_certified'):
        return 'shutdown_uncertified'
    return None


def process_parent(pid):
    try:
        text = Path('/proc/%d/stat' % pid).read_text()
        return int(text[text.rindex(')') + 2:].split()[1])
    except (OSError, ValueError, IndexError):
        return 0


def session_dir_arguments(args):
    for index, arg in enumerate(args):
        if arg == '--session-dir' and index + 1 < len(args):
            yield args[index + 1]
        elif arg.startswith('--session-dir='):
            yield arg.split('=', 1)[1]


def session_processes(session):
    """(bloquants, ignores) parmi les processus vivants qui referencent la session, hors soi-meme et ses
    ancetres. Seuls BLOQUENT ceux qui peuvent muter la VM : le processus de session lui-meme (PID de
    launch.json, ou `v10_session.py ... --child` sur ce dossier) et les scripts gardes de la session
    (host/start_and_verify.sh, host/stop_and_verify.sh). Les observateurs (tail -f, attente de DONE,
    SSH de sondage orphelin) sont ignores et journalises. Chemins compares apres resolution des liens."""
    marker = os.path.realpath(session)
    guards = {os.path.join(marker, 'host', name) for name in ('start_and_verify.sh', 'stop_and_verify.sh')}
    try:
        launched = json.loads((Path(session) / 'launch.json').read_text()).get('pid')
    except (OSError, ValueError, AttributeError):
        launched = None
    excluded, pid = set(), os.getpid()
    while pid > 1 and pid not in excluded:
        excluded.add(pid)
        pid = process_parent(pid)
    blocking, ignored = [], []
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit() or int(entry.name) in excluded:
            continue
        try:
            args = [arg.decode(errors='replace') for arg in (entry / 'cmdline').read_bytes().split(b'\0') if arg]
        except OSError:
            continue
        real = [os.path.realpath(arg) if arg.startswith('/') else arg for arg in args]
        is_child = '--child' in args and any(arg.endswith('v10_session.py') for arg in args) and (
            int(entry.name) == launched or
            any(os.path.realpath(value) == marker for value in session_dir_arguments(args)))
        is_guard = any(path in guards for path in real)
        if not (is_child or is_guard or any(path == marker or path.startswith(marker + '/') or marker in arg or
                                            str(session) in arg for path, arg in zip(real, args))):
            continue
        item = {'pid': int(entry.name), 'cmdline': ' '.join(args)[:300]}
        (blocking if is_child or is_guard else ignored).append(item)
    return blocking, ignored


def take_lock(path, wait_seconds=0.0, create=True):
    """flock exclusif ; None si un autre processus de la session le tient encore apres wait_seconds.
    Sans create, un verrou absent n'a jamais ete tenu : on rend -1 (rien a exclure)."""
    try:
        descriptor = os.open(path, os.O_RDWR | (os.O_CREAT if create else 0), 0o600)
    except FileNotFoundError:
        if create:
            raise
        return -1
    end = time.monotonic() + wait_seconds
    while True:
        try:
            fcntl.flock(descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
            return descriptor
        except BlockingIOError:
            if time.monotonic() >= end:
                os.close(descriptor)
                return None
            time.sleep(1)


# ---------------------------------------------------------------------------------------------
# Resultats (extraction et verification apres l'arret).

def archive_file_bytes(archive_path):
    """Taille DECOMPRESSEE (somme des membres) lue dans l'index du tar, sans rien extraire."""
    total = 0
    with tarfile.open(archive_path, 'r:gz') as archive:
        for member in archive.getmembers():
            if member.isfile():
                total += member.size
    return total


def safe_extract(archive_path, destination):
    """Extrait results/ : fichiers et dossiers seulement, jamais de chemin absolu ou remontant."""
    skipped = []
    destination.mkdir(mode=0o700)
    with tarfile.open(archive_path, 'r:gz') as archive:
        seen = set()
        for member in archive.getmembers():
            name = member.name.rstrip('/')
            parts = PurePosixPath(name)
            need(name and not parts.is_absolute() and '..' not in parts.parts and parts.parts[0] == 'results' and
                 name not in seen, 'membre dangereux dans l\'archive : ' + member.name)
            seen.add(name)
            target = destination.joinpath(*parts.parts)
            if member.isdir():
                target.mkdir(mode=0o700, parents=True, exist_ok=True)
            elif member.isfile():
                target.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
                with archive.extractfile(member) as source, open(target, 'xb') as sink:
                    shutil.copyfileobj(source, sink)
            else:
                skipped.append(name)
    return skipped


def decode_manifest_line(line):
    """Ligne sha256sum ; un nom echappe (ligne commencant par « \\ ») est desechappe en UN passage."""
    escaped = line.startswith('\\')
    body = line[1:] if escaped else line
    digest, separator, name = body[:64], body[64:66], body[66:]
    need(SHA_RE.fullmatch(digest) and separator == '  ', 'ligne de MANIFEST invalide')
    if escaped:
        name = re.sub(r'\\(.)', lambda m: {'n': '\n', 'r': '\r', '\\': '\\'}.get(m.group(1), m.group(0)), name)
    return digest, name


def verify_manifest(results):
    manifest = results / 'MANIFEST.sha256'
    need(manifest.is_file(), 'MANIFEST.sha256 absent des resultats')
    listed = {}
    for line in manifest.read_text(errors='surrogateescape').splitlines():
        digest, name = decode_manifest_line(line)
        need(name.startswith('./'), 'ligne de MANIFEST invalide')
        listed[name[2:]] = digest
    present = {path.relative_to(results).as_posix() for path in results.rglob('*') if path.is_file()}
    present.discard('MANIFEST.sha256')
    need(set(listed) == present, 'MANIFEST.sha256 ne couvre pas exactement les fichiers extraits')
    need(all(sha_file(results / name) == digest for name, digest in listed.items()),
         'hachage d\'un fichier de resultat different de MANIFEST.sha256')
    return len(listed)


def parse_results(results):
    worker = {}
    if (results / 'worker.txt').is_file():
        worker = fields((results / 'worker.txt').read_text())
    commands = []
    table = results / 'commands.tsv'
    if table.is_file():
        lines = table.read_text().splitlines()
        header = lines[0].split('\t') if lines else []
        commands = [dict(zip(header, line.split('\t'))) for line in lines[1:]]
    return worker, commands


def read_lines(path, limit=20):
    try:
        return path.read_text(errors='replace').splitlines()[:limit]
    except OSError:
        return []


# ---------------------------------------------------------------------------------------------
# Processus de session (detache).

class Session:
    def __init__(self, context, args, report, run_dir):
        self.context, self.args, self.report, self.run_dir = context, args, report, run_dir
        self.session, self.gcloud = context['session'], context['gcloud']
        self.host = self.session / 'host'
        self.package_dir, self.results_dir = self.session / 'package', self.session / 'results'
        self.marks, self.logs = self.host / 'guardmarks', self.host / 'logs'
        self.key = self.session / 'key'
        self.reserve = self.host / 'reserve.bin'
        self.run_reserve = run_dir / 'reserve.bin'
        self.runner = None
        self.remote = self.worker_pid = self.common = None
        self.deadline = self.worker_deadline = None
        self.archive = None
        self.mono_offset = time.monotonic() - time.time()
        self.state = {
            'schema': RECEIPT_SCHEMA, 'status': None, 'target': TARGET, 'backend': 'reference_cpu',
            'public_status': 'not_claimed', 'comparability': COMPARABILITY, 'commit': context['commit'],
            'package_sha256': context['package_sha'], 'plan_sha256': report['plan_sha256'],
            'worker_plan_sha256': report['worker_plan_sha256'], 'data_manifest_sha256': report['data_manifest_sha256'],
            'data_files': context['data_files'], 'python_pins': report.get('python_pins'),
            'results_cap_bytes': context['plan']['results_cap_bytes'], 'run_dir': str(run_dir),
            'max_run_seconds': context['max_run'], 'guest_shutdown_minutes': context['guest_minutes'],
            'os_login_ttl_minutes': context['ttl_minutes'], 'pre_start_generation': context['pre_start'],
            'generation': None, 'gcp_mutation_phase_entered': False, 'oslogin_key_maybe_added': False,
            'start_may_have_been_requested': False, 'start_certified': False, 'worker_launched': False,
            'worker_outcome': None, 'worker_exit_code': None, 'results_verified': False,
            'targeted_shutdown_certified': False, 'closure': None, 'errors': [], 'warnings': []}

    # -- aides ----------------------------------------------------------------------------------

    def remaining(self, until_epoch):
        return min(until_epoch - time.time(), until_epoch + self.mono_offset - time.monotonic())

    def mono(self, until_epoch):
        return until_epoch + self.mono_offset

    def ssh(self, name, script, timeout, quiet=False, interruptible=True):
        return self.runner.run(name, [self.gcloud, 'compute', 'ssh', TARGET['instance'], *self.common,
                                      '--ssh-flag=-n', '--ssh-flag=-o BatchMode=yes', '--ssh-flag=-o ConnectTimeout=15',
                                      '--ssh-flag=-o ServerAliveInterval=15', '--ssh-flag=-o ServerAliveCountMax=4',
                                      '--command=' + script], timeout=timeout, quiet=quiet,
                               interruptible=interruptible)

    def scp(self, name, sources, destination, timeout):
        return self.runner.run(name, [self.gcloud, 'compute', 'scp', *self.common,
                                      '--scp-flag=-o BatchMode=yes', '--scp-flag=-o ConnectTimeout=15',
                                      '--scp-flag=-o ServerAliveInterval=15', '--scp-flag=-o ServerAliveCountMax=4',
                                      *sources, destination], timeout=timeout)

    def target_ok(self, name, until=None):
        """Lecture seule (relectures illisibles retentees ~3 min, jamais au-dela de `until`) : RUNNING sur
        NOTRE generation sans arret posterieur a celle-ci ; sinon plus aucun SSH (cible perdue)."""
        observed = observe(self.runner, self.gcloud, name, until=until, patience=POLL_PATIENCE)
        generation = self.state['generation']
        try:
            ours = (observed is not None and observed.get('status') == 'RUNNING' and
                    observed.get('lastStartTimestamp') == generation and not stopped_after(observed, generation))
        except (Refusal, ValueError):
            ours = False
        if ours:
            SIGNALS.check()
            return True
        self.state['target_lost'] = {'observed_by': name, 'observed': observed}
        return False

    def release_reserve(self):
        for reserve in (self.reserve, self.run_reserve):
            try:
                reserve.unlink()
                self.state['reserve_released'] = True
            except OSError:
                pass

    def local_disk_low(self):
        try:
            return (shutil.disk_usage(self.session).free < SESSION_FREE_FLOOR or
                    shutil.disk_usage(self.run_dir).free < RUN_DIR_FLOOR)
        except OSError:
            return True

    def worker_marker(self):
        return self.remote + '/src/' + WORKER

    def wait_poll(self, window, timeout=None):
        """Un seul SSH qui attend jusqu'a `window` s la sortie du worker (identite verifiee par cmdline)."""
        exit_file = shlex.quote(self.remote + '/worker.exit')
        script = ('e=%s; p=%d; m=%s; alive() { [ -r /proc/$p/cmdline ] && tr "\\0" " " < /proc/$p/cmdline | '
                  'grep -qF -- "$m"; }; end=$(( $(date +%%s) + %d )); while :; do '
                  'if [ -f "$e" ]; then printf "EXIT=%%s\\n" "$(cat "$e")"; break; fi; '
                  'if ! alive; then if [ -f "$e" ]; then printf "EXIT=%%s\\n" "$(cat "$e")"; else echo DEAD; fi; '
                  'break; fi; if [ "$(date +%%s)" -ge "$end" ]; then echo ALIVE; break; fi; sleep %s; done') % (
                      exit_file, self.worker_pid, shlex.quote(self.worker_marker()), int(window),
                      self.args.poll_seconds)
        rc, out, _ = self.ssh('poll', script, timeout=timeout or window + 120, quiet=True)
        self.state['polls'] = self.state.get('polls', 0) + 1
        self.state['last_poll'] = {'exit_code': rc, 'output': out.strip()[-200:]}
        if rc != 0:
            return 'unknown', None
        match = re.search(r'^EXIT=([0-9]+)$', out, re.M)
        if match:
            return 'exit', int(match.group(1))
        if re.search(r'^DEAD$', out, re.M):
            return 'dead', None
        return ('alive', None) if re.search(r'^ALIVE$', out, re.M) else ('unknown', None)

    # -- deroulement ----------------------------------------------------------------------------

    def prepare(self):
        for directory in (self.host, self.package_dir, self.results_dir, self.marks, self.logs):
            directory.mkdir(mode=0o700)
            os.chmod(directory, 0o700)
        CONSOLE.path = self.run_dir / 'session.log'
        with open(self.reserve, 'xb') as stream:
            for _ in range(RESERVE_BYTES // (1 << 20)):
                stream.write(b'\0' * (1 << 20))
            stream.flush()
            os.fsync(stream.fileno())
        with open(self.run_reserve, 'wb') as stream:
            stream.write(b'\0' * RUN_DIR_RESERVE_BYTES)
            stream.flush()
            os.fsync(stream.fileno())
        shutil.copyfile(self.context['package'], self.package_dir / 'package.tar.gz')
        need(sha_file(self.package_dir / 'package.tar.gz') == self.context['package_sha'], 'copie du paquet alteree')
        write_new(self.package_dir / 'plan.json', self.context['plan_raw'])
        write_new(self.package_dir / 'plan.sh', self.context['worker_plan'])
        (self.package_dir / 'data').mkdir(mode=0o700)
        write_new(self.package_dir / 'data' / 'SHA256SUMS', self.context['data_manifest'])
        for name in (START_GUARD, STOP_GUARD):
            write_new(self.host / Path(name).name, self.context['protocol'][name], mode=0o700)
            os.chmod(self.host / Path(name).name, 0o700)
        write_new(self.session / 'preflight.json', json_bytes(self.report))
        keygen = subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-C', 'ehgp-v10-' + self.session.name,
                                 '-f', str(self.key)], stdin=subprocess.DEVNULL, capture_output=True, timeout=60)
        need(keygen.returncode == 0, 'ssh-keygen a echoue')
        os.chmod(self.key, 0o600)
        logs = self.run_dir / 'logs'
        logs.mkdir(mode=0o700, exist_ok=True)
        self.runner = Runner(logs, runner_env(self.gcloud, self.key, self.context['account']), mirror=self.logs)

    def gcp_phase(self):
        state, context = self.state, self.context
        max_run, guest_minutes = context['max_run'], context['guest_minutes']
        # Premiere mutation : inscription OS Login de la seule cle publique de session (marquee avant
        # l'appel : un ajout applique puis coupe par le delai sera quand meme retire).
        state['gcp_mutation_phase_entered'] = True
        state['oslogin_key_maybe_added'] = True
        log('inscription OS Login de la cle de session (ttl %d min)' % context['ttl_minutes'])
        rc, _, err = self.runner.run('oslogin_add', [self.gcloud, 'compute', 'os-login', 'ssh-keys', 'add',
                                                     '--project=' + TARGET['project'], '--key-file=%s.pub' % self.key,
                                                     '--ttl=%dm' % context['ttl_minutes'], '--format=json',
                                                     '--quiet'], timeout=120)
        need(rc == 0, 'inscription OS Login refusee : ' + err.strip()[:300])
        # Demarrage garde : jamais signale (il est borne) ; un signal hote est differe jusqu'a sa fin.
        log('demarrage garde (start_and_verify.sh du commit)')
        rc, out, _ = self.runner.run('guarded_start', [self.host / 'start_and_verify.sh', '--yes',
                                                       '--guest-shutdown-minutes', guest_minutes, '--handoff-file',
                                                       self.host / 'handoff.json', '--lifecycle-state-file',
                                                       self.host / 'lifecycle.txt', '--guard-mark-dir', self.marks],
                                     timeout=START_TIMEOUT, guard=True, interruptible=False)
        _, generation = read_generation(self.host)
        need(rc == 0 and generation is not None, 'demarrage garde non certifie (code %s)' % rc)
        expiration = re.findall(r'expiration fixe=(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{6}Z)', out)
        need(len(expiration) == 1 and epoch(expiration[0]) > time.time(), 'expiration SSH non certifiee')
        mark = fields((self.marks / 'double_guard_verified').read_text())
        need(mark.get('schema') == 'e-hgp.guard-mark.v1' and mark.get('mark') == 'double_guard_verified' and
             all(mark.get(k) == v for k, v in TARGET.items()) and mark.get('generation') == generation and
             mark.get('max_run_seconds') == str(max_run) and mark.get('guest_shutdown_minutes') == str(guest_minutes),
             'marque double_guard_verified incoherente')
        state.update(start_certified=True, generation=generation, ssh_key_expiration=expiration[0],
                     recovery_command=recovery_command(self.host, generation, context['account']))
        try:
            write_atomic(self.host / 'RECOVERY.txt', recovery_text(self.session, self.host, generation,
                                                                   context['account']))
        except OSError as error:
            state['warnings'].append('RECOVERY.txt non ecrit : %s' % error)
        SIGNALS.check()
        self.common = ['--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--quiet',
                       '--ssh-key-file=%s' % self.key, '--ssh-key-expiration=' + expiration[0]]
        need(self.target_ok('recertify_running'), 'cible non RUNNING sur la generation demarree')
        # Echeance : l'arret invite D relu sur la VM, borne par l'echeance sure GCE.
        rc, out, _ = self.ssh('guest_schedule', 'sudo -n cat /run/systemd/shutdown/scheduled', timeout=90)
        need(rc == 0, 'arret invite illisible')
        schedule = fields(out)
        need(schedule.get('MODE') == 'poweroff' and re.fullmatch(r'[0-9]{1,18}', schedule.get('USEC', '')),
             'arret invite non programme en poweroff')
        self.deadline = int(schedule['USEC']) // 1_000_000
        need(time.time() + CLOSING_RESERVE + 120 < self.deadline <= epoch(generation) + max_run - 300,
             'echeance invitee hors de la fenetre sure GCE')
        self.worker_deadline = self.deadline - CLOSING_RESERVE
        state.update(guest_deadline_epoch=self.deadline, worker_deadline_epoch=self.worker_deadline,
                     ssh_cutoff_epoch=self.deadline - SSH_CUTOFF,
                     guest_schedule={'MODE': schedule['MODE'], 'USEC': schedule['USEC']})
        # Repertoire distant prive, HORS /tmp ; les parties regenerables des sessions precedentes sont elaguees.
        rc, out, _ = self.ssh('remote_mkdir', 'umask 077; mkdir -p "$HOME/ehgp-v10" && chmod 700 "$HOME/ehgp-v10" && '
                                              'for o in "$HOME"/ehgp-v10/ehgp-v10.*; do if [ -d "$o" ] && [ ! -L "$o" ]; '
                                              'then rm -rf "$o/build" "$o/src" "$o/data" "$o/joblib" "$o/pyuser" '
                                              '"$o/overflow" "$o/package.tar.gz" "$o/salvage.tar.gz"; if [ -f '
                                              '"$o/results.tar.gz" ]; then rm -rf "$o/results"; fi; fi; '
                                              'done; d=$(mktemp -d "$HOME/ehgp-v10/ehgp-v10.XXXXXXXXXX") && '
                                              'mkdir "$d/data" && printf "DIR=%s\\n" "$d" && '
                                              'df -Pk "$d" | awk \'NR==2 {print "AVAIL_KB=" $4}\' && '
                                              'du -sk "$HOME/ehgp-v10" | awk \'{print "USED_KB=" $1}\'', timeout=90)
        values = fields('\n'.join(line for line in out.splitlines() if re.match(r'(DIR|AVAIL_KB|USED_KB)=', line)))
        need(rc == 0 and REMOTE_DIR_RE.fullmatch(values.get('DIR', '')) and
             re.fullmatch(r'[0-9]+', values.get('AVAIL_KB', '')), 'repertoire distant non cree')
        self.remote = values['DIR']
        state.update(remote_directory=self.remote, remote_used_kb=values.get('USED_KB'))
        need(int(values['AVAIL_KB']) * 1024 >= context['package_size'] + context['data_total'] + REMOTE_FREE_MARGIN,
             'espace distant insuffisant : %s Kio' % values['AVAIL_KB'])
        instance_path = TARGET['instance'] + ':' + self.remote

        def upload_timeout(size):
            value = min(120 + size // UPLOAD_TIMEOUT_RATE, self.remaining(self.worker_deadline) - 60)
            need(value > 30, 'fenetre epuisee avant le televersement')
            return value
        log('televersement du paquet')
        rc, _, _ = self.scp('upload_package', [self.package_dir / 'package.tar.gz', self.package_dir / 'plan.sh',
                                               self.package_dir / 'plan.json'], instance_path + '/',
                            timeout=upload_timeout(context['package_size']))
        need(rc == 0, 'televersement du paquet en echec')
        pins = [(context['package_sha'], 'package.tar.gz'), (self.report['worker_plan_sha256'], 'plan.sh'),
                (self.report['plan_sha256'], 'plan.json')]
        checks = ['cd ' + shlex.quote(self.remote)]
        checks += ['test "$(sha256sum %s | cut -d " " -f 1)" = %s' % (name, pin) for pin, name in pins]
        checks += ['mkdir -m 700 src', 'tar --no-same-owner --no-same-permissions -xzf package.tar.gz -C src',
                   'test -f src/%s/CMakeLists.txt' % SOURCE_ROOT, 'test -f src/' + WORKER, 'echo UNPACKED']
        rc, out, _ = self.ssh('verify_unpack', 'umask 077; ' + ' && '.join(checks), timeout=300)
        need(rc == 0 and 'UNPACKED' in out.split(), 'hachages ou extraction du paquet en echec sur la VM')
        log('televersement de %d fichier(s) de donnees' % len(context['data_files']))
        sources = [context['data_dir'] / item['name'] for item in context['data_files']]
        rc, _, _ = self.scp('upload_data', sources + [self.package_dir / 'data' / 'SHA256SUMS'], instance_path + '/data/',
                            timeout=upload_timeout(context['data_total']))
        need(rc == 0, 'televersement des donnees en echec')
        count = len(context['data_files'])
        verify = ['cd ' + shlex.quote(self.remote + '/data'),
                  'test "$(sha256sum SHA256SUMS | cut -d " " -f 1)" = ' + self.report['data_manifest_sha256'],
                  'test "$(ls -A | wc -l)" = %d' % (count + 1)]
        if count:
            verify.append('sha256sum -c --strict --quiet SHA256SUMS')
        verify.append('echo DATA_OK')
        rc, out, _ = self.ssh('verify_data', ' && '.join(verify),
                              timeout=min(120 + context['data_total'] // (50 * 2 ** 20),
                                          max(60, self.remaining(self.worker_deadline) - 60)))
        need(rc == 0 and 'DATA_OK' in out.split(), 'sha256 des donnees differents sur la VM')
        state['data_verified_remote'] = True
        # Worker detache ; son PID vient de worker.pid, qu'il ecrit lui-meme.
        need(self.remaining(self.worker_deadline) > 120, 'fenetre epuisee avant le lancement du worker')
        need(self.target_ok('recertify_before_worker', until=self.mono(self.deadline - SSH_CUTOFF - 120)),
             'cible perdue avant le lancement du worker')
        worker_argv = ['bash', self.worker_marker(), '--src', self.remote + '/src', '--data', self.remote + '/data',
                       '--plan', self.remote + '/plan.sh', '--work', self.remote,
                       '--deadline-epoch', str(self.worker_deadline), '--commit', context['commit'],
                       '--plan-sha256', self.report['worker_plan_sha256'], '--package-sha256', context['package_sha'],
                       '--generation', generation]
        pid_file = shlex.quote(self.remote + '/worker.pid')
        launch = ('umask 077; cd %s || exit 1; setsid nohup %s > %s 2>&1 < /dev/null & '
                  'i=0; while [ ! -s %s ] && [ "$i" -lt 100 ]; do sleep 0.1; i=$((i + 1)); done; '
                  'printf "LAUNCHED=%%s\\n" "$(cat %s 2>/dev/null)"' % (
                      shlex.quote(self.remote), shlex.join(worker_argv), shlex.quote(self.remote + '/worker.log'),
                      pid_file, pid_file))
        log('lancement du worker (echeance %d)' % self.worker_deadline)
        rc, out, _ = self.ssh('launch_worker', launch, timeout=90)
        match = re.search(r'^LAUNCHED=([1-9][0-9]*)$', out, re.M)
        if rc != 0 or not match:
            # Le SSH a pu casser apres le demarrage du worker : relire worker.pid (identite verifiee).
            need(self.target_ok('recertify_after_launch_failure', until=self.mono(self.deadline - SSH_CUTOFF - 120)),
                 'lancement du worker en echec, cible perdue')
            probe = ('p=$(cat %s 2>/dev/null); if [ -n "$p" ] && [ -r /proc/$p/cmdline ] && tr "\\0" " " < '
                     '/proc/$p/cmdline | grep -qF -- %s; then printf "LAUNCHED=%%s\\n" "$p"; fi') % (
                         pid_file, shlex.quote(self.worker_marker()))
            rc, out, _ = self.ssh('launch_probe', probe, timeout=90)
            match = re.search(r'^LAUNCHED=([1-9][0-9]*)$', out, re.M)
            need(rc == 0 and match, 'lancement du worker en echec')
            state['worker_launch_recovered'] = True
        self.worker_pid = int(match.group(1))
        state.update(worker_launched=True, worker_pid=self.worker_pid)
        # Sondage long : un SSH attend jusqu'a --poll-window s, puis cible et disque local sont recertifies.
        # Il se termine au plus tard a W + HOST_WAIT_GRACE = D - SSH_CUTOFF - RETRIEVE_BUDGET.
        limit = min(self.worker_deadline + HOST_WAIT_GRACE, self.deadline - SSH_CUTOFF - RETRIEVE_BUDGET)
        while True:
            SIGNALS.check()
            if self.local_disk_low():
                state['worker_outcome'] = 'local_disk_low'
                state['errors'].append('disque de session (< %d o) ou dossier d\'execution (< %d o) presque plein : '
                                       'fermeture anticipee' % (SESSION_FREE_FLOOR, RUN_DIR_FLOOR))
                break
            budget = self.remaining(limit)
            if budget <= 0:
                state['worker_outcome'] = 'overdue'
                break
            kind, code = self.wait_poll(max(1, min(self.args.poll_window, budget)))
            if kind == 'exit':
                state.update(worker_outcome='exited', worker_exit_code=code)
                break
            if kind == 'dead':
                state['worker_outcome'] = 'died_without_exit_file'
                break
            if not self.target_ok('recertify_poll', until=self.mono(self.deadline - SSH_CUTOFF - 60)):
                state['worker_outcome'] = 'target_lost'
                break
            if kind == 'unknown':
                time.sleep(self.args.poll_seconds)
        log('worker : %s (code %s)' % (state['worker_outcome'], state['worker_exit_code']))

    def retrieve_phase1(self):
        """VM allumee : borne a RETRIEVE_BUDGET ET a D - SSH_CUTOFF (avant /run/nologin), recertifications
        comprises ; VM recertifiee avant chaque etape, taille bornee, espace local verifie."""
        state = self.state
        end = time.monotonic() + RETRIEVE_BUDGET
        if self.deadline is not None:
            end = min(end, self.mono(self.deadline - SSH_CUTOFF))

        def window(cap):
            value = min(cap, end - time.monotonic())
            need(value > 5, 'fenetre de rapatriement epuisee (avant /run/nologin)')
            return value
        cap = self.context['plan']['results_cap_bytes']
        instance_path = TARGET['instance'] + ':' + self.remote
        need(self.target_ok('recertify_before_retrieve', until=end), 'cible perdue avant le rapatriement')
        rc, out, _ = self.ssh('guest_schedule_reread', 'sudo -n cat /run/systemd/shutdown/scheduled', timeout=window(60))
        try:
            reread = fields(out) if rc == 0 else {}
        except Refusal:
            reread = {}
        state['guest_guard_intact'] = ({k: reread.get(k) for k in ('MODE', 'USEC')} == state.get('guest_schedule'))
        if not state['guest_guard_intact']:
            state['errors'].append('garde invitee modifiee ou illisible avant le rapatriement : %r' % reread)
        rc, _, _ = self.scp('download_worker_log', [instance_path + '/worker.log'], str(self.results_dir) + '/',
                            timeout=window(60))
        state['worker_log_retrieved'] = rc == 0
        if self.worker_pid is None:
            return
        if state['worker_exit_code'] is None:
            need(self.target_ok('recertify_before_abort', until=end), 'cible perdue avant l\'abandon du worker')
            kind, code = self.wait_poll(0, timeout=window(60))
            if kind == 'alive':
                abort = ('p=%d; if [ -r /proc/$p/cmdline ] && tr "\\0" " " < /proc/$p/cmdline | grep -qF -- %s; '
                         'then kill -TERM "$p" && echo SENT; else echo GONE; fi') % (
                             self.worker_pid, shlex.quote(self.worker_marker()))
                self.ssh('abort_worker', abort, timeout=window(60))
                state['worker_aborted'] = True
                grace = int(min(ABORT_GRACE_MAX, end - time.monotonic() - MIN_SALVAGE_WINDOW))
                if grace > 5:
                    kind, code = self.wait_poll(grace, timeout=window(grace + 60))
            if kind == 'exit':
                state['worker_outcome_before_retrieve'] = state['worker_outcome']
                state.update(worker_outcome='exited', worker_exit_code=code)
        need(self.target_ok('recertify_before_results', until=end), 'cible perdue avant la lecture des resultats')
        quoted = shlex.quote(self.remote)
        name, size, digest = 'results.tar.gz', None, None
        if state['worker_exit_code'] is not None:
            rc, out, _ = self.ssh('results_hash', 'cd %s && cat SHA256SUMS && sha256sum results.tar.gz && '
                                                  'stat -c "SIZE=%%s" results.tar.gz' % quoted, timeout=window(90))
            digests = re.findall(r'^([0-9a-f]{64})  results\.tar\.gz$', out, re.M)
            sizes = re.findall(r'^SIZE=([0-9]+)$', out, re.M)
            if rc == 0 and len(digests) == 2 and digests[0] == digests[1] and len(sizes) == 1:
                digest, size = digests[0], int(sizes[0])
        if digest is None:
            name = 'salvage.tar.gz'
            rc, out, _ = self.ssh('salvage', 'cd %s && test -d results && tar -czf salvage.tar.gz results && '
                                             'sha256sum salvage.tar.gz && stat -c "SIZE=%%s" salvage.tar.gz' % quoted,
                                  timeout=window(180))
            digests = re.findall(r'^([0-9a-f]{64})  salvage\.tar\.gz$', out, re.M)
            sizes = re.findall(r'^SIZE=([0-9]+)$', out, re.M)
            need(rc == 0 and len(digests) == 1 and len(sizes) == 1, 'aucune archive de resultats sur la VM')
            digest, size = digests[0], int(sizes[0])
        state.update(results_archive='worker' if name == 'results.tar.gz' else 'salvage', results_bytes=size)
        need(size <= cap + results_slack(cap),
             'archive de %d octets au-dela du plafond : laissee sur la VM (%s)' % (size, self.remote))
        free = shutil.disk_usage(self.session).free
        need(free >= 2 * size + LOCAL_FREE_MARGIN, 'espace local insuffisant pour %d octets : %d libres' % (size, free))
        need(self.target_ok('recertify_before_download', until=end), 'cible perdue avant le telechargement')
        sources = [instance_path + '/' + name] + ([instance_path + '/SHA256SUMS'] if name == 'results.tar.gz' else [])
        rc, _, _ = self.scp('download', sources, str(self.results_dir) + '/', timeout=window(600))
        need(rc == 0 and (self.results_dir / name).is_file(), 'telechargement des resultats en echec')
        for path in self.results_dir.iterdir():
            if path.is_file():
                os.chmod(path, 0o600)
        local = sha_file(self.results_dir / name)
        need(local == digest, 'hachage local de %s different du hachage distant' % name)
        if name == 'results.tar.gz':
            need((self.results_dir / 'SHA256SUMS').read_text().split() == [local, name], 'SHA256SUMS rapatrie incoherent')
        state['results_sha256'] = local
        self.archive = name

    def retrieve_phase2(self):
        """Apres l'arret : taille DECOMPRESSEE bornee avant toute extraction, extraction sure, manifeste,
        provenance recoupee avec la session."""
        state, context = self.state, self.context
        cap = context['plan']['results_cap_bytes']
        expanded = archive_file_bytes(self.results_dir / self.archive)
        state['results_expanded_bytes'] = expanded
        need(expanded <= cap + results_slack(cap),
             'extraction refusee : %d octets decompresses au-dela du plafond (%d) ; archive conservee' % (expanded, cap))
        free = shutil.disk_usage(self.session).free
        need(free >= 2 * expanded + LOCAL_FREE_MARGIN,
             'extraction refusee : %d octets libres pour %d octets decompresses' % (free, expanded))
        state['results_skipped_members'] = safe_extract(self.results_dir / self.archive,
                                                        self.results_dir / 'extracted')
        extracted = self.results_dir / 'extracted' / 'results'
        worker, commands = parse_results(extracted)
        if self.archive != 'results.tar.gz':
            state.update(unverified_worker=worker, unverified_commands=commands)
            return
        state.update(worker=worker, commands=commands)
        state['overflow'] = {'evicted': read_lines(extracted / 'overflow.txt'),
                             'truncated_streams': read_lines(extracted / 'truncated.txt')}
        state['results_manifest_files'] = verify_manifest(extracted)
        need(worker.get('schema') == WORKER_RESULT_SCHEMA, 'worker.txt : schema inattendu')
        need(worker.get('commit') == context['commit'] and worker.get('plan_sha256') == state['worker_plan_sha256'] and
             worker.get('package_sha256') == context['package_sha'] and worker.get('generation') == state['generation'],
             'provenance du worker (commit, plan, paquet, generation) differente de la session')
        if worker.get('fatal'):
            state['errors'].append('refus du worker : %s' % worker['fatal'])
            return
        need(sha_file(extracted / 'plan.sh') == state['worker_plan_sha256'], 'plan.sh rapatrie different du plan rendu')
        need([row.get('name') for row in commands] == [c['name'] for c in context['plan']['commands']],
             'commandes rapportees differentes du plan')
        binaries = {}
        for line in read_lines(extracted / 'provenance' / 'binaries.sha256', 1000):
            digest, _, path = line.partition('  ')
            binaries[path.lstrip('./')] = digest
        state['provenance'] = {'binaries_sha256': binaries,
                               'compiler': read_lines(extracted / 'provenance' / 'compiler.txt'),
                               'cmakecache': read_lines(extracted / 'provenance' / 'cmakecache.txt')}
        state['results_verified'] = not state['results_skipped_members']

    def remove_key(self):
        """Toujours, des qu'un ajout a pu avoir lieu : retrait OS Login (mutation OS Login seule), puis la cle
        privee est effacee. Aucun SSH ne suit."""
        state = self.state
        if state['oslogin_key_maybe_added']:
            rc, _, _ = self.runner.run('oslogin_remove', [self.gcloud, 'compute', 'os-login', 'ssh-keys', 'remove',
                                                          '--project=' + TARGET['project'],
                                                          '--key-file=%s.pub' % self.key, '--quiet'],
                                       timeout=120, critical=True, interruptible=False)
            state['oslogin_key_removed'] = rc == 0
            if rc != 0:
                state['warnings'].append('retrait OS Login en echec : la cle expire d\'elle-meme (%d min)' %
                                         self.context['ttl_minutes'])
        try:
            self.key.unlink()
            state['private_key_deleted'] = True
        except FileNotFoundError:
            state['private_key_deleted'] = True
        except OSError as error:
            state['warnings'].append('cle privee non effacee : %s' % error)

    def finalize(self):
        state = self.state
        if SIGNALS.received:
            state['host_signals'] = list(SIGNALS.received)
        # 1. Rapatriement VM allumee : jamais apres une interruption hote, une cible perdue ou un disque plein.
        if self.remote is not None and self.common is not None:
            if SIGNALS.received:
                state['retrieval'] = 'skipped_host_interrupted'
            elif state.get('target_lost'):
                state['retrieval'] = 'skipped_target_lost'
            elif state.get('worker_outcome') == 'local_disk_low' or self.local_disk_low():
                state['retrieval'] = 'skipped_local_disk_low'
            else:
                try:
                    try:
                        SIGNALS.raised, SIGNALS.interruptible = False, True
                        self.retrieve_phase1()
                    finally:
                        SIGNALS.interruptible = False
                    state['retrieval'] = 'downloaded'
                except BaseException as error:  # noqa: B902 -- l'arret suit toujours
                    SIGNALS.interruptible = False
                    state['retrieval'] = 'failed'
                    state['errors'].append('rapatriement : %s: %s' % (type(error).__name__, error))
        # 2. Arret cible, des la premiere mutation GCP (reserve liberee juste avant).
        if state['gcp_mutation_phase_entered']:
            try:
                close_by_generation(state, self.runner, self.host, self.gcloud, self.context['pre_start'],
                                    release_reserve=self.release_reserve, account=self.context['account'])
            except BaseException as error:  # noqa: B902
                state['errors'].append('arret : %s: %s' % (type(error).__name__, error))
        self.release_reserve()
        try:
            self.remove_key()
        except BaseException as error:  # noqa: B902
            state['warnings'].append('retrait de cle : %s: %s' % (type(error).__name__, error))
        # 3. Extraction bornee et verification locales, VM arretee.
        if self.archive is not None:
            try:
                self.retrieve_phase2()
            except BaseException as error:  # noqa: B902
                state['errors'].append('verification des resultats : %s: %s' % (type(error).__name__, error))
        if SIGNALS.received:
            state['host_signals'] = list(SIGNALS.received)
            state['errors'].append('signal(aux) hote recu(s) : %s' % SIGNALS.received)
        worker = state.get('worker') or {}
        if worker or state.get('commands'):
            state['remote_summary'] = {
                'worker_status': worker.get('status'), 'worker_fatal': worker.get('fatal') or None,
                'worker_pip': worker.get('pip'), 'worker_build': worker.get('build'), 'worker_data': worker.get('data'),
                'overflow_files': worker.get('overflow_files'), 'truncated_streams': worker.get('truncated_streams'),
                'overflow_unresolved': worker.get('overflow_unresolved'), 'overflow': state.get('overflow'),
                'failed_commands': [[r.get('name'), r.get('status'), r.get('exit_code')]
                                    for r in state.get('commands', []) if r.get('status') != 'ok']}
        state['host_commands'] = self.runner.rows if self.runner is not None else []
        state['console_write_failures'] = CONSOLE.failures
        rows = state.get('commands', [])
        remote_ok = (state['worker_outcome'] == 'exited' and state['worker_exit_code'] == 0 and
                     not state.get('worker_aborted') and state['results_verified'] and
                     worker.get('status') == 'completed' and worker.get('overflow_files') == '0' and
                     worker.get('truncated_streams') == '0' and
                     len(rows) == len(self.context['plan']['commands']) and all(r.get('status') == 'ok' for r in rows))
        status = None if not state['gcp_mutation_phase_entered'] else closure_status(state)
        if status is None:
            if not state['start_certified']:
                status = 'failed_before_start'
            elif remote_ok and not state['errors']:
                status = 'completed'
            else:
                status = 'failed_remote'
        state['status'] = status
        state['receipt_path'] = str(self.session / 'receipt.json')
        written = write_with_fallback(self.session / 'receipt.json', self.run_dir / 'receipt.json', json_bytes(state))
        if written != state['receipt_path']:
            state['receipt_path'] = written
            if written is None:
                log('ecriture du recu impossible (session et dossier d\'execution)')
            else:
                write_with_fallback(Path(written), Path(written), json_bytes(state))
        if status in ('shutdown_uncertified', 'foreign_generation_active'):
            log('%s : %s' % (status.upper(), state.get('recovery') or state.get('recovery_command')))
        return EXIT_CODES[status]

    def run(self):
        try:
            self.prepare()
        except (Refusal, OSError, subprocess.SubprocessError) as error:
            self.state.update(status='failed_before_start', errors=['preparation locale : %s' % error])
            write_with_fallback(self.session / 'receipt.json', self.run_dir / 'receipt.json', json_bytes(self.state))
            return EXIT_CODES['failed_before_start']
        SIGNALS.install()
        try:
            try:
                SIGNALS.interruptible = True
                self.gcp_phase()
            finally:
                SIGNALS.interruptible = False
        except BaseException as error:  # noqa: B902 -- tout echec passe par la fermeture
            SIGNALS.interruptible = False
            self.state['errors'].append('%s: %s' % (type(error).__name__, error))
        try:
            return self.finalize()
        except BaseException as error:  # noqa: B902 -- defensif : finalize ne doit jamais s'echapper
            self.state['errors'].append('finalize : %s: %s' % (type(error).__name__, error))
            self.state['status'] = 'shutdown_uncertified'
            write_with_fallback(self.session / 'receipt.json', self.run_dir / 'receipt.json', json_bytes(self.state))
            return EXIT_CODES['shutdown_uncertified']


def summary_of(state, session):
    value = {key: state.get(key) for key in (
        'status', 'commit', 'generation', 'closure', 'closing_generation', 'targeted_shutdown_certified',
        'gcp_mutation_phase_entered', 'start_may_have_been_requested', 'worker_outcome', 'worker_exit_code',
        'results_verified', 'remote_summary', 'errors', 'recovery', 'recovery_command', 'receipt_path',
        'public_status')}
    value.update(session=str(session),
                 commands=[{k: row.get(k) for k in ('name', 'status', 'exit_code', 'wall_seconds', 'max_rss_kb')}
                           for row in state.get('commands', [])])
    return value


def read_done(session):
    for path in (Path(session) / 'DONE', run_directory(session) / 'DONE'):
        try:
            return int(path.read_text().strip()), path
        except (OSError, ValueError):
            continue
    return None, None


def read_receipt(session):
    for path in (Path(session) / 'receipt.json', run_directory(session) / 'receipt.json'):
        try:
            return strict_json(path.read_bytes())
        except (OSError, Refusal):
            continue
    return None


# ---------------------------------------------------------------------------------------------
# Lanceur, attente, reprise.

def launch(args, report, context, argv):
    session = context['session']
    session.mkdir(mode=0o700)
    os.chmod(session, 0o700)
    write_new(session / 'session.lock', b'')
    run_dir = make_run_directory(session)
    for stale in ('DONE', 'receipt.json', 'reserve.bin'):   # dossier d'execution reutilise : rien de perime
        try:
            (run_dir / stale).unlink()
        except FileNotFoundError:
            pass
    child_argv = [sys.executable, str(Path(__file__).resolve())] + [a for a in argv if a != '--wait'] + ['--child']
    with open(session / 'session.stdout', 'wb') as out, open(session / 'session.stderr', 'wb') as err:
        process = subprocess.Popen(child_argv, stdin=subprocess.DEVNULL, stdout=out, stderr=err,
                                   start_new_session=True, close_fds=True)
    info = {'status': 'launched', 'pid': process.pid, 'session': str(session), 'sentinel': str(session / 'DONE'),
            'receipt': str(session / 'receipt.json'), 'stderr': str(session / 'session.stderr'),
            'run_dir': str(run_dir), 'recover': 'python3 %s --recover --session-dir %s' % (CONTROLLER, session),
            'started_utc': utc_now()}
    write_new(session / 'launch.json', json_bytes(info))
    log('session lancee (PID %d) : suivre %s ; fin quand %s existe' % (process.pid, info['stderr'], info['sentinel']))
    if not args.wait:
        CONSOLE.emit(info)
        return 0
    while read_done(session)[0] is None:
        if process.poll() is not None:
            time.sleep(1)
            if read_done(session)[0] is not None:
                break
            code = process.returncode
            if code in EXIT_CODES.values():
                # DONE n'a pas pu etre ecrit (disque plein...) mais l'enfant a rendu un statut connu.
                log('sentinelle DONE absente ; code reel du processus de session : %d' % code)
                receipt = read_receipt(session)
                CONSOLE.emit(dict(summary_of(receipt, session) if receipt else info, done_missing=True,
                                  exit_code=code))
                return code
            log('processus de session mort sans sentinelle (code %s) : NE RIEN relancer ; attendre qu\'aucun '
                'processus ne reference %s (ps), puis « %s » (il le verifie : code %d sinon).' % (
                    code, session, info['recover'], SESSION_ALIVE_CODE))
            CONSOLE.emit(dict(info, status='child_died_without_sentinel', exit_code=code))
            return CHILD_DIED_CODE
        time.sleep(min(2.0, max(0.1, args.poll_seconds)))
    process.wait()
    code, _ = read_done(session)
    receipt = read_receipt(session)
    CONSOLE.emit(summary_of(receipt, session) if receipt else dict(info, status='receipt_unreadable', exit_code=code))
    return code


def recover(args):
    """Reprise : exclusion (verrou, puis aucun processus capable de muter la VM : processus de session ou
    script garde de la session ; les observateurs sont ignores et journalises), puis fermeture par la
    generation gravee, avec le meme describe prealable que la session ; jamais de generation devinee."""
    session = Path(args.session_dir).absolute()
    host = session / 'host'
    gcloud = Path(args.gcloud)
    if not session.is_dir():
        CONSOLE.emit({'status': 'session_absente', 'session': str(session),
                      'recovery': 'dossier de session introuvable : verifier le chemin ; rien n\'a ete fait'})
        return EXIT_CODES['failed_before_start']
    state = {'schema': 'ehgp.v10.recovery_receipt.v2', 'session': str(session), 'target': TARGET,
             'generation': None, 'targeted_shutdown_certified': False, 'errors': [], 'warnings': [],
             'started_utc': utc_now()}
    SIGNALS.install()
    lock = None
    code = EXIT_CODES['shutdown_uncertified']
    try:
        try:
            run_dir = make_run_directory(session)
        except OSError as error:
            run_dir = run_directory(session)
            state['warnings'].append('dossier d\'execution non cree : %s' % error)
        lock = take_lock(session / 'session.lock', args.recover_wait, create=False)
        end = time.monotonic() + args.recover_wait
        alive, ignored = session_processes(session)
        while alive and time.monotonic() < end:
            time.sleep(1)
            alive, ignored = session_processes(session)
        state['ignored_processes'] = ignored
        if lock is None or alive:
            state.update(status='session_alive', processes=alive, lock_held=lock is None,
                         recovery='le processus de session ou un script garde de la session vit encore : attendre '
                                  'sa fin, puis relancer --recover. Rien n\'a ete arrete.')
            code = SESSION_ALIVE_CODE
        elif not (host / 'start_and_verify.sh').exists():
            state.update(closure='no_start_requested', recovery='aucun demarrage n\'a pu etre lance : rien a arreter')
            code = 0
        else:
            for name, pin in GUARD_PINS.items():
                need(sha_file(host / Path(name).name) == pin, 'script garde de la session different de son epingle')
            preflight_report = {}
            try:
                preflight_report = strict_json((session / 'preflight.json').read_bytes())
            except (OSError, Refusal):
                state['warnings'].append('preflight.json illisible')
            account = preflight_report.get('gcloud_account')
            runner = Runner(run_dir / 'logs', runner_env(gcloud, account=account),
                            prefix='recover_%d_' % int(time.time()),
                            mirror=host / 'logs' if (host / 'logs').is_dir() else None)

            def release():
                for reserve in (host / 'reserve.bin', run_dir / 'reserve.bin'):
                    try:
                        reserve.unlink()
                    except OSError:
                        pass
            close_by_generation(state, runner, host, gcloud, preflight_report.get('pre_start_generation'),
                                release_reserve=release, account=account)
            if (session / 'key.pub').exists():
                rc, _, _ = runner.run('oslogin_remove', [gcloud, 'compute', 'os-login', 'ssh-keys', 'remove',
                                                         '--project=' + TARGET['project'],
                                                         '--key-file=%s' % (session / 'key.pub'), '--quiet'],
                                      timeout=120, critical=True, interruptible=False)
                state['oslogin_key_removed'] = rc == 0
                try:
                    (session / 'key').unlink()
                except OSError:
                    pass
            state['host_commands'] = runner.rows
            status = closure_status(state)
            code = 0 if status in (None, 'failed_before_start') else EXIT_CODES[status]
            state['status'] = status or 'stopped'
    except BaseException as error:  # noqa: B902
        state['errors'].append('%s: %s' % (type(error).__name__, error))
        state.setdefault('status', 'recovery_failed')
        code = EXIT_CODES['shutdown_uncertified']
    finally:
        if lock is not None and lock >= 0:
            os.close(lock)
    name = 'recovery_%s.json' % utc_now().replace(':', '')
    state['recovery_receipt'] = write_with_fallback(session / name, run_directory(session) / name, json_bytes(state))
    CONSOLE.emit({k: state.get(k) for k in ('status', 'closure', 'closing_generation', 'targeted_shutdown_certified',
                                             'processes', 'ignored_processes', 'recovery', 'recovery_command',
                                             'errors', 'recovery_receipt')})
    return code


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--commit', help='SHA du commit pousse sur origin/main')
    parser.add_argument('--plan', help='plan JSON (schema %s)' % PLAN_SCHEMA)
    parser.add_argument('--data', help='dossier plat des donnees (.u32le), jamais versionnees')
    parser.add_argument('--session-dir', help='dossier neuf, enfant direct de ' + DEFAULT_SESSIONS_ROOT)
    parser.add_argument('--max-run-seconds', type=int, help='maxRunDuration GCE exige (egal a celui de la VM)')
    parser.add_argument('--execute', action='store_true', help='lancer la session (detachee) ; sinon dry-run')
    parser.add_argument('--wait', action='store_true', help='avec --execute : attendre la sentinelle DONE')
    parser.add_argument('--recover', action='store_true',
                        help='reprise : arret cible par la generation gravee dans --session-dir/host')
    parser.add_argument('--gcloud', default=DEFAULT_GCLOUD, help=argparse.SUPPRESS)
    parser.add_argument('--sessions-root', default=DEFAULT_SESSIONS_ROOT, help=argparse.SUPPRESS)
    parser.add_argument('--poll-seconds', type=float, default=5.0, help=argparse.SUPPRESS)
    parser.add_argument('--poll-window', type=int, default=600, help=argparse.SUPPRESS)
    parser.add_argument('--recover-wait', type=float, default=RECOVER_WAIT, help=argparse.SUPPRESS)
    parser.add_argument('--no-fetch', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--child', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    args.gcloud = str(Path(args.gcloud).absolute())
    if args.recover:
        if not args.session_dir:
            parser.error('--recover exige --session-dir')
        return recover(args)
    missing = [name for name in ('commit', 'plan', 'data', 'session_dir', 'max_run_seconds')
               if getattr(args, name) is None]
    if missing:
        parser.error('arguments obligatoires : ' + ', '.join('--' + m.replace('_', '-') for m in missing))
    report = {'schema': 'ehgp.v10.session_preflight.v3', 'mode': 'execute' if args.execute else 'dry_run',
              'target': TARGET, 'backend': 'reference_cpu', 'public_status': 'not_claimed', 'gcp_mutations': 'none'}
    child = args.child and args.execute
    session = Path(args.session_dir)
    root_lock_path = Path(args.sessions_root) / '.ehgp-v10.lock'
    if child:
        lock = take_lock(session / 'session.lock') if session.is_dir() else None
        if lock is None:
            CONSOLE.emit(dict(report, status='refused', reason='session deja tenue par un autre processus'))
            return EXIT_CODES['failed_before_start']
        # Une seule session v10 a la fois depuis cette racine (la cible est unique) : verrou tenu a vie.
        root_lock = take_lock(root_lock_path) if root_lock_path.parent.is_dir() else None
        if root_lock is None:
            CONSOLE.emit(dict(report, status='refused', reason='une autre session v10 est en cours (verrou %s)' %
                              root_lock_path))
            return EXIT_CODES['failed_before_start']
    elif args.execute and root_lock_path.parent.is_dir():
        probe = take_lock(root_lock_path, create=False)   # sonde seulement : un refus ne cree aucun fichier
        if probe is None:
            CONSOLE.emit(dict(report, status='refused', reason='une autre session v10 est en cours (verrou %s)' %
                              root_lock_path))
            return EXIT_CODES['failed_before_start']
        if probe >= 0:
            os.close(probe)
    code = EXIT_CODES['failed_before_start']
    run_dir = None
    try:
        with tempfile.TemporaryDirectory(prefix='ehgp-v10-package-') as scratch:
            try:
                context = preflight(args, report, scratch, child)
            except (Refusal, OSError, subprocess.SubprocessError) as error:
                report.update(status='refused' if not args.execute else 'failed_before_start', reason=str(error))
                if child:
                    run_dir = make_run_directory(session)
                    write_with_fallback(session / 'receipt.json', run_dir / 'receipt.json',
                                        json_bytes(dict(report, errors=[str(error)])))
                CONSOLE.emit(report)
                return code
            if not args.execute:
                report.update(status='dry_run_ok', steps=planned_steps(context))
                CONSOLE.emit(report)
                code = 0
                return code
            if not child:
                code = launch(args, report, context, argv)
                return code
            run_dir = make_run_directory(session)
            code = EXIT_CODES['shutdown_uncertified']   # prudent tant que la session n'a pas rendu son code
            code = Session(context, args, report, run_dir).run()
            receipt = read_receipt(session)
            if receipt is not None:
                CONSOLE.emit(summary_of(receipt, session))
            return code
    finally:
        if child:
            write_with_fallback(session / 'DONE', run_directory(session) / 'DONE', '%d\n' % code)


if __name__ == '__main__':
    sys.exit(main())
