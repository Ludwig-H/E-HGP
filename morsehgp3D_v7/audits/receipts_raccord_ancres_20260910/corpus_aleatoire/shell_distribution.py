#!/usr/bin/env python3
"""Distribution des coquilles (u), interieurs (p), arites (q_min) et tailles de
lots a niveau egal dans les catalogues d'une campagne (regeneres par graine).
Aucun assert."""
import json
import random
import sys
import tower_corpus as tc


def main():
    d = json.load(open(sys.argv[1]))
    rng = random.Random(d['seed'])
    clouds = []
    if d['fixtures']:
        for name, points, kmax in tc.FIXTURES:
            n = len(points)
            clouds.append(dict(name=name, n=n, points=points, ids=[4294967295, 17, 0, 902, 2147483648, 3, 65536, 42][:n], kmax=kmax))
    for _ in range(len(d['clouds']) - len(clouds)):
        clouds.append(tc.make_cloud(rng) if d['family'] == 'random' else tc.make_cocircular(rng))
    u_hist, p_hist, q_hist, gap_hist, lot_hist = {}, {}, {}, {}, {}
    for cloud, summary in zip(clouds, d['clouds']):
        model = tc.build_model(cloud['points'])
        rows, info = tc.catalogue(model, cloud['kmax'], cloud['ids'])
        if info['balls'] != summary['catalogue']['balls']:
            print('REFUSAL regeneration_mismatch', file=sys.stderr)
            return 3
        for r in rows:
            u_hist[r['u']] = u_hist.get(r['u'], 0) + 1
            p_hist[r['p']] = p_hist.get(r['p'], 0) + 1
            q_hist[r['arity']] = q_hist.get(r['arity'], 0) + 1
            gap_hist[r['u'] - r['arity']] = gap_hist.get(r['u'] - r['arity'], 0) + 1
        for k in range(1, cloud['kmax'] + 1):
            groups = {}
            for r in rows:
                if r['p'] + r['arity'] - 1 <= k <= min(cloud['kmax'], r['p'] + r['u']):
                    groups[r['radius']] = groups.get(r['radius'], 0) + 1
            for count in groups.values():
                if count >= 2:
                    lot_hist[count] = lot_hist.get(count, 0) + 1
    fmt = lambda h: {str(k): v for k, v in sorted(h.items())}
    print(json.dumps(dict(file=sys.argv[1], shell_size_u=fmt(u_hist), interior_p=fmt(p_hist), arity_qmin=fmt(q_hist),
                          extra_shell_gap_u_minus_qmin=fmt(gap_hist), equal_level_lot_sizes=fmt(lot_hist)),
                     sort_keys=True, separators=(',', ':')))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except tc.Refusal as r:
        print('REFUSAL', r.reason, file=sys.stderr)
        sys.exit(r.code)
