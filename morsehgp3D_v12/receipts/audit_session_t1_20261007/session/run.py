#!/usr/bin/env python3
"""Contre-preuves locales du port session v12. Aucun appel reseau/GCP/GPU.

python3 -O run.py [--session /chemin/session/historique] [--output RESULT.json]
Les empreintes de MANIFEST.json sont exigees avant et apres les controles.
--session ne lit que les petits recus et l'archive de resultats epingles ; jamais les donnees.
"""
import argparse
import ast
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import types
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
CHECKS = 0


def check(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(manifest):
    for name, expected in manifest['sources_sha256'].items():
        check(digest(ROOT / name) == expected, 'source modifiee : ' + name)
    for name, expected in manifest['witness_sha256'].items():
        check(digest(HERE / name) == expected, 'temoin modifie : ' + name)


def load(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def publication_cases(module, root):
    session = root / 'session'
    files = session / 'results/extracted/results'
    files.mkdir(parents=True)
    (session / 'receipt.json').write_text('{}')
    account = 'audit@example.invalid'
    (session / 'preflight.json').write_text(json.dumps({'gcloud_account': account}))
    (files / (account + '.log')).write_text('Contenu anonyme.\n')
    destination = root / 'public'
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        code = module.main(['recu_session', '--session', str(session), '--dest', str(destination),
                            '--include', '*'])
    check(code == 0, 'CST0219 : code 0 attendu')
    check((destination / 'resultats' / (account + '.log')).is_file(), 'CST0219 : nom non expurge attendu')
    check(account in (destination / 'SHA256SUMS').read_text(), 'CST0219 : identite dans manifeste attendue')
    check(account not in (destination / 'receipt.json').read_text(), 'CST0219 : contenu du recu expurge')
    old = root / 'destination_preexistante'
    old.mkdir()
    sentinel = old / 'travail_preexistant.bin'
    sentinel.write_bytes(b'\0fichier preexistant /home/exemple')
    check(sentinel.is_file(), 'CST0221 : sentinelle absente avant appel')
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        code2 = module.main(['recu_session', '--session', str(session), '--dest', str(old)])
    check(code2 == 3, 'CST0221 : refus identite code 3 attendu')
    check(not old.exists(), 'CST0221 : destination preexistante effacee attendue')
    return {'CST0219': {'exit_code': code, 'identity_in_filename': True, 'identity_in_SHA256SUMS': True},
            'CST0221': {'exit_code': code2, 'preexisting_destination_removed': True,
                        'scope': 'repertoire temporaire synthetique uniquement'}}


def cache_case(module, root):
    # Le systeme de fichiers cree un vrai lien dur. Seul le compteur d'espace libre
    # est modele (zero), pour reproduire une penurie sans remplir le disque.
    cache = module.Cache(root / 'cache', 100, 0, 0)
    try:
        body = b'abcdef'
        sha = hashlib.sha256(body).hexdigest()
        source = cache.object_path(sha)
        source.write_bytes(body)
        source.chmod(0o400)
        build = root / 'build'
        build.mkdir()
        check(module.link_into(cache, build, 'sample', sha) == 'hardlink', 'CST0220 : vrai lien dur attendu')
        link = build / 'sample'
        check(source.stat().st_nlink == 2 and source.stat().st_ino == link.stat().st_ino,
              'CST0220 : inode partage attendu')
        with patch.object(cache, 'free', return_value=0):
            try:
                cache.make_room(1, set(), set())
            except module.EntryFailure:
                refused = True
            else:
                refused = False
        check(refused, 'CST0220 : refus final attendu')
        check(not source.exists(), 'CST0220 : objet evince attendu')
        check(link.read_bytes() == body and link.stat().st_nlink == 1, 'CST0220 : inode vivant apres eviction')
        check(cache.evicted == [{'kind': 'object', 'sha256': sha, 'bytes': 6}], 'CST0220 : evacuation comptabilisee')
        return {'CST0220': {'refused': True, 'evicted_bytes': 6, 'live_hardlink_bytes': 6,
                            'disk_free_model': '0 avant et apres : inode encore reference',
                            'large_allocation': False}}
    finally:
        os.close(cache.lock)


def lifecycle_comparison():
    old = (ROOT / 'gcp-migration/v11_session.py').read_text()
    for before, after in [('v11', 'v12'), ('V11', 'V12'), ('mhgp11', 'mhgp12'), ('MHGP11', 'MHGP12')]:
        old = old.replace(before, after)
    new = (ROOT / 'gcp-migration/v12_session.py').read_text()
    def functions(text):
        return {node.name: node for node in ast.parse(text).body if isinstance(node, ast.FunctionDef)}
    before, after = functions(old), functions(new)
    names = ['read_generation', 'recorded_target', 'stop_decision', 'stopped_after', 'close_by_generation',
             'observe', 'session_processes', 'take_lock', 'runner_env', 'safe_extract', 'verify_manifest']
    for name in names:
        check(ast.dump(before[name]) == ast.dump(after[name]), 'AST critique modifie : ' + name)
    for script in ('gcp-migration/v12_worker.sh', 'gcp-migration/start_and_verify.sh',
                   'gcp-migration/stop_and_verify.sh'):
        process = subprocess.run(['bash', '-n', str(ROOT / script)], capture_output=True, text=True, timeout=10)
        check(process.returncode == 0, 'syntaxe shell : ' + script)
    return {'critical_functions_AST_identical_after_rename': names, 'shell_syntax_checked': 3}


def secondary_cases(module, root):
    cache = module.Cache(root / 'cache_secondary', 100, 0, 0)
    body = b'abcdef'
    sha = hashlib.sha256(body).hexdigest()
    cache.object_path(sha).write_bytes(body)
    os.close(cache.lock)
    results = root / 'work/results'
    out = results / 'cmd/000_sample/files'
    out.mkdir(parents=True)
    alias = root / 'alias_results'
    alias.symlink_to(results, target_is_directory=True)
    manifest_path = root / 'datasets.json'
    manifest_path.write_text(json.dumps({'schema': module.MANIFEST_SCHEMA, 'datasets': [
        {'name': 'sample', 'url': 'https://example.invalid/sample', 'license': 'synthetic',
         'sha256': sha, 'size': len(body)}]}))
    made, constructor = [], module.Cache
    handlers = {sig: signal.getsignal(sig) for sig in module.SIGNALS}
    def create(*args):
        value = constructor(*args)
        made.append(value)
        return value
    try:
        with patch.dict(os.environ, {'V12_OUT': str(out)}), patch.object(module, 'Cache', side_effect=create), \
                contextlib.redirect_stdout(io.StringIO()):
            code = module.main(['--get', 'sample', '--cache-dir', str(root / 'cache_secondary'),
                                '--manifest', str(manifest_path), '--min-free-bytes', '0',
                                '--link', str(alias / 'cmd/000_sample/files/data')])
    finally:
        for item in made:
            os.close(item.lock)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    check(code == 0 and (out / 'data/sample').read_bytes() == body, 'observation : lien via ancetre symlink accepte')
    with patch.object(module.time, 'monotonic', return_value=20), \
            patch.object(module, 'fetch', return_value=(sha, len(body))) as fetch:
        value = module.fetch_with_retries(None, {}, 6, set(), set(), {},
                                         types.SimpleNamespace(retries=0, retry_base_seconds=0, socket_timeout=1),
                                         deadline=10)
    check(fetch.call_count == 1 and value == (sha, 6), 'observation : premier fetch apres deadline')
    return {'results_destination_alias': {'exit_code': code, 'linked_under_results': True,
                                         'historical_publication_affected': 'non etabli'},
            'cache_deadline': {'clock': 20, 'deadline': 10, 'fetch_calls': 1,
                               'worker_external_deadline_tested': False}}


def historical_session(path, manifest, controller):
    # Aucune valeur d'identite n'est ajoutee au resultat. Ne jamais imprimer ces JSON bruts.
    for name, expected in manifest['historical_session_sha256'].items():
        check(digest(path / name) == expected, 'trace historique modifiee : ' + name)
    raw = json.loads((path / 'receipt.json').read_text())
    preflight = json.loads((path / 'preflight.json').read_text())
    handoff = json.loads((path / 'host/handoff.json').read_text())
    generation = raw['generation']
    check(raw['status'] == 'completed' and raw['closure'] == 'stopped', 'etat historique attendu')
    check(raw['targeted_shutdown_certified'] is True, 'arret historique certifie attendu')
    check(handoff['last_start_timestamp'] == generation == raw['closing_generation'], 'generation historique unique')
    check(raw['observed_before_stop']['lastStartTimestamp'] == generation and
          raw['observed_before_stop']['status'] == 'RUNNING', 'etat avant arret attendu')
    check(raw['observed_after']['lastStartTimestamp'] == generation and
          raw['observed_after']['status'] == 'TERMINATED', 'relecture historique TERMINATED attendue')
    check(raw['stop_attempts'] == 1 and raw['stop_exit_code'] == 0, 'arret historique unique code 0 attendu')
    check(raw['oslogin_key_removed'] and raw['private_key_deleted'], 'effacement historique des cles attendu')
    check(raw['results_verified'] and not raw['errors'] and not raw['warnings'], 'resultats historiques verifies')
    check(digest(path / 'results/results.tar.gz') == raw['results_sha256'], 'hash archive historique')
    for relative, expected in preflight['protocol_sha256'].items():
        check(digest(ROOT / relative) == expected, 'protocole historique different : ' + relative)
    for relative, expected in controller.GUARD_PINS.items():
        check(digest(path / 'host' / Path(relative).name) == expected, 'garde historique differente')
    return {'mode': 'lecture historique seulement', 'status': raw['status'], 'closure': raw['closure'],
            'same_generation_before_after': True, 'after_status': 'TERMINATED', 'stop_attempts': 1,
            'keys_removed': True, 'protocol_matches_audited_sources': True,
            'source_kind': raw['source_kind'], 'source_head': raw['source']['head_commit'],
            'package_sha256': raw['package_sha256'], 'results_sha256': raw['results_sha256']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    manifest = json.loads((HERE / 'MANIFEST.json').read_text())
    verify(manifest)
    head_before = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    publication = load('audit_publication', 'morsehgp3D_v12/microbancs/outils/recu_session.py')
    cache = load('audit_cache', 'morsehgp3D_v12/bench/data_cache.py')
    controller = load('audit_controller', 'gcp-migration/v12_session.py')
    result = {'schema': 'audit.session_t1.v1', 'audited_commit': manifest['audited_commit'],
              'current_head': head_before, 'python_optimized': bool(sys.flags.optimize),
              'cloud_calls': 0, 'gpu_calls': 0, 'real_session_actions': 0}
    # Interdiction defensive du reseau, y compris en cas de regression du temoin de cache.
    with patch.object(cache.urllib.request, 'urlopen', side_effect=RuntimeError('reseau interdit par audit')):
        with tempfile.TemporaryDirectory(prefix='ehgp-v12-session-audit-') as temp:
            root = Path(temp)
            result['findings'] = publication_cases(publication, root)
            result['findings'].update(cache_case(cache, root))
            result['secondary_observations'] = secondary_cases(cache, root)
    result['port'] = lifecycle_comparison()
    target = subprocess.run([sys.executable, '-O', str(ROOT / 'gcp-migration/v12_target_selftest.py')],
                            capture_output=True, text=True, timeout=30)
    check(target.returncode == 0 and target.stdout.strip() == 'v12_target_verdict conforme checks70 native0 cloud0',
          'target_selftest -O : 70 controles attendus')
    result['target_selftest'] = {'exit_code': 0, 'checks': 70, 'optimized': True,
                               'cloud': 'double de test', 'native': 0}
    if args.session:
        result['historical_session'] = historical_session(args.session, manifest, controller)
    else:
        result['historical_session'] = {'mode': 'non rejouee : --session absent'}
    verify(manifest)
    check(subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip() == head_before,
          'HEAD a change durant le controle')
    result.update(source_hashes_before_after='identiques aux empreintes du commit audite', checks=CHECKS)
    encoded = json.dumps(result, indent=2, sort_keys=True) + '\n'
    if args.output:
        args.output.write_text(encoded)
    print(encoded, end='')


if __name__ == '__main__':
    main()
