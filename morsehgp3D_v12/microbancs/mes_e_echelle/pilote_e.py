#!/usr/bin/env python3
"""MES-E : la v11 gelee sur des decoupes de scenes LiDAR reelles de 1 a 8 millions de sites (PLAN.md de la v12, T0).

Mesure publiee, sans regle d'adoption : volumes par site (boules, incidences), pic memoire, temps, point de rupture,
pour fixer les objectifs du regime (b) et la taille des lots de la v12. Chaque cas est joue par la sonde FULL de la v11
gelee (bench/full_probe.cpp, cible mhgp11_full_bench, voie CPU de reference, masque 802811), dans un processus neuf,
vidage FULL ecrit vers /dev/null (il n'est pas l'objet de la mesure), feuilles de 16 a K <= 5 et de 24 au-dela
(MESURE.md, paragraphe 4).

Usage : pilote_e.py --v11-build DIR --donnees DIR --sortie DIR --cas NOM:K[,NOM:K...] [--fils 48] [--budget-gio 160]
                    [--delai 1800] [--jobs 44]
  NOM designe <donnees>/NOM.u32le et <donnees>/NOM.ids.u32le (sites distincts, decoupes de bench/data).
Sorties : <sortie>/mes_e.json (une entree par cas : code, raison, secondes, pic RSS, comptes de la sonde) et
<sortie>/mes_e.md ; les lignes brutes de la sonde sous <sortie>/<NOM>_k<K>.jsonl.
Codes : 0 toutes les prises rendues (un refus ou une expiration de la sonde est un resultat publie, pas une erreur
du pilote) ; 2 usage ou entree absente ; 3 construction impossible. Bibliotheque standard seule (Python 3.10 nu).
"""
import argparse
import json
import os
import signal
import subprocess
import sys
import threading
import time

MASK = '802811'
KEYS = ('sites', 'points', 'catalogue_balls', 'catalogue_incidences', 'domain_ns', 'forest_ns', 'cloud_ns', 'read_ns',
        'peak_reserved_bytes', 'cloud_peak_bytes', 'status', 'reason')


def build(v11_build, jobs):
    target = os.path.join(v11_build, 'mhgp11_full_bench')
    done = subprocess.run(['cmake', '--build', v11_build, '-j', str(jobs), '--target', 'mhgp11_full_bench'],
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if done.returncode != 0 or not os.path.isfile(target):
        sys.stderr.write(done.stdout.decode('utf-8', 'replace')[-4000:])
        return None
    return target


def flatten(line, out):
    """Recopie dans out les cles numeriques ou textuelles de KEYS, ou qu'elles soient dans la ligne JSON."""
    if isinstance(line, dict):
        for key, value in line.items():
            if key in KEYS and isinstance(value, (int, float, str)):
                out[key] = value
            elif isinstance(value, (dict, list)):
                flatten(value, out)
    elif isinstance(line, list):
        for value in line:
            flatten(value, out)


def run_case(binary, data, out_dir, name, k, threads, budget, delay):
    leaf = 16 if k <= 5 else 24
    xyz, ids = os.path.join(data, name + '.u32le'), os.path.join(data, name + '.ids.u32le')
    if not os.path.isfile(xyz) or not os.path.isfile(ids):
        return dict(cas=name, k=k, code=None, raison='entree absente')
    argv = [binary, xyz, ids, '/dev/null', str(k), str(leaf), '256', '0', '4294967295', str(budget), str(threads), MASK]
    log = os.path.join(out_dir, '%s_k%d.jsonl' % (name, k))
    err = os.path.join(out_dir, '%s_k%d.err' % (name, k))
    start = time.monotonic()
    with open(log, 'wb') as out, open(err, 'wb') as errors:
        child = subprocess.Popen(argv, stdout=out, stderr=errors, start_new_session=True)
        expired = []

        def expire():
            expired.append(True)
            try:
                os.killpg(child.pid, signal.SIGKILL)
            except OSError:
                pass
        timer = threading.Timer(delay, expire)
        timer.start()
        _pid, status, usage = os.wait4(child.pid, 0)
        timer.cancel()
    seconds = time.monotonic() - start
    code = os.waitstatus_to_exitcode(status)
    entry = dict(cas=name, k=k, feuille=leaf, fils=threads, code=code, secondes=round(seconds, 3),
                 pic_rss_octets=usage.ru_maxrss * 1024, expire=bool(expired))
    fields = {}
    with open(log, encoding='utf-8', errors='replace') as handle:
        for raw in handle:
            raw = raw.strip()
            if raw.startswith('{'):
                try:
                    flatten(json.loads(raw), fields)
                except ValueError:
                    entry['ligne_illisible'] = True
    entry['sonde'] = fields
    sites = fields.get('sites')
    if isinstance(sites, int) and sites > 0:
        for key in ('catalogue_balls', 'catalogue_incidences'):
            if isinstance(fields.get(key), int):
                entry[key + '_par_site'] = round(fields[key] / sites, 3)
        entry['octets_rss_par_site'] = round(entry['pic_rss_octets'] / sites, 1)
        entry['microsecondes_par_site'] = round(seconds * 1e6 / sites, 3)
    return entry


def table(entries):
    lines = ['| cas | K | code | secondes | pic RSS (Gio) | boules / site | incidences / site | octets RSS / site |'
             ' µs / site |', '| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |']
    for e in entries:
        lines.append('| %s | %d | %s | %s | %s | %s | %s | %s | %s |' % (
            e['cas'], e['k'], e.get('code'), e.get('secondes', '-'),
            '%.2f' % (e['pic_rss_octets'] / 2 ** 30) if 'pic_rss_octets' in e else '-',
            e.get('catalogue_balls_par_site', '-'), e.get('catalogue_incidences_par_site', '-'),
            e.get('octets_rss_par_site', '-'), e.get('microsecondes_par_site', '-')))
    return '\n'.join(lines) + '\n'


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--v11-build', required=True)
    parser.add_argument('--donnees', required=True)
    parser.add_argument('--sortie', required=True)
    parser.add_argument('--cas', required=True)
    parser.add_argument('--fils', type=int, default=48)
    parser.add_argument('--budget-gio', type=int, default=160)
    parser.add_argument('--delai', type=int, default=1800)
    parser.add_argument('--jobs', type=int, default=44)
    try:
        args = parser.parse_args(argv[1:])
        cases = []
        for item in args.cas.split(','):
            name, k = item.rsplit(':', 1)
            cases.append((name, int(k)))
    except (SystemExit, ValueError):
        return 2
    if not cases or any(not 1 <= k <= 12 for _name, k in cases) or args.fils < 1 or args.budget_gio < 1:
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    binary = build(args.v11_build, args.jobs)
    if binary is None:
        return 3
    entries = []
    for name, k in cases:
        entry = run_case(binary, args.donnees, args.sortie, name, k, args.fils, args.budget_gio << 30, args.delai)
        entries.append(entry)
        print(json.dumps(entry, sort_keys=True), flush=True)
        with open(os.path.join(args.sortie, 'mes_e.json'), 'w', encoding='utf-8') as out:
            json.dump(dict(mesure='MES-E', masque=MASK, budget_gio=args.budget_gio, prises=entries), out, indent=1,
                      sort_keys=True)
        with open(os.path.join(args.sortie, 'mes_e.md'), 'w', encoding='utf-8') as out:
            out.write('# MES-E : v11 gelee sur des decoupes LiDAR reelles\n\n' + table(entries))
    absent = [e['cas'] for e in entries if e.get('raison') == 'entree absente']
    if absent:
        sys.stderr.write('entrees absentes : %s\n' % ', '.join(absent))
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
