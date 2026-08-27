#!/usr/bin/env python3
"""Porte du registre des mutants (morsehgp3D_v5/src/core/mutants.hpp).

Deux regles mecaniques :
  1. chaque nom de `kMutants` a AU MOINS un point d'injection MHGP5_MUTANT("nom")
     dans src/ (ou, pour les mutants d'oracle declares comme tels dans
     tests/, dans tests/) ;
  2. chaque point d'injection rencontre dans src/ et tests/ est declare.
Codes : 0 conforme, 3 registre incoherent. Jamais un `assert` (python3 -O).
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
REG = ROOT / "src" / "core" / "mutants.hpp"
text = REG.read_text(encoding="utf-8")
block = text[text.index("kMutants[] = {"):text.index("};", text.index("kMutants[] = {"))]
declared = set(re.findall(r'"([a-z0-9-]+)"', block))
sites: dict[str, list[str]] = {}
for d in ("src", "tests", "oracle", "cli", "bench"):
    for f in sorted((ROOT / d).rglob("*")):
        if f.suffix not in (".hpp", ".cpp", ".cu") or f == REG:
            continue
        for m in re.finditer(r'MHGP5_MUTANT\("([a-z0-9-]+)"\)', f.read_text(encoding="utf-8")):
            sites.setdefault(m.group(1), []).append(str(f.relative_to(ROOT)))
errors = []
for name in sorted(declared):
    if name not in sites:
        errors.append(f"mutant declare sans point d'injection : {name}")
for name in sorted(sites):
    if name not in declared:
        errors.append(f"point d'injection non declare : {name} ({', '.join(sites[name])})")
for e in errors:
    print("KO :", e)
print(f"registre : {len(declared)} mutants declares, {len(sites)} noms injectes")
sys.exit(3 if errors else 0)
