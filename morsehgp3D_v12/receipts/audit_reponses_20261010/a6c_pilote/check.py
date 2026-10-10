#!/usr/bin/env python3
"""A6c : rejeu du juge sur journaux artificiels, sans sonde ni donnees reelles."""
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile

PIN = "aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66"
PREFIX = "morsehgp3D_v12/microbancs/"
FILES = ["mes_t2d_a6c/pilote_t2d_a6c.py", "outils/banc_full.py",
         "outils/lecteur_full.py", "outils/test_lecteur_full.py"]


def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def run(repo):
    with tempfile.TemporaryDirectory(prefix="audit-a6c-json-") as tmp:
        root = Path(tmp)
        hashes = {}
        for rel in FILES:
            data = subprocess.check_output(["git", "-C", str(repo), "show", PIN + ":" + PREFIX + rel])
            target = root / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            hashes[PREFIX + rel] = hashlib.sha256(data).hexdigest()
        pilot = load("audit_a6c", root / FILES[0])
        fixture = load("audit_a6c_fixture", root / FILES[-1])
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            need(pilot.etape_auto_test() == 0, "auto-test officiel")
        cases = [("synthetic_%02d" % i, 70000 if i < 21 else 40000) for i in range(37)]
        large = cases[:21]
        plan = dict(cas=[list(x) for x in cases], fils=48, passes=10, tours=5, tours_grandes=6, essai=False)
        shas = {b: ("b" if b == "apres" else "a") * 64 for b in pilot.BRAS}
        report = dict(construction=dict(binaires={b: dict(sha256=h) for b, h in shas.items()}),
                      environnement={b: dict(gpu_apps="") for b in ("avant", "apres")},
                      identite=dict(prises={}), grandes=dict(trames=[n for n, _ in large], tours=[]),
                      ng=dict(passes=10, trames={n: [] for n in pilot.TRAMES}, binaires_apres=shas))
        journal_id = 0

        def take(arm, frames, k, threads, digest, wall):
            nonlocal journal_id
            path = root / ("journal_%03d.jsonl" % journal_id)
            journal_id += 1
            expected = dict(voie="appareil", k=k, fils=threads, passes=len(frames), empreinte=digest,
                            trames=[list(x) for x in frames], budget_appareil="partage", bits=21, schema="recouvert")
            rows = [dict(phase="open", status="ok", reason="none", wall_ns=5, budget_appareil="partage")]
            for i, (name, sites) in enumerate(frames):
                r = fixture.overlapped_row(i, wall)
                r.update(trame=name, sites=sites, kmax=k, threads=threads, pic_octets=1 << 30,
                         rss_max_octets=2 << 30, pic_appareil_octets=0,
                         memoire_octets=dict(P=[1 << 27, 1 << 28], C=[1 << 28, 1 << 30], tour=[1 << 29, 1 << 30]))
                if k == 10:
                    r["fins_par_ordre_ns"] += copy.deepcopy(r["fins_par_ordre_ns"][1:]) + [r["fins_par_ordre_ns"][-1][:]]
                if digest:
                    r["full_sha256"] = hashlib.sha256((name + ":" + str(k)).encode()).hexdigest()
                else:
                    r.pop("full_sha256")
                rows += [r, dict(phase="liberation", pass_=i, liberation_ns=3)]
            rows.append(dict(phase="exit", status="ok", reason="none"))
            path.write_text(fixture.dump(rows))
            state, why, summary = pilot.admettre(str(path), 0, expected)
            need(state == "ok", why)
            return dict(journal=path.name, journal_sha256=pilot.bf.sha256_file(path), code=0, bras=arm,
                        attendu=expected, secondes=0, etat=state, raison=why, resume=summary)

        for arm in ("avant", "apres"):
            identities = []
            for name, sites in pilot.TRAMES.items():
                for k, count in ((5, 2), (10, 1)):
                    identities.append((name + "_k%d" % k, [(name, sites)] * count, k, 48))
            identities.append(("ng00_k5_1fil", [("ng00", 39885)], 5, 1))
            identities += [("u%d_k5" % n, [("u%d" % n, n)], 5, 48) for n in pilot.UNIFORMES]
            identities.append(("v12set_k5", cases, 5, 48))
            for name, frames, k, threads in identities:
                report["identite"]["prises"].setdefault(name, {})[arm] = take(
                    arm, frames, k, threads, True, 100_000_000)
        for t in range(6):
            frames = large[t:] + large[:t]
            turn = dict(ordre=[n for n, _ in frames])
            turn.update({b: take(b, frames * 2, 5, 48, False, 160_000_000 if b == "apres" else 200_000_000)
                         for b in pilot.BRAS})
            report["grandes"]["tours"].append(turn)
        for name, sites in pilot.TRAMES.items():
            report["ng"]["trames"][name] = [
                {b: take(b, [(name, sites)] * 10, 5, 48, False, 100_000_000) for b in pilot.BRAS}
                for _ in range(5)]

        results = {}

        def judge(label, candidate, expected):
            result = pilot.juger(candidate, str(root), plan=plan)
            need(result["verdict"] == expected, label + ": " + str(result))
            results[label] = dict(verdict=result["verdict"], refus=result["refus"], identite_ok=result["identite_ok"])

        judge("nominal", report, "adopte")
        mutations = {
            "identite_absente": lambda r: r["identite"]["prises"].pop("ng00_k5"),
            "tour_manquant": lambda r: r["grandes"]["tours"].pop(),
            "aa_autre_binaire": lambda r: (r["construction"]["binaires"]["avant_bis"].update(sha256="c" * 64),
                                           r["ng"]["binaires_apres"].update(avant_bis="c" * 64)),
            "code_booleen": lambda r: r["identite"]["prises"]["ng00_k5"]["avant"].update(code=False),
            "resume_identite_altere": lambda r: r["identite"]["prises"]["ng00_k5"]["avant"]["resume"].update(
                empreintes=["cd" * 32] * 2),
            "mauvais_w": lambda r: r["ng"]["trames"]["ng00"][0]["apres"]["attendu"].update(fils=1),
        }
        for name, mutate in mutations.items():
            changed = copy.deepcopy(report)
            mutate(changed)
            judge(name, changed, "refuse")
        p = report["identite"]["prises"]["ng00_k5"]["apres"]
        path = root / p["journal"]
        original = path.read_bytes()
        path.unlink()
        judge("journal_identite_absent", report, "refuse")
        path.write_bytes(original)
        changed = copy.deepcopy(report)
        dest = changed["identite"]["prises"]["ng00_k5"]["apres"]
        path.write_text(original.decode().replace('"full_sha256": "', '"full_sha256": "f', 1))
        dest["journal_sha256"] = pilot.bf.sha256_file(path)
        judge("journal_identite_rehache_invalide", changed, "refuse")
        path.write_bytes(original)
        decision = pilot.decision_chaine(plan)
        need(set(decision) == set(dict(cases)) | set(pilot.TRAMES), "decisions")
        need(all(set(v) == {"sites", "chaine"} for v in decision.values()), "schema diagnostic")
        need(not list(root.glob("*.err")), "fixture sans stderr")
        # Le nominal deja accepte sans aucun stderr : limite du format, pas incident natif.
        return dict(pin=PIN, source_sha256=hashes, journals=journal_id, strict_replay=True,
                    official_selftest_stdout=out.getvalue(), cases=results,
                    diagnostic_fields=["chaine", "sites"],
                    missing_diagnostic_fields=["retard_predit", "rapport_par_trame", "proche_seuil"],
                    individual_stderr_required=False, native_executed=False,
                    qualified_performance=False)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: check.py GIT_REPO")
    print(json.dumps(run(Path(sys.argv[1]).resolve()), indent=2, sort_keys=True, ensure_ascii=False))
