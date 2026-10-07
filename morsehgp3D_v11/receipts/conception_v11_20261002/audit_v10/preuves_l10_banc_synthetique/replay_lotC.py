"""Audit L10 : rejeu d'un echantillon du lot C avec les scripts et binaires EPINGLES (verification du recu, pas de mesure
nouvelle). Compare chaque ligne rejouee a la ligne du recu results.csv.
Usage : python3 replay_lotC.py <dossier scripts epingles> <dossier binaires epingles> <results.csv du recu> <n> <par famille> <jobs>
"""
import csv, json, os, sys, time
from concurrent.futures import ProcessPoolExecutor
src, build, receipt, size, per_family, jobs = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6])
sys.path.insert(0, src)
import run_test  # noqa: E402

def work(spec, prereg, build):
    t0 = time.time()
    rows = run_test.run_unit(spec, prereg, build)
    return rows, time.time() - t0

def main():
    prereg = json.load(open(os.path.join(src, 'prereg', 'PREREG_V10_COVER_C_20260929.json')))
    errors = run_test.check_pins(prereg, build)
    print('check_pins :', errors if errors else 'toutes les epingles conformes (binaires, 6 scripts, versions, manifeste)')
    if errors:
        return 2
    specs, digest = run_test.plan_manifest(prereg)
    ref = {(r['unit'], r['method']): r for r in csv.DictReader(open(receipt))}
    # echantillon deterministe : par famille, les `per_family` premieres scenes de taille `size` dans l'ordre du plan,
    # en alternant les niveaux (hard, extreme, medium, easy) et le bruit
    order = {'hard': 0, 'extreme': 1, 'medium': 2, 'easy': 3}
    chosen = []
    for fam in prereg['plan']['families']:
        cand = [s for s in specs if s['family'] == fam and s['n'] == size]
        cand.sort(key=lambda s: (order[s['level']], s['noise_fraction'], s['seed']))
        seen, pick = set(), []
        for s in cand:
            key = (s['level'], s['noise_fraction'])
            if key in seen:
                continue
            seen.add(key); pick.append(s)
            if len(pick) == per_family:
                break
        chosen += pick
    print('scenes rejouees :', len(chosen), 'taille', size)
    keys = ('ari_s', 'ari_nc', 'ami_nc', 'coverage', 'clusters', 'refused', 'points', 'duplicates', 'zhat')
    stats = {}
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        futs = [pool.submit(work, s, prereg, build) for s in chosen]
        for s, fu in zip(chosen, futs):
            rows, dt = fu.result()
            for r in rows:
                kind = 'tour' if r['method'].startswith(('tw_', 'cap_')) else ('mreach' if r['method'].startswith('mrb') else 'sklearn')
                want = ref[(r['unit'], r['method'])]
                got = {c: str(r.get(c, '')) for c in keys}
                same = all(got[c] == want[c] for c in keys)
                st = stats.setdefault(kind, [0, 0, 0.0])
                st[0] += 1; st[1] += 0 if same else 1
                st[2] = max(st[2], abs(float(got['ari_s']) - float(want['ari_s'])))
                if not same and kind != 'sklearn':
                    print('ECART', r['unit'], r['method'], {c: (got[c], want[c]) for c in keys if got[c] != want[c]})
            print('%-60s %.0f s' % (run_test.unit_name(s), dt), flush=True)
    for kind, (n, d, m) in stats.items():
        print('%-8s lignes %d, differentes du recu %d, ecart max ARI_s %.6f' % (kind, n, d, m))
    return 0

if __name__ == '__main__':
    sys.exit(main())
