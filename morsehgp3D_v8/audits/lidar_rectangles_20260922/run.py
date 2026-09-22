"""Audit runner: full native fronts; stratified rectangle samples, never a FULL timing."""
import argparse, hashlib, json, platform, statistics, subprocess, sys
from pathlib import Path

ap=argparse.ArgumentParser()
ap.add_argument('--binary',type=Path,required=True)
ap.add_argument('--output',type=Path,required=True)
ap.add_argument('--samples',type=int,default=24)
ap.add_argument('--repeat',type=int,default=3)
a=ap.parse_args()
root=Path(__file__).resolve().parents[4]
sys.path.insert(0,str(root/'morsehgp3D_v8/bench'))
from run_ground_baseline import inputs_1mm
inputs,manifests=inputs_1mm()
a.output.mkdir(parents=True,exist_ok=False)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
report={'scope':'full-cloud native front, sampled rectangles filter only; no HGP atlas or FULL timing',
        'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
        'source_tree':subprocess.check_output(['git','rev-parse','HEAD:morsehgp3D_v8/src'],cwd=root,text=True).strip(),
        'binary_sha256':sha(a.binary),'platform':platform.platform(),'inputs':inputs,'manifest_sha256':manifests,
        'samples_per_mass_class':a.samples,'repeat':a.repeat,'classes':{'0':'1','1':'2..63','2':'64..1023','3':'1024..65536','4':'>65536 not benchmarked'},
        'modes':{'0':'native rectangle then native pairs','1':'reuse singleton rectangle decision','2':'mode1 plus selective factor plans with h','3':'mode1 plus all nonsingleton factor plans'},'rows':[]}
for scene,entry in inputs.items():
    for k in (5,10):
        stem=f'scene_{scene}_K{k}'
        command=[str(a.binary.resolve()),str(root/entry['path']),str(k),str(a.samples),str(a.repeat)]
        with (a.output/(stem+'.json')).open('w') as f:
            p=subprocess.run(command,stdout=f,stderr=subprocess.PIPE,text=True,timeout=180)
        (a.output/(stem+'.stderr')).write_text(p.stderr)
        if p.returncode: raise RuntimeError(f'{stem}: return {p.returncode}: {p.stderr}')
        raw=json.loads((a.output/(stem+'.json')).read_text())
        if raw['n']!=entry['n'] or raw['pair_lane_comparisons']!='pass': raise RuntimeError('scope or result mismatch')
        rows=[]
        for c in range(4):
            for mode in range(4):
                rs=[r for r in raw['measures'] if r['class']==c and r['mode']==mode]
                if not rs:continue
                row=dict(rs[0]);row.pop('rep');row['cpu_ms']=statistics.median(r['cpu_ms'] for r in rs)
                for r in rs:
                    if any(r[key]!=row[key] for key in ('surviving_pairs','pair_searches','plans','factor_sites','local_tests','singleton_reuses')):
                        raise RuntimeError('nondeterministic discrete counters')
                rows.append(row)
        summary={'scene':scene,'K':k,'n':raw['n'],'front_products':raw['front_products'],
                 'front_cpu_ms':raw['front_cpu_ms'],'classes':raw['classes'],'medians':rows,
                 'raw_sha256':sha(a.output/(stem+'.json'))}
        report['rows'].append(summary)
        (a.output/'SUMMARY.partial.json').write_text(json.dumps(report,indent=2)+'\n')
        print('LIDAR_AUDIT '+json.dumps(summary,separators=(',',':')),flush=True)
report['status']='completed'
(a.output/'SUMMARY.json').write_text(json.dumps(report,indent=2)+'\n')
(a.output/'SUMMARY.partial.json').unlink()
print('LIDAR_AUDIT_COMPLETE '+json.dumps({'status':'completed','commit':report['commit'],'rows':len(report['rows'])}),flush=True)
