#!/usr/bin/env python3
"""Relit les captures F2/E sans executer de binaire ni chronometrer le produit.

Python standard ; garde explicite sous -O. Par defaut : JSON sur stdout.
--check compare avec resultats.json. Les fichiers lus sont epingles dans pins.json.
Les sommes/soustractions de sous-chronos sont des diagnostics conditionnels,
jamais des mesures d'un pipeline modifie ou GPU.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import statistics
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
V12 = "morsehgp3D_v12/"
F2 = V12 + "receipts/g4_t1f_20261007/"
E = V12 + "receipts/g4_t2e_20261007/"
G1 = E + "resultats/cmd/002_g1_k5_publier/files/g1_k5/"
M7 = E + "resultats/cmd/003_m7/files/m7/"
ME = E + "resultats/cmd/004_mes_e/files/mes_e/"
PHASES = ("traversal", "count", "fill", "levels", "sort", "assemble", "table")
GIT_PIN = None
BLOBS = {}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def object_pairs(pairs):
    out = {}
    for key, value in pairs:
        require(key not in out, "cle JSON dupliquee : " + key)
        out[key] = value
    return out


def invalid_constant(value):
    raise ValueError("constante JSON non finie : " + value)


def decode(text):
    return json.loads(text, object_pairs_hook=object_pairs, parse_constant=invalid_constant)


def source_bytes(path):
    if path not in BLOBS:
        if GIT_PIN is None:
            BLOBS[path] = (ROOT / path).read_bytes()
        else:
            BLOBS[path] = subprocess.check_output(
                ["git", "show", GIT_PIN + ":" + path], cwd=ROOT, stderr=subprocess.PIPE)
    return BLOBS[path]


def read(path):
    return decode(source_bytes(path).decode("utf-8"))


def lines(path):
    return [decode(s) for s in source_bytes(path).decode("utf-8").splitlines() if s.strip()]


def number(value, minimum=0):
    require(type(value) in (int, float) and math.isfinite(value) and value >= minimum,
            "nombre invalide : " + repr(value))
    return value


def ms(values):
    values = list(values)
    return {"median": round(statistics.median(values) / 1e6, 6),
            "min": round(min(values) / 1e6, 6), "max": round(max(values) / 1e6, 6)}


def catalogue(pins):
    sources = sorted(p for p in pins if p.startswith(F2) and p.endswith("/stdout") and "_cat_" in p)
    require(len(sources) == 12, "F2 : douze cas requis")
    results = []
    for source in sources:
        raw = lines(source)
        require(len(raw) == 21 and raw[-1] == {"phase": "exit", "status": "ok", "reason": "none"},
                source + " : vingt lignes de mesure puis sortie ok requises")
        rows, digests = raw[:-1:2], raw[1:-1:2]
        require([r["phase"] for r in rows] == ["catalogue"] * 10, source)
        require([r["pass"] for r in rows] == list(range(10)), source + " : passes")
        require(all(d["phase"] == "digest" for d in digests), source + " : digests")
        hashes = {d["catalogue_sha256"] for d in digests}
        require(len(hashes) == 1 and len(next(iter(hashes))) == 64, source + " : digest instable")
        first = rows[0]
        stable = ("kmax", "leaf", "sites", "balls", "incidences", "levels", "ledger")
        for row in rows:
            require(row["status"] == "ok" and row["reason"] == "none", source + " : refus")
            require(row["coord_bits"] == 21 and row["threads"] == 48, source + " : regime")
            require(all(row[k] == first[k] for k in stable), source + " : travail instable")
            number(row["wall_ns"], 1)
            for key in PHASES:
                number(row["diagnostics"][key + "_ns"])
            require(sum(row["diagnostics"][k + "_ns"] for k in PHASES) <= row["wall_ns"],
                    source + " : sous-chronos depassent wall")
        warm = rows[1:]
        diag = lambda r, name: r["diagnostics"][name + "_ns"]
        wall = ms(r["wall_ns"] for r in warm)
        result = {"cas": Path(source).parent.name.split("_", 1)[1], "source": source,
                  "processus": 1, "passes": 10, "passes_chaudes": 9,
                  **{k: first[k] for k in stable if k != "ledger"},
                  "catalogue_digest": next(iter(hashes)), "premiere_passe_ms": first["wall_ns"] / 1e6,
                  "wall_chaud_ms": wall,
                  "phases_medianes_ms": {k: ms(diag(r, k) for r in warm)["median"] for k in PHASES},
                  "assemblage_plus_table_ms": ms(diag(r, "assemble") + diag(r, "table") for r in warm),
                  "reste_si_parcours_et_feuilles_gratuits_ms": ms(
                      r["wall_ns"] - sum(diag(r, k) for k in ("traversal", "count", "fill")) for r in warm),
                  "residu_non_ventile_ms": ms(r["wall_ns"] - sum(diag(r, k) for k in PHASES) for r in warm),
                  "passes_chaudes_sous_100ms": sum(r["wall_ns"] < 100_000_000 for r in warm)}
        if "_k5_" in result["cas"] and "ng" in result["cas"]:
            reference = {"ng00": 200, "ng01": 163, "ng02": 195}[result["cas"].split("_")[1]]
            result["ratio_descriptif_sur_domain_v11_historique"] = round(wall["median"] / reference, 6)
        results.append(result)
    return results


def bootstrap(ratios, rng):
    logs = [math.log(x) for x in ratios]
    samples = sorted(sum(logs[rng.randrange(len(logs))] for _ in logs) / len(logs) for _ in range(10000))
    return math.exp(sum(logs) / len(logs)), [math.exp(samples[250]), math.exp(samples[9749])]


def jumps():
    report = read(G1 + "rapport_g1.json")
    rng = random.Random(20261007)
    results = []
    for case in ("ng00_k5", "ng01_k5", "ng02_k5"):
        campaigns = report["campagnes"][case]
        require(len(campaigns) == 1, "G1 : campagne ambigue")
        campaign = next(iter(campaigns.values()))
        takes = campaign["prises"]
        require(len(takes) == 5 and campaign["passes_par_processus"] == 3, "G1 : repetition")
        ratios, facts = [], []
        for take in takes:
            path = G1 + take["journal"]
            require(hashlib.sha256(source_bytes(path)).hexdigest() == take["journal_sha256"], path)
            rows = [r for r in lines(path) if r["phase"] == "resolution_saut"]
            require([r["k"] for r in rows] == [2, 3, 4, 5], path + " : ordres")
            require(all(r["foret"]["identique"] and r["foret"]["controle_v11_reproduit"] and
                        r["foret"]["ecarts_structure"] == 0 and r["foret"]["representants_hors_composante"] == 0
                        for r in rows), path + " : foret differente")
            base = sum(number(r["secondes"]["replique_v12"], 1e-12) for r in rows)
            variant = sum(number(r["secondes"]["replique_v12_saut"], 1e-12) for r in rows)
            ratios.append(variant / base)
            total = sum(r["census"]["replique_v12"]["satures"] for r in rows)
            remaining = sum(r["census"]["replique_v12_saut"]["satures"] for r in rows)
            facts.append((total, remaining))
            require(abs(ratios[-1] - take["totaux"]["rapport_total"]) < 1e-12, path + " : ratio recalcule")
        require(len(set(facts)) == 1, case + " : travail instable")
        gm, interval = bootstrap(ratios, rng)
        published = report["verdict"]["cas"][case]
        require(abs(gm - published["moyenne_geometrique"]) < 1e-12 and
                max(abs(a - b) for a, b in zip(interval, published["ic95"])) < 1e-12,
                case + " : statistique differente")
        results.append({"cas": case, "processus": 5, "passes_par_processus": 3,
                        "statistique": "ratio des sommes par ordre des minima de trois passes",
                        "rapports_par_processus": [round(x, 9) for x in ratios],
                        "moyenne_geometrique": round(gm, 9), "ic95": [round(x, 9) for x in interval],
                        "satures_evites_pct": round(100 * (1 - facts[0][1] / facts[0][0]), 6),
                        "forets_identiques_selon_journaux": True,
                        "rejet_performance_reproduit": interval[1] >= 1})
    return results


def profile():
    results = []
    for case, kmax in (("ng00", 5), ("ng01", 5), ("ng02", 5), ("ng00", 10)):
        source = M7 + case + "_k" + str(kmax) + ".jsonl"
        rows = [r for r in lines(source) if r["phase"] == "profil_resolution"]
        require([r["k"] for r in rows] == list(range(2, kmax + 1)), source + " : ordres")
        require(all(r["fils"] == 1 and r["controle_vidage"]["conforme"] for r in rows), source)
        cycles = sum(number(r["cycles_total"], 1) for r in rows)
        components = rows[0]["composantes"].keys()
        require(all(sum(x["cycles"] for x in r["composantes"].values()) == r["cycles_total"] for r in rows), source)
        instrumented = sum(r["secondes_total"] for r in rows)
        uninstrumented = sum(r["secondes_replique_v12_non_instrumentee"] for r in rows)
        results.append({"cas": case + "_k" + str(kmax), "fils_resolution": 1,
                        "secondes_instrumentees": round(instrumented, 6),
                        "secondes_non_instrumentees_meme_journal": round(uninstrumented, 6),
                        "rapport_instrumente_sur_non_instrumente": round(instrumented / uninstrumented, 6),
                        "parts_cycles_pct": {c: round(100 * sum(r["composantes"][c]["cycles"] for r in rows)
                                                        / cycles, 6) for c in components}})
    return results


def millions():
    data = read(ME + "mes_e.json")
    require(data["masque"] == "802811" and len(data["prises"]) == 11, "MES-E : regime")
    results = []
    for row in data["prises"]:
        require(row["code"] == 0 and not row["expire"] and row["fils"] == 48, "MES-E : prise invalide")
        results.append({"cas": row["cas"], "k": row["k"], "sites": row["sonde"]["sites"],
                        "processus": 1, "mur_processus_secondes": row["secondes"],
                        "pic_rss_gio": round(row["pic_rss_octets"] / 2 ** 30, 6),
                        "domain_s": row["sonde"]["domain_ns"] / 1e9,
                        "forest_s": row["sonde"]["forest_ns"] / 1e9})
    scaling = {}
    for scene in ("ign_lyon", "eth3d_courtyard"):
        selected = sorted((r for r in results if r["cas"].startswith(scene) and r["k"] == 5), key=lambda r: r["sites"])
        require(len(selected) == 4, "MES-E : quatre tailles requises")
        a, b = selected[0], selected[-1]
        scaling[scene] = {"exposant_descriptif_1M_8M": round(math.log(b["mur_processus_secondes"] /
                            a["mur_processus_secondes"]) / math.log(b["sites"] / a["sites"]), 6),
                         "rapport_forest_8M_1M": round(b["forest_s"] / a["forest_s"], 6)}
    return {"voie": "v11 gelee CPU 48 fils, une prise froide par cas, serialization comprise",
            "cas": results, "echelle_descriptive_sans_intervalle": scaling}


def main():
    global GIT_PIN
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--git-pin", nargs="?", const="BASE", metavar="COMMIT",
                        help="lire les entrees via git show ; sans valeur : pins.base_commit")
    args = parser.parse_args()
    pins = decode((HERE / "pins.json").read_text(encoding="utf-8"))
    if args.git_pin is not None:
        revision = pins["base_commit"] if args.git_pin == "BASE" else args.git_pin
        GIT_PIN = subprocess.check_output(
            ["git", "rev-parse", "--verify", "--end-of-options", revision + "^{commit}"],
            cwd=ROOT, stderr=subprocess.PIPE, text=True).strip()
    for path, expected in pins["sha256"].items():
        require(not Path(path).is_absolute() and ".." not in Path(path).parts, "chemin non local")
        actual = hashlib.sha256(source_bytes(path)).hexdigest()
        require(actual == expected, "epingle modifiee : " + path)
    capture_sources = {}
    for receipt, sources in (
        (F2 + "receipt.json", ("bench/catalogue_probe.cpp", "src/catalogue/catalogue.cpp",
                               "src/catalogue/assemble.cpp", "src/catalogue/table.cpp")),
        (E + "receipt.json", ("microbancs/mes_g1_saut/pilote_g1.py", "microbancs/mes_e_echelle/pilote_e.py",
                              "microbancs/mes_m3_m4_tour/vidage/vidage_v11.cpp"))):
        captured = {p["path"]: p["sha256"] for p in read(receipt)["source"]["manifest"]}
        for source in sources:
            path = V12 + source
            require(captured.get(path) == pins["sha256"][path], "source differente de la capture : " + path)
        capture_sources[receipt] = len(sources)
    result = {"schema": "audit_performance_F2_E_v1", "base_commit": pins["base_commit"],
              "inputs_verified": len(pins["sha256"]), "sources_identiques_aux_captures": capture_sources,
              "catalogue_F2": catalogue(pins["sha256"]),
              "saut_G_L3": jumps(), "profil_M7": profile(), "MES_E": millions()}
    encoded = json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False) + "\n"
    if args.check:
        require(encoded == (HERE / "resultats.json").read_text(encoding="utf-8"), "resultats non reproductibles")
        print("OK : %d empreintes, 120 passes catalogue, 15 prises G-L3, 4 profils, 11 cas MES-E ; aucun banc execute"
              % result["inputs_verified"])
    else:
        sys.stdout.write(encoded)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, TypeError, subprocess.CalledProcessError) as error:
        sys.exit("REFUS : " + str(error))
