#!/usr/bin/env python3
"""Portée OLS et métadonnées historiques seulement ; aucune mesure MES-C ni sonde."""
import argparse
import ast
from collections import Counter
from fractions import Fraction as F
import hashlib
import json
import math
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def exact_fit(points):
    count = len(points)
    sx, sy = sum(x for x, _ in points), sum(y for _, y in points)
    b = F(count * sum(x*y for x, y in points) - sx*sy,
          count * sum(x*x for x, _ in points) - sx*sx)
    return F(sy, count) - b * F(sx, count), b


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--manifest', type=Path, required=True)
    args = ap.parse_args()
    cap = json.loads((HERE/'capture.json').read_text())
    blobs = {}
    for name, pin in cap['sources'].items():
        raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', pin['git']+':'+name])
        need(sha(raw) == pin['sha256'], 'source changée: '+name)
        blobs[name] = raw
    raw = args.manifest.read_bytes()
    need(sha(raw) == cap['manifest_sha256'], 'manifeste différent')
    cases = json.loads(raw)['cases']
    real_names = {c['name']:c['count'] for c in cases if c.get('subfamily') in
                  ('boule_knn', 'bout_v11', 'objet_reel', 'objet_reel_contexte')}
    groups = Counter('reel' if c['name'] in real_names else c['provenance']['family'] for c in cases)
    need(groups == dict(reel=132, uniform=5, clusters8=5, slab=5, lattice=5, line=2, sphere=5), 'cohorte')
    source = ast.parse(blobs['morsehgp3D_v12/microbancs/mes_c_petits/pilote_c.py'])
    fit_node = next(n for n in source.body if isinstance(n, ast.FunctionDef) and n.name == 'fit')
    namespace = {'statistics':statistics}
    exec(compile(ast.Module(body=[fit_node], type_ignores=[]), 'fit_epingle', 'exec'), namespace)
    limit = F(241300000, 64740)
    examples = [
        ('affine', [(n, 1000000 + 3000*n) for n in (100, 1000, 10000)]),
        ('residu', [(100,200000), (1000,7100000), (10000,31700000)]),
        ('quadratique', [(n,n*n) for n in (100,200,300)])]
    witnesses = []
    for name, points in examples:
        a, b = exact_fit(points)
        approx = namespace['fit'](points)
        need(all(math.isclose(float(exact), seen, rel_tol=1e-12, abs_tol=1e-7)
                 for exact, seen in zip((a,b), approx)), 'OLS produit/oracle')
        need(a <= 2000000 and b <= limit, 'C1/C2 du témoin')
        residuals = [F(y)-a-b*n for n,y in points]
        need(sum(residuals) == sum(n*r for (n,_),r in zip(points,residuals)) == 0, 'orthogonalité')
        witnesses.append(dict(nom=name, points_ns=points, a_ns=str(a), b_ns_par_site=str(b),
                              residus_ns=list(map(str,residuals)), max_ns_par_site=str(max(F(y,n) for n,y in points))))
    need(max(F(y,n) for n,y in examples[0][1]) > limit, 'affine dépasse le ratio')
    need(F(7100000) > 2000000 + limit*1000, 'point dépasse même enveloppe affine cible')
    need(exact_fit(examples[2][1])[0] < 0, 'interception négative')
    k = json.loads(blobs['morsehgp3D_v12/receipts/audit_reponses_20261008/session_k_full/mesures.json'])
    rows = k['mesures']['v12set_k5_appareil']
    n, t = statistics.median(r[1] for r in rows), statistics.median(r[3] for r in rows)
    need(len(rows)==37 and n==64740 and t==241309401, 'référence K')
    median_t = next(r[0] for r in rows if r[3]==t)
    median_n = next(r[0] for r in rows if r[1]==n)
    need(median_t != median_n, 'deux trames distinctes')
    history = {}
    for label, path in cap['mes_p'].items():
        data = json.loads(blobs[path])
        takes = [r for r in data['prises'] if r['k']==5 and r['fils']==48 and r['code']==0
                 and r['chaud'] is not None and r['nuage'] in real_names]
        need(all(real_names[r['nuage']]==r['sites'] for r in takes), 'comptes historiques')
        history[label] = len(takes)
    need(history == dict(G=132,H=123), 'cohortes MES-P')
    # Indexation abstraite des trois tours, pas des durées réelles.
    healthy = sum(groups[x] for x in ('reel','uniform','clusters8','slab'))
    sequence = list(range(3*healthy))
    need(all(sequence[i::healthy] == [i,i+healthy,i+2*healthy] for i in range(healthy)), 'tours')
    result = dict(exemples_algebriques_pas_chronos=witnesses, groupes=dict(groups),
        tours=3, nuages_session=healthy, passes_session=3*healthy, chaudes_par_nuage=2,
        reference_K=dict(trames=37, mediane_sites=n, mediane_temps_ns=t,
                        seuil_fige_ns_par_site=str(limit), rapport_medianes_ns_par_site=str(F(t,n)),
                        mediane_rapports_ns_par_site=str(statistics.median(F(r[3],r[1]) for r in rows)),
                        max_rapport_ns_par_site=str(max(F(r[3],r[1]) for r in rows)),
                        medianes_sur_meme_trame=False),
        cohortes_reelles_MES_P_K5_W48=history, mesures_MES_C_lues=False, moteur_execute=False)
    need(args.manifest.read_bytes() == raw, 'manifeste modifié pendant lecture')
    print(json.dumps(result, indent=2, sort_keys=True, ensure_ascii=False))


if __name__ == '__main__':
    main()
