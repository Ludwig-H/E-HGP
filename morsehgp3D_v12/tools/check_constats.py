#!/usr/bin/env python3
"""Registre et hygiène du canal actif ; ne juge ni les preuves ni le moteur."""
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


def check_directory(folder):
    """Limites du canal courant, sans modifier ni parcourir les reçus historiques."""
    errors = []
    total = 0
    actors = set()
    for path in sorted(folder.iterdir()):
        if not path.is_file() or path.is_symlink() or path.suffix != ".md":
            errors.append(f"{path.name}: seuls les fichiers Markdown courants sont admis")
            continue
        size = path.stat().st_size
        total += size
        limit = 4096 if path.name == "README.md" else 8192
        if path.name != "CONSTATS.md" and size > limit:
            errors.append(f"{path.name}: note trop longue ({size} octets, limite {limit})")
        match = re.fullmatch(r"AUDIT_(CODEX|CLAUDE)(?:_\d{8})?\.md", path.name)
        if match:
            if match[1] in actors:
                errors.append(f"{path.name}: plusieurs notes vivantes du même auditeur")
            actors.add(match[1])
        try:
            content = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            errors.append(f"{path.name}: UTF-8 requis")
            continue
        for target in re.findall(r"\]\(([^)]+)\)", content):
            target = target.strip().strip("<>")
            if target.startswith("#") or re.match(r"[A-Za-z][A-Za-z0-9+.-]*:", target):
                continue
            destination = target.split("#", 1)[0]
            if destination and not (path.parent / destination).exists():
                errors.append(f"{path.name}: lien local absent: {destination}")
    if total > 64 * 1024:
        errors.append(f"canal trop lourd ({total} octets, limite 65536)")
    for required in ("README.md", "CONSTATS.md"):
        if not (folder / required).is_file():
            errors.append(f"{required}: fichier requis")
    return total, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", nargs="?", type=Path,
                        help="autre registre : contrôle structurel seul")
    parser.add_argument("--hygiene", action="store_true", help="contrôler aussi le dossier d'un autre registre")
    args = parser.parse_args()
    path = args.path or Path(__file__).resolve().parents[1] / "audits" / "CONSTATS.md"
    identifiers, errors = check(path)
    total = None
    if args.path is None or args.hygiene:
        total, hygiene_errors = check_directory(path.parent)
        errors.extend(hygiene_errors)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    suffix = "" if total is None else f", canal {total} octets"
    print(f"OK: {len(identifiers)} constats{suffix}, cohérence structurelle seulement")
    return 0


if __name__ == "__main__":
    sys.exit(main())
