"""Small independent dump mutations; production judges are imported unchanged."""
import copy
import hashlib
import importlib.util
import json
import os
import shutil
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path('/workspaces/E-HGP/build/v10-fixes/oracles')
OUT = Path(sys.argv[1])
OUT.mkdir(exist_ok=True)
CGATE = ROOT / 'src/morsehgp3D_v10/tests/oracle/test_catalogue_oracle.py'
TGATE = ROOT / 'src/morsehgp3D_v10/tests/oracle/test_tower_oracle.py'
EXE = ROOT / 'build/mhgp10_tower'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


before = {str(p): digest(p) for p in (CGATE, TGATE, EXE)}
c = load('catalogue_counteraudit_gate', CGATE)
t = load('tower_counteraudit_gate', TGATE)
archive = ROOT / 'scratch/c_lv.txt'
shutil.copyfile(archive, OUT / 'catalogue_base_archived.txt')
got = c.parse_dump(OUT / 'catalogue_base_archived.txt')
sites, weights = c.fixture('square')
ref = c.Ref(sites)
js = dict(balls=len(got), levels=len({b['rank'] for b in got}))
catalogue = {'archive': str(archive), 'archive_sha256': digest(archive),
             'baseline': c.judge(ref, weights, 3, got, js), 'mutants': {}}


def cat_mut(name, func):
    mutated = copy.deepcopy(got)
    func(mutated)
    catalogue['mutants'][name] = c.judge(ref, weights, 3, mutated, js)


cat_mut('duplicate_I', lambda g: next(b for b in g if b['I'])['I'].append(
    next(b for b in g if b['I'])['I'][0]))
cat_mut('duplicate_U_extended', lambda g: next(b for b in g if len(b['U']) > b['q'])['U'].append(
    next(b for b in g if len(b['U']) > b['q'])['U'][0]))
cat_mut('reverse_export_order', lambda g: g.reverse())
cat_mut('swap_equal_level_support_order', lambda g: g.__setitem__(slice(0, 2), [g[1], g[0]]))
cat_mut('reverse_shell_order', lambda g: g[0]['U'].reverse())
cat_mut('rank_offset_positive_control', c.mut_rank_offset)
cat_mut('noncanonical_support_positive_control', c.mut_support_not_canonical)

archived_tower = ROOT / 'apres/replay_new/tower.txt'
shutil.copyfile(archived_tower, OUT / 'tower_base_archived.txt')
aud = t.parse(OUT / 'tower_base_archived.txt')
amut = copy.deepcopy(aud)
t.mut_vertical_audit(amut)
dead = copy.deepcopy(aud)
pt, _v, entry = dead[2]['points'][-1]
dead[2]['points'][-1] = (pt, 1, entry)
tower_archived = dict(archive=str(archived_tower), archive_sha256=digest(archived_tower),
                      baseline=t.judge(t.AUDIT3, aud, 2),
                      wrong_empty_vertical=t.judge(t.AUDIT3, amut, 2),
                      point_attached_to_dead_descendant=t.judge(t.AUDIT3, dead, 2))

# A new short fixture, independent from any campaign. Preserve its raw dump.
nary_dir = OUT / 'nary'
nary_dir.mkdir(exist_ok=True)
err, nary = t.run_tower(str(EXE), t.TRIANGLE, 3, str(nary_dir))
if err:
    raise RuntimeError(err)
nary_base = t.judge(t.TRIANGLE, nary, 3)
o = nary[2]
parent = next(v for v in range(len(o['nodes']))
              if sum(par == v for par, _lv, _low in o['nodes']) >= 3)
children = [v for v, (par, _lv, _low) in enumerate(o['nodes']) if par == parent]
mutated = copy.deepcopy(nary)
old = mutated[2]
remap = lambda v: v + (v >= parent) if v >= 0 else v
nodes = [(remap(par), lv, low) for par, lv, low in old['nodes']]
_par, lv, low = o['nodes'][parent]
nodes.insert(parent, (parent + 1, lv, low))
for ch in children[:2]:
    _, clv, clo = nodes[remap(ch)]
    nodes[remap(ch)] = (parent, clv, clo)
old['nodes'] = nodes
old['ids'] = list(range(len(nodes)))
old['births'] = {remap(v): W for v, W in old['births'].items()}
old['points'] = [(p, remap(v), e) for p, v, e in old['points']]
mutated[3]['nodes'] = [(par, lv, remap(low)) for par, lv, low in mutated[3]['nodes']]


def dump(path, orders):
    with path.open('w') as f:
        for k, order in sorted(orders.items()):
            f.write('order %d %d %d\n' % (k, len(order['nodes']), len(order['points'])))
            for v, (par, lv, low) in enumerate(order['nodes']):
                witness = ''.join(' ' + ','.join(map(str, p)) for p in order['births'].get(v, ()))
                f.write('node %d %d %d %d %d%s\n' % (v, par, lv.numerator, lv.denominator, low, witness))
            for p, v, e in order['points']:
                f.write('point %d %d %d %d %d\n' % (*p, v, e))


dump(nary_dir / 'tower_binarized_same_level.txt', mutated)
parsed_mutant = t.parse(nary_dir / 'tower_binarized_same_level.txt')
nary_result = dict(P=t.TRIANGLE, K=3, baseline=nary_base, mutated=t.judge(t.TRIANGLE, parsed_mutant, 3),
                   order=2, original_parent=parent, original_arity=len(children),
                   added_intermediate=parent, level=str(lv),
                   original_sha256=digest(nary_dir / 'tower.txt'),
                   mutant_sha256=digest(nary_dir / 'tower_binarized_same_level.txt'))
after = {str(p): digest(p) for p in (CGATE, TGATE, EXE)}
print(json.dumps(dict(before=before, after=after, catalogue=catalogue, tower_archived=tower_archived,
                     nary=nary_result, null_means='production judge accepted dump'), indent=2))
if before != after or catalogue['baseline'] or tower_archived['baseline'] or nary_base:
    sys.exit(1)
