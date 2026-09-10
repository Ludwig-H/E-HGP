#!/usr/bin/env python3
"""Assemble review.json : sha256 des sources lues et des artefacts, commandes,
temps (logs/*.time), comptes des campagnes et des mutants. Aucun assert."""
import hashlib
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SCRATCH = os.path.abspath(os.path.join(HERE, "..", ".."))
OVERLAY = os.path.join(SCRATCH, "overlay_wip_130402")
REPO = "/workspaces/E-HGP"
AUDITS = os.path.join(REPO, "morsehgp3D_v7", "audits")

SOURCES_READ = {
    "overlay": [
        "src/forest/full_ball_tower.hpp", "src/forest/full_coverage_certificate.hpp",
        "src/forest/anchor_meb.hpp", "src/forest/local_plateau.hpp", "src/forest/full_certificate.hpp",
        "src/tree/cloud_index.hpp", "src/core/types.hpp", "src/lanes/keys.hpp", "src/lanes/level.hpp",
        "src/pipeline/expand.hpp", "tests/full_ball_tower_gate.cpp", "tests/full_ball_tower_gate_v2.cpp",
        "oracle/local_plateau_oracle.hpp", "bench/full_ball_tower_probe.cpp",
    ],
    "audits": [
        "receipts_plateaux_full_20260906/README.md", "receipts_plateaux_full_20260906/BALL_ANCHORS.md",
        "receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md", "NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md",
        "receipts_plateaux_full_20260906/plateau_model.py", "receipts_plateaux_full_20260906/ball_anchor_model.py",
        "meb_rational_oracle_20260905.py",
    ],
}
OWN_SOURCES = ["tower_bridge.cpp", "tower_corpus.py", "silent_mutant_analysis.py", "redundant_merge_check.py",
               "shell_distribution.py", "make_review.py"]
ARTIFACTS = ["bin/tower_bridge", "corpus_normal.json", "corpus_optimized.json", "corpus_ext_normal.json",
             "corpus_ext_optimized.json", "corpus_cocircular_normal.json", "corpus_cocircular_optimized.json",
             "mutants/drop_extra_ball.json", "mutants/wrong_arity.json", "mutants/ref_open_as_closed.json",
             "mutants/drop_extra_ball_silent_analysis.json", "mutants/drop_extra_ball_redundant_merge.json",
             "distributions_corpus_normal.json", "distributions_corpus_ext_normal.json",
             "distributions_corpus_cocircular_normal.json"]


def sha(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def read_time(name):
    path = os.path.join(HERE, "logs", name + ".time")
    if not os.path.exists(path):
        return None
    text = open(path).read()
    real = re.search(r"real\s+(\S+)", text)
    exit_code = re.search(r"exit=(\d+)", text)
    # Les codes de sortie ont ete ecrits sur stdout des shells de campagne, pas
    # dans ces fichiers : ils se deduisent du statut JSON (passed -> 0, sinon 1),
    # mapping applique par tower_corpus.main.
    return dict(real=real.group(1) if real else None, exit_logged=int(exit_code.group(1)) if exit_code else None)


def load(name):
    return json.load(open(os.path.join(HERE, name)))


def campaign(name, optimized):
    d = load(name)
    same = sha(os.path.join(HERE, name)) == sha(os.path.join(HERE, optimized))
    return dict(file=name, optimized_file=optimized, normal_equals_optimized_bytes=same, status=d["status"],
                exit_code_from_status=0 if d["status"] == "passed" else 1,
                seed=d["seed"], family=d["family"], fixtures=d["fixtures"], cloud_sizes=d["cloud_sizes"],
                totals=d["totals"], product_stats_sums=d["product_stats_totals"],
                product_stats_note="sums over clouds; max_chain_steps is a SUM of per-cloud maxima",
                refusal_reasons=d["refusal_reasons"], divergence_count=d["divergence_count"],
                time_normal=read_time(name.replace(".json", "")),
                time_optimized=read_time(optimized.replace(".json", "")))


def mutant(name):
    d = load(os.path.join("mutants", name + ".json"))
    silent = sum(1 for c in d["clouds"] if c["catalogue"]["mutated"] and "comparison" in c and not c["comparison"]["divergences"])
    return dict(file="mutants/" + name + ".json", status=d["status"], exit_code_from_status=0 if d["status"] == "passed" else 1,
                mutated_clouds=d["totals"]["mutated_clouds"],
                product_refusals=d["totals"]["product_refusals"], refusal_reasons=d["refusal_reasons"],
                divergence_count=d["divergence_count"],
                clouds_with_divergence=sum(1 for c in d["clouds"] if "comparison" in c and c["comparison"]["divergences"]),
                mutated_clouds_silent=silent, time=read_time("mutant_" + name))


def main():
    git_head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=REPO, capture_output=True, text=True, check=False).stdout.strip()
    gxx = subprocess.run(["g++", "--version"], capture_output=True, text=True, check=False).stdout.splitlines()[0]
    snapshot_sums = {}
    for line in open(os.path.join(SCRATCH, "receipts_raccord_ancres_20260910", "snapshots", "SHA256SUMS")):
        digest, name = line.split()
        snapshot_sums[name] = digest
    overlay_matches_snapshot = all(
        sha(os.path.join(OVERLAY, rel)) == snapshot_sums[os.path.basename(rel)]
        for rel in ("src/forest/anchor_meb.hpp", "src/forest/full_ball_tower.hpp", "tests/full_ball_tower_gate.cpp"))
    review = dict(
        title="Corpus differentiel de nuages entiers aleatoires : juge rationnel vs build_full_ball_tower (WIP raccord ancres)",
        date="2026-09-10", public_status="not_claimed", phase="exploration_v7_hors_registre",
        backend="cpu_reference", profile="quantized_u16_input_only",
        mode="audit_independant_math_and_architecture", GCP_used=False, repo_writes=False,
        repo_head=git_head, overlay=OVERLAY, overlay_matches_snapshot_sha256sums=overlay_matches_snapshot,
        environment=dict(gxx=gxx, python=sys.version.split()[0]),
        sources_read={
            "overlay": {rel: sha(os.path.join(OVERLAY, rel)) for rel in SOURCES_READ["overlay"]},
            "audits": {rel: sha(os.path.join(AUDITS, rel)) for rel in SOURCES_READ["audits"]},
        },
        own_sources={rel: sha(os.path.join(HERE, rel)) for rel in OWN_SOURCES},
        artifacts={rel: sha(os.path.join(HERE, rel)) for rel in ARTIFACTS},
        commands=[
            "cd corpus_aleatoire && g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I ../../overlay_wip_130402 tower_bridge.cpp -o bin/tower_bridge",
            "python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 > corpus_normal.json",
            "python3 -B -O tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 > corpus_optimized.json",
            "python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 2000 --seed 20260911 --no-fixtures > corpus_ext_normal.json",
            "python3 -B -O tower_corpus.py --bridge bin/tower_bridge --clouds 2000 --seed 20260911 --no-fixtures > corpus_ext_optimized.json",
            "python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 300 --seed 20260912 --no-fixtures --family cocircular > corpus_cocircular_normal.json",
            "python3 -B -O tower_corpus.py --bridge bin/tower_bridge --clouds 300 --seed 20260912 --no-fixtures --family cocircular > corpus_cocircular_optimized.json",
            "python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 --mutant drop_extra_ball > mutants/drop_extra_ball.json",
            "python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 --mutant wrong_arity > mutants/wrong_arity.json",
            "python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 --mutant ref_open_as_closed > mutants/ref_open_as_closed.json",
            "python3 -B silent_mutant_analysis.py mutants/drop_extra_ball.json > mutants/drop_extra_ball_silent_analysis.json",
            "python3 -B redundant_merge_check.py mutants/drop_extra_ball_silent_analysis.json > mutants/drop_extra_ball_redundant_merge.json",
            "python3 -B shell_distribution.py <campaign>.json > distributions_<campaign>.json",
            "python3 -B make_review.py > review.json",
        ],
        campaigns=dict(
            main=campaign("corpus_normal.json", "corpus_optimized.json"),
            extended=campaign("corpus_ext_normal.json", "corpus_ext_optimized.json"),
            cocircular=campaign("corpus_cocircular_normal.json", "corpus_cocircular_optimized.json"),
        ),
        distributions={name: load("distributions_" + name + ".json") for name in
                       ("corpus_normal", "corpus_ext_normal", "corpus_cocircular_normal")},
        mutants=dict(
            drop_extra_ball=mutant("drop_extra_ball"), wrong_arity=mutant("wrong_arity"),
            ref_open_as_closed=mutant("ref_open_as_closed"),
            drop_extra_ball_silent_analysis={k: v for k, v in load("mutants/drop_extra_ball_silent_analysis.json").items() if k != "cases"},
            drop_extra_ball_redundant_merge=dict(all_explained=load("mutants/drop_extra_ball_redundant_merge.json")["all_explained"]),
        ),
        not_exercised=[
            "couvertures dupliquees a une meme coupe (ambiguous_cuts=0 sur toutes les campagnes) : aucune ambiguite declaree, donc toutes les images verticales ont ete jugees",
            "same_radius_steps>0 seulement sur la fixture actual_equal_radius_descent (1 pas) ; jamais sur un nuage aleatoire",
            "n<=12, coordonnees dans un cube 0..7 ou sur le cercle x^2+y^2=25 : aucune conclusion d'echelle, de cout ou de completude WSPD",
        ],
    )
    print(json.dumps(review, sort_keys=True, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
