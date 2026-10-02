"""Lecteur LIVE des sessions catalogue : brut local et archive unique obligatoires.

Code0 signifie coherence des pieces, meme si campagne echouee/non executee. Aucune execution native/cloud,
aucune extraction tar, aucun affichage de flux bruts. Les binaires canoniques mesures ne sont pas archives :
leurs empreintes declarees sont comparees, jamais presentees comme recalculees par ce lecteur.
"""
import hashlib
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('foundation_reader', HERE.parent / 'developpement_20261002/check.py')
foundation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(foundation)
js, fields, need, sha = foundation.js, foundation.fields, foundation.need, foundation.sha
BASE = foundation.BASE
BENCH = 'results/cmd/001_catalogue/'
CAP = 80 * 1024**2
HEX = re.compile('[0-9a-f]{64}')
EARLY = {'configure_failed', 'list_failed', 'timeout', 'requirement_missing', 'not_run_deadline', 'interrupted',
         'internal_error'}
CASE_COUNTS = dict(lidar_ng00=39885, lidar_ng01=35551, lidar_ng02=45845,
                   uniform_u18_n8000=8000, uniform_u18_n16000=16000, uniform_u18_n32000=32000)


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def read_capture(folder):
    receipt = js((folder / 'receipt.json').read_bytes())
    raw_bytes = Path(receipt['raw_receipt_local']).read_bytes()
    need(sha(raw_bytes) == receipt['original_receipt_sha256'], 'hash_brut_local')
    raw = js(raw_bytes)
    need(all(json.dumps(raw[k], sort_keys=True) == json.dumps(v, sort_keys=True)
             for k, v in receipt.items() if k in raw), 'compact_different_du_brut')
    need(receipt['schema'] == 'ehgp.v11.session_receipt.v1' and receipt['source_kind'] == 'commit' and
         receipt['evidence_grade'] == 'pushed_commit' and re.fullmatch('[0-9a-f]{40}', receipt['commit']), 'source')
    need(receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'] is True and
         receipt['results_verified'] is True and receipt['stop_exit_code'] == 0 and not receipt['errors'], 'cloture')
    need(receipt['generation'] == receipt['closing_generation'] == receipt['observed_after']['lastStartTimestamp']
         and receipt['observed_after']['status'] == 'TERMINATED', 'generation')
    path = folder / 'results.tar.gz'
    need(path.stat().st_size == receipt['results_bytes'] and digest(path) == receipt['results_sha256'], 'archive')
    data, seen, size = {}, set(), 0
    with tarfile.open(path, 'r:gz') as archive:
        for member in archive:
            name = PurePosixPath(member.name)
            need(member.name not in seen and not name.is_absolute() and '..' not in name.parts and
                 (member.isfile() or member.isdir()), 'membre_tar')
            seen.add(member.name)
            size += member.size
            need(0 <= member.size <= CAP and size <= CAP, 'taille_tar')
            if member.isfile():
                data[member.name] = archive.extractfile(member).read()
    need(size == receipt['results_expanded_bytes'], 'taille_decompressee')
    worker = fields(data['results/worker.txt'])
    need(worker['source'] == 'commit:' + receipt['commit'] and worker['generation'] == receipt['generation'] and
         worker['package_sha256'] == receipt['package_sha256'], 'source_archive')
    return receipt, worker, data


def provenance(config, data):
    path = BASE + config['name'] + '/build_provenance.json'
    if path not in data:
        need(config['status'] != 'ok', 'provenance_absente_configuration_conforme')
        return {}
    value = js(data[path])
    need(value['schema'] == 'ehgp.v11.build_provenance.v1' and
         value['complete'] is (not bool(value['errors'])), 'provenance_statut')
    names = [row['path'] for row in value['files']]
    need(len(names) == len(set(names)), 'provenance_doublon')
    for row in value['files']:
        relative = PurePosixPath(row['path'])
        need(not relative.is_absolute() and '..' not in relative.parts and HEX.fullmatch(row['sha256']) and
             type(row['size']) is int and row['size'] >= 0, 'provenance_champs')
        if 'text' in row:
            text = row['text'].encode('utf-8')
            need(len(text) == row['size'] and sha(text) == row['sha256'], 'provenance_texte_hash')
    if config['status'] == 'ok':
        need(value['complete'] is True and 'CMakeCache.txt' in names, 'provenance_configuration_conforme')
        if config['name'] != 'style':
            need({'libmhgp11.a', 'mhgp11_catalogue_bench'} <= set(names), 'provenance_binaires_manquants')
    return {row['path']: row for row in value['files']}


def matrix(data, folder):
    source = data[BASE + 'summary.json']
    need(source == (folder / 'matrix.json').read_bytes(), 'copie_matrice')
    summary = js(source)
    configs = summary['configurations']
    names = [c['name'] for c in configs]
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         len(names) == len(foundation.NAMES) and set(names) == foundation.NAMES and
         len(summary['requested']) == len(names) and set(summary['requested']) == set(names), 'matrice_inventaire')
    statuses = {c['name']: c['status'] for c in configs}
    need(summary['statuses'] == statuses, 'matrice_statuts')
    counts, builds = {}, {}
    for config in configs:
        name = config['name']
        if config['status'] in EARLY and 'tests' not in config:
            need(js(data[BASE + name + '/result.json']) == config and config['conforming'] is False,
                 'echec_precoce_configuration')
            counts[name] = (0, 0, 0, 0)
        else:
            counts[name] = foundation.judge_config(config, data)
        builds[name] = provenance(config, data)
    failed = [s for s in statuses.values() if s not in ('ok', 'absent')]
    code = (1 if summary.get('signals') or any(s not in ('vacuous', 'incomplete', 'floor_violated') for s in failed)
            else 3 if failed else 0)
    need(summary['exit_code'] == code and summary['conforming'] is (code == 0), 'matrice_verdict')
    return summary, counts, builds, code


def inputs(folder, receipt):
    source = (folder / 'inputs.json').read_bytes()
    manifest = js(source)
    cases = manifest['cases']
    need(len(cases) == 6 and {c['name']: c['count'] for c in cases} == CASE_COUNTS, 'entrees_inventaire')
    rows = receipt['data_files']
    need(len({r['name'] for r in rows}) == len(rows), 'donnees_upload_doublon')
    uploaded = {r['name']: r for r in rows}
    need(uploaded['manifest.json']['sha256'] == sha(source) and uploaded['manifest.json']['size'] == len(source),
         'manifest_entrees_upload')
    for case in cases:
        for key, hash_key, width in [('coordinates', 'sha256', 12), ('point_ids', 'ids_sha256', 4)]:
            need(uploaded[case[key]]['sha256'] == case[hash_key] and
                 uploaded[case[key]]['size'] == width * case['count'], 'identite_entrees_upload')
    return manifest, sha(source)


def unit(row):
    return row['case'], row['kmax'], row['repetition']


def attempt_events(row, v2):
    if not v2:
        return [js(line) for line in row['stdout'].splitlines() if line.startswith('{')]
    errors = row['errors']
    need(type(errors) is list and all(type(e) is dict and set(e) == {'stage', 'type', 'message'} and
         all(type(v) is str for v in e.values()) and e['stage'] in
         ('launch', 'process', 'events', 'success', 'artifact', 'cleanup') for e in errors), 'bench_erreurs_v2')
    need(type(row['stdout']) is str and type(row['stderr']) is str, 'bench_flux_v2')
    parsed, invalid = [], 0
    for line in row['stdout'].splitlines():
        if not line.strip():
            continue
        try:
            event = js(line)
            need(type(event) is dict, 'evenement_non_objet')
            parsed.append(event)
        except (foundation.Refusal, ValueError, TypeError):
            invalid += 1
    stages = [e['stage'] for e in errors]
    need(stages.count('events') == invalid, 'bench_compte_erreurs_evenements')
    state, code = row['status'], row['exit_code']
    if state in ('ok', 'nondeterministic'):
        need(not errors, 'bench_erreur_verdie')
    elif state == 'launch_error':
        need(code is None and 'launch' in stages, 'bench_lancement_v2')
    elif state == 'invalid_output':
        need(code == 0 and any(s in ('events', 'success') for s in stages), 'bench_sortie_invalide_v2')
    elif state == 'artifact_error':
        need(code == 0 and any(s in ('artifact', 'cleanup') for s in stages), 'bench_artefact_v2')
    elif state == 'timeout':
        need(code is None and 'process' in stages, 'bench_delai_v2')
    elif state == 'refused':
        need(code == 2, 'bench_refus_v2')
    elif state == 'failed':
        need(type(code) is int and code not in (0, 2), 'bench_echec_v2')
    else:
        need(False, 'bench_statut_v2_inconnu')
    return parsed


def benchmark(data, folder, report, manifest, manifest_hash, builds, qualified):
    need(qualified and report['schema'] == 'ehgp.v11.catalogue_benchmark.v1' and report['manifest'] == manifest and
         report['manifest_sha256'] == manifest_hash and report['qualification_sha256'] == sha(data[BASE + 'summary.json'])
         and report['executable_sha256'] == builds['gcc_release']['mhgp11_catalogue_bench']['sha256'], 'bench_identite')
    need(report['full_contract'] == 'not_testable_missing_tower' and report['requested_runs'] == 36 and
         report['repetitions_requested'] == 3, 'bench_perimetre')
    need(type(report['complete']) is bool and
         (report['complete'] or report.get('full_schedule_completed', False) is False), 'bench_partiel_verdi')
    attempt_schema = report.get('attempt_schema')
    need(attempt_schema in (None, 'ehgp.v11.catalogue_attempt.v2'), 'bench_schema_tentatives')
    v2 = attempt_schema is not None
    need(not v2 or 'leaf_size' in report, 'bench_feuille_absente')
    leaf = report['leaf_size'] if v2 else report.get('leaf_size', 32)
    need(type(leaf) is int and (13 <= leaf <= 256 if v2 else leaf == 32), 'bench_feuille')
    cases = {c['name']: c for c in manifest['cases']}
    expected = {(name, k, r) for name in cases for k in (5, 10) for r in range(3)}
    observed, omitted = [unit(r) for r in report['runs']], [unit(r) for r in report['not_run']]
    need(len(observed) == len(set(observed)) and len(omitted) == len(set(omitted)) and
         not set(observed) & set(omitted) and set(observed + omitted) <= expected, 'bench_unites')
    position, justified = 0, set()
    for case in sorted(manifest['cases'], key=lambda c: (c['name'].startswith('lidar'), c['count'])):
        for k in (5, 10):
            failed_k = False
            for r in range(3):
                if position == len(observed):
                    need(not report['complete'], 'bench_arret_injustifie')
                    break
                row = report['runs'][position]
                need(unit(row) == (case['name'], k, r), 'bench_ordre_tentatives')
                position += 1
                failed_k = row['status'] != 'ok'
                if failed_k or row['process_wall_seconds'] > 10:
                    justified.update((case['name'], k, n) for n in range(r + 1, 3))
                    if failed_k and k == 5:
                        justified.update((case['name'], 10, n) for n in range(3))
                    break
            if failed_k:
                break
    need(position == len(observed) and set(omitted) <= justified and
         (not report['complete'] or set(omitted) == justified), 'bench_omissions_injustifiees')
    hashes = {}
    for row in report['runs']:
        name, k, r = unit(row)
        argv = row['argv']
        need(row['whole_input'] is True and row['count'] == cases[name]['count'] and len(argv) == 10 and
             Path(argv[0]).name == 'mhgp11_catalogue_bench' and Path(argv[1]).name == cases[name]['coordinates'] and
             Path(argv[2]).name == cases[name]['point_ids'] and
             argv[4:] == [str(k), str(leaf), '256', '0', str(2**32 - 1), str(8 * 1024**3)], 'bench_commande')
        need(isinstance(row['process_wall_seconds'], (int, float)) and
             math.isfinite(row['process_wall_seconds']) and row['process_wall_seconds'] >= 0, 'bench_temps')
        events = attempt_events(row, v2)
        need(events == row['events'], 'bench_evenements')
        status = row['status']
        if status in ('ok', 'nondeterministic'):
            need(row['exit_code'] == 0 and [e['phase'] for e in events] == ['cloud', 'catalogue', 'exit'] and
                 all(e.get('status', 'ok') == 'ok' for e in events), 'bench_faux_succes')
            cloud, cat, _ = events
            need(cloud['points'] == cloud['sites'] == cases[name]['count'] and cat['coord_bits'] == 18 and
                 cat['kmax'] == k and cat['balls'] > 0 and cat['generation_passes'] == 2, 'bench_calcul')
            need(all(type(obj[key]) is int and obj[key] >= 0 for obj, keys in
                     [(cloud, ('read_ns', 'cloud_ns', 'cloud_peak_bytes')),
                      (cat, ('wall_ns', 'peak_reserved_bytes', 'reserved_after_bytes'))] for key in keys) and
                 cat['reserved_after_bytes'] <= cat['peak_reserved_bytes'] <= 8 * 1024**3 and
                 cloud['cloud_peak_bytes'] <= 8 * 1024**3, 'bench_metriques')
            need(row['catalogue_ms'] == cat['wall_ns'] / 1e6 and row['read_ms'] == cloud['read_ns'] / 1e6 and
                 row['cloud_ms'] == cloud['cloud_ns'] / 1e6 and
                 row['catalogue_within_100ms'] is (cat['wall_ns'] < 100000000), 'bench_chronos')
            need(HEX.fullmatch(row['canonical_sha256']) and row['canonical_bytes'] > 10, 'bench_empreinte')
            group = hashes.setdefault((name, k), set())
            group.add(row['canonical_sha256'])
            need((status == 'nondeterministic') is (len(group) != 1), 'bench_repetabilite')
        else:
            if v2:
                need(status in ('refused', 'failed', 'timeout', 'launch_error', 'invalid_output', 'artifact_error'),
                     'bench_echec_v2_statut')
                need(status == 'artifact_error' or 'canonical_sha256' not in row, 'bench_hash_apres_echec')
                if 'canonical_sha256' in row:
                    need(HEX.fullmatch(row['canonical_sha256']), 'bench_hash_partiel')
                if 'canonical_bytes' in row:
                    need(type(row['canonical_bytes']) is int and row['canonical_bytes'] >= 0, 'bench_taille_partielle')
            else:
                need(status in ('refused', 'failed', 'timeout') and 'canonical_sha256' not in row and
                     (row['exit_code'] is None if status == 'timeout' else row['exit_code'] != 0), 'bench_echec')
                if status == 'refused':
                    need(row['exit_code'] == 2, 'bench_refus_code')
    if report['complete']:
        need(set(observed + omitted) == expected, 'bench_unites_manquantes')
        need(report['all_attempted_ok'] is all(r['status'] == 'ok' for r in report['runs']) and
             report['full_schedule_completed'] is (len(observed) == 36 and report['all_attempted_ok']), 'bench_verdict')
    return dict(attempted=len(observed), ok=sum(r['status'] == 'ok' for r in report['runs']), omitted=len(omitted),
                complete=report['complete'], schedule_ok=report.get('full_schedule_completed', False))


def check(folder):
    receipt, worker, data = read_capture(folder)
    summary, counts, builds, matrix_code = matrix(data, folder)
    manifest, manifest_hash = inputs(folder, receipt)
    metas = [fields(data[p + 'meta.txt']) for p in ('results/cmd/000_matrice/', BENCH)]
    need(int(metas[0]['exit_code']) == matrix_code and metas[0]['group_closed'] == '1', 'commande_matrice')
    bench_meta = metas[1]
    if BENCH + 'files/catalogue.json' in data:
        source = data[BENCH + 'files/catalogue.json']
        need(source == (folder / 'catalogue.json').read_bytes(), 'copie_bench')
        report = js(source)
        bench = benchmark(data, folder, report, manifest, manifest_hash, builds, matrix_code == 0)
        if report.get('leaf_size') == 16:
            successful = [r for r in report['runs'] if r['case'] == 'uniform_u18_n8000' and
                          r['kmax'] == 5 and r['status'] == 'ok']
            if successful:
                _, _, reference_data = read_capture(HERE / 'catalogue2')
                reference = js(reference_data[BENCH + 'files/catalogue.json'])
                hashes = {r['canonical_sha256'] for r in reference['runs'] if r['case'] == 'uniform_u18_n8000' and
                          r['kmax'] == 5 and r['status'] == 'ok'}
                need(len(hashes) == 1 and all(r['canonical_sha256'] in hashes for r in successful),
                     'ablation_feuille_hash_different')
        need((bench_meta['status'] == 'ok') is bench['schedule_ok'] and
             (int(bench_meta['exit_code']) == 0) is bench['schedule_ok'], 'commande_bench_verdict')
        if report['complete']:
            need(int(bench_meta['exit_code']) == (0 if bench['schedule_ok'] else 1), 'commande_bench_code')
    else:
        need(bench_meta['status'] != 'ok' and not (folder / 'catalogue.json').exists(), 'bench_absent_faussement_ok')
        bench = dict(attempted=0, ok=0, omitted=36, complete=False, schedule_ok=False)
    command_ok = sum(m['status'] == 'ok' for m in metas)
    need(worker['commands_total'] == '2' and int(worker['commands_ok']) == command_ok and
         worker['status'] == ('completed' if command_ok == 2 else 'failed'), 'worker_verdict')
    need(receipt['status'] == ('completed' if command_ok == 2 else 'failed_remote') and
         receipt['worker_exit_code'] == (0 if command_ok == 2 else 1), 'session_verdict')
    need(metas[0]['status'] == ('ok' if matrix_code == 0 else 'failed'), 'matrice_statut_worker')
    if 'group_closed' in bench_meta:
        need(bench_meta['group_closed'] == '1', 'fermeture_commande_bench')
    print('%s coherence=ok campagne=%s commit=%s' % (folder.name, 'CONFORME' if command_ok == 2 else 'ECHEC', receipt['commit']))
    print('  catalogue essais=%d ok=%d non_joues=%d calendrier_complet=%s' %
          (bench['attempted'], bench['ok'], bench['omitted'], bench['schedule_ok']))
    for name in sorted(counts):
        print('  %s %s selection/passes/echecs/non_joues=%s' %
              (name, summary['statuses'][name], '/'.join(map(str, counts[name]))))
    return command_ok != 2


def main():
    folders = [Path(p) for p in sys.argv[1:]] or sorted(p for p in HERE.glob('catalogue[0-9]*') if p.is_dir())
    need(folders, 'aucune_capture')
    failed = sum(check(folder) for folder in folders)
    print('captures_coherentes=%d campagnes_echouees=%d' % (len(folders), failed))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (foundation.Refusal, OSError, ValueError, KeyError, TypeError, IndexError,
            foundation.ET.ParseError, tarfile.TarError) as error:
        print('REFUS ' + (str(error) if isinstance(error, foundation.Refusal) else type(error).__name__))
        sys.exit(1)
