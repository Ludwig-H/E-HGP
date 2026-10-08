#!/usr/bin/env python3
"""Admission des journaux D6 M figés ; sources Git et provenance extérieure distinctes."""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pairs(items):
    result = {}
    for key, value in items:
        need(key not in result, 'clé JSON répétée')
        result[key] = value
    return result


def constant(_value):
    raise ValueError('constante JSON non finie')


def decode(data):
    return json.loads(data, object_pairs_hook=pairs, parse_constant=constant)


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


def review(repo, folder, capture):
    bodies = {}
    for path, digest in capture['files'].items():
        body = subprocess.check_output(['git', '-C', str(repo), 'show', capture['source_commit'] + ':' + path])
        need(sha(body) == digest, 'source différente : ' + path)
        bodies[Path(path).name] = body
    pilot = {'__name__': 'audit_d6_import_only'}
    exec(compile(bodies['pilote_d6.py'], 'pilote_d6_pinned', 'exec'), pilot)
    report_raw = (folder / 'mes_d6.json').read_bytes()
    need(sha(report_raw) == capture['report_sha256'], 'rapport hors capture')
    report = decode(report_raw)
    need(type(report) is dict and set(report) == {'mesure', 'prises', 'rapports', 'controles'}
         and report['mesure'] == 'MES-D6', 'rapport absent ou partiel')
    combos = capture['combos']
    expected_order = [(case, bits, factor, turn) for case in sorted(capture['sites'])
                      for turn in range(capture['tours']) for bits, factor in combos[turn:] + combos[:turn]]
    entries = report['prises']
    need(type(entries) is list and all(type(e) is dict for e in entries), 'prises mal formées')
    need([(e['cas'], e['profil'], e['facteur'], e['tour']) for e in entries] == expected_order, 'cohorte ou ordre')
    observed, rebuilt, samples, per_stage = {}, [], {}, {'catalogue': 0, 'tour_g': 0}
    for entry in entries:
        item = {k: entry[k] for k in ('cas', 'profil', 'facteur', 'tour')}
        need(all(type(entry[k]) is int for k in ('profil', 'facteur', 'tour')), 'types de configuration')
        for phase, suffix, digest_key in [('catalogue', 'catalogue', 'catalogue_sha256'),
                                           ('tour_g', 'tour', 'resolution_sha256')]:
            name = '{cas}_p{profil}_x{facteur}_t{tour}_'.format(**item) + suffix + '.jsonl'
            raw = (folder / 'brut' / name).read_bytes()
            observed[name] = sha(raw)
            rows = [decode(line) for line in raw.splitlines()]
            code = entry[phase]['code']
            need(type(code) is int and code == 0, 'code de sonde déclaré non conforme')
            expected = dict(passes=capture['passes'], coord_bits=entry['profil'], kmax=capture['kmax'],
                            threads=capture['threads'], leaf=24)
            exported = phase == 'catalogue' and entry['facteur'] == 1 and entry['tour'] == 0
            summary = pilot['summarize'](code, rows, phase, digest_key, expected, exported)
            need(summary['ok'] and summary['sites'] == capture['sites'][entry['cas']], 'sortie non conforme')
            stages = [r for r in rows if r.get('phase') == phase]
            need(len(stages) == capture['passes'], 'nombre de passes')
            warm_ns = [r['wall_ns'] for r in stages[1:]]
            need(statistics.median(warm_ns) / 1e6 == summary['chaud_ms'], 'médiane de prise')
            if phase == 'catalogue':
                body_hash = entry[phase]['corps_sha256']
                need(pilot['hex_sha'](body_hash) if exported else body_hash is None, 'hash corps déclaré')
                summary['corps_sha256'] = body_hash
                need(all(r['ledger']['emitted'] == r['balls'] and r['ledger']['incidences'] == r['incidences']
                         for r in stages), 'ledger C incohérent')
                key = (entry['cas'], entry['profil'], entry['facteur'])
                samples.setdefault(key, set()).update(tuple(r['diagnostics'][k] for k in
                    ('leaves_narrow', 'leaves_medium', 'leaves_wide', 'leaves_exact')) +
                    (r['ledger']['region_line_fallbacks'],) for r in stages)
            else:
                for r in stages:
                    d = r['diagnostics']
                    need(d['tables_ns'] + d['resolve_ns'] <= r['wall_ns'], 'sous-horloges G')
                    need(d['tables_ns'] == sum(d['table_ns']) and d['resolve_ns'] == sum(d['pass_ns']) and
                         d['orders_ns'] == sum(d['order_ns']), 'diagnostics Gc incohérents')
            need(canonical(summary) == canonical(entry[phase]), 'résumé différent du brut')
            item[phase] = summary
            per_stage[phase] += 1
        need(item['catalogue']['sites'] == item['tour_g']['sites'], 'sites C/G')
        rebuilt.append(item)
    need(set(observed) == {x.name for x in (folder / 'brut').iterdir() if x.is_file()}, 'inventaire brut différent')
    need(sha(canonical(observed).encode()) == capture['journal_inventory_sha256'], 'inventaire hors capture')
    need(pilot['checks'](rebuilt) == report['controles'] == [], 'contrôles D6')
    ratios = []
    for case in sorted(capture['sites']):
        for bits, factor in combos:
            row = dict(cas=case, profil=bits, facteur=factor)
            matched = [e for e in rebuilt if (e['cas'], e['profil'], e['facteur']) == (case, bits, factor)]
            need(len(matched) == capture['tours'], 'cohorte de cellule')
            for phase in ('catalogue', 'tour_g'):
                times = [e[phase]['chaud_ms'] for e in matched]
                values = [e[phase]['chaud_ms'] / next(b for b in rebuilt if b['cas'] == case and
                    b['profil'] == 21 and b['facteur'] == 1 and b['tour'] == e['tour'])[phase]['chaud_ms']
                    for e in matched]
                row[phase] = dict(chaud_ms=statistics.median(times), rapport=statistics.median(values),
                                  rapport_min=min(values), rapport_max=max(values))
            ratios.append(row)
    need(canonical(ratios) == canonical(report['rapports']) == canonical(pilot['ratios'](rebuilt)), 'rapports')
    shapes = {}
    for case in sorted(capture['sites']):
        mine = [e for e in rebuilt if e['cas'] == case]
        shapes[case] = dict(balls=mine[0]['catalogue']['boules'], incidences=mine[0]['catalogue']['incidences'],
                            levels=mine[0]['catalogue']['niveaux'],
                            corps_x1_declares=sorted({e['catalogue']['corps_sha256'] for e in mine
                                                      if e['facteur'] == 1 and e['tour'] == 0}),
                            resolution_x1=sorted({e['tour_g']['empreinte'] for e in mine if e['facteur'] == 1}))
    need(all(len(x) == 1 for x in samples.values()), 'classes de feuilles variables')
    for name, digest in observed.items():
        need(sha((folder / 'brut' / name).read_bytes()) == digest, 'journal changé durant lecture')
    need((folder / 'mes_d6.json').read_bytes() == report_raw, 'rapport changé durant lecture')
    return dict(source_commit=capture['source_commit'], report_sha256=sha(report_raw),
                journal_inventory_sha256=sha(canonical(observed).encode()),
                processes=len(observed), passes=len(observed) * capture['passes'],
                warm_passes=len(observed) * (capture['passes'] - 1), per_stage=per_stage,
                shapes=shapes, ratios=ratios, native_executed=False,
                physical_samples=[dict(cas=k[0], profil=k[1], facteur=k[2], **dict(zip(
                    ('leaves_narrow', 'leaves_medium', 'leaves_wide', 'leaves_exact', 'region_line_fallbacks'),
                    next(iter(v))))) for k,v in sorted(samples.items())],
                limits='codes déclarés par le pilote ; hashes de corps repris, exports supprimés ; C/G CPU séparés, pas FULL')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--repo', type=Path, required=True)
    p.add_argument('--folder', type=Path, required=True)
    args = p.parse_args()
    print(json.dumps(review(args.repo, args.folder, decode((HERE / 'capture.json').read_bytes())),
                     indent=2, sort_keys=True, ensure_ascii=False))
