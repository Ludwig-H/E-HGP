"""Exact reservation equations against frozen metrics; no native code execution."""
from pathlib import Path
import json

checks=0

def require(condition,note):
    global checks
    checks+=1
    if not condition: raise RuntimeError(note)

r=Path(__file__).parent
report=json.loads((r/'profiles.json').read_text())
receipt=json.loads((r/'receipt.json').read_text())
require(receipt['commit']=='9df77494732b03ddf11dbcf1dcb11d96bef54a3b','qualified source')
require(report['complete'] is True and report['conforming'] is False,'closed failed campaign')
require(len(report['runs'])==33 and len(report['not_run'])==3,'all 36 units accounted')

# ABI reconstruction from the fields of the captured C++ classes on the qualified x86-64 ABI.
# Wide<W> : bool then padding to8 then W u64 ; native i128 : size16 align16.
# No sizeof call or build is made by this script.
sizes={}
for bits in (18,21,24):
    nbits,dbits=8*bits+12,6*bits+8
    def integer_size(budget):
        if budget<=63: return 8,8
        if budget<=127: return 16,16
        return 8+8*((budget+63)//64),8
    ns,na=integer_size(nbits); ds,da=integer_size(dbits)
    align=max(na,da)
    def rounded(value,a): return (value+a-1)//a*a
    level=rounded(rounded(ns,da)+ds,align)
    emission=rounded(rounded(32,align)+level+8,max(align,8))
    sizes[bits]={'level':level,'emission':emission}
    require((level,emission)=={18:(48,96),21:(64,104),24:(72,112)}[bits],'ABI field reconstruction')

rows=[]; cases={}; successes=[x for x in report['runs'] if x['status']=='ok']
require(len(successes)==15,'fifteen completed metrics only')
for row in successes:
    event=row['events'][1]; bits=row['coord_bits']; n=row['count']; N=event['balls']; L=event['levels']; P=event['incidences']
    require(event['phase']=='catalogue' and event['coord_bits']==bits and row['kmax']==5,'metric identity')
    # All successful benchmark inputs have n distinct, unit-weight sites and n original IDs.
    cloud=28*n+8
    E=sizes[bits]['emission']*N+4*P
    F=40*N+sizes[bits]['level']*L+4*P+8
    require(cloud+E+F==event['peak_reserved_bytes'],'observed peak equals assembly coexistence')
    require(cloud+F==event['reserved_after_bytes'],'retained output equals exact result plus Cloud')
    require(event['peak_reserved_bytes']-event['reserved_after_bytes']==E,'transient emissions and old population')
    values={'case':row['case'],'bits':bits,'sites':n,'balls':N,'levels_including_zero':L,'incidences':P,
            'cloud_bytes':cloud,'emissions_bytes':E,'result_bytes':F,'peak_bytes':cloud+E+F,
            'retained_bytes':cloud+F,'old_population_bytes':4*P,'api_seconds':event['wall_ns']/1e9}
    rows.append(values); cases.setdefault(row['case'],{})[bits]=(row,values)
for name,groups in cases.items():
    require(set(groups)=={18,21,24},'all profiles successful for case')
    r18,v18=groups[18]; r21,v21=groups[21]; r24,v24=groups[24]
    require(len({x[0]['semantic']['sha256'] for x in groups.values()})==1,'same declared mathematical output')
    require(r18['events'][1]['logical']==r21['events'][1]['logical']==r24['events'][1]['logical'],'same discrete geometry')
    N,L=v18['balls'],v18['levels_including_zero']
    require(v21['peak_bytes']-v18['peak_bytes']==8*N+16*L,'B21 peak delta comes from Emission and Level layout')
    require(v24['peak_bytes']-v21['peak_bytes']==8*N+8*L,'B24 peak delta comes from Emission and Level layout')
    require(r21['canonical_bytes']-r18['canonical_bytes']==8*L and
            r24['canonical_bytes']-r21['canonical_bytes']==8*L,'file encoding delta is one limb per level')
require(len(cases)==5,'five successful full comparisons')
print(json.dumps({'status':'PASS','checks':checks,'abi_sizes_reconstructed':sizes,'metrics':rows,
                  'scope':'frozen declared Buffer metrics, ABI reconstruction and source equations; not RSS/native sizeof/massive qualification'},sort_keys=True,indent=2))
