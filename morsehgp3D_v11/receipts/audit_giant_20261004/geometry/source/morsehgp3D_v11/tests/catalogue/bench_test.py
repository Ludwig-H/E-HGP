#!/usr/bin/env python3
"""Porte du banc binaire : petits attendus propres, IDs, entree entiere et refus sans sortie.

Usage : python3 [-O] bench_test.py <mhgp11_catalogue_bench>. Execution native sur G4 seulement.
"""
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'bench'))
import catalogue_semantic
import catalogue_parallel


CHECKS = 0


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def strict_json(line):
    def pairs(items):
        answer = {}
        for key, value in items:
            require(key not in answer, 'JSON duplicate key')
            answer[key] = value
        return answer

    def constant(_):
        raise ValueError('JSON nonfinite constant')

    return json.loads(line, object_pairs_hook=pairs, parse_constant=constant)


def decode(data):
    require(data[:10] == b'MHGP11CAT1', 'canonical format signature')
    cursor = 10

    def word():
        nonlocal cursor
        require(cursor + 8 <= len(data), 'canonical truncated word')
        result = struct.unpack_from('<Q', data, cursor)[0]
        cursor += 8
        return result

    def integer():
        negative, size = word(), word()
        require(negative in (0, 1) and 1 <= size <= 64, 'canonical integer header')
        value = sum(word() << (64 * index) for index in range(size))
        return -value if negative else value

    bits, kmax, count = word(), word(), word()
    require(bits in (18, 21, 24) and kmax == 2 and count == 3, 'canonical complete input header')
    sites = []
    for _ in range(count):
        point = (word(), word(), word())
        weight = word()
        require(weight == 1, 'canonical unit weights')
        sites.append((point, word()))
    require(word() == 3, 'canonical three exact levels')
    levels = [Fraction(integer(), integer()) for _ in range(3)]
    require(levels == [Fraction(0), Fraction(1), Fraction(4)], 'canonical rational levels')
    require(word() == 3, 'canonical all three balls')
    balls = [tuple(word() for _ in range(8)) for _ in range(3)]
    none = 2**32 - 1
    require(balls == [(2, 0, 2, 1, 0, 1, none, none), (2, 0, 2, 1, 1, 2, none, none),
                      (2, 1, 2, 2, 0, 2, none, none)], 'canonical supports/populations/ranks')
    require([word() for _ in range(4)] == [0, 2, 4, 7], 'canonical population offsets')
    require([word() for _ in range(7)] == [0, 1, 1, 2, 1, 0, 2], 'canonical complete interior then shell')
    require(cursor == len(data), 'canonical unexpected trailing bytes')
    return bits, sites


def main():
    require(len(sys.argv) == 2, 'one native executable argument required')
    executable = Path(sys.argv[1]).resolve()
    points = [(0, 0, 0), (2, 0, 0), (4, 0, 0)]
    ids = [4294967294, 17, 4000000000]
    good_points = b''.join(struct.pack('<III', *point) for point in points)
    good_ids = b''.join(struct.pack('<I', value) for value in ids)
    calls = 0
    with tempfile.TemporaryDirectory(prefix='mhgp11_catalogue_bench_test_') as temporary:
        root = Path(temporary)

        def launch(name, xyz=good_points, names=good_ids, budget=16 * 1024**2, kmax=2,
                   ball_limit=2**32 - 1, workers=None):
            nonlocal calls
            source, identities, output = (root / (name + suffix) for suffix in ('.u32le', '.ids', '.bin'))
            if xyz is not None:
                source.write_bytes(xyz)
            identities.write_bytes(names)
            before = {path: path.read_bytes() for path in (source, identities) if path.exists()}
            command = [str(executable), str(source), str(identities), str(output), str(kmax), '32', '256',
                       '0', str(ball_limit), str(budget)]
            if workers is not None:
                command.append(str(workers))
            result = subprocess.run(command, capture_output=True, check=False, timeout=20)
            calls += 1
            require(result.returncode >= 0, name + ': native signal')
            require(result.stderr == b'', name + ': unexpected native stderr')
            events = [strict_json(line) for line in result.stdout.decode('utf-8').splitlines()]
            require(events and events[-1].get('phase') == 'exit', name + ': missing exit verdict')
            for path, payload in before.items():
                require(path.read_bytes() == payload, name + ': input file modified')
            return result.returncode, events, output

        hashes = []
        for repetition in range(2):
            code, events, output = launch('repeat%d' % repetition)
            require(code == 0 and [e.get('phase') for e in events] == ['cloud', 'catalogue', 'exit'],
                    'complete successful phases')
            require(all(e.get('status', 'ok') == 'ok' for e in events), 'successful status')
            require(events[0]['points'] == 3 and events[0]['sites'] == 3, 'all input records retained')
            require(events[1]['balls'] == 3 and events[1]['levels'] == 3 and events[1]['incidences'] == 7,
                    'native catalogue counts')
            require(events[1]['work'] == {'q4_candidates': 0, 'q4_levels': 0}, 'line: no q4 work')
            data = output.read_bytes()
            bits, actual = decode(data)
            require(actual == list(zip(points, ids)), 'original u32 return IDs attached to correct coordinates')
            hashes.append(hashlib.sha256(data).hexdigest())
        require(hashes[0] == hashes[1], 'canonical hashes differ between repeated processes')
        sequential_ledger = events[1]['logical']
        for workers in (1, 8):
            code, parallel_events, output = launch('workers%d' % workers, workers=workers)
            require(code == 0 and parallel_events[1]['workers'] == workers, 'native worker option')
            require(type(parallel_events[1]['pool_ns']) is int and parallel_events[1]['pool_ns'] >= 0,
                    'separate pool construction interval')
            catalogue_parallel.check_timings(parallel_events[1])
            require(parallel_events[1]['logical'] == sequential_ledger, 'parallel geometric work differs')
            require(hashlib.sha256(output.read_bytes()).hexdigest() == hashes[0], 'parallel canonical differs')
        for workers in ('0', '257', '-1', '1x'):
            result = subprocess.run([str(executable), 'absent', 'absent', 'absent', '2', '32', '256', '0',
                                     str(2**32 - 1), str(16 * 1024**2), workers],
                                    capture_output=True, check=False, timeout=20)
            calls += 1
            require(result.returncode == 2 and not result.stdout and not result.stderr,
                    'invalid worker option must refuse before input IO')
        permutation = [2, 0, 1]
        code, events, output = launch('permutation',
                                     b''.join(struct.pack('<III', *points[j]) for j in permutation),
                                     b''.join(struct.pack('<I', ids[j]) for j in permutation))
        require(code == 0 and events[-1]['status'] == 'ok', 'permuted input success')
        require(hashlib.sha256(output.read_bytes()).hexdigest() == hashes[0], 'input permutation changes canonical')

        right = [(0, 0, 0), (4, 0, 0), (0, 4, 0), (0, 0, 4)]
        code, events, output = launch('right_tetra', b''.join(struct.pack('<III', *p) for p in right),
                                     struct.pack('<4I', 7, 2, 8, 19), kmax=5)
        require(code == 0 and events[1]['work'] == {'q4_candidates': 1, 'q4_levels': 0},
                'outside-hull q4 candidate is rejected before level')
        decoded = catalogue_semantic.inspect(output, bits, 5, 4, arity_counts=True)
        require(decoded['qmin_counts']['4'] == 0, 'outside-hull circumsphere published as qmin4')

        cases = [
            ('missing', None, good_ids, {}, 'input_unreadable', 'invalid_input'),
            ('empty', b'', b'', {}, 'input_unreadable', 'invalid_input'),
            ('truncated_xyz', good_points[:-1], good_ids, {}, 'input_unreadable', 'invalid_input'),
            ('extra_xyz_byte', good_points + b'\x00', good_ids, {}, 'input_unreadable', 'invalid_input'),
            ('missing_id', good_points, good_ids[:-4], {}, 'input_unreadable', 'invalid_input'),
            ('extra_id', good_points, good_ids + struct.pack('<I', 2), {}, 'input_unreadable', 'invalid_input'),
            ('duplicate_id', good_points, struct.pack('<III', 17, 17, 18), {}, 'duplicate_point_id', 'invalid_input'),
            ('budget', good_points, good_ids, {'budget': 1}, 'memory_budget', 'resource_exhausted'),
            ('ball_limit', good_points, good_ids, {'ball_limit': 1}, 'index_overflow_u32', 'resource_exhausted'),
            ('k_zero', good_points, good_ids, {'kmax': 0}, 'kmax_out_of_range', 'invalid_input'),
        ]
        # The out-of-domain record is beyond the native reader's 4096-record buffer: never run a large catalogue.
        tail_points = b''.join(struct.pack('<III', i, 0, 0) for i in range(4096)) + struct.pack('<III', 1 << bits, 0, 0)
        tail_ids = b''.join(struct.pack('<I', i) for i in range(4097))
        cases.append(('last_block', tail_points, tail_ids, {}, 'coordinate_out_of_domain', 'invalid_input'))
        for name, xyz, names, options, reason, status in cases:
            code, events, output = launch(name, xyz, names, **options)
            require(code == 2, name + ': refusal must be code2, not a signal or success')
            require(events[-1] == {'phase': 'exit', 'status': status, 'reason': reason}, name + ': exact refusal')
            require(not output.exists(), name + ': refusal published canonical bytes')
    print(json.dumps({'status': 'ok', 'native_calls': calls, 'checks': CHECKS, 'refusals': len(cases),
                      'canonical_sha256': hashes[0], 'coord_bits': bits}, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
