#!/usr/bin/env python3
"""Read B metadata/JSONL only; import pinned readers, reconstruct statistics independently."""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import statistics
import subprocess
import tempfile

PIN = '41d4d828b70ad033f8bf392aca20a4af13cea242'
FILES = ('microbancs/mes_t2d_b/pilote_t2d_b.py', 'microbancs/mes_t2d_b/bras_t2d_b.json',
         'microbancs/outils/lecteur_full.py')
FRAMES = {'ng00': 39885, 'ng01': 35551, 'ng02': 45845}
ARMS = ('avant', 'avant_bis', 'garde', 'report', 'temoins', 'census', 'proposition', 'apres')
PAIRS = (('lot_t2d_b', 'avant', 'apres'), ('garde_seule', 'avant', 'garde'),
         ('report_seul', 'avant', 'report'), ('temoins_seuls', 'avant', 'temoins'),
         ('census_combine', 'avant', 'census'), ('proposition', 'avant', 'proposition'),
         ('temoins_apres_garde', 'garde', 'census'), ('proposition_apres_census', 'census', 'apres'),
         ('A/A', 'avant', 'avant_bis'))

def need(ok, message):
    if not ok:
        raise ValueError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def canonical(x):
    return json.dumps(x, sort_keys=True, separators=(',', ':'), allow_nan=False)

def strict(data):
    def pairs(xs):
        d = {}
        for k, v in xs:
            need(k not in d, 'duplicate JSON key')
            d[k] = v
        return d
    def bad(value):
        raise ValueError('nonfinite JSON ' + value)
    return json.loads(data, object_pairs_hook=pairs, parse_constant=bad)

def compare(a, b, path='', floats=None):
    if isinstance(a, dict):
        need(type(b) is dict and set(a) == set(b), 'keys ' + path)
        for k in a:
            compare(a[k], b[k], path + '/' + k, floats)
    elif isinstance(a, list):
        need(type(b) is list and len(a) == len(b), 'length ' + path)
        for i, v in enumerate(a):
            compare(v, b[i], path + '/' + str(i), floats)
    elif type(a) is float and type(b) is float and a != b and floats is not None:
        delta = abs(a-b)
        need(delta <= max(math.ulp(a), math.ulp(b)), 'float mismatch ' + path)
        floats.append({'path': path, 'local': a, 'archived': b, 'ulp': delta/max(math.ulp(a), math.ulp(b))})
    else:
        need(type(a) is type(b) and a == b, 'value ' + path)

def bootstrap(values, rng):
    logs = [math.log(x) for x in values]
    boot = sorted(sum(logs[rng.randrange(len(logs))] for _ in logs)/len(logs) for _ in range(10000))
    return {'moyenne_geometrique': math.exp(sum(logs)/len(logs)),
            'ic95': [math.exp(boot[250]), math.exp(boot[9749])],
            'rapports': [math.exp(x) for x in logs]}

def run(repo, returned, capture):
    raw_report = (returned/'rapport_t2d_b.json').read_bytes()
    need(sha(raw_report) == capture['report_sha256'], 'report pin')
    report = strict(raw_report)
    sources = {}
    with tempfile.TemporaryDirectory(prefix='audit-b-read-') as tmp:
        for name in FILES:
            data = subprocess.check_output(['git', 'show', PIN+':morsehgp3D_v12/'+name], cwd=repo)
            need(sha(data) == capture['source_sha256'][name], 'source pin ' + name)
            p = Path(tmp)/name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)
            sources[name] = p
        spec = importlib.util.spec_from_file_location('audit_pinned_b', sources[FILES[0]])
        b = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(b)
        camp = report['campagne_k5']
        need((camp['fils'], camp['passes'], camp['tours_demandes']) == (48, 10, 10), 'campaign plan')
        compare(report['regle'], json.loads(json.dumps(b.REGLE_T2D_B)))
        construction = report['construction']
        need(set(construction['binaires']) == set(ARMS), 'binary cohort')
        need(construction['archive_avant_sha256'] == capture['before_archive_sha256'], 'before archive')
        need(construction['bras_sha256'] == capture['source_sha256'][FILES[1]], 'arm substitutions')
        need(set(camp['binaires_apres']) == set(ARMS), 'binary end cohort')
        for arm in ARMS:
            h = b.hexadecimal(construction['binaires'][arm]['sha256'])
            need(h == camp['binaires_apres'][arm], 'binary changed ' + arm)
        need(construction['binaires']['avant']['sha256'] == construction['binaires']['avant_bis']['sha256'], 'A/A binary')
        seen, gcounts, fullcounts = {}, [0, 0, 0], [0, 0, 0]

        def journal(take, relative):
            need(take['journal'] == relative and relative not in seen, 'journal binding/duplicate')
            need(type(take['code']) is int and take['code'] == 0, 'process code')
            data = (returned/relative).read_bytes()
            h = sha(data)
            if 'journal_sha256' in take:
                need(h == take['journal_sha256'], 'journal hash')
            seen[relative] = h
            return returned/relative, data

        def gtake(take, relative, k, threads, passes, sites, profile=False):
            path, raw = journal(take, relative)
            parsed = b.lire_prise(path, take['code'], k, threads, passes, profile)
            need(parsed['sites'] == sites, 'input sites')
            for key, value in parsed.items():
                compare(value, take[key], relative+'/'+key)
            rows = [strict(x) for x in raw.splitlines()]
            gcounts[0] += 1
            gcounts[1] += passes
            gcounts[2] += passes-1
            return parsed, rows

        need(set(camp['trames']) == set(FRAMES), 'decisive frame cohort')
        decisive, official_cases, rng = {}, {}, random.Random(20261008)
        for frame, sites in FRAMES.items():
            turns = camp['trames'][frame]
            need(type(turns) is list and len(turns) == 10, 'turn count')
            by_arm = {a: [] for a in ARMS}
            digests, work, objects = set(), set(), set()
            for t, turn in enumerate(turns):
                need(set(turn) == set(ARMS), 'arm cohort')
                for arm in ARMS:
                    take, rows = gtake(turn[arm], f'journaux/k5/{frame}/{arm}_t{t:02}.jsonl', 5, 48, 10, sites)
                    by_arm[arm].append(take)
                    digests.add(take['empreinte'])
                    work.add(take['travail_sha256'])
                    objects.add(canonical([r['objet'] for r in rows if r['phase'] == 'ordre']))
            need(len(digests) == 1 and len(objects) == 1, 'object identity')
            digest = next(iter(digests))
            need(frame != 'ng00' or digest.startswith('e5a81154fb1b15f1'), 'ng00 engraved gate')
            c = {'tours': 10, 'empreintes': [digest[:16]], 'empreinte_identique': True,
                 'travail_identique_entre_bras': len(work) == 1,
                 'g_ms_median': {a: statistics.median(q['g_ns'] for q in by_arm[a])/1e6 for a in ARMS}}
            for name, before, after in PAIRS:
                c[name] = bootstrap([by_arm[after][i]['g_ns']/by_arm[before][i]['g_ns'] for i in range(10)], rng)
            official_cases[frame] = c
            decisive[frame] = {'sites': sites, 'resolution_sha256': digest,
                              'work_identical': len(work) == 1, 'object_rows_identical': True,
                              'g_ms_median_process': c['g_ms_median'],
                              'comparisons': {n:{key:value for key,value in c[n].items() if key != 'rapports'}
                                              for n,_,_ in PAIRS}}
        aa = [official_cases[f]['A/A']['moyenne_geometrique'] for f in FRAMES]
        need(all(abs(x-1) <= 0.015 for x in aa), 'A/A veto')
        verdicts = {name: 'adopte' if all(official_cases[f][name]['ic95'][1] < 1 for f in FRAMES) else 'rejete'
                    for name,_,_ in PAIRS[:6]}
        judgement = {'verdicts': verdicts, 'refus': [], 'regle': report['regle'], 'cas': official_cases,
                     'controle_aa': {'moyennes': aa, 'dans_la_fenetre': True}}
        float_deltas = []
        compare(judgement, report['jugement'], floats=float_deltas)
        official = json.loads(json.dumps(b.juger(report, str(returned))))
        compare(judgement, official)
        need(set(report['informations']) == {'constructions','debut','fin','k10_w48','uniformes_k5_w48',
                                            'profil_k5_w1','full_k5_appareil'}, 'information cohort')
        information = {}
        plans = [('k10_w48', FRAMES, 10, 48, 5, 2, ('avant','apres'), False),
                 ('uniformes_k5_w48', {f'u{n}':n for n in (8000,16000,32000)},5,48,5,1,('avant','apres'),False),
                 ('profil_k5_w1', {'ng00':39885},5,1,3,1,('profil_avant','profil_apres'),True)]
        for name, frames, k, threads, passes, turns_n, arms, prof in plans:
            block = report['informations'][name]
            need((block['k'],block['fils'],block['passes']) == (k,threads,passes), 'information config')
            need(set(block['tours']) == set(frames), 'information frames')
            information[name] = {}
            for frame, sites in frames.items():
                turns = block['tours'][frame]
                need(len(turns) == turns_n, 'information turns')
                by_arm = {a:[] for a in arms}
                order_rows = {a:[] for a in arms}
                for t, turn in enumerate(turns):
                    need(set(turn) == set(arms), 'information arms')
                    for arm in arms:
                        parsed, rows = gtake(turn[arm], f'journaux/{name}/{frame}/{arm}_t{t:02}.jsonl',
                                          k,threads,passes,sites,prof)
                        by_arm[arm].append(parsed)
                        order_rows[arm].append([x for x in rows if x['phase'] == 'ordre'])
                ds = {x['empreinte'] for v in by_arm.values() for x in v}
                need(len(ds) == 1, 'information identity')
                digest = next(iter(ds))
                if frame.startswith('u'):
                    need(digest.startswith(b.UNIFORMES[sites]), 'uniform engraved gate')
                if prof:
                    need(digest == decisive[frame]['resolution_sha256'], 'W1/W48 identity')
                object_rows = {canonical([x['objet'] for x in run]) for runs in order_rows.values() for run in runs}
                need(len(object_rows) == 1, 'information object rows')
                need(all(all(run == runs[0] for run in runs) for runs in order_rows.values()),
                     'information work varies between processes of same arm')
                deltas = []
                before, after = (order_rows[a][0] for a in arms)
                for first, last in zip(before, after):
                    for key, value in first['travail'].items():
                        if value != last['travail'][key]:
                            deltas.append({'k':first['k'], 'counter':key, 'before':value, 'after':last['travail'][key]})
                information[name][frame] = {'resolution_sha256': digest,
                    'g_ms_median_process': {a:statistics.median(x['g_ns'] for x in v)/1e6 for a,v in by_arm.items()},
                    'object_rows_identical':True, 'work_same_within_each_arm':True, 'work_deltas':deltas,
                    'work_identical': len({x['travail_sha256'] for v in by_arm.values() for x in v}) == 1}
        full = report['informations']['full_k5_appareil']
        need((full['k'],full['fils'],full['passes']) == (5,48,10), 'FULL config')
        need(set(full['tours']) == set(FRAMES), 'FULL frame cohort')
        full_result = {}
        for frame, sites in FRAMES.items():
            need(len(full['tours'][frame]) == 2, 'FULL turns')
            by_arm = {a:[] for a in ('full_avant','full_apres')}
            ds = set()
            for t, turn in enumerate(full['tours'][frame]):
                need(set(turn) == set(by_arm), 'FULL arms')
                for arm in by_arm:
                    take = turn[arm]
                    path, raw = journal(take, f'journaux/full_k5/{frame}/{arm}_t{t:02}.jsonl')
                    parsed = b.lire_full(path, take['code'], 10, frame, 48, sites)
                    for key,value in parsed.items():
                        compare(value,take[key],frame+'/'+arm+'/'+key)
                    rows = [strict(x) for x in raw.splitlines()]
                    passes = [x for x in rows if x['phase'] == 'full']
                    need(all(x['pic_octets'] >= 16*sites and x['appareil_octets'] > 0 for x in passes), 'FULL capacities')
                    ds.update(x['full_sha256'] for x in passes)
                    by_arm[arm].append(passes)
                    fullcounts[0] += 1; fullcounts[1] += 10; fullcounts[2] += 9
            need(len(ds) == 1, 'FUL1 identity all arms/passes')
            full_result[frame] = {'full_sha256': next(iter(ds)), 'arms': {}}
            for arm, process in by_arm.items():
                hot = [x for rows in process for x in rows[1:]]
                medians = [statistics.median(x['wall_ns'] for x in rows[1:]) for rows in process]
                stages = {k:statistics.median(statistics.median(x['etapes_ns'][k] for x in rows[1:]) for rows in process)/1e6
                          for k in ('P','C','G','raccord','TMVR','T','M','V','R')}
                full_result[frame]['arms'][arm] = {'processes':2, 'warm_passes':18,
                    'wall_ms_median_process':statistics.median(medians)/1e6,
                    'wall_ms_max_process_median':max(medians)/1e6,
                    'wall_ms_max_warm':max(x['wall_ns'] for x in hot)/1e6,
                    'stages_ms_median_process':stages,
                    'cpu_s_median_process':statistics.median(statistics.median(x['cpu_ns'] for x in rows[1:]) for rows in process)/1e9,
                    'max_memory_budget_peak_bytes':max(x['pic_octets'] for x in hot),
                    'max_rss_bytes':max(x['rss_max_octets'] for x in hot),
                    'max_device_capacity_bytes':max(x['appareil_octets'] for x in hot),
                    'max_pinned_capacity_bytes':max(x['epinglee_octets'] for x in hot)}
        need(set(seen) == {p.relative_to(returned).as_posix() for p in (returned/'journaux').rglob('*.jsonl')}, 'extra or missing JSONL')
        inventory = sha(canonical(dict(sorted(seen.items()))).encode())
        need(inventory == capture['journal_inventory_sha256'], 'journal inventory pin')
        for name,h in seen.items():
            need(sha((returned/name).read_bytes()) == h, 'journal changed during review')
        need(sha((returned/'rapport_t2d_b.json').read_bytes()) == capture['report_sha256'], 'report changed')
        compare(b.resume_informations(report), report['resume_informations'])
        return {'source_commit': PIN, 'before_commit':'902041f6675a079deb4a642068b036635171db1f',
                'admission':'complete', 'journal_count':len(seen),
                'G':{'processes':gcounts[0], 'passes':gcounts[1], 'warm_passes':gcounts[2]},
                'FULL':{'processes':fullcounts[0], 'passes':fullcounts[1], 'warm_passes':fullcounts[2]},
                'all_process_codes':0, 'verdicts':verdicts, 'refus':[], 'aa_means':aa,
                'archived_judgement_float_deltas':float_deltas, 'decisive':decisive,
                'information_G':information, 'information_FULL':full_result,
                'scope':'G CPU on 902 plus B; FULL GPU sequential informative, shared budget; no current overlapped FULL qualification'}

def main():
    p=argparse.ArgumentParser(); p.add_argument('--repo',type=Path,required=True); p.add_argument('--returned',type=Path,required=True)
    a=p.parse_args(); capture=strict((Path(__file__).parent/'capture.json').read_bytes())
    print(json.dumps(run(a.repo,a.returned,capture),indent=2,sort_keys=True))

if __name__ == '__main__':
    main()
