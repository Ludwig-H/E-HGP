# Condensation : ne pas oublier les départs de points

Audit CPU du 30 septembre 2026, sources `8bb4618e5`, moteur inchangé,
`public_status=not_claimed`. GCP non utilisé. Cette preuve porte sur la
tête qui transforme une hiérarchie de points valide en clusters, non sur
la construction FULL, sa précision, sa vitesse ou sa qualité statistique.

## Résultat et cause

La tête actuelle teste `min_cluster_size` lors des divisions entre enfants
géométriques. Elle ne le teste pas lorsqu'un point directement attaché
quitte un cluster. Une branche peut donc continuer avec une masse devenue
trop petite. Elle reçoit une stabilité EOM excessive, ce qui peut changer
les clusters retenus, **même avec la racine globale exclue et mcs=5**.

Dans la hiérarchie implicite sur les points, les départs font aussi partie
des divisions. Dès qu'une cohorte de départs laisse moins de mcs unités,
les points restants doivent quitter cette branche au même niveau.
La hiérarchie spatiale/FULL ne doit pas être modifiée pour réparer la tête.

## Contre-exemple exact

A porte trois points admis aux rayons carrés β=1,4,9 ; B en porte deux
admis à β=16. A et B fusionnent en R à β=25. Tous les poids valent un.
À z=1, λ=1/√β. Après le départ à β=4, A ne garde qu'un point : pour mcs2,
il faut le faire sortir à λ=1/2, non attendre λ=1.

| Stabilité à z1/mcs2 | Code actuel | Condensation des points |
| --- | ---: | ---: |
| A | 37/30 | 11/15 |
| B | 1/10 | 1/10 |
| R, naissance λ0 | 1 | 1 |
| A+B | 4/3 | 5/6 |

Avec une racine sélectionnable, EOM choisit donc A/B au lieu de R.
Pour enlever cette condition, ajouter C, deux points à β100, et une
racine globale à β1600. R naît alors à λ1/40 et sa stabilité vaut 7/8 :
la tête choisit A/B/C, la condensation correcte R/C, racine globale exclue.
Répéter chaque observation trois fois comme ID API distinct, puis prendre
mcs5, donne le même renversement sur 21 points. Ce ne sont pas des
coordonnées dupliquées nouvellement acceptées par le moteur.

À z2, la stabilité de A est également incorrecte ; ces cas gardent néanmoins
la même sélection. Ils ne démontrent pas que z2 serait généralement protégé.

## Trois contre-vérifications

1. [Paquet natif clos](native/README.md) : le vrai `head.cpp` et le validateur
   sont compilés depuis sept blobs Git privés exacts. Six invocations
   normal/UBSan, 48 lignes, 24 configurations distinctes par mode : seize
   concordantes, huit désaccords et quatre renversements EOM. Douze jugements
   Fraction normal/−O relisent ces mêmes captures ; aucun diagnostic UBSan.
2. [HDBSCAN réellement appelé](sklearn/sklearn_capture/receipt.json) :
   sklearn1.9.1, 16 configurations × normal/−O = 32 `fit_predict`, plus
   seize appels de préflight séparés. Il retrouve les décisions correctes.
   `min_samples=1`, métrique pré-calculée, EOM, mêmes mcs et choix de racine.
   La matrice ultramétrique réalise exactement les mêmes coupes de points ;
   pour z2, la matrice est carrée afin de conserver λ=β⁻¹. Ce n'est pas une
   comparaison de méthodes sur un nuage 3D ni une mesure d'ARI.
3. [Oracle indépendant par coupes strictes](sklearn/judge_sklearn_events.py) :
   retirer simultanément les arêtes à chaque distance, examiner les masses
   des composantes, puis intégrer EOM en Fraction. Les seize configurations
   concordent avec les vrais résultats sklearn, en normal et −O, sans
   réexécuter sklearn ou le C++. Il ne dépend pas de l'oracle natif.

La définition des paramètres de référence est celle de
[sklearn HDBSCAN](https://scikit-learn.org/stable/modules/generated/sklearn.cluster.HDBSCAN.html).
Dans cette comparaison de tête, `min_samples=1` annule les rayons cœur ;
il ne prétend pas remplacer un appariement statistique K5 sur données 3D.

## Portée et correction demandée

Les arbres passent `validate(PointDendrogram)` ; la réalisation de ces
contre-exemples précis par un catalogue HGP 3D n'est pas établie ici.
Le défaut réfute la généralité de la tête/API, pas les sorties FULL du
catalogue ni les scores A/C historiques. Leur impact nécessite un rejeu.

La porte actuelle `mhgp10_head_condensation_vs_sklearn` compare 360 cas
sur trente nuages, K1/2, mcs5/15/40, EOM/feuilles. Elle utilise l'entrée
cœur de l'atteignabilité mutuelle : une attache par feuille, sans dates
différées. Elle ne couvre donc pas ce défaut de l'entrée cover.

Réparer par cohortes de rang exact, jamais par point/ordre d'ID ou par
regroupement approximatif des λ double. Maintenir la masse active, clore
au seuil, ne pas repayer les départs antérieurs. Coordonner les départs
avec les divisions spatiales à même date. Préserver les continuations
FULL ; juger coûts de tri, mémoire et travail total avant optimisation.
Ajouter ces cas et des permutations d'IDs aux portes, puis rejouer le
binaire réellement intégré avant de reprendre la comparaison statistique.

## Lecture et preuves

`python3 -B verify.py` et `python3 -B -O verify.py` relisent les archives.
Le lecteur vérifie les empreintes avant d'importer un oracle. Il ne lance
aucun calcul HGP/sklearn, compilateur ou appel GCP. Le manifeste racine
couvre aussi les deux sous-paquets et leur documentation.

Les binaires, bibliothèques système et l'environnement sklearn complet ne
sont pas embarqués : archives d'exécutions, pas qualification LIVE.
Six pins sklearn ciblés restent distincts d'une clôture de toutes ses
dépendances. Les deux préflights outil ont des sorties tronquées et ne
remplacent pas les captures complètes. L'erreur de préparation du README
natif reste conservée. Aucun chrono FULL/G4, gain de croissance ou profil
large nouvellement qualifié.
