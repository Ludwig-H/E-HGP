# Risques et réfutations

Ce qui peut faire échouer HGP-FM, comment on le détecte tôt, et ce qui est déjà
fermé. Document destiné à vieillir : chaque risque devient un fait ou
disparaît.

## 1. Les quatre risques majeurs

### R1 — la tour n'apporte rien qu'un canal de densité n'apporte déjà

*Le risque.* Le gain espéré vient peut-être simplement de ce que le modèle voit
la densité locale. Or il suffit de donner $\hat f_K(x)$ comme variable par
point à un réseau inchangé pour lui donner la densité.

*Détection.* Témoin **T2** de [`MESURE.md`](MESURE.md), phase 0. Une variable
d'entrée supplémentaire sur une recette existante : c'est le témoin le moins
cher et le plus tranchant du protocole.

*Si le risque se réalise.* Arrêter la voie architecturale et publier le canal
de densité comme résultat utile. Ce serait un résultat honnête et d'une réelle
valeur pratique.

### R2 — c'est la hiérarchie en général qui aide, pas celle-ci

*Le risque.* Toute hiérarchie multi-échelle aide un réseau 3D ; les superpoints
de SPT le montrent déjà, avec 212 k paramètres et des résultats de premier
plan. Le fait que notre hiérarchie soit canonique et exacte n'y change peut-être
rien.

*Détection.* Témoin **T1**, tour brouillée : mêmes niveaux, mêmes tailles
d'unités, points réaffectés au hasard dans une partition locale. Et la
comparaison directe de pureté à nombre d'unités égal contre SPT, porte 0.2.

*Si le risque se réalise.* La contribution se réduit à « une hiérarchie de plus,
mais canonique et déterministe ». C'est encore quelque chose — reproductibilité,
absence de réglage — mais ce n'est plus une thèse de modèle de fondation.

### R3 — les objets filiformes naissent trop tard

*Le risque.* À $K \geq 2$ il faut $K$ boules qui s'intersectent simultanément.
Une structure mince — poteau, tronc, panneau, barrière, cycliste — n'y parvient
qu'à grand rayon, où elle a peut-être déjà fusionné avec le sol ou la
végétation. Or c'est sur ces classes que se joue la marge de mIoU.

*Détection.* Plafond d'oracle **par classe** et par taille d'objet, porte 0.1.
Puis la prédiction P5 : le gain sur les classes filiformes doit dépendre du
calendrier de $K$ et **disparaître si l'on retire $K = 1$**.

*Parade.* $K = 1$ est le Single-Linkage, précoce sur les structures minces. Le
calendrier de $K$ selon la profondeur — $K$ petit aux niveaux fins — est la
parade naturelle, et elle doit être mesurée, pas supposée.

### R4 — la condensation réintroduit la constante, ou mange les objets minces

*Le risque.* La condensation est nécessaire (le § 9.1 la prescrit, et elle
fournit le squelette, la stabilité, les $\hat\lambda_x$ et la tête de
sélection). Mais `min_cluster_size` est **exactement le genre de constante
posée à la main** que tout ce dossier cherche à supprimer : un seuil en nombre
de points ne transfère ni d'un capteur à l'autre, ni du champ proche au champ
lointain. Et un seuil, quel qu'il soit, élague d'abord les branches de faible
masse, c'est-à-dire les poteaux lointains.

*Détection.* Porte 0.6 de [`MESURE.md`](MESURE.md) : plafond d'oracle **par
classe, avant et après condensation**, en fonction du seuil relatif $\alpha$.

*Parade.* **Un rapport transfère, une longueur non.** Le seuil doit être
relatif — une scission n'est validée que si chaque branche conserve au moins
une fraction $\alpha$ de la masse $m_\tau$ du parent. Si le plafond chute quand
même, garder deux échelles : une condensée pour le contexte, une brute pour les
niveaux fins.

*Trois pièges d'implémentation propres à HGP.* Ne pas **binariser les
multifusions** — la tour publie des événements à trois parents ou plus au même
niveau exact, et les binariser inventerait un ordre inexistant. **Coupler
$\alpha$ entre les ordres** et **revérifier la naturalité des cartes verticales
après condensation** : si une fusion est retirée à $K$ mais gardée à $K-1$, le
carré peut cesser de commuter. Enfin, pour $K \geq 2$ un point appartient à
plusieurs branches : le « niveau de sortie » est un événement partiel, et c'est
la masse $m_\tau$ du § 9.1 qui le gère, pas un comptage.

### R5 — l'avantage se referme avec l'échelle

*Le risque.* Un a priori structurel aide le plus quand les données manquent. Un
modèle de fondation est entraîné sur beaucoup de données. L'écart peut donc se
resserrer, voire s'inverser si la contrainte devient un carcan.

*Détection.* Axe 4 de [`MESURE.md`](MESURE.md) : trois tailles de modèle par
trois tailles de corpus, tendance publiée.

*Si le risque se réalise.* La revendication honnête devient **efficacité en
échantillons et transfert**, pas plafond asymptotique. C'est une revendication
défendable, à condition de ne pas avoir promis l'autre.

## 2. Risques de second rang

| risque | détection | parade |
| --- | --- | --- |
| Le recouvrement rend $P_\ell$ trop dense | porte 0.5, nombre de non-zéros par niveau | seuiller les poids $w_{x\tau}$ faibles ; mesurer la perte |
| L'échelle n'est pas réellement adaptative (le rayon ne varie pas avec la portée) | porte 0.5, histogramme de $r$ par tranche de portée | changer de règle de contraction ; si aucune ne l'est, la thèse centrale tombe |
| Le sol domine la hiérarchie en une composante géante | plafond d'oracle sur la classe « route », statistiques de niveau | l'axe $K$ ; mesurer aussi le régime sans sol, sans en faire le contrat |
| Les lots de trames ont des hiérarchies de formes différentes | ingénierie | cibles de compte par niveau, comme tout réseau épars |
| Le modèle apprend la portée plutôt que la forme | ablation du canal $\log r$, prédictions P1 et P2 | garder la portée explicite ; égaliser en augmentation |
| Le gain vient du budget de calcul | règle du budget apparié, prédiction P6 | comparaison à paramètres, époques et matériel égaux |
| L'axe des ordres ne sert à rien | bras S5, témoin T4 | le retirer et simplifier ; résultat négatif net, à publier |
| Une variable de filtration donnée en entrée fuit vers sa propre cible de pré-entraînement | revue de conception | ne jamais donner en entrée ce que l'on prédit au même moment |
| La tête de sélection apprise ne bat pas l'excès de masse sur le même arbre | bras S7 et son témoin | garder la sélection statistique ; le coût appris ne se justifie pas |
| Contamination du jeu de test | règle de découpage | val 08 seule ; le serveur n'est touché qu'une fois |

## 3. Risques de revendication

Aucune brique n'est nouvelle isolément : U-Net 3D, attention éparse
hiérarchique, partitions multi-échelles adaptatives, auto-distillation,
persistance multiparamètre, descripteurs radiaux ou sphériques, grilles de
distances à sondes fixes. Revendiquer l'une d'elles est une erreur qu'un
relecteur sanctionnera immédiatement.

Deux antériorités doivent être citées franchement et tôt :

- **Superpoint Transformer** (ICCV 2023) pour l'idée d'une partition
  hiérarchique adaptative consommée par une attention éparse. C'est
  l'antécédent architectural le plus proche ;
- **la bifiltration degré-Rips** (Lesnick et Wright 2015 ; Rolle et Scoccola,
  JMLR 2024) pour l'objet lui-même et pour son théorème de stabilité. Le
  projet ne découvre pas cet objet : il le calcule exactement à une échelle où
  la littérature ne le calculait pas, et il l'utilise comme squelette de calcul
  plutôt que comme invariant à vectoriser.

La revue d'antériorité de ce dossier est **ciblée sur les décisions de
conception**. Elle ne remplace pas une recherche exhaustive au moment de la
soumission, qui reste à faire une fois, sérieusement.

## 4. Pistes fermées, à ne pas rouvrir

| piste | fermée par |
| --- | --- |
| Consommer une seule tranche $\lambda$, un seul $r$ ou un seul $K$ | instable par Rolle–Scoccola ; l'architecture hériterait de l'instabilité |
| Sélectionner quelques milliers de jetons de la tour pour un Transformer plat | jette la hiérarchie, qui est la contribution ; et le cadrage du coût était faux ([`ARCHITECTURE.md`](ARCHITECTURE.md) § 2) |
| Vectoriser la persistance en variables d'entrée d'un réseau standard | voie TDA classique, largement explorée, et elle jette la structure |
| Écrire un réseau neuf de zéro plutôt qu'une modification de PTv3 | rend la substitution ininterprétable et le résultat invérifiable |
| Représenter un nœud par sa seule fonction support | $h_P = h_{\mathrm{conv}(P)}$ : aveugle à la non-convexité et aux trous |
| Représenter un nœud par une seule fonction radiale $\rho(u)$ | une direction peut ne rencontrer aucune couche ou plusieurs |
| Remplacer $P_v$ par son enveloppe convexe | le polyèdre est ouvert, non convexe, partiellement occulté |
| Forcer une partition stricte des points à l'entrée | pour $K \geq 2$ le recouvrement **est** l'information (§ 9.1) |
| Apprendre la partition (superpoints appris, $k$-moyennes différentiables) | on reperd la canonicité et le déterminisme, et l'ablation redevient ininterprétable |
| Condenser avec un `min_cluster_size` en **nombre de points** | c'est réintroduire la constante métrique que tout le dossier supprime ; le seuil doit être un **rapport** de masse |
| Binariser les multifusions pour réutiliser la condensation de HDBSCAN telle quelle | inventerait un ordre qui n'existe pas et détruirait la canonicité |
| Construire l'échelle sur la forêt **brute** | à $K=1$, 39 796 fusions pour 39 885 sites : les premiers niveaux n'absorberaient que des singletons |
| Atlas de cartes appris par nœud | coutures et ancres changent sous décimation ; isole mal l'effet du tokenizer |
| Champ implicite ajusté par polyèdre | coûteux et redondant, la surface est déjà explicite |
| Versionner des scans bruts ou des nuages dérivés KITTI | licence non commerciale, dépôt public ; seuls les manifestes |

Une piste ne se rouvre qu'avec un **fait nouveau** : une preuve, une fixture ou
une mesure épinglée qui contredit la raison de sa fermeture. Jamais sur un banc
d'essai.

## 5. Ce qui n'est pas un risque

- **La laminarité.** § 9.1 démontre que l'arbre est une partition des
  $(K-1)$-simplexes ; la partition de l'unité $w_{x\tau} = S_\tau/T_x$ relie
  points et facettes. Ce point a été soulevé et tranché.
- **Le déterminisme et la reproductibilité** de la tokenisation : mesurés.
- **L'équivariance rigide et d'échelle** : rotations, translations et
  homothéties commutent avec la tour, donc les augmentations correspondantes
  sont gratuites.
- **Le coût de séquence** : un U-Net lit $L$ coupes, pas les millions de nœuds
  de la tour.
