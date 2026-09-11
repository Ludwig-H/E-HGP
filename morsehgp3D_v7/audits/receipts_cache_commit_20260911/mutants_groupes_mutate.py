#!/usr/bin/env python3
"""Applique un mutant causal par remplacement textuel UNIQUE sur une copie du header,
jamais sur les sources actives. Les deux textes reprennent les spécifications du
paquet constructeur `receipts/ball_resolver_residence_20260910/combined/mutant.py`
et les diffs du reçu auditeur `lots_groupes_vacuite/`. Sans assert."""
from __future__ import annotations

import pathlib
import sys

SPECS = {
    "nominal": (None, None),
    "growth_grouped": (
        "if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));",
        "if (action.parents.size() != 1) batch.actions.push_back(std::move(action));  // MUTANT groupe : croissance omise",
    ),
    "inert_grouped": (
        "for (size_t b = 0; b < blocks.size(); ++b) {\n      require(anchors[blocks[b].ball]",
        "for (size_t b = 0; b < blocks.size(); ++b) {\n      if (blocks[b].roots.size() == 1 && !blocks[b].contribution && !blocks[b].interior) continue;  // MUTANT groupe : ancre inerte omise\n      require(anchors[blocks[b].ball]",
    ),
}


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[1] not in SPECS:
        print("usage: mutate.py <nominal|growth_grouped|inert_grouped> <header copié>", file=sys.stderr)
        return 2
    old, new = SPECS[sys.argv[1]]
    path = pathlib.Path(sys.argv[2])
    text = path.read_text(encoding="utf-8")
    if old is None:
        print("nominal : header inchangé")
        return 0
    if text.count(old) != 1:
        print(f"site du mutant non unique : {text.count(old)} occurrence(s)", file=sys.stderr)
        return 3
    path.write_text(text.replace(old, new), encoding="utf-8")
    print(f"{sys.argv[1]} : mutant appliqué sur la copie")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
