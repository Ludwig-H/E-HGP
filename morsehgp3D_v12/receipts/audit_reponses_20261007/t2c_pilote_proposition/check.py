#!/usr/bin/env python3
"""Applique la proposition en repertoire temporaire ; Python/JSON uniquement."""
import argparse
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
import pathlib
import subprocess
import sys
import tempfile
import types

HERE = pathlib.Path(__file__).resolve().parent
PRIOR = HERE.parent / "t2c_pilote_admission"
REL = pathlib.Path("morsehgp3D_v12/microbancs/mes_t2c_g/pilote_t2c.py")


def require(test, message):
    if not test:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load(data, name):
    module = types.ModuleType(name)
    module.__file__ = str(HERE / (name + ".py"))
    exec(compile(data, module.__file__, "exec"), module.__dict__)
    return module


def prepared(root):
    capture = json.loads((HERE / "capture.json").read_text())
    source = (PRIOR / "pilote_t2c.snapshot.py").read_bytes()
    require(sha(source) == capture["old_snapshot_sha256"], "snapshot historique differente")
    target = root / REL
    target.parent.mkdir(parents=True)
    target.write_bytes(source)
    for patch in ("prototype_delta.patch", "proposition.patch"):
        for extra in (["--check"], []):
            subprocess.run(["git", "apply", *extra, str(HERE / patch)], cwd=root, check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if patch == "prototype_delta.patch":
            require(sha(target.read_bytes()) == capture["inputs"][0]["sha256"], "preimage differente")
    require(sha(target.read_bytes()) == capture["proposed_sha256"], "proposition differente")
    return load(target.read_bytes(), "proposition")


def fixtures():
    old = load((PRIOR / "check.py").read_bytes(), "fixtures")
    capture = json.loads((PRIOR / "capture.json").read_text())
    return old, capture


def rows(module, old, capture, schema="apres", profile=False):
    data = old.rows(capture, module.EMPREINTE_NG00_K5 + "0" * 48)
    if schema == "avant":
        for row in data[:6]:
            row["diagnostics"] = {k: v for k, v in row["diagnostics"].items()
                                  if k in module.DIAG_AVANT or k == "order_ns"}
    if profile:
        expanded = []
        for row in data:
            expanded.append(row)
            if row["phase"] == "tour_g":
                for k in range(2, 6):
                    expanded.append(dict(phase="profil_g", **{"pass": row["pass"]}, k=k,
                                         ghz_tsc=2.0, biais_cycles=1.0, table_ns=0, join_ns=0,
                                         sections={s: dict(cycles=0, n=0, part=0.0, ns=0.0)
                                                   for s in module.SECTIONS_PROFIL},
                                         reste=dict(cycles=0, part=0.0)))
        data = expanded
    return data


def trials(module, root, old, capture):
    exe = root / "fake.py"
    exe.write_text("#!" + sys.executable + "\nimport pathlib,sys\np=pathlib.Path(__file__).parent\n"
                   "sys.stdout.buffer.write((p/'wire').read_bytes())\n"
                   "sys.exit(int((p/'code').read_text()))\n")
    exe.chmod(0o700)
    args = types.SimpleNamespace(donnees=str(root), sortie=str(root))
    cases = []
    nominal = rows(module, old, capture)
    def add(name, data, accepted=False, schema="apres", profile=False, code=0):
        raw = old.wire(data).encode() if type(data) is list else data
        cases.append((name, raw, accepted, schema, profile, code))
    add("nominal_apres", nominal, True)
    add("nominal_avant", rows(module, old, capture, "avant"), True, "avant")
    add("nominal_profil", rows(module, old, capture, profile=True), True, profile=True)
    add("schema_avant_sur_bras_apres", rows(module, old, capture, "avant"))
    add("profil_non_declare", rows(module, old, capture, profile=True))
    add("code_nonzero", nominal, code=2)
    add("cinq_passes_pour_six", nominal[1:])
    for name, value in (("wall_bool", True), ("wall_negatif", -1)):
        data = copy.deepcopy(nominal)
        for row in data[:6]:
            row["wall_ns"] = value
        add(name, data)
    data = copy.deepcopy(nominal)
    for row in data[:6]:
        row["pass"] = 0
    add("indices_repetes", data)
    data = copy.deepcopy(nominal)
    for row in data[:6]:
        row["kmax"], row["threads"], row["coord_bits"] = 10, 1, 24
    add("configuration_autre", data)
    add("ordres_absents", [r for r in nominal if r["phase"] != "ordre"])
    for name, key, value in (("exit_bool", "order", False), ("exit_reason", "reason", "other")):
        data = copy.deepcopy(nominal)
        data[-1][key] = value
        add(name, data)
    data = copy.deepcopy(nominal)
    data[-2]["resolution_sha256"] = "ab" * 8
    add("digest_tronque", data)
    data = copy.deepcopy(nominal)
    del data[0]["coord_bits"]
    add("metadonnees_absentes", data)
    data = copy.deepcopy(nominal)
    data[6]["travail"]["route_t1"] = True
    add("compteur_bool", data)
    data = copy.deepcopy(nominal)
    del data[0]["diagnostics"]["orders_ns"]
    add("diagnostic_manquant", data)
    raw = old.wire(nominal).encode()
    add("cle_json_repetee", raw.replace(b'"wall_ns": 60000000', b'"wall_ns": 1,"wall_ns": 60000000', 1))
    add("texte_suffixe", raw + b'not-json\n')
    add("utf8_invalide", raw + b'\xff\n')
    results = []
    for name, data, expected, schema, profile, code in cases:
        (root / "wire").write_bytes(data)
        (root / "code").write_text(str(code))
        take = module.prise(args, str(exe), "ng00", 5, 48, 6, str(root / "take.jsonl"), schema, profile)
        require(take["valide"] is expected, "admission inattendue : " + name)
        results.append(dict(case=name, valide=take["valide"], admission=take.get("admission")))
    return results


def campaigns(module, root, old, capture):
    arms = module.BRAS_JUGES
    report = {"construction": {"binaires": {b: {"sha256": "ab" * 32} for b in arms}},
              "campagne_k5": {"fils": 48, "passes": 6, "tours_demandes": 8,
                              "binaires_apres": {b: "ab" * 32 for b in arms}, "trames": {}}}
    for frame in module.TRAMES:
        turns = []
        for turn in range(8):
            entries = {}
            for arm in arms:
                name = "%s_%d_%s.jsonl" % (frame, turn, arm)
                raw = old.wire(rows(module, old, capture, module.schema_bras(arm))).encode()
                path = root / name
                path.write_bytes(raw)
                record = module.lire_prise(str(path), 0, 5, 48, 6, module.schema_bras(arm))
                record.update(journal=name, journal_sha256=sha(raw), code=0)
                entries[arm] = record
            turns.append(entries)
        report["campagne_k5"]["trames"][frame] = turns
    baseline = module.juger(report, str(root))
    require(not baseline["refus"] and all(v == "rejete" for v in baseline["verdicts"].values()),
            "nominal complet refuse")
    changed = copy.deepcopy(report)
    for turns in changed["campagne_k5"]["trames"].values():
        for turn in turns:
            turn["apres"]["g_ns"] = 30_000_000
    summary = module.juger(changed, str(root))
    require(all(v == "refuse" for v in summary["verdicts"].values()), "resume forge non refuse")
    first = report["campagne_k5"]["trames"]["ng00"][0]["apres"]
    path = root / first["journal"]
    path.write_text("not-json\n")
    first["journal_sha256"] = sha(path.read_bytes())
    invalid = module.juger(report, str(root))
    require(all(v == "refuse" for v in invalid["verdicts"].values()), "brut invalide rehache non refuse")
    return {"nominal": baseline["verdicts"], "resume_diverge": summary["verdicts"],
            "resume_motif": summary["refus"], "brut_invalide_rehache": invalid["verdicts"],
            "brut_motif": invalid["refus"]}


def native_formats(module, root):
    capture = json.loads((HERE / "capture.json").read_text())
    results = []
    for name, schema in (("before", "avant"), ("after", "apres")):
        meta = capture["root_native_format_fixtures"][name]
        path = HERE / (name + ".jsonl")
        require(sha(path.read_bytes()) == meta["stdout_sha256"], "capture native modifiee")
        require(meta["stderr_bytes"] == 0, "stderr natif non vide")
        checked = module.lire_prise(str(path), meta["code"], 5, 1, 2, schema)
        results.append(dict(fixture=name, schema=schema, valide=checked["valide"], sites=checked["sites"]))
        # Variante de FORME uniquement : K=5, sites=2 -> deux lignes ordre,
        # diagnostics de longueur K, toujours un seul digest et exit final.
        rows = [json.loads(line) for line in path.read_text().splitlines()]
        rows = [row for row in rows if row["phase"] != "ordre" or row["k"] <= 2]
        for row in rows:
            if row["phase"] == "tour_g":
                row["sites"] = 2
        dest = root / (name + "_sites2.jsonl")
        dest.write_text("".join(json.dumps(row) + "\n" for row in rows))
        small = module.lire_prise(str(dest), 0, 5, 1, 2, schema)
        require(small["sites"] == 2, "ordres clippes refuses")
        results.append(dict(fixture=name + "_synthetic_sites2", schema=schema, valide=small["valide"], sites=2))
    return results


def replay():
    old, capture = fixtures()
    with tempfile.TemporaryDirectory(prefix="t2c_proposition_") as name:
        root = pathlib.Path(name)
        module = prepared(root)
        native = native_formats(module, root)
        cases = trials(module, root, old, capture)
        judgments = campaigns(module, root, old, capture)
        with contextlib.redirect_stdout(io.StringIO()) as output:
            status = module.etape_auto_test()
        require(status == 0, "auto-tests statistiques modifies")
    return dict(scope="proposition_non_appliquee_python_seul", prises=cases, jugements=judgments,
                formats_natifs_root_et_derives=native,
                auto_test={"code": status, "stdout": output.getvalue().strip()})


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result = replay()
    if args.check:
        require(result == json.loads((HERE / "results.json").read_text()), "resultats differents")
        print("t2c_proposition_ok: 21 prises, 3 campagnes, 5 auto-tests ; Python uniquement")
    else:
        print(json.dumps(result, indent=1, sort_keys=True))
