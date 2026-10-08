#!/usr/bin/env python3
"""Relecture de métadonnées FUL1 existantes ; aucun moteur ni payload."""
import argparse,hashlib,json,re,subprocess,types
from pathlib import Path
HERE=Path(__file__).resolve().parent

def need(ok,label):
    if not ok:raise ValueError(label)

def verify(raw,pin):
    need(len(raw)==pin['bytes'] and hashlib.sha256(raw).hexdigest()==pin['sha256'],'hash')

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True);ap.add_argument('--a6',type=Path,required=True);a=ap.parse_args()
    c=json.loads((HERE/'capture.json').read_text())
    raw=subprocess.check_output(['git','-C',str(a.repo),'show',c['reader_git']+':'+c['reader_path']]);verify(raw,c['reader'])
    lf=types.ModuleType('pinned_lf');exec(compile(raw,'pinned_lf','exec'),lf.__dict__)
    mraw=(a.a6/'v12set/bundle_manifest.json').read_bytes();verify(mraw,c['input_manifest'])
    cases=[r for r in sorted(json.loads(mraw)['cases'],key=lambda x:x['count']) if r['count']>60000]
    expected=[(r['name'],r['count']) for r in cases]
    need(len(expected)==21 and [list(x) for x in expected]==c['expected'],'cohort')
    for p,pin in c['inputs'].items():verify((a.a6/p).read_bytes(),pin)
    log=(a.a6/'identite_grandes_72.log').read_text()
    need('codes=0/0/0 empreintes=21/21/21 IDENTIQUE' in log and c['external_codes']==[0,0,0],'external codes')
    all_digests=[]
    for arm,entry in c['arms'].items():
        f=a.a6/'identite72'/('grandes_'+arm+'.jsonl');raw=f.read_bytes();verify(raw,entry['journal'])
        rows=[json.loads(l) for l in raw.splitlines()]
        need(len(rows)==43 and [r['phase'] for r in rows]==['full','liberation']*21+['exit'],'phase/cardinality')
        digests=[]
        for i,(r,(name,count)) in enumerate(zip(rows[::2][:-1],expected)):
            need((r['pass'],r['trame'],r['sites'],r['voie'],r['coord_bits'],r['kmax'],r['threads'],r['status'])==(i,name,count,'cpu',21,5,3,'ok'),'configuration')
            need(re.fullmatch('[0-9a-f]{64}',r['full_sha256']) is not None,'nonempty digest');digests.append(r['full_sha256'])
        att={'voie':'cpu','k':5,'fils':3,'passes':21,'empreinte':True,'trames':expected,'budget_appareil':'partage','bits':21,'schema':entry['schema']}
        lu=lf.parse_output(0,raw.decode(),att)
        need(lu['etat']==entry['reader_state']=='ok' and rows[-1]==entry['exit'],'strict admission')
        need(digests==c['digests'],'FUL1 identity');all_digests.append(digests)
        verify(f.read_bytes(),entry['journal'])
    need(all(x==all_digests[0] for x in all_digests) and c['all_21_ful1_equal'] is True,'three arms')
    for p,pin in c['additional_observations'].items():verify((a.a6/p).read_bytes(),pin)
    # Current ELF observations are explicitly not an execution-time closure.
    for arm,build in [('base','build_term72'),('a6','build_a6')]:verify((a.a6/build/'mhgp12_full_probe').read_bytes(),c['current_probe_artifacts'][arm])
    for p,pin in c['inputs'].items():verify((a.a6/p).read_bytes(),pin)
    print('21 expected frames x three arms: 63 FULL/63 releases, external codes0, strict reader admitted, all 21 FUL1 equal; no interleaving qualification')
if __name__=='__main__':main()
