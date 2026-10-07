#!/usr/bin/env python3
"""Fichiers vises par les mutants des outils de campagne du raccord R2 (import des outils copies sous /tmp, python3 -B)."""
import collections, importlib, sys, os
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tools'))
def paths_in(obj, acc):
    if isinstance(obj, str):
        if (obj.startswith(('src/', 'cli/', 'tests/', 'bench/', 'cmake/')) or obj == 'CMakeLists.txt') and '\n' not in obj and len(obj) < 90:
            acc.append(obj)
    elif isinstance(obj, dict):
        for v in obj.values():
            paths_in(v, acc)
    elif isinstance(obj, (list, tuple)):
        for v in obj:
            paths_in(v, acc)
for name in ('mutants_pool_raccord', 'mutants_bancs_raccord', 'mutants_juges_integres', 'mutants_cli_juges', 'float_filter_mutants'):
    try:
        mod = importlib.import_module(name)
    except SystemExit as e:
        print(name, 'SystemExit', e); continue
    except Exception as e:
        print(name, 'ERREUR import', repr(e)[:200]); continue
    M = getattr(mod, 'MUTANTS', None)
    print('==', name, 'MUTANTS =', None if M is None else len(M), type(M).__name__)
    if M is None:
        continue
    cnt = collections.Counter(); sans = 0
    items = M.items() if isinstance(M, dict) else enumerate(M)
    for k, mu in items:
        acc = []
        paths_in(mu, acc)
        if acc:
            cnt[acc[0]] += 1
        else:
            sans += 1
    for f, n in cnt.most_common():
        print(f'   {n:3d}  {f}')
    if sans:
        print(f'   {sans:3d}  (aucun chemin reconnu dans la definition)')
