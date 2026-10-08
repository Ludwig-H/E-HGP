#!/usr/bin/env python3
"""Relecture de journaux existants. Usage: python [-O] check.py DEPOT RETOUR PLAN.
Aucun moteur, tar de données ou service externe. Les gardes de structure renforcent
l'admission ; la règle statistique est celle figée AVANT la campagne.
"""
import copy
import hashlib
import json
import math
from pathlib import Path
import random
import re
import statistics as st
import subprocess
import sys
import types
sys.dont_write_bytecode = True
C = json.loads(Path(__file__).with_name('capture.json').read_text())
REPO, RAW, PLAN = map(Path, sys.argv[1:4])
FRAMES = ('ng00', 'ng01', 'ng02')
ARMS = ('avant', 'apres')
U64 = (1 << 64) - 1

def need(ok, why):
    if not ok:
        raise ValueError(why)

def sha(b):
    return hashlib.sha256(b).hexdigest()

def pairs(xs):
    out = {}
    for k, v in xs:
        need(k not in out, 'clé JSON répétée')
        out[k] = v
    return out

def constant(x):
    raise ValueError('constante JSON non finie')

def loads(s):
    return json.loads(s, object_pairs_hook=pairs, parse_constant=constant)

def canonical(x):
    return json.dumps(x, sort_keys=True, allow_nan=False)

FLOAT_DELTAS = []
def equivalent(a, b, path=''):
    if type(a) is float and type(b) is float:
        if a != b:
            need(abs(a-b) <= 2*max(math.ulp(a), math.ulp(b)), 'écart flottant: '+path)
            FLOAT_DELTAS.append({'field':path,'local':a,'archived':b,'absolute':abs(a-b)})
        return True
    if isinstance(a, (list, tuple)) and isinstance(b, (list, tuple)):
        return len(a)==len(b) and all(equivalent(x,y,path+'/'+str(i)) for i,(x,y) in enumerate(zip(a,b)))
    if type(a) is dict and type(b) is dict:
        return set(a)==set(b) and all(equivalent(a[k],b[k],path+'/'+k) for k in a)
    return type(a) is type(b) and a == b

def u(x, positive=False):
    return type(x) is int and (1 if positive else 0) <= x <= U64

def ints(x, keys):
    need(type(x) is dict and set(x) == set(keys) and all(u(v) for v in x.values()), 'dictionnaire u64')

def blob(path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', C['commit'] + ':' + path])

MUR = ('P', 'C', 'G', 'raccord', 'TMVR')
STAGES = MUR + ('T', 'M', 'V', 'R')
CS = ('parcours', 'feuilles', 'emission', 'fin_etage', 'transferts', 'publication')
WS = ('G', 'foret', 'foret_apres_g', 'T', 'M', 'V', 'R')
RS = ('tour_ns', 'ouverture_ns', 'fin_g_ns', 'fin_ns', 'queue_ns',
      'noyau_reprises', 'noyau_arrets', 'admis_octets')
COMMON = ('phase', 'pass', 'trame', 'voie', 'status', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns',
          'etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns', 'pic_octets', 'cpu_ns', 'rss_max_octets',
          'appareil_octets', 'epinglee_octets', 'pic_appareil_octets', 'memoire_octets')
EXTRA = ('etapes_schema', 'recouvrement', 'fenetres_ns', 'fins_par_ordre_ns')

def expected(arm, names, k=5, threads=48, digest=False, sequential=False):
    return dict(bras=arm, trames=names, k=k, fils=threads, passes=len(names), empreinte=digest,
                appareil=True, schema='sequentiel' if arm == 'avant' or sequential else 'recouvert')


def validate(rows, spec, sites):
    """Source épinglée : mur disjoint ; fenêtres recouvertes jamais additionnées au mur."""
    n = spec['passes']
    need(len(rows) == 2 * n + 2 and all(type(r) is dict for r in rows), 'séquence incomplète/non objet')
    op, ex = rows[0], rows[-1]
    need(set(op) == {'phase', 'status', 'reason', 'wall_ns', 'budget_appareil'} and
         op['phase'] == 'open' and op['status'] == 'ok' and op['reason'] == 'none' and
         u(op['wall_ns']) and op['budget_appareil'] == 'partage', 'ouverture')
    need(ex == {'phase': 'exit', 'status': 'ok', 'reason': 'none'}, 'fermeture')
    rec = spec['schema'] == 'recouvert'
    floor = 16 * sum(sites[x] for x in set(spec['trames']))
    fulls = []
    for i, name in enumerate(spec['trames']):
        r, free = rows[2*i+1:2*i+3]
        need(set(r) == set(COMMON + (EXTRA if rec else ()) + (('full_sha256',) if spec['empreinte'] else ())),
             'champs FULL')
        need(r['phase'] == 'full' and r['status'] == 'ok' and r['voie'] == 'device' and
             r['trame'] == name and type(r['pass']) is int and r['pass'] == i, 'identité de passe')
        need(all(type(r[k]) is int and r[k] == v for k, v in
                 [('coord_bits', 21), ('kmax', spec['k']), ('threads', spec['fils']), ('sites', sites[name])]),
             'profil/cohorte')
        need(u(r['wall_ns'], True) and u(r['sites'], True), 'mur/sites positifs')
        need(set(free) == {'phase', 'pass', 'liberation_ns'} and free['phase'] == 'liberation' and
             type(free['pass']) is int and free['pass'] == i and u(free['liberation_ns']), 'libération')
        if spec['empreinte']:
            need(type(r['full_sha256']) is str and re.fullmatch('[a-f0-9]{64}', r['full_sha256']), 'FUL1')
        for k in ('pic_octets', 'appareil_octets', 'epinglee_octets', 'pic_appareil_octets'):
            need(u(r[k]), 'ressource u64')
        for k in ('cpu_ns', 'rss_max_octets'):
            need(r[k] is None or u(r[k]), 'getrusage u64 ou indisponible')
        ints(r['hors_mur_ns'], ('validation', 'empreinte'))
        ints(r['c_ns'], CS)  # inclusions internes C non supposées
        e = r['etapes_ns']
        ints(e, MUR if rec else STAGES)
        need(sum(e[k] for k in MUR) <= r['wall_ns'], 'étapes hors mur')
        ms = ('P', 'C', 'tour') if rec else MUR
        mem = r['memoire_octets']
        need(type(mem) is dict and set(mem) == set(ms), 'mémoire par étage absente')
        need(all(type(v) is list and len(v) == 2 and all(u(x) for x in v) and v[0] <= v[1]
                 for v in mem.values()), 'résident/pic invalides')
        need(max(v[1] for v in mem.values()) == r['pic_octets'] and
             all(mem[b][1] >= mem[a][0] for a, b in zip(ms, ms[1:])), 'pics non cohérents')
        need(r['pic_octets'] >= floor and r['pic_appareil_octets'] == 0 and
             r['appareil_octets'] <= r['pic_octets'] and r['epinglee_octets'] <= r['pic_octets'],
             'budget partagé / entrée résidente')
        if rec:
            need(r['etapes_schema'] == 'recouvert' and e['raccord'] == 0, 'schéma recouvert')
            q, w, fins, g = r['recouvrement'], r['fenetres_ns'], r['fins_par_ordre_ns'], r['g_ns']
            ints(q, RS); ints(w, WS); ints(g, ('ouverture', 'tables'))
            need(q['fin_g_ns'] == e['G'] and q['queue_ns'] == e['TMVR'] and
                 q['fin_ns'] == q['fin_g_ns'] + q['queue_ns'] and q['fin_ns'] <= q['tour_ns'] and
                 q['ouverture_ns'] == g['ouverture'] and e['P'] + e['C'] + q['tour_ns'] <= r['wall_ns'],
                 'queue ou tour incohérents')
            need(type(fins) is list and len(fins) == spec['k'] and
                 all(type(a) is list and len(a) == 5 and all(u(x) and x <= q['fin_ns'] for x in a) for a in fins),
                 'fins par ordre')
            need(g['tables'] <= g['ouverture'] <= q['fin_g_ns'] and
                 max(a[0] for a in fins) == q['fin_g_ns'] and
                 all(a[0] <= a[1] <= a[2] <= a[4] and
                     (a[3] == 0 if k == 0 else a[2] <= a[3]) for k,a in enumerate(fins)),
                 'ordre partiel des fins G/T/M et V,R indépendants')
        else:
            ints(r['g_ns'], ('tables', 'resolution'))
            need(sum(e[k] for k in ('T', 'M', 'V', 'R')) <= e['TMVR'] and
                 sum(r['g_ns'].values()) <= e['G'], 'sous-étages séquentiels')
        fulls.append(r)
    return fulls


def main(P):
    need(sha(PLAN.read_bytes()) == C['plan_sha256'], 'plan modifié')
    planned = loads(PLAN.read_text())
    need([x for x in planned['commands'] if x['name'] == 't2da_pilote'] == [C['plan_pilote']], 'commande planifiée')
    files = {str(p.relative_to(RAW)): sha(p.read_bytes()) for p in sorted(RAW.rglob('*')) if p.is_file()}
    need(len(files) == C['returned_files'] and sha(''.join(h+' '+p+'\n' for p,h in sorted(files.items())).encode())
         == C['returned_tree_sha256'], 'inventaire des traces changé')
    need(files['rapport_t2d_a.json'] == C['report_sha256'], 'rapport changé')
    r = loads((RAW/'rapport_t2d_a.json').read_text())
    need(r['schema'] == 'ehgp.v12.t2d_a_pilote.v1' and canonical(r['regle']) == canonical(P.REGLE_T2D_A), 'protocole changé')
    for e in r['environnement'].values():
        need(type(e['gpu']) is str and bool(e['gpu'].strip()) and e['gpu_apps'] == '' and type(e['gpu_apps']) is str,
             'GPU non attesté vide')
    need(set(r['environnement']) == set(ARMS), 'deux observations GPU')
    construction = r['construction']
    need(construction['archive_avant_sha256'] == C['before_archive_sha256'] and
         set(construction['binaires']) == set(ARMS), 'archive avant/binaire absent')
    for arm, b in construction['binaires'].items():
        need(re.fullmatch('[a-f0-9]{64}', b['sha256']) is not None, 'SHA binaire')
        need({'CMAKE_BUILD_TYPE:STRING=Release', 'MHGP12_COORD_BITS:STRING=21', 'MHGP12_ENABLE_CUDA:BOOL=ON',
              'MHGP12_SANITIZE:BOOL=OFF', 'MHGP12_POISON:BOOL=OFF'} <= set(b['cmake']), 'configuration build')
    cb = blob(C['cohort_source'])
    need(sha(cb) == C['cohort_source_sha256'] and loads(cb)['cohort'] == C['cohort'], 'métadonnées 37 modifiées')
    names = [x['name'][-23:] for x in C['cohort']]
    sites = {x['name'][-23:]: x['sites'] for x in C['cohort']}
    sites.update(C['ng_sites'])
    need(len(names) == len(set(names)) == 37, 'cohorte 37 distincte')
    seen, allfull, specs = {}, {}, {}

    def read(take, path, spec, site_map=sites):
        need(take['journal'] == path and path not in seen, 'journal étranger/répété')
        need(take['journal_sha256'] == files[path] and type(take['code']) is int and take['code'] == 0 and
             take['valide'] is True and take['bras'] == spec['bras'], 'code/hash/bras')
        need(canonical(take['attendu']) == canonical(spec), 'commande attendue auto-déclarée différente du plan')
        body = (RAW/path).read_text(encoding='ascii')
        rows = [loads(line) for line in body.splitlines() if line.strip()]
        fs = validate(rows, spec, site_map)
        summary = P.lire_prise(str(RAW/path), 0, spec)
        need(all(canonical(summary[k]) == canonical(take[k]) for k in P.RESUME), 'résumé différent des bruts')
        seen[path] = files[path]; allfull[path] = fs; specs[path] = (spec, site_map)
        return fs

    identity = r['identite']['prises']
    expected_keys = {f+'_k'+str(k) for f in FRAMES for k in (5,10)} | {'u8000_k5','u16000_k5','u32000_k5','v12set_k5'}
    need(set(identity) == expected_keys, 'cohorte des identités')
    hashes = {}
    for key, takes in identity.items():
        if key.startswith('ng'):
            f, k = key.split('_k'); k = int(k)
            groups = set(ARMS) | ({'apres_sequentiel'} if k == 5 else set())
            if key == 'ng00_k5': groups |= {'avant_1fil', 'apres_1fil'}
        else: groups = set(ARMS)
        need(set(takes) == groups, 'bras identités absents/étrangers')
        for group, take in takes.items():
            arm = 'avant' if group.startswith('avant') else 'apres'
            smap = sites
            if key == 'v12set_k5': nn, k = names, 5
            elif key.startswith('u'):
                size = int(key[1:].split('_')[0]); nn, k = ['uniforme'], 5
                smap = {'uniforme': size}
            else:
                f, k = key.split('_k'); k = int(k)
                nn = [f] * (1 if k == 10 or group.endswith('1fil') else 2)
            spec = expected(arm, nn, k, 1 if group.endswith('1fil') else 48, True, group.endswith('sequentiel'))
            fs = read(take, 'journaux/identite/'+key+'_'+group+'.jsonl', spec, smap)
            for row in fs:
                hk = (key, row['trame'])
                need(hk not in hashes or hashes[hk] == row['full_sha256'], 'empreintes FULL différentes')
                hashes[hk] = row['full_sha256']

    camp = r['campagne_k5']
    need(all(type(camp[x]) is int and camp[x] == n for x,n in [('fils',48),('passes',10),('tours_demandes',5)])
         and set(camp['trames']) == set(FRAMES), 'cohorte chronométrée')
    need(camp['binaires_apres'] == {a:construction['binaires'][a]['sha256'] for a in ARMS}, 'binaire changé')
    timing = {f:{a:[] for a in ARMS} for f in FRAMES}
    for f in FRAMES:
        need(type(camp['trames'][f]) is list and len(camp['trames'][f]) == 5, 'cinq tours requis')
        for i,tour in enumerate(camp['trames'][f]):
            need(set(tour) == set(ARMS), 'bras chronique')
            for arm in ARMS:
                fs = read(tour[arm], 'journaux/k5/%s_%s_t%02d.jsonl'%(f,arm,i), expected(arm,[f]*10))
                timing[f][arm].append(fs[1:])
    ses = r['session_v12set']
    need(type(ses['processus']) is int and ses['processus'] == 3 and set(ses['prises']) == set(ARMS), 'Sessions3')
    sessions = {a:{n:[] for n in names} for a in ARMS}
    for arm in ARMS:
        need(len(ses['prises'][arm]) == 3, 'trois processus Session')
        for i, take in enumerate(ses['prises'][arm]):
            order = names[i:] + names[:i]
            need(take['ordre'] == order, 'ordre de tournée')
            fs = read(take, 'journaux/v12set/%s_r%d.jsonl'%(arm,i), expected(arm,order*2))
            for row in fs[37:]: sessions[arm][row['trame']].append(row)
    need(set(seen) == {p for p in files if p.endswith('.jsonl')}, 'journaux non affectés')
    need(equivalent(P.juger(r, str(RAW), True), r['jugement'], 'juge'), 'juge publié différent du rejeu')
    need(P.statistiques_session(r) == r['statistiques']['session_v12set'], 'statistiques Session publiées différentes')
    rng = random.Random(20261008)
    outcome = {}
    def stats(rows):
        out = {'passes':len(rows), 'wall_median_ms':st.median(x['wall_ns'] for x in rows)/1e6,
               'wall_max_ms':max(x['wall_ns'] for x in rows)/1e6,
               'above_100ms':sum(x['wall_ns'] > 100000000 for x in rows),
               'etapes_median_ms':{k:st.median(x['etapes_ns'][k] for x in rows)/1e6 for k in rows[0]['etapes_ns']},
               'pic_budget_max_octets':max(x['pic_octets'] for x in rows),
               'rss_max_octets':max(x['rss_max_octets'] for x in rows if x['rss_max_octets'] is not None),
               'cpu_s_median':st.median(x['cpu_ns'] for x in rows if x['cpu_ns'] is not None)/1e9,
               'cpu_s_sum':sum(x['cpu_ns'] for x in rows if x['cpu_ns'] is not None)/1e9}
        if 'recouvrement' in rows[0]:
            out['tour_median_ms'] = st.median(x['recouvrement']['tour_ns'] for x in rows)/1e6
            out['queue_median_ms'] = st.median(x['recouvrement']['queue_ns'] for x in rows)/1e6
        return out
    for f in FRAMES:
        med = {a:[st.median(x['wall_ns'] for x in rows) for rows in timing[f][a]] for a in ARMS}
        ratios = [b/a for a,b in zip(med['avant'], med['apres'])]
        logs = [math.log(x) for x in ratios]
        bootstrap = sorted(sum(logs[rng.randrange(5)] for _ in range(5))/5 for _ in range(10000))
        gm, lo, hi = math.exp(sum(logs)/5), math.exp(bootstrap[250]), math.exp(bootstrap[9749])
        j = r['jugement']['cas'][f]
        need(equivalent([gm,lo,hi], [j['moyenne_geometrique'],*j['ic95']], 'recalcul/'+f), 'IC reconstitué différent')
        outcome[f] = {'ratio_gm':gm,'ic95':[lo,hi], 'median_process_medians_ms':{a:st.median(med[a])/1e6 for a in ARMS},
                      'max_process_medians_ms':{a:max(med[a])/1e6 for a in ARMS},
                      'arms':{a:stats([x for rows in timing[f][a] for x in rows]) for a in ARMS}}
    session_out = {}
    for arm, table in sessions.items():
        per = {n:stats(rows) for n,rows in table.items()}
        need(all(len(rows) == 3 for rows in table.values()), 'trois chaudes par trame')
        old = r['statistiques']['session_v12set'][arm]
        need(all(per[n]['wall_median_ms'] == old['trames'][n]['mediane_ms'] and
                 per[n]['wall_max_ms'] == old['trames'][n]['max_ms'] for n in names), 'résumé37 différent')
        session_out[arm] = {'mediane_37_medianes_ms':st.median(x['wall_median_ms'] for x in per.values()),
                           'max_37_medianes_ms':max(x['wall_median_ms'] for x in per.values()),
                           'max_111_passes_ms':max(x['wall_max_ms'] for x in per.values()),
                           'trames_mediane_above_100ms':sum(x['wall_median_ms']>100 for x in per.values()),
                           'passes_above_100ms':sum(x['above_100ms'] for x in per.values()),
                           'chaudes_reunies':stats([x for rows in table.values() for x in rows]),
                           'trames':{n:{k:v for k,v in d.items() if k in
                                     ('wall_median_ms','wall_max_ms','above_100ms')} for n,d in per.items()}}
    # Contre-JSON dérivés d'un flux réel conforme : chaque garde doit refuser proprement.
    path = 'journaux/k5/ng00_apres_t00.jsonl'
    original = [loads(x) for x in (RAW/path).read_text().splitlines() if x.strip()]
    spec, smap = specs[path]
    scenarios = [
        ('exit_absent',lambda x:x.pop()),
        ('open_reason_invalide',lambda x:x[0].update(reason='memory_budget')),
        ('threads_bool',lambda x:x[1].update(threads=True)),
        ('liberation_bool',lambda x:x[2].update(**{'pass':False})),
        ('queue_incoherente',lambda x:x[1]['recouvrement'].update(fin_ns=x[1]['recouvrement']['fin_ns']-1)),
        ('memoire_absente',lambda x:x[1].pop('memoire_octets')),
        ('pic_entree_trop_bas',lambda x:x[1].update(pic_octets=0)),
        ('pic_device_partage',lambda x:x[1].update(pic_appareil_octets=1)),
        ('schema_sequentiel_a_la_place',lambda x:x[1].update(etapes_schema='sequentiel')),
        ('trame_etrangere',lambda x:x[1].update(trame='autre')),
        ('fins_hors_tour',lambda x:x[1]['fins_par_ordre_ns'][0].__setitem__(0,U64))]
    refused = []
    for label, mutate in scenarios:
        rows = copy.deepcopy(original); mutate(rows)
        try: validate(rows,spec,smap)
        except (ValueError,KeyError,TypeError): refused.append(label)
        else: raise ValueError('contre-JSON admis: '+label)
    need(r['jugement']['verdict'] == 'adopte' and all(v['ic95'][1] < 1 for v in outcome.values()), 'règle adoption')
    return {'admission':'conforme', 'processus':len(seen), 'passes':sum(len(x) for x in allfull.values()),
            'identites_processus':25,'identites_passes':106,'empreintes_distinctes_par_objet':len(hashes),
            'chrono_processus':30,'chrono_chaudes':270,'sessions_processus':6,'sessions_chaudes':222,
            'recouvert_passes':sum('recouvrement' in r for fs in allfull.values() for r in fs),
            'cpu_ns_manquants':sum(r['cpu_ns'] is None for fs in allfull.values() for r in fs),
            'rss_manquants':sum(r['rss_max_octets'] is None for fs in allfull.values() for r in fs),
            'verdict_lot':'adopte','regle_inchangee':True,'ng_k5':outcome,'session_37_k5':session_out,
            'contre_flux_refuses':refused, 'ecarts_flottants':FLOAT_DELTAS,
            'codes_processus':'entiers 0 archivés dans le rapport du pilote, pas reçus externes individuels',
            'limites':['pas de nouvelle exécution native','sources/binaires liés par provenance séparée',
                       'identités dans processus distincts des chronos sans digest',
                       'prise K10 identité seule, pas campagne chaude K10',
                       'gain du lot27eca→5f5, pas gain causal du seul recouvrement',
                       'fenêtres chevauchantes non additionnées au mur ni nommées CPU·s']}

for path, h in C['sources'].items():
    need(sha(blob(path)) == h, 'source épinglée modifiée')
for path, h in C['before_sources'].items():
    need(sha(subprocess.check_output(['git','-C',str(REPO),'show',C['before_commit']+':'+path])) == h,
         'source avant modifiée')
P = types.ModuleType('pilote_a_epingle'); P.__file__ = 'git:pilote_t2d_a.py'
exec(compile(blob('morsehgp3D_v12/microbancs/mes_t2d_a/pilote_t2d_a.py'), P.__file__, 'exec'), P.__dict__)
print(json.dumps(main(P), ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False))
