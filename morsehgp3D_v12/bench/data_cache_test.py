#!/usr/bin/env python3
"""Porte de l'eviction et des gardes du cache de donnees de la VM (bench/data_cache.py) : constat CST-0220 et les deux
observations secondaires du recu audit_session_t1_20261007/session (sans numero).

Aucun reseau (urlopen interdit), aucun gros fichier : de vrais liens durs de quelques octets, l'espace libre seul est
modele (comme le temoin de l'auditeur), pour reproduire une penurie sans remplir le disque.
  - temoin de l'auditeur : objet de six octets lie dans {build} (st_nlink = 2), espace libre nul, make_room(1) :
    refus AVANT toute eviction, objet garde (le code d'origine l'evincait puis refusait) ;
  - plancher : seul un objet sans autre lien dur est evince (octets rendus publies), jamais l'objet encore lie ;
  - plafond seul : un objet encore lie peut etre evince (le plafond est logique), octets rendus au disque : 0 ;
  - espace libre qui baisse pendant l'eviction : refus apres eviction partielle, chaque entree evincee au rapport ;
  - delai (--deadline-seconds) deja atteint : aucun essai, pas meme le premier (le code d'origine en lancait un) ;
  - --link sous un ancetre symbolique des resultats rapatries : refus code 2, rien n'est lie (le code d'origine liait).
Usage : python3 -S -O bench/data_cache_test.py      Codes : 0 conforme, 1 ecart (detail en JSON).
"""
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import signal
import sys
import tempfile
import types
from unittest.mock import patch

sys.dont_write_bytecode = True
ICI = Path(__file__).resolve().parent


class Ecart(Exception):
    pass


def exiger(condition, message):
    if not condition:
        raise Ecart(message)


def charger():
    spec = importlib.util.spec_from_file_location('data_cache_porte', ICI / 'data_cache.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def objet(module, cache, corps):
    sha = hashlib.sha256(corps).hexdigest()
    chemin = cache.object_path(sha)
    chemin.write_bytes(corps)
    chemin.chmod(0o400)
    return sha, chemin


def ouvrir(module, racine, nom, plafond=1000, plancher=0):
    return module.Cache(racine / nom, plafond, plancher, 0)


def fermer(cache):
    os.close(cache.lock)


def porte_eviction(module, racine):
    lignes = []
    build = racine / 'build'
    build.mkdir()
    # 1. Temoin de l'auditeur.
    cache = ouvrir(module, racine, 'c1')
    try:
        sha, source = objet(module, cache, b'abcdef')
        exiger(module.link_into(cache, build, 'sample', sha) == 'hardlink', 'lien dur attendu')
        exiger(source.stat().st_nlink == 2, 'inode partage attendu')
        with patch.object(cache, 'free', return_value=0):
            try:
                cache.make_room(1, set(), set())
                refuse = False
            except module.EntryFailure as error:
                refuse, message = True, str(error)
        exiger(refuse and 'rien n\'est evince' in message, 'temoin de l\'auditeur : refus attendu')
        exiger(source.exists() and cache.evicted == [], 'temoin de l\'auditeur : objet evince malgre le refus')
        lignes.append({'cas': 'temoin_auditeur_lien_dur', 'refus': True, 'objet_garde': True, 'evinces': 0})
    finally:
        fermer(cache)
    # 2. Plancher : deux objets, l'ancien encore lie, le recent seul ; espace libre modele = octets rendus.
    cache = ouvrir(module, racine, 'c2')
    try:
        lie, chemin_lie = objet(module, cache, b'L' * 64)
        os.utime(chemin_lie, (1, 1))  # le plus ancien : premier candidat d'une eviction LRU naive
        exiger(module.link_into(cache, build, 'lie', lie) == 'hardlink', 'lien dur attendu (2)')
        seul, chemin_seul = objet(module, cache, b'S' * 32)
        etat = {'libre': 10}

        def libre():
            rendu = sum(item.get('freed_disk_bytes', 0) for item in cache.evicted)
            return etat['libre'] + rendu
        with patch.object(cache, 'free', side_effect=libre):
            cache.make_room(20, set(), set())  # plancher 0 : il faut 20 libres, 10 + 32 rendus suffisent
        exiger(chemin_lie.exists() and not chemin_seul.exists(), 'plancher : objet lie evince ou objet seul garde')
        exiger(cache.evicted == [{'kind': 'object', 'sha256': seul, 'bytes': 32, 'links': 1,
                                  'freed_disk_bytes': 32}], 'plancher : rapport %r' % cache.evicted)
        lignes.append({'cas': 'plancher_jamais_un_objet_lie', 'evince': 'objet seul (32 octets rendus)',
                       'garde': 'objet lie (64 octets)'})
    finally:
        fermer(cache)
    # 3. Plafond seul : l'objet lie peut sortir du cache (plafond logique), aucun octet rendu au disque.
    cache = ouvrir(module, racine, 'c3', plafond=100)
    try:
        lie, chemin_lie = objet(module, cache, b'P' * 64)
        exiger(module.link_into(cache, build, 'plafond', lie) == 'hardlink', 'lien dur attendu (3)')
        with patch.object(cache, 'free', return_value=1 << 40):
            cache.make_room(50, set(), set())  # 64 + 50 > 100 : plafond depasse
        exiger(not chemin_lie.exists() and (build / 'plafond').read_bytes() == b'P' * 64,
               'plafond : objet non evince, ou lien du build touche')
        exiger(cache.evicted == [{'kind': 'object', 'sha256': lie, 'bytes': 64, 'links': 2, 'freed_disk_bytes': 0}],
               'plafond : rapport %r' % cache.evicted)
        lignes.append({'cas': 'plafond_objet_lie_evince', 'octets_rendus_au_disque': 0})
    finally:
        fermer(cache)
    # 4. Espace libre qui baisse pendant l'eviction (autre processus) : refus apres eviction partielle, publie.
    cache = ouvrir(module, racine, 'c4')
    try:
        a, chemin_a = objet(module, cache, b'a' * 16)
        os.utime(chemin_a, (1, 1))
        b, chemin_b = objet(module, cache, b'b' * 16)
        lectures = {'n': 0}

        def libre_qui_baisse():
            lectures['n'] += 1
            return 20 if lectures['n'] <= 3 else 0  # decision prise sur 20 libres ; puis le disque se remplit
        with patch.object(cache, 'free', side_effect=libre_qui_baisse):
            try:
                cache.make_room(30, set(), set())
                refuse = False
            except module.EntryFailure as error:
                refuse, message = True, str(error)
        exiger(refuse and 'eviction partielle' in message, 'baisse pendant l\'eviction : refus explicite attendu')
        exiger(len(cache.evicted) == 2 and all(item['freed_disk_bytes'] == 16 for item in cache.evicted),
               'baisse pendant l\'eviction : entrees evincees non publiees %r' % cache.evicted)
        lignes.append({'cas': 'espace_libre_qui_baisse', 'refus': 'apres eviction partielle, publie',
                       'evinces': len(cache.evicted)})
    finally:
        fermer(cache)
    return lignes


def porte_delai(module):
    corps = b'abcdef'
    sha = hashlib.sha256(corps).hexdigest()
    args = types.SimpleNamespace(retries=0, retry_base_seconds=0, socket_timeout=1)
    with patch.object(module.time, 'monotonic', return_value=20), \
            patch.object(module, 'fetch', return_value=(sha, len(corps))) as fetch:
        stats = {}
        try:
            module.fetch_with_retries(None, {'sha256': sha}, 6, set(), set(), stats, args, deadline=10)
            refuse = False
        except module.EntryFailure as error:
            refuse, message = True, str(error)
    exiger(refuse and fetch.call_count == 0 and 'avant le premier essai' in message and stats.get('attempts') == 0,
           'delai depasse : %d essai(s)' % fetch.call_count)
    with patch.object(module.time, 'monotonic', return_value=5), \
            patch.object(module, 'fetch', return_value=(sha, len(corps))) as fetch:
        value = module.fetch_with_retries(None, {'sha256': sha}, 6, set(), set(), {}, args, deadline=10)
    exiger(fetch.call_count == 1 and value == (sha, 6), 'dans le delai : essai attendu')
    return [{'cas': 'delai_depasse_aucun_essai', 'essais': 0}, {'cas': 'dans_le_delai_un_essai', 'essais': 1}]


def porte_lien_symbolique(module, racine):
    corps = b'abcdef'
    sha = hashlib.sha256(corps).hexdigest()
    cache_dir = racine / 'cache_secondaire'
    cache = module.Cache(cache_dir, 100, 0, 0)
    cache.object_path(sha).write_bytes(corps)
    fermer(cache)
    resultats = racine / 'work' / 'results'
    sortie = resultats / 'cmd' / '000_sample' / 'files'
    sortie.mkdir(parents=True)
    alias = racine / 'alias_results'
    alias.symlink_to(resultats, target_is_directory=True)
    manifeste = racine / 'datasets.json'
    manifeste.write_text(json.dumps({'schema': module.MANIFEST_SCHEMA, 'datasets': [
        {'name': 'sample', 'url': 'https://example.invalid/sample', 'license': 'synthetique', 'sha256': sha,
         'size': len(corps)}]}))
    rapport = racine / 'rapport.json'
    gestionnaires = {sig: signal.getsignal(sig) for sig in module.SIGNALS}
    try:
        with patch.dict(os.environ, {'V12_OUT': str(sortie)}), contextlib.redirect_stdout(io.StringIO()):
            code = module.main(['--get', 'sample', '--cache-dir', str(cache_dir), '--manifest', str(manifeste),
                                '--min-free-bytes', '0', '--lock-wait', '0', '--report', str(rapport),
                                '--link', str(alias / 'cmd' / '000_sample' / 'files' / 'data')])
            code_build = module.main(['--get', 'sample', '--cache-dir', str(cache_dir), '--manifest', str(manifeste),
                                      '--min-free-bytes', '0', '--lock-wait', '0',
                                      '--link', str(racine / 'build_lie')])
    finally:
        for sig, gestionnaire in gestionnaires.items():
            signal.signal(sig, gestionnaire)
    erreur = json.loads(rapport.read_text()).get('error', '')
    exiger(code == 2 and '{out}' in erreur and not (sortie / 'data').exists(),
           'ancetre symbolique des resultats : code %d, lien %s' % (code, (sortie / 'data').exists()))
    exiger(code_build == 0 and (racine / 'build_lie' / 'sample').read_bytes() == corps, 'lien dans {build} refuse')
    return [{'cas': 'link_sous_ancetre_symbolique_des_resultats', 'code': code},
            {'cas': 'link_dans_build', 'code': code_build}]


def main(argv):
    if len(argv) != 1:
        print(__doc__)
        return 2
    module = charger()
    resultats, ecarts = [], []
    with patch.object(module.urllib.request, 'urlopen', side_effect=RuntimeError('reseau interdit par la porte')):
        with tempfile.TemporaryDirectory(prefix='mhgp12-cache-') as tmp:
            racine = Path(tmp)
            for nom, porte in (('eviction', lambda: porte_eviction(module, racine)), ('delai', lambda: porte_delai(module)),
                               ('lien', lambda: porte_lien_symbolique(module, racine))):
                try:
                    resultats += porte()
                except Ecart as e:
                    ecarts.append('%s : %s' % (nom, e))
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({'porte': 'data_cache_v12', 'cas': len(resultats), 'ecarts': ecarts, 'optimise': sys.flags.optimize,
                      'python': sys.version.split()[0]}, ensure_ascii=False))
    return 0 if not ecarts else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
