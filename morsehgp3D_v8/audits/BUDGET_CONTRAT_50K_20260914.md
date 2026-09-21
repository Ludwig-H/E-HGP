# Budget du contrat 50k : ce que chaque poste peut coûter

14 septembre 2026, après **85015a8c**. Auditeur indépendant B.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

> **Statut au 21 septembre 2026 (auditeur B)** : note historique (85015a8c). Le budget par poste n'a pas été réactualisé après le critère du 21 septembre (croissance mesurée sur les scans SemanticKITTI plutôt que borne uniforme préalable) ; aucun contrat 50k n'est acquis et les chiffres ci-dessous ne valent pas pour les sources actuelles.

Aucun document v8 ne chiffre le travail admissible par poste pour « toute
la tour K=1..10 à 50 000 points sous une seconde sur G4 », ni le plancher
d'écriture de la sortie. Cette note le fait à partir des seuls chiffres
publiés (v7 : `CONTRATS_ET_MESURES.md` §2 ; v8 : reçus des tranches ; front
pur : [REGIME_WSPD_20260914.md](REGIME_WSPD_20260914.md)). Ce sont des
ordres de grandeur pour orienter les tranches, pas des prévisions
matérielles ; un budget se vérifie ensuite poste par poste sur G4.

## 1. Ce que 1 s et 100 ms autorisent

Convention : « équivalent séquentiel » = temps mural × nombre de voies
effectivement utilisées, à mise à l'échelle parfaite. Sur 48 cœurs CPU le
budget d'une seconde vaut 48 s de travail séquentiel ; sur un GPU, un
kernel plat à 10⁴ threads ne dispense pas de compter le travail total.

| Poste | Volume à 50k (source) | Budget par unité à 1 s, 48 voies | À 100 ms |
| --- | ---: | ---: | ---: |
| Rectangles du front pur (s=8 v4 ; 34 279 860 mesurés à 50k en emprise u16 pleine) | ≈ 34 M | ≈ 1,4 µs par rectangle | ≈ 140 ns |
| Visites de témoins du front v7 | 4,95·10⁹ | ≈ 10 ns par visite | ≈ 1 ns |
| Essais de supports MEB du constructeur FULL v7 | 3,90·10⁹ | ≈ 12 ns par essai | ≈ 1,2 ns |
| Nœuds FULL retenus | 27,3 M | ≈ 1,8 µs par nœud | ≈ 180 ns |

Le chiffre mesuré qui compte est le temps CPU réellement consommé : la
génération amont v7 du run 50k du 6 septembre (GNU time, `n50000_k10.stderr`)
a coûté **682 s de temps utilisateur pour 21 s murales**, contre 48 s
séquentiels disponibles pour toute la tour ; le constructeur FULL y ajoute
390 s en mono. À mise à l'échelle parfaite, l'amont seul doit donc faire
**quatorze fois moins de travail**, et non seulement mieux se paralléliser.
Le front pur seul coûte environ 1,5 s séquentiels à 50k en emprise u16
pleine (probe v4, quatre mesures entre 1,53 et 1,65 s), soit trois pour
cent du budget avant tout témoin.

## 2. Plancher de la sortie

La tour v7 retient 27 273 218 nœuds FULL. Au format v7 (64 octets par
nœud d'après la ligne de configuration du reçu du 6 septembre), l'écriture
seule représente 1,75 Go ; un réfutateur l'a mesurée sur cet hôte à 62,7 ms
en mono-fil (27,9 Go/s) et 61,6 ms à huit fils. **Le repli 100 ms laisse
donc moins de 40 ms à tout le reste de la tour** avec la représentation v7 ;
il exige un nœud plus compact, ou une sortie implicite déclarée comme
contrat distinct (question d'ouverture n°5 du journal). Décider du format
de nœud cible avant de réclamer 100 ms ; mesurer ce plancher sur G4.

## 3. Où les tranches P0 pèsent, par famille

Sur les familles du plan de test, le front WSPD n'a aucun facteur d'au
moins 1 024 sites et 75 à 94 % des rectangles ont deux facteurs d'au plus
sept sites (mesure v4, s=8). Le seul chronométrage isolé des histogrammes
locaux sur une vraie WSPD est l'uniforme 8k v7 : 93,8 ms pour 754 686
rectangles survivants à facteurs d'au plus sept sites, contre un front de
28,9 s (reçu de séparation du 6 septembre, `WSPD_Q2_Q3_Q4.md` §6).
Extrapolé en n², ce poste vaudrait environ 3,7 s séquentiels à
50k, soit moins d'un pour cent de la tour v7 mais huit pour cent du
budget d'une seconde à 48 voies. Sur terrain et huit amas, la masse de
paires se concentre sur des facteurs de 64 à 700 sites : le poste y est
plus lourd, mais son volume n'est publié qu'à petite taille (le compteur
`p_factor` de `generate.hpp` v7, somme des auto-produits par lane, figure
dans les reçus à n ≤ 1 000, pas dans le reçu 50k ni pour ces familles aux
tailles d'intérêt). Il ne domine que sur les nappes et les amas
séparés par au moins s fois leur diamètre (le « deux amas » v7, rectangle
racine construit à la main, 31 s à 32k), régime absent des familles de
mesure. Le poids réel de P0 par famille reste donc **inconnu tant qu'un
pilote WSPD v8 ne somme pas sur tous les rectangles**.

## 4. Trois formulations à corriger dans les documents de tête

- « Croissance sous-quadratique dans ces familles » (README, PASSATION,
  ETAT_COURANT) : le nombre de candidates des recettes `sheet_full` et
  `grid` est borné linéairement par construction (`M ≤ m(2h+1)²`,
  `M ≤ |A|(2h²+6h+1)`), et le temps du composant suit ce résidu à environ
  0,6 µs par candidate. La linéarité en n est un théorème de la fixture,
  pas une mesure du moteur : sur une WSPD, la quantité que P0 vise croît
  quadratiquement (Σ(|A|²+|B|²) ≥ n(n−1) sur toute WSPD, par
  l'inégalité arithmético-géométrique).
- « Baisse de 4 à 10 % » des bornes préparées : le bras Pairwise inchangé
  varie de −4,2 % à +9,6 % entre les mêmes révisions ; le gain n'est pas
  séparable du bruit, ce que le reçu dit mais pas les documents de tête.
  Le résultat acquis est « travail discret identique ».
- Fixtures à coordonnées dans une bande étroite (1 000 à 1 300, et
  60 000) : profondeur d'index observée 16 dans le reçu 50k, sur 48
  possibles. Le nombre de rectangles du front pur ne change presque pas
  entre emprise par défaut et emprise u16 pleine (3 435 133 contre
  3 388 617 à 8k), mais les profondeurs d'index et les clés le font :
  mesurer les échelles en emprise pleine, comme la v7 le faisait.

## 5. Ordre proposé pour les trois prochaines tranches

1. Pilote de front réel sur nuage partagé, avec rejet précoce (lentille et
   témoins de cœur, voir [VERROUS_MATHEMATIQUES_20260914.md](VERROUS_MATHEMATIQUES_20260914.md) §6),
   chemin scalaire sans préparation pour les petits facteurs, et les
   compteurs du reçu de régime aux séries 8k→64k sur uniforme, terrain,
   huit amas, en emprise pleine et pour une convention de `s` fixée.
2. Tranche FULL minimale de bout en bout à Kmax = 1 puis 2, boules
   canonisées, confrontée à `build_exhaustive_hierarchy` de `reference/`
   (voir le dialogue) et au juge différentiel v4.
3. Census q3/q4 sur les rectangles du pilote : à 50k v7, q3 et q4
   représentaient 90 % des candidates (7,9 M et 9,4 M contre 2,2 M pour q2).

La passation du constructeur après 85015a8c retient déjà le premier point ;
cette note en fixe les chiffres à atteindre.
