"""Audit L10 : rejeu des seules methodes sklearn du lot C sur les 8 scenes de l'echantillon, ecarts au recu G4."""
import csv, json, os, sys
sys.path.insert(0, 'lotC_src')
import run_test, methods, metrics, scenes
prereg = json.load(open('lotC_src/prereg/PREREG_V10_COVER_C_20260929.json'))
specs, _ = run_test.plan_manifest(prereg)
ref = {(r['unit'], r['method']): r for r in csv.DictReader(open('lotC/results.csv'))}
order = {'hard': 0, 'extreme': 1, 'medium': 2, 'easy': 3}
chosen = []
for fam in prereg['plan']['families']:
    cand = [s for s in specs if s['family'] == fam and s['n'] == 8000]
    cand.sort(key=lambda s: (order[s['level']], s['noise_fraction'], s['seed']))
    chosen.append(cand[0])
out = []
for spec in chosen:
    P, L, _ = scenes.generate(spec)
    G, T, dups, _ = scenes.quantize18(P, L)
    n = len(G); cache = {}
    for m in prereg['methods']:
        if m['kind'] != 'sklearn':
            continue
        raw = run_test.run_method(m, G, n, 3.0, None, cache)
        lab = run_test.apply_fill(G, raw, m['fill'], m.get('k', m.get('min_samples', 5)))
        s = metrics.scores(T, lab)
        want = ref[(run_test.unit_name(spec), m['name'])]
        d = round(s['ari_s'], 6) - float(want['ari_s'])
        out.append((abs(d), run_test.unit_name(spec), m['name'], round(s['ari_s'], 6), float(want['ari_s']), s['clusters'], int(want['clusters'])))
out.sort(reverse=True)
print('lignes sklearn rejouees', len(out), '; differentes', sum(1 for o in out if o[0] > 0))
print('|ecart| unite methode ARI_s local ARI_s G4 amas local amas G4')
for o in out[:14]:
    print('%.6f %-52s %-12s %.6f %.6f %d %d' % o)
import statistics
print('ecart absolu moyen %.6f ; mediane %.6f' % (statistics.mean(o[0] for o in out), statistics.median(o[0] for o in out)))
