"""Exact arithmetic witnesses for port contracts; not the native sorter."""
from fractions import Fraction as F
import json
from random import Random


def require(value,message):
    if not value:
        raise ValueError(message)


def certified_groups(rows):
    """Intervals sorted by lower bound; running maximum upper bound is essential."""
    order = sorted(rows,key=lambda row:(row['lo'],row['id']))
    groups = []
    for row in order:
        if not groups or row['lo']>groups[-1]['max_hi']:
            groups.append(dict(max_hi=row['hi'],rows=[row]))
        else:
            groups[-1]['rows'].append(row)
            groups[-1]['max_hi'] = max(groups[-1]['max_hi'],row['hi'])
    return groups


def run():
    # The old resize(false)->true lemma depends on numerator<2^192.
    numerator,denominator,threshold = 2**200,2**193,1
    old_resize_fails = threshold*denominator>=2**192
    require(old_resize_fails and not F(numerator,denominator)<=threshold,
            'widening witness absent')
    # Double-view event coalescence is distinct from exact rank equality.
    left,right = F(2**256),F(2**256+1)
    require(left<right and float(left)==float(right),'double collision absent')
    # Interval overlap is not transitive, so it is not sort equivalence.
    a,b,c = (dict(lo=F(lo),hi=F(hi)) for lo,hi in ((0,2),(1,4),(3,5)))
    overlap = lambda x,y: x['lo']<=y['hi'] and y['lo']<=x['hi']
    require(overlap(a,b) and overlap(b,c) and not overlap(a,c),'overlap witness absent')
    # A long first interval must remain in the running maximum.
    nested = [dict(id=0,lo=F(0),hi=F(100),value=F(90)),
              dict(id=1,lo=F(1),hi=F(2),value=F(1)),
              dict(id=2,lo=F(3),hi=F(4),value=F(3))]
    require(len(certified_groups(nested))==1,'long upper bound was forgotten')
    rng = Random(0x466_20260930)
    fixtures = [nested]
    for _ in range(32):
        rows = []
        for index in range(64):
            value = F(rng.randrange(0,10000),rng.randrange(1,33))
            rows.append(dict(id=index,value=value,lo=value-F(rng.randrange(17),9),
                             hi=value+F(rng.randrange(17),9)))
        fixtures.append(rows)
    intervals = boundaries = 0
    for rows in fixtures:
        groups = certified_groups(rows)
        merged = []
        for group in groups:
            for row in group['rows']:
                require(row['lo']<=row['value']<=row['hi'],'invalid certified interval')
                intervals += 1
            merged.extend(sorted(group['rows'],key=lambda row:(row['value'],row['id'])))
        require([r['id'] for r in merged]==[r['id'] for r in
                sorted(rows,key=lambda row:(row['value'],row['id']))],'grouped exact order differs')
        for left_group,right_group in zip(groups,groups[1:]):
            require(left_group['max_hi']<min(r['lo'] for r in right_group['rows']),
                    'uncertified group boundary')
            boundaries += 1
    return dict(status='PASS',interval_fixtures=len(fixtures),intervals=intervals,
                certified_boundaries=boundaries,widened_resize_witness=dict(
                    numerator=str(numerator),denominator=str(denominator),threshold=threshold,
                    exact_ratio='128',old_conditional_true_is_wrong=True),
                exact_double_collision=True,overlap_nontransitive=True,
                scope='rational model only; native sort/converters/performance/FULL not tested',GCP_used=False)


if __name__=='__main__':
    print(json.dumps(run(),sort_keys=True))
