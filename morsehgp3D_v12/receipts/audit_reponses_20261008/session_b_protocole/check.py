#!/usr/bin/env python3
"""Provenance de préparation B et scénario comptable A, sans moteur ni payload."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import statistics as st
import subprocess
import sys
import tarfile
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--session', type=Path, required=True)
    ap.add_argument('--before-sources', type=Path, required=True)
    ap.add_argument('--returned-a', type=Path, required=True)
    a = ap.parse_args()
    c = json.loads((HERE / 'capture.json').read_text())
    helper = a.repo / c['helper_path']
    need(sha(helper.read_bytes()) == c['helper_sha256'], 'lecteur de sources différent')
    plan_raw = (a.session / 'package/plan.json').read_bytes()
    need(sha(plan_raw) == c['plan_sha256'], 'plan différent')
    plan = json.loads(plan_raw)
    need([{k: x[k] for k in ('name', 'timeout_seconds')} for x in plan['commands']] == c['commands'], 'commandes')
    args = plan['commands'][1]['argv']
    for key, value in c['selected_args'].items():
        need(args[args.index(key) + 1] == value, 'argument public différent')
    need(Path(args[args.index('--avant-archive') + 1]).name == c['before_name'], 'archive avant')
    cmd = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    with tempfile.TemporaryDirectory(prefix='audit-b-protocole-') as tmp:
        for arm, archive in [('avant', a.before_sources), ('paquet', a.session / 'package/package.tar.gz')]:
            path = Path(tmp) / (arm + '.json')
            path.write_text(json.dumps(c['source_archives'][arm]))
            subprocess.run(cmd + [str(helper), '--repo', str(a.repo), '--package', str(archive),
                                  '--plan', str(a.session / 'package/plan.json'), '--capture', str(path)],
                           check=True, stdout=subprocess.PIPE)

    def blob(pin, rel):
        return subprocess.check_output(['git', '-C', str(a.repo), 'show', pin + ':' + rel])

    pin = c['source_archives']['paquet']['source_git']
    with tarfile.open(a.session / 'package/package.tar.gz') as archive:
        for name, expected in c['pilot_sources'].items():
            raw = archive.extractfile(name).read()
            need(len(raw) == expected['bytes'] and sha(raw) == expected['sha256'] and raw == blob(pin, name),
                 'source du pilote différente')
    manifest = json.loads(blob(pin, 'morsehgp3D_v12/microbancs/mes_t2d_b/bras_t2d_b.json'))
    proposed = c['future_baseline_proposal']
    entries, delivered = 0, 0
    for arm, description in manifest['bras'].items():
        for rel, entry in description['fichiers'].items():
            name = 'morsehgp3D_v12/' + rel
            raw = blob(proposed['git'], name)
            need(sha(raw) == entry['sha256_avant'], 'préimage future base')
            text = raw.decode()
            for change in entry['substitutions']:
                need(text.count(change['cherche']) == 1, 'motif ambigu')
                text = text.replace(change['cherche'], change['remplace'])
            need(sha(text.encode()) == entry['sha256_apres'], 'postimage future base')
            entries += 1
            if arm == 'apres':
                need(blob(pin, name) == text.encode(), 'produit livré différent')
                delivered += 1
    need(entries == 31 and delivered == 10, 'périmètre des substitutions')
    frames = defaultdict(list)
    for name, expected in c['a_logs'].items():
        raw = (a.returned_a / name).read_bytes()
        need(sha(raw) == expected, 'journal A différent')
        rows = [json.loads(line) for line in raw.splitlines()]
        full = [row for row in rows if row['phase'] == 'full']
        need(len(full) == 74 and [r['pass'] for r in full] == list(range(74)), 'cohorte A')
        need([r['trame'] for r in full[:37]] == [r['trame'] for r in full[37:]], 'deux visites A')
        for r in full[37:]:
            e, overlap = r['etapes_ns'], r['recouvrement']
            need(r['kmax'] == 5 and r['threads'] == 48 and r['etapes_schema'] == 'recouvert', 'régime A')
            need(e['raccord'] == 0 and e['TMVR'] == overlap['queue_ns'] and
                 overlap['fin_ns'] == e['G'] + overlap['queue_ns'], 'queue A')
            pc = e['P'] + e['C']
            pcg = pc + e['G']
            without_queue = r['wall_ns'] - overlap['queue_ns']
            need(without_queue >= pcg, 'résidu mural négatif')
            frames[r['trame']].append(dict(P_plus_C=pc, P_plus_C_plus_G=pcg,
                mur_sans_queue=without_queue, residu=without_queue-pcg, mur=r['wall_ns']))
    need(len(frames) == 37 and all(len(v) == 3 for v in frames.values()), '37 fois 3 chaudes')
    def summarize(key):
        medians = [st.median(r[key] for r in rows) for rows in frames.values()]
        maxima = [max(r[key] for r in rows) for rows in frames.values()]
        flat = [r[key] for rows in frames.values() for r in rows]
        return dict(mediane_des_37_medianes_ns=st.median(medians), max_des_37_medianes_ns=max(medians),
                    mediane_111_ns=st.median(flat), max_111_ns=max(flat),
                    medianes_trames_superieures_100ms=sum(x > 100_000_000 for x in medians),
                    trames_avec_passe_superieure_100ms=sum(x > 100_000_000 for x in maxima))
    for name, expected in c['a_logs'].items():
        need(sha((a.returned_a / name).read_bytes()) == expected, 'journal A devenu mobile')
    result = dict(native_calls=0, sources={k:v['files_exact'] for k,v in c['source_archives'].items()},
                  future_pre_postimages=entries, delivered_after_files=delivered,
                  scope='A mesuré, 37 trames, trois chaudes par trame ; scénario sans queue à autres coûts inchangés',
                  a_scenario_ns={k:summarize(k) for k in next(iter(frames.values()))[0]},
                  b_results_qualified=False, shutdown_qualified=False)
    print(json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
