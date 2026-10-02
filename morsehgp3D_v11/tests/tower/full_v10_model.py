"""Pure countertests of the common native FULL comparison; no C++ process is run."""
import copy
import tempfile
from pathlib import Path

import full_v10_codec as codec
import full_v10_diff as driver
import forest_model_test as model


def old_payload(row):
    """Synthetic v10 numbering from explicit native-like birth seeds, not the codec's T2 classifier."""
    sites, balls = row['sites'],row['balls']; levels = sorted(set(codec.fraction(b['level'],16) for b in balls))
    cat = []
    for ball in balls:
        fields = []
        for key in ('support','inner','shell'):
            fields.append(' '.join(','.join(map(str,sites[i])) for i in ball[key]))
        q, p, m = ball['qmin'],len(ball['inner']),len(ball['shell'])
        cat.append('%d %d %d %d %d | %s' %
                   (levels.index(codec.fraction(ball['level'],16)),q,p,m,int(q != m),' | '.join(fields)))
    lines, previous = [], None
    for order in row['orders']:
        k, nodes = order['order'],order['nodes']
        births = [i for i,n in enumerate(nodes) if n['seed'] is not None]
        perm = sorted(births,key=lambda i:nodes[i]['seed']['site' if k == 1 else 'ball'])
        mapping = {i:j for j,i in enumerate(perm)}; minimum = dict(mapping)
        for i,n in enumerate(nodes):
            if n['seed'] is None: minimum[i] = min(minimum[c] for c in n['children'])
        perm += sorted((i for i,n in enumerate(nodes) if n['seed'] is None),
                       key=lambda i:(codec.fraction(nodes[i]['level'],16),minimum[i]))
        mapping = {i:j for j,i in enumerate(perm)}
        lines.append('order %d %d %d' % (k,len(nodes),len(sites)))
        for i in perm:
            n = nodes[i]; level = codec.fraction(n['level'],16)
            lines.append('node %d %d %d %d %d' %
                         (mapping[i],-1 if n['parent'] is None else mapping[n['parent']],
                          level.numerator*7,level.denominator*7,
                          -1 if k == 1 else previous[order['lower'][i]]))
        for p in sites: lines.append('point %d %d %d 0 0' % tuple(p))
        previous = mapping
    return '\n'.join(cat)+'\n' if cat else '', '\n'.join(lines)+'\n'


def main():
    requests = {r['name']:r for r in codec.geometry.requests(18)}
    positives, corruptions = 0, 0
    samples = {}
    for name in driver.NAMES:
        req = requests[name]; row = model.answer(req,18); sites = [tuple(p) for p in row['sites']]
        cat, dump = old_payload(row)
        balls = codec.catalogue(cat,sites); old = codec.old_orders(dump,balls,sites,req['kmax'])
        old_bytes = codec.common(sites,old)
        for bits in (18,21,24):
            new = dict(row,coord_bits=bits)
            codec.need(codec.common(sites,codec.new_orders(new,bits,sites,req['kmax'])) == old_bytes,
                       'positive native-like comparison '+name)
            positives += 1
        samples[name] = (row,sites,cat,dump,old)
    def reject(thunk):
        nonlocal corruptions
        try: thunk()
        except (ValueError,KeyError,TypeError,IndexError):
            corruptions += 1; return
        raise ValueError('undetected corruption')
    row, sites, cat, dump, old = samples['line024']
    for bad in (dump.replace('order 1 4','order 1 5',1),dump.replace('node 3 -1','node 3 0',1),
                dump.replace('node 3 -1 7 7','node 3 -1 14 7',1),dump+'extra\n',
                dump.replace('point 0 0 0','point 99 0 0',1)):
        def compare(bad=bad):
            value = codec.common(sites,codec.old_orders(bad,codec.catalogue(cat,sites),sites,3))
            codec.need(value == codec.common(sites,old),'different common bytes')
        reject(compare)
    for mutation in (
        lambda v:v['orders'][0]['nodes'][3]['children'].pop(),
        lambda v:v['orders'][0]['nodes'][0].__setitem__('parent',None),
        lambda v:v['orders'][1]['lower'].__setitem__(0,0),
        lambda v:v['orders'][0]['nodes'][3].__setitem__('level',['0','1']),
        lambda v:v['orders'][0]['nodes'][0]['seed'].__setitem__('site',1),
        lambda v:v.__setitem__('coord_bits',24),
        lambda v:v.__setitem__('status','resource_exhausted'),
        lambda v:v.__setitem__('owner_after',4),
        lambda v:v['orders'].pop(),
    ):
        bad = copy.deepcopy(row); mutation(bad)
        def compare(bad=bad):
            codec.need(codec.common(sites,codec.new_orders(bad,18,sites,3)) == codec.common(sites,old),'different bytes')
        reject(compare)
    for bad in (cat.replace('0 2 0 2 0','0 2 1 2 0',1),cat+cat,
                cat.replace('2,0,0','3,0,0',1),cat.replace('0 2 0 2 0','0 2 0 2 1',1)):
        reject(lambda bad=bad:codec.catalogue(bad,sites))
    manifest = driver.profiles.load(driver.PIN)
    with tempfile.TemporaryDirectory(prefix='full-v10-pure-') as temp:
        fake = Path(temp)/'source.tar.gz'; fake.write_bytes(b'corrupted source archive')
        reject(lambda:driver.extract(fake,Path(temp)/'destination',manifest))
    codec.need(positives == 42 and corruptions == 19,'nonvacuity')
    print('full_v10_model_verdict conforme positives42 corruptions19 native0')


if __name__ == '__main__':
    main()
