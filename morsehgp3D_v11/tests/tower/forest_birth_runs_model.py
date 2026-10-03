"""Faits Gamma/Fraction indépendants pour le tri des cohortes de naissances.

Definition énumère les k-parties/cofaces ; le catalogue Fraction fournit les
identités des boules. Le modèle ne copie ni le tri par tas ni largest_birth_run.
"""
import copy
from collections import Counter
from fractions import Fraction as F
import forest_oracle as oracle


CASES = (
    dict(name='points',points=((8,0,0),(0,8,0),(0,0,8),(0,0,0),(0,8,8)),k=1,
         keys=(0,3,2,4,1),levels=(F(0),)*5,presentations=0,scratch=0),
    dict(name='right',points=((0,4,0),(4,0,0),(4,4,0)),k=2,
         keys=(1,0),levels=(F(4),F(4)),presentations=2,scratch=2,balls=3,nodes=3,edges=2),
    dict(name='distinct',points=tuple((x,0,0) for x in (0,2,5,9)),k=2,
         keys=(0,1,2),levels=(F(1),F(9,4),F(4)),presentations=0,scratch=0,balls=5,nodes=5,edges=4),
    dict(name='interrupted',points=tuple((x,0,0) for x in (0,4,6,8,12)),k=2,
         keys=(0,1,2,4),levels=(F(1),F(1),F(4),F(4)),presentations=4,scratch=2,balls=7,nodes=6,edges=5),
    dict(name='interrupted_maximum',points=tuple((x,0,0) for x in (0,4,6,8,12,16)),k=2,
         keys=(0,1,2,4,5),levels=(F(1),F(1),F(4),F(4),F(4)),presentations=5,scratch=3,balls=9,nodes=8,edges=7),
    dict(name='extended',points=((0,12,0),(2,10,0),(4,12,0),(2,14,0),
                                 (10,2,0),(12,0,0),(14,2,0),(12,4,0)),k=3,
         keys=(9,8,12),levels=(F(4),F(4),F(34)),presentations=2,scratch=2,balls=19,nodes=4,edges=3))


def derive(case,factor,reverse):
    points = tuple(tuple(x*factor for x in p) for p in case['points'])
    records = oracle.data.fixtures.records(points[::-1] if reverse else points)
    sites,_,orders = oracle.truth(records,case['k'])
    _,_,balls,_ = oracle.data.geometry(records,case['k'])
    order = orders[-1]
    births = [n for n in order.nodes if not n.children]
    if case['k'] == 1:
        keys = tuple(sites.index(n.center) for n in births)
        repeated = []
    else:
        keys = tuple(next(i for i,b in enumerate(balls) if b.center == n.center and b.level == n.level)
                     for n in births)
        # Histogramme global des niveaux, sans parcours des positions du catalogue.
        repeated = [count for count in Counter(n.level for n in births).values() if count > 1]
    result = dict(keys=keys,levels=tuple(n.level for n in births),presentations=sum(repeated),
                  scratch=max(repeated,default=0))
    if case['k'] > 1:
        result.update(balls=len(balls),nodes=len(order.nodes),edges=sum(len(n.children) for n in order.nodes))
    facts = 0
    if case['name'] in ('interrupted','interrupted_maximum'):
        oracle.equal([balls[i].level for i in (2,3,4)],[F(4)*factor*factor]*3)
        oracle.equal([len(balls[i].inner) for i in (2,3,4)],[0,1,0])
        oracle.equal([i in keys for i in (2,3,4)],[True,False,True]); facts += 3
    if case['name'] == 'extended':
        oracle.equal([(balls[i].p,len(balls[i].shell),balls[i].qmin) for i in keys],[(0,4,2)]*3)
        oracle.equal([(n.level,len(n.children)) for n in order.nodes if n.children],[(F(50)*factor*factor,3)])
        oracle.equal([n.center for n in births],
                     [tuple(F(v)*factor for v in p) for p in ((2,12,0),(12,2,0),(7,7,0))])
        # Une vraie naissance étendue à k3 : toutes ses triples de coquille gardent le rayon.
        import itertools
        ref = oracle.reference(sites)
        for i in keys:
            oracle.require(all(ref.beta(part) == balls[i].level
                               for part in itertools.combinations(balls[i].shell,3)), 'naissance étendue')
            facts += 1
        facts += 3
    return result,facts


def expected(case,factor):
    return {key:(tuple(v*factor*factor for v in value) if key == 'levels' else value)
            for key,value in case.items() if key not in ('name','points','k')}


def validate(actual,wanted):
    oracle.require(actual.keys() == wanted.keys(),'champs du fait')
    for key in wanted:
        oracle.equal(actual[key],wanted[key])
    return len(wanted)+1


def split_capacity(actual):
    # Mutation geometrique des cohortes : conserver le resultat canonique, mais couper uniquement
    # la cohorte du niveau4*s² a la boule3 non-naissance. Ce n'est pas un scan largest_birth_run.
    cut_level = actual['levels'][-1]
    pairs = tuple(zip(actual['keys'],actual['levels']))
    other = Counter(level for _,level in pairs if level != cut_level)
    left = sum(key < 3 for key,level in pairs if level == cut_level)
    right = sum(key > 3 for key,level in pairs if level == cut_level)
    bad = copy.deepcopy(actual)
    bad['scratch'] = max(tuple(other.values())+(left,right))
    return bad


def run():
    cases = checks = corruptions = facts = 0
    for bits in (18,21,24):
        for case in CASES:
            for factor in (1,1 << (bits-(4 if case['name']=='points' else 5))):
                for reverse in (False,True):
                    actual,extra = derive(case,factor,reverse)
                    wanted = expected(case,factor)
                    checks += validate(actual,wanted); facts += extra; cases += 1
                    for field in ('keys','levels','presentations','scratch'):
                        bad = copy.deepcopy(actual)
                        if field == 'keys': bad[field] = tuple(reversed(bad[field]))
                        elif field == 'levels': bad[field] = (bad[field][0]+1,)+bad[field][1:]
                        else: bad[field] += 1
                        try:
                            validate(bad,wanted)
                        except ValueError:
                            corruptions += 1
                        else:
                            raise ValueError('corruption acceptée : '+field)
                    if case['name'] == 'interrupted_maximum':
                        bad = split_capacity(actual)
                        oracle.require(bad['scratch'] == 2 and actual['scratch'] == 3,'vraie sous-estimation'); facts += 1
                        for key in wanted.keys() - {'scratch'}:
                            oracle.equal(bad[key],wanted[key]); checks += 1
                        try:
                            validate(bad,wanted)
                        except ValueError:
                            corruptions += 1
                        else:
                            raise ValueError('coupure a la non-naissance acceptee')
    oracle.require(cases == 72 and corruptions == 300,'inventaire non vacant')
    print(f'birth_runs_model_verdict conforme cases{cases} checks{checks} facts{facts} corruptions{corruptions} native0')


if __name__ == '__main__':
    run()
