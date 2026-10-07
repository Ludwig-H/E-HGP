#!/usr/bin/env python3
"""Cache de donnees persistant de la VM G4 (lignee v12) : jeux publics telecharges par URL, verifies par
sha256 et gardes d'une session a l'autre. Bibliotheque standard seulement (Python 3.10 nu de la VM).

Lance par une commande ordinaire du plan de session (gcp-migration/v12_session.py) : c'est un script du
paquet comme un autre, le protocole n'admet aucune commande nouvelle.

  python3 {src}/morsehgp3D_v12/bench/data_cache.py --manifest {src}/morsehgp3D_v12/bench/JEUX.json \
      --get NOM [NOM ...] --link {build}/datasets --report {out}/data_cache.json
  python3 {src}/morsehgp3D_v12/bench/data_cache.py --manifest ... --probe NOM [NOM ...] --report {out}/probe.json
  python3 {src}/morsehgp3D_v12/bench/data_cache.py (--status | --trim | --purge) --report {out}/cache.json

Racine : --cache-dir, sinon $MHGP12_CACHE_DIR, exporte par gcp-migration/v12_worker.sh ($HOME/ehgp-v12/cache :
frere des dossiers de session ehgp-v12.*, que le controleur elague a chaque session, lui jamais).

Manifeste (versionne dans le paquet ; jamais de donnees) :
  {"schema": "ehgp.v12.public_datasets.v1",
   "datasets": [{"name": "scene.laz", "url": "https://...", "sha256": "<64 hex>" ou null,
                 "size": <octets> ou null, "license": "...", "note": "..."}]}

Garanties :
- rien n'entre dans le cache sans verification : taille et sha256 egaux a l'epingle du manifeste ; ecrit sous
  partial/, renomme atomiquement en objects/<sha256>, en lecture seule (0400) ; le cache est adresse par le
  contenu, le nom n'est qu'une entree du manifeste ;
- --get exige une epingle (sha256 et taille) pour chaque nom, avant tout telechargement ; un objet present est
  RE-VERIFIE a chaque usage ; un objet altere est retire puis retelecharge ;
- --probe telecharge une entree (epinglee ou non) et rapporte son sha256 et sa taille ; l'objet est range sous
  son sha256 mais ne sert sous son nom qu'une fois l'epingle ecrite au manifeste (premier usage de confiance) ;
- reprise : un telechargement interrompu (delai de la commande, preemption) reprend a l'octet pres a l'appel
  suivant (Range + If-Range) ; un serveur sans Range recommence a zero ; seule la verification finale fait foi ;
- place : apres un telechargement il reste au moins --min-free-bytes (defaut 16 Gio) libres et le cache tient
  sous --max-cache-bytes (defaut 40 Gio) ; sinon les partiels puis les objets les moins recemment utilises,
  non demandes par cet appel, sont evinces. Seul ce qui rend vraiment des blocs au disque compte pour le
  plancher (CST-0220) : un partiel, ou un objet dont le seul lien dur est celui du cache ; un objet encore lie
  ailleurs (--link) ne compte que pour le plafond, et n'est jamais evince pour le plancher. Si tout l'evincable
  ne suffirait pas (tailles et liens observes avant toute eviction), refus explicite sans telechargement et
  cache intact. Limite declaree : si l'espace libre baisse pendant l'eviction (autre processus, objet evince
  encore ouvert), un refus apres eviction partielle reste possible ; chaque entree evincee est alors au rapport,
  avec les octets reellement rendus au disque ;
- --link DIR cree DIR/<nom> (lien dur ; copie si autre systeme de fichiers) : chemins stables pour les
  commandes suivantes (cwd = {build}) ; refus si DIR est dans les resultats rapatries de la session (sous
  $V12_OUT/../.., exporte par le worker), chemins reels compris (un ancetre symbolique ne contourne pas le
  refus) : on lie dans {build}, jamais dans {out} ;
- --deadline-seconds : aucun essai, pas meme le premier, ne commence au-dela du delai ;
- le rapport JSON ne contient aucun chemin absolu ni aucune identite (la racine y est notee $MHGP12_CACHE_DIR) ;
- URL http ou https seulement, sans identifiants ; le contenu est authentifie par l'epingle, pas par le transport.
Codes de sortie : 0 conforme ; 1 au moins une entree en echec (ou interruption) ; 2 usage ou manifeste
invalide ; 3 cache inutilisable (racine, droits, verrou).
"""
import argparse
from datetime import datetime, timezone
import errno
import fcntl
import hashlib
import http.client
import json
import os
from pathlib import Path
import re
import shutil
import signal
import stat
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

MANIFEST_SCHEMA = 'ehgp.v12.public_datasets.v1'
REPORT_SCHEMA = 'ehgp.v12.data_cache_report.v1'
CACHE_ENV = 'MHGP12_CACHE_DIR'
NAME_RE = re.compile(r'[A-Za-z0-9][A-Za-z0-9._-]{0,127}')
SHA_RE = re.compile(r'[0-9a-f]{64}')
KEY_RE = re.compile(r'(?:[0-9a-f]{64}|probe-[0-9a-f]{32})')
ENTRY_KEYS = {'name', 'url', 'sha256', 'size', 'license', 'note'}
DEFAULT_MAX_CACHE = 40 * 2 ** 30
DEFAULT_MIN_FREE = 16 * 2 ** 30
DEFAULT_PROBE_MAX = 32 * 2 ** 30
BLOCK = 1 << 20
FREE_CHECK_EVERY = 64 * 2 ** 20     # taille inconnue : place relue tous les 64 Mio ecrits
USER_AGENT = 'ehgp-v12-data-cache/1'
SIGNALS = (signal.SIGTERM, signal.SIGINT, signal.SIGHUP)


class Usage(Exception):
    code = 2


class Unusable(Exception):
    code = 3


class EntryFailure(Exception):
    """Echec d'une entree ; `keep_partial` dit si le partiel reste utilisable pour une reprise."""

    def __init__(self, message, keep_partial=True):
        super().__init__(message)
        self.keep_partial = keep_partial


class Stop(BaseException):
    """Signal recu (delai de la commande, abandon du worker)."""


def utc(epoch=None):
    return datetime.fromtimestamp(time.time() if epoch is None else epoch,
                                  timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')


def reason(error):
    """Cause d'une erreur, sans le chemin qu'un OSError porte dans son texte."""
    if isinstance(error, OSError) and not isinstance(error, urllib.error.URLError) and error.strerror:
        return '%s: %s' % (type(error).__name__, error.strerror)
    return '%s: %s' % (type(error).__name__, error)


def strict_json(raw):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise Usage('cle JSON dupliquee : ' + key)
            result[key] = value
        return result

    def constant(name):
        raise Usage('constante JSON interdite : ' + name)
    try:
        return json.loads(raw, object_pairs_hook=pairs, parse_constant=constant)
    except (ValueError, UnicodeDecodeError) as error:
        raise Usage('JSON invalide : %s' % error) from error


def check_url(url, where):
    if type(url) is not str or not 1 <= len(url) <= 2048 or any(ord(c) <= 32 or ord(c) == 127 for c in url):
        raise Usage(where + 'url : chaine de 1 a 2048 caracteres sans espace ni controle')
    parts = urllib.parse.urlsplit(url)
    if parts.scheme not in ('http', 'https') or not parts.hostname:
        raise Usage(where + 'url : http ou https avec un hote')
    if parts.username is not None or parts.password is not None:
        raise Usage(where + 'url : identifiants interdits dans l\'URL')


def load_manifest(path):
    """({nom: entree}, sha256 du manifeste) ; refuse tout ce qui n'est pas exactement au schema."""
    try:
        raw = Path(path).read_bytes()
    except OSError as error:
        raise Usage('manifeste illisible : ' + reason(error)) from error
    value = strict_json(raw)
    if type(value) is not dict or set(value) != {'schema', 'datasets'} or value['schema'] != MANIFEST_SCHEMA:
        raise Usage('manifeste : objet {schema, datasets} au schema ' + MANIFEST_SCHEMA)
    if type(value['datasets']) is not list or len(value['datasets']) > 10000:
        raise Usage('manifeste : datasets doit etre une liste (10 000 entrees au plus)')
    entries = {}
    for index, entry in enumerate(value['datasets']):
        where = 'entree %d : ' % index
        if type(entry) is not dict or not {'name', 'url', 'license'} <= set(entry) <= ENTRY_KEYS:
            raise Usage(where + 'cles name, url, license obligatoires ; sha256, size, note facultatives')
        name = entry['name']
        if type(name) is not str or not NAME_RE.fullmatch(name) or name in entries:
            raise Usage(where + 'nom unique [A-Za-z0-9][A-Za-z0-9._-]{0,127} attendu')
        check_url(entry['url'], where)
        sha, size = entry.get('sha256'), entry.get('size')
        if sha is not None and (type(sha) is not str or not SHA_RE.fullmatch(sha)):
            raise Usage(where + 'sha256 : 64 chiffres hexadecimaux minuscules, ou null')
        if size is not None and (type(size) is not int or size <= 0):
            raise Usage(where + 'size : entier positif, ou null')
        if (sha is None) != (size is None):
            raise Usage(where + 'sha256 et size s\'epinglent ensemble (ou sont tous deux null)')
        if type(entry['license']) is not str or not 1 <= len(entry['license']) <= 200:
            raise Usage(where + 'license : chaine non vide de 200 caracteres au plus')
        if type(entry.get('note', '')) is not str or len(entry.get('note', '')) > 2000:
            raise Usage(where + 'note : chaine de 2000 caracteres au plus')
        entries[name] = {'name': name, 'url': entry['url'], 'sha256': sha, 'size': size, 'license': entry['license']}
    return entries, hashlib.sha256(raw).hexdigest()


def private_directory(path):
    """Dossier prive (0700) appartenant a l'utilisateur courant, jamais un lien symbolique ; cree s'il manque."""
    try:
        os.mkdir(path, 0o700)
    except FileExistsError:
        pass
    except OSError as error:
        raise Unusable('dossier %s impossible a creer : %s' % (path.name, reason(error))) from error
    info = os.lstat(path)
    if not stat.S_ISDIR(info.st_mode) or info.st_uid != os.geteuid():
        raise Unusable('%s doit etre un dossier reel (pas un lien) de l\'utilisateur courant' % path.name)
    if info.st_mode & 0o777 != 0o700:
        os.chmod(path, 0o700)


def sha_of(path):
    digest, total = hashlib.sha256(), 0
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(BLOCK), b''):
            digest.update(block)
            total += len(block)
    return digest.hexdigest(), total


def partial_key(entry):
    return entry['sha256'] or 'probe-' + hashlib.sha256(entry['url'].encode()).hexdigest()[:32]


class Cache:
    """objects/<sha256> (verifies, 0400), partial/<cle>.part et .json (reprises), .lock (flock exclusif)."""

    def __init__(self, root, max_bytes, min_free, lock_wait):
        if not root.is_absolute():
            raise Unusable('racine du cache : chemin absolu exige')
        if not root.parent.is_dir() or root.parent.is_symlink():
            raise Unusable('le parent de la racine du cache doit exister (dossier reel)')
        self.root, self.max_bytes, self.min_free = root, max_bytes, min_free
        private_directory(root)
        self.objects, self.partial = root / 'objects', root / 'partial'
        private_directory(self.objects)
        private_directory(self.partial)
        try:
            self.lock = os.open(root / '.lock', os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW, 0o600)
        except OSError as error:
            raise Unusable('verrou du cache illisible : ' + reason(error)) from error
        end = time.monotonic() + lock_wait
        while True:
            try:
                fcntl.flock(self.lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                if time.monotonic() >= end:
                    raise Unusable('cache tenu par un autre processus (verrou)')
                time.sleep(1)
        self.evicted = []

    def object_path(self, sha):
        return self.objects / sha

    def object_entries(self):
        """[(sha, octets, date d'usage, liens durs)] des objets reguliers ; tout autre nom est ignore."""
        items = []
        for path in self.objects.iterdir():
            info = os.lstat(path)
            if SHA_RE.fullmatch(path.name) and stat.S_ISREG(info.st_mode):
                items.append((path.name, info.st_size, info.st_mtime, info.st_nlink))
        return items

    def object_list(self):
        """[(sha, octets, date d'usage)] des objets reguliers ; tout autre nom est ignore."""
        return [(sha, size, mtime) for sha, size, mtime, _ in self.object_entries()]

    def partial_list(self):
        items = []
        for path in self.partial.iterdir():
            info = os.lstat(path)
            if path.suffix == '.part' and KEY_RE.fullmatch(path.stem) and stat.S_ISREG(info.st_mode):
                items.append((path.stem, info.st_size, info.st_mtime))
        return items

    def used(self):
        return sum(item[1] for item in self.object_list()) + sum(item[1] for item in self.partial_list())

    def free(self):
        return shutil.disk_usage(self.root).free

    def drop_partial(self, key):
        for suffix in ('.part', '.json'):
            try:
                (self.partial / (key + suffix)).unlink()
            except FileNotFoundError:
                pass

    def drop_object(self, sha):
        try:
            self.object_path(sha).unlink()
        except FileNotFoundError:
            pass

    def make_room(self, need, protected_objects, protected_partials):
        """Place pour `need` octets : plancher d'espace libre (min_free) et plafond logique du cache (max_bytes).

        Ce que l'eviction rend vraiment (CST-0220) : retirer un partiel, ou un objet dont le seul lien dur est celui
        du cache (st_nlink = 1), rend ses blocs au disque ; retirer un objet encore lie ailleurs (lien dur pose par
        --link) ne rend rien au disque, seulement au plafond du cache. Le plancher ne compte donc que les premiers,
        le plafond compte tout. La decision precede toute eviction, sur les tailles et les liens observes : si
        evincer tout l'evincable ne suffirait pas, refus et cache intact. Sinon on evince le necessaire, partiels
        d'abord puis objets les moins recemment utilises, et pour le plancher jamais un objet encore lie. Limite
        declaree : si l'espace libre baisse pendant l'eviction (autre processus, objet evince encore ouvert), un
        refus apres eviction partielle reste possible ; chaque entree evincee est au rapport, avec les octets
        reellement rendus au disque (freed_disk_bytes)."""
        def floor_short():
            return self.free() - need < self.min_free

        def cap_short():
            return self.used() + need > self.max_bytes

        def candidates():
            partials = sorted((mtime, key, size) for key, size, mtime in self.partial_list()
                              if key not in protected_partials)
            objects = sorted((mtime, sha, size, links) for sha, size, mtime, links in self.object_entries()
                             if sha not in protected_objects)
            return partials, objects
        if not floor_short() and not cap_short():
            return
        partials, objects = candidates()
        freeable = sum(size for _, _, size in partials) + sum(size for _, _, size, links in objects if links == 1)
        removable = sum(size for _, _, size in partials) + sum(size for _, _, size, _ in objects)
        free, used = self.free(), self.used()
        if free + freeable - need < self.min_free or used - removable + need > self.max_bytes:
            raise EntryFailure('place insuffisante, meme en evincant tout l\'evincable : %d octets demandes, %d libres '
                               '(plancher %d), %d liberables sur le disque, cache %d (plafond %d) ; rien n\'est '
                               'evince' % (need, free, self.min_free, freeable, used, self.max_bytes))
        while True:
            floor = floor_short()
            if not floor and not cap_short():
                return
            partials, objects = candidates()
            if floor:  # pour le plancher, seul un objet sans autre lien dur rend des blocs
                objects = [item for item in objects if item[3] == 1]
            if partials:
                _, key, size = partials[0]
                self.drop_partial(key)
                self.evicted.append({'kind': 'partial', 'key': key, 'bytes': size, 'freed_disk_bytes': size})
            elif objects:
                _, sha, size, links = objects[0]
                self.drop_object(sha)
                self.evicted.append({'kind': 'object', 'sha256': sha, 'bytes': size, 'links': links,
                                     'freed_disk_bytes': size if links == 1 else 0})
            else:
                raise EntryFailure('place insuffisante apres eviction partielle (espace libre change pendant '
                                   'l\'eviction) : %d octets demandes, %d libres (plancher %d), cache %d '
                                   '(plafond %d) ; entrees evincees au rapport'
                                   % (need, self.free(), self.min_free, self.used(), self.max_bytes))

    def check_object(self, sha, size):
        """'absent', 'ok' (contenu exact, date d'usage rafraichie) ou 'altered' (objet retire)."""
        path = self.object_path(sha)
        try:
            info = os.lstat(path)
        except FileNotFoundError:
            return 'absent'
        if stat.S_ISREG(info.st_mode):
            digest, total = sha_of(path)
            if digest == sha and (size is None or total == size):
                os.utime(path)
                return 'ok'
        self.drop_object(sha)
        return 'altered'

    def admit(self, part, sha):
        os.chmod(part, 0o400)
        os.replace(part, self.object_path(sha))
        descriptor = os.open(self.objects, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
        os.utime(self.object_path(sha))

    def status(self):
        objects, partials = self.object_entries(), self.partial_list()
        return {'objects': len(objects), 'object_bytes': sum(item[1] for item in objects),
                'object_bytes_linked_elsewhere': sum(item[1] for item in objects if item[3] > 1),
                'partials': len(partials), 'partial_bytes': sum(item[1] for item in partials),
                'free_bytes': self.free(), 'max_cache_bytes': self.max_bytes, 'min_free_bytes': self.min_free,
                'listing': [{'sha256': sha, 'bytes': size, 'last_used_utc': utc(mtime), 'links': links}
                            for sha, size, mtime, links in sorted(objects, key=lambda item: -item[2])[:1000]]}


def announced_total(response, offset):
    """Taille totale annoncee (Content-Range d'une reprise, Content-Length sinon), ou None."""
    if response.status == 206:
        match = re.fullmatch(r'bytes (\d+)-(\d+)/(\d+|\*)', response.headers.get('Content-Range', '').strip())
        if not match or int(match.group(1)) != offset:
            raise EntryFailure('Content-Range incoherent avec la reprise a %d octets' % offset, keep_partial=False)
        return None if match.group(3) == '*' else int(match.group(3))
    length = response.headers.get('Content-Length', '').strip()
    return int(length) if length.isdigit() else None


def fetch(cache, entry, cap, protected_objects, protected_partials, stats, socket_timeout):
    """Un essai : telecharge l'URL de `entry` vers partial/<cle>.part (reprise si possible), verifie, range
    l'objet ; rend (sha256, octets). Sans epingle (sondage), le plafond est `cap`."""
    url, expected_sha, expected_size = entry['url'], entry['sha256'], entry['size']
    key = partial_key(entry)
    part, meta_path = cache.partial / (key + '.part'), cache.partial / (key + '.json')
    try:
        meta = strict_json(meta_path.read_bytes()) if meta_path.exists() else {}
    except Usage:
        meta = {}
    if not part.exists() or type(meta) is not dict or meta.get('url') != url or meta.get('sha256') != expected_sha:
        cache.drop_partial(key)          # partiel d'une autre URL ou d'une autre epingle : jamais repris
        meta = {}
    meta.update(url=url, sha256=expected_sha, size=expected_size)
    meta_path.write_text(json.dumps(meta, sort_keys=True))
    part.touch(mode=0o600, exist_ok=True)
    offset = part.stat().st_size
    if offset > cap:
        raise EntryFailure('partiel au-dela de la taille attendue', keep_partial=False)
    digest = hashlib.sha256()
    if offset:
        with open(part, 'rb') as stream:
            for block in iter(lambda: stream.read(BLOCK), b''):
                digest.update(block)
        stats['resumed_from'] = offset
    if expected_size is not None:        # taille epinglee : la place se juge AVANT toute requete
        cache.make_room(expected_size - offset, protected_objects, protected_partials | {key})
    headers = {'User-Agent': USER_AGENT, 'Accept-Encoding': 'identity'}
    if offset:
        headers['Range'] = 'bytes=%d-' % offset
        if meta.get('validator'):
            headers['If-Range'] = meta['validator']
    written = offset
    try:
        response = urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=socket_timeout)
    except urllib.error.HTTPError as error:
        error.close()
        if error.code != 416 or not offset:
            raise
        if expected_size is None or offset != expected_size:
            raise EntryFailure('reprise refusee par le serveur (416)', keep_partial=False) from error
        response = None                  # partiel deja complet (arret entre le dernier octet et le rangement)
    if response is not None:
        with response:
            if response.status not in (200, 206) or (response.status == 206 and not offset):
                raise EntryFailure('reponse HTTP %d inattendue' % response.status, keep_partial=False)
            encoding = response.headers.get('Content-Encoding', 'identity').strip().lower()
            if encoding not in ('', 'identity'):
                raise EntryFailure('Content-Encoding %s refuse : le sha256 porte sur les octets du fichier' % encoding,
                                   keep_partial=False)
            if response.status == 200 and offset:
                stats['restarted'] = True    # serveur sans Range, ou ressource changee (If-Range) : depuis zero
                stats.pop('resumed_from', None)
                offset = written = 0
                digest = hashlib.sha256()
                with open(part, 'wb'):
                    pass
            total = announced_total(response, offset)
            if total is not None and expected_size is not None and total != expected_size:
                raise EntryFailure('taille annoncee %d differente de l\'epingle %d' % (total, expected_size),
                                   keep_partial=False)
            if total is not None and total > cap:
                raise EntryFailure('taille annoncee %d au-dela du plafond %d' % (total, cap), keep_partial=False)
            validator = response.headers.get('ETag') or ''
            if validator.startswith('W/') or not validator:
                validator = response.headers.get('Last-Modified') or ''
            if response.status == 200 and validator:
                meta['validator'] = validator
                meta_path.write_text(json.dumps(meta, sort_keys=True))
            final = total if total is not None else expected_size
            cache.make_room((final if final is not None else cap) - offset, protected_objects,
                            protected_partials | {key})
            since_check = 0
            with open(part, 'ab') as stream:
                while True:
                    block = response.read(BLOCK)
                    if not block:
                        break
                    written += len(block)
                    if written > cap:
                        raise EntryFailure('contenu au-dela de %d octets' % cap, keep_partial=False)
                    digest.update(block)
                    stream.write(block)
                    stats['bytes_transferred'] += len(block)
                    since_check += len(block)
                    if final is None and since_check >= FREE_CHECK_EVERY:
                        since_check = 0
                        stream.flush()
                        if cache.free() < cache.min_free:
                            raise EntryFailure('plancher d\'espace libre atteint pendant le telechargement')
                stream.flush()
                os.fsync(stream.fileno())
        if final is not None and written != final:
            raise EntryFailure('telechargement incomplet : %d octets sur %d (partiel garde)' % (written, final))
    sha = digest.hexdigest()
    if expected_sha is not None and sha != expected_sha:
        raise EntryFailure('sha256 different de l\'epingle : contenu refuse', keep_partial=False)
    if cache.check_object(sha, written) == 'ok':
        cache.drop_partial(key)          # meme contenu deja range (autre nom, ou sondage anterieur)
    else:
        cache.admit(part, sha)
        cache.drop_partial(key)
    return sha, written


def fetch_with_retries(cache, entry, cap, protected_objects, protected_partials, stats, args, deadline):
    """Erreur reseau ou telechargement incomplet : nouvel essai depuis le partiel (base, 3 x base, 9 x base
    secondes) ; contenu refuse : partiel retire, et une seule relance depuis zero s'il venait d'une reprise."""
    restarted = False
    delays = [0] + [args.retry_base_seconds * 3 ** i for i in range(args.retries)]
    for attempt, delay in enumerate(delays):
        if deadline is not None and time.monotonic() + delay >= deadline:
            break                        # aucun essai, pas meme le premier, au-dela du delai
        if delay:
            time.sleep(delay)
        stats['attempts'] = attempt + 1
        try:
            return fetch(cache, entry, cap, protected_objects, protected_partials, stats, args.socket_timeout)
        except EntryFailure as error:
            stats['last_error'] = str(error)
            if not error.keep_partial:
                cache.drop_partial(partial_key(entry))
                if stats.pop('resumed_from', None) and not restarted:
                    restarted = True
                    continue
                raise
            if 'place insuffisante' in str(error) or 'plancher' in str(error):
                raise
        except urllib.error.HTTPError as error:
            stats['last_error'] = 'HTTP %d' % error.code
            if 400 <= error.code < 500 and error.code not in (408, 429):
                raise EntryFailure('HTTP %d : %s' % (error.code, error.reason)) from error
        except (urllib.error.URLError, http.client.HTTPException, OSError, ValueError) as error:
            stats['last_error'] = reason(error)
    if not stats.get('attempts'):
        stats['attempts'] = 0
        raise EntryFailure('delai (--deadline-seconds) atteint avant le premier essai : aucun telechargement')
    raise EntryFailure('echec apres %d essai(s) : %s' % (stats.get('attempts', 0), stats.get('last_error')))


def link_into(cache, directory, name, sha):
    """directory/name -> objects/<sha> : lien dur ; copie en lecture seule si autre systeme de fichiers."""
    target, source = directory / name, cache.object_path(sha)
    try:
        info = os.lstat(target)
    except FileNotFoundError:
        info = None
    if info is not None:
        origin = os.stat(source)
        if stat.S_ISREG(info.st_mode) and (info.st_ino, info.st_dev) == (origin.st_ino, origin.st_dev):
            return 'already_linked'
        raise EntryFailure('lien impossible : %s existe deja dans le dossier de liens' % name)
    try:
        os.link(source, target)
        return 'hardlink'
    except OSError as error:
        if error.errno not in (errno.EXDEV, errno.EPERM, errno.EMLINK):
            raise EntryFailure('lien impossible : ' + reason(error)) from error
    if shutil.disk_usage(directory).free - os.stat(source).st_size < cache.min_free:
        raise EntryFailure('copie impossible : plancher d\'espace libre')
    shutil.copyfile(source, target)
    os.chmod(target, 0o400)
    return 'copy'


def scrub(text, replacements):
    for value, token in replacements:
        if value and len(value) > 1:
            text = text.replace(value, token)
    return text


def write_report(path, report, replacements):
    """Rapport JSON atomique ; tout chemin du cache, du dossier de liens ou du $HOME devient un jeton."""
    if path is None:
        return
    report['written_utc'] = utc()
    text = scrub(json.dumps(report, sort_keys=True, indent=2) + '\n', replacements)
    temporary = Path(str(path) + '.partial')
    temporary.write_text(text)
    os.replace(temporary, path)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--get', nargs='+', metavar='NOM', help='entrees epinglees a rendre disponibles')
    action.add_argument('--probe', nargs='+', metavar='NOM', help='entrees a telecharger pour en lire l\'epingle')
    action.add_argument('--status', action='store_true', help='etat du cache, sans reseau')
    action.add_argument('--trim', action='store_true', help='evince jusqu\'a respecter plafond et plancher')
    action.add_argument('--purge', action='store_true', help='vide le cache (objets et partiels)')
    parser.add_argument('--manifest', help='manifeste ' + MANIFEST_SCHEMA + ' (exige par --get et --probe)')
    parser.add_argument('--cache-dir', help='racine du cache (defaut : $%s)' % CACHE_ENV)
    parser.add_argument('--link', metavar='DOSSIER', help='avec --get : liens DOSSIER/<nom> vers les objets')
    parser.add_argument('--report', metavar='FICHIER', help='rapport JSON (reecrit apres chaque entree)')
    parser.add_argument('--max-cache-bytes', type=int, default=DEFAULT_MAX_CACHE)
    parser.add_argument('--min-free-bytes', type=int, default=DEFAULT_MIN_FREE)
    parser.add_argument('--probe-max-bytes', type=int, default=DEFAULT_PROBE_MAX)
    parser.add_argument('--socket-timeout', type=float, default=60.0)
    parser.add_argument('--retries', type=int, default=3)
    parser.add_argument('--retry-base-seconds', type=float, default=5.0)
    parser.add_argument('--deadline-seconds', type=float, help='aucun nouvel essai au-dela de ce delai')
    parser.add_argument('--lock-wait', type=float, default=60.0)
    args = parser.parse_args(argv)
    started = time.monotonic()
    deadline = None if args.deadline_seconds is None else started + args.deadline_seconds
    report_path = Path(args.report).absolute() if args.report else None
    mode = next(name for name in ('get', 'probe', 'status', 'trim', 'purge') if getattr(args, name))
    report = {'schema': REPORT_SCHEMA, 'started_utc': utc(), 'mode': mode, 'entries': [], 'evicted': [],
              'cache_dir': '--cache-dir' if args.cache_dir else '$' + CACHE_ENV, 'interrupted': False,
              'exit_code': None}
    root = args.cache_dir or os.environ.get(CACHE_ENV)
    home = os.environ.get('HOME', '')
    replacements = [(str(Path(root).absolute()) if root else '', report['cache_dir']),
                    (str(Path(args.link).absolute()) if args.link else '', '<link>'),
                    (home if re.fullmatch(r'/[A-Za-z0-9._/-]*[A-Za-z0-9._-]', home) else '', '$HOME')]

    def on_signal(signum, _frame):
        for sig in SIGNALS:
            signal.signal(sig, signal.SIG_IGN)     # nettoyage court : le worker tue le groupe 10 s apres
        raise Stop('signal %d' % signum)
    for sig in SIGNALS:
        signal.signal(sig, on_signal)
    code, cache = 0, None
    try:
        try:
            if args.max_cache_bytes <= 0 or args.min_free_bytes < 0 or args.probe_max_bytes <= 0 or \
                    args.retries < 0 or args.retry_base_seconds < 0:
                raise Usage('plafonds, plancher et relances : entiers positifs')
            if args.link and not args.get:
                raise Usage('--link ne sert qu\'avec --get')
            entries, names = {}, args.get or args.probe or []
            if names:
                if not args.manifest:
                    raise Usage('--manifest exige avec --get et --probe')
                entries, manifest_sha = load_manifest(args.manifest)
                report.update(manifest_sha256=manifest_sha, manifest_name=Path(args.manifest).name)
                unknown = [name for name in names if name not in entries]
                if unknown or len(set(names)) != len(names):
                    raise Usage('noms absents du manifeste ou dupliques : ' + ', '.join(unknown or names))
                unpinned = [name for name in names if entries[name]['sha256'] is None]
                if args.get and unpinned:
                    raise Usage('--get exige une epingle sha256 et size : ' + ', '.join(unpinned) +
                                ' (--probe pour la lire)')
            if not root:
                raise Usage('racine du cache absente : --cache-dir ou $' + CACHE_ENV)
            link_dir = None
            if args.link:
                link_dir = Path(args.link).absolute()
                out = os.environ.get('V12_OUT')     # {out} de la commande, exporte par v12_worker.sh
                results = None
                if out and len(Path(out).absolute().parents) > 2:
                    results = Path(out).absolute().parents[2]          # $WORK/results : tout y est rapatrie

                def inside_results(path):
                    """Sous les resultats, lexicalement ou reellement (un ancetre symbolique ne contourne rien)."""
                    if results is None:
                        return False
                    real, real_results = Path(os.path.realpath(path)), Path(os.path.realpath(results))
                    return path == results or results in path.parents or real == real_results or \
                        real_results in real.parents
                if inside_results(link_dir):
                    raise Usage('--link dans les resultats rapatries ({out}) : interdit, lier dans {build}')
                link_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
                if link_dir.is_symlink() or not link_dir.is_dir() or inside_results(link_dir):
                    raise Usage('--link : dossier reel attendu, hors des resultats rapatries ({out})')
                report['link_dir_name'] = link_dir.name
            cache = Cache(Path(root).absolute(), args.max_cache_bytes, args.min_free_bytes, args.lock_wait)
            if args.purge:
                for sha, size, _, links in cache.object_entries():
                    cache.drop_object(sha)
                    cache.evicted.append({'kind': 'object', 'sha256': sha, 'bytes': size, 'links': links,
                                          'freed_disk_bytes': size if links == 1 else 0})
                for key, size, _ in cache.partial_list():
                    cache.drop_partial(key)
                    cache.evicted.append({'kind': 'partial', 'key': key, 'bytes': size, 'freed_disk_bytes': size})
            elif args.trim:
                try:
                    cache.make_room(0, set(), set())
                except EntryFailure as error:
                    report['error'] = str(error)
                    code = 1
            protected_objects = {entries[name]['sha256'] for name in names if entries[name]['sha256']}
            protected_partials = {partial_key(entries[name]) for name in names}
            for name in names:
                entry = entries[name]
                row = {'name': name, 'url': entry['url'], 'license': entry['license'],
                       'pinned_sha256': entry['sha256'], 'pinned_size': entry['size'], 'status': None}
                report['entries'].append(row)
                stats, begun = {'bytes_transferred': 0}, time.monotonic()
                try:
                    found = cache.check_object(entry['sha256'], entry['size']) if entry['sha256'] else 'absent'
                    if found == 'ok':
                        row.update(status='hit', sha256=entry['sha256'], size=entry['size'])
                    else:
                        cap = entry['size'] or args.probe_max_bytes
                        sha, size = fetch_with_retries(cache, entry, cap, protected_objects, protected_partials,
                                                       stats, args, deadline)
                        row.update(sha256=sha, size=size, status='probed' if args.probe else
                                   'redownloaded' if found == 'altered' else
                                   'resumed' if stats.get('resumed_from') else 'downloaded')
                    if link_dir is not None:
                        row['link'] = link_into(cache, link_dir, name, row['sha256'])
                except EntryFailure as error:
                    row.update(status='failed', error=str(error))
                    code = 1
                for item in ('bytes_transferred', 'attempts', 'resumed_from', 'restarted'):
                    if item in stats:
                        row[item] = stats[item]
                row['seconds'] = round(time.monotonic() - begun, 3)
                if stats['bytes_transferred'] and row['seconds'] > 0:
                    row['mib_per_second'] = round(stats['bytes_transferred'] / 2 ** 20 / row['seconds'], 2)
                report['evicted'] = list(cache.evicted)
                write_report(report_path, report, replacements)
        except (Usage, Unusable) as error:
            report['error'] = str(error)
            code = error.code
    except Stop as error:
        report.update(interrupted=True, error='interrompu (%s) : partiels gardes pour reprise' % error)
        code = 1
        for row in report['entries']:
            if row.get('status') is None:
                row['status'] = 'interrupted'
    finally:
        try:
            if cache is not None:
                report['evicted'] = list(cache.evicted)
                report['cache'] = cache.status()
            report.update(exit_code=code, seconds=round(time.monotonic() - started, 3))
            write_report(report_path, report, replacements)
        except BaseException as error:   # noqa: B902 -- le code reste celui du travail fait
            print('rapport non ecrit : ' + reason(error), file=sys.stderr)
            code = code or 1
    print(scrub(json.dumps({'mode': mode, 'exit_code': code, 'error': report.get('error'),
                            'entries': [[row.get('name'), row.get('status')] for row in report['entries']]},
                           sort_keys=True), replacements))
    return code


if __name__ == '__main__':
    sys.exit(main())
