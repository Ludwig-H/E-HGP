"""Classification par Gram/Fraction ; ni test d'angles ni cross du produit.

La positivite des trois poids du centre affine juge le triangle strict.
Le transport conserve code, signal, sorties et delai via le helper q4 deja teste.
"""
import contextlib
import io
import itertools as it
import json
import subprocess
import sys
from fractions import Fraction as F
from unittest.mock import patch

import q4_presentation_oracle as process
from fraction_oracle import dot, require, sub


def truth(points, bits):
    if any(not 0 <= v < 1 << bits for p in points for v in p):
        return 'refused coordinate_out_of_domain'
    u, v = (sub(p, points[0]) for p in points[1:])
    uu, uv, vv = dot(u, u), dot(u, v), dot(v, v)
    gram = uu * vv - uv * uv
    if gram == 0:
        return 'ok degenerate 0 0'
    # Resolution exacte de Gram * (alpha,beta) = (uu/2,vv/2).
    alpha = F(uu * vv - uv * vv, 2 * gram)
    beta = F(uu * vv - uv * uu, 2 * gram)
    strict = min(alpha, beta, 1 - alpha - beta) > 0
    return 'ok strict 1 1' if strict else 'ok non_strict 1 0'


def cases(bits):
    m = (1 << bits) - 1
    zero = (0, 0, 0)
    bases = ((zero, zero, (4, 0, 0)), (zero, (2, 0, 0), (4, 0, 0)),
             (zero, (4, 0, 0), (2, 3, 0)), (zero, (4, 0, 0), (0, 3, 0)),
             (zero, (4, 0, 0), (1, 1, 0)), (zero, (2, 2, 2), (4, 4, 4)),
             (zero, (4, 4, 0), (4, 0, 4)), (zero, (4, 0, 0), (1, 2, 0)))
    rows = []
    for base in bases:
        for scale in (1, m // 4):
            for shift in (0, m - 4 * scale):
                for order in it.permutations(base):
                    rows.append(tuple(tuple(shift + scale * v for v in p) for p in order))
    extremes = (((m, m, m), (m-1, m, m), (m, m-1, m)),
                (zero, (m, m, 0), (m, 0, m)), (zero, (m, m, m), (m-1, m-1, m-1)),
                (zero, (m, m-1, m), (m-1, m-2, m-1)),
                ((0, m, m), (m, 0, m), (m, m, 0)))
    for base in extremes:
        rows.extend(it.permutations(base))
    rows.extend(it.combinations(tuple(it.product((0, m), repeat=3)), 3))
    for axis in range(3):
        for outside in (-1, m+1):
            p = [0, 0, 0]
            p[axis] = outside
            rows.append((tuple(p), (1, 2, 0), (2, 0, 1)))
    return rows


def judge(points, bits, line):
    expected = truth(points, bits)
    actual = line.split()
    require(len(actual) == len(expected.split()), 'nombre de champs triangle')
    require(actual == expected.split(), 'classe, degenerescence ou positivite triangle')
    return 2


def encode(rows):
    return ''.join(' '.join(str(v) for p in row for v in p) + '\n' for row in rows)


def check_output(executable, child, rows, bits):
    index = None
    try:
        lines = child.stdout.splitlines()
        require(lines and lines.pop(0) == f'bits {bits}', 'batch header')
        require(len(lines) == len(rows), 'request count')
        checks = 0
        for index, (row, line) in enumerate(zip(rows, lines)):
            checks += judge(row, bits, line)
        return checks
    except ValueError as error:
        detail = dict(error=str(error), request_index=index, request=rows[index] if index is not None else None)
        process.diagnostic(executable, 'judge', child.returncode, child.stdout, child.stderr,
                           reason=json.dumps(detail))
        raise


def run(executable):
    header = process.run_child(executable, '', 15, 'header')
    try:
        words = header.stdout.split()
        require(len(words) == 2 and words[0] == 'bits', 'header')
        bits = int(words[1])
        require(bits in (18, 21, 24), 'profile')
    except ValueError as error:
        process.diagnostic(executable, 'header', header.returncode, header.stdout, header.stderr, reason=error)
        raise
    rows = cases(bits)
    child = process.run_child(executable, encode(rows), 45, 'native')
    checks = check_output(executable, child, rows, bits)
    for malformed in ('bad\n', '1 2\n'):
        process.run_child(executable, malformed, 15, 'malformed', expected=2)
    print(f'triangle_kind_fraction_verdict conforme bits{bits} requests{len(rows)} checks{checks} malformed2')


def process_checks():
    checks = scenarios = 0
    rows = cases(21)[:2]
    good = 'bits 21\n' + ''.join(truth(row, 21)+'\n' for row in rows)
    header = subprocess.CompletedProcess(['/fake/triangle'], 0, b'bits 21\n', b'')
    failures = ((3, b'bits 21\n', b'native failure'), (-11, b'', b'\xff'),
                (0, good.encode(), b'unexpected stderr'), (0, b'bits 21\nok strict 1 1\n', b''))
    for code, stdout, stderr in failures:
        child = subprocess.CompletedProcess(['/fake/triangle'], code, stdout, stderr)
        stream = io.StringIO()
        with patch.object(sys.modules[__name__], 'cases', return_value=rows), \
             patch.object(process.subprocess, 'run', side_effect=[header, child]) as call, \
             contextlib.redirect_stderr(stream):
            try:
                run('/fake/triangle')
            except ValueError:
                pass
            else:
                raise ValueError('echec processus accepte')
        info = json.loads(stream.getvalue())
        require(info['returncode'] == code and info['stdout'] == process.decoded(stdout), 'diagnostic code/stdout')
        require(info['stderr'] == process.decoded(stderr) and call.call_count == 2, 'diagnostic stderr/arret')
        scenarios += 1
        checks += 2
    error = subprocess.TimeoutExpired(['/fake/triangle'], 45, output=b'bits 21\n', stderr=b'partial\xff')
    stream = io.StringIO()
    with patch.object(process.subprocess, 'run', side_effect=error), contextlib.redirect_stderr(stream):
        try:
            process.run_child('/fake/triangle', 'payload', 45, 'native')
        except subprocess.TimeoutExpired as caught:
            require(caught is error, 'delai conserve')
        else:
            raise ValueError('delai accepte')
    info = json.loads(stream.getvalue())
    require(info['returncode'] is None and info['timeout'] == 45 and info['stderr'] == 'partial\\xff', 'delai diagnostic')
    return scenarios+1, checks+2


def selftest():
    count = checks = corruptions = 0
    for bits in (18, 21, 24):
        rows = cases(bits)
        count += len(rows)
        for row in rows:
            checks += judge(row, bits, truth(row, bits))
        kinds = {truth(row, bits) for row in rows}
        require(kinds == {'ok degenerate 0 0', 'ok non_strict 1 0', 'ok strict 1 1',
                          'refused coordinate_out_of_domain'}, 'quatre familles non vides')
        checks += 1
        for expected in sorted(kinds):
            row = next(row for row in rows if truth(row, bits) == expected)
            for bad in sorted(kinds - {expected}) + [expected+' trailing', '', expected.replace(' 0', ' 2', 1)
                                                     if ' 0' in expected else 'ok strict 1 0']:
                try:
                    judge(row, bits, bad)
                except ValueError:
                    corruptions += 1
                else:
                    raise ValueError('corruption triangle acceptee')
    scenarios, extra = process_checks()
    print(f'triangle_kind_model_verdict conforme requests{count} checks{checks+extra} '
          f'corruptions{corruptions} processes{scenarios} native0')


if __name__ == '__main__':
    try:
        if len(sys.argv) == 2 and sys.argv[1] == '--selftest':
            selftest()
        elif len(sys.argv) == 2:
            run(sys.argv[1])
        else:
            raise ValueError('usage oracle EXE | --selftest')
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print('REFUS triangle_kind_oracle:', error, file=sys.stderr)
        sys.exit(1)
