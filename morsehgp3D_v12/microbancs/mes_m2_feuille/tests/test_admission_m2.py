#!/usr/bin/env python3
"""Porte native de l'admission des vidages MHGP12LF (constat CST-0215) sur les binaires hote construits.

1. mhgp12_dump_admission_selftest ecrit un vidage synthetique valide et des mutants a empreinte FNV-1a juste (dont les
   six injections du recu audit_socle_microbancs_20261007/feuille : site_out_of_cloud, wrapped_job_begin, zero_k,
   profile_mismatch, coordinate_out_of_profile, short_record_population) : code 0 exige (valide admis, mutants
   refuses par dump::read).
2. Les vrais outils rejouent ces fichiers : mhgp12_leaf_identity, mhgp12_mes_s et mhgp12_arena_selftest rendent le
   code 2 sur CHAQUE mutant sans une seule ligne de resultat (aucun noyau joue) ; avec un mutant place apres le
   vidage valide dans la meme commande, aucun noyau n'est joue non plus (admission de tous les vidages d'abord) ;
   le vidage valide donne l'identite des deux formes (code 0) et une ligne d'admission citant son empreinte.
   Avant correction, l'auditeur observait : six vidages admis, et identite de code 0 sur zero_k, profile_mismatch et
   coordinate_out_of_profile.
3. Option --vidages-reels DIR : les vidages reels (ng00..02, K5/16, K5/24, K10/24) sont tous admis par le lecteur
   durci ; seuls leurs comptes et empreintes sont publies (aucune coordonnee).

Usage : python3 -S -O tests/test_admission_m2.py --binaires <construction hote> [--vidages-reels DIR]
Bibliotheque standard ; aucune garde par assert. Codes : 0 conforme, 1 ecart, 2 usage.
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

AUDITEUR = ('site_out_of_cloud', 'wrapped_job_begin', 'zero_k', 'profile_mismatch', 'coordinate_out_of_profile',
            'short_record_population')


def jouer(cmd, timeout=600):
    proc = subprocess.run([str(c) for c in cmd], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                          timeout=timeout)
    lignes = [json.loads(l) for l in proc.stdout.splitlines() if l.startswith('{')]
    return proc.returncode, lignes, proc.stderr


def main(argv):
    binaires = reels = None
    i = 1
    while i < len(argv):
        if argv[i] == '--binaires' and i + 1 < len(argv):
            binaires = Path(argv[i + 1])
        elif argv[i] == '--vidages-reels' and i + 1 < len(argv):
            reels = Path(argv[i + 1])
        else:
            print(__doc__)
            return 2
        i += 2
    if binaires is None:
        print(__doc__)
        return 2
    outils = {n: binaires / n for n in ('mhgp12_dump_admission_selftest', 'mhgp12_leaf_identity', 'mhgp12_mes_s',
                                        'mhgp12_arena_selftest')}
    absents = [n for n, p in outils.items() if not p.is_file()]
    if absents:
        print(json.dumps({'porte': 'admission_m2', 'ecarts': ['binaires absents : %s' % absents]}))
        return 1
    ecarts, resultats = [], []
    with tempfile.TemporaryDirectory(prefix='admission-m2-') as tmp:
        tmp = Path(tmp)
        code, lignes, _ = jouer([outils['mhgp12_dump_admission_selftest'], tmp])
        cas = {l['cas']: l for l in lignes}
        mutants = [n for n in cas if n != 'valide']
        if code != 0 or cas.get('valide', {}).get('admis') is not True or len(mutants) < 40 or \
                any(cas[n]['admis'] is not False for n in mutants):
            ecarts.append('porte native : code %d, %d mutants' % (code, len(mutants)))
        for n in AUDITEUR:
            if cas.get(n, {}).get('admis') is not False:
                ecarts.append('injection de l\'auditeur admise : ' + n)
        resultats.append({'porte': 'dump_admission_selftest', 'code': code, 'mutants_refuses':
                          sum(cas[n]['admis'] is False for n in mutants), 'injections_auditeur_refusees':
                          sum(cas.get(n, {}).get('admis') is False for n in AUDITEUR)})
        valide = tmp / 'valide.bin'
        code, lignes, _ = jouer([outils['mhgp12_leaf_identity'], '--threads', '2', valide])
        formes = {l['form']: l for l in lignes if 'form' in l}
        if code != 0 or sorted(formes) != ['coherent', 'j3'] or not all(l['identity'] for l in formes.values()):
            ecarts.append('vidage valide : identite code %d' % code)
        code_adm, adm, _ = jouer([outils['mhgp12_leaf_identity'], '--admission', valide])
        if code_adm != 0 or len(adm) != 1 or adm[0].get('admis') is not True or \
                adm[0].get('dump_fnv1a') != cas['valide'].get('dump_fnv1a'):
            ecarts.append('admission du vidage valide : code %d' % code_adm)
        resultats.append({'cas': 'valide', 'identite_code': code, 'formes': sorted(formes),
                          'dump_fnv1a': cas['valide'].get('dump_fnv1a')})
        refus = {}
        for n in mutants:
            chemin = tmp / (n + '.bin')
            codes = []
            for outil, options in (('mhgp12_leaf_identity', ['--threads', '2']), ('mhgp12_mes_s', []),
                                   ('mhgp12_arena_selftest', [])):
                code, lignes, _ = jouer([outils[outil]] + options + [chemin])
                resultat = [l for l in lignes if 'form' in l or 'leaf' in l or 'leaves' in l or 'sites' in l]
                if code != 2 or resultat:
                    ecarts.append('%s sur %s : code %d, %d lignes de resultat' % (outil, n, code, len(resultat)))
                codes.append(code)
            refus[n] = codes
        resultats.append({'cas': 'mutants_rejoues_par_les_outils', 'mutants': len(refus),
                          'codes_identite_mes_s_arene': sorted({tuple(c) for c in refus.values()})})
        code, lignes, _ = jouer([outils['mhgp12_leaf_identity'], valide, tmp / 'zero_k.bin'])
        if code != 2 or any('form' in l for l in lignes):
            ecarts.append('valide puis mutant : un noyau a ete joue avant le refus (code %d)' % code)
        resultats.append({'cas': 'valide_puis_mutant_dans_une_commande', 'code': code,
                          'lignes_de_resultat': sum('form' in l for l in lignes)})
    if reels is not None:
        vidages = sorted(reels.glob('ng0*_k*_l*.bin'))
        code, lignes, _ = jouer([outils['mhgp12_leaf_identity'], '--admission'] + vidages, timeout=1800)
        if code != 0 or len(lignes) != len(vidages) or not all(l.get('admis') for l in lignes) or not vidages:
            ecarts.append('vidages reels : code %d, %d admis sur %d' % (code, sum(bool(l.get('admis')) for l in lignes),
                                                                       len(vidages)))
        resultats.append({'cas': 'vidages_reels_admis', 'code': code, 'vidages': len(vidages),
                          'comptes': [{k: l.get(k) for k in ('kmax', 'leaf_size', 'leaves', 'records', 'population',
                                                             'dump_fnv1a')} for l in lignes]})
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({'porte': 'admission_m2', 'cas': len(resultats), 'ecarts': ecarts, 'optimise': sys.flags.optimize},
                     ensure_ascii=False))
    return 0 if not ecarts else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
