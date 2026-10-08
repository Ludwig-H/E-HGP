#!/usr/bin/env python3
"""Lecteur strict du vidage FUL1 (MHGP11FUL1) de la tour, adapte aux profils 21, 24 et 32 de la v12.

Port de bench/full_semantic.py de la v11 gelee (commit ac081a06f) et des fonctions natural et need de
bench/catalogue_semantic.py, a l'identique hormis deux adaptations (contrat de la tour, paragraphe 9) : profils admis
21, 24 et 32 (18 est abandonne, decision D6) ; ordre de Morton exact sur 3B bits, B = 32 compris (la boucle de la v11
le calculait deja sur `bits`). Memes controles, meme empreinte SEMANTIQUE (schema ehgp.v11.full_semantic.v1 :
rationnels normalises, rembourrage des mots et profil retires) : c'est elle que les vidages des profils 24 et 32
doivent reproduire.

    python3 full_reader.py <vidage> <bits> <K> <sites>      une ligne JSON (empreintes, comptes) ; code 0, ou 3

Texte de la v11 :
Strict FULL codec, without a geometric oracle on the measured cloud.

Streaming through a read-only mapping; auxiliary storage is O(sites + two adjacent
orders), not the whole payload. Rational normalization removes profile and limb
padding. Vertical naturality checks every child, using monotonically activated
lower merges and compressed ancestor paths, not a fresh full climb per edge.
"""
from array import array
from fractions import Fraction
import hashlib
import mmap
from pathlib import Path
import struct

import json
import sys

MAGIC = b'MHGP11FUL1'
SCHEMA = 'ehgp.v11.full_semantic.v1'
NONE = (1 << 32)-1
WORD = struct.Struct('<Q')
PROFILES = (21, 24, 32)


def need(value, message):
    if not value:
        raise ValueError(message)


def natural(value):
    payload = value.to_bytes(max(1, (value.bit_length() + 7) // 8), 'little')
    return WORD.pack(len(payload)) + payload


class Tree:
    def __init__(self, births, count, edge_count, root):
        self.births, self.count, self.edge_count, self.root = births, count, edge_count, root
        self.parent, self.begin, self.cardinal, self.lower, self.edges = (array('I') for _ in range(5))
        self.levels = []


def living(tree, node, level):
    need(0 <= node < tree.count, 'vertical index')
    parent = tree.parent[node]
    return tree.levels[node] <= level and (parent == NONE or level < tree.levels[parent])


def naturality(upper, lower):
    # Upper merges and lower merges are each in nondecreasing level order.
    activated = array('I', range(lower.count))
    cursor = lower.births

    def find(node):
        root = node
        while activated[root] != root:
            root = activated[root]
        while activated[node] != node:
            nxt = activated[node]; activated[node] = root; node = nxt
        return root

    for node in range(upper.births,upper.count):
        level = upper.levels[node]
        while cursor < lower.count and lower.levels[cursor] <= level:
            begin, size = lower.begin[cursor], lower.cardinal[cursor]
            for offset in range(begin,begin+size):
                activated[lower.edges[offset]] = cursor
            cursor += 1
        start, count = upper.begin[node], upper.cardinal[node]
        for offset in range(start,start+count):
            child = upper.edges[offset]
            need(find(upper.lower[child]) == upper.lower[node], 'vertical naturality of every child')


def decode(data, expected_bits, expected_k, expected_count):
    size, cursor = len(data), 0
    need(type(expected_bits) is int and expected_bits in PROFILES and
         type(expected_k) is int and 1 <= expected_k <= 12 and
         type(expected_count) is int and 0 < expected_count < NONE, 'expected header')
    need(size >= 42 and data[:10] == MAGIC, 'FULL signature/size')
    cursor = 10
    semantic = hashlib.sha256(SCHEMA.encode()+b'\0')

    def word():
        nonlocal cursor
        need(cursor+8 <= size, 'FULL truncated word')
        value, = WORD.unpack_from(data,cursor); cursor += 8
        return value

    def exact():
        negative, limbs = word(), word()
        need(negative in (0,1) and 1 <= limbs <= 64 and cursor+8*limbs <= size, 'FULL integer header')
        value = sum(word() << (64*i) for i in range(limbs))
        need(not negative or value > 0, 'FULL negative zero')
        return -value if negative else value

    def fraction():
        numerator, denominator = exact(), exact()
        need(numerator >= 0 and denominator > 0, 'FULL level sign')
        return Fraction(numerator,denominator)

    def feed_words(values):
        for value in values:
            semantic.update(WORD.pack(value))

    def feed_fraction(value):
        semantic.update(natural(int(value.numerator < 0))+natural(abs(value.numerator))+natural(value.denominator))

    bits, kmax, sites, points = word(), word(), word(), word()
    need((bits,kmax,sites) == (expected_bits,expected_k,expected_count) and kmax <= sites and
         points == sites, 'FULL header/domain/unit weights')
    need(sites <= (size-cursor)//40, 'FULL site count exceeds payload')
    feed_words((kmax,sites,points))
    identities, coordinates = set(), set()
    lo, hi = [1 << bits]*3, [-1]*3
    previous = -1
    for _ in range(sites):
        xyz = tuple(word() for _ in range(3)); weight = word()
        need(weight == 1 and max(xyz) < 1 << bits, 'FULL site domain/unit weight')
        identity = word()
        need(identity <= NONE and identity not in identities, 'FULL point ID duplicate/domain')
        identities.add(identity); coordinates.add(xyz)
        morton = sum(((value >> bit) & 1) << (3*bit+axis)
                     for axis,value in enumerate(xyz) for bit in range(bits))
        need(morton > previous, 'FULL Morton order/duplicate site')
        previous = morton
        for axis,value in enumerate(xyz):
            lo[axis], hi[axis] = min(lo[axis],value), max(hi[axis],value)
        feed_words((*xyz,weight,identity))
    identities.clear()
    previous_tree, orders = None, []
    for k in range(1,kmax+1):
        order, births, count, edges, root = (word() for _ in range(5))
        need(order == k and 0 < births <= count < NONE and count <= 2*births-1 and
             edges == count-1 and root < count, 'FULL order header/tree counts')
        need(k != 1 or births == sites, 'FULL order1 births')
        minimum = count*(72+(8 if k > 1 else 0))+births*96+edges*8
        need(minimum <= size-cursor, 'FULL node count exceeds payload')
        feed_words((order,births,count,edges,root))
        tree = Tree(births,count,edges,root)
        previous_birth, edge_cursor = None, 0
        for node in range(count):
            parent, begin, cardinal = word(), word(), word()
            need((parent == NONE) is (node == root) and (parent == NONE or node < parent < count), 'FULL parent')
            if node < births:
                need(begin == cardinal == 0, 'FULL birth CSR')
            else:
                need(cardinal >= 2 and begin == edge_cursor and cardinal <= edges-edge_cursor, 'FULL merge CSR')
                edge_cursor += cardinal
            level = fraction()
            need((level == 0) is (k == 1 and node < births), 'FULL zero level')
            feed_words((parent,begin,cardinal)); feed_fraction(level)
            tree.parent.append(parent); tree.begin.append(begin); tree.cardinal.append(cardinal); tree.levels.append(level)
            if node < births:
                numerators = tuple(exact() for _ in range(3)); denominator = exact()
                need(denominator > 0, 'FULL center denominator')
                center = tuple(Fraction(n,denominator) for n in numerators)
                need(all(lo[j] <= center[j] <= hi[j] for j in range(3)), 'FULL center outside input box')
                if k == 1:
                    need(center in coordinates, 'FULL order1 center is not a site')
                key = (level,center)
                need(previous_birth is None or previous_birth < key, 'FULL canonical birth order/duplicate')
                previous_birth = key
                for value in center:
                    feed_fraction(value)
            if k > 1:
                image = word()
                need(living(previous_tree,image,level), 'FULL vertical not alive at closed cut')
                tree.lower.append(image); feed_words((image,))
        need(edge_cursor == edges, 'FULL CSR coverage')
        seen, minimum_birth = bytearray(count), array('I',range(count))
        previous_merge = None
        for node in range(births,count):
            last, smallest = -1, NONE
            for _ in range(tree.cardinal[node]):
                child = word()
                need(last < child < node and not seen[child] and tree.parent[child] == node, 'FULL children/parents')
                need(tree.levels[child] < tree.levels[node], 'FULL child not strictly earlier')
                seen[child] = 1; last = child; smallest = min(smallest,minimum_birth[child])
                tree.edges.append(child); feed_words((child,))
            minimum_birth[node] = smallest
            key = (tree.levels[node],smallest)
            need(previous_merge is None or previous_merge < key, 'FULL canonical merge order')
            previous_merge = key
        need(len(tree.edges) == edges and all(bool(seen[i]) is (i != root) for i in range(count)), 'FULL unique root/coverage')
        if previous_tree is not None:
            naturality(tree,previous_tree)
        if k == 1:
            coordinates.clear()
        previous_tree = tree
        orders.append(dict(order=k,nodes=count,births=births,merges=count-births,edges=edges,root=root,
                           verticals=count if k > 1 else 0))
    need(cursor == size, 'FULL trailing bytes')
    raw = hashlib.sha256()
    for begin in range(0,size,1 << 20):
        raw.update(data[begin:min(size,begin+(1 << 20))])
    totals = {key: sum(order[key] for order in orders) for key in ('nodes','births','merges','edges','verticals')}
    return dict(schema=SCHEMA,sha256=semantic.hexdigest(),raw_sha256=raw.hexdigest(),bytes=size,
                coord_bits=bits,kmax=kmax,sites=sites,points=points,orders=orders,**totals)


def inspect(path, bits, kmax, count):
    path = Path(path)
    need(path.stat().st_size >= 42, 'FULL file size')
    with path.open('rb') as source, mmap.mmap(source.fileno(),0,access=mmap.ACCESS_READ) as data:
        return decode(data,bits,kmax,count)


def main(argv):
    if len(argv) != 5:
        print('usage : full_reader.py <vidage> <bits> <K> <sites>', file=sys.stderr)
        return 2
    try:
        result = inspect(argv[1], int(argv[2]), int(argv[3]), int(argv[4]))
    except (ValueError, OSError) as error:
        print(json.dumps(dict(refus=str(error))))
        return 3
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
