#!/usr/bin/env python3
"""Compile deux copies temporaires, sans modifier le produit et sans chronometre."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import struct


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def replace_once(text, old, new):
    require(text.count(old) == 1, "point d'instrumentation absent ou ambigu")
    return text.replace(old, new)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[4])
    parser.add_argument("--capture", type=Path, help="capture initiale JSON, sinon verification seule")
    args = parser.parse_args()
    root = args.repo_root.resolve()
    here = Path(__file__).resolve().parent
    src = root / "morsehgp3D_v12/src"
    expected = here / "result.json"
    saved = json.loads(expected.read_text()) if expected.exists() else None
    if saved:
        for rel, digest in saved["source_sha256"].items():
            require(sha(root / rel) == digest, "source changee : " + rel)
    outputs, deps = {}, set()
    with tempfile.TemporaryDirectory(prefix="mhgp12-leaf-physical-") as td:
        temp = Path(td)
        for mode in ("reference", "instrumented", "scalar_stop"):
            inc = temp / mode
            target = inc / "catalogue"
            target.mkdir(parents=True)
            common = (src / "catalogue/leaf_common.hpp").read_text()
            census = (src / "catalogue/leaf_census.hpp").read_text()
            if mode != "reference":
                common = replace_once(common, "        if (y == x) continue;",
                                      "        if (y == x) continue;\n        ++audit::counts.dominance;")
                census = replace_once(census, "  ++X.judged;", "  ++X.judged;\n"
                    "  audit::counts.census_lanes += N;\n"
                    "  auto audit_side_mask = W::empty();")
                census = replace_once(census, "        int r = 0;", "        int r = 0;\n"
                    "        ++audit::counts.side_calls;\n        audit_side_mask |= W::bit(s);")
                census = replace_once(census, "    if (s < m) {", "    if (s < m) {\n"
                                      "      ++audit::counts.census_sites;")
                census = replace_once(census, "  if (t < m) {", "  if (t < m) {\n"
                    "    ++audit::counts.rejected;\n"
                    "    audit::counts.side_after_stop += simt::popc(audit_side_mask & W::above(t));")
                if mode == "scalar_stop":
                    census = replace_once(census, "  simt::Lanes<bool, N> is_in, is_on, fault;",
                        "  u32 audit_seen_inside = 0;\n  simt::Lanes<bool, N> is_in, is_on, fault;")
                    census = replace_once(census, "    if (s < m) {",
                        "    if (s < m && audit_seen_inside <= static_cast<u32>(X.K + 1) - q) {")
                    census = replace_once(census, "  }\n  const auto I = simt::ballot<N>(is_in), C = simt::ballot<N>(is_on);",
                        "    if (is_in[s]) ++audit_seen_inside;\n  }\n"
                        "  const auto I = simt::ballot<N>(is_in), C = simt::ballot<N>(is_on);")
            (target / "leaf_common.hpp").write_text(common)
            (target / "leaf_census.hpp").write_text(census)
            exe, dep = temp / (mode + ".bin"), temp / (mode + ".d")
            command = ["g++", "-std=c++20", "-O2", "-DMHGP12_COORD_BITS=21", "-MMD", "-MF", str(dep),
                       "-I", str(inc), "-I", str(src), str(here / "probe.cpp"), "-o", str(exe)]
            subprocess.run(command, check=True, capture_output=True, text=True)
            result = subprocess.run([str(exe)], check=True, capture_output=True, text=True)
            outputs[mode] = [json.loads(line) for line in result.stdout.splitlines()]
            dependency_words = dep.read_text().replace("\\\n", " ").split()[1:]
            for word in dependency_words:
                path = Path(word)
                if path.is_relative_to(src):
                    deps.add(path.relative_to(root).as_posix())
            deps.add("morsehgp3D_v12/src/catalogue/leaf_common.hpp")
            deps.add("morsehgp3D_v12/src/catalogue/leaf_census.hpp")
    baseline, physical = outputs["reference"], outputs["instrumented"]
    require(len(baseline) == len(physical) == 7, "nombre de cas")
    for left, right in zip(baseline, physical):
        require({k: v for k, v in left.items() if k != "physical"} ==
                {k: v for k, v in right.items() if k != "physical"}, "instrumentation altere resultat")
        logical, measured = right["logical"], right["physical"]
        require(measured["dominance"] == 2 * logical[0], "pas deux decisions physiques par paire")
        require(measured["census_sites"] == right["sites"] * logical[2], "census incomplet inattendu")
        require(measured["census_lanes"] == right["width"] * logical[2], "largeur inattendue")
        require(measured["census_sites"] >= logical[3], "compteur logique depasse physique")
    require(physical[0]["physical"]["side_after_stop"] > 0, "temoin causal absent")
    scalar = outputs["scalar_stop"]
    require(len(scalar) == len(baseline), "nombre de cas scalaire")
    for left, right in zip(physical, scalar):
        require({k: v for k, v in left.items() if k != "physical"} ==
                {k: v for k, v in right.items() if k != "physical"}, "arret scalaire altere resultat")
        require(right["physical"]["side_calls"] == left["physical"]["side_calls"] -
                left["physical"]["side_after_stop"], "reduction non causale")
        require(right["physical"]["census_sites"] == right["logical"][3], "arret scalaire non exact")
    # Les comparaisons ci-dessus incluent tous les mots emis, avant de ne conserver que leur SHA-256.
    for cases in outputs.values():
        for case in cases:
            words = case.pop("emission_words")
            case["emission_sha256"] = hashlib.sha256(b"".join(struct.pack("<Q", word) for word in words)).hexdigest()
    document = {
        "pin": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
        "scope": "sept feuilles synthetiques isolees ; aucun temps, LiDAR, GCP ou FULL",
        "compiler": subprocess.check_output(["g++", "--version"], text=True).splitlines()[0],
        "flags": ["-std=c++20", "-O2", "-DMHGP12_COORD_BITS=21"],
        "source_sha256": {rel: sha(root / rel) for rel in sorted(deps)},
        "logical_fields": ["dominance_tests", "prefixes", "judged", "census_tests", "emitted", "incidences",
                           "q4_candidates", "q4_levels", "region_pair_tests", "region_pair_rejects",
                           "region_line_tests", "region_line_rejects", "region_line_evaluations",
                           "region_line_cache_hits", "region_line_fallbacks"],
        "instrumentation_and_scalar_stop_preserve_all_logical_counts_and_emission_words": True,
        "cases": physical,
        "scalar_stop_host_only_experiment": scalar,
    }
    if saved:
        require(document["source_sha256"] == saved["source_sha256"], "sources modifiees pendant execution")
        require(document["cases"] == saved["cases"], "resultats differents de la capture")
        if "scalar_stop_host_only_experiment" in saved:
            require(document["scalar_stop_host_only_experiment"] == saved["scalar_stop_host_only_experiment"],
                    "resultats arret scalaire differents")
    if args.capture:
        args.capture.write_text(json.dumps(document, indent=2) + "\n")
    print(json.dumps({"status": "ok", "cases": 7, "pinned_sources": len(deps),
                      "physical": [{"case": case["case"], **case["physical"]} for case in physical]}))


if __name__ == "__main__":
    main()
