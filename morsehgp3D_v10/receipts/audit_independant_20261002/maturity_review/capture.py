#!/usr/bin/env python3
"""Capture exclusive des sources lues : aucune mutation hors de ce nouveau reçu."""
from pathlib import Path
import hashlib
import json


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    own = Path(__file__).resolve().parent
    v10 = own.parents[2]
    checkout = v10.parent
    build = checkout.parents[1]/"build"/"v10-tour-vers-points"/"regles2"/"existence_mure"
    sources = {"em_geo.py": build/"code/em_geo.py", "CONCEPTION.md": build/"CONCEPTION.md",
               "REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md": v10/"audits/REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md",
               "QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md": v10/"audits/audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md"}
    before = {name: {"path":str(path),"sha256":sha(path)} for name,path in sources.items()}
    (own/"sources").mkdir(exist_ok=False)
    for name,path in sources.items():
        if name == "QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md":
            lines = path.read_text().splitlines()
            content = "\n".join(f"{i+1}: {line}" for i,line in enumerate(lines[:118]))+"\n"
            (own/"sources"/"Q13_15_lignes_1_118.txt").write_text(content)
        else:
            with (own/"sources"/name).open("xb") as out:
                out.write(path.read_bytes())
    excerpt = []
    for name,lo,hi in (("em_geo.py",1,46),("CONCEPTION.md",164,207)):
        lines = sources[name].read_text().splitlines()
        excerpt.append(f"SOURCE {name} SHA256 {before[name]['sha256']}")
        excerpt.extend(f"{i+1}: {line}" for i,line in enumerate(lines) if lo <= i+1 <= hi)
    (own/"sources"/"contexte_lignes.txt").write_text("\n".join(excerpt)+"\n")
    after = {name:sha(path) for name,path in sources.items()}
    if any(before[name]["sha256"] != after[name] for name in sources):
        raise ValueError("source modifiée pendant capture")
    data = {"before":before,"after_sha256":after,"closure":"before=after","native_engine_calls":0,
            "note":"sources privées exploratoires ; définition géométrique non implémentée comme règle complète"}
    with (own/"SOURCE_LEDGER.json").open("x") as out:
        json.dump(data,out,ensure_ascii=False,sort_keys=True,indent=2)
        out.write("\n")
    print(json.dumps({"sources":len(sources),"closure":"before=after","native_engine_calls":0}))


if __name__ == "__main__":
    main()
