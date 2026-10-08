#!/usr/bin/env python3
"""Témoins synthétiques JSON ; aucun moteur. Compare aussi les statistiques au pilote épinglé."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import reader as r


def encoded(rows):
    return b''.join(json.dumps(row, separators=(',', ':')).encode() + b'\n' for row in rows)


def rows_for(spec, sites):
    rows = [{'phase': 'open', 'status': 'ok', 'reason': 'none', 'wall_ns': 500}] if spec['device'] else []
    input_bytes = 16 * sum(sites[name] for name in set(spec['names']))
    for i, name in enumerate(spec['names']):
        # FUL1 fictif, distinct par nom/K, egal entre voies. Un max brut >100ms est volontaire.
        digest = hashlib.sha256((name + str(spec['kmax'])).encode()).hexdigest()
        row = dict(phase='full', **{'pass': i}, trame=name[-23:], voie='device' if spec['device'] else 'cpu',
                   status='ok', coord_bits=21, kmax=spec['kmax'], threads=48, sites=sites[name],
                   wall_ns=200_000_000 if i == 1 and len(spec['names']) == 10 else 80_000_000 + 100*i,
                   etapes_ns=dict(P=100, C=200, G=100, raccord=10, TMVR=400, T=100, M=100, V=100, R=100),
                   c_ns={k: 100 for k in r.CAT}, g_ns=dict(tables=10, resolution=20),
                   hors_mur_ns=dict(validation=100, empreinte=100), pic_octets=input_bytes+1000+i, full_sha256=digest)
        rows.extend([row, dict(phase='liberation', **{'pass': i}, liberation_ns=50)])
    rows.append(dict(phase='exit', status='ok', reason='none'))
    return rows


def main():
    meta = json.loads(Path(__file__).with_name('capture.json').read_text())
    cohort, ng = meta['cohort'], meta['ng_sites']
    specs = r.specs(cohort)
    sites = dict(ng, **{x['name']: x['sites'] for x in cohort})
    raws = {name: encoded(rows_for(spec, sites)) for name, spec in specs.items()}
    expected = r.reconstruct(raws, cohort, ng)
    r.require(expected['verdict'] == 'tenu', 'temoin conforme')
    r.require(expected['statistiques']['k5_appareil']['ng00']['max_ns'] > r.BUDGET, 'max brut informatif')
    options = dict(fils=48, processus=5, passes=10, jobs=44, delai=900, essai=False, sonde=None,
                   **{k: 'synthetic' for k in ('src', 'travail', 'donnees', 'sortie', 'archive_v12set')})
    env = dict(gpu_apps='', gpu='synthetic GPU', cmake='synthetic cmake', nvcc='synthetic nvcc')
    report = dict(mesure='MES-FULL', budget_ns=r.BUDGET, options=options, refus=[],
                  environnement_avant=env, environnement_apres=env, duree_s=1.0, **expected)
    r.require(r.review(report, expected)['verdict_independant'] == 'tenu', 'rapport conforme')
    killed = []
    filename = 'k5_ng00_r0.jsonl'
    source_rows = rows_for(specs[filename], sites)
    def bad(name, change):
        rows = copy.deepcopy(source_rows)
        change(rows)
        candidate = dict(raws, **{filename: encoded(rows)})
        try:
            r.reconstruct(candidate, cohort, ng)
        except r.Refusal:
            killed.append(name)
            return
        raise RuntimeError('mutation admise : ' + name)
    for name, key, value in (('mur_negatif', 'wall_ns', -1), ('mur_bool', 'wall_ns', True),
                             ('mur_float', 'wall_ns', 80.0), ('mur_u64_depasse', 'wall_ns', 1 << 64),
                             ('mauvaise_trame', 'trame', 'ng01'), ('mauvais_profil', 'coord_bits', 18),
                             ('mauvais_fils', 'threads', 1), ('mauvais_k', 'kmax', 10),
                             ('mauvaise_voie', 'voie', 'cpu'), ('mauvais_sites', 'sites', 1),
                             ('sha_non_hex', 'full_sha256', 'z'*64), ('statut_refus', 'status', 'refused'),
                             ('ordre_pass_bool', 'pass', False), ('diagnostics_scalaire', 'c_ns', 7),
                             ('pic_negatif', 'pic_octets', -1)):
        bad(name, lambda rows, k=key, v=value: rows[1].__setitem__(k, v))
    bad('etages_absents', lambda rows: rows[1]['etapes_ns'].pop('P'))
    bad('etages_hors_mur', lambda rows: rows[1]['etapes_ns'].__setitem__('P', 300_000_000))
    bad('tmvr_hors_enveloppe', lambda rows: rows[1]['etapes_ns'].__setitem__('R', 401))
    bad('table_g_hors_enveloppe', lambda rows: rows[1]['g_ns'].__setitem__('tables', 101))
    bad('cumul_g_hors_enveloppe', lambda rows: rows[1].__setitem__('g_ns', dict(tables=60, resolution=41)))
    bad('pic_nul', lambda rows: rows[1].__setitem__('pic_octets', 0))
    bad('pic_sous_entree', lambda rows: rows[1].__setitem__('pic_octets', 16*sites['ng00']-1))
    bad('liberation_absente', lambda rows: rows.pop(2))
    bad('liberation_mauvaise_passe', lambda rows: rows[2].__setitem__('pass', 3))
    bad('liberation_negative', lambda rows: rows[2].__setitem__('liberation_ns', -1))
    bad('open_absent', lambda rows: rows.pop(0))
    bad('open_refuse', lambda rows: rows[0].__setitem__('reason', 'device_unavailable'))
    bad('exit_absent', lambda rows: rows.pop())
    bad('empreinte_divergente', lambda rows: rows[1].__setitem__('full_sha256', '0'*64))
    for name, raw in (
            ('null', b'null\n'+raws[filename]), ('liste', b'[]\n'+raws[filename]),
            ('ligne_vide', b'\n'+raws[filename]), ('texte', b'garbage\n'+raws[filename]),
            ('utf8_invalide', b'\xff\n'+raws[filename]),
            ('cle_dupliquee', raws[filename].replace(b'"wall_ns":500', b'"wall_ns":0,"wall_ns":500', 1)),
            ('nan', raws[filename].replace(b'"wall_ns":500', b'"wall_ns":NaN', 1))):
        try:
            r.reconstruct(dict(raws, **{filename: raw}), cohort, ng)
        except r.Refusal:
            killed.append(name)
        else:
            raise RuntimeError('contreflux admis : ' + name)
    for name, candidate, cases in (
            ('processus_absent', {k:v for k,v in raws.items() if k != filename}, cohort),
            ('processus_extra', dict(raws, extra=raws[filename]), cohort),
            ('cohorte_36', raws, cohort[:-1]),
            ('cohorte_dupliquee', raws, cohort[:-1]+cohort[:1]),
            ('collision_etiquette', raws, cohort[:-1]+[dict(name='prefix_'+cohort[0]['name'].rjust(23, 'x'), sites=1)])):
        # Pour la collision, forcer egalite des deux derniers 23 caracteres.
        if name == 'collision_etiquette':
            cases = copy.deepcopy(cohort)
            cases[0]['name'] = 'a' + 'x'*23
            cases[1]['name'] = 'b' + 'x'*23
        try:
            r.reconstruct(candidate, cases, ng)
        except r.Refusal:
            killed.append(name)
        else:
            raise RuntimeError('cohorte admise : ' + name)
    for name, change in (
            ('gpu_avant_inconnu', lambda x: x['environnement_avant'].__setitem__('gpu_apps', None)),
            ('gpu_apres_occupe', lambda x: x['environnement_apres'].__setitem__('gpu_apps', '123')),
            ('gpu_identite_absente', lambda x: x['environnement_avant'].__setitem__('gpu', None)),
            ('configuration_differente', lambda x: x['options'].__setitem__('passes', 11)),
            ('configuration_float', lambda x: x['options'].__setitem__('fils', 48.0)),
            ('essai', lambda x: x['options'].__setitem__('essai', True)),
            ('refus_non_vide', lambda x: x['refus'].append('code externe non nul')),
            ('statistiques_falsifiees', lambda x: x['statistiques']['k5_cpu']['ng00'].__setitem__('valeurs', 1)),
            ('tenu_entier', lambda x: x['contrat']['ng00_02'].__setitem__('tenu', 1)),
            ('verdict_falsifie', lambda x: x.__setitem__('verdict', 'non tenu'))):
        candidate = copy.deepcopy(report)
        change(candidate)
        r.require(r.review(candidate, expected)['verdict_independant'] == 'refuse', 'rapport mutation : '+name)
        killed.append(name)
    codes = {name: 0 for name in raws}
    r.require(r.review(report, expected, codes)['verdict_independant'] == 'tenu', 'codes externes conformes')
    codes[filename] = 2
    r.require(r.review(report, expected, codes)['verdict_independant'] == 'refuse', 'code externe non nul')
    killed.append('code_externe_non_nul')
    sequence = rows_for(specs['v12set_r0.jsonl'], sites)
    sequence[1]['pic_octets'] = 16 * sum(sites[name] for name in set(specs['v12set_r0.jsonl']['names'])) - 1
    try:
        r.reconstruct(dict(raws, **{'v12set_r0.jsonl': encoded(sequence)}), cohort, ng)
    except r.Refusal:
        killed.append('pic_sous_somme_37_entrees')
    else:
        raise RuntimeError('pic sequence admis sous ses entrees residentes')
    # Le maximum brut reste diagnostique ; un depassement de la mediane de processus rejette le contrat.
    slow = copy.deepcopy(source_rows)
    for row in slow:
        if row.get('phase') == 'full':
            row['wall_ns'] = 100_000_001
    slower = r.reconstruct(dict(raws, **{filename: encoded(slow)}), cohort, ng)
    r.require(slower['verdict'] == 'non tenu', 'maximum medianes respecte')
    # Comparaison de formules a la source du pilote, sans en appeler run/build/main.
    pin = meta['sources'][1]
    body = subprocess.check_output(['git', 'show', pin['commit']+':'+pin['path']])
    r.require(r.sha(body) == pin['sha256'], 'pilote pin')
    module = {'__name__': 'audit_import_only'}
    exec(compile(body, 'pilote_epingle', 'exec'), module)
    for group, stats in expected['statistiques'].items():
        groups = {}
        for filename, spec in specs.items():
            if spec['group'] != group:
                continue
            rows = module['parse_process'](0, raws[filename].decode(), len(spec['names']), spec['kmax'], spec['device'])[0]
            if filename.startswith('v12set_'):
                for i, name in enumerate(spec['names'][:37]):
                    groups.setdefault(name, []).append([rows[i], rows[i+37]])
            else:
                groups.setdefault(spec['names'][0], []).append(rows)
        for name, runs in groups.items():
            r.require(r.same(module['frame_stats'](runs, 1), stats[name]), 'statistiques contre pilote')
    r.require(r.same(module['contract'](expected['statistiques']['k5_appareil']), expected['contrat']['ng00_02']), 'contrat')
    output = dict(synthetic_only=True, native_executed=False, processes=len(raws), passes=sum(len(s['names']) for s in specs.values()),
                  warm_passes=sum(v['valeurs'] for g in expected['statistiques'].values() for v in g.values()),
                  positive='tenu', max_brut_gt_100ms_admis=True, max_mediane_processus_gt_100ms='non tenu',
                  statistiques_identiques_pilote=True, mutations_refusees=sorted(killed), mutations=len(killed))
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
