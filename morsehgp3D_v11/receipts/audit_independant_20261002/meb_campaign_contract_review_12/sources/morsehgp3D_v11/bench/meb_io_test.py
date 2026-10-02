#!/usr/bin/env python3
"""Small native I/O gate, run only by the qualification matrix; never a timing experiment."""
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

import catalogue_g4 as base
import meb_semantic as sem


def main():
    exe, bits = Path(sys.argv[1]), int(sys.argv[2])
    points = [(0, 0, 0), (6, 6, 0), (6, 0, 6), (0, 6, 6), (2, 2, 2), (3, 0, 0),
              (0, 3, 0), (0, 0, 3), (9, 2, 1), (1, 9, 2), (2, 1, 9), (5, 4, 3)]
    names = [99, 2**32 - 1, 7, 123] + list(range(204, 212))
    calls = 0
    with tempfile.TemporaryDirectory(prefix='mhgp11-meb-io-') as temp:
        root = Path(temp)
        xyz, ids, output = root / 'points', root / 'ids', root / 'proof'

        def write(order):
            xyz.write_bytes(b''.join(struct.pack('<III', *points[i]) for i in order))
            ids.write_bytes(b''.join(struct.pack('<I', names[i]) for i in order))

        def child(budget=sem.BUDGET):
            nonlocal calls
            calls += 1
            output.unlink(missing_ok=True)
            result = subprocess.run([str(exe), str(xyz), str(ids), str(output), str(budget)],
                                    stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    timeout=15, check=False)
            return result, [base.event_json(line) for line in result.stdout.decode().splitlines() if line.strip()]

        digests = []
        for order in (list(range(12)), list(range(12)), list(reversed(range(12)))):
            write(order)
            result, events = child()
            sem.need(result.returncode == 0, 'tiny native success')
            value = sem.inspect(output, bits, 12, events)
            sem.need(value['queries'] == 48 and value['raw_sha256'] == hashlib.sha256(output.read_bytes()).hexdigest(),
                     'native identity/count/hash')
            digests.append(value['sha256'])
        sem.need(len(set(digests)) == 1, 'repeat/permutation changed result')
        for mode in ('truncated_xyz', 'truncated_ids', 'duplicate_id', 'out_of_domain', 'budget'):
            write(range(12))
            if mode == 'truncated_xyz':
                xyz.write_bytes(xyz.read_bytes()[:-1])
            elif mode == 'truncated_ids':
                ids.write_bytes(ids.read_bytes()[:-1])
            elif mode == 'duplicate_id':
                ids.write_bytes(struct.pack('<12I', 1, 1, *range(2, 12)))
            elif mode == 'out_of_domain':
                data = xyz.read_bytes()
                xyz.write_bytes(struct.pack('<I', 2**bits) + data[4:])
            result, _ = child(0 if mode == 'budget' else sem.BUDGET)
            sem.need(result.returncode == 2 and not output.exists(), 'refusal published output: ' + mode)
    sem.need(calls == 8, 'native I/O floor')
    print('meb_io_verdict conforme attempts8 queries144 refusals5')


if __name__ == '__main__':
    main()
