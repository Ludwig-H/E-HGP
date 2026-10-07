#!/usr/bin/env python3
"""Contre-jugement Python seulement : aucune execution du moteur, aucune mesure."""
import argparse
import copy
import hashlib
import json
import pathlib
import sys
import tempfile
import types

HERE = pathlib.Path(__file__).resolve().parent
PASSES, K, THREADS, SITES = 6, 5, 48, 5


def require(test, message):
    if not test:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load():
    capture = json.loads((HERE / "capture.json").read_text())
    source = (HERE / "pilote_t2c.snapshot.py").read_bytes()
    require(sha(source) == capture["inputs"][0]["sha256"], "snapshot modifie")
    module = types.ModuleType("pilote_snapshot")
    module.__file__ = str(HERE / "pilote_t2c.snapshot.py")
    exec(compile(source, module.__file__, "exec"), module.__dict__)
    return module, capture


def rows(capture, digest, wall=60_000_000):
    """Forme emise par les C++ epingles ; valeurs synthetiques, sans oracle geometrique."""
    fields = capture["producer_fields"]
    diag = dict.fromkeys(fields["diagnostic_scalar"], 0)
    diag["reste_ns"] = wall
    for name in fields["diagnostic_arrays"]:
        diag[name] = [0] * (K - 1 if name in ("table_ns", "join_ns") else K)
    result = [dict(phase="tour_g", **{"pass": p}, status="ok", reason="none", order=0,
                   coord_bits=21, kmax=K, threads=THREADS, sites=SITES,
                   wall_ns=wall, diagnostics=copy.deepcopy(diag)) for p in range(PASSES)]
    for k in range(1, K + 1):
        obj = dict.fromkeys(fields["object_counts"], 1)
        work = dict.fromkeys(fields["work_counts"], 0)
        work["chaines"] = [0] * 16
        result.append(dict(phase="ordre", k=k, objet=obj, travail=work))
    # Le producteur n'emet ordres et digest qu'apres la derniere passe, puis exit.
    result += [dict(phase="digest", resolution_sha256=digest),
               dict(phase="exit", status="ok", reason="none", order=0)]
    return result


def wire(data):
    return "".join(json.dumps(row, sort_keys=True) + "\n" for row in data)


def trial_cases(module, capture, root):
    producer = root / "fake_probe.py"
    producer.write_text("#!" + sys.executable + "\nimport pathlib,sys\n"
                        "p=pathlib.Path(__file__).parent\n"
                        "sys.stdout.write((p/'wire.jsonl').read_text())\n"
                        "sys.exit(int((p/'code').read_text()))\n")
    producer.chmod(0o700)
    args = types.SimpleNamespace(donnees=str(root), sortie=str(root))
    nominal = rows(capture, module.EMPREINTE_NG00_K5 + "0" * 48)
    cases = [("nominal_forme_complete", nominal, 0, True)]
    cases.append(("code_nonzero", nominal, 2, False))
    cases.append(("cinq_passes_pour_six", nominal[1:], 0, False))
    for name, value in (("wall_booleen", True), ("wall_negatif", -1)):
        altered = copy.deepcopy(nominal)
        for row in altered[:PASSES]:
            row["wall_ns"] = value
        cases.append((name, altered, 0, True))
    altered = copy.deepcopy(nominal)
    for row in altered[:PASSES]:
        row["pass"] = 0
    cases.append(("six_indices_zero", altered, 0, True))
    altered = copy.deepcopy(nominal)
    for row in altered[:PASSES]:
        row["kmax"], row["threads"], row["coord_bits"] = 10, 1, 24
    cases.append(("configuration_autre", altered, 0, True))
    altered = [row for row in copy.deepcopy(nominal) if row["phase"] != "ordre"]
    cases.append(("ordres_absents", altered, 0, True))
    out = []
    for name, data, code, expected in cases:
        (root / "wire.jsonl").write_text(wire(data))
        (root / "code").write_text(str(code))
        journal = root / (name + ".jsonl")
        take = module.prise(args, str(producer), "ng00", K, THREADS, PASSES, str(journal))
        require(take["valide"] is expected, "resultat inattendu : " + name)
        out.append(dict(case=name, valide=take["valide"], code=take["code"],
                        g_ns=take.get("g_ns"), murs=take["murs_ns"]))
    return out


def campaign_cases(module, capture, root):
    """120 journaux synthetiques generes ; aucun processus natif ni temps mesure."""
    arms = module.BRAS_JUGES
    report = {"construction": {"binaires": {b: {"sha256": "ab" * 32} for b in arms}},
              "campagne_k5": {"fils": THREADS, "passes": PASSES, "tours_demandes": 8,
                              "binaires_apres": {b: "ab" * 32 for b in arms}, "trames": {}}}
    records = []
    for frame in module.TRAMES:
        digest = (module.EMPREINTE_NG00_K5 + "0" * 48) if frame == "ng00" else "cd" * 32
        raw = wire(rows(capture, digest)).encode()
        turns = []
        for turn in range(8):
            entries = {}
            for arm in arms:
                name = "%s_%d_%s.jsonl" % (frame, turn, arm)
                (root / name).write_bytes(raw)
                records.append((name, sha(raw)))
                entries[arm] = dict(journal=name, journal_sha256=sha(raw), code=0, valide=True,
                                    murs_ns=[60_000_000] * PASSES, g_ns=60_000_000,
                                    empreinte=digest, profil=[], diagnostics=rows(capture, digest)[0]["diagnostics"])
            turns.append(entries)
        report["campagne_k5"]["trames"][frame] = turns
    baseline = module.juger(report, str(root))  # verifier_journaux=True, bootstrap inchange
    require(all(v == "rejete" for v in baseline["verdicts"].values()), "controle nominal incoherent")
    altered = copy.deepcopy(report)
    for turns in altered["campagne_k5"]["trames"].values():
        for turn in turns:
            turn["apres"]["g_ns"] = 30_000_000
    changed = module.juger(altered, str(root))
    require(changed["verdicts"]["lot_t2c"] == "adopte", "temoin lot non reproduit")
    require(changed["verdicts"]["G-L7"] == "adopte", "temoin G-L7 non reproduit")
    require(all(sha((root / name).read_bytes()) == digest for name, digest in records),
            "journal modifie pendant le rejeu")
    return {"journaux": len(records), "verifier_journaux": True, "bootstrap": module.REGLE_T2C["bootstrap"],
            "nominal_verdicts": baseline["verdicts"], "seul_g_ns_apres_divise_par_deux": changed["verdicts"],
            "journaux_inchanges": True, "murs_ns_inchanges": True,
            "refus_apres_alteration": changed["refus"],
            "ratios_lot_apres_alteration": {f: changed["cas"][f]["lot_t2c"] for f in module.TRAMES}}


def replay():
    module, capture = load()
    with tempfile.TemporaryDirectory(prefix="t2c_admission_") as tmp:
        root = pathlib.Path(tmp)
        trial = trial_cases(module, capture, root)
        campaign = campaign_cases(module, capture, root)
    actual = capture["new_object_digest_prefixes"]["ng00"]
    require(actual != module.EMPREINTE_NG00_K5, "ecart empreinte non reproduit")
    return {"scope": "prototype_unpublished_python_only_synthetic_values",
            "pilot_sha256": capture["inputs"][0]["sha256"], "prise": trial, "juger": campaign,
            "empreinte_ng00": {"pilote": module.EMPREINTE_NG00_K5,
                                "producteur_patch_objet": actual,
                                "nouvelle_empreinte_acceptee_par_prefixe": actual.startswith(module.EMPREINTE_NG00_K5)}}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result = replay()
    if args.check:
        require(result == json.loads((HERE / "results.json").read_text()), "resultats differents")
        print("t2c_admission_ok: 8 prises synthetiques, 2 jugements de 120 journaux ; aucun moteur")
    else:
        print(json.dumps(result, indent=1, sort_keys=True))


if __name__ == "__main__":
    main()
