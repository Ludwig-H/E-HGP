#!/usr/bin/env python3
import json,re
from pathlib import Path
p=json.loads((Path(__file__).resolve().parent/'proof.json').read_text())
s=p['gate_statement']
match=re.fullmatch(r'mhgp11_add_unit\(mhgp11_catalogue_leaf_coop\s+SOURCES\s+leaf_coop_test\.cpp\s+GROUPS\s+(.*?)\s+LABELS\s+fast\)',s,re.S)
if not match:raise ValueError('registration')
g=match.group(1).split()
if len(g)!=len(set(g)):raise ValueError('duplicate group')
names=['mhgp11_catalogue_leaf_coop_'+x for x in g]+['mhgp11_catalogue_leaf_coop_inventaire','mhgp11_tower_full_leaf_lanes']
if names!=p['registered_names']:raise ValueError('inventory')
if '_mhgp11_register(${target}_${group}' not in p['source_anchors']['cmake_registration']:raise ValueError('helper')
if "'-R', '^%s$' % re.escape(name)" not in p['source_anchors']['exact_runner']:raise ValueError('exact runner')
m={m['id']:m for m in p['proposed_selected_manifest']}
if m['coop_publication_partielle']['options']!=['-DMHGP11_COORD_BITS=21']:raise ValueError('mutant profile')
if p['near_max']['u18']['expected_unresolved']!=0 or not p['near_max']['u18']['partial_positive_impossible']:raise ValueError('u18')
f=p['tight_box_fixture']
if f['dom_row0']!=2 or f['dom_row3']!=4 or f['lo']!=[2,2,2] or f['hi']!=[5,5,5]:raise ValueError('tight fixture')
if p['module_selection']['current']!=['tower'] or p['module_selection']['proposed']!=['tower','catalogue']:raise ValueError('units')
if 'foreach(unit IN LISTS MHGP11_UNITS)' not in p['source_anchors']['test_unit_selection'] or 'include(tests/${unit}/tests.cmake)' not in p['source_anchors']['test_unit_selection']:raise ValueError('unit test include')
rows=[]
for a,b in zip(p['old_mutants'],p['proposed_mutants']):
 if a['id']!=b['id']:raise ValueError('mutant order')
 old=[n for n in names if re.fullmatch(re.escape(a['gate']),n)]
 new=[n for n in names if re.fullmatch(re.escape(b['gate']),n)]
 if old or new!=[b['gate']]:raise ValueError(a['id'])
 rows.append({'id':a['id'],'old_matches':0,'suggested_gate':b['gate'],'new_matches':1})
print(json.dumps({'verdict':'conforme','source_base':p['source_base'],'resolution':rows,'current_units':p['module_selection']['current'],'proposed_units':p['module_selection']['proposed'],'native_runs':0},sort_keys=True))
