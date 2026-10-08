import argparse, copy, importlib.util, json, pathlib, tempfile, hashlib
parser=argparse.ArgumentParser()
parser.add_argument('--pilot',type=pathlib.Path,required=True)
args=parser.parse_args()
SRC=args.pilot
pins=json.loads((pathlib.Path(__file__).parent/'pins.json').read_text())
if hashlib.sha256(SRC.read_bytes()).hexdigest()!=pins['sources'][0]['sha256']:
    raise SystemExit('pilot source hash mismatch')
spec=importlib.util.spec_from_file_location('pilote_a',SRC); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

def rows(attendu, wall_bad=False):
    arm=attendu['bras']; after=arm=='apres'; ds=[]
    ds.append({'phase':'open','status':'ok','reason':'none','wall_ns':100,'budget_appareil':'partage'})
    for p,name in enumerate(attendu['trames']):
        stages={'P':5000,'C':10000,'G':25000 if after else 35000,'raccord':0,
                'TMVR':15000 if after else 40000,'T':(80000 if attendu['fils']>1 else 10000) if after else 25000,'M':5000,'V':2000,'R':3000}
        row={'phase':'full','pass':p,'trame':name,'voie':'device','status':'ok','coord_bits':21,
             'kmax':attendu['k'],'threads':attendu['fils'],'sites':100,'wall_ns':70000 if after else 100000,
             'etapes_ns':stages,'c_ns':{'parcours':1000,'feuilles':1000,'emission':1000,'fin_etage':1000,'transferts':1000,'publication':1000},
             'g_ns':{'tables':1000,'resolution':20000},'hors_mur_ns':{'validation':1000,'empreinte':1000 if attendu['empreinte'] else 0},
             'pic_octets':1000000,'cpu_ns':(70000 if after else 100000)*min(2,attendu['fils']),'rss_max_octets':10000000,'appareil_octets':10000,
             'epinglee_octets':10000,'pic_appareil_octets':10000}
        if after:
            row['etapes_schema']='recouvert'
            row['recouvrement']={'tour_ns':45000,'ouverture_ns':5000,'fin_g_ns':25000,'fin_ns':40000,'queue_ns':15000,
                                'fils_g_ns':10000,'fils_foret_ns':90000 if attendu['fils']>1 else 20000,'foret_apres_g_ns':10000,
                                'noyau_reprises':5,'noyau_arrets':2,'admis_octets':500000}
            row['fins_par_ordre_ns']=[[25000,30000,32000,0 if i==0 else 34000,36000] for i in range(attendu['k'])]
            if wall_bad: row['wall_ns']=1
        if attendu['empreinte']:row['full_sha256']='ab'*32
        ds += [row,{'phase':'liberation','pass':p,'liberation_ns':1000}]
    return ds+[{'phase':'exit','status':'ok','reason':'none'}]

def take(root, label, arm, frames, k=5, threads=48, digest=False, wall_bad=False):
    expected={'k':k,'fils':threads,'passes':len(frames),'bras':arm,'empreinte':digest,'trames':frames,'appareil':True}
    path=root/(label+'.jsonl');path.write_text(''.join(json.dumps(x,sort_keys=True)+'\n' for x in rows(expected,wall_bad)))
    summary=m.lire_prise(str(path),0,expected)
    return dict(journal=path.name,journal_sha256=m.sha256(path),code=0,bras=arm,attendu=expected,valide=True,**summary)

def report(root, wall_bad=False):
    bs={b:{'sha256':('12' if b=='avant' else '34')*32} for b in m.BRAS}
    out={'construction':{'binaires':bs},'campagne_k5':{'passes':10,'tours_demandes':5,'binaires_apres':{b:d['sha256'] for b,d in bs.items()},'trames':{}},
         'identite':{'prises':{}},'environnement':{'avant':{'gpu_apps':[]},'apres':{'gpu_apps':[]}}}
    for frame in m.TRAMES:
        tours=[]
        for i in range(5):tours.append({b:take(root,'camp_%s_%s_%s'%(frame,i,b),b,[frame]*10,wall_bad=wall_bad) for b in m.BRAS})
        out['campagne_k5']['trames'][frame]=tours
        for k in (5,10):
            key='%s_k%d'%(frame,k); n=2 if k==5 else 1
            out['identite']['prises'][key]={b:take(root,'id_'+key+'_'+b,b,[frame]*n,k,digest=True) for b in m.BRAS}
    for n in m.UNIFORMES:
        key='u%d_k5'%n
        out['identite']['prises'][key]={b:take(root,'id_'+key+'_'+b,b,['uniforme'],digest=True) for b in m.BRAS}
    out['identite']['prises']['v12set_k5']={b:take(root,'id_v12set_'+b,b,['frame%02d'%i for i in range(37)],digest=True) for b in m.BRAS}
    for b in m.BRAS:out['identite']['prises']['ng00_k5'][b+'_1fil']=take(root,'id_ng00_1fil_'+b,b,['ng00'],threads=1,digest=True)
    return out

result={}
with tempfile.TemporaryDirectory(prefix='a-admission-') as d:
    root=pathlib.Path(d); nominal=report(root)
    for name,obj in [('nominal',nominal),('identite_tronquee',{**nominal,'identite':{'prises':{'ng00_k5':nominal['identite']['prises']['ng00_k5']}}})]:
        j=m.juger(obj,str(root),verifier=True);result[name]={'verdict':j['verdict'],'refus':j['refus'],'identite_ok':j['identite_ok']}
    bad=report(root,True);j=m.juger(bad,str(root),verifier=True);result['mur_impossible']={'verdict':j['verdict'],'refus':j['refus'],'identite_ok':j['identite_ok'],'wall_ns':1,'P_plus_C_plus_G_plus_TMVR_ns':55000}
    a={'k':5,'fils':1,'passes':1,'bras':'apres','empreinte':False,'trames':['ng00'],'appareil':True}
    base=rows(a)
    for name,change in [('queue_fausse',lambda x:x[1]['recouvrement'].__setitem__('queue_ns',0)),('fils_bool',lambda x:x[1].__setitem__('threads',True)),('liberation_bool',lambda x:x[2].__setitem__('pass',False)),('cpu_absent',lambda x:x[1].pop('cpu_ns'))]:
        data=copy.deepcopy(base);change(data);p=root/(name+'.jsonl');p.write_text(''.join(json.dumps(x)+'\n' for x in data))
        try:m.lire_prise(str(p),0,a);result[name]='admis'
        except (ValueError,TypeError,KeyError) as e:result[name]='refus: '+str(e)
print(json.dumps(result,indent=2,sort_keys=True))

expected={'nominal':'adopte','identite_tronquee':'adopte','mur_impossible':'adopte'}
for name,verdict in expected.items():
    if result[name]['verdict']!=verdict or result[name]['refus']:
        raise SystemExit('witness changed: '+name)
for name in ('queue_fausse','fils_bool','liberation_bool','cpu_absent'):
    if result[name]!='admis':raise SystemExit('witness changed: '+name)
