"""Scalar capacity calculation, not an allocation or LiDAR prediction."""
from pathlib import Path
import json
p=Path(__file__).resolve().parent
checks=0
def need(ok,msg):
 global checks
 checks+=1
 if not ok:raise RuntimeError(msg)
nearest=json.loads((p/'nearest.stdout.json').read_text())
layout=json.loads((p/'layout.stdout.json').read_text())
need(nearest['status']=='PASS','native nearest result')
need(nearest['sites']==144 and nearest['requested_k']==1,'bounded sphere fixture')
need(nearest['sphere_candidates']==144 and nearest['cand_capacity']==256 and nearest['out_capacity']==256,'highwater capacities')
need(nearest['after_site_query_cand_size']==2 and nearest['after_site_query_out_size']==1,'smaller second query')
need(nearest['pair_double_u32_bytes']==16 and nearest['pair_i128_u32_bytes']==32,'candidate element footprints')
need(layout['current_Level_bytes']==56 and layout['current_record_field_model_bytes']==104,'current record layout')
need(layout['candidate_4_3_Level_bytes']==72 and layout['candidate_5_4_Level_bytes']==88,'candidate level layouts')
need(layout['candidate_4_3_record_bytes']==120 and layout['candidate_5_4_record_bytes']==136,'candidate record layouts')
need(not (p/'nearest.stderr.txt').read_bytes() and not (p/'layout.stderr.txt').read_bytes(),'native stderr empty')
def units(b):return {'bytes':b,'GB':b/10**9,'GiB':b/2**30}
cases=[]
for n in (10_000_000,30_000_000,50_000_000):
 for ratio in (20,100):
  B=n*ratio;P=8*B;L=B//4;K=10
  cases.append({'n_equals_N':n,'K':K,'scenario_balls_per_site':ratio,'B':B,'P_equals_8B':P,'L_equals_B_over_4':L,
   'B_fits_u32_without_sentinel':B<2**32-1,
   'catalogue_persistent':units(42*B+4*P+56*L+8),
   'generator_assembly_plus_cli_cloud_tree_coordinate_socle':units(179*B+8*P+56*L+160*n+8),
   'candidate_5_4_level_assembly_same_fields_same_counts_plus_socle':units(211*B+8*P+88*L+160*n+8),
   'core_full_K_point_arrays_only':units(12*n*K),
   'cover_full_K_extra0_point_arrays_only':units((16*K-4)*n),
   'cover_full_K_extra0_unused_point_level_arrays_above_K1':units(8*n*(K-1)),
   'cover_full_K_optional_ball_nodes_only_K2_up':units(4*B*(K-1)),
   'one_order_CLI_cover_vote_inversion_if_Pv_equals_P':units(8*(n+1)+4*P),
   'one_order_CLI_vote_inversion_with_fillc_phase_if_Pv_equals_P':units(16*n+8+4*P),
   'scope':'Hypothetical counts, no atlas/forest validity or LiDAR growth/capacity claim; vector capacity overhead omitted'})
out={'status':'PASS','checks':checks,'native_highwater':nearest,'candidate_layouts':layout,
 'units':'GB=10^9 bytes; GiB=2^30 bytes','scenarios':cases,
 'phase_formulas':{
  'PointDendrogram_final_logical':'12*Q+4*E+12*n+8*Ld+4',
  'PointDendrogram_build_with_temporaries':'28*Q+4*E+20*n+4*(Rmax+2)+8*Ld+4, above still-live Catalogue/Tower/Cloud/CLI/Tree; ignores capacities',
  'condense_near_return_known_arrays':'16*Q+16*n+28*Chead+4, above PointDendrogram/forest/catalogue; excludes work/stack/big capacities',
  'generator_repeat_after_first_pass':'cold_generator_assembly_payload + previous_Catalogue_payload + previous_returned_Tower_payload, not multiplied by repeat count',
  'tower_repeat_after_first_pass':'new_Catalogue + new_Tower_build_states + previous_returned_Tower; old Catalogue already released',
  'fixed_band_cover_future':'Unspecified here; do not transfer scalar Python prototype memory to native product'},
 'no_large_allocation':True,'no_native_wide_or_FULL_10M_qualification':True}
print(json.dumps(out,indent=2))
