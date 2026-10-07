#!/usr/bin/env python3
"""MES-D6 : cout des profils de coordonnees u24 et u32 face a u21 (decision D6, constat CST-0207), sur trames reelles.

Mesure publiee, sans regle d'adoption : la v12 vise au moins u21, puis u24 et u32 ; il faut savoir ce que coute un
profil elargi sur les memes trames. Le pilote construit le produit a chaque profil demande (cibles
mhgp12_catalogue_probe et mhgp12_tower_probe, Release), puis joue, par tours alternes, pour chaque trame et chaque
couple (profil, dilatation) admissible : la sonde du catalogue puis celle de l'etage G de la tour, P passes dans un
processus neuf. Dilatations : facteur 1 (memes coordonnees), 8 (trois bits de plus, une grille 8 fois plus fine) et
2048 (onze bits de plus), retenues seulement si la plus grande coordonnee dilatee tient dans le profil.

Controles (jamais un temps) :
  - identite : a facteur 1, le catalogue et la resolution sont les memes aux trois profils (meme objet, meme entree) :
    empreinte de la resolution (en-tete exclu par la sonde) et SHA-256 de l'export du catalogue PRIVE de son en-tete de
    64 octets (qui porte les bits du profil), export fait au premier tour seulement puis efface ;
  - invariance par dilatation : les compteurs de l'objet de chaque ordre (naissances, cellules, cellules inertes et
    etendues, representants) et le nombre de boules et d'incidences du catalogue ne changent pas (tous les predicats
    sont homogenes, l'ordre des positions est conserve) ;
  - une empreinte par prise, constante sur ses passes.
Temps : mediane des passes 2..P de chaque prise ; par tour, rapport au couple (21, 1) de la meme trame ; publie la
mediane, le minimum et le maximum des rapports sur les tours.

Usage : pilote_d6.py --src DOSSIER_V12 --racine DOSSIER --donnees DOSSIER --sortie DOSSIER [--cas ng00,ng01,ng02]
                     [--profils 21,24,32] [--k 5] [--fils 48] [--passes 5] [--tours 3] [--jobs 44] [--delai 900]
Sorties : <sortie>/mes_d6.json et mes_d6.md, lignes brutes sous <sortie>/brut/. Codes : 0 rendu et controles
conformes ; 1 controle viole (identite ou invariance) ; 2 usage ou entree illisible ; 3 construction impossible.
Bibliotheque standard seule (Python 3.10 nu), aucun assert.
"""
import argparse
import array
import hashlib
import json
import os
import shutil
import signal
import statistics
import subprocess
import sys

FACTORS = (1, 8, 2048)
OBJECT_KEYS = ('births', 'cells', 'inert_cells', 'extended_cells', 'representatives')


def build(src, root, bits, jobs):
    folder = os.path.join(root, 'p%d' % bits)
    configure = ['cmake', '-S', src, '-B', folder, '-DCMAKE_BUILD_TYPE=Release', '-DMHGP12_COORD_BITS=%d' % bits]
    targets = ['cmake', '--build', folder, '-j', str(jobs), '--target', 'mhgp12_catalogue_probe', 'mhgp12_tower_probe']
    for argv in (configure, targets):
        if subprocess.run(argv, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
            return None
    return folder


def dilate(data, case, factor, folder):
    """Ecrit l'entree dilatee de la trame ; rend (chemin xyz, chemin ids, plus grande coordonnee)."""
    xyz = os.path.join(data, 'lidar_%s.u32le' % case)
    ids = os.path.join(data, 'lidar_%s.ids.u32le' % case)
    values = array.array('I')
    with open(xyz, 'rb') as handle:
        values.frombytes(handle.read())
    if sys.byteorder != 'little':
        values.byteswap()
    largest = max(values) * factor
    if factor == 1:
        return xyz, ids, largest
    if largest >= 1 << 32:
        return None, None, largest
    scaled = array.array('I', (v * factor for v in values))
    if sys.byteorder != 'little':
        scaled.byteswap()
    path = os.path.join(folder, 'lidar_%s_x%d.u32le' % (case, factor))
    with open(path, 'wb') as out:
        out.write(scaled.tobytes())
    return path, ids, largest


def run(argv, delay, raw_path):
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
    with open(raw_path, 'wb') as handle:
        handle.write(out)
    lines = []
    for raw in out.decode('utf-8', 'replace').splitlines():
        try:
            line = json.loads(raw)
        except ValueError:
            continue
        if isinstance(line, dict):
            lines.append(line)
    return code, lines


def summarize(code, lines, phase, digest_key):
    walls = [x['wall_ns'] for x in lines if x.get('phase') == phase and isinstance(x.get('wall_ns'), int)]
    digests = sorted(set(x[digest_key] for x in lines if x.get('phase') == 'digest' and digest_key in x))
    ok = code == 0 and len(walls) >= 2 and len(digests) == 1
    first = next((x for x in lines if x.get('phase') == phase), {})
    objects = {x['k']: {k: x['objet'][k] for k in OBJECT_KEYS}
               for x in lines if x.get('phase') == 'ordre' and isinstance(x.get('objet'), dict)}
    return dict(code=code, ok=ok, chaud_ms=statistics.median(walls[1:]) / 1e6 if ok else None,
                passes_ms=[w / 1e6 for w in walls], empreinte=digests[0] if len(digests) == 1 else None,
                boules=first.get('balls'), incidences=first.get('incidences'), objet=objects)


def body_sha(path):
    """SHA-256 d'un export MHGP12DP prive de son en-tete de 64 octets (bits du profil, trame) ; None s'il manque."""
    digest = hashlib.sha256()
    try:
        with open(path, 'rb') as handle:
            if len(handle.read(64)) != 64:
                return None
            for block in iter(lambda: handle.read(1 << 20), b''):
                digest.update(block)
    except OSError:
        return None
    return digest.hexdigest()


def take(binaries, inputs, combo, args, raw_dir, round_index):
    bits, factor = combo
    xyz, ids, _largest = inputs[combo[1]]
    common = ['--k=%d' % args.k, '--threads=%d' % args.fils, '--passes=%d' % args.passes, '--digest']
    tag = '%s_p%d_x%d_t%d' % (args.case_now, bits, factor, round_index)
    export = os.path.join(args.racine, 'export_' + tag)
    extra = ['--out=' + export, '--frame=' + args.case_now] if round_index == 0 and factor == 1 else []
    code_c, lines_c = run([os.path.join(binaries[bits], 'mhgp12_catalogue_probe'), xyz, ids] + common + extra,
                          args.delai, os.path.join(raw_dir, tag + '_catalogue.jsonl'))
    body = body_sha(os.path.join(export, 'cat.bin')) if extra else None
    shutil.rmtree(export, ignore_errors=True)
    code_t, lines_t = run([os.path.join(binaries[bits], 'mhgp12_tower_probe'), xyz, ids] + common, args.delai,
                          os.path.join(raw_dir, tag + '_tour.jsonl'))
    catalogue = summarize(code_c, lines_c, 'catalogue', 'catalogue_sha256')
    catalogue['corps_sha256'] = body
    if extra and body is None:
        catalogue['ok'] = False
    return dict(cas=args.case_now, profil=bits, facteur=factor, tour=round_index, catalogue=catalogue,
                tour_g=summarize(code_t, lines_t, 'tour_g', 'resolution_sha256'))


def checks(entries):
    """Identite a facteur 1 entre profils ; invariance des comptes de l'objet par dilatation ; prises conformes."""
    problems = []
    for e in entries:
        for stage in ('catalogue', 'tour_g'):
            if not e[stage]['ok']:
                problems.append('%s p%d x%d tour %d : %s non conforme (code %s)' % (
                    e['cas'], e['profil'], e['facteur'], e['tour'], stage, e[stage]['code']))
    for case in sorted(set(e['cas'] for e in entries)):
        mine = [e for e in entries if e['cas'] == case and e['catalogue']['ok'] and e['tour_g']['ok']]
        native = [e for e in mine if e['facteur'] == 1]
        if len(set(e['tour_g']['empreinte'] for e in native)) > 1:
            problems.append('%s : empreintes de la resolution differentes entre profils a facteur 1' % case)
        bodies = set(e['catalogue']['corps_sha256'] for e in native if e['tour'] == 0)
        if len(bodies) != 1 or None in bodies:
            problems.append('%s : exports du catalogue (en-tete exclu) differents entre profils a facteur 1' % case)
        shapes = set(json.dumps([e['catalogue']['boules'], e['catalogue']['incidences'], e['tour_g']['objet']],
                                sort_keys=True) for e in mine)
        if len(shapes) > 1:
            problems.append('%s : comptes de l\'objet changes par profil ou dilatation' % case)
    return problems


def ratios(entries):
    rows = []
    for case in sorted(set(e['cas'] for e in entries)):
        combos = sorted(set((e['profil'], e['facteur']) for e in entries if e['cas'] == case))
        for combo in combos:
            row = dict(cas=case, profil=combo[0], facteur=combo[1])
            for stage in ('catalogue', 'tour_g'):
                values = []
                for e in entries:
                    if e['cas'] != case or (e['profil'], e['facteur']) != combo or e[stage]['chaud_ms'] is None:
                        continue
                    base = [b for b in entries if b['cas'] == case and b['tour'] == e['tour'] and b['profil'] == 21
                            and b['facteur'] == 1 and b[stage]['chaud_ms']]
                    if base:
                        values.append(e[stage]['chaud_ms'] / base[0][stage]['chaud_ms'])
                times = [e[stage]['chaud_ms'] for e in entries if e['cas'] == case and
                         (e['profil'], e['facteur']) == combo and e[stage]['chaud_ms'] is not None]
                row[stage] = dict(chaud_ms=statistics.median(times) if times else None,
                                  rapport=statistics.median(values) if values else None,
                                  rapport_min=min(values) if values else None,
                                  rapport_max=max(values) if values else None)
            rows.append(row)
    return rows


def markdown(rows, problems, args):
    lines = ['# MES-D6 : profils u21, u24 et u32 sur trames reelles', '',
             'K = %d, %d fils, %d passes par prise, %d tours alternes ; temps chaud = mediane des passes 2..P ; '
             'rapport au couple (21, x1) de la meme trame et du meme tour.' % (args.k, args.fils, args.passes,
                                                                                args.tours), '',
             '| trame | profil | dilatation | catalogue (ms) | rapport | etage G (ms) | rapport |',
             '| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
    for r in rows:
        cells = []
        for stage in ('catalogue', 'tour_g'):
            s = r[stage]
            cells.append('-' if s['chaud_ms'] is None else '%.1f' % s['chaud_ms'])
            cells.append('-' if s['rapport'] is None else '%.3f [%.3f ; %.3f]' % (s['rapport'], s['rapport_min'],
                                                                                 s['rapport_max']))
        lines.append('| %s | %d | x%d | %s | %s | %s | %s |' % (r['cas'], r['profil'], r['facteur'], *cells))
    lines += ['', 'Controles : %s.' % ('conformes' if not problems else '%d ecart(s)' % len(problems))]
    lines += ['- ' + p for p in problems]
    return '\n'.join(lines) + '\n'


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ('--src', '--racine', '--donnees', '--sortie'):
        parser.add_argument(name, required=True)
    parser.add_argument('--cas', default='ng00,ng01,ng02')
    parser.add_argument('--profils', default='21,24,32')
    for name, default in (('--k', 5), ('--fils', 48), ('--passes', 5), ('--tours', 3), ('--jobs', 44),
                          ('--delai', 900)):
        parser.add_argument(name, type=int, default=default)
    try:
        args = parser.parse_args(argv[1:])
        cases = [c for c in args.cas.split(',') if c]
        profiles = [int(p) for p in args.profils.split(',')]
    except (SystemExit, ValueError):
        return 2
    if not cases or 21 not in profiles or any(p not in (21, 24, 32) for p in profiles) or args.passes < 2 or \
            args.tours < 1 or not 1 <= args.k <= 12 or args.fils < 1:
        return 2
    os.makedirs(os.path.join(args.sortie, 'brut'), exist_ok=True)
    inputs_dir = os.path.join(args.racine, 'entrees')
    os.makedirs(inputs_dir, exist_ok=True)
    binaries = {}
    for bits in profiles:
        folder = build(args.src, args.racine, bits, args.jobs)
        if folder is None:
            return 3
        binaries[bits] = folder
    entries = []
    for case in cases:
        try:
            inputs = {f: dilate(args.donnees, case, f, inputs_dir) for f in FACTORS}
        except OSError:
            return 2
        combos = [(bits, f) for bits in profiles for f in FACTORS
                  if inputs[f][0] is not None and inputs[f][2] < 1 << bits]
        args.case_now = case
        for round_index in range(args.tours):
            shift = round_index % len(combos)
            for combo in combos[shift:] + combos[:shift]:
                entries.append(take(binaries, inputs, combo, args, os.path.join(args.sortie, 'brut'), round_index))
                with open(os.path.join(args.sortie, 'mes_d6.json'), 'w', encoding='utf-8') as out:
                    json.dump(dict(mesure='MES-D6', prises=entries), out, indent=1, sort_keys=True)
    problems = checks(entries)
    rows = ratios(entries)
    with open(os.path.join(args.sortie, 'mes_d6.json'), 'w', encoding='utf-8') as out:
        json.dump(dict(mesure='MES-D6', prises=entries, rapports=rows, controles=problems), out, indent=1,
                  sort_keys=True)
    with open(os.path.join(args.sortie, 'mes_d6.md'), 'w', encoding='utf-8') as out:
        out.write(markdown(rows, problems, args))
    print(json.dumps(dict(prises=len(entries), ecarts=len(problems)), sort_keys=True))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
