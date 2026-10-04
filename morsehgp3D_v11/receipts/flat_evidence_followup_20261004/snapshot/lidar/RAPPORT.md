# Sortie plate de $H^{r}_{k+1}$ : ce qu'attendent le LiDAR et Zoltan, quelles têtes

4 octobre 2026. Rôle « besoins du LiDAR et de `Zoltan/` » du workflow `v11-points-select`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (oracle exact de la référence v11, étages A et B ; Python local, petits nuages)
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé. Aucune commande git qui écrit. Aucune construction native. Aucun réseau.
Écritures : build/v11-points-select/lidar/ seulement (plus une copie temporaire de SYNTHESE.md dans le
scratchpad de session, effacée).
```

Consigne de l'utilisateur du 4 octobre : on travaille dans la v11 ; la v10 est une source de données et de
contre-exemples, jamais une autorité. Les chiffres v10 cités ici sont des données à recouper, pas des acquis.

## Résumé et décisions proposées

1. **L'aval n'attend pas une sortie plate unique.** Le tokenizer HGP-UNet (composants FP, S1 à S6) consomme une
   hiérarchie : coupes emboîtées, condensation à seuil relatif, affectation des points. La sur-segmentation fine y est
   voulue. La sortie plate ne sert qu'à deux usages : la tête d'instance S7, jugée en PQ (appariement à IoU > 1/2,
   instances d'au moins 50 points, mêmes prédictions sémantiques que ses témoins EOM et ALPINE), et les sondes
   d'oracle. Deux régimes ne doivent jamais être mélangés :
   - **R0, sans sémantique** : celui des démos et du § 7 de la note ;
   - **R1, sémantique par classe** : celui de S7, d'ALPINE et de la thèse (§ 5.4 : « pour chaque classe d'intérêt,
     nous générons une hiérarchie »).
2. **La sur-segmentation à mcs = K de la v10 a deux causes, démontrées sur des fixtures exactes.**
   - L'entrée par première couverture fait de chaque amas d'échantillonnage de K sites un bloc de masse K dès sa
     naissance.
   - L'EOM retient ces blocs quand leur contraste de λ dépasse celui du parent.

   $H^{r}_{k+1}$ supprime exactement les blocs des structures isolées de k sites (dimères, segments d'anneau de
   k sites). Il ne supprime pas ceux de k + 1 sites ou plus. Il réduit donc la sur-segmentation de `cover` sans
   l'abolir :

   | Fixture | `cover` | $H^{r}_{k+1}$ | Remarque |
   | --- | ---: | ---: | --- |
   | dimères | 8 clusters | 2 | corrigé |
   | mur régulier | 15 | 2 | corrigé |
   | trimères | — | — | toutes les règles éclatent à mcs ≤ k + 1 |

   **mcs = k n'est pas un point de fonctionnement LiDAR**, pour aucune règle.
3. **Choix de mcs.** Deux lemmes exacts, liés au protocole de la PQ, encadrent mcs.
   - **L2** : mcs ≤ 26 ne retire aucun bloc capable d'apparier une instance évaluable.
   - **L1** : une instance de $n_g\leq$ mcs/2 points ne peut être appariée par aucun cluster sans void.
   - **Conséquence** : sur les 2 918 instances du criblage, mcs = √n rend 28,3 % des instances (45,1 % des petites
     classes) inappariables, quelle que soit la hiérarchie. Il faut retirer √n du volet LiDAR de E1.
   - **Recommandation** : mcs = 20 comme valeur primaire, 26 comme borne, k et 10 en diagnostic.
4. **Sélection.**
   - **EOM** en $\lambda=1/r$, toujours publiée : c'est la convention de scikit-learn, donc la comparaison équitable.
     En variante sans échelle, $\lambda=-\log r$.
   - **Exclus** : z = 3 est écarté pour le LiDAR ; les feuilles restent en diagnostic.
   - **Régime R1** : « EOM-boîte », c'est-à-dire le coût géométrique du § 5.2 (la boîte de référence de la classe).
     Le programme dynamique est exact, car la faisabilité est monotone.
   - **Complétion** par lignée. À défaut, des segments singletons, jamais l'identifiant 0.
5. **Où la tour gagne sur le LiDAR.** On relit les mesures G4 existantes par strates, au niveau B. L'avantage de la
   tour se concentre sur :
   - les vélos : +0,045 à k = 5, +0,093 à k = 10 ;
   - les contacts avec une structure ;
   - les objets à moins de 0,3 m d'une autre instance ;
   - les petites instances ;
   - la portée inférieure à 20 m.

   Il est nul sur les voitures. Au-delà de 30 m, la tour perd 0,010 à k = 2, sur une strate de 105 instances.

   Un mécanisme exact en rend compte. Prenons deux objets sur les mêmes anneaux : pas s le long de l'anneau, a·s
   entre anneaux, a ≈ 2,2. La tour les sépare dès une lacune g > s à k = 3, ou g > s(√(1+a²) − 1) ≈ 1,42 s à
   k = 2. HDBSCAN ne les sépare que pour g > a·s.
6. **Mesure LiDAR de E1 (cellule primaire, écrite d'avance).**
   - **Comparaison** : $H^{r}_{k+1}$ contre l'arbre de HDBSCAN, sous la même tête.
   - **Réglages** : k ∈ {3, 5}, mcs = 20, EOM z = 1, sans complétion.
   - **Métrique** : PQ_th sans sémantique, sur les instances « things » vraies, l'IoU étant calculée sur des clusters
     privés de leurs seuls points void (§ 6.3).
   - **Compléments** : strates, régime R1 contre ALPINE, prédictions et règles de décision au § 6.

## 0. Sources et méthode

Lu en entier : `Zoltan/FoundationModel/` (README, OBJET, SPECIFICATION, MESURE, ARCHITECTURE § 4–5, PLAN § 5,
REAUDIT § SEL), `Zoltan/demos/` (README général, README des cinq démos, `recherche/README.md`, `tools/search_frames.py`),
`morsehgp3D_v11/docs/HIERARCHIE_POINTS.md`, la synthèse du juge (`points_math/SYNTHESE.md`), les réponses de
l'auditeur, les extraits de la thèse (§ 4.4.4, ch. 5, § 9.1), le banc v11 (`points_radius.py`, `points_hierarchy.py`,
`points_reference.py`, `points_summary.py`, `points_lidar_prepare.py`), l'article ALPINE (PDF de la racine,
texte extrait dans `sorties/alpine_texte.txt`).

Données, sans KITTI sur le disque :

- **Criblage** : `Zoltan/demos/recherche/criblage_08.jsonl`. Pour chaque instance : classe, points sans sol,
  portée, classes à moins de 0,3 m, écart à l'instance voisine, meilleur IoU HDBSCAN à K = 1, 5 et 10.
- **Archives G4 v11** : sessions D (`claudepts4`) et F (`claudepts6`), extraites par `git show origin/main:…` dans
  `donnees_g4/`. Elles ne contiennent que des scores et des étiquettes, aucune coordonnée.
- **Oracle exact de la v11** : étage A pour n ≤ 20, étage B pour 25 à 48 sites.

Une seule tête de sélection sert à toutes les hiérarchies (`scripts/lidar_lib.py`) :

- condensation N-aire à mcs, sans jamais binariser une multifusion ;
- EOM en $\lambda=r^{-z}$ ou $-\log r$, ou sélection des feuilles ;
- racine exclue ; à égalité, le parent l'emporte.

Appliquée à l'arbre de HDBSCAN avec $\lambda=1/r$, elle rend **exactement** les étiquettes de
`sklearn.cluster.HDBSCAN` (scikit-learn 1.9.1) : 1 920 comparaisons, 0 écart, à k ∈ {1, 2, 3, 5},
mcs ∈ {2, 3, 5, 8}, en EOM et en feuilles (`sorties/selftest_tete.txt`).

## 1. Ce que l'aval attend (a)

### 1.1 Le tokenizer consomme une hiérarchie, pas une sortie plate

Le tokenizer n'entre pas une partition. Il entre, pour chaque ordre K (SPECIFICATION § 2 à 4) :

- l'arbre condensé à seuil **relatif** α de la masse du parent, avec multifusions atomiques et réserves ;
- L coupes emboîtées, soit environ 160 000 unités par trame brute, tous niveaux confondus ;
- l'affectation $P_1$ des points au niveau 1 ;
- les quotients durs entre niveaux et le graphe de fusion.

Conséquences pour $H^{r}_{k+1}$ :

- **Granularité.** Aux niveaux fins, la granularité voulue est celle de superpoints. La porte 0.2 compare la pureté
  des nœuds à SPT à nombre d'unités égal. Une sur-segmentation n'y est pas un défaut ; un bloc qui chevauche deux
  objets en est un. $H^{r}_{k+1}$ n'en crée aucun avant la fusion FULL (fidélité F1).
- **Affectation dure.** $H^{r}_{k+1}$ fournit une affectation dure datée : les sites non encore entrés sont des
  réserves. La SPECIFICATION prévoit une $P_1$ douce par facettes, celle du § 9.1. $H^{r}_{k+1}$ en est une
  alternative dure, à déclarer comme telle.
- **Stabilité (corollaire de H3, prouvé).**
  - Requantifier à 1 mm le nuage exactement tourné déplace chaque site d'au plus $\sqrt{3}/2$ mm.
  - Si aucun site n'en fusionne avec un autre (H3 suppose des sites distincts appariés), dates et hauteurs de
    réunion de $H^{r}_{k+1}$ bougent donc d'au plus $3\sqrt{3}/2\approx 2{,}6$ mm.
  - C'est une borne déterministe pour la porte 0.7 de MESURE, valable pour la hiérarchie. Toute coupe plate prend ses
    valeurs dans un ensemble discret : elle est discontinue.
  - Aucune borne n'existe sous décimation (porte 0.3), car H3 exclut l'insertion et la suppression (témoin {0, 2, 4}
    de l'auditeur).
- **Seuil relatif seul.** La SPECIFICATION reconnaît que le seuil relatif « filtre le déséquilibre sans fixer de
  masse minimale » (§ 2.2). Or les amas d'échantillonnage et les anneaux d'une surface LiDAR forment des scissions
  équilibrées, que tout α ≤ 1/2 conserve. Un plancher absolu (mcs) ou un critère de durée reste nécessaire (§ 3).

### 1.2 La tête d'instance S7 et les sondes

**S7** sélectionne une antichaîne par programme dynamique, avec un coût d'abord statistique, appris ensuite. Ses
témoins sont fixés : l'EOM sur le même arbre et ALPINE, avec les mêmes prédictions sémantiques. Ses métriques sont
PQ, PQ_th, RQ et SQ (MESURE § 3, ARCHITECTURE § 5 SEL).

**Protocole SemanticKITTI.**

- Appariement unique à IoU > 1/2.
- Points void exclus : non étiqueté, aberrant, autre structure, autre objet.
- min_points = 50 (`Zoltan/demos/README.md`).

**Code d'évaluation public** (`PanopticEval` de semantic-kitti-api, **de mémoire : à vérifier sur le code épinglé
avant E1**) :

- FN et FP ne comptent que les segments d'au moins min_points ;
- les identifiants d'instance sont décalés de 1, si bien que les points d'identifiant 0 d'une classe forment un seul
  segment.

**Deux conséquences.**

- Les miettes de moins de 50 points ne sont pas des FP. Elles ne coûtent que par l'IoU ou l'appariement de l'objet
  qu'elles fragmentent.
- Le bruit d'une tête doit être complété, ou rendu en segments singletons d'identifiants propres. Sous l'identifiant 0,
  il formerait un faux segment.

**Sondes.** La porte 0.1 est le plafond d'oracle par nœud, stratifié par classe, portée et taille. La porte 0.9 le
stratifie par contact, avec et sans sol. Le meilleur bloc par objet du § 7 de la note est cette sonde : c'est le
niveau B.

### 1.3 Ce qu'est une bonne sortie, d'après les démos

- **Granularité.** La bonne sortie a la granularité de l'objet : une instance « thing » par objet évaluable
  (≥ 50 points sans sol), ni une partie, ni une rangée.
- **Les échecs ciblés par les démos.** Ce sont de petits objets au contact :
  - d'un voisin de même classe (vélos en rang) ;
  - d'une structure (façade) ;
  - ou d'un sol conservé.

  Les voitures ne posent pas problème : 0 échec de la hiérarchie HDBSCAN sur 2 188.
- **L'axe K.** C'est le levier contre les ponts ténus. Il agit sur la hiérarchie (niveau B), pas sur la sélection.
  - Sur les démos (`sorties/demos_objets.txt`), $H^{r}_{k+1}$ à k = 3 rattrape le vélo C de 04 (0,557 contre 0,348)
    et le vélo 55 (01 : 0,550 ; 04 : 0,512).
  - Aucune règle de la tour ne rattrape 01-B (au plus 0,478), 02-A (0,404), 02-B (0,327) ni 03-A (0,468).
  - ALPINE sans sémantique, en vue de dessus, rattrape 01-B (0,82) et 03-A (0,71).
- **Le bruit.** Une trame LiDAR n'a pas de bruit au sens de HDBSCAN : tout retour appartient à une surface. Un point
  « bruit » dans un objet est une perte de rappel.
- **R0 et R1 diffèrent sur la difficulté elle-même.** En R1, la façade et le sol sortent du sous-nuage de la classe
  vélo. Il ne reste que les contacts de même classe : vélos en rang, piétons en groupe, voitures en file. La
  thèse (§ 5.4) et ALPINE travaillent en R1. Les démos et le § 7 travaillent en R0.

## 2. Ce que disent les données LiDAR (c, première partie)

### 2.1 Le criblage : 299 trames, 2 918 instances d'au moins 50 points (`sorties/criblage_stats.txt`)

Échantillon : trames de 32 462 à 101 472 sites sans sol (médiane 62 342), soit 9,8 instances évaluables par trame.
Biais de présélection : chaque trame a au moins un petit objet et deux voitures.

Taille médiane, en points sans sol, par classe et par portée :

| Classe | 0–10 m | 10–20 m | 20–30 m | 30–50 m |
| --- | ---: | ---: | ---: | ---: |
| voiture | 2 158 | 488 | 138 | 76 |
| piéton | 410 | 105 | 64 | — |
| vélo | 190 | 77 | 57 | — |
| moto | 556 | 156 | 104 | 57 |
| cycliste | 584 | 181 | 62 | 74 |

La pente du log de la taille contre le log de la portée vaut −1,73 pour les voitures, −1,57 pour les piétons et −1,34
pour les vélos. La troncature à 50 points la biaise vers 0 ; la géométrie du balayage donne −2.

Écarts, contacts et échecs de HDBSCAN :

| Strate | Toutes classes | Vélos |
| --- | ---: | ---: |
| écart à une autre instance < 0,1 m | 1,6 % | 15,2 % |
| écart de 0,1 à 0,3 m | 2,7 % | 15,2 % |
| sol résiduel à < 0,3 m (après Patchwork++) | 30,5 % | 56,5 % |
| structure à < 0,3 m | 9,8 % | 52,9 % |
| échec du meilleur nœud HDBSCAN, K = 5 | 1,1 % | 15,9 % |
| échec, K = 5, écart < 0,1 m | 19,6 % | 42,9 % |
| échec, K = 5, contact structure | 9,5 % | 24,7 % |

Lecture : les échecs sont rares en moyenne, à 1,1 %, et concentrés sur les petits objets au contact.

L'effet attendu d'une meilleure hiérarchie sur la PQ moyenne est donc vraisemblablement faible. C'est une estimation,
non mesurée en sortie plate : au niveau B, le solde des sauvetages et des pertes de $H^{r}_{k+1}$ face à HDBSCAN vaut

- 0 % des instances sur les témoins ;
- de 1,4 à 4,2 % sur les échecs et les voisines, selon k (`sorties/ecart_cover_margin.txt`).

Une mesure qui ne stratifie pas ne verra pas cet effet.

### 2.2 Les mesures G4 v11 restratifiées (niveau B ; `sorties/strates_lidar.txt`)

**Contrôle.** Le script recalcule les cohortes du § 7 de la note : démos 5/72, échecs 37/425, voisines 71/859,
témoins 20/204. Il retrouve toutes les cellules, à l'arrondi près (0,976 au lieu de 0,977 pour un seul témoin).

**Strates.** On joint ensuite 1 546 instances aux attributs du criblage, démo 04 exclue (elle garde le sol).
Les cohortes regroupées ne forment **pas** un échantillon aléatoire : les échecs et les voisines sont choisis sur
l'échec de HDBSCAN. Lecture indicative ; les tables par cohorte sont dans la sortie.

Chaque cellule donne le meilleur IoU moyen, HDBSCAN puis $H^{r}_{k+1}$, et les sauvetages et pertes
(+sauvetages/−pertes). Un sauvetage est un objet que HDBSCAN laisse à 1/2 ou moins et que la règle dépasse.

| Strate | n | k = 3 | k = 5 | k = 10 |
| --- | ---: | --- | --- | --- |
| vélos | 253 | 0,665 / 0,696, +25/−5 | 0,652 / 0,697, +23/−3 | 0,586 / 0,679, +40/−1 |
| piétons | 162 | 0,849 / 0,855, +2/−5 | 0,845 / 0,861, +7/−1 | 0,848 / 0,870, +7/−1 |
| voitures | 992 | 0,969 / 0,965, +1/−0 | 0,965 / 0,963, +1/−0 | 0,959 / 0,959, +0/−0 |
| écart < 0,1 m | 125 | 0,676 / 0,699, +8/−0 | 0,670 / 0,703, +9/−3 | 0,621 / 0,699, +12/−0 |
| écart 0,1–0,3 m | 106 | 0,745 / 0,784, +6/−3 | 0,735 / 0,787, +5/−0 | 0,684 / 0,777, +16/−1 |
| contact structure | 231 | 0,643 / 0,675, +21/−7 | 0,630 / 0,672, +25/−3 | 0,597 / 0,666, +26/−1 |
| contact sol résiduel | 596 | 0,867 / 0,871, +14/−0 | 0,860 / 0,870, +12/−2 | 0,840 / 0,862, +17/−0 |
| aucun voisin à 0,3 m | 706 | 0,967 / 0,967, +4/−0 | 0,964 / 0,966, +6/−1 | 0,951 / 0,963, +12/−0 |
| taille 50–99 | 390 | 0,831 / 0,838, +14/−6 | 0,823 / 0,839, +20/−2 | 0,799 / 0,837, +27/−1 |
| portée < 10 m | 549 | 0,882 / 0,893, +19/−4 | 0,878 / 0,892, +15/−3 | 0,861 / 0,886, +21/−1 |
| portée 10–20 m | 620 | 0,874 / 0,879, +12/−6 | 0,867 / 0,878, +19/−1 | 0,849 / 0,874, +30/−1 |
| portée ≥ 30 m | 105 | 0,957 / 0,945, +0/−0 | 0,950 / 0,951, +0/−0 | 0,945 / 0,952, +0/−0 |

Lecture :

- **Où l'avantage se loge.** Il se loge là où HDBSCAN échoue : vélos, écarts sous 0,3 m, contacts.
- **Avec l'ordre.** L'écart croît avec k parce que HDBSCAN se dégrade quand k augmente. La tour reste presque stable :
  entre k = 5 et k = 10, les vélos passent de 0,697 à 0,679 (−0,018), contre 0,652 à 0,586 pour HDBSCAN (−0,066).
  Sur LiDAR, c'est la prédiction du § 4.4.5 de la thèse, observée.
- **Aucun avantage** sur les voitures ni sur les objets isolés.
- **À grande portée et petit k**, la tour est légèrement en dessous de HDBSCAN : à 30 m et au-delà, −0,010 à k = 2
  et −0,012 à k = 3 ; sur les seuls témoins, −0,028 et −0,033 (22 instances). Cause non établie : à stratifier
  dans E1.
- **Pas d'effet de la qualification.** À chaque k, l'écart moyen entre `cover` et $H^{r}_{k+1}$ vaut au plus 0,0054
  dans toutes les strates d'au moins 20 instances (`scripts/ecart_cover_margin.py`) : au niveau B, la qualification
  et la marge ne pèsent pas sur le LiDAR.

### 2.3 Un mécanisme exact : les objets en rang

**Modèle.** Deux objets de 3 × 3 sites sur les mêmes anneaux : pas s = 100 le long de l'anneau, a·s = 220 entre
anneaux, lacune g de bord à bord. C'est le rapport des pas angulaires du HDL-64 de KITTI, environ 0,4° contre 0,18°.
On cherche à partir de quelle lacune un bloc égal à chaque objet existe (niveau B = 1).

Oracle exact, étage A (`scripts/seuil_rang_k3.py`, `sorties/seuil_rang.txt`) :

| Hiérarchie | Séparés pour | Non séparés pour |
| --- | --- | --- |
| HDBSCAN, `min_samples` = 2 ou 3 | g = 221 | g ≤ 219 |
| tour, k = 2 (`cover`, `first`, $H^{r}_{3}$, `core`) | g ≥ 142 | g ≤ 141 |
| tour, k = 3 (`cover`, `first`, $H^{r}_{4}$) | g ≥ 101 | g ≤ 100 (égalité exacte à 100) |
| `core`, k = 3 | jamais (g ≤ 221 testé) | — |

**Énoncés.**

- **HDBSCAN (prouvé).** Pour un site d'extrémité, la distance de cœur vaut au plus g, et l'atteignabilité mutuelle
  à travers la lacune vaut max(d_K, g). La liaison verticale interne vaut max(d_K, a·s). Les deux objets se
  rejoignent donc par les anneaux avant d'être connexes verticalement dès que g < a·s.
- **Tour à k = 2 (argument et calcul exact).**
  - À travers la lacune, le triplet colinéaire de plus petite boule a pour rayon (s + g)/2.
  - Dans l'objet, le triangle rectangle entre anneaux a pour rayon $s\sqrt{1+a^{2}}/2$.
  - Les objets sont donc séparables si et seulement si $g>s(\sqrt{1+a^{2}}-1)=141{,}66$ ; l'oracle le confirme.
- **Tour à k = 3 (calcul exact, mécanisme identifié).**
  - La configuration isocèle de base 2s et de hauteur a·s connecte chaque objet au rayon $s(1+a^{2})/(2a)$.
  - À g = s exactement, la même configuration enjambe la lacune au même rayon, d'où l'égalité. Au-delà, elle
    l'enjambe plus tard.
- **Statut.** Le seuil général (toute taille d'objet, tout a) est une **conjecture** ; seuls les cas calculés sont
  établis.

**Portée.** C'est l'explication la plus directe des sauvetages de vélos en rang.

- HDBSCAN exige une lacune plus grande que l'écart entre anneaux à cette portée : 7 cm à 10 m.
- La tour à k = 3 se contente d'une lacune plus grande que le pas horizontal : 3 cm à 10 m.

Avec du bruit, l'expérience du § 5 (famille `rang`) le confirme à g = 150 et g = 200 : la tour a un niveau B de
1,0, HDBSCAN de 0,33.

## 3. Sur-segmentation à mcs = K, choix de mcs et de la sélection (b)

### 3.1 La mesure de la v10

Cinq trames de `Zoltan/`, K = 5, mcs = K, hiérarchie `cover` : 5 732 clusters par trame, contre 867 pour HDBSCAN
(CONTEXTE.md, qui cite la v10). La v10 l'explique ainsi : « chaque boule de K points fonde une feuille de masse K qui
passe le seuil ».

Le résumé de session du 1er octobre donne en outre, en IoU moyen plat de la v10 (pas une PQ) :

| Règle | IoU moyen plat |
| --- | ---: |
| `cover` | 0,450 |
| HDBSCAN | 0,580 |
| `core` | 0,597 |
| `VC[coeur,W1]` (1 223 clusters par trame) | 0,601 |

Le rapport source (`mesure_vc`) n'est plus sur le disque : chiffres non revérifiés.

Deux réserves sur ces chiffres :

- le compte inclut les clusters de « stuff » (façades, végétation) ;
- la métrique n'est pas la PQ.

### 3.2 Le mécanisme, sur fixtures exactes (`scripts/fixtures_mecanismes.py`, `sorties/fixtures_mecanismes.txt`)

Toutes les fixtures sont calculées par l'oracle de la définition (étage A, recoupé par l'étage B sur F1). Chacune
contient deux groupes lointains, pour que la racine, exclue de la sélection, se scinde.

| Fixture, k | Hiérarchie | mcs = k, EOM z = 1 | z = 3 | −log r | mcs = k + 1 | mcs = k + 2 |
| --- | --- | --- | --- | --- | --- | --- |
| F1 dimères (paires d'écart 2, lacunes 4), k = 2 | `cover` | 8 paires | 8 paires | 2 chaînes | 2 chaînes | — |
| F1 | $H^{r}_{3}$ | **2 chaînes** | **2 chaînes** | 2 chaînes | 2 chaînes | — |
| F1 | HDBSCAN | 8 paires | 8 paires | 2 chaînes | 2 chaînes | — |
| F2 trimères (écart 2, lacunes 6), k = 2 | toutes les règles et HDBSCAN | 6 trimères | 6 trimères | 2 chaînes | 6 trimères | 2 chaînes |
| F3 grille 3 anneaux × 5, pas 10 et 22, k = 2 | `cover` | 3 (grille, 2 paires) | 3 | 2 | 2 | — |
| F3, k = 2 | $H^{r}_{3}$, `first`, `core` | **2** | **2** | 2 | 2 | — |
| F3, k = 2 | HDBSCAN | 5 (3 anneaux, 2 paires) | 5 | 2 | 4 (3 anneaux) | — |
| F3, k = 3 | toute la tour | 2 | 2 | 2 | 2 | — |
| F3, k = 3 | HDBSCAN | 2 | 4 (3 anneaux) | 2 | 2 | — |

**Mécanisme 1 : l'entrée à la première couverture.**

- Sous `cover`, un site entre à $\alpha_k(x)$, rayon de sa plus petite boule de k sites, compris entre $d_k/2$ et
  $d_k$. C'est l'échelle des fluctuations d'échantillonnage.
- Une boule de k sites qui est la première couverture de ses sites devient un bloc de masse k dès sa naissance.
  À mcs = k, c'est un cluster.
- Les dimères de F1 le montrent exactement : 8 paires.

**Mécanisme 2 : le contraste de λ.**

- L'EOM garde les enfants quand la somme de leurs stabilités dépasse celle du parent. Une stabilité en $r^{-z}$ est
  dominée par les plus petits rayons d'entrée, d'autant plus que z est grand.
- Sur une chaîne de paires d'écart s et de lacunes g, le rapport entre fusion et naissance des paires vaut
  (s + g)/s pour la tour (paires à s/2, fusion à (s + g)/2), mais g/s pour HDBSCAN. Il est toujours plus grand
  pour la tour.
- À même entrée et même tête, ce contraste plus fort favorise l'éclatement par l'EOM aux petits mcs. C'est un
  raisonnement, pas un théorème : sur F1, les deux arbres éclatent pareillement.

**Mécanisme 3 : l'anisotropie du balayage (proposition R-H, prouvée).**

- *Cadre.* Grille plane infinie, pas 1 le long des anneaux et a > 1 (non entier) entre anneaux, `min_samples` = K
  (point compté).
- *Énoncé.* Si $K\leq 2\lfloor a\rfloor+1$, alors $d_K=\lceil (K-1)/2\rceil$. À tout niveau ρ de $[d_K,a)$, les
  composantes du graphe d'atteignabilité mutuelle sont exactement les anneaux.
- *Preuve.* Deux sites voisins sur un anneau sont à l'atteignabilité $d_K\leq\rho$ ; deux sites d'anneaux distincts
  sont à distance au moins a > ρ. □
- *Application.* Pour a = 2,2 : phase « anneaux » de rapport 2,2 à K = 2 et 3, de rapport 1,1 à K = 4 et 5,
  aucune à K ≥ 6.

**Proposition R-T, k = 2 (preuve esquissée, vérifiée sur F3).**

- *Énoncé.* Sur la même grille, pour $r\in[1,\sqrt{1+a^{2}}/2)$, les composantes de $L_2(r)$ qui couvrent au moins
  3 sites sont les anneaux. Cet intervalle est non vide si et seulement si $a>\sqrt{3}$.
- *Preuve.* Un triplet de sites consécutifs d'un même anneau a pour boule minimale un rayon de 1, et tout triplet à
  cheval sur deux anneaux a pour boule minimale un rayon d'au moins $\sqrt{1+a^{2}}/2$. Le cas d'égalité est le
  triangle rectangle de côtés 1 et a ; un site hors des colonnes adjacentes allonge le plus grand côté. Sur
  l'intervalle, les sommets de $\Gamma_2$ sont donc reliés seulement le long des anneaux, et les paires verticales
  y restent des sommets isolés. Par le théorème 2, l'amas discret d'une composante est la réunion de ses sommets :
  celui d'une composante d'anneau est l'anneau, celui d'une paire isolée a 2 sites.
- *Vérification.* À k = 3, l'oracle ne montre aucune phase « anneaux » sur F3 : la grille entière fusionne en une
  multifusion à 17 enfants au rayon 13,27. La généralisation à tout k ≥ 3 est une **conjecture**.

### 3.3 Ce que $H^{r}_{k+1}$ corrige, et ce qu'il ne corrige pas

**Ce qui est prouvé.** Il n'existe aucun bloc d'une structure isolée de k sites (note, prix 3), et toute entrée a
lieu au plus tôt à $\alpha_{k+1}$.

Effet exact :

- F1 : 8 clusters pour `cover`, 2 pour $H^{r}_{3}$, à mcs = 2, en z = 1 comme en z = 3.
- Mur régulier bruité du § 5 (k = 2, mcs = 2, z = 1) : 15 clusters pour `cover`, 2 pour $H^{r}_{3}$, 5 anneaux pour
  HDBSCAN.

**Ce qui n'est pas corrigé (contre-exemple exact F2).** Les amas de k + 1 sites ou plus restent des blocs, pour toutes
les règles :

- les trimères de F2 éclatent à mcs ≤ 3 ;
- les segments d'anneau de 3 sites de la famille `rang` du § 5 éclatent à g = 150, k = 2, mcs = 2, en 9 clusters
  pour toutes les règles.

Le remède est un mcs supérieur à la taille des amas d'échantillonnage qualifiés, pas la qualification.

**Prédiction pour E1.**

- À mcs = k, $H^{r}_{k+1}$ produira beaucoup moins de clusters par trame que `cover`, sans revenir au compte de
  HDBSCAN. La raison : ses entrées tombent entre $\alpha_{k+1}$ et $\alpha_{k+1}+d_k/2$ (H5), à l'échelle des
  entrées `core`, $d_k$.
- Or `core` faisait 0,597 en v10, contre 0,450 pour `cover` (§ 3.1).
- Statut : **conjecture**, écrite au § 6.4 comme prédiction P1.

### 3.4 Quel mcs (lemmes exacts et données ; `scripts/mcs_lemmes.py`, `sorties/mcs_lemmes.txt`)

**Notations.** Soit g une instance vraie de $n_g$ points non void. Soit p un cluster prédit, restreint à ses points
non void ; il apparie g si IoU(p, g) > 1/2.

**Fait.** Un appariement exige $n_g/2<|p|<2n_g$.

*Preuve.* $\mathrm{IoU}>1/2$ équivaut à $3|p\cap g|>|p|+n_g$. Avec $|p\cap g|\leq n_g$, on obtient $|p|<2n_g$.
Avec $|p\cap g|\leq|p|$, on obtient $|p|>n_g/2$. □

**L1 (perte certaine).** Si $n_g\leq\mathrm{mcs}/2$, aucun cluster sans void de masse au moins mcs n'apparie g, et ce
pour toute hiérarchie et toute sélection.

**L2 (absence de perte au niveau B).**

- *Énoncé.* Si $\mathrm{mcs}\leq\lfloor n_g/2\rfloor+1$, tout bloc de la hiérarchie qui apparie g a au moins mcs
  sites. Par la factorisation (synthèse § 5.1), c'est encore un bloc de la hiérarchie condensée à mcs, au même rayon.
- *Application.* Pour SemanticKITTI ($n_g\geq 50$), **mcs ≤ 26 ne retire aucun bloc appariant une instance
  évaluable**.
- *Limite.* L2 porte sur les blocs (niveau B). Les clusters sélectionnables de l'arbre condensé sont les blocs de
  tête de chaque segment. Ils contiennent le bloc appariant, éventuellement augmenté de masse légère absorbée. Le
  plafond propre à la sortie plate est l'oracle C (§ 6.3).

Sur les 2 918 instances évaluables du criblage :

| mcs | Instances inappariables (L1), toutes / petites classes | Instances hors de la zone sûre de L2, toutes / petites classes |
| --- | --- | --- |
| 5 à 26 | 0 % / 0 % | 0 % / 0 % |
| 30 | 0 % / 0 % | 4,3 % / 8,3 % |
| 50 | 0 % / 0 % | 21,1 % / 34,6 % |
| round(√n), soit 180 à 319 selon la trame | **28,3 % / 45,1 %** | 64,0 % / 88,0 % |

**Bornes sur mcs.**

- *Borne haute.* mcs ≤ 26, prouvée sans perte au niveau B pour toute instance évaluable. **√n est exclu du volet
  LiDAR** : il est raisonnable sur les scènes synthétiques à groupes de centaines de points, il ne l'est pas ici.
- *Borne basse* (raisonnée, non prouvée).
  - mcs doit dépasser la taille des amas d'échantillonnage qui restent des blocs : au moins k + 2 d'après F2.
  - Sur une trame réelle, ces amas sont des fluctuations de densité de quelques k sites ; à k ≤ 10, mcs ≈ 20 les
    dépasse. 20 est aussi la valeur de 3DUIS.
- *Proposition.* **mcs = 20** comme valeur primaire, 26 comme borne, 10 et k en diagnostic.

**Densité décroissante avec la portée.**

- *Constat.* Un mcs fixe, exprimé en points, équivaut à une aire physique minimale qui croît comme $R^{2}$. Mais le
  protocole de la PQ compte lui aussi en points (min_points = 50) : un mcs en points est donc cohérent avec ce que
  l'on mesure.
- *Pourquoi ne pas l'adapter.* Un mcs adapté à la portée réintroduirait une constante métrique, ce que la thèse de
  Zoltan veut éviter. ALPINE a d'ailleurs trouvé moins bon un seuil proportionnel à la portée : 75,9 contre 76,3 de
  PQ sur nuScenes (tableau 9 de l'article).
- *Variante permise (prouvée ici).* La factorisation reste vraie pour toute mesure positive μ des sites, par exemple
  $\mu_x\propto R_x^{2}$, l'empreinte du retour. La preuve de la synthèse n'utilise que deux faits : la masse couverte
  croît le long d'une lignée, et un bloc est inclus dans l'amas discret de son nœud. La variante est permise, à
  déclarer, mais pas recommandée en primaire.
- *Recommandation.* Pas d'adaptation à la portée ; stratifier par portée.

### 3.5 Quelle sélection

- **EOM en $\lambda=1/r$ (z = 1).** C'est la convention de scikit-learn, donc la comparaison équitable avec HDBSCAN.
  C'est aussi le réglage que la v10 retenait pour le LiDAR (résumé de session du 1er octobre). À **toujours
  publier** (consigne du 1er octobre).
- **EOM en $\lambda=-\log r$.**
  - *Propriété.* La décision ne dépend que des rapports de rayons, sans surpondérer les petits rayons.
  - *Fixtures.* Elle a évité tous les éclatements d'amas et d'anneaux de F1 à F3 et du mur. Elle n'évite pas ceux
    des segments d'anneau de la famille `rang` à g = 150. À g = 200, k = 2, mcs = 2, $H^{r}_{3}$ passe de 7,0 clusters
    (z = 1) à 5,0, quand `cover` et HDBSCAN restent à 9.
  - *Bilan de l'expérience* (345 cellules famille × paramètre × k × mcs × règle ; `sorties/comparaison_z.txt`) :
    −log r bat z = 1 dans 18 cellules. Il est battu dans 4 cellules notables, toutes de la famille `rang` à g = 150
    et k = 5, et dans 3 cellules négligeables (moins de 0,001).
  - *Statut.* Sa supériorité sur trames réelles est une **conjecture** (prédiction P3).
  - *Limite.* Toute puissance $r^{-z}$ est aussi invariante par homothétie globale. Aucune ne l'est quand la lacune
    entre objets est physique et que les rayons internes croissent avec la portée.
- **EOM en z = 3.** Ce réglage vient du synthétique v10 (« z est un cadran de granularité, pas une dimension »). Sur
  des surfaces, il favorise le plus les fragments à petits rayons.
  - *Fixtures F1 à F3 et mur* : jamais meilleur que z = 1, souvent pire.
  - *Expérience* : pire que z = 1 dans 22 cellules ; meilleur de façon notable dans une seule (`first`, `rang`,
    g = 150, k = 3, mcs = 6), et de moins de 0,001 dans 26 cellules de la famille `sol`.
  - **À écarter** pour le LiDAR, sauf comme référence v10.
- **Feuilles.** À mcs = 20, les feuilles sont les plus petits amas de 20 points : elles fragmentent les voitures
  proches, qui comptent des centaines à des milliers de points. Diagnostic seulement : les familles filiformes du
  synthétique les veulent, pas le LiDAR.
- **Coût géométrique du § 5.2 de la thèse.** La thèse parle de « volume maximal attendu d'un objet ». Sa version
  exacte sur l'arbre est l'**EOM-boîte** (`select_box`) :

  $$V(C)=\max\left(S(C),\sum_{D}V(D)\right)\text{ si }C\text{ tient dans la boîte, sinon }\sum_{D}V(D).$$

  - *Exactitude.* Tenir dans une boîte est monotone : tout sous-ensemble d'un ensemble qui tient tient aussi. La
    récurrence est donc l'optimum exact de la somme des stabilités sur les antichaînes faisables : c'est l'EOM
    restreinte aux sous-arbres maximaux faisables. Elle accepte les multifusions N-aires. Une feuille trop grande est
    gardée, et cette politique est déclarée.
  - *Démonstration.* Sur la famille `rang` (g = 150, mcs = 4 ; `sorties/experience_boite.txt`), l'EOM-boîte fait
    passer la tour de 2,3 à 3 objets appariés sur 3. Sur l'arbre de HDBSCAN, elle n'y change rien (0 sur 3) : une
    boîte ne peut choisir qu'une scission présente dans la hiérarchie.
  - *Régime R1.* C'est la tête naturelle, avec la boîte de référence de la classe. ALPINE en prend le plus petit côté
    pour seuil $t_c$, et découpe tout amas qui ne tient pas dans cette boîte agrandie de 30 %. La thèse décrit la même
    règle au § 5.4 : le volume aberrant force la scission.
  - *Régime R0.* Seule une boîte « plus grand objet thing » est disponible, et elle ne sépare pas deux vélos.
- **Epsilon de sélection.**
  - En unités métriques, c'est un seuil d'ALPINE : un témoin, pas une tête de la tour.
  - La v10 le donnait en quantile des rayons de fusion, une quantité sans unité.

### 3.6 Complétion et existence

- **Complétion.** Par défaut, par lignée : on rattache le site au cluster choisi que traverse la remontée de son
  premier point qualifié, s'il est unique. Elle est sans paramètre, équivariante et sans réunion nouvelle (synthèse
  § 5.4).
  - La PQ compte le bruit dans les objets comme une perte d'IoU ; à défaut de complétion, rendre le bruit en
    singletons (§ 1.2).
  - Le 1-NN de l'Algorithme 1 de la thèse (étape 9) est une complétion non laminaire : bras déclaré seulement.
- **Existence des clusters (P4, ouvert).** Sur LiDAR, un « point de bord » est typiquement un retour de la façade ou
  du sol résiduel, couvert par l'amas discret du vélo.
  - À mcs = 20, quelques points de bord ne font presque jamais à eux seuls passer un amas au-dessus du seuil. Le critère
    d'existence pèse donc surtout aux petits mcs, qui sont déconseillés ici.
  - Mesurer l'existence par cœur (`VC[coeur]` de la v10) contre l'existence par masse couverte, comme bras
    secondaire seulement.

## 4. Contrôles LiDAR spécifiques (c)

Tous sont calculables en temps linéaire, ou par table de contingence clairsemée instances × clusters : ni juge
O(n³), ni tableau par paire.

| Contrôle | Définition | Pourquoi | Repère existant |
| --- | --- | --- | --- |
| Clusters par trame | décomposés : total ; ≥ 50 points ; touchant le masque vrai des things (≥ 5 points) | la v10 comptait aussi le stuff | v10 : `cover` 5 732, HDBSCAN 867 (mcs = K = 5, 5 trames) |
| Fragmentation | par instance : nombre de clusters qui en couvrent ≥ 10 % ; part de ses points hors de son meilleur cluster | seule sur-segmentation qui coûte en PQ | — |
| Fusions | par cluster : nombre d'instances qu'il couvre à ≥ 10 % chacune ; paires fusionnées, par écart vrai | échec typique de HDBSCAN | criblage : 1,6 % des instances à < 0,1 m d'une autre |
| Taille des clusters | distribution en points et en étendue au sol ; part des points thing dans des clusters de moins de 50 points | miettes : pas des FP, mais une perte d'IoU | — |
| Contact sol | strate « sol résiduel à < 0,3 m », plus un bras avec sol (démo 04 et quelques trames) | Patchwork++ laisse du sol près de 30,5 % des instances | niveau B, k = 5 : 0,870 contre 0,860 |
| Objets à < 0,1 m | strate d'écart vrai < 0,1 m, et 0,1–0,3 m | rangées (§ 2.3) | niveau B, k = 5 : 0,703 contre 0,670 ; HDBSCAN échoue 19,6 % |
| Petites instances | 50–99 et 100–199 points (43 % des instances jointes) | sensibles à mcs et aux miettes | niveau B, k = 5 : +0,016 et +0,017 |
| Portée | < 10, 10–20, 20–30, ≥ 30 m | densité en $R^{-2}$ ; léger déficit au-delà de 30 m à petit k | § 2.2 |
| Anneaux par cluster | nombre d'anneaux couverts par cluster choisi (indice estimé par l'élévation) | phase « anneaux » de HDBSCAN (proposition R-H) | — |
| Stabilité de la sortie plate | PQ et ARI entre une trame et sa rotation requantifiée | la hiérarchie est stable à 3ε, la coupe plate non (§ 1.1) | relève de E2 |
| Équité | HDBSCAN sur la même machine, sous la même tête et en étiquettes natives ; `min_samples` = k | étiquettes de scikit-learn dépendantes de la machine (ex aequo, `np.argsort` instable ; CONTEXTE, v10) | — |

## 5. Expérience locale illustrative (d)

`scripts/experience_lidar.py`, `scripts/resume_experience.py`, `sorties/experience_lidar.txt` : 69 tâches, 34 s de
CPU.

**Montage.** Oracle exact de la tour (étage B), 25 à 42 sites par scène, aucune donnée KITTI.

- *Surfaces verticales* échantillonnées en anneaux : 100 mm le long de l'anneau, 220 mm entre anneaux.
- *Bruit de portée* : 15 mm selon la normale, 5 mm dans le plan.
- *Sol* : deux anneaux à 600 mm l'un de l'autre.
- *Répétitions* : trois graines par cellule.
- *Comparaison* : `cover`, `first`, $H^{r}_{k+1}$, `core` et HDBSCAN, sous la même tête. PQ-like : SQ × RQ, toutes
  les surfaces comptées comme segments vrais.

C'est un oracle de correction : ces chiffres n'établissent ni fréquence ni pente. Chaque cellule donne
clusters / groupes appariés / PQ-like, en moyenne sur les graines.

| Scène | k | Niveau B (tour ; HDBSCAN) | mcs = k, EOM z = 1 : `cover` | $H^{r}_{k+1}$ | HDBSCAN | mcs = 4, z = 1 : $H^{r}_{k+1}$ | HDBSCAN |
| --- | --- | --- | --- | --- | --- | --- | --- |
| mur 4 × 9 et objet lointain | 2 | 1,00 ; 1,00 | 15,0 / 0 / 0,00 | **2,0 / 2 / 1,00** | 5,0 / 0,3 / 0,13 | 2,0 / 2 / 1,00 | 5,0 / 1 / 0,29 |
| mur | 3 | 1,00 ; 1,00 | 2,0 / 2 / 1,00 (z = 3 : 7,7 / 0,3) | 2,0 / 2 / 1,00 | 2,0 / 2 / 1,00 (z = 3 : 5,0 / 1) | 2,0 / 2 / 1,00 | 2,0 / 2 / 1,00 |
| rang, g = 150 | 2 | 1,00 ; 0,33 | 9,0 / 0 / 0,00 | 9,0 / 0 / 0,00 | 9,0 / 0 / 0,00 | **2,7 / 2,3 / 0,80** | 3,0 / 0 / 0,00 |
| rang, g = 150 | 3 | 1,00 ; 0,34 | 9,0 / 0 / 0,00 | **2,7 / 2,3 / 0,80** | 3,0 / 0 / 0,00 | 2,7 / 2,3 / 0,80 | 3,0 / 0 / 0,00 |
| rang, g = 200 | 3 | 1,00 ; 0,33 | 9,0 / 0 / 0,00 | **3,0 / 3 / 1,00** | 4,3 / 0 / 0,00 | 3,0 / 3 / 1,00 | 2,7 / 0 / 0,00 |
| rang, g = 300 | 2 | 1,00 ; 1,00 | 9,0 / 0 / 0,00 | **3,0 / 3 / 1,00** | 9,0 / 0 / 0,00 | 3,0 / 3 / 1,00 | 3,0 / 3 / 1,00 |
| rang, g = 150 | 5 | 0,38 (`cover` 0,67) ; 0,37 | 3,3 / 1,7 / 0,34 | 2,0 / 0 / 0,00 | 0 / 0 / 0,00 | 2,0 / 0 / 0,00 | 0 / 0 / 0,00 |
| façade, d = 250 | 3 | 0,90 ; 0,87 | 7,3 / 0 / 0,00 | **2,7 / 1,7 / 0,73** | 5,0 / 0,3 / 0,11 | 2,3 / 1,7 / 0,74 | 3,7 / 0,7 / 0,22 |
| façade, d ≤ 120 | 2, 3 | ≤ 0,53 ; ≤ 0,52 (objet ≤ 0,43) | aucun objet apparié | aucun | aucun | aucun | aucun |
| sol, h = 50 et 150 | 2, 3, 5 | objet 0,53 à 0,62, sol 0,64, pour tous | objet apparié 0/6 (k = 2), 3/6 (k = 3) | 3/6 (k = 2), 6/6 (k = 3) | 0/6 (k = 2), 3/6 (k = 3) | objet 6/6 à k = 2 et 3 (4/6 à k = 5) | objet 6/6 |

Moyenne sur les familles à k = 3, en PQ-like, EOM z = 1 :

| mcs | $H^{r}_{4}$ | `first` | HDBSCAN | `core` | `cover` |
| --- | ---: | ---: | ---: | ---: | ---: |
| 3 | **0,562** | 0,506 | 0,264 | 0,253 | 0,153 |
| 4 | 0,563 | 0,543 | 0,306 | 0,214 | 0,554 |

Ce que l'expérience illustre :

1. **La sur-segmentation de `cover` à mcs = k est le mécanisme de la v10, et $H^{r}_{k+1}$ la corrige sur une surface
   régulière** : 15 clusters contre 2 sur le mur.
2. **HDBSCAN éclate les surfaces par anneaux** à k = 2, et à k = 3 en z = 3, même à mcs = 4 et 6 : c'est la
   proposition R-H.
3. **Rangées** : la tour sépare les objets à g = 150 et 200, HDBSCAN non (§ 2.3). La sortie plate en tire profit dès
   que mcs dépasse la taille des segments d'anneau, ou dès k = 3.
4. **À k = 5, de trop petits objets (9 sites) sont perdus par $H^{r}_{6}$** (niveau B 0,38, contre 0,67 pour
   `cover`) : k + 1 doit rester très en dessous de la taille des objets.
5. **Contact serré.**
   - *Façade à 120 mm ou moins* : aucune hiérarchie de densité ne sépare l'objet (niveau B de l'objet ≤ 0,43). C'est
     la limite de fond d'ARCHITECTURE § 4.6, retrouvée.
   - *Sol à 40 mm sous l'objet* : l'objet n'est retrouvé qu'avec une partie du sol (niveau B de 0,53 à 0,62 pour toutes
     les hiérarchies). Il est apparié à mcs ≥ 4.
   - *Le sol lui-même*, deux anneaux espacés de 600 mm, n'est jamais un segment unique ; la PQ-like plafonne donc
     à 0,26.

   Appariements par groupe vrai : `scripts/appariements_par_groupe.py`, `sorties/appariements_par_groupe.txt`.

## 6. Recommandation pour la mesure LiDAR de E1 (e)

### 6.1 Régimes et données

- **R0, sans sémantique.** Trame entière sans sol. La métrique, PQ_th sans sémantique, est définie au § 6.3.
  - *Cohortes déjà préparées* (sessions D et F) : 5 démos, 37 échecs, 20 témoins, 71 voisines.
  - *Ajout indispensable* : un tirage aléatoire de 40 trames parmi les 299 du criblage, graine publiée. Les échecs et
    les voisines sont choisis sur l'échec de HDBSCAN, ce qui gonfle l'effet. D'après le résumé de session v10 du
    2 octobre (rapport source absent du disque, chiffres non revérifiés), `cover` gagnait +0,0104 sur ces trames mais
    +0,0002 en pondérant sur les 299.
- **R1, sémantique oracle.** Étiquettes vraies ; une tour par sous-nuage de classe thing, comme la thèse (§ 5.4) et
  ALPINE.
  - *Témoins* : ALPINE avec et sans découpage par boîtes (seuils de l'article), et HDBSCAN par classe.
  - *Statut* : c'est une sonde, qui utilise des étiquettes (MESURE, axe 0), pas un résultat de système.
  - *Rapport* : PQ_th seul, car le stuff est parfait en sémantique oracle.
- **Bras avec sol.** Démo 04 et quatre trames du même tirage, en R0. Coût multiplié par 2 à 3 en sites.

### 6.2 Bras

| Facteur | Valeurs | Rôle |
| --- | --- | --- |
| hiérarchie | $H^{r}_{k+1}$ (κ = 1) ; $P_2\circ\Pi_{k+1}$ (κ = 2) ; `first` ; `cover` ; `core` ; arbre de HDBSCAN sous la même tête ; HDBSCAN natif de scikit-learn ; ALPINE sans sémantique (R0) ; ALPINE (R1) | équité et témoins |
| k | 3 et 5 (primaire) ; 2 et 10 (secondaire) | § 2.2 : gains croissants avec k, petits objets à k = 3 |
| mcs | 20 (primaire) ; 26 ; 10 ; k (diagnostic de sur-segmentation) ; √n seulement comme contrôle de l'évaluateur (P7) | § 3.4 |
| sélection | EOM z = 1 (primaire) ; EOM −log r ; EOM z = 3 (référence v10) ; feuilles ; EOM-boîte (R1) | § 3.5 |
| complétion | aucune (primaire) ; par lignée | § 3.6 |

**Cellule primaire, fixée d'avance.** $H^{r}_{k+1}$ contre l'arbre de HDBSCAN sous la même tête, k ∈ {3, 5},
mcs = 20, EOM z = 1, sans complétion, R0, PQ_th.

### 6.3 Métriques

- **PQ_th, RQ et SQ par classe (R1).** Protocole SemanticKITTI : min_points = 50, void exclu, bruit en singletons.
- **PQ_th sans sémantique (R0).**
  - *IoU* : calculée sur le cluster privé de ses seuls points void, comme dans l'évaluateur v11. Les points de stuff
    restent et pénalisent un vélo fusionné à sa façade.
  - *Vrai positif* : IoU > 1/2.
  - *Faux négatif* : instance évaluable (≥ 50 points) non appariée.
  - *Faux positif* : cluster non apparié qui contient au moins 50 points thing non void ; un cluster de façade ne
    compte pas.
- **IoU moyen apparié**, qui vaut 0 pour une instance non appariée.
- **Oracle C.** Meilleur cluster de l'arbre condensé par instance. Il distingue la perte due à la sélection (oracle C
  moins sortie plate) de celle due à la hiérarchie (niveau B moins oracle C). Selon la v10, « la perte est dans la
  sélection ».
- **Niveau B**, en rappel.
- **Contrôles du § 4**, par strate : classe, portée, taille, écart, contact.
- **Intervalles bootstrap** par trame, par blocs de trames consécutives pour les voisines, qui sont corrélées.

### 6.4 Prédictions écrites d'avance

| # | Prédiction | Raison |
| --- | --- | --- |
| P1 | à mcs = k = 5, EOM z = 1, R0 : nombre de clusters par trame de $H^{r}_{k+1}$ ≤ 0,5 fois celui de `cover`, et ≤ 2 fois celui de l'arbre de HDBSCAN | § 3.2 et 3.3 ; entrées à l'échelle de `core` |
| P2 | cellule primaire : PQ_th($H^{r}_{k+1}$) − PQ_th(HDBSCAN) ≥ −0,005 sur l'ensemble ; RQ_th($H^{r}_{k+1}$) > RQ_th(HDBSCAN) sur la strate {vélos ∪ contact structure ∪ écart < 0,3 m}, IC 95 % > 0 | § 2.2 et 2.3 |
| P3 | pour $H^{r}_{k+1}$ à mcs ∈ {10, 20} : PQ_th(−log r) ≥ PQ_th(z = 1) ≥ PQ_th(z = 3) | § 3.2, fixtures |
| P4 | écart de RQ_th entre $H^{r}_{k+1}$ et HDBSCAN : au plus 0,01 en valeur absolue au-delà de 30 m ; l'écart favorable concentré sous 20 m | § 2.2 |
| P5 | à k = 3, mcs = 20, au moins un des vélos rattrapés au niveau B (04-C, 01-55, 04-55) est apparié en sortie plate ; 01-B, 02-A, 02-B et 03-A restent non appariés pour toute règle de la tour | démos (plafond FULL < 1/2) |
| P6 | R1 : $H^{r}_{k+1}$ + EOM-boîte ≥ ALPINE avec découpage − 0,01 en PQ_th, et ≥ HDBSCAN + EOM-boîte | § 3.5 ; ALPINE est fort (sa PQ n'est saturée qu'à +4,3 par l'oracle d'instances sur nuScenes) |
| P7 | à mcs = round(√n), la part d'instances évaluables appariées par des clusters sans void ne dépasse 1 − f pour aucune méthode, où f est la part d'instances de $n_g\leq$ mcs/2 dans les trames tirées (28,3 % sur les 299 du criblage) ; les appariements par des clusters contenant du void sont comptés à part | L1, exact : contrôle de l'évaluateur |

### 6.5 Règles de décision

- **P2 échoue** ($H^{r}_{k+1}$ sous HDBSCAN avec un IC < 0) : selon la consigne du 28 septembre, c'est la tête tirée
  de la tour qui est à revoir. On compare d'abord les oracles C.
  - Oracle C de $H^{r}_{k+1}$ ≥ celui de HDBSCAN : la perte est dans la sélection (−log r, boîte, existence).
  - Sinon : c'est la condensation ou la projection qui perd.
- **P1 échoue** : la qualification ne pèse pas sur la sur-segmentation LiDAR. Retirer mcs = k de toute tête.
- **P3 échoue** : garder z = 1 seul.
- **P6 échoue** : appliquer la règle de MESURE § 10 (S7 ne bat pas l'excès de masse ou ALPINE : garder la sélection
  statistique, ne pas engager de coût appris).

### 6.6 Budget G4 (estimation, à confirmer par une première session)

Repère : la session F a traité 5 démos et 72 voisines en 1 285 s pour 4 ordres et 5 règles (niveau B). Les têtes
plates sont linéaires en nœuds par cellule ; leur coût réel reste à mesurer.

Découpage proposé, chaque session sous environ 2 300 s :

1. R0 : démos, témoins et tirage aléatoire de 40 trames ;
2. R0 : échecs et voisines ;
3. R1 (tours par classe, petits sous-nuages) et bras avec sol.

HDBSCAN tourne toujours sur la même machine que la tour, en scikit-learn 1.7.2 épinglé sur G4 (la tête commune est
validée ici en 1.9.1).

## 7. Lecture critique

### La thèse

- **Coût du § 5.2.** « Volume maximal d'un objet » est une information de **classe**. Au § 5.4, la thèse construit
  une hiérarchie par classe à partir d'une sémantique donnée : c'est le régime d'ALPINE, pas une segmentation
  d'instances sans sémantique. La thèse ne rapporte aucune PQ.
- **Masses fractionnaires du § 9.1.** Elles ne s'accordent ni au comptage en points de la PQ ni au lemme L2, dont les
  masses sont entières. Elles perdent déjà T0 à mcs 3.
- **Étape 9 de l'Algorithme 1** (réaffectation au plus proche voisin) : complétion non laminaire.
- **Dégradation avec K (§ 4.4.5).** La thèse attribue à la connexité de HDBSCAN la dégradation quand K croît. Sur
  LiDAR, au niveau B, c'est observé : de k = 5 à k = 10, HDBSCAN perd 0,066 sur les vélos, la tour 0,018.

### Zoltan (SPECIFICATION)

- **Seuil relatif α sans plancher.** Il ne peut pas retirer les scissions équilibrées d'une surface : amas et
  anneaux.
- **Poids $\psi=1/t^{3}$.** C'est un analogue du z = 3, une densité volumique : sur des surfaces, il surpondère les
  fragments à petits rayons, c'est-à-dire proches.
- **Règle `E-relative`** ($r/r_K(x)$) : elle va dans le sens de la sélection en −log r.
- **Mesure S7.** Le plan est bon, avec ses témoins EOM sur le même arbre et ALPINE, à mêmes prédictions sémantiques.
  Il manque d'y écrire le protocole de la PQ pour le bruit (identifiant 0), et le lemme L1 contre les mcs « en racine
  de n ».

### L'auditeur

- **Obstruction de Palm.** Elle ne se voit pas à ces tailles : au niveau B, `cover` et $H^{r}_{k+1}$ diffèrent d'au
  plus 0,0054 en moyenne par strate LiDAR.
- **Fermeture qualifiée.** Elle réunit les amas avant FULL, jusqu'à un facteur 2 en rayon : c'est le mauvais sens pour
  des objets en rang. Je n'ai pas mesuré son seuil de lacune, qui reste à établir si on la garde comme témoin dans E1.

### La v10

- **Les 5 732 clusters** sont un fait réel et bien expliqué, mais compté avec le stuff, et mesuré sans PQ.
- **« Le LiDAR veut mcs = K, z = 1, ε 1–2 »** (résumé de session v10 du 1er octobre, non revérifié) : ce constat
  repose sur 5 trames choisies. Il n'est pas jugé en PQ, et il ne tient pas compte du fait que les échecs sont rares.
- **z = 3 et mcs = √n** sont des réglages du synthétique : √n est nuisible sur LiDAR, et c'est prouvé (L1, 28,3 %).

### Le juge (synthèse § 8.2)

- **Grille de mcs de E1** ({k, 10, 20, √n}) : retirer √n pour le LiDAR, ajouter 26.
- **Prédiction « la qualification réduit la sur-segmentation de `cover` à mcs = k »** : soutenue par les fixtures,
  mais les amas de k + 1 sites et les anneaux laissent mcs = k mauvais quoi qu'il arrive.
- **« Le vélo C reste au-dessus de 0,5 à K = 3 »** : c'est un énoncé de niveau B. Il est tenu (0,557).

### Le développeur (§ 7 de la note)

Le « meilleur bloc » est pris sur tous les rayons : c'est un plafond plus optimiste que l'ensemble des clusters
sélectionnables. Ajouter l'oracle C.

### Ce rapport

- **Scènes miniatures.** Ce sont des grilles idéalisées, en petit nombre.
- **Strates G4.** Elles sont au niveau B, sur des cohortes non aléatoires.
- **Protocole de la PQ.** Je le cite de mémoire, d'où l'avertissement du § 1.2.
- **Aucune mesure de sortie plate sur trame réelle.** C'est l'objet de E1.

## 8. Statut des affirmations

| Énoncé | Statut | Lieu |
| --- | --- | --- |
| La tête commune reproduit scikit-learn sur l'arbre de HDBSCAN | vérifié (1 920 comparaisons, 0 écart) | § 0 |
| Borne 3√3/2 mm sous rotation exacte puis requantification à 1 mm | corollaire de H3 (prouvé), sous l'hypothèse de sites distincts | § 1.1 |
| Lemmes L1 et L2 ; mcs ≤ 26 sans perte au niveau B | prouvé ici | § 3.4 |
| Factorisation pour une mesure positive μ | prouvé ici (même preuve que la synthèse § 5.1) | § 3.4 |
| Proposition R-H (phase « anneaux » de HDBSCAN) | prouvé ici (grille infinie) | § 3.2 |
| Proposition R-T, k = 2 | preuve esquissée ; vérifiée sur F3 | § 3.2 |
| Absence de phase « anneaux » de la tour à k ≥ 3 | vérifié à k = 3 sur F3 ; conjecture en général | § 3.2 |
| Seuils de rangée : HDBSCAN g > a·s | prouvé | § 2.3 |
| Seuils de rangée de la tour (k = 2 : s(√(1+a²) − 1) ; k = 3 : s) | vérifiés exactement sur les cas calculés ; argument à k = 2 ; conjecture en général | § 2.3 |
| Aucun bloc d'une structure isolée de k sites sous $H^{r}_{k+1}$ | prouvé (note) ; illustré F1 | § 3.3 |
| Blocs des amas de k + 1 sites | contre-exemple exact F2 | § 3.3 |
| L'EOM-boîte est l'optimum exact | prouvé ici (faisabilité monotone) | § 3.5 |
| −log r ≥ z = 1 ≥ z = 3 sur trames réelles ; P1 à P6 | conjectures et prédictions | § 6.4 |
| Strates LiDAR | mesures existantes relues (niveau B, cohortes non aléatoires) | § 2.2 |
| Effets du protocole de la PQ sur les miettes et le bruit (identifiant 0) | cité de mémoire, à vérifier | § 1.2 |

## 9. Reproduction

Depuis `build/v11-points-select/lidar/`, avec `PYTHONDONTWRITEBYTECODE=1 python3 -B`. Python 3.12, numpy 2.5.3,
scikit-learn 1.9.1. Arbre de lecture `build/v11-claude-20261003` (ab1a739d1).

| Script | Sortie | Contenu |
| --- | --- | --- |
| `scripts/selftest_tete.py` | `sorties/selftest_tete.txt` | tête commune contre scikit-learn : 1 920 comparaisons, 0 écart |
| `scripts/criblage_stats.py` | `sorties/criblage_stats.txt` | tailles, loi taille-portée, écarts, contacts, échecs de HDBSCAN (§ 2.1) |
| `scripts/strates_lidar.py` | `sorties/strates_lidar.txt`, `.json` | contrôle du § 7 de la note, puis strates du niveau B (§ 2.2) ; archives extraites dans `donnees_g4/` |
| `scripts/demos_objets.py` | `sorties/demos_objets.txt` | objets suivis des démos, toutes règles, k = 2, 3, 5, 10 |
| `scripts/mcs_lemmes.py` | `sorties/mcs_lemmes.txt` | conséquences de L1 et L2 (§ 3.4) |
| `scripts/fixtures_mecanismes.py` | `sorties/fixtures_mecanismes.txt` | fixtures F0 à F3 (§ 3.2) ; argument `f4` : `sorties/fixtures_rang_f4.txt` |
| `scripts/seuil_rang_k3.py` | `sorties/seuil_rang.txt` | seuils de lacune des rangées (§ 2.3) |
| `scripts/experience_lidar.py`, `scripts/resume_experience.py` | `sorties/experience_lidar.json`, `.txt`, `_journal.txt` | expérience miniature (§ 5) |
| `scripts/experience_boite.py` | `sorties/experience_boite.txt` | tête EOM-boîte (§ 3.5) |
| `scripts/appariements_par_groupe.py` | `sorties/appariements_par_groupe.txt` | appariements par groupe vrai de l'expérience (§ 5) |
| `scripts/ecart_cover_margin.py` | `sorties/ecart_cover_margin.txt` | écart `cover` contre $H^{r}_{k+1}$ par strate ; soldes sauvetages moins pertes par cohorte (§ 2) |
| `scripts/comparaison_z.py` | `sorties/comparaison_z.txt` | z = 1, z = 3 et −log r, cellule par cellule (§ 3.5) |
| `scripts/timing_probe.py`, `scripts/timing_probe2.py` | (console) | temps de l'étage B : 21 s à n = 60, k = 2 |
| `pdftotext` sur le PDF d'ALPINE | `sorties/alpine_texte.txt` | tableaux 6, 8, 9 et § 3 cités |

**Rejeu.** Tous les scripts ont été relancés dans `verif_rejeu/`. Toutes les sorties texte sont identiques octet pour
octet, et `experience_lidar.json` est identique hors chronométrage.

Extraction des archives G4 :

- commande : `git show origin/main:morsehgp3D_v11/receipts/developpement_20261003/points_g4/sessions/<s>/results.tar.gz`
  pour `claudepts4` et `claudepts6`, puis `tar -xzf` dans `donnees_g4/<s>/` ;
- manifestes : `data_manifest_pts3.json` et `data_manifest_pts2_roles.json`, copiés par `git show`.
