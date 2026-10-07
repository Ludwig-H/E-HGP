#!/usr/bin/env python3
"""Porte du juge et du pilote de MES-M2 (scripts/g4_leaf_bench.py) : constats CST-0018 et CST-0215.

Rejoue, sans GPU ni donnee reelle, les injections du recu audit_socle_microbancs_20261007/preuves (probe_pilotes.py) :
les operations externes du pilote (construction, environnement, commandes) sont remplacees par des simulations ; main,
judge, la collecte et l'admission des prises restent ceux du script. Les vidages sont de petits fichiers MHGP12LF
synthetiques et valides (un site), ecrits ici avec leur empreinte FNV-1a. Attendus :
  - temoin complet (toutes les preuves presentes et fraiches) : « adopte » et choix j3 ;
  - identite hote en echec, prises perimees d'un autre vidage, aucun cas qui decide, durees non finies, auto-test
    d'arene en echec, banc de code 2 (les six cas de l'auditeur), et les autres preuves manquantes (sanitizers,
    isolation, jeton, empreinte, temoin, admission, options hors contrat, binaire modifie) : « refuse », jamais
    « adopte » ni « rejete », aucun choix ;
  - les trois preuves contradictoires du recu audit_cd_corrections_20261007/m2 (CST-0018) : prises Compute Sanitizer
    sans formes ni feuilles, echauffement de code 1 aux identites vraies, formes d'identite vraie aux compteurs de
    divergence non nuls et en debordement : « refuse » ; de meme, chacune seule, une prise Compute Sanitizer sans
    formes, une prise a la couverture fausse, un sanitizer de code 1 aux identites vraies, un temoin faux dans les
    seules prises du banc ou dans la seule prise d'echauffement ;
  - identite du banc en defaut, borne haute au-dela de 1/3, ou mediane declaree contraire aux durees brutes (le juge
    lit les durees brutes) avec toutes les preuves : « rejete ».
Puis, sur des sorties REELLES (recu G4 g4_t0a_20261007, 45 prises de 9 cas) : le juge pur redonne a l'identique les
verdicts, rapports et intervalles publies (statistique inchangee), et l'admission stricte refuse ces prises anciennes
comme preuves d'une nouvelle session (sans jeton).

Usage : python3 -S -O tests/test_juge_m2.py [--recu-g4 DOSSIER_m2]   (dossier par defaut : recu du depot)
Bibliotheque standard ; aucune garde par assert. Codes : 0 conforme, 1 ecart (detail en JSON), 2 usage.
"""
import contextlib
import importlib.util
import io
import json
import math
import os
import struct
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True  # aucune trace dans l'arbre des sources (hachees par le pilote)
ICI = Path(__file__).resolve().parent
MICROBANC = ICI.parent
SCRIPT = MICROBANC / 'scripts' / 'g4_leaf_bench.py'
RECU_G4 = MICROBANC.parents[1] / 'receipts' / 'g4_t0a_20261007' / 'resultats' / 'cmd' / '004_m2_publier' / 'files' / 'm2'
CAS_CONTRAT = ['%s_k%d_l%d' % (f, k, l) for f in ('ng00', 'ng01', 'ng02') for (k, l) in ((5, 16), (5, 24), (10, 24))]


class Ecart(Exception):
    pass


def exiger(condition, message):
    if not condition:
        raise Ecart(message)


def charger(nom):
    spec = importlib.util.spec_from_file_location(nom, SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fnv1a(data):
    h = 14695981039346656037
    for octet in data:
        h ^= octet
        h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


def section(octets):
    return octets + b'\0' * ((8 - len(octets) % 8) % 8)


def ecrire_vidage(chemin, kmax, feuille):
    """Vidage MHGP12LF v1 valide minimal : un site (1,1,1), une feuille de boite [0,2)^3, aucune emission."""
    tete = struct.pack('<8s15Q', b'MHGP12LF', 1, 21, kmax, feuille, 256, 3, 1, 1, 1, 0, 0, 15, 1, 0, 0)
    corps = tete
    for _ in range(3):
        corps += section(struct.pack('<I', 1))
    corps += section(struct.pack('<QII3q3q', 0, 1, 0, 0, 0, 0, 2, 2, 2))
    corps += section(struct.pack('<I', 0))
    compteurs = [0] * 15
    compteurs[1] = 1  # prefixes = m
    corps += section(struct.pack('<15I', *compteurs))
    corps += section(bytes([1]))
    corps += section(struct.pack('<2Q', 0, 0))
    corps += section(struct.pack('<2Q', 0, 0))
    corps += struct.pack('<Q', fnv1a(corps))
    Path(chemin).write_bytes(corps)


def valeur(cmd, option, defaut=None):
    return cmd[cmd.index(option) + 1] if option in cmd else defaut


def charge_banc(m, chemin_vidage, ident, formes, reps, warmup, jeton, scenario, feuilles=None):
    """Prise telle que le vrai banc l'ecrit : --leaves borne la couverture aux premieres feuilles (leaf_bench.cu) ;
    chaque forme publie ses compteurs, son arene et son debordement."""
    temps = scenario.get('ms', {})
    lignes = []
    for f in formes:
        ms = temps.get(f, 100.0 if f == 'witness' else 10.0)
        identite = scenario.get('identite_forme', {}).get(f, True)
        ligne = {'form': f, 'median_ms': ms, 'ms': [ms] * reps, 'identity': identite, 'unresolved': 0,
                 'mismatched_counts': 0, 'mismatched_emissions': 0, 'overflow': False,
                 'records': ident['n_records'], 'population': ident['n_population']}
        if scenario.get('compteurs_contradictoires'):  # identite vraie mais ecarts et debordement (auditeur)
            ligne.update(mismatched_counts=1, overflow=True)
        if scenario.get('mediane_fausse') and f == 'j3':  # durees brutes de 60 ms, mediane declaree de 6 ms
            ligne.update(ms=[60.0] * reps, median_ms=6.0)
        lignes.append(ligne)
    return {'bench': 'mhgp12_leaf_bench', 'nonce': scenario.get('jeton_banc', jeton), 'device': 'SYNTHETIQUE',
            'context_ms': 1.0, 'reps': reps, 'warmup': warmup,
            'cases': [{'dump': str(chemin_vidage), 'dump_fnv1a': scenario.get('fnv_banc', ident['fnv1a']),
                       'coord_bits': ident['coord_bits'], 'sites': ident['n_sites'], 'dump_leaves': ident['n_leaves'],
                       'kmax': ident['kmax'], 'leaf_size': ident['leaf_size'],
                       'leaves': ident['n_leaves'] if not feuilles else min(feuilles, ident['n_leaves']),
                       'reference_records': ident['n_records'], 'reference_population': ident['n_population'],
                       'forms': lignes}],
            'identity': all(x['identity'] for x in lignes)}


def simulation(m, scenario, journal):
    """Remplace Session.run : chaque commande du pilote rend la sortie d'un binaire conforme, sauf injection."""
    def run(self, name, cmd, timeout, cwd=None, env=None, capture=False):
        cmd = [str(c) for c in cmd]
        journal.append(name)
        code, sortie = 0, ''
        injection = scenario.get('commandes', {}).get(name)
        if name == 'git_head':
            sortie = 'f' * 40
        elif name.startswith('gpu_apps_'):
            sortie = scenario.get('gpu_apps', {}).get(name, '')
        elif name == 'admission_selftest':
            lignes = [{'cas': 'valide', 'admis': True}] + [{'cas': 'mutant_%d' % i, 'admis': False}
                                                             for i in range(46)]
            sortie = '\n'.join(json.dumps(x) for x in lignes)
        elif name == 'admission_vidages':
            lignes = []
            for chemin in cmd[2:]:
                ident = m.dump_identity(Path(chemin))
                refuse = Path(chemin).name in scenario.get('refus_admission', ())
                lignes.append({'dump': chemin, 'admis': not refuse, 'dump_fnv1a': ident['fnv1a']})
            code = 2 if any(not x['admis'] for x in lignes) else 0
            sortie = '\n'.join(json.dumps(x) for x in lignes)
        elif name == 'identity_host':
            formes = valeur(cmd, '--forms').split(',')
            chemins = [c for c in cmd[1:] if c.endswith('.bin')]
            lignes = []
            for chemin in chemins:
                ident = m.dump_identity(Path(chemin))
                for f in formes:
                    ok = not scenario.get('identite_hote_fausse')
                    lignes.append({'dump': chemin, 'dump_fnv1a': ident['fnv1a'], 'form': f, 'leaves': ident['n_leaves'],
                                   'dump_leaves': ident['n_leaves'], 'resolved': ident['n_leaves'], 'unresolved': 0,
                                   'mismatched_counts': 0, 'mismatched_emissions': 0 if ok else 1, 'identity': ok})
            code = 0 if not scenario.get('identite_hote_fausse') else 1
            sortie = '\n'.join(json.dumps(x) for x in lignes)
        elif name == 'arena_selftest':
            ident = m.dump_identity(Path(cmd[1]))
            sortie = json.dumps({'dump_fnv1a': ident['fnv1a'], 'leaves': 1, 'identity': True, 'mutants_vivants': 0})
        elif name == 'mes_s':
            sortie = ''
        elif name.startswith('sanitizer_') or name.startswith('bench_'):
            chemin = Path(valeur(cmd, '--dump'))
            cible = Path(valeur(cmd, '--json'))
            jeton = valeur(cmd, '--nonce')
            formes = valeur(cmd, '--forms').split(',')
            reps, warmup = int(valeur(cmd, '--reps')), int(valeur(cmd, '--warmup'))
            ident = m.dump_identity(chemin)
            feuilles = int(valeur(cmd, '--leaves')) if '--leaves' in cmd else None
            ecrire = not (name.startswith('bench_') and name != 'bench_discarded' and scenario.get('banc_sans_ecrire'))
            if ecrire:
                portee = dict(scenario)  # identites par forme limitees a l'echauffement ou aux prises du banc
                if name == 'bench_discarded' and 'identite_forme_echauffement' in scenario:
                    portee['identite_forme'] = scenario['identite_forme_echauffement']
                elif name.startswith('bench_ng') and 'identite_forme_banc' in scenario:
                    portee['identite_forme'] = scenario['identite_forme_banc']
                charge = charge_banc(m, chemin, ident, formes, reps, warmup, jeton, portee, feuilles)
                if scenario.get('ms_non_finies') and name.startswith('bench_ng'):
                    charge['cases'][0]['forms'][1]['ms'][0] = float('nan')
                if scenario.get('sanitizer_vide') and name.startswith('sanitizer_'):  # auditeur : ni formes ni feuilles
                    charge['cases'][0]['forms'] = []
                    charge['cases'][0]['leaves'] = 0
                if scenario.get('sanitizer_sans_formes') and name.startswith('sanitizer_'):  # formes seules absentes
                    charge['cases'][0]['forms'] = []
                if scenario.get('sanitizer_sans_feuilles') and name.startswith('sanitizer_'):  # couverture seule fausse
                    charge['cases'][0]['leaves'] = 0
                cible.write_text(json.dumps(charge))
                code = 0 if charge['identity'] else 1
            if name.startswith('bench_ng') and scenario.get('modifier_binaire'):
                binaire = Path(cmd[0])
                binaire.write_bytes(binaire.read_bytes() + b'!')
            sortie = '========= ERROR SUMMARY: 0 errors' if name.startswith('sanitizer_') else ''
        if injection is not None:
            code = injection
        self.steps.append({'step': name, 'code': code, 'seconds': 0.0})
        return code, sortie, ''
    return run


def jouer(scenario, options):
    """Joue main() du pilote avec ses frontieres externes simulees ; rend le rapport et le journal des commandes."""
    m = charger('g4_leaf_bench_' + scenario['nom'])
    journal = []
    with tempfile.TemporaryDirectory(prefix='juge-m2-') as tmp:
        tmp = Path(tmp)
        depot, vidages, construction, sortie = tmp / 'depot', tmp / 'vidages', tmp / 'b', tmp / 'out'
        (depot / 'morsehgp3D_v11' / 'src' / 'catalogue').mkdir(parents=True)
        (depot / 'morsehgp3D_v11' / 'src' / 'catalogue' / 'leaf.cpp').write_text('// synthetique\n')
        vidages.mkdir()
        construction.mkdir()
        for nom in m.REQUIRED_BINARIES + ('nvcc', 'compute-sanitizer'):
            if nom == 'compute-sanitizer' and scenario.get('sans_sanitizer'):
                continue
            (construction / nom).write_bytes(b'BINAIRE_SYNTHETIQUE_' + nom.encode())
        configs = valeur(options, '--configs', '5:16,5:24,10:24')
        trames = valeur(options, '--frames', 'ng00,ng01,ng02')
        for trame in trames.split(','):
            for config in configs.split(','):
                k, l = (int(x) for x in config.split(':'))
                kmax = scenario.get('kmax_faux', k)
                ecrire_vidage(vidages / ('%s_k%d_l%d.bin' % (trame, k, l)), kmax, l)
        if scenario.get('prises_perimees'):  # l'injection de l'auditeur : JSON d'un autre vidage, K10/16
            (sortie / 'runs').mkdir(parents=True)
            autre = vidages / 'autre_k10_l16.bin'
            ecrire_vidage(autre, 10, 16)
            ident = m.dump_identity(autre)
            for nom in [p.stem for p in vidages.glob('ng*.bin')]:
                for i in range(5):
                    charge = charge_banc(m, autre, ident, ['witness', 'j3'], 15, 3, 'ancien', {})
                    (sortie / 'runs' / ('%s_p%d.json' % (nom, i))).write_text(json.dumps(charge))
        m.Session.run = simulation(m, scenario, journal)
        m.environment = lambda *a: {'gpu_apps': scenario.get('gpu_apps_debut', ''), 'mock': 'operations simulees'}
        m.find_nvcc = lambda *a: str(construction / 'nvcc')
        m.build_feuille = lambda *a: construction
        argv = ['g4_leaf_bench.py', '--out', str(sortie), '--repo', str(depot), '--dumps', str(vidages),
                '--forms', 'witness,j3', '--processes', '5', '--jobs', '2'] + options
        ancien = sys.argv
        sys.argv = argv
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                code = m.main()
        finally:
            sys.argv = ancien
        rapport = json.loads((sortie / 'report.json').read_text())
    return code, rapport, journal


INJECTIONS = [
    # (nom, scenario, options, verdict attendu de j3) ; les six premiers sont ceux de l'auditeur.
    ('temoin_complet', {}, [], 'adopte'),
    ('identite_hote_en_echec', {'identite_hote_fausse': True}, [], 'refuse'),
    ('prises_perimees_autre_vidage', {'prises_perimees': True, 'banc_sans_ecrire': True}, [], 'refuse'),
    ('aucun_cas_qui_decide', {}, ['--configs', '5:16'], 'refuse'),
    ('arene_en_echec', {'commandes': {'arena_selftest': 1}}, [], 'refuse'),
    ('banc_code_2', {'commandes': {'bench_ng00_k5_l24_p2': 2}}, [], 'refuse'),
    ('durees_non_finies_dans_la_prise', {'ms_non_finies': True}, [], 'refuse'),
    ('sanitizer_introuvable', {'sans_sanitizer': True}, [], 'refuse'),
    ('sanitizer_en_echec', {'commandes': {'sanitizer_racecheck': 9}}, [], 'refuse'),
    ('isolation_apres_banc', {'gpu_apps': {'gpu_apps_apres_banc': '4242, intrus, 10 MiB'}}, [], 'refuse'),
    ('isolation_au_debut', {'gpu_apps_debut': '4242, intrus, 10 MiB'}, [], 'refuse'),
    ('moins_de_cinq_processus', {}, ['--processes', '3'], 'refuse'),
    ('jeton_different', {'jeton_banc': 'autre-session'}, [], 'refuse'),
    ('empreinte_differente', {'fnv_banc': '0123456789abcdef'}, [], 'refuse'),
    ('temoin_faux', {'identite_forme': {'witness': False}}, [], 'refuse'),
    ('admission_refusee', {'refus_admission': ('ng01_k10_l24.bin',)}, [], 'refuse'),
    ('vidage_hors_cas', {'kmax_faux': 4}, [], 'refuse'),
    ('identite_non_jouee', {}, ['--skip-identity'], 'refuse'),
    ('sanitizers_non_joues', {}, ['--skip-sanitizer'], 'refuse'),
    ('sans_cuda', {}, ['--no-cuda'], 'refuse'),
    ('trames_hors_contrat', {}, ['--frames', 'ng00,ng01'], 'refuse'),
    ('binaire_modifie_pendant_la_session', {'modifier_binaire': True}, [], 'refuse'),
    ('admission_selftest_en_echec', {'commandes': {'admission_selftest': 3}}, [], 'refuse'),
    # Recu audit_cd_corrections_20261007/m2 : trois preuves contradictoires (CST-0018), puis le controle positif.
    ('auditeur_sanitizer_sans_formes_ni_feuilles', {'sanitizer_vide': True}, [], 'refuse'),
    ('auditeur_echauffement_code_1', {'commandes': {'bench_discarded': 1}}, [], 'refuse'),
    ('auditeur_compteurs_contradictoires', {'compteurs_contradictoires': True}, [], 'refuse'),
    ('auditeur_mediane_declaree_contraire', {'mediane_fausse': True}, [], 'rejete'),
    # Chaque garde seule (mutants de outils/mutants_juges.py) : formes seules, couverture seule, code du sanitizer.
    ('sanitizer_sans_formes', {'sanitizer_sans_formes': True}, [], 'refuse'),
    ('sanitizer_sans_feuilles', {'sanitizer_sans_feuilles': True}, [], 'refuse'),
    ('sanitizer_code_1_identite_vraie', {'commandes': {'sanitizer_memcheck': 1}}, [], 'refuse'),
    ('temoin_faux_hors_echauffement', {'identite_forme_banc': {'witness': False}}, [], 'refuse'),
    ('temoin_faux_a_l_echauffement', {'identite_forme_echauffement': {'witness': False}}, [], 'refuse'),
    ('identite_du_banc_en_defaut', {'identite_forme': {'j3': False}}, [], 'rejete'),
    ('borne_haute_au_dela_du_tiers', {'ms': {'j3': 50.0}}, [], 'rejete'),
]


def porte_injections():
    resultats = []
    for nom, scenario, options, attendu in INJECTIONS:
        scenario = dict(scenario, nom=nom)
        code, rapport, journal = jouer(scenario, options)
        v = rapport['verdicts']['j3']
        ligne = {'cas': nom, 'code': code, 'verdict': v['verdict'], 'attendu': attendu, 'choix': rapport['choice'],
                 'refus': len(rapport['refusals'])}
        exiger(code == 0, '%s : code %d du pilote' % (nom, code))
        exiger(v['verdict'] == attendu, '%s : verdict %s, attendu %s (%s)' % (nom, v['verdict'], attendu,
                                                                          v.get('refused')))
        if attendu == 'adopte':
            exiger(rapport['choice'] == 'j3' and not rapport['refusals'] and rapport['gpu_isolation'] is True,
                   '%s : choix ou preuves' % nom)
            exiger(abs(v['ratio_gm_all_cases'] - 0.1) < 1e-12, '%s : rapport %r' % (nom, v['ratio_gm_all_cases']))
            exiger(len(rapport['evidence']['runs']) == 45 and len(rapport['evidence']['dumps']) == 9,
                   '%s : preuves citees incompletes' % nom)
        else:
            exiger(rapport['choice'] is None, '%s : un choix malgre le verdict %s' % (nom, v['verdict']))
        if attendu == 'refuse':
            exiger(v['refused'], '%s : refus sans raison' % nom)
        if nom == 'admission_refusee':
            exiger(not any(x.startswith('bench_') or x == 'identity_host' for x in journal),
                   'admission refusee : un noyau a ete joue (%s)' % journal)
        if nom == 'prises_perimees_autre_vidage':
            raisons = ' '.join(rapport['refusals'])
            exiger('fichier de prise absent' in raisons, 'prises perimees : raison %s' % raisons)
        resultats.append(ligne)
    return resultats


def porte_juge_pur():
    """Injection de l'auditeur au niveau du juge : cinq processus aux durees NaN ; puis un cas sans processus."""
    m = charger('g4_leaf_bench_juge_pur')
    forme = lambda nom, ms: {'form': nom, 'ms': ms, 'identity': True, 'unresolved': 0}
    run = {'forms': {'witness': forme('witness', [100.0] * 15), 'j3': forme('j3', [float('nan')])},
           'witness_identity': True}
    verdicts, choix = m.judge({'synthetic_k5_l24': {'runs': [run] * 5}}, ['witness', 'j3'], 5)
    exiger(verdicts['j3']['verdict'] == 'refuse' and choix is None, 'NaN : %s' % verdicts['j3'])
    complet = {'forms': {'witness': forme('witness', [100.0] * 15), 'j3': forme('j3', [10.0] * 15)},
               'witness_identity': True}
    verdicts, choix = m.judge({'ng00_k5_l24': {'runs': [complet] * 5}}, ['witness', 'j3'], 5,
                              expected=['ng00_k5_l24', 'ng00_k10_l24'])
    exiger(verdicts['j3']['verdict'] == 'refuse' and choix is None, 'cas attendu absent : %s' % verdicts['j3'])
    verdicts, choix = m.judge({'ng00_k5_l16': {'runs': [complet] * 5}}, ['witness', 'j3'], 5,
                              expected=['ng00_k5_l16'])
    exiger(verdicts['j3']['verdict'] == 'refuse', 'aucun cas qui decide : %s' % verdicts['j3'])
    return [{'cas': 'juge_durees_nan', 'verdict': 'refuse'}, {'cas': 'juge_cas_attendu_absent', 'verdict': 'refuse'},
            {'cas': 'juge_aucun_cas_qui_decide', 'verdict': 'refuse'}]


def porte_recu_reel(dossier):
    """Sorties reelles du recu G4 g4_t0a_20261007 : 45 prises de 9 cas (5 processus), report.json publie."""
    m = charger('g4_leaf_bench_recu')
    rapport = json.loads((dossier / 'report.json').read_text())
    formes = rapport['args']['forms'].split(',')
    cas = {}
    for chemin in sorted((dossier / 'runs').glob('ng*_p*.json')):
        nom, processus = chemin.stem.rsplit('_p', 1)
        prise = json.loads(chemin.read_text())
        exiger(len(prise['cases']) == 1, '%s : un cas par prise' % chemin.name)
        lignes = {f['form']: f for f in prise['cases'][0]['forms']}
        exiger(list(lignes) == formes, '%s : formes' % chemin.name)
        cas.setdefault(nom, {'runs': []})['runs'].append(
            {'process': int(processus), 'forms': lignes, 'witness_identity': lignes['witness']['identity'] is True})
    exiger(sorted(cas) == sorted(CAS_CONTRAT) and all(len(c['runs']) == 5 for c in cas.values()),
           'recu : 9 cas de 5 processus attendus')
    verdicts, choix = m.judge(cas, formes, 5, ((5, 24), (10, 24)), CAS_CONTRAT)
    publies = rapport['verdicts']
    # Python 3.10 (celui de la VM qui a juge) : egalite a l'octet. Depuis 3.12, sum() des flottants est compensee :
    # memes verdicts, ecarts au dernier bit seulement (tolerance relative 1e-12).
    exact = sys.version_info < (3, 12)
    egal = (lambda a, b: a == b) if exact else (lambda a, b: abs(a - b) <= 1e-12 * max(1.0, abs(b)))
    for f in formes[1:]:
        exiger(verdicts[f]['verdict'] == publies[f]['verdict'], '%s : verdict %s, publie %s' % (
            f, verdicts[f]['verdict'], publies[f]['verdict']))
        exiger(egal(verdicts[f]['ratio_gm_all_cases'], publies[f]['ratio_gm_all_cases']), '%s : rapport global' % f)
        for nom, valeurs in publies[f]['cases'].items():
            calcule = verdicts[f]['cases'][nom]
            exiger(all(egal(a, b) for a, b in zip(calcule['ci95'], valeurs['ci95'])) and
                   egal(calcule['ratio_gm'], valeurs['ratio_gm']), '%s %s : intervalle' % (f, nom))
    exiger(choix == rapport['choice'] == 'j3_r168', 'choix %s' % choix)
    # Regles de coherence des compteurs (CST-0018) sur les vraies prises : toutes les formes des 45 prises et des trois
    # prises Compute Sanitizer (premieres feuilles, formes warp seulement) les respectent ; aucune vraie prise refusee.
    contradictions, formes_jugees = [], 0
    sanitizers = [json.loads((dossier / ('sanitizer_%s.json' % t)).read_text()) for t in ('memcheck', 'racecheck',
                                                                                         'synccheck')]
    for prise in [json.loads(c.read_text()) for c in sorted((dossier / 'runs').glob('ng*_p*.json'))] + sanitizers:
        for c in prise['cases']:
            for ligne in c['forms']:
                formes_jugees += 1
                raison = m.form_counters_consistent(ligne, c['reference_records'], c['reference_population'])
                if raison is not None:
                    contradictions.append('%s %s : %s' % (c['dump'], ligne['form'], raison))
    exiger(not contradictions and formes_jugees == 45 * len(formes) + 3 * (len(formes) - 1),
           'compteurs des vraies prises : %d formes, %s' % (formes_jugees, contradictions[:3]))
    exiger(all(s['cases'][0]['leaves'] == rapport['args']['sanitizer_leaves'] and
               [f['form'] for f in s['cases'][0]['forms']] == formes[1:] for s in sanitizers),
           'couverture ou formes des prises Compute Sanitizer reelles')
    # Ces prises anciennes ne sont pas des preuves d'une nouvelle session : sans jeton, l'admission les refuse.
    ident = {'fnv1a': None, 'coord_bits': 21, 'kmax': 5, 'leaf_size': 24, 'n_leaves': 123581, 'n_sites': None,
             'n_records': 1306696, 'n_population': 6097121}
    prise, raison = m.check_bench_json(dossier / 'runs' / 'ng00_k5_l24_p0.json', 'jeton-de-cette-session',
                                       Path('ng00_k5_l24.bin'), ident, formes, 15, 3)
    exiger(prise is None and 'jeton' in raison, 'prise ancienne admise : %s' % raison)
    pire = {f: max(c['ci95'][1] for c in verdicts[f]['cases'].values() if c['deciding']) for f in formes[1:]}
    return [{'cas': 'recu_g4_t0a_rejuge', 'prises': sum(len(c['runs']) for c in cas.values()),
             'choix': choix, 'verdicts': {f: verdicts[f]['verdict'] for f in formes[1:]},
             'moyenne_geometrique_j3_r168': round(verdicts['j3_r168']['ratio_gm_all_cases'], 6),
             'pire_borne_haute_j3_r168': round(pire['j3_r168'], 6),
             'identique_au_publie': 'a l\'octet' if exact else 'a 1e-12 pres (sum() compensee de Python >= 3.12)'},
            {'cas': 'recu_g4_t0a_compteurs_coherents', 'formes': formes_jugees, 'contradictions': 0},
            {'cas': 'recu_g4_t0a_prise_ancienne_refusee', 'raison': raison}]


def main(argv):
    dossier = RECU_G4
    if len(argv) == 3 and argv[1] == '--recu-g4':
        dossier = Path(argv[2])
    elif len(argv) != 1:
        print(__doc__)
        return 2
    resultats, ecarts = [], []
    for porte in (porte_injections, porte_juge_pur):
        try:
            resultats += porte()
        except Ecart as e:
            ecarts.append(str(e))
    if (dossier / 'report.json').is_file():
        try:
            resultats += porte_recu_reel(dossier)
        except Ecart as e:
            ecarts.append(str(e))
    else:
        ecarts.append('recu G4 absent : %s (preuve positive non jouee)' % dossier)
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({'porte': 'juge_m2', 'cas': len(resultats), 'ecarts': ecarts, 'optimise': sys.flags.optimize,
                      'python': sys.version.split()[0]}, ensure_ascii=False))
    return 0 if not ecarts else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
