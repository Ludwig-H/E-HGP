#!/usr/bin/env python3
"""Reconciliation de comptes publics et des six journaux existants, sans recalcul LiDAR."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PIN = '274592a30f6961cb7702125dcd2f031ff22b7df2'
PUBLIC = ROOT/'morsehgp3D_v12/receipts/mes_g1_m7_local_20261007'


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def near(actual, expected):
    return abs(actual-expected) <= max(0.000002, abs(expected)*0.00001)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--external-dir', type=Path, required=True,
                    help='dossier historique v12_g1, jamais copie dans le recu')
    args = ap.parse_args()
    names = ['README.md','mes_g1.json','mes_g1.md','mes_m7_local.json','mes_m7_local.md']
    paths = [PUBLIC/name for name in names]
    paths += [ROOT/'morsehgp3D_v12'/name for name in [
        'docs/CONTRAT_TOUR.md','docs/PLAN.md',
        'microbancs/mes_g1_saut/mes_g1.cpp','microbancs/mes_g1_saut/voisins.hpp',
        'microbancs/mes_m3_m4_tour/vidage/vidage_v11.cpp']]
    before = {p.relative_to(ROOT).as_posix():sha(p.read_bytes()) for p in paths}
    for p,h in before.items():
        raw = subprocess.run(['git','show',PIN+':'+p], cwd=ROOT, check=True, capture_output=True).stdout
        need(sha(raw)==h, 'source pin '+p)
    g1 = json.loads((PUBLIC/'mes_g1.json').read_text())
    m7 = json.loads((PUBLIC/'mes_m7_local.json').read_text())
    journals, external_hashes = {}, {}
    for p in args.external_dir.rglob('*.jsonl'):
        journals[sha(p.read_bytes())] = p
    def journal(data, label):
        h = data['journal_sha256']; need(h in journals, 'missing journal '+label)
        p = journals[h]; external_hashes[label] = (p,h)
        return [json.loads(line) for line in p.read_text().splitlines() if line.startswith('{')]
    gresults = {}
    for case,d in g1['cas'].items():
        raw = journal(d, 'G1/'+case)
        for name in ['entree','voisins','bilan']:
            need([x for x in raw if x.get('phase')==name]==[d[name]], case+' raw '+name)
        need([x for x in raw if x.get('phase')=='ordre']==d['ordres'], case+' raw orders')
        K = d['entree']['K']; b = d['bilan']; orders = d['ordres']
        need([o['k'] for o in orders]==list(range(2,K+1)), case+' complete orders')
        need(b['code']==0 and b['ecarts_juge']==b['ecarts_voisins']==0, case+' verdict')
        for o in orders:
            need(o['parties']==o['route1']+o['route2']['parties']+o['route3']['parties'], case+' partition')
            for r in ['route2','route3']:
                need(o[r]['parties']==o[r]['hors_catalogue']+o[r]['au_catalogue'], case+' catalogue split')
            need(o['ecarts_juge']==0 and o['lem_hors_cat']['contre_exemples']==0, case+' per-order verdict')
        for r in ['route2','route3']:
            need(b[r]['parties']==sum(o[r]['parties'] for o in orders), case+' route sum')
        for ensemble,vals in b['route2']['ensembles'].items():
            for key in ['certifiees','cible_egale_v11','cible_naissance','cible_v11_naissance_sur_certifiees','ecarts_juge']:
                need(vals[key]==sum(o['route2']['ensembles'][ensemble][key] for o in orders), case+' ensemble sum '+key)
            need(near(vals['part_certifiees'],vals['certifiees']/b['route2']['parties']), case+' certified rate')
        count=b['route2']['parties']; v=b['route2']['ensembles']['voisins']
        gresults[case]={'saturated':count,'certifiable':v['certifiees'],
                        'fraction':v['certifiees']/count,
                        'changed_target_fraction':1-v['cible_egale_v11']/v['certifiees'],
                        'complete_in_catalogue':b['route3']['census_complets_sphere_au_catalogue_s_etoile_hors_de_f']}
    mresults = {}
    for case,d in m7['cas'].items():
        raw = journal(d, 'M7/'+case)
        need([x for x in raw if x.get('phase')=='profil_resolution']==d['ordres'], case+' raw profile')
        K=int(case.split('_k')[1]); orders=d['ordres']
        need([o['k'] for o in orders]==list(range(2,K+1)), case+' profile coverage')
        sums={}; cycles=0; ratios=[]; biases=[]
        for o in orders:
            c=o['composantes']; ctrl=o['controle_vidage']; routes=o['routes_v12']
            need(ctrl['conforme'] and ctrl['graines_identiques'], case+' identity')
            need(sum(x['cycles'] for x in c.values())==o['cycles_total'], case+' time partition')
            for name,x in c.items():
                sums[name]=sums.get(name,0)+x['cycles']
                need(near(x['part'],x['cycles']/o['cycles_total']), case+' share')
            pairs={'sonde':'sondes','proposition_t1':'parties','census_sature':'route_census_saturated',
                   'census_complet':'route_census_complete','saut':'action_interior','trace_stricte':'action_trace_ou_terminal'}
            for name,key in pairs.items():need(c[name]['occurrences']==ctrl[key], case+' route occurrence')
            need(routes['t1']+routes['cert_table']+routes['repli_table']==ctrl['route_catalogue'], case+' catalogue route')
            gorder=next(x for x in g1['cas'][case]['ordres'] if x['k']==o['k'])
            need(ctrl['parties']==gorder['parties'] and ctrl['route_census_saturated']==gorder['route2']['parties'] and ctrl['route_census_complete']==gorder['route3']['parties'], case+' G1/M7 reconciliation')
            cycles+=o['cycles_total']; ratios.append(o['secondes_total']/o['secondes_replique_v12_non_instrumentee'])
            biases.append(o['cycles_par_section_vide'])
        mresults[case]={'cycle_shares':{k:v/cycles for k,v in sums.items()},
                        'instrumented_uninstrumented_ratio_range':[min(ratios),max(ratios)],
                        'empty_section_bias_cycles_range':[min(biases),max(biases)]}
    binary_hashes = {}
    for name,expected in [('mhgp12_mes_g1',g1['binaire_sha256']),('mhgp12_vidage',m7['binaire_sha256'])]:
        matches=[p for p in args.external_dir.rglob(name) if sha(p.read_bytes())==expected]
        need(matches, 'historical binary '+name)
        binary_hashes[name]=expected; external_hashes['binary/'+name]=(matches[0],expected)
    for label,(p,h) in external_hashes.items():need(sha(p.read_bytes())==h,'external changed '+label)
    need(before=={p:sha((ROOT/p).read_bytes()) for p in before}, 'sources changed')
    total=sum(v['saturated'] for k,v in gresults.items() if k.endswith('_k5'))
    success=sum(v['certifiable'] for k,v in gresults.items() if k.endswith('_k5'))
    print(json.dumps({'schema':'ehgp.audit.t2.measure_receipts.v1','pin':PIN,'source_sha256':before,
                      'witness_sha256':sha(Path(__file__).read_bytes()),'sources_before_after_unchanged':True,
                      'historical_journal_sha256':{k:h for k,(_,h) in external_hashes.items() if not k.startswith('binary/')},
                      'historical_binary_sha256':binary_hashes,'g1':gresults,'m7':mresults,
                      'k5_total':{'saturated':total,'certifiable':success,'fraction':success/total},
                      'limits':['No real-data replay','Dump hashes recorded but payloads not reread','Historical linked binary source closure not rebuilt','Raw instrumented local cycles, not G4 performance','G-L3 not adopted']},indent=2,sort_keys=True))


if __name__=='__main__':
    main()
