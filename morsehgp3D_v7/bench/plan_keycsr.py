#!/usr/bin/env python3
"""Générateur du PLAN PRÉ-INSCRIT de la campagne de mesure KeyCSR.

Verrou : audits/REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md (« Réponse au
verrou de pré-inscription 53610911 », point 3 : graine externe ; « Plan
apparié et décision exhaustive »), pré-inscription
audits/QUESTION_CLAUDE_PREREG_MESURE_KEYCSR_20260902.md, accusé § 15 de
audits/REPONSE_CLAUDE_MULTICPU_GPU_20260901.md.

Ce que ce script fait, et rien d'autre : il écrit `plan.txt`, DÉTERMINISTE,
qui liste TOUS les runs de la campagne (strates, échauffements, six blocs
appariés par strate avec leurs orientations tirées, commandes exactes) et se
termine par `sha256_plan=` ; le lanceur de campagne hache ce fichier AVANT
tout run. Il n'exécute rien, ne lit aucun binaire, ne touche pas GCP.

Canon (rejoué par tests/plan_keycsr_gate.py, fixtures gravées) :
  - graine de base = huit premiers octets, lus en big-endian, du SHA-256 des
    octets UTF-8 exacts `morsehgp3D_v6:keycsr-prereg:v1:2026-09-02` (sans saut
    de ligne ni octet terminal) ; indépendante du commit, donc non reroulable
    par amendement ; les auditeurs annoncent 0xa2ffb4db2884ddc4 — la valeur
    est RECALCULÉE ici et la dérivation prime (les deux sont gravées) ;
  - PRNG SplitMix64 (Steele–Lea–Flood), état 64 bits initialisé à la graine ;
  - Fisher–Yates classique de Durstenfeld sur [AB, AB, AB, BA, BA, BA], i de
    n-1 à 1, j = mot mod (i + 1) : passage du mot 64 bits à l'indice borné
    DÉFINI ici (modulo) ; son biais (< 2^-61 pour i + 1 <= 6) est négligeable
    et ACCEPTÉ explicitement — jamais `random.shuffle` ni une bibliothèque ;
  - UN SEUL flux de tirages, consommé dans l'ordre canonique des strates :
    famille `uniform`, tailles 16000, 32000, 8000 (8000 diagnostique), cinq
    strates par taille (profil_callback, release_digest_off,
    release_digest_on, aa_digest_off, aa_digest_on), puis l'extension
    `eight_clusters` 32000 (cinq strates) consommée APRÈS uniform ; les
    échauffements ne consomment aucun tirage.

CLI : plan_keycsr.py --sortie plan.txt [--famille-extension eight_clusters]
      [--graine-hex 0x...]
      `--graine-hex` n'est admis qu'ÉGAL à la graine dérivée (aucune graine
      libre) ; toute autre valeur est un refus 2. Une sortie préexistante est
      refusée (2) : un plan gravé ne s'écrase pas.
Codes : 0 plan écrit ; 2 usage ou refus.
"""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

SCHEMA = "e-hgp.plan-keycsr.v1"

# Chaîne de dérivation de la graine externe (octets UTF-8 exacts, sans '\n').
# Historical v6 schedule domain retained by the explicit v7 port.
CHAINE_DERIVATION = "morsehgp3D_v6:keycsr-prereg:v1:2026-09-02"
# Valeur ANNONCÉE par les auditeurs (point 3 du verrou). Gravée pour
# confrontation ; la dérivation ci-dessous prime toujours.
GRAINE_ANNONCEE = 0xA2FFB4DB2884DDC4

MASQUE64 = (1 << 64) - 1
SPLITMIX_GAMMA = 0x9E3779B97F4A7C15
SPLITMIX_M1 = 0xBF58476D1CE4E5B9
SPLITMIX_M2 = 0x94D049BB133111EB

# Liste mélangée par strate : trois AB et trois BA, six blocs.
ORIENTATIONS_BASE = ("AB", "AB", "AB", "BA", "BA", "BA")
NB_BLOCS = 6

# Paramètres communs à toutes les commandes (pré-inscription § 1 et § 3).
THREADS = 8
INFLIGHT = 2
S = 8
SMAX = 11
SEED_ENTREE = 3
# Aucun `--coord` : le binaire applique cloud_family_default_coord
# (src/cloud/families.hpp) ; la commande le grave comme `coord=defaut`.
COORD = "defaut"
# Flag du callback témoin : n'existe PAS encore dans le binaire de profil ;
# gravé tel quel et signalé À CONFIRMER dans l'en-tête.
FLAG_CALLBACK = "--callback-temoin"

FAMILLE_PRIMAIRE = "uniform"
TAILLES_PRIMAIRES = (16000, 32000, 8000)  # 16k et 32k décisionnelles, 8k diagnostique
FAMILLES_EXTENSION = ("eight_clusters",)
TAILLE_EXTENSION = 32000

# Les cinq strates, dans l'ordre canonique. Champs : join, digest, callback,
# binaire/schéma, layouts des bras A et B.
STRATES = (
    {"nom": "profil_callback", "join": 1, "digest": "off", "callback": "on",
     "binaire": "profil", "schema": "reduce_v3", "A": "classic", "B": "csr"},
    {"nom": "release_digest_off", "join": 0, "digest": "off", "callback": "off",
     "binaire": "release", "schema": "release", "A": "classic", "B": "csr"},
    {"nom": "release_digest_on", "join": 0, "digest": "on", "callback": "off",
     "binaire": "release", "schema": "release", "A": "classic", "B": "csr"},
    {"nom": "aa_digest_off", "join": 0, "digest": "off", "callback": "off",
     "binaire": "release", "schema": "release", "A": "classic", "B": "classic"},
    {"nom": "aa_digest_on", "join": 0, "digest": "on", "callback": "off",
     "binaire": "release", "schema": "release", "A": "classic", "B": "classic"},
)


def graine_derivee() -> int:
    """Huit premiers octets big-endian du SHA-256 de la chaîne de dérivation."""
    condensat = hashlib.sha256(CHAINE_DERIVATION.encode("utf-8")).digest()
    return int.from_bytes(condensat[:8], "big")


class SplitMix64:
    """SplitMix64 de Steele–Lea–Flood, arithmétique modulo 2^64 explicite."""

    def __init__(self, graine: int) -> None:
        self.etat = graine & MASQUE64

    def suivant(self) -> int:
        self.etat = (self.etat + SPLITMIX_GAMMA) & MASQUE64
        z = self.etat
        z = ((z ^ (z >> 30)) * SPLITMIX_M1) & MASQUE64
        z = ((z ^ (z >> 27)) * SPLITMIX_M2) & MASQUE64
        return z ^ (z >> 31)


def mot_vers_indice(mot: int, borne: int) -> int:
    """Passage DÉFINI du mot 64 bits à l'indice dans [0, borne) : modulo.

    Le biais de modulo vaut au plus borne / 2^64 (< 2^-61 pour borne <= 6) ;
    il est négligeable et ACCEPTÉ explicitement par la pré-inscription. Ce
    passage est rejoué par une fixture gravée de la porte.
    """
    return mot % borne


def fisher_yates(liste: list, prng: SplitMix64) -> list:
    """Fisher–Yates classique (Durstenfeld) : i de n-1 à 1, j = mot mod (i+1).

    Consomme exactement n-1 mots du flux, dans l'ordre. Retourne une copie.
    """
    resultat = list(liste)
    for i in range(len(resultat) - 1, 0, -1):
        j = mot_vers_indice(prng.suivant(), i + 1)
        resultat[i], resultat[j] = resultat[j], resultat[i]
    return resultat


def liste_strates(famille_extension: str | None) -> list:
    """Ordre canonique : uniform (16000, 32000, 8000) × cinq strates, puis
    l'extension (32000 × cinq strates) comme bloc séparé, après uniform."""
    cellules = [(FAMILLE_PRIMAIRE, n) for n in TAILLES_PRIMAIRES]
    if famille_extension is not None:
        cellules.append((famille_extension, TAILLE_EXTENSION))
    strates = []
    for famille, n in cellules:
        for spec in STRATES:
            strates.append({"famille": famille, "n": n, **spec})
    return strates


def commande(strate: dict, layout: str) -> str:
    """argv exact d'un run (sans le chemin du binaire), ordre fixe."""
    args = [
        "--family=%s" % strate["famille"],
        "--n=%d" % strate["n"],
        "--s=%d" % S,
        "--smax=%d" % SMAX,
        "--seed=%d" % SEED_ENTREE,
        "--threads=%d" % THREADS,
        "--fold-inflight=%d" % INFLIGHT,
        "--fold-join=%d" % strate["join"],
        "--layout=%s" % layout,
    ]
    if strate["digest"] == "on":
        args.append("--digest")
    if strate["callback"] == "on":
        args.append(FLAG_CALLBACK)
    return " ".join(args)


def ligne_run(strate: dict, bloc: int, position: int, orientation: str,
              bras: str) -> str:
    layout = strate[bras]
    champs = [
        "strate=%s" % strate["nom"],
        "famille=%s" % strate["famille"],
        "n=%d" % strate["n"],
        "bloc=%d" % bloc,
        "position=%d" % position,
        "orientation=%s" % orientation,
        "bras=%s" % bras,
        "layout=%s" % layout,
        "digest=%s" % strate["digest"],
        "callback=%s" % strate["callback"],
        "join=%d" % strate["join"],
        "threads=%d" % THREADS,
        "inflight=%d" % INFLIGHT,
        "s=%d" % S,
        "smax=%d" % SMAX,
        "seed=%d" % SEED_ENTREE,
        "coord=%s" % COORD,
        "schema=%s" % strate["schema"],
        "binaire=%s" % strate["binaire"],
        "commande=%s" % commande(strate, layout),
    ]
    return " ".join(champs)


def lignes_strate(strate: dict, orientations: list) -> list:
    """Deux échauffements (bloc 0, ordre canonique A puis B, hors tirage) puis
    six blocs de deux runs adjacents dans l'orientation tirée."""
    lignes = [
        ligne_run(strate, 0, 1, "AB", "A"),
        ligne_run(strate, 0, 2, "AB", "B"),
    ]
    for bloc, orientation in enumerate(orientations, start=1):
        premier, second = orientation[0], orientation[1]
        lignes.append(ligne_run(strate, bloc, 1, orientation, premier))
        lignes.append(ligne_run(strate, bloc, 2, orientation, second))
    return lignes


def en_tete(graine: int, strates: list, famille_extension: str | None,
            mots_consommes: int, nb_runs: int) -> list:
    """En-tête ASCII (clé=valeur) : schéma, graine, dérivation, PRNG, mapping,
    strates. Aucune ligne ne commence par `strate=` (réservé aux runs)."""
    accord = "oui" if graine == GRAINE_ANNONCEE else "NON (la derivation prime)"
    noms = ",".join("%s:%d:%s" % (s["famille"], s["n"], s["nom"]) for s in strates)
    return [
        "schema=%s" % SCHEMA,
        "graine_hex=0x%016x" % graine,
        "derivation=huit premiers octets big-endian de sha256(utf8(\"%s\")),"
        " sans saut de ligne ni octet terminal" % CHAINE_DERIVATION,
        "graine_annoncee_auditeurs=0x%016x" % GRAINE_ANNONCEE,
        "graine_accord=%s" % accord,
        "prng=splitmix64 (Steele-Lea-Flood) : state = (state + 0x9E3779B97F4A7C15) mod 2^64 ;"
        " z = state ; z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) mod 2^64 ;"
        " z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) mod 2^64 ; mot = z ^ (z >> 31)",
        "melange=fisher_yates_durstenfeld sur [AB,AB,AB,BA,BA,BA] :"
        " pour i de 5 a 1, j = mot mod (i+1), echange(liste[i], liste[j]) ;"
        " cinq mots consommes par strate",
        "mapping=mot 64 bits -> indice borne : j = mot mod (i+1)"
        " (biais de modulo < 2^-61, ACCEPTE explicitement)",
        "flux=unique, consomme dans l'ordre canonique de la liste `strates` ;"
        " aucun tirage pour les echauffements ; aucun nouveau tirage ni remplacement",
        "mots_consommes=%d" % mots_consommes,
        "bras=A:classic B:csr ; strates aa_* : A:classic B:classic"
        " (l'orientation ne designe que la position ; pseudo-rapport = position 2 / position 1)",
        "orientation=AB : position 1 = A puis position 2 = B ; BA : position 1 = B puis position 2 = A",
        "echauffement=bloc=0, un run par bras dans l'ordre canonique A puis B,"
        " hors tirage, hors estimateur, attache a sa strate",
        "strate_profil_callback=binaire profil (MHGP7_PROFILE_REDUCE, schema reduce_v3),"
        " --fold-join=1, callback temoin arme, sans --digest",
        "strate_release_digest_off=binaire release, --fold-join=0, sans --digest",
        "strate_release_digest_on=binaire release, --fold-join=0, --digest",
        "strate_aa_digest_off=A/A binaire release, --fold-join=0, sans --digest,"
        " classic aux deux positions",
        "strate_aa_digest_on=A/A binaire release, --fold-join=0, --digest,"
        " classic aux deux positions",
        "callback_flag=%s A_CONFIRMER (le flag exact du callback temoin"
        " n'existe pas encore dans le binaire de profil)" % FLAG_CALLBACK,
        "coord=defaut (aucun --coord : cloud_family_default_coord de src/cloud/families.hpp)",
        "affinite=hors plan : posee par le lanceur (taskset 0-7 apres attestation du cpuset ;"
        " huit fils materiels sur quatre coeurs physiques)",
        "parametres_communs=threads=%d inflight=%d s=%d smax=%d seed=%d"
        % (THREADS, INFLIGHT, S, SMAX, SEED_ENTREE),
        "famille_extension=%s" % (famille_extension if famille_extension else "aucune"),
        "strates=%s" % noms,
        "nb_strates=%d" % len(strates),
        "nb_runs=%d" % nb_runs,
    ]


def generer(famille_extension: str | None) -> list:
    """Toutes les lignes du plan, hors la ligne finale sha256_plan."""
    graine = graine_derivee()
    strates = liste_strates(famille_extension)
    prng = SplitMix64(graine)
    lignes_runs = []
    mots = 0
    for strate in strates:
        orientations = fisher_yates(list(ORIENTATIONS_BASE), prng)
        mots += len(ORIENTATIONS_BASE) - 1
        lignes_runs.extend(lignes_strate(strate, orientations))
    nb_runs = len(lignes_runs)
    return en_tete(graine, strates, famille_extension, mots, nb_runs) + lignes_runs


def sceller(lignes: list) -> bytes:
    """Octets du fichier : lignes + '\\n', puis `sha256_plan=<sha256 des
    octets précédents>` + '\\n'."""
    corps = "".join(ligne + "\n" for ligne in lignes).encode("ascii")
    return corps + ("sha256_plan=%s\n" % hashlib.sha256(corps).hexdigest()).encode("ascii")


def usage(message: str) -> int:
    sys.stderr.write("REFUS : %s\n" % message)
    sys.stderr.write("usage : plan_keycsr.py --sortie plan.txt"
                     " [--famille-extension eight_clusters] [--graine-hex 0x...]\n")
    return 2


def main(argv: list) -> int:
    sortie = None
    famille_extension = None
    graine_hex = None
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--sortie" and i + 1 < len(argv):
            if sortie is not None:
                return usage("--sortie duplique")
            sortie = argv[i + 1]
            i += 2
        elif arg == "--famille-extension" and i + 1 < len(argv):
            if famille_extension is not None:
                return usage("--famille-extension duplique")
            famille_extension = argv[i + 1]
            i += 2
        elif arg == "--graine-hex" and i + 1 < len(argv):
            if graine_hex is not None:
                return usage("--graine-hex duplique")
            graine_hex = argv[i + 1]
            i += 2
        else:
            return usage("argument inconnu ou incomplet : %s" % arg)
    if sortie is None:
        return usage("--sortie obligatoire")
    if famille_extension is not None and famille_extension not in FAMILLES_EXTENSION:
        return usage("famille d'extension non pre-inscrite : %s (admises : %s)"
                     % (famille_extension, ", ".join(FAMILLES_EXTENSION)))
    graine = graine_derivee()
    if graine_hex is not None:
        # Aucune graine libre : la valeur fournie doit ÊTRE la graine dérivée.
        texte = graine_hex.lower()
        if not texte.startswith("0x") or len(texte) != 18:
            return usage("--graine-hex doit etre 0x suivi de 16 chiffres hexadecimaux")
        try:
            valeur = int(texte[2:], 16)
        except ValueError:
            return usage("--graine-hex illisible : %s" % graine_hex)
        if valeur != graine:
            return usage("--graine-hex 0x%016x differe de la graine DERIVEE 0x%016x"
                         " (aucune graine libre)" % (valeur, graine))
    chemin = Path(sortie)
    if chemin.exists():
        return usage("sortie preexistante, un plan grave ne s'ecrase pas : %s" % sortie)
    octets = sceller(generer(famille_extension))
    try:
        with open(chemin, "xb") as f:
            f.write(octets)
    except OSError as exc:
        return usage("ecriture impossible : %s (%s)" % (sortie, exc))
    sys.stdout.write("plan ecrit : %s (graine 0x%016x, %s)\n"
                     % (sortie, graine, octets.splitlines()[-1].decode("ascii")))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
