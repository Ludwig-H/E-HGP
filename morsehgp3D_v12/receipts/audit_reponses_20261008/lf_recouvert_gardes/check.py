#!/usr/bin/env python3
"""Usage : python [-O] check.py CAPTURE_LF RETOUR_A DEPOT. Aucun moteur exécuté."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types
sys.dont_write_bytecode = True
HERE=Path(__file__).resolve().parent
C=json.loads((HERE/'capture.json').read_text())
SOURCE, RAW, REPO=map(Path,sys.argv[1:4])
def need(ok,why):
    if not ok: raise ValueError(why)
def sha(b): return hashlib.sha256(b).hexdigest()
def module(content):
    m=types.ModuleType('lecteur_capture');exec(compile(content,'lecteur_epingle.py','exec'),m.__dict__);return m
for path,h in C['files'].items():need(sha((SOURCE/Path(path).name).read_bytes())==h,'source changée')
need(sha((HERE/'proposition.patch').read_bytes())==C['patch_sha256'],'patch modifié')
need(sha((RAW/'rapport_t2d_a.json').read_bytes())==C['report_sha256'],'rapport A modifié')
for p,h in C['logs'].items():need(sha((RAW/p).read_bytes())==h,'journal A modifié')
for path,h in C['published_files'].items():
    need(sha(subprocess.check_output(['git','-C',str(REPO),'show',C['published_commit']+':'+path]))==h
         and h==C['files'][path], 'capture différente du pin livré')
source=(SOURCE/'lecteur_full.py').read_bytes();old=module(source)
with tempfile.TemporaryDirectory() as t:
    target=Path(t)/'morsehgp3D_v12/microbancs/outils/lecteur_full.py';target.parent.mkdir(parents=True);target.write_bytes(source)
    subprocess.run(['git','apply','--check',str(HERE/'proposition.patch')],cwd=t,check=True,capture_output=True)
    subprocess.run(['git','apply',str(HERE/'proposition.patch')],cwd=t,check=True,capture_output=True)
    candidate=target.read_bytes();need(sha(candidate)==C['candidate_sha256'],'postimage incorrecte');new=module(candidate)
report=json.loads((RAW/'rapport_t2d_a.json').read_text())
takes={}
def visit(x):
    if isinstance(x,dict):
        if 'journal' in x and 'attendu' in x:
            need(x['journal'] not in takes,'journal partagé');takes[x['journal']]=x
        else:
            for v in x.values():visit(v)
    elif isinstance(x,list):
        for v in x:visit(v)
visit(report)
need(set(takes)==set(C['logs']),'inventaire A')
def spec(t):
    a=t['attendu']
    return dict(voie='appareil',k=a['k'],fils=a['fils'],passes=a['passes'],empreinte=a['empreinte'],
                trames=list(zip(a['trames'],t['sites'])),budget_appareil='partage',bits=21,schema=a['schema'])
stats={'processus':0,'passes':0,'recouvert_processus':0,'recouvert_passes':0,'sequentiel_processus':0,'sequentiel_passes':0,
       'R_avant_V':0,'V_avant_R':0}
for path,t in takes.items():
    text=(RAW/path).read_text();a=spec(t)
    o=old.parse_output(t['code'],text,a);n=new.parse_output(t['code'],text,a)
    need(o['etat']=='ok' and n==o,'positif natif changé: '+path)
    stats['processus']+=1;stats['passes']+=len(n['passes']);stats[a['schema']+'_processus']+=1;stats[a['schema']+'_passes']+=len(n['passes'])
    if a['schema']=='recouvert':
        for row in n['passes']:
            for e in row['fins_par_ordre_ns'][1:]:
                stats['R_avant_V']+=e[4]<e[3];stats['V_avant_R']+=e[3]<e[4]
path='journaux/k5/ng00_apres_t00.jsonl';a=spec(takes[path]);original=[json.loads(x) for x in (RAW/path).read_text().splitlines()]
def ends(x):return x[1]['fins_par_ordre_ns']
cases=[('tour_hors_mur',lambda x:x[1]['recouvrement'].update(tour_ns=x[1]['wall_ns']+1)),
       ('ouverture_incoherente',lambda x:x[1]['recouvrement'].update(ouverture_ns=0)),
       ('fins_G_nulles',lambda x:[e.__setitem__(0,0) for e in ends(x)]),
       ('noyau_avant_G',lambda x:ends(x)[0].__setitem__(1,0)),
       ('M_avant_noyau',lambda x:ends(x)[0].__setitem__(2,0)),
       ('R_avant_M',lambda x:ends(x)[0].__setitem__(4,0)),
       ('V_avant_M',lambda x:ends(x)[1].__setitem__(3,0)),
       # Ce flux réel a M(3)>M(4) : seul le prédécesseur de l'ordre inférieur est violé ici.
       ('V_avant_M_inferieur',lambda x:ends(x)[3].__setitem__(3,ends(x)[3][2])),
       ('maximum_G_faux',lambda x:[e.__setitem__(0,x[1]['recouvrement']['ouverture_ns']) for e in ends(x)])]
need(ends(original)[2][2]>ends(original)[3][2],'témoin ordre inférieur changé')
def read(m,rows):return m.parse_output(0,'\n'.join(json.dumps(x) for x in rows)+'\n',a)
res={}
for name,mut in cases:
    x=copy.deepcopy(original);mut(x);before=read(old,x);after=read(new,x)
    need(before['etat']=='ok' and after['etat']=='illisible', 'contre-flux non causal: '+name)
    res[name]={'avant':before['etat'],'propose':after['etat'],'raison':after['raison']}
# Limite de l'émetteur : g_end=max(1,g_max), mais maxima locaux non rehaussés.
# Modifier uniquement la première passe ; les autres restent natives et conformes.
x=copy.deepcopy(original);row=x[1];q=row['recouvrement'];g=q['ouverture_ns']
for e in row['fins_par_ordre_ns']:e[0]=g
q['fin_g_ns']=g+1;row['etapes_ns']['G']=g+1
q['queue_ns']=q['fin_ns']-q['fin_g_ns'];row['etapes_ns']['TMVR']=q['queue_ns']
need(read(old,x)['etat']=='ok' and read(new,x)['etat']=='ok','sentinelle1ns refusée')
# Mutants du correctif : chaque retrait syntaxiquement valide doit laisser passer son témoin dédié.
mutants=[('enveloppe'," or st['P'] + st['C'] + rec['tour_ns'] > row['wall_ns']",'', 'tour_hors_mur'),
         ('ouverture',"rec['ouverture_ns'] != g['ouverture'] or ",'','ouverture_incoherente'),
         ('pred_M_inferieur',"max(e[2], ends[j-1][2])","e[2]",'V_avant_M_inferieur'),
         ('maximum_G',"if rec['fin_g_ns'] != max_g and not (max_g == rec['ouverture_ns'] and rec['fin_g_ns'] == max_g + 1):",
          'if False:','maximum_G_faux')]
killed=[]
for name,find,repl,witness in mutants:
    code=candidate.decode();need(code.count(find)==1,'motif mutant absent');bad=module(code.replace(find,repl,1))
    need(read(bad,original)['etat']=='ok','mutant détruit le positif')
    x=copy.deepcopy(original);dict(cases)[witness](x)
    need(read(bad,x)['etat']=='ok','mutant non exposé');killed.append(name)
print(json.dumps({'positifs_A':stats,'contre_flux':res,'sentinelle_offset_G_1ns':'admis',
                  'mutants_proposition_exposes':killed,'postimage_sha256':C['candidate_sha256'],
                  'published_commit':C['published_commit'],
                  'scope':'compatibilité avec 61 traces déjà admises ; pas de nouvelle qualification native'},
                 ensure_ascii=False,sort_keys=True,indent=2))
