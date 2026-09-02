#!/usr/bin/env python3
"""Porte de VACUITE du lanceur et de l'agregateur de la sonde d'ablation du
reduce (bench/sonde_ablation_reduce.sh, bench/sonde_ablation_reduce.py) —
fermeture minimale exigee par audits/ALERTE_SONDE_ABLATION_REDUCE_20260902.md.

Un FAUX mhgp6_profile_sonde (script bash, < 50 ms par appel) accepte les
memes options, imprime dix lignes `profil_reduce K=1..10` conformes
(fenetres finies, somme = somme des composantes, mur_reduce_interne,
residuel), `temps_mur_ms=`, `rss_max_kb=`, et refuse un --inject= inconnu
(code 2). Ses mutants sont pilotes par l'environnement (FAKE_COMPTEUR,
FAKE_MUTANT, FAKE_MUTANT_AT) : deterministes, appliques au n-ieme appel.

Scenes :
  (a) nominal REPS=4, N_LIST="64 128" : reçu publie (plus de .partial),
      resume.txt non vide et REPRODUCTIBLE bit a bit depuis la copie
      archivee, plan.txt equilibre (chaque bras une fois par position),
      copie privee immuable au sha256 du META, HASHES avant==apres,
      SHA256SUMS verifie et couvrant exactement les fichiers, libelle
      exploratory_noncausal_upper_bounds, borne composite etiquetee,
      differences appariees exactes ; dossier preexistant => refus 2 ;
  (b) REPS=3 => refus 2 ; (c) N_LIST vide => refus 2 ; (d) REPS=0, -4,
      abc => refus 2 ;
  (e) copie privee alteree pendant un tuple => INVALIDE 3, reste en
      .partial, arret immediat (trois statuts) ;
  (f) ligne K=7 omise sur un run => INVALIDE 3 (agregateur), sans manifeste ;
  (g) touch=nan sur un run => INVALIDE 3 (agregateur) ;
  (h) alteration d'un fichier entre la generation de SHA256SUMS et sa
      verification (faux sha256sum en PATH n'agissant que sur -c) =>
      INVALIDE 3, jamais publie ;
  (i) binaire PRODUIT (build/v6/mhgp6_profile s'il existe, sinon un faux
      refusant --inject=) => refus 2, aucun .partial laisse.

Usage : sonde_ablation_gate.py <dossier bench>. Codes : 0 conforme ;
1 au moins un controle en echec. Aucun assert (python3 -O).
"""

import hashlib
import os
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path

BRAS = ("aucune", "ablation-mat-sans-copie", "ablation-mat-sans-tris",
        "ablation-post-cle-factice")

FAUX_SONDE = r"""#!/usr/bin/env bash
# Faux mhgp6_profile_sonde de la porte : memes options, dix lignes
# profil_reduce conformes, refus 2 d'un --inject= inconnu, rapide.
set -u
inject=""
n=0
for a in "$@"; do
  case "$a" in
    --inject=*) inject="${a#--inject=}" ;;
    --n=*) n="${a#--n=}" ;;
    --family=*|--s=*|--smax=*|--seed=*|--threads=*|--fold-inflight=*|--fold-join=*) ;;
    *) echo "argument inconnu : $a" >&2; exit 2 ;;
  esac
done
c=0
if [ -n "${FAKE_COMPTEUR:-}" ]; then
  c=$(cat "${FAKE_COMPTEUR}" 2>/dev/null || echo 0)
  c=$((c + 1))
  echo "$c" > "${FAKE_COMPTEUR}"
fi
case "$inject" in
  ""|ablation-mat-sans-copie|ablation-mat-sans-tris|ablation-post-cle-factice) ;;
  *) echo "REFUS : arguments (mutant inconnu ${inject})" >&2; exit 2 ;;
esac
mutant=""
if [ -n "${FAKE_MUTANT:-}" ] && [ "$c" = "${FAKE_MUTANT_AT:-0}" ]; then
  mutant="${FAKE_MUTANT}"
fi
if [ "$mutant" = corrompre_copie ]; then
  chmod 755 "$0" && echo "# altere" >> "$0"
fi
echo "profil_kind=reduce_v2 fold_join=1 inflight_demande=2 pic_workers_b=1 pic_reduce_actif=1"
mat_f=6
post_f=4
case "$inject" in
  ablation-mat-sans-copie) mat_f=3 ;;
  ablation-mat-sans-tris) mat_f=4 ;;
  ablation-post-cle-factice) mat_f=5; post_f=1 ;;
esac
u=$((n / 64))
for k in 1 2 3 4 5 6 7 8 9 10; do
  if [ "$mutant" = omettre_k7 ] && [ "$k" -eq 7 ]; then continue; fi
  init=$((k * u)); tch=$((2 * k * u)); pre=$((3 * k * u)); unite=$((k * u))
  post=$((post_f * k * u)); mat=$((mat_f * k * u)); part=$((k * u)); lib=0
  somme=$((init + tch + pre + unite + post + mat + part + lib))
  tch_s="${tch}.000"
  [ "$mutant" = touch_nan ] && tch_s=nan
  printf 'profil_reduce K=%d init=%d.000 touch=%s pre=%d.000 unite=%d.000 post_remplissage=%d.000 materialisation_tri_copie=%d.000 liveness=0.000 partition=%d.000 liberation=%d.000 somme=%d.000 mur_reduce_interne=%d.000 residuel=0.000 reduce_interne_debut=0.000 reduce_interne_fin=%d.000 a_debut=0.000 a_fin=0.000 duree_digest_foret_k_ms=0.000\n' \
    "$k" "$init" "$tch_s" "$pre" "$unite" "$post" "$mat" "$part" "$lib" "$somme" "$somme" "$somme"
done
echo "temps_mur_ms=$((100 * u)).0 (faux binaire de porte)"
echo "rss_max_kb=$((1000 * u))"
exit 0
"""

FAUX_PRODUIT = """#!/usr/bin/env bash
# Faux binaire PRODUIT : --inject= est un argument inconnu (code 2).
for a in "$@"; do
  case "$a" in --inject=*) echo "argument inconnu : $a" >&2; exit 2 ;; esac
done
echo "temps_mur_ms=1.0"
exit 0
"""

# Mutant (h) : n'agit que sur `sha256sum -c` (cwd = dossier du reçu) et
# altere META.txt APRES la generation du manifeste, AVANT sa verification.
FAUX_SHA256SUM = """#!/usr/bin/env bash
if [ "${1:-}" = "-c" ]; then
  echo "# alteration tardive (mutant de porte)" >> META.txt
fi
exec %REAL% "$@"
"""


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for bloc in iter(lambda: f.read(1 << 16), b""):
            h.update(bloc)
    return h.hexdigest()


def ecrire_exec(path, contenu):
    path.write_text(contenu, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR | stat.S_IRUSR)


def lire_kv(path):
    kv = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" in line and not line.startswith("#"):
            k, v = line.split("=", 1)
            kv.setdefault(k, v)
    return kv


class Porte:
    def __init__(self, bench):
        self.lanceur = bench / "sonde_ablation_reduce.sh"
        self.echecs = 0
        self.scenes = 0
        self.cpu = str(min(os.sched_getaffinity(0)))

    def check(self, nom, ok, detail=""):
        if not ok:
            self.echecs += 1
            print(f"ECHEC porte : {nom} {detail}", file=sys.stderr)

    def scene(self, nom):
        self.scenes += 1
        print(f"scene {self.scenes} : {nom}")

    def lancer(self, out, binaire, n_list, reps, env_extra=None):
        env = dict(os.environ, THREADS="1", CPUS=self.cpu, FAMILY="uniform")
        if env_extra:
            env.update(env_extra)
        argv = ["bash", str(self.lanceur), str(out), str(binaire)]
        if n_list is not None:
            argv.append(n_list)
        if reps is not None:
            argv.append(reps)
        return subprocess.run(argv, capture_output=True, text=True, env=env)

    def env_mutant(self, base, mutant, appel):
        compteur = base / f"compteur_{mutant}"
        if compteur.exists():
            compteur.unlink()
        return {"FAKE_COMPTEUR": str(compteur), "FAKE_MUTANT": mutant,
                "FAKE_MUTANT_AT": str(appel)}


def scene_nominal(p, base, faux):
    p.scene("(a) nominal REPS=4 N_LIST='64 128' : reçu publie et fail-closed verifie")
    out = base / "nominal"
    partial = Path(str(out) + ".partial")
    r = p.lancer(out, faux, "64 128", "4")
    p.check("nominal : rc=0", r.returncode == 0, f"rc={r.returncode} err={r.stderr[-300:]!r}")
    p.check("nominal : publie (plus de .partial)", out.is_dir() and not partial.exists())
    if not out.is_dir():
        return
    meta = lire_kv(out / "META.txt")
    p.check("nominal : statut exploratory_noncausal_upper_bounds",
            meta.get("statut", "").startswith("exploratory_noncausal_upper_bounds"))
    p.check("nominal : schema v2", meta.get("schema") == "e-hgp.sonde-ablation-reduce.v2")
    p.check("nominal : etiquette borne composite au META",
            "borne composite (lecture keys[] + tri de cles egales)"
            in meta.get("etiquette_ablation-post-cle-factice", ""))
    copie = out / "bin" / "mhgp6_profile_sonde"
    p.check("nominal : copie privee presente", copie.is_file())
    if copie.is_file():
        p.check("nominal : sha256 de la copie == META", sha256(copie) == meta.get("binaire_sha256"))
        p.check("nominal : copie immuable (aucun bit d'ecriture)",
                copie.stat().st_mode & 0o222 == 0)
        p.check("nominal : copie == source", sha256(copie) == sha256(faux))
    # Plan equilibre : chaque bras une fois par position sur les quatre blocs.
    plan = {}
    for line in (out / "plan.txt").read_text(encoding="utf-8").splitlines():
        if line.startswith("#") or not line.strip():
            continue
        kv = dict(t.split("=", 1) for t in line.split())
        plan[(int(kv["bloc"]), int(kv["position"]))] = kv["bras"]
    p.check("plan : 16 cases (4 blocs x 4 positions)", len(plan) == 16, f"{len(plan)}")
    for pos in range(1, 5):
        p.check(f"plan : position {pos} occupee une fois par chaque bras",
                sorted(plan.get((b, pos), "?") for b in range(1, 5)) == sorted(BRAS))
    for bloc in range(1, 5):
        p.check(f"plan : bloc {bloc} = quatre bras distincts",
                sorted(plan.get((bloc, pos), "?") for pos in range(1, 5)) == sorted(BRAS))
    # Ensemble exact des sorties, codes nuls, positions du plan executees.
    statuts = sorted((out / "out").glob("*.status"))
    p.check("nominal : 32 statuts (4 bras x 2 tailles x 4 blocs)", len(statuts) == 32,
            f"{len(statuts)}")
    p.check("nominal : 32 sorties .txt", len(list((out / "out").glob("*.txt"))) == 32)
    ok_codes = ok_pos = True
    for st in statuts:
        kv = dict(t.split("=", 1) for t in st.read_text(encoding="utf-8").splitlines()[0].split())
        ok_codes = ok_codes and kv.get("code") == "0"
        ok_pos = ok_pos and plan.get((int(kv["rep"]), int(kv["position"]))) == kv["ablation"]
    p.check("nominal : tous les codes nuls", ok_codes)
    p.check("nominal : chaque statut a la position du plan", ok_pos)
    hashes = (out / "HASHES.txt").read_text(encoding="utf-8").splitlines()
    p.check("nominal : 32 lignes HASHES avant==apres==META",
            len(hashes) == 32 and all(
                h.split()[1][6:] == h.split()[2][6:] == meta.get("binaire_sha256") for h in hashes))
    p.check("nominal : runs_effectues=32 au META", meta.get("runs_effectues") == "32")
    # Worktree : coherence du libelle et du patch embarque.
    if meta.get("worktree_sources_modifies") == "0":
        p.check("nominal : worktree propre => worktree_diff=aucun",
                meta.get("worktree_diff") == "aucun" and not (out / "worktree_diff.patch").exists())
    else:
        p.check("nominal : worktree modifie => patch et resume embarques",
                (out / "worktree_diff.patch").is_file()
                and (out / "worktree_diff_summary.txt").is_file()
                and meta.get("worktree_diff", "").startswith("worktree_diff.patch"))
    # Resume non vide, borne composite etiquetee, differences appariees exactes.
    resume = (out / "resume.txt").read_text(encoding="utf-8")
    p.check("nominal : resume.txt non vide", len(resume) > 0)
    p.check("nominal : resume.err vide", (out / "resume.err").stat().st_size == 0)
    p.check("nominal : borne composite etiquetee dans le resume",
            "ablation-post-cle-factice (borne composite (lecture keys[] + tri de cles egales))"
            in resume)
    p.check("nominal : libelle non causal dans le resume",
            "statut=exploratory_noncausal_upper_bounds" in resume and "NON CAUSALES" in resume)
    # n=64, K=8 : materialisation_tri_copie temoin 48, sans-copie 24 => d=-24.0 sur chaque bloc.
    lignes = resume.splitlines()
    try:
        i64 = lignes.index("## n=64")
        i128 = lignes.index("## n=128")
        bloc64 = lignes[i64:i128]
        ik8 = next(i for i, l in enumerate(bloc64) if l.startswith("K=8\t"))
        ligne_copie = next(l for l in bloc64[ik8:ik8 + 5]
                           if l.startswith("ablation-mat-sans-copie"))
        cols = ligne_copie.split("\t")
        p.check("nominal : difference appariee K=8 materialisation_tri_copie = -24.0 [-24.0;-24.0]",
                cols[6] == "-24.0 [-24.0;-24.0]", repr(cols[6]))
        isig = next(i for i, l in enumerate(bloc64) if l.startswith("Σ_K\t"))
        ligne_sig = next(l for l in bloc64[isig:isig + 5]
                         if l.startswith("ablation-mat-sans-copie"))
        col_sig = ligne_sig.split("\t")[6]
        p.check("nominal : difference appariee Σ_K materialisation_tri_copie = -165.0",
                col_sig == "-165.0 [-165.0;-165.0]", repr(col_sig))
        p.check("nominal : part appariee Σ_K sans-copie = +50.0 %",
                any("materialisation_tri_copie +50.0 [+50.0;+50.0] %" in l
                    and l.strip().startswith("ablation-mat-sans-copie") for l in bloc64))
    except (ValueError, StopIteration, IndexError) as e:
        p.check("nominal : structure du resume (n=64, K=8, Σ_K)", False, repr(e))
    # Manifeste : verifie, couvrant exactement les fichiers, dernier ecrit.
    v = subprocess.run(["sha256sum", "-c", "--quiet", "--strict", "SHA256SUMS"],
                       cwd=out, capture_output=True, text=True)
    p.check("nominal : sha256sum -c --strict vert", v.returncode == 0, v.stderr[-200:])
    fichiers = {str(f.relative_to(out)) for f in out.rglob("*") if f.is_file()} - {"SHA256SUMS"}
    entrees = {l.split("  ", 1)[1]
               for l in (out / "SHA256SUMS").read_text(encoding="utf-8").splitlines()}
    p.check("nominal : SHA256SUMS couvre exactement les autres fichiers", fichiers == entrees,
            f"manque={sorted(fichiers - entrees)[:3]} excedent={sorted(entrees - fichiers)[:3]}")
    p.check("nominal : aucune entree speciale ni lien",
            all((f.is_file() or f.is_dir()) and not f.is_symlink() for f in out.rglob("*")))
    # Reproductibilite bit a bit depuis la copie archivee de l'agregateur.
    a = subprocess.run([sys.executable, str(out / "protocole_agregateur.py"), str(out)],
                       capture_output=True, text=True)
    p.check("nominal : resume reproductible bit a bit (copie archivee)",
            a.returncode == 0 and a.stdout == resume)
    p.check("nominal : lanceur archive == lanceur courant",
            sha256(out / "protocole_lanceur.sh") == sha256(p.lanceur))
    # Dossier preexistant : refus 2.
    r2 = p.lancer(out, faux, "64", "4")
    p.check("dossier preexistant : refus 2", r2.returncode == 2, f"rc={r2.returncode}")


def scene_refus_parametres(p, base, faux):
    p.scene("(b)(c)(d) REPS=3, N_LIST vide, REPS=0/-4/abc : refus 2 sans trace")
    cas = [("reps3", "64", "3", "(b) REPS=3 non multiple de 4"),
           ("nvide", "", "4", "(c) N_LIST vide"),
           ("nblancs", "   ", "4", "(c) N_LIST blancs"),
           ("reps0", "64", "0", "(d) REPS=0"),
           ("repsneg", "64", "-4", "(d) REPS=-4"),
           ("repsabc", "64", "abc", "(d) REPS=abc"),
           ("nnonentier", "64 x", "4", "taille non entiere")]
    for nom, n_list, reps, libelle in cas:
        out = base / nom
        r = p.lancer(out, faux, n_list, reps)
        p.check(f"{libelle} : refus 2", r.returncode == 2, f"rc={r.returncode}")
        p.check(f"{libelle} : aucun dossier laisse",
                not out.exists() and not Path(str(out) + ".partial").exists())


def scene_copie_alteree(p, base, faux):
    p.scene("(e) copie privee alteree pendant le troisieme tuple : INVALIDE 3, reste en .partial")
    out = base / "alteree"
    partial = Path(str(out) + ".partial")
    # Appels : 1 et 2 = sondes d'identite ; 3, 4, 5 = tuples 1, 2, 3.
    r = p.lancer(out, faux, "64 128", "4", p.env_mutant(base, "corrompre_copie", 5))
    p.check("copie alteree : rc=3", r.returncode == 3, f"rc={r.returncode}")
    p.check("copie alteree : jamais publiee, reste en .partial",
            not out.exists() and partial.is_dir())
    if not partial.is_dir():
        return
    meta = (partial / "META.txt").read_text(encoding="utf-8")
    p.check("copie alteree : META porte INVALIDE hash APRES",
            "campagne INVALIDE : hash APRES" in meta)
    statuts = sorted((partial / "out").glob("*.status"))
    p.check("copie alteree : arret immediat (exactement trois statuts)", len(statuts) == 3,
            f"{len(statuts)}")
    hashes = (partial / "HASHES.txt").read_text(encoding="utf-8").splitlines()
    p.check("copie alteree : troisieme ligne HASHES avant != apres",
            len(hashes) == 3 and hashes[2].split()[1][6:] != hashes[2].split()[2][6:]
            and hashes[0].split()[1][6:] == hashes[0].split()[2][6:])
    p.check("copie alteree : pas de manifeste ni de resume",
            not (partial / "SHA256SUMS").exists() and not (partial / "resume.txt").exists())


def scene_agregateur(p, base, faux):
    p.scene("(f)(g) ligne K=7 omise / touch=nan sur un run : INVALIDE 3 par l'agregateur")
    for nom, mutant, motifs in (("k7", "omettre_k7", ("K manquante", "[7]")),
                                ("nan", "touch_nan", ("touch", "non finie"))):
        out = base / nom
        partial = Path(str(out) + ".partial")
        r = p.lancer(out, faux, "64 128", "4", p.env_mutant(base, mutant, 5))
        p.check(f"{mutant} : rc=3", r.returncode == 3, f"rc={r.returncode}")
        p.check(f"{mutant} : jamais publie, reste en .partial",
                not out.exists() and partial.is_dir())
        if not partial.is_dir():
            continue
        err = (partial / "resume.err").read_text(encoding="utf-8")
        meta = (partial / "META.txt").read_text(encoding="utf-8")
        p.check(f"{mutant} : motif de refus {motifs}", all(m in err for m in motifs),
                repr(err[:200]))
        p.check(f"{mutant} : agregateur en echec au META",
                "campagne INVALIDE : agregateur en echec" in meta)
        p.check(f"{mutant} : aucun manifeste (agregateur fatal avant SHA256SUMS)",
                not (partial / "SHA256SUMS").exists())
        p.check(f"{mutant} : matrice complete executee (32 statuts, codes nuls)",
                len(list((partial / "out").glob("*.status"))) == 32)


def scene_manifeste(p, base, faux):
    p.scene("(h) alteration entre generation et verification de SHA256SUMS : INVALIDE 3")
    reel = shutil.which("sha256sum")
    p.check("sha256sum reel trouve", reel is not None)
    if reel is None:
        return
    fakebin = base / "fakebin"
    fakebin.mkdir()
    ecrire_exec(fakebin / "sha256sum", FAUX_SHA256SUM.replace("%REAL%", reel))
    out = base / "manifeste"
    partial = Path(str(out) + ".partial")
    r = p.lancer(out, faux, "64", "4", {"PATH": f"{fakebin}:{os.environ['PATH']}"})
    p.check("manifeste altere : rc=3", r.returncode == 3, f"rc={r.returncode}")
    p.check("manifeste altere : jamais publie, reste en .partial",
            not out.exists() and partial.is_dir())
    if partial.is_dir():
        meta = (partial / "META.txt").read_text(encoding="utf-8")
        p.check("manifeste altere : verification finale en echec au META",
                "campagne INVALIDE : verification finale sha256sum -c" in meta)
        p.check("manifeste altere : le manifeste existe (genere avant l'alteration)",
                (partial / "SHA256SUMS").is_file())


def scene_produit(p, base, bench):
    p.scene("(i) binaire PRODUIT (refuse --inject=) : refus 2, aucun .partial")
    produit = bench.parent.parent / "build" / "v6" / "mhgp6_profile"
    if produit.is_file() and os.access(produit, os.X_OK):
        binaire, quoi = produit, "build/v6/mhgp6_profile"
    else:
        binaire, quoi = base / "faux_produit", "faux produit"
        ecrire_exec(binaire, FAUX_PRODUIT)
    out = base / "produit"
    r = p.lancer(out, binaire, "64", "4")
    p.check(f"produit ({quoi}) : refus 2", r.returncode == 2, f"rc={r.returncode}")
    p.check("produit : aucun dossier laisse",
            not out.exists() and not Path(str(out) + ".partial").exists())
    p.check("produit : motif --inject au refus", "--inject" in r.stderr)
    # Un faux qui ACCEPTE un nom inconnu n'est pas la cible non plus.
    laxiste = base / "faux_laxiste"
    refus_inconnu = '*) echo "REFUS : arguments (mutant inconnu ${inject})" >&2; exit 2 ;;'
    p.check("laxiste : la clause de refus existe dans le faux", refus_inconnu in FAUX_SONDE)
    ecrire_exec(laxiste, FAUX_SONDE.replace(refus_inconnu, "*) ;;"))
    out2 = base / "laxiste"
    r2 = p.lancer(out2, laxiste, "64", "4")
    p.check("binaire acceptant un --inject= inconnu : refus 2", r2.returncode == 2,
            f"rc={r2.returncode}")
    p.check("laxiste : aucun dossier laisse",
            not out2.exists() and not Path(str(out2) + ".partial").exists())


def main():
    if len(sys.argv) != 2:
        print("usage: sonde_ablation_gate.py <dossier bench>", file=sys.stderr)
        return 2
    bench = Path(sys.argv[1]).resolve()
    p = Porte(bench)
    if not p.lanceur.is_file() or not (bench / "sonde_ablation_reduce.py").is_file():
        print(f"ECHEC porte : lanceur ou agregateur absent sous {bench}", file=sys.stderr)
        return 1
    with tempfile.TemporaryDirectory(prefix="sonde_ablation_gate_") as td:
        base = Path(td)
        faux = base / "faux_sonde"
        ecrire_exec(faux, FAUX_SONDE)
        scene_nominal(p, base, faux)
        scene_refus_parametres(p, base, faux)
        scene_copie_alteree(p, base, faux)
        scene_agregateur(p, base, faux)
        scene_manifeste(p, base, faux)
        scene_produit(p, base, bench)
    if p.echecs:
        print(f"sonde_ablation_gate : {p.echecs} controle(s) en echec sur {p.scenes} scenes")
        return 1
    print(f"sonde_ablation_gate : {p.scenes} scenes vertes (nominal apparie, refus de parametres, "
          "copie alteree, agregateur fail-closed, manifeste fatal, binaire produit)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
