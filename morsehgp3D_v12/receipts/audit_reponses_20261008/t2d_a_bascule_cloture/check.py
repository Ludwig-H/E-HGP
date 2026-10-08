#!/usr/bin/env python3
"""Pins, propositions appliquees a part et JSON synthetiques ; jamais de sonde."""
import ast
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREFIX = "morsehgp3D_v12/"


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    repo = Path(sys.argv[1])
    c = json.loads((HERE / "capture.json").read_text())

    def blob(pin, name):
        return subprocess.check_output(["git", "-C", str(repo), "show", pin + ":" + PREFIX + name])

    source = {name: blob(c["pin"], name) for name in c["sources"]}
    for name, value in source.items():
        need(sha(value) == c["sources"][name], name)
    for name, expected in c["base_sources"].items():
        need(sha(blob(c["base"], name)) == expected, "base " + name)
    pilot = "microbancs/mes_t2d_a/pilote_t2d_a.py"
    pipe = "src/tower/pipeline_run.cpp"
    history = "receipts/audit_reponses_20261008/"
    # Reutilisation explicite des fonctions JSON deja publiees, sans le main du lecteur historique.
    fixture = ast.parse(source[history + "t2d_a_schema902/check.py"])
    functions = [n for n in fixture.body if isinstance(n, ast.FunctionDef) and n.name in ("rows", "take", "report")]
    need(len(functions) == 3, "fixture")
    former = ast.parse(source[history + "t2da_integration/check.py"])
    exercise = [n for n in former.body if isinstance(n, ast.FunctionDef) and n.name == "exercise"]
    need(len(exercise) == 1, "exercise")
    scope = dict(ast=ast, copy=copy, importlib=__import__("importlib"), json=json, Path=Path,
                 tempfile=tempfile, need=need, functions=functions)
    exec(compile(ast.Module(body=exercise, type_ignores=[]), "exercise-publie", "exec"), scope)
    with tempfile.TemporaryDirectory(prefix="audit-a-bascule-") as folder:
        root = Path(folder)
        for name in (pilot, pipe):
            target = root / PREFIX / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(blob(c["base"], name))
        for name in ("t2da_integration/campagne.patch", "t2d_a_fenetre_patch/proposition.patch"):
            patch = root / "verify.patch"
            patch.write_bytes(source[history + name])
            for flags in (["--check"], []):
                subprocess.run(["git", "apply", *flags, str(patch)], cwd=root, check=True, capture_output=True)
        for name in (pilot, pipe):
            need((root / PREFIX / name).read_bytes() == source[name], "postimage " + name)
        p = root / PREFIX / pilot
        json_results = scope["exercise"](p, corrected=True)
        # Module courant charge pour ses fonctions uniquement ; main n'est jamais invoque.
        spec = importlib.util.spec_from_file_location("pilot_command", p)
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        patch = HERE / "commande.patch"
        need(sha(patch.read_bytes()) == c["commande_patch_sha256"], "patch commande")
        for flags in (["--check"], []):
            subprocess.run(["git", "apply", *flags, str(patch)], cwd=root, check=True, capture_output=True)
        need(sha(p.read_bytes()) == c["proposition_pilote_sha256"], "postimage commande")
        spec2 = importlib.util.spec_from_file_location("pilot_proposed", p)
        n = importlib.util.module_from_spec(spec2)
        spec2.loader.exec_module(n)
        checks = 0
        for arm, schema in (("avant", "sequentiel"), ("apres", "sequentiel"), ("apres", "recouvert")):
            for device in (False, True):
                for digest in (False, True):
                    expected = dict(k=5, fils=48, passes=2, empreinte=digest, schema=schema, bras=arm, appareil=device)
                    before = m.commande("probe", ["--uniform=8,1,21"], expected)
                    after = n.commande("probe", ["--uniform=8,1,21"], expected)
                    want_seq = arm == "apres" and schema == "sequentiel"
                    need(("--sequentiel" in after) == want_seq, "voie explicite")
                    need(("--recouvert" in after) == (schema == "recouvert"), "voie recouverte")
                    need([v for v in after if v != "--sequentiel"] == before, "autre argument modifie")
                    if want_seq:
                        need("--sequentiel" not in before and "--recouvert" not in before, "temoin historique")
                    checks += 1
        # La sonde avant la bascule ignore --sequentiel : ne jamais le lui transmettre.
        need(b'--sequentiel' not in blob(c["base"], "bench/full_probe.cpp"), "ancien CLI")
        for pin, digest in c["archives_avant"].items():
            archive = blob(pin, "bench/full_probe.cpp")
            need(sha(archive) == digest and b'--sequentiel' not in archive, "CLI archive")
        need(b'overlapped = true;' in source["bench/full_probe.cpp"], "nouveau defaut")
        need(b'else if (a == "--sequentiel") o.overlapped = false;' in source["bench/full_probe.cpp"], "nouveau CLI")
    print(json.dumps(dict(pin=c["pin"], sources=len(source), postimages_publiees_identiques=2,
                         json=json_results, commande_cases=checks, compatibilite_avant=True,
                         archives_avant=len(c["archives_avant"]),
                         proposition_appliquee_isolee=True, moteur_execute=False), ensure_ascii=False, sort_keys=True))


if __name__ == "__main__":
    main()
