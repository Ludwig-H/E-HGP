#!/usr/bin/env python3
"""Capture initiale, exclusive : aucun fichier développeur ou reçu existant modifié."""
from pathlib import Path
import hashlib
import json
import subprocess


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    own = Path(__file__).resolve().parent
    checkout = own.parents[4]
    pdf = checkout/"docs/references/MANUSCRIT_THESE_HAUSEUX.pdf"
    v10 = checkout/"morsehgp3D_v10"
    source = {"thesis.pdf": pdf,
              "REPONSE_CLAUDE_AUDIT_GEANT_20260930.md": v10/"audits/REPONSE_CLAUDE_AUDIT_GEANT_20260930.md",
              "head.hpp": v10/"src/head/head.hpp",
              "head.cpp": v10/"src/head/head.cpp"}
    for name in ("aretes_plus_courtes", "pont_plus_court", "pont_plus_long"):
        for ext in ("json", "u32le"):
            source[f"{name}.{ext}"] = Path("/tmp/deux_triangles")/f"{name}.{ext}"
    before = {k: {"source_path": str(p), "sha256": sha(p)} for k,p in source.items()}
    if before["thesis.pdf"]["sha256"] != "579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef":
        raise ValueError("PDF différent de la source relue")
    (own/"inputs").mkdir(exist_ok=False)
    (own/"sources").mkdir(exist_ok=False)
    for k,p in source.items():
        if k == "thesis.pdf" or k.startswith("pont_plus_long"):
            continue
        target = own/("inputs" if k.endswith(("json", "u32le")) else "sources")/k
        with target.open("xb") as f:
            f.write(p.read_bytes())
    pages = subprocess.run(["pdftotext", "-f", "80", "-l", "85", "-layout", str(pdf), "-"],
                           capture_output=True, check=True).stdout
    with (own/"sources"/"these_imprimees_54_59_pdf_80_85.txt").open("xb") as f:
        f.write(pages)
    commands = []
    for page in (81,82):
        prefix = own/"sources"/f"these_pdf_{page}"
        cmd = ["pdftoppm", "-f", str(page), "-l", str(page), "-singlefile", "-scale-to", "1400", "-png", str(pdf), str(prefix)]
        subprocess.run(cmd, capture_output=True, check=True)
        commands.append(cmd)
    after = {k: sha(p) for k,p in source.items()}
    if any(before[k]["sha256"] != after[k] for k in source):
        raise ValueError("source modifiée pendant la capture")
    receipt = {"sources_before": before, "sources_after_sha256": after,
               "pdf_pages_one_based": [80,81,82,83,84,85], "printed_pages": [54,55,56,57,58,59],
               "page_images_commands": commands, "engine_calls": 0,
               "inherited_native_exports": True,
               "alias_pont_plus_long": before["pont_plus_long.json"]["sha256"] == before["aretes_plus_courtes.json"]["sha256"]}
    with (own/"SOURCE_CAPTURE.json").open("x") as f:
        json.dump(receipt, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    print(json.dumps({"captured_sources": len(source), "source_closure": "before=after", "engine_calls": 0}))


if __name__ == "__main__":
    main()
