#!/usr/bin/env python3
"""Contre-lecteur L1/L2, schema ea62/902 seulement, seconde execution L1r. Aucun lancement de commande moteur."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess

HERE = Path(__file__).resolve().parent
STAGES = ('P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R')
CAT = ('parcours', 'feuilles', 'emission', 'fin_etage', 'transferts', 'publication')
INTS = ('pass', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns', 'pic_octets',
    'appareil_octets', 'epinglee_octets', 'pic_appareil_octets')
FULL = (*INTS, 'phase', 'trame', 'voie', 'status', 'etapes_ns', 'c_ns', 'g_ns',
    'hors_mur_ns', 'cpu_ns', 'rss_max_octets', 'memoire_octets')
SHA = re.compile(r'[0-9a-f]{64}\Z')
U64 = (1 << 64) - 1


class Refusal(ValueError):
  pass


def need(ok, why):
  if not ok:
    raise Refusal(why)


def sha(data):
  return hashlib.sha256(data).hexdigest()


def uint(x):
  return type(x) is int and 0 <= x <= U64


def finite(x):
  return type(x) in (int, float) and 0 <= x <= U64 and math.isfinite(x)


def unique(pairs):
  result = {}
  for key, value in pairs:
    need(key not in result, 'cle JSON repetee')
    result[key] = value
  return result


def decode(raw):
  def invalid(_):
    raise Refusal('constante JSON non finie')
  try:
    return json.loads(raw.decode('utf-8', 'strict'), object_pairs_hook=unique, parse_constant=invalid)
  except (ValueError, UnicodeError, RecursionError) as error:
    raise Refusal('JSON invalide : ' + str(error)) from error


def keys(x, names, label):
  need(type(x) is dict and set(x) == set(names), label + ' : champs')


def ints(x, names, label):
  keys(x, names, label)
  need(all(uint(v) for v in x.values()), label + ' : u64 hors bool')


def same(a, b):
  if type(b) is dict:
    return type(a) is dict and set(a) == set(b) and all(same(a[k], v) for k, v in b.items())
  if type(b) is list:
    return type(a) is list and len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
  return type(a) is type(b) and a == b


def options(argv):
  need(type(argv) is list and len(argv) % 2 == 0 and all(type(x) is str for x in argv), 'argv')
  result = {}
  for flag, value in zip(argv[::2], argv[1::2]):
    need(flag.startswith('--') and flag not in result and value, 'option repetee/invalide')
    result[flag] = value
  return result


def cohort(plan, sites):
  """Etiquettes affectees a chaque cas, meme non joue ou scene deja vue."""
  argv = plan['commands'][1]['argv']
  opt = options(argv[2:])
  required = {'--src', '--travail', '--donnees', '--sortie', '--cas', '--fils', '--budget-gio',
        '--budget-appareil-gio', '--delai-global', '--empreinte-max-sites', '--jobs'}
  need(set(opt) in (required, required | {'--series'}), 'options du plan')
  need(opt['--fils'] == '48' and opt['--budget-gio'] == '160' and opt['--budget-appareil-gio'] == '88'
    and opt['--empreinte-max-sites'] == '1600000' and opt['--jobs'] == '44', 'regime du plan')
  cases, used, seen = [], set(), set()
  for item in opt['--cas'].split(','):
    name, k, mode, passes = item.split(':')
    need(re.fullmatch(r'[A-Za-z0-9_-]+', name) and name in sites, 'nom de cas')
    need(k.isdigit() and passes.isdigit() and 1 <= int(k) <= 12 and 1 <= int(passes) <= 20, 'K/passes')
    need(mode in ('cpu', 'appareil') and (name, k, mode) not in seen, 'cas repete/voie')
    seen.add((name, k, mode))
    label, rank = name[-23:], 1
    while label in used:
      prefix = str(rank) + '_'
      label, rank = prefix + name[-(23 - len(prefix)):], rank + 1
    used.add(label)
    cases.append(dict(nom=name, k=int(k), voie=mode, expected=int(passes), fils=48,
             sites=sites[name], etiquette=label, empreinte=sites[name] <= 1600000))
  return cases, opt


def full(row, i, case):
  keys(row, (*FULL, 'full_sha256') if case['empreinte'] else FULL, 'full')
  need(all(uint(row[k]) for k in INTS), 'full : u64 hors bool')
  expected = dict(phase='full', status='ok', coord_bits=21, kmax=case['k'], threads=case['fils'],
          sites=case['sites'], trame=case['etiquette'], voie='device' if case['voie'] == 'appareil' else 'cpu')
  need(all(same(row[k], v) for k, v in expected.items()) and row['pass'] == i, 'full : commande/ordre')
  need(row['wall_ns'] > 0, 'mur nul : aucune statistique logarithmique admissible')
  ints(row['etapes_ns'], STAGES, 'etages')
  ints(row['c_ns'], CAT, 'C')
  ints(row['g_ns'], ('tables', 'resolution'), 'G')
  ints(row['hors_mur_ns'], ('validation', 'empreinte'), 'hors mur')
  st = row['etapes_ns']
  need(sum(st[k] for k in STAGES[:5]) <= row['wall_ns'], 'etages > mur')
  need(sum(st[k] for k in STAGES[5:]) <= st['TMVR'], 'T/M/V/R > TMVR')
  need(sum(row['g_ns'].values()) <= st['G'], 'tables/resolution > G')
  need(16 * case['sites'] <= row['pic_octets'] <= 160 * (1 << 30), 'pic hote / entrees / budget')
  need(row['epinglee_octets'] <= row['pic_octets'], 'epingle > pic hote')
  need(row['appareil_octets'] <= row['pic_appareil_octets'] <= 88 * (1 << 30), 'capacite/pic/budget appareil')
  mem = row['memoire_octets']
  keys(mem, STAGES[:5], 'memoire par etage')
  for name in STAGES[:5]:
    pair = mem[name]
    need(type(pair) is list and len(pair) == 2 and all(uint(v) for v in pair), 'memoire : paire u64 hors bool')
    need(16 * case['sites'] <= pair[0] <= pair[1], 'memoire : resident/usage/pic')
  need(max(v[1] for v in mem.values()) == row['pic_octets'], 'memoire : maximum global')
  need(all(mem[a][0] <= mem[b][1] for a,b in zip(STAGES[:4],STAGES[1:5])), 'memoire : frontiere')
  need(mem['raccord'] == [mem['G'][0],mem['G'][0]], 'memoire : raccord sans allocation budgetee')
  if case['voie'] == 'cpu':
    need(not any(row[k] for k in ('appareil_octets', 'epinglee_octets', 'pic_appareil_octets')), 'memoire CPU')
  for key in ('cpu_ns', 'rss_max_octets'):
    need(row[key] is None or uint(row[key]), key + ' : null ou u64 hors bool')
  if case['empreinte']:
    need(type(row['full_sha256']) is str and SHA.fullmatch(row['full_sha256']), 'empreinte demandee')
  else:
    need(row['hors_mur_ns']['empreinte'] == 0, 'empreinte hors plan')


def stream(raw, code, case, reasons):
  need(code == 'expire' or type(code) is int and -(1 << 31) <= code < (1 << 31), 'code externe absent/maltype')
  if code == 'expire' or type(code) is int and code < 0:
    return dict(etat='echec', passes=[], conditions=[], issue='interruption',
                raison='expire' if code == 'expire' else 'signal '+str(-code))
  need(type(raw) is bytes and raw.isascii(), 'flux ASCII')
  lines = raw.splitlines()
  need(lines and all(line.strip() for line in lines), 'flux vide/ligne vide')
  rows = [decode(line) for line in lines]
  need(all(type(r) is dict for r in rows), 'ligne non objet')
  end = rows.pop()
  keys(end, ('phase', 'status', 'reason'), 'exit')
  need(end['phase'] == 'exit' and type(end['reason']) is str and end['reason'] in reasons
    and reasons[end['reason']] == end['status'], 'issue inconnue/incoherente')
  conditions, opened = [], None
  if case['voie'] == 'appareil':
    need(rows and rows[0].get('phase') == 'open', 'open absent')
    opened = rows.pop(0)
    keys(opened, ('phase', 'status', 'reason', 'wall_ns', 'budget_appareil'), 'open')
    need(uint(opened['wall_ns']) and opened['budget_appareil'] == 'separe', 'open budget/temps')
    need(type(opened['reason']) is str and opened['reason'] in reasons
      and reasons[opened['reason']] == opened['status'], 'open issue')
    if opened['status'] != 'ok':
      need(not rows and opened['reason'] == end['reason'] and code == 2, 'open refuse incoherent')
      return dict(etat='illisible', passes=[], conditions=['appareil indisponible'], issue=end['reason'])
  need(len(rows) % 2 == 0 and len(rows) // 2 <= case['expected'], 'nombre full/liberation')
  passes, rss = [], 0
  for i in range(len(rows) // 2):
    row, release = rows[2*i:2*i+2]
    full(row, i, case)
    keys(release, ('phase', 'pass', 'liberation_ns'), 'liberation')
    need(release['phase'] == 'liberation' and uint(release['pass']) and release['pass'] == i
      and uint(release['liberation_ns']), 'liberation ordre/temps')
    for key in ('cpu_ns', 'rss_max_octets'):
      if row[key] is None:
        conditions.append(key + ' indisponible passe ' + str(i))
    if row['rss_max_octets'] is not None:
      need(row['rss_max_octets'] >= rss, 'ru_maxrss decroissant')
      rss = row['rss_max_octets']
    passes.append(dict(row, liberation_ns=release['liberation_ns']))
  status = end['status']
  if status == 'ok' and code == 0 and len(passes) == case['expected']:
    state = 'ok'
  elif (status in ('invalid_input', 'unsupported_degeneracy', 'resource_exhausted') and code == 2
     and len(passes) < case['expected']):
    state = 'refus'
  else:
    state = 'echec'
    if not (status == 'invariant_violated' and code == 3 and len(passes) < case['expected']):
      conditions.append('code/issue/nombre de passes incoherents')
  result = dict(etat=state, passes=passes, conditions=conditions, issue=end['reason'])
  result['raison'] = ('' if state == 'ok' else status+'/'+end['reason'] if state == 'refus' else
                      f"sortie {status}/{end['reason']}, code {code}, {len(passes)} passes")
  if opened is not None and state in ('ok', 'refus'):
    result['open_ns'] = opened['wall_ns']
  return result


def statistics_case(passes):
  if not passes:
    return None
  warm, last = passes[1:], passes[-1]
  return dict(passes=len(passes), premiere_ns=passes[0]['wall_ns'], derniere_ns=last['wall_ns'],
        regime_derniere='chaude' if warm else 'froide',
        mediane_chaude_ns=statistics.median(p['wall_ns'] for p in warm) if warm else None,
        max_chaud_ns=max(p['wall_ns'] for p in warm) if warm else None,
        cpu_derniere_ns=last['cpu_ns'], rss_max_octets=max((p['rss_max_octets'] for p in passes
          if p['rss_max_octets'] is not None), default=None),
        hote_peak_octets=max(p['pic_octets'] for p in passes),
        device_peak_octets=max(p['pic_appareil_octets'] for p in passes),
        device_capacity_last_octets=last['appareil_octets'], pinned_capacity_last_octets=last['epinglee_octets'])


def criteria(cases, series):
  """Etats independants ; details textuels du pilote non utilises comme donnees."""
  def verdict(rows, bad):
    return 'non evalue' if not rows else 'non tenu' if bad else 'tenu'
  def rate_bad(c, limit):
    return c['etat'] == 'ok' and c['passes'][-1]['wall_ns'] > limit * 1000 * c['sites']
  k5 = [c for c in cases if c['k'] == 5 and c['voie'] == 'appareil' and c['etat'] != 'non_joue']
  small = [c for c in k5 if c['sites'] < 10000000]
  k10 = [c for c in cases if c['k'] == 10 and c['etat'] != 'non_joue']
  fits = []
  for members in series:
    found = {c['nom']: c for c in k5 if c['nom'] in members and c['etat'] == 'ok'}
    if set(found) != set(members):
      continue
    xs = [math.log(found[n]['sites']) for n in members]
    ys = [math.log(found[n]['passes'][-1]['wall_ns']) for n in members]
    if len(set(xs)) < 2:
      continue
    mx, my = sum(xs)/len(xs), sum(ys)/len(ys)
    slope = sum((x-mx)*(y-my) for x,y in zip(xs,ys))/sum((x-mx)**2 for x in xs)
    fits.append(dict(membres=members, pente=slope))
  return dict(B1=verdict(k5, any(rate_bad(c,2) or c['etat'] == 'echec' or
        c['etat'] == 'refus' and c['sites'] < 10000000 for c in k5)),
        B2=verdict(small, any(c['etat'] != 'ok' for c in small)),
        B3=verdict(fits, any(f['pente'] > 1.1 for f in fits)),
        B4=verdict(k10, any(c['etat'] != 'ok' or rate_bad(c,10) for c in k10))), fits


def review(report, raws, specs, opt, manifest_sha, reasons, external=None):
  keys(report, ('mesure','regime','verdict','criteres','controles','environnement','provenance',
         'parametres','empreintes','cas','duree_s'), 'rapport')
  need(report['mesure'] == 'MES-B' and report['regime'] == 'b' and finite(report['duree_s']), 'mesure/duree')
  conditions = []
  def check(ok, why):
    if not ok:
      conditions.append(why)
  params = report['parametres']
  keys(params, ('fils','budget_octets','budget_appareil_octets','delai_global_s','series','argv'), 'parametres')
  need(same(params['fils'],48) and same(params['budget_octets'],160 << 30)
    and same(params['budget_appareil_octets'],88 << 30), 'budgets/fils commandes')
  need(type(params['delai_global_s']) is float and params['delai_global_s'] == float(opt['--delai-global']), 'delai')
  got = options(params['argv'])
  need(set(got) == set(opt), 'options rapport')
  need(all(got[k] == v for k,v in opt.items() if k not in ('--src','--travail','--donnees','--sortie')), 'commande')
  series = [s.split(',') for s in opt.get('--series','').split(';') if s]
  need(same(params['series'],series), 'series')
  keys(report['provenance'], ('cmake','sonde_sha256','pilote_sha256','manifeste_sha256'), 'provenance')
  provenance = report['provenance']
  for key in ('sonde_sha256','pilote_sha256','manifeste_sha256'):
    need(type(provenance[key]) is str and SHA.fullmatch(provenance[key]), 'SHA provenance')
  need(provenance['pilote_sha256'] == '457d0e6ff6f27daca0eb22f5852fe6b1e53fce1611c330fd3931a5b071c8c49d'
    and provenance['manifeste_sha256'] == manifest_sha, 'pilote/manifeste differents')
  check(type(provenance['cmake']) is list and bool(provenance['cmake'])
     and all(type(v) is str for v in provenance['cmake']), 'trace CMake absente')
  keys(report['environnement'], ('avant','apres'), 'environnement')
  for when, env in report['environnement'].items():
    keys(env, ('cmake','nvcc','gpu','gpu_apps','noyau','fils_hote','memoire_hote'), 'environnement '+when)
    check(env['gpu_apps'] == '', 'GPU non connu vide '+when)
    check(all(type(env[k]) is str and env[k].strip() for k in ('cmake','nvcc','gpu','noyau','memoire_hote'))
       and uint(env['fils_hote']) and env['fils_hote'] > 0, 'environnement incomplet '+when)
  need(type(report['cas']) is list and len(report['cas']) == len(specs), 'cohorte incomplete')
  results, launched, digests = [], set(), {}
  base = {'nom','k','voie','fils','empreinte','sites','etiquette','prevision_s','etat','raison','passes'}
  for entry, case in zip(report['cas'], specs):
    need(type(entry) is dict, 'cas non objet')
    for key in ('nom','k','voie','fils','empreinte','sites','etiquette'):
      need(key in entry and same(entry[key], case[key]), 'cas : '+key)
    need(finite(entry.get('prevision_s')) and type(entry.get('raison')) is str, 'cas prevision/raison')
    tag = f"{case['nom']}_k{case['k']}_{case['voie']}"
    if entry.get('etat') == 'non_joue':
      keys(entry, base, 'cas non joue')
      need(entry['passes'] == [] and entry['raison'].startswith('delai : '), 'non joue')
      result = dict(etat='non_joue', passes=[], conditions=[], issue='delai')
    else:
      launched.add(tag)
      need(tag+'.jsonl' in raws and tag+'.err' in raws, 'brut/stderr manquant')
      required = base | {'code','secondes','pic_nvidia_smi_mio'}
      need(set(entry) in (required, required | {'open_ns'}), 'champs cas joue')
      need(finite(entry['secondes']) and (entry['pic_nvidia_smi_mio'] is None or uint(entry['pic_nvidia_smi_mio'])),
        'duree processus/pic echantillonne')
      if external is not None:
        need(tag in external and same(external[tag],entry['code']), 'code externe divergent')
      try:
        result = stream(raws[tag+'.jsonl'], entry['code'], case, reasons)
      except Refusal as error:
        result = dict(etat='illisible', passes=[], conditions=[str(error)], issue='schema')
      check(entry['etat'] == result['etat'], tag+' : etat divergent')
      if result['etat'] != 'illisible':
        check(entry['raison'] == result['raison'], tag+' : raison divergente')
      check(same(entry['passes'],result['passes']), tag+' : passes divergentes')
      check(('open_ns' in entry) == ('open_ns' in result) and
         ('open_ns' not in result or same(entry['open_ns'],result['open_ns'])), tag+' : open divergent')
      conditions.extend(tag+' : '+c for c in result['conditions'])
    for row in result['passes']:
      if 'full_sha256' in row:
        digests.setdefault(f"{case['nom']}:K{case['k']}",set()).add(row['full_sha256'])
    results.append(dict(case, **result, statistiques=statistics_case(result['passes'])))
  need(set(raws) == {t+s for t in launched for s in ('.jsonl','.err')}, 'bruts supplementaires/non joues')
  if external is not None:
    need(set(external) == launched, 'cohorte codes externes')
  check(all(len(v) == 1 for v in digests.values()), 'identites divergentes intra/intervoies')
  digest_table = {k:sorted(v) for k,v in sorted(digests.items())}
  check(same(report['empreintes'],digest_table), 'rapport empreintes divergent')
  states, fits = criteria(results,series)
  keys(report['criteres'], states, 'criteres')
  for key, state in states.items():
    keys(report['criteres'][key], ('etat','detail'), 'critere '+key)
    check(report['criteres'][key]['etat'] == state, 'rapport '+key+' divergent')
    need(type(report['criteres'][key]['detail']) is list and
      all(type(v) is str for v in report['criteres'][key]['detail']), 'detail critere')
  need(type(report['controles']) is list and all(type(v) is str for v in report['controles']), 'controles')
  check(not report['controles'], 'controles du pilote non vides')
  held = all(states[b] != 'non evalue' for b in ('B1','B2')) and all(v in ('tenu','non evalue') for v in states.values())
  verdict = 'refuse' if conditions else 'tenu' if held else 'non tenu'
  check(report['verdict'] == verdict, 'verdict rapport divergent')
  return dict(bruts_admis=not conditions, conditions=conditions, criteres=states, pentes=fits,
        verdict='refuse' if conditions else verdict, cas=results, empreintes=digest_table,
        codes='externes fournis' if external is not None else 'declares par le rapport du pilote epingle',
        qualification_campagne=False)


def sources(repo, capture):
  blobs = {}
  for path, spec in capture['sources'].items():
    raw = subprocess.check_output(['git','-C',str(repo),'show',spec['pin']+':'+path])
    need(sha(raw) == spec['sha256'], 'source epinglee differente')
    blobs[path] = raw
  text = blobs['morsehgp3D_v12/src/core/reasons.def'].decode()
  return dict(re.findall(r'^MHGP12_REASON\((\w+),\s*(\w+),',text,re.M))


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--repo', type=Path, required=True)
  parser.add_argument('--plan', type=Path, required=True)
  parser.add_argument('--session', choices=('L1r',), required=True)
  parser.add_argument('--results', type=Path, required=True, help='Dossier rapport_b.json et brut/')
  parser.add_argument('--codes', type=Path, help='Objet tag -> code, capture independante facultative')
  args = parser.parse_args()
  capture = decode((HERE/'capture.json').read_bytes())
  reasons = sources(args.repo,capture)
  meta = capture['sessions'][args.session]
  plan_raw = args.plan.read_bytes()
  need(sha(plan_raw) == meta['plan_sha256'], 'plan non epingle')
  specs,opt = cohort(decode(plan_raw),meta['sites'])
  report_path = args.results/'rapport_b.json'
  report_raw = report_path.read_bytes()
  paths = sorted((args.results/'brut').iterdir())
  need(all(p.is_file() and not p.is_symlink() for p in paths), 'brut non fichier')
  raws = {p.name:p.read_bytes() for p in paths}
  external = decode(args.codes.read_bytes()) if args.codes else None
  if external is not None:
    need(type(external) is dict, 'codes non objet')
  result = review(decode(report_raw),raws,specs,opt,meta['manifest_sha256'],reasons,external)
  need(report_path.read_bytes() == report_raw and args.plan.read_bytes() == plan_raw, 'rapport/plan instable')
  need(sorted(p.name for p in (args.results/'brut').iterdir()) == sorted(raws) and
    all(p.read_bytes() == raws[p.name] for p in paths), 'bruts instables')
  result['sha256'] = {'rapport_b.json':sha(report_raw), **{k:sha(v) for k,v in raws.items()}}
  print(json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2))
  if not result['bruts_admis']:
    raise SystemExit(1)


if __name__ == '__main__':
  try:
    main()
  except (Refusal,OSError,subprocess.CalledProcessError,KeyError,TypeError) as error:
    print(json.dumps(dict(bruts_admis=False,erreur=str(error)),ensure_ascii=False))
    raise SystemExit(1)
