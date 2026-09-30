"""Tiny independent check of three integration witnesses, not the 2444-row oracle."""
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys


def require(ok, message):
    if not ok:
        raise ValueError(message)


def run(path):
    rows = [json.loads(line) for line in Path(path).read_text().splitlines()]
    require(len(rows) == 4, 'four native JSON rows required')
    for row, m in zip(rows[:2], (2**23, 2**31)):
        # |(m,0,0)|² and |(m,1,0)|², each divided by four for the diameter MEB.
        x, y, e = Fraction(m*m, 4), Fraction(m*m+1, 4), m*m//4
        require(int(row['first_numerator']) == m*m and
                int(row['second_numerator']) == m*m+1 and row['denominator'] == 4,
                'geometric numerator mismatch')
        require(int(row['threshold']) == e and row['cmp'] == -1, 'exact q2 ordering mismatch')
        require(x <= e < y and row['first_at_most'] is True and row['second_at_most'] is False,
                'closed cut mismatch')
        require(row['double_first'] == float(x) and row['double_second'] == float(y),
                'reported double quotient differs from Python')
        require(row['double_collision'] is (m == 2**31), 'palier double collision mismatch')
        values = [(y, 0), (x, 2), (Fraction(4*m*m, 16), 3), (Fraction(e), 4), (Fraction(0), 7)]
        require(row['exact_sorted_ids'] == [rid for _, rid in sorted(values)],
                'exact sorted order or rational equality mismatch')
        require(row['closed_upper_bound'] == sum(value <= e for value, _ in values),
                'upper_bound must include exact ties')
        # A< B by exact level; B< I and I< A by support ID only after a refused
        # comparison's default zero was incorrectly treated as equality.
        require(row['invalid_status'] == 'REFUSED_ZERO_DEN' and
                row['naive_relation_cycle'] == [True, True, True], 'refusal cycle not observed')
    bridge = rows[2]
    e = 3 * (2**32-1)**2
    d = 2**200-1
    n = sum(word << (64*i) for i, word in enumerate(bridge['middle_numerator_words_low_to_high']))
    require(int(bridge['threshold']) == e and bridge['threshold_bits'] == e.bit_length() == 66,
            'max geometric KNN threshold mismatch')
    require(int(bridge['denominator']) == d and n == e*d, 'native shift/borrow bridge mismatch')
    require(n+1 < 2**266 and 0 < d < 2**200, 'bridge outside guarded domain')
    require(Fraction(n-1, d) < e == Fraction(n, d) < Fraction(n+1, d) and
            bridge['cmp_below_equal_above'] == [-1, 0, 1], 'native bridge order mismatch')
    require(rows[3] == dict(status='PASS', geometric_pairs=2, threshold_bridges=1,
                           invalid_sort_executed=False, FULL_tested=False, GCP_used=False),
            'native summary mismatch')
    return dict(status='PASS', geometric_pairs=2, threshold_bridges=1,
                refusal_cycle_confirmed=True, native_stdout_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest(),
                scope='q2/66-bit threshold and future-adapter contracts only; no native wide constructors, FULL, timing or campaign')


if __name__ == '__main__':
    print(json.dumps(run(sys.argv[1]), sort_keys=True))
