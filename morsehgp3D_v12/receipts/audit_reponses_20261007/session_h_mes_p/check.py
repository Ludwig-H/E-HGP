#!/usr/bin/env python3
"""Relire les agrégats MES-P publiés ; aucun moteur, donnée XYZ ou service externe."""
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import statistics
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[3]
DATA = ROOT / 'receipts/g4_t2h_20261007/resultats/cmd/013_mes_p_fils_1_4/files/mes_p/mes_p.json'
READER = ROOT / 'microbancs/mes_p_petits/analyse_p.py'
PILOT = ROOT / 'microbancs/mes_p_petits/pilote_p.py'


def need(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    raw = DATA.read_bytes()
    data = json.loads(raw)
    takes = data['prises']
    keys = [(t['nuage'], t['k'], t['fils']) for t in takes]
    need(len(keys) == len(set(keys)), 'prise répétée')
    need(data['passes'] == 4 and data['masque'] == '802811', 'régime inattendu')
    good = [t for t in takes if t['code'] == 0]
    bad = [t for t in takes if t['code'] != 0]
    for t in good:
        need(len(t['passes']) == 4, 'passes incomplètes')
        need(type(t['sites']) is int and t['sites'] > 0, 'sites invalides')
        need(math.isfinite(t['chaud']) and t['chaud'] > 0, 'durée invalide')
        # La liste publiée est arrondie à six décimales ; chaud garde les ns d'origine.
        need(abs(t['chaud'] - statistics.median(t['passes'][1:])) <= 0.000000501,
             'médiane incohérente au-delà de l’arrondi publié')
    need(all(t['chaud'] is None for t in bad), 'échec doté d’un chrono chaud')
    spec = importlib.util.spec_from_file_location('mes_p_analysis', READER)
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    cohorts, rows = {}, []
    for k in (5, 10):
        common = set.intersection(*[
            {t['nuage'] for t in reader.real_rows(good, k, f)} for f in (1, 4, 48)])
        cohorts[str(k)] = len(common)
        for f in (1, 4, 48):
            real = reader.real_rows(good, k, f)
            need({t['nuage'] for t in real} == common, 'cohorte différente selon les fils')
            bins = []
            for low, high in reader.BINS:
                selected = [t for t in real if low <= t['sites'] < high]
                bins.append(dict(lo=low, hi_exclusive=high, takes=len(selected),
                                 median_sites=statistics.median(t['sites'] for t in selected),
                                 median_ms=statistics.median(t['chaud'] * 1000 for t in selected),
                                 median_us_per_site=statistics.median(
                                     t['chaud'] * 1000000 / t['sites'] for t in selected)))
            fit = reader.fit([(t['sites'], t['chaud']) for t in real])
            rows.append(dict(k=k, threads=f, bins=bins,
                             fitted_intercept_ms=fit[0] * 1000,
                             fitted_slope_us=fit[1] * 1000000))
    outputs = []
    for flags in ([], ['-O']):
        p = subprocess.run([sys.executable, '-B', '-S', *flags, str(READER), str(DATA)],
                           capture_output=True, check=False)
        need(p.returncode == 0 and not p.stderr, 'échec du lecteur officiel')
        outputs.append(p.stdout)
    need(outputs[0] == outputs[1], 'lecteur normal/-O différent')
    # Contre-modèle algébrique, EN MICROSECONDES INVENTÉES, pas un ajustement aux prises.
    # Tout le terme fixe du régime 48 fils est ici attribué à l’ordonnancement du pool.
    model = [dict(sites=n, serial_us=100*n, parallel_us=6000+4*n,
                  scheduling_fixed_us=6000) for n in (100, 300, 1000, 3000, 10000)]
    need(all(x['serial_us'] > x['parallel_us'] for x in model), 'contre-modèle invalide')
    result = dict(source_commit='136e07628', takes=len(takes), good=len(good),
                  cases=len({t['nuage'] for t in takes}), cohorts=cohorts,
                  excluded=data['exclus'], failed=[{k: t[k] for k in ('nuage', 'k', 'fils', 'code', 'chaud')}
                                                  for t in bad],
                  rows=rows, official_reader_stdout_sha256=hashlib.sha256(outputs[0]).hexdigest(),
                  pins={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                        for p in (DATA, READER, PILOT)},
                  hypothetical_not_measured=model)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
