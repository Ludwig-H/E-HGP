"""Exact physical scale checks; no native engine, product import or LiDAR access."""
from fractions import Fraction as F
from pathlib import Path
import json

count=0

def require(value,note):
    global count
    count+=1
    if not value: raise RuntimeError(note)

def rounded(x,h):
    return (x/h+F(1,2)).numerator//(x/h+F(1,2)).denominator

# Metres; the same old millimetre coordinates may be encoded at all three widths.
h=F(1,1000)
rows=[]
for bits,factor in ((18,1),(21,8),(24,64)):
    fine_h=h/factor
    q=[(0,0,0),(2,0,0),(4,0,0),(0,(1<<18)-1,0)]
    lifted=[tuple(factor*x for x in point) for point in q]
    require(all(0<=x<(1<<bits) for point in lifted for x in point),'scaled u18 fits wider profile')
    require(all(tuple(h*x for x in point)==tuple(fine_h*x for x in new)
                for point,new in zip(q,lifted)),'upscaling alone preserves physical points')
    beta=F(25,4)
    require(h*h*beta==fine_h*fine_h*(factor*factor*beta),'physical levels scale by h squared')
    rows.append({'bits':bits,'max_span_at_1mm_metres':str(h*((1<<bits)-1)),
                 'nested_step_metres':str(fine_h),'nested_max_span_metres':str(fine_h*((1<<bits)-1)),
                 'scale_only_is_new_geometric_precision':False})

# Actual refinement starts from original coordinates, not from old rounded integers.
original=[F(0),F(3,8)*h,h]
ids=[10,20,30]
coarse=[rounded(x,h) for x in original]
fine=[rounded(x,h/8) for x in original]
require(coarse==[0,0,1] and fine==[0,3,8],'true finer grid separates two original returns')
require([8*x for x in coarse]!=fine,'rescaling the old grid cannot recover lost detail')

def groups(coordinates):
    result={}
    for q,identity in zip(coordinates,ids): result.setdefault(q,[]).append(identity)
    return [{'coordinate':q,'weight':len(v),'ids':v} for q,v in sorted(result.items())]
require(sum(g['weight'] for g in groups(coarse))==len(original),'retain all returns after coarse collision')
require(any(g['weight']>1 for g in groups(coarse)),'coarse first catalogue must refuse weights')
require(all(g['weight']==1 for g in groups(fine)),'fine toy becomes unit weights')

# Two approximate clouds share IDs; Euclidean epsilon bound follows coordinatewise.
for denominator in (1,8,64):
    step=h/denominator
    for numerator in (-17,-8,-1,0,1,4,7,8,17):
        x=F(numerator,16)*h
        error=abs(step*rounded(x,step)-x)
        require(error<=step/2,'nearest rounding coordinate error')
        coarse_error=abs(h*rounded(x,h)-step*rounded(x,step))
        require(coarse_error<=(h+step)/2,'paired grid coordinate displacement bound')

# The coarse grid hides the asymmetry of the already proven boundary fixture.
x=[F(0),2*h,F(33,8)*h]
require([rounded(v,h) for v in x]==[0,2,4],'coarse boundary fixture is symmetric')
require([rounded(v,h/8) for v in x]==[0,16,33],'fine boundary fixture keeps asymmetry')
manifest=json.loads((Path(__file__).parent/'inputs_snapshot.json').read_text())
lidar=[case for case in manifest['cases'] if case['name'].startswith('lidar')]
require(len(lidar)==3,'three old whole-frame manifests')
for case in lidar:
    p=case['provenance']; step=p['grid_step_metres']
    require(F(step['numerator'],step['denominator'])==h,'current LiDAR benchmark is 1mm')
    require(case['profile']=='quantized_u18_input_only','same old u18 coordinates across compiled profiles')
    require(case['duplicate_sites']==0 and case['unit_site_weights'] is True,'declared unchanged unit-weight input')
    require(p['original_return_ids'] is True and p['whole_frame_after_ground_mask'] is True and not p['subsampling'],
            'declared IDs and entire nonground input')
print(json.dumps({'status':'PASS','checks':count,'profiles':rows,'coarse_groups':groups(coarse),
                  'fine_groups':groups(fine),'scope':'exact scale/model checks and frozen metadata only; no finer LiDAR qualification'},sort_keys=True,indent=2))
