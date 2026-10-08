#!/usr/bin/env python3
"""Diagnostic additif A6b2 ; journaux publics et sources Git, aucun moteur."""
from pathlib import Path
import argparse, collections, hashlib, importlib.util, json, statistics, subprocess, sys, tempfile
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PIN = 'f2c106d93f1c835f129cef60e4e5e65ae185bd65'
PREFIX = 'morsehgp3D_v12/'
ARMS = ('avant','avant_bis','apres')
COMPONENTS = ('wall','P','C','open','G_region','queue','other')
ARCHIVE_REPORT = '23d92706dc9b66cca094537f3b14db5bc71c51bf8e9d3ebecffdc2ad9f967959'
PUBLIC_REPORT = 'f9497277b132145f148b08ece67ac8b2af922e5f30f17034e17aeb1536ae74fc'
PUBLIC_PATH = PREFIX+'receipts/g4_a6b_20261008/a6b2/resultats/cmd/001_t2da6b_pilote/files/t2da6b/rapport_t2d_a6b.json'

def need(ok, why):
    if not ok: raise ValueError(why)

def pin(b): return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())

def fields(row):
    e, g, w = row['etapes_ns'], row['g_ns'], row['wall_ns']
    out = dict(wall=w, P=e['P'], C=e['C'], open=g['ouverture'],
        G_region=e['G']-g['ouverture'], queue=e['TMVR'], other=w-sum(e.values()))
    need(e['raccord']==0 and all(type(x) is int and x>=0 for x in out.values()), 'partition')
    need(out['wall']==sum(out[k] for k in COMPONENTS[1:]), 'additive identity')
    for key in ('G','foret','foret_apres_g','T','M','V','R'):
        out['thread_'+key] = row['fenetres_ns'][key]
    need(type(row['cpu_ns']) is int, 'CPU metadata absent')
    out['CPU'] = row['cpu_ns']
    out['kernel_jobs'] = row['recouvrement']['noyau_reprises']
    out['kernel_stops'] = row['recouvrement']['noyau_arrets']
    return out

def group(pairs):
    sums = {arm:collections.Counter() for arm in ('avant','apres')}
    last = {arm:collections.Counter() for arm in ('avant','apres')}
    lag = {arm:[dict(positive=0,sum_ns=0) for _ in range(5)] for arm in ('avant','apres')}
    for before, after in pairs:
        need((before['trame'],before['pass'])==(after['trame'],after['pass']), 'pair mismatch')
        for arm,row in [('avant',before),('apres',after)]:
            sums[arm].update(fields(row))
            ends = row['fins_par_ordre_ns']
            need(len(ends)==5, 'K5 ends')
            maximum = max((value,k,column) for k,order in enumerate(ends,1) for column,value in enumerate(order))
            need(sum(value==maximum[0] for order in ends for value in order)==1, 'ambiguous final end')
            last[arm]['K%d:%s'%(maximum[1],('G','kernel','M','V','R')[maximum[2]])] += 1
            for k,order in enumerate(ends):
                delay=max(0,order[1]-row['etapes_ns']['G'])
                lag[arm][k]['positive'] += delay>0
                lag[arm][k]['sum_ns'] += delay
    delta={key:sums['apres'][key]-sums['avant'][key] for key in sums['avant']}
    need(delta['wall']==sum(delta[k] for k in COMPONENTS[1:]), 'paired additive identity')
    return dict(pairs=len(pairs), sums_ns={k:dict(v) for k,v in sums.items()},
        delta_sums_ns=delta, last_published_end={k:dict(sorted(v.items())) for k,v in last.items()},
        kernel_lag_global_G=lag)

def run(args):
    rels=['src/tower/pipeline_run.cpp','src/tower/pipeline.cpp','src/tower/pipeline.hpp',
          'src/tower/pipeline_steps.cpp','src/tower/tower.hpp','bench/full_probe.cpp',
          'microbancs/outils/lecteur_full.py']
    sources={rel:subprocess.check_output(['git','-C',str(args.repo),'show',PIN+':'+PREFIX+rel]) for rel in rels}
    report_raw=(args.results/'rapport_t2d_a6b.json').read_bytes()
    need(pin(report_raw)['sha256']==ARCHIVE_REPORT, 'use original returned archive report, not sanitized copy')
    report=json.loads(report_raw)
    public_raw=(args.repo/PUBLIC_PATH).read_bytes()
    need(pin(public_raw)['sha256']==PUBLIC_REPORT, 'sanitized public report changed')
    original_without_paths=json.loads(report_raw)
    public_without_paths=json.loads(public_raw)
    sanitized=[]
    for arm in ARMS:
        left=original_without_paths['construction']['binaires'][arm]
        right=public_without_paths['construction']['binaires'][arm]
        need(type(left['chemin']) is str and type(right['chemin']) is str and left['chemin']!=right['chemin'], 'binary path sanitization')
        left.pop('chemin');right.pop('chemin')
        sanitized.append(['construction','binaires',arm,'chemin'])
    need(original_without_paths==public_without_paths, 'public report changed beyond declared paths')
    need(report['cohorte_demandee']['passes']==10 and report['cohorte_demandee']['tours']==5
         and report['cohorte_demandee']['tours_grandes']==6, 'campaign size')
    need(len(report['grandes']['trames'])==21, 'large cohort')
    inventory={};groups={};per_process={};all_count=0;selected=0
    with tempfile.TemporaryDirectory() as temp:
        path=Path(temp)/'lecteur_full.py';path.write_bytes(sources['microbancs/outils/lecteur_full.py'])
        spec=importlib.util.spec_from_file_location('a6b_steps_lf',path)
        lf=importlib.util.module_from_spec(spec);spec.loader.exec_module(lf)
        def process(entry):
            nonlocal all_count
            rel=entry['journal'];need(rel.startswith('journaux/'),'journal location')
            raw=(args.results/rel).read_bytes();inventory[rel]=pin(raw)
            err=(args.results/(rel+'.err')).read_bytes();need(not err.strip(),'stderr')
            inventory[rel+'.err']=pin(err)
            need(pin(raw)['sha256']==entry['journal_sha256'] and entry['code']==0 and entry['etat']=='ok','journal pin/code')
            parsed=lf.parse_output(entry['code'],raw.decode('ascii'),entry['attendu'])
            need(parsed['etat']=='ok','strict FULL reader')
            rows=parsed['passes'];all_count+=len(rows)
            need(all(r['threads']==48 and r['kmax']==5 and r['coord_bits']==21 for r in rows),'regime')
            for r in rows: fields(r)
            return rows
        for label in ('ng00','ng01','ng02','grandes'):
            rounds=report['grandes']['tours'] if label=='grandes' else report['ng']['trames'][label]
            need(len(rounds)==(6 if label=='grandes' else 5),'round count')
            pairs=[];medians={arm:[] for arm in ARMS};frames=collections.defaultdict(list)
            for iteration,round_ in enumerate(rounds):
                rows={arm:process(round_[arm]) for arm in ARMS}
                start=21 if label=='grandes' else 1
                count=42 if label=='grandes' else 10
                need(all(len(v)==count for v in rows.values()),'passes')
                hot={arm:values[start:] for arm,values in rows.items()}
                for arm,values in hot.items():
                    medians[arm].append(statistics.median(r['wall_ns'] for r in values))
                pairs.extend(zip(hot['avant'],hot['apres']))
                if label=='grandes':
                    for before,after in zip(hot['avant'],hot['apres']): frames[before['trame']].append((before,after))
            groups[label]=group(pairs);selected+=2*len(pairs);per_process[label]=medians
            if label=='grandes':
                groups[label]['frame_count']=len(frames)
                groups[label]['selected_frames']={name:group(frames[name]) for name in (
                    'kitti_ng_02_001606','kitti_ng_00_001896','kitti_ng_08_002119')}
    # The full probe has no public hint counter, no N/H split; their absence is evidence scope.
    probe=sources['bench/full_probe.cpp'].decode()
    need('hint_jobs_during_g' not in probe and 'hinted_reps' not in probe, 'probe changed hint scope')
    need(all('hint_jobs_during_g' not in json.dumps(json.loads(line)) for rel in inventory if rel.endswith('.jsonl')
             for line in (args.results/rel).read_text().splitlines()), 'unexpected emitted hint field')
    capture=dict(source_git=PIN,source_pins={k:pin(v) for k,v in sources.items()},report=pin(report_raw),
        report_primary='original returned archive',public_report=pin(public_raw),sanitized_fields=sanitized,
        public_report_other_fields_identical=True,
        journal_inventory=inventory,processes=len(inventory)//2,full_passes_checked=all_count,
        selected_full_passes=selected,components=list(COMPONENTS),
        metric='arithmetic sum of after-before per same round/frame/pass; divide by pair count for mean',
        coverage='diagnostic only; cohort/provenance/verdict admission is a separate receipt',
        no_native_execution=True,no_payload_read=True)
    result=dict(groups=groups,process_warm_wall_medians_ns=per_process,
        exact_partition_verified=True,hint_counters_emitted=False,numbering_history_times_separate=False)
    return capture,result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,required=True)
    parser.add_argument('--results',type=Path,required=True)
    parser.add_argument('--write',action='store_true')
    args=parser.parse_args();capture,result=run(args)
    for name,obj in [('capture.json',capture),('results.json',result)]:
        path=HERE/name
        if args.write:path.write_text(json.dumps(obj,ensure_ascii=False,separators=(',',':'))+'\n')
        else:need(json.loads(path.read_text())==obj,name+' changed')
    print(json.dumps(dict(processes=capture['processes'],full_passes_checked=capture['full_passes_checked'],
        selected_full_passes=capture['selected_full_passes'],paired_partition=True)))

if __name__=='__main__':main()
