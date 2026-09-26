# Risques et réfutations

Ce qui peut faire échouer HGP-FM, comment on le détecte tôt, et ce qui est déjà
fermé. Document destiné à vieillir : chaque risque devient un fait ou
disparaît.

Le [protocole de guidage](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md) distingue
les risques du tokenizer de ceux d'un enseignant FULL hors ligne. Le pilote
primaire reste mono-scan ; FM-6 est une extension temporelle.

## 1. Les six risques majeurs

### R1 — la tour n'apporte rien qu'un canal de densité n'apporte déjà

*Le risque.* Le gain espéré peut venir de statistiques locales simples.
$\hat f_K(x)$ donne une concentration ambiante, sans identifier à lui seul
une densité physique de surface. Le témoin doit aussi tester rayons K-NN,
comptes multi-rayons et anisotropie, à canaux d'acquisition appariés.

*Détection.* Témoin **T2** de [`MESURE.md`](MESURE.md), préparé avant
l'apprentissage puis évalué sur la même recette. Ces variables locales
forment un témoin appris peu coûteux à définir et discriminant.

*Si le risque se réalise.* Simplifier la variante architecturale concernée,
puis tester séparément FULL enseignant. Une égalité dans ce bras et ce budget
ne réfute pas tous les usages de la tour.

### R2 — c'est la hiérarchie en général qui aide, pas celle-ci

*Le risque.* Toute hiérarchie multi-échelle aide un réseau 3D ; les superpoints
de SPT le montrent déjà, avec 212 k paramètres et des résultats de premier
plan. Le fait que notre hiérarchie soit canonique et exacte n'y change peut-être
rien.

*Détection.* Témoin **T1**, hiérarchie locale de budgets appariés, avec
couverture, quotients et composition valides. Une réaffectation qui casse les
parents n'est pas ce témoin. Ajouter la comparaison de pureté à nombre
d'unités égal contre SPT, porte 0.2.

*Si le risque se réalise.* Limiter la revendication de structure exacte dans
le bras étudié ; comparer les rôles enseignant et tokenizer avant de conclure
sur le projet entier. Canonicité ne signifie pas absence de réglage du réseau.

### R3 — les objets filiformes naissent trop tard

*Le risque.* À $K \geq 2$ il faut $K$ boules qui s'intersectent simultanément.
Une structure mince — poteau, tronc, panneau, barrière, cycliste — n'y parvient
qu'à grand rayon, où elle a peut-être déjà fusionné avec le sol ou la
végétation. Or c'est sur ces classes que se joue la marge de mIoU.

*Détection.* Plafond d'oracle **par classe** et par taille d'objet, porte 0.1.
Puis la prédiction P5 : comparer les branches K avec et sans K1, en publiant
couverture et budget. L'oracle par nœud unique ne borne pas une sortie point
avec connexion fine.

*Parade.* Le pilote conserve des branches K autonomes, dont K1, et la
connexion fine des retours. Mesurer leur apport ; un calendrier géométrique
emboîtant inter-K ne garantit pas la composition des poids du réseau.

### R4 — la condensation réintroduit la constante, ou mange les objets minces

*Le risque.* La condensation du §9.1 fournit squelette, stabilité et sorties
par incidence pour la tête de sélection. Mais `min_cluster_size` est **exactement le genre de constante
posée à la main** que tout ce dossier cherche à supprimer : un seuil en nombre
de points ne transfère ni d'un capteur à l'autre, ni du champ proche au champ
lointain. Et un seuil, quel qu'il soit, élague d'abord les branches de faible
masse, c'est-à-dire les poteaux lointains.

*Détection.* Porte 0.6 de [`MESURE.md`](MESURE.md) : plafond d'oracle **par
classe, avant et après condensation**, en fonction du seuil relatif $\alpha$.

*Parade à mesurer.* Tester un seuil relatif α et la règle 0/1/plusieurs
branches lourdes définie par le [contrat](CONTRAT_COUPES_ET_MASSES_20260926.md).
Le caractère sans dimension ne garantit pas le transfert. Comparer à une
échelle brute, et conserver les réserves partielles ainsi que les sorties
par incidence.

*Trois pièges d'implémentation propres à HGP.* Ne pas **binariser les
multifusions** — la tour publie des événements à trois parents ou plus au même
niveau exact, et les binariser inventerait un ordre inexistant. **Coupler
$\alpha$ entre les ordres** et **revérifier la naturalité des cartes verticales
après condensation** : si une fusion est retirée à $K$ mais gardée à $K-1$, le
carré peut cesser de commuter. Enfin, pour $K \geq 2$ un point appartient à
plusieurs branches : le « niveau de sortie » est un événement partiel, et c'est
la masse $m_\tau$ du § 9.1 qui le gère, pas un comptage.

### R5 — la densité ne sépare pas ce qui se touche

*Le risque, et c'est le plus fondamental.* La tour sépare par la densité, sans
normale ni géométrie différentielle. Une voiture posée sur l'asphalte, un
poteau dans l'herbe, un piéton sur la chaussée peuvent former des ponts de
retours. Un ordre $K$ peut les rompre ou non selon le scan ; aucun résultat
général ne garantit une séparation avant disparition de l'objet. Mesurer le
plafond d'oracle aux contacts avec le sol.

*Détection.* Porte 0.9 de [`MESURE.md`](MESURE.md) : plafond d'oracle par
classe, **stratifié par contact avec le sol**, en fonction de $K$, avec et sans
retrait du sol.

*Parades à comparer.* L'**axe K** peut rompre certains ponts ; mesurer si
l'objet reste représenté au rayon concerné. Le **sans-sol** est une variante
légitime et prioritaire : comparer tour brute et tour non-sol avec branche
sol/contexte, raccordées à tous les IDs. Le masque est géométrique, figé sur
la trame entière avant les vues ; garder les unknown, payer son coût et ses
erreurs, sans assimiler le sol à une classe sémantique. Ce régime ne remplace
pas le contrat brut. Le **relèvement métrique** en $(x,y,z,\lambda n)$ reste
exploratoire ; il exige un moteur distinct et une mesure des naissances
(porte 0.10), sans héritage automatique de la spécification 3D.

### R6 — l'avantage se referme avec l'échelle

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
| L'adaptativité suit l'acquisition sans améliorer la représentation | porte 0.5, rayon, couverture et masse conservée par portée | comparer les règles de contraction et un témoin local fort |
| Le sol domine la hiérarchie en une composante géante | plafond d'oracle sur la classe « route », statistiques de niveau | comparer au brut le régime non-sol avec branche sol/contexte et raccord vers tous les IDs |
| Les lots de trames ont des hiérarchies de formes différentes | ingénierie | cibles de compte par niveau, comme tout réseau épars |
| Le modèle apprend la portée plutôt que la forme | ablation du canal $\log r$, prédictions P1 et P2 | garder la portée explicite ; égaliser en augmentation |
| **La quantification à 1 mm casse l'équivariance** : `tourner → quantifier → tour` ne commute pas, et un prédicat exact peut basculer sur une égalité | porte 0.7, effets sur affectations et sorties utiles | recalculer par vue ; comparer les précisions si la dérive est pénalisante, avec coût publié |
| FM-5 impose une structure incompatible avec l'occultation | contre-exemple K1 du protocole de guidage, contrôles des IDs/supports communs | ne pas imposer l'égalité des arbres recalculés ; déclarer la couverture et l'incertitude |
| Les objets mobiles et erreurs de recalage altèrent l'agrégat de FM-6 | contrôles de visibilité et de mouvement, régime temporel séparé | déclarer fenêtres, causalité et pondération des observations ; apparier accès aux trames/odométrie |
| Une représentation constante satisfait l'accord régional | variance/rang des features, distributions des prédictions et évaluation aval | conserver les mécanismes de diversité et les cibles relationnelles ; une cible dense ne suffit pas |
| Payer $K \leq 10$ alors que $K \leq 6$ suffirait | porte 0.8 | fixer $K_{\max}$ sur la mesure, pas sur le domaine du moteur |
| Le gain vient du budget de calcul | FLOPs, préparation/cache, mémoire et débit mesurés | comparer à exposition égale puis à coût total égal ; paramètres/époques seuls ne suffisent pas |
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

Deux filiations doivent être explicites :

- **Superpoint Transformer** (ICCV 2023) pour l'idée d'une partition
  hiérarchique adaptative consommée par une attention éparse. C'est
  l'antécédent architectural le plus proche ;
- **Morse HGP 3D** pour l'objet mathématique, ses supports critiques et
  l'algorithme FULL. La contribution du présent projet est de transformer
  cette sortie exacte en opérateurs de réseau vérifiables, puis de mesurer leur
  apport propre.

La revue d'antériorité de ce dossier est **ciblée sur les décisions de
conception**. Elle ne remplace pas une recherche exhaustive au moment de la
soumission, qui reste à faire une fois, sérieusement.

## 4. Pistes fermées, à ne pas rouvrir

| piste | fermée par |
| --- | --- |
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
| Choisir l'échelle en comptant aveuglément les fusions brutes | ce compte peut surpondérer les micro-fusions ; les coupes globales brutes restent la référence valide du pilote, puis la condensation se mesure contre elles |
| Atlas de cartes appris par nœud | coutures et ancres changent sous décimation ; isole mal l'effet du tokenizer |
| Champ implicite ajusté par polyèdre | coûteux et redondant, la surface est déjà explicite |
| Versionner des scans bruts ou des nuages dérivés KITTI | licence non commerciale, dépôt public ; seuls les manifestes |

Une piste ne se rouvre qu'avec un **fait nouveau** : une preuve, une fixture ou
une mesure épinglée qui contredit la raison de sa fermeture. Jamais sur un banc
d'essai.

## 5. Trois erreurs de conception déjà commises et corrigées

Elles figurent ici parce qu'elles sont instructives, et pour que personne ne les
refasse.

1. **« Il faut sélectionner quelques milliers de jetons parmi 16 M nœuds. »**
   Faux cadrage : un U-Net lit $L$ coupes, soit l'ordre de grandeur d'un U-Net
   3D ordinaire. Corrigé en [`ARCHITECTURE.md`](ARCHITECTURE.md) § 2.
2. **« Le chemin diagonal iso-densité est le bon défaut. »** Faux comme
   contrat général : faire croître $K$ avec $r$ ne garantit pas l'emboîtement,
   donc aucune application de pooling dure n'est garantie. Grossir avec
   garantie, c'est $r \uparrow$ et $K \downarrow$. L'iso-densité reste une
   famille **latérale** à comparer par OM.
3. **« Rotations, translations et homothéties commutent avec la tour, donc ces
   augmentations sont gratuites. »** Vrai de l'objet, faux du moteur : la
   quantification à 1 mm change l'accrochage à la grille. Corrigé en § 7.5 ; on
   recalcule, et la dérive devient une mesure utile (porte 0.7).

Une quatrième correction, de sens : une trajectoire géométrique emboîtante
autorise K décroissant et rayon croissant. Elle ne prouve pas la compatibilité
des poids entre K ; le pilote garde donc des branches autonomes.

## 6. Ce qui n'est pas un risque

- **La laminarité.** § 9.1 démontre que l'arbre est une partition des
  $(K-1)$-simplexes ; la partition de l'unité $w_{x\tau} = S_\tau/T_x$ relie
  points et facettes. Ce point a été soulevé et tranché.
- **Le déterminisme de FULL** : mesuré sur les cas qualifiés. La
  reproductibilité du tokenizer dérivé reste une porte à fermer.
- **Le pooling dur à K fixé, sous les conditions du contrat** : univers et
  poids gelés, quotients valides, fibres complètes et réserves préservées.
  Les naissances tardives et changements de K ne satisfont pas automatiquement
  ces conditions ; les retours peuvent toujours recouvrir plusieurs tokens.
- **La longueur de séquence envisagée** : un U-Net lirait $L$ coupes, pas les
  millions de nœuds de la tour. Le coût de FULL, des coupes et du graphe reste
  à mesurer séparément.
