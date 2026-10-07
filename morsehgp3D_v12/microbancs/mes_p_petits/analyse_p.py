#!/usr/bin/env python3
"""MES-P : lecture d'un mes_p.json par famille de nuages (le pilote n'ajuste qu'une droite sur tous les nuages).

Une droite unique t = a + b n melange les petits nuages LiDAR reels et les familles synthetiques degenerees (reseau,
quasi-sphere), dont le cout explose : son cout fixe sort negatif et ne fixe aucun seuil. Ce lecteur separe, toujours
par couple (K, fils) affiche dans chaque ligne (CST-0238 : reunir 1, 4 et 48 fils donnerait une pente d'aucun regime) :
  - les familles reelles (prefixes bout, kctx, knn, kobj : morceaux de trames SemanticKITTI sans sol) : medianes du
    temps chaud par classe de taille, puis droite des moindres carres sur ces seules familles ;
  - si plusieurs nombres de fils sont joues pour un meme K : la meme droite sur la COHORTE COMMUNE (nuages reels
    rendus a tous ces nombres de fils), avec le nombre de nuages ecartes par fils ;
  - chaque famille synthetique (synth_<famille>) : temps chaud par taille, et, si --brut est donne, la part de l'etage
    foret (forest_ns, qui couvre preparation, resolution, noyau et contraction de la v11) dans la derniere passe ;
  - les prises en echec ou expirees, avec leurs fils et le motif lu dans leur ligne brute {"phase":"exit"} ; une prise
    expiree n'a aucune valeur chaude, meme si des passes ont ete rendues avant le delai.
Usage : analyse_p.py MES_P_JSON [--brut DOSSIER] ; ecrit le Markdown sur la sortie standard. Codes : 0 rendu ; 2
entree illisible. Bibliotheque standard seule (Python 3.10 nu).
"""
import json
import os
import statistics
import sys

REAL = ('bout', 'kctx', 'knn', 'kobj')
BINS = ((100, 300), (300, 1000), (1000, 3000), (3000, 10001))


def family(name):
    parts = name.split('_')
    return '_'.join(parts[:2]) if parts[0] == 'synth' else parts[0]


def raw_lines(brut, take):
    path = os.path.join(brut, '%s_k%d_f%d.jsonl' % (take['nuage'], take['k'], take['fils']))
    lines = []
    try:
        with open(path, encoding='utf-8', errors='replace') as handle:
            for raw in handle:
                try:
                    line = json.loads(raw)
                except ValueError:
                    continue
                if isinstance(line, dict):
                    lines.append(line)
    except OSError:
        return []
    return lines


def fit(points):
    if len(set(n for n, _t in points)) < 2:
        return None
    mean_n = sum(n for n, _t in points) / len(points)
    mean_t = sum(t for _n, t in points) / len(points)
    b = sum((n - mean_n) * (t - mean_t) for n, t in points) / sum((n - mean_n) ** 2 for n, _t in points)
    return mean_t - b * mean_n, b


def real_rows(takes, k, threads):
    return [t for t in takes if t['k'] == k and t['fils'] == threads and family(t['nuage']) in REAL]


def real_table(takes, k, threads, out):
    out.append('| familles reelles, K = %d, %d fils | prises | sites (mediane) | chaud mediane (ms) | min | max '
               '| µs par site |' % (k, threads))
    out.append('| --- | ---: | ---: | ---: | ---: | ---: | ---: |')
    mine = real_rows(takes, k, threads)
    for low, high in BINS:
        rows = [t for t in mine if low <= t['sites'] < high]
        if not rows:
            continue
        warm = [t['chaud'] * 1e3 for t in rows]
        sites = statistics.median(t['sites'] for t in rows)
        per_site = statistics.median(t['chaud'] * 1e6 / t['sites'] for t in rows)
        out.append('| %d a %d sites | %d | %d | %.1f | %.1f | %.1f | %.1f |' % (
            low, high - 1, len(rows), sites, statistics.median(warm), min(warm), max(warm), per_site))
    line = fit([(t['sites'], t['chaud']) for t in mine])
    if line is not None:
        out.append('')
        out.append('Droite des moindres carres (familles reelles, K = %d, %d fils) : cout fixe %.2f ms, %.2f µs par '
                   'site.' % (k, threads, line[0] * 1e3, line[1] * 1e6))
    out.append('')


def cohort_table(takes, good, k, out):
    """Droites par nombre de fils sur les seuls nuages reels rendus a tous les nombres de fils joues pour K."""
    threads = sorted(set(t['fils'] for t in takes if t['k'] == k))
    if len(threads) < 2:
        return
    common = set.intersection(*[set(t['nuage'] for t in real_rows(good, k, f)) for f in threads])
    out.append('| cohorte commune, K = %d | nuages | ecartes (hors cohorte) | cout fixe (ms) | µs par site |' % k)
    out.append('| --- | ---: | ---: | ---: | ---: |')
    for f in threads:
        mine = real_rows(good, k, f)
        line = fit([(t['sites'], t['chaud']) for t in mine if t['nuage'] in common])
        cells = ('%.2f' % (line[0] * 1e3), '%.2f' % (line[1] * 1e6)) if line is not None else ('-', '-')
        out.append('| %d fils | %d | %d | %s | %s |' % (f, len(common), len(mine) - len(common), cells[0], cells[1]))
    out.append('')


def forest_share(brut, take):
    passes = [x for x in raw_lines(brut, take) if x.get('phase') == 'pass' and isinstance(x.get('wall_ns'), int)]
    if not passes or passes[-1]['wall_ns'] <= 0 or not isinstance(passes[-1].get('forest_ns'), int):
        return ''
    return ' [%d %%]' % round(100 * passes[-1]['forest_ns'] / passes[-1]['wall_ns'])


def synthetic_table(takes, brut, out):
    out.append('| famille synthetique | K | fils | sites : chaud (ms) [part de l\'etage foret] |')
    out.append('| --- | ---: | ---: | --- |')
    for name in sorted(set(family(t['nuage']) for t in takes) - set(REAL)):
        for k, threads in sorted(set((t['k'], t['fils']) for t in takes)):
            rows = sorted((t for t in takes if family(t['nuage']) == name and t['k'] == k and t['fils'] == threads),
                          key=lambda t: t['sites'])
            cells = ['%d : %.1f%s' % (t['sites'], t['chaud'] * 1e3, forest_share(brut, t) if brut else '')
                     for t in rows]
            if cells:
                out.append('| %s | %d | %d | %s |' % (name, k, threads, ' ; '.join(cells)))
    out.append('')


def failure_table(failed, brut, out):
    out.append('| prise en echec | K | fils | sites | code | motif |')
    out.append('| --- | ---: | ---: | ---: | --- | --- |')
    for t in failed:
        exits = [x for x in raw_lines(brut, t) if x.get('phase') == 'exit'] if brut else []
        reason = '%s / %s' % (exits[-1].get('status'), exits[-1].get('reason')) if exits else '-'
        out.append('| %s | %d | %s | %s | %s | %s |' % (t['nuage'], t['k'], t['fils'], t['sites'], t['code'], reason))


def main(argv):
    if len(argv) not in (2, 4) or (len(argv) == 4 and argv[2] != '--brut'):
        print(__doc__.splitlines()[0], file=sys.stderr)
        return 2
    brut = argv[3] if len(argv) == 4 else None
    try:
        with open(argv[1], encoding='utf-8') as handle:
            takes = json.load(handle)['prises']
        good = [t for t in takes if t['code'] == 0 and t['chaud'] is not None and t['sites']]
        failed = [t for t in takes if t['code'] != 0]
    except (OSError, ValueError, KeyError, TypeError):
        return 2
    out = ['# MES-P par famille de nuages', '', 'Prises : %d ; rendues : %d ; en echec ou expirees : %d.' % (
        len(takes), len(good), len(failed)), '']
    for k, threads in sorted(set((t['k'], t['fils']) for t in good)):
        real_table(good, k, threads, out)
    for k in sorted(set(t['k'] for t in takes)):
        cohort_table(takes, good, k, out)
    synthetic_table(good, brut, out)
    if failed:
        failure_table(failed, brut, out)
    print('\n'.join(out))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
