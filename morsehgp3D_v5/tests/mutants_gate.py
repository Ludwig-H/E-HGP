#!/usr/bin/env python3
"""Porte du registre des mutants (morsehgp3D_v5/src/core/mutants.hpp).

Trois regles mecaniques :
  1. chaque nom de `kMutants` a AU MOINS un point d'injection MHGP5_MUTANT("nom")
     dans src/ (ou, pour les mutants d'oracle declares comme tels dans
     oracle/ ou tests/) ;
  2. chaque point d'injection rencontre est declare ;
  3. chaque nom declare a AU MOINS une porte CTest attendue en code 4
     (`mhgp5_gate(<nom> 4 <cible> "... --inject=<mutant> ...")` ou une boucle
     `foreach(m ...)` dont le corps injecte `${m}` en code 4) dans CMakeLists.txt.
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
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
gated: set[str] = set(re.findall(r'mhgp5_gate\([A-Za-z0-9_${}-]+ 4 [A-Za-z0-9_]+ "[^"]*--inject=([a-z0-9-]+)', cmake))
for m in re.finditer(r'foreach\(m ([^)]*)\)(.*?)endforeach\(\)', cmake, re.S):
    body = m.group(2)
    if re.search(r'mhgp5_gate\([A-Za-z0-9_${}-]+\s+4\s', body) and '--inject=${m}' in body:
        gated.update(m.group(1).split())
errors = []
for name in sorted(declared):
    if name not in sites:
        errors.append(f"mutant declare sans point d'injection : {name}")
    if name not in gated:
        errors.append(f"mutant declare sans porte CTest en code 4 : {name}")
for name in sorted(sites):
    if name not in declared:
        errors.append(f"point d'injection non declare : {name} ({', '.join(sites[name])})")
for e in errors:
    print("KO :", e)
print(f"registre : {len(declared)} mutants declares, {len(sites)} noms injectes, {len(gated & declared)} avec porte en code 4")
sys.exit(3 if errors else 0)
