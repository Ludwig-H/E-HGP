import sys; sys.path.insert(0,'.')
from extract import load, phases
def rows(P):
    R={}
    R['1 tower window static(5)+A(5)']=P['tower.window']
    R['2 q3/q4 host glue (untimed + host in calls)']=P['glue_untimed']+P['host_in_calls']
    R['3 front q3/q4']=P['front']
    R['4 census']=P['census']
    R['5 device time of the 3 calls (kern+xfer)']=P['dev_total']
    R['6 tower tail (pop+img+bank+enc)']=P['tower.tail']
    R['7 validation']=P['tower.validate']
    R['8 merge+gen_index+tower_index+prepare+residuals']=P['merge']+P['gen_index']+P['tower_index']+P['prepare']+P['chain_residual']+P['tail']+P['tower.residual']+P['tower.pool']
    return R
sets={'R21':('r21',[22,26,30]),'R22':('r22',[24,28,32])}
out={}
for nm,(r,ids) in sets.items():
    out[nm]=[(rows(phases(load(r,i))),phases(load(r,i))['chain_total']) for i in ids]
keys=list(out['R21'][0][0].keys())
print('| row | R21 b00 | R21 b01 | R21 b02 | R22 b00 | R22 b01 | R22 b02 | R22 mean | Δ mean | share R22 mean |')
print('|'+'---|'*10)
for k in keys:
    a=[x[0][k] for x in out['R21']]; b=[x[0][k] for x in out['R22']]
    mb=sum(b)/3; ma=sum(a)/3; ch=sum(x[1] for x in out['R22'])/3
    print(f'| {k} | '+' | '.join(f'{v:.1f}' for v in a+b)+f' | {mb:.1f} | {mb-ma:+.1f} | {100*mb/ch:.1f} % |')
for nm in out:
    s=[sum(x[0].values()) for x in out[nm]]; c=[x[1] for x in out[nm]]
    print(nm,'sum rows',[round(v,1) for v in s],'chain',[round(v,1) for v in c])
# extra details for projections
for i in [24,28,32]:
    P=phases(load('r22',i)); d=load('r22',i)
    print(i,'front',round(P['front'],1),'max_job',round(P['front_max_job'],1),'job_sum/48',round(P['front_job_sum/48'],1),'glue',round(P['glue_untimed'],1),'host_in_calls',round(P['host_in_calls'],1),'filter_host',round(P['filter_host'],1),'cert_host',round(P['cert_host'],1),'lanes_host',round(P['lanes_host'],1),'lanes_convert',round(P['lanes_convert'],1),'kern',round(P['kern_total'],1),'xfer',round(P['xfer_total'],1),'q2cen',round(P['q2_census(bg)'],1),'q2own',round(P['q2_own'],1))
