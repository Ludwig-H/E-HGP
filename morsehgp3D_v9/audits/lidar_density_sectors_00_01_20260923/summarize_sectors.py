#!/usr/bin/env python3
"""Verify and consolidate K5/K10 density and spatial slopes for scenes 00/01."""
import hashlib
import json
import math
import re
import struct
from pathlib import Path

ROOT=Path('/tmp/mhgp9-density-audit-20260923/sectors_00_01')
CROSS=ROOT.parent/'crossscene'
BASE=Path('/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6')
V12=Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/lidar_scaling_local_20260923')
BIN=Path('/workspaces/E-HGP/build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe')
SECTORS=('full','half_x_neg','half_x_nonneg','quarter_x_neg_y_neg',
         'quarter_x_neg_y_nonneg','quarter_x_nonneg_y_neg','quarter_x_nonneg_y_nonneg')
CHILDREN={'full':('half_x_neg','half_x_nonneg'),
          'half_x_neg':('quarter_x_neg_y_neg','quarter_x_neg_y_nonneg'),
          'half_x_nonneg':('quarter_x_nonneg_y_neg','quarter_x_nonneg_y_nonneg')}
DENSITIES=('quarter','half','full')
METRICS=('expanded_pairs','core_sites','dead_core_form_sites','dead_core_uniform_tests',
         'core_cover_node_visits','core_cover_bound_tests','atlas_point_tests',
         'q3_emitted','q4_emitted','catalogue_balls','chain_cpu_s','chain_total_ms')
MASK=(1<<64)-1

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def fnv_input(p):
    b=p.read_bytes();assert len(b)%12==0
    h=14695981039346656037
    for w in (len(b)//12,*struct.unpack(f'<{len(b)//4}I',b)):
        for c in struct.pack('<Q',w):h=((h^c)*1099511628211)&MASK
    return f'{h:016x}'

def words(p):
    b=p.read_bytes();assert len(b)%4==0
    return struct.unpack(f'<{len(b)//4}I',b)

def paths(scene,density,sector):
    if density=='full':
        base=BASE/f'scene_{scene}_grid'
        return (base/f'{sector}.u32le',base/f'{sector}.site_ids.u32le',
                base/f'{sector}.original_site_ids.u32le')
    if sector=='full':
        base=CROSS/f's{scene}_{density}_full'
    else:
        base=ROOT/f's{scene}_{density}_{sector}'
    return (Path(f'{base}.u32le'),Path(f'{base}.site_ids.u32le'),
            Path(f'{base}.original_site_ids.u32le'))

def input_manifest(scene,density,sector,man,cross,src):
    if density=='full':return src[scene]['datasets'][sector]
    if sector=='full':return cross['scenes'][scene]['datasets'][density]
    return man['source_scenes'][scene]['density'][density][sector]

def case(scene,density,sector,k,man,cross,src):
    p,ids,oid=paths(scene,density,sector)
    d=input_manifest(scene,density,sector,man,cross,src)
    assert sha(p)==d['points_sha256']
    assert sha(ids)==d['site_ids_sha256']
    assert sha(oid)==d['original_site_ids_sha256']
    assert len(p.read_bytes())//12==d['sites']
    if density=='full':
        rpath=V12/'out'/f's{scene}_k{k}_w8_r0'/f's{scene}_k{k}_s8_w8_r0_piece_{sector}.json'
        source='v12_sensor_piece'
    elif sector=='full':
        rpath=CROSS/f's{scene}_{density}_full_k{k}.json'
        source='crossscene_full_density'
    else:
        rpath=ROOT/f's{scene}_{density}_{sector}_k{k}.json'
        source='new_sector_density'
    r=json.loads(rpath.read_text())
    assert r['exit_code']==0
    if source=='v12_sensor_piece':
        assert r['input']['points_sha256']==sha(p)
        assert r['input']['site_ids_sha256']==sha(ids)
    else:
        assert r['input_sha256']==sha(p)
        assert r['binary_sha256']==sha(BIN)
    q=r['probe']
    assert q['schema']=='mhgp9_tower_probe_v12' and q['status']=='complete_relative'
    assert q['input']['format']=='u32le' and q['input']['sites']==d['sites']
    assert q['input']['hash']==fnv_input(p)
    assert q['input']['grid']==('unspecified' if source=='v12_sensor_piece' else '1mm')
    assert q['options']['K']==k and q['options']['K_effective']==k
    assert q['options']['s']==8 and q['options']['workers']==8 and q['options']['tower_static_threads']==8
    assert len(q['orders'])==k and [x['K'] for x in q['orders']]==list(range(1,k+1))
    assert re.fullmatch('[0-9a-f]{16}',q['tower_digest'])
    assert q['catalogue']['balls']==q['catalogue']['unique_keys']
    assert q['catalogue']['balls']==sum(q['catalogue']['by_qmin'])
    assert q['generator']['q3_emitted']==q['catalogue']['q3_presentations']
    assert q['generator']['q4_emitted']==q['catalogue']['q4_presentations']
    return {
        'scene':scene,'density':density,'sector':sector,'K':k,'source':source,
        'n':d['sites'],'input_sha256':sha(p),'site_ids_sha256':sha(ids),
        'original_site_ids_sha256':sha(oid),'input_fnv64':q['input']['hash'],
        'grid_label':q['input']['grid'],'status':q['status'],
        'tower_digest':q['tower_digest'],'receipt_sha256':sha(rpath),
        'receipt_ref':str(rpath),'options':q['options'],
        'expanded_pairs':q['ledger']['expanded_pairs'],
        'core_sites':q['ledger']['core_sites'],
        'dead_core_form_sites':q['ledger']['dead_core_form_sites'],
        'dead_core_uniform_tests':q['ledger']['dead_core_uniform_tests'],
        'core_cover_node_visits':q['ledger']['core_cover_node_visits'],
        'core_cover_bound_tests':q['ledger']['core_cover_bound_tests'],
        'atlas_point_tests':q['ledger']['atlas_point_tests'],
        'q3_emitted':q['generator']['q3_emitted'],
        'q4_emitted':q['generator']['q4_emitted'],
        'catalogue_balls':q['catalogue']['balls'],
        'chain_cpu_s':q['chain_cpu_s'],'chain_total_ms':q['times_ms']['chain_total'],
        'external_wall_s':r['external_wall_s']}

def slope(a,b,m):
    x,y=a[m],b[m]
    assert a['n']<b['n']
    if x<=0 or y<=0:return None
    return round(math.log(y/x)/math.log(b['n']/a['n']),6)

def validate_partitions(man,cross,src):
    evidence={}
    for scene in ('00','01'):
        evidence[scene]={}
        for density in DENSITIES:
            pieces={s:set(words(paths(scene,density,s)[2])) for s in SECTORS}
            assert len(pieces['full'])==input_manifest(scene,density,'full',man,cross,src)['sites']
            assert pieces['half_x_neg'].isdisjoint(pieces['half_x_nonneg'])
            assert pieces['half_x_neg']|pieces['half_x_nonneg']==pieces['full']
            quarters=[pieces[s] for s in SECTORS[3:]]
            assert sum(len(x) for x in quarters)==len(set.union(*quarters))
            assert set.union(*quarters)==pieces['full']
            assert quarters[0]|quarters[1]==pieces['half_x_neg']
            assert quarters[2]|quarters[3]==pieces['half_x_nonneg']
            evidence[scene][density]={'sites':len(pieces['full']),
                                      'halves':[len(pieces[s]) for s in SECTORS[1:3]],
                                      'quarters':[len(pieces[s]) for s in SECTORS[3:]]}
        for sector in SECTORS:
            ids={d:set(words(paths(scene,d,sector)[2])) for d in DENSITIES}
            assert ids['quarter']<ids['half']<ids['full']
    return evidence

def main():
    man=json.loads((ROOT/'MANIFEST.json').read_text())
    cross=json.loads((CROSS/'MANIFEST.json').read_text())
    assert man['crossscene_manifest_sha256']==sha(CROSS/'MANIFEST.json')
    assert man['selection']['seed_hex']==cross['selection']['seed_hex']=='d1da73a520260923'
    assert sha(BIN)=='e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80'
    assert (V12/'probe_binary.sha256').read_text().startswith(sha(BIN)+' ')
    src={}
    for scene in ('00','01'):
        source=BASE/f'scene_{scene}_grid'
        assert man['source_scenes'][scene]['source_manifest_sha256']==sha(source/'MANIFEST.json')
        src[scene]=json.loads((source/'MANIFEST.json').read_text())
    partition=validate_partitions(man,cross,src)
    cases={};density_slopes={};spatial_slopes={};aggregates={}
    density_exceptions=[];spatial_exceptions=[]
    for scene in ('00','01'):
        cases[scene]={};density_slopes[scene]={};spatial_slopes[scene]={};aggregates[scene]={}
        for k in (5,10):
            ks=str(k);cases[scene][ks]={};density_slopes[scene][ks]={};spatial_slopes[scene][ks]={};aggregates[scene][ks]={}
            for sector in SECTORS:
                ds={den:case(scene,den,sector,k,man,cross,src) for den in DENSITIES}
                cases[scene][ks][sector]=ds
                density_slopes[scene][ks][sector]={}
                for low,high in (('quarter','half'),('half','full'),('quarter','full')):
                    vals={m:slope(ds[low],ds[high],m) for m in METRICS}
                    vals['n_ratio']=round(ds[high]['n']/ds[low]['n'],6)
                    vals['finite_p_lt_2']={m:(vals[m]<2 if vals[m] is not None else None) for m in METRICS}
                    density_slopes[scene][ks][sector][low+'_to_'+high]=vals
                    if (low,high) in (('quarter','half'),('half','full')):
                        for m in METRICS:
                            if vals[m] is not None and vals[m]>=2:
                                density_exceptions.append({'scene':scene,'K':k,'sector':sector,
                                                           'interval':low+'_to_'+high,'metric':m,'p':vals[m]})
            for den in DENSITIES:
                spatial_slopes[scene][ks][den]={};aggregates[scene][ks][den]={}
                for parent,children in CHILDREN.items():
                    p=cases[scene][ks][parent][den]
                    c=[cases[scene][ks][x][den] for x in children]
                    assert sum(x['n'] for x in c)==p['n']
                    spatial_slopes[scene][ks][den][parent]={}
                    for child in children:
                        x=cases[scene][ks][child][den]
                        vals={m:slope(x,p,m) for m in METRICS}
                        vals['n_ratio']=round(p['n']/x['n'],6)
                        spatial_slopes[scene][ks][den][parent][child]=vals
                        for m in METRICS:
                            if vals[m] is not None and vals[m]>=2:
                                spatial_exceptions.append({'scene':scene,'K':k,'density':den,
                                                           'parent':parent,'child':child,'metric':m,'p':vals[m]})
                    aggregates[scene][ks][den][parent]={m:round(sum(x[m] for x in c)/p[m],6)
                                                        if p[m]>0 else None for m in METRICS}
                p=cases[scene][ks]['full'][den]
                cs=[cases[scene][ks][x][den] for x in SECTORS[3:]]
                assert sum(x['n'] for x in cs)==p['n']
                aggregates[scene][ks][den]['four_quarters_to_full']={m:round(sum(x[m] for x in cs)/p[m],6)
                                                                      if p[m]>0 else None for m in METRICS}
    levers=cases['00']['5']['full']['full']['options']['levers']
    for scene in ('00','01'):
        for k in ('5','10'):
            for sector in SECTORS:
                for den in DENSITIES:
                    opt=cases[scene][k][sector][den]['options']
                    assert opt['levers']==levers and opt['run_tower'] is True
    control_path=ROOT/'CONTROL.json'
    control=json.loads(control_path.read_text())
    assert control['input_sha256']==cases['01']['10']['quarter_x_nonneg_y_nonneg']['quarter']['input_sha256']
    assert control['original_tower_digest']==control['rerun_tower_digest']
    assert all(control['checks'].values())
    out={
        'schema':'ephemeral_mhgp9_lidar_sensor_density_matrix_00_01_v1',
        'scope':'SemanticKITTI sequence 08 scenes 000000 and 000100, full-frame ground mask, quantized 1mm sensor partitions, W8 s8 K5/K10',
        'status':'complete_48_new_cases_plus_36_reused_baselines',
        'limits':'finite local slopes on one sequence; quantized sign sectors not exactly raw float32 sign sectors; no asymptotic/FULL/G4 qualification; CPU timings shared-host indicative',
        'selection':man['selection'],'partition_evidence':partition,
        'generator_script_sha256':sha(ROOT/'generate_sectors.py'),
        'runner_script_sha256':sha(ROOT/'run_sectors.py'),
        'source_manifest_sha256':{s:man['source_scenes'][s]['source_manifest_sha256'] for s in ('00','01')},
        'input_manifest_sha256':sha(ROOT/'MANIFEST.json'),
        'crossscene_manifest_sha256':sha(CROSS/'MANIFEST.json'),
        'binary_sha256':sha(BIN),'source_commit':'4530644b',
        'command':'nice -n 19 mhgp9_tower_probe INPUT.u32le K 8 --s=8 --static=8 --grid=1mm',
        'effective_levers':levers,
        'baseline_note':'Full-density sector baselines are v12 receipts using same binary and effective static8 but grid label unspecified; full-sector density samples reuse separate crossscene runs',
        'slope_formula':'p=ln(work_high/work_low)/ln(n_high/n_low) using actual sector site counts',
        'cases':cases,'density_slopes':density_slopes,'spatial_slopes':spatial_slopes,
        'aggregates':aggregates,
        'density_exceptions_adjacent_ge_2':density_exceptions,
        'spatial_exceptions_adjacent_ge_2':spatial_exceptions,
        'validation':{'new_sector_cases':48,'reused_full_sector_density_cases':8,
                      'reused_v12_full_density_cases':28,'all_complete_relative':True,
                      'all_sha256_and_fnv64_recomputed':True,
                      'all_effective_options_orders_catalogue_and_digest_checked':True,
                      'digest_rerun_control_sha256':sha(control_path),
                      'digest_rerun_control_case':control['source_case'],
                      'digest_rerun_control_all_checks_equal':True,
                      'all_sectors_partition_full_and_density_sets_nested':True},
    }
    path=ROOT/'SUMMARY.json';path.write_text(json.dumps(out,indent=2,sort_keys=True)+'\n')
    print(path,sha(path))
    print('adjacent density exceptions',len(density_exceptions),'spatial exceptions',len(spatial_exceptions))
    for ex in density_exceptions:print(ex)

if __name__=='__main__':main()
