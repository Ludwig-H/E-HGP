#!/usr/bin/env python3
"""Porte du juge et du pilote de MES-M5 (scripts/g4_traversal_bench.py) : constat CST-0018 de l'auditeur Codex.

Rejoue, sans GPU ni donnee reelle, les injections du recu audit_b_m5_20261007/juge_format (runner_probe.py) et les autres
preuves exigees : le VRAI main du pilote tourne dans une copie jetable du microbanc (et de MES-M2) ; seules ses
frontieres externes sont simulees (commandes de Session.run, environnement, nvcc, vidages en parallele). Les vidages
sont de petits fichiers MHGP12TR VALIDES, ecrits par l'oracle Python du microbanc (oracle_parcours.walk + encode) sur
des nuages synthetiques ; les prises simulees citent le jeton, les chemins et les comptes que leur commande demande,
sauf injection. Attendus :
  - temoin complet (toutes les preuves presentes et fraiches) : « adopte » ;
  - les six injections de l'auditeur (grille reduite a un cas et un processus, cinq fixtures manquantes, identite hote
    de code 2, fixtures appareil de code 7, une seule passe v11, mediane GPU contraire aux durees brutes) et les autres
    preuves manquantes, perimees ou incoherentes : « refuse », jamais « adopte » ni « rejete » ;
  - banc lent et honnete, identite appareil en defaut (code 1 coherent) : « rejete » ;
  - auto-test du juge pur (--selftest-judge) : conforme ;
  - sorties REELLES de la session G4 C (recu g4_t0c_20261007) : le juge durci redonne « adopte » et les statistiques
    publiees (a l'octet sous Python < 3.12, celui de la VM ; a 1e-12 pres au-dela) ; exigees comme preuves d'une
    NOUVELLE session (jeton), ces prises anciennes sont refusees.
Usage : python3 -S -O tests/test_juge_m5.py [--recu-g4 DOSSIER_m5]   (dossier par defaut : recu du depot)
Bibliotheque standard ; aucune garde par assert. Codes : 0 conforme, 1 ecart (detail en JSON), 2 usage.
"""
import contextlib
import importlib.util
import io
import json
import shutil
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True
ICI = Path(__file__).resolve().parent
MICROBANC = ICI.parent
M2 = MICROBANC.parent / 'mes_m2_feuille'
RECU_G4 = MICROBANC.parents[1] / 'receipts' / 'g4_t0c_20261007' / 'resultats' / 'cmd' / '001_m5' / 'files' / 'm5'
FIXTURES = {'coquille24_k2_l16': (21, 2, 16), 'coquille48_k5_l24': (21, 5, 24), 'coquille48_k5_l8_m8': (21, 5, 8),
            'coquille48_u32_k5_l24': (32, 5, 24), 'bord_u32_k2_l5': (32, 2, 5), 'uniforme_u32_k3_l8': (32, 3, 8)}


class Ecart(Exception):
  pass


def exiger(condition, message):
  if not condition:
    raise Ecart(message)


def charger(chemin, nom):
  spec = importlib.util.spec_from_file_location(nom, str(chemin))
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


def valeur(cmd, option, defaut=None):
  return cmd[cmd.index(option) + 1] if option in cmd else defaut


def nuage(n, graine, bits=21):
  """Sites distincts deterministes (u32), dans [0, 2^bits)."""
  pts, x = [], graine * 7919 + 1
  while len(pts) < n:
    x = (x * 6364136223846793005 + 1442695040888963407) & ((1 << 64) - 1)
    p = ((x >> 5) % (1 << bits), (x >> 21) % (1 << bits), (x >> 37) % (1 << bits))
    if p not in pts:
      pts.append(p)
  return pts


VIDAGES = {}  # vidages deterministes, calcules une fois par l'oracle et reecrits a chaque scenario


def ecrire_vidage(oracle, chemin, pts, kmax, feuille, bits, max_leaf=256, kmax_entete=None):
  cle = (tuple(pts), kmax, feuille, bits, max_leaf, kmax_entete)
  if cle not in VIDAGES:
    xs, ys, zs = [p[0] for p in pts], [p[1] for p in pts], [p[2] for p in pts]
    statut, noeuds, feuilles, sites, livre = oracle.walk(xs, ys, zs, kmax, feuille, max_leaf, bits)
    tete = {'producer': 1, 'coord_bits': bits, 'kmax': kmax if kmax_entete is None else kmax_entete,
            'leaf_size': feuille, 'max_leaf': max_leaf, 'status': statut, 'ledger': livre}
    VIDAGES[cle] = (oracle.encode(tete, xs, ys, zs, noeuds, feuilles, sites), statut, livre)
  octets, statut, livre = VIDAGES[cle]
  Path(chemin).write_bytes(octets)
  return statut, dict(livre)


def prise_banc(m, chemins, jeton, reps, warmup, scenario, nom_etape):
  """JSON du banc CUDA pour ces vidages, tel que le vrai banc l'ecrit (sauf injection)."""
  cas = []
  for chemin in chemins:
    ident = m.dump_identity(Path(chemin))
    brut = scenario.get('gpu_brut', 6.0)
    declare = scenario.get('gpu_declare', brut)
    identite = not (scenario.get('identite_appareil_fausse') and nom_etape.startswith('gpu_ng01_k5_l24'))
    c = {'dump': str(chemin) + ('.wrong' if scenario.get('vidage_errone') and nom_etape.startswith('gpu_') else ''),
         'kmax': ident['kmax'], 'leaf_size': ident['leaf_size'], 'coord_bits': ident['coord_bits'],
         'sites': ident['n_sites'], 'identity': identite, 'total_ms': [brut] * reps, 'resident_ms': [5.0] * reps,
         'median_ms': {'total': declare, 'resident': 5.0}, 'timed_allocations': scenario.get('reservations', 0),
         'mutants': {mu: {'killed': True} for mu in m.MUTANTS}, 'profile': {'kernels_ms': 1.0}}
    cas.append(c)
  return {'bench': 'mhgp12_traversal_bench', 'nonce': scenario.get('jeton_banc', jeton), 'device': 'SYNTHETIQUE',
          'reps': reps, 'warmup': warmup, 'cases': cas, 'identity': all(c['identity'] for c in cas)}


def simulation(m, oracle, scenario, journal, bdir, travail):
  """Remplace Session.run : chaque commande rend la sortie d'un outil conforme, sauf injection."""
  def run(self, name, cmd, timeout, cwd=None, env=None, capture=False):
    cmd = [str(c) for c in cmd]
    journal.append(name)
    code, sortie = 0, ''
    if name.startswith('gpu_apps_'):
      sortie = scenario.get('gpu_apps', {}).get(name, '')
    elif name == 'm5_build':
      bdir.mkdir(parents=True, exist_ok=True)
      for outil in m.REQUIRED_BINARIES + ('mhgp12_traversal_bench',):
        (bdir / outil).write_bytes(b'BINAIRE_SYNTHETIQUE_' + outil.encode())
    elif name == 'format_selftest':
      lignes = [{'cas': 'valide_%d' % i, 'attendu': True, 'admis': True} for i in range(4)]
      lignes += [{'cas': 'invalide_%d' % i, 'attendu': False, 'admis': False} for i in range(17)]
      sortie = '\n'.join(json.dumps(x) for x in lignes) + '\n{"porte":"format_m5","cas":21,"ecarts":0}'
    elif name == 'fixtures':
      dossier = Path(valeur(cmd, '--out'))
      dossier.mkdir(parents=True, exist_ok=True)
      entrees = []
      for i, (nom, (bits, kmax, feuille)) in enumerate(FIXTURES.items()):
        if scenario.get('fixtures_manquantes') and nom != 'coquille48_u32_k5_l24':
          continue
        pts = nuage(20 + i, 100 + i, bits)
        statut, livre = ecrire_vidage(oracle, dossier / (nom + '.bin'), pts, kmax, feuille, bits,
                                      max_leaf=feuille if nom.endswith('_m8') else 256)
        ident = m.dump_identity(dossier / (nom + '.bin'))
        entrees.append({'name': nom, 'bits': bits, 'kmax': kmax, 'leaf': feuille, 'sites': len(pts), 'present': True,
                        'controls_ok': True, 'dump_sha256': ident['sha256'],
                        'oracle_check': {'status': statut, 'ledger': livre, 'identity': True}})
      (dossier / 'fixtures.json').write_text(json.dumps({'schema': 'ehgp.v12.mes_m5.fixtures.v1',
                                                         'nonce': scenario.get('jeton_fixtures', valeur(cmd, '--nonce')),
                                                         'fixtures': entrees}))
    elif name == 'identity_host':
      cible, jeton = Path(valeur(cmd, '--json')), valeur(cmd, '--nonce')
      chemins = [c for c in cmd if c.endswith('.bin')]
      unite = {'phase': 'unit', 'nonce': jeton, 'ok': True}
      if not scenario.get('sans_temoin_cst0222'):
        unite['emit_tasks_u64'] = True
      lignes = [unite]
      for chemin in chemins:
        ident = m.dump_identity(Path(chemin))
        livre = {'nodes': ident['nodes'], 'leaves': ident['leaves'], 'filter_tests': ident['filter_tests'],
                 'max_depth': ident['max_depth'], 'max_leaf': ident['max_leaf_seen']}
        lignes.append({'phase': 'identity', 'nonce': jeton, 'dump': chemin, 'kmax': ident['kmax'],
                       'leaf_size': ident['leaf_size'], 'coord_bits': ident['coord_bits'], 'sites': ident['n_sites'],
                       'identity': True, 'status': ident['status'], 'reference_status': ident['status'],
                       'ledger': livre, 'reference_ledger': livre, 'nodes_checked': ident['status'] == 0,
                       'nodes_equal': ident['status'] == 0,
                       'mutants': {mu: {'killed': True} for mu in m.MUTANTS}})
      cible.write_text('\n'.join(json.dumps(x) for x in lignes) + '\n')
    elif name.startswith('sanitizer_') or name == 'device_fixtures' or name.startswith('gpu_'):
      cible, jeton = Path(valeur(cmd, '--json')), valeur(cmd, '--nonce')
      chemins = [cmd[i + 1] for i, v in enumerate(cmd) if v == '--dump']
      reps, warmup = int(valeur(cmd, '--reps')), int(valeur(cmd, '--warmup'))
      ecrire = not (name.startswith('sanitizer_') and scenario.get('sanitizer_sans_prise')) and \
          not (name.startswith('gpu_') and scenario.get('banc_sans_ecrire'))
      if ecrire:
        prise = prise_banc(m, chemins, jeton, reps, warmup, scenario, name)
        cible.write_text(json.dumps(prise))
        code = 0 if prise['identity'] else 1
      if name.startswith('gpu_') and scenario.get('code_identite_discordants'):
        code = 0 if code == 1 else code
      if name.startswith('gpu_ng00') and scenario.get('modifier_binaire'):
        binaire = bdir / 'mhgp12_traversal_bench'
        binaire.write_bytes(binaire.read_bytes() + b'!')
      if name.startswith('gpu_ng00') and scenario.get('modifier_source'):
        source = travail / 'microbancs' / 'mes_m5_parcours' / 'include' / 'mhgp12' / 'traversal' / 'bfs.hpp'
        source.write_text(source.read_text() + '\n// modifie pendant la session\n')
    elif name.startswith('v11_'):
      cas = name[len('v11_'):name.rindex('_p')]
      ident = m.dump_identity(travail / 'work' / 'dumps' / (cas + '.bin'))
      passes = scenario.get('passes_v11', int(cmd[6]))
      sortie = json.dumps({'phase': 'v11_traversal_timing', 'kmax': int(cmd[3]), 'leaf_size': int(cmd[4]),
                           'workers': int(cmd[5]), 'passes': passes, 'sites': ident['n_sites'],
                           'prefix_ns': [20000000] * passes, 'single_pass_ns': [40000000] * passes,
                           'traversal_ns': [60000000] * passes, 'catalogue_nodes': ident['nodes'],
                           'catalogue_filter_tests': ident['filter_tests']})
    injection = scenario.get('commandes', {}).get(name)
    if injection is not None:
      code = injection
    self.steps.append({'step': name, 'code': code, 'seconds': 0.0})
    return code, sortie, ''
  return run


def jouer(scenario, options):
  """Joue le vrai main() du pilote dans une copie jetable du microbanc ; rend (code, rapport, journal)."""
  with tempfile.TemporaryDirectory(prefix='juge-m5-') as tmp:
    tmp = Path(tmp)
    travail = tmp
    shutil.copytree(MICROBANC, tmp / 'microbancs' / 'mes_m5_parcours',
                    ignore=shutil.ignore_patterns('__pycache__', 'build', 'b_*'))
    shutil.copytree(M2, tmp / 'microbancs' / 'mes_m2_feuille', ignore=shutil.ignore_patterns('__pycache__', 'build'))
    m = charger(tmp / 'microbancs' / 'mes_m5_parcours' / 'scripts' / 'g4_traversal_bench.py',
                'g4_traversal_bench_' + scenario['nom'])
    oracle = charger(tmp / 'microbancs' / 'mes_m5_parcours' / 'oracle' / 'oracle_parcours.py', 'oracle_m5_test')
    depot, donnees, sortie, outils = tmp / 'depot', tmp / 'donnees', tmp / 'out', tmp / 'cuda' / 'bin'
    (depot / 'morsehgp3D_v11' / 'src' / 'catalogue').mkdir(parents=True)
    (depot / 'morsehgp3D_v11' / 'src' / 'catalogue' / 'boxes.cpp').write_text('// synthetique\n')
    donnees.mkdir()
    outils.mkdir(parents=True)
    for nom in ('nvcc', 'compute-sanitizer'):
      (outils / nom).write_bytes(b'#!/bin/sh\nexit 0\n')
    bibliotheque = tmp / 'libmhgp11.a'
    bibliotheque.write_bytes(b'bibliotheque synthetique, jamais executee')
    trames = valeur(options, '--frames', 'ng00,ng01,ng02').split(',')
    for i, trame in enumerate(trames):
      pts = nuage(40, i)
      (donnees / ('lidar_%s.u32le' % trame)).write_bytes(b''.join(p[0].to_bytes(4, 'little') + p[1].to_bytes(4, 'little')
                                                                  + p[2].to_bytes(4, 'little') for p in pts))
      (donnees / ('lidar_%s.ids.u32le' % trame)).write_bytes(b''.join(j.to_bytes(4, 'little') for j in range(40)))
    bdir = tmp / 'work' / 'b_m5'
    journal = []
    m2 = charger(tmp / 'microbancs' / 'mes_m2_feuille' / 'scripts' / 'g4_leaf_bench.py', 'm2_session_' + scenario['nom'])
    m2.Session.run = simulation(m, oracle, scenario, journal, bdir, travail)
    m2.environment = lambda *a: {'gpu_apps': scenario.get('gpu_apps_debut', ''), 'simulation': True}
    m2.find_nvcc = lambda *a: str(outils / 'nvcc')
    m.load_m2 = lambda _: m2

    def vidages(s, commandes, jobs, timeout):
      resultats = {}
      for nom, argv in commandes:
        argv = [str(a) for a in argv]
        cible = Path(argv[5])
        n = len(Path(argv[1]).read_bytes()) // 12
        if '--crop' in argv:
          n = min(n, int(valeur(argv, '--crop')))
        brut = Path(argv[1]).read_bytes()[:12 * n]
        pts = [tuple(int.from_bytes(brut[12 * j + 4 * a:12 * j + 4 * a + 4], 'little') for a in range(3))
               for j in range(n)]
        kmax_entete = scenario.get('kmax_faux') if nom.startswith('dump_ng01') else None
        statut, livre = ecrire_vidage(oracle, cible, pts, int(argv[3]), int(argv[4]), 21, kmax_entete=kmax_entete)
        resultats[nom] = (0, json.dumps(dict(livre, sites=n, kmax=int(argv[3]), leaf_size=int(argv[4]), coord_bits=21,
                                             status=statut, phase='dump')))
        s.steps.append({'step': nom, 'code': 0, 'seconds': 0.0})
      return resultats
    m.run_parallel = vidages
    if scenario.get('prise_perimee'):
      # Prises COMPLETES d'une autre session laissees dans runs/ (meme vidage, memes comptes, autre jeton), pour tous
      # les cas et tous les tours ; le banc de cette session n'ecrit rien. Seuls l'effacement des cibles et le jeton les
      # ecartent (gardes doublees : outils/mutants_juges.py les retire ensemble).
      (sortie / 'runs').mkdir(parents=True)
      for trame in trames:
        for k, feuille in ((5, 16), (5, 24), (10, 24)):
          nom = '%s_k%d_l%d' % (trame, k, feuille)
          chemin = tmp / 'work' / 'dumps' / (nom + '.bin')
          for tour in range(6):
            prise = {'bench': 'mhgp12_traversal_bench', 'nonce': 'm5-ancienne-session', 'reps': 15, 'warmup': 3,
                     'identity': True, 'cases': [{'dump': str(chemin), 'kmax': k, 'leaf_size': feuille,
                                                  'coord_bits': 21, 'sites': 40, 'identity': True,
                                                  'total_ms': [6.0] * 15, 'resident_ms': [5.0] * 15,
                                                  'median_ms': {'total': 6.0, 'resident': 5.0},
                                                  'timed_allocations': 0}]}
            (sortie / 'runs' / ('%s_p%d_gpu.json' % (nom, tour))).write_text(json.dumps(prise))
    argv = ['g4_traversal_bench.py', '--out', str(sortie), '--work', str(tmp / 'work'), '--data', str(donnees),
            '--repo', str(depot), '--v11-lib', str(bibliotheque), '--extra', '', '--jobs', '1', '--cmake',
            '/fake/cmake'] + options
    ancien = sys.argv
    sys.argv = argv
    try:
      with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        code = m.main()
    finally:
      sys.argv = ancien
    rapport = json.loads((sortie / 'report.json').read_text())
  return code, rapport, journal


INJECTIONS = [
  # (nom, scenario, options, verdict attendu) ; les six premiers apres le temoin sont ceux de l'auditeur Codex.
  ('temoin_complet', {}, [], 'adopte'),
  ('auditeur_grille_reduite_un_processus', {}, ['--frames', 'ng00', '--configs', '5:24', '--processes', '1'], 'refuse'),
  ('auditeur_cinq_fixtures_manquantes', {'fixtures_manquantes': True}, [], 'refuse'),
  ('auditeur_identite_hote_code_2', {'commandes': {'identity_host': 2}}, [], 'refuse'),
  ('auditeur_fixtures_appareil_code_7', {'commandes': {'device_fixtures': 7}}, [], 'refuse'),
  ('auditeur_une_passe_v11', {'passes_v11': 1}, [], 'refuse'),
  ('auditeur_mediane_gpu_contraire', {'gpu_brut': 60.0, 'gpu_declare': 6.0}, [], 'refuse'),
  ('auditeur_controle_lent_et_honnete', {'gpu_brut': 60.0}, [], 'rejete'),
  ('auditeur_controle_vidage_errone', {'vidage_errone': True}, [], 'refuse'),
  ('jeton_different', {'jeton_banc': 'autre-session'}, [], 'refuse'),
  ('prise_perimee_banc_muet', {'prise_perimee': True, 'banc_sans_ecrire': True}, [], 'refuse'),
  ('sanitizer_sans_prise', {'sanitizer_sans_prise': True}, [], 'refuse'),
  ('sanitizer_en_echec', {'commandes': {'sanitizer_racecheck': 9}}, [], 'refuse'),
  ('isolation_apres_tours', {'gpu_apps': {'gpu_apps_apres_tours': '4242, intrus, 10 MiB'}}, [], 'refuse'),
  ('isolation_au_debut', {'gpu_apps_debut': '4242, intrus, 10 MiB'}, [], 'refuse'),
  ('porte_du_lecteur_en_echec', {'commandes': {'format_selftest': 1}}, [], 'refuse'),
  ('portes_unitaires_sans_temoin_cst0222', {'sans_temoin_cst0222': True}, [], 'refuse'),
  ('binaire_modifie_pendant_la_session', {'modifier_binaire': True}, [], 'refuse'),
  ('source_modifiee_pendant_la_session', {'modifier_source': True}, [], 'refuse'),
  ('manifeste_des_fixtures_perime', {'jeton_fixtures': 'ancien'}, [], 'refuse'),
  ('vidage_k_faux', {'kmax_faux': 4}, [], 'refuse'),
  ('reservation_chronometree', {'reservations': 2}, [], 'refuse'),
  ('code_0_et_identite_en_defaut', {'identite_appareil_fausse': True, 'code_identite_discordants': True}, [], 'refuse'),
  ('sans_cuda', {}, ['--no-cuda'], 'refuse'),
  ('sanitizers_non_joues', {}, ['--skip-sanitizer'], 'refuse'),
  ('passes_v11_hors_contrat', {}, ['--v11-passes', '5'], 'refuse'),
  ('identite_appareil_en_defaut', {'identite_appareil_fausse': True}, [], 'rejete'),
]


def porte_injections():
  resultats = []
  for nom, scenario, options, attendu in INJECTIONS:
    code, rapport, journal = jouer(dict(scenario, nom=nom), options)
    ligne = {'cas': nom, 'code': code, 'verdict': rapport['verdict'], 'attendu': attendu,
             'refus': len(rapport['refused']), 'rejets': len(rapport['rejected'])}
    exiger(code == 0, '%s : code %d du pilote' % (nom, code))
    exiger(rapport['verdict'] == attendu, '%s : verdict %s, attendu %s (refus %s ; rejets %s)' % (
        nom, rapport['verdict'], attendu, rapport['refused'][:4], rapport['rejected'][:4]))
    if attendu == 'adopte':
      exiger(not rapport['refused'] and rapport['gpu_isolation'] is True and len(rapport['stats']) == 9 and
             all(len(r) == 5 for r in rapport['rounds'].values()), '%s : preuves incompletes' % nom)
      exiger(all(abs(v['ratio_gm'] - 0.1) < 1e-12 for v in rapport['stats'].values()), '%s : rapports' % nom)
      exiger(rapport['format_selftest']['conform'] is True and rapport['hashes_end']['sources_equal'] is True,
             '%s : porte du lecteur ou rehachage' % nom)
    if attendu == 'refuse':
      exiger(rapport['refused'], '%s : refus sans raison' % nom)
    if attendu == 'rejete':
      exiger(not rapport['refused'] and rapport['rejected'], '%s : rejet sans raison ou avec refus' % nom)
    resultats.append(ligne)
  return resultats


def porte_juge_pur():
  m = charger(MICROBANC / 'scripts' / 'g4_traversal_bench.py', 'g4_traversal_bench_juge_pur')
  capture = io.StringIO()
  with contextlib.redirect_stdout(capture):
    code = m.selftest_judge()
  valeur_json = json.loads(capture.getvalue())
  exiger(code == 0 and valeur_json['selftest_judge'] == 'conforme' and len(valeur_json['scenarios']) >= 31,
         'auto-test du juge : %s' % [s for s in valeur_json['scenarios'] if not s['ok']])
  return [{'cas': 'selftest_judge', 'scenarios': len(valeur_json['scenarios']), 'code': code}]


def porte_recu_reel(dossier):
  """Sorties reelles de la session G4 C : rejugees par le juge durci, puis exigees comme preuves fraiches."""
  m = charger(MICROBANC / 'scripts' / 'g4_traversal_bench.py', 'g4_traversal_bench_recu')
  evidence, rapport, notes = m.evidence_from_outputs(dossier)
  exiger(evidence is not None, 'recu illisible : %s' % notes)
  verdict, refus, rejets, stats = m.judge(evidence)
  exiger(verdict == rapport['verdict'] == 'adopte' and not refus and not rejets,
         'session C : verdict %s (refus %s, rejets %s)' % (verdict, refus[:5], rejets[:5]))
  exiger(m.same_stats(stats, rapport['stats']), 'session C : statistiques differentes du publie')
  tours = sum(len(v) for v in evidence['rounds'].values())
  exiger(tours == 60 and len(evidence['fixtures_host']) == 6 and len(evidence['fixtures_device']) == 6,
         'session C : %d tours, fixtures %d / %d' % (tours, len(evidence['fixtures_host']),
                                                    len(evidence['fixtures_device'])))
  # Exigees comme preuves d'une NOUVELLE session (jeton), les memes sorties sont refusees.
  strict, _, _ = m.evidence_from_outputs(dossier, nonce='m5-jeton-de-cette-session')
  verdict_strict, refus_strict, _, _ = m.judge(strict)
  exiger(verdict_strict == 'refuse' and any('jeton' in r for r in refus_strict),
         'prises anciennes admises comme preuves fraiches : %s' % verdict_strict)
  pire = max(v['ci95'][1] for v in stats.values() if v['deciding'])
  return [{'cas': 'recu_g4_t0c_rejuge', 'verdict': verdict, 'tours': tours, 'pire_borne_haute': round(pire, 6),
           'identique_au_publie': 'a l\'octet' if sys.version_info < (3, 12) else 'a 1e-12 pres (sum() compensee)',
           'non_rejouable': len(notes)},
          {'cas': 'recu_g4_t0c_comme_preuves_fraiches', 'verdict': verdict_strict,
           'refus': len(refus_strict)}]


def main(argv):
  dossier = RECU_G4
  if len(argv) == 3 and argv[1] == '--recu-g4':
    dossier = Path(argv[2])
  elif len(argv) != 1:
    print(__doc__)
    return 2
  resultats, ecarts = [], []
  for porte in (porte_juge_pur, porte_injections):
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
  print(json.dumps({'porte': 'juge_m5', 'cas': len(resultats), 'ecarts': ecarts, 'optimise': sys.flags.optimize,
                    'python': sys.version.split()[0]}, ensure_ascii=False))
  return 0 if not ecarts else 1


if __name__ == '__main__':
  sys.exit(main(sys.argv))
