#!/usr/bin/env python3
"""Native G4-only IO checks at real 21/24-bit extremes, against an independent exact tetrahedron."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from bench_semantic_test import fixture, IDS, need, semantic, tetra


def main():
    need(len(sys.argv) == 3, 'native executable and independently declared profile required')
    executable, bits = Path(sys.argv[1]).resolve(), int(sys.argv[2])
    need(bits in (18, 21, 24), 'known profile')
    results = []
    with tempfile.TemporaryDirectory(prefix='mhgp11_catalogue_profiles_') as folder:
        root = Path(folder)
        for maximum in (7, 2**21 - 1, 2**24 - 1, 2**bits):
            xyz, identities, output = (root / name for name in ('xyz.u32le', 'ids.u32le', 'canonical.bin'))
            xyz.write_bytes(b''.join(struct.pack('<III', *point) for point in tetra(maximum)))
            identities.write_bytes(struct.pack('<4I', *IDS))
            before = (xyz.read_bytes(), identities.read_bytes())
            argv = [str(executable), str(xyz), str(identities), str(output), '5', '16', '256', '0',
                    str(2**32 - 1), str(16 * 1024**2)]
            answer = subprocess.run(argv, capture_output=True, timeout=20, check=False)
            need(answer.stderr == b'', 'unexpected stderr')
            need(before == (xyz.read_bytes(), identities.read_bytes()), 'input modified')
            events = [json.loads(line) for line in answer.stdout.decode().splitlines()]
            if maximum >= 2**bits:
                need(answer.returncode == 2 and events == [{'phase': 'exit', 'status': 'invalid_input',
                                                          'reason': 'coordinate_out_of_domain'}], 'narrow profile refusal')
                need(not output.exists(), 'refusal published output')
                results.append({'maximum': maximum, 'status': 'refused'})
                continue
            need(answer.returncode == 0 and [e['phase'] for e in events] == ['cloud', 'catalogue', 'exit'] and
                 all(e.get('status', 'ok') == 'ok' for e in events), 'complete native success')
            need(events[1]['coord_bits'] == bits, 'wrong native profile')
            expected, _ = fixture(bits, maximum)
            actual = semantic.inspect(output, bits, 5, 4)
            need(actual == semantic.decode(expected, bits, 5, 4), 'exact tetrahedron mismatch')
            results.append({'maximum': maximum, 'status': 'ok', 'semantic_sha256': actual['sha256'],
                            'canonical_sha256': hashlib.sha256(output.read_bytes()).hexdigest()})
            output.unlink()
    need(len(results) == 4, 'native non-vacuity')
    print(json.dumps({'bits': bits, 'native_calls': 4, 'fixtures': results}, sort_keys=True))
    print('catalogue_profiles_io_verdict conforme fixtures4')


if __name__ == '__main__':
    main()
