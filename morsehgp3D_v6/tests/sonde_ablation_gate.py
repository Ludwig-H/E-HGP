#!/usr/bin/env python3
"""Porte de VACUITE du lanceur et de l'agregateur de la sonde d'ablation du
reduce (bench/sonde_ablation_reduce.sh, bench/sonde_ablation_reduce.py) —
fermeture minimale exigee par audits/ALERTE_SONDE_ABLATION_REDUCE_20260902.md,
puis les cinq contre-fixtures encore vertes de son « Etat du WIP » et du
§ 5.21 de REPONSE_AUDITEURS_MULTICPU_V6_20260901.md (scenes (j)–(n)).

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
  Contre-fixtures des auditeurs (chacune : le mutant precis, le code attendu,
  le motif attendu ; les reçus synthetiques sont des copies du reçu nominal
  (a) modifiees d'UN seul defaut, puis l'agregateur est appele directement) :
  (j) taille DUPLIQUEE : N_LIST="8000 8000" et "64 064" (meme taille sous
      deux graphies) => refus 2 du lanceur avant tout run, aucun .partial ;
      META n_list="64 64" => agregateur 1 ;
  (k) hash VIDE ou hors grammaire 64-hex : META binaire_sha256= vide ou a
      63 caracteres, HASHES.txt avant= vide, .status sha256_apres= vide =>
      agregateur 1 ; faux sha256sum muet en PATH => lanceur refus 2, aucun
      .partial (jamais un hash vide grave) ;
  (l) champ DUPLIQUE a valeurs differentes : .status avec code= repete (en
      fin de fichier ou sur la premiere ligne), META reps= repete, plan.txt
      position= repete sur une ligne, profil `touch=nan touch=<fini>` =>
      agregateur 1 (« duplique »), jamais la premiere ni la derniere valeur ;
  (m) plan LATIN CYCLIQUE (ABCD/BCDA/CDAB/DABC : equilibre par position, non
      Williams — A suit D trois fois), positions des .status alignees sur ce
      plan pour isoler le defaut => agregateur 1 (« NON Williams ») ; temoin
      de causalite : les quatre lignes de Williams en ordre permute
      (BDAC/CADB/DCBA/ABCD) => agregateur 0 ;
  (n) inventaire EXACT de out/ : out/intrus.txt, out/SHA256SUMS, out/sub/ ou
      un .err retire => agregateur 1 ; `--inventaire` du lanceur liste
      out/intrus.txt et out/SHA256SUMS (seul ./SHA256SUMS racine exclu) et
      coincide avec le manifeste nominal ; binaire creant out/intrus.txt et
      out/SHA256SUMS pendant un tuple => INVALIDE 3 par l'agregateur ; faux
      python3 ajoutant intrus_racine.txt et out/SHA256SUMS APRES l'agregateur
      et AVANT le manifeste => INVALIDE 3 par l'inventaire, les deux intrus
      figurant dans le SHA256SUMS du .partial (haches, donc visibles).
  Dents du 2 septembre (ETAT_COURANT, ligne « sonde equilibree … harnais »,
  et fin du § 5.22 ; memes regles : un mutant, un code, un motif) :
  (o) runs_effectues / runs_attendus OBLIGATOIRES : META sans l'un des deux
      => agregateur 1 (« champ obligatoire ») ; runs_effectues=31 => 1 ;
      schema v2 explicite (sans les champs d'identite) => 0 avec « claim
      borne, NON VERIFIE » imprime ; v2 sans runs_attendus => 1 ; v1 => 1 ;
      le reçu publie receipts/sonde_ablation_reduce_20260902b, s'il est
      present, => 0 sans modification, claim borne imprime ;
  (p) ligne profil_reduce MALFORMEE, jamais ignoree : touch=abc, jeton sans
      `=`, K=8.0 / K=huit, valeur vide (tronquee), ligne sans fenetre, ligne
      `profil_reduce` seule, champ non requis non numerique, notation
      exposant, temps_mur_ms ou rss_max_kb duplique => agregateur 1 ;
  (q) repertoire VIDE vide/ ou inattendu a la racine, lien a la racine,
      bin/ pollue, fichier intrus a la racine => agregateur 1 ; faux python3
      creant vide/ APRES l'agregateur et AVANT le manifeste => INVALIDE 3 du
      lanceur (inventaire des repertoires, invisible au manifeste) ;
  (r) protocole_lanceur.sh / protocole_agregateur.py alteres apres coup ou
      retires => agregateur 1 (hash RECALCULE != META) ; faux sha256sum
      alterant la copie archivee du lanceur APRES le manifeste => INVALIDE 3
      du lanceur (copie relue avant publication != META) ;
  (s) identite de cible (claim BORNE : la cible reelle accepte tout kMutants,
      verifie ici sur build/v6/mhgp6_profile_sonde s'il existe) : META porte
      identite_cible et injections_autorisees == injections_emises == les
      trois ablations ; .status avec --inject=drop-nonmerge, META
      injections_emises ou ablations portant drop-nonmerge, v3 sans
      identite_cible => agregateur 1 ; lanceur dont ABLATIONS est etendu a
      drop-nonmerge => refus 2 avant toute ecriture ; plan.txt altere en
      campagne vers bras=drop-nonmerge => INVALIDE 3, aucun .status ne porte
      ce --inject= ;
  (t) TOCTOU avant scellement : faux python3 MODIFIANT out/aucune_n64_r1.txt
      APRES l'agregateur et AVANT le manifeste (le manifeste scelle le
      contenu mute, `sha256sum -c` seul le publierait) => INVALIDE 3
      (empreinte d'avant agregation != manifeste) ; faux sha256sum modifiant
      le meme fichier APRES la generation du manifeste => INVALIDE 3
      (verification finale, derniere operation avant le mv) ;
  (u) `diff` fail-open : faux diff rendant 2 sans sortie => INVALIDE 3
      (« comparaison impossible ») ; faux diff rendant 1 sans sortie =>
      INVALIDE 3 (jamais un test sur la seule sortie vide).

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
LETTRES = dict(zip("ABCD", BRAS))
LATIN_CYCLIQUE = ("A B C D", "B C D A", "C D A B", "D A B C")
WILLIAMS_PERMUTE = ("B D A C", "C A D B", "D C B A", "A B C D")
SCHEMA = "e-hgp.sonde-ablation-reduce.v3"
SCHEMA_V2 = "e-hgp.sonde-ablation-reduce.v2"
INJECTIONS = "ablation-mat-sans-copie ablation-mat-sans-tris ablation-post-cle-factice"
IDENTITE_CIBLE = ("mhgp6_profile_sonde (accepte tout mutant de kMutants ; "
                  "seules les ablations sont selectionnees ici)")
CHAMPS_V3 = ("identite_cible", "injections_autorisees", "injections_emises")
LIGNE_ABLATIONS = ('ABLATIONS="aucune ablation-mat-sans-copie ablation-mat-sans-tris '
                   'ablation-post-cle-factice"')

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
if [ "$mutant" = intrus_out ]; then
  # Le binaire ecrit deux intrus dans le dossier de sa sortie (out/) : un
  # fichier arbitraire et un manifeste imbrique. /proc/$$/fd/1 (le script,
  # pas le sous-shell de la substitution, dont fd 1 serait le tube) resout
  # le fichier <recu>/out/<tag>.txt ; hors d'un dossier out/ le mutant
  # s'arrete code 9 plutot que d'ecrire n'importe ou (la scene echouerait).
  d="$(dirname "$(readlink -f "/proc/$$/fd/1")")"
  if [ "$(basename "$d")" != out ]; then
    echo "mutant intrus_out : sortie non redirigee vers un dossier out/ ($d)" >&2
    exit 9
  fi
  : > "${d}/intrus.txt"
  : > "${d}/SHA256SUMS"
fi
if [ "$mutant" = plan_hors_sonde ]; then
  # Le binaire altere plan.txt du reçu pendant la campagne : le bras D des
  # lignes de plan devient un mutant PRODUIT (drop-nonmerge). Le lanceur ne
  # doit jamais l'emettre (garde de run_one, INVALIDE 3).
  d="$(dirname "$(readlink -f "/proc/$$/fd/1")")"
  if [ "$(basename "$d")" != out ]; then
    echo "mutant plan_hors_sonde : sortie non redirigee vers un dossier out/ ($d)" >&2
    exit 9
  fi
  sed -i 's/bras=ablation-post-cle-factice$/bras=drop-nonmerge/' "${d}/../plan.txt"
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

# Mutant (k) : sha256sum MUET (code 0, aucune sortie) — la contre-fixture
# « hash vide » de l'alerte : le lanceur ne doit jamais graver une chaine vide.
FAUX_SHA256SUM_VIDE = """#!/usr/bin/env bash
exit 0
"""

# Mutant (n) : faux python3 qui execute l'agregateur REEL puis, apres son
# retour et avant le manifeste, ajoute un intrus a la racine et un
# out/SHA256SUMS — la fenetre entre l'agregateur et la generation du manifeste.
FAUX_PYTHON3_INTRUS = """#!/usr/bin/env bash
"%REAL%" "$@"
rc=$?
w="${@: -1}"
: > "${w}/intrus_racine.txt"
: > "${w}/out/SHA256SUMS"
exit $rc
"""

# Mutant (q) : faux python3 qui, apres l'agregateur reel et avant le
# manifeste, cree un repertoire VIDE a la racine du reçu — invisible a un
# manifeste de fichiers, visible a l'inventaire des repertoires du lanceur.
FAUX_PYTHON3_VIDE = """#!/usr/bin/env bash
"%REAL%" "$@"
rc=$?
w="${@: -1}"
mkdir "${w}/vide"
exit $rc
"""

# Mutant (t) : faux python3 qui, apres l'agregateur reel et avant le
# manifeste, MODIFIE une sortie deja lue (mutation semantique) : le manifeste
# scelle alors le contenu mute et `sha256sum -c` seul publierait.
FAUX_PYTHON3_MUTATION = """#!/usr/bin/env bash
"%REAL%" "$@"
rc=$?
w="${@: -1}"
echo "# mutation apres agregation (mutant de porte)" >> "${w}/out/aucune_n64_r1.txt"
exit $rc
"""

# Mutants (r)(t) : faux sha256sum qui n'agit que sur la GENERATION du
# manifeste (plusieurs fichiers en arguments, pas -c ; hash_de n'en passe
# qu'un) : il execute le vrai puis modifie %CIBLE% — APRES le manifeste,
# AVANT sa verification finale. Les appels a un fichier et -c passent au vrai.
FAUX_SHA256SUM_TARDIF = """#!/usr/bin/env bash
if [ "$#" -gt 1 ] && [ "${1:-}" != "-c" ]; then
  %REAL% "$@"
  rc=$?
  echo "# mutation apres le manifeste (mutant de porte)" >> "%CIBLE%"
  exit $rc
fi
exec %REAL% "$@"
"""

# Mutants (u) : faux diff MUET rendant %RC% — un diff en erreur (2) ou
# « different » sans sortie (1) ne doit jamais valoir « identique ».
FAUX_DIFF = """#!/usr/bin/env bash
exit %RC%
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
        self.agregateur = bench / "sonde_ablation_reduce.py"
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

    def lancer(self, out, binaire, n_list, reps, env_extra=None, lanceur=None):
        env = dict(os.environ, THREADS="1", CPUS=self.cpu, FAMILY="uniform")
        if env_extra:
            env.update(env_extra)
        argv = ["bash", str(lanceur or self.lanceur), str(out), str(binaire)]
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

    def agreger(self, dossier):
        """Agregateur COURANT (bench/) appele directement sur un dossier."""
        return subprocess.run([sys.executable, str(self.agregateur), str(dossier)],
                              capture_output=True, text=True)

    def refus_agregateur(self, nom, dossier, motifs):
        """L'agregateur doit rendre 1 avec chaque motif sur stderr."""
        a = self.agreger(dossier)
        self.check(f"{nom} : agregateur rc=1", a.returncode == 1, f"rc={a.returncode}")
        self.check(f"{nom} : motif de refus {motifs}", all(m in a.stderr for m in motifs),
                   repr(a.stderr[-300:]))
        return a

    def copier_recu(self, base, nominal, nom):
        """Copie du reçu nominal a modifier d'UN seul defaut."""
        cible = base / nom
        shutil.copytree(nominal, cible)
        return cible

    def remplacer(self, path, ancien, nouveau, nom):
        """Remplace la PREMIERE occurrence ; l'absence est un echec de porte."""
        texte = path.read_text(encoding="utf-8")
        self.check(f"{nom} : cible du mutant presente ({ancien[:40]!r})", ancien in texte)
        path.write_text(texte.replace(ancien, nouveau, 1), encoding="utf-8")


def reecrire_plan(dossier, lignes):
    """Grave un plan (quatre lignes de lettres, blocs 1..4) et aligne la
    position de chaque .status sur ce plan : le seul ecart restant est la
    propriete du plan lui-meme."""
    plan = {}
    texte = ["# plan de porte (lettres : " + " | ".join(lignes) + ")"]
    for bloc, ligne in enumerate(lignes, 1):
        for pos, lettre in enumerate(ligne.split(), 1):
            plan[(bloc, LETTRES[lettre])] = pos
            texte.append(f"bloc={bloc} position={pos} bras={LETTRES[lettre]}")
    (dossier / "plan.txt").write_text("\n".join(texte) + "\n", encoding="utf-8")
    for st in sorted((dossier / "out").glob("*.status")):
        lignes_st = st.read_text(encoding="utf-8").splitlines()
        kv = dict(t.split("=", 1) for t in lignes_st[0].split())
        pos = plan[(int(kv["rep"]), kv["ablation"])]
        lignes_st[0] = " ".join(f"{k}={pos if k == 'position' else v}" for k, v in kv.items())
        st.write_text("\n".join(lignes_st) + "\n", encoding="utf-8")


def remplacer_ligne(p, path, prefixe, nouvelle, nom):
    """Remplace la premiere ligne commencant par `prefixe` ; l'absence est un
    echec de porte (le mutant doit viser une cible presente)."""
    lignes = path.read_text(encoding="utf-8").splitlines()
    idx = [i for i, l in enumerate(lignes) if l.startswith(prefixe)]
    p.check(f"{nom} : ligne cible presente ({prefixe!r})", bool(idx))
    if idx:
        lignes[idx[0]] = nouvelle
    path.write_text("\n".join(lignes) + "\n", encoding="utf-8")


def retirer_ligne(p, path, prefixe, nom):
    """Retire la premiere ligne commencant par `prefixe` (champ absent)."""
    lignes = path.read_text(encoding="utf-8").splitlines()
    idx = [i for i, l in enumerate(lignes) if l.startswith(prefixe)]
    p.check(f"{nom} : ligne cible presente ({prefixe!r})", bool(idx))
    if idx:
        del lignes[idx[0]]
    path.write_text("\n".join(lignes) + "\n", encoding="utf-8")


def retrograder_v2(p, dossier):
    """META v3 -> v2 : schema v2 et retrait des trois champs d'identite (le
    lanceur v2 ne les gravait pas) ; tout le reste est inchange."""
    meta = dossier / "META.txt"
    remplacer_ligne(p, meta, "schema=", f"schema={SCHEMA_V2}", "retrogradation v2")
    for champ in CHAMPS_V3:
        retirer_ligne(p, meta, f"{champ}=", "retrogradation v2")


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
    p.check("nominal : schema v3", meta.get("schema") == SCHEMA, repr(meta.get("schema")))
    p.check("nominal : identite_cible (claim borne) gravee au META",
            meta.get("identite_cible") == IDENTITE_CIBLE, repr(meta.get("identite_cible")))
    p.check("nominal : injections_autorisees == injections_emises == les trois ablations",
            meta.get("injections_autorisees") == INJECTIONS
            and meta.get("injections_emises") == INJECTIONS,
            f"{meta.get('injections_autorisees')!r} / {meta.get('injections_emises')!r}")
    p.check("nominal : runs_attendus=32 au META", meta.get("runs_attendus") == "32")
    p.check("nominal : sha256_lanceur/sha256_agregateur == copies archivees recalculees",
            meta.get("sha256_lanceur") == sha256(out / "protocole_lanceur.sh")
            and meta.get("sha256_agregateur") == sha256(out / "protocole_agregateur.py"))
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
    # Williams : les douze successions ordonnees, chacune exactement une fois.
    successions = [(plan.get((bloc, pos), "?"), plan.get((bloc, pos + 1), "?"))
                   for bloc in range(1, 5) for pos in range(1, 4)]
    p.check("plan : douze successions ordonnees distinctes (Williams)",
            len(set(successions)) == 12 and all(x != y for x, y in successions))
    # Ensemble exact des sorties, codes nuls, positions du plan executees.
    statuts = sorted((out / "out").glob("*.status"))
    p.check("nominal : 32 statuts (4 bras x 2 tailles x 4 blocs)", len(statuts) == 32,
            f"{len(statuts)}")
    p.check("nominal : 32 sorties .txt", len(list((out / "out").glob("*.txt"))) == 32)
    p.check("nominal : 32 fichiers .err", len(list((out / "out").glob("*.err"))) == 32)
    p.check("nominal : out/ = exactement 96 fichiers", len(list((out / "out").iterdir())) == 96)
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
    p.check("nominal : identite de cible (claim borne) dans le resume, aucun claim NON VERIFIE (v3)",
            "identite de cible (claim borne) : mhgp6_profile_sonde (accepte tout mutant" in resume
            and "NON VERIFIE" not in resume)
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
    # Un reçu marque INVALIDE n'est jamais agrege, meme appele directement.
    p.refus_agregateur("copie alteree : .partial INVALIDE refuse par l'agregateur", partial,
                       ("campagne INVALIDE",))


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


def scene_taille_dupliquee(p, base, faux, nominal):
    p.scene("(j) taille dupliquee : lanceur refus 2 avant tout run ; META n_list duplique : agregateur 1")
    for nom, n_list, libelle in (("ndup", "8000 8000", "N_LIST='8000 8000'"),
                                 ("ndup3", "64 128 64", "N_LIST='64 128 64'"),
                                 ("ndup0", "64 064", "N_LIST='64 064' (meme taille, deux graphies)")):
        out = base / nom
        r = p.lancer(out, faux, n_list, "4")
        p.check(f"{libelle} : refus 2", r.returncode == 2, f"rc={r.returncode}")
        p.check(f"{libelle} : motif « taille dupliquee »", "taille dupliquee" in r.stderr,
                repr(r.stderr[-200:]))
        p.check(f"{libelle} : aucun dossier ni .partial laisse (refus avant tout run)",
                not out.exists() and not Path(str(out) + ".partial").exists())
    # Agregateur seul : META n_list="64 64" sur une copie du reçu nominal.
    recu = p.copier_recu(base, nominal, "meta_ndup")
    p.remplacer(recu / "META.txt", "n_list=64 128", "n_list=64 64", "meta n_list duplique")
    p.refus_agregateur("META n_list='64 64'", recu, ("n_list", "dupliquee", "[64]"))


def scene_hash_vide(p, base, faux, nominal):
    p.scene("(k) hash vide ou hors grammaire 64-hex : agregateur 1 ; sha256sum muet : lanceur refus 2")
    h = lire_kv(nominal / "META.txt").get("binaire_sha256", "")
    p.check("hash : binaire_sha256 nominal a 64 hexadecimaux",
            len(h) == 64 and all(c in "0123456789abcdef" for c in h))
    recu = p.copier_recu(base, nominal, "hash_meta_vide")
    p.remplacer(recu / "META.txt", f"binaire_sha256={h}", "binaire_sha256=", "META binaire_sha256 vide")
    p.refus_agregateur("META binaire_sha256= vide", recu, ("binaire_sha256", "64-hex"))
    recu = p.copier_recu(base, nominal, "hash_meta_court")
    p.remplacer(recu / "META.txt", f"binaire_sha256={h}", f"binaire_sha256={h[:63]}",
                "META binaire_sha256 a 63 caracteres")
    p.refus_agregateur("META binaire_sha256= a 63 caracteres", recu, ("binaire_sha256", "64-hex"))
    recu = p.copier_recu(base, nominal, "hash_hashes_vide")
    p.remplacer(recu / "HASHES.txt", f"avant={h}", "avant=", "HASHES.txt avant= vide")
    p.refus_agregateur("HASHES.txt avant= vide", recu, ("HASHES.txt", "avant=", "64-hex"))
    recu = p.copier_recu(base, nominal, "hash_status_vide")
    st = recu / "out" / "ablation-mat-sans-tris_n128_r3.status"
    p.remplacer(st, f"sha256_apres={h}", "sha256_apres=", ".status sha256_apres= vide")
    p.refus_agregateur(".status sha256_apres= vide", recu,
                       ("ablation-mat-sans-tris_n128_r3.status", "sha256_apres", "64-hex"))
    # Lanceur : sha256sum MUET en PATH => refus 2, aucun hash vide grave, aucun .partial.
    fakebin = base / "fakebin_sha_vide"
    fakebin.mkdir()
    ecrire_exec(fakebin / "sha256sum", FAUX_SHA256SUM_VIDE)
    out = base / "sha_vide"
    r = p.lancer(out, faux, "64", "4", {"PATH": f"{fakebin}:{os.environ['PATH']}"})
    p.check("sha256sum muet : lanceur refus 2", r.returncode == 2, f"rc={r.returncode}")
    p.check("sha256sum muet : motif 64-hex au refus", "64-hex" in r.stderr, repr(r.stderr[-200:]))
    p.check("sha256sum muet : aucun dossier ni .partial laisse",
            not out.exists() and not Path(str(out) + ".partial").exists())


def scene_champ_duplique(p, base, nominal):
    p.scene("(l) champ duplique a valeurs differentes (.status code=, META reps=, plan position=, profil touch=) : agregateur 1")
    recu = p.copier_recu(base, nominal, "dup_code_fin")
    st = recu / "out" / "aucune_n64_r1.status"
    with open(st, "a", encoding="utf-8") as f:
        f.write("code=1\n")
    p.refus_agregateur(".status code= repete en fin de fichier (0 puis 1)", recu,
                       ("aucune_n64_r1.status", "code=", "duplique"))
    recu = p.copier_recu(base, nominal, "dup_code_ligne1")
    st = recu / "out" / "aucune_n64_r1.status"
    p.remplacer(st, " code=0", " code=0 code=1", ".status code= repete sur la ligne 1")
    p.refus_agregateur(".status code= repete sur la premiere ligne (0 puis 1)", recu,
                       ("aucune_n64_r1.status", "code=", "duplique"))
    recu = p.copier_recu(base, nominal, "dup_reps")
    with open(recu / "META.txt", "a", encoding="utf-8") as f:
        f.write("reps=8\n")
    p.refus_agregateur("META reps= repete (4 puis 8)", recu, ("META.txt", "reps=", "duplique"))
    recu = p.copier_recu(base, nominal, "dup_plan")
    p.remplacer(recu / "plan.txt", "bloc=1 position=1 bras=aucune",
                "bloc=1 position=1 position=2 bras=aucune", "plan position= repete")
    p.refus_agregateur("plan.txt position= repete sur une ligne (1 puis 2)", recu,
                       ("plan.txt", "position=", "duplique"))
    recu = p.copier_recu(base, nominal, "dup_profil")
    txt = recu / "out" / "aucune_n64_r1.txt"
    p.remplacer(txt, "K=8 init=8.000 touch=16.000", "K=8 init=8.000 touch=nan touch=16.000",
                "profil touch= repete")
    p.refus_agregateur("profil `touch=nan touch=16.000` (jamais la derniere valeur)", recu,
                       ("aucune_n64_r1.txt", "touch=", "duplique"))


def scene_plan_non_williams(p, base, nominal):
    p.scene("(m) carre latin cyclique (equilibre par position, non Williams) : agregateur 1 ; Williams permute : 0")
    recu = p.copier_recu(base, nominal, "latin_cyclique")
    reecrire_plan(recu, LATIN_CYCLIQUE)
    a = p.refus_agregateur("plan latin cyclique ABCD/BCDA/CDAB/DABC", recu,
                           ("plan.txt", "NON Williams", "repetee"))
    p.check("plan latin cyclique : motif nomme la succession D->A repetee trois fois",
            "ablation-post-cle-factice->aucune x3" in a.stderr, repr(a.stderr[-300:]))
    # Temoin de causalite : le meme reçu avec un plan de Williams dont les
    # lignes sont permutees (memes douze successions) est accepte.
    temoin = p.copier_recu(base, nominal, "williams_permute")
    reecrire_plan(temoin, WILLIAMS_PERMUTE)
    a2 = p.agreger(temoin)
    p.check("plan de Williams permute (BDAC/CADB/DCBA/ABCD) : agregateur 0",
            a2.returncode == 0 and len(a2.stdout) > 0, f"rc={a2.returncode} {a2.stderr[-200:]!r}")


def scene_inventaire(p, base, faux, nominal):
    p.scene("(n) out/intrus.txt, out/SHA256SUMS, out/sub/, .err retire : agregateur 1 ; le lanceur les hache et refuse")
    recu = p.copier_recu(base, nominal, "intrus_txt")
    (recu / "out" / "intrus.txt").write_text("intrus\n", encoding="utf-8")
    p.refus_agregateur("out/intrus.txt ajoute apres coup", recu, ("out/", "inattendu", "intrus.txt"))
    recu2 = p.copier_recu(base, nominal, "intrus_sums")
    (recu2 / "out" / "SHA256SUMS").write_text("", encoding="utf-8")
    p.refus_agregateur("out/SHA256SUMS ajoute apres coup", recu2, ("out/", "inattendu", "SHA256SUMS"))
    recu3 = p.copier_recu(base, nominal, "intrus_dir")
    (recu3 / "out" / "sub").mkdir()
    p.refus_agregateur("out/sub/ ajoute apres coup", recu3, ("out/sub", "non reguliere"))
    recu4 = p.copier_recu(base, nominal, "err_absent")
    (recu4 / "out" / "aucune_n64_r1.err").unlink()
    p.refus_agregateur("out/aucune_n64_r1.err retire", recu4, ("out/", "absent", "aucune_n64_r1.err"))
    # Le lanceur les aurait haches : son inventaire (seul ./SHA256SUMS racine
    # exclu) liste les deux intrus imbriques ; sur le nominal il coincide
    # exactement avec les entrees du manifeste publie.
    (recu / "out" / "SHA256SUMS").write_text("", encoding="utf-8")
    inv = subprocess.run(["bash", str(p.lanceur), "--inventaire", str(recu)],
                         capture_output=True, text=True)
    lignes = set(inv.stdout.splitlines())
    p.check("--inventaire : rc=0", inv.returncode == 0, f"rc={inv.returncode} {inv.stderr[-200:]!r}")
    p.check("--inventaire : out/intrus.txt et out/SHA256SUMS listes, ./SHA256SUMS racine exclu",
            {"out/intrus.txt", "out/SHA256SUMS", "META.txt"} <= lignes and "SHA256SUMS" not in lignes,
            f"{sorted(l for l in lignes if 'SHA256SUMS' in l or 'intrus' in l)}")
    inv_nom = subprocess.run(["bash", str(p.lanceur), "--inventaire", str(nominal)],
                             capture_output=True, text=True)
    entrees = {l.split("  ", 1)[1]
               for l in (nominal / "SHA256SUMS").read_text(encoding="utf-8").splitlines()}
    p.check("--inventaire du nominal == entrees du manifeste publie",
            inv_nom.returncode == 0 and set(inv_nom.stdout.splitlines()) == entrees)
    # Lanceur, dynamique : le binaire cree out/intrus.txt et out/SHA256SUMS
    # pendant le troisieme tuple => l'agregateur (fatal) refuse, INVALIDE 3.
    out = base / "intrus_binaire"
    partial = Path(str(out) + ".partial")
    r = p.lancer(out, faux, "64 128", "4", p.env_mutant(base, "intrus_out", 5))
    p.check("binaire ecrivant des intrus dans out/ : rc=3", r.returncode == 3, f"rc={r.returncode}")
    p.check("binaire ecrivant des intrus : jamais publie, reste en .partial",
            not out.exists() and partial.is_dir())
    if partial.is_dir():
        err = (partial / "resume.err").read_text(encoding="utf-8")
        meta = (partial / "META.txt").read_text(encoding="utf-8")
        p.check("binaire ecrivant des intrus : refus de l'agregateur nomme les intrus",
                "inattendu" in err and "SHA256SUMS" in err and "intrus.txt" in err, repr(err[-300:]))
        p.check("binaire ecrivant des intrus : agregateur en echec au META, aucun manifeste",
                "campagne INVALIDE : agregateur en echec" in meta
                and not (partial / "SHA256SUMS").exists())
        p.check("binaire ecrivant des intrus : les deux intrus sont bien dans out/",
                (partial / "out" / "intrus.txt").is_file() and (partial / "out" / "SHA256SUMS").is_file())
    # Lanceur, dynamique : intrus ajoutes APRES l'agregateur et AVANT le
    # manifeste (faux python3) => haches par le manifeste, puis refuses par
    # l'inventaire exact : INVALIDE 3, jamais publie.
    reel = shutil.which("python3") or sys.executable
    fakebin = base / "fakebin_python3"
    fakebin.mkdir()
    ecrire_exec(fakebin / "python3", FAUX_PYTHON3_INTRUS.replace("%REAL%", reel))
    out2 = base / "intrus_tardifs"
    partial2 = Path(str(out2) + ".partial")
    r2 = p.lancer(out2, faux, "64", "4", {"PATH": f"{fakebin}:{os.environ['PATH']}"})
    p.check("intrus tardifs (racine + out/SHA256SUMS) : rc=3", r2.returncode == 3,
            f"rc={r2.returncode} {r2.stderr[-200:]!r}")
    p.check("intrus tardifs : jamais publie, reste en .partial",
            not out2.exists() and partial2.is_dir())
    if partial2.is_dir():
        meta2 = (partial2 / "META.txt").read_text(encoding="utf-8")
        p.check("intrus tardifs : inventaire exact en echec au META, les deux intrus nommes",
                "campagne INVALIDE : inventaire du reçu != ensemble attendu" in meta2
                and "> intrus_racine.txt" in meta2 and "> out/SHA256SUMS" in meta2,
                repr(meta2[-300:]))
        sums = partial2 / "SHA256SUMS"
        p.check("intrus tardifs : le manifeste existe", sums.is_file())
        if sums.is_file():
            entrees2 = {l.split("  ", 1)[1] for l in sums.read_text(encoding="utf-8").splitlines()}
            p.check("intrus tardifs : le lanceur a HACHE intrus_racine.txt et out/SHA256SUMS",
                    {"intrus_racine.txt", "out/SHA256SUMS"} <= entrees2,
                    f"{sorted(e for e in entrees2 if 'intrus' in e or 'SHA256SUMS' in e)}")
        p.check("intrus tardifs : l'agregateur reel avait accepte (resume.txt non vide)",
                (partial2 / "resume.txt").is_file() and (partial2 / "resume.txt").stat().st_size > 0)


def scene_runs_obligatoires(p, base, nominal, bench):
    p.scene("(o) runs_effectues/runs_attendus obligatoires (v3 et v2) : agregateur 1 ; schema v2 sans identite : 0 + claim borne ; v1 : 1")
    for champ in ("runs_attendus", "runs_effectues"):
        recu = p.copier_recu(base, nominal, f"sans_{champ}")
        retirer_ligne(p, recu / "META.txt", f"{champ}=", f"META sans {champ}")
        p.refus_agregateur(f"META sans {champ}", recu,
                           ("META.txt", "champ obligatoire", f"{champ}="))
    recu = p.copier_recu(base, nominal, "runs_31")
    p.remplacer(recu / "META.txt", "runs_effectues=32", "runs_effectues=31", "runs_effectues=31")
    p.refus_agregateur("META runs_effectues=31 != cardinal 32", recu,
                       ("runs_effectues=31", "cardinal", "32"))
    # Schema v2 explicite : accepte, les champs d'identite imprimes en claim
    # borne NON VERIFIE ; mais runs_* restent obligatoires en v2 aussi.
    v2 = p.copier_recu(base, nominal, "schema_v2")
    retrograder_v2(p, v2)
    a = p.agreger(v2)
    p.check("schema v2 sans identite_cible : agregateur 0, « claim borne, NON VERIFIE » imprime",
            a.returncode == 0 and "claim borne, NON VERIFIE" in a.stdout
            and "schema anterieur" in a.stdout, f"rc={a.returncode} {a.stderr[-200:]!r}")
    v2b = p.copier_recu(base, nominal, "schema_v2_sans_runs")
    retrograder_v2(p, v2b)
    retirer_ligne(p, v2b / "META.txt", "runs_attendus=", "v2 sans runs_attendus")
    p.refus_agregateur("schema v2 sans runs_attendus (strict aussi en v2)", v2b,
                       ("champ obligatoire", "runs_attendus="))
    v3b = p.copier_recu(base, nominal, "v3_sans_identite")
    retirer_ligne(p, v3b / "META.txt", "identite_cible=", "v3 sans identite_cible")
    p.refus_agregateur("schema v3 sans identite_cible (obligatoire en v3)", v3b,
                       ("champ obligatoire", "identite_cible="))
    v1 = p.copier_recu(base, nominal, "schema_v1")
    remplacer_ligne(p, v1 / "META.txt", "schema=", "schema=e-hgp.sonde-ablation-reduce.v1", "v1")
    p.refus_agregateur("schema v1 (jamais accepte)", v1, ("schema", "v1"))
    # Reçu publie du 2 septembre (schema v2) : accepte SANS modification.
    publie = bench.parent / "receipts" / "sonde_ablation_reduce_20260902b"
    if publie.is_dir():
        avant = sorted((str(f.relative_to(publie)), f.stat().st_mtime_ns)
                       for f in publie.rglob("*") if f.is_file())
        a = p.agreger(publie)
        apres = sorted((str(f.relative_to(publie)), f.stat().st_mtime_ns)
                       for f in publie.rglob("*") if f.is_file())
        p.check("reçu publie 20260902b (v2) : agregateur 0, claim borne imprime, reçu intact",
                a.returncode == 0 and "claim borne, NON VERIFIE" in a.stdout and avant == apres,
                f"rc={a.returncode} {a.stderr[-200:]!r}")


def scene_profil_malforme(p, base, nominal):
    p.scene("(p) ligne profil_reduce malformee (touch=abc, jeton sans =, K non entier, tronquee, exposant, mur/rss dupliques) : agregateur 1")
    txt = "out/aucune_n64_r1.txt"
    cible = "K=8 init=8.000 touch=16.000"
    cas = [("abc", cible, "K=8 init=8.000 touch=abc",
            ("aucune_n64_r1.txt", "touch=", "non numerique")),
           ("sans_egal", cible, "K=8 init=8.000 touch16.000",
            ("aucune_n64_r1.txt", "sans `=`", "touch16.000")),
           ("vide", cible, "K=8 init=8.000 touch=",
            ("aucune_n64_r1.txt", "touch=", "tronquee")),
           ("exposant", cible, "K=8 init=8.000 touch=16e0",
            ("aucune_n64_r1.txt", "touch=", "hors grammaire decimale")),
           ("non_requis", "reduce_interne_debut=0.000", "reduce_interne_debut=abc",
            ("aucune_n64_r1.txt", "reduce_interne_debut=", "non numerique")),
           ("k_decimal", "profil_reduce K=8 ", "profil_reduce K=8.0 ",
            ("aucune_n64_r1.txt", "K non entier")),
           ("k_lettres", "profil_reduce K=8 ", "profil_reduce K=huit ",
            ("aucune_n64_r1.txt", "K non entier"))]
    for nom, ancien, nouveau, motifs in cas:
        recu = p.copier_recu(base, nominal, f"profil_{nom}")
        p.remplacer(recu / txt, ancien, nouveau, f"profil {nom}")
        p.refus_agregateur(f"profil {nom} ({nouveau.strip()!r})", recu, motifs)
    recu = p.copier_recu(base, nominal, "profil_ligne_tronquee")
    remplacer_ligne(p, recu / txt, "profil_reduce K=8 ", "profil_reduce K=8", "ligne K=8 tronquee")
    p.refus_agregateur("ligne `profil_reduce K=8` sans fenetre", recu,
                       ("aucune_n64_r1.txt", "K=8", "tronquee"))
    recu = p.copier_recu(base, nominal, "profil_ligne_seule")
    remplacer_ligne(p, recu / txt, "profil_reduce K=8 ", "profil_reduce", "ligne profil_reduce seule")
    p.refus_agregateur("ligne `profil_reduce` seule", recu, ("aucune_n64_r1.txt", "tronquee"))
    for champ, valeur in (("temps_mur_ms", "1.0"), ("rss_max_kb", "1")):
        recu = p.copier_recu(base, nominal, f"dup_{champ}")
        with open(recu / txt, "a", encoding="utf-8") as f:
            f.write(f"{champ}={valeur}\n")
        p.refus_agregateur(f"{champ} duplique", recu, ("aucune_n64_r1.txt", champ, "duplique"))


def scene_repertoire_inattendu(p, base, faux, nominal):
    p.scene("(q) repertoire vide/ ou inattendu, lien, bin/ pollue, intrus racine : agregateur 1 ; vide/ cree apres l'agregateur : INVALIDE 3")
    recu = p.copier_recu(base, nominal, "rep_vide")
    (recu / "vide").mkdir()
    p.refus_agregateur("repertoire vide vide/ a la racine", recu,
                       ("racine", "repertoire", "inattendu", "vide"))
    recu = p.copier_recu(base, nominal, "rep_extra")
    (recu / "extra").mkdir()
    (recu / "extra" / "x.txt").write_text("x\n", encoding="utf-8")
    p.refus_agregateur("repertoire extra/ non vide a la racine", recu,
                       ("racine", "repertoire", "inattendu", "extra"))
    recu = p.copier_recu(base, nominal, "rep_lien")
    os.symlink("META.txt", recu / "lien")
    p.refus_agregateur("lien symbolique a la racine", recu, ("racine", "lien"))
    recu = p.copier_recu(base, nominal, "bin_pollue")
    (recu / "bin" / "autre").write_text("", encoding="utf-8")
    p.refus_agregateur("bin/autre ajoute", recu, ("bin/", "autre", "mhgp6_profile_sonde"))
    recu = p.copier_recu(base, nominal, "intrus_racine")
    (recu / "intrus_racine.txt").write_text("", encoding="utf-8")
    p.refus_agregateur("fichier intrus a la racine", recu,
                       ("racine", "inattendu", "intrus_racine.txt"))
    recu = p.copier_recu(base, nominal, "out_absent")
    shutil.rmtree(recu / "out")
    p.refus_agregateur("out/ retire", recu, ("racine", "absent", "out"))
    # Lanceur : vide/ cree APRES l'agregateur reel, AVANT le manifeste — un
    # manifeste de fichiers ne le voit pas ; l'inventaire des repertoires oui.
    reel = shutil.which("python3") or sys.executable
    fakebin = base / "fakebin_python3_vide"
    fakebin.mkdir()
    ecrire_exec(fakebin / "python3", FAUX_PYTHON3_VIDE.replace("%REAL%", reel))
    out = base / "rep_vide_lanceur"
    partial = Path(str(out) + ".partial")
    r = p.lancer(out, faux, "64", "4", {"PATH": f"{fakebin}:{os.environ['PATH']}"})
    p.check("vide/ tardif : rc=3", r.returncode == 3, f"rc={r.returncode} {r.stderr[-200:]!r}")
    p.check("vide/ tardif : jamais publie, reste en .partial", not out.exists() and partial.is_dir())
    if partial.is_dir():
        meta = (partial / "META.txt").read_text(encoding="utf-8")
        p.check("vide/ tardif : inventaire des repertoires en echec au META, vide nomme",
                "campagne INVALIDE : repertoires du reçu != {bin out}" in meta and "> vide" in meta,
                repr(meta[-300:]))
        p.check("vide/ tardif : le manifeste existe et le repertoire vide est bien la",
                (partial / "SHA256SUMS").is_file() and (partial / "vide").is_dir())


def scene_protocoles_alteres(p, base, faux, nominal):
    p.scene("(r) protocoles archives alteres/retires : agregateur 1 (hash recalcule != META) ; copie du lanceur alteree apres le manifeste : INVALIDE 3")
    for nom, champ in (("protocole_lanceur.sh", "sha256_lanceur"),
                       ("protocole_agregateur.py", "sha256_agregateur")):
        recu = p.copier_recu(base, nominal, f"altere_{nom}")
        with open(recu / nom, "a", encoding="utf-8") as f:
            f.write("# altere apres coup\n")
        p.refus_agregateur(f"{nom} altere apres coup", recu, (nom, "recalcule", champ))
    recu = p.copier_recu(base, nominal, "lanceur_retire")
    (recu / "protocole_lanceur.sh").unlink()
    p.refus_agregateur("protocole_lanceur.sh retire", recu, ("protocole_lanceur.sh", "absent"))
    # Lanceur : la copie archivee est alteree APRES la generation du manifeste
    # (faux sha256sum n'agissant que sur la generation) : relue avant
    # publication, elle ne porte plus le hash du META => INVALIDE 3.
    reel = shutil.which("sha256sum")
    p.check("sha256sum reel trouve", reel is not None)
    if reel is None:
        return
    fakebin = base / "fakebin_sha_protocole"
    fakebin.mkdir()
    ecrire_exec(fakebin / "sha256sum", FAUX_SHA256SUM_TARDIF.replace("%REAL%", reel)
                .replace("%CIBLE%", "protocole_lanceur.sh"))
    out = base / "protocole_tardif"
    partial = Path(str(out) + ".partial")
    r = p.lancer(out, faux, "64", "4", {"PATH": f"{fakebin}:{os.environ['PATH']}"})
    p.check("copie du lanceur alteree apres le manifeste : rc=3", r.returncode == 3,
            f"rc={r.returncode} {r.stderr[-200:]!r}")
    p.check("copie du lanceur alteree : jamais publiee, reste en .partial",
            not out.exists() and partial.is_dir())
    if partial.is_dir():
        meta = (partial / "META.txt").read_text(encoding="utf-8")
        p.check("copie du lanceur alteree : motif « relu avant publication != sha256_lanceur »",
                "campagne INVALIDE : protocole_lanceur.sh archive relu avant publication" in meta
                and "sha256_lanceur du META" in meta, repr(meta[-300:]))
        p.check("copie du lanceur alteree : le manifeste existait deja (mutation apres lui)",
                (partial / "SHA256SUMS").is_file()
                and "mutation apres le manifeste" in
                (partial / "protocole_lanceur.sh").read_text(encoding="utf-8"))


def scene_identite_cible(p, base, faux, nominal, bench):
    p.scene("(s) identite de cible (claim borne) : mutant produit dans un .status/META => 1 ; lanceur a bras hors sonde => 2 ; plan altere en campagne => 3")
    recu = p.copier_recu(base, nominal, "inject_produit")
    p.remplacer(recu / "out" / "ablation-mat-sans-tris_n64_r1.status",
                "--inject=ablation-mat-sans-tris", "--inject=drop-nonmerge", ".status drop-nonmerge")
    p.refus_agregateur(".status commande= avec --inject=drop-nonmerge", recu,
                       ("ablation-mat-sans-tris_n64_r1.status", "drop-nonmerge", "hors sonde"))
    recu = p.copier_recu(base, nominal, "inject_double")
    p.remplacer(recu / "out" / "ablation-mat-sans-tris_n64_r1.status",
                "--inject=ablation-mat-sans-tris", "--inject=ablation-mat-sans-copie",
                ".status mutant d'un autre bras")
    p.refus_agregateur(".status commande= avec l'ablation d'un autre bras", recu,
                       ("ablation-mat-sans-tris_n64_r1.status", "exactement le mutant du bras"))
    recu = p.copier_recu(base, nominal, "temoin_injecte")
    p.remplacer(recu / "out" / "aucune_n64_r1.status", "--fold-join=1 ",
                "--fold-join=1 --inject=ablation-mat-sans-tris", "temoin avec --inject=")
    p.refus_agregateur("bras temoin avec un --inject=", recu,
                       ("aucune_n64_r1.status", "temoin", "--inject="))
    recu = p.copier_recu(base, nominal, "meta_emises_produit")
    p.remplacer(recu / "META.txt", "injections_emises=ablation-mat-sans-copie",
                "injections_emises=drop-nonmerge ablation-mat-sans-copie", "META injections_emises")
    p.refus_agregateur("META injections_emises avec drop-nonmerge", recu,
                       ("injections_emises", "trois ablations"))
    recu = p.copier_recu(base, nominal, "meta_bras_produit")
    p.remplacer(recu / "META.txt", "ablations=aucune ", "ablations=aucune drop-nonmerge ",
                "META ablations")
    p.refus_agregateur("META ablations avec un cinquieme bras drop-nonmerge", recu,
                       ("ablations", "hors sonde"))
    recu = p.copier_recu(base, nominal, "meta_identite_autre")
    remplacer_ligne(p, recu / "META.txt", "identite_cible=", "identite_cible=mhgp6_profile",
                    "identite_cible autre")
    p.refus_agregateur("META identite_cible ne nommant pas mhgp6_profile_sonde", recu,
                       ("identite_cible", "mhgp6_profile_sonde"))
    # Lanceur mutant : ABLATIONS etendu a un mutant produit => refus 2 avant
    # toute ecriture (la liste des --inject= emissibles est fermee).
    mut = base / "lanceur_mutant"
    mut.mkdir()
    shutil.copy(p.agregateur, mut / "sonde_ablation_reduce.py")
    texte = p.lanceur.read_text(encoding="utf-8")
    p.check("lanceur mutant : la ligne ABLATIONS existe telle quelle", LIGNE_ABLATIONS in texte)
    ecrire_exec(mut / "sonde_ablation_reduce.sh",
                texte.replace(LIGNE_ABLATIONS, LIGNE_ABLATIONS[:-1] + ' drop-nonmerge"', 1))
    out = base / "bras_produit"
    r = p.lancer(out, faux, "64", "4", lanceur=mut / "sonde_ablation_reduce.sh")
    p.check("lanceur a bras drop-nonmerge : refus 2", r.returncode == 2, f"rc={r.returncode}")
    p.check("lanceur a bras drop-nonmerge : motif « hors sonde »", "hors sonde" in r.stderr,
            repr(r.stderr[-200:]))
    p.check("lanceur a bras drop-nonmerge : aucun dossier laisse",
            not out.exists() and not Path(str(out) + ".partial").exists())
    # Plan altere en campagne (le binaire reecrit bras=D en drop-nonmerge au
    # premier tuple) : la garde de run_one refuse avant d'emettre.
    out2 = base / "plan_altere"
    partial2 = Path(str(out2) + ".partial")
    r2 = p.lancer(out2, faux, "64 128", "4", p.env_mutant(base, "plan_hors_sonde", 3))
    p.check("plan altere en campagne : rc=3", r2.returncode == 3,
            f"rc={r2.returncode} {r2.stderr[-200:]!r}")
    p.check("plan altere : jamais publie, reste en .partial", not out2.exists() and partial2.is_dir())
    if partial2.is_dir():
        meta2 = (partial2 / "META.txt").read_text(encoding="utf-8")
        statuts = list((partial2 / "out").glob("*.status"))
        p.check("plan altere : motif « bras drop-nonmerge hors sonde » au META",
                "campagne INVALIDE : bras drop-nonmerge hors sonde" in meta2, repr(meta2[-300:]))
        p.check("plan altere : arret avant la matrice complete, aucun .status n'emet drop-nonmerge",
                0 < len(statuts) < 32 and not any(
                    "drop-nonmerge" in s.read_text(encoding="utf-8") for s in statuts),
                f"{len(statuts)} statuts")
        p.check("plan altere : plan.txt du reçu porte bien la mutation",
                "bras=drop-nonmerge" in (partial2 / "plan.txt").read_text(encoding="utf-8"))
    # La cible REELLE, si elle est construite : elle accepte un mutant produit
    # (claim borne, verifie ; jamais une mesure).
    reel = bench.parent.parent / "build" / "v6" / "mhgp6_profile_sonde"
    if reel.is_file() and os.access(reel, os.X_OK):
        try:
            rr = subprocess.run([str(reel), "--family=uniform", "--n=64", "--threads=1",
                                 "--inject=drop-nonmerge"], capture_output=True, timeout=300)
            p.check("build/v6/mhgp6_profile_sonde accepte --inject=drop-nonmerge (claim P2 borne, "
                    "non fermable par le lanceur)", rr.returncode == 0, f"rc={rr.returncode}")
        except subprocess.TimeoutExpired:
            p.check("build/v6/mhgp6_profile_sonde : sonde d'identite en temps borne", False)
    else:
        print("  (build/v6/mhgp6_profile_sonde absent : claim P2 borne non re-verifie sur la cible reelle)")


def scene_toctou(p, base, faux):
    p.scene("(t) TOCTOU avant scellement : sortie modifiee apres l'agregateur (manifeste scelle le mute) ou apres le manifeste : INVALIDE 3")
    reel_py = shutil.which("python3") or sys.executable
    fakebin = base / "fakebin_python3_mutation"
    fakebin.mkdir()
    ecrire_exec(fakebin / "python3", FAUX_PYTHON3_MUTATION.replace("%REAL%", reel_py))
    out = base / "toctou_agregation"
    partial = Path(str(out) + ".partial")
    r = p.lancer(out, faux, "64", "4", {"PATH": f"{fakebin}:{os.environ['PATH']}"})
    p.check("mutation apres agregation : rc=3", r.returncode == 3,
            f"rc={r.returncode} {r.stderr[-200:]!r}")
    p.check("mutation apres agregation : jamais publiee, reste en .partial",
            not out.exists() and partial.is_dir())
    if partial.is_dir():
        meta = (partial / "META.txt").read_text(encoding="utf-8")
        p.check("mutation apres agregation : motif « modifie entre la lecture de l'agregateur et le scellement »",
                "campagne INVALIDE : out/aucune_n64_r1.txt modifie entre la lecture de l'agregateur "
                "et le scellement" in meta, repr(meta[-300:]))
        sums = partial / "SHA256SUMS"
        p.check("mutation apres agregation : le manifeste existe", sums.is_file())
        if sums.is_file():
            entrees = dict(l.split("  ", 1)[::-1]
                           for l in sums.read_text(encoding="utf-8").splitlines())
            fichier = partial / "out" / "aucune_n64_r1.txt"
            p.check("mutation apres agregation : le manifeste SCELLE le contenu mute (sha256sum -c "
                    "seul aurait publie)", entrees.get("out/aucune_n64_r1.txt") == sha256(fichier)
                    and "mutation apres agregation" in fichier.read_text(encoding="utf-8"))
        p.check("mutation apres agregation : l'agregateur reel avait accepte (resume.txt non vide)",
                (partial / "resume.txt").is_file() and (partial / "resume.txt").stat().st_size > 0)
    # Apres le manifeste, avant sa verification finale : la derniere
    # operation avant le mv est `sha256sum -c --strict`, qui refuse.
    reel_sha = shutil.which("sha256sum")
    p.check("sha256sum reel trouve", reel_sha is not None)
    if reel_sha is None:
        return
    fakebin2 = base / "fakebin_sha_tardif"
    fakebin2.mkdir()
    ecrire_exec(fakebin2 / "sha256sum", FAUX_SHA256SUM_TARDIF.replace("%REAL%", reel_sha)
                .replace("%CIBLE%", "out/aucune_n64_r1.txt"))
    out2 = base / "toctou_manifeste"
    partial2 = Path(str(out2) + ".partial")
    r2 = p.lancer(out2, faux, "64", "4", {"PATH": f"{fakebin2}:{os.environ['PATH']}"})
    p.check("mutation apres le manifeste : rc=3", r2.returncode == 3,
            f"rc={r2.returncode} {r2.stderr[-200:]!r}")
    p.check("mutation apres le manifeste : jamais publiee, reste en .partial",
            not out2.exists() and partial2.is_dir())
    if partial2.is_dir():
        meta2 = (partial2 / "META.txt").read_text(encoding="utf-8")
        p.check("mutation apres le manifeste : verification finale en echec au META",
                "campagne INVALIDE : verification finale sha256sum -c" in meta2, repr(meta2[-300:]))
        p.check("mutation apres le manifeste : le fichier mute et le manifeste anterieur existent",
                (partial2 / "SHA256SUMS").is_file() and "mutation apres le manifeste" in
                (partial2 / "out" / "aucune_n64_r1.txt").read_text(encoding="utf-8"))


def scene_diff_fail_open(p, base, faux):
    p.scene("(u) diff fail-open : faux diff muet rendant 2 (erreur) ou 1 (different) : INVALIDE 3, jamais « identique »")
    for rc_faux, motif in ((2, "comparaison impossible (diff rc=2)"),
                           (1, "(< attendu absent, > inattendu)")):
        fakebin = base / f"fakebin_diff_{rc_faux}"
        fakebin.mkdir()
        ecrire_exec(fakebin / "diff", FAUX_DIFF.replace("%RC%", str(rc_faux)))
        out = base / f"diff_rc{rc_faux}"
        partial = Path(str(out) + ".partial")
        r = p.lancer(out, faux, "64", "4", {"PATH": f"{fakebin}:{os.environ['PATH']}"})
        p.check(f"faux diff rc={rc_faux} muet : rc=3", r.returncode == 3,
                f"rc={r.returncode} {r.stderr[-200:]!r}")
        p.check(f"faux diff rc={rc_faux} : jamais publie, reste en .partial",
                not out.exists() and partial.is_dir())
        if partial.is_dir():
            meta = (partial / "META.txt").read_text(encoding="utf-8")
            p.check(f"faux diff rc={rc_faux} : motif « {motif} » au META",
                    "campagne INVALIDE : inventaire du reçu != ensemble attendu" in meta
                    and motif in meta, repr(meta[-300:]))
            p.check(f"faux diff rc={rc_faux} : le manifeste existe, l'agregateur avait accepte",
                    (partial / "SHA256SUMS").is_file()
                    and (partial / "resume.txt").stat().st_size > 0)


def main():
    if len(sys.argv) != 2:
        print("usage: sonde_ablation_gate.py <dossier bench>", file=sys.stderr)
        return 2
    bench = Path(sys.argv[1]).resolve()
    p = Porte(bench)
    if not p.lanceur.is_file() or not p.agregateur.is_file():
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
        nominal = base / "nominal"
        if not nominal.is_dir():
            p.check("scenes (j)–(n) : reçu nominal (a) requis comme base des reçus synthetiques", False)
        else:
            scene_taille_dupliquee(p, base, faux, nominal)
            scene_hash_vide(p, base, faux, nominal)
            scene_champ_duplique(p, base, nominal)
            scene_plan_non_williams(p, base, nominal)
            scene_inventaire(p, base, faux, nominal)
            scene_runs_obligatoires(p, base, nominal, bench)
            scene_profil_malforme(p, base, nominal)
            scene_repertoire_inattendu(p, base, faux, nominal)
            scene_protocoles_alteres(p, base, faux, nominal)
            scene_identite_cible(p, base, faux, nominal, bench)
            scene_toctou(p, base, faux)
            scene_diff_fail_open(p, base, faux)
    if p.echecs:
        print(f"sonde_ablation_gate : {p.echecs} controle(s) en echec sur {p.scenes} scenes")
        return 1
    print(f"sonde_ablation_gate : {p.scenes} scenes vertes (nominal apparie, refus de parametres, "
          "copie alteree, agregateur fail-closed, manifeste fatal, binaire produit, taille dupliquee, "
          "hash vide, champ duplique, plan non Williams, inventaire exact, compteurs obligatoires, "
          "profil malforme, repertoire inattendu, protocoles recalcules, identite de cible bornee, "
          "TOCTOU avant scellement, diff fail-open)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
