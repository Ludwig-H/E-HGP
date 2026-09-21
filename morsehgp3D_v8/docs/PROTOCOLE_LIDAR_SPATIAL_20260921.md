# Passage à l'échelle LiDAR : scène, moitiés et quarts

Protocole demandé par l'utilisateur le 21 septembre 2026. Il remplace
les préfixes sous-échantillonnés comme expérience principale de croissance
LiDAR. La préférence numérique précisée ensuite est le float32 sans perte,
avec grille optionnelle de1mm par défaut. Voir la
[note de précision](PRECISION_FLOAT32_ET_GRILLE_20260921.md).
Le moteur historique reste CPU/u16, `not_claimed` ; ce changement d'entrée
ne qualifie aucun nouveau moteur ni contrat G4.

## Une même scène, sans raréfier ses points

Partir d'un scan brut complet, dans **son propre repère capteur**. Le
premier plan vertical est `x=0`, le second `y=0` : ils sont perpendiculaires
et passent tous deux par l'origine du capteur. Conserver sept objets :

- la scène entière ;
- les deux moitiés `x<0` et `x>=0` ;
- leurs quatre quarts, obtenus par `y<0` et `y>=0`.

Tous les morceaux sont mesurés, pas seulement le plus favorable. Aucun
tirage, préfixe hashé, plafond de points, rééquilibrage des effectifs ou
adaptation de la résolution. Les tailles sont celles de la scène : on ne
les force pas à 8k/16k/32k. Les tests synthétiques à ces tailles et les
mesures historiques50k restent distincts. La décision ultérieure du même
jour fixe le [contrat principal](CONTRAT_TRAMES_SEMANTICKITTI_20260921.md)
sur une trame entière, pas sur l'un de ses morceaux.

Les préparations historiques des scans 100 et 200 étaient recalées dans
le repère du scan 0. Les couper à `x=y=0` ne passerait donc pas par leur
capteur. Le nouveau préparateur repart des `.bin` bruts, sans poses,
calibration, recentrage ni rotation dépendant du morceau.

## Profil actif et historique quantifié

Avec `prepare_lidar_precision.py`, le défaut float32 conserve exactement
XYZ et coupe selon les signes bruts x/y. L'option `--profile grid`
applique une grille isotrope, `--precision-mm` valant1 par défaut, puis
une translation entière commune aux sept objets pour l'encodage u32.
Les plans capteur suivent cette translation ; jamais de recentrage par
morceau. Les changements de côté dus à la grille sont publiés.
Correspondances, déduplication globale et restrictions conservent le
protocole ci-dessous. Ces fichiers ne sont **pas compatibles** avec les
sondes u16 : le raccord moteur fin reste à développer.

Le reste de cette section documente le profil historique20mm, afin de
conserver l'interprétation exacte des reçus déjà clos.

Le moteur actuel traite les **sites distincts u16**, pas le multiensemble
des retours float32. Une grille isotrope fixe de 2 cm est appliquée une
seule fois à la scène : `q=floor(50*x + 32768 + 0.5)`. La décision est
calculée avec des entiers à partir du rationnel exact de la valeur float32.
Aucune saturation, perturbation ou adaptation des axes n'est autorisée.
Une entrée non finie ou hors de la grille est refusée.

La fusion des coordonnées identiques se fait globalement, avant découpe.
Les plans passent alors par `qx=32768` et `qy=32768`, image exacte de
l'origine capteur sur cette grille. Les sites sur un plan vont du côté
non négatif. Les sept objets sont donc des restrictions disjointes et
exhaustives du **même nuage quantifié**, à résolution inchangée.

Cette convention n'affirme pas une identité avec les quarts du nuage brut :
la quantification peut déplacer un retour proche du plan, et fusionner
deux retours situés de part et d'autre. Publier ces deux phénomènes.
Découper d'abord les retours puis dédupliquer chaque morceau répliquerait
certains sites entre morceaux ; ce n'est pas le protocole retenu.

Conserver tous les retours dans la correspondance `raw_return_id ->
full_site_id`, les coordonnées originales par leur fichier source haché,
et `local_site_id -> full_site_id` pour chaque morceau. L'ordre local est
la restriction de l'ordre lexicographique global. Les égalités géométriques
et les départages d'IDs restent ainsi interprétables.

## Ce qu'il faut mesurer

Chaque morceau est une entrée autonome : reconstruire son index, ses
candidats et son census avec **ses seuls sites**. Ne pas filtrer les
sorties de la scène entière et ne pas employer de témoins extérieurs.
Les sorties des morceaux ne sont pas censées former la sortie du nuage
entier : les profondeurs et les coquilles peuvent changer à la coupe.

Pour chaque configuration identique (version, K, s, workers, options),
conserver les sept coûts et les six relations parent/enfant. Pour un poste
de travail positif W, rapporter les rapports réels `r=Nparent/Nenfant`,
`Wparent/Wenfant`, le seuil quadratique `r²` et l'exposant empirique
`log(Wparent/Wenfant)/log(r)`. Ne pas remplacer r par 2. Une taille égale,
un morceau vide ou un coût nul rend l'exposant non estimable ; conserver
le cas au lieu de le supprimer. Un exposant sur trois échelles d'une scène
est un diagnostic mesuré, pas une borne asymptotique démontrée.

Publier séparément la somme des quatre coûts, celle des deux coûts et
le coût du nuage complet. C'est l'effet du découpage, pas un exposant en n.
Ne jamais trier les quarts par effectif pour inventer une série de croissance.
Comparer s=8/10/12 à entrées identiques ; distinguer travail géométrique,
préparations, mémoire, sorties et temps mur. Pour les chronos, éviter les
campagnes concurrentes et faire des répétitions appariées.

## Statut et conservation des preuves historiques20mm

Le préparateur est [prepare_lidar_spatial.py](../bench/prepare_lidar_spatial.py).
Il produit un manifeste haché, les sept nuages, leurs correspondances et
un lecteur qui reconstruit les partitions depuis la source brute. Aucun
répertoire existant n'est remplacé. Les lecteurs historiques des tranches
31–34 ne sont pas adaptés à ces nouveaux fichiers ; ne pas maquiller leurs
noms, tailles ou commandes pour les faire passer.

Exécution unitaire, avec un répertoire de sortie neuf :

```bash
python morsehgp3D_v8/bench/prepare_lidar_spatial.py prepare --input chemin/scan.bin --output chemin/partition_neuve
python morsehgp3D_v8/bench/prepare_lidar_spatial.py read --path chemin/partition_neuve
```

Le [lanceur de qualification](../bench/run_lidar_spatial_checks.py) prend
`--output` et `--inputs` (les chemins bruts explicites). Il capture les
tests en Python normal et `-O`, puis la préparation et ses deux relectures
pour chaque scène, avec sources et fichiers hachés avant/après. Il réutilise
explicitement le collecteur de processus P0, pas ses verdicts géométriques.
Une interruption annule le groupe de la commande possédée et attend sa fin ;
il n'y a pas de plafond sur la durée d'un calcul normal.

Les anciens reçus restent immuables et utiles aux comparaisons de versions
sur une entrée identique. Leurs ratios 8k/16k/32k décrivent un changement
de densité par sous-échantillonnage ; ils ne sont **pas** les résultats de
cette nouvelle expérience spatiale. Le
[nouveau raccord de mesures](Q34_MESURES_SPATIALES_20260921.md) consomme
les sept fichiers entiers et vérifie leurs commandes et hashes. La
campagne scan0/K5/s8/W4 est close ; les six rapports, favorables ou non,
sont publiés dans les [reçus moteur](../receipts/q34_spatial_20260921/README.md).

Préparations des scans0/100/200 maintenant closes :21nuages,11 tests
normal/−O,11 commandes et78 artefacts vérifiés. Les effectifs et toutes
les différences brut/quantifié figurent dans les
[reçus spatiaux](../receipts/lidar_spatial_20260921/README.md).
