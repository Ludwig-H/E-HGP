# Contrelecture statistique de la campagne gaussienne

27 septembre 2026. Lecture seule des sources et captures initiales ; cette
note et `metric_audit/` sont ajoutés **après** clôture du benchmark. Aucun
processus géométrique relancé, aucune source moteur modifiée, aucun GPU/GCP.
Cadre exploratoire hors registre ; aucune qualification FULL/G4 héritée.

## Verdict et périmètre rejoué

Pas de défaut matériel trouvé dans les agrégations ou le raccord des métriques
contrôlées. La capture close contient 48 scènes, 96 exports, 1 920 lignes et
zéro échec. Son SHA256 est
`8f9aba99396b6341f7a6872956e66f50a7776cc1535d9a3f9137907d2945157a`.

Le [script indépendant du runner](metric_audit/replay.py) a rejoué 720 lignes
depuis les labels individuels, en normal et en `-O` : **PASS**, 97 767
comparaisons de champs sans divergence. Les deux reçus sont identiques à
l'octet : [normal](metric_audit/receipt.json),
[optimisé](metric_audit/receipt_optimized.json), SHA256 commun
`12045f807af03a434c16c61dd81d455d06ebc99a0507806d8821dc7588bbad84`.
Ce compte de comparaisons n'est pas un nombre d'expériences indépendantes.

L'échantillon était fixé avant lecture des scores : une graine alternée dans
chacune des 12 cellules sphériques G×δ et des six cellules stress régime×δ,
soit 18 scènes. Pour chacune : K5/10, les quatre tailles minimales, HGP et
HDBSCAN commun expZ1/2, HDBSCAN standard expZ1. Les identifiants exacts sont
dans le reçu d'audit ; aucun cas n'est choisi selon son score.

L'ARI est recalculé par contingences et fractions rationnelles, le NMI par
comptes/entropies, les couvertures, communautés exactement retrouvées et
précisions/rappels par ensembles de points. L'appariement reconstruit sa
matrice indépendamment, mais utilise le **même solveur hongrois SciPy** ;
il n'est pas une deuxième implémentation de ce solveur. Les 720 cellules
d'agrégation sont recalculées depuis toutes les 1 920 lignes. La grille
unique prescrite et les métadonnées des scènes sont contrôlées.

En complément, les fichiers publics `rows.json`, `scores.csv`,
`aggregates.csv`, `TABLES.md`, `primary.svg` et le hash de raccord du reçu
public ont été comparés aux sorties du formateur gelé : concordance.
Les hiérarchies/puretés ne sont pas toutes reconstruites indépendamment ici ;
le rejeu de labels couvre 720 lignes, pas les 1 920. Les entrées et gros
payloads privés restent nécessaires : ce n'est pas une archive autonome
permettant de régénérer la géométrie depuis les seuls tableaux publics.

## Raccord qualification → campagne

Les 17 sources communes portent les mêmes hashes dans la qualification,
la campagne et les fichiers relus. `report.py` est propre à la campagne ;
`build_native.py` est propre à la qualification, avec son hash hérité vérifié.
Le binaire exporteur est identique (`897a715a…`), de même que l'EOM hérité
(`c7121c85…`), la provenance sklearn et ses quatre fichiers épinglés.
Le reçu du build est inchangé. Les neuf commandes prévues, leurs retours zéro
et les 18 hashes stdout/stderr concordent. Reçu de qualification :
`c9c6a0b820456b430cc5ce9bc7a4cb0e6a93380a684ff3409cf2d04496fc696d`.
Ce raccord explicite complète le contrôle `status=passed` du formateur ;
aucune modification de ce dernier n'a été nécessaire.

## Lecture principale, sans choisir le meilleur paramètre

K5, taille20, expZ1. ARI où chaque abstention devient un singleton distinct ;
moyennes descriptives par cellule ci-dessous. HDB désigne **EOM commun**.
Les victoires/défaites sont appariées par scène, tolérance de lecture 1e-12.

| Régime | δ | Scènes | ARI singletons HGP / HDB | Couverture HGP / HDB | Victoires / égalités / défaites HGP |
|---|---:|---:|---:|---:|---:|
| Sphérique, quatre G | 8 | 12 | 0.999682 / 0.998912 | 100.00 / 99.92 % | 4 / 8 / 0 |
| Sphérique, quatre G | 4 | 12 | 0.592945 / 0.273485 | 82.44 / 73.33 % | 11 / 0 / 1 |
| Sphérique, quatre G | 2 | 12 | 0.016500 / 0.013831 | 65.16 / 40.60 % | 6 / 0 / 6 |
| Anisotrope G8 | 8 | 2 | 0.993323 / 0.981847 | 99.75 / 98.50 % | 2 / 0 / 0 |
| Anisotrope G8 | 4 | 2 | 0.198974 / 0.285861 | 95.04 / 70.25 % | 1 / 0 / 1 |
| Anisotrope G8 | 2 | 2 | 0.043112 / 0.016302 | 53.79 / 49.83 % | 2 / 0 / 0 |
| Déséquilibré G8 | 8 | 2 | 1.000000 / 0.998382 | 100.00 / 99.79 % | 2 / 0 / 0 |
| Déséquilibré G8 | 4 | 2 | 0.567960 / 0.276158 | 86.04 / 89.71 % | 2 / 0 / 0 |
| Déséquilibré G8 | 2 | 2 | 0.093624 / 0.066640 | 62.71 / 47.33 % | 1 / 0 / 1 |

L'avantage moyen sur les sphériques vient principalement de δ4 ; δ8 est
presque parfait pour les deux, δ2 reste faible pour les deux. Il n'y a pas
de supériorité universelle. Sur les six anisotropes, l'ARI singletons moyen
HGP est 0.411803, contre 0.428003 pour HDB commun, malgré cinq victoires sur
six : compter seulement les victoires masquerait l'amplitude de la défaite.

Le cas anisotrope δ4 illustre aussi le risque d'ARI tous points : celui-ci
donne HGP 0.196989 contre HDB 0.185647, donc l'ordre **inverse** de l'ARI
singletons. Le F1 macro apparié est 0.193994 contre 0.441022. La couverture
supérieure de HGP n'est donc pas à elle seule un meilleur regroupement.

## HDBSCAN standard et commun ne sont pas interchangeables

La version binaire conservée reproduit les labels standards dans les
384/384 configurations expZ1. Après atomisation des plateaux, 217/384
partitions diffèrent. Les deux sorties sont conservées ; l'écart ne doit
pas être qualifié de toujours négligeable.

À `anisotropic_g8_d4_s2`, K5/taille20/expZ1, HDB commun trouve huit clusters,
ARI tous points 0.304886, ARI singletons 0.500958 et couverture 61.58 %.
Le standard trouve deux clusters, respectivement 0.066292, 0.067835 et
85.67 %. HGP trouve deux clusters, ARI singletons 0.075568, couverture
98.67 %. Ici, l'atomisation **améliore HDB**, non l'inverse. Sur les deux
graines anisotropes δ4, le standard a une moyenne ARI tous points 0.066047
et ARI singletons 0.068945. Le choix préfixé d'un EOM atomique commun est
cohérent pour comparer les arbres, mais ces résultats ne peuvent être
présentés comme les seuls labels HDBSCAN standards.

## Condensation et limites d'interprétation

- Les mêmes K self-inclus, tailles minimales, masses unitaires, racine exclue
  et epsilon0 sont utilisés. ExpZ2 modifie EOM pour les **deux** méthodes ;
  ce n'est pas HDBSCAN standard. Pas de sélection de paramètre par la vérité.
- Le meilleur F1 par branche est un oracle supervisé, non une extraction
  utilisable sans labels. Ses branches peuvent se recouvrir ou coïncider ;
  elles ne sont pas nécessairement une coupe ou une sélection EOM commune.
  Sur l'anisotrope δ4, F1 de branche condensée HGP 0.806867 contre 0.675031
  pour HDB, mais F1 des labels HGP inférieur : bon arbre ne suffit pas.
- Les ensembles de naissance condensés incluent les sorties propres et
  descendantes. Ce ne sont pas les coupes vivantes datées. La compression
  conserve 1 200 sorties individuelles, pas la généalogie complète des
  sous-branches supprimées ; ne pas prétendre conserver toutes les distances
  cophenétiques ni recalculer une « pureté améliorée » en retirant le bruit.
- À n=1 200 fixé, augmenter G diminue l'effectif par classe de 600 à 75 et
  change la configuration des moyennes : ce n'est pas un effet G isolé.
  Taille100 est incompatible avec les classes de 75 points à G16 et les
  quatre classes minoritaires de 60 points dans le stress 4:1. Elles restent
  dans l'évaluation ; leur élimination n'est pas un bug géométrique démontré.
- Les communautés vraies sont les composantes génératrices, pas forcément
  les modes de densité. Le MAP à paramètres connus est une difficulté
  empirique à modèle connu, pas un concurrent de clustering, ni une borne
  supérieure d'ARI. L'exactitude d'une communauté entière exige zéro point
  ajouté, manquant ou rejeté : un petit défaut suffit à faire perdre ce score.
- Les agrégats réunissent les graines development et evaluation ; ils ne
  sont pas des résultats held-out seuls. Trois graines principales/deux de
  stress et les séparations appariées ne prouvent pas une supériorité générale.
  Les IDs sont groupés par classe génératrice et communs aux méthodes ; ce
  lot n'ajoute aucun test global de permutation des 48 scènes.
- ARI/NMI tous points regroupent les labels -1 ; ARI des seuls classifiés
  peut favoriser l'abstention. Publier couverture et ARI singletons en regard.
  Les temps natifs 1..K et les fits HDB avec contrôles ne constituent pas un
  protocole de vitesse équitable ; les lignes répétées par m/z ne sont pas
  de nouvelles répétitions géométriques. Aucune extrapolation LiDAR/G4.

## Rejeu sans géométrie

Depuis ce dossier, avec les captures privées présentes et une sortie neuve :

```sh
OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 -B metric_audit/replay.py \
  --run /workspaces/E-HGP/build/v9-gaussian-clustering-benchmark-20260927-r1 \
  --checks /workspaces/E-HGP/build/v9-gaussian-clustering-checks-20260927-r1 \
  --output /CHEMIN/NEUF/audit.json
```

Ajouter `-O` et choisir une autre sortie pour le mode optimisé. Le script
refuse l'écrasement ; il n'appelle ni l'exporteur natif ni un entraînement.
