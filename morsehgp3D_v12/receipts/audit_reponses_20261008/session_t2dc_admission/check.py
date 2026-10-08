#!/usr/bin/env python3
"""Admission de journaux déjà produits ; aucune sonde ni lecture de données.
Usage: python [-O] check.py DEPOT DOSSIER_RETOURNE
"""
import hashlib
import importlib
import json
import math
from pathlib import Path
import random
import shlex
import statistics as st
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
C = json.loads((Path(__file__).with_name('capture.json')).read_text())
REPO, RAW = map(Path, sys.argv[1:3])


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def pairs(items):
    out = {}
    for k, v in items:
        need(k not in out, 'clé JSON répétée')
        out[k] = v
    return out


def bad_constant(value):
    raise ValueError('constante JSON interdite')


def loads(s):
    return json.loads(s, object_pairs_hook=pairs, parse_constant=bad_constant)


def blob(path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', C['commit'] + ':' + path])


def exact(entries, fields, expected):
    need(type(entries) is list, 'cohorte non liste')
    found = {}
    for e in entries:
        need(type(e) is dict, 'entrée non objet')
        key = tuple(e.get(k) for k in fields)
        need(all(type(v) in (str, int) for v in key), 'clé non scalaire stricte')
        need(key in expected and key not in found, 'cohorte étrangère ou répétée')
        found[key] = e
    need(set(found) == set(expected), 'cohorte incomplète')
    return found


def main(L, J):
    files = {str(p.relative_to(RAW)): sha(p.read_bytes()) for p in sorted(RAW.rglob('*')) if p.is_file()}
    need(len(files) == C['returned_files'] and sha(''.join(h + ' ' + p + '\n' for p, h in sorted(files.items())).encode())
         == C['returned_tree_sha256'], 'inventaire des traces changé')
    need(files['report.json'] == C['report_sha256'], 'rapport changé')
    r = loads((RAW / 'report.json').read_text())
    s, opts = r['steps'], r['options']
    need(r['schema'] == 'mhgp12_g4_catalogue_flux_v2' and r['rule'] == J.RULE, 'protocole changé')
    need(all(type(opts[k]) is int and opts[k] == v for k, v in [('threads', 48), ('passes', 10), ('rounds', 10)])
         and opts['essai'] is False, 'configuration étrangère')
    cohort = blob(C['cohort_path'])
    need(sha(cohort) == C['cohort_capture_sha256'], 'pin métadonnées 37')
    names = {e['name']: e['sites'] for e in loads(cohort)['cohort']}
    stems = dict(ng00='lidar_ng00', ng01='lidar_ng01', ng02='lidar_ng02',
                 u8000='uniform_u18_n8000', u16000='uniform_u18_n16000', u32000='uniform_u18_n32000')
    checked, passes = {}, {'catalogue': 0, 'full': 0}
    refs = {(e['case'], e['k']): next(x for x in e['cpu']['rows'] if x['phase'] == 'catalogue')
            for e in s['identity']}
    v12refs = {}

    def read(run, label, case, arm, spec=None, full_k=None):
        need(type(run['code']) is int and run['code'] == 0 and run['timeout'] is False and
             type(run['bad_lines']) is int and run['bad_lines'] == 0, 'exécution non réussie: ' + label)
        text = (RAW / 'logs' / (label + '.log')).read_text()
        head, body = text.split('\n', 1)
        stdout, stderr = body.split('\n--- stderr ---\n', 1)
        need(not stderr.strip() and stdout.isascii() and head.startswith('$ '), 'journal mal formé: ' + label)
        rows = [loads(line) for line in stdout.splitlines() if line.strip()]
        need(rows == run['rows'] and all(type(x) is dict for x in rows), 'journal/rapport différents: ' + label)
        cmd = shlex.split(head[2:])
        probe = Path(cmd[0])
        binary = 'mhgp12_full_probe' if full_k else 'mhgp12_catalogue_probe'
        need(probe.name == binary, 'sonde incorrecte')
        need(probe.parent.name == ('b' if arm == 'mutant' else 'b_' + arm), 'bras binaire incorrect')
        stem = stems.get(case, case)
        if full_k:
            fields = cmd[1].removeprefix('--trame=').split(',')
            need(cmd[1].startswith('--trame=') and len(fields) == 3 and fields[2] == case and
                 [Path(x).name for x in fields[:2]] == [stem + '.u32le', stem + '.ids.u32le'], 'entrée FULL')
            expected = L.full_options(full_k, 48, int(run['options'][2].split('=')[1]))
            need(cmd[2:] == run['options'] == expected, 'options FULL journal/rapport')
            state, why, _, _, _ = L.read_full(run, case, full_k, 48, int(expected[2].split('=')[1]))
        else:
            need([Path(x).name for x in cmd[1:3]] == [stem + '.u32le', stem + '.ids.u32le'], 'entrée catalogue')
            need(cmd[3:] == run['options'] == L.catalogue_options(spec), 'options catalogue journal/rapport')
            state, why, _ = L.read_catalogue(run, spec)
        need(state == 'ok', 'lecteur: ' + label + ': ' + why)
        for row in rows:
            phase = row['phase']
            if phase == 'liberation':
                need(type(row['pass']) is int, 'indice booléen')
            if phase == 'catalogue':
                need(row['wall_ns'] > 0 and row['sites'] > 0 and row['balls'] == row['ledger']['emitted'] and
                     row['incidences'] == row['ledger']['incidences'], 'compteurs catalogue incohérents')
                if spec['path'] == 'cpu':
                    need(all(v == 0 for v in row['device'].values()), 'diagnostic CPU appareil non nul')
                ref = refs.get((case, spec['k']))
                if arm != 'mutant' and ref is not None:
                    need(all(row[k] == ref[k] for k in L.COUNTS) and row['ledger'] == ref['ledger'],
                         'catalogue différent de son identité')
                if case in names:
                    ref = v12refs.setdefault(case, row)
                    need(all(row[k] == ref[k] for k in L.COUNTS) and row['ledger'] == ref['ledger'],
                         'comptes ou grand livre des 37 instables')
            if phase == 'full':
                mem = row['memoire_octets']
                need(row['pic_octets'] >= 16 * row['sites'] and row['pic_appareil_octets'] == 0 and
                     row['epinglee_octets'] <= row['pic_octets'], 'mémoire FULL partagée')
                need(all(mem[b][1] >= mem[a][0] for a, b in zip(L.MEM_STAGES, L.MEM_STAGES[1:])),
                     'pic suivant inférieur au résident précédent')
                need(row['sites'] == refs[case, full_k]['sites'], 'sites FULL hors identité')
                if case + ':' + str(full_k) in J.FUL1_SESSION_K:
                    need(row['full_sha256'] == J.FUL1_SESSION_K[case + ':' + str(full_k)],
                         'empreinte FULL différente de K')
            if phase in passes:
                passes[phase] += 1
                if case in names:
                    need(row['sites'] == names[case], 'sites des 37')
        need(label not in checked, 'journal réutilisé')
        checked[label] = files['logs/' + label + '.log']

    ids = exact(s['identity'], ('case', 'k'), set(J.IDENTITY_CASES))
    for (case, k), e in ids.items():
        for route, n, suffix in [('cpu', 1, 'cpu'), ('device', 3, 'dev')]:
            read(e[route], 'id_%s_k%d_%s' % (case, k, suffix), case, 'apres',
                 L.catalogue_spec(route, k, 48, n, True, True, route == 'device'))
    arms = exact(s['arms_identity'], ('arm', 'case'), {(a, f) for a in J.BUILT_ARMS for f in J.FRAMES})
    for (a, f), e in arms.items():
        read(e['run'], 'bras_%s_%s' % (a, f), f, a, L.catalogue_spec('device', 5, 48, 2, True))
    ful = exact(s['ful1'], ('arm', 'case', 'k'), {(a, f, k) for a in ('avant', 'apres') for f, k in J.FUL1_CASES})
    for (a, f, k), e in ful.items():
        need(e['run']['options'] == L.full_options(k, 48, 2), 'passes FUL1')
        read(e['run'], 'ful1_%s_%s_k%d' % (a, f, k), f, a, full_k=k)
    campaign = exact(s['campaign'], ('round', 'frame', 'arm'),
                     {(i, f, a) for i in range(10) for f in J.FRAMES for a in J.ARMS})
    order_keys = []
    for i in range(10):
        order = list(J.ARMS[i % 7:] + J.ARMS[:i % 7])
        if i % 2:
            order.reverse()
        for f in J.FRAMES:
            for pos, a in enumerate(order):
                e = campaign[i, f, a]
                need(type(e['position']) is int and e['position'] == pos, 'ordre de campagne')
                order_keys.append((i, f, a))
                read(e['run'], 'c_r%d_%s_%s' % (i, f, a), f, 'avant' if a == 'avant_bis' else a,
                     L.catalogue_spec('device', 5, 48, 10, sorties=a not in J.HISTORICAL))
    need(order_keys == [(e['round'], e['frame'], e['arm']) for e in s['campaign']], 'ordre des prises')
    need(s['mutant']['id'] == 'flux_sans_attente_appareil', 'identité mutant')
    read(s['mutant']['run'], 'mutant_ng00', 'ng00', 'mutant', L.catalogue_spec('device', 5, 48, 2, True))
    info = s['informations']
    need(info['non_joues'] == [], 'informations manquantes')
    for group, k, n, arms2 in [('k10', 10, 5, ('avant', 'apres')), ('cache', 5, 10, ('apres', 'apres_cache'))]:
        entries = exact(info[group], ('round', 'frame', 'arm'), {(i, f, a) for i in range(3) for f in J.FRAMES for a in arms2})
        for (i, f, a), e in entries.items():
            need(e['k'] == k and type(e['k']) is int, 'K informatif')
            read(e['run'], 'i_%s_r%d_%s_%s' % (group, i, f, a), f, 'apres' if a == 'apres_cache' else a,
                 L.catalogue_spec('device', k, 48, n, sorties=a not in J.HISTORICAL, cache=(4 << 30) if a == 'apres_cache' else 0))
    entries = exact(info['full'], ('round', 'frame', 'arm'), {(i, f, a) for i in range(3) for f in J.FRAMES for a in ('avant', 'apres')})
    for (i, f, a), e in entries.items():
        need(e['run']['options'] == L.full_options(5, 48, 10), 'passes FULL informatif')
        read(e['run'], 'i_full_r%d_%s_%s' % (i, f, a), f, a, full_k=5)
    entries = exact(info['v12set'], ('frame', 'arm'), {(f, a) for f in names for a in ('avant', 'apres')})
    for (f, a), e in entries.items():
        read(e['run'], 'i_v12set_%s_%s' % (f, a), f, a,
             L.catalogue_spec('device', 5, 48, 6, sorties=a not in J.HISTORICAL))
    verdict = J.judge(r)
    gate_logs = {}
    for group in ('device_open', 'device_open_budget'):
        gate = s['gates'][group]
        need(type(gate['code']) is int and gate['code'] == 0 and gate['timeout'] is False, 'code porte GPU')
        _, body = (RAW / 'logs' / (group + '.log')).read_text().split('\n', 1)
        stdout, stderr = body.split('\n--- stderr ---\n', 1)
        need(stdout[-4000:] == gate['stdout'] and not stderr.strip(), 'porte GPU journal/rapport')
        gate_logs[group] = gate['stdout'].splitlines()
    for moment in ('avant', 'apres'):
        _, body = (RAW / 'logs' / ('gpu_apps_' + moment + '.log')).read_text().split('\n', 1)
        stdout, stderr = body.split('\n--- stderr ---\n', 1)
        need(not stdout.strip() and not stderr.strip() and s['gpu_quiet_' + ('before' if moment == 'avant' else 'after')]
             is True, 'GPU occupé ou observation manquante')
    need(verdict['verdict'] == r['verdict']['verdict'] == 'adopte' and not verdict['refused'] and not verdict['rejected'],
         'verdict en écart')
    need(verdict['stats']['mutant'] == 'tue (empreinte)', 'mutant non comparé')
    # Recalcul indépendant par produits des rapports et tirages d'indices (le juge somme leurs logarithmes).
    rng = random.Random(20261008)
    draws = [rng.choices(range(10), k=10) for _ in range(10000)]
    independent = {}
    for name, a, b, decisive in J.LEVERS + (J.CONTROL + (J.FRAMES,),):
        independent[name] = {}
        for frame in J.FRAMES:
            ratios = []
            for i in range(10):
                medians = [st.median(x['wall_ns'] for x in campaign[i, frame, arm]['run']['rows']
                                    if x['phase'] == 'catalogue' and x['pass'] > 0) for arm in (a, b)]
                ratios.append(medians[1] / medians[0])
            boots = sorted(math.prod(ratios[i] for i in draw) ** 0.1 for draw in draws)
            values = dict(gm=math.prod(ratios) ** 0.1, low=boots[250], high=boots[9749])
            official = verdict['stats']['levers'][name]['frames'][frame]
            need(all(math.isclose(v, official[k], rel_tol=2e-15) for k, v in values.items()),
                 'recalcul indépendant des rapports en écart')
            independent[name][frame] = values
        held = (all(abs(independent[name][f]['gm'] - 1) <= .015 for f in decisive) if name == 'A/A' else
                all(independent[name][f]['high'] < 1 for f in decisive))
        need(held, 'règle indépendante non tenue: ' + name)
    numerical = []
    for name, lever in verdict['stats']['levers'].items():
        old = r['verdict']['stats']['levers'][name]
        need(lever['verdict'] == old['verdict'], 'décision de levier différente')
        for frame, values in lever['frames'].items():
            for key in ('gm', 'low', 'high'):
                x, y = values[key], old['frames'][frame][key]
                need(math.isclose(x, y, rel_tol=2e-15), 'écart numérique significatif')
                if x != y:
                    numerical.append({'lever': name, 'frame': frame, 'field': key, 'absolute_delta': abs(x-y)})

    def warm(e, phase='catalogue'):
        return [x for x in e['run']['rows'] if x['phase'] == phase][1:]

    def measure(entries, a, f, phase='catalogue'):
        es = [e for e in entries if e['arm'] == a and e['frame'] == f]
        ns = [x['wall_ns'] for e in es for x in warm(e, phase)]
        return {'processes': len(es), 'warm_passes': len(ns), 'median_ns': st.median(ns),
                'max_process_median_ns': max(st.median(x['wall_ns'] for x in warm(e, phase)) for e in es)}

    summaries = {}
    for group, rows, arms2, phase in [('C5', s['campaign'], J.ARMS, 'catalogue'),
                                     ('C10', info['k10'], ('avant', 'apres'), 'catalogue'),
                                     ('cache', info['cache'], ('apres', 'apres_cache'), 'catalogue'),
                                     ('FULL5', info['full'], ('avant', 'apres'), 'full')]:
        summaries[group] = {f: {a: measure(rows, a, f, phase) for a in arms2} for f in J.FRAMES}
    v12 = {f: {a: measure(info['v12set'], a, f) for a in ('avant', 'apres')} for f in names}
    v12_summary = {a: {'frames': len(v12), 'median_frame_median_ns': st.median(v[a]['median_ns'] for v in v12.values()),
                       'max_frame_median_ns': max(v[a]['median_ns'] for v in v12.values())} for a in ('avant', 'apres')}
    stages = {}
    for frame in J.FRAMES:
        stages[frame] = {}
        for arm in ('avant', 'apres'):
            blocks = []
            for i in range(10):
                rows = campaign[i, frame, arm]['run']['rows']
                out_rows = {x['pass']: x for x in rows if x['phase'] == 'sorties'}
                for row in rows:
                    if row['phase'] != 'catalogue' or row['pass'] == 0:
                        continue
                    fields = dict(row['diagnostics'], transfer_ns=row['device']['transfer_ns'],
                                  publish_ns=row['device']['publish_ns'])
                    if arm == 'apres':
                        fields['outputs_ns'] = out_rows[row['pass']]['outputs_ns']
                    blocks.append({k: sum(fields[f] for f in keys) if all(f in fields for f in keys) else None
                                   for k, keys in J.STAGES.items()})
            stages[frame][arm] = {k: st.median(b[k] for b in blocks) if blocks[0][k] is not None else None
                                  for k in J.STAGES}
    return {'verdict': verdict, 'independent_ratio_statistics': independent, 'gate_logs': gate_logs,
            'small_numeric_differences': numerical, 'logs_linked': len(checked),
            'logs_tree_sha256': sha(''.join(h+' '+n+'\n' for n,h in sorted(checked.items())).encode()),
            'passes': passes, 'campaign_processes': len(campaign), 'campaign_warm_passes': 1890,
            'full_identity_cases': len(ful)//2, 'tables': summaries, 'C5_stages_medians_ns': stages,
            'C5_v12set': v12_summary,
            'return_codes_scope': 'entiers archivés dans report.json par Session.run ; pas de codes externes par sonde'}


with tempfile.TemporaryDirectory(prefix='t2dc-admission-') as folder:
    root = Path(folder)
    for name, expected in C['sources'].items():
        b = blob('morsehgp3D_v12/bench/' + name)
        need(sha(b) == expected, 'source différente')
        (root / name).write_bytes(b)
    sys.path.insert(0, str(root))
    L = importlib.import_module('g4_catalogue_flux_lecteur')
    J = importlib.import_module('g4_catalogue_flux_judge')
    print(json.dumps(main(L, J), ensure_ascii=False, indent=2, sort_keys=True))
