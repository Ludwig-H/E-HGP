#!/usr/bin/env python3
"""Relecture de metadonnees/JSON D6 K. Aucun moteur, build ni payload geometrique."""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import tarfile


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def unique(pairs):
    out = {}
    for k, v in pairs:
        need(k not in out, 'cle JSON repetee')
        out[k] = v
    return out


def bad_constant(_):
    raise ValueError('constante JSON non finie')


def decode(data):
    return json.loads(data.decode('utf-8'), object_pairs_hook=unique, parse_constant=bad_constant)


def normalized(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    here = Path(__file__).parent
    capture = decode((here/'capture.json').read_bytes())
    before = args.archive.read_bytes()
    need(sha(before) == capture['archive_sha256'], 'archive differente')
    modules = []
    for pin in capture['readers']:
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', pin['commit']+':'+pin['path']])
        need(sha(body) == pin['sha256'], 'lecteur different')
        module = {'__name__': 'audit_import_only'}
        exec(compile(body, pin['commit'], 'exec'), module)
        modules.append(module)
    old, new = modules
    for pin in capture['bench_sources']:
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', pin['commit']+':'+pin['path']])
        need(sha(body) == pin['sha256'], 'source de sonde differente')
    base = 'results/cmd/001_mes_d6_profils/'
    files = {}
    with tarfile.open(args.archive, 'r:gz') as tar:
        seen = set()
        for item in tar:
            need(item.name not in seen, 'membre tar repete')
            seen.add(item.name)
            if item.name.startswith(base) and item.isfile():
                relative = item.name[len(base):]
                allowed = relative in ('meta.txt', 'argv.txt', 'stdout', 'stderr', 'time.txt',
                                       'files/d6/mes_d6.json', 'files/d6/mes_d6.md') or \
                          relative.startswith('files/d6/brut/') and relative.endswith('.jsonl')
                need(allowed and item.size < 100000, 'membre D6 inattendu')
                files[relative] = tar.extractfile(item).read()
    need(sha(files['files/d6/mes_d6.json']) == capture['report_sha256'], 'rapport different')
    meta = dict(line.split('=', 1) for line in files['meta.txt'].decode().splitlines())
    need(meta['exit_code'] == '1' and meta['group_closed'] == '1' and meta['streams_truncated'] == '0'
         and meta['residual_group_killed'] == '0', 'statut externe D6')
    need(decode(files['stdout']) == dict(ecarts=66, prises=63), 'stdout D6')
    report = decode(files['files/d6/mes_d6.json'])
    need(set(report) == {'controles', 'mesure', 'prises', 'rapports'} and report['mesure'] == 'MES-D6', 'schema rapport')
    combos = [(21, 1), (21, 8), (24, 1), (24, 8), (32, 1), (32, 8), (32, 2048)]
    expected_order = [(case, bits, factor, turn) for case in ('ng00', 'ng01', 'ng02') for turn in range(3)
                      for bits, factor in combos[turn:]+combos[:turn]]
    entries = report['prises']
    need([(e['cas'], e['profil'], e['facteur'], e['tour']) for e in entries] == expected_order, 'cohorte/order prises')
    rebuilt, expected_files, summaries_equal = [], set(), 0
    admission = dict(old_catalogue=0, old_tour_g=0, new_catalogue=0, new_tour_g=0)
    physical = {}
    for entry in entries:
        item = {key: entry[key] for key in ('cas', 'profil', 'facteur', 'tour')}
        for phase, tail, digest in (('catalogue', 'catalogue', 'catalogue_sha256'),
                                    ('tour_g', 'tour', 'resolution_sha256')):
            name = f"files/d6/brut/{entry['cas']}_p{entry['profil']}_x{entry['facteur']}_t{entry['tour']}_{tail}.jsonl"
            expected_files.add(name)
            lines = [decode(line) for line in files[name].splitlines()]
            expected = dict(passes=5, coord_bits=entry['profil'], kmax=5, threads=48, leaf=24)
            exported = phase == 'catalogue' and entry['facteur'] == 1 and entry['tour'] == 0
            code = entry[phase]['code']
            need(type(code) is int and code == 0, 'code de sonde declare')
            previous = old['summarize'](code, lines, phase, digest, expected, exported)
            current = new['summarize'](code, lines, phase, digest, expected, exported)
            admission['old_'+phase] += int(previous['ok'])
            admission['new_'+phase] += int(current['ok'])
            if phase == 'catalogue':
                body = entry[phase]['corps_sha256']
                need((new['hex_sha'](body) if exported else body is None), 'hash corps declare')
                previous['corps_sha256'] = current['corps_sha256'] = body
                census = [line for line in lines if line['phase'] == 'catalogue']
                need(all(row['ledger']['emitted'] == row['balls'] and row['ledger']['incidences'] == row['incidences']
                         for row in census), 'liaison compteur catalogue')
                physical.setdefault((entry['cas'], entry['profil'], entry['facteur']), set()).update(
                    tuple(row['diagnostics'][key] for key in ('leaves_narrow', 'leaves_medium', 'leaves_wide', 'leaves_exact'))
                    + (row['ledger']['region_line_fallbacks'],) for row in census)
            need(normalized(previous) == normalized(entry[phase]), 'ancien resume non reproduit')
            summaries_equal += 1
            need(current['ok'] and current['sites'] == capture['sites'][entry['cas']], 'relecture corrigee/site')
            stages = [line for line in lines if line.get('phase') == phase]
            need(len(stages) == 5, 'passes')
            independent_ms = statistics.median(row['wall_ns'] for row in stages[1:]) / 1e6
            need(independent_ms == current['chaud_ms'], 'mediane independante')
            if phase == 'tour_g':
                for row in stages:
                    diag = row['diagnostics']
                    need(diag['tables_ns'] + diag['resolve_ns'] <= row['wall_ns'], 'G sous-chronos')
                    need(diag['tables_ns'] == sum(diag['table_ns']) and
                         diag['resolve_ns'] == sum(diag['pass_ns']) and
                         diag['orders_ns'] == sum(diag['order_ns']), 'somme diagnostics Gc')
            item[phase] = current
        need(item['catalogue']['sites'] == item['tour_g']['sites'], 'sites C/G')
        rebuilt.append(item)
    need({n for n in files if n.startswith('files/d6/brut/')} == expected_files, 'couverture des bruts')
    need(old['checks'](entries) == report['controles'], '66 ecarts non reproduits')
    need(normalized(old['ratios'](entries)) == normalized(report['rapports']), 'rapports originaux non reproduits')
    need(new['checks'](rebuilt) == [], 'controle nouveau non conforme')
    ratios = new['ratios'](rebuilt)
    for row in ratios:
        for phase in ('catalogue', 'tour_g'):
            matched = [e for e in rebuilt if (e['cas'], e['profil'], e['facteur']) ==
                       (row['cas'], row['profil'], row['facteur'])]
            values = [e[phase]['chaud_ms'] / next(b for b in rebuilt if b['cas'] == e['cas'] and
                      b['tour'] == e['tour'] and b['profil'] == 21 and b['facteur'] == 1)[phase]['chaud_ms'] for e in matched]
            need(len(values) == 3 and row[phase] == dict(chaud_ms=statistics.median(e[phase]['chaud_ms'] for e in matched),
                 rapport=statistics.median(values), rapport_min=min(values), rapport_max=max(values)), 'rapports recalcules')
    shape = {}
    for case in ('ng00', 'ng01', 'ng02'):
        mine = [e for e in rebuilt if e['cas'] == case]
        shape[case] = dict(catalogue_balls=mine[0]['catalogue']['boules'], incidences=mine[0]['catalogue']['incidences'],
                          levels=mine[0]['catalogue']['niveaux'],
                          corps_x1_declares=sorted({e['catalogue']['corps_sha256'] for e in mine if e['facteur'] == 1 and e['tour'] == 0}),
                          resolution_x1=sorted({e['tour_g']['empreinte'] for e in mine if e['facteur'] == 1}))
    result = dict(external_command_code=1, source_sonde_codes='126 zeros declares par le rapport',
                  processes=126, passes=630, warm_passes=504, prises=63, admission=admission,
                  old_summaries_equal=summaries_equal, original_alerts=66, corrected_alerts=0,
                  original_body_alerts_cascade=3, body_hashes_independently_recomputed=False,
                  native_executed=False, shapes=shape, ratios=ratios,
                  raw_tree_sha256=sha(b''.join(n.encode()+b'\0'+sha(files[n]).encode()+b'\n' for n in sorted(expected_files))),
                  narrow_broad_profiles_only=True)
    need(all(len(values) == 1 for values in physical.values()), 'diagnostics feuilles variables entre passes/tours')
    result['physical_samples'] = [dict(cas=key[0], profil=key[1], facteur=key[2],
                                     **dict(zip(('leaves_narrow', 'leaves_medium', 'leaves_wide', 'leaves_exact',
                                                 'region_line_fallbacks'), next(iter(values)))))
                                  for key, values in sorted(physical.items()) if (key[1], key[2]) in ((21,1), (32,1), (32,2048))]
    need(args.archive.read_bytes() == before, 'archive modifiee pendant lecture')
    if args.check:
        need(normalized(result) == normalized(capture['result']), 'resultat different de capture')
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
