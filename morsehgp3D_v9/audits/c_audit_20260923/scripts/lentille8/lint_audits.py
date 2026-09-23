import sys, importlib.util
from pathlib import Path
root=Path(sys.argv[1])
spec=importlib.util.spec_from_file_location("cd", root/"tools/check_docs.py"); m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
m.ROOT=root
files=sorted((root/"morsehgp3D_v9/audits").rglob("*.md"))+[root/"audits/COORDINATION_MORSEHGP3D_V9.md"]
tot=0
for f in files:
    errs=m.validate(f)
    if errs:
        tot+=len(errs); print(f.relative_to(root), len(errs)); [print("   ",e) for e in errs[:3]]
print("files",len(files),"errors",tot)
