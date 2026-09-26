"""Porte a code de sortie exact pour le moteur de segment E-HGP.

Cadre annonce :

    phase=exploration_ehgp_hors_registre
    backend=python_reference
    profile=any_dimension_rational_exact
    mode=audit_independant_math_and_architecture
    public_status=not_claimed

Codes de sortie (convention du depot) :

    0  conforme
    1  desaccord du juge (le moteur non mute contredit une verite independante)
    2  refus avant calcul (options invalides, mutant sans effet possible)
    3  plancher de couverture ou invariant viole (dont mutant SURVIVANT)
    4  mutant tue (attendu quand `--inject` est donne)

La porte n'utilise nulle part le mot-cle `assert` : elle tient sous
`python3 -O`. Elle ne declare aucun resultat qu'elle n'a pas calcule.

Juges independants du code teste, ecrits dans ce fichier :

* les coefficients de `e_l(t) = ||y(t) - x_l||^2 = A t^2 + B_l t + C_l` sont
  reconstruits depuis les seules distances au carre aux extremites
  (`A = ||q - p||^2`, `C_l = e_l(0)`, `B_l = e_l(1) - e_l(0) - A`), sans
  reprendre le produit scalaire de `engine/segment.py` ;
* le temps annonce est valide GEOMETRIQUEMENT : le point `y(t)` est construit
  et toutes les distances au carre sont recalculees ;
* a l'ordre 1, le dendrogramme de l'arbre couvrant minimal euclidien
  (niveaux `d^2 / 4`) est recalcule par un Kruskal ecrit ici, et doit
  coincider EXACTEMENT avec la liaison simple des poids de segment ;
* quand l'etage exact est actif, l'ultrametrique projetee de la tour FULL
  (`exact/projection.py`) sert de verite : le moteur doit la MAJORER.

Mutants causaux (option `--inject`) :

    extremites               le maximum n'est evalue qu'en t = 0 et t = 1
    ordre_precedent          la k-ieme plus petite devient la (k-1)-ieme
    croisements_extremites   seuls les croisements des deux extremites sont
                             retenus comme temps candidats
    sans_extremites          les temps 0 et 1 sont retires du jeu de candidats

Chaque mutant doit etre TUE (code 4) par une porte qui, sans mutant, sort 0.
Les planchers sont verifies AVANT le verdict de mise a mort, pour qu'aucun
mutant ne soit declare tue sur un corpus vide.

Discipline des codes de sortie. Une exception non rattrapee ferait sortir
Python avec le code 1, c'est-a-dire le code du DESACCORD DU JUGE : un
plantage se lirait alors comme une contradiction du moteur. Le point d'entree
rattrape donc toute exception, imprime sa trace et rend 3 (invariant de la
porte viole). Le mutant `sans_extremites` peut par ailleurs vider le jeu de
temps candidats (positions confondues : aucune droite ne se croise) et rendre
un niveau `None` ; `poids_moteur` isole ces paires au lieu de les donner a
trier, ou la comparaison `None < Fraction` leverait une `TypeError`.
"""

import argparse
import os
import random
import sys
import traceback
from fractions import Fraction
from itertools import combinations

_RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if os.path.join(_RACINE, "src") not in sys.path:
    sys.path.insert(0, os.path.join(_RACINE, "src"))

from ehgp.engine.point_tower import PointTower
from ehgp.engine.segment import affine_parts, candidate_times, order_statistic
from ehgp.engine.segment import segment_maximum
from ehgp.exact.projection import projected_ultrametric
from ehgp.exact.tower import FullTower

CODE_CONFORME = 0
CODE_DESACCORD = 1
CODE_REFUS = 2
CODE_PLANCHER = 3
CODE_MUTANT_TUE = 4

MUTANTS = (
    "aucun",
    "extremites",
    "ordre_precedent",
    "croisements_extremites",
    "sans_extremites",
)

LIMITE_EXACTE_EFFECTIF = 9
LIMITE_EXACTE_ORDRE = 4


# ---------------------------------------------------------------------------
# Juges
# ---------------------------------------------------------------------------


def carre_distance(gauche, droite):
    """Distance au carre exacte, ecrite ici et pas importee."""
    total = 0
    for premiere, seconde in zip(gauche, droite):
        ecart = premiere - seconde
        total = total + ecart * ecart
    return total


def coefficients_extremites(nuage, source, cible):
    """`(A, [(B_l, C_l)])` reconstruits depuis les distances aux extremites."""
    depart = nuage[source]
    arrivee = nuage[cible]
    dominant = carre_distance(depart, arrivee)
    parties = []
    for point in nuage:
        debut = carre_distance(depart, point)
        fin = carre_distance(arrivee, point)
        parties.append((fin - debut - dominant, debut))
    return dominant, parties


def temps_croisements(parties, restreint_a=None):
    """Temps de `(0, 1)` ou deux droites du juge se croisent.

    `restreint_a` limite les croisements a un couple d'indices donne : c'est
    le jeu de temps du mutant `croisements_extremites`.
    """
    temps = {Fraction(0), Fraction(1)}
    if restreint_a is None:
        couples = combinations(range(len(parties)), 2)
    else:
        couples = (restreint_a,)
    for gauche, droite in couples:
        ecart = parties[gauche][0] - parties[droite][0]
        if ecart == 0:
            continue
        instant = Fraction(parties[droite][1] - parties[gauche][1], ecart)
        if 0 < instant < 1:
            temps.add(instant)
    return sorted(temps)


def valeur_juge(dominant, parties, instant, ordre):
    """`a_ordre(y(instant))` cote juge."""
    valeurs = sorted(pente * instant + constante for pente, constante in parties)
    return dominant * instant * instant + valeurs[ordre - 1]


def maximum_sur_temps(dominant, parties, temps, ordre):
    """Plus grande valeur du juge sur un jeu de temps donne."""
    meilleur = None
    for instant in temps:
        valeur = valeur_juge(dominant, parties, instant, ordre)
        if meilleur is None or valeur > meilleur:
            meilleur = valeur
    return meilleur


def verites_et_potentiels(nuage, source, cible, ordre):
    """Verite du juge et valeurs qu'AURAIENT les trois mutants.

    Renvoie un dictionnaire : `vrai`, `extremites`, `ordre_precedent`
    (`None` si l'ordre vaut 1, le mutant etant alors l'identite),
    `croisements_extremites` et `sans_extremites` (`None` quand le segment n'a
    aucun croisement interieur, cas des positions confondues).
    """
    dominant, parties = coefficients_extremites(nuage, source, cible)
    complet = temps_croisements(parties)
    extremites = [Fraction(0), Fraction(1)]
    restreints = temps_croisements(parties, (source, cible))
    interieurs = [instant for instant in complet if 0 < instant < 1]
    resultat = {
        "vrai": maximum_sur_temps(dominant, parties, complet, ordre),
        "extremites": maximum_sur_temps(dominant, parties, extremites, ordre),
        "croisements_extremites": maximum_sur_temps(
            dominant, parties, restreints, ordre
        ),
        "sans_extremites": maximum_sur_temps(dominant, parties, interieurs, ordre),
        "temps_candidats": len(complet),
    }
    if ordre >= 2:
        resultat["ordre_precedent"] = maximum_sur_temps(
            dominant, parties, complet, ordre - 1
        )
    else:
        resultat["ordre_precedent"] = None
    return resultat


def valeur_geometrique(nuage, source, cible, ordre, instant):
    """`a_ordre` au temps donne, en construisant reellement le point `y(t)`."""
    depart = nuage[source]
    arrivee = nuage[cible]
    point = tuple(
        Fraction(premiere) + instant * (seconde - premiere)
        for premiere, seconde in zip(depart, arrivee)
    )
    distances = sorted(carre_distance(point, autre) for autre in nuage)
    return distances[ordre - 1]


def ultrametrique_liaison_simple(poids, effectif):
    """Liaison simple explicite sur un dictionnaire de poids de paires."""
    aretes = sorted(
        (poids[(gauche, droite)], gauche, droite)
        for gauche, droite in combinations(range(effectif), 2)
    )
    parent = list(range(effectif))

    def racine(element):
        while parent[element] != element:
            element = parent[element]
        return element

    ultrametrique = {}
    for niveau, gauche, droite in aretes:
        racine_gauche = racine(gauche)
        racine_droite = racine(droite)
        if racine_gauche == racine_droite:
            continue
        membres_gauche = [
            index for index in range(effectif) if racine(index) == racine_gauche
        ]
        membres_droite = [
            index for index in range(effectif) if racine(index) == racine_droite
        ]
        for premier in membres_gauche:
            for second in membres_droite:
                ultrametrique[(min(premier, second), max(premier, second))] = niveau
        parent[racine_gauche] = racine_droite
    return ultrametrique


def ultrametrique_acm(nuage):
    """Dendrogramme de l'arbre couvrant minimal euclidien, niveaux `d^2 / 4`."""
    effectif = len(nuage)
    poids = {}
    for gauche, droite in combinations(range(effectif), 2):
        poids[(gauche, droite)] = carre_distance(nuage[gauche], nuage[droite]) * Fraction(
            1, 4
        )
    return ultrametrique_liaison_simple(poids, effectif)


# ---------------------------------------------------------------------------
# Moteur, eventuellement mute
# ---------------------------------------------------------------------------


def maximum_moteur(nuage, source, cible, ordre, mutant):
    """Maximum de segment du moteur, avec mutation eventuelle au site d'appel.

    Sans mutant, c'est exactement `segment_maximum` qui repond : la porte
    juge le moteur du depot, pas une copie.
    """
    if mutant == "aucun":
        return segment_maximum(nuage, source, cible, ordre)
    dominant, parties = affine_parts(nuage, source, cible)
    if mutant == "extremites":
        temps = [Fraction(0), Fraction(1)]
    elif mutant == "sans_extremites":
        temps = [instant for instant in candidate_times(parties) if 0 < instant < 1]
    elif mutant == "croisements_extremites":
        temps = [Fraction(0), Fraction(1)]
        ecart = parties[source][0] - parties[cible][0]
        if ecart != 0:
            instant = Fraction(parties[cible][1] - parties[source][1], ecart)
            if 0 < instant < 1:
                temps.append(instant)
        temps.sort()
    else:
        temps = candidate_times(parties)
    ordre_lu = ordre - 1 if (mutant == "ordre_precedent" and ordre >= 2) else ordre
    meilleur = None
    meilleur_temps = None
    for instant in temps:
        valeur = order_statistic(dominant, parties, instant, ordre_lu)
        if meilleur is None or valeur > meilleur:
            meilleur = valeur
            meilleur_temps = instant
    return meilleur, meilleur_temps


def poids_moteur(nuage, ordre, mutant):
    """`(poids, paires sans niveau)` de toutes les paires a un ordre donne.

    Un mutant peut vider le jeu de temps candidats et rendre `None` : une
    telle paire est ISOLEE et jamais donnee a trier, sans quoi la comparaison
    `None < Fraction` leverait une `TypeError` et la porte sortirait avec le
    code 1, celui du desaccord du juge.
    """
    effectif = len(nuage)
    poids = {}
    manquants = []
    for gauche, droite in combinations(range(effectif), 2):
        niveau, _temps = maximum_moteur(nuage, gauche, droite, ordre, mutant)
        if niveau is None:
            manquants.append((gauche, droite))
            continue
        poids[(gauche, droite)] = niveau
    return poids, manquants


# ---------------------------------------------------------------------------
# Corpus
# ---------------------------------------------------------------------------


def nuage_entier_alea(tirage, effectif, dimension, etendue):
    """Positions entieres distinctes, profil quantifie du depot."""
    points = set()
    garde = 0
    while len(points) < effectif:
        points.add(tuple(tirage.randrange(etendue) for _ in range(dimension)))
        garde += 1
        if garde > 100000:
            return None
    return sorted(points)


def nuage_colineaire(effectif, dimension):
    """Contre-famille colineaire."""
    return [
        tuple(index if axe == 0 else 0 for axe in range(dimension))
        for index in range(effectif)
    ]


def nuage_deux_lignes(effectif, dimension):
    """Contre-famille `two_lines` : deux droites paralleles entieres."""
    points = []
    for index in range(effectif):
        coordonnees = [0] * dimension
        coordonnees[0] = 2 * (index // 2)
        if index % 2 == 1:
            coordonnees[dimension - 1] = 3
        points.append(tuple(coordonnees))
    return points


def nuage_grappes(tirage, effectif, dimension, ecart):
    """Deux grappes entieres separees par `ecart`."""
    points = set()
    tentative = 0
    while len(points) < effectif:
        decalage = ecart if tentative % 2 else 0
        points.add(tuple(tirage.randrange(3) + decalage for _ in range(dimension)))
        tentative += 1
        if tentative > 100000:
            return None
    return sorted(points)


def nuage_duplique(tirage, effectif, dimension, etendue):
    """Positions dupliquees, admises par le profil d'entree."""
    base = nuage_entier_alea(tirage, max(2, effectif - 2), dimension, etendue)
    if base is None:
        return None
    return list(base) + [base[0], base[-1]]


def construire_corpus(options):
    """Corpus deterministe de nuages : `(nom, nuage)`."""
    tirage = random.Random(options.seed)
    corpus = []
    for numero in range(options.cases):
        nuage = nuage_entier_alea(
            tirage, options.n, options.d, max(4 * options.n, 8)
        )
        if nuage is not None:
            corpus.append(("uniforme_%d" % numero, nuage))
    corpus.append(("colineaire", nuage_colineaire(options.n, options.d)))
    if options.d >= 2:
        corpus.append(("deux_lignes", nuage_deux_lignes(options.n, options.d)))
    grappes = nuage_grappes(tirage, options.n, options.d, 20 * options.n)
    if grappes is not None:
        corpus.append(("grappes", grappes))
    if options.n >= 4:
        duplique = nuage_duplique(tirage, options.n, options.d, max(4 * options.n, 8))
        if duplique is not None:
            corpus.append(("duplique", duplique))
    if options.d == 2:
        corpus.append(("cocyclique", [(-2, 0), (0, -2), (0, 2), (2, 0)]))
    return corpus


# ---------------------------------------------------------------------------
# Etapes de la porte
# ---------------------------------------------------------------------------


def etape_juge(corpus, options, rapport):
    """Compare le moteur au juge, paire par paire et ordre par ordre."""
    desaccords = []
    for nom, nuage in corpus:
        effectif = len(nuage)
        ordre_maximal = min(options.k, effectif)
        for source, cible in combinations(range(effectif), 2):
            for ordre in range(1, ordre_maximal + 1):
                verites = verites_et_potentiels(nuage, source, cible, ordre)
                rapport["cas_juges"] += 1
                rapport["temps_candidats"] += verites["temps_candidats"]
                if verites["extremites"] != verites["vrai"]:
                    rapport["potentiel_extremites"] += 1
                if verites["croisements_extremites"] != verites["vrai"]:
                    rapport["potentiel_croisements_extremites"] += 1
                if verites["sans_extremites"] != verites["vrai"]:
                    rapport["potentiel_sans_extremites"] += 1
                if (
                    verites["ordre_precedent"] is not None
                    and verites["ordre_precedent"] != verites["vrai"]
                ):
                    rapport["potentiel_ordre_precedent"] += 1
                niveau, instant = maximum_moteur(
                    nuage, source, cible, ordre, options.inject
                )
                if niveau != verites["vrai"]:
                    desaccords.append(
                        "niveau %s : %s paire (%d,%d) k=%d moteur=%s juge=%s"
                        % (nom, nuage, source, cible, ordre, niveau, verites["vrai"])
                    )
                    continue
                if instant is None or not Fraction(0) <= instant <= Fraction(1):
                    desaccords.append(
                        "temps hors segment %s paire (%d,%d) k=%d : %s"
                        % (nom, source, cible, ordre, instant)
                    )
                    continue
                if valeur_geometrique(nuage, source, cible, ordre, instant) != niveau:
                    desaccords.append(
                        "temps non realisant %s paire (%d,%d) k=%d : t=%s"
                        % (nom, source, cible, ordre, instant)
                    )
    return desaccords


def etape_ordre_un(corpus, options, rapport):
    """A l'ordre 1, liaison simple des poids = dendrogramme de l'ACM."""
    desaccords = []
    for nom, nuage in corpus:
        effectif = len(nuage)
        poids, manquants = poids_moteur(nuage, 1, options.inject)
        if manquants:
            desaccords.append(
                "ordre 1 %s : %d paires sans niveau de segment, ex. %s"
                % (nom, len(manquants), manquants[0])
            )
            continue
        obtenue = ultrametrique_liaison_simple(poids, effectif)
        reference = ultrametrique_acm(nuage)
        rapport["paires_ordre_un"] += len(reference)
        if obtenue != reference:
            differentes = [
                cle for cle in reference if obtenue.get(cle) != reference[cle]
            ]
            desaccords.append(
                "ordre 1 %s : %d paires hors dendrogramme ACM, ex. %s (moteur=%s acm=%s)"
                % (
                    nom,
                    len(differentes),
                    differentes[0],
                    obtenue.get(differentes[0]),
                    reference[differentes[0]],
                )
            )
    return desaccords


def etape_forets(corpus, options, rapport):
    """Confronte la liaison simple ecrite ici a celle de `point_tower.py`."""
    desaccords = []
    for nom, nuage in corpus:
        effectif = len(nuage)
        ordre_maximal = min(options.k, effectif)
        moteur = PointTower(nuage, ordre_maximal)
        for ordre in range(1, ordre_maximal + 1):
            poids, manquants = poids_moteur(nuage, ordre, "aucun")
            if manquants:
                desaccords.append(
                    "foret %s ordre %d : %d paires sans niveau de segment"
                    % (nom, ordre, len(manquants))
                )
                continue
            obtenue = ultrametrique_liaison_simple(poids, effectif)
            if obtenue != moteur.cophenetic(ordre):
                desaccords.append(
                    "foret %s ordre %d : cophenetique du moteur differente de la"
                    " liaison simple independante"
                    % (nom, ordre)
                )
            rapport["paires_forets"] += len(obtenue)
    return desaccords


def etape_exacte(corpus, options, rapport):
    """Le moteur doit MAJORER l'ultrametrique exacte projetee."""
    desaccords = []
    for nom, nuage in corpus:
        effectif = len(nuage)
        if effectif > LIMITE_EXACTE_EFFECTIF:
            continue
        ordre_maximal = min(options.k, effectif, LIMITE_EXACTE_ORDRE)
        tour = FullTower(nuage, ordre_maximal)
        for ordre in range(1, ordre_maximal + 1):
            exacte = projected_ultrametric(tour, ordre)
            poids, manquants = poids_moteur(nuage, ordre, options.inject)
            if manquants:
                desaccords.append(
                    "exact %s ordre %d : %d paires sans niveau de segment"
                    % (nom, ordre, len(manquants))
                )
                continue
            candidate = ultrametrique_liaison_simple(poids, effectif)
            for paire, reference in exacte.items():
                if reference is None:
                    continue
                proposition = candidate.get(paire)
                rapport["paires_exactes"] += 1
                if proposition is None:
                    desaccords.append(
                        "exact %s ordre %d paire %s : le moteur ne relie jamais cette paire"
                        % (nom, ordre, paire)
                    )
                elif proposition < reference:
                    desaccords.append(
                        "exact %s ordre %d paire %s : moteur=%s SOUS la verite %s"
                        % (nom, ordre, paire, proposition, reference)
                    )
                elif proposition == reference:
                    rapport["paires_exactes_egales"] += 1
                else:
                    rapport["paires_exactes_strictes"] += 1
    return desaccords


def verifier_planchers(options, rapport):
    """Renvoie la liste des planchers non atteints."""
    manques = []
    seuil_cas = 10 * options.min_cases
    if rapport["cas_juges"] < seuil_cas:
        manques.append("cas_juges=%d < %d" % (rapport["cas_juges"], seuil_cas))
    if rapport["familles"] < 3:
        manques.append("familles=%d < 3" % rapport["familles"])
    if rapport["potentiel_extremites"] < options.min_cases:
        manques.append(
            "potentiel_extremites=%d < %d"
            % (rapport["potentiel_extremites"], options.min_cases)
        )
    if rapport["potentiel_croisements_extremites"] < options.min_cases:
        manques.append(
            "potentiel_croisements_extremites=%d < %d"
            % (rapport["potentiel_croisements_extremites"], options.min_cases)
        )
    if rapport["potentiel_sans_extremites"] < options.min_cases:
        manques.append(
            "potentiel_sans_extremites=%d < %d"
            % (rapport["potentiel_sans_extremites"], options.min_cases)
        )
    if options.inject == "aucun" and rapport["paires_forets"] < options.min_cases:
        manques.append(
            "paires_forets=%d < %d" % (rapport["paires_forets"], options.min_cases)
        )
    if options.k >= 2 and rapport["potentiel_ordre_precedent"] < options.min_cases:
        manques.append(
            "potentiel_ordre_precedent=%d < %d"
            % (rapport["potentiel_ordre_precedent"], options.min_cases)
        )
    if rapport["paires_ordre_un"] < options.min_cases:
        manques.append(
            "paires_ordre_un=%d < %d" % (rapport["paires_ordre_un"], options.min_cases)
        )
    if rapport["etage_exact"] and rapport["paires_exactes"] < options.min_cases:
        manques.append(
            "paires_exactes=%d < %d" % (rapport["paires_exactes"], options.min_cases)
        )
    return manques


# ---------------------------------------------------------------------------
# Ligne de commande
# ---------------------------------------------------------------------------


def construire_analyseur():
    """Analyseur d'options de la porte."""
    analyseur = argparse.ArgumentParser(
        prog="gate_engine.py",
        description="Porte du moteur de segment E-HGP (codes 0/1/2/3/4).",
        epilog="0 conforme, 1 desaccord du juge, 2 refus avant calcul, "
        "3 plancher ou invariant viole, 4 mutant tue.",
    )
    analyseur.add_argument("--n", type=int, default=7, help="effectif par nuage")
    analyseur.add_argument("--d", type=int, default=3, help="dimension ambiante")
    analyseur.add_argument("--k", type=int, default=3, help="ordre maximal teste")
    analyseur.add_argument("--seed", type=int, default=3, help="graine du corpus")
    analyseur.add_argument(
        "--cases", type=int, default=2, help="nombre de nuages uniformes tires"
    )
    analyseur.add_argument(
        "--min-cases",
        dest="min_cases",
        type=int,
        default=8,
        help="plancher de couverture par mutant et par etape",
    )
    analyseur.add_argument(
        "--inject",
        default="aucun",
        help="mutant injecte : %s" % ", ".join(MUTANTS),
    )
    analyseur.add_argument(
        "--exact",
        default="auto",
        help="etage exact (tour FULL) : auto, on, off",
    )
    return analyseur


def valider(options):
    """Renvoie la liste des refus avant calcul."""
    refus = []
    if options.n < 2:
        refus.append("--n doit valoir au moins 2")
    if options.d < 1:
        refus.append("--d doit valoir au moins 1")
    if options.k < 1:
        refus.append("--k doit valoir au moins 1")
    if options.k > options.n:
        refus.append("--k ne peut depasser --n")
    if options.cases < 1:
        refus.append("--cases doit valoir au moins 1")
    if options.min_cases < 1:
        refus.append("--min-cases doit valoir au moins 1")
    if options.seed < 0:
        refus.append("--seed doit etre positif ou nul")
    if options.inject not in MUTANTS:
        refus.append("--inject inconnu : %s" % options.inject)
    if options.exact not in ("auto", "on", "off"):
        refus.append("--exact doit valoir auto, on ou off")
    if options.inject == "ordre_precedent" and options.k < 2:
        refus.append(
            "le mutant ordre_precedent est l'identite a l'ordre 1 : exiger --k >= 2"
        )
    if options.n > 2 ** 14:
        refus.append("--n hors domaine de cette porte quadratique")
    if options.exact == "on" and options.n > LIMITE_EXACTE_EFFECTIF:
        refus.append(
            "--exact=on exige --n <= %d (l'oracle de la tour FULL est borne) :"
            " sinon l'etage exact ne verrait aucun nuage" % LIMITE_EXACTE_EFFECTIF
        )
    return refus


def principale(arguments):
    """Execute la porte et renvoie son code de sortie."""
    analyseur = construire_analyseur()
    options = analyseur.parse_args(arguments)
    refus = valider(options)
    if refus:
        for ligne in refus:
            print("refus : %s" % ligne)
        print("verdict : refus avant calcul")
        return CODE_REFUS
    etage_exact = options.exact == "on" or (
        options.exact == "auto"
        and options.n <= LIMITE_EXACTE_EFFECTIF
        and options.k <= LIMITE_EXACTE_ORDRE
    )
    corpus = construire_corpus(options)
    rapport = {
        "familles": len(corpus),
        "cas_juges": 0,
        "temps_candidats": 0,
        "potentiel_extremites": 0,
        "potentiel_ordre_precedent": 0,
        "potentiel_croisements_extremites": 0,
        "potentiel_sans_extremites": 0,
        "paires_ordre_un": 0,
        "paires_forets": 0,
        "paires_exactes": 0,
        "paires_exactes_egales": 0,
        "paires_exactes_strictes": 0,
        "etage_exact": etage_exact,
    }
    desaccords = []
    desaccords.extend(etape_juge(corpus, options, rapport))
    desaccords.extend(etape_ordre_un(corpus, options, rapport))
    if options.inject == "aucun":
        desaccords.extend(etape_forets(corpus, options, rapport))
    if etage_exact:
        desaccords.extend(etape_exacte(corpus, options, rapport))

    print("porte du moteur de segment E-HGP")
    print(
        "  options      n=%d d=%d k=%d seed=%d cases=%d min-cases=%d inject=%s exact=%s"
        % (
            options.n,
            options.d,
            options.k,
            options.seed,
            options.cases,
            options.min_cases,
            options.inject,
            "on" if etage_exact else "off",
        )
    )
    print("  familles     %s" % ", ".join(nom for nom, _nuage in corpus))
    for cle in sorted(rapport):
        if cle in ("familles", "etage_exact"):
            continue
        print("  %-32s %s" % (cle, rapport[cle]))
    print("  desaccords                       %d" % len(desaccords))
    for ligne in desaccords[:5]:
        print("    - %s" % ligne)

    manques = verifier_planchers(options, rapport)
    if manques:
        for ligne in manques:
            print("plancher : %s" % ligne)
        print("verdict : plancher de couverture non atteint")
        return CODE_PLANCHER
    if options.inject == "aucun":
        if desaccords:
            print("verdict : desaccord du juge sur le moteur non mute")
            return CODE_DESACCORD
        print("verdict : conforme")
        return CODE_CONFORME
    if desaccords:
        print("verdict : mutant %s tue" % options.inject)
        return CODE_MUTANT_TUE
    print(
        "verdict : mutant %s SURVIVANT (la porte ne le voit pas)" % options.inject
    )
    return CODE_PLANCHER


if __name__ == "__main__":
    try:
        _CODE = principale(sys.argv[1:])
    except Exception:
        traceback.print_exc()
        print("verdict : exception inattendue (invariant de la porte viole)")
        _CODE = CODE_PLANCHER
    sys.exit(_CODE)
