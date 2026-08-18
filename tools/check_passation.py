#!/usr/bin/env python3
"""Porte de fraicheur de la passation v4 (audit `dd0d4a6` § 5).

Le defaut observe le 18 aout : `morsehgp3D_v4/PASSATION.md` decrivait
comme ouverts deux chantiers deja implementes, testes et recus. Une
session suivant ce document aurait refait du travail termine et optimise
le mauvais poste. La liste des chantiers est une autorite de fait pour
la session suivante : elle doit donc etre verifiable, pas seulement
relue.

Deux regles, toutes deux mecaniques :

1. **Aucune reference morte.** Tout fichier `.md` cite dans la passation
   (recu, audit, note) doit exister. Une reference morte est le premier
   signe qu'un document a derive de l'arbre.
2. **Un item OPEN ne peut pas s'appuyer sur un recu deja declare
   execute**, sauf a porter explicitement sa partie residuelle. Un recu
   cite dans la section des resultats acquis est un travail fait ; s'il
   reapparait dans la section des chantiers ouverts, l'item doit dire ce
   qui reste (marqueur `LIVRÉ` ou `OPEN` dans le texte de l'item).

La porte ne juge pas le contenu scientifique : elle interdit seulement
qu'un document se contredise sur ce qui est fait.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PASSATION = ROOT / "morsehgp3D_v4" / "PASSATION.md"
# Repertoires ou un document cite peut vivre.
SEARCH_DIRS = (
    ROOT / "morsehgp3D_v4" / "receipts",
    ROOT / "morsehgp3D_v4" / "audits",
    ROOT / "morsehgp3D_v4" / "docs",
    ROOT / "morsehgp3D_v4",
    ROOT / "docs",
)
DOC_RE = re.compile(r"`([A-Za-z0-9_./-]+\.md)`")
# La porte vise le corpus qui DERIVE : recus, audits, notes et dossiers
# v4. Les documents normatifs de la racine (AGENTS.md, CLAUDE.md) sont
# couverts ailleurs, et un fichier de canal operationnel peut vivre sur
# une autre branche — les citer n'est pas une derive.
TRACKED_PREFIXES = (
    "ADDENDUM_",
    "AUDIT_",
    "CONTRE_AUDIT_",
    "NOTE_CLAUDE_",
    "QUESTION_CLAUDE_",
    "REPONSE_",
    "JOURNAL_",
    "POSTSCRIPT_",
    "COMPLEMENT_",
    "HARMONISATION_",
    "ETAT_COURANT",
    "MATHEMATIQUES",
    "ARCHITECTURE",
    "PLAN_DE_TESTS",
)
SECTION_RE = re.compile(r"^##\s+(\d+)\.")
ITEM_RE = re.compile(r"^\d+\.\s")
RESIDUAL_MARKERS = ("LIVRÉ", "OPEN")


def sections(text: str) -> dict[str, list[str]]:
    """Decoupe le document par section de premier niveau numerotee."""

    out: dict[str, list[str]] = {}
    current = ""
    for line in text.splitlines():
        match = SECTION_RE.match(line)
        if match:
            current = match.group(1)
            out.setdefault(current, [])
        elif current:
            out[current].append(line)
    return out


def cited_docs(lines: list[str]) -> set[str]:
    names: set[str] = set()
    for line in lines:
        for raw in DOC_RE.findall(line):
            name = Path(raw).name
            if name.startswith(TRACKED_PREFIXES):
                names.add(name)
    return names


def exists(name: str) -> bool:
    for directory in SEARCH_DIRS:
        if not directory.is_dir():
            continue
        if any(directory.rglob(name)):
            return True
    return False


def items(lines: list[str]) -> list[list[str]]:
    """Regroupe une section en items numerotes (un item = son bloc)."""

    blocks: list[list[str]] = []
    for line in lines:
        if ITEM_RE.match(line):
            blocks.append([line])
        elif blocks and line.startswith(("   ", "\t")):
            blocks[-1].append(line)
        elif line.strip() == "":
            continue
        else:
            blocks.append([line])
    return blocks


def main() -> int:
    if not PASSATION.is_file():
        print(f"KO : {PASSATION} absent")
        return 1
    text = PASSATION.read_text(encoding="utf-8")
    parts = sections(text)
    errors: list[str] = []

    for name in sorted(cited_docs(text.splitlines())):
        if not exists(name):
            errors.append(f"reference morte : {name}")

    delivered = cited_docs(parts.get("2", []))
    for block in items(parts.get("5", [])):
        body = "\n".join(block)
        for name in cited_docs(block):
            if name in delivered and not any(m in body for m in RESIDUAL_MARKERS):
                errors.append(
                    "chantier ouvert appuye sur un recu deja execute "
                    f"({name}) sans partie residuelle explicite"
                )

    if errors:
        for err in errors:
            print(f"KO : {err}")
        return 1
    print(
        f"PASSATION coherente : {len(cited_docs(text.splitlines()))} documents cites, "
        f"{len(delivered)} recus declares executes"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
