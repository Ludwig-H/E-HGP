#!/usr/bin/env python3
"""Contre-exemples Python du juge T1-d ; sources Git, aucune sonde native."""
import argparse
import copy
import hashlib
import inspect
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True
PIN = '5f8e777cffbeddfe90e92fc616a920c28c1b985c'
PORT_PIN = 'c31beaf2200d1a7f8eed09b0c1c7c0f6af3b74f3'
PREFIX = 'morsehgp3D_v12/'
SOURCES = [PREFIX + 'bench/' + f for f in (
    'g4_catalogue_flux_judge.py', 'g4_catalogue_flux_lecteur.py', 'g4_catalogue_flux_selftest.py',
    'g4_catalogue_t1d_judge.py', 'g4_catalogue_t1d_selftest.py')]
SOURCES.append(PREFIX + 'microbancs/outils/lecteur_full.py')
JOURNAL = PREFIX + 'receipts/g4_t2db2_20261008/resultats/cmd/001_t2d_b2_pilote/files/t2d_b2/journaux/identite/ng00/apres.jsonl'


def child(tree, repo):
    sys.path.insert(0, str(tree / PREFIX / 'bench'))
    import g4_catalogue_flux_judge as J
    import g4_catalogue_flux_lecteur as L
    import g4_catalogue_t1d_judge as T
    import g4_catalogue_t1d_selftest as S

    report = S.synthetic_report()
    original = T.judge(copy.deepcopy(report))['verdict']
    # Meme temoin memoire coherent sur les deux lecteurs ; FULL non consulte par ces controles unitaires.
    for e in report['steps']['slices']:
        for row in e['run']['rows']:
            if row.get('phase') == 'catalogue':
                row['device']['device_bytes'] = e['budget'] or S.HELD
            elif row.get('phase') == 'tranches':
                row.update(device_bytes=e['budget'] or S.HELD, device_peak=e['budget'])
    refs_out = dict(refused=[], rejected=[], stats={})
    refs = J.check_identity(report['steps'], refs_out, 48)

    def slices(r):
        out = dict(refused=[], rejected=[], stats={})
        args = (r['steps'], out, 48)
        if 'refs' in inspect.signature(T.check_slices).parameters:
            args += (refs,)
        T.check_slices(*args)
        return 'refuse' if out['refused'] else 'rejete' if out['rejected'] else 'admis'

    def campaign(r):
        out = dict(refused=[], rejected=[], stats={})
        T.campaign_table(r['steps'], out, 10, 10, 48, refs)
        return 'refuse' if out['refused'] else 'admis'

    def entry(r):
        return S.slice_entry(r, 'ng00', 5, 0)

    def tranche(r, p=0):
        return next(x for x in entry(r)['run']['rows'] if x.get('phase') == 'tranches' and x['pass'] == p)

    def refusal(r, foreign=False):
        e = entry(r)
        row = {k: v for k, v in e['run']['rows'][1].items() if k in L.CAT_BASE_KEYS}
        row.update(status='resource_exhausted', reason='memory_budget')
        if foreign:
            row['threads'] = 1
        e['run'].update(code=2, rows=[e['run']['rows'][0], row,
                                      dict(phase='exit', status='resource_exhausted', reason='memory_budget')])

    def partial(r, bad_digest=False, bad_counts=False):
        e = entry(r)
        row = {k: v for k, v in e['run']['rows'][4].items() if k in L.CAT_BASE_KEYS}
        row.update(status='resource_exhausted', reason='memory_budget')
        e['run'].update(code=2, rows=e['run']['rows'][:4] + [row,
                       dict(phase='exit', status='resource_exhausted', reason='memory_budget')])
        if bad_digest:
            e['run']['rows'][3]['catalogue_sha256'] = '0' * 64
        if bad_counts:
            e['run']['rows'][1]['sites'] = 1

    def decreasing(r):
        tranche(r, 1).update(device_bytes=1, device_peak=1)
        next(x for x in entry(r)['run']['rows'] if x.get('phase') == 'catalogue' and x['pass'] == 1)[
            'device']['device_bytes'] = 1

    def extra(r, step, **changes):
        e = copy.deepcopy(r['steps'][step][0])
        e.update(changes)
        r['steps'][step].append(e)

    edits = [
        ('memoire_nominale', slices, lambda r: None),
        ('pic_depasse_budget', slices, lambda r: tranche(r).update(device_peak=S.HELD)),
        ('usage_superieur_pic', slices, lambda r: tranche(r).update(device_peak=0)),
        ('capacite_hors_catalogue', slices, lambda r: tranche(r).update(device_bytes=1)),
        ('pic_decroissant', slices, decreasing),
        ('budget_surplus', slices, lambda r: extra(r, 'slices', budget=7)),
        ('refus_memoire_valide', slices, refusal),
        ('refus_memoire_W1_hors_commande', slices, lambda r: refusal(r, True)),
        ('prefixe_refuse_valide', slices, partial),
        ('prefixe_refuse_faux', slices, lambda r: partial(r, bad_digest=True)),
        ('prefixe_refuse_comptes', slices, lambda r: partial(r, bad_counts=True)),
        ('campagne_nominale', campaign, lambda r: None),
        ('tour_surplus', campaign, lambda r: extra(r, 'campaign', round=10)),
        ('bras_surplus', campaign, lambda r: extra(r, 'campaign', arm='inconnu')),
    ]
    results = {'auto_test_nominal': original, 'controles': {}}
    for name, evaluate, edit in edits:
        r = copy.deepcopy(report)
        edit(r)
        results['controles'][name] = evaluate(r)
    # Journal public : deux vraies passes intactes, pas de donnees de scene.
    data = (repo / JOURNAL).read_bytes()
    rows = [json.loads(line) for line in data.splitlines()]
    run = dict(code=0, timeout=False, bad_lines=0, rows=rows, options=L.full_options(5, 48, 2))
    result = L.read_full(run, 'ng00', 5, 48, 2, 39885)
    results['full_reel'] = dict(sha256=hashlib.sha256(data).hexdigest(), etat=result[0], raison=result[1],
                              passes=len(result[2]))
    return results


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    p.add_argument('--child', type=Path)
    p.add_argument('--port-c31', action='store_true')
    args = p.parse_args()
    if args.child:
        print(json.dumps(child(args.child, args.repo), sort_keys=True, ensure_ascii=False))
        return
    receipt = Path(__file__).resolve().parent
    pin = PORT_PIN if args.port_c31 else PIN
    patch = 'port_c31.patch' if args.port_c31 else 'proposition.patch'
    out = dict(pin=pin, sources={}, avant=None, proposition=None)
    with tempfile.TemporaryDirectory(prefix='mhgp12_audit_t1d_') as d:
        tree = Path(d)
        for name in SOURCES:
            data = subprocess.check_output(['git', 'show', pin + ':' + name], cwd=args.repo)
            target = tree / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            out['sources'][name] = hashlib.sha256(data).hexdigest()
        for arm in ('avant', 'proposition'):
            if arm == 'proposition':
                subprocess.run(['git', 'apply', '--check', str(receipt / patch)], cwd=tree, check=True)
                subprocess.run(['git', 'apply', str(receipt / patch)], cwd=tree, check=True)
            cmd = [sys.executable, '-B', '-S'] + (['-O'] if sys.flags.optimize else [])
            cmd += [str(Path(__file__).resolve()), '--repo', str(args.repo), '--child', str(tree)]
            out[arm] = json.loads(subprocess.check_output(cmd, text=True))
    expected = {'memoire_nominale', 'refus_memoire_valide', 'campagne_nominale', 'prefixe_refuse_valide'}
    a, b = out['avant'], out['proposition']
    ok = a['auto_test_nominal'] == b['auto_test_nominal'] == 'adopte'
    ok = ok and all(v == 'admis' for v in a['controles'].values())
    ok = ok and all(v == ('admis' if k in expected else 'rejete' if k == 'prefixe_refuse_faux' else 'refuse') for k, v in b['controles'].items())
    ok = ok and a['full_reel']['etat'] == 'illisible'
    ok = ok and b['full_reel']['etat'] == ('illisible' if args.port_c31 else 'ok')
    ok = ok and b['full_reel']['passes'] == (0 if args.port_c31 else 2)
    out['conforme'] = ok
    print(json.dumps(out, indent=2, sort_keys=True, ensure_ascii=False))
    if not ok:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
