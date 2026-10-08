#!/usr/bin/env python3
"""Contre-JSON A6 : journaux FULL synthetiques, LF et juge livres ; aucun moteur."""
import copy
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
PIN = "30a69104a697fa2ea2dadd8499bde6c7cb8d72c0"
PREFIX = "morsehgp3D_v12/"


def need(ok, why):
    if not ok:
        raise ValueError(why)


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def main():
    repo = Path(sys.argv[1]).resolve() if len(sys.argv) == 2 else HERE
    cap = json.loads((HERE / "capture.json").read_text())
    with tempfile.TemporaryDirectory(prefix="audit-a6-json-") as raw:
        root = Path(raw)
        for rel, sha in cap["sources"].items():
            b = subprocess.check_output(["git", "-C", str(repo), "show", PIN + ":" + PREFIX + rel])
            need(hashlib.sha256(b).hexdigest() == sha, rel)
            p = root / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(b)
        tools = root / "microbancs/outils"
        sys.path.insert(0, str(tools))
        pilot = load("audit_a6", root / "microbancs/mes_t2d_a6/pilote_t2d_a6.py")
        patch = str(HERE / "proposition.patch")
        subprocess.run(["git", "apply", "-p2", "--check", patch], cwd=root, check=True)
        subprocess.run(["git", "apply", "-p2", patch], cwd=root, check=True)
        proposed = load("audit_a6_proposed", root / "microbancs/mes_t2d_a6/pilote_t2d_a6.py")
        fixture = load("audit_full_fixture", tools / "test_lecteur_full.py")
        # Noms/counts artificiels : 37 cas dont exactement 21 >60000, aucun jeu reel.
        cases = [("synthetic_%02d" % i, 70000 if i < 21 else 40000) for i in range(37)]
        large = cases[:21]
        plan = dict(cas=[list(x) for x in cases], fils=48, passes=10, tours=5, tours_grandes=6, essai=False)
        report = {
            "schema": "ehgp.v12.t2d_a6_pilote.v1", "regle": pilot.REGLE_T2D_A6,
            "construction": {"binaires": {b: {"sha256": ("b" if b == "apres" else "a") * 64}
                                            for b in pilot.BRAS}},
            "environnement": {b: {"gpu_apps": ""} for b in ("avant", "apres")},
            "identite": {"prises": {}},
            "grandes": {"trames": [n for n, _ in large], "tours": []},
            "ng": {"passes": 10, "trames": {n: [] for n in pilot.TRAMES},
                   "binaires_apres": {b: ("b" if b == "apres" else "a") * 64 for b in pilot.BRAS}},
        }
        journal_id = 0

        def take(arm, frames, k, threads, digest, wall, cpu=False):
            nonlocal journal_id
            path = root / ("journal_%03d.jsonl" % journal_id)
            journal_id += 1
            expected = dict(voie="cpu" if cpu else "appareil", k=k, fils=threads, passes=len(frames), empreinte=digest,
                            trames=[list(x) for x in frames], budget_appareil="partage", bits=21, schema="recouvert")
            rows = [] if cpu else [dict(phase="open", status="ok", reason="none", wall_ns=5, budget_appareil="partage")]
            for i, (name, sites) in enumerate(frames):
                r = fixture.overlapped_row(i, wall)
                r.update(trame=name, sites=sites, kmax=k, threads=threads, pic_octets=1 << 30,
                         rss_max_octets=2 << 30, pic_appareil_octets=0,
                         memoire_octets=dict(P=[1 << 27, 1 << 28], C=[1 << 28, 1 << 30], tour=[1 << 29, 1 << 30]))
                if cpu:
                    r.update(voie="cpu", appareil_octets=0, epinglee_octets=0)
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
            for name, sites in pilot.TRAMES.items():
                for k, passes in ((5, 2), (10, 1)):
                    report["identite"]["prises"].setdefault(name + "_k%d" % k, {})[arm] = take(
                        arm, [(name, sites)] * passes, k, 48, True, 100_000_000)
            report["identite"]["prises"].setdefault("ng00_k5_1fil", {})[arm] = take(
                arm, [("ng00", 39885)], 5, 1, True, 100_000_000)
            for n in pilot.UNIFORMES:
                report["identite"]["prises"].setdefault("u%d_k5" % n, {})[arm] = take(
                    arm, [("u%d" % n, n)], 5, 48, True, 100_000_000)
            report["identite"]["prises"].setdefault("v12set_k5", {})[arm] = take(
                arm, cases, 5, 48, True, 100_000_000)
        for t in range(6):
            frames = large[t:] + large[:t]
            turn = {"ordre": [n for n, _ in frames]}
            for arm in pilot.BRAS:
                turn[arm] = take(arm, frames * 2, 5, 48, False, 160_000_000 if arm == "apres" else 200_000_000)
            report["grandes"]["tours"].append(turn)
        for name, sites in pilot.TRAMES.items():
            for t in range(5):
                report["ng"]["trames"][name].append({
                    arm: take(arm, [(name, sites)] * 10, 5, 48, False, 100_000_000)
                    for arm in pilot.BRAS})

        # Six auto-tests officiels, puis vrais rejugements verifier=True (pas d'admettre simule).
        with contextlib.redirect_stdout(io.StringIO()):
            need(pilot.etape_auto_test() == 0, "auto-test officiel")
        def verdict(r):
            out = {}
            for label, j in (("livre", pilot.juger(r, str(root))),
                             ("propose", proposed.juger(r, str(root), plan=plan))):
                out[label] = {"verdict": j["verdict"], "refus": j["refus"], "identite_ok": j["identite_ok"]}
            return out

        results = {"nominal": verdict(report)}
        need(all(r["verdict"] == "adopte" for r in results["nominal"].values()), "nominal")
        changed = copy.deepcopy(report)
        changed["identite"]["prises"] = {"ng00_k5": changed["identite"]["prises"]["ng00_k5"]}
        results["identite_une_cle_sur_onze"] = verdict(changed)
        changed = copy.deepcopy(report)
        changed["grandes"]["tours"].pop()
        results["cinq_tours_au_lieu_de_six"] = verdict(changed)
        changed = copy.deepcopy(report)
        for turn in changed["grandes"]["tours"]:
            turn["ordre"] = turn["ordre"][:1]
        results["ordre_une_trame_bruts_42_passes"] = verdict(changed)
        changed = copy.deepcopy(report)
        changed["construction"]["binaires"]["avant_bis"]["sha256"] = "c" * 64
        changed["ng"]["binaires_apres"]["avant_bis"] = "c" * 64
        results["aa_binaire_distinct_de_avant"] = verdict(changed)
        changed = copy.deepcopy(report)
        for p in pilot.prises_du_rapport(changed):
            if p["attendu"]["empreinte"]:
                continue
            path = root / p["journal"]
            rows = [json.loads(x) for x in path.read_text().splitlines()]
            for row in rows:
                if row["phase"] == "full":
                    row["threads"] = 1
            path.write_text("".join(json.dumps(x) + "\n" for x in rows))
            p["attendu"]["fils"] = 1
            p["journal_sha256"] = pilot.bf.sha256_file(path)
            state, why, p["resume"] = pilot.admettre(str(path), 0, p["attendu"])
            need(state == "ok", why)
        results["chronos_w1_au_lieu_de_w48"] = verdict(changed)
        # Controle negatif : le veto statistique A/A existe et rejette bien l'appariement.
        results["veto_aa_103"] = pilot.juger(pilot.synthetique(0.8, {}, aa=1.03), "", verifier=False)["verdict"]
        for name, result in results.items():
            if name == "veto_aa_103":
                need(result == "refuse", name)
            else:
                need(result["livre"]["verdict"] == "adopte", name)
                need(result["propose"]["verdict"] == ("adopte" if name == "nominal" else "refuse"), name)
        campaign_journals = journal_id
        # Mode essai : manifeste37, identite tronquee2, grandes dernieres2, deux uniformes.
        ep = dict(plan, essai=True, fils=1, passes=2, tours=1, tours_grandes=1)
        e = copy.deepcopy(report)
        e["identite"] = {"prises": {}}
        for arm in ("avant", "apres"):
            for name, sites in pilot.TRAMES.items():
                e["identite"]["prises"].setdefault(name + "_k5", {})[arm] = take(
                    arm, [(name, sites)] * 2, 5, 1, True, 100_000_000, cpu=True)
            e["identite"]["prises"].setdefault("ng00_k5_1fil", {})[arm] = take(
                arm, [("ng00", 39885)], 5, 1, True, 100_000_000, cpu=True)
            for n in pilot.UNIFORMES[:2]:
                e["identite"]["prises"].setdefault("u%d_k5" % n, {})[arm] = take(
                    arm, [("u%d" % n, n)], 5, 1, True, 100_000_000, cpu=True)
            e["identite"]["prises"].setdefault("v12set_k5", {})[arm] = take(
                arm, cases[:2], 5, 1, True, 100_000_000, cpu=True)
        frames = large[-2:]
        turn = {"ordre": [n for n, _ in frames]}
        for arm in pilot.BRAS:
            turn[arm] = take(arm, frames * 2, 5, 1, False, 160_000_000 if arm == "apres" else 200_000_000, cpu=True)
        e["grandes"] = {"trames": [n for n, _ in frames], "tours": [turn]}
        e["ng"]["passes"] = 2
        for name, sites in pilot.TRAMES.items():
            e["ng"]["trames"][name] = [{arm: take(arm, [(name, sites)] * 2, 5, 1, False, 100_000_000, cpu=True)
                                        for arm in pilot.BRAS}]
        need(proposed.verifier_cohorte(e, ep, True) == "", "cohorte essai")
        archive = root / "synthetic_manifest_only.tar"
        content = json.dumps({"cases": [{"name": n, "count": s} for n, s in cases]}).encode()
        with tarfile.open(archive, "w") as tar:
            info = tarfile.TarInfo("bundle_manifest.json")
            info.size = len(content)
            tar.addfile(info, io.BytesIO(content))
        args = SimpleNamespace(archive_v12set=str(archive), fils=1, passes=2, tours=1, tours_grandes=1,
                               essai=True, sortie=str(root))
        need(proposed.plan_demande(args) == ep, "lecture manifeste37 essai")
        old_essai = pilot.etape_rapport(args, copy.deepcopy(e))["verdict"]
        new_essai = proposed.etape_rapport(args, copy.deepcopy(e))["verdict"]
        need(old_essai == "adopte" and new_essai == "essai", "verdict essai")
        need("**essai**" in (root / "tableaux_t2d_a6.md").read_text(), "tableaux essai")
        args.archive_v12set = None
        try:
            proposed.plan_demande(args)
        except RuntimeError:
            missing_manifest_refused = True
        else:
            missing_manifest_refused = False
        need(missing_manifest_refused, "rapport sans manifeste")
        out = {"source": PIN, "journaux_synthetiques": campaign_journals, "nominal_identites": 11,
               "nominal_grandes": 21, "nominal_tours_grandes": 6, "nominal_tours_ng": 5,
               "official_selftests": 6, "verifier_true": True, "results": results,
               "native_executed": False, "essai": {"journaux": journal_id - campaign_journals,
                   "identites": len(e["identite"]["prises"]), "manifest_cases": 37,
                   "identity_frames": 2, "large_frames": 2, "uniformes": 2,
                   "ng00_1fil_passes": 1, "old_table_verdict": old_essai, "proposed_table_verdict": new_essai,
                   "missing_manifest_refused": missing_manifest_refused}}
        if "result" in cap:
            need(out == cap["result"], "resultat capture different")
        print(json.dumps(out, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
