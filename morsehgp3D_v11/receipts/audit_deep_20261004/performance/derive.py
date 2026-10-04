#!/usr/bin/env python3
"""Read-only performance derivation from frozen JSON/stdout; never loads product."""
from pathlib import Path
import hashlib
import json
import statistics

B=Path(__file__).resolve().parent
checks=0

def need(ok,message):
 global checks
 checks+=1
 if not ok: raise ValueError(message)

def load(path): return json.loads((B/path).read_text())
def median(values): return statistics.median(values)
def bounds(values): return {'median':median(values),'min':min(values),'max':max(values)}

paired=load('selected/c40_paired/results/cmd/002_paired_full/files/full_paired.json')
ab=load('selected/b872_ab7/results/cmd/000_ab/files/ab_report.json')
profiles=load('selected/profiles1/results/cmd/001_profiles/files/profiles.json')
reuse=load('source/morsehgp3D_v11/receipts/full_regular_vertical_20261003/reuse1/metrics.json')
oldmeta=load('selected/v10s4/receipt_scope.json')['selected_fields']
abmeta=load('selected/b872/receipt_scope.json')['selected_fields']
manifest={x['name']:x for x in paired['manifest']['cases']}
oldfiles={x['sha256']:x for x in oldmeta['data_files']}
abfiles={x['name']:x for x in abmeta['data_files']}
need(paired['complete'] is True and paired['conforming'] is True,'paired complete')
need(paired['requested_runs']==len(paired['runs'])==81,'paired calendar')
need(ab['verdict']=='conforme' and not ab['refusals'],'ab verdict')
need(len(ab['timings'])==36,'ab calendar')
need(len({x['name'] for x in ab['timings']})==36,'ab unique run names')
need(set(ab['mutants']['tower']['ids'])==set(ab['mutants']['tower']['killed']) and len(ab['mutants']['tower']['ids'])==11,'ab causal mutant list')
old={}
for cmd,case in [('008_tower_lidar00_k5_w48','lidar_ng00'),('010_tower_lidar01_k5_w48','lidar_ng01'),('012_tower_lidar02_k5_w48','lidar_ng02')]:
 r=load('source/morsehgp3D_v10/receipts/g4_session4_j2c_20260929/results/cmd/'+cmd+'/stdout')
 need(r['status']=='ok' and r['points'] is False and r['K']==5 and r['threads']==48,'old full whole K5 without points')
 old[case]=r

rows=[];equality=[];profile_rows=[];source_of_ratios='descriptive_cross_capture_only_not_paired_v10_v11'
for case in ['lidar_ng00','lidar_ng01','lidar_ng02']:
 m=manifest[case];v10=old[case]
 need(m['sha256'] in oldfiles,'old/new XYZ digest match')
 need(oldfiles[m['sha256']]['size']==12*m['count'],'same XYZ count and format')
 need(abfiles[m['coordinates']]['sha256']==m['sha256'] and abfiles[m['point_ids']]['sha256']==m['ids_sha256'],'ab inputs identical to c40')
 need(m['count']==v10['n'] and m['duplicate_sites']==0 and m['unit_site_weights'] is True,'same sites/weights')
 v10_ms=(v10['catalogue_s']+v10['tower_s'])*1000
 vruns=[x for x in paired['runs'] if x['case']==case and x['workers']==48]
 allr=[x for x in paired['runs'] if x['case']==case]
 need(len(vruns)==9 and len(allr)==27,'paired case calendar')
 need(len({x['semantic']['raw_sha256'] for x in allr})==1,'paired output identity')
 for r in allr:
  need(r['status']=='ok' and r['exit_code']==0 and not r['errors'] and r['whole_input'] is True,'paired run ok/whole')
  need(r['build_pin']==r['build_pin_after'],'paired binary stable')
 for a,z in zip(v10['orders'],vruns[0]['semantic']['orders']):
  need(a['k']==z['order'] and a['nodes']==z['nodes'] and a['births']==z['births'] and a['merges']==z['merges'],'same order node/birth/merge counts')
  equality.append({'case':case,'order':a['k'],'nodes':a['nodes'],'births':a['births'],'merges':a['merges'],'scope':'cardinalities_equal_not_full_dump_canonical_equality'})
 for r in vruns:
  need(r['events'][1]['catalogue_balls']==v10['balls'],'same ball count')
 variants={}
 for variant in ['baseline','current2047','current16379']:
  rr=[x for x in vruns if x['build_variant']==variant];need(len(rr)==3,'three repetitions')
  variants[variant]={'full_ms':bounds([x['full_ms'] for x in rr]),'stage_ms':{key:bounds([x['stage_ms'][key] for x in rr]) for key in ['index','domain','forest']},'cpu_seconds':bounds([x['events'][2]['cpu_seconds'] for x in rr]),'reserved_peak_bytes':rr[0]['whole_peak_reserved_bytes'],'process_ms':bounds([1000*x['process_wall_seconds'] for x in rr]),'semantic_ms':bounds([1000*x['semantic_wall_seconds'] for x in rr]),'per_run_global_phase_residue_ms':bounds([x['full_ms']-sum(x['stage_ms'].values()) for x in rr])}
  works=[x['events'][2]['orders'] for x in rr]
  totals=[]
  for orders in works:
   total={k:sum(o['work'].get(k,0) for o in orders) for k in ['part_meb_presentations','descent_steps','population_hits','traces','census_calls','census_point_tests','vertical_descents','vertical_reuses']}
   totals.append(total)
  need(all(x==totals[0] for x in totals),'work stable between three takes')
  variants[variant]['forest_work']=totals[0]
  variants[variant]['catalogue_work']=rr[0]['events'][1]['catalogue_work']
  variants[variant]['catalogue_geometry_passes']=rr[0]['events'][1]['execution']['geometry_passes']
 absums={}
 for variant in ['base','new']:
  rr=[x for x in ab['timings'] if x['frame']==case and x['variant']==variant and x['workers']=='48'];need(len(rr)==5,'ab five takes')
  need(all(x['code']==0 and x['summary']['status']=='ok' and x['dump_sha256']==ab['identity'][case] for x in rr),'ab exact recorded identity')
  need(ab['identity'][case]==allr[0]['semantic']['raw_sha256'],'same v11 output bytes across c40/ab')
  absums[variant]={'full_ms':bounds([x['summary']['wall_ns']/1e6 for x in rr]),'stage_ms':{k:bounds([x['summary'][k+'_ns']/1e6 for x in rr]) for k in ['index','domain','forest']},'cpu_seconds':bounds([x['summary']['cpu_seconds'] for x in rr]),'reserved_peak_bytes':rr[0]['summary']['peak_reserved_bytes'],'regular_ns_definition':'new pipeline time to last resolution; publication/vertical tails thereafter, do not compare raw stage meanings to c40'}
 reuse_case=[x for x in reuse['attempts'] if x['case']==case and x['mode']==2047 and x['coord_bits'] in [21,24] and x['workers']==48]
 rows.append({'case':case,'points':m['count'],'xyz_sha256':m['sha256'],'ids_sha256_v11':m['ids_sha256'],'physical_grid_m':{'numerator':1,'denominator':1000},'v10_777':{'profile':18,'full_last_warm_ms':v10_ms,'first_warm_pass_full_ms':1000*(v10['passes_catalogue_s'][0]+v10['passes_tower_s'][0]),'prepare_cold_ms':v10['prepare_s']*1000,'catalogue_ms':v10['catalogue_s']*1000,'forest_ms':v10['tower_s']*1000,'balls':v10['balls'],'rss_kib':v10.get('rss_kib'),'reported_meb_calls_join':v10['stages']['join']['meb'],'reported_seed_hits_join':v10['stages']['join']['seed_hits'],'reported_steps_join':v10['stages']['join']['steps'],'counter_scope':'MEB calls cannot be compared to v11 support presentations'},'reuse1':[{'coord_bits':x['coord_bits'],'full_ms':x['full_ms'],'stage_ms':x['stage_ms']} for x in reuse_case],'c40':variants,'b872_engine_ab7':absums,'cross_capture_ratios':{'scope':source_of_ratios,'reuse1_u21_over_v10':next(x['full_ms'] for x in reuse_case if x['coord_bits']==21)/v10_ms,'c40_16379_over_v10':variants['current16379']['full_ms']['median']/v10_ms,'b872_over_v10':absums['new']['full_ms']['median']/v10_ms},'paired_v11_ratios':{'c40_16379_over_baseline':variants['current16379']['full_ms']['median']/variants['baseline']['full_ms']['median'],'b872_new_over_base':absums['new']['full_ms']['median']/absums['base']['full_ms']['median']}})
 pr={x['coord_bits']:x for x in profiles['runs'] if x.get('case')==case and x.get('kmax')==5 and x.get('status')=='ok'}
 need(set(pr)=={18,21,24},'catalogue profile successful triple')
 for r in pr.values(): need(r['events'][1]['generation_passes']==2,'historical profile two passes')
 profile_rows.append({'case':case,'source':'9df774947_historical_catalogue','scope':'mono_catalogue_two_geometry_passes_one_take_per_profile_same_XYZ_not_current_FULL','catalogue_ms':{str(k):r['catalogue_ms'] for k,r in pr.items()},'u21_over_u18':pr[21]['catalogue_ms']/pr[18]['catalogue_ms'],'u24_over_u21':pr[24]['catalogue_ms']/pr[21]['catalogue_ms'],'raw_binary_hashes_different_by_profile':len({r['canonical_sha256'] for r in pr.values()})==3,'canonical_geometric_comparison':'equal per profile collector; profile header/limb storage differ'})
result={'schema':'ehgp.audit.deep_performance.v1','pin':'e02a6c235bc4a706519cdaa15f4b1465a6275eba','checks':checks,'scope':'metadata derivation only no native/cloud/fit','rows':rows,'same_order_cardinalities':equality,'profile_diagnostic':profile_rows,'limitations':['No canonical v10/v11 whole LiDAR dump differential closed. XYZ exact identity and same output cardinalities proven; v10 implicit IDs differ from original raw IDs v11.','v10 third warm pass vs v11 fresh processes, separate generations. Cross-capture ratios descriptive, not causal A/B.','Global and subphase medians are independent; never sum phase medians to assert total.','MemoryBudget peaks do not equal RSS, and process/semantic timing do not equal native FULL.','Current c40/b872 LiDAR performance u21 only. Historical u18/u24 results do not qualify current FULL performance.']}
print(json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2))
