#!/usr/bin/env python3
"""Bounded AST check of the published qualification API; no native code or geometry run."""
import ast
import hashlib
import json
import subprocess
from types import SimpleNamespace

import numpy as np

COMMIT = 'ab1a739d17f801823a66d74609209696152c8705'
REPO = '/workspaces/E-HGP/build/v11-claude-20261003'
SOURCE = 'morsehgp3D_v11/bench/points_hierarchy.py'
EXPECTED = '0da8fce4a4a4140f4e6be39c5bb5cb4ce1461e408019a76d81aafbbf5af58f70'


def main():
    raw = subprocess.check_output(['git', '-C', REPO, 'show', COMMIT + ':' + SOURCE])
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise RuntimeError('published source mismatch')
    names = {'ExportError', 'need', 'qualify', 'qualify_next', 'qualify_general', 'qualified_starts'}
    tree = ast.parse(raw.decode())
    nodes = [node for node in tree.body if isinstance(node, (ast.FunctionDef, ast.ClassDef)) and node.name in names]
    if {node.name for node in nodes} != names:
        raise RuntimeError('AST closure mismatch')
    namespace = {'np': np}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), SOURCE, 'exec'), namespace)
    checks = []
    # A FULL_n with one component born on its enclosing ball, all n incidences on that node.
    # These states isolate the qualification/cardinality contract, not the MEB constructor.
    for n, k, m, expected in ((1, 1, 1, 'active'), (1, 1, 2, 'refusal'),
                              (4, 4, 4, 'active'), (4, 4, 5, 'refusal'), (4, 4, 6, 'refusal')):
        order = SimpleNamespace(n=n, k=k, size=1, rank=np.array([1], dtype=np.int64),
                                parent=np.array([-1], dtype=np.int64),
                                inc_node=np.zeros(n, dtype=np.int64), inc_site=np.arange(n, dtype=np.int64),
                                inc_rank=np.ones(n, dtype=np.int64))
        qual = namespace['qualify'](order, m)
        try:
            owner, rank = namespace['qualified_starts'](order, qual)
        except namespace['ExportError'] as error:
            if expected != 'refusal' or str(error) != 'jamais_qualifie':
                raise RuntimeError('unexpected qualification refusal') from error
            checks.append(dict(n=n, k=k, m=m, qual=qual.tolist(), outcome='refusal', reason=str(error)))
        else:
            if expected != 'active' or owner.tolist() != [0] * n or rank.tolist() != [1] * n:
                raise RuntimeError('unexpected active output')
            checks.append(dict(n=n, k=k, m=m, qual=qual.tolist(), outcome='active'))
    print(json.dumps(dict(status='PASS', source_sha256=EXPECTED, cases=checks,
                          scope='AST scalar/small-array qualification only; no native FULL, fit or GCP'),
                     sort_keys=True, indent=1))


if __name__ == '__main__':
    main()
