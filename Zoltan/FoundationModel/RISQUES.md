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

*Le risque.* Le gain peut venir d'une hiérarchie multi-échelle générique.
SPT fournit un précédent utile, sans démontrer que toute hiérarchie aide. Le fait que notre hiérarchie soit canonique et exacte n'y change peut-être
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

*Le risque.* Une condensation peut supprimer les faibles masses utiles ou
ne rien compresser. Un seuil absolu dépend de la densité acquise ; α relatif
au parent reste un hyperparamètre et filtre surtout le déséquilibre.
Dans un arbre binaire équilibré, α≤1/2 conserve toutes les scissions ; α>1/2
ne permet aucune bifurcation à deux branches lourdes.

*Détection.* Porte 0.6 : compression et profondeur, masse retenue,
puis plafond d'oracle par classe avant/après condensation. Couvrir aussi les
arbres équilibrés et les continuations de faible masse.

*Parade à mesurer.* Comparer coupes brutes, seuil local, référence de masse
gelée et critère de durée ; déclarer chacun séparément. Garder réserves et
sorties par incidence. Une quantité sans dimension ne garantit pas le transfert.

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
| Le recouvrement rend $P_\ell$ trop dense | porte 0.5, nombre de non-zéros par niveau | sparsification déclarée avant gel, masse retirée en réserve ; contrôler composition et perte |
| L'adaptativité suit l'acquisition sans améliorer la représentation | porte 0.5, rayon, couverture et masse conservée par portée | comparer les règles de contraction et un témoin local fort |
| Le sol domine la hiérarchie en une composante géante | plafond d'oracle sur la classe « route », statistiques de niveau | comparer au brut le régime non-sol avec branche sol/contexte et raccord vers tous les IDs |
| Les lots de trames ont des hiérarchies de formes différentes | ingénierie | cibles de compte par niveau, comme tout réseau épars |
| Le modèle apprend la portée plutôt que la forme | ablation du canal $\log r$, prédictions P1 et P2 | garder la portée explicite ; égaliser en augmentation |
| **La quantification à 1 mm casse l'équivariance** : `tourner → quantifier → tour` ne commute pas, et un prédicat exact peut basculer sur une égalité | porte 0.7, effets sur affectations et sorties utiles | recalculer par vue ; comparer les précisions si la dérive est pénalisante, avec coût publié |
| FM-5 impose une structure incompatible avec l'occultation | contre-exemple K1 du protocole de guidage, contrôles des IDs/supports communs | ne pas imposer l'égalité des arbres recalculés ; déclarer la couverture et l'incertitude |
| Les objets mobiles et erreurs de recalage altèrent l'agrégat de FM-6 | contrôles de visibilité et de mouvement, régime temporel séparé | déclarer fenêtres, causalité et pondération des observations ; apparier accès aux trames/odométrie |
| Une représentation constante satisfait l'accord ou un prior dépendant du rayon résout les relations | témoin encodeur constant/rayon seul, Brier conditionnel Y_V=0, puis évaluation aval | conserver la diversité SSL ; mesurer le gain de contexte au-delà des statistiques visibles |
| Payer $K \leq 10$ alors que $K \leq 6$ suffirait | porte 0.8 | fixer $K_{\max}$ sur la mesure, pas sur le domaine du moteur |
| Le gain vient du budget de calcul | FLOPs, préparation/cache, mémoire et débit mesurés | comparer à exposition égale puis à coût total égal ; paramètres/époques seuls ne suffisent pas |
| L'axe des ordres ne sert à rien | bras S5, témoin T4 | le retirer et simplifier ; résultat négatif net, à publier |
| Une variable de filtration donnée en entrée fuit vers sa propre cible de pré-entraînement | revue de conception | ne jamais donner en entrée ce que l'on prédit au même moment |
| La tête de sélection apprise ne bat pas l'excès de masse sur le même arbre | bras S7 et son témoin | garder la sélection statistique ; le coût appris ne se justifie pas |
| Contamination des partitions | manifestes SSL/enseignants/caches et blocs temporels | exclure 08 et test des ajustements du corpus primaire ; bilan sur 08 après choix sur blocs d'entraînement |

### Risques révélés par le réaudit global

| risque | conséquence | choix concret |
| --- | --- | --- |
| Un seul support minimal est pris comme géométrie du token | un réétiquetage peut changer les features d'une même boule | définir toutes les incidences de supports, les boules datées et la mesure ; garder les retours comme socle |
| Γ est interprété comme similarité de distributions | des distributions identiques diffuses semblent dissemblables | déclarer coaffectation ou comparer une cible de recouvrement explicitement différente |
| Le DP optimise une somme d'IoU locaux | fragmentation même avec des scores parfaits | objectif avec rejet/cardinalité ou apprentissage structuré ; contrôler le vote final |
| La clique des frères est appelée éparse | coût quadratique aux multifusions | hubs O(m), avec effet du goulot et des sauts mesuré |
| Un biais est ajouté au chemin FlashAttention standard | changement de noyau confondu avec gain HGP | mêmes noyaux pour S4, coût du changement publié |
| Le budget de 160k tokens est réutilisé pour chaque K | coût réel multiplié par les branches | budget global, niveau point partagé, réserves et arêtes inclus |

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

## 4. Choix du pilote et exclusions justifiées

Distinguer perte d'information démontrée et choix de priorité. La table
précédente fermait trop de variantes pour des raisons non établies.

| variante | décision |
| --- | --- |
| Transformer plat sur quelques tokens | témoin possible ; le pilote conserve les événements et compare le coût |
| Résumé vectoriel de filtration | témoin peu coûteux possible ; il mesure la valeur du résumé face au graphe |
| Réseau entièrement neuf | différé pour faciliter reproduction et attribution des effets |
| Fonction support seule | identique sur un ensemble et son enveloppe convexe ; ne distingue pas leurs trous |
| Fonction radiale à une seule couche | perd les couches multiples ; représentation insuffisante sans autres canaux |
| Enveloppe convexe | autre réalisation, à nommer ; elle n'est pas l'union des supports ni la surface physique |
| Partition stricte des points | ne remplace pas silencieusement le recouvrement de K supérieur ; témoin déclaré possible |
| Tokenizer appris | concurrent possible, déterministe une fois figé selon son exécution ; sa dépendance aux poids est différente |
| Seuil absolu de masse | témoin à comparer au relatif, sans garantie de transfert pour aucun |
| Binarisation des multifusions exactes | interdite si elle invente une priorité entre événements simultanés |
| Comptage seul des fusions | diagnostic insuffisant de masse ou d'adaptativité ; coupes brutes restent la référence |
| Atlas ou champ implicite appris | différé : coût et apport propre à établir ; les supports HGP ne donnent pas déjà une surface physique |
| Scans bruts/nuages KITTI versionnés | rester aux manifestes autorisés dans ce dépôt |

Une variante se décide par contrat et comparaison utile. Une absence de
nouveauté isolée n'interdit pas d'en faire un témoin.

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
- **La consommation de toute la tour par une attention dense** n'est pas
  imposée par le projet. Les budgets des coupes, branches, réserves et arêtes
  restent toutefois des risques à mesurer, en plus de FULL et de son export.
