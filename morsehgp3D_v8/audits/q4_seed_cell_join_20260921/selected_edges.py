#!/usr/bin/env python3
"""Additional fixed LiDAR anchors after the first edge sample emitted no support.

The first campaign is preserved. This one changes only the declared selection,
not the frozen prototype; edges do not depend on its answers. No point matching.
"""
from pathlib import Path
import struct
import sys
from campaign import (BASE, ROOT, ORACLE, append, begin, close, execute,
                      load, require, sha)


def run(folder, binary):
    before, manifest = begin(folder, 'Three additional fixed anchors, ranks4/16/64, scans0/100/200, n8k/16k/32k, K5/10')
    own = Path(__file__).resolve()
    own_sha = sha(own)
    manifest['sources'][str(own.relative_to(ROOT))] = own_sha
    # begin deliberately exposes its source inventory; keep the closure input
    # as the original frozen campaign inventory, and close this extra pin too.
    before = {name:value for name,value in before.items() if name != str(own.relative_to(ROOT))}
    manifest['binaries'] = {str(binary.relative_to(ROOT)):sha(binary)}
    for scan in (0,100,200):
        directory = BASE.parent/'lidar08_20260914/prepared'/f'single_{scan:06d}'
        hashes = {r['n']:r['sha256'] for r in load(directory/'METADATA.json')['samples']}
        small = list(struct.iter_unpack('<HHH', (directory/'n8000.u16le').read_bytes()))
        for anchor in (1000,3000,6000):
            order = sorted((i for i in range(len(small)) if i != anchor),
                           key=lambda i:(ORACLE.distance(small[anchor], small[i]), i))
            for n in (8000,16000,32000):
                path = directory/f'n{n}.u16le'
                require(sha(path) == hashes[n], 'LiDAR hash changed')
                require(list(struct.iter_unpack('<HHH', path.read_bytes()[:48000])) == small, 'Nested prefix changed')
                for rank in (4,16,64):
                    edge = sorted((anchor,order[rank-1]))
                    for k in (5,10):
                        command = [str(binary),str(path),str(k),*map(str,edge),'64']
                        append(folder,manifest,dict(case=f'lidar_{scan:06d}_{n}_anchor{anchor}_rank{rank}',
                               n=n,kmax=k,edge=edge,grain=64,rank_selected_at_n=8000,
                               input_sha256=sha(path),**execute(command)))
            print('PASS anchor',scan,anchor,flush=True)
    require(len(manifest['records']) == 162, 'Unexpected additional matrix size')
    require(sha(own) == own_sha, 'Selection script changed')
    close(folder,manifest,before)


if __name__ == '__main__':
    require(len(sys.argv)==3,'usage: selected_edges.py receipt_folder release_binary')
    run(Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve())
