"""Contre-JSON inventés ; aucun moteur ni commande du pilote exécuté."""
import argparse
import ast
import copy
import hashlib
import json
from pathlib import Path
import statistics
import subprocess

import reader as audit


def subset(text, names, namespace):
    tree = ast.parse(text)
    body = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in names]
    audit.need(len(body) == len(names), 'fonctions fixture manquantes')
    exec(compile(ast.Module(body=body, type_ignores=[]), 'pinned_fixture_ast', 'exec'), namespace)
    return namespace


def fixture(capture, repo, lf):
    cap = copy.deepcopy(capture)
    cap['cases'] = [['real_a', 'c000', 100, 'reel'], ['uniform_a', 'c001', 200, 'uniform'],
                    ['real_b', 'c002', 400, 'reel'], ['uniform_b', 'c003', 800, 'uniform'],
                    ['hard_a', 'c004', 150, 'lattice'], ['hard_b', 'c005', 300, 'sphere']]
    get = lambda p: subprocess.check_output(['git', '-C', repo, 'show',
                                             cap['commit'] + ':morsehgp3D_v12/' + p]).decode()
    rows = subset(get('microbancs/outils/test_lecteur_full.py'), {'full_row'},
                  dict(lf=lf, LABEL='synthetic', SITES=100, SHA='a' * 64))
    producer = subset(get('microbancs/mes_c_petits/pilote_c.py'),
                      {'fit', 'session_values', 'fits_of', 'verdicts'},
                      dict(statistics=statistics, FIXED_LIMIT_NS=2e6, MAIN_REGIME_NS_PER_SITE=241.3e6 / 64740))
    env = dict(cmake='cmake synthetic', nvcc='nvcc synthetic', gpu='synthetic GPU', gpu_apps='')
    p = dict(pilote_sha256=cap['sources']['microbancs/mes_c_petits/pilote_c.py'],
             lecteur_sha256=cap['sources']['microbancs/outils/lecteur_full.py'],
             archive_sha256=cap['data_archive']['sha256'], sonde_sha256='b' * 64,
             cmake=['CMAKE_BUILD_TYPE:STRING=Release', 'MHGP12_COORD_BITS:STRING=21', 'MHGP12_ENABLE_CUDA:BOOL=ON'])
    argv = []
    for key, value in cap['command_options'].items():
        argv += [key, value if value is not None else '/synthetic/' +
                 (cap['data_archive']['name'] if key == '--archive' else 'path')]
    report = dict(mesure='MES-C', regime='c', environnement=dict(avant=env, apres=env.copy()), provenance=p,
                  controles=[], configurations={}, difficiles=[],
                  parametres=dict(tours=3, budget_octets=cap['configuration']['budget_octets'], argv=argv))
    raws, codes = {}, {}
    for spec in audit.specifications(cap):
        sequence, passes = [], []
        if spec['voie'] == 'appareil':
            sequence.append(dict(phase='open', status='ok', reason='none', wall_ns=100, budget_appareil='separe'))
        floor = 16 * sum(c['sites'] for c in spec['cases'])
        for i in range(len(spec['cases']) * spec['rounds']):
            case = spec['cases'][i % len(spec['cases'])]
            turn = i // len(spec['cases'])
            wall = 1_000_000 + 10 * case['sites'] + turn * 10000 + (90_000_000 if turn == 0 else 0)
            row = rows['full_row'](i, wall)
            row['pass'] = row.pop('pass_')
            row.update(trame=case['etiquette'], sites=case['sites'], kmax=spec['k'], threads=spec['fils'],
                       voie='device' if spec['voie'] == 'appareil' else 'cpu', cpu_ns=wall,
                       full_sha256=hashlib.sha256(('%s:%d' % (case['nom'], spec['k'])).encode()).hexdigest(),
                       pic_octets=floor + 4096, rss_max_octets=floor + 8192,
                       memoire_octets={stage: [floor, floor + 4096] for stage in audit.STAGES},
                       appareil_octets=0, epinglee_octets=0, pic_appareil_octets=0)
            if spec['voie'] == 'appareil':
                row.update(appareil_octets=1024, epinglee_octets=64, pic_appareil_octets=2048)
            sequence += [row, dict(phase='liberation', pass_=i, liberation_ns=3)]
            sequence[-1]['pass'] = sequence[-1].pop('pass_')
            passes.append(row)
        sequence.append(dict(phase='exit', status='ok', reason='none'))
        raws[spec['file']] = ''.join(json.dumps(row) + '\n' for row in sequence)
        codes[spec['file']] = 0
        if spec['kind'] == 'session':
            values = producer['session_values'](passes, spec['cases'])
            report['configurations'][spec['key']] = dict(etat='ok', raison='', valeurs=values,
                                                       droites=producer['fits_of'](values))
        else:
            c = spec['cases'][0]
            report['difficiles'].append(dict(nom=c['nom'], groupe=c['groupe'], sites=c['sites'], voie=spec['voie'],
                                            k=spec['k'], fils=spec['fils'], etat='ok', raison='',
                                            passes=[dict(mur_ns=p['wall_ns'], full_sha256=p['full_sha256']) for p in passes]))
    report['criteres'] = producer['verdicts'](report['configurations'], report['difficiles'])
    report['verdict'] = 'tenu'
    return cap, report, raws, codes


def mutate_line(raws, name, change, phase='full', occurrence=0):
    lines = [json.loads(s) for s in raws[name].splitlines()]
    chosen = [r for r in lines if r['phase'] == phase][occurrence]
    change(chosen)
    raws[name] = ''.join(json.dumps(r) + '\n' for r in lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', required=True)
    args = parser.parse_args()
    capture = json.loads(Path(__file__).with_name('capture.json').read_text())
    lf, reasons = audit.load_sources(args.repo, capture)
    cap, report, raws, codes = fixture(capture, args.repo, lf)
    base = audit.review(report, raws, cap, lf, reasons, codes)
    audit.need(base['verdict'] == 'tenu' and not base['differences'] and not base['controles'], 'nominal')
    audit.need(base['passes_completes'] == 160 and base['passes_chaudes'] == 104, 'temperature par trame')
    line = base['configurations']['cpu:5:48']['droites']['reel']
    audit.need(line['fixe_ns'] == 1_015_000 and line['par_site_ns'] == 10, 'regression connue')
    infer = audit.review(report, raws, cap, lf, reasons)
    audit.need(infer['criteres'] == base['criteres'] and infer['passes_completes'] == 160, 'codes inferes')
    results = {'nominal': 'tenu', 'codes_inferes': 'tenu', 'cold_removed_per_frame': True,
               'K_hashes_separate': True, 'mutations': {}}
    file = 'session_cpu_5_48.jsonl'
    alterations = {
        'label': lambda r: r.update(trame='c001'),
        'sites': lambda r: r.update(sites=r['sites'] + 1),
        'threads': lambda r: r.update(threads=1),
        'K': lambda r: r.update(kmax=10),
        'zero_wall': lambda r: r.update(wall_ns=0),
        'bool': lambda r: r.update(cpu_ns=True),
        'u64': lambda r: r.update(cpu_ns=1 << 64),
        'memory_floor': lambda r: (r.update(pic_octets=10), r.update(memoire_octets={s: [1, 10] for s in audit.STAGES})),
        'memory_continuity': lambda r: r['memoire_octets'].update(G=[1, 1]),
        'pinned_peak': lambda r: r.update(epinglee_octets=r['pic_octets'] + 1),
        'G_overlap': lambda r: r['g_ns'].update(tables=r['etapes_ns']['G']),
        'digest': lambda r: r.update(full_sha256='f' * 64),
    }
    for name, mutation in alterations.items():
        rr = copy.deepcopy(raws)
        mutate_line(rr, file, mutation)
        got = audit.review(report, rr, cap, lf, reasons, codes)
        audit.need(got['verdict'] == 'refuse', name)
        results['mutations'][name] = got['verdict']
    for name, change in [('nan', lambda t: t.replace('"cpu_ns":', '"cpu_ns": NaN, "unused":', 1)),
                         ('duplicate', lambda t: t.replace('"phase": "full",', '"phase": "full", "phase": "full",', 1)),
                         ('nonobject', lambda t: 'null\n' + t)]:
        rr = dict(raws)
        rr[file] = change(rr[file])
        got = audit.review(report, rr, cap, lf, reasons, codes)
        audit.need(got['verdict'] == 'refuse', name)
        results['mutations'][name] = got['verdict']
    for code in [False, 0.0]:
        cc = dict(codes)
        cc[file] = code
        got = audit.review(report, raws, cap, lf, reasons, cc)
        audit.need(got['verdict'] == 'refuse', 'code exact')
    results['mutations']['code_bool_float'] = 'refuse'
    for name, action in [('OLS', lambda r: r['configurations']['cpu:5:48']['droites']['reel'].update(par_site_ns=0)),
                         ('cold_summary', lambda r: r['configurations']['cpu:5:48']['valeurs']['real_b'].update(chaud_ns=91_004_000))]:
        rp = copy.deepcopy(report)
        action(rp)
        got = audit.review(rp, raws, cap, lf, reasons, codes)
        audit.need(got['differences'] and got['criteres'] == base['criteres'], name)
        results['mutations'][name] = 'difference publiee detectee, bruts inchanges'
    for name in ['missing', 'non_joue', 'duplicate', 'threads']:
        rp, rr, cc = copy.deepcopy(report), dict(raws), dict(codes)
        row = rp['difficiles'][0]
        filename = '%s_k%d_%s.jsonl' % (row['nom'], row['k'], row['voie'])
        if name in {'missing', 'non_joue'}:
            rr.pop(filename)
            cc.pop(filename)
            if name == 'missing':
                rp['difficiles'].pop(0)
            else:
                row.update(etat='non_joue', raison='delai', passes=[])
            got = audit.review(rp, rr, cap, lf, reasons, cc)
            audit.need(got['criteres']['C3'] == 'non evalue' and got['verdict'] == 'refuse', name)
        else:
            if name == 'duplicate':
                rp['difficiles'].append(copy.deepcopy(row))
            else:
                row['fils'] = 1
            try:
                audit.review(rp, rr, cap, lf, reasons, cc)
            except audit.Invalid:
                pass
            else:
                raise audit.Invalid('cohorte difficile ' + name)
        results['mutations']['C3_' + name] = 'refuse/incomplet'
    for reason in ['memory_budget', 'unknown']:
        rp, rr, cc = copy.deepcopy(report), dict(raws), dict(codes)
        row = rp['difficiles'][0]
        filename = '%s_k%d_%s.jsonl' % (row['nom'], row['k'], row['voie'])
        rr[filename] = json.dumps(dict(phase='exit', status='resource_exhausted', reason=reason)) + '\n'
        cc[filename] = 2
        row.update(etat='refus', raison='resource_exhausted/' + reason, passes=[])
        got = audit.review(rp, rr, cap, lf, reasons, cc)
        audit.need(got['criteres']['C3'] == ('non tenu' if reason == 'memory_budget' else 'non evalue'), reason)
        results['mutations']['refus_' + reason] = got['verdict']
    for name, intercept, slope, expected in [('C1', 3_000_000, 10, 'C1'), ('C2', 1_000_000, 5000, 'C2')]:
        rr = dict(raws)
        lines = [json.loads(s) for s in rr[file].splitlines()]
        for row in lines:
            if row['phase'] == 'full':
                row['wall_ns'] = intercept + slope * row['sites']
                row['etapes_ns'] = {k: 1 for k in row['etapes_ns']}
                row['etapes_ns']['TMVR'] = 4
                row['etapes_ns']['G'] = 2
                row['g_ns'] = dict(tables=1, resolution=1)
        rr[file] = ''.join(json.dumps(r) + '\n' for r in lines)
        got = audit.review(report, rr, cap, lf, reasons, codes)
        audit.need(got['criteres'][expected] == 'non tenu' and got['verdict'] == 'non tenu', name)
        results['mutations'][name + '_measured_failure'] = got['verdict']
    rp, rr, cc = copy.deepcopy(report), dict(raws), dict(codes)
    row = rp['difficiles'][0]
    filename = '%s_k%d_%s.jsonl' % (row['nom'], row['k'], row['voie'])
    row.update(etat='echec', raison='expire', passes=[])
    rr[filename] = ''
    cc[filename] = 'expire'
    got = audit.review(rp, rr, cap, lf, reasons, cc)
    audit.need(got['criteres']['C3'] == 'non tenu', 'expiration observee')
    results['mutations']['expiration'] = got['verdict']
    device_file = 'session_appareil_5_48.jsonl'
    rr = dict(raws)
    mutate_line(rr, device_file, lambda r: r.update(appareil_octets=r['pic_appareil_octets'] + 1))
    audit.need(audit.review(report, rr, cap, lf, reasons, codes)['verdict'] == 'refuse', 'pic appareil')
    results['mutations']['device_peak'] = 'refuse'
    rp = copy.deepcopy(report)
    position = rp['parametres']['argv'].index('--fils') + 1
    rp['parametres']['argv'][position] = '48'
    audit.need(audit.review(rp, raws, cap, lf, reasons, codes)['verdict'] == 'refuse', 'arguments')
    results['mutations']['plan_threads'] = 'refuse'
    rp, rr, cc = copy.deepcopy(report), dict(raws), dict(codes)
    key, filename = 'cpu:10:1', 'session_cpu_10_1.jsonl'
    rr.pop(filename)
    cc.pop(filename)
    rp['configurations'][key] = dict(etat='non_joue', raison='delai')
    got = audit.review(rp, rr, cap, lf, reasons, cc)
    audit.need(got['verdict'] == 'tenu' and not got['cohorte_complete'], 'K10 informatif non joue')
    results['K10_non_joue'] = 'criteres K5 tenus, cohorte partielle'
    rp['configurations'].pop(key)
    audit.need(audit.review(rp, rr, cap, lf, reasons, cc)['verdict'] == 'refuse', 'ligne K10 absente')
    results['mutations']['K10_row_missing'] = 'refuse'
    rp, rr, cc = copy.deepcopy(report), dict(raws), dict(codes)
    rp['configurations'][key] = dict(etat='echec', raison='expire')
    rr[filename], cc[filename] = '', 'expire'
    audit.need(audit.review(rp, rr, cap, lf, reasons, cc)['verdict'] == 'refuse', 'Session K10 expiree')
    results['mutations']['K10_session_expired'] = 'refuse'
    rp, rr, cc = copy.deepcopy(report), dict(raws), dict(codes)
    hard10 = next(h for h in rp['difficiles'] if h['k'] == 10 and h['voie'] == 'cpu')
    filename10 = '%s_k10_cpu.jsonl' % hard10['nom']
    hard10.update(etat='echec', raison='expire', passes=[])
    rr[filename10], cc[filename10] = '', 'expire'
    audit.need(audit.review(rp, rr, cap, lf, reasons, cc)['verdict'] == 'tenu', 'difficile K10 informatif')
    results['K10_hard_expired'] = 'criteres K5 tenus'
    rp, rr, cc = copy.deepcopy(report), dict(raws), dict(codes)
    rp['configurations']['cpu:5:1'] = dict(etat='non_joue', raison='delai')
    rr.pop('session_cpu_5_1.jsonl')
    cc.pop('session_cpu_5_1.jsonl')
    got = audit.review(rp, rr, cap, lf, reasons, cc)
    audit.need(got['verdict'] == 'tenu' and not got['cohorte_complete'], 'Session K5 informative non jouee')
    results['K5_W1_non_joue'] = 'criteres W48 tenus, cohorte partielle'
    rr = dict(raws)
    mutate_line(rr, file, lambda r: r.update(cpu_ns=None), occurrence=4)
    audit.need(audit.review(report, rr, cap, lf, reasons, codes)['verdict'] == 'refuse', 'CPU null')
    results['mutations']['cpu_null'] = 'refuse/metrique manquante'
    rp = copy.deepcopy(report)
    rp['environnement']['apres']['gpu_apps'] = None
    audit.need(audit.review(rp, raws, cap, lf, reasons, codes)['verdict'] == 'refuse', 'GPU inconnu')
    results['mutations']['gpu_unknown'] = 'refuse'
    print(json.dumps(results, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
