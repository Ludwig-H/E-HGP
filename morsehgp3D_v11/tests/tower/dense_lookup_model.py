"""Dense birth maps judged against Definition/Fraction, not the native lookup or its sorting code.

This checks the representation strategy and analytic fixtures. It does not run,
qualify or time C++; native differential and fault gates remain separate.
"""
import copy
import sys

import forest_oracle as oracle

NONE = 2**32-1
CASES = (
    ('singleton',((1,2,3),),1),
    ('points',((8,0,0),(0,8,0),(0,0,8),(0,0,0),(0,8,8)),4),
    ('line6',tuple((x,0,0) for x in range(0,11,2)),4),
    ('diamond',((0,1,0),(1,0,0),(2,1,0),(1,2,0)),4),
    ('tetra',((0,0,0),(2,2,0),(2,0,2),(0,2,2)),4),
    ('right',((0,4,0),(4,0,0),(4,4,0)),2),
    ('distinct',((0,0,0),(2,0,0),(5,0,0),(9,0,0)),2),
)


def definition(points,kmax):
    records = oracle.data.fixtures.records(points)
    sites,_,orders = oracle.truth(records,kmax)
    _,_,balls,_ = oracle.data.geometry(records,kmax)
    maps = []
    for order in orders:
        births = [node for node in order.nodes if not node.children]
        identities = {(tuple(s),oracle.data.F(0)):i for i,s in enumerate(sites)} if order.k == 1 else {
            (ball.center,ball.level):i for i,ball in enumerate(balls)}
        keys = [identities[node.center,node.level] for node in births]
        oracle.require(len(keys)==len(set(keys)),'unique key per birth')
        maps.append(dict(order=order.k,keys=keys,extent=len(sites) if order.k==1 else len(balls)))
    return sites,balls,maps


def candidate(fact):
    relation = dict(zip(fact['keys'],range(len(fact['keys']))))
    return dict(order=fact['order'],values=[relation.get(i,NONE) for i in range(fact['extent'])],
                reserved_bytes=4*fact['extent'])


def judge(row,fact):
    oracle.require(type(row) is dict and row.keys()=={'order','values','reserved_bytes'},'map fields')
    oracle.require(type(row['order']) is int and row['order']==fact['order'],'map order')
    oracle.require(type(row['reserved_bytes']) is int and row['reserved_bytes']==4*fact['extent'],'map bytes')
    values = row['values']
    oracle.require(type(values) is list and len(values)==fact['extent'],'complete key domain')
    reverse = {node:key for node,key in enumerate(fact['keys'])}
    count = 0
    for key,value in enumerate(values):
        oracle.require(type(value) is int and 0 <= value <= NONE,'NodeIdx or sentinel')
        if key in fact['keys']:
            oracle.require(value in reverse and reverse[value]==key,'canonical birth identity')
            count += 1
        else:
            oracle.require(value==NONE,'non-birth remains absent, including poison initialization')
    oracle.require(count==len(reverse),'all births present')
    return 5+len(values)


def main():
    cases = orders = checks = corruptions = absent = nonidentity = 0
    for bits in (18,21,24):
        for name,points,kmax in CASES:
            for factor,reverse in ((1,False),(1 << (bits-4),True)):
                xyz = tuple(tuple(v*factor for v in p) for p in points)
                if reverse: xyz = xyz[::-1]
                sites,balls,maps = definition(xyz,kmax); cases += 1
                if name=='right': oracle.require(maps[1]['keys']==[1,0] and len(balls)==3,'right triangle holes')
                if name=='points': oracle.require(maps[0]['keys']==[0,3,2,4,1],'XYZ differs from Morton')
                if name=='distinct': oracle.require(maps[1]['keys']==[0,1,2] and len(balls)==5,'three distinct birth levels')
                for fact in maps:
                    good = candidate(fact); checks += judge(good,fact); orders += 1
                    absent += good['values'].count(NONE)
                    nonidentity += sum(key!=i for i,key in enumerate(fact['keys']))
                    for field in ('last','missing','bytes','order','bool','identity','poison'):
                        bad = copy.deepcopy(good)
                        if field=='last': bad['values'][-1] = (bad['values'][-1]+1) % (NONE+1)
                        elif field=='missing': bad['values'].pop()
                        elif field=='bytes': bad['reserved_bytes'] += 1
                        elif field=='order': bad['order'] += 1
                        elif field=='bool': bad['values'][0] = True
                        elif field=='identity':
                            if fact['keys']==list(range(len(fact['keys']))): continue
                            for key in fact['keys']: bad['values'][key] = key
                        else:
                            if NONE not in good['values']: continue
                            bad['values'][good['values'].index(NONE)] = 0
                        try: judge(bad,fact)
                        except ValueError: corruptions += 1
                        else: raise ValueError('dense corruption accepted: '+field)
    # All K, not merely the last forest: collinear intervals have exactly n-k+1 births.
    sites,balls,maps = definition(tuple((2*i,0,0) for i in range(12)),12)
    oracle.require(len(sites)==12 and len(balls)==66 and [len(m['keys']) for m in maps]==list(range(12,0,-1)),
                   'line12 ALL K analytic births and catalogue')
    for fact in maps: checks += judge(candidate(fact),fact); orders += 1
    dense = sum(4*m['extent'] for m in maps)
    sparse = sum(8*len(m['keys']) for m in maps)
    oracle.require(dense==4*(12+11*66)==2952 and sparse==8*78==624,'all-order coexistence')
    # Largest legal dense universe: arithmetic stays in u64; this allocates no giant array.
    oracle.require(4*((NONE-1)+11*(NONE-1)) < 2**64,'whole tower byte arithmetic')
    oracle.require(cases==42 and orders>=120 and corruptions>=600 and absent>0 and nonidentity>0,'non-vacant model')
    oracle.require(not any(name.endswith('.constructive') for name in sys.modules),'constructive reference not imported')
    print('dense_lookup_model_verdict conforme '+ ' '.join('%s%d'%item for item in
          dict(cases=cases,orders=orders,checks=checks,corruptions=corruptions,absent=absent,
               nonidentity=nonidentity,all_k_bytes=dense).items())+' native0')


if __name__=='__main__': main()
