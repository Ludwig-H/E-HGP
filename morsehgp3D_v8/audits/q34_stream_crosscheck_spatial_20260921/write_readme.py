#!/usr/bin/env python3
"""Rédige le README du dossier spatial à partir des reçus exhaustifs (quarts deux voies ; moitiés et scène, voie q3).

Lecture seule des reçus ; aucune mesure. Un reçu absent ou partiel est signalé comme tel dans le README (le README
n'est commis qu'avec des reçus `passed`). Aucun assert : tient sous python3 -O.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
QUARTERS = HERE / "Q34_STREAM_CROSSCHECK_SPATIAL.json"
LARGE = HERE / "Q3_STREAM_CROSSCHECK_SPATIAL_LARGE.json"
LABELS = {"quarter_xpos_ypos": "quart x+ y+", "quarter_xpos_yneg": "quart x+ y−", "quarter_xneg_yneg": "quart x− y−",
          "quarter_xneg_ypos": "quart x− y+", "half_xpos": "moitié x+", "half_xneg": "moitié x−", "full": "scène"}


def load(path):
    if not path.exists():
        partial = path.with_suffix(".partial.json")
        if partial.exists():
            return json.loads(partial.read_text()), "partiel"
        return None, "absent"
    receipt = json.loads(path.read_text())
    return receipt, "final" if receipt.get("status", "passed") == "passed" else receipt.get("status")


def fmt(value):
    return f"{value:,}".replace(",", " ") if isinstance(value, int) else str(value)


def quarters_row(row):
    probe, h3, h4 = row["probe"], row["harness_q3"], row["harness_q4"]
    s3, q4, witness = h3["sample"]["q3"], h4["q4"], probe["work"]["witness"]
    residual = probe["front"]["work"]["residual_pair_mass"]
    mode = "oui" if row.get("mode_check") is not None else "—"
    return " | ".join([
        LABELS.get(row.get("piece"), row.get("piece", "préfixe")), fmt(row["n"]), str(row["kmax"]),
        f"{fmt(residual[1])} / {fmt(residual[2])}",
        f"{fmt(witness['rectangle_q3_pairs'])} + {fmt(witness['pair_q3_pairs'])} / {fmt(witness['rectangle_q4_pairs'])} + {fmt(witness['pair_q4_pairs'])}",
        f"{fmt(s3['kept'])} / {fmt(q4['kept'])}", fmt(s3["seeds"]["kept"]),
        f"{fmt(row['probe_q3_records'])} = {fmt(row['harness_q3_records'])}",
        f"{fmt(row['probe_q4_distinct'])} = {fmt(row['harness_q4_distinct'])} ({fmt(row['probe_q4_tetras'])})",
        f"{fmt(s3['lemma']['pairs'])} / {s3['lemma']['violations']}", f"{fmt(q4['lemma']['pairs'])} / {q4['lemma']['violations']}", mode,
        f"{row['probe_seconds']:.0f} / {row['harness_q3_seconds']:.0f} / {row['harness_q4_seconds']:.0f}"])


def large_row(row):
    probe, h3 = row["probe"], row["harness_q3"]
    s3, witness = h3["sample"]["q3"], probe["work"]["witness"]
    residual = probe["front"]["work"]["residual_pair_mass"]
    mode = "oui" if row.get("mode_check") is not None else "—"
    return " | ".join([
        LABELS.get(row.get("piece"), row.get("piece", "préfixe")), fmt(row["n"]), str(row["kmax"]), f"{fmt(residual[1])} / {fmt(residual[2])}",
        f"{fmt(witness['rectangle_q3_pairs'])} + {fmt(witness['pair_q3_pairs'])}", fmt(s3["kept"]), fmt(s3["seeds"]["kept"]),
        f"{fmt(row['probe_q3_records'])} = {fmt(row['harness_q3_records'])}", f"{fmt(row['probe_q4_tetras'])} ({row['probe_q4_off_sphere']} hors sphère)",
        f"{fmt(s3['lemma']['pairs'])} / {s3['lemma']['violations']}", mode, f"{row['probe_seconds']:.0f} / {row['harness_q3_seconds']:.0f}"])


def section(receipt, status, title, header, formatter, expected):
    lines = [f"## {title}", ""]
    if receipt is None:
        lines += [f"Reçu absent ({status}) : campagne non encore rendue.", ""]
        return lines
    rows = receipt["rows"]
    note = "" if status == "final" else f" — **reçu {status}**, {len(rows)} ligne(s) sur {expected}, à ne pas citer"
    lines += [f"Commit sonde **{receipt['pins']['commit'][:8]}**, {len(rows)} ligne(s){note}.", "", header,
              "| " + " | ".join("---" for _ in header.split("|")[1:-1]) + " |"]
    lines += ["| " + formatter(row) + " |" for row in rows]
    lines.append("")
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quarters", type=Path, default=QUARTERS)
    parser.add_argument("--large", type=Path, default=LARGE)
    parser.add_argument("--out", type=Path, default=HERE / "README.md")
    args = parser.parse_args()
    quarters, qs = load(args.quarters)
    large, ls = load(args.large)
    pins = (quarters or large or {}).get("pins", {})
    commit = pins.get("commit", "????????")[:8]
    text = [
        "# Morceaux spatiaux de la scène 0 : flux q3 et q4 de la sonde contre les énumérations exhaustives indépendantes",
        "",
        "Auditeur B, 21 septembre 2026. Découpage spatial préparé indépendamment",
        "([prepare_spatial_b.py](prepare_spatial_b.py) : quantification exacte en Fractions",
        "floor(50x + 32768 + 1/2), dédoublonnage global, coupes à 32 768 ; manifeste",
        "[SPATIAL_PIECES_SCAN0.json](SPATIAL_PIECES_SCAN0.json), 119 142 sites, identique",
        "aux fichiers du constructeur ; morceaux `pieces/` non versionnés, régénérables).",
        f"Sonde du constructeur `mhgp8_wspd_q34_probe` construite à **{commit}** (arbre de",
        "travail détaché, Release), modes `rectangle-pair` / `boxes` / `affine` / atlas q4",
        "`joined` (bloc 64), masque 6, Local28, quatre workers, mode `records`. Harnais de B",
        "[q3_stream_probe_v2.cpp](q3_stream_probe_v2.cpp) et",
        "[q4_stream_probe_v2.cpp](q4_stream_probe_v2.cpp) : mêmes énumérations exhaustives",
        "que les reçus des tranches 31 à 34, mais la décision du citron et la couverture",
        "passent par des descentes d'index gardées (sorties identiques à l'octet aux",
        "harnais V1 sur 1k à 8k : [V2_VALIDATION.json](V2_VALIDATION.json)).",
        "`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,",
        "`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,",
        "`public_status=not_claimed`. GCP non utilisé. Contrôle du **calcul** (même objet,",
        "deux codes), pas une qualification ; coquilles non comparées.",
        "",
        "## Ce qui est comparé",
        "",
        "Par morceau : masses résiduelles par voie et nombre de paires du front ; toutes",
        "les paires résiduelles jugées par le harnais (citron exact) ; rejet de la",
        "recherche de témoins (par rectangle puis par paire) égal aux paires rejetables du",
        "citron, paires développées égales aux paires conservées ; seeds q3 des paires",
        "survivantes ; boules q3 émises en nombre et multiensemble (support, profondeur) ;",
        "ensemble des boules q4 distinctes (clé entière réduite, profondeur) et supports",
        "sur leur sphère ; descentes saturantes (préordre et milieu d'abord) sans",
        "désaccord ; lemme du citron sans violation sur le budget de paires indiqué ;",
        "sur le premier quart, la sonde relancée avec l'atlas `live` émet le même flux",
        "(comptes, empreintes, IDs de coquille). Sur les moitiés et la scène, la voie q4",
        "n'est contrôlée que par la validité des records (supports sur leur sphère) : son",
        "contrôle bilatéral est dans [README_Q4_BILATERAL.md](README_Q4_BILATERAL.md) (K5)",
        "et [README_Q4_BILATERAL_K10.md](README_Q4_BILATERAL_K10.md).",
        "",
    ]
    text += section(quarters, qs, "Quarts à K = 5, deux voies exhaustives",
                    "| morceau | n | K | résiduel q3 / q4 | rejeté rect + paire q3 / q4 | conservées q3 / q4 | seeds q3 | boules q3 sonde = harnais | boules q4 distinctes sonde = harnais (présentations) | lemme q3 paires / viol. | lemme q4 paires / viol. | joined = live | s sonde / q3 / q4 |",
                    quarters_row, 4)
    text += section(large, ls, "Moitiés et scène à K = 5, voie q3 exhaustive",
                    "| morceau | n | K | résiduel q3 / q4 | rejeté rect + paire q3 | conservées q3 | seeds q3 | boules q3 sonde = harnais | records q4 sonde | lemme q3 paires / viol. | joined = live | s sonde / q3 |",
                    large_row, 3)
    text += [
        "Lecture : sur chaque morceau contrôlé, la sonde et l'énumération indépendante",
        "émettent les mêmes boules ; la colonne « rejeté » est la masse de paires retirée",
        "avant toute couverture, et la colonne « présentations » compte les records q4",
        "(plusieurs par boule sur un plateau cosphérique). Un scan, une coupe spatiale :",
        "pas de qualification, pas de FULL, et aucune pente d'échelle (voir le dialogue,",
        "revue du protocole spatial).",
        "",
        "## Rejouer",
        "",
        "```bash",
        "python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/prepare_spatial_b.py",
        f"python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_crosscheck.py read --commit {commit}",
        f"python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_q3_large.py read --commit {commit}",
        f"git worktree add --detach /tmp/wt-{commit} {commit}",
        f"cmake -S /tmp/wt-{commit}/morsehgp3D_v8 -B /tmp/wt-{commit}/build/v8-audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost> && cmake --build /tmp/wt-{commit}/build/v8-audit --parallel",
        f"python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_crosscheck.py run --worktree /tmp/wt-{commit} --scratch /tmp/crosscheck_sp --commit {commit} --output /tmp/Q34_STREAM_CROSSCHECK_SPATIAL.json",
        f"python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_q3_large.py run --worktree /tmp/wt-{commit} --scratch /tmp/crosscheck_sp3 --commit {commit} --output /tmp/Q3_STREAM_CROSSCHECK_SPATIAL_LARGE.json",
        "python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/write_readme.py",
        "```",
        "",
    ]
    args.out.write_text("\n".join(text))
    print(json.dumps(dict(out=str(args.out), quarters=qs, large=ls)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
