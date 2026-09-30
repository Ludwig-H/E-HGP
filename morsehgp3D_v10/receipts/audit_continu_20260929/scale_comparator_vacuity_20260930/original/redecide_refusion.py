"""Differentiels sur entrees valides archivees (verificateur) : re-decision des lots A et C par l'ancien et le nouveau
decide.py (decision complete, numpy) et re-fusion des sessions c1/c2 du lot C par l'ancien et le nouveau
merge_sessions.py (le nouveau aussi sous python3 -S).

  python3 redecide_refusion.py <racine base> <racine corrigee> <dossier de travail>
"""
import csv
import gzip
import hashlib
import json
import os
import shutil
import subprocess
import sys


def sha(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


def lot_dir(receipts, name, results, dest):
    os.makedirs(dest)
    shutil.copy(os.path.join(receipts, name, 'run.json'), dest)
    opener = gzip.open if results.endswith('.gz') else open
    with opener(os.path.join(receipts, name, results), 'rb') as fin, \
            open(os.path.join(dest, 'results.csv'), 'wb') as fout:
        fout.write(fin.read())


def run_decide(root, prereg, lot, extra=()):
    r = subprocess.run([sys.executable, '-B'] + list(extra) +
                       [os.path.join(root, 'morsehgp3D_v10/bench/synthetic/decide.py'), '--prereg', prereg,
                        '--run', lot], capture_output=True, text=True, timeout=3600)
    dj, md = os.path.join(lot, 'DECISION.json'), os.path.join(lot, 'DECISION.md')
    return dict(code=r.returncode, json=sha(dj) if os.path.exists(dj) else None,
                md=sha(md) if os.path.exists(md) else None, stdout_sha=hashlib.sha256(r.stdout.encode()).hexdigest(),
                err=r.stderr[-300:])


def main():
    base, new, work = sys.argv[1:4]
    if os.path.exists(work):
        shutil.rmtree(work)
    os.makedirs(work)
    receipts = os.path.join(new, 'morsehgp3D_v10/receipts')
    prereg_dir = os.path.join(new, 'morsehgp3D_v10/bench/synthetic/prereg')
    out = {}
    for label, name, results, prereg in (
            ('A', 'test_kmatch_A_20260929', 'results.csv', 'PREREG_V10_KMATCH_A_20260928.json'),
            ('C', 'test_cover_C_20260929', 'results.csv.gz', 'PREREG_V10_COVER_C_20260929.json')):
        ppath = os.path.join(prereg_dir, prereg)
        arch_json = sha(os.path.join(receipts, name, 'DECISION.json'))
        arch_md = sha(os.path.join(receipts, name, 'DECISION.md'))
        rec = dict(archive_json=arch_json, archive_md=arch_md)
        for who, root in (('ancien', base), ('nouveau', new)):
            d = os.path.join(work, 'lot_%s_%s' % (label, who))
            lot_dir(receipts, name, results, d)
            rec[who] = run_decide(root, ppath, d)
        d = os.path.join(work, 'lot_%s_nouveau_check_S' % label)
        lot_dir(receipts, name, results, d)
        r = subprocess.run([sys.executable, '-S', '-B', os.path.join(new, 'morsehgp3D_v10/bench/synthetic/decide.py'),
                            '--prereg', ppath, '--run', d, '--check-only'], capture_output=True, text=True)
        rec['nouveau_check_only_S'] = dict(code=r.returncode, stdout=r.stdout.strip())
        rec['ancien_egal_nouveau'] = (rec['ancien']['json'], rec['ancien']['md'], rec['ancien']['stdout_sha']) == (
            rec['nouveau']['json'], rec['nouveau']['md'], rec['nouveau']['stdout_sha'])
        rec['nouveau_egal_archive'] = (rec['nouveau']['json'], rec['nouveau']['md']) == (arch_json, arch_md)
        out['decide_' + label] = rec
        print(json.dumps({('decide_' + label): rec}), flush=True)

    # re-fusion c1/c2 du lot C
    name = 'test_cover_C_20260929'
    with open(os.path.join(receipts, name, 'run.json')) as f:
        merged_info = json.load(f)
    with gzip.open(os.path.join(receipts, name, 'results.csv.gz'), 'rt', newline='') as f:
        reader = csv.DictReader(f)
        fields = reader.fieldnames
        rows = list(reader)
    order = []
    for r in rows:
        if not order or order[-1] != r['unit']:
            order.append(r['unit'])
    first = merged_info['segments'][0]
    n1 = first.get('computed', None)
    counts = [s.get('computed') for s in merged_info['segments']]
    units1 = set(order[:338])
    sess = []
    for i, (seg, keep) in enumerate(((merged_info['segments'][0], lambda u: u in units1),
                                     (merged_info['segments'][1], lambda u: u not in units1))):
        d = os.path.join(work, 'sessions', 'c%d' % (i + 1))
        os.makedirs(d)
        with open(os.path.join(d, 'run.json'), 'w') as f:
            json.dump(seg, f, indent=1, sort_keys=True)
        with open(os.path.join(d, 'results.csv'), 'w', newline='') as f:
            w = csv.DictWriter(f, fieldnames=fields)
            w.writeheader()
            w.writerows([r for r in rows if keep(r['unit'])])
        sess.append(d)
    ppath = os.path.join(prereg_dir, 'PREREG_V10_COVER_C_20260929.json')
    fusion = dict(segment_computed=counts, units_contigus=len(order) == len(set(order)))
    for who, root, extra in (('ancien', base, []), ('nouveau', new, []), ('nouveau_S', new, ['-S'])):
        d = os.path.join(work, 'fusion_' + who)
        r = subprocess.run([sys.executable, '-B'] + extra + [os.path.join(root, 'morsehgp3D_v10/bench/g4/merge_sessions.py'),
                                                             '--prereg', ppath, '--out', d] + sess,
                           capture_output=True, text=True)
        fusion[who] = dict(code=r.returncode, stdout=r.stdout.strip(),
                           results=sha(os.path.join(d, 'results.csv')) if os.path.exists(os.path.join(d, 'results.csv')) else None,
                           run=sha(os.path.join(d, 'run.json')) if os.path.exists(os.path.join(d, 'run.json')) else None,
                           done=sha(os.path.join(d, 'done.u32le')) if os.path.exists(os.path.join(d, 'done.u32le')) else None)
    arch_results = hashlib.sha256(gzip.open(os.path.join(receipts, name, 'results.csv.gz'), 'rb').read()).hexdigest()
    fusion['archive'] = dict(results=arch_results, run=sha(os.path.join(receipts, name, 'run.json')))
    fusion['tous_egaux'] = (len({(fusion[w]['results'], fusion[w]['run'], fusion[w]['done'])
                                 for w in ('ancien', 'nouveau', 'nouveau_S')}) == 1 and
                            fusion['nouveau']['results'] == arch_results and
                            fusion['nouveau']['run'] == fusion['archive']['run'])
    out['fusion_C'] = fusion
    print(json.dumps({'fusion_C': fusion}), flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
