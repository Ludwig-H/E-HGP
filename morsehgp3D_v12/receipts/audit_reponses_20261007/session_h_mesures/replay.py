#!/usr/bin/env python3
"""Relecture locale des preuves epinglees de H ; aucun moteur, build, reseau ou cloud."""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import statistics
import subprocess

PIN = '136e0762898ce8dd40c96dafba6b2d616f3b7b1f'
BASE = 'morsehgp3D_v12/receipts/g4_t2h_20261007/'


def need(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def uint(value):
    return type(value) is int and 0 <= value < 1 << 64


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'cle JSON repetee')
        result[key] = value
    return result


def proof(repo):
    def git(path, pin=PIN):
        return subprocess.check_output(['git', '-C', str(repo), 'show', pin + ':' + path])
    receipt_data = git(BASE + 'receipt.json')
    receipt = json.loads(receipt_data)
    hashed = git(BASE + 'SHA256SUMS').decode().splitlines()
    for row in hashed:
        expected, path = row.split('  ', 1)
        need(sha(git(BASE + path)) == expected, 'hash recu : ' + path)
    source_pin = receipt['source']['head_commit']
    sources = {item['path']: item for item in receipt['source']['manifest']}
    selected = [p for p in sources if p.startswith(('morsehgp3D_v12/src/', 'morsehgp3D_v12/bench/')) or
                p == 'morsehgp3D_v12/CMakeLists.txt']
    for path in selected:
        data = git(path, source_pin)
        need(sha(data) == sources[path]['sha256'] and len(data) == sources[path]['size'], 'source snapshot : ' + path)
    anchor_paths = ['morsehgp3D_v12/bench/tower_probe.cpp', 'morsehgp3D_v12/bench/tower_export.hpp',
                    'morsehgp3D_v12/src/tower/stage.cpp', 'morsehgp3D_v12/tests/tower/g_determinism.py',
                    'morsehgp3D_v12/tests/tower/tests.cmake']
    anchors = {}
    for path in anchor_paths:
        data = git(path, source_pin)
        need(sha(data) == sources[path]['sha256'], 'source outil : ' + path)
        anchors[path] = dict(sha256=sha(data), bytes=len(data))
    cmake = git(anchor_paths[-1], source_pin).decode()
    reference_prefixes = {int(n): digest for n, digest in re.findall(
        r'set\(tower_scale_(\d+) "empreinte=([0-9a-f]{16})', cmake)}
    ng00_reference = re.search(r'cas=lidar_ng00_k5 fils=1,8 empreinte=([0-9a-f]{16})', cmake).group(1)
    commands = list(csv.DictReader(io.StringIO(git(BASE + 'resultats/commands.tsv').decode()), delimiter='\t'))
    need(commands == receipt['commands'], 'commands.tsv different du recu')
    runs = []
    for command in commands:
        name = command['name']
        match = re.fullmatch(r'tour_(ng\d+|u\d+)_k(5|10)_w(1|48)', name)
        if not match:
            continue
        case, kmax, threads = match[1], int(match[2]), int(match[3])
        count = 2 if threads == 1 else 3 if kmax == 10 else 10 if case.startswith('ng') else 5
        stem = BASE + 'resultats/cmd/%03d_%s/' % (int(command['index']), name)
        meta = dict(line.split('=', 1) for line in git(stem + 'meta.txt').decode().splitlines())
        need(meta['status'] == 'ok' and meta['exit_code'] == '0' and meta['group_closed'] == '1' and
             meta['streams_truncated'] == '0', 'commande non conforme : ' + name)
        rows = [json.loads(line, object_pairs_hook=unique) for line in git(stem + 'stdout').decode().splitlines()]
        need(all(type(row) is dict for row in rows), 'ligne non objet')
        need(len(rows) == count + kmax + 2, 'compte de lignes : ' + name)
        stages, orders, digest, end = rows[:count], rows[count:-2], rows[-2], rows[-1]
        for number, row in enumerate(stages):
            need(row.get('phase') == 'tour_g' and row.get('status') == 'ok' and row.get('reason') == 'none', 'phase/statut')
            need(all(uint(row.get(k)) for k in ('pass', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns', 'order')), 'entier stage')
            need((row['pass'], row['coord_bits'], row['kmax'], row['threads'], row['order']) ==
                 (number, 21, kmax, threads, 0), 'configuration')
            diag = row.get('diagnostics')
            need(type(diag) is dict and set(diag) == set(('count_ns','fill_ns','tables_ns','resolve_ns',
                 'workspace_bytes','table_bytes','peak_bytes','order_ns')), 'diagnostics')
            need(all(uint(v) for k, v in diag.items() if k != 'order_ns') and type(diag['order_ns']) is list and
                 len(diag['order_ns']) == kmax and all(uint(v) for v in diag['order_ns']), 'entiers diagnostics')
        sites = stages[0]['sites']
        need(1 <= sites <= 0xFFFFFFFF and all(s['sites'] == sites for s in stages), 'sites')
        if case.startswith('ng'):
            data = {item['name']: item for item in receipt['data_files']}
            need(data['lidar_' + case + '.u32le']['size'] == sites * 12 and
                 data['lidar_' + case + '.ids.u32le']['size'] == sites * 4, 'taille entree')
        else:
            need(sites == int(case[1:]), 'taille uniforme')
        for k, row in enumerate(orders, 1):
            need(row.get('phase') == 'ordre' and uint(row.get('k')) and row['k'] == k, 'ordres')
            need(type(row.get('objet')) is dict and type(row.get('travail')) is dict and
                 all(uint(v) for v in row['objet'].values()) and all(uint(v) for key, v in row['travail'].items()
                 if key != 'chaines'), 'compteurs')
            chains = row['travail'].get('chaines')
            need(type(chains) is list and len(chains) == 16 and all(uint(v) for v in chains), 'histogramme')
        need(orders[0]['objet']['births'] == sites, 'naissances K1')
        need(set(digest) == {'phase', 'resolution_sha256'} and digest['phase'] == 'digest' and
             type(digest['resolution_sha256']) is str and re.fullmatch('[0-9a-f]{64}', digest['resolution_sha256']), 'digest')
        need(end == dict(phase='exit', status='ok', reason='none', order=0) and type(end['order']) is int, 'exit')
        warm = stages[1:]
        median = lambda values: statistics.median(list(values))
        scalar = lambda key: [s['diagnostics'][key] for s in warm]
        residual = [s['wall_ns'] - sum(s['diagnostics'][k] for k in ('tables_ns','resolve_ns','count_ns','fill_ns')) for s in warm]
        without_tables = [s['wall_ns'] - s['diagnostics']['tables_ns'] for s in warm]
        # Bornes des chronos disjointes dans stage.cpp 2682f239 seulement, PAS dans le prototype G-c.
        projection = [s['wall_ns'] - s['diagnostics']['tables_ns'] - s['diagnostics']['resolve_ns'] / 2 + 3000000 for s in warm]
        need(min(residual) >= 0, 'residu negatif : bornes chevauchantes ?')
        reference = ng00_reference if case == 'ng00' and kmax == 5 else reference_prefixes.get(sites) if case[0] == 'u' else None
        if reference:
            need(digest['resolution_sha256'].startswith(reference), 'prefixe different de la reference CTest')
        runs.append(dict(name=name, case=case, sites=sites, kmax=kmax, threads=threads, coord_bits=21,
            processes=1, passes=count, warm_count=len(warm), first_wall_ns=stages[0]['wall_ns'],
            wall_median_ns=median(s['wall_ns'] for s in warm), wall_min_ns=min(s['wall_ns'] for s in warm),
            wall_max_ns=max(s['wall_ns'] for s in warm), warm_wall_ns=[s['wall_ns'] for s in warm],
            phase_medians_ns={k:median(scalar(k)) for k in ('count_ns','fill_ns','tables_ns','resolve_ns')},
            order_medians_ns=[median(s['diagnostics']['order_ns'][i] for s in warm) for i in range(kmax)],
            budget_peak_bytes=max(s['diagnostics']['peak_bytes'] for s in stages),
            process_max_rss_kb=int(meta['max_rss_kb']), process_wall_seconds=float(meta['wall_seconds']),
            digest64=digest['resolution_sha256'], known_CTest_prefix16=reference,
            derived=dict(r_median_ns=median(residual),r_max_ns=max(residual),
                wall_without_tables_median_ns=median(without_tables),
                projection_tables3ms_resolve_half_median_ns=median(projection),
                projection_tables3ms_resolve_half_max_ns=max(projection))))
    need(len(runs) == 10, 'dix prises G attendues')
    lookup = {r['name']:r for r in runs}
    mono, many = lookup['tour_ng00_k5_w1'], lookup['tour_ng00_k5_w48']
    need(mono['digest64'] == many['digest64'], 'digest ng00 mono/48 different')
    speedup = dict(stage=mono['wall_median_ns']/many['wall_median_ns'],
                   tables=mono['phase_medians_ns']['tables_ns']/many['phase_medians_ns']['tables_ns'],
                   resolve=mono['phase_medians_ns']['resolve_ns']/many['phase_medians_ns']['resolve_ns'])
    observed = receipt['observed_after']
    need(receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'] is True and
         observed['status'] == 'TERMINATED' and observed['lastStartTimestamp'] == receipt['generation'] and
         receipt['stop_exit_code'] == 0 and receipt['stop_attempts'] == 1, 'arret archive non conforme')
    need(receipt['results_verified'] is True and receipt['data_verified_remote'] is True and
         not receipt['errors'] and not receipt['overflow']['truncated_streams'], 'reception non conforme')
    need(receipt['remote_summary']['failed_commands'] == [['mes_p_fils_1_4','deadline_cut','124']], 'echec autre que MES-P')
    gates = {}
    for index, name, expected in [(0,'socle_ctest',637),(11,'lidar_ctest',6)]:
        text = git(BASE + 'resultats/cmd/%03d_%s/stdout' % (index,name)).decode()
        need('100%% tests passed, 0 tests failed out of %d' % expected in text, 'compte portes')
        gates[name] = expected
    return dict(receipt_pin=PIN, receipt_sha256=sha(receipt_data), source_pin=source_pin,
        product_bench_cmake_entries_verified=len(selected), source_anchors=anchors,
        public_SHA256SUMS_entries_verified=len(hashed),
        binary_sha256=receipt['provenance']['binaries_sha256']['mhgp12_tower_probe'],
        package_sha256=receipt['package_sha256'], data_manifest_sha256=receipt['data_manifest_sha256'],
        build=dict(kind='Release',cxx='GNU 11.4.0',backend='cpu_reference',coord_bits=21,workers_available=48),
        runs=runs, ng00_k5_single_process_speedup=speedup, historical_gates=gates,
        stop_archive=dict(closure=receipt['closure'],status=receipt['status'],worker_exit_code=receipt['worker_exit_code'],
                          observed_status=observed['status'],generation=observed['lastStartTimestamp'],
                          last_stop=observed['lastStopTimestamp'],targeted_shutdown_certified=True),
        own_cloud_calls=0, own_native_runs=0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path.cwd())
    parser.add_argument('--out',type=Path)
    args=parser.parse_args()
    encoded=json.dumps(proof(args.repo),indent=1,sort_keys=True)+'\n'
    if args.out:args.out.write_text(encoded)
    else:print(encoded,end='')


if __name__=='__main__':
    main()
