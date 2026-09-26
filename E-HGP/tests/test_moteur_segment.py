"""Portes du moteur de segment E-HGP : juges independants, fixtures, planchers.

Cadre annonce :

    phase=exploration_ehgp_hors_registre
    backend=python_reference
    profile=any_dimension_rational_exact
    mode=audit_independant_math_and_architecture
    public_status=not_claimed

Le moteur n'est jamais juge par lui-meme. Trois chemins d'evaluation
independants du code teste sont ecrits dans ce module.

1. Juge par distances aux extremites. Les coefficients de
   `e_l(t) = ||y(t) - x_l||^2 = A t^2 + B_l t + C_l` sont reconstruits a
   partir des seules distances au carre aux deux extremites du segment :
   `A = ||q - p||^2`, `C_l = e_l(0)`, `B_l = e_l(1) - e_l(0) - A`. Aucun
   produit scalaire de `engine/segment.py` n'est repris.
2. Juge geometrique. Au temps rationnel annonce par le moteur, le point
   `y(t)` est CONSTRUIT et toutes les distances au carre sont recalculees :
   la k-ieme plus petite est lue sur la geometrie, pas sur une
   decomposition affine.
3. Echantillonnage dense entier. Sur les temps `j / N` (N = 2000 au moins),
   la valeur est evaluee en ENTIERS apres mise a l'echelle par `N^2`. C'est
   un MINORANT certifie du maximum : une porte contre un maximum
   sous-estime, jamais une preuve d'exactitude.

Ce que ces portes etablissent, et ce qu'elles n'etablissent pas. L'egalite
avec les juges est une verification d'IMPLEMENTATION. La majoration de
l'ultrametrique exacte projetee par celle du moteur est un THEOREME (un
segment contenu dans `L_k(a)` prouve que ses extremites sont dans la meme
composante, et le maximum d'une ultrametrique de liaison simple est
monotone en ses poids) : une violation serait donc un defaut de code, pas
une surprise mathematique. Aucune porte de ce module ne promeut quoi que ce
soit a `public_status=exact`.

Faits mesures hors suite (pour memoire, non rejoues ici) : sur les 2300
nuages de trois points de la grille entiere 5 x 5 en dimension 2, AUCUN ne
donne de majoration stricte a l'ordre 2 ; le temoin grave
`((0, 0), (0, 1), (1, 3), (2, 2))` est donc minimal en effectif sur cette
grille. La suite en rejoue une version bornee (grille 3 x 3).
"""

import os
import random
import sys
import unittest
from fractions import Fraction
from itertools import combinations

_RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if os.path.join(_RACINE, "src") not in sys.path:
    sys.path.insert(0, os.path.join(_RACINE, "src"))

from ehgp.engine.point_tower import PointTower
from ehgp.engine.segment import (
    affine_parts,
    candidate_times,
    entry_levels,
    order_statistic,
    rational_cloud,
    segment_maximum,
)
from ehgp.exact.projection import compare_ultrametrics, projected_ultrametric
from ehgp.exact.tower import FullTower

# ---------------------------------------------------------------------------
# Planchers de couverture : la suite echoue si l'un d'eux n'est pas atteint.
# ---------------------------------------------------------------------------

PLANCHER_DIMENSIONS_JUGE = 5
PLANCHER_TIRAGES_JUGE = 15
PLANCHER_COMPARAISONS_JUGE = 600
PLANCHER_ECHANTILLONS = 2000
PLANCHER_MAXIMA_INTERIEURS = 120
PLANCHER_FIXTURES_SEGMENT = 14
PLANCHER_TIRAGES_ORDRE_UN = 15
PLANCHER_PAIRES_ORDRE_UN = 300
PLANCHER_DIMENSIONS_ORDRE_UN = 5
PLANCHER_CAS_MAJORATION = 10
PLANCHER_PAIRES_MAJORATION = 250
PLANCHER_MAJORATIONS_STRICTES = 10
PLANCHER_DIMENSIONS_MAJORATION = 4
PLANCHER_CAS_EQUIVARIANCE = 6
PLANCHER_DEGENERESCENCES = 14

NOMBRE_ECHANTILLONS = 2000

COMPTEURS = {}
MESURES = {}


def _compter(cle, quantite=1):
    """Incremente un compteur de couverture."""
    COMPTEURS[cle] = COMPTEURS.get(cle, 0) + quantite


def _mesurer(cle, valeur):
    """Enregistre une mesure non entiere (ratio, part d'egalite)."""
    MESURES[cle] = valeur


# ---------------------------------------------------------------------------
# Juges independants
# ---------------------------------------------------------------------------


def _carre_distance(gauche, droite):
    """Distance au carre exacte, ecrite ici et pas importee."""
    total = 0
    for premiere, seconde in zip(gauche, droite):
        ecart = premiere - seconde
        total = total + ecart * ecart
    return total


def _coefficients_extremites(nuage, source, cible):
    """`(A, [(B_l, C_l)])` reconstruits depuis les distances aux extremites."""
    depart = nuage[source]
    arrivee = nuage[cible]
    dominant = _carre_distance(depart, arrivee)
    parties = []
    for point in nuage:
        debut = _carre_distance(depart, point)
        fin = _carre_distance(arrivee, point)
        parties.append((fin - debut - dominant, debut))
    return dominant, parties


def _temps_candidats_juge(parties):
    """Temps de `[0, 1]` ou l'ordre des droites du juge peut changer."""
    temps = {Fraction(0), Fraction(1)}
    nombre = len(parties)
    for gauche in range(nombre):
        for droite in range(gauche + 1, nombre):
            ecart = parties[gauche][0] - parties[droite][0]
            if ecart == 0:
                continue
            instant = Fraction(parties[droite][1] - parties[gauche][1], ecart)
            if 0 < instant < 1:
                temps.add(instant)
    return sorted(temps)


def _valeur_juge(dominant, parties, instant, ordre):
    """`a_ordre(y(instant))` cote juge, coefficients reconstruits."""
    valeurs = sorted(pente * instant + constante for pente, constante in parties)
    return dominant * instant * instant + valeurs[ordre - 1]


def _maximum_juge(nuage, source, cible, ordre):
    """`(niveau, nombre de temps candidats)` juges pour le segment."""
    dominant, parties = _coefficients_extremites(nuage, source, cible)
    temps = _temps_candidats_juge(parties)
    meilleur = None
    for instant in temps:
        valeur = _valeur_juge(dominant, parties, instant, ordre)
        if meilleur is None or valeur > meilleur:
            meilleur = valeur
    return meilleur, len(temps)


def _valeur_geometrique(nuage, source, cible, ordre, instant):
    """`a_ordre` au temps donne, en construisant reellement le point `y(t)`."""
    depart = nuage[source]
    arrivee = nuage[cible]
    point = tuple(
        Fraction(premiere) + instant * (seconde - premiere)
        for premiere, seconde in zip(depart, arrivee)
    )
    distances = sorted(_carre_distance(point, autre) for autre in nuage)
    return distances[ordre - 1]


def _maximum_echantillonne(nuage, source, cible, ordre, nombre):
    """Plus grande valeur sur les temps `j / nombre`, en entiers exacts.

    Mise a l'echelle : `nombre^2 a_k(j / nombre) = A j^2 + B_l j nombre +
    C_l nombre^2`. L'ordre des `B_l j nombre + C_l nombre^2` est celui des
    `B_l j / nombre + C_l`, donc la k-ieme plus petite est la meme. Le
    resultat est un MINORANT du maximum sur le segment.
    """
    dominant, parties = _coefficients_extremites(nuage, source, cible)
    carre = nombre * nombre
    meilleur = None
    for pas in range(nombre + 1):
        valeurs = sorted(
            pente * pas * nombre + constante * carre for pente, constante in parties
        )
        valeur = dominant * pas * pas + valeurs[ordre - 1]
        if meilleur is None or valeur > meilleur:
            meilleur = valeur
    return Fraction(meilleur, carre)


def _ultrametrique_acm(nuage):
    """Dendrogramme de l'arbre couvrant minimal euclidien, niveaux `d^2 / 4`.

    Reference ecrite ici (Kruskal plus cophenetique explicite), sans aucun
    emprunt a `engine/point_tower.py`.
    """
    effectif = len(nuage)
    aretes = sorted(
        (
            _carre_distance(nuage[gauche], nuage[droite]) * Fraction(1, 4),
            gauche,
            droite,
        )
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
                cle = (min(premier, second), max(premier, second))
                ultrametrique[cle] = niveau
        parent[racine_gauche] = racine_droite
    return ultrametrique


def _transporter(ultrametrique, permutation):
    """Transporte une ultrametrique par `permute[i] = base[permutation[i]]`."""
    inverse = [0] * len(permutation)
    for nouveau, ancien in enumerate(permutation):
        inverse[ancien] = nouveau
    transportee = {}
    for (gauche, droite), niveau in ultrametrique.items():
        image = (inverse[gauche], inverse[droite])
        transportee[(min(image), max(image))] = niveau
    return transportee


def _mettre_a_echelle(ultrametrique, facteur):
    """Multiplie tous les niveaux definis par `facteur`, garde `None`."""
    return {
        cle: (None if niveau is None else niveau * facteur)
        for cle, niveau in ultrametrique.items()
    }


# ---------------------------------------------------------------------------
# Familles de nuages
# ---------------------------------------------------------------------------


def _nuage_entier_alea(tirage, effectif, dimension, etendue):
    """Positions entieres distinctes, profil quantifie du depot."""
    points = set()
    while len(points) < effectif:
        points.add(tuple(tirage.randrange(etendue) for _ in range(dimension)))
    return sorted(points)


def _nuage_colineaire(effectif, dimension):
    """Contre-famille colineaire : une refutation, jamais un regime."""
    return [
        tuple(index if axe == 0 else 0 for axe in range(dimension))
        for index in range(effectif)
    ]


def _nuage_deux_lignes(effectif, dimension):
    """Contre-famille `two_lines` : deux droites paralleles entieres."""
    points = []
    for index in range(effectif):
        coordonnees = [0] * dimension
        coordonnees[0] = 2 * (index // 2)
        if index % 2 == 1:
            coordonnees[dimension - 1] = 3
        points.append(tuple(coordonnees))
    return points


def _nuage_grappes(tirage, effectif, dimension, ecart):
    """Deux grappes entieres separees par `ecart` sur tous les axes."""
    points = set()
    tentative = 0
    while len(points) < effectif:
        decalage = ecart if tentative % 2 else 0
        points.add(tuple(tirage.randrange(3) + decalage for _ in range(dimension)))
        tentative += 1
    return sorted(points)


def _nuage_duplique(tirage, effectif, dimension, etendue):
    """Positions dupliquees : admises par le profil d'entree, bucketisees."""
    base = _nuage_entier_alea(tirage, max(2, effectif - 2), dimension, etendue)
    return list(base) + [base[0], base[-1]]


def _nuage_rationnel(tirage, effectif, dimension, etendue, denominateur):
    """Nuage a coordonnees rationnelles non entieres."""
    points = set()
    while len(points) < effectif:
        points.add(
            tuple(
                Fraction(tirage.randrange(etendue), denominateur)
                for _ in range(dimension)
            )
        )
    return sorted(points)


# ---------------------------------------------------------------------------
# Fixtures gravees : coordonnees exactes, niveaux exacts, temps exacts.
# ---------------------------------------------------------------------------

_TRIANGLE = ((0, 0), (4, 0), (0, 3))
_COLINEAIRE_QUATRE = ((0, 0), (1, 0), (2, 0), (3, 0))
_CARRE_COCYCLIQUE = ((-2, 0), (0, -2), (0, 2), (2, 0))
_DUPLIQUE = ((0, 0), (0, 0), (5, 0))
_SIMPLEXE_D20 = (
    tuple(0 for _ in range(20)),
    tuple(1 if axe == 0 else 0 for axe in range(20)),
    tuple(2 if axe == 1 else 0 for axe in range(20)),
)
_TIERS_TROIS = ((0, 0), (6, 0), (3, 1))
_TIERS_QUATRE = ((0, 0), (6, 0), (1, 1), (5, 1))
_TIERS_CINQ = ((0, 0), (8, 0), (2, 3), (6, 1))

FIXTURES_SEGMENT = (
    # nom, nuage, paire, ordre, niveau exact, temps exact renvoye
    ("paire_simple", ((0, 0), (4, 0)), (0, 1), 1, Fraction(4), Fraction(1, 2)),
    ("triangle_k1", _TRIANGLE, (0, 1), 1, Fraction(4), Fraction(1, 2)),
    ("triangle_k1_bis", _TRIANGLE, (1, 2), 1, Fraction(25, 4), Fraction(1, 2)),
    ("triangle_k2", _TRIANGLE, (0, 1), 2, Fraction(16), Fraction(1)),
    ("triangle_k3", _TRIANGLE, (0, 1), 3, Fraction(25), Fraction(1)),
    ("colineaire_k1", _COLINEAIRE_QUATRE, (0, 3), 1, Fraction(1, 4), Fraction(1, 6)),
    ("colineaire_k2", _COLINEAIRE_QUATRE, (0, 3), 2, Fraction(1), Fraction(0)),
    ("colineaire_k4", _COLINEAIRE_QUATRE, (0, 3), 4, Fraction(9), Fraction(0)),
    ("cocyclique_k1", _CARRE_COCYCLIQUE, (0, 3), 1, Fraction(4), Fraction(1, 2)),
    ("cocyclique_k3", _CARRE_COCYCLIQUE, (0, 3), 3, Fraction(8), Fraction(0)),
    ("duplique_k1", _DUPLIQUE, (0, 2), 1, Fraction(25, 4), Fraction(1, 2)),
    ("duplique_k2", _DUPLIQUE, (0, 2), 2, Fraction(25), Fraction(1)),
    ("duplique_confondu", _DUPLIQUE, (0, 1), 2, Fraction(0), Fraction(0)),
    ("simplexe_d20_k1", _SIMPLEXE_D20, (0, 1), 1, Fraction(1, 4), Fraction(1, 2)),
    ("simplexe_d20_k3", _SIMPLEXE_D20, (0, 1), 3, Fraction(5), Fraction(1)),
    ("tiers_trois_k1", _TIERS_TROIS, (0, 1), 1, Fraction(25, 9), Fraction(5, 18)),
    ("tiers_quatre_k2", _TIERS_QUATRE, (0, 1), 2, Fraction(169, 25), Fraction(13, 30)),
    ("tiers_cinq_k1", _TIERS_CINQ, (0, 1), 1, Fraction(1369, 144), Fraction(37, 96)),
    ("tiers_cinq_k2", _TIERS_CINQ, (0, 1), 2, Fraction(225, 16), Fraction(17, 32)),
)

# Temoins graves de MAJORATION STRICTE : le moteur des segments est un
# majorant certifie, et il est parfois STRICTEMENT au-dessus de la verite.
# Aucune violation (moteur sous la verite) n'a ete trouvee ; ces fixtures
# gravent donc l'ecart, pas un defaut.
FIXTURES_MAJORATION = (
    {
        "nom": "temoin_minimal_n4",
        "nuage": ((0, 0), (0, 1), (1, 3), (2, 2)),
        "ordre": 2,
        "exact": {
            (0, 1): Fraction(1),
            (0, 2): Fraction(2),
            (0, 3): Fraction(2),
            (1, 2): Fraction(2),
            (1, 3): Fraction(2),
            (2, 3): Fraction(2),
        },
        "moteur": {
            (0, 1): Fraction(1),
            (0, 2): Fraction(20, 9),
            (0, 3): Fraction(20, 9),
            (1, 2): Fraction(20, 9),
            (1, 3): Fraction(20, 9),
            (2, 3): Fraction(2),
        },
        "strictes": 4,
        "ratio_max": Fraction(10, 9),
    },
    {
        "nom": "temoin_n5",
        "nuage": ((0, 0), (0, 1), (1, 3), (2, 1), (3, 2)),
        "ordre": 2,
        "exact": {
            (0, 1): Fraction(1),
            (0, 2): Fraction(5),
            (0, 3): Fraction(2),
            (0, 4): Fraction(2),
            (1, 2): Fraction(5),
            (1, 3): Fraction(2),
            (1, 4): Fraction(2),
            (2, 3): Fraction(5),
            (2, 4): Fraction(5),
            (3, 4): Fraction(2),
        },
        "moteur": {
            (0, 1): Fraction(1),
            (0, 2): Fraction(5),
            (0, 3): Fraction(5, 2),
            (0, 4): Fraction(5, 2),
            (1, 2): Fraction(5),
            (1, 3): Fraction(5, 2),
            (1, 4): Fraction(5, 2),
            (2, 3): Fraction(5),
            (2, 4): Fraction(5),
            (3, 4): Fraction(2),
        },
        "strictes": 4,
        "ratio_max": Fraction(5, 4),
    },
    {
        "nom": "cocyclique_ordre_trois",
        "nuage": _CARRE_COCYCLIQUE,
        "ordre": 3,
        "exact": {
            (0, 1): Fraction(8),
            (0, 2): Fraction(8),
            (0, 3): Fraction(8),
            (1, 2): Fraction(8),
            (1, 3): Fraction(8),
            (2, 3): Fraction(8),
        },
        "moteur": {
            (0, 1): Fraction(10),
            (0, 2): Fraction(10),
            (0, 3): Fraction(8),
            (1, 2): Fraction(8),
            (1, 3): Fraction(10),
            (2, 3): Fraction(10),
        },
        "strictes": 4,
        "ratio_max": Fraction(5, 4),
    },
    {
        "nom": "colineaire_ordre_trois_egalite",
        "nuage": _COLINEAIRE_QUATRE,
        "ordre": 3,
        "exact": {
            (0, 1): Fraction(4),
            (0, 2): Fraction(4),
            (0, 3): Fraction(4),
            (1, 2): Fraction(9, 4),
            (1, 3): Fraction(4),
            (2, 3): Fraction(4),
        },
        "moteur": {
            (0, 1): Fraction(4),
            (0, 2): Fraction(4),
            (0, 3): Fraction(4),
            (1, 2): Fraction(9, 4),
            (1, 3): Fraction(4),
            (2, 3): Fraction(4),
        },
        "strictes": 0,
        "ratio_max": Fraction(1),
    },
    {
        "nom": "duplique_ordre_deux_egalite",
        "nuage": _DUPLIQUE,
        "ordre": 2,
        "exact": {(0, 1): Fraction(0), (0, 2): Fraction(25), (1, 2): Fraction(25)},
        "moteur": {(0, 1): Fraction(0), (0, 2): Fraction(25), (1, 2): Fraction(25)},
        "strictes": 0,
        # `compare_ultrametrics` exclut du ratio les paires de niveau exact
        # nul (les deux positions confondues) : le ratio maximal vaut donc 1
        # et non `None`.
        "ratio_max": Fraction(1),
    },
)


def _violation_presente(nuage, ordre, paire):
    """Vrai si le moteur passe STRICTEMENT SOUS la verite sur cette paire."""
    tour = FullTower(list(nuage), ordre)
    moteur = PointTower(list(nuage), ordre)
    exacte = projected_ultrametric(tour, ordre)
    candidate = moteur.cophenetic(ordre)
    reference = exacte.get(paire)
    proposition = candidate.get(paire)
    if reference is None:
        return False
    if proposition is None:
        return True
    return proposition < reference


def _reduire_violation(nuage, ordre, paire):
    """Reduit une violation a un sous-nuage minimal qui la reproduit encore."""
    gardes = list(range(len(nuage)))
    courant = list(nuage)
    paire_courante = paire
    progresse = True
    while progresse and len(gardes) > 2:
        progresse = False
        for position in range(len(gardes) - 1, -1, -1):
            if position in paire_courante:
                continue
            candidat = [courant[index] for index in range(len(courant)) if index != position]
            decalage = (
                paire_courante[0] - (1 if paire_courante[0] > position else 0),
                paire_courante[1] - (1 if paire_courante[1] > position else 0),
            )
            nouvelle_paire = (min(decalage), max(decalage))
            if _violation_presente(candidat, ordre, nouvelle_paire):
                courant = candidat
                paire_courante = nouvelle_paire
                gardes.pop(position)
                progresse = True
                break
    return courant, paire_courante


# ---------------------------------------------------------------------------
# (a) Maximum de segment
# ---------------------------------------------------------------------------


class TestAMaximumSegment(unittest.TestCase):
    """Le maximum de segment est exact, realise, et majore tout echantillon."""

    def test_maximum_contre_trois_juges(self):
        tirage = random.Random(3)
        dimensions = (2, 3, 5, 20, 50)
        effectif = 6
        ordres = (1, 2, 3)
        comparaisons = 0
        interieurs = 0
        tirages = 0
        temps_candidats = 0
        for dimension in dimensions:
            for _ in range(3):
                nuage = _nuage_entier_alea(tirage, effectif, dimension, 4 * effectif)
                tirages += 1
                for source, cible in combinations(range(effectif), 2):
                    for ordre in ordres:
                        niveau, instant = segment_maximum(nuage, source, cible, ordre)
                        juge, nombre_temps = _maximum_juge(nuage, source, cible, ordre)
                        temps_candidats += nombre_temps
                        comparaisons += 1
                        self.assertEqual(
                            niveau,
                            juge,
                            "desaccord du juge par extremites : d=%d nuage=%s paire=(%d,%d) k=%d"
                            % (dimension, nuage, source, cible, ordre),
                        )
                        self.assertTrue(
                            Fraction(0) <= instant <= Fraction(1),
                            "temps hors du segment : %s" % (instant,),
                        )
                        geometrique = _valeur_geometrique(
                            nuage, source, cible, ordre, instant
                        )
                        self.assertEqual(
                            geometrique,
                            niveau,
                            "temps non realisant : nuage=%s paire=(%d,%d) k=%d"
                            % (nuage, source, cible, ordre),
                        )
                        minorant = _maximum_echantillonne(
                            nuage, source, cible, ordre, NOMBRE_ECHANTILLONS
                        )
                        self.assertLessEqual(
                            minorant,
                            niveau,
                            "echantillon au-dessus du maximum : nuage=%s paire=(%d,%d) k=%d"
                            % (nuage, source, cible, ordre),
                        )
                        if Fraction(0) < instant < Fraction(1):
                            interieurs += 1
        _compter("juge_dimensions", len(dimensions))
        _compter("juge_tirages", tirages)
        _compter("juge_comparaisons", comparaisons)
        _compter("juge_temps_candidats", temps_candidats)
        _compter("juge_maxima_interieurs", interieurs)
        _compter("juge_echantillons_par_comparaison", NOMBRE_ECHANTILLONS)
        self.assertGreaterEqual(len(dimensions), PLANCHER_DIMENSIONS_JUGE)
        self.assertGreaterEqual(tirages, PLANCHER_TIRAGES_JUGE)
        self.assertGreaterEqual(comparaisons, PLANCHER_COMPARAISONS_JUGE)
        self.assertGreaterEqual(NOMBRE_ECHANTILLONS, PLANCHER_ECHANTILLONS)
        self.assertGreaterEqual(interieurs, PLANCHER_MAXIMA_INTERIEURS)

    def test_maximum_sur_contre_familles(self):
        tirage = random.Random(17)
        familles = (
            ("colineaire", _nuage_colineaire(6, 3)),
            ("colineaire_d20", _nuage_colineaire(5, 20)),
            ("deux_lignes", _nuage_deux_lignes(6, 3)),
            ("deux_lignes_d10", _nuage_deux_lignes(6, 10)),
            ("grappes", _nuage_grappes(tirage, 6, 4, 40)),
            ("duplique", _nuage_duplique(tirage, 6, 3, 12)),
            ("cocyclique", list(_CARRE_COCYCLIQUE)),
        )
        comparaisons = 0
        for nom, nuage in familles:
            effectif = len(nuage)
            for source, cible in combinations(range(effectif), 2):
                for ordre in (1, 2, 3):
                    niveau, instant = segment_maximum(nuage, source, cible, ordre)
                    juge, _nombre = _maximum_juge(nuage, source, cible, ordre)
                    comparaisons += 1
                    self.assertEqual(
                        niveau, juge, "desaccord sur la famille %s" % nom
                    )
                    self.assertEqual(
                        _valeur_geometrique(nuage, source, cible, ordre, instant),
                        niveau,
                        "temps non realisant sur la famille %s" % nom,
                    )
                    minorant = _maximum_echantillonne(nuage, source, cible, ordre, 512)
                    self.assertLessEqual(minorant, niveau)
        _compter("familles_comparaisons", comparaisons)
        _compter("familles_nombre", len(familles))
        self.assertGreaterEqual(len(familles), 7)

    def test_maximum_sur_nuages_rationnels(self):
        tirage = random.Random(23)
        comparaisons = 0
        for dimension in (2, 3, 5, 20):
            nuage = _nuage_rationnel(tirage, 5, dimension, 13, 3)
            for source, cible in combinations(range(5), 2):
                for ordre in (1, 2, 3):
                    niveau, instant = segment_maximum(nuage, source, cible, ordre)
                    juge, _nombre = _maximum_juge(nuage, source, cible, ordre)
                    comparaisons += 1
                    self.assertEqual(niveau, juge)
                    self.assertEqual(
                        _valeur_geometrique(nuage, source, cible, ordre, instant),
                        niveau,
                    )
                    entier = [
                        tuple(coordonnee * 3 for coordonnee in point) for point in nuage
                    ]
                    niveau_entier, _temps = segment_maximum(entier, source, cible, ordre)
                    self.assertEqual(niveau_entier, niveau * 9)
        _compter("rationnels_comparaisons", comparaisons)
        self.assertGreaterEqual(comparaisons, 100)

    def test_fixtures_gravees_du_segment(self):
        for nom, nuage, paire, ordre, niveau, instant in FIXTURES_SEGMENT:
            source, cible = paire
            obtenu, temps = segment_maximum(list(nuage), source, cible, ordre)
            self.assertEqual(obtenu, niveau, "fixture %s : niveau" % nom)
            self.assertEqual(temps, instant, "fixture %s : temps canonique" % nom)
            juge, _nombre = _maximum_juge(list(nuage), source, cible, ordre)
            self.assertEqual(juge, niveau, "fixture %s : juge" % nom)
            self.assertEqual(
                _valeur_geometrique(list(nuage), source, cible, ordre, temps),
                niveau,
                "fixture %s : realisation geometrique" % nom,
            )
            _compter("fixtures_segment")
        self.assertGreaterEqual(
            COMPTEURS.get("fixtures_segment", 0), PLANCHER_FIXTURES_SEGMENT
        )

    def test_equivariance_du_maximum_par_isometrie_entiere(self):
        tirage = random.Random(29)
        controles = 0
        for dimension in (2, 3, 5, 20):
            nuage = _nuage_entier_alea(tirage, 5, dimension, 20)
            decalage = tuple(tirage.randrange(-9, 10) for _ in range(dimension))
            translate = [
                tuple(coordonnee + pas for coordonnee, pas in zip(point, decalage))
                for point in nuage
            ]
            for source, cible in combinations(range(5), 2):
                for ordre in (1, 2, 3):
                    niveau, temps = segment_maximum(nuage, source, cible, ordre)
                    niveau_translate, temps_translate = segment_maximum(
                        translate, source, cible, ordre
                    )
                    self.assertEqual(niveau_translate, niveau)
                    self.assertEqual(temps_translate, temps)
                    homothetie = [
                        tuple(4 * coordonnee for coordonnee in point) for point in nuage
                    ]
                    niveau_homothetie, temps_homothetie = segment_maximum(
                        homothetie, source, cible, ordre
                    )
                    self.assertEqual(niveau_homothetie, niveau * 16)
                    self.assertEqual(temps_homothetie, temps)
                    controles += 1
        _compter("isometrie_controles", controles)
        self.assertGreaterEqual(controles, 100)


# ---------------------------------------------------------------------------
# (b) Ordre un : demi-distance et dendrogramme de l'arbre couvrant minimal
# ---------------------------------------------------------------------------


class TestBOrdreUn(unittest.TestCase):
    """A l'ordre 1 le moteur est EXACTEMENT le dendrogramme de l'ACM."""

    def test_poids_de_paire_seule_est_la_demi_distance(self):
        tirage = random.Random(31)
        controles = 0
        for dimension in (1, 2, 3, 5, 20, 50):
            for _ in range(3):
                nuage = _nuage_entier_alea(tirage, 2, dimension, 25)
                niveau, temps = segment_maximum(nuage, 0, 1, 1)
                attendu = _carre_distance(nuage[0], nuage[1]) * Fraction(1, 4)
                self.assertEqual(niveau, attendu)
                self.assertEqual(temps, Fraction(1, 2))
                tour = PointTower(nuage, 1)
                self.assertEqual(tour.cophenetic(1)[(0, 1)], attendu)
                controles += 1
        _compter("demi_distance_controles", controles)
        self.assertGreaterEqual(controles, 15)

    def test_liaison_simple_egale_le_dendrogramme_acm(self):
        tirage = random.Random(37)
        dimensions = (2, 3, 5, 20, 50)
        tirages = 0
        paires = 0
        for dimension in dimensions:
            for _ in range(3):
                nuage = _nuage_entier_alea(tirage, 8, dimension, 32)
                tour = PointTower(nuage, 1)
                obtenue = tour.cophenetic(1)
                reference = _ultrametrique_acm(nuage)
                self.assertEqual(
                    obtenue,
                    reference,
                    "ordre 1 : liaison simple differente du dendrogramme ACM (d=%d, nuage=%s)"
                    % (dimension, nuage),
                )
                tirages += 1
                paires += len(reference)
        for nom, nuage in (
            ("colineaire", _nuage_colineaire(6, 3)),
            ("deux_lignes", _nuage_deux_lignes(6, 4)),
            ("grappes", _nuage_grappes(tirage, 6, 3, 30)),
            ("duplique", _nuage_duplique(tirage, 6, 3, 10)),
            ("cocyclique", list(_CARRE_COCYCLIQUE)),
        ):
            tour = PointTower(nuage, 1)
            self.assertEqual(
                tour.cophenetic(1),
                _ultrametrique_acm(nuage),
                "ordre 1 : famille %s" % nom,
            )
            tirages += 1
            paires += len(tour.cophenetic(1))
        _compter("acm_tirages", tirages)
        _compter("acm_paires", paires)
        _compter("acm_dimensions", len(dimensions))
        self.assertGreaterEqual(tirages, PLANCHER_TIRAGES_ORDRE_UN)
        self.assertGreaterEqual(paires, PLANCHER_PAIRES_ORDRE_UN)
        self.assertGreaterEqual(len(dimensions), PLANCHER_DIMENSIONS_ORDRE_UN)

    def test_ordre_un_egale_la_projection_exacte(self):
        """L'ordre 1 n'est pas seulement un majorant : il est exact."""
        tirage = random.Random(41)
        controles = 0
        for dimension in (2, 3, 5):
            nuage = _nuage_entier_alea(tirage, 6, dimension, 14)
            tour = FullTower(nuage, 1)
            moteur = PointTower(nuage, 1)
            exacte = projected_ultrametric(tour, 1)
            rapport = compare_ultrametrics(exacte, moteur.cophenetic(1))
            self.assertEqual(rapport["violations"], [])
            self.assertEqual(rapport["strictly_above"], 0)
            self.assertGreater(rapport["equal"], 0)
            controles += 1
        _compter("ordre_un_exact_controles", controles)
        self.assertGreaterEqual(controles, 3)


# ---------------------------------------------------------------------------
# (c) Majoration certifiee de la projection exacte
# ---------------------------------------------------------------------------


class TestCMajorationCertifiee(unittest.TestCase):
    """Pour tout ordre, le moteur MAJORE l'ultrametrique exacte projetee."""

    def _confronter(self, nom, nuage, ordre_max, ordres, accumulateur):
        tour = FullTower(list(nuage), ordre_max)
        moteur = PointTower(list(nuage), ordre_max)
        for ordre in ordres:
            exacte = projected_ultrametric(tour, ordre)
            candidate = moteur.cophenetic(ordre)
            rapport = compare_ultrametrics(exacte, candidate)
            if rapport["violations"]:
                paire = rapport["violations"][0][0]
                minimal, paire_minimale = _reduire_violation(nuage, ordre, paire)
                self.fail(
                    "VIOLATION de la majoration sur %s (ordre %d) : nuage minimal %s,"
                    " paire %s, exact %s, moteur %s"
                    % (
                        nom,
                        ordre,
                        minimal,
                        paire_minimale,
                        rapport["violations"][0][1],
                        rapport["violations"][0][2],
                    )
                )
            accumulateur["egales"] += rapport["equal"]
            accumulateur["strictes"] += rapport["strictly_above"]
            accumulateur["paires"] += rapport["total"]
            accumulateur["cas"] += 1
            if rapport["ratio_max"] is not None:
                if (
                    accumulateur["ratio_max"] is None
                    or rapport["ratio_max"] > accumulateur["ratio_max"]
                ):
                    accumulateur["ratio_max"] = rapport["ratio_max"]

    def test_moteur_majore_la_projection_exacte(self):
        tirage = random.Random(43)
        accumulateur = {
            "egales": 0,
            "strictes": 0,
            "paires": 0,
            "cas": 0,
            "ratio_max": None,
        }
        dimensions = set()
        cas = (
            (7, 2, 3),
            (8, 2, 4),
            (9, 2, 4),
            (7, 3, 3),
            (9, 3, 4),
            (7, 5, 3),
            (8, 5, 2),
            (8, 20, 3),
        )
        for effectif, dimension, ordre_max in cas:
            nuage = _nuage_entier_alea(tirage, effectif, dimension, 4 * effectif)
            dimensions.add(dimension)
            self._confronter(
                "alea_n%d_d%d" % (effectif, dimension),
                nuage,
                ordre_max,
                tuple(range(2, ordre_max + 1)),
                accumulateur,
            )
        contre_familles = (
            ("cocyclique", list(_CARRE_COCYCLIQUE), 3, (2, 3)),
            ("colineaire_sept", _nuage_colineaire(7, 2), 3, (2, 3)),
            ("deux_lignes", _nuage_deux_lignes(6, 3), 3, (2, 3)),
            ("duplique", _nuage_duplique(tirage, 6, 3, 9), 3, (2, 3)),
            ("grappes", _nuage_grappes(tirage, 6, 5, 20), 2, (2,)),
        )
        for nom, nuage, ordre_max, ordres in contre_familles:
            dimensions.add(len(nuage[0]))
            self._confronter(nom, nuage, ordre_max, ordres, accumulateur)
        _compter("majoration_cas", accumulateur["cas"])
        _compter("majoration_paires", accumulateur["paires"])
        _compter("majoration_egales", accumulateur["egales"])
        _compter("majoration_strictes", accumulateur["strictes"])
        _compter("majoration_dimensions", len(dimensions))
        _mesurer(
            "majoration_part_egalite",
            "%d/%d" % (accumulateur["egales"], accumulateur["paires"]),
        )
        _mesurer("majoration_ratio_max", str(accumulateur["ratio_max"]))
        self.assertEqual(
            accumulateur["egales"] + accumulateur["strictes"], accumulateur["paires"]
        )
        self.assertGreaterEqual(accumulateur["cas"], PLANCHER_CAS_MAJORATION)
        self.assertGreaterEqual(accumulateur["paires"], PLANCHER_PAIRES_MAJORATION)
        self.assertGreaterEqual(len(dimensions), PLANCHER_DIMENSIONS_MAJORATION)
        self.assertGreaterEqual(
            accumulateur["strictes"], PLANCHER_MAJORATIONS_STRICTES
        )

    def test_fixtures_de_majoration_gravees(self):
        for fixture in FIXTURES_MAJORATION:
            nuage = list(fixture["nuage"])
            ordre = fixture["ordre"]
            tour = FullTower(nuage, ordre)
            moteur = PointTower(nuage, ordre)
            exacte = projected_ultrametric(tour, ordre)
            candidate = moteur.cophenetic(ordre)
            self.assertEqual(exacte, fixture["exact"], "fixture %s : exact" % fixture["nom"])
            self.assertEqual(
                candidate, fixture["moteur"], "fixture %s : moteur" % fixture["nom"]
            )
            rapport = compare_ultrametrics(exacte, candidate)
            self.assertEqual(rapport["violations"], [])
            self.assertEqual(
                rapport["strictly_above"],
                fixture["strictes"],
                "fixture %s : nombre de majorations strictes" % fixture["nom"],
            )
            self.assertEqual(
                rapport["ratio_max"],
                fixture["ratio_max"],
                "fixture %s : ratio maximal" % fixture["nom"],
            )
            _compter("fixtures_majoration")
        self.assertGreaterEqual(COMPTEURS.get("fixtures_majoration", 0), 5)

    def test_aucune_majoration_stricte_a_trois_observations(self):
        """Grille 3 x 3 exhaustive : a n = 3 le moteur est exact a l'ordre 2."""
        grille = [(abscisse, ordonnee) for abscisse in range(3) for ordonnee in range(3)]
        nuages = 0
        for nuage in combinations(grille, 3):
            tour = FullTower(list(nuage), 2)
            moteur = PointTower(list(nuage), 2)
            rapport = compare_ultrametrics(
                projected_ultrametric(tour, 2), moteur.cophenetic(2)
            )
            self.assertEqual(rapport["violations"], [])
            self.assertEqual(
                rapport["strictly_above"],
                0,
                "majoration stricte inattendue a n = 3 : %s" % (nuage,),
            )
            nuages += 1
        _compter("n3_nuages_exhaustifs", nuages)
        self.assertEqual(nuages, 84)

    def test_reduction_d_une_violation_simulee(self):
        """La reduction de violation est elle-meme testee, sur un cas simule.

        Aucune violation reelle n'existe dans le moteur courant : la porte de
        reduction est donc exercee sur un predicat de substitution, pour
        qu'elle ne soit pas du code mort le jour ou une violation apparait.
        """
        nuage = ((0, 0), (0, 1), (1, 3), (2, 2), (5, 5))
        paire = (0, 2)

        def presente(candidat, _ordre, paire_candidate):
            return len(candidat) >= 3 and paire_candidate[0] != paire_candidate[1]

        global _violation_presente
        original = _violation_presente
        try:
            _violation_presente = presente
            minimal, paire_minimale = _reduire_violation(nuage, 2, paire)
        finally:
            _violation_presente = original
        self.assertEqual(len(minimal), 3)
        self.assertIn(nuage[0], minimal)
        self.assertIn(nuage[2], minimal)
        self.assertEqual(minimal[paire_minimale[0]], nuage[0])
        self.assertEqual(minimal[paire_minimale[1]], nuage[2])
        _compter("reduction_controles")


# ---------------------------------------------------------------------------
# (d) Equivariance
# ---------------------------------------------------------------------------


class TestDEquivariance(unittest.TestCase):
    """Permutation, translation entiere, homothetie entiere."""

    def _invariants_sans_etiquette(self, tour):
        """Invariants de la tour qui ne dependent PAS de l'etiquetage.

        L'extraction est agnostique au schema de l'enregistrement canonique :
        pour chaque ordre, toute rubrique dont les entrees portent un champ
        `level` donne son multiensemble de niveaux. Un enrichissement du
        schema amont ne casse donc pas la porte d'equivariance.
        """
        enregistrement = tour.canonical_record()
        niveaux = {}
        for ordre in range(1, tour.effective + 1):
            bloc = enregistrement["orders"][str(ordre)]
            for rubrique, entrees in sorted(bloc.items()):
                if not isinstance(entrees, list):
                    continue
                if any(not isinstance(entree, dict) or "level" not in entree
                       for entree in entrees):
                    continue
                niveaux[(ordre, rubrique)] = sorted(
                    tuple(entree["level"]) for entree in entrees
                )
        if not niveaux:
            self.fail("aucune rubrique de niveaux dans l'enregistrement canonique")
        return (
            list(tour.levels),
            {ordre: tour.merge_levels(ordre) for ordre in range(1, tour.effective + 1)},
            tour.component_counts(),
            niveaux,
        )

    def test_permutation_des_observations(self):
        tirage = random.Random(47)
        cas = 0
        digests_egaux = 0
        for dimension in (2, 3, 5):
            for _ in range(2):
                effectif = 6
                base = _nuage_entier_alea(tirage, effectif, dimension, 20)
                permutation = list(range(effectif))
                tirage.shuffle(permutation)
                permute = [base[permutation[index]] for index in range(effectif)]
                tour_base = FullTower(base, 3)
                tour_permute = FullTower(permute, 3)
                self.assertEqual(
                    self._invariants_sans_etiquette(tour_base),
                    self._invariants_sans_etiquette(tour_permute),
                    "invariants sans etiquette non preserves par permutation",
                )
                if tour_base.digest() == tour_permute.digest():
                    digests_egaux += 1
                self.assertEqual(tour_base.digest(), FullTower(base, 3).digest())
                moteur_base = PointTower(base, 3)
                moteur_permute = PointTower(permute, 3)
                for ordre in (1, 2, 3):
                    self.assertEqual(
                        _transporter(
                            projected_ultrametric(tour_base, ordre), permutation
                        ),
                        projected_ultrametric(tour_permute, ordre),
                        "ultrametrique exacte non equivariante (ordre %d)" % ordre,
                    )
                    self.assertEqual(
                        _transporter(moteur_base.cophenetic(ordre), permutation),
                        moteur_permute.cophenetic(ordre),
                        "ultrametrique du moteur non equivariante (ordre %d)" % ordre,
                    )
                cas += 1
        _compter("permutation_cas", cas)
        _compter("permutation_digests_egaux", digests_egaux)
        _mesurer(
            "permutation_digest_invariant",
            "%d/%d (le digest est un invariant ETIQUETE : il change en general)"
            % (digests_egaux, cas),
        )
        self.assertGreaterEqual(cas, 6)

    def test_translation_entiere(self):
        tirage = random.Random(53)
        cas = 0
        for dimension in (2, 3, 20):
            base = _nuage_entier_alea(tirage, 6, dimension, 18)
            decalage = tuple(tirage.randrange(-11, 12) for _ in range(dimension))
            translate = [
                tuple(coordonnee + pas for coordonnee, pas in zip(point, decalage))
                for point in base
            ]
            tour_base = FullTower(base, 2)
            tour_translate = FullTower(translate, 2)
            self.assertEqual(tour_base.digest(), tour_translate.digest())
            moteur_base = PointTower(base, 2)
            moteur_translate = PointTower(translate, 2)
            self.assertEqual(moteur_base.digest(), moteur_translate.digest())
            for ordre in (1, 2):
                self.assertEqual(
                    projected_ultrametric(tour_base, ordre),
                    projected_ultrametric(tour_translate, ordre),
                )
                self.assertEqual(
                    moteur_base.cophenetic(ordre), moteur_translate.cophenetic(ordre)
                )
            self.assertEqual(
                entry_levels(rational_cloud(base), 2),
                entry_levels(rational_cloud(translate), 2),
            )
            cas += 1
        _compter("translation_cas", cas)
        self.assertGreaterEqual(cas, 3)

    def test_homothetie_entiere(self):
        tirage = random.Random(59)
        facteur = 3
        carre = facteur * facteur
        cas = 0
        for dimension in (2, 3, 5):
            base = _nuage_entier_alea(tirage, 6, dimension, 16)
            dilate = [
                tuple(facteur * coordonnee for coordonnee in point) for point in base
            ]
            tour_base = FullTower(base, 3)
            tour_dilate = FullTower(dilate, 3)
            self.assertEqual(
                [niveau * carre for niveau in tour_base.levels], list(tour_dilate.levels)
            )
            for ordre in (1, 2, 3):
                self.assertEqual(
                    [niveau * carre for niveau in tour_base.merge_levels(ordre)],
                    tour_dilate.merge_levels(ordre),
                )
                self.assertEqual(
                    _mettre_a_echelle(projected_ultrametric(tour_base, ordre), carre),
                    projected_ultrametric(tour_dilate, ordre),
                )
            moteur_base = PointTower(base, 3)
            moteur_dilate = PointTower(dilate, 3)
            for ordre in (1, 2, 3):
                self.assertEqual(
                    _mettre_a_echelle(moteur_base.cophenetic(ordre), carre),
                    moteur_dilate.cophenetic(ordre),
                )
            self.assertEqual(
                [
                    [niveau * carre for niveau in ligne]
                    for ligne in entry_levels(rational_cloud(base), 3)
                ],
                entry_levels(rational_cloud(dilate), 3),
            )
            cas += 1
        _compter("homothetie_cas", cas)
        self.assertGreaterEqual(cas, 3)


# ---------------------------------------------------------------------------
# (e) Degenerescences : resultat exact ou refus explicite, jamais autre chose
# ---------------------------------------------------------------------------


class TestEDegenerescences(unittest.TestCase):
    """Chaque entree degeneree a un comportement DEFINI et grave."""

    def test_observations_dupliquees(self):
        nuage = [(0, 0), (0, 0), (5, 0)]
        moteur = PointTower(nuage, 2)
        self.assertEqual(moteur.cophenetic(1)[(0, 1)], Fraction(0))
        self.assertEqual(moteur.cophenetic(2)[(0, 1)], Fraction(0))
        self.assertEqual(moteur.cophenetic(2)[(0, 2)], Fraction(25))
        tour = FullTower(nuage, 2)
        self.assertEqual(
            projected_ultrametric(tour, 2),
            {(0, 1): Fraction(0), (0, 2): Fraction(25), (1, 2): Fraction(25)},
        )
        self.assertEqual(tour.digest(), FullTower(nuage, 2).digest())
        _compter("degenerescences")

    def test_triple_position_identique(self):
        nuage = [(2, 7), (2, 7), (2, 7)]
        moteur = PointTower(nuage, 3)
        for ordre in (1, 2, 3):
            for paire, niveau in moteur.cophenetic(ordre).items():
                self.assertEqual(niveau, Fraction(0), "paire %s" % (paire,))
        tour = FullTower(nuage, 3)
        self.assertEqual(tour.levels, [Fraction(0)])
        _compter("degenerescences")

    def test_points_colineaires(self):
        nuage = _nuage_colineaire(5, 3)
        moteur = PointTower(nuage, 3)
        tour = FullTower(nuage, 3)
        for ordre in (2, 3):
            rapport = compare_ultrametrics(
                projected_ultrametric(tour, ordre), moteur.cophenetic(ordre)
            )
            self.assertEqual(rapport["violations"], [])
        self.assertEqual(
            moteur.cophenetic(1)[(0, 4)], Fraction(1, 4)
        )
        _compter("degenerescences")

    def test_points_cocycliques(self):
        nuage = list(_CARRE_COCYCLIQUE)
        tour = FullTower(nuage, 3)
        moteur = PointTower(nuage, 3)
        self.assertEqual(
            projected_ultrametric(tour, 3),
            {
                (0, 1): Fraction(8),
                (0, 2): Fraction(8),
                (0, 3): Fraction(8),
                (1, 2): Fraction(8),
                (1, 3): Fraction(8),
                (2, 3): Fraction(8),
            },
        )
        rapport = compare_ultrametrics(
            projected_ultrametric(tour, 3), moteur.cophenetic(3)
        )
        self.assertEqual(rapport["violations"], [])
        self.assertEqual(rapport["strictly_above"], 4)
        _compter("degenerescences")

    def test_six_points_cospheriques_en_dimension_cinq(self):
        """Plateau cospherique en grande dimension : simplexe regulier."""
        nuage = [tuple(6 if axe == indice else 0 for axe in range(5)) for indice in range(5)]
        nuage.append(tuple(0 for _ in range(5)))
        moteur = PointTower(nuage, 2)
        tour = FullTower(nuage, 2)
        rapport = compare_ultrametrics(
            projected_ultrametric(tour, 2), moteur.cophenetic(2)
        )
        self.assertEqual(rapport["violations"], [])
        _compter("degenerescences")

    def test_une_seule_observation(self):
        moteur = PointTower([(1, 2)], 1)
        self.assertEqual(moteur.cophenetic(1), {})
        self.assertEqual(moteur.count, 1)
        self.assertEqual(moteur.effective, 1)
        tour = FullTower([(1, 2)], 1)
        self.assertEqual(tour.levels, [Fraction(0)])
        self.assertEqual(projected_ultrametric(tour, 1), {})
        _compter("degenerescences")

    def test_deux_observations(self):
        nuage = [(0, 0), (2, 0)]
        moteur = PointTower(nuage, 2)
        self.assertEqual(moteur.cophenetic(1), {(0, 1): Fraction(1)})
        self.assertEqual(moteur.cophenetic(2), {(0, 1): Fraction(4)})
        tour = FullTower(nuage, 2)
        self.assertEqual(projected_ultrametric(tour, 2), {(0, 1): Fraction(4)})
        _compter("degenerescences")

    def test_ordre_egal_a_l_effectif(self):
        nuage = [(0, 0), (4, 0), (0, 3)]
        moteur = PointTower(nuage, 3)
        self.assertEqual(moteur.effective, 3)
        self.assertEqual(moteur.cophenetic(3)[(0, 1)], Fraction(25))
        tour = FullTower(nuage, 3)
        self.assertEqual(tour.effective, 3)
        # A l'ordre 3 le seul sommet de Gamma_3 nait au niveau 25/4 (rayon
        # carre circonscrit), mais la PROJECTION exige en plus l'entree des
        # deux observations : a_3(x_1) = a_3(x_2) = 25 fixe donc le niveau.
        self.assertEqual(
            projected_ultrametric(tour, 3),
            {(0, 1): Fraction(25), (0, 2): Fraction(25), (1, 2): Fraction(25)},
        )
        self.assertEqual(
            [ligne[2] for ligne in entry_levels(rational_cloud(nuage), 3)],
            [Fraction(16), Fraction(25), Fraction(25)],
        )
        _compter("degenerescences")

    def test_ordre_superieur_a_l_effectif_est_ramene(self):
        nuage = [(0, 0), (4, 0), (0, 3)]
        moteur = PointTower(nuage, 5)
        self.assertEqual(moteur.effective, 3)
        self.assertEqual(moteur.k_max, 5)
        self.assertEqual(moteur.cophenetic(3)[(0, 1)], Fraction(25))
        tour = FullTower(nuage, 5)
        self.assertEqual(tour.effective, 3)
        # Au-dela de l'effectif utile, la demande doit etre refusee et non
        # servie : le refus courant est une exception de recherche de cle.
        with self.assertRaises((KeyError, ValueError, IndexError)):
            moteur.cophenetic(5)
        _compter("degenerescences")

    def test_ordre_nul_ou_negatif(self):
        nuage = [(0, 0), (4, 0), (0, 3)]
        with self.assertRaises(ValueError):
            FullTower(nuage, 0)
        with self.assertRaises(ValueError):
            FullTower(nuage, -2)
        # `PointTower` NE refuse PAS `k_max = 0` : il construit une tour vide.
        # Le comportement est grave ici tel qu'il est, et l'ecart de refus
        # avec `FullTower` est rapporte comme defaut.
        vide = PointTower(nuage, 0)
        self.assertEqual(vide.effective, 0)
        self.assertEqual(vide.weights, {})
        with self.assertRaises((KeyError, ValueError)):
            vide.cophenetic(1)
        _compter("degenerescences")

    def test_ordre_hors_domaine_du_maximum_de_segment(self):
        nuage = [(0, 0), (4, 0), (0, 3)]
        # Ordre strictement superieur a l'effectif : refus par exception, pas
        # de valeur servie.
        with self.assertRaises((IndexError, ValueError)):
            segment_maximum(nuage, 0, 1, 5)
        # Ordre nul ou negatif : le moteur SERT une valeur (statistique
        # d'ordre lue par index negatif). Aucune exception, aucun refus : ce
        # test grave la situation courante, qui est un defaut rapporte.
        servi = segment_maximum(nuage, 0, 1, 0)
        self.assertEqual(len(servi), 2)
        self.assertEqual(servi[0], Fraction(25))
        _compter("degenerescences")

    def test_segment_degenere_source_egale_cible(self):
        nuage = [(0, 0), (4, 0), (0, 3)]
        niveau, temps = segment_maximum(nuage, 1, 1, 2)
        self.assertEqual(temps, Fraction(0))
        self.assertEqual(niveau, Fraction(16))
        table = entry_levels(rational_cloud(nuage), 2)
        self.assertEqual(niveau, table[1][1])
        _compter("degenerescences")

    def test_entrees_invalides_refusees(self):
        with self.assertRaises(ValueError):
            FullTower([], 1)
        with self.assertRaises(ValueError):
            PointTower([], 1)
        with self.assertRaises(ValueError):
            FullTower([(0, 0), (1, 1, 1)], 1)
        with self.assertRaises(ValueError):
            PointTower([(), ()], 1)
        _compter("degenerescences")

    def test_dimension_un(self):
        nuage = [(0,), (1,), (4,)]
        moteur = PointTower(nuage, 2)
        self.assertEqual(moteur.cophenetic(1)[(0, 1)], Fraction(1, 4))
        tour = FullTower(nuage, 2)
        rapport = compare_ultrametrics(
            projected_ultrametric(tour, 2), moteur.cophenetic(2)
        )
        self.assertEqual(rapport["violations"], [])
        _compter("degenerescences")

    def test_grande_dimension_petit_effectif(self):
        nuage = [
            tuple(1 if axe == indice else 0 for axe in range(60)) for indice in range(4)
        ]
        moteur = PointTower(nuage, 2)
        self.assertEqual(moteur.dimension, 60)
        for paire in combinations(range(4), 2):
            self.assertEqual(moteur.cophenetic(1)[paire], Fraction(1, 2))
        tour = FullTower(nuage, 2)
        rapport = compare_ultrametrics(
            projected_ultrametric(tour, 2), moteur.cophenetic(2)
        )
        self.assertEqual(rapport["violations"], [])
        _compter("degenerescences")


# ---------------------------------------------------------------------------
# (f) Planchers globaux : le vert par vacuite est refuse.
# ---------------------------------------------------------------------------


class TestZPlanchersCouverture(unittest.TestCase):
    """Verifie les planchers accumules par la suite complete."""

    _ATTENDUS = (
        ("juge_dimensions", PLANCHER_DIMENSIONS_JUGE),
        ("juge_tirages", PLANCHER_TIRAGES_JUGE),
        ("juge_comparaisons", PLANCHER_COMPARAISONS_JUGE),
        ("juge_maxima_interieurs", PLANCHER_MAXIMA_INTERIEURS),
        ("juge_echantillons_par_comparaison", PLANCHER_ECHANTILLONS),
        ("fixtures_segment", PLANCHER_FIXTURES_SEGMENT),
        ("acm_tirages", PLANCHER_TIRAGES_ORDRE_UN),
        ("acm_paires", PLANCHER_PAIRES_ORDRE_UN),
        ("acm_dimensions", PLANCHER_DIMENSIONS_ORDRE_UN),
        ("majoration_cas", PLANCHER_CAS_MAJORATION),
        ("majoration_paires", PLANCHER_PAIRES_MAJORATION),
        ("majoration_strictes", PLANCHER_MAJORATIONS_STRICTES),
        ("majoration_dimensions", PLANCHER_DIMENSIONS_MAJORATION),
        ("permutation_cas", PLANCHER_CAS_EQUIVARIANCE),
        ("degenerescences", PLANCHER_DEGENERESCENCES),
    )

    def test_planchers_atteints(self):
        manquants = [cle for cle, _seuil in self._ATTENDUS if cle not in COMPTEURS]
        if manquants:
            self.skipTest(
                "suite partielle : compteurs absents %s" % ",".join(sorted(manquants))
            )
        for cle, seuil in self._ATTENDUS:
            self.assertGreaterEqual(
                COMPTEURS[cle], seuil, "plancher non atteint : %s" % cle
            )


if __name__ == "__main__":
    _resultat = unittest.main(verbosity=2, exit=False)
    print("")
    print("compteurs de couverture :")
    for _cle in sorted(COMPTEURS):
        print("  %-40s %s" % (_cle, COMPTEURS[_cle]))
    print("mesures :")
    for _cle in sorted(MESURES):
        print("  %-40s %s" % (_cle, MESURES[_cle]))
    sys.exit(0 if _resultat.result.wasSuccessful() else 1)
