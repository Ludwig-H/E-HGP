#!/usr/bin/env python3
"""Session G4 SPOT gardee et minimale pour morsehgp3D_v12 (reference_cpu). Inerte sans --execute.

  python3 gcp-migration/v12_session.py (--commit SHA | --snapshot RACINE) --plan PLAN.json \
      --data DOSSIER --session-dir /workspaces/.ehgp-sessions/NOM --max-run-seconds S [--execute [--wait]]
  python3 gcp-migration/v12_session.py --recover --session-dir /workspaces/.ehgp-sessions/NOM

Port explicite de gcp-migration/v11_session.py (main f31845d16), lui-meme port de v10_session.py : le
cycle de vie, les gardes, les echeances, les codes de sortie, la reprise et les epingles sont INCHANGES.
Les differences avec la v11 se limitent a la lignee (morsehgp3D_v12, mhgp12*, $HOME/ehgp-v12/, schemas
ehgp.v12.*, sessions nommees v12.<date>.<objet>) et a un compte rendu sans effet sur les gardes : le recu
n'ecrit plus le chemin personnel de la VM (le $HOME du compte OS Login, derive de l'adresse du compte) mais
le texte litteral $HOME, et il dit quels fichiers rapatries portent encore une identite
(results_identity_scan : chemins relatifs, jamais les valeurs). Le cache de donnees persistant de la VM ne
change pas le controleur : le worker exporte MHGP12_CACHE_DIR ($HOME/ehgp-v12/cache, jamais elague) et un
script du paquet le gere (morsehgp3D_v12/bench/data_cache.py). Le VERROU de la VM reste le fichier de la
v10 (sessions_root/.ehgp-v10.lock) : il n'y a qu'une VM, les sessions v10, v11 et v12 s'excluent.
L'adresse du compte gcloud reste dans preflight.json, RECOVERY.txt et recovery_command : elle fige le
compte de l'arret et de la reprise (garde de la revue adverse 2), et son retrait attend une revue.

Deux modes de source :
  --commit SHA       commit pousse sur origin/main, controleur execute identique a celui du commit ;
                     seul mode qui produit un RECU (source_kind = commit, evidence_grade = pushed_commit).
  --snapshot RACINE  instantane de l'arbre de travail RACINE (celui du controleur execute) : fichiers
                     reguliers de RACINE/morsehgp3D_v12, suivis ou non ; exclus : dossiers __pycache__ et
                     build*, fichiers *.pyc, fichiers de plus de 32 Mio ; refuses : liens symboliques,
                     fichiers speciaux, cles privees. Worker et controleur sont ceux de l'arbre de
                     travail (sha256 releves), les scripts gardes egalent toujours leurs epingles.
                     source_kind = worktree_snapshot, evidence_grade = dev_snapshot : essai de
                     developpement, JAMAIS une preuve publiable.

Sans --execute : validation complete (source, protocole epingle, plan, donnees, tailles, espace disque,
budget, cible TERMINATED, OS Login et maxRunDuration par lectures GCP seules) et impression de ce qui
serait fait. Aucune commande GCP mutante, aucun dossier de session ; en mode --commit, seul
`git fetch origin` met a jour refs/remotes/origin et FETCH_HEAD ; en mode --snapshot, aucun fetch.

Avec --execute : la meme validation, puis creation du dossier de session et lancement DETACHE
(nouvelle session Unix, stdout/stderr vers des fichiers, stdin /dev/null) du processus de session ;
le lanceur rend la main aussitot (ou attend la sentinelle DONE avec --wait). C'est l'unique facon
de lancer.

Cycle de vie (repris de tower_session_v9.py, sans exec de modules v7/v9) : cle ed25519 de session
inscrite dans OS Login avec une expiration bornee -> start_and_verify.sh de la source (epingle, jamais
signale) -> recertification RUNNING + generation -> televersement du paquet puis des donnees (sha256
des deux cotes) -> worker detache v12_worker.sh, sonde avec recertification de la cible a chaque
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
import gzip
import hashlib
import io
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
DEFAULT_TARGET = {'project': 'devpod-gpu-exploration', 'zone': 'us-central1-b',
                  'instance': 'ehgp-v7-4fa0e0789a7d5bb06b787d35'}
TARGET = dict(DEFAULT_TARGET)  # Configuration figee par main avant tout appel cloud du processus.
MACHINE_TYPE = 'g4-standard-48'
DEFAULT_GCLOUD = '/home/codespace/google-cloud-sdk/bin/gcloud'
DEFAULT_SESSIONS_ROOT = '/workspaces/.ehgp-sessions'
SOURCE_ROOT = 'morsehgp3D_v12'
CONTROLLER = 'gcp-migration/v12_session.py'
WORKER = 'gcp-migration/v12_worker.sh'
START_GUARD = 'gcp-migration/start_and_verify.sh'
STOP_GUARD = 'gcp-migration/stop_and_verify.sh'
# Scripts gardes epingles (memes epingles que full_probe_session_v7.py / tower_session_v9.py) :
# toute modification au commit exige une revue et une nouvelle epingle ici.
GUARD_PINS = {START_GUARD: '73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0',
              STOP_GUARD: 'ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c'}
PLAN_SCHEMA = 'ehgp.v12.session_plan.v1'
WORKER_PLAN_MAGIC = '# ehgp.v12.worker_plan.v1'
WORKER_RESULT_SCHEMA = 'ehgp.v12.worker_result.v1'
RECEIPT_SCHEMA = 'ehgp.v12.session_receipt.v1'
PYTHON_PINS = {'numpy': '2.2.6', 'scipy': '1.15.3', 'scikit-learn': '1.7.2', 'hdbscan': '0.8.44'}
PYTHON_PACKAGES = ('none', 'pinned')   # plan : none = Python nu de la VM (defaut) ; pinned = PYTHON_PINS
COMPARABILITY = ('Versions effectives relevees dans env/vm_facts.txt et la provenance des builds ; aucune '
                 'version de l\'ancienne VM n\'est supposee sur une nouvelle cible. Python nu sauf plan '
                 'python_packages = pinned. Temps et versions NON comparables entre VM et codespace.')
# Verrou de la VM : le MEME fichier que v10_session.py et v11_session.py, volontairement (une seule VM pour
# toutes les lignees : une session v10, v11 ou v12 exclut les deux autres).
ROOT_LOCK_NAME = '.ehgp-v10.lock'
# Source du paquet : un commit pousse (recu) ou un instantane de l'arbre de travail (essai de developpement).
EVIDENCE_GRADES = {'commit': 'pushed_commit', 'worktree_snapshot': 'dev_snapshot'}
SNAPSHOT_MAX_FILE_BYTES = 32 * 2 ** 20   # un fichier plus gros est exclu de l'instantane, et liste
SNAPSHOT_MAX_FILES = 20000               # au-dela : refus (ce n'est plus un arbre de sources)
SNAPSHOT_MAX_BYTES = 2 ** 30             # octets non compresses de l'instantane ; au-dela : refus
SNAPSHOT_MTIME = 1577836800              # 2020-01-01T00:00:00Z : dates figees, meme arbre => meme paquet
SNAPSHOT_EXCLUDED_LISTED = 200           # exclusions listees dans le rapport (le compte reste exact)

MIN_RUN_SECONDS, MAX_RUN_SECONDS = 30, 28800
GCE_GUARD_OVERHEAD = 900       # 300 (reserve GCE) + 120 (systemd) + 480 (armement) : start_and_verify.sh
MIN_GUEST_MINUTES = 5
NOLOGIN_LEAD = 300             # shutdown -P cree /run/nologin 5 min avant l'arret invite D (pam_nologin)
SSH_CUTOFF = NOLOGIN_LEAD + 60  # aucun SSH/SCP ne part apres D - 360
RETRIEVE_BUDGET = 240          # rapatriement complet (recertifications comprises), borne par D - SSH_CUTOFF
HOST_WAIT_GRACE = 60           # attente au-dela de l'echeance du worker
CLOSING_RESERVE = SSH_CUTOFF + RETRIEVE_BUDGET + HOST_WAIT_GRACE   # echeance du worker = D - 660
WORKER_PACK_RESERVE = 120      # identique a PACK_RESERVE_SECONDS de v12_worker.sh
ABORT_GRACE_MAX = 90           # attente de worker.exit apres SIGTERM
MIN_SALVAGE_WINDOW = 100       # temps garde pour l'archive de secours et le telechargement
SETUP_ESTIMATE = 180
UPLOAD_RATE = 10 * 2 ** 20     # estimation (octets/s) pour le budget
UPLOAD_TIMEOUT_RATE = 2 * 2 ** 20
PIP_TIMEOUT = 600              # identique a PIP_TIMEOUT_SECONDS de v12_worker.sh
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
TARGET_RE = re.compile(r'mhgp12(?:_[A-Za-z0-9_]+)?')   # mhgp12 (le CLI, architecture § 2) ou mhgp12_*
BINARY_RE = re.compile(r'\./(' + TARGET_RE.pattern + r')')
PYTHON_RE = re.compile(r'\{src\}/(' + SOURCE_ROOT + r'/[A-Za-z0-9_./-]+\.py)')
DATA_REF_RE = re.compile(r'\{data\}/([A-Za-z0-9._-]*)')
DATA_NAME_RE = re.compile(r'[A-Za-z0-9][A-Za-z0-9._-]{0,127}')
REMOTE_DIR_RE = re.compile(r'/[A-Za-z0-9._/-]*/ehgp-v12/ehgp-v12\.[A-Za-z0-9]{10}')
SHA_RE = re.compile(r'[0-9a-f]{64}')


class Refusal(Exception):
    """Refus explicite ; le message dit pourquoi."""


class RecoveryBusy(Refusal):
    """Une autre session detient le verrou commun ; aucun appel cloud de reprise."""


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
    return Path(tempfile.gettempdir()) / 'ehgp-v12-runs' / ('%s-%s' % (session.name,
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
        line = '[v12 %s] %s\n' % (utc_now(), message)
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
    """Fichiers suivis de morsehgp3D_v12 au commit ; refuse liens symboliques et sous-modules."""
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


def resolve_snapshot(root):
    """Mode --snapshot : RACINE doit etre l'arbre de travail Git du controleur execute. Rend
    (racine, HEAD, sortie de `git status --porcelain`) ; aucun fetch, aucun verrou Git optionnel."""
    path = Path(root)
    need(path.is_absolute() and path.is_dir() and not path.is_symlink(),
         '--snapshot doit etre un dossier absolu existant, non symbolique')
    top = git('rev-parse', '--show-toplevel').strip()
    need(Path(top).resolve() == REPO and path.resolve() == REPO,
         '--snapshot doit designer l\'arbre de travail du controleur execute : ' + str(REPO))
    head = git('rev-parse', '--verify', '--quiet', 'HEAD^{commit}', check=False)
    need(head.returncode == 0, 'HEAD illisible dans ' + str(REPO))
    status = git('--no-optional-locks', 'status', '--porcelain', binary=True)
    return REPO, head.stdout.decode().strip(), status


def open_beneath(root, path, directory=False):
    """Descripteur de RACINE/chemin, ouvert composant par composant : chaque composant est ouvert relativement
    au descripteur du precedent et sans jamais suivre un lien symbolique (O_NOFOLLOW). Un dossier ou un
    fichier remplace par un lien pendant la lecture est donc refuse, et rien n'est lu hors de RACINE.
    O_NONBLOCK : l'ouverture d'un tube nomme ne bloque pas ; il est refuse ensuite comme non regulier."""
    parts = path.split('/')
    descriptor = None
    try:
        descriptor = os.open(root, os.O_RDONLY | os.O_DIRECTORY)
        for index, part in enumerate(parts):
            leaf = index == len(parts) - 1 and not directory
            child = os.open(part, os.O_RDONLY | os.O_NOFOLLOW | (os.O_NONBLOCK if leaf else os.O_DIRECTORY),
                            dir_fd=descriptor)
            os.close(descriptor)
            descriptor = child
        opened, descriptor = descriptor, None
        return opened
    except OSError as error:
        raise Refusal('chemin illisible ou lien symbolique dans l\'arbre de travail : %s (%s)' % (path, error)) from error
    finally:
        if descriptor is not None:
            os.close(descriptor)


def read_regular(root, path, limit=None):
    """(octets, mode) d'un fichier REGULIER de l'arbre de travail, lu une seule fois ; au plus limit + 1 octets.
    Le type est verifie sur le descripteur, avant toute lecture (dossier, tube, peripherique : refus)."""
    descriptor = open_beneath(root, path)
    try:
        info = os.fstat(descriptor)
        need(stat.S_ISREG(info.st_mode), 'fichier non regulier dans l\'arbre de travail : ' + path)
        stream = os.fdopen(descriptor, 'rb')
    except BaseException:
        os.close(descriptor)
        raise
    with stream:
        return (stream.read() if limit is None else stream.read(limit + 1)), info.st_mode


def snapshot_protocol_file(root, name):
    """Fichier du protocole lu dans l'arbre de travail : regulier, jamais un lien symbolique."""
    return read_regular(root, name)[0]


def snapshot_files(root):
    """Fichiers de l'instantane : (chemins tries, exclusions). Fichiers REGULIERS de RACINE/morsehgp3D_v12,
    suivis ou non. Exclus sans etre parcourus : dossiers __pycache__ et build* ; exclus : fichiers *.pyc et
    fichiers de plus de SNAPSHOT_MAX_FILE_BYTES. Refuses : liens symboliques (y compris vers un dossier,
    y compris nommes build*), fichiers speciaux, noms a caracteres non imprimables. Le parcours se fait par
    descripteurs de dossiers (jamais par chemins) : il ne suit aucun lien, meme pose pendant le parcours."""
    try:
        base = os.lstat(root / SOURCE_ROOT)
    except OSError as error:
        raise Refusal('%s absent de l\'arbre de travail (%s)' % (SOURCE_ROOT, error)) from error
    need(stat.S_ISDIR(base.st_mode), SOURCE_ROOT + ' doit etre un dossier reel (ni lien symbolique, ni fichier)')
    files, excluded = [], []

    def walk(directory):
        descriptor = open_beneath(root, directory, directory=True)
        try:
            for name in sorted(os.listdir(descriptor)):
                path = directory + '/' + name
                need(name.isprintable(), 'nom non imprimable dans l\'instantane : %r' % path)
                info = os.lstat(name, dir_fd=descriptor)
                if stat.S_ISLNK(info.st_mode):
                    raise Refusal('lien symbolique dans l\'instantane : ' + path)
                if stat.S_ISDIR(info.st_mode):
                    if name == '__pycache__' or name.startswith('build'):
                        excluded.append({'path': path + '/', 'reason': 'excluded_directory'})
                    else:
                        walk(path)
                elif not stat.S_ISREG(info.st_mode):
                    raise Refusal('fichier special dans l\'instantane : ' + path)
                elif name.endswith('.pyc'):
                    excluded.append({'path': path, 'reason': 'pyc'})
                elif info.st_size > SNAPSHOT_MAX_FILE_BYTES:
                    excluded.append({'path': path, 'reason': 'larger_than_%d_bytes' % SNAPSHOT_MAX_FILE_BYTES})
                else:
                    files.append(path)
                need(len(files) <= SNAPSHOT_MAX_FILES, 'instantane de plus de %d fichiers' % SNAPSHOT_MAX_FILES)
        finally:
            os.close(descriptor)
    walk(SOURCE_ROOT)
    return sorted(files), sorted(excluded, key=lambda item: item['path'])


def read_snapshot_file(root, path):
    """Octets d'un fichier de l'instantane, lus UNE fois ; None s'il depasse SNAPSHOT_MAX_FILE_BYTES a la
    lecture. Rend (octets, mode 644 ou 755)."""
    data, mode = read_regular(root, path, SNAPSHOT_MAX_FILE_BYTES)
    if len(data) > SNAPSHOT_MAX_FILE_BYTES:
        return None, None
    need(b'PRIVATE KEY-----' not in data, 'cle privee dans l\'instantane : refus (' + path + ')')
    return data, 0o755 if mode & 0o111 else 0o644


def build_snapshot_package(root, worker, directory):
    """Paquet de l'instantane, de meme forme que l'archive d'un commit : morsehgp3D_v12/... et le worker.

    Chaque membre est ecrit depuis les octets lus UNE fois, hache sur ces memes octets : le manifeste
    decrit exactement le paquet, meme si l'arbre change pendant la lecture. Ordre, proprietaire, dates et
    en-tete gzip sont figes : le meme arbre donne le meme paquet. Le paquet est ensuite RELU et chaque
    membre recompare au manifeste. Rend (paquet, sha256, taille, manifeste, exclusions)."""
    names, excluded = snapshot_files(root)
    need(SOURCE_ROOT + '/CMakeLists.txt' in names, SOURCE_ROOT + '/CMakeLists.txt absent de l\'instantane')
    package = Path(directory) / 'package.tar.gz'
    manifest, total = [], 0
    with open(package, 'wb') as raw, \
            gzip.GzipFile(filename='', mode='wb', compresslevel=6, fileobj=raw, mtime=0) as zipped, \
            tarfile.open(fileobj=zipped, mode='w', format=tarfile.PAX_FORMAT) as archive:
        for name in [WORKER] + names:
            data, mode = (worker, 0o644) if name == WORKER else read_snapshot_file(root, name)
            if data is None:
                excluded.append({'path': name, 'reason': 'larger_than_%d_bytes' % SNAPSHOT_MAX_FILE_BYTES})
                continue
            total += len(data)
            need(total <= SNAPSHOT_MAX_BYTES, 'instantane au-dela de %d octets non compresses' % SNAPSHOT_MAX_BYTES)
            member = tarfile.TarInfo(name)
            member.size, member.mode, member.mtime = len(data), mode, SNAPSHOT_MTIME
            archive.addfile(member, io.BytesIO(data))
            manifest.append({'path': name, 'size': len(data), 'sha256': sha_bytes(data), 'mode': '%o' % mode})
    size = package.stat().st_size
    need(size <= MAX_PACKAGE_BYTES, 'paquet au-dela de %d octets' % MAX_PACKAGE_BYTES)
    need(SOURCE_ROOT + '/CMakeLists.txt' in {item['path'] for item in manifest},
         SOURCE_ROOT + '/CMakeLists.txt absent du paquet')
    with tarfile.open(package, 'r:gz') as archive:
        members = archive.getmembers()
        need([member.name for member in members] == [item['path'] for item in manifest] and
             all(member.isfile() for member in members), 'paquet relu : membres differents du manifeste')
        for member, item in zip(members, manifest):
            with archive.extractfile(member) as stream:
                need(sha_bytes(stream.read()) == item['sha256'], 'paquet relu : contenu different du manifeste : ' +
                     item['path'])
    return package, sha_file(package), size, manifest, sorted(excluded, key=lambda item: item['path'])


def snapshot_manifest_bytes(manifest):
    return ''.join('%s  %s\n' % (item['sha256'], item['path']) for item in manifest).encode()


def check_ctest(where, argv):
    """Anti vert par vacuite : --no-tests=error obligatoire JUSTE APRES ctest (jamais la valeur d'une
    autre option) ; modes script, tableau de bord, repetition, silence et listage interdits."""
    need(len(argv) >= 2 and argv[1] == '--no-tests=error' and argv.count('--no-tests=error') == 1 and
         all(arg == '--no-tests=error' for arg in argv if arg.startswith('--no-tests')),
         where + 'ctest exige --no-tests=error en premier argument (CTest 3.22 rend 0 sans aucun test)')
    for arg in argv[2:]:
        need(not any(arg.startswith(prefix) for prefix in CTEST_FORBIDDEN),
             where + 'option ctest interdite : ' + arg + ' (-S, -D, --build-and-test, -T, --test-dir, -Q, --repeat, -N...)')


def validate_plan(value, tracked, data_names):
    """`tracked` : chemins des fichiers du paquet (suivis au commit, ou manifeste de l'instantane)."""
    need(type(value) is dict, 'le plan doit etre un objet JSON')
    allowed = {'schema', 'commands', 'python_packages', 'default_build', 'build_targets', 'build_timeout_seconds',
               'results_cap_bytes', 'note'}
    need(set(value) <= allowed, 'cles de plan inconnues : ' + ', '.join(sorted(set(value) - allowed)))
    need(value.get('schema') == PLAN_SCHEMA, 'schema de plan attendu : ' + PLAN_SCHEMA)
    python_packages = value.get('python_packages', 'none')
    need(type(python_packages) is str and python_packages in PYTHON_PACKAGES,
         'python_packages : "none" (defaut : Python 3.10 nu de la VM, aucun controle pip) ou "pinned" '
         '(paquets epingles exiges, installes par pip a defaut)')
    default_build = value.get('default_build', True)
    need(type(default_build) is bool, 'default_build doit etre un booleen')
    build_timeout = value.get('build_timeout_seconds', DEFAULT_BUILD_TIMEOUT)
    need(type(build_timeout) is int and 60 <= build_timeout <= 7200, 'build_timeout_seconds entier entre 60 et 7200')
    cap = value.get('results_cap_bytes', MAX_RESULTS_BYTES)
    need(type(cap) is int and MIN_RESULTS_BYTES <= cap <= MAX_RESULTS_BYTES,
         'results_cap_bytes entier entre %d et %d' % (MIN_RESULTS_BYTES, MAX_RESULTS_BYTES))
    note = value.get('note', '')
    need(type(note) is str and len(note) <= 2000, 'note : chaine de 2000 caracteres au plus')
    targets = value.get('build_targets')
    if targets is not None:
        need(default_build, 'build_targets sans construction par defaut (default_build = false)')
        need(type(targets) is list and targets and len(set(map(str, targets))) == len(targets) and
             all(type(t) is str and TARGET_RE.fullmatch(t) for t in targets),
             'build_targets : liste non vide de cibles de la forme mhgp12 ou mhgp12_*, sans doublon')
    commands = value.get('commands')
    need(type(commands) is list and 1 <= len(commands) <= 128, 'commands : liste de 1 a 128 commandes')
    names, normalized = set(), []
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
            need(default_build, where + head + ' exige la construction par defaut (default_build = true)')
            need(targets is None or binary.group(1) in targets, where + head + ' absent de build_targets')
        elif head == 'python3':
            script = PYTHON_RE.fullmatch(argv[1]) if len(argv) >= 2 else None
            need(script is not None and script.group(1) in tracked,
                 where + 'python3 exige {src}/' + SOURCE_ROOT + '/<script du paquet>.py en second argument')
        else:
            need(head == 'ctest', where + 'argv[0] doit etre ./mhgp12, ./mhgp12_*, python3 ou ctest')
            need(default_build, where + 'ctest exige la construction par defaut (default_build = true)')
            check_ctest(where, argv)
        for arg in argv:
            for ref in DATA_REF_RE.findall(arg):
                need(ref in data_names, where + 'fichier de donnees absent de --data : {data}/' + ref)
        normalized.append({'name': name, 'argv': list(argv), 'timeout_seconds': timeout})
    return {'schema': PLAN_SCHEMA, 'python_packages': python_packages, 'default_build': default_build,
            'build_timeout_seconds': build_timeout, 'build_targets': list(targets or []), 'results_cap_bytes': cap,
            'commands': normalized, 'note': note}


def results_slack(cap):
    return min(64 * 2 ** 20, cap // 4)


def render_worker_plan(plan):
    quote = shlex.quote
    lines = [WORKER_PLAN_MAGIC,
             'PLAN_PYTHON_PINNED=' + ('1' if plan['python_packages'] == 'pinned' else '0'),
             'PLAN_DEFAULT_BUILD=' + ('1' if plan['default_build'] else '0'),
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


def configured_target(zone=None, instance=None):
    need((zone is None) == (instance is None), '--zone et --instance sont requis ensemble')
    if zone is None:
        return dict(DEFAULT_TARGET)
    need(type(zone) is str and re.fullmatch(r'[a-z]+-[a-z]+[0-9]+-[a-z]', zone), 'zone standard invalide')
    need(type(instance) is str and re.fullmatch(r'[a-z](?:[-a-z0-9]{0,61}[a-z0-9])?', instance),
         'nom instance invalide')
    return dict(project=DEFAULT_TARGET['project'], zone=zone, instance=instance)


def recorded_target(session, explicit=None, warnings=None, allow_absent=False):
    """La reprise ne devine jamais une nouvelle cible. Toutes les traces lisibles doivent s'accorder."""
    candidates = []
    for relative in ('preflight.json', 'launch.json', 'receipt.json', 'host/handoff.json', 'host/lifecycle.txt'):
        path = session / relative
        if not os.path.lexists(path):
            continue
        need(path.is_file() and not path.is_symlink(), 'trace cible non reguliere : ' + relative)
        try:
            raw = path.read_bytes()
            record = fields(raw.decode()) if relative.endswith('.txt') else strict_json(raw)
        except (OSError, ValueError, Refusal):
            if relative.startswith('host/'):
                raise
            if warnings is not None:
                warnings.append('trace cible illisible, autres preuves exigees : ' + relative)
            continue
        need(type(record) is dict, 'trace cible invalide : ' + relative)
        target = {key: record.get(key) for key in DEFAULT_TARGET} if relative.startswith('host/') else record.get('target')
        if target is None and relative == 'launch.json':  # Ancien format, sans cible dans le lanceur.
            continue
        need(type(target) is dict and set(target) == set(DEFAULT_TARGET) and
             target.get('project') == DEFAULT_TARGET['project'], 'projet/cible invalide : ' + relative)
        value = configured_target(target.get('zone'), target.get('instance'))
        need(value == target, 'cible incomplete : ' + relative)
        candidates.append(value)
    if not candidates and allow_absent:  # Aucun script de start existe : aucune action cloud possible.
        return dict(explicit or DEFAULT_TARGET)
    need(candidates, 'cible de reprise absente : aucune generation ni cible ne sera devinee')
    target = candidates[0]
    need(all(value == target for value in candidates), 'cibles contradictoires dans les traces de session')
    need(explicit is None or explicit == target, 'cible CLI differente de celle de la session a reprendre')
    return target


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
    need(session.name != ROOT_LOCK_NAME, 'nom de session reserve')
    need(session.is_absolute() and session.parent == root and DATA_NAME_RE.fullmatch(session.name) and
         ':' not in str(session), '--session-dir doit etre un chemin absolu sans « : », enfant direct de ' + str(root))
    if child:
        need(session.is_dir() and not session.is_symlink() and session.stat().st_mode & 0o777 == 0o700 and
             set(os.listdir(session)) <= LAUNCH_FILES, 'dossier de session du lanceur absent ou deja utilise')
    else:
        need(not os.path.lexists(session), '--session-dir existe deja : ' + str(session))
    snapshot = args.snapshot is not None
    if snapshot:
        tree, head, status = resolve_snapshot(args.snapshot)
        commit, where = None, 'dans l\'arbre de travail'
        protocol = {name: snapshot_protocol_file(tree, name) for name in (CONTROLLER, WORKER, START_GUARD, STOP_GUARD)}
    else:
        commit, where = resolve_commit(args.commit, not args.no_fetch), 'au commit'
        protocol = {name: commit_file(commit, name) for name in (CONTROLLER, WORKER, START_GUARD, STOP_GUARD)}
    report['commit'] = commit
    need(sha_bytes(protocol[CONTROLLER]) == sha_file(Path(__file__)),
         'le controleur execute differe de ' + CONTROLLER + ' ' + where + (
             ' (modifie pendant la validation)' if snapshot else ' : committer et pousser le protocole'))
    for name, pin in GUARD_PINS.items():
        need(sha_bytes(protocol[name]) == pin,
             'script garde %s %s different de son epingle : revue requise' % (name, where))
    report['protocol_sha256'] = {name: sha_bytes(raw) for name, raw in sorted(protocol.items())}
    if snapshot:
        # Le paquet de l'instantane est construit des maintenant : son manifeste dit quels scripts existent.
        package, package_sha, package_size, manifest, excluded = build_snapshot_package(tree, protocol[WORKER],
                                                                                        scratch)
        tracked = {item['path'] for item in manifest}
        snapshot_manifest = snapshot_manifest_bytes(manifest)
        source = {'root': str(tree), 'head_commit': head, 'status_sha256': sha_bytes(status),
                  'status_entries': len(status.splitlines()), 'files': len(manifest),
                  'bytes': sum(item['size'] for item in manifest), 'manifest_sha256': sha_bytes(snapshot_manifest),
                  'manifest': manifest, 'excluded_count': len(excluded),
                  'excluded': excluded[:SNAPSHOT_EXCLUDED_LISTED]}
        worker_source = 'worktree_snapshot:' + head
    else:
        tracked = tracked_tree(commit)
        snapshot_manifest, source, worker_source = None, None, 'commit:' + commit
    report['source'] = source
    data_dir, data_files, data_total, data_manifest = validate_data(args.data)
    report.update(data_dir=str(data_dir), data_files=data_files, data_bytes=data_total,
                  data_manifest_sha256=sha_bytes(data_manifest))
    plan_path = Path(args.plan)
    need(plan_path.is_file() and plan_path.stat().st_size <= 1 << 20, '--plan : fichier JSON de 1 Mio au plus')
    plan_raw = plan_path.read_bytes()
    plan = validate_plan(strict_json(plan_raw), tracked, {item['name'] for item in data_files})
    worker_plan = render_worker_plan(plan)
    report.update(plan_sha256=sha_bytes(plan_raw), worker_plan_sha256=sha_bytes(worker_plan),
                  python_pins=PYTHON_PINS if plan['python_packages'] == 'pinned' else None,
                  plan={key: plan[key] for key in ('python_packages', 'default_build', 'build_timeout_seconds',
                                                  'build_targets', 'results_cap_bytes', 'commands')})
    if not snapshot:
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
    fixed = ((plan['build_timeout_seconds'] if plan['default_build'] else 0) +
             (PIP_TIMEOUT if plan['python_packages'] == 'pinned' else 0) + 120)
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
    return dict(commit=commit, source=source, worker_source=worker_source, snapshot_manifest=snapshot_manifest,
                protocol=protocol, plan=plan, plan_raw=plan_raw, worker_plan=worker_plan,
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
        'televerser package.tar.gz (%d o, source %s), plan.sh, plan.json puis %d fichier(s) de donnees (%d o) ; '
        'sha256 verifies sur la VM ; repertoire distant $HOME/ehgp-v12/ (hors /tmp, anciens builds et donnees '
        'elagues ; le cache $HOME/ehgp-v12/cache ne l\'est jamais)' % (
            context['package_size'], context['worker_source'], len(context['data_files']), context['data_total']),
        'lancer v12_worker.sh detache (faits de la VM, %s%s, puis %d commande(s)) ; sondage long avec '
        'recertification de la cible et controle du disque local a chaque cycle' % (
            'Python epingle, ' if context['plan']['python_packages'] == 'pinned' else '',
            'construction Release -j nproc' if context['plan']['default_build'] else
            'aucune construction par defaut', len(context['plan']['commands'])),
        'rapatriement borne a %d s et avant D - %d s (garde invitee relue, VM recertifiee, taille <= %d o)' % (
            RETRIEVE_BUDGET, SSH_CUTOFF, context['plan']['results_cap_bytes']),
        'finally : liberer la reserve ; describe ; stop_and_verify.sh --yes --expected-last-start-timestamp '
        '<generation> seulement si RUNNING/SUSPENDING de cette generation sans arret posterieur, ou STOPPING ; '
        'relire TERMINATED ; retirer la '
        'cle OS Login ; puis extraction bornee et verification des resultats ; recu sans le $HOME de la VM, '
        'avec la liste des fichiers rapatries qui portent encore une identite',
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
    launch.json, ou `v12_session.py ... --child` sur ce dossier) et les scripts gardes de la session
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
        is_child = '--child' in args and any(arg.endswith('v12_session.py') for arg in args) and (
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


def read_fields(path):
    """Fichier cle=valeur des resultats (faits de la VM) ; {} s'il manque ou s'il est mal forme : un
    releve, jamais une condition de conformite."""
    try:
        return fields(path.read_text(errors='replace'))
    except (OSError, Refusal):
        return {}


# ---------------------------------------------------------------------------------------------
# Recus sans identite (v12) : compte rendu seulement ; aucune garde, aucun statut n'en depend.

IDENTITY_SCAN_MAX_BYTES = 2 * 2 ** 30   # au-dela, l'examen s'arrete et le dit (complete = false)
IDENTITY_SCAN_LISTED = 200              # fichiers listes dans le recu (le compte reste exact)


def remote_home_of(directory):
    """$HOME de la VM deduit du dossier de session $HOME/ehgp-v12/ehgp-v12.XXXXXXXXXX ; None si douteux
    (vide, racine, caracteres inhabituels) : rien n'est alors remplace."""
    marker = '/ehgp-v12/ehgp-v12.'
    if not isinstance(directory, str) or marker not in directory:
        return None
    home = directory[:directory.rindex(marker)].rstrip('/')
    return home if re.fullmatch(r'/[A-Za-z0-9._/-]*[A-Za-z0-9._-]', home) else None


def without_home(value, home):
    """Copie de `value` ou le chemin personnel de la VM (`home`, derive de l'adresse du compte OS Login)
    devient le texte litteral $HOME. `value` n'est jamais modifie : etat, journaux d'hote et commandes
    executees restent exacts."""
    if home is None:
        return value
    if isinstance(value, str):
        return '$HOME' if value == home else value.replace(home + '/', '$HOME/')
    if isinstance(value, dict):
        return {key: without_home(item, home) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [without_home(item, home) for item in value]
    return value


def identity_scan(root, needles):
    """Fichiers rapatries sous `root` (archives *.tar.gz exceptees, leur contenu est extrait) qui portent
    encore une valeur d'identite : rend des chemins relatifs et des genres, JAMAIS les valeurs. Les sorties
    des commandes ne sont pas reecrites (elles sont des resultats) ; ce releve dit lesquelles ne peuvent pas
    entrer telles quelles dans un recu publie."""
    patterns = {kind: value.encode() for kind, value in needles.items() if value and len(value) >= 4}
    report = {'kinds': sorted(patterns), 'scanned_files': 0, 'scanned_bytes': 0, 'complete': True,
              'files_count': 0, 'files': []}
    if not patterns or not root.is_dir():
        return report
    keep = max(len(pattern) for pattern in patterns.values()) - 1
    for path in sorted(root.rglob('*')):
        if path.is_symlink() or not path.is_file() or path.name.endswith('.tar.gz'):
            continue
        found, tail = set(), b''
        with open(path, 'rb') as stream:
            for block in iter(lambda: stream.read(1 << 20), b''):
                if report['scanned_bytes'] + len(block) > IDENTITY_SCAN_MAX_BYTES:
                    report['complete'] = False
                    break
                report['scanned_bytes'] += len(block)
                window = tail + block
                found.update(kind for kind, pattern in patterns.items() if pattern in window)
                tail = window[-keep:]
        report['scanned_files'] += 1
        if found:
            report['files_count'] += 1
            if len(report['files']) < IDENTITY_SCAN_LISTED:
                report['files'].append({'path': path.relative_to(root).as_posix(), 'kinds': sorted(found)})
        if not report['complete']:
            break
    return report


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
        self.remote_home = None          # $HOME de la VM : remplace par le texte $HOME dans le recu
        self.deadline = self.worker_deadline = None
        self.archive = None
        self.mono_offset = time.monotonic() - time.time()
        self.state = {
            'schema': RECEIPT_SCHEMA, 'status': None, 'target': TARGET, 'backend': 'reference_cpu',
            'public_status': 'not_claimed', 'comparability': COMPARABILITY, 'commit': context['commit'],
            'source_kind': report['source_kind'], 'evidence_grade': report['evidence_grade'],
            'source': context['source'], 'worker_source': context['worker_source'],
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

    def receipt_bytes(self):
        """Recu JSON ou le chemin personnel de la VM devient $HOME ; a defaut (compte rendu seulement),
        l'etat brut : jamais le recu au prix de la redaction."""
        try:
            return json_bytes(without_home(self.state, self.remote_home))
        except Exception:  # noqa: BLE001
            return json_bytes(self.state)

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
        if self.context['snapshot_manifest'] is not None:
            write_new(self.package_dir / 'SNAPSHOT_MANIFEST.sha256', self.context['snapshot_manifest'])
        write_new(self.package_dir / 'plan.json', self.context['plan_raw'])
        write_new(self.package_dir / 'plan.sh', self.context['worker_plan'])
        (self.package_dir / 'data').mkdir(mode=0o700)
        write_new(self.package_dir / 'data' / 'SHA256SUMS', self.context['data_manifest'])
        for name in (START_GUARD, STOP_GUARD):
            write_new(self.host / Path(name).name, self.context['protocol'][name], mode=0o700)
            os.chmod(self.host / Path(name).name, 0o700)
        write_new(self.session / 'preflight.json', json_bytes(self.report))
        keygen = subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-C', 'ehgp-v12-' + self.session.name,
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
        log('demarrage garde (start_and_verify.sh de la source, egal a son epingle)')
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
        rc, out, _ = self.ssh('remote_mkdir', 'umask 077; mkdir -p "$HOME/ehgp-v12" && chmod 700 "$HOME/ehgp-v12" && '
                                              'for o in "$HOME"/ehgp-v12/ehgp-v12.*; do if [ -d "$o" ] && [ ! -L "$o" ]; '
                                              'then rm -rf "$o/build" "$o/src" "$o/data" "$o/joblib" "$o/pyuser" '
                                              '"$o/overflow" "$o/package.tar.gz" "$o/salvage.tar.gz"; if [ -f '
                                              '"$o/results.tar.gz" ]; then rm -rf "$o/results"; fi; fi; '
                                              'done; d=$(mktemp -d "$HOME/ehgp-v12/ehgp-v12.XXXXXXXXXX") && '
                                              'mkdir "$d/data" && printf "DIR=%s\\n" "$d" && '
                                              'df -Pk "$d" | awk \'NR==2 {print "AVAIL_KB=" $4}\' && '
                                              'du -sk "$HOME/ehgp-v12" | awk \'{print "USED_KB=" $1}\'', timeout=90)
        values = fields('\n'.join(line for line in out.splitlines() if re.match(r'(DIR|AVAIL_KB|USED_KB)=', line)))
        need(rc == 0 and REMOTE_DIR_RE.fullmatch(values.get('DIR', '')) and
             re.fullmatch(r'[0-9]+', values.get('AVAIL_KB', '')), 'repertoire distant non cree')
        self.remote = values['DIR']
        self.remote_home = remote_home_of(self.remote)
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
                       '--deadline-epoch', str(self.worker_deadline), '--source', context['worker_source'],
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
        state['vm_facts'] = read_fields(extracted / 'env' / 'vm_facts.txt')
        need(worker.get('schema') == WORKER_RESULT_SCHEMA, 'worker.txt : schema inattendu')
        need(worker.get('source') == context['worker_source'] and
             worker.get('plan_sha256') == state['worker_plan_sha256'] and
             worker.get('package_sha256') == context['package_sha'] and worker.get('generation') == state['generation'],
             'provenance du worker (source, plan, paquet, generation) differente de la session')
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
        # 3 bis. Recu sans identite : fichiers rapatries qui portent encore l'adresse du compte ou le chemin
        # personnel de la VM (releve seulement : jamais une erreur, jamais un statut).
        try:
            state['results_identity_scan'] = identity_scan(self.results_dir, {
                'account': self.context.get('account'), 'vm_home': self.remote_home})
        except BaseException as error:  # noqa: B902
            state['warnings'].append('examen d\'identite des resultats : %s: %s' % (type(error).__name__, error))
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
        written = write_with_fallback(self.session / 'receipt.json', self.run_dir / 'receipt.json',
                                      self.receipt_bytes())
        if written != state['receipt_path']:
            state['receipt_path'] = written
            if written is None:
                log('ecriture du recu impossible (session et dossier d\'execution)')
            else:
                write_with_fallback(Path(written), Path(written), self.receipt_bytes())
        if status in ('shutdown_uncertified', 'foreign_generation_active'):
            log('%s : %s' % (status.upper(), state.get('recovery') or state.get('recovery_command')))
        return EXIT_CODES[status]

    def run(self):
        try:
            self.prepare()
        except (Refusal, OSError, subprocess.SubprocessError) as error:
            self.state.update(status='failed_before_start', errors=['preparation locale : %s' % error])
            write_with_fallback(self.session / 'receipt.json', self.run_dir / 'receipt.json', self.receipt_bytes())
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
            write_with_fallback(self.session / 'receipt.json', self.run_dir / 'receipt.json', self.receipt_bytes())
            return EXIT_CODES['shutdown_uncertified']


def summary_of(state, session):
    value = {key: state.get(key) for key in (
        'status', 'commit', 'source_kind', 'evidence_grade', 'generation', 'closure', 'closing_generation',
        'targeted_shutdown_certified',
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
    info = {'status': 'launched', 'pid': process.pid, 'session': str(session), 'target': TARGET,
            'sentinel': str(session / 'DONE'),
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
    global TARGET
    session = Path(args.session_dir).absolute()
    host = session / 'host'
    gcloud = Path(args.gcloud)
    if not session.is_dir():
        CONSOLE.emit({'status': 'session_absente', 'session': str(session),
                      'recovery': 'dossier de session introuvable : verifier le chemin ; rien n\'a ete fait'})
        return EXIT_CODES['failed_before_start']
    state = {'schema': 'ehgp.v12.recovery_receipt.v1', 'session': str(session), 'target': TARGET,
             'generation': None, 'targeted_shutdown_certified': False, 'errors': [], 'warnings': [],
             'started_utc': utc_now()}
    SIGNALS.install()
    lock = root_lock = None
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
            if any((session / name).exists() for name in ('preflight.json', 'launch.json', 'receipt.json')):
                explicit = configured_target(args.zone, args.instance) if args.zone is not None else None
                TARGET = recorded_target(session, explicit, state['warnings'], allow_absent=True)
                state['target'] = TARGET
            state.update(closure='no_start_requested', recovery='aucun demarrage n\'a pu etre lance : rien a arreter')
            code = 0
        else:
            root_lock = take_lock(session.parent / ROOT_LOCK_NAME)
            if root_lock is None:
                raise RecoveryBusy('une autre session G4 (v10, v11 ou v12) tient le verrou commun')
            explicit = configured_target(args.zone, args.instance) if args.zone is not None else None
            TARGET = recorded_target(session, explicit, state['warnings'])
            state['target'] = TARGET
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
    except RecoveryBusy as error:
        state.update(status='session_alive', recovery=str(error))
        code = SESSION_ALIVE_CODE
    except BaseException as error:  # noqa: B902
        state['errors'].append('%s: %s' % (type(error).__name__, error))
        state.setdefault('status', 'recovery_failed')
        code = EXIT_CODES['shutdown_uncertified']
    finally:
        if root_lock is not None and root_lock >= 0:
            os.close(root_lock)
        if lock is not None and lock >= 0:
            os.close(lock)
    name = 'recovery_%s.json' % utc_now().replace(':', '')
    state['recovery_receipt'] = write_with_fallback(session / name, run_directory(session) / name, json_bytes(state))
    CONSOLE.emit({k: state.get(k) for k in ('status', 'target', 'closure', 'closing_generation', 'targeted_shutdown_certified',
                                             'processes', 'ignored_processes', 'recovery', 'recovery_command',
                                             'errors', 'recovery_receipt')})
    return code


def main(argv=None):
    global TARGET
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--commit', help='SHA du commit pousse sur origin/main (seul mode qui produit un recu)')
    parser.add_argument('--snapshot', metavar='RACINE',
                        help='instantane de l\'arbre de travail RACINE du controleur execute : essai de '
                             'developpement (evidence_grade = dev_snapshot), jamais un recu')
    parser.add_argument('--plan', help='plan JSON (schema %s)' % PLAN_SCHEMA)
    parser.add_argument('--data', help='dossier plat des donnees (.u32le), jamais versionnees')
    parser.add_argument('--session-dir', help='dossier neuf, enfant direct de ' + DEFAULT_SESSIONS_ROOT)
    parser.add_argument('--max-run-seconds', type=int, help='maxRunDuration GCE exige (egal a celui de la VM)')
    parser.add_argument('--zone', help='zone standard explicite ; exige --instance (sinon ancienne cible par defaut)')
    parser.add_argument('--instance', help='nom exact de la VM gardee ; exige --zone, projet fixe')
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
    try:
        TARGET = configured_target(args.zone, args.instance)
    except Refusal as error:
        CONSOLE.emit(dict(status='refused', reason=str(error), gcp_mutations='none'))
        return EXIT_CODES['failed_before_start']
    if args.recover:
        if not args.session_dir:
            parser.error('--recover exige --session-dir')
        return recover(args)
    missing = [name for name in ('plan', 'data', 'session_dir', 'max_run_seconds') if getattr(args, name) is None]
    if missing:
        parser.error('arguments obligatoires : ' + ', '.join('--' + m.replace('_', '-') for m in missing))
    if (args.commit is None) == (args.snapshot is None):
        parser.error('exactement une source : --commit SHA (recu) ou --snapshot RACINE (essai de developpement)')
    source_kind = 'commit' if args.snapshot is None else 'worktree_snapshot'
    report = {'schema': 'ehgp.v12.session_preflight.v1', 'mode': 'execute' if args.execute else 'dry_run',
              'target': TARGET, 'backend': 'reference_cpu', 'public_status': 'not_claimed', 'gcp_mutations': 'none',
              'source_kind': source_kind, 'evidence_grade': EVIDENCE_GRADES[source_kind]}
    child = args.child and args.execute
    session = Path(args.session_dir)
    root_lock_path = Path(args.sessions_root) / ROOT_LOCK_NAME
    if child:
        lock = take_lock(session / 'session.lock') if session.is_dir() else None
        if lock is None:
            CONSOLE.emit(dict(report, status='refused', reason='session deja tenue par un autre processus'))
            return EXIT_CODES['failed_before_start']
        # Une seule session G4 a la fois depuis cette racine, v10, v11 OU v12 (la cible est unique) : le verrou
        # est le fichier de la v10, tenu a vie.
        root_lock = take_lock(root_lock_path) if root_lock_path.parent.is_dir() else None
        if root_lock is None:
            CONSOLE.emit(dict(report, status='refused', reason='une autre session G4 (v10, v11 ou v12) est en cours '
                                                               '(verrou %s)' % root_lock_path))
            return EXIT_CODES['failed_before_start']
    elif args.execute and root_lock_path.parent.is_dir():
        probe = take_lock(root_lock_path, create=False)   # sonde seulement : un refus ne cree aucun fichier
        if probe is None:
            CONSOLE.emit(dict(report, status='refused', reason='une autre session G4 (v10, v11 ou v12) est en cours '
                                                               '(verrou %s)' % root_lock_path))
            return EXIT_CODES['failed_before_start']
        if probe >= 0:
            os.close(probe)
    code = EXIT_CODES['failed_before_start']
    run_dir = None
    try:
        with tempfile.TemporaryDirectory(prefix='ehgp-v12-package-') as scratch:
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
