#!/usr/bin/env python3
"""Validate closed records in normal and optimized Python."""
import gzip
import json
from pathlib import Path
import sys
from evidence import ROOT,sha


def require(condition,message):
    if not condition:raise RuntimeError(message)


def main():
    folder=Path(sys.argv[1])
    manifest=json.loads((folder/'MANIFEST.json').read_text())
    completion=json.loads((folder/'COMPLETION.json').read_text())
    require(manifest['schema']=='audit_local_sweeps_capture_v1','Wrong schema')
    require(manifest['status']==completion['status']=='completed','Incomplete capture')
    require(completion['manifest_sha256']==sha(folder/'MANIFEST.json'),'Manifest changed')
    require(len(manifest['records'])==completion['records']==66,'Missing measurements')
    for p,digest in manifest['sources'].items():require(sha(ROOT/p)==digest,'Source changed: '+p)
    rows=[];identities=set()
    for entry in manifest['records']:
        path=folder/entry['path']
        require(sha(path)==entry['sha256'],'Record changed')
        record=json.loads(gzip.decompress(path.read_bytes()))
        require(record['returncode']==0 and not record['stderr'],'Failed measurement')
        data=json.loads(record['stdout'])
        require(data['schema']=='mhgp8_audit_q4_local_sweeps_v1' and data['status']=='passed','Wrong child schema/status')
        require(data['positive_support_filter_executed'] is False and data['product_code_used'] is False,'Unexpected scope')
        require(data['n']==record['n'] and data['input_sha256']==record['input_sha256'],'Input mismatch')
        require(data['kmax']==10 and data['depth']==7 and data['node_budget']==4096,'Configuration mismatch')
        require(data['domain']==record['domain'] and data['mode']==manifest['mode'],'Option mismatch')
        require(record['command'][2:]==['10','7','4096',str(record['domain']),str(manifest['mode'])],'Command mismatch')
        work,memory=data['work'],data['memory']
        require(work['cell_visits']==memory['tree_nodes']<=data['node_budget'],'Tree budget mismatch')
        require(work['cell_visits']==sum(work[k] for k in ('deep','unknown','internal','out_disk','out_hull')),'Cell partition mismatch')
        require(work['I']==work['query_unknown'],'Fragment incidence mismatch')
        require(work['A']<=work['retained_active_capacity_slots']<=work['peak_live_active_capacity_slots'],'Active capacity mismatch')
        require(data['rejected']+data['survivors']==data['seeds'] and data['forecast_scan_reads']==data['cover_sites']*data['survivors'],'Survivor partition mismatch')
        require(len(set(data['rejected_seed_ids']))==len(data['rejected_seed_ids'])==data['rejected'],'Rejected IDs mismatch')
        require(work['groups']==work['groups_outside_cell']+work['groups_not_owned']+work['owned_groups'],'Group ownership mismatch')
        require(data['shallow_covered_roots']==work['lowdepthgroups']==data['root_digest']['count']<=work['owned_groups'],'Shallow root mismatch')
        if manifest['mode']==1:
            require(work['W']==sum(work[k] for k in ('events','constant_inside','constant_shell_ids','constant_outside')),'Local site census mismatch')
            require(work['event_side_tests']==work['events'],'Event processing mismatch')
        else:
            require(work['events']==work['groups']==work['lowdepthgroups']==0,'Count mode executed sweep')
        identity=(record['case'],record['domain'])
        require(identity not in identities,'Duplicate configuration')
        identities.add(identity)
        rows.append(dict(case=record['case'],provenance=record['provenance'],**data))
    print(json.dumps(dict(status='passed',records=len(rows),mode=manifest['mode'],
                         scope='local covered roots; not qualified positive q4 candidates',rows=rows),sort_keys=True,indent=2))


if __name__=='__main__':main()
