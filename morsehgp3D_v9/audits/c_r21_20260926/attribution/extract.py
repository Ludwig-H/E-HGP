import json,sys
V='/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r21_20260925/vm/'
def load(i): return json.loads(open(V+f'probe_{i}.stdout').read())
def phases(d):
    t=d['times_ms']; b=d.get('q34_batch',{}); tp=d['tower_phases_ms']; td=d.get('tower_detail',{})
    P={}
    P['prepare']=t['prepare']; P['gen_index']=t['gen_index']
    P['q2(own)']=t['q2']; P['q2_wait']=t['q2_wait']
    P['q34']=t['q34']
    if b.get('used'):
        P['q34.front']=b['front_ms']; P['q34.filter']=b['filter_ms']; P['q34.filter_dev']=b['device_ms']
        P['q34.filter_kern']=b['filter_kernel_ms']; P['q34.filter_xfer']=b['filter_transfer_ms']
        P['q34.cert']=b['certificate_ms']; P['q34.cert_dev']=b['certificate_device_ms']; P['q34.cert_kern']=b['certificate_kernel_ms']; P['q34.cert_xfer']=b['certificate_transfer_ms']
        P['q34.lanes']=b['lanes_ms']; P['q34.lanes_dev']=b['lanes_device_ms']; P['q34.lanes_kern']=b['lanes_kernel_ms']; P['q34.lanes_xfer']=b['lanes_transfer_ms']
        P['q34.lanes_plan']=b['lanes_plan_ms']; P['q34.lanes_task']=b['lanes_task_ms']; P['q34.lanes_compact']=b['lanes_compact_ms']
        P['q34.lanes_convert']=b['lanes_convert_ms']; P['q34.lanes_host_other']=b['lanes_ms']-b['lanes_device_ms']-b['lanes_convert_ms']
        P['q34.edges(cpu,under lanes)']=b['edges_ms']; P['q34.lanes_wait']=b['lanes_wait_ms']
        P['q34.tail']=b['tail_ms']
        P['q34.residual']=t['q34']-(b['front_ms']+b['filter_ms']+b['certificate_ms']+b['lanes_ms']+b['tail_ms'])
        P['gpu_prepare(bg)']=b['gpu_prepare_ms']
    P['merge']=t['merge']; P['tower_index']=t['tower_index']; P['census']=t['census']
    P['tower']=t['tower']
    for k in ['validate','static','lots','populations','images','bank','encode']: P['tower.'+k]=tp[k]
    P['tower.pool']=td.get('pool_ms',0)
    P['tower.residual']=t['tower']-sum(tp[k] for k in ['validate','static','lots','populations','images','bank','encode'])-td.get('pool_ms',0)
    crit=t['prepare']+t['gen_index']+t['q34']+t['merge']+t['tower_index']+t['census']+t['tower']
    crit+= t['q2_wait'] if b.get('used') and d['options']['levers'].get('q2_during_device') else t['q2']
    P['chain_residual']=t['chain_total']-crit
    P['chain_total']=t['chain_total']
    P['digest(out)']=t['digest']; P['catalogue_digest(out)']=t['catalogue_digest']
    return P
def work(d):
    g=d['generator']; c=d['catalogue']; b=d.get('q34_batch',{}); tw=d['tower_work']; L=d['ledger']
    W={}
    W['sites']=d['input']['sites']
    W['q2_front_rect']=g['q2_front_rectangles']; W['q2_cand_pairs']=g['q2_candidate_pairs']; W['q2_accepted']=g['q2_accepted_pairs']
    W['q34_rect']=L['q34_input_rectangles']; W['q34_expanded_pairs']=g['q34_expanded_pairs']
    W['survivors']=b.get('survivors',0); W['lanes_asked']=b.get('lanes_asked',0); W['lanes_records']=b.get('lanes_records',0)
    W['lanes_tasks']=b.get('lanes_tasks',0)
    W['q3_emitted']=g['q3_emitted']; W['q4_emitted']=g['q4_emitted']
    W['balls']=c['balls']; W['cat_bytes']=c['bytes']; W['census_nodes']=c['census_nodes']
    W['tower.representatives']=tw['representatives']; W['tower.intruder_nodes']=tw['intruder_nodes']; W['tower.meb_calls']=tw['meb_calls']
    W['tower.births+merges']=tw['births']+tw['merges']
    W['forest_nodes']=sum(o['nodes'] for o in d['orders'])
    W['lanes_census_pt']=L['lanes_census_point_tests']; W['lanes4_pass_site_tests']=L['lanes4_pass_site_tests']
    W['core_sites']=L['core_sites']; W['cover_sites']=L['cover_sites']
    W['rss_GiB']=d['peak_rss_kb']/1048576; W['chain_cpu_s']=d['chain_cpu_s']
    W['q34_cpu_sum_s']=d['q34_occupancy']['cpu_sum_s']
    return W
