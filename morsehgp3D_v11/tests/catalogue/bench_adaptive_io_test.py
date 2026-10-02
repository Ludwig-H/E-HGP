#!/usr/bin/env python3
"""G4-only native bench protocol: all masks, requested diagnostics and exact small canonical outputs."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from bench_semantic_test import fixture, IDS, need, semantic, tetra
import catalogue_diagnostics as diagnostics


def main():
    need(len(sys.argv) == 3, 'executable and declared profile required')
    executable, bits = Path(sys.argv[1]).resolve(), int(sys.argv[2])
    need(bits in (18, 21, 24), 'known coordinate profile')
    calls, refused = 0, 0
    with tempfile.TemporaryDirectory(prefix='mhgp11_catalogue_adaptive_io_') as folder:
        root = Path(folder)
        xyz, ids, output = (root/name for name in ('xyz', 'ids', 'canonical'))

        def inputs(points, identities):
            xyz.write_bytes(b''.join(struct.pack('<III', *p) for p in points))
            ids.write_bytes(b''.join(struct.pack('<I', v) for v in identities))
            return xyz.read_bytes(), ids.read_bytes()

        def call(k, leaf, suffix):
            nonlocal calls
            calls += 1
            answer = subprocess.run([str(executable), str(xyz), str(ids), str(output), str(k), str(leaf), '256',
                                     '0', str(2**32-1), str(16*1024**2), *suffix], capture_output=True,
                                    check=False, timeout=15)
            need(answer.stderr == b'', 'unexpected native stderr')
            return answer

        original = inputs(tetra(7), IDS)
        canonical = None
        expected = semantic.decode(fixture(bits)[0], bits, 5, 4)
        for mode in range(8):
            for requested in (False, True):
                answer = call(5, 16, ['4', str(mode), str(int(requested))])
                need(answer.returncode == 0, 'valid mask/diagnostic request refused')
                events = [json.loads(line) for line in answer.stdout.decode().splitlines()]
                need([e['phase'] for e in events] == ['cloud', 'catalogue', 'exit'] and
                     events[-1]['status'] == 'ok', 'closed native success')
                need(diagnostics.check_request(dict(optimizations=mode, diagnostics=requested), events[1]) is requested,
                     'native diagnostic request changed')
                if requested:
                    diagnostics.check(events[1], 4, bits)
                need(semantic.inspect(output, bits, 5, 4) == expected, 'analytic tetrahedron changed')
                raw = output.read_bytes()
                canonical = raw if canonical is None else canonical
                need(raw == canonical and original == (xyz.read_bytes(), ids.read_bytes()), 'bytes or inputs changed')
                output.unlink()
        # More than one ready node, distinct scheduling and exact contact-rich line catalogue.
        original = inputs([(2*i, 0, 0) for i in range(9)], range(9))
        canonical, stable = None, None
        for workers in (1, 4):
            answer = call(1, 4, [str(workers), '7', '1'])
            need(answer.returncode == 0, 'adaptive split fixture refused')
            event = json.loads(answer.stdout.decode().splitlines()[1])
            diagnostics.check(event, 9, bits)
            need(event['timings']['tasks'] > 1, 'adaptive split not exercised')
            shape = diagnostics.stable(event['diagnostics'])
            stable = shape if stable is None else stable
            need(stable == shape, 'deterministic plan changed across workers')
            decoded = semantic.inspect(output, bits, 1, 9)
            need(decoded['balls'] == 8 and decoded['incidences'] == 16 and decoded['levels'] == 2,
                 'line catalogue is not the eight adjacent equal-radius edges')
            raw = output.read_bytes(); canonical = raw if canonical is None else canonical
            need(canonical == raw and original == (xyz.read_bytes(), ids.read_bytes()), 'line bytes or inputs changed')
            output.unlink()
        for suffix in (['4', '8'], ['4', '-1'], ['4', 'x'], ['4', '7', '2'], ['4', '7', '-1'],
                       ['4', '7', 'x'], ['0', '7', '1'], ['257', '7', '1'], ['4', '7', '1', 'extra']):
            answer = call(1, 4, suffix)
            need(answer.returncode == 2 and answer.stdout == b'' and not output.exists(), 'malformed CLI accepted')
            refused += 1
    need(calls == 27 and refused == 9, 'native call inventory')
    print(json.dumps(dict(coord_bits=bits, calls=calls, refusals=refused,
                          last_sha256=hashlib.sha256(canonical).hexdigest()), sort_keys=True))
    print('catalogue_adaptive_io_verdict conforme attempts27 refusals9')


if __name__ == '__main__':
    main()
