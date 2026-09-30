"""Petite fixture géométrique, sans génération de graines dev/test : tête et --allow-single.

Usage : python3 evidence_head_semantics.py <release build>
"""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

import numpy as np
from sklearn.cluster import HDBSCAN


def main():
    build = Path(sys.argv[1])
    points = np.array([[i, 0, 0] for i in (*range(8), 50, 100)], dtype='<u4')
    reference = HDBSCAN(min_samples=1, min_cluster_size=3, allow_single_cluster=True,
                        algorithm='kd_tree', copy=True).fit(points.astype(np.float64)).labels_.tolist()
    with tempfile.TemporaryDirectory(prefix='mhgp10-audit-head-semantics-') as temp:
        temp = Path(temp)
        src = temp / 'in.u32le'
        points.tofile(src)
        for binary, extra in [('mhgp10_cluster', []), ('mhgp10_mreach_cluster', ['--source=mreach'])]:
            out = temp / (binary + '.i32le')
            cmd = [str(build / binary), str(src), str(out), '--k=1', '--mcs=3', '--allow-single',
                   '--threads=1'] + extra
            run = subprocess.run(cmd, capture_output=True, text=True)
            got = np.fromfile(out, dtype='<i4').tolist() if out.exists() else None
            print(json.dumps(dict(probe='allow_single', binary=binary, exit_code=run.returncode,
                                  labels=got, sklearn_labels=reference, stdout=run.stdout.strip()), sort_keys=True))
        for z in ('nan', '0', '-1', 'inf'):
            out = temp / ('z_' + z + '.i32le')
            run = subprocess.run([str(build / 'mhgp10_cluster'), str(src), str(out), '--k=1', '--mcs=3',
                                  '--threads=1', '--z=' + z], capture_output=True, text=True)
            print(json.dumps(dict(probe='scale_domain', z=z, exit_code=run.returncode,
                                  output_written=out.exists(), stdout=run.stdout.strip()), sort_keys=True))


if __name__ == '__main__':
    main()
