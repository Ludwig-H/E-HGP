#!/usr/bin/env python3
"""Porte du générateur de plan pré-inscrit KeyCSR (bench/plan_keycsr.py).

Verrou : audits/REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md (« Plan
apparié et décision exhaustive » : « le passage d'un mot de 64 bits à
l'indice borné de Fisher–Yates est lui aussi défini et rejoué par une
fixture ; ne pas déléguer le canon à un shuffle »). Le générateur n'est
JAMAIS importé : tout est rejoué par une implémentation indépendante minimale
(SHA-256 de la chaîne, SplitMix64, Fisher–Yates de Durstenfeld, j = mot mod
(i+1)) et comparé à des FIXTURES GRAVÉES EN DUR.

Scènes (chacune : un défaut ou une propriété, un code attendu) :
  (a) fixture gravée : graine dérivée en hex, dix premiers mots SplitMix64,
      six orientations de la PREMIÈRE strate (uniform 16000 profil_callback),
      recalculées indépendamment ET lues dans le plan produit ;
  (b) déterminisme : deux générations => octets identiques ; le plan avec
      extension eight_clusters reprend les lignes uniform à l'identique
      (un seul flux, extension consommée APRÈS uniform) ;
  (c) chaque strate : exactement 3 AB et 3 BA sur les blocs 1..6, six blocs,
      deux échauffements (bloc 0, un par bras), positions 1/2 cohérentes ;
  (d) `sha256_plan=` exact (sha256 des octets qui précèdent) et dernière
      ligne ;
  (e) `--graine-hex` différent de la dérivée => refus 2, sortie non créée ;
      égal => 0 et plan bit à bit identique ;
  (f) nombre total de runs = strates × (2 + 12), sans puis avec extension ;
  (g) ordre canonique des strates : uniform 16000, 32000, 8000 × les cinq
      strates dans l'ordre, puis eight_clusters 32000 × cinq ;
  (h) A/A : layout classic aux deux positions (échauffements compris) ;
  (i) cohérence ligne/commande : layout, join, digest, callback, famille, n,
      threads, inflight, s, smax, seed reflétés dans l'argv ; en-tête :
      schéma, graine, callback À CONFIRMER, coord=defaut ;
  (j) usages refusés (2) : sans --sortie, extension non pré-inscrite, sortie
      préexistante, argument inconnu.

Codes : 0 conforme ; 1 au moins une scène échoue. Jamais `assert`
(doit tenir sous python3 -O).
"""

from __future__ import annotations

import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
GENERATEUR = HERE.parent / "bench" / "plan_keycsr.py"

# ---- Fixtures GRAVÉES (calculées le 2 septembre 2026, indépendamment) ----
CHAINE = "morsehgp3D_v6:keycsr-prereg:v1:2026-09-02"
GRAINE_HEX = "0xa2ffb4db2884ddc4"  # = valeur annoncée par les auditeurs
MOTS_SPLITMIX64 = [
    0x8DD381D499FF1BFF,
    0x8F937AF4D2B8ECCA,
    0xB98772527E29727D,
    0xDA144AE74ACC71D6,
    0x5978C2535D0C9CD0,
    0x5EDD31F8F7C20A3E,
    0xAA14E78BE6792873,
    0x736E787E4B79EB02,
    0x307D565A35136786,
    0x6D48404441DB822F,
]
ORIENTATIONS_STRATE_1 = ["BA", "AB", "AB", "AB", "BA", "BA"]

STRATES = ["profil_callback", "release_digest_off", "release_digest_on",
           "aa_digest_off", "aa_digest_on"]
CELLULES_UNIFORM = [("uniform", 16000), ("uniform", 32000), ("uniform", 8000)]
CELLULE_EXT = ("eight_clusters", 32000)
RUNS_PAR_STRATE = 2 + 12

ECHECS = []


def echec(scene: str, message: str) -> None:
    ECHECS.append("(%s) %s" % (scene, message))
    sys.stdout.write("ECHEC (%s) %s\n" % (scene, message))


def exiger(scene: str, condition: bool, message: str) -> bool:
    if not condition:
        echec(scene, message)
    return condition


# ---- Implémentation INDÉPENDANTE minimale (jamais un import du générateur) --
M64 = (1 << 64) - 1


def splitmix64_mots(graine: int, nombre: int) -> list:
    etat = graine & M64
    mots = []
    for _ in range(nombre):
        etat = (etat + 0x9E3779B97F4A7C15) & M64
        z = etat
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & M64
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & M64
        mots.append(z ^ (z >> 31))
    return mots


def durstenfeld(liste: list, mots: list) -> list:
    """i de n-1 à 1, j = mot mod (i+1) ; consomme len(liste)-1 mots."""
    resultat = list(liste)
    k = 0
    for i in range(len(resultat) - 1, 0, -1):
        j = mots[k] % (i + 1)
        k += 1
        resultat[i], resultat[j] = resultat[j], resultat[i]
    return resultat


# ---- Outils -----------------------------------------------------------------
def generer(chemin: Path, *options: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(GENERATEUR), "--sortie", str(chemin), *options],
                          capture_output=True, text=True, check=False)


def champs_de(ligne: str) -> dict:
    """Champs clé=valeur d'une ligne de run ; `commande=` absorbe le reste."""
    champs = {}
    reste = ligne
    while reste:
        if reste.startswith("commande="):
            champs["commande"] = reste[len("commande="):]
            break
        jeton, _, reste = reste.partition(" ")
        cle, sep, valeur = jeton.partition("=")
        if sep != "=":
            return {}
        champs[cle] = valeur
    return champs


def lire_plan(chemin: Path) -> tuple:
    """(en-tête dict, runs list[dict], lignes brutes, octets)."""
    octets = chemin.read_bytes()
    lignes = octets.decode("ascii").split("\n")
    if lignes and lignes[-1] == "":
        lignes.pop()
    en_tete = {}
    runs = []
    for ligne in lignes:
        if ligne.startswith("strate="):
            runs.append(champs_de(ligne))
        else:
            cle, _, valeur = ligne.partition("=")
            en_tete[cle] = valeur
    return en_tete, runs, lignes, octets


def par_strate(runs: list) -> list:
    """Groupes (famille, n, strate) dans l'ordre d'apparition."""
    groupes = []
    for run in runs:
        cle = (run.get("famille"), run.get("n"), run.get("strate"))
        if not groupes or groupes[-1][0] != cle:
            groupes.append((cle, []))
        groupes[-1][1].append(run)
    return groupes


# ---- Scènes -----------------------------------------------------------------
def scene_a(en_tete: dict, runs: list) -> None:
    """Fixture gravée : graine, mots, orientations de la première strate."""
    graine = int.from_bytes(hashlib.sha256(CHAINE.encode("utf-8")).digest()[:8], "big")
    exiger("a", "0x%016x" % graine == GRAINE_HEX,
           "graine derivee independamment 0x%016x != fixture %s" % (graine, GRAINE_HEX))
    exiger("a", en_tete.get("graine_hex") == GRAINE_HEX,
           "graine_hex du plan %r != fixture %s" % (en_tete.get("graine_hex"), GRAINE_HEX))
    mots = splitmix64_mots(graine, 10)
    exiger("a", mots == MOTS_SPLITMIX64,
           "dix premiers mots SplitMix64 != fixture : %s" % ["0x%016x" % m for m in mots])
    rejoue = durstenfeld(["AB", "AB", "AB", "BA", "BA", "BA"], mots[:5])
    exiger("a", rejoue == ORIENTATIONS_STRATE_1,
           "Fisher-Yates rejoue %s != fixture %s" % (rejoue, ORIENTATIONS_STRATE_1))
    groupes = par_strate(runs)
    if not exiger("a", bool(groupes), "aucun run dans le plan"):
        return
    cle, premiers = groupes[0]
    exiger("a", cle == ("uniform", "16000", "profil_callback"),
           "premiere strate %r != uniform 16000 profil_callback" % (cle,))
    lues = [r.get("orientation") for r in premiers if r.get("position") == "1" and r.get("bloc") != "0"]
    exiger("a", lues == ORIENTATIONS_STRATE_1,
           "orientations lues %s != fixture %s" % (lues, ORIENTATIONS_STRATE_1))
    # Le plan entier est rejoué depuis le flux unique : chaque strate consomme
    # cinq mots, dans l'ordre canonique, échauffements hors tirage.
    flux = splitmix64_mots(graine, 5 * len(groupes))
    for k, (cle_k, runs_k) in enumerate(groupes):
        attendu = durstenfeld(["AB", "AB", "AB", "BA", "BA", "BA"], flux[5 * k:5 * k + 5])
        lues_k = [r.get("orientation") for r in runs_k if r.get("position") == "1" and r.get("bloc") != "0"]
        exiger("a", lues_k == attendu, "strate %r : orientations %s != rejeu %s" % (cle_k, lues_k, attendu))


def scene_b(octets_1: bytes, octets_2: bytes, runs_sans: list, runs_avec: list) -> None:
    exiger("b", octets_1 == octets_2, "deux generations different (non deterministe)")
    lignes_sans = [r for r in runs_sans]
    prefixe_avec = runs_avec[:len(lignes_sans)]
    exiger("b", prefixe_avec == lignes_sans,
           "les lignes uniform du plan avec extension different du plan sans extension")
    exiger("b", all(r.get("famille") == "eight_clusters" for r in runs_avec[len(lignes_sans):]),
           "les lignes au-dela d'uniform ne sont pas toutes eight_clusters")


def scene_c(runs: list) -> None:
    for cle, groupe in par_strate(runs):
        chauffe = [r for r in groupe if r.get("bloc") == "0"]
        blocs = {}
        for r in groupe:
            if r.get("bloc") != "0":
                blocs.setdefault(r.get("bloc"), []).append(r)
        exiger("c", len(chauffe) == 2, "strate %r : %d echauffements != 2" % (cle, len(chauffe)))
        exiger("c", [r.get("bras") for r in chauffe] == ["A", "B"] and
               [r.get("position") for r in chauffe] == ["1", "2"],
               "strate %r : echauffements hors ordre canonique A(1) puis B(2)" % (cle,))
        exiger("c", sorted(blocs) == ["1", "2", "3", "4", "5", "6"],
               "strate %r : blocs %s != 1..6" % (cle, sorted(blocs)))
        orientations = []
        for bloc, paire in sorted(blocs.items()):
            if not exiger("c", len(paire) == 2, "strate %r bloc %s : %d runs != 2" % (cle, bloc, len(paire))):
                continue
            p1, p2 = paire
            o = p1.get("orientation")
            exiger("c", p2.get("orientation") == o, "strate %r bloc %s : orientations divergentes" % (cle, bloc))
            exiger("c", (p1.get("position"), p2.get("position")) == ("1", "2"),
                   "strate %r bloc %s : positions != (1, 2)" % (cle, bloc))
            exiger("c", o in ("AB", "BA") and p1.get("bras") == o[0] and p2.get("bras") == o[1],
                   "strate %r bloc %s : bras (%s, %s) incoherents avec %s"
                   % (cle, bloc, p1.get("bras"), p2.get("bras"), o))
            orientations.append(o)
        exiger("c", orientations.count("AB") == 3 and orientations.count("BA") == 3,
               "strate %r : %s n'a pas 3 AB et 3 BA" % (cle, orientations))


def scene_d(lignes: list, octets: bytes) -> None:
    if not exiger("d", lignes and lignes[-1].startswith("sha256_plan="), "derniere ligne != sha256_plan="):
        return
    derniere = lignes[-1]
    corps = octets[:len(octets) - len((derniere + "\n").encode("ascii"))]
    exiger("d", octets.endswith((derniere + "\n").encode("ascii")), "le fichier ne finit pas par sha256_plan=...\\n")
    exiger("d", derniere == "sha256_plan=" + hashlib.sha256(corps).hexdigest(),
           "sha256_plan inexact : %s" % derniere)
    exiger("d", sum(1 for l in lignes if l.startswith("sha256_plan=")) == 1, "sha256_plan= duplique")


def scene_e(dossier: Path, octets_ref: bytes) -> None:
    faux = dossier / "e_faux.txt"
    r = generer(faux, "--graine-hex", "0x0000000000000001")
    exiger("e", r.returncode == 2, "graine differente : code %d != 2" % r.returncode)
    exiger("e", not faux.exists(), "graine differente : sortie creee malgre le refus")
    exiger("e", "graine" in r.stderr.lower(), "graine differente : motif absent de stderr")
    r = generer(faux, "--graine-hex", "abc")
    exiger("e", r.returncode == 2 and not faux.exists(), "graine illisible : code %d != 2" % r.returncode)
    vrai = dossier / "e_vrai.txt"
    r = generer(vrai, "--graine-hex", GRAINE_HEX)
    exiger("e", r.returncode == 0, "graine egale a la derivee : code %d != 0" % r.returncode)
    exiger("e", vrai.exists() and vrai.read_bytes() == octets_ref, "graine egale : plan non identique")
    vrai_maj = dossier / "e_vrai_maj.txt"
    r = generer(vrai_maj, "--graine-hex", GRAINE_HEX.upper().replace("0X", "0x"))
    exiger("e", r.returncode == 0 and vrai_maj.read_bytes() == octets_ref, "graine en majuscules : refusee ou differente")


def scene_f(en_tete: dict, runs: list, nb_strates: int, libelle: str) -> None:
    exiger("f", len(runs) == nb_strates * RUNS_PAR_STRATE,
           "%s : %d runs != %d x %d" % (libelle, len(runs), nb_strates, RUNS_PAR_STRATE))
    exiger("f", en_tete.get("nb_runs") == str(nb_strates * RUNS_PAR_STRATE),
           "%s : nb_runs en-tete %r != %d" % (libelle, en_tete.get("nb_runs"), nb_strates * RUNS_PAR_STRATE))
    exiger("f", en_tete.get("nb_strates") == str(nb_strates),
           "%s : nb_strates en-tete %r != %d" % (libelle, en_tete.get("nb_strates"), nb_strates))
    exiger("f", en_tete.get("mots_consommes") == str(5 * nb_strates),
           "%s : mots_consommes %r != %d" % (libelle, en_tete.get("mots_consommes"), 5 * nb_strates))


def scene_g(en_tete: dict, runs: list, avec_extension: bool) -> None:
    cellules = list(CELLULES_UNIFORM) + ([CELLULE_EXT] if avec_extension else [])
    attendu = [(fam, str(n), s) for fam, n in cellules for s in STRATES]
    obtenu = [cle for cle, _ in par_strate(runs)]
    exiger("g", obtenu == attendu, "ordre des strates %s != canonique %s" % (obtenu, attendu))
    liste = ",".join("%s:%s:%s" % c for c in attendu)
    exiger("g", en_tete.get("strates") == liste, "en-tete strates= != liste canonique")
    exiger("g", en_tete.get("famille_extension") == ("eight_clusters" if avec_extension else "aucune"),
           "en-tete famille_extension incoherent")


def scene_h(runs: list) -> None:
    aa = [r for r in runs if r.get("strate", "").startswith("aa_")]
    exiger("h", bool(aa), "aucune strate A/A")
    for r in aa:
        exiger("h", r.get("layout") == "classic" and "--layout=classic" in r.get("commande", "").split(" "),
               "A/A %s n=%s bloc=%s position=%s : layout %r != classic"
               % (r.get("strate"), r.get("n"), r.get("bloc"), r.get("position"), r.get("layout")))
    autres = [r for r in runs if not r.get("strate", "").startswith("aa_")]
    for r in autres:
        attendu = "classic" if r.get("bras") == "A" else "csr"
        exiger("h", r.get("layout") == attendu,
               "%s n=%s bloc=%s : bras %s -> layout %r != %s"
               % (r.get("strate"), r.get("n"), r.get("bloc"), r.get("bras"), r.get("layout"), attendu))


def scene_i(en_tete: dict, runs: list) -> None:
    exiger("i", en_tete.get("schema") == "e-hgp.plan-keycsr.v1", "schema en-tete %r" % en_tete.get("schema"))
    exiger("i", "A_CONFIRMER" in en_tete.get("callback_flag", "") and
           en_tete.get("callback_flag", "").startswith("--callback-temoin"),
           "callback_flag non signale A_CONFIRMER")
    exiger("i", en_tete.get("coord", "").startswith("defaut"), "coord en-tete != defaut")
    exiger("i", en_tete.get("graine_annoncee_auditeurs") == GRAINE_HEX and en_tete.get("graine_accord") == "oui",
           "graine annoncee / accord absents ou differents")
    exiger("i", "sha256" in en_tete.get("derivation", "") and CHAINE in en_tete.get("derivation", ""),
           "chaine de derivation absente de l'en-tete")
    exiger("i", "splitmix64" in en_tete.get("prng", "") and "0x9E3779B97F4A7C15" in en_tete.get("prng", ""),
           "PRNG non decrit")
    exiger("i", "mod (i+1)" in en_tete.get("mapping", "") and "ACCEPTE" in en_tete.get("mapping", ""),
           "mapping mot -> indice non decrit / biais non accepte explicitement")
    exiger("i", not any(k.startswith("strate=") for k in en_tete), "en-tete contenant une cle strate=")
    cles = ["strate", "famille", "n", "bloc", "position", "orientation", "bras", "layout", "digest",
            "callback", "join", "threads", "inflight", "s", "smax", "seed", "coord", "schema", "binaire", "commande"]
    for r in runs:
        if not exiger("i", list(r) == cles, "champs hors ordre ou manquants : %s" % list(r)):
            continue
        argv = r["commande"].split(" ")
        attendu = ["--family=%s" % r["famille"], "--n=%s" % r["n"], "--s=%s" % r["s"], "--smax=%s" % r["smax"],
                   "--seed=%s" % r["seed"], "--threads=%s" % r["threads"], "--fold-inflight=%s" % r["inflight"],
                   "--fold-join=%s" % r["join"], "--layout=%s" % r["layout"]]
        if r["digest"] == "on":
            attendu.append("--digest")
        if r["callback"] == "on":
            attendu.append("--callback-temoin")
        exiger("i", argv == attendu, "commande %r != %r" % (argv, attendu))
        exiger("i", (r["threads"], r["inflight"], r["s"], r["smax"], r["seed"], r["coord"]) ==
               ("8", "2", "8", "11", "3", "defaut"), "parametres communs alteres : %s" % r)
        exiger("i", not any(a.startswith("--coord") for a in argv), "--coord present : coord doit rester defaut")
        profil = r["strate"] == "profil_callback"
        exiger("i", (r["binaire"], r["schema"], r["join"], r["callback"]) ==
               (("profil", "reduce_v3", "1", "on") if profil else ("release", "release", "0", "off")),
               "strate %s : binaire/schema/join/callback incoherents : %s" % (r["strate"], r))
        exiger("i", r["digest"] == ("on" if r["strate"].endswith("_on") else "off"),
               "strate %s : digest %s" % (r["strate"], r["digest"]))


def scene_j(dossier: Path) -> None:
    r = subprocess.run([sys.executable, str(GENERATEUR)], capture_output=True, text=True, check=False)
    exiger("j", r.returncode == 2, "sans --sortie : code %d != 2" % r.returncode)
    inconnue = dossier / "j_inconnue.txt"
    r = generer(inconnue, "--famille-extension", "terrain")
    exiger("j", r.returncode == 2 and not inconnue.exists(), "extension terrain : code %d != 2" % r.returncode)
    existante = dossier / "j_existante.txt"
    existante.write_bytes(b"grave\n")
    r = generer(existante)
    exiger("j", r.returncode == 2 and existante.read_bytes() == b"grave\n",
           "sortie preexistante : code %d != 2 ou ecrasee" % r.returncode)
    inconnu = dossier / "j_arg.txt"
    r = generer(inconnu, "--graine")
    exiger("j", r.returncode == 2 and not inconnu.exists(), "argument inconnu : code %d != 2" % r.returncode)
    dupliquee = dossier / "j_dupliquee.txt"
    r = generer(dupliquee, "--famille-extension", "eight_clusters", "--famille-extension", "eight_clusters")
    exiger("j", r.returncode == 2 and not dupliquee.exists(), "option dupliquee : code %d != 2" % r.returncode)


def main() -> int:
    if not GENERATEUR.is_file():
        sys.stdout.write("generateur absent : %s\n" % GENERATEUR)
        return 1
    with tempfile.TemporaryDirectory(prefix="plan_keycsr_gate_") as tmp:
        dossier = Path(tmp)
        p1, p2, p3 = dossier / "p1.txt", dossier / "p2.txt", dossier / "p3.txt"
        for chemin, options in ((p1, ()), (p2, ()), (p3, ("--famille-extension", "eight_clusters"))):
            r = generer(chemin, *options)
            if not exiger("nominal", r.returncode == 0 and chemin.is_file(),
                          "generation %s : code %d, stderr=%r" % (chemin.name, r.returncode, r.stderr)):
                return 1
            exiger("nominal", "sha256_plan=" in r.stdout and "Traceback" not in r.stderr,
                   "stdout/stderr nominaux inattendus")
        en_tete_1, runs_1, lignes_1, octets_1 = lire_plan(p1)
        _, _, _, octets_2 = lire_plan(p2)
        en_tete_3, runs_3, lignes_3, octets_3 = lire_plan(p3)

        scene_a(en_tete_1, runs_1)
        scene_a(en_tete_3, runs_3)
        scene_b(octets_1, octets_2, runs_1, runs_3)
        scene_c(runs_1)
        scene_c(runs_3)
        scene_d(lignes_1, octets_1)
        scene_d(lignes_3, octets_3)
        scene_e(dossier, octets_1)
        scene_f(en_tete_1, runs_1, 15, "sans extension")
        scene_f(en_tete_3, runs_3, 20, "avec extension")
        scene_g(en_tete_1, runs_1, False)
        scene_g(en_tete_3, runs_3, True)
        scene_h(runs_1)
        scene_h(runs_3)
        scene_i(en_tete_1, runs_1)
        scene_i(en_tete_3, runs_3)
        scene_j(dossier)

    sys.stdout.write("graine=%s orientations_strate_1=%s sha256_plan(sans extension)=%s\n"
                     % (GRAINE_HEX, ",".join(ORIENTATIONS_STRATE_1), lignes_1[-1][len("sha256_plan="):]))
    if ECHECS:
        sys.stdout.write("plan_keycsr_gate : %d echec(s)\n" % len(ECHECS))
        return 1
    sys.stdout.write("plan_keycsr_gate : conforme (scenes a-j, %d + %d runs)\n" % (len(runs_1), len(runs_3)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
