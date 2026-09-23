import json
R='/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/lidar_scaling_local_20260923/out'
A='/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/d3_echelles'
def probe(sc,k,sz): return json.load(open(f'{R}/{sc}_k{k}_w8_r0/{sc}_k{k}_s8_w8_r0_{sz}.json'))['probe']
def share(f,jthr,delta=0.0):
    d=json.load(open(f'{A}/{f}'))
    ce=[max(0,c-delta*1e3*e) for c,e in zip(d['cpu_ns'],d['edges'])]
    cr=[max(0,c-delta*1e3*e) for c,e in zip(d['rect_cpu_ns'],d['rect_count'])]
    return (sum(ce[jthr+1:])+sum(cr[jthr+1:]))/(sum(ce)+sum(cr)), d
# R7b (MEB ON, moyenne des deux repetitions) : chaine, q34
r7b={('s00',5):(5.63,4.065),('s01',5):(3.74,2.595),('s02',5):(6.505,4.795),
     ('s00',10):(13.92,8.20),('s01',10):(9.58,5.27),('s02',10):(15.29,9.825)}
# calibration s01 : croissance du CPU local (<=0,8 m) 8k->32k contre croissance des emissions
s8,d8=share('attrib_s01_k5_s8_w8_r0_nested_8000_k5.json',4); s32,d32=share('attrib_s01_k5_s8_w8_r0_nested_32000_k5.json',4)
loc8=sum(d8['cpu_ns'][:5]); loc32=sum(d32['cpu_ns'][:5])
em8=sum(d8['q3_emitted'])+sum(d8['q4_emitted']); em32=sum(d32['q3_emitted'])+sum(d32['q4_emitted'])
print('calibration s01 K5 8k->32k : CPU aretes<=0,8 m x%.2f ; emissions q3/q4 x%.2f ; aretes<=0,8 m x%.2f'%(loc32/loc8, em32/em8, sum(d32['edges'][:5])/sum(d8['edges'][:5])))
print()
print('%-8s %-6s %6s %6s %6s | %-22s | %-22s | %s'%('cas','h','G_q34','G_cpu','E_emis','part 8k -> plein (min)','D3 ideal proj -> corrige','gain chaine'))
for sc,k,f in (('s00',5,'attrib_s00_k5_s8_w8_r0_nested_8000_k5.json'),('s02',5,'attrib_s02_k5_s8_w8_r0_nested_8000_k5.json'),
               ('s00',10,'attrib_s00_k5_s8_w8_r0_nested_8000_k10.json'),('s02',10,'attrib_s02_k5_s8_w8_r0_nested_8000_k10.json')):
    p8=probe(sc,k,'nested_8000'); pf=probe(sc,k,'piece_full')
    Gq=pf['times_ms']['q34']/p8['times_ms']['q34']; Gc=pf['chain_cpu_s']/p8['chain_cpu_s']
    E=(pf['generator']['q3_emitted']+pf['generator']['q4_emitted'])/(p8['generator']['q3_emitted']+p8['generator']['q4_emitted'])
    G=min(Gq,Gc)
    chain,q34=r7b[(sc,k)]
    for h,j in ((0.5,4),(1.0,5)):
        s,_=share(f,j)
        sf=1-(1-s)*E/G
        old=chain-q34+s*q34; new=chain-q34+sf*q34
        print('%-8s %-6s %6.2f %6.2f %6.2f | %5.1f %% -> %5.1f %%        | %5.2f s -> %5.2f s       | x%.2f -> x%.2f'%(f'{sc} K{k}',h,Gq,Gc,E,100*s,100*sf,old,new,chain/old,chain/new))
