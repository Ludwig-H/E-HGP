#!/usr/bin/env python3
"""Vrais appels MES-G1 sur une entree collineaire synthetique, sans donnees reelles."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PIN = '274592a30f6961cb7702125dcd2f031ff22b7df2'
NONE = 2**32-1


def need(value, why):
    if not value:
        raise RuntimeError(why)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def dump(kind, order, sections):
    raw = struct.pack('<8s6IQ24s', b'MHGP12DP', 1, kind, 21, 2, order, len(sections), 4, b'audit')
    for tag, width, count, data in sections:
        need(len(data) == width * count, 'section size')
        raw += struct.pack('<8sIIQ', tag.encode(), width, 0, count) + data
        raw += b'\0' * (-len(data) % 8)
    return raw


def catalogue():
    # x = 0,1,2,3. Exactly the adjacent pairs (p=0) and distance-two pairs
    # (p=1) belong to Cat_2. Diameter [0,3] has p=2,q=2 and is outside it.
    records, values, offsets = [], [], [0]
    for a, b in [(0,1), (1,2), (2,3), (0,2), (1,3)]:
        interior = list(range(a+1, b))
        records.append(struct.pack('<8I', b-a, len(interior), 2, 2, a, b, NONE, NONE))
        values += interior + [a,b]
        offsets.append(len(values))
    return dump(1, 0, [
        ('SITEXYZ', 12, 4, b''.join(struct.pack('<3I', x, 0, 0) for x in range(4))),
        ('BALLS', 32, 5, b''.join(records)),
        ('POPOFF', 8, 6, struct.pack('<6Q', *offsets)),
        ('POPVAL', 4, len(values), struct.pack('<'+'I'*len(values), *values)),
        ('NLEVELS', 8, 1, struct.pack('<Q', 3)),
    ])


def order(route):
    # F={0,3}, census saturated at k=2, I={1,2}, next part hits birth ball 1.
    # Only the four sections consumed by this microbench are supplied.
    return dump(2, 2, [
        ('PARTS', 8, 1, struct.pack('<2I', 0, 3)),
        ('PARTINF', 8, 1, struct.pack('<4BI', route, 1, 0, 0, NONE)),
        ('PARTOFF', 8, 2, struct.pack('<2Q', 0, 1)),
        ('SEEDS', 12, 1, struct.pack('<2I2BH', 1, 0, 2, 1, 0)),
    ])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--binary', required=True, type=Path)
    ap.add_argument('--build', required=True, type=Path)
    args = ap.parse_args()
    build = json.loads(args.build.read_text())
    before = {p: digest((ROOT/p).read_bytes()) for p in build['source_sha256']}
    need(before == build['source_sha256'], 'build dependency hashes')
    for p,h in before.items():
        blob = subprocess.run(['git','show',PIN+':'+p], cwd=ROOT, check=True, capture_output=True).stdout
        need(digest(blob) == h, 'pin '+p)
    bh = digest(args.binary.read_bytes())
    need(bh == build['binary_sha256'], 'binary identity')
    results = {}
    with tempfile.TemporaryDirectory(prefix='ehgp-g1-audit-') as temp:
        folder = Path(temp)
        (folder/'cat.bin').write_bytes(catalogue())
        # The independent transition reader admits this exact reference catalogue.
        cp = subprocess.run(['python3','-B','-S','-O',str(ROOT/'morsehgp3D_v12/reference/transition_catalogue.py'),str(folder/'cat.bin'),str(folder/'cat.bin'),'--convention-candidat','v11'], capture_output=True, text=True, timeout=10)
        need(cp.returncode == 0, 'synthetic catalogue invalid: '+cp.stderr)
        for name, route, extra in [('valid',2,[]), ('forged_catalogue_route',1,[]), ('selected_order_absent',2,['--ordres','9'])]:
            (folder/'ordre_2.bin').write_bytes(order(route))
            p = subprocess.run([str(args.binary),str(folder),*extra], capture_output=True, text=True, timeout=10)
            lines = [json.loads(line) for line in p.stdout.splitlines()]
            need(p.returncode == 0, name+' unexpected refusal')
            summary = next(line for line in lines if line.get('phase')=='bilan')
            orders = [line for line in lines if line.get('phase')=='ordre']
            need(summary['code']==0, name+' summary')
            results[name] = {'exit_code':p.returncode,'orders':[line['k'] for line in orders],
                             'route1':sum(line['route1'] for line in orders),
                             'route2':summary['route2']['parties'],'judge_errors':summary['ecarts_juge'],
                             'nearest_neighbour_errors':summary['ecarts_voisins'],
                             'certified':summary['route2']['ensembles']['voisins']['certifiees']}
        need(results['valid']['route2']==1 and results['valid']['certified']==1, 'positive control')
        need(results['forged_catalogue_route']['route1']==1 and results['forged_catalogue_route']['route2']==0, 'route bypass')
        need(results['selected_order_absent']['orders']==[], 'empty selection')
    need(before == {p:digest((ROOT/p).read_bytes()) for p in before}, 'sources changed')
    need(digest(args.binary.read_bytes())==bh, 'binary changed')
    print(json.dumps({'pin':PIN,'schema':'ehgp.audit.t2.g1_probe.v1','results':results,
                      'source_hashes_closed':True,'binary_sha256':bh,'library_sha256':build['library_sha256'],
                      'script_sha256':digest(Path(__file__).read_bytes()),
                      'scope':'Synthetic admission/judge only; no claim that historical real dumps were forged.',
                      'gcp_used':False}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
