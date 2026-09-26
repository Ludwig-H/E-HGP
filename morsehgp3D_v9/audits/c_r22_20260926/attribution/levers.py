import sys
sys.path.insert(0,'.')
from extract import load, phases
from tables import avg
ARMS={5:[('R21 warm','r21',[0,13]),('R22 gpu_r21','r22',[12,16]),('seal off','r22',[18]),('early off','r22',[20]),('pinned off','r22',[22]),('R22 GPU','r22',[0,13])],
      10:[('R21 warm','r21',[2,15]),('R22 gpu_r21','r22',[14,17]),('seal off','r22',[19]),('early off','r22',[21]),('pinned off','r22',[23]),('R22 GPU','r22',[2,15])]}
ROWS=['chain_total','q2_own','q2_census(bg)','q2_census_index(bg)','q34','front','filter','filter_host','cert','cert_host','lanes','lanes_dev','lanes_xfer','lanes_host','lanes_convert','lanes_download','lanes_download_copy','glue_untimed','tail','merge','tower_index','census','tower','tower.validate','tower.static','A(K)','tower.window','tower.tail','tower.populations','tower.images','tower.encode','chain_residual']
for K in (5,10):
    data=[(n,avg(r,ids)) for n,r,ids in ARMS[K]]
    print(f'\n## 00 K{K}')
    print('| phase | '+' | '.join(n for n,_ in data)+' | drift (gpu_r21−R21) | seal | early | pinned | sum of three | all three (GPU−gpu_r21) |')
    print('|'+'---|'*(len(data)+7))
    for row in ROWS:
        v={n:dd.get(row,0) for n,dd in data}
        gpu=v['R22 GPU']
        seal=gpu-v['seal off']; early=gpu-v['early off']; pin=gpu-v['pinned off']
        print(f'| {row} | '+' | '.join(f'{v[n]:.1f}' for n,_ in data)+f' | {v["R22 gpu_r21"]-v["R21 warm"]:+.1f} | {seal:+.1f} | {early:+.1f} | {pin:+.1f} | {seal+early+pin:+.1f} | {gpu-v["R22 gpu_r21"]:+.1f} |')
    for i in [x for _,_,ids in ARMS[K] for x in ids]: pass
# individual repeats
for K,ids21,ids22,idsr in ((5,[0,13],[0,13],[12,16]),(10,[2,15],[2,15],[14,17])):
    print('K',K,'R21 warm chain',[load('r21',i)['times_ms']['chain_total'] for i in ids21],'R22 GPU',[load('r22',i)['times_ms']['chain_total'] for i in ids22],'gpu_r21',[load('r22',i)['times_ms']['chain_total'] for i in idsr])
    for nm,r,ids in (('R21',"r21",ids21),('GPU','r22',ids22),('gpu_r21','r22',idsr)):
        print(' ',nm,'census',[round(load(r,i)['times_ms']['census'],1) for i in ids],'tower',[round(load(r,i)['times_ms']['tower'],1) for i in ids],'q34',[round(load(r,i)['times_ms']['q34'],1) for i in ids],'merge',[round(load(r,i)['times_ms']['merge'],1) for i in ids])
