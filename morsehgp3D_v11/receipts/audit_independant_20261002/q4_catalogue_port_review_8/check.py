#!/usr/bin/env python3
"""Lecteur autonome : intégrité des copies et contrat abstrait des compteurs.

Aucun import de code produit, aucun appel natif. Les étapes abstraites sont
des données de contrat, pas une simulation géométrique du catalogue.
"""
from dataclasses import dataclass
from hashlib import sha256
from itertools import combinations
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
N = 0


def require(condition, context):
    global N
    if not condition:
        raise RuntimeError(context)
    N += 1


before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
after = json.loads((HERE / "SOURCE_AFTER.json").read_text())
require(before["commit"] == after["frozen_commit"], "commit figé")
require(len(before["files"]) == len(after["files"]) == 24, "24 sources")
for old, new in zip(before["files"], after["files"]):
    require(old["path"] == new["path"], "même liste source")
    data = (HERE / "source" / old["path"]).read_bytes()
    require(len(data) == old["bytes"], old["path"] + " taille")
    require(sha256(data).hexdigest() == old["sha256"] == new["copy_sha256"]
            == new["git_sha256"] == new["working_sha256"], old["path"] + " SHA")
for item in json.loads((HERE / "BASELINE_PINS.json").read_text())["files"]:
    data = (HERE / "baseline" / item["path"]).read_bytes()
    require(sha256(data).hexdigest() == item["declared_sha256"]
            == item["observed_sha256"], "pin " + item["path"])

# Un support d'arité <4 a un remplissage sentinelle. Il ne peut égaler une
# présentation q4 à quatre indices valides, indépendamment de la géométrie.
none = 2**32 - 1
support_comparisons = 0
for generated in combinations(range(6), 4):
    for arity in (2, 3):
        for smaller in combinations(range(6), arity):
            padded = smaller + (none,) * (4 - arity)
            require(padded != generated, "qmin inférieur ne matérialise pas q4")
            support_comparisons += 1


@dataclass(frozen=True)
class Presentation:
    name: str
    nondegenerate: bool = True
    strictly_inside: bool = True
    owner: bool = True
    census_admitted: bool = True
    canonical_equal: bool = True
    order_admitted: bool = True
    qmin: int = 4
    incidences: int = 4


def counters(presentations, capacity=None):
    """Contrat par événements ; l'échec interdit toute publication."""
    candidate = level = emitted = incidences = 0
    accepted = []
    for p in presentations:
        if not p.nondegenerate:
            continue
        candidate += 1
        if not all((p.strictly_inside, p.owner, p.census_admitted,
                    p.canonical_equal, p.order_admitted)):
            continue
        if p.qmin != 4:
            raise RuntimeError("égalité canonique q4 incompatible avec qmin<4")
        level += 1
        # Collector peut refuser après la matérialisation. Aucun compteur de
        # brouillon n'est alors une propriété d'un catalogue publié.
        if capacity is not None and emitted == capacity:
            return None, (candidate, level, emitted, incidences)
        emitted += 1
        incidences += p.incidences
        accepted.append(p.name)
    return tuple(accepted), (candidate, level, emitted, incidences)


cases = (
    Presentation("dépendant", nondegenerate=False),
    Presentation("hors tétraèdre", strictly_inside=False),
    Presentation("autre propriétaire", owner=False),
    Presentation("trop d'intérieurs", census_admitted=False),
    Presentation("canonique q2", canonical_equal=False, qmin=2),
    Presentation("canonique q3", canonical_equal=False, qmin=3),
    Presentation("autre présentation q4", canonical_equal=False),
    Presentation("hors ordre", order_admitted=False),
    Presentation("q4 admis", incidences=5),
)
first = counters(cases)
second = counters(cases)
require(first == second, "passes abstraites identiques")
require(first == (("q4 admis",), (8, 1, 1, 5)), "compteurs de succès")
require(first[1][1] == len(first[0]), "niveaux q4 == sorties qmin4 sur succès")
require(first[1][0] >= first[1][1], "candidats >= niveaux")
require(tuple(2 * x for x in first[1]) == (16, 2, 2, 10),
        "travail additif de deux passes, publication d'une seule")
failed = counters(cases, capacity=0)
require(failed == (None, (8, 1, 0, 0)), "refus après niveau sans publication")
require(failed[1][1] != failed[1][2], "égalité limitée au succès")

manifest = HERE / "SHA256SUMS"
if manifest.exists():
    for line in manifest.read_text().splitlines():
        digest, name = line.split("  ", 1)
        require(sha256((HERE / name).read_bytes()).hexdigest() == digest,
                "fermeture " + name)

print(json.dumps({"status": "PASS", "sources": 24, "baseline_pins": 3,
                  "support_comparisons": support_comparisons,
                  "abstract_presentations": len(cases),
                  "logical_success_counters": first[1],
                  "refusal_draft_counters": failed[1]}, sort_keys=True))
