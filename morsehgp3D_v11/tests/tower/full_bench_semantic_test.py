"""FULL codec controls on exact Definition trees, malformed binary fixtures, no native program."""
import copy
from fractions import Fraction
import hashlib
import math
from pathlib import Path
import struct
import sys
import tempfile

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'bench'))
import full_semantic as codec
import forest_oracle as oracle

CHECKS = 0


def check(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def fixture(points, kmax):
    records = oracle.data.fixtures.records(tuple(points))
    sites, identifiers, expected = oracle.truth(records,kmax)
    orders = []
    for order in expected:
        nodes = [dict(level=n.level,center=n.center,children=list(n.children),parent=codec.NONE) for n in order.nodes]
        for parent,node in enumerate(nodes):
            for child in node['children']:
                nodes[child]['parent'] = parent
        orders.append(dict(nodes=nodes,births=sum(n['center'] is not None for n in nodes),
                           root=next(i for i,n in enumerate(nodes) if n['parent'] == codec.NONE),
                           lower=None if order.lower is None else list(order.lower)))
    return dict(sites=sites,ids=[ids[0] for ids in identifiers],orders=orders)


def encode(value, bits=18, padding=0, scale=1):
    output, positions = bytearray(codec.MAGIC), {}
    def word(key, number):
        positions[key] = len(output); output.extend(struct.pack('<Q',number))
    def exact(key, number):
        width = max(1,(abs(number).bit_length()+63)//64)+padding
        word(key+'.sign',int(number < 0)); word(key+'.width',width)
        for i in range(width):
            word(key+'.word'+str(i),(abs(number) >> (64*i)) & ((1 << 64)-1))
    word('bits',bits); word('K',len(value['orders'])); word('sites',len(value['sites'])); word('points',len(value['sites']))
    for i,(point,identifier) in enumerate(zip(value['sites'],value['ids'])):
        for axis,coordinate in enumerate(point):
            word('s%d.xyz%d' % (i,axis),coordinate)
        word('s%d.weight' % i,1); word('s%d.id' % i,identifier)
    for k,order in enumerate(value['orders'],1):
        prefix = 'o%d.' % k
        nodes = order['nodes']; edges = sum(len(n['children']) for n in nodes)
        for key,number in (('k',k),('births',order['births']),('N',len(nodes)),('E',edges),('root',order['root'])):
            word(prefix+key,number)
        cursor = 0
        for i,node in enumerate(nodes):
            tag = prefix+'n%d.' % i
            word(tag+'parent',node['parent']); word(tag+'begin',cursor if node['children'] else 0)
            word(tag+'count',len(node['children'])); cursor += len(node['children'])
            level = node['level']
            exact(tag+'num',level.numerator*scale); exact(tag+'den',level.denominator*scale)
            if i < order['births']:
                center = node['center']; common = math.lcm(*(v.denominator for v in center))*scale
                for axis,coordinate in enumerate(center):
                    exact(tag+'center%d' % axis,int(coordinate*common))
                exact(tag+'center_den',common)
            if k > 1:
                word(tag+'lower',order['lower'][i])
        for i,node in enumerate(nodes):
            for j,child in enumerate(node['children']):
                word(prefix+'n%d.child%d' % (i,j),child)
    return bytes(output), positions


def decoded(raw, value, bits=18):
    return codec.decode(raw,bits,len(value['orders']),len(value['sites']))


def swapped(order, a, b):
    """A structurally valid relabelling; canonical ordering must reject it."""
    def remap(i):
        return b if i == a else a if i == b else i
    order['nodes'][a],order['nodes'][b] = order['nodes'][b],order['nodes'][a]
    for node in order['nodes']:
        node['parent'] = remap(node['parent'])
        node['children'] = sorted(remap(i) for i in node['children'])
    order['root'] = remap(order['root'])


def main():
    positive, corruptions = 0, 0
    examples = [fixture([(0,0,0)],1), fixture([(0,0,0),(3,0,0)],2),
                fixture([(0,0,0),(2,0,0),(4,0,0)],3),
                fixture([(0,0,0),(4,0,0),(0,4,0),(4,4,0)],4),
                fixture([(x,0,0) for x in (0,2,4,20,22,24)],3),
                fixture([(0,0,0),(2,2,0),(2,0,2),(0,2,2)],4)]
    for value in examples:
        semantic, hashes = set(), set()
        for bits,padding,scale in ((18,0,1),(21,1,7),(24,3,1 << 130)):
            raw,_ = encode(value,bits,padding,scale)
            result = decoded(raw,value,bits); positive += 1
            semantic.add(result['sha256']); hashes.add(result['raw_sha256'])
            check(result['raw_sha256'] == hashlib.sha256(raw).hexdigest() and result['bytes'] == len(raw), 'raw bytes/hash')
            check(result['sites'] == result['points'] == len(value['sites']), 'whole unit input')
            check(result['nodes'] == sum(len(o['nodes']) for o in value['orders']), 'nodes total')
            check(result['births'] == sum(o['births'] for o in value['orders']), 'births total')
            check(result['edges'] == result['nodes']-len(value['orders']) and
                  result['merges'] == result['nodes']-result['births'], 'forest totals')
            check(result['verticals'] == sum(len(o['nodes']) for o in value['orders'][1:]), 'vertical total')
        check(len(semantic) == 1 and len(hashes) == 3, 'profile/padding/rational normalization')
    single = examples[0]
    raw,_ = encode(single,padding=63)
    check(decoded(raw,single)['sha256'] == decoded(encode(single)[0],single)['sha256'], '64 limbs accepted')
    positive += 1
    for bits in (21,24):
        maximum = (1 << bits)-1
        high = fixture([(0,0,0),(maximum,maximum,0),(maximum,0,maximum),(0,maximum,maximum)],4)
        raw,_ = encode(high,bits,padding=1)
        check(decoded(raw,high,bits)['nodes'] > 0, 'actual high coordinate bits')
        positive += 1
    value = examples[2]; raw,positions = encode(value)
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-codec-') as directory:
        path = Path(directory)/'full.bin'; path.write_bytes(raw)
        check(codec.inspect(path,18,3,3) == decoded(raw,value), 'mapped file decoder')
        path.write_bytes(b'')
        try:
            codec.inspect(path,18,3,3)
        except ValueError:
            corruptions += 1
        else:
            raise ValueError('empty file accepted')

    def refuses(payload, expected='', source=value, bits=18):
        nonlocal corruptions
        try:
            decoded(payload,source,bits)
        except ValueError as error:
            check(not expected or expected in str(error), 'wrong refusal: '+str(error))
            corruptions += 1
            return
        raise ValueError('corruption accepted')
    def word_fault(key, number, expected=''):
        bad = bytearray(raw); struct.pack_into('<Q',bad,positions[key],number); refuses(bad,expected)
    refuses(b'X'+raw[1:],'signature')
    refuses(raw[:-1],'payload')
    refuses(raw+b'\0','trailing')
    for key,number,expected in (
        ('bits',17,'header'), ('K',2,'header'), ('sites',4,'header'), ('points',4,'unit'),
        ('s0.xyz0',1 << 18,'domain'), ('s0.weight',2,'unit'), ('s0.id',1 << 32,'ID'),
        ('s1.id',value['ids'][0],'duplicate'), ('s1.xyz0',0,'Morton'),
        ('o1.k',2,'order header'), ('o1.births',4,'order1 births'), ('o1.N',1 << 32,'order header'),
        ('o1.E',2,'order header'), ('o1.root',0,'parent'),
        ('o1.n0.parent',codec.NONE,'parent'), ('o1.n0.parent',0,'parent'),
        ('o1.n3.parent',(1 << 64)-1,'parent'), ('o1.n0.begin',1,'birth CSR'),
        ('o1.n0.count',2,'birth CSR'), ('o1.n3.begin',1,'merge CSR'), ('o1.n3.count',1,'merge CSR'),
        ('o1.n3.count',4,'merge CSR'), ('o1.n0.num.sign',1,'negative zero'),
        ('o1.n3.num.sign',2,'integer header'), ('o1.n3.num.sign',1,'level sign'),
        ('o1.n3.num.width',0,'integer header'), ('o1.n3.num.width',65,'integer header'),
        ('o1.n3.den.word0',0,'level sign'), ('o1.n0.num.word0',1,'zero level'),
        ('o1.n3.num.word0',0,'zero level'), ('o1.n0.center_den.word0',0,'denominator'),
        ('o1.n1.center0.sign',1,'outside'), ('o1.n0.center0.word0',1,'not a site'),
        ('o1.n0.center0.word0',2,'birth order'), ('o1.n3.child0',1,'children/parents'),
        ('o1.n3.child2',3,'children/parents'), ('o1.n3.child2',1,'children/parents'),
        ('o2.n0.lower',codec.NONE,'vertical index'), ('o2.n0.lower',0,'not alive'),
    ):
        word_fault(key,number,expected)
    # A zero birth above order one is refused before inspecting its vertical.
    pair = examples[1]; pair_raw,pair_pos = encode(pair)
    bad = bytearray(pair_raw); struct.pack_into('<Q',bad,pair_pos['o2.n0.num.word0'],0)
    refuses(bad,'zero level',pair)
    # All levels remain positive: an equal-level merge parent must reach the edge guard.
    equal = copy.deepcopy(examples[4]); equal['orders'] = equal['orders'][:1]
    equal['orders'][0]['nodes'][equal['orders'][0]['root']]['level'] = Fraction(1)
    refuses(encode(equal)[0],'child not strictly earlier',equal)
    # Equal-level relabelling keeps counts, CSR and parents valid but violates min-birth ordering.
    groups = copy.deepcopy(examples[4]); groups['orders'] = groups['orders'][:1]
    swapped(groups['orders'][0],6,7)
    refuses(encode(groups)[0],'canonical merge order',groups)
    # One non-first child's vertical still names a living component, but is not natural.
    groups = copy.deepcopy(examples[4])
    upper,lower = groups['orders'][1],groups['orders'][0]
    target = next(i for i,n in enumerate(upper['nodes']) if len(n['children']) >= 2 and n['level'] == 4)
    child = upper['nodes'][target]['children'][1]
    foreign = next(i for i,n in enumerate(lower['nodes']) if n['level'] == 1 and i != upper['lower'][target])
    groups['orders'][1]['lower'][child] = foreign
    refuses(encode(groups)[0],'naturality',groups)
    groups = copy.deepcopy(examples[4]); groups['orders'][1]['lower'][target] = foreign
    refuses(encode(groups)[0],'naturality',groups)
    # The hash preserves IDs and geometry even for separately valid trees.
    changed = copy.deepcopy(value); changed['ids'][0] = 7
    check(decoded(encode(changed)[0],changed)['sha256'] != decoded(raw,value)['sha256'], 'ID hash identity')
    scaled = fixture([(0,0,0),(4,0,0),(8,0,0)],3)
    check(decoded(encode(scaled)[0],scaled)['sha256'] != decoded(raw,value)['sha256'], 'geometry hash identity')
    check(positive == 21 and corruptions == 48 and CHECKS >= 150, 'codec floors')
    print('full_semantic_verdict conforme positives%d corruptions%d checks%d native0' % (positive,corruptions,CHECKS))


if __name__ == '__main__':
    try:
        main()
    except (ValueError,KeyError,TypeError,IndexError) as error:
        print('REFUS '+str(error),file=sys.stderr)
        raise SystemExit(1)
