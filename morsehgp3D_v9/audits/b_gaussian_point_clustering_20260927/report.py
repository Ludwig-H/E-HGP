#!/usr/bin/env python3
"""Small public tables from the closed Gaussian capture; raw clouds stay private."""
import argparse
import csv
import hashlib
import html
import json
from pathlib import Path
import statistics


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, obj):
    Path(path).write_text(json.dumps(obj, sort_keys=True, indent=2, allow_nan=False) + "\n")


def flatten(row):
    flat = {key: row[key] for key in ("case", "n", "communities", "regime", "separation", "seed", "k", "min_cluster_size", "exp_z", "method")}
    flat.update(row["metrics"], dendrogram_purity=row["dendrogram_purity"])
    for key in ("exact_classes", "exact_class_fraction", "cluster_count_error", "cluster_count_absolute_error",
                "matched_macro_precision", "matched_macro_recall", "matched_macro_f1", "matched_micro_f1",
                "eligible_classes", "classes_below_min_cluster_size"):
        flat[key] = row["extra"][key]
    for name in ("raw_recoverability", "raw_size_filtered_recoverability", "condensed_recoverability"):
        value = row.get(name, {})
        flat[name + "_macro_f1"] = value.get("macro_best_f1")
        flat[name + "_exact_classes"] = value.get("exact_classes")
    for key in ("raw_internal_nodes", "atomic_internal_nodes", "condensed_clusters", "condensed_nonroot_clusters",
                "suppressed_small_internal_nodes", "suppressed_continuation_internal_nodes", "point_exits", "points_removed_from_input"):
        flat[key] = row.get("condensation_stats", {}).get(key)
    for key in ("native_chain_ms", "projection_ms", "condensation_and_selection_ms",
                "common_z1_matches_standard", "preserved_tree_z1_matches_standard"):
        flat[key] = row.get(key)
    return flat


def write_csv(path, rows):
    keys = list(rows[0])
    if any(set(row) != set(keys) for row in rows):
        raise ValueError("nonuniform CSV columns")
    with Path(path).open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=keys, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def aggregates(rows):
    keys = ("regime", "communities", "separation", "k", "min_cluster_size", "exp_z", "method")
    measured = ("ari_all", "ari_inliers_noise_singletons", "coverage", "clusters", "dendrogram_purity",
                "matched_macro_f1", "exact_class_fraction", "raw_internal_nodes", "condensed_clusters",
                "raw_recoverability_macro_f1", "raw_size_filtered_recoverability_macro_f1",
                "condensed_recoverability_macro_f1", "classes_below_min_cluster_size")
    groups = {}
    for row in rows:
        groups.setdefault(tuple(row[k] for k in keys), []).append(row)
    result = []
    for key, group in sorted(groups.items()):
        if len({row["seed"] for row in group}) != len(group):
            raise ValueError("repeated seed in aggregation")
        aggregate = dict(zip(keys, key), repetitions=len(group))
        for metric in measured:
            values = [row[metric] for row in group]
            if all(value is None for value in values):
                mean, std = None, None
            elif any(value is None for value in values):
                raise ValueError("incomplete aggregation metric")
            else:
                mean = statistics.mean(values)
                std = statistics.stdev(values) if len(values) > 1 else 0.0
            aggregate[metric + "_mean"], aggregate[metric + "_sd"] = mean, std
        result.append(aggregate)
    return result


def primary_svg(aggregate):
    """Two small data-native SVG heatmaps, no bitmap/image generation dependency."""
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="940" height="335" viewBox="0 0 940 335">',
           '<rect width="940" height="335" fill="white"/>',
           '<g font-family="sans-serif" fill="#17202a">',
           '<text x="20" y="24" font-size="18">Gaussiennes 3D équilibrées — K=5, min_cluster_size=20, expZ=1</text>',
           '<text x="20" y="47" font-size="12">ARI : moyenne ± écart-type de 3 graines ; 1 200 points/scène. Pas un intervalle de confiance.</text>']
    for panel, (method, title) in enumerate((("hgp_first_coverage", "HGP : première couverture"), ("hdbscan_common", "HDBSCAN : EOM commun"))):
        x0 = 65 + panel * 465
        out.append(f'<text x="{x0}" y="77" font-size="16">{html.escape(title)}</text>')
        for col, communities in enumerate((2, 4, 8, 16)):
            out.append(f'<text x="{x0+col*95+43}" y="100" text-anchor="middle" font-size="12">G={communities}</text>')
        for row_index, delta in enumerate((8, 4, 2)):
            y = 111 + row_index * 57
            out.append(f'<text x="{x0-10}" y="{y+30}" text-anchor="end" font-size="12">δ={delta}</text>')
            for col, communities in enumerate((2, 4, 8, 16)):
                cell = next(r for r in aggregate if r["regime"] == "spherical" and r["communities"] == communities
                            and r["separation"] == delta and r["k"] == 5 and r["min_cluster_size"] == 20
                            and r["exp_z"] == 1 and r["method"] == method)
                value, sd = cell["ari_all_mean"], cell["ari_all_sd"]
                green = min(1.0, max(0.0, value))
                color = f'rgb({round(244-170*green)},{round(160+38*green)},{round(150-17*green)})'
                x = x0 + col * 95
                out.append(f'<rect x="{x}" y="{y}" width="91" height="52" rx="3" fill="{color}"/>')
                out.append(f'<text x="{x+45}" y="{y+23}" text-anchor="middle" font-size="15">{value:.3f}</text>')
                out.append(f'<text x="{x+45}" y="{y+42}" text-anchor="middle" font-size="11">± {sd:.3f}</text>')
    out.append('<text x="20" y="314" font-size="12">δ = distance minimale entre moyennes / σ. Valeurs numériques = résultats ; couleurs bornées à [0,1].</text></g></svg>')
    return "\n".join(out) + "\n"


def tables(aggregate):
    def get(regime, g, delta, m, method, z=1, k=5):
        return next(r for r in aggregate if (r["regime"], r["communities"], r["separation"], r["min_cluster_size"], r["method"], r["exp_z"], r["k"])
                    == (regime, g, delta, m, method, z, k))
    text = ["# Tableaux issus de la capture close", "", "Moyenne ± écart-type entre graines ; pas d'intervalle de confiance.", "",
            "## Principal : sphériques, K5, taille20, expZ1", "",
            "| G | δ | ARI HGP | ARI HDBSCAN commun | Clusters trouvés HGP / HDB | Couverture HGP / HDB |",
            "|---:|---:|---:|---:|---:|---:|"]
    for g in (2, 4, 8, 16):
        for delta in (8, 4, 2):
            a, b = [get("spherical", g, delta, 20, method) for method in ("hgp_first_coverage", "hdbscan_common")]
            text.append(f'| {g} | {delta} | {a["ari_all_mean"]:.3f} ± {a["ari_all_sd"]:.3f} | {b["ari_all_mean"]:.3f} ± {b["ari_all_sd"]:.3f} | {a["clusters_mean"]:.1f} / {b["clusters_mean"]:.1f} | {100*a["coverage_mean"]:.1f} / {100*b["coverage_mean"]:.1f} % |')
    text.extend(["", "## Effet du seuil : sphériques, séparation4, K5, expZ1", "",
        "Nœuds = nœuds internes bruts → clusters condensés (racine comprise). Les 1 200 sorties de points sont conservées en plus.", "",
        "| G | Taille minimale | Nœuds HGP | Nœuds HDBSCAN | ARI HGP / HDB | Classes vraies sous le seuil |",
        "|---:|---:|---:|---:|---:|---:|"])
    for g in (2, 4, 8, 16):
        for m in (10, 20, 50, 100):
            a, b = [get("spherical", g, 4, m, method) for method in ("hgp_first_coverage", "hdbscan_common")]
            text.append(f'| {g} | {m} | {a["raw_internal_nodes_mean"]:.1f} → {a["condensed_clusters_mean"]:.1f} | {b["raw_internal_nodes_mean"]:.1f} → {b["condensed_clusters_mean"]:.1f} | {a["ari_all_mean"]:.3f} / {b["ari_all_mean"]:.3f} | {a["classes_below_min_cluster_size_mean"]:.0f} |')
    text.extend(["", "## Stress : G8, K5, taille20, expZ1", "",
        "Deux graines par cellule ; delta en unité isotrope, pas en distance de Mahalanobis.", "",
        "| Régime | δ | ARI HGP | ARI HDBSCAN commun | Couverture HGP / HDB |",
        "|---|---:|---:|---:|---:|"])
    for regime in ("anisotropic", "unbalanced"):
        for delta in (8, 4, 2):
            a, b = [get(regime, 8, delta, 20, method) for method in ("hgp_first_coverage", "hdbscan_common")]
            text.append(f'| {regime} | {delta} | {a["ari_all_mean"]:.3f} ± {a["ari_all_sd"]:.3f} | {b["ari_all_mean"]:.3f} ± {b["ari_all_sd"]:.3f} | {100*a["coverage_mean"]:.1f} / {100*b["coverage_mean"]:.1f} % |')
    return "\n".join(text) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--checks", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt_path = args.run / "receipt.json"
    receipt = json.loads(receipt_path.read_text())
    if receipt["status"] != "completed" or receipt["failures"] or len(receipt["rows"]) != 1920:
        raise ValueError("incomplete experiment")
    if receipt["sources_before"] != receipt["sources_after"]:
        raise ValueError("source closure")
    for path, expected in receipt["sources_before"].items():
        if sha(path) != expected:
            raise ValueError("experiment source changed")
    if sha(receipt["manifest_path"]) != receipt["manifest_sha256"] or sha(receipt["native_binary"]) != receipt["native_binary_sha256"]:
        raise ValueError("input or executable changed")
    for path, expected in receipt["input_hashes"].items():
        if sha(path) != expected:
            raise ValueError("input payload changed")
    keys = [(r["case"], r["k"], r["min_cluster_size"], r["exp_z"], r["method"]) for r in receipt["rows"]]
    cases = {key[0] for key in keys}
    expected = {(c, k, m, z, method) for c in cases for k in (5, 10) for m in (10, 20, 50, 100)
                for z in (1, 2) for method in (("hgp_first_coverage", "hdbscan_common", "hdbscan_standard") if z == 1
                                            else ("hgp_first_coverage", "hdbscan_common"))}
    if len(cases) != 48 or len(set(keys)) != 1920 or set(keys) != expected or len(receipt["commands"]) != 96:
        raise ValueError("incomplete parameter grid")
    checks = json.loads((args.checks / "receipt.json").read_text())
    if checks["status"] != "passed":
        raise ValueError("qualification not passed")
    for command in receipt["commands"]:
        directory = args.run / (command["case"] + "_k" + str(command["k"]))
        if command["returncode"] or sha(directory / "native.json") != command["stdout_sha256"] or sha(directory / "native.stderr") != command["stderr_sha256"]:
            raise ValueError("native capture changed or failed")
    args.output.mkdir(parents=True, exist_ok=False)
    flat = [flatten(row) for row in receipt["rows"]]
    aggregate = aggregates(flat)
    write_csv(args.output / "scores.csv", flat)
    write_csv(args.output / "aggregates.csv", aggregate)
    save(args.output / "rows.json", flat)
    # Keep large per-class/replay payloads private, with the exact receipt hash.
    metadata = {key: value for key, value in receipt.items() if key != "rows"}
    metadata.update(full_private_receipt=str(receipt_path.resolve()), full_private_receipt_sha256=sha(receipt_path),
                    row_count=len(flat), condensed_payloads_scope="private captured explicit arrays, one per method/m/z")
    save(args.output / "receipt.json", metadata)
    save(args.output / "datasets_manifest.json", json.loads(Path(receipt["manifest_path"]).read_text()))
    save(args.output / "checks_receipt.json", checks)
    (args.output / "TABLES.md").write_text(tables(aggregate))
    (args.output / "primary.svg").write_text(primary_svg(aggregate))
    for command in checks["commands"]:
        for suffix in ("stdout", "stderr"):
            path = args.checks / (command["name"] + "." + suffix)
            if sha(path) != command[suffix + "_sha256"]:
                raise ValueError("qualification output changed")
            (args.output / path.name).write_bytes(path.read_bytes())
    print(json.dumps(dict(status="published", rows=len(flat), aggregate_rows=len(aggregate), GCP_used=False), sort_keys=True))


if __name__ == "__main__":
    main()
