#!/usr/bin/env python3
"""Read frozen AST schedules only. No invocation of bench.main or external tools."""
import ast
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
checks = 0


def need(ok, why):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(why)


def expression(node):
    return compile(ast.Expression(node), '<frozen order expression>', 'eval')


def prepare(name):
    tree = ast.parse((ROOT / name).read_text())
    main = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'main')
    loops = [n for n in ast.walk(main) if isinstance(n, ast.For)
             and isinstance(n.target, ast.Name) and n.target.id == 'variant']
    need(len(loops) == 1, 'one variant timing loop')
    turns = [n for n in ast.walk(main) if isinstance(n, ast.Assign)
             and any(isinstance(t, ast.Name) and t.id == 'turn' for t in n.targets)]
    need(len(turns) == (1 if name == 'current.py' else 0), 'rotation scope')
    return expression(loops[0].iter), expression(turns[0].value) if turns else None


def frozen_order(code, variants, rep):
    env = {'variants': variants, 'rep': rep, 'list': list, 'reversed': reversed, 'len': len}
    loop, turn = code
    if turn is not None:
        env['turn'] = eval(turn, {'__builtins__': {}}, env)
    return list(eval(loop, {'__builtins__': {}}, env))


def suggested_order(variants, rep):
    """Reverse after a complete cycle of rotations; guard the empty case."""
    n = len(variants)
    if not n:
        return []
    shift = rep % n
    turn = variants[shift:] + variants[:shift]
    return turn if (rep // n) % 2 == 0 else turn[::-1]


def position_counts(schedule, variants):
    return {v: [sum(row[j] == v for row in schedule) for j in range(len(variants))]
            for v in variants}


def main():
    records = json.loads((ROOT / 'SOURCE.json').read_text())['records']
    for record in records:
        need(hashlib.sha256((ROOT / record['capture']).read_bytes()).hexdigest()
             == record['sha256'], 'source identity')
    current = prepare('current.py')
    previous = prepare('previous.py')
    pair = ['base', 'new']
    old = [frozen_order(previous, pair, r) for r in range(6)]
    actual = [frozen_order(current, pair, r) for r in range(6)]
    need(old == [pair, pair[::-1]] * 3, 'previous A/B alternates')
    need(actual == [pair] * 6, 'new A/B is fixed base then new')
    failures = []
    for n in range(1, 9):
        variants = ['v%d' % j for j in range(n)]
        original = [frozen_order(current, variants, r) for r in range(2 * n)]
        balanced = [suggested_order(variants, r) for r in range(2 * n)]
        counts = position_counts(original, variants)
        if any(c != [2] * n for c in counts.values()):
            failures.append({'variants': n, 'position_counts': counts})
        for schedule in (original, balanced):
            for row in schedule:
                need(sorted(row) == variants, 'each variant exactly once')
        need(all(c == [2] * n for c in position_counts(balanced, variants).values()),
             'suggestion balances all positions on two complete cycles')
        for a in variants:
            for b in variants:
                if a != b:
                    need(sum(row.index(a) < row.index(b) for row in balanced) == n,
                         'suggestion balances pair precedence')
        for reps in range(1, 4 * n + 2):
            counts = position_counts([suggested_order(variants, r) for r in range(reps)], variants)
            need(all(max(c) - min(c) <= 1 for c in counts.values()),
                 'partial cycles differ by at most one at each position')
    need([f['variants'] for f in failures] == [2, 4, 6, 8], 'even N regression domain')
    need(frozen_order(current, [], 0) == suggested_order([], 0) == [], 'empty schedule')
    print(json.dumps({'status': 'PASS', 'checks': checks,
                      'new_two_variant_order': actual,
                      'previous_two_variant_order': old,
                      'even_variant_failures': failures,
                      'suggested_two_variant_order': [suggested_order(pair, r) for r in range(6)],
                      'scope': 'Frozen scheduling AST only; no timing, build, native execution or GCP.'},
                     sort_keys=True))


if __name__ == '__main__':
    main()
