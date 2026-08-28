#!/usr/bin/env python3
"""Porte LOCALE de la phase SCALE_THREADS de la campagne v5 (P0 de l'audit
« rendement GPU et multi-CPU » du 28 aout 2026) : le validateur
`gcp-migration/validate_v5_campaign.py` est juge sur une FIXTURE DE SORTIE
SYNTHETIQUE (plan annonce, topologie, statuts, sorties, GNU time), sans une
seule commande gcloud ni binaire compile. Complement statique : syntaxe des
scripts, propagation des variables SCALE_* par la session, phase strictement
optionnelle et CPU dans le script distant.

Le validateur ne conclut jamais sur une acceleration : ce test verifie aussi
que le tableau resume ne contient aucune conclusion de speedup.
"""

from __future__ import annotations

import importlib.util
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
REMOTE = ROOT / "gcp-migration" / "v5_campaign_remote.sh"
SESSION = ROOT / "gcp-migration" / "session_campagne_v5_scale_g4.sh"
SELFTEST = ROOT / "gcp-migration" / "selftest_campagne_v5.sh"
VALIDATOR = ROOT / "gcp-migration" / "validate_v5_campaign.py"

DIGEST_ALL = "0123456789abcdef" * 4
GENERATION = (
    "generation rect_alive=7379/14563/15374 ancres=11990/47282/53317 "
    "candidats=10982/32163/23942 tues_profondeur=0/388726/103000 ancres_w4=22860 "
    "ancres_w3=19562 ancres_secteurs=3061/5557 ancres_cellules=0/459 "
    "seeds_cellules=0/2382 grilles=0/536 seeds_core_tues=157009 "
    "seeds_corde_tues=130572 float_cert=7389726/6171068 repli=174685 "
    "jung=2255407/1473815/0"
)
PIN = ("cafedeca", "beefbeef", "feedf00d")


def load_validator():
    spec = importlib.util.spec_from_file_location("validate_v5_campaign", VALIDATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def probe_output(family: str, n: str, threads: str, inflight: str, digest: bool,
                 mur_ms: float, generation: str = GENERATION,
                 digest_all: str = DIGEST_ALL, cards_scale: int = 1) -> str:
    lines = [
        "payload=mhgp5-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
        "backend=cpu_reference",
        "profil=complet_k10",
        f"famille={family} n={n} coord=147 s=8 smax=11 seed=3 threads={threads} emis=67087 "
        "boules_uniques=67077 mortes_profondeur=2307 survivantes=64770 census_int=317555 "
        "census_shell=210051 evenements=65247 facettes=394430 fusions=394259 deltas=63290 noeuds=44334",
        generation,
        f"ouvriers wspd={threads}/{threads}/{threads} rects={threads}/{threads}/{threads} rle={threads} "
        f"prefiltre={threads} census={threads} expansion={threads} fold={threads}",
        "temps_ms index=0.1 gen=549.2 (wspd 37.8/71.3/69.2 rects 3.4/126.5/241.2) rle=7.1 prefiltre=170.6 "
        "census=139.9 comptage=0.5 expansion=14.4 fold=194.4 (tri 23.7 intern 87.6 fusion 36.7 reduce 46.3) digest=61.3",
        f"temps_mur_ms={mur_ms:.1f} (etages A et B du fold pipelines : fold+digest ci-dessus sont des cumuls par etage, pas le mur)",
        f"temps_fold_mur_ms=190.2 (etages A et B, fold_inflight={inflight}, pic_mesure_en_vol=1)",
        "rss_mb apres_generation=22 apres_rle=22 apres_prefiltre=24 apres_census=46 max_fold=62 fin=48",
    ]
    for k in range(1, 11):
        lines.append(
            f"cardinalites K={k} evenements={k * 100 * cards_scale} facettes={k * 300} deltas={k * 90} "
            f"attachements={k * 50} fusions={k * 299} noeuds={k * 40}"
        )
    if digest:
        lines.append("digest_balls=" + "abcdef0123456789" * 4)
        for k in range(1, 11):
            lines.append(f"digest_forest_K{k}=" + f"{k:064d}")
        lines.append(f"digest_all={digest_all}")
    lines.append("rss_max_kb=62688")
    return "\n".join(lines) + "\n"


class ScaleFixture:
    """Fixture synthetique d'une phase SCALE_THREADS complete."""

    def __init__(self, out: Path, threads=("1", "2"), families=("eight_clusters",), n="400",
                 inflight=("1", "2"), digest=("0", "1"), repeats=1):
        self.out = out
        self.params = {
            "threads_list": " ".join(threads), "families": " ".join(families), "n": n,
            "inflight_list": " ".join(inflight), "digest_list": " ".join(digest),
            "repeats": str(repeats),
        }
        self.runs = []
        seq = 0
        for r in range(1, repeats + 1):
            order = list(threads) if r % 2 == 1 else list(reversed(threads))
            for fam in families:
                for infl in inflight:
                    for dig in digest:
                        for t in order:
                            seq += 1
                            self.runs.append({
                                "seq": str(seq), "name": f"scale_{fam}_n{n}_t{t}_f{infl}_d{dig}_r{r}",
                                "family": fam, "threads": t, "inflight": infl, "digest": dig, "repeat": str(r)})

    def write(self, *, mutate=None):
        out = self.out
        out.mkdir(parents=True, exist_ok=True)
        plan = ["scale_threads_plan=v1"]
        plan += [f"{k}={v}" for k, v in self.params.items()]
        plan += ["run_timeout_s=60", "s=8 smax=11 seed=3"]
        for run in self.runs:
            plan.append(
                f"seq={run['seq']} name={run['name']} family={run['family']} threads={run['threads']} "
                f"inflight={run['inflight']} digest={run['digest']} repeat={run['repeat']}")
        plan.append(f"runs={len(self.runs)}")
        (out / "scale_threads_plan.txt").write_text("\n".join(plan) + "\n", encoding="utf-8")
        (out / "topologie.txt").write_text(
            "nproc=8\naffinite_runner=0-7\ndate_utc=2026-08-28T00:00:00Z\nuname=Linux x86_64\n"
            "MemTotal:       32868864 kB\n--- lscpu ---\nArchitecture: x86_64\nCPU(s): 8\n"
            "--- affinite (taskset -pc) ---\npid 1's current affinity list: 0-7\n", encoding="utf-8")
        for run in self.runs:
            t = int(run["threads"])
            mur = 2000.0 / t + 10.0 * int(run["repeat"])
            kwargs = {}
            if mutate is not None:
                kwargs = mutate(run) or {}
            body = probe_output(run["family"], self.params["n"], run["threads"], run["inflight"],
                                run["digest"] == "1", mur, **kwargs)
            (out / f"{run['name']}.txt").write_text(body, encoding="utf-8")
            cmd = (f"./build/mhgp5 --family={run['family']} --n={self.params['n']} --s=8 --smax=11 --seed=3 "
                   f"--threads={run['threads']} --fold-inflight={run['inflight']}"
                   + (" --digest" if run["digest"] == "1" else ""))
            status = "\n".join([
                "code=0", f"duree_s={int(mur / 1000) + 1}", "peak_rss_kb=300000", "timing_scope=scale_threads",
                f"threads={run['threads']}", f"fold_inflight={run['inflight']}", f"digest={run['digest']}",
                f"family={run['family']}", f"n={self.params['n']}", f"repeat={run['repeat']}",
                f"seq={run['seq']}", f"commande={cmd}", f"source_commit={PIN[0]}",
                f"source_payload_sha256={PIN[1]}", f"protocol_manifest_sha256={PIN[2]}", "finished=1"]) + "\n"
            (out / f"{run['name']}.status").write_text(status, encoding="utf-8")
            (out / f"{run['name']}.status.time").write_text(
                f"\tCommand being timed: \"{cmd}\"\n\tElapsed (wall clock) time (h:mm:ss or m:ss): 0:02.00\n"
                "\tMaximum resident set size (kbytes): 300000\n\tExit status: 0\n", encoding="utf-8")
        return self


class ScaleThreadsValidatorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp(prefix="v5-scale-threads-"))
        self.addCleanup(shutil.rmtree, self.tmp, True)
        self.v = load_validator()

    def judge(self, out: Path):
        bad = []
        params, runs = self.v.read_scale_plan(str(out), bad)
        measures = None
        if params is not None:
            measures = self.v.check_scale_runs(str(out), params, runs, *PIN, bad)
        return bad, params, runs, measures

    def test_counterbalanced_sequence_is_recomputed_exactly(self) -> None:
        seq = self.v.scale_sequence({
            "threads_list": "1 2 4", "families": "eight_clusters scanline_single_pass", "n": "50000",
            "inflight_list": "1 3", "digest_list": "0 1", "repeats": "2"})
        self.assertEqual(len(seq), 3 * 2 * 2 * 2 * 2)
        first_block = [r["threads"] for r in seq[:3]]
        second_repeat_first_block = [r["threads"] for r in seq if r["repeat"] == "2"][:3]
        self.assertEqual(first_block, ["1", "2", "4"])
        self.assertEqual(second_repeat_first_block, ["4", "2", "1"])
        self.assertEqual(seq[0]["name"], "scale_eight_clusters_n50000_t1_f1_d0_r1")
        self.assertEqual(seq[-1]["name"], "scale_scanline_single_pass_n50000_t1_f3_d1_r2")
        self.assertEqual([r["seq"] for r in seq], [str(i) for i in range(1, len(seq) + 1)])

    def test_complete_fixture_is_accepted_and_resume_has_no_speedup_claim(self) -> None:
        fx = ScaleFixture(self.tmp / "out", repeats=2).write()
        bad, params, runs, measures = self.judge(fx.out)
        self.assertEqual(bad, [])
        self.assertEqual(len(runs), 16)
        self.v.write_scale_resume(str(fx.out), params, runs, measures)
        resume = (fx.out / "scale_threads_resume.txt").read_text(encoding="utf-8")
        rows = [l for l in resume.splitlines() if not l.startswith("#")]
        self.assertEqual(rows[0].split("\t")[:5], ["famille", "fils", "inflight", "digest", "runs"])
        self.assertEqual(len(rows), 1 + 8)
        # (eight_clusters, 2 fils, inflight 1, digest 0) : deux repetitions, murs 1010 et 1020 -> mediane 1015.
        row = [r for r in rows[1:] if r.startswith("eight_clusters\t2\t1\t0\t")][0].split("\t")
        self.assertEqual(row[4:8], ["2", "1015.0", "1010.0", "1020.0"])
        self.assertEqual(row[9], "300000")
        self.assertIn("Aucune conclusion de speedup", resume)
        for word in ("speedup", "accelerat", "gain", "lineaire", "efficacite"):
            self.assertNotIn(word, "\n".join(rows).lower())

    def test_missing_announced_run_is_named(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        (fx.out / "scale_eight_clusters_n400_t2_f1_d1_r1.status").unlink()
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("scale_eight_clusters_n400_t2_f1_d1_r1: .status ABSENT" in b for b in bad), bad)

    def test_digest_drift_across_threads_is_refused(self) -> None:
        def mutate(run):
            return {"digest_all": "f" * 64} if run["threads"] == "2" else {}
        fx = ScaleFixture(self.tmp / "out").write(mutate=mutate)
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("eight_clusters: digest_all DIFFERENT" in b for b in bad), bad)

    def test_generation_counters_drift_is_refused(self) -> None:
        def mutate(run):
            return {"generation": GENERATION.replace("ancres_w4=22860", "ancres_w4=22861")} \
                if run["inflight"] == "2" else {}
        fx = ScaleFixture(self.tmp / "out").write(mutate=mutate)
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("eight_clusters: ligne generation DIFFERENT" in b for b in bad), bad)

    def test_cardinalities_drift_is_refused(self) -> None:
        def mutate(run):
            return {"cards_scale": 2} if run["threads"] == "1" else {}
        fx = ScaleFixture(self.tmp / "out").write(mutate=mutate)
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("eight_clusters: cardinalites DIFFERENT" in b for b in bad), bad)

    def test_inflight_not_honored_by_probe_is_refused(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        p = fx.out / "scale_eight_clusters_n400_t1_f2_d0_r1.txt"
        p.write_text(p.read_text(encoding="utf-8").replace("fold_inflight=2,", "fold_inflight=1,"), encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("fold_inflight imprime 1 != 2 demande" in b for b in bad), bad)

    def test_legacy_fold_line_format_is_still_accepted(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        for p in fx.out.glob("scale_*.txt"):
            body = p.read_text(encoding="utf-8")
            body = re.sub(r"fold_inflight=(\d+), pic_mesure_en_vol=\d+", r"\1 ordre(s) en vol", body)
            p.write_text(body, encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertEqual(bad, [])

    def test_digest_printed_without_flag_is_refused(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        p = fx.out / "scale_eight_clusters_n400_t1_f1_d0_r1.txt"
        p.write_text(p.read_text(encoding="utf-8") + f"digest_all={DIGEST_ALL}\n", encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("digest imprime alors que digest=0" in b for b in bad), bad)

    def test_status_threads_and_command_must_match_the_announced_run(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        p = fx.out / "scale_eight_clusters_n400_t2_f1_d0_r1.status"
        p.write_text(p.read_text(encoding="utf-8").replace("threads=2\n", "threads=1\n")
                     .replace("--threads=2", "--threads=1"), encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("threads=1 != 2 (annonce)" in b for b in bad), bad)
        self.assertTrue(any("commande gravee sans les arguments contractuels" in b for b in bad), bad)

    def test_gpu_command_is_refused_on_cpu_campaign(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        p = fx.out / "scale_eight_clusters_n400_t2_f1_d0_r1.status"
        p.write_text(p.read_text(encoding="utf-8").replace("--seed=3", "--seed=3 --gpu"), encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("campagne CPU (--gpu refuse)" in b for b in bad), bad)

    def test_plan_reordered_is_not_counterbalanced(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        p = fx.out / "scale_threads_plan.txt"
        lines = p.read_text(encoding="utf-8").splitlines()
        i = [k for k, l in enumerate(lines) if l.startswith("seq=1 ")][0]
        lines[i], lines[i + 1] = (lines[i + 1].replace("seq=2 ", "seq=1 "), lines[i].replace("seq=1 ", "seq=2 "))
        p.write_text("\n".join(lines) + "\n", encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("sequence annoncee != sequence contrebalancee" in b for b in bad), bad)

    def test_plan_run_count_must_match(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        p = fx.out / "scale_threads_plan.txt"
        p.write_text(p.read_text(encoding="utf-8").replace("runs=8", "runs=7"), encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("runs=7 != 8 runs recalcules" in b for b in bad), bad)

    def test_topology_and_gnu_time_are_required(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        (fx.out / "topologie.txt").unlink()
        (fx.out / "scale_eight_clusters_n400_t1_f1_d0_r1.status.time").unlink()
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("topologie.txt: ABSENT" in b for b in bad), bad)
        self.assertTrue(any("sortie complete de GNU time absente" in b for b in bad), bad)

    def test_nonzero_code_and_forbidden_pattern_are_refused(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        st = fx.out / "scale_eight_clusters_n400_t1_f2_d1_r1.status"
        st.write_text(st.read_text(encoding="utf-8").replace("code=0", "code=124"), encoding="utf-8")
        tx = fx.out / "scale_eight_clusters_n400_t2_f2_d1_r1.txt"
        tx.write_text(tx.read_text(encoding="utf-8") + "REFUS invariant\n", encoding="utf-8")
        bad, *_ = self.judge(fx.out)
        self.assertTrue(any("scale_eight_clusters_n400_t1_f2_d1_r1: code=124" in b for b in bad), bad)
        self.assertTrue(any("scale_eight_clusters_n400_t2_f2_d1_r1: motif interdit" in b for b in bad), bad)

    def test_main_treats_scale_runs_without_plan_as_unexpected(self) -> None:
        fx = ScaleFixture(self.tmp / "out").write()
        (fx.out / "scale_threads_plan.txt").unlink()
        result = subprocess.run(
            [sys.executable, str(VALIDATOR), str(fx.out), *PIN, "0", "0"],
            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
        self.assertEqual(result.returncode, 1)
        self.assertIn("campaign_status=partial_or_failed", result.stdout)
        self.assertIn("scale_eight_clusters_n400_t1_f1_d0_r1.txt: fichier inattendu", result.stdout)
        # Les fichiers auxiliaires de la phase ne sont jamais des runs inattendus.
        self.assertNotIn("topologie.txt: fichier inattendu", result.stdout)


class ScaleThreadsScriptsStaticTests(unittest.TestCase):
    def test_shell_entrypoints_remain_syntactically_valid(self) -> None:
        subprocess.run(["bash", "-n", str(REMOTE), str(SESSION), str(SELFTEST)], cwd=ROOT, check=True)

    def test_session_forwards_every_scale_variable_and_nothing_else_changes(self) -> None:
        script = SESSION.read_text(encoding="utf-8")
        for var in ("SCALE_THREADS", "SCALE_FAMILIES", "SCALE_N", "SCALE_INFLIGHT",
                    "SCALE_DIGEST", "SCALE_REPEATS", "SCALE_RUN_TIMEOUT"):
            self.assertIn(f"{var}='${{{var}:-}}'", script)
        self.assertIn("bash gcp-migration/v5_campaign_remote.sh", script)
        self.assertIn("./gcp-migration/stop_and_verify.sh --yes", script)

    def test_remote_phase_is_optional_cpu_only_and_counterbalanced(self) -> None:
        script = REMOTE.read_text(encoding="utf-8")
        for required in (
            'SCALE_THREADS="${SCALE_THREADS:-}"',
            'SCALE_FAMILIES="${SCALE_FAMILIES:-eight_clusters scanline_single_pass}"',
            'SCALE_N="${SCALE_N:-50000}"',
            'SCALE_INFLIGHT="${SCALE_INFLIGHT:-1 2 3}"',
            'SCALE_DIGEST="${SCALE_DIGEST:-0 1}"',
            'SCALE_REPEATS="${SCALE_REPEATS:-2}"',
            'if [ -n "${SCALE_THREADS}" ]; then',
            "${OUT_DIR}/topologie.txt",
            "scale_threads_plan.txt",
            "SCALE_THREADS_REV",
            'if [ $((r % 2)) -eq 1 ]; then order="${SCALE_THREADS_FWD}"; else order="${SCALE_THREADS_REV}"; fi',
            '"--threads=${t}" "--fold-inflight=${infl}"',
            "printf 'commande=%s\\n' \"$*\"",
            "run_one \"${name}\" scale_threads",
        ):
            self.assertIn(required, script)
        self.assertNotIn("gcloud", script)
        phase = script[script.index("# PHASE 4 (optionnelle"):]
        code = "\n".join(l for l in phase.splitlines() if not l.lstrip().startswith("#"))
        self.assertNotIn("--gpu", code)
        self.assertNotIn("GPU_BIN", code)

    def test_validator_never_concludes_on_speedup(self) -> None:
        source = VALIDATOR.read_text(encoding="utf-8")
        self.assertIn("Aucune conclusion de speedup", source)
        for word in ("speedup=", "acceleration=", "efficacite="):
            self.assertNotIn(word, source)


if __name__ == "__main__":
    unittest.main()
