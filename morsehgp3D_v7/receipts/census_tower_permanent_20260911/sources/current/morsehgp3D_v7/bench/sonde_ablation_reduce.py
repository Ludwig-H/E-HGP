#!/usr/bin/env python3
"""Agregateur FAIL-CLOSED de la sonde d'ablation du reduce
(bench/sonde_ablation_reduce.sh ; fermeture minimale exigee par
audits/ALERTE_SONDE_ABLATION_REDUCE_20260902.md, puis les cinq
contre-fixtures encore vertes de son « Etat du WIP » et du § 5.21 de
REPONSE_AUDITEURS_MULTICPU_V6 : taille dupliquee, hash vide, champ duplique,
carre latin non Williams, artefact inattendu dans out/ ; puis les dents du
2 septembre d'ETAT_COURANT (ligne « sonde equilibree … harnais ») et de la fin
du § 5.22 : compteurs facultatifs, grammaire profil incomplete, repertoire
vide a la racine, hashes de protocoles non recalcules, identite de cible) ;
puis les deux coutures ACTIVES nommees par
REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902 (« Reponse au verrou de
pre-inscription 53610911 ») : outils de publication graves hors PATH (v5) et
LIAISON EXACTE commande / META / sortie (tout schema).

Autorite : META.txt (n_list, reps, ablations, statut, binaire_sha256,
runs_*, sha256_*, identite_cible, injections_*, interpreteur, famille,
parametres, outils, topologie, cpuset, affinite) et plan.txt (bloc,
position, bras — carre de Williams). Le resume n'est produit que si :
  - META.txt porte le schema COURANT (v5) ou un schema ANTERIEUR accepte
    EXPLICITEMENT (v2 : les champs d'identite de cible, l'interpreteur et
    les champs v5 n'existaient pas ; v3 : l'interpreteur et les champs v5 ;
    v4 : les champs v5 — outils, topologie, cpuset, affinite ; le resume
    imprime alors « claim borne, NON VERIFIE » pour les seuls champs que ce
    schema ne gravait pas — tout le reste est aussi strict qu'en v5),
    aucune ligne `campagne INVALIDE`, des tailles distinctes (deux tuples
    de meme taille porteraient le meme tag), des hash a la grammaire EXACTE
    ^[0-9a-f]{64}$ (binaire, lanceur, agregateur) ;
  - LIAISON commande / META / sortie (tout schema : v2 gravait deja
    famille= et parametres=) : `famille=` et `parametres=` (threads, cpus,
    fold_inflight, fold_join=1, seed, s, smax) sont OBLIGATOIRES ; pour
    CHAQUE tuple, l'argv EXACT est reconstruit depuis META (`taskset -c
    <cpus> bin/mhgp7_profile_sonde --family=<famille> --n=<n> --s=<s>
    --smax=<smax> --seed=<seed> --threads=<threads>
    --fold-inflight=<inflight> --fold-join=<join> [--inject=<bras>]`) et
    doit etre EGAL, apres normalisation par blancs, a la ligne commande= du
    .status (ni argument en plus, ni en moins, ni ordre different) ; la
    sortie porte EXACTEMENT UNE ligne d'identite `famille= n= [coord=] s=
    smax= seed= threads=` dont chaque champ est egal a l'argv ; sur la ligne
    profil_kind, fold_join= et (s'il est present) inflight_demande= sont
    egaux a l'argv ; coord= (si imprime) est identique entre tous les tuples
    d'une meme taille (meme entree). Tout ecart est un refus ;
  - v5 : `outils=` est une liste `nom=chemin:sha256` (chemin absolu
    canonique sans blanc, hash 64-hex, noms uniques) qui contient au moins
    les outils de publication (mv sha256sum cmp diff find sort taskset git
    readlink cp chmod mkdir mktemp rm uname xargs) — l'agregateur verifie la
    GRAMMAIRE, jamais les hashes contre la machine courante (le resume doit
    rester reproductible ailleurs) ; `topologie=` (sockets, coeurs, fils,
    cpus_en_ligne), `cpuset=` (liste et masque du lanceur) et `affinite=`
    (cpus, fils_materiels, coeurs_physiques, sockets) sont coherents entre
    eux et avec parametres cpus= : cpus ⊆ cpuset ⊆ … et cpus ⊆ cpus_en_ligne,
    fils_materiels = |cpus|, coeurs_physiques ≤ fils_materiels ≤ fils ;
  - `runs_effectues` et `runs_attendus` sont OBLIGATOIRES (tout schema : le
    lanceur les a toujours graves) et egaux au cardinal exact bras x tailles
    x repetitions ; un champ obligatoire absent est un refus (jamais un
    defaut substitue) ;
  - `statut` est EXACTEMENT le libelle exploratoire du lanceur (jeton
    exploratory_noncausal_upper_bounds et sa glose, LIBELLE_STATUT) : un
    statut promu, un prefixe etendu, une glose reformulee sont des refus ;
    `ablations` est EXACTEMENT la liste fermee en ORDRE FIXE temoin puis les
    trois ablations (BRAS_SONDE : bras manquant, renomme, duplique, permute
    ou hors sonde = refus) ; identite de cible : mhgp7_profile_sonde accepte
    tout mutant de kMutants, donc un reçu ne vaut que si les seuls --inject=
    emis — lus sur la ligne commande= de CHAQUE .status — sont ces trois
    ablations, une par tuple non temoin ; v3+ : injections_autorisees et
    injections_emises du META sont cette liste exacte en ordre fixe et
    identite_cible nomme mhgp7_profile_sonde ; v4+ : `interpreteur` est un
    chemin ABSOLU sans blanc (l'interpreteur de la reagregation du jeu
    scelle par le lanceur, grave avant la campagne) ;
  - les hashes sha256_lanceur / sha256_agregateur du META sont RECALCULES
    depuis protocole_lanceur.sh / protocole_agregateur.py presents dans le
    reçu (difference ou absence = refus : 64 zeros ne sont pas un hash) ;
  - la RACINE du reçu porte exactement les fichiers attendus (+ SHA256SUMS
    optionnel, + le couple worktree_diff.* si et seulement si
    worktree_sources_modifies != 0) et exactement les repertoires bin/ et
    out/ : un repertoire VIDE ou inattendu, un lien, une entree speciale est
    un refus ; bin/ contient exactement mhgp7_profile_sonde ;
  - tout champ est UNIQUE : un champ present deux fois dans META.txt, un
    .status, une ligne de plan.txt ou une ligne profil_reduce est un refus
    (jamais la premiere ni la derniere valeur) ; temps_mur_ms et rss_max_kb
    exactement une fois par sortie ;
  - la matrice bras x tailles x repetitions est EXACTE : out/ contient
    exactement le triplet <tag>.txt|.err|.status de chaque tuple attendu et
    RIEN d'autre (ni intrus, ni repertoire, ni lien, ni out/SHA256SUMS),
    chaque .status a code=0, un contenu coherent avec son nom, la position du
    plan, et des sha256_avant/apres 64-hex egaux au binaire_sha256 du META ;
    HASHES.txt porte exactement une ligne 64-hex par tuple ; la copie privee
    bin/mhgp7_profile_sonde a le hash du META ;
  - plan.txt est un carre de WILLIAMS : par groupe de quatre blocs chaque
    bras occupe chaque position une fois ET chaque succession ordonnee X→Y
    (bras adjacents dans un bloc) apparait exactement une fois — un carre
    latin cyclique (ABCD/BCDA/CDAB/DABC) est refuse ;
  - chaque sortie porte EXACTEMENT UNE ligne `profil_kind=` a jetons
    `cle=valeur` uniques, avec `profil_kind=reduce_v2` (jamais reduce_v1,
    jamais reduce_v2+liveness : autre instrumentation) et `fold_join=1`
    (etage B isole) ; le jeton `layout=` — que le prototype KeyCSR ajoute a
    cette ligne — n'est exige que s'il est present et vaut alors
    EXACTEMENT `classic` (`csr` : autre objet, autre sonde) ; la ligne
    absente, dupliquee ou malformee est un refus ;
  - chaque sortie porte exactement dix lignes `profil_reduce K=1..10` (une
    par K, aucune autre) a la grammaire STRICTE : `K=<entier>` en tete, puis
    des jetons `nom=valeur` (nom ^[a-z][a-z0-9_]*$, valeur decimale
    ^-?[0-9]+(\\.[0-9]+)?$ finie) ; un jeton sans `=`, une valeur vide, non
    numerique, nan/inf ou en notation exposant, un K non entier, une ligne
    tronquee sont des refus — jamais ignores ; toutes les fenetres requises
    presentes, `somme` coherente avec ses composantes (seuil 0.0051 du
    § 5.13), `temps_mur_ms` et `rss_max_kb` presents, uniques et finis.
Une mesure absente n'est JAMAIS remplacee par zero : elle est un refus.

Tableau principal : differences APPARIEES PAR BLOC (bras − aucune au sein
du meme bloc et de la meme taille), mediane [min ; max] sur les blocs.
Second tableau : medianes brutes par bras. Le bras ablation-post-cle-factice
est une BORNE COMPOSITE (lecture keys[] + tri de cles egales), jamais
« lecture seule ». Bornes exploratoires non causales sur binaire
instrumente : jamais un benchmark, jamais un mur, jamais un choix de palier.

Codes : 0 resume sur stdout ; 1 refus (motif sur stderr) ; 2 usage.
Aucun assert (doit tenir sous python3 -O).
"""
import hashlib
import math
import os
import re
import statistics
import sys

SCHEMA = "e-hgp.sonde-ablation-reduce.v5"
# Champs du META nes avec un schema : v3 = identite de cible ; v4 =
# interpreteur de la reagregation ; v5 = outils de publication resolus hors
# PATH, topologie et cpuset attestes, affinite recalculee. Un schema
# ANTERIEUR accepte EXPLICITEMENT est nomme avec les champs que son lanceur
# ne gravait pas encore — ces champs-la sont alors un claim borne, NON
# VERIFIE au META, imprime en tete du resume (presents malgre tout, ils sont
# verifies). Le v1 est refuse.
CHAMPS_META_V3 = ("identite_cible", "injections_autorisees", "injections_emises")
CHAMPS_META_V4 = ("interpreteur",)
CHAMPS_META_V5 = ("outils", "topologie", "cpuset", "affinite")
SCHEMAS_ANTERIEURS = {
    "e-hgp.sonde-ablation-reduce.v2": CHAMPS_META_V3 + CHAMPS_META_V4 + CHAMPS_META_V5,
    "e-hgp.sonde-ablation-reduce.v3": CHAMPS_META_V4 + CHAMPS_META_V5,
    "e-hgp.sonde-ablation-reduce.v4": CHAMPS_META_V5,
}
HEX64 = re.compile(r"^[0-9a-f]{64}$")
# Liaison commande / META / sortie (tout schema) : parametres= du META
# (tous obligatoires, fold_join=1), argv reconstruit, ligne d'identite.
PARAMETRES_REQUIS = ("threads", "cpus", "fold_inflight", "fold_join", "seed", "s", "smax")
PARAMETRES_ENTIERS = ("threads", "fold_inflight", "seed", "s", "smax")
NOM_FAMILLE = re.compile(r"^[a-z][a-z0-9_]*$")
CPU_LISTE = re.compile(r"^[0-9]+(?:-[0-9]+)?(?:,[0-9]+(?:-[0-9]+)?)*$")
LIGNE_IDENTITE = re.compile(r"^famille=.*$", re.M)
IDENTITE_REQUIS = ("famille", "n", "s", "smax", "seed", "threads")
# Correspondance champ d'identite -> option d'argv (valeurs EGALES).
IDENTITE_ARGV = (("famille", "--family"), ("n", "--n"), ("s", "--s"), ("smax", "--smax"),
                 ("seed", "--seed"), ("threads", "--threads"))
# v5 : outils graves `nom=chemin:sha256` ; grammaire seule (jamais les
# hashes contre la machine courante : le resume reste reproductible ailleurs).
OUTIL_JETON = re.compile(r"^([a-z][a-z0-9_+.-]*)=(/[^\s:]+):([0-9a-f]{64})$")
OUTILS_REQUIS = ("mv", "sha256sum", "cmp", "diff", "find", "sort", "taskset", "git",
                 "readlink", "cp", "chmod", "mkdir", "mktemp", "rm", "uname", "xargs")
TOPOLOGIE = re.compile(r"^sockets=([0-9]+) coeurs=([0-9]+) fils=([0-9]+) cpus_en_ligne=(\S+)(?: \(.*\))?$")
CPUSET = re.compile(r"^(\S+) masque=([0-9a-f,]+)(?: \(.*\))?$")
AFFINITE = re.compile(r"^cpus=(\S+) fils_materiels=([0-9]+) coeurs_physiques=([0-9]+) sockets=([0-9]+)(?: \(.*\))?$")
NOM_CHAMP = re.compile(r"^[a-z][a-z0-9_]*$")
DECIMAL = re.compile(r"^-?[0-9]+(?:\.[0-9]+)?$")
LIGNE_PROFIL = re.compile(r"^profil_reduce(?:[ \t]+(.*))?[ \t]*$", re.M)
LIGNE_KIND = re.compile(r"^profil_kind=.*$", re.M)
# Verrou de la ligne `profil_kind=` : jetons EXIGES a valeur exacte, jetons
# verifies seulement s'ils sont presents (layout= : ajoute par le prototype
# KeyCSR ; cette sonde ne mesure que la route classic).
PROFIL_JETONS_REQUIS = (("profil_kind", "reduce_v2"), ("fold_join", "1"))
PROFIL_JETONS_SI_PRESENTS = (("layout", "classic"),)
WINDOWS = ("init", "touch", "pre", "unite", "post_remplissage",
           "materialisation_tri_copie", "partition", "liberation", "somme")
# Composantes dont `somme` est la somme (liveness figure dans reduce_v2 ; si
# elle est absente elle ne compte pas — mais une composante requise absente
# reste un refus).
COMPOSANTES = ("init", "touch", "pre", "unite", "post_remplissage",
               "materialisation_tri_copie", "liveness", "partition",
               "liberation")
CHAMPS_REQUIS = WINDOWS + ("mur_reduce_interne", "residuel")
KS = tuple(range(1, 11))
KS_SHOWN = (8, 9, 10)
BRAS_TEMOIN = "aucune"
# Identite de cible (claim BORNE) : mhgp7_profile_sonde est construit sous
# MHGP7_TESTING et `mutants_enable` accepte TOUT nom de kMutants
# (drop-nonmerge compris) ; la sonde ne selectionne que ces trois ablations
# et refuse tout reçu dont un .status en emet un autre.
ABLATIONS_SONDE = ("ablation-mat-sans-copie", "ablation-mat-sans-tris",
                   "ablation-post-cle-factice")
# Liste FERMEE des bras en ORDRE FIXE (celui du lanceur, ABLATIONS=...) :
# `ablations=` du META doit lui etre EGALE, pas seulement egale a l'ensemble.
BRAS_SONDE = (BRAS_TEMOIN,) + ABLATIONS_SONDE
IDENTITE_CIBLE = "mhgp7_profile_sonde"
STATUT_ATTENDU = "exploratory_noncausal_upper_bounds"
# Libelle EXACT grave par le lanceur (jeton + glose) : le seul statut d'un
# reçu de sonde — jamais une promotion, jamais un prefixe etendu.
LIBELLE_STATUT = (STATUT_ATTENDU + " (bornes exploratoires non causales sur binaire instrumente,"
                  " join=1 : jamais un benchmark, jamais un mur, jamais un choix de palier)")
SEUIL_SOMME = 0.0051
ETIQUETTES = {
    "ablation-mat-sans-copie": "borne : copie profonde retiree (objet change)",
    "ablation-mat-sans-tris": "borne : tris retires (objet change)",
    "ablation-post-cle-factice":
        "borne composite (lecture keys[] + tri de cles egales)",
}
PARTS = ("materialisation_tri_copie", "post_remplissage")
EXTENSIONS = ("txt", "err", "status")
CHAMPS_STATUS = ("ablation", "n", "rep", "position", "code", "commande",
                 "sha256_avant", "sha256_apres")
CHAMPS_META = ("schema", "n_list", "reps", "ablations", "statut",
               "binaire_sha256", "sha256_lanceur", "sha256_agregateur",
               "runs_effectues", "runs_attendus", "worktree_sources_modifies",
               "famille", "parametres")
FICHIERS_RACINE = ("HASHES.txt", "META.txt", "lscpu.txt", "plan.txt",
                   "protocole_agregateur.py", "protocole_lanceur.sh",
                   "resume.err", "resume.txt")
FICHIERS_RACINE_OPTIONNELS = ("SHA256SUMS",)   # absent tant que le lanceur agrege
FICHIERS_WORKTREE = ("worktree_diff.patch", "worktree_diff_summary.txt")
REPERTOIRES_RACINE = ("bin", "out")
COPIE_PRIVEE = "mhgp7_profile_sonde"
PROTOCOLES = (("protocole_lanceur.sh", "sha256_lanceur"),
              ("protocole_agregateur.py", "sha256_agregateur"))


class Refus(Exception):
    """Motif de refus (code 1) — jamais un zero substitue."""


def exiger_hex64(valeur, contexte):
    """Grammaire EXACTE d'un sha256 : une chaine vide n'est jamais « egale »."""
    if not HEX64.match(valeur):
        raise Refus(f"{contexte} : sha256 vide ou hors grammaire 64-hex ({valeur!r})")
    return valeur


def tokens_kv(tokens, contexte):
    """Jetons `cle=valeur` -> dict ; jeton sans `=` ou cle dupliquee = refus."""
    kv = {}
    for tok in tokens:
        if "=" not in tok:
            raise Refus(f"{contexte} : jeton malforme sans `=` ({tok!r})")
        k, v = tok.split("=", 1)
        if k in kv:
            raise Refus(f"{contexte} : champ {k}= duplique ({kv[k]!r} puis {v!r}) — "
                        "jamais la premiere ni la derniere valeur")
        kv[k] = v
    return kv


def lire_kv(path, requis=()):
    """Fichier `cle=valeur` par ligne ; un champ present deux fois est un refus,
    un champ OBLIGATOIRE absent aussi (jamais un defaut substitue)."""
    kv = {}
    base = os.path.basename(path)
    with open(path, encoding="utf-8") as f:
        for no, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            if k in kv:
                raise Refus(f"{base} : champ {k}= duplique (ligne {no}, {kv[k]!r} puis {v!r})"
                            " — jamais la premiere ni la derniere valeur")
            kv[k] = v
    for k in requis:
        if k not in kv:
            raise Refus(f"{base} : champ obligatoire {k}= absent — refus, jamais un defaut")
    return kv


def entier(texte, contexte):
    try:
        return int(texte)
    except ValueError:
        raise Refus(f"{contexte}={texte!r} non entier")


def lire_meta(work):
    """-> (meta, ns, reps, bras, bornes) ; bornes = claims NON VERIFIES d'un
    schema anterieur, imprimes en tete du resume."""
    path = os.path.join(work, "META.txt")
    if not os.path.isfile(path):
        raise Refus("META.txt absent")
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.startswith("campagne INVALIDE"):
                raise Refus(f"META.txt porte « {line.strip()} » — reçu invalide, jamais agrege")
    meta = lire_kv(path, CHAMPS_META)
    schema = meta["schema"]
    if schema == SCHEMA:
        absents = ()
    elif schema in SCHEMAS_ANTERIEURS:
        absents = SCHEMAS_ANTERIEURS[schema]
    else:
        raise Refus(f"META.txt : schema {schema!r} ni courant ({SCHEMA}) ni anterieur accepte "
                    f"{sorted(SCHEMAS_ANTERIEURS)}")
    bornes = []
    for k in CHAMPS_META_V3 + CHAMPS_META_V4 + CHAMPS_META_V5:
        if k not in meta and k not in absents:
            raise Refus(f"META.txt : champ obligatoire {k}= absent (schema {schema})")
    if absents:
        bornes.append(f"{' / '.join(absents)} : claim borne, NON VERIFIE au META (schema {schema}"
                      " anterieur a ces champs) ; les --inject= de chaque .status sont neanmoins"
                      " verifies contre les trois ablations de la sonde, la liaison commande /"
                      " META / sortie (argv exact, ligne d'identite, profil_kind) est verifiee"
                      " quel que soit le schema, et la reagregation du jeu scelle reste rejouable"
                      " depuis protocole_agregateur.py")
        if any(k in absents for k in CHAMPS_META_V5):
            bornes.append("outils de publication (mv, sha256sum, cmp, diff, find, sort, taskset…)"
                          " NON graves : rien ne prouve qu'ils ont ete resolus hors PATH ni que"
                          " la topologie / le cpuset ont ete attestes (schema anterieur a v5)")
    # Champs d'identite et d'interpreteur : verifies des qu'ils sont presents.
    if "identite_cible" in meta and not meta["identite_cible"].startswith(IDENTITE_CIBLE):
        raise Refus(f"META.txt : identite_cible={meta['identite_cible']!r} ne nomme pas "
                    f"{IDENTITE_CIBLE}")
    for k in ("injections_autorisees", "injections_emises"):
        if k in meta and tuple(meta[k].split()) != ABLATIONS_SONDE:
            raise Refus(f"META.txt : {k}={meta[k]!r} != les trois ablations de la sonde en ordre "
                        f"fixe {list(ABLATIONS_SONDE)} (identite de cible : jamais un mutant "
                        "produit de kMutants, jamais une liste permutee)")
    if "interpreteur" in meta:
        v = meta["interpreteur"]
        if not v.startswith("/") or any(c.isspace() for c in v) or "/../" in v + "/":
            raise Refus(f"META.txt : interpreteur={v!r} n'est pas un chemin absolu canonique sans "
                        "blanc — l'interpreteur de la reagregation est grave, jamais relatif")
    if "outils" in meta:
        verifier_outils(meta["outils"])
    if meta["statut"] != LIBELLE_STATUT:
        raise Refus(f"META.txt : statut={meta['statut']!r} != libelle exploratoire exact "
                    f"{LIBELLE_STATUT!r} — un reçu de sonde n'a pas d'autre statut (jamais une "
                    "promotion, jamais un prefixe etendu, jamais une glose reformulee)")
    for k in ("binaire_sha256", "sha256_lanceur", "sha256_agregateur"):
        exiger_hex64(meta[k], f"META.txt {k}=")
    ns = [entier(x, "META.txt n_list element") for x in meta["n_list"].split()]
    reps = entier(meta["reps"], "META.txt reps")
    bras = meta["ablations"].split()
    if not ns or reps <= 0:
        raise Refus("META.txt : matrice vide (n_list vide ou reps <= 0)")
    if any(n <= 0 for n in ns):
        raise Refus(f"META.txt : taille non positive dans n_list ({ns})")
    if len(ns) != len(set(ns)):
        doublons = sorted({n for n in ns if ns.count(n) > 1})
        raise Refus(f"META.txt : n_list avec taille dupliquee {doublons} — les tags "
                    "<bras>_n<n>_r<bloc> de deux tuples s'ecraseraient")
    if tuple(bras) != BRAS_SONDE:
        raise Refus(f"META.txt : ablations={bras} != liste fermee en ordre fixe {list(BRAS_SONDE)} "
                    "(bras hors sonde, duplique, absent, renomme ou permute : identite de cible "
                    "non tenue)")
    cardinal = len(bras) * len(ns) * reps
    for k in ("runs_effectues", "runs_attendus"):
        v = entier(meta[k], f"META.txt {k}")
        if v != cardinal:
            raise Refus(f"META.txt : {k}={v} != cardinal de la matrice {cardinal} "
                        f"({len(bras)} bras x {len(ns)} tailles x {reps} blocs)")
    entier(meta["worktree_sources_modifies"], "META.txt worktree_sources_modifies")
    return meta, ns, reps, bras, bornes


def expandre_cpus(spec, contexte):
    """Liste de CPU `a-b[,c[,d-e]]` -> ensemble d'entiers ; grammaire, plage
    inversee ou demesuree = refus."""
    if not CPU_LISTE.match(spec):
        raise Refus(f"{contexte} : liste de CPU hors grammaire a-b[,c[,d-e]] ({spec!r})")
    cpus = set()
    for part in spec.split(","):
        a, _, b = part.partition("-")
        lo, hi = int(a), int(b) if b else int(a)
        if lo > hi or hi - lo > 4096:
            raise Refus(f"{contexte} : plage de CPU inversee ou demesuree ({part!r})")
        cpus.update(range(lo, hi + 1))
    return cpus


def lire_parametres(meta):
    """`famille=` et `parametres=` du META -> dict des parametres de regime
    (threads, cpus, fold_inflight, fold_join, seed, s, smax) : tous
    obligatoires, entiers positifs, fold_join=1 (etage B isole), cpus a la
    grammaire des listes de CPU. C'est la source de l'argv reconstruit."""
    if not NOM_FAMILLE.match(meta["famille"]):
        raise Refus(f"META.txt : famille={meta['famille']!r} hors grammaire ^[a-z][a-z0-9_]*$")
    params = tokens_kv(meta["parametres"].split(), "META.txt parametres=")
    for k in PARAMETRES_REQUIS:
        if k not in params:
            raise Refus(f"META.txt : parametres= sans {k}= (attendus {list(PARAMETRES_REQUIS)}) — "
                        "l'argv de chaque commande ne peut pas etre reconstruit, refus")
    for k in PARAMETRES_ENTIERS:
        if entier(params[k], f"META.txt parametres {k}") <= 0:
            raise Refus(f"META.txt : parametres {k}={params[k]!r} non positif")
    if params["fold_join"] != "1":
        raise Refus(f"META.txt : parametres fold_join={params['fold_join']!r} != '1' — la sonde "
                    "isole l'etage B (join=1) ; un autre regime n'est pas une mesure de cette sonde")
    expandre_cpus(params["cpus"], "META.txt parametres cpus=")
    return params


def verifier_outils(valeur):
    """v5 `outils=` : jetons nom=chemin:sha256, chemins absolus canoniques
    sans blanc, hashes 64-hex, noms uniques, outils de publication presents."""
    noms = {}
    for tok in valeur.split():
        m = OUTIL_JETON.match(tok)
        if not m:
            raise Refus(f"META.txt : outils= jeton hors grammaire nom=/chemin/absolu:sha256 ({tok!r})")
        nom, chemin, h = m.groups()
        if nom in noms:
            raise Refus(f"META.txt : outils= nom {nom} duplique ({noms[nom]} puis {chemin})")
        if "//" in chemin or "/./" in chemin or "/../" in chemin + "/" or chemin.endswith("/"):
            raise Refus(f"META.txt : outils= chemin non canonique pour {nom} ({chemin!r})")
        exiger_hex64(h, f"META.txt outils= {nom}")
        noms[nom] = chemin
    manquants = sorted(set(OUTILS_REQUIS) - set(noms))
    if manquants:
        raise Refus(f"META.txt : outils= sans {manquants} — les outils de publication doivent "
                    "tous etre graves (resolus hors PATH, canoniques, ELF, haches)")
    return noms


def verifier_topologie(meta, params):
    """v5 : topologie=, cpuset=, affinite= coherents entre eux et avec
    parametres cpus= (chacun n'est verifie que s'il est present)."""
    cpus = expandre_cpus(params["cpus"], "META.txt parametres cpus=")
    en_ligne = fils = coeurs = None
    if "topologie" in meta:
        m = TOPOLOGIE.match(meta["topologie"])
        if not m:
            raise Refus(f"META.txt : topologie={meta['topologie']!r} hors grammaire "
                        "sockets= coeurs= fils= cpus_en_ligne=")
        sockets, coeurs, fils = int(m.group(1)), int(m.group(2)), int(m.group(3))
        en_ligne = expandre_cpus(m.group(4), "META.txt topologie cpus_en_ligne=")
        if not 1 <= sockets <= coeurs <= fils or len(en_ligne) != fils:
            raise Refus(f"META.txt : topologie incoherente (sockets={sockets} coeurs={coeurs} "
                        f"fils={fils} |cpus_en_ligne|={len(en_ligne)})")
        if not cpus <= en_ligne:
            raise Refus(f"META.txt : parametres cpus={params['cpus']} hors des CPU en ligne "
                        f"({m.group(4)}) — affinite non attestee")
    if "cpuset" in meta:
        m = CPUSET.match(meta["cpuset"])
        if not m:
            raise Refus(f"META.txt : cpuset={meta['cpuset']!r} hors grammaire <liste> masque=<hex>")
        cpuset = expandre_cpus(m.group(1), "META.txt cpuset=")
        if not cpus <= cpuset:
            raise Refus(f"META.txt : parametres cpus={params['cpus']} hors du cpuset du lanceur "
                        f"({m.group(1)}) — l'affinite demandee n'etait pas effective")
    if "affinite" in meta:
        m = AFFINITE.match(meta["affinite"])
        if not m:
            raise Refus(f"META.txt : affinite={meta['affinite']!r} hors grammaire cpus= "
                        "fils_materiels= coeurs_physiques= sockets=")
        if m.group(1) != params["cpus"]:
            raise Refus(f"META.txt : affinite cpus={m.group(1)} != parametres cpus={params['cpus']}")
        fils_mat, coeurs_phys, sockets_aff = int(m.group(2)), int(m.group(3)), int(m.group(4))
        if fils_mat != len(cpus):
            raise Refus(f"META.txt : affinite fils_materiels={fils_mat} != |cpus| = {len(cpus)}")
        if not 1 <= sockets_aff <= coeurs_phys <= fils_mat:
            raise Refus(f"META.txt : affinite incoherente (sockets={sockets_aff} "
                        f"coeurs_physiques={coeurs_phys} fils_materiels={fils_mat})")
        if fils is not None and (fils_mat > fils or coeurs_phys > coeurs):
            raise Refus(f"META.txt : affinite (fils_materiels={fils_mat}, coeurs_physiques="
                        f"{coeurs_phys}) depasse la topologie (fils={fils}, coeurs={coeurs})")


def argv_attendu(famille, params, n, abl):
    """L'argv EXACT que le lanceur emet pour un tuple, reconstruit depuis
    META ; la ligne commande= du .status doit lui etre egale."""
    argv = ["taskset", "-c", params["cpus"], f"bin/{COPIE_PRIVEE}", f"--family={famille}",
            f"--n={n}", f"--s={params['s']}", f"--smax={params['smax']}",
            f"--seed={params['seed']}", f"--threads={params['threads']}",
            f"--fold-inflight={params['fold_inflight']}", f"--fold-join={params['fold_join']}"]
    if abl != BRAS_TEMOIN:
        argv.append(f"--inject={abl}")
    return argv


def verifier_liaison(nom, commande, famille, params, n, abl, identite, jetons_kind):
    """Liaison commande / META / sortie d'un tuple : argv exact (ni en plus,
    ni en moins, ni un autre ordre), ligne d'identite egale a l'argv champ a
    champ, profil_kind fold_join= / inflight_demande= egaux a l'argv."""
    argv = commande.split()
    attendu = argv_attendu(famille, params, n, abl)
    if argv != attendu:
        i = next((j for j, (x, y) in enumerate(zip(argv, attendu)) if x != y), min(len(argv), len(attendu)))
        en_plus = [x for x in argv if x not in attendu]
        en_moins = [x for x in attendu if x not in argv]
        if en_plus and not en_moins:
            ecart = f"argument(s) en plus {en_plus}"
        elif en_moins and not en_plus:
            ecart = f"argument(s) en moins {en_moins}"
        elif sorted(argv) == sorted(attendu):
            ecart = f"ordre different (position {i} : {argv[i]!r} au lieu de {attendu[i]!r})"
        else:
            lu = argv[i] if i < len(argv) else "(fin)"
            att = attendu[i] if i < len(attendu) else "(fin)"
            ecart = f"position {i} : {lu!r} au lieu de {att!r}"
        raise Refus(f"{nom} : commande= != argv reconstruit depuis META ({ecart}) — regime, "
                    "famille ou affinite non lies au META, jamais agrege")
    options = dict(tok.split("=", 1) for tok in argv if tok.startswith("--") and "=" in tok)
    for champ, option in IDENTITE_ARGV:
        if identite[champ] != options[option]:
            raise Refus(f"{nom} : ligne d'identite de la sortie {champ}={identite[champ]!r} != "
                        f"{option}={options[option]!r} de la commande — la sortie n'est pas celle "
                        "de la commande gravee")
    if jetons_kind["fold_join"] != options["--fold-join"]:
        raise Refus(f"{nom} : profil_kind fold_join={jetons_kind['fold_join']!r} != "
                    f"--fold-join={options['--fold-join']!r} de la commande")
    if "inflight_demande" in jetons_kind and jetons_kind["inflight_demande"] != options["--fold-inflight"]:
        raise Refus(f"{nom} : profil_kind inflight_demande={jetons_kind['inflight_demande']!r} != "
                    f"--fold-inflight={options['--fold-inflight']!r} de la commande")


def inventaire_racine(work, worktree_modifie):
    """La racine du reçu = exactement les fichiers attendus et les repertoires
    bin/ et out/. Un repertoire VIDE ou inattendu, un lien, une entree
    speciale, un fichier intrus ou attendu absent : refus."""
    attendus = set(FICHIERS_RACINE)
    if worktree_modifie:
        attendus |= set(FICHIERS_WORKTREE)
    fichiers, repertoires = set(), set()
    for e in sorted(os.listdir(work)):
        chemin = os.path.join(work, e)
        if os.path.islink(chemin):
            raise Refus(f"racine : {e} est un lien symbolique — aucun lien dans un reçu")
        if os.path.isdir(chemin):
            repertoires.add(e)
        elif os.path.isfile(chemin):
            fichiers.add(e)
        else:
            raise Refus(f"racine : {e} entree speciale — aucune entree speciale dans un reçu")
    inattendus = sorted(repertoires - set(REPERTOIRES_RACINE))
    if inattendus:
        raise Refus(f"racine : repertoire(s) inattendu(s) {inattendus[:4]} (vide ou non) — "
                    f"seuls {list(REPERTOIRES_RACINE)} sont attendus")
    manquants = sorted(set(REPERTOIRES_RACINE) - repertoires)
    if manquants:
        raise Refus(f"racine : repertoire(s) attendu(s) absent(s) {manquants}")
    inattendus = sorted(fichiers - attendus - set(FICHIERS_RACINE_OPTIONNELS))
    if inattendus:
        raise Refus(f"racine : fichier(s) inattendu(s) {inattendus[:4]} — inventaire racine exact "
                    f"(worktree_sources_modifies={'!=0' if worktree_modifie else '0'})")
    manquants = sorted(attendus - fichiers)
    if manquants:
        raise Refus(f"racine : fichier(s) attendu(s) absent(s) {manquants[:4]}")
    contenu_bin = sorted(os.listdir(os.path.join(work, "bin")))
    if contenu_bin != [COPIE_PRIVEE]:
        raise Refus(f"bin/ : contenu {contenu_bin[:4]} != [{COPIE_PRIVEE!r}] — la copie privee "
                    "seule")


def verifier_protocoles(work, meta):
    """sha256_lanceur / sha256_agregateur du META RECALCULES depuis les copies
    archivees ; difference ou absence = refus (protocole altere apres coup)."""
    for nom, champ in PROTOCOLES:
        p = os.path.join(work, nom)
        if os.path.islink(p) or not os.path.isfile(p):
            raise Refus(f"{nom} absent ou non regulier — le protocole archive est obligatoire")
        h = sha256_fichier(p)
        if h != meta[champ]:
            raise Refus(f"{nom} : sha256 recalcule {h} != {champ} du META {meta[champ]} "
                        "(protocole archive altere apres coup)")


def verifier_williams(plan, bras, blocs):
    """Chaque succession ordonnee X→Y (bras adjacents dans un bloc) exactement
    une fois sur le groupe de blocs : m blocs x (m-1) successions = m(m-1)
    paires ordonnees. Un carre latin cyclique n'y satisfait pas."""
    comptes = {}
    for bloc in blocs:
        ordre = sorted(bras, key=lambda b: plan[(bloc, b)])
        for s in zip(ordre, ordre[1:]):
            comptes[s] = comptes.get(s, 0) + 1
    attendues = {(x, y) for x in bras for y in bras if x != y}
    repetees = sorted(s for s, c in comptes.items() if c > 1)
    absentes = sorted(attendues - set(comptes))
    if repetees or absentes:
        detail_rep = ", ".join(f"{x}->{y} x{comptes[(x, y)]}" for x, y in repetees[:3])
        detail_abs = ", ".join(f"{x}->{y}" for x, y in absentes[:3])
        raise Refus(f"plan.txt : blocs {list(blocs)} equilibres par position mais NON Williams"
                    f" — succession(s) repetee(s) [{detail_rep}], absente(s) [{detail_abs}] ;"
                    " chaque succession ordonnee X->Y de bras adjacents doit apparaitre"
                    f" exactement une fois par groupe de {len(bras)} blocs")


def lire_plan(work, reps, bras):
    """plan.txt -> {(bloc, bras): position} ; refuse un plan non Williams."""
    plan = {}
    path = os.path.join(work, "plan.txt")
    if not os.path.isfile(path):
        raise Refus("plan.txt absent")
    with open(path, encoding="utf-8") as f:
        for no, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            kv = tokens_kv(line.split(), f"plan.txt ligne {no}")
            try:
                bloc, pos, b = int(kv["bloc"]), int(kv["position"]), kv["bras"]
            except (KeyError, ValueError):
                raise Refus(f"plan.txt : ligne illisible ({line})")
            hors = b not in bras or not 1 <= bloc <= reps or not 1 <= pos <= len(bras)
            if hors:
                raise Refus(f"plan.txt : ligne hors matrice ({line})")
            if (bloc, b) in plan:
                raise Refus(f"plan.txt : bras {b} duplique au bloc {bloc}")
            plan[(bloc, b)] = pos
    for bloc in range(1, reps + 1):
        positions = sorted(plan.get((bloc, b), 0) for b in bras)
        if positions != list(range(1, len(bras) + 1)):
            raise Refus(f"plan.txt : bloc {bloc} incomplet ou positions non distinctes")
    if reps % len(bras) != 0:
        raise Refus(f"plan.txt : {reps} blocs ne forment pas des groupes de {len(bras)}")
    for g in range(0, reps, len(bras)):
        blocs = range(g + 1, g + len(bras) + 1)
        for pos in range(1, len(bras) + 1):
            occupants = sorted(b for bloc in blocs for b in bras if plan[(bloc, b)] == pos)
            if occupants != sorted(bras):
                raise Refus(f"plan.txt : position {pos} non equilibree sur les blocs {list(blocs)}")
        verifier_williams(plan, bras, blocs)
    return plan


def lire_status(path):
    """-> dict des champs. Premiere ligne = jetons ; lignes suivantes =
    cle=valeur ; tout champ present deux fois (sur la ligne 1 ou entre
    lignes) est un refus ; tout champ de CHAMPS_STATUS est obligatoire."""
    base = os.path.basename(path)
    with open(path, encoding="utf-8") as f:
        lignes = f.read().splitlines()
    if not lignes:
        raise Refus(f"{base} : statut vide")
    kv = tokens_kv(lignes[0].split(), f"{base} ligne 1")
    for no, line in enumerate(lignes[1:], 2):
        if not line or "=" not in line:
            continue
        k, v = line.split("=", 1)
        if k in kv:
            raise Refus(f"{base} : champ {k}= duplique (ligne {no}, {kv[k]!r} puis {v!r})"
                        " — jamais la premiere ni la derniere valeur")
        kv[k] = v
    for k in CHAMPS_STATUS:
        if k not in kv:
            raise Refus(f"{base} : champ obligatoire {k}= absent")
    for k in ("n", "rep", "position", "code"):
        entier(kv[k], f"{base} {k}")
    return kv


def injections_de(commande):
    """Noms passes par --inject= sur la ligne commande= (csv accepte par la
    CLI : chaque nom compte)."""
    noms = []
    for tok in commande.split():
        if tok.startswith("--inject="):
            noms.extend(x for x in tok[len("--inject="):].split(","))
    return noms


def verifier_injections(nom, abl, commande):
    """Identite de cible tenue par les faits : le bras temoin n'emet aucun
    --inject=, tout autre bras emet exactement le sien, et tout nom emis est
    l'une des trois ablations de la sonde (un mutant produit de kMutants,
    que mhgp7_profile_sonde accepte, n'est pas une mesure de la sonde)."""
    if f"bin/{COPIE_PRIVEE}" not in commande.split():
        raise Refus(f"{nom} : commande= n'execute pas bin/{COPIE_PRIVEE} (copie privee)")
    inj = injections_de(commande)
    for x in inj:
        if x not in ABLATIONS_SONDE:
            raise Refus(f"{nom} : --inject={x} hors sonde — seules les trois ablations "
                        f"{list(ABLATIONS_SONDE)} sont selectionnees (identite de cible : "
                        "un mutant produit de kMutants n'est jamais une mesure de la sonde)")
    if abl == BRAS_TEMOIN:
        if inj:
            raise Refus(f"{nom} : bras temoin {BRAS_TEMOIN} avec --inject={inj}")
    elif inj != [abl]:
        raise Refus(f"{nom} : --inject= emis {inj} != [{abl!r}] (exactement le mutant du bras)")
    return inj


def lire_hashes(work, tags, h_meta):
    """HASHES.txt : exactement une ligne `<tag> avant=<64hex> apres=<64hex>`
    par tuple attendu, chaque hash egal au binaire_sha256 du META."""
    path = os.path.join(work, "HASHES.txt")
    if not os.path.isfile(path):
        raise Refus("HASHES.txt absent")
    vus = set()
    with open(path, encoding="utf-8") as f:
        for no, line in enumerate(f, 1):
            tokens = line.split()
            if not tokens:
                continue
            if len(tokens) != 3:
                raise Refus(f"HASHES.txt ligne {no} : attendu `<tag> avant=<h> apres=<h>` ({line.strip()!r})")
            tag = tokens[0]
            if tag not in tags:
                raise Refus(f"HASHES.txt ligne {no} : tag hors matrice ({tag})")
            if tag in vus:
                raise Refus(f"HASHES.txt ligne {no} : tag duplique ({tag})")
            vus.add(tag)
            kv = tokens_kv(tokens[1:], f"HASHES.txt ligne {no}")
            for k in ("avant", "apres"):
                if k not in kv:
                    raise Refus(f"HASHES.txt ligne {no} : champ {k}= absent")
                exiger_hex64(kv[k], f"HASHES.txt {tag} {k}=")
                if kv[k] != h_meta:
                    raise Refus(f"HASHES.txt {tag} {k}={kv[k]} != binaire_sha256 du META {h_meta}")
    manquants = sorted(tags - vus)
    if manquants:
        raise Refus(f"HASHES.txt : {len(manquants)} tuple(s) sans ligne de hash, premiers {manquants[:4]}")


def sha256_fichier(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for bloc in iter(lambda: f.read(1 << 16), b""):
            h.update(bloc)
    return h.hexdigest()


def valeur_decimale(base, k, name, v):
    """Valeur d'une fenetre : decimale finie, ou refus explicite (vide,
    non numerique, nan/inf, exposant) — jamais un nan silencieux."""
    if v == "":
        raise Refus(f"{base} : K={k} {name}= vide (ligne profil_reduce tronquee)")
    try:
        x = float(v)
    except ValueError:
        raise Refus(f"{base} : K={k} valeur non numerique {name}={v!r} (ligne profil_reduce "
                    "malformee) — jamais ignoree")
    if not math.isfinite(x):
        raise Refus(f"{base} : K={k} fenetre {name} non finie ({v})")
    if not DECIMAL.match(v):
        raise Refus(f"{base} : K={k} {name}={v!r} hors grammaire decimale ^-?[0-9]+(.[0-9]+)?$")
    return x


def verifier_profil_kind(base, text):
    """EXACTEMENT une ligne `profil_kind=` : jetons `cle=valeur` uniques,
    profil_kind=reduce_v2 et fold_join=1 exiges, layout= (s'il est present)
    exactement classic — une instrumentation ou une route differente n'est
    pas une mesure de cette sonde."""
    lignes = LIGNE_KIND.findall(text)
    if len(lignes) != 1:
        raise Refus(f"{base} : ligne profil_kind= absente ou dupliquee ({len(lignes)} occurrence(s))"
                    " — jamais la premiere ni la derniere valeur")
    jetons = tokens_kv(lignes[0].split(), f"{base} ligne profil_kind")
    for k in jetons:
        if not NOM_CHAMP.match(k):
            raise Refus(f"{base} : ligne profil_kind, nom de jeton hors grammaire ({k!r})")
    for k, attendu in PROFIL_JETONS_REQUIS:
        if k not in jetons:
            raise Refus(f"{base} : ligne profil_kind sans jeton {k}= (attendu {k}={attendu})")
        if jetons[k] != attendu:
            raise Refus(f"{base} : {k}={jetons[k]!r} != {attendu!r} sur la ligne profil_kind — "
                        "la sonde exige reduce_v2 avec join=1 (etage B isole) ; une autre "
                        "instrumentation ou un join libre n'est pas une mesure de cette sonde")
    for k, attendu in PROFIL_JETONS_SI_PRESENTS:
        if k in jetons and jetons[k] != attendu:
            raise Refus(f"{base} : {k}={jetons[k]!r} != {attendu!r} sur la ligne profil_kind — "
                        "cette sonde ne mesure que la route classic (csr : autre objet, autre "
                        "sonde)")
    return jetons


def lire_identite(base, text):
    """EXACTEMENT une ligne d'identite `famille= n= [coord=] s= smax= seed=
    threads= …` : jetons uniques, champs requis presents ; liee ensuite a
    l'argv de la commande (verifier_liaison)."""
    lignes = LIGNE_IDENTITE.findall(text)
    if len(lignes) != 1:
        raise Refus(f"{base} : ligne d'identite famille= absente ou dupliquee ({len(lignes)} "
                    "occurrence(s)) — la sortie ne peut pas etre liee a sa commande")
    identite = tokens_kv(lignes[0].split(), f"{base} ligne d'identite")
    for k in IDENTITE_REQUIS:
        if k not in identite:
            raise Refus(f"{base} : ligne d'identite sans {k}= (attendus {list(IDENTITE_REQUIS)})")
    return identite


def parse_profile(path):
    """-> ({K: {champ: float}}, temps_mur_ms, rss_max_kb, jetons profil_kind,
    identite) ; refus sinon. La ligne `profil_kind=` est verrouillee, la
    ligne d'identite est unique et toute ligne commencant par
    `profil_reduce` est parsee STRICTEMENT."""
    base = os.path.basename(path)
    with open(path, encoding="utf-8") as f:
        text = f.read()
    jetons_kind = verifier_profil_kind(base, text)
    identite = lire_identite(base, text)
    per_k = {}
    for m in LIGNE_PROFIL.finditer(text):
        ligne = m.group(0).strip()
        tokens = (m.group(1) or "").split()
        if not tokens:
            raise Refus(f"{base} : ligne profil_reduce tronquee ou vide ({ligne!r})")
        if not tokens[0].startswith("K="):
            raise Refus(f"{base} : ligne profil_reduce sans K= en tete ({ligne[:80]!r})")
        k_txt = tokens[0][2:]
        if not re.fullmatch(r"[0-9]+", k_txt):
            raise Refus(f"{base} : K non entier ({tokens[0]!r}) — ligne profil_reduce malformee")
        k = int(k_txt)
        if k not in KS:
            raise Refus(f"{base} : K={k} hors 1..10")
        if k in per_k:
            raise Refus(f"{base} : ligne K={k} dupliquee")
        if len(tokens) == 1:
            raise Refus(f"{base} : ligne K={k} tronquee (aucune fenetre)")
        fields = {}
        for name, v in tokens_kv(tokens[1:], f"{base} profil_reduce K={k}").items():
            if not NOM_CHAMP.match(name):
                raise Refus(f"{base} : K={k} nom de champ hors grammaire ({name!r})")
            fields[name] = valeur_decimale(base, k, name, v)
        for name in CHAMPS_REQUIS:
            if name not in fields:
                raise Refus(f"{base} : K={k} sans fenetre {name}")
        recalc = sum(fields[c] for c in COMPOSANTES if c in fields)
        if not math.isfinite(recalc) or abs(recalc - fields["somme"]) > SEUIL_SOMME:
            raise Refus(f"{base} : K={k} somme imprimee {fields['somme']:.3f}"
                        f" != somme des composantes {recalc:.3f}")
        per_k[k] = fields
    manquants = [k for k in KS if k not in per_k]
    if manquants:
        raise Refus(f"{base} : ligne(s) K manquante(s) {manquants}")
    murs = re.findall(r"^temps_mur_ms=(\S*)", text, re.M)
    if len(murs) != 1:
        raise Refus(f"{base} : temps_mur_ms absent ou duplique ({len(murs)} occurrence(s)) — "
                    "jamais la premiere ni la derniere valeur")
    try:
        mur_ms = float(murs[0])
    except ValueError:
        raise Refus(f"{base} : temps_mur_ms illisible ({murs[0]!r})")
    if not math.isfinite(mur_ms):
        raise Refus(f"{base} : temps_mur_ms non fini")
    rss = re.findall(r"^rss_max_kb=(\S*)", text, re.M)
    if len(rss) != 1:
        raise Refus(f"{base} : rss_max_kb absent ou duplique ({len(rss)} occurrence(s)) — "
                    "jamais la premiere ni la derniere valeur")
    if not re.fullmatch(r"[0-9]+", rss[0]):
        raise Refus(f"{base} : rss_max_kb non entier ({rss[0]!r})")
    return per_k, mur_ms, int(rss[0]), jetons_kind, identite


def inventaire_out(out, attendus):
    """out/ = EXACTEMENT le triplet <tag>.txt|.err|.status de chaque tuple."""
    attendus_fichiers = {f"{a}_n{n}_r{r}.{ext}" for (a, n, r) in attendus for ext in EXTENSIONS}
    entrees = sorted(os.listdir(out))
    for e in entrees:
        chemin = os.path.join(out, e)
        if os.path.islink(chemin) or not os.path.isfile(chemin):
            raise Refus(f"out/{e} : entree non reguliere (repertoire, lien ou special) — "
                        "out/ ne contient que des fichiers <tag>.txt|.err|.status")
    inattendus = sorted(set(entrees) - attendus_fichiers)
    if inattendus:
        raise Refus(f"out/ : {len(inattendus)} artefact(s) inattendu(s) hors de l'ensemble exact "
                    f"<tag>.txt|.err|.status des tuples attendus : {inattendus[:4]}")
    manquants = sorted(attendus_fichiers - set(entrees))
    if manquants:
        raise Refus(f"out/ : {len(manquants)} artefact(s) attendu(s) absent(s) : {manquants[:4]}")


def charger(work):
    meta, ns, reps, bras, bornes = lire_meta(work)
    params = lire_parametres(meta)
    verifier_topologie(meta, params)
    inventaire_racine(work, int(meta["worktree_sources_modifies"]) != 0)
    verifier_protocoles(work, meta)
    plan = lire_plan(work, reps, bras)
    h_meta = meta["binaire_sha256"]
    copie = os.path.join(work, "bin", COPIE_PRIVEE)
    if not os.path.isfile(copie) or os.path.islink(copie):
        raise Refus(f"bin/{COPIE_PRIVEE} absent ou non regulier")
    h_copie = sha256_fichier(copie)
    if h_copie != h_meta:
        raise Refus(f"bin/{COPIE_PRIVEE} : sha256 {h_copie} != binaire_sha256 du META {h_meta}")
    out = os.path.join(work, "out")
    attendus = sorted((a, n, r) for a in bras for n in ns for r in range(1, reps + 1))
    lire_hashes(work, {f"{a}_n{n}_r{r}" for (a, n, r) in attendus}, h_meta)
    inventaire_out(out, attendus)
    mesures = {}
    emis = set()
    coords = {}
    for key in attendus:
        a, n, r = key
        tag = f"{a}_n{n}_r{r}"
        f = f"{tag}.status"
        kv = lire_status(os.path.join(out, f))
        abl, n_lu, rep, pos = kv["ablation"], int(kv["n"]), int(kv["rep"]), int(kv["position"])
        if (abl, n_lu, rep) != key:
            raise Refus(f"nom de statut incoherent avec son contenu : {f} "
                        f"(contenu {abl}_n{n_lu}_r{rep})")
        if int(kv["code"]) != 0:
            raise Refus(f"run en echec (code {kv['code']}) : {f}")
        if plan[(rep, abl)] != pos:
            raise Refus(f"position executee {pos} != plan {plan[(rep, abl)]} : {f}")
        for k in ("sha256_avant", "sha256_apres"):
            exiger_hex64(kv[k], f"{f} {k}=")
            if kv[k] != h_meta:
                raise Refus(f"{f} : {k}={kv[k]} != binaire_sha256 du META {h_meta}")
        emis.update(verifier_injections(f, abl, kv["commande"]))
        per_k, mur, rss, jetons_kind, identite = parse_profile(os.path.join(out, f"{tag}.txt"))
        verifier_liaison(f"{tag}.txt / {f}", kv["commande"], meta["famille"], params, n, abl,
                         identite, jetons_kind)
        if "coord" in identite:
            if coords.setdefault(n, identite["coord"]) != identite["coord"]:
                raise Refus(f"{tag}.txt : coord={identite['coord']!r} != {coords[n]!r} des autres "
                            f"tuples de n={n} — les bras n'ont pas recu la meme entree")
        mesures[key] = (per_k, mur, rss)
    if sorted(emis) != sorted(ABLATIONS_SONDE):
        raise Refus(f"ensemble des --inject= emis {sorted(emis)} != les trois ablations "
                    f"{list(ABLATIONS_SONDE)}")
    manquants = sorted(set(attendus) - set(mesures))
    if manquants:
        raise Refus(f"matrice incomplete : {len(manquants)} tuple(s) absent(s), "
                    f"premiers {manquants[:4]}")
    return meta, ns, reps, bras, mesures, bornes, params, coords


def valeurs(mesures, abl, n, rep):
    """Colonnes d'un tuple : fenetres par K, sommes sur K, mur, rss."""
    per_k, mur, rss = mesures[(abl, n, rep)]
    cols = {}
    for K in KS_SHOWN:
        for w in WINDOWS:
            cols[(K, w)] = per_k[K][w]
    for w in WINDOWS:
        cols[("sigma", w)] = sum(per_k[K][w] for K in KS)
    cols[("sigma", "mur")] = mur
    cols[("sigma", "rss")] = float(rss)
    return cols


def etiquette(abl):
    return f"{abl} ({ETIQUETTES[abl]})" if abl in ETIQUETTES else abl


def fmt3(vals, prec=1):
    med, mn, mx = statistics.median(vals), min(vals), max(vals)
    return f"{med:+.{prec}f} [{mn:+.{prec}f};{mx:+.{prec}f}]"


def main():
    if len(sys.argv) != 2:
        print("usage: sonde_ablation_reduce.py <dossier de reçu (.partial ou publie)>",
              file=sys.stderr)
        return 2
    try:
        meta, ns, reps, bras, mesures, bornes, params, coords = charger(sys.argv[1])
    except Refus as e:
        print(f"REFUS : {e}", file=sys.stderr)
        return 1
    except OSError as e:
        print(f"REFUS : lecture impossible ({e})", file=sys.stderr)
        return 1
    autres = [a for a in bras if a != BRAS_TEMOIN]
    print(f"# sonde ablation reduce — statut={meta['statut']}")
    print("# bornes exploratoires NON CAUSALES sur binaire instrumente (join=1) : jamais un "
          "benchmark, jamais un mur, jamais un choix de palier.")
    print(f"# schema={meta['schema']} ; identite de cible (claim borne) : "
          f"{meta.get('identite_cible', IDENTITE_CIBLE + ' (schema anterieur : non grave)')} ; "
          f"--inject= emis, verifies sur chaque .status : {' '.join(ABLATIONS_SONDE)} ; "
          f"interpreteur de la reagregation du jeu scelle : "
          f"{meta.get('interpreteur', '(schema anterieur : non grave)')}")
    for b in bornes:
        print(f"# claim borne, NON VERIFIE : {b}")
    print("# liaison commande / META / sortie VERIFIEE sur chaque tuple : argv exact "
          f"`taskset -c {params['cpus']} bin/{COPIE_PRIVEE} --family={meta['famille']} --n=<n> "
          f"--s={params['s']} --smax={params['smax']} --seed={params['seed']} "
          f"--threads={params['threads']} --fold-inflight={params['fold_inflight']} "
          f"--fold-join={params['fold_join']} [--inject=<bras>]` == commande= du .status == "
          "ligne d'identite famille=/n=/s=/smax=/seed=/threads= de la sortie == profil_kind "
          "fold_join=/inflight_demande= ; coord par taille : "
          + (" ".join(f"n={n}:coord={coords[n]}" for n in ns if n in coords) or "non imprime"))
    for k in CHAMPS_META_V5:
        if k in meta:
            print(f"# {k}={meta[k]}")
    print("# bras : " + " ; ".join([f"{BRAS_TEMOIN} (temoin)"] + [etiquette(a) for a in autres]))
    print(f"# plan : williams_4x4, blocs={reps}, tailles={ns} ; appariement PAR BLOC : "
          f"d_b = bras(b) − {BRAS_TEMOIN}(b), meme bloc, meme taille.")
    print("# tableau principal : mediane [min;max] sur les blocs des differences appariees d_b "
          "(ms ; negatif = fenetre reduite). Second tableau : medianes brutes par bras.")
    sigma_cols = list(WINDOWS) + ["mur", "rss"]
    sigma_head = "\t".join(WINDOWS) + "\tmur_instrumente_ms\trss_max_kb"
    for n in ns:
        print(f"\n## n={n}")
        cols = {(a, r): valeurs(mesures, a, n, r) for a in bras for r in range(1, reps + 1)}
        print(f"\n### differences appariees par bloc (blocs={reps})")
        for K in KS_SHOWN:
            print(f"K={K}\t" + "\t".join(WINDOWS))
            for a in autres:
                d = {w: [cols[(a, r)][(K, w)] - cols[(BRAS_TEMOIN, r)][(K, w)]
                         for r in range(1, reps + 1)] for w in WINDOWS}
                print(f"{etiquette(a)}\t" + "\t".join(fmt3(d[w]) for w in WINDOWS))
        print("Σ_K\t" + sigma_head)
        for a in autres:
            d = {w: [cols[(a, r)][("sigma", w)] - cols[(BRAS_TEMOIN, r)][("sigma", w)]
                     for r in range(1, reps + 1)] for w in sigma_cols}
            print(f"{etiquette(a)}\t" + "\t".join(fmt3(d[w]) for w in WINDOWS)
                  + f"\t{fmt3(d['mur'])}\t{fmt3(d['rss'], 0)}")
        print("\n### parts Σ_K retirees de la fenetre du temoin, appariees par bloc "
              "(mediane [min;max], % ; borne non causale)")
        for a in autres:
            morceaux = []
            for w in PARTS:
                parts = []
                for r in range(1, reps + 1):
                    ref = cols[(BRAS_TEMOIN, r)][("sigma", w)]
                    if ref <= 0:
                        continue
                    parts.append(100.0 * (ref - cols[(a, r)][("sigma", w)]) / ref)
                morceaux.append(f"{w} {fmt3(parts)} %" if parts else f"{w} indefini (temoin nul)")
            print(f"  {etiquette(a)} : " + " ; ".join(morceaux))
        print("\n### medianes brutes sur blocs (second tableau, non apparie)")
        for K in KS_SHOWN:
            print(f"K={K}\t" + "\t".join(WINDOWS))
            for a in bras:
                print(f"{etiquette(a)}\t" + "\t".join(
                    f"{statistics.median(cols[(a, r)][(K, w)] for r in range(1, reps + 1)):.1f}"
                    for w in WINDOWS))
        print("Σ_K\t" + sigma_head)
        for a in bras:
            med = {w: statistics.median(cols[(a, r)][("sigma", w)] for r in range(1, reps + 1))
                   for w in sigma_cols}
            print(f"{etiquette(a)}\t" + "\t".join(f"{med[w]:.1f}" for w in WINDOWS)
                  + f"\t{med['mur']:.1f}\t{med['rss']:.0f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

