# Sortie plate de la hiérarchie de points v11 (expérience E1)

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only (synthétique) ; grille de 1 mm (LiDAR)
public_status=not_claimed
```

4 octobre 2026. Cette note dit comment on passe de la hiérarchie de points $H^{r}_{k+1}$
([HIERARCHIE_POINTS.md](HIERARCHIE_POINTS.md)) à un clustering plat, et comment on le compare à
`sklearn.cluster.HDBSCAN`. Elle suit, de façon critique, la spécification du juge final du workflow
`v11-points-select` (rédigée de 08 h 41 à 09 h 16 UTC, recherche privée hors dépôt), et la consigne de
l'utilisateur du même jour : la sélection plate doit être **élégante mathématiquement** et retrouver des clusters
**pertinents, sans trop de fusions parasites**, sur les démos LiDAR où la hiérarchie HGP marche.

## 1. Ce qui est repris de la spécification

- **Tête.** Arbre de points N-aire à plateaux atomiques (au plus $2n-1$ blocs ; toutes les fusions et toutes les
  entrées de même date exacte forment un plateau, en coupe fermée), condensation au **critère A** (un bloc est gros
  s'il compte au moins mcs sites engagés ; un gros enfant prolonge le cluster, deux ou plus le scindent, aucun : un
  cluster naît ; les sites des petits blocs absorbés rejoignent le cluster au niveau du plateau), score
  $S(C)=\sum_{p}(\varphi(\text{sortie condensée de }p)-\varphi(\text{haut de }C))$ avec $\varphi(r)=r^{-z}$ dans
  l'unité native de l'arbre, EOM N-aire (parent sur égalité **certifiée**), feuilles, racine exclue, aucune
  complétion, $\varepsilon=0$. Code : [`points_flat.py`](../bench/points_flat.py).
- **Arithmétique.** Filtre flottant à borne d'erreur rigoureuse, puis repli exact sur des sommes de racines de
  rationnels : égalité certifiée quand toutes les classes de carrés s'annulent, signe par encadrements entiers,
  sinon refus compté. La réciproque d'une date $\sqrt{t}+\sqrt{m}-\sqrt{q}$ suit le reçu `eom_exact_audit` de
  l'auditeur.
- **Équité.** Adversaire R0 = `sklearn` tel quel (`min_samples` = k, `min_cluster_size` = mcs, EOM,
  $\varepsilon=0$, racine exclue) ; bras d'attribution A = la même tête sur l'arbre du lien simple de `sklearn`,
  niveaux $\sqrt{N}$ relus exactement ; R0p = permutations de l'entrée ; décomposition
  $M(T)-M(R0)=[M(T)-M(A)]+[M(A)-M(R0)]$.
- **Métriques, plans, règle de décision.** mIoU un-à-un (hongrois) en primaire, PQ (test entier) et ARI_s en
  gardes, MAP comme seconde vérité ; scènes synthétiques gelées hors de la VM ; population LiDAR P08 ; règle de
  décision écrite d'avance.
- **Porte.** Oracle indépendant des petits nuages ([`points_flat_oracle.py`](../bench/points_flat_oracle.py) :
  arithmétique propre par parties sans facteur carré, énumération des antichaînes), fixtures F1 à F15, mutants
  causaux, planchers : [`points_flat_gate.py`](../bench/points_flat_gate.py).

## 2. Écarts critiques à la spécification

1. **La sélection publiée n'est pas fixée à EOM z = 1 par principe.** La spécification la choisit pour l'équité
   littérale (`sklearn` ne connaît que $\lambda=1/d$), en reconnaissant elle-même (§ 1.2, contre-exemple G1) que
   z = 1 fusionne, pour toute hiérarchie, des objets en rang dont la fragmentation interne est du même ordre que
   la lacune. C'est exactement la fusion parasite que la consigne de l'utilisateur interdit. Le choix se fait donc
   sur les exemples LiDAR où la hiérarchie marche, selon un critère écrit avant toute lecture
   ([`points_flat_study.py`](../bench/points_flat_study.py), poussé en b4632db51), parmi des règles sans paramètre
   ajouté : EOM pour z = 1, 2 et 3, et feuilles. L'équité reste assurée par le bras A, qui reçoit la même règle,
   et par R0, qui reste `sklearn` par défaut.
2. **V1, V2, κ = 2, `first`, `cover` et les complétions 1-NN sont reportés à un E1-bis.** Ils ne servent pas la
   règle de décision primaire et multiplient le coût.
3. **Mutants.** M2 (« masse = sites couverts ») est réalisé comme « masse = sites qui finiront sous le bloc ». M9
   (V2 sans unicité) suit V2 et est remplacé par la coupe ouverte au niveau d'une fusion. Le mutant « entrées de
   même date en plateaux séparés » est équivalent sous le critère A (une entrée ne fusionne jamais deux blocs) : il
   est écarté.
4. **F14** (arbres abstraits, tête seule) est construite ici : enfants gagnants de $2^{-70}$, plateau à gros et
   petits enfants, entrée entre deux niveaux qui franchit mcs, $n<$ mcs, forêt. **Forêt** : racine virtuelle au
   niveau infini ($\varphi=0$), dont les enfants sont admissibles, comme `sklearn` pour des composantes disjointes.
5. **F11** : la borne haute du bloc des sites 6 et 7 vaut exactement $\sqrt{7307}/2\approx 42{,}7405$ ; le
   42,741 de la spécification est un double arrondi.
6. **Défaut trouvé par la porte au premier essai** : un radicande carré parfait n'était pas regroupé avec les
   termes rationnels, si bien qu'une égalité exacte finissait en refus. Corrigé avant tout usage (b4632db51).

## 3. Choix de la sélection publiée

Population, mesures et règle de décision : docstring de [`points_flat_study.py`](../bench/points_flat_study.py),
écrite à 10 h 30 UTC, avant toute lecture d'une sortie plate LiDAR. Arbres exportés par la session G4
`claudeflat0` (commit 4fac50118 ; [`points_flat_dump.py`](../bench/points_flat_dump.py)) : 36 exemples organisés
de `Zoltan/demos` (niveaux exacts) et 360 bouts du criblage (niveaux flottants bornés), k = 2, 3, 5, 10.

### 3.1 Résultat du critère écrit d'avance

Population : 1 462 couples (scène, k) où la hiérarchie trouve **tous** les objets (bloc d'IoU > 1/2 pour chacun).
Sortie plate de la tour, mcs 20 ; « fusionnés » : objets dont le cluster majoritaire contient aussi plus de la moitié
d'un autre objet.

| Règle | Tous les objets retrouvés | Objets retrouvés | Objets fusionnés |
| --- | --- | --- | --- |
| EOM z = 1 | 0,685 | 0,828 | 0,021 |
| EOM z = 2 | 0,414 | 0,575 | 0,009 |
| EOM z = 3 | 0,304 | 0,441 | 0,005 |
| feuilles | 0,103 | 0,201 | 0,000 |
| `sklearn` tel quel | 0,566 | 0,721 | 0,025 |
| même tête sur l'arbre de `sklearn`, z = 1 | 0,564 | 0,719 | 0,025 |

À mcs 10, toutes les règles font moins bien. Le critère écrit d'avance retient l'EOM z = 1 à mcs 20, qui bat aussi
`sklearn` à sélection égale (le gain vient de la hiérarchie, la tête sur l'arbre de `sklearn` égalant `sklearn`).

### 3.2 Après lecture, la consigne de l'utilisateur change l'objectif

Trois précisions de l'utilisateur, le 4 octobre vers 10 h 45 UTC : « pour les surfaces, z = 2 est la bonne solution
mathématique… mais en pratique ? » ; « il vaut mieux découper un objet que de fusionner deux objets » ; « pour le
LiDAR ; sur les benchmarks synthétiques, il faut retrouver exactement les bonnes classes ». Le critère LiDAR devient
asymétrique : une fusion coûte plus qu'une découpe. Le critère synthétique reste symétrique (mIoU un-à-un, PQ,
ARI_s).

Mesure par objet trouvé par la hiérarchie, mcs 20 : intact / fusionné à un autre objet / découpé en morceaux purs.

| Règle | 31 exemples organisés (258 objets) | 5 démos (246) | 329 autres bouts (2 832) |
| --- | --- | --- | --- |
| EOM z = 1 | 0,70 / 0,109 / 0,16 | 0,79 / 0,004 / 0,21 | 0,83 / 0,016 / 0,16 |
| EOM z = 2 | 0,54 / 0,085 / 0,35 | 0,61 / 0 / 0,39 | 0,57 / 0,005 / 0,42 |
| EOM z = 3 | 0,45 / 0,062 / 0,45 | 0,54 / 0 / 0,46 | 0,43 / 0,003 / 0,56 |
| feuilles | 0,29 / 0 / 0,65 | 0,32 / 0 / 0,61 | 0,19 / 0 / 0,72 |
| `sklearn` | 0,57 / 0,143 / 0,21 | 0,68 / 0,004 / 0,32 | 0,72 / 0,018 / 0,26 |

- **z règle le compromis**, comme le prévoit le théorème 9 : de z = 1 aux feuilles, les fusions baissent et les
  découpes montent (le plus souvent en deux morceaux). L'ordre k ne change pas ce compromis : sur les exemples
  organisés, l'EOM z = 1 fusionne environ 10 % des objets à chaque k.
- **Les fusions résiduelles viennent de séparations fugaces.** Dans les trois vélos `b00_001472` à k = 5, le vélo 3
  n'est un bloc séparé qu'entre les rayons 107,4 et 107,8 mm, alors que l'union des vélos 2 et 3 persiste jusqu'à
  456,9 mm : toute EOM, à tout z, retient l'union ; seules les feuilles, qui ne lisent pas la persistance, la
  séparent, au prix de cinq morceaux et de points laissés au bruit.
- **Les feuilles** ne fusionnent jamais, mais laissent au bruit 4 à 9 % des objets : leurs points entrés au-dessus
  du niveau des feuilles restent hors de tout cluster.
- **Marge** : sur le même arbre condensé, une antichaîne optimale (oracle, vérité connue) retrouverait 252 des
  258 objets des exemples organisés, contre 198 pour l'EOM z = 1. Une règle sans vérité qui attrape les séparations
  fugaces sans morceler reste à trouver (piste : cohérence sur l'axe k).

### 3.3 En pratique, exemple par exemple

Sur les 23 exemples organisés où la hiérarchie HGP réussit (`Zoltan/demos/hgp_reussit_*`), à l'ordre montré par
chaque exemple, mcs 20 : images `plat_k<k>.png` (vérité, `sklearn`, puis la tour en z = 1, z = 2, feuilles) et
comptes `plat.json` dans chaque dossier.

| Règle | Exemples dont tous les objets sont retrouvés | Exemples avec une fusion parasite |
| --- | --- | --- |
| EOM z = 1 | 11 | 2 |
| EOM z = 2 | 9 | 3 |
| `sklearn` | 8 | 4 |
| feuilles | 4 | 0 |

- Sur les vélos et piétons accolés, l'EOM z = 1 de la tour retrouve tous les objets là où `sklearn` en fusionne deux
  (trois vélos `b06_000800`, piéton et deux vélos `b06_000800`).
- Sur les voitures, la hiérarchie de la tour garde les lignes de balayage comme des blocs : plus z est grand, plus
  la sortie plate les découpe en lignes (trois voitures `b00_000600` : `sklearn` les garde entières, z = 1 en
  découpe une, z = 2 et les feuilles les déchiquettent).
- z = 2, juste pour une densité de surface, ne retire pas les fusions parasites (elles viennent des séparations
  fugaces) et découpe davantage : en pratique, il ne fait pas mieux que z = 1.

### 3.4 Décision

- **LiDAR : EOM z = 1**, mcs 20 (ligne principale), avec **z = 2** et les **feuilles** publiées à côté : la
  préférence de l'utilisateur pour la découpe ne change pas le classement, puisque z = 2 ne réduit pas les fusions
  sur les exemples où HGP marche et que les feuilles perdent la plupart des objets. Le préenregistrement P08
  ([`e1_prereg_lidar_20261004.json`](../plans/e1_prereg_lidar_20261004.json)) teste les deux lectures : moins de
  fusions pour z = 2 (asymétrique) et non-infériorité de z = 1 en IoU un-à-un (symétrique).
- **Synthétique : choix sur le dev**, à classes exactes (mIoU un-à-un, PQ, ARI_s), parmi z = 1, 2, 3 et feuilles ;
  sessions S1a et S1b.
- Ces choix sont fixés avant toute scène de test ; le bras A reçoit la même règle.
- **Ouvert** : une règle qui garde les séparations fugaces sans déchiqueter les objets à lignes de balayage
  (l'oracle en montre la place : 252 objets sur 258).
