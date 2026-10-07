#!/usr/bin/env python3
"""Porte du pilote de MES-M6 (run_m6.py) : complement de pilote du constat CST-0018 (recu audit_session_t1_20261007/m6).

Le VRAI main du pilote tourne ; seules ses frontieres externes sont simulees (nvcc, binaire du banc, nvidia-smi,
uptime) : aucun compilateur ni GPU n'est lance. Les prises simulees sont ecrites ici, independamment du pilote, dans
l'ordre et au format de mes_m6_session_cost.cu. Attendus :
  - temoin exact de l'auditeur (nvcc simule, toute capture de code 0 sans sortie ; etait mes_m6_ok, code 0, neuf
    fichiers vides) : code 3, aucun succes ; le meme avec un binaire produit : neuf prises vides refusees, code 3 ;
  - prises synthetiques conformes (2000 repetitions et 3 processus, 10 et 1, effectif impair) : mes_m6_ok, code 0 ;
  - chaque corruption d'une seule prise (ligne absente, en double, en trop, remplacee par un doublon, non JSON, mode
    faux, effectif faux, quantiles desordonnes, valeur infinie, negative ou « nan » du banc, champ en trop, premier
    usage renomme, prise tronquee), code non nul a sortie complete, appareil different, GPU occupe au debut, avant ou
    apres une prise, nvidia-smi illisible, compilation en echec, binaire absent ou modifie pendant les prises, source
    modifiee pendant le passage : code 3 ; contexte de la prise encore liste au seul premier releve d'apres : admis
    (le releve seul est repete) ; sorties perimees d'un passage anterieur effacees avant tout ;
  - relecture (--rejuger) des sorties REELLES de la session G4 A (recu g4_t0a_20261007, 002_m6 : 9 prises, 585
    lignes, code publie 0 donc mes_m6_ok retrouve, medianes egales a celles de l'auditeur, manques du rapport v1
    declares non rejouables), d'un dossier v2 conforme, et de dossiers alteres ou de rapports v2 falsifies (code
    3) : code non nul, isolation non prouvee, et les trois mutations du recu audit_juges_emst_20261007/juges
    (provenance vide, isolation de prise quiet=true avec code 9 et un processus, refus explicite laisse sous un
    verdict positif), puis chaque garde seule (empreinte du binaire vide, sources vides ou incompletes, isolation du
    debut contredite, prise declaree non conforme, medianes publiees fausses, champ obligatoire absent).
Usage : python3 -S -O tests/test_juge_m6.py [--recu-g4 DOSSIER_m6]   (dossier par defaut : recu du depot)
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
RECUS = MICROBANC.parents[1] / 'receipts'
RECU_G4 = RECUS / 'g4_t0a_20261007' / 'resultats' / 'cmd' / '002_m6' / 'files' / 'm6'
AUDITEUR = RECUS / 'audit_session_t1_20261007' / 'm6' / 'result.json'
APPAREIL = 'NVIDIA RTX PRO 6000 Blackwell Server Edition'
OCCUPE = '4242, python3, 1024 MiB'


class Ecart(Exception):
  pass


def exiger(condition, message):
  if not condition:
    raise Ecart(message)


def charger():
  spec = importlib.util.spec_from_file_location('run_m6_porte', str(MICROBANC / 'run_m6.py'))
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


# ---------------------------------------------------------------- prise synthetique (format du banc)
def prise(mode, reps, sm=188):
  """Prise conforme dans l'ordre et au format de mes_m6_session_cost.cu (effectifs du banc, valeurs plausibles)."""
  lignes = []

  def une(nom, us, extra=''):
    lignes.append('{"mes":"M6","name":"%s"%s,"n":1,"us":%.3f}' % (nom, extra, us))

  def chaude(nom, n, base, extra=''):
    une(nom + '_first', 3 * base, extra)
    lignes.append('{"mes":"M6","name":"%s"%s,"n":%d,"p05_us":%.3f,"p50_us":%.3f,"p95_us":%.3f,"max_us":%.3f}' % (
        nom, extra, n, base, 1.1 * base, 1.5 * base, 2 * base))

  une('context_open', 116000.0)
  lignes.append('{"mes":"M6","name":"device","device":"%s","sm":%d,"cc":"12.0","sync":"%s","global_mib":97249}' % (
      APPAREIL, sm, mode))
  une('two_streams', 66.5)
  une('first_launch', 85.7)
  une('pinned_alloc_256mib', 30000.0)
  une('pool_alloc_cold_256mib', 500.0)
  chaude('pool_alloc_free_256mib', min(reps, 1000), 20.0)
  chaude('empty_kernel_launch_sync', reps, 8.0)
  chaude('ten_launches_sync', reps, 30.0)
  une('graph_of_ten_prepare', 40.0)
  chaude('graph_of_ten_sync', reps, 11.0)
  for octets in (4 << 10, 64 << 10, 720 << 10, 4 << 20, 64 << 20, 256 << 20):
    n = max(10, reps // 100) if octets >= 64 << 20 else max(10, reps // 10)
    for hote in ('pinned', 'pageable'):
      for sens in ('h2d', 'd2h'):
        chaude('copy', n, 5.0 + octets / 5e4, ',"bytes":%d,"host":"%s","dir":"%s"' % (octets, hote, sens))
  chaude('touch_256mib', max(10, reps // 100), 368.0, ',"bytes_rw":%d' % (2 * (256 << 20)))
  return '\n'.join(lignes) + '\n'


def ligne_de(texte, debut):
  return next(l for l in texte.splitlines() if l.startswith(debut))


def remplacer(texte, avant, apres):
  exiger(texte.count(avant) == 1, 'corruption non applicable : %s' % avant[:60])
  return texte.replace(avant, apres)


Q = '{"mes":"M6","name":"'
CORRUPTIONS = {  # nom -> (corruption de la prise yield 1, motif attendu dans les refus)
    'ligne_absente': (lambda t: t.replace(ligne_de(t, Q + 'touch_256mib_first') + '\n', ''), 'absentes'),
    'ligne_en_double': (lambda t: t + ligne_de(t, Q + 'context_open') + '\n', 'en double'),
    'ligne_remplacee_par_un_doublon': (lambda t: t.replace(ligne_de(t, Q + 'two_streams'),
                                                           ligne_de(t, Q + 'first_launch')), 'en double'),
    'ligne_en_trop': (lambda t: t + Q + 'bonus","n":1,"us":1.000}\n', 'inattendue'),
    'ligne_non_json': (lambda t: t.replace(ligne_de(t, Q + 'first_launch'), 'avertissement du pilote'), 'illisible'),
    'mode_faux': (lambda t: remplacer(t, '"sync":"yield"', '"sync":"spin"'), 'mode'),
    'effectif_faux': (lambda t: remplacer(t, '"bytes":737280,"host":"pinned","dir":"h2d","n":200,',
                                          '"bytes":737280,"host":"pinned","dir":"h2d","n":199,'), 'effectif'),
    'quantiles_desordonnes': (lambda t: remplacer(t, '"empty_kernel_launch_sync","n":2000,"p05_us":8.000',
                                                  '"empty_kernel_launch_sync","n":2000,"p05_us":9.000'),
                              'non ordonnes'),
    'valeur_infinie': (lambda t: remplacer(t, '"p95_us":16.500,"max_us":22.000', '"p95_us":16.500,"max_us":Infinity'),
                       'non fini'),
    'nan_du_banc': (lambda t: remplacer(t, '"context_open","n":1,"us":116000.000',
                                        '"context_open","n":1,"us":nan'), 'illisible'),
    'valeur_negative': (lambda t: remplacer(t, '"first_launch","n":1,"us":85.700', '"first_launch","n":1,"us":-85.700'),
                        'negative'),
    'champ_en_trop': (lambda t: remplacer(t, '"ten_launches_sync","n":2000,', '"ten_launches_sync","n":2000,"us":1.0,'),
                      'champs'),
    'premier_usage_renomme': (lambda t: remplacer(t, '"copy_first","bytes":4096,"host":"pinned","dir":"h2d"',
                                                  '"copy_first","bytes":8192,"host":"pinned","dir":"h2d"'),
                              'inattendue'),
    'prise_tronquee': (lambda t: t[:-20], 'tronquee'),
}


class Simulation:
  """Frontieres externes du pilote : capture(command, timeout) et find_nvcc ; etat du passage simule."""

  def __init__(self, module, reps, processes=3, corrompre=None, codes=None, apps=None, sm=None, creer=True,
               compilation=0, pendant=None):
    self.reps, self.creer, self.compilation, self.pendant = reps, creer, compilation, pendant
    self.corrompre, self.codes, self.apps, self.sm = corrompre or {}, codes or {}, apps, sm or {}
    self.requetes, self.prises = 0, 0
    module.find_nvcc = lambda explicite: '/simule/nvcc'
    module.capture = self.capture

  def capture(self, command, timeout):
    command = [str(c) for c in command]
    if command[0] == '/simule/nvcc':
      if '--version' in command:
        return 0, 'nvcc simule\nBuild cuda_12.9.simule\n', ''
      if self.creer:
        Path(command[command.index('-o') + 1]).write_bytes(b'binaire simule\n')
      return self.compilation, '', '' if self.compilation == 0 else 'erreur simulee'
    if command[0] == 'nvidia-smi':
      if command[1].startswith('--query-compute-apps'):
        self.requetes += 1
        return self.apps(self.requetes) if self.apps else (0, '', '')
      return 0, 'GPU simule\n', ''
    if command[0] == 'uptime':
      return 0, '2026-10-07 00:00:00\n', ''
    mode = command[1].split('=', 1)[1]
    self.prises += 1
    if self.pendant:
      self.pendant(self.prises, command[0])
    index = (self.prises - 1) // 3
    texte = prise(mode, self.reps, self.sm.get((mode, index), 188))
    if (mode, index) in self.corrompre:
      texte = self.corrompre[(mode, index)](texte)
    return self.codes.get((mode, index), 0), texte, ''


def jouer(module, args):
  sortie, erreur = io.StringIO(), io.StringIO()
  with contextlib.redirect_stdout(sortie), contextlib.redirect_stderr(erreur):
    code = module.main(['run_m6.py'] + [str(a) for a in args])
  return code, sortie.getvalue(), erreur.getvalue()


def passage(tmp, nom, reps=2000, processes=3, **scenario):
  """Un passage du vrai main sous simulation ; rend (code, stdout, rapport, dossier de sortie, simulation)."""
  module = charger()
  sim = Simulation(module, reps, processes, **{k: v for k, v in scenario.items() if k != 'module_hook'})
  if 'module_hook' in scenario:
    scenario['module_hook'](module)
  dossier = Path(tmp) / nom
  code, sortie, _ = jouer(module, ['--work', dossier / 'work', '--out', dossier / 'out', '--reps', reps,
                                   '--processes', processes])
  rapport = json.loads((dossier / 'out' / 'm6_report.json').read_text())
  return code, sortie, rapport, dossier / 'out', sim


def rejuger(dossier, *options):
  code, sortie, _ = jouer(charger(), ['--rejuger', dossier] + list(options))
  return code, json.loads(sortie.strip().splitlines()[-1])


# ---------------------------------------------------------------- cas
def cas_auditeur(tmp):
  resultats = []
  module = charger()  # temoin exact de check.py (m6) : find_nvcc et capture remplaces, vrai main
  module.find_nvcc = lambda explicit: '/synthetic/nvcc'
  captures = []

  def capture(command, timeout):
    captures.append(command)
    return 0, '', ''

  module.capture = capture
  out = Path(tmp) / 'auditeur' / 'out'
  code, sortie, _ = jouer(module, ['--work', Path(tmp) / 'auditeur' / 'work', '--out', out])
  rapport = json.loads((out / 'm6_report.json').read_text())
  exiger(code == 3 and 'mes_m6_ok' not in sortie and rapport['verdict'] == 'mes_m6_echec' and
         any('binaire absent' in r for r in rapport['refusals']),
         'temoin de l auditeur : code %d, %s' % (code, sortie.strip()))
  resultats.append({'cas': 'auditeur_prises_vides_code_0', 'code': code, 'verdict': rapport['verdict'],
                    'prises': len(rapport['runs']), 'avant_correction': 'mes_m6_ok, code 0, neuf fichiers vides'})
  c, r = rejuger(out)
  exiger(c == 3 and r['verdict'] == 'mes_m6_echec', 'relecture du dossier de l auditeur : code %d' % c)
  resultats.append({'cas': 'rejuge_dossier_de_l_auditeur', 'code': c, 'verdict': r['verdict']})
  sim_vide = {'corrompre': {(m, i): (lambda t: '') for m in ('spin', 'yield', 'blocking') for i in range(3)}}
  code, sortie, rapport, _, _ = passage(tmp, 'auditeur_binaire', **sim_vide)
  vides = [r for r in rapport['runs'] if r['problems'] == ['prise vide']]
  exiger(code == 3 and len(vides) == 9 and 'mes_m6_ok' not in sortie,
         'prises vides avec binaire : code %d, %d prises vides refusees' % (code, len(vides)))
  resultats.append({'cas': 'auditeur_prises_vides_binaire_present', 'code': code, 'verdict': rapport['verdict'],
                    'prises_vides_refusees': len(vides)})
  return resultats


def cas_conformes(tmp):
  resultats = []
  for nom, reps, processes in (('conforme_2000_3', 2000, 3), ('conforme_10_1', 10, 1), ('conforme_1999_2', 1999, 2)):
    code, sortie, rapport, out, sim = passage(tmp, nom, reps, processes)
    fichiers = sorted(p.name for p in out.iterdir())
    exiger(code == 0 and sortie.strip() == 'mes_m6_ok processus=%d' % (3 * processes) and
           rapport['verdict'] == 'mes_m6_ok' and all(r['conform'] for r in rapport['runs']) and
           len(fichiers) == 3 * processes + 1 and sim.requetes == 1 + 6 * processes,
           '%s : code %d, %s, refus %s' % (nom, code, sortie.strip(), rapport['refusals'][:2]))
    resultats.append({'cas': nom, 'code': code, 'verdict': rapport['verdict'], 'prises': len(rapport['runs']),
                      'isolation_relevee': sim.requetes})
  return resultats


def cas_corruptions(tmp):
  resultats = []
  for nom, (corruption, motif) in CORRUPTIONS.items():
    code, sortie, rapport, _, _ = passage(tmp, nom, corrompre={('yield', 1): corruption})
    fautive = [r for r in rapport['runs'] if (r['mode'], r['process']) == ('yield', 1)][0]
    autres = [r for r in rapport['runs'] if (r['mode'], r['process']) != ('yield', 1)]
    exiger(code == 3 and rapport['verdict'] == 'mes_m6_echec' and not fautive['conform'] and
           any(motif in p for p in fautive['problems']) and all(r['conform'] for r in autres),
           '%s : code %d, problemes %s' % (nom, code, fautive['problems'][:3]))
    resultats.append({'cas': 'prise_' + nom, 'code': code, 'refus': fautive['problems'][0][:80]})
  return resultats


def cas_passage(tmp):
  resultats = []
  occupe_debut = lambda k: (0, OCCUPE, '') if k == 1 else (0, '', '')  # noqa: E731
  occupe_avant = lambda k: (0, OCCUPE, '') if k == 4 else (0, '', '')  # noqa: E731 (avant yield 0)
  occupe_apres = lambda k: (0, OCCUPE, '') if k in (3, 4, 5) else (0, '', '')  # noqa: E731 (apres spin 0, 3 releves)
  illisible = lambda k: (-1, '', 'nvidia-smi introuvable')  # noqa: E731

  def modifier_binaire(rang, binaire):
    if rang == 5:
      with open(binaire, 'ab') as f:
        f.write(b'modifie\n')

  source = Path(tmp) / 'source_copie.cu'
  shutil.copyfile(str(MICROBANC / 'mes_m6_session_cost.cu'), str(source))

  def modifier_source(rang, binaire):
    if rang == 5:
      with open(source, 'a') as f:
        f.write('// modifie\n')

  def pointer_source(module):
    module.SOURCE = source

  scenarios = (
      ('code_non_nul_sortie_complete', {'codes': {('blocking', 2): 1}}, 'code 1'),
      ('appareil_different', {'sm': {('spin', 2): 187}}, 'appareil different'),
      ('gpu_occupe_au_debut', {'apps': occupe_debut}, 'au debut'),
      ('gpu_occupe_avant_une_prise', {'apps': occupe_avant}, 'avant'),
      ('gpu_occupe_apres_une_prise', {'apps': occupe_apres}, 'apres'),
      ('nvidia_smi_illisible', {'apps': illisible}, 'au debut'),
      ('compilation_en_echec', {'compilation': 1}, 'compilation'),
      ('binaire_absent_apres_compilation', {'creer': False}, 'binaire absent'),
      ('binaire_modifie_pendant_les_prises', {'pendant': modifier_binaire}, 'binaire modifie'),
      ('source_modifiee_pendant_le_passage', {'pendant': modifier_source, 'module_hook': pointer_source},
       'source ou pilote modifie'),
  )
  for nom, scenario, motif in scenarios:
    code, sortie, rapport, _, _ = passage(tmp, nom, **scenario)
    exiger(code == 3 and rapport['verdict'] == 'mes_m6_echec' and 'mes_m6_ok' not in sortie and
           any(motif in r for r in rapport['refusals']), '%s : code %d, refus %s' % (nom, code, rapport['refusals'][:2]))
    resultats.append({'cas': nom, 'code': code, 'refus': rapport['refusals'][0][:80]})
  # Contexte de la prise encore liste au premier releve d'apres, absent au second : admis (releve repete).
  code, sortie, rapport, _, sim = passage(tmp, 'contexte_lent', apps=lambda k: (0, OCCUPE, '') if k == 3 else (0, '', ''))
  exiger(code == 0 and sim.requetes == 1 + 6 * 3 + 1 and rapport['runs'][0]['isolation_after']['attempts'] == 2,
         'contexte encore liste au premier releve : code %d, %s' % (code, rapport['refusals'][:2]))
  resultats.append({'cas': 'contexte_encore_liste_au_premier_releve', 'code': code, 'releves': sim.requetes})
  # Sorties perimees d'un passage anterieur : effacees avant tout, le reste du dossier intact.
  out = Path(tmp) / 'perimees' / 'out'
  out.mkdir(parents=True)
  for nom in ('m6_spin_7.jsonl', 'm6_report.json', 'autre.txt'):
    (out / nom).write_text('ancien\n')
  code, sortie, rapport, out, _ = passage(tmp, 'perimees')
  exiger(code == 0 and not (out / 'm6_spin_7.jsonl').exists() and (out / 'autre.txt').exists() and
         rapport['cleared'] == ['m6_report.json', 'm6_spin_7.jsonl'], 'sorties perimees : code %d, effaces %s' % (
             code, rapport.get('cleared')))
  resultats.append({'cas': 'sorties_perimees_effacees', 'code': code, 'effaces': rapport['cleared']})
  return resultats


def cas_relecture(tmp, recu):
  resultats = []
  meta = (recu.parents[1] / 'meta.txt').read_text() if (recu.parents[1] / 'meta.txt').is_file() else ''
  code_publie = 0 if 'exit_code=0' in meta.split() else None
  c, r = rejuger(recu)
  exiger(c == 0 and r['verdict'] == 'mes_m6_ok' and code_publie == 0 and r['prises'] == 9 and r['lignes'] == 585 and
         len(r['non_rejouable']) == 3, 'session G4 A : code %d, %s, refus %s' % (c, r['verdict'], r['refus'][:2]))
  egales = None
  voisins = [recu.parents[5] / AUDITEUR.relative_to(RECUS)] if len(recu.parents) > 5 else []
  chemin = next((p for p in voisins + [AUDITEUR] if p.is_file()), None)  # releve de l'auditeur, a cote du recu
  if chemin is not None:
    auditeur = json.loads(chemin.read_text())['summary']
    egales = all(r['medianes_us'][mode][nom] == valeurs['median_across_processes_us']
                 for mode, noms in auditeur.items() for nom, valeurs in noms.items())
    exiger(egales, 'session G4 A : medianes differentes de celles de l auditeur')
  resultats.append({'cas': 'rejuge_session_g4_a', 'code': c, 'verdict': r['verdict'], 'code_publie': code_publie,
                    'prises': r['prises'], 'lignes': r['lignes'], 'medianes_egales_auditeur': egales,
                    'non_rejouable': len(r['non_rejouable'])})
  c, r = rejuger(recu, '--reps', '1000')
  exiger(c == 3, 'session G4 A relue avec --reps 1000 : code %d' % c)
  resultats.append({'cas': 'rejuge_session_g4_a_effectif_faux', 'code': c, 'refus': r['refus'][0][:80]})
  for nom, alterer in (('prise_alteree', lambda d: (d / 'm6_yield_1.jsonl').write_text(
                          (d / 'm6_yield_1.jsonl').read_text().replace('"p05_us":', '"p05_us":1', 1))),
                       ('prise_absente', lambda d: (d / 'm6_blocking_2.jsonl').unlink()),
                       ('rapport_absent', lambda d: (d / 'm6_report.json').unlink())):
    copie = Path(tmp) / ('session_a_' + nom)
    shutil.copytree(str(recu), str(copie))
    alterer(copie)
    c, r = rejuger(copie)
    exiger(c == 3 and r['verdict'] == 'mes_m6_echec', 'session G4 A, %s : code %d' % (nom, c))
    resultats.append({'cas': 'rejuge_session_g4_a_' + nom, 'code': c, 'refus': r['refus'][0][:80]})
  code, _, _, out, _ = passage(tmp, 'v2')
  c, r = rejuger(out)
  exiger(code == 0 and c == 0 and r['verdict'] == 'mes_m6_ok' and r['non_rejouable'] == [],
         'dossier v2 conforme : code %d, refus %s' % (c, r['refus'][:2]))
  resultats.append({'cas': 'rejuge_v2_conforme', 'code': c, 'verdict': r['verdict'], 'non_rejouable': 0})
  texte = (out / 'm6_spin_0.jsonl').read_text()
  (out / 'm6_spin_0.jsonl').write_text(texte.replace('"us":66.500', '"us":66.400', 1))
  c, r = rejuger(out)
  exiger(c == 3 and any('empreinte' in x for x in r['refus']), 'dossier v2 altere : code %d' % c)
  resultats.append({'cas': 'rejuge_v2_prise_alteree_mais_conforme', 'code': c, 'refus': r['refus'][0][:80]})
  for nom, falsifier, motif in FALSIFICATIONS_V2:
    code, _, _, out, _ = passage(tmp, 'v2_' + nom)
    rapport = json.loads((out / 'm6_report.json').read_text())
    falsifier(rapport)
    (out / 'm6_report.json').write_text(json.dumps(rapport))
    c, r = rejuger(out)
    exiger(code == 0 and c == 3 and any(motif in x for x in r['refus']), 'rapport v2 falsifie (%s) : code %d, %s' % (
        nom, c, r['refus'][:2]))
    resultats.append({'cas': 'rejuge_v2_rapport_falsifie_' + nom, 'code': c, 'refus': r['refus'][0][:80]})
  return resultats


def _medianes_fausses(rapport):
  rapport['summary_median_us']['spin']['context_open'] += 1.0


FALSIFICATIONS_V2 = (  # (nom, falsification d'un rapport v2 conforme, motif attendu dans les refus de la relecture)
    ('code_non_nul', lambda r: r['runs'][4].update(code=1), 'code 1'),
    ('isolation_non_prouvee', lambda r: r['runs'][2]['isolation_after'].update(quiet=False), 'apres la prise'),
    # Recu audit_juges_emst_20261007/juges : provenance vide, isolation contredite, refus explicite laisse.
    ('auditeur_provenance_vide', lambda r: r.update(binary_sha256='', sources_sha256={}), 'empreinte du binaire'),
    ('auditeur_isolation_contredite',
     lambda r: r['runs'][0]['isolation_before'].update(code=9, processes='synthetic-process', quiet=True), 'contredit'),
    ('auditeur_refus_explicite', lambda r: r.update(refusals=['binaire modifie ou retire pendant les prises']),
     'refus publies'),
    # Chaque garde seule.
    ('empreinte_du_binaire_vide', lambda r: r.update(binary_sha256=''), 'empreinte du binaire'),
    ('sources_vides', lambda r: r.update(sources_sha256={}), 'sources'),
    ('sources_incompletes', lambda r: r['sources_sha256'].pop('mes_m6_session_cost.cu'), 'sources'),
    ('isolation_du_debut_contredite', lambda r: r['isolation_start'].update(code=-1), 'du debut'),
    ('prise_declaree_non_conforme', lambda r: r['runs'][1].update(conform=False), 'non conforme'),
    ('medianes_publiees_fausses', _medianes_fausses, 'medianes'),
    ('champ_obligatoire_absent', lambda r: r.pop('date_utc'), 'date_utc'),
)


def cas_usage(tmp):
  resultats = []
  for nom, args in (('reps_hors_bornes', ['--work', tmp, '--out', tmp, '--reps', '5']),
                    ('rejuger_avec_work', ['--rejuger', tmp, '--work', tmp]),
                    ('sortie_absente', ['--work', tmp])):
    code, _, _ = jouer(charger(), args)
    exiger(code == 2, 'usage %s : code %d' % (nom, code))
    resultats.append({'cas': 'usage_' + nom, 'code': code})
  return resultats


def main(argv):
  recu = RECU_G4
  if len(argv) == 3 and argv[1] == '--recu-g4':
    recu = Path(argv[2])
  elif len(argv) != 1:
    print(__doc__)
    return 2
  if not (recu / 'm6_report.json').is_file():
    print('test_juge_m6 : recu G4 A introuvable : %s' % recu, file=sys.stderr)
    return 2
  ecarts, nombre = [], 0
  with tempfile.TemporaryDirectory(prefix='juge-m6-') as tmp:
    for partie in (cas_auditeur, cas_conformes, cas_corruptions, cas_passage, cas_relecture, cas_usage):
      try:
        lignes = partie(tmp, recu) if partie is cas_relecture else partie(tmp)
      except Ecart as ecart:
        ecarts.append('%s : %s' % (partie.__name__, ecart))
        lignes = []
      except Exception as erreur:  # une exception de la porte est un ecart, jamais un succes
        ecarts.append('%s : exception %s: %s' % (partie.__name__, type(erreur).__name__, erreur))
        lignes = []
      for ligne in lignes:
        print(json.dumps(ligne, ensure_ascii=False, sort_keys=True))
      nombre += len(lignes)
  print(json.dumps({'porte': 'juge_m6', 'cas': nombre, 'ecarts': ecarts, 'optimise': sys.flags.optimize,
                    'python': '%d.%d.%d' % sys.version_info[:3]}, ensure_ascii=False))
  return 0 if not ecarts else 1


if __name__ == '__main__':
  sys.exit(main(sys.argv))
