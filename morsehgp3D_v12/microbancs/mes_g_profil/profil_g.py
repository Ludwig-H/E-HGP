#!/usr/bin/env python3
"""Profil de l'etage G par poste sur G4 (information, sans regle ni verdict) : la sonde de G (bench/tower_probe.cpp,
resolve_tower, voie CPU de reference) est construite avec MHGP12_TOWER_PROFILE (compteur de cycles encadre par lfence,
src/tower/profile.hpp) et jouee a 48 fils puis a un fil sur trois trames : ng00, la trame mediane du v12set
(kitti_ng_02_001606, 64 740 sites) et la plus grande (kitti_ng_08_002119, 99 099 sites), a K5. Les cycles d'une section
sont sommes sur les fils : a 48 fils, ils comptent aussi l'attente de la memoire et la contention ; le rapport des
temps-fils 48 / 1 d'une section mesure sa perte au passage a l'echelle. Le profil ajoute deux lectures encadrees par
occurrence : il gonfle les petites sections (biais publie, non retranche des parts). Rien de ce banc n'est le produit :
la Session recouverte n'est pas jouee, et les murs de G publies ici sont ceux de la sonde profilee.

Pourquoi : apres T2-d-B et B2, choisir le levier suivant de G (vers le contrat FULL de 100 ms) sur des parts mesurees a
48 fils sur G4, et non sur un profil local a 3 fils sous charge.

  python3 profil_g.py --src <racine du paquet> --travail <dossier> --donnees <donnees> --sortie <sortie> \\
      [--fils 48] [--passes 4] [--passes-un-fil 2] [--jobs 44]
  python3 profil_g.py --auto-test

Donnees : lidar_ng00.u32le et lidar_ng00.ids.u32le, g4_kitti_v12set_xyz.tar (archive plate du v12set). Sorties :
journaux bruts par execution, profil_g.json (resume), profil_g.md (tableaux). Python 3.10 nu, sans assert (jouable sous
python3 -S -O). Codes : 0 profil complet ; 1 auto-test en echec ; 2 usage ; 3 construction, deballage ou execution en
echec (les executions deja faites restent publiees).
"""
import argparse
import json
import os
import statistics
import sys

ICI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ICI, '..', 'outils'))
import banc_full as bf  # noqa: E402

SECTIONS = ('trace', 'sonde', 'proposition', 't1', 'certificat', 'repli', 'census_sature', 'census_complet', 'pas',
            'arret')
TRAMES = (('ng00', None), ('mediane', 'kitti_ng_02_001606'), ('maximum', 'kitti_ng_08_002119'))
K = 5


def lire(sortie):
    """Lignes JSON de la sonde : passes tour_g et profils profil_g ; rend (passes, profils) ou None si illisible."""
    passes, profils = {}, {}
    for ligne in sortie.splitlines():
        if not ligne.startswith('{'):
            continue
        try:
            d = json.loads(ligne)
        except ValueError:
            return None
        if d.get('phase') == 'tour_g':
            if d.get('status') != 'ok':
                return None
            passes[d['pass']] = d
        elif d.get('phase') == 'profil_g':
            profils.setdefault(d['pass'], {})[d['k']] = d
    if not passes or set(profils) != set(passes):
        return None
    if any(set(profils[p]) != set(range(2, K + 1)) for p in profils):
        return None  # un profil par ordre 2..K et par passe, sinon les sommes seraient partielles
    return passes, profils


def resumer(passes, profils):
    """Mediane sur les passes 2..P (la premiere, a froid, ecartee s'il y en a plusieurs) : mur, tables, resolution ;
    par section (ordres 2..K sommes) : temps-fil (ms), part des cycles, occurrences, ns par occurrence."""
    garde = sorted(passes)[1:] if len(passes) > 1 else sorted(passes)
    med = lambda xs: statistics.median(xs) if xs else 0.0  # noqa: E731
    res = {'passes': len(garde), 'mur_ms': med([passes[p]['wall_ns'] / 1e6 for p in garde]),
           'tables_ms': med([passes[p]['diagnostics']['tables_ns'] / 1e6 for p in garde]),
           'resolution_ms': med([passes[p]['diagnostics']['resolve_ns'] / 1e6 for p in garde]),
           'sections': {}, 'ordres': {}}
    for nom in SECTIONS + ('reste',):
        ms, part, n, ns = [], [], [], []
        for p in garde:
            cyc = tot = occ = 0
            ghz = 0.0
            for k, d in profils[p].items():
                s = d['reste'] if nom == 'reste' else d['sections'][nom]
                cyc += s['cycles']
                occ += s.get('n', 0)
                tot += d['sections']['total']['cycles']
                ghz = d['ghz_tsc']
            ms.append(cyc / ghz / 1e6 if ghz > 0 else 0.0)
            part.append(cyc / tot if tot else 0.0)
            n.append(occ)
            ns.append(cyc / ghz / occ if ghz > 0 and occ else 0.0)
        res['sections'][nom] = {'temps_fil_ms': med(ms), 'part': med(part), 'occurrences': med(n),
                                'ns_par_occurrence': med(ns)}
    for k in sorted(profils[garde[0]]):
        res['ordres'][k] = {'temps_fil_ms': med([profils[p][k]['sections']['total']['cycles'] / profils[p][k]['ghz_tsc']
                                                 / 1e6 for p in garde if profils[p][k]['ghz_tsc'] > 0]),
                            'table_ms': med([profils[p][k]['table_ns'] / 1e6 for p in garde])}
    return res


def tableaux(resume):
    lignes = ['# Profil de l\'etage G par poste (information, sans verdict)', '',
              'Sonde de G profilee (`MHGP12_TOWER_PROFILE`), voie CPU de reference, K5. Temps-fils sommes sur les fils '
              '(ordres 2 a 5) ; mediane des passes 2..P.', '']
    for cle, r in resume.items():
        lignes += ['## %s' % cle, '', 'Mur de la sonde %.1f ms, index des naissances %.1f ms, resolution %.1f ms.' %
                   (r['mur_ms'], r['tables_ms'], r['resolution_ms']), '',
                   '| poste | temps-fil (ms) | part | occurrences | ns par occurrence |', '| --- | ---: | ---: | ---: | ---: |']
        for nom in SECTIONS + ('reste',):
            s = r['sections'][nom]
            lignes.append('| %s | %.1f | %.3f | %d | %.0f |' % (nom, s['temps_fil_ms'], s['part'], s['occurrences'],
                                                               s['ns_par_occurrence']))
        lignes.append('')
    return '\n'.join(lignes) + '\n'


def auto_test():
    def sortie(passes, cycles):
        out = []
        for p in range(passes):
            out.append(json.dumps({'phase': 'tour_g', 'pass': p, 'status': 'ok', 'wall_ns': 10_000_000 * (p + 1),
                                   'diagnostics': {'tables_ns': 1_000_000, 'resolve_ns': 8_000_000}}))
            for k in range(2, K + 1):
                sec = {nom: {'cycles': cycles, 'n': 10} for nom in SECTIONS}
                sec['total'] = {'cycles': cycles * (len(SECTIONS) + 1), 'n': 0}
                out.append(json.dumps({'phase': 'profil_g', 'pass': p, 'k': k, 'ghz_tsc': 2.0, 'biais_cycles': 30.0,
                                       'table_ns': 250_000, 'join_ns': 0, 'sections': sec,
                                       'reste': {'cycles': cycles, 'part': 1 / 11}}))
        return '\n'.join(out) + '\n'
    lu = lire(sortie(3, 2000))
    if lu is None:
        return False
    r = resumer(*lu)
    ok = (r['passes'] == 2 and abs(r['mur_ms'] - 25.0) < 1e-9 and abs(r['sections']['t1']['temps_fil_ms'] - 0.004) < 1e-12
          and abs(r['sections']['t1']['part'] - 1 / 11) < 1e-12 and r['sections']['t1']['occurrences'] == 40
          and abs(r['sections']['t1']['ns_par_occurrence'] - 100.0) < 1e-9 and len(r['ordres']) == K - 1)
    illisible = lire(sortie(2, 10) + '{"phase":"tour_g","pass":7,"status":"refused"}\n') is None
    incomplet = lire('\n'.join(sortie(2, 10).splitlines()[:-1]) + '\n') is None  # profil d'un ordre manquant
    return ok and illisible and incomplet and '| t1 |' in tableaux({'x': r})


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--auto-test', action='store_true')
    ap.add_argument('--src')
    ap.add_argument('--travail')
    ap.add_argument('--donnees')
    ap.add_argument('--sortie')
    ap.add_argument('--fils', type=int, default=48)
    ap.add_argument('--passes', type=int, default=4)
    ap.add_argument('--passes-un-fil', type=int, default=2)
    ap.add_argument('--jobs', type=int, default=44)
    args = ap.parse_args()
    if args.auto_test:
        ok = auto_test()
        print('profil_g_auto_test_%s' % ('ok' if ok else 'echec'))
        return 0 if ok else 1
    if not (args.src and args.travail and args.donnees and args.sortie):
        ap.print_usage(sys.stderr)
        return 2
    if not auto_test():
        print('profil_g : auto-test en echec', file=sys.stderr)
        return 1
    os.makedirs(args.sortie, exist_ok=True)
    os.makedirs(args.travail, exist_ok=True)
    journal = os.path.join(args.sortie, 'construction.log')
    dossier = os.path.join(args.travail, 'build_profil')
    for argv in (['cmake', '-S', os.path.join(args.src, 'morsehgp3D_v12'), '-B', dossier, '-DCMAKE_BUILD_TYPE=Release',
                  '-DMHGP12_COORD_BITS=21', '-DMHGP12_ENABLE_CUDA=OFF', '-DCMAKE_CXX_FLAGS=-DMHGP12_TOWER_PROFILE'],
                 ['cmake', '--build', dossier, '-j', str(args.jobs), '--target', 'mhgp12_tower_probe']):
        code, out, err, _s = bf.run(argv, 1800)
        with open(journal, 'a', encoding='utf-8') as log:
            log.write('$ %s\n%s%s' % (' '.join(argv), out, err))
        if code != 0:
            print('profil_g : construction en echec (%s)' % code, file=sys.stderr)
            return 3
    sonde = os.path.join(dossier, 'mhgp12_tower_probe')
    v12set = os.path.join(args.travail, 'v12set')
    if bf.unpack(os.path.join(args.donnees, 'g4_kitti_v12set_xyz.tar'), v12set) is None:
        print('profil_g : deballage du v12set en echec', file=sys.stderr)
        return 3
    resume, echecs = {}, []
    for nom, trame in TRAMES:
        base = os.path.join(args.donnees, 'lidar_ng00') if trame is None else os.path.join(v12set, trame)
        for fils, passes in ((args.fils, args.passes), (1, args.passes_un_fil)):
            cle = '%s, %d fil%s' % (nom, fils, 's' if fils > 1 else '')
            brut = os.path.join(args.sortie, '%s_f%d' % (nom, fils))
            code, out, _err, sec = bf.run([sonde, base + '.u32le', base + '.ids.u32le', '--k=%d' % K,
                                           '--threads=%d' % fils, '--passes=%d' % passes], 1200, brut + '.out',
                                          brut + '.err')
            lu = lire(out) if code == 0 else None
            if lu is None:
                echecs.append({'cas': cle, 'code': code, 'secondes': round(sec, 1)})
                continue
            resume[cle] = resumer(*lu)
            resume[cle]['secondes'] = round(sec, 1)
    with open(os.path.join(args.sortie, 'profil_g.json'), 'w', encoding='utf-8') as out:
        json.dump({'schema': 'ehgp.v12.profil_g.v1', 'k': K, 'sonde_sha256': bf.sha256_file(sonde),
                   'cmake': bf.cmake_extract(dossier), 'resume': resume, 'echecs': echecs}, out, indent=1,
                  sort_keys=True)
    with open(os.path.join(args.sortie, 'profil_g.md'), 'w', encoding='utf-8') as out:
        out.write(tableaux(resume))
    print('profil_g : %d cas, %d echecs' % (len(resume), len(echecs)))
    return 3 if echecs else 0


if __name__ == '__main__':
    sys.exit(main())
