"""Campagne appariee : tour a temoins contre projection exacte de l'oracle.

Pour chaque nuage, chaque ordre, on compare TROIS ultrametriques sur les
observations :

* `exact`   : la projection de la tour FULL exacte (`exact/projection.py`),
              obtenue en rejouant `Gamma_k` sur toutes les parties ;
* `segment` : le majorant obtenu en ne reliant que des paires d'observations
              par un segment droit (`engine/point_tower.py`) ;
* `temoins` : le majorant obtenu en ajoutant les centres critiques trouves
              par descente MEB-Lloyd (`engine/witness_tower.py`).

Les deux majorants doivent MAJORER l'exact : une valeur strictement plus
petite est une violation, donc un defaut ou une hypothese fausse, et le
script sort alors avec le code 1. Le script publie la part d'egalite, qui
est la seule mesure honnete de la qualite du moteur : l'egalite n'est PAS
demontree, elle est mesuree.

Codes de sortie : 0 conforme, 1 violation de majoration, 2 refus avant
calcul, 3 plancher de couverture viole.

Usage :

    python3 bench/witness_campaign.py --ns 8,9 --dims 2,3,5,20 --k-max 3 \
        --seeds 17,23,31 --out receipts/witness_campaign.json
"""

import argparse
import json
import random
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.engine.point_tower import PointTower
from ehgp.engine.witness_tower import witness_ultrametric
from ehgp.exact.projection import compare_ultrametrics, projected_ultrametric
from ehgp.exact.tower import FullTower

EXIT_OK = 0
EXIT_VIOLATION = 1
EXIT_REFUSED = 2
EXIT_FLOOR = 3


def make_cloud(family, count, dimension, generator, extent=200):
    """Familles de nuages : uniforme, amas, colineaire, sphere entiere."""
    if family == "uniform":
        return [
            tuple(generator.randint(0, extent) for _ in range(dimension))
            for _ in range(count)
        ]
    if family == "clusters":
        centres = [
            tuple(generator.randint(0, extent) for _ in range(dimension)) for _ in range(2)
        ]
        cloud = []
        for index in range(count):
            centre = centres[index % 2]
            cloud.append(
                tuple(
                    coordinate + generator.randint(-extent // 20, extent // 20)
                    for coordinate in centre
                )
            )
        return cloud
    if family == "collinear":
        direction = tuple(generator.randint(1, 5) for _ in range(dimension))
        return [
            tuple(step * component for component in direction) for step in range(count)
        ]
    if family == "grid":
        cloud = []
        step = max(1, extent // max(2, count))
        for index in range(count):
            cloud.append(
                tuple(
                    ((index >> axis) & 1) * step * (axis + 1) for axis in range(dimension)
                )
            )
        return cloud
    raise ValueError("famille inconnue : " + str(family))


def run_case(cloud, k_max, triples=False):
    """Compare les trois ultrametriques pour un nuage et tous les ordres."""
    started = time.time()
    tower = FullTower(cloud, k_max)
    oracle_seconds = time.time() - started
    started = time.time()
    engine = PointTower(cloud, k_max)
    segment_seconds = time.time() - started
    rows = []
    for order in range(1, min(k_max, len(cloud)) + 1):
        exact = projected_ultrametric(tower, order)
        segment = compare_ultrametrics(exact, engine.cophenetic(order))
        started = time.time()
        witness, statistics = witness_ultrametric(cloud, order, triples=triples)
        witness_seconds = time.time() - started
        witnessed = compare_ultrametrics(exact, witness)
        rows.append(
            {
                "order": order,
                "segment": {
                    "equal": segment["equal"],
                    "above": segment["strictly_above"],
                    "violations": len(segment["violations"]),
                },
                "witness": {
                    "equal": witnessed["equal"],
                    "above": witnessed["strictly_above"],
                    "violations": len(witnessed["violations"]),
                    "witnesses": statistics.get("witnesses"),
                    "nodes": statistics.get("nodes"),
                },
                "seconds": {
                    "oracle": round(oracle_seconds, 3),
                    "segment": round(segment_seconds, 3),
                    "witness": round(witness_seconds, 3),
                },
                "violation_samples": [
                    [list(pair), str(reference), str(proposal)]
                    for pair, reference, proposal in (
                        segment["violations"][:3] + witnessed["violations"][:3]
                    )
                ],
            }
        )
    return rows


def main(argv=None):
    parser = argparse.ArgumentParser(description="campagne appariee E-HGP")
    parser.add_argument("--ns", type=str, default="8")
    parser.add_argument("--dims", type=str, default="2,3,5,20")
    parser.add_argument("--k-max", type=int, default=3)
    parser.add_argument("--seeds", type=str, default="17")
    parser.add_argument("--families", type=str, default="uniform,clusters")
    parser.add_argument("--min-cases", type=int, default=4)
    parser.add_argument("--min-pairs", type=int, default=100)
    parser.add_argument("--out", type=str, default="")
    parser.add_argument("--triples", action="store_true",
                        help="ajoute les barycentres de triplets aux departs de descente")
    options = parser.parse_args(argv)

    counts = [int(value) for value in options.ns.split(",") if value]
    dimensions = [int(value) for value in options.dims.split(",") if value]
    seeds = [int(value) for value in options.seeds.split(",") if value]
    families = [value for value in options.families.split(",") if value]
    if not counts or not dimensions or not seeds or not families:
        return EXIT_REFUSED

    report = {
        "object": "ehgp.witness_campaign.v1",
        "k_max": options.k_max,
        "triples": bool(options.triples),
        "cases": [],
    }
    total_pairs = 0
    violations = 0
    equal_total = 0
    above_total = 0
    print(
        "{:>9} {:>4} {:>3} {:>4} | {:>5} {:>5} {:>4} | {:>5} {:>5} {:>4} {:>5} | {:>7}".format(
            "famille", "d", "n", "k", "eg.s", "maj.s", "vio", "eg.t", "maj.t", "vio", "tem.", "s"
        )
    )
    for family in families:
        for dimension in dimensions:
            for count in counts:
                for seed in seeds:
                    generator = random.Random(seed * 1000 + dimension * 10 + count)
                    cloud = make_cloud(family, count, dimension, generator)
                    if len(set(cloud)) < len(cloud) and family != "collinear":
                        continue
                    rows = run_case(cloud, options.k_max, triples=options.triples)
                    for row in rows:
                        total_pairs += row["segment"]["equal"] + row["segment"]["above"]
                        equal_total += row["witness"]["equal"]
                        above_total += row["witness"]["above"]
                        violations += row["segment"]["violations"]
                        violations += row["witness"]["violations"]
                        print(
                            "{:>9} {:>4} {:>3} {:>4} | {:>5} {:>5} {:>4} | "
                            "{:>5} {:>5} {:>4} {:>5} | {:>7.1f}".format(
                                family,
                                dimension,
                                count,
                                row["order"],
                                row["segment"]["equal"],
                                row["segment"]["above"],
                                row["segment"]["violations"],
                                row["witness"]["equal"],
                                row["witness"]["above"],
                                row["witness"]["violations"],
                                row["witness"]["witnesses"] or 0,
                                row["seconds"]["witness"],
                            )
                        )
                    report["cases"].append(
                        {
                            "family": family,
                            "dimension": dimension,
                            "count": count,
                            "seed": seed,
                            "cloud": [list(point) for point in cloud],
                            "rows": rows,
                        }
                    )
    report["totals"] = {
        "cases": len(report["cases"]),
        "pairs": total_pairs,
        "witness_equal": equal_total,
        "witness_above": above_total,
        "violations": violations,
    }
    print(
        "total : {} cas, {} paires, temoins egaux {} majores {} violations {}".format(
            len(report["cases"]), total_pairs, equal_total, above_total, violations
        )
    )
    if options.out:
        destination = Path(options.out)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(report, indent=1, sort_keys=True), encoding="ascii")
        print("recu ecrit : " + str(destination))
    if violations:
        return EXIT_VIOLATION
    if len(report["cases"]) < options.min_cases or total_pairs < options.min_pairs:
        return EXIT_FLOOR
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
