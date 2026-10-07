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
                    [--k 5,10] [--passes 6] [--delai 300] [--jobs 44] [--limite N]
  --archive : le paquet g4_small en une seule archive tar (le televersement d'une session paie chaque fichier : 320
  petits fichiers depassent son delai) ; deballee dans --deballage apres controle de chaque membre (fichier simple, nom
  simple, aucun chemin), puis lue comme --donnees.
Sorties : <sortie>/mes_p.json, <sortie>/mes_p.md, lignes brutes sous <sortie>/brut/. Codes : 0 rendu ; 2 usage ou
manifeste illisible ; 3 construction impossible. Bibliotheque standard seule (Python 3.10 nu).
"""
import argparse
import json
import os
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


def pass_seconds(line):
    """Duree d'une passe FULL d'apres une ligne {"phase":"pass"} de la sonde : wall_ns, sinon domaine + forets."""
    if not isinstance(line, dict) or line.get('phase') != 'pass':
        return None
    if isinstance(line.get('wall_ns'), int):
        return line['wall_ns'] / 1e9
    total = 0
    found = False
    for key in ('domain_ns', 'forest_ns'):
        if isinstance(line.get(key), int):
            total += line[key]
            found = True
    return total / 1e9 if found else None


def run_cloud(binary, data, raw_dir, name, k, threads, passes, delay):
    leaf = 16 if k <= 5 else 24
    argv = [binary, os.path.join(data, name + '.u32le'), os.path.join(data, name + '.ids.u32le'), '/dev/null',
            str(k), str(leaf), '256', '0', '4294967295', str(8 << 30), str(threads), MASK, str(passes)]
    start = time.monotonic()
    try:
        done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=delay)
        code, out = done.returncode, done.stdout
    except subprocess.TimeoutExpired as expired:
        code, out = 'expire', expired.stdout or b''
    seconds = time.monotonic() - start
    with open(os.path.join(raw_dir, '%s_k%d_f%d.jsonl' % (name, k, threads)), 'wb') as handle:
        handle.write(out)
    per_pass, sites = [], None
    for raw in out.decode('utf-8', 'replace').splitlines():
        raw = raw.strip()
        if not raw.startswith('{'):
            continue
        try:
            line = json.loads(raw)
        except ValueError:
            continue
        if isinstance(line, dict) and isinstance(line.get('sites'), int):
            sites = line['sites']
        value = pass_seconds(line)
        if value is not None:
            per_pass.append(value)
    warm = statistics.median(per_pass[1:]) if len(per_pass) >= 2 else None
    return dict(nuage=name, k=k, fils=threads, code=code, sites=sites, processus_secondes=round(seconds, 4),
                passes=[round(v, 6) for v in per_pass], froid=per_pass[0] if per_pass else None, chaud=warm)


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
    if args.limite > 0:
        names = names[:args.limite]
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
            json.dump(dict(mesure='MES-P', masque=MASK, passes=args.passes, prises=entries), out, indent=1,
                      sort_keys=True)
    fits = {}
    for k in orders:
        for t in threads:
            points = [(e['sites'], e['chaud']) for e in entries
                      if e['k'] == k and e['fils'] == t and e['code'] == 0 and e['sites'] and e['chaud'] is not None]
            fits['k%d_f%d' % (k, t)] = fit(points)
    with open(os.path.join(args.sortie, 'mes_p.json'), 'w', encoding='utf-8') as out:
        json.dump(dict(mesure='MES-P', masque=MASK, passes=args.passes, prises=entries, ajustements=fits), out,
                  indent=1, sort_keys=True)
    lines = ['# MES-P : v11 gelee sur les petits nuages, a chaud', '', '| K et fils | cout fixe (ms) | cout par site (µs) '
             '| nuages |', '| --- | ---: | ---: | ---: |']
    for key, value in sorted(fits.items()):
        if value is None:
            lines.append('| %s | - | - | - |' % key)
        else:
            lines.append('| %s | %s | %s | %d |' % (key, value['fixe_ms'], value['par_site_us'], value['points']))
    failed = [e for e in entries if e['code'] != 0]
    lines += ['', 'Prises : %d ; en echec ou expirees : %d.' % (len(entries), len(failed))]
    with open(os.path.join(args.sortie, 'mes_p.md'), 'w', encoding='utf-8') as out:
        out.write('\n'.join(lines) + '\n')
    print(json.dumps(dict(prises=len(entries), echecs=len(failed), ajustements=fits), sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
