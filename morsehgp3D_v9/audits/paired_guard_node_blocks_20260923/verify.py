#!/usr/bin/env python3
"""Static exact reader for positive node-pair certificates; no product binary."""
from collections import defaultdict
from hashlib import sha256
import argparse
import json
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
OLD = HERE.parent / 'paired_guards_precore_20260923'
SAMPLE = HERE.parent / 'paired_guard_index_shadow_20260923' / 'SAMPLES.tsv'
OUTPUT = HERE / 'RESULT.stdout'


def must(ok, message):
    if not ok:
        raise RuntimeError(message)


def box(text):
    node, size, limits, listed = text.split(':')
    xyz = tuple(map(int, limits.split(',')))
    pts = {}
    for item in listed.split(','):
        rid, x, y, z = map(int, item.split('/'))
        must(rid not in pts, 'duplicate raw ID inside block')
        pts[rid] = (x, y, z)
    must(len(pts) == int(size) and len(xyz) == 6, 'block count or box')
    must(all(min(p[i] for p in pts.values()) == xyz[i] and
             max(p[i] for p in pts.values()) == xyz[i+3] for i in range(3)),
         'stored box does not equal point enclosure')
    return int(node), pts, xyz[:3], xyz[3:]


def norm(v):
    return sum(x*x for x in v)


def calc(a, b, G, H):
    d = tuple(b[i]-a[i] for i in range(3))
    s = tuple(a[i]+b[i] for i in range(3))
    D = norm(d)
    must(D > 0, 'zero edge')
    intervals = []
    Q = []
    for low, high in (G, H):
        w = [(2*low[i]-s[i], 2*high[i]-s[i]) for i in range(3)]
        intervals.append(w)
        Q.append(sum(max(lo*lo, hi*hi) for lo, hi in w))
    Hmin = 2*D-Q[0]-Q[1]
    t = [(intervals[0][i][0]+intervals[1][i][0],
          intervals[0][i][1]+intervals[1][i][1]) for i in range(3)]
    Xmax = 0
    for i in range(3):
        j, k = (i+1)%3, (i+2)%3
        left = [d[j]*x for x in t[k]]
        right = [d[k]*x for x in t[j]]
        lo, hi = min(left)-max(right), max(left)-min(right)
        Xmax += max(lo*lo,hi*hi)
    return Hmin, Xmax


def point_pair(a, b, g, h, lane):
    d = tuple(b[i]-a[i] for i in range(3))
    s = tuple(a[i]+b[i] for i in range(3))
    wg = tuple(2*g[i]-s[i] for i in range(3))
    wh = tuple(2*h[i]-s[i] for i in range(3))
    H = 2*norm(d)-norm(wg)-norm(wh)
    t = tuple(wg[i]+wh[i] for i in range(3))
    X = sum((d[(i+1)%3]*t[(i+2)%3]-d[(i+2)%3]*t[(i+1)%3])**2
            for i in range(3))
    return H > 0 and (3*H*H > 4*X if lane == 3 else H*H > 2*X), H, X


def main():
    manifest = json.loads((HERE/'MANIFEST.json').read_text())
    must(manifest['schema']=='mhgp9_audit_node_blocks_manifest_v1',
         'manifest schema')
    for relative, expected in manifest['sha256'].items():
        path=HERE/relative
        must(sha256(path.read_bytes()).hexdigest()==expected,
             f'manifest SHA mismatch: {relative}')
    # q4 contact is strict: this singleton equality admits q3 but not q4.
    eq_a, eq_b = (10,10,10), (14,10,10)
    eq_g, eq_h = (11,9,9), (11,10,10)
    must(calc(eq_a,eq_b,(eq_g,eq_g),(eq_h,eq_h)) == (16,128),
         'equality fixture interval')
    must(point_pair(eq_a,eq_b,eq_g,eq_h,3)[0] and
         not point_pair(eq_a,eq_b,eq_g,eq_h,4)[0],
         'strict q4 equality fixture')
    must(calc(eq_a,eq_b,(eq_a,eq_a),(eq_b,eq_b))[0] == 0 and
         not point_pair(eq_a,eq_b,eq_a,eq_b,3)[0],
         'H=0 equality fixture')
    ap = argparse.ArgumentParser()
    ap.add_argument('--points',type=Path)
    ap.add_argument('--raw-ids',type=Path)
    args = ap.parse_args()
    must((args.points is None)==(args.raw_ids is None), 'both LIVE inputs required')
    live = None
    if args.points:
        provenance=json.loads((OLD/'PROVENANCE.json').read_text())['input_sha256']
        must(sha256(args.points.read_bytes()).hexdigest()==provenance['points'] and
             sha256(args.raw_ids.read_bytes()).hexdigest()==provenance['raw_ids'],
             'LIVE input SHA mismatch')
        pts=[tuple(x) for x in struct.iter_unpack('<III',args.points.read_bytes())]
        ids=[x[0] for x in struct.iter_unpack('<I',args.raw_ids.read_bytes())]
        must(len(pts)==len(ids)==123389 and len(set(ids))==len(ids),'LIVE input identity')
        live=dict(zip(ids,pts))
    first = json.loads((OLD/'RESULT.json').read_text())
    second = json.loads((OLD/'RESULT_SEED2.json').read_text())
    source = {(data['sample_seed'], row['a'], row['b']): row
              for data in (first, second) for row in data['rows']}
    must(len(source) == 120, 'old sample identity')
    must(len(SAMPLE.read_text().splitlines()) == 121, 'sample TSV length')
    rows = {}
    proofs = defaultdict(list)
    meta = None
    for line in OUTPUT.read_text().splitlines():
        item = line.split()
        if item[0] == 'META':
            must(meta is None and len(item) == 5, 'META schema')
            meta = list(map(int,item[1:]))
        elif item[0] == 'ROW':
            must(len(item) == 22, 'ROW schema')
            (cap, budget, seed, a, b, F, mask, oracle, c3, c4, closed,
             visits, boxes, rejected, endpoint, candidates, pair_tests,
             positive3, positive4, exhausted, ns) = map(int,item[1:])
            key = cap,budget,seed,a,b
            must(key not in rows and (seed,a,b) in source, 'ROW identity')
            old = source[seed,a,b]
            must((F,mask,oracle) == (old['F'],old['mask'],int(old['results']['16']['closed'])),
                 'old S2 row mismatch')
            must(closed == int((not mask&2 or c3>=4) and (not mask&4 or c4>=3)),
                 'joint threshold')
            must(visits <= 4*budget and candidates <= 64 and
                 boxes >= visits and rejected <= boxes and pair_tests <= 2016 and
                 positive3 <= pair_tests and positive4 <= positive3 and
                 endpoint <= visits and exhausted <= 4 and ns >= 0,
                 'work counter invariants')
            rows[key] = dict(F=F,mask=mask,oracle=oracle,c3=c3,c4=c4,
                             closed=closed,visits=visits,boxes=boxes,
                             rejected=rejected,endpoint=endpoint,
                             candidates=candidates,pair_tests=pair_tests,
                             positive3=positive3,positive4=positive4,
                             exhausted=exhausted,ns=ns)
        elif item[0] == 'PROOF':
            must(len(item) == 12, 'PROOF schema')
            cap,budget,seed,a,b,lane,pop,Hmin,Xmax = map(int,item[1:10])
            G = box(item[10]); H = box(item[11])
            must(lane in (3,4) and pop == min(len(G[1]),len(H[1])),
                 'proof lane or capacity')
            must(G[0] != H[0] and set(G[1]).isdisjoint(H[1]) and
                 a not in G[1] and b not in G[1] and
                 a not in H[1] and b not in H[1],
                 'proof block disjointness/endpoints')
            old = source[seed,a,b]
            endpoints = [tuple(first['proof_points'].get(str(rid),
                                second['proof_points'].get(str(rid), [])))
                         for rid in (a,b)]
            must(all(len(x)==3 for x in endpoints), 'stored endpoint coordinates')
            if live:
                must(all(live[rid]==point for rid,point in zip((a,b),endpoints)),
                     'LIVE endpoint coordinate mismatch')
            want = calc(*endpoints, (G[2],G[3]), (H[2],H[3]))
            must(want == (Hmin,Xmax) and Hmin > 0 and
                 (3*Hmin*Hmin > 4*Xmax if lane == 3 else Hmin*Hmin > 2*Xmax),
                 'strict block interval certificate')
            for rid,p in list(G[1].items())+list(H[1].items()):
                saved = first['proof_points'].get(str(rid),
                          second['proof_points'].get(str(rid)))
                # The old oracle may not have used this block site; pinned input
                # identity is checked separately in the LIVE replay.
                if saved is not None: must(tuple(saved)==p, 'old point coordinate mismatch')
                if live: must(live[rid]==p, 'LIVE block point coordinate mismatch')
            for gp in G[1].values():
                for hp in H[1].values():
                    okay, exact_H, exact_X = point_pair(*endpoints,gp,hp,lane)
                    must(okay and exact_H >= Hmin and exact_X <= Xmax,
                         'cross product point predicate')
            proofs[cap,budget,seed,a,b,lane].append((G,H,pop))
        else:
            raise RuntimeError('unknown output line')
    must(meta and meta[0:2] == [123389,246777] and len(rows)==1920,
         'META or ROW count')
    for cap in (1,2,4,8):
        for budget in (64,256,1024,4096):
            relevant = {k:v for k,v in rows.items() if k[:2]==(cap,budget)}
            must(len(relevant)==120, 'configuration missing rows')
            for key,row in relevant.items():
                witnessed = budget==4096 or (budget==64 and cap in (4,8))
                if witnessed and row['closed']:
                    for lane,mask_bit,threshold in ((3,2,4),(4,4,3)):
                        group=proofs[cap,budget,key[2],key[3],key[4],lane]
                        if row['mask']&mask_bit:
                            allids=[]
                            credit=0
                            for G,H,pop in group:
                                allids.extend(G[1]); allids.extend(H[1]); credit+=pop
                            must(len(allids)==len(set(allids)) and credit>=threshold,
                                 'proof matching or threshold')
                        else:
                            must(not group, 'proof on inactive lane')
                elif witnessed:
                    must(not proofs[cap,budget,key[2],key[3],key[4],3] and
                         not proofs[cap,budget,key[2],key[3],key[4],4],
                         'proof on open edge')
    # Singleton blocks reproduce the published B16 joint decisions here.
    must(all(v['closed']==v['oracle'] for k,v in rows.items()
             if k[:2]==(1,256)), 'singleton/oracle joint decisions')
    for cap in (1,2,4,8):
        for budget in (1024,4096):
            for (c,b,seed,a,e),row in rows.items():
                if (c,b)!=(cap,budget): continue
                prior=rows[cap,256,seed,a,e]
                must({k:v for k,v in row.items() if k!='ns'} ==
                     {k:v for k,v in prior.items() if k!='ns'},
                     'high-budget plateau mismatch')
    print(json.dumps(dict(schema='mhgp9_node_block_static_verify_v1',
                          output_sha256=sha256(OUTPUT.read_bytes()).hexdigest(),
                          row_count=len(rows),proof_pairs=sum(map(len,proofs.values())),
                          live_inputs_checked=bool(live),
                          block_members_checked=sum(len(G[1])+len(H[1])
                                                    for group in proofs.values()
                                                    for G,H,_ in group)),sort_keys=True))


if __name__ == '__main__':
    main()
