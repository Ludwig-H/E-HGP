#!/usr/bin/env python3
"""MES-P : la v11 gelee sur les petits nuages (100 a 10 000 sites), a chaud : cout fixe et cout par site (PLAN.md, T0).

Mesure publiee, sans regle d'adoption : elle fixe le seuil entre voie CPU et voie GPU des petits nuages de la v12. Chaque
nuage du paquet `g4_small` (manifeste `bundle_manifest.json`, sites distincts) est joue par la sonde FULL de la v11
gelee (cible mhgp11_full_bench, voie CPU 802811) dans un processus neuf, avec P passes FULL successives dans le meme
processus (regime chaud : la premiere passe paie les couts froids), vidage vers /dev/null. Pour chaque nuage, chaque
nombre de fils et chaque K : temps de chaque passe (cle `pass`, champs de duree de la sonde), mediane des passes 2..P.
Puis un ajustement par moindres carres, par K et par nombre de fils, du temps chaud t = a + b n : a est le cout fixe,
b le cout par site.

Usage : pilote_p.py --v11-build DIR (--donnees DIR | --archive TAR --deballage DIR) --sortie DIR [--fils 1,48]
                    [--k 5,10] [--passes 6] [--delai 300] [--jobs 44] [--limite N] [--exclure PREFIXE,...]
  --exclure : nuages dont le nom commence par l'un des prefixes ecartes (par exemple synth_lattice,synth_sphere : la
  v11 y passe des dizaines de secondes par passe, session G du 7 octobre) ; les ecartes sont listes dans mes_p.json.
  --archive : le paquet g4_small en une seule archive tar (le televersement d'une session paie chaque fichier : 320
  petits fichiers depassent son delai) ; deballee dans --deballage apres controle de chaque membre (fichier simple, nom
  simple, aucun chemin), puis lue comme --donnees.
Sorties : <sortie>/mes_p.json, <sortie>/mes_p.md, lignes brutes sous <sortie>/brut/. Codes : 0 rendu ; 2 usage,
manifeste illisible ou selection vide ; 3 construction impossible. Une prise expiree (groupe de processus tue au
delai) ou en echec n'a aucune valeur chaude. Bibliotheque standard seule (Python 3.10 nu).
"""
import argparse
import json
import os
import signal
import statistics
import subprocess
import sys
import tarfile
import time

MASK = '802811'


def unpack(archive, folder):
    """Deballe une archive tar plate (fichiers simples a nom simple) ; rend le dossier, ou None si un membre est refuse."""
    os.makedirs(folder, exist_ok=True)
    with tarfile.open(archive) as tar:
        members = tar.getmembers()
        for member in members:
            name = member.name
            if not member.isfile() or '/' in name or name in ('', '.', '..') or name.startswith('.'):
                return None
        for member in members:
            source = tar.extractfile(member)
            with open(os.path.join(folder, member.name), 'wb') as out:
                out.write(source.read())
    return folder


def build(v11_build, jobs):
    target = os.path.join(v11_build, 'mhgp11_full_bench')
    done = subprocess.run(['cmake', '--build', v11_build, '-j', str(jobs), '--target', 'mhgp11_full_bench'],
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return target if done.returncode == 0 and os.path.isfile(target) else None


def uint(value):
    return type(value) is int and 0 <= value < 1 << 64


def pass_seconds(line):
    """Diagnostic d'une passe : seul wall_ns mesure FULL dans la sonde gelee, aucun repli partiel."""
    if type(line) is dict and line.get('phase') == 'pass' and uint(line.get('wall_ns')):
        return line['wall_ns'] / 1e9
    return None


def admitted(rows, k, threads, passes):
    """Admission du protocole full_probe.cpp ac081a06f, pas oracle geometrique."""
    if not all(type(v) is int for v in (k, threads, passes)) or not 1 <= k <= 12 or \
            threads < 1 or not 2 <= passes <= 64 or len(rows) != passes + 4:
        return False
    # Le domaine detaille precede la derniere passe ; full puis exit ferment le flux.
    phases = ['cloud'] + ['pass'] * (passes - 1) + ['domain', 'pass', 'full', 'exit']
    if not all(type(row) is dict for row in rows) or [r.get('phase') for r in rows] != phases:
        return False
    cloud, domain, full, end = rows[0], rows[-4], rows[-2], rows[-1]
    if not uint(cloud.get('sites')) or not 1 <= cloud['sites'] <= 0xFFFFFFFF or \
            end != {'phase': 'exit', 'status': 'ok', 'reason': 'none'} or \
            full.get('status') != 'ok' or full.get('reason') != 'none':
        return False
    if not all(uint(full.get(key)) for key in ('coord_bits', 'kmax', 'workers', 'optimizations')) or \
            (full['coord_bits'], full['kmax'], full['workers'], full['optimizations']) != (21, k, threads, int(MASK)) or \
            not uint(domain.get('leaf_size')) or domain['leaf_size'] != (16 if k <= 5 else 24):
        return False
    stages = rows[1:passes] + [rows[-3]]
    times = ('wall_ns', 'index_ns', 'domain_ns', 'forest_ns')
    for number, row in enumerate(stages, 1):
        if not uint(row.get('pass')) or row['pass'] != number or row.get('status') != 'ok' or \
                not all(uint(row.get(key)) for key in times) or \
                row['wall_ns'] < row['index_ns'] + row['domain_ns'] + row['forest_ns']:
            return False
    if not all(uint(full.get(key)) and full[key] == stages[-1][key] for key in times) or \
            not all(uint(domain.get(key)) and domain[key] == stages[-1][key] for key in ('index_ns', 'domain_ns')):
        return False
    return True


def run_cloud(binary, data, raw_dir, name, k, threads, passes, delay):
    leaf = 16 if k <= 5 else 24
    argv = [binary, os.path.join(data, name + '.u32le'), os.path.join(data, name + '.ids.u32le'), '/dev/null',
            str(k), str(leaf), '256', '0', '4294967295', str(8 << 30), str(threads), MASK, str(passes)]
    start = time.monotonic()
    # Groupe de processus neuf : le delai tue tout le groupe, descendants d'un futur lanceur compris.
    proc = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        out, _err = proc.communicate(timeout=delay)
        code = proc.returncode
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        out, _err = proc.communicate()
        code = 'expire'
    seconds = time.monotonic() - start
    with open(os.path.join(raw_dir, '%s_k%d_f%d.jsonl' % (name, k, threads)), 'wb') as handle:
        handle.write(out)
    def unique_object(pairs):
        row = {}
        for key, value in pairs:
            if key in row:
                raise ValueError('cle JSON repetee')
            row[key] = value
        return row

    def bad_constant(_value):
        raise ValueError('constante JSON non finie')

    try:
        rows = [json.loads(raw, object_pairs_hook=unique_object, parse_constant=bad_constant)
                for raw in out.decode('utf-8').splitlines()]
        if not all(type(row) is dict for row in rows):
            raise ValueError('ligne non objet')
    except (ValueError, UnicodeError):
        rows = []  # brut conserve, aucune ligne invalide ignoree
    per_pass = [v for row in rows if (v := pass_seconds(row)) is not None]
    sites = rows[0].get('sites') if rows and rows[0].get('phase') == 'cloud' else None
    if not uint(sites) or sites == 0:
        sites = None
    okay = type(code) is int and code == 0 and admitted(rows, k, threads, passes)
    # Les passes partielles restent diagnostiques ; aucune valeur chaude ne vient d'un flux incomplet ou refuse.
    warm = statistics.median(per_pass[1:]) if okay else None
    return dict(nuage=name, k=k, fils=threads, code=code, sites=sites, processus_secondes=round(seconds, 4),
                passes=[round(v, 6) for v in per_pass], froid=per_pass[0] if per_pass else None, chaud=warm,
                admission='conforme' if okay else 'processus_ou_protocole_invalide')


def fit(points):
    """Moindres carres t = a + b n sur des couples (n, t) ; None s'il y a moins de deux tailles distinctes."""
    if len(set(n for n, _t in points)) < 2:
        return None
    mean_n = sum(n for n, _t in points) / len(points)
    mean_t = sum(t for _n, t in points) / len(points)
    var = sum((n - mean_n) ** 2 for n, _t in points)
    b = sum((n - mean_n) * (t - mean_t) for n, t in points) / var
    return dict(fixe_ms=round((mean_t - b * mean_n) * 1e3, 4), par_site_us=round(b * 1e6, 4), points=len(points))


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--v11-build', required=True)
    parser.add_argument('--donnees')
    parser.add_argument('--archive')
    parser.add_argument('--deballage')
    parser.add_argument('--sortie', required=True)
    parser.add_argument('--fils', default='1,48')
    parser.add_argument('--k', default='5,10')
    parser.add_argument('--passes', type=int, default=6)
    parser.add_argument('--delai', type=int, default=300)
    parser.add_argument('--jobs', type=int, default=44)
    parser.add_argument('--limite', type=int, default=0)
    parser.add_argument('--exclure', default='')
    try:
        args = parser.parse_args(argv[1:])
        if (args.donnees is None) == (args.archive is None) or (args.archive is not None and args.deballage is None):
            return 2
        if args.archive is not None:
            args.donnees = unpack(args.archive, args.deballage)
            if args.donnees is None:
                return 2
        threads = [int(x) for x in args.fils.split(',')]
        orders = [int(x) for x in args.k.split(',')]
        manifest = json.load(open(os.path.join(args.donnees, 'bundle_manifest.json'), encoding='utf-8'))
        names = [case['name'] for case in manifest['cases']]
    except (SystemExit, ValueError, OSError, KeyError, TypeError):
        return 2
    if args.passes < 2 or not threads or not orders or any(t < 1 for t in threads) or any(not 1 <= k <= 12 for k in orders):
        return 2
    prefixes = [x for x in args.exclure.split(',') if x]
    excluded = [name for name in names if any(name.startswith(x) for x in prefixes)]
    names = [name for name in names if name not in excluded]
    if args.limite > 0:
        names = names[:args.limite]
    if not names:
        print('pilote_p : selection vide (%d nuages exclus)' % len(excluded), file=sys.stderr)
        return 2
    raw_dir = os.path.join(args.sortie, 'brut')
    os.makedirs(raw_dir, exist_ok=True)
    binary = build(args.v11_build, args.jobs)
    if binary is None:
        return 3
    entries = []
    for name in names:
        for k in orders:
            for t in threads:
                entries.append(run_cloud(binary, args.donnees, raw_dir, name, k, t, args.passes, args.delai))
        with open(os.path.join(args.sortie, 'mes_p.json'), 'w', encoding='utf-8') as out:
            json.dump(dict(mesure='MES-P', masque=MASK, passes=args.passes, prises=entries, exclus=excluded), out,
                      indent=1, sort_keys=True)
    fits = {}
    for k in orders:
        for t in threads:
            points = [(e['sites'], e['chaud']) for e in entries
                      if e['k'] == k and e['fils'] == t and e['code'] == 0 and e['sites'] and e['chaud'] is not None]
            fits['k%d_f%d' % (k, t)] = fit(points)
    with open(os.path.join(args.sortie, 'mes_p.json'), 'w', encoding='utf-8') as out:
        json.dump(dict(mesure='MES-P', masque=MASK, passes=args.passes, prises=entries, ajustements=fits,
                       exclus=excluded), out, indent=1, sort_keys=True)
    lines = ['# MES-P : v11 gelee sur les petits nuages, a chaud', '', '| K et fils | cout fixe (ms) | cout par site (µs) '
             '| nuages |', '| --- | ---: | ---: | ---: |']
    for key, value in sorted(fits.items()):
        if value is None:
            lines.append('| %s | - | - | - |' % key)
        else:
            lines.append('| %s | %s | %s | %d |' % (key, value['fixe_ms'], value['par_site_us'], value['points']))
    failed = [e for e in entries if e['code'] != 0 or e['chaud'] is None]
    lines += ['', 'Prises : %d ; en echec ou expirees : %d.' % (len(entries), len(failed))]
    with open(os.path.join(args.sortie, 'mes_p.md'), 'w', encoding='utf-8') as out:
        out.write('\n'.join(lines) + '\n')
    print(json.dumps(dict(prises=len(entries), echecs=len(failed), ajustements=fits), sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
