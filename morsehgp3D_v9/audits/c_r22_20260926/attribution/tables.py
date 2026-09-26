import sys
sys.path.insert(0, '.')
from extract import load, phases, work
CASES = {
 ('00',5): {'r21':[0,13],'r22':[0,13]}, ('00',10): {'r21':[2,15],'r22':[2,15]},
 ('01',5): {'r21':[4],'r22':[4]}, ('01',10): {'r21':[6],'r22':[6]},
 ('02',5): {'r21':[8],'r22':[8]}, ('02',10): {'r21':[10],'r22':[10]},
 ('b00',5): {'r21':[22],'r22':[24]}, ('b00',10): {'r21':[24],'r22':[26]},
 ('b01',5): {'r21':[26],'r22':[28]}, ('b01',10): {'r21':[28],'r22':[30]},
 ('b02',5): {'r21':[30],'r22':[32]}, ('b02',10): {'r21':[32],'r22':[34]},
}
ROWS = ['chain_total','prepare','gen_index','q2_own','q2_wait','q2_census(bg)','q2_census_index(bg)','q34','front','front_max_job','front_job_sum/48',
 'filter','filter_dev','filter_kern','filter_xfer','filter_host','cert','cert_dev','cert_kern','cert_xfer','cert_host',
 'lanes','lanes_dev','lanes_kern','lanes_xfer','lanes_host','lanes_convert','lanes_plan','lanes_task','lanes_upload','lanes_download','lanes_download_copy',
 'tail','glue_untimed','dev_total','kern_total','xfer_total','host_in_calls','merge','tower_index','census',
 'tower','tower.validate','tower.static','static(K)','static_resolve(K)','A(K)','tower.window','window_model','tower.lots','tower.tail','tower.populations','tower.images','tower.bank','tower.encode','tower.pool','tower.residual','chain_residual']
def avg(r, ids):
    Ps=[phases(load(r,i)) for i in ids]
    out={}
    for k in Ps[0]:
        if k.startswith('_'): out[k]=Ps[0][k]; continue
        out[k]=sum(P[k] for P in Ps)/len(Ps)
    return out
def table(K, frames):
    hdr='| phase | ' + ' | '.join(f'{f} R21 | {f} R22 | Δ' for f in frames) + ' |'
    print(hdr)
    print('|'+'---|'*(1+3*len(frames)))
    data={f:(avg('r21',CASES[(f,K)]['r21']),avg('r22',CASES[(f,K)]['r22'])) for f in frames}
    for row in ROWS:
        cells=[]
        for f in frames:
            a,b=data[f]
            cells += [f'{a.get(row,0):.1f}', f'{b.get(row,0):.1f}', f'{b.get(row,0)-a.get(row,0):+.1f}']
        print(f'| {row} | '+' | '.join(cells)+' |')
    for f in frames:
        a,b=data[f]
        print(f, 'ends R21', [round(x,1) for x in a['_ends']], 'bind', a['_bind'], '| R22', [round(x,1) for x in b['_ends']], 'bind', b['_bind'])
for K in (5,10):
    print(f'\n## K{K} no ground'); table(K,['00','01','02'])
    print(f'\n## K{K} raw'); table(K,['b00','b01','b02'])
