"""Replay archived dump mutations against unchanged, snapshotted v10 judges.

No compiler, executable, production source or external account is needed.
An optional output directory keeps the small catalogue mutant dumps.
"""
import copy
import importlib.util
import json
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True
CAPTURE = Path(__file__).resolve().parents[1]
SOURCES = CAPTURE / 'sources/morsehgp3D_v10/tests/oracle'
EVIDENCE = CAPTURE / 'evidence/normal'


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def export_catalogue(path, records):
    with path.open('w') as f:
        for b in records:
            level = b['level']
            f.write('%d %d %d %d %d %d %d' %
                    (b['rank'], b['q'], b['p'], b['u'], b['flags'], level.numerator, level.denominator))
            for field in ('sup', 'I', 'U'):
                f.write(' | ' + ' '.join(','.join(map(str, p)) for p in b[field]))
            f.write('\n')


def replay(out):
    c = load('captured_catalogue_gate', SOURCES / 'test_catalogue_oracle.py')
    t = load('captured_tower_gate', SOURCES / 'test_tower_oracle.py')
    sites, weights = c.fixture('square')
    ref = c.Ref(sites)
    got = c.parse_dump(EVIDENCE / 'catalogue_base_archived.txt')
    js = dict(balls=len(got), levels=len({b['rank'] for b in got}))
    results = {'catalogue': {'baseline': c.judge(ref, weights, 3, got, js), 'mutants': {}}}

    def apply(name, mutate):
        g = copy.deepcopy(got)
        mutate(g)
        dest = out / (name + '.txt')
        export_catalogue(dest, g)
        results['catalogue']['mutants'][name] = c.judge(ref, weights, 3, c.parse_dump(dest), js)

    apply('duplicate_I', lambda g: next(b for b in g if b['I'])['I'].append(
        next(b for b in g if b['I'])['I'][0]))
    apply('duplicate_U_extended', lambda g: next(b for b in g if len(b['U']) > b['q'])['U'].append(
        next(b for b in g if len(b['U']) > b['q'])['U'][0]))
    apply('reverse_export_order', lambda g: g.reverse())
    apply('swap_equal_level_support_order', lambda g: g.__setitem__(slice(0, 2), [g[1], g[0]]))
    apply('reverse_shell_order', lambda g: g[0]['U'].reverse())
    apply('rank_offset_positive_control', c.mut_rank_offset)
    apply('noncanonical_support_positive_control', c.mut_support_not_canonical)

    aud = t.parse(EVIDENCE / 'tower_base_archived.txt')
    wrong = copy.deepcopy(aud)
    t.mut_vertical_audit(wrong)
    dead = copy.deepcopy(aud)
    pt, _v, e = dead[2]['points'][-1]
    dead[2]['points'][-1] = (pt, 1, e)
    results['tower_archived'] = dict(baseline=t.judge(t.AUDIT3, aud, 2),
                                    wrong_empty_vertical=t.judge(t.AUDIT3, wrong, 2),
                                    point_attached_to_dead_descendant=t.judge(t.AUDIT3, dead, 2))
    base = t.parse(EVIDENCE / 'nary/tower.txt')
    nary = t.parse(EVIDENCE / 'nary/tower_binarized_same_level.txt')
    results['nary'] = dict(baseline=t.judge(t.TRIANGLE, base, 3),
                           binarized_same_level=t.judge(t.TRIANGLE, nary, 3))
    results['null_means'] = 'unchanged snapshotted judge accepts dump'
    print(json.dumps(results, indent=2))
    failed_baseline = results['catalogue']['baseline'] or results['tower_archived']['baseline'] or results['nary']['baseline']
    killed_controls = all(results['catalogue']['mutants'][name] is not None for name in
                          ('rank_offset_positive_control', 'noncanonical_support_positive_control'))
    killed_controls &= results['tower_archived']['wrong_empty_vertical'] is not None
    return int(bool(failed_baseline) or not killed_controls)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        output = Path(sys.argv[1])
        output.mkdir(exist_ok=False)
        sys.exit(replay(output))
    with tempfile.TemporaryDirectory(prefix='v10_oracle_snapshot_replay_') as tmp:
        sys.exit(replay(Path(tmp)))
