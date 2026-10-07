#!/usr/bin/env python3
"""Analyse statique (modulefinder) des imports des scripts Python lances par les portes CTest de la v11.

A lancer avec l'interprete nu (venv sans paquets) : un module introuvable hors bibliotheque standard et hors depot
est un module tiers que G4 (Python 3.10 nu) n'aurait pas. Lecture seule ; aucun bytecode ecrit (python3 -B).
Usage : imports_nus.py <CTestTestfile.cmake> <racine v11>
"""
import modulefinder
import os
import re
import sys


def main():
    ctest, root = sys.argv[1], sys.argv[2]
    text = open(ctest).read()
    scripts = set()
    for m in re.finditer(r'add_test\(\[=\[(\w+)\]=\] (.*?)\)\n', text):
        for arg in re.findall(r'"-DARG0=([^"]*\.py)"', m.group(2)):
            if arg.startswith(root):
                scripts.add(arg)
    extra = sorted({os.path.dirname(s) for s in scripts} | {os.path.join(root, d) for d in
                   ('bench', 'reference', 'tests/support', 'tools')})
    path = extra + sys.path
    missing = {}
    for script in sorted(scripts):
        finder = modulefinder.ModuleFinder(path=path)
        try:
            finder.run_script(script)
        except Exception as error:  # un script illisible est rapporte, pas masque
            missing.setdefault('<erreur>', []).append('%s: %s' % (script, error))
            continue
        repo = {'__main__'} | {n for n, mod in finder.modules.items()
                               if getattr(mod, '__file__', None) and str(mod.__file__).startswith(root)}
        for name, importers in finder.badmodules.items():
            top = name.split('.')[0]
            if top in sys.stdlib_module_names:
                continue
            if not set(importers) & repo:
                continue
            missing.setdefault(name, []).append(os.path.relpath(script, root) + ' <- ' + ','.join(sorted(importers)))
    print('scripts', len(scripts))
    print('modules introuvables (hors stdlib)', len(missing))
    for name in sorted(missing):
        print(name)
        for who in missing[name][:8]:
            print('   ', who)
        if len(missing[name]) > 8:
            print('    ... (%d)' % len(missing[name]))


if __name__ == '__main__':
    main()
