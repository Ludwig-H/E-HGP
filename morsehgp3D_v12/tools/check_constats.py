#!/usr/bin/env python3
"""Contrôle structurel du registre d'audit ; ne juge ni les preuves ni le moteur."""
import argparse
import datetime
from pathlib import Path
import re
import sys


def check(path):
    identifiers = set()
    errors = []
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.lstrip().startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if cells[0] == "Identifiant" or all(set(cell) <= set("-: ") for cell in cells):
            continue
        if len(cells) != 10:
            errors.append(f"{number}: dix colonnes requises")
            continue
        key = cells[0].strip("`")
        if not re.fullmatch(r"CST-\d{4}", key) or key in identifiers:
            errors.append(f"{number}: identifiant mal formé ou dupliqué: {key}")
        identifiers.add(key)
        try:
            date = datetime.date.fromisoformat(cells[2])
            if date.isoformat() != cells[2]:
                raise ValueError("date non canonique")
        except ValueError:
            errors.append(f"{number}: date ISO requise")
        if cells[4] not in {"exactitude", "concurrence", "mémoire", "numérique", "outillage", "mesure", "document"}:
            errors.append(f"{number}: classe inconnue")
        if cells[8] not in {"ouvert", "en cours", "clos", "refusé", "en attente de l'utilisateur (D14)"}:
            errors.append(f"{number}: état inconnu")
        if any(not cells[i] or cells[i] == "—" for i in (1, 3, 5, 6)):
            errors.append(f"{number}: constat, rôle, gravité et pin requis")
        # Le pin peut être hérité du préambule du bloc : les premières lignes utilisent des sections.
        # Un texte de clôture n'est pas une certification ; sa vérification appartient à la relecture.
        if cells[8] == "clos" and cells[9] in {"", "—"}:
            errors.append(f"{number}: clôture sans preuve")
    if not identifiers:
        errors.append("registre sans constat")
    return identifiers, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", nargs="?", type=Path,
                        default=Path(__file__).resolve().parents[1] / "audits" / "CONSTATS.md")
    args = parser.parse_args()
    identifiers, errors = check(args.path)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"OK: {len(identifiers)} constats, cohérence structurelle seulement")
    return 0


if __name__ == "__main__":
    sys.exit(main())
