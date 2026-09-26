# Réaudit de tout Zoltan — décisions pour une architecture LiDAR utile

26 septembre 2026. Base inspectée :
`d0e711e23617a7fa2838cac9b370ff125d4be5fb`.
Périmètre : les **52 fichiers présents sous Zoltan/** à cette base,
inventoriés avec taille et SHA256 dans le
[reçu](receipts/reaudit_global_20260926/README.md).

Hypothèse de travail : **Morse HGP 3D v9 suit exactement sa spécification**.
L'audit porte sur les idées et l'interface vers le réseau, sans rouvrir la
qualification du moteur. Zoltan désigne la collaboration avec **Zoltán Kató**.
L'objet mathématique utilisé est celui de Morse HGP 3D.

```text
phase=conception_modele_fondation_hors_registre
producer_assumption=morsehgp3D_v9_conforme
mode=reaudit_global_zoltan
public_status=not_claimed
native_engine_run=false
learning_run=false
gcp_used=false
```

## 1. Avis d'ensemble

**Le projet mérite un pilote, avec une priorité au guidage enseignant.**
FULL fournit des relations exactes, des histoires et des correspondances
qui peuvent superviser des représentations sans ajouter FULL à l'inférence.
Ce bras demande moins de choix architecturaux que HGP-UNet et permet de
mesurer rapidement la valeur du signal. Il ne démontre pas à lui seul
l'intérêt d'ordres supérieurs : à K1, l'objet est le single-linkage.

**La proposition HGP-UNet reste cohérente sous les contrats de coupes
déjà écrits**, mais plusieurs garanties prêtées à ses composants étaient
fausses. Trois défauts pouvaient invalider directement une expérience :
géométrie du token dépendant d'un support arbitraire, objectif SEL favorable
aux fragments, prétexte résolu par le rayon seul. Les autres constats
changent surtout l'ordre de construction et les témoins.

La présentation avec Kató formule une hypothèse intéressante : normaliser
la forme et garder séparément métrique et acquisition. Elle ne prétend pas
déjà reconstruire la surface cachée. Le travail utile est de rendre
**canonique et mesurable la réalisation encodée**, puis de tester son apport
au-delà des retours et de leurs statistiques simples.

### Décisions à prendre avant le réseau complet

| priorité | constat | décision incorporée |
| --- | --- | --- |
| P0 | un support minimal représentant ne définit pas le token | réalisation et mesure explicites ; retours comme socle |
| P0 | la somme d'IoU locaux fragmente même avec scores parfaits | SEL différé, objectif d'instance à définir ; DP corrigé |
| P0 | le rayon seul peut résoudre le prétexte | témoin constant/rayon, gain conditionnel et évaluation aval |
| P1 | α relatif au parent ne garantit pas la compression | comparer brut, référence gelée et durée |
| P1 | clique de frères et étoile d'événement n'ont ni le même coût ni le même opérateur | graphe réellement épars, effet du goulot mesuré |
| P1 | biais normalisé et probabilités prédites ne sont pas des ultramétriques | contrats séparés ; CDF monotone par paire proposée |
| P1 | le coût des branches K et du noyau d'attention était sous-spécifié | budget total et témoins sur mêmes noyaux |
| P1 | Γ est une coaffectation, pas une similarité de distributions | cible nommée ; recouvrement alternatif explicite |

## 2. Géométrie du token : préserver la canonicité jusqu'aux features

Le [catalogue natif](../../morsehgp3D_v9/src/tower/forest/ball_data.hpp) conserve
boule, niveau, q_min, intérieurs et coquille. Il ne fournit pas déjà une
surface physique ni une géométrie canonique construite à partir d'un seul
support représentant. La
[table de coquille](../../morsehgp3D_v9/src/tower/forest/local_plateau.hpp)
énumère plusieurs supports minimaux possibles.

**Contre-exemple exact.** Les quatre sommets (±1,±1,0) ont la même boule de
centre 0 et rayon carré 2. Chacune des deux diagonales la certifie par un
support q2. À la sonde (1,1,0), la distance carrée au support choisi vaut
0 ou 2 ; le moment normalisé xy vaut +1/3 ou −1/3. Boule et FULL sont
inchangés, features différentes. Le choix par ID ne répare pas cela sous
réétiquetage.

**Construction proposée.** Fixer l'ensemble des boules datées qui alimentent
chaque snapshot, puis comparer :

1. statistiques des retours couverts, avec multiplicité par site/retour déclarée ;
2. histogramme de q_min **par boule**, puis en option union de tous les
   supports positifs minimaux ;
3. surfels estimés à partir des retours, avec incertitude et visibilité.

L'union des deux diagonales du carré diffère du carré rempli conv(U).
Cette dernière construction reste possible sous un autre identifiant.
La signature q_min ne compte pas tous les supports. Les snapshots sans
support gardent le canal retours et un masque ; pas de distance zéro inventée.

**La mesure est un second choix.** Une somme de mesures sur les primitives
compte les recouvrements avec multiplicité ; la mesure de leur union ne le
fait pas. Les moments doivent déclarer cette distinction. Un repère PCA
introduit des ambiguïtés aux valeurs propres multiples : commencer dans
le repère capteur, avec forme normalisée et métrique séparées.

**Conseil pratique.** Les 512 distances dominent le descripteur. Commencer
avec retours, moments et signature d'arité ; ajouter les sondes seulement
si elles améliorent le résultat à cache et calcul publiés. Leur distance
échantillonnée ne certifie pas l'égalité des formes. [Contrat complet](JETON.md).

## 3. Condensation : une règle locale n'est pas un budget global

La règle α relative à la masse du parent conserve chaque bifurcation d'un
arbre binaire équilibré pour α≤1/2. Avec huit feuilles, les quinze nœuds
restent présents. Pour α>1/2, aucune scission ne peut garder deux branches
lourdes. À profondeur d, une feuille de masse relative 2^−d peut survivre :
**ni masse minimale globale ni compression ne sont garanties**.

Cela n'invalide pas l'opérateur : il filtre les déséquilibres. Cela invalide
l'argument selon lequel il éliminerait automatiquement les micro-fusions.
Un compte proche de n−1 fusions ne renseigne pas sur leur équilibre.

Comparer trois choses distinctes : coupes brutes ; α local ; seuil référé
à une masse gelée ou ordre de contraction par durée. Mesurer profondeur,
compression, réserves et survie des structures minces. Aucun seuil n'est
écarté pour sa seule unité. Une multifusion reste atomique ; le même α à
tous les K ne prouve pas la naturalité des quotients.
[SPECIFICATION](SPECIFICATION.md), [contrat antérieur](CONTRAT_COUPES_ET_MASSES_20260926.md).

## 4. SEL : le problème est l'objectif, avant l'optimiseur

Pour un score additif à maximiser, la récurrence est :

```text
V(C) = max(g(C), somme des V(enfants))
```

Comparer le parent aux scores bruts des enfants est incorrect. Un parent
de score 9, deux enfants de score 4, et quatre petits-enfants de score 3
ont pour optimum 12. La récurrence erronée choisit 9.

**Même le bon DP peut optimiser la mauvaise cible.** Si un nœud représente
parfaitement une instance G et que ses enfants partitionnent G, l'IoU du
parent vaut 1 et la somme des IoU des enfants aussi. Un départage vers les
enfants fragmente l'objet. Des scores appris parfaits ne résolvent rien.

Il faut définir objet, fond/rejet, cardinalité et couverture. Un coût par
objet peut corriger les égalités mais créer un biais de fusion. Un
apprentissage structuré sur les sélections complètes est une autre piste.
À K supérieur, les supports en retours se recouvrent : le vote peut changer
quand un autre nœud est sélectionné. La qualité après vote est généralement
non additive ; elle n'hérite pas automatiquement du DP linéaire.

**Décision : SEL devient une extension après le pilote sémantique.**
Comparer à l'excès de masse sur le même arbre, puis à ALPINE à mêmes
prédictions sémantiques. L'antichaîne est une contrainte utile sur les
facettes, pas une garantie d'une instance par objet.
[ARCHITECTURE, SEL](ARCHITECTURE.md).

## 5. Attention : choisir l'opérateur et payer le vrai réseau

### Événements épars et biais

Une multifusion à m branches induit une clique à m(m−1)/2 arêtes si chaque
frère parle directement à tous les autres. Un hub d'événement utilise
m incidences et deux passages, mais agrège l'information dans un goulot.
La fixture scalaire démontre qu'un agrégat partagé ne reproduit pas une
attention pair-à-pair générale.

Proposition : forêt réduite des événements reliant les états consommés,
messages aller/retour, plus canal local. Publier nombre de sauts, arêtes
et qualité des détails. Pour n états représentés, la forêt réduite sans
chaînes unaires superflues a O(n) incidences ; cela ne borne pas le coût
d'extraction depuis FULL ni toutes les arêtes ajoutées entre K.

Les rayons de fusion à K fixé définissent une ultramétrique sur une
antichaîne appropriée, avec diagonale zéro et horizon explicite. Le biais
φ(log(r_uv/r_u)) est en général asymétrique. Les niveaux 2,4,4 et les rayons
propres 1,3/2,1 donnent 4>max(2,8/3) après normalisation :
l'inégalité forte est perdue. **Aucun élagage automatique des logits n'en
découle.** La covariance d'échelle du rapport reste une propriété distincte.

### Intégration réelle dans PTv3

La [source officielle](https://github.com/Pointcept/PointTransformerV3/blob/main/model.py)
emploie SerializedPooling et SerializedUnpooling par indices inverses et
skip. Remplacer une interpolation K-NN n'était donc pas le bon raccord.
CPE et sérialisation gardent leurs coordonnées/grilles après remplacement
du pooling.

Le chemin FlashAttention par défaut refuse le RPE explicite. Pour S4,
comparer zéro, XYZ et HGP avec le **même noyau compatible**, puis publier
la comparaison système avec les meilleurs noyaux. Sinon la latence et le
gain mélangent deux interventions. [Table de substitution](ETAT_DE_LART.md#3-le-point-de-substitution-exact-dans-ptv3).

### Le budget doit additionner les branches

Le schéma 120k points → 30k → 7,5k → … totalise environ 160k unités
pour **une branche**. Quatre branches partageant les points donnent environ
280k ; si elles dupliquent aussi ce niveau, environ 640k. Réserves et
hubs s'ajoutent, ainsi que les arêtes OM. La correction n'interdit pas OM ;
elle impose un témoin **K1 répété à budget égal**. Une permutation fixe
des noms K est seulement un réétiquetage apprenable.

## 6. Guidage : apprendre le contexte plutôt que réussir la requête

### Un prétexte exact peut se résoudre sans features

Considérer deux groupes de rayons avec prévalences positives 1/10 et 9/10.
Le corpus total est équilibré. Une tête qui ignore les features et utilise
seulement le rayon atteint pourtant **90 % d'accuracy** et Brier 0,09,
contre 0,25 pour le prior global.

Ajouter les témoins encodeur constant + même tête, rayon seul, statistiques
locales et connexité sur la vue élève V. Sous l'inclusion déclarée V⊂T,
Y_V=1 donne déjà Y_T=1 ; l'information nouvelle est sur **Y_V=0**.
Publier la masse de cette strate et le gain au-delà des priors visibles,
sur la même loi de requêtes. Rééquilibrer artificiellement les labels change
la calibration ; conserver une évaluation représentative ou des poids
de sélection corrects.

La perte structurelle supervise une tête auxiliaire. Elle ne doit pas
forcer les distances des embeddings à être les distances de fusion :
route et voiture peuvent être connectées, deux voitures séparées peuvent
partager une classe. La preuve d'utilité reste la représentation aval.

### CDF proposée et portée probabiliste

Une tête peut produire **une distribution fixe par paire/vue** sur des
intervalles de rayon, plus une queue au-delà de H. Le rayon interrogé
sélectionne un cumul, sans changer cette distribution. Cela impose la
monotonie aux seuils déclarés et respecte la censure. Une fusion inconnue
après H n'est pas une fusion à H.

Cela n'impose pas une hiérarchie dure aux probabilités. Un mélange 50/50
de {ij}|k et i|{jk} donne (p_ij,p_jk,p_ik)=(1/2,1/2,0), parfaitement valide.
Forcer p_ik≥min(p_ij,p_jk) rejetterait ce posterior. La borne nécessaire
p_ik≥p_ij+p_jk−1 est différente ; (0,9,0,9,0,1) la viole.
Ces bornes ne suffisent pas à certifier une loi jointe générale.

### Γ ne signifie pas « mêmes distributions »

Pour des lignes normalisées π_i,π_j, Γ=Σ_t π_it π_jt est la probabilité
de collision de deux tirages indépendants de tokens. Deux distributions
identiques uniformes sur m tokens ont Γ=1/m, même à couverture complète.
Une baisse peut traduire la diffusion des incidences, sans désaccord.

Garder Γ si la cible voulue est la coaffectation. Si l'objectif est la
ressemblance des distributions, comparer explicitement
O=Σ_t min(π_it,π_jt), qui vaut 1 pour deux distributions identiques.
Ce changement n'est pas une correction silencieuse de Γ : c'est un autre
prétexte. [Guidage complet](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md).

## 7. Le pilote conseillé

1. **coverage_v1 → GuidanceBundle K1**, avec censure, IDs et séparation des
   vues ; contrôles de raccourci déjà définis.
2. **PTv3 de référence + SSL**, puis une seule perte FULL. Même information,
   recette et budget ; sonde linéaire, faible annotation, transfert.
3. En parallèle, **weighted_gabriel_v1 → coupes/FP/PUR** : univers gelé,
   réserves, composition ; retours et statistiques simples comme tokens.
4. Ajouter **graphe d'événements**, puis plusieurs K, seulement sur la base
   de comparaisons attribuables et d'un budget global.
5. Encoder davantage de géométrie et développer SEL comme extensions,
   lorsque leur objectif et leur interface sont définis.

À chaque étape, conserver les résultats négatifs par bras sans annuler
abusivement tout le projet. Le modèle peut bénéficier du guidage et pas
du tokenizer, ou l'inverse. Le plan croisé A0G0/A0G1/A1G0/A1G1 est conservé.
[PLAN](PLAN.md) détaille les sorties et décisions.

Pour le régime extérieur : pré-entraînement primaire mono-scan,
sans validation 08/test dans le corpus SSL ou les statistiques apprises ;
fractions de **scans annotés** distinctes de fractions de points.
Le retrait d'anneaux teste une raréfaction, pas tout le changement de capteur.
Conserver brut et non-sol comme bras distincts, IDs complets et branche
sol/contexte ; l'oracle d'un nœud seul ne borne pas une sortie avec skip fin.
Une revendication de fondation attend plusieurs tâches et transferts.

## 8. Audit des sources, scripts et archives

| corpus initial | contrôle réalisé | conclusion |
| --- | --- | --- |
| 14 Markdown racine/FoundationModel | contrats, architecture, pertes, plan, risques, littérature et cohérence croisée | corrections dans les documents actifs ; lien de suite dans les deux rapports antérieurs |
| 3 scripts de référence | lecture et rejeu normal/−O, sortie comparée aux archives | 21 fixtures, 42 variantes réfutées, résultats byte-identiques aux archives |
| 8 fichiers de reçus historiques | JSON, liens, hashes source/sortie et portée déclarée | archives conservées ; aucune qualification native héritée |
| 27 fichiers de présentation | sources principales, bibliographie, 6 figures TikZ, Makefile, préparation, thèmes ; texte des 18 pages PDF | contenu audité ; aucun octet modifié |
| assets de présentation inclus dans ces 27 | dimensions PNG, égalité PNG/base64 SZTE ; inspection visuelle de la seule page 11 | intégrité contrôlée ; pas de nouvelle recompilation ni validation visuelle des 18 pages |

Les références primaires ont été vérifiées pour les décisions de conception :
PTv3, SPT, Sonata, BPS, PolyhedronNet, guidage SSL extérieur et ALPINE.
La [revue ciblée](ETAT_DE_LART.md) et [MESURE](MESURE.md) donnent les liens.
PolyhedronNet part de polyèdres avec faces/attributs ; son gain ne démontre
pas celui des supports Gabriel pour le LiDAR. ALPINE dépend des prédictions
sémantiques et de priors de classe. Les deux restent des comparateurs utiles
dans leur régime. Aucune recherche d'antériorité exhaustive n'est revendiquée.

### Vérifications nouvelles

Trois scripts ajoutent **12 fixtures et 27 variantes incorrectes réfutées** :

- géométrie du support : 4 fixtures, 7 variantes, 24 permutations du carré ;
- choix architecturaux : 4 fixtures, 8 variantes ;
- contrôles du guidage : 4 fixtures, 12 variantes.

Avec les scripts existants : **33 fixtures distinctes, 69 variantes réfutées,
six scripts exécutés en normal et −O**, mêmes sorties pour les deux modes.
Les variantes sont des propositions démenties par les exemples, pas des
mutants compilés du moteur. Les arbres et distributions abstraits ne sont
pas prétendus tous réalisables par un nuage HGP.

Les [reçus complets](receipts/reaudit_global_20260926/README.md) conservent
commandes, sorties et hashes. Aucun apprentissage, scan, benchmark moteur
ou accès GCP n'a été lancé.

## 9. Statut documentaire après corrections

Ce rapport complète l'[audit v9](AUDIT_V9_ET_ARCHITECTURE_20260926.md)
et le [contrat des coupes](CONTRAT_COUPES_ET_MASSES_20260926.md).
Leurs preuves sur états datés, masses gelées, réserves et composition
restent le socle. Il corrige les interprétations de condensation,
géométrie, attention et sélection, sans remplacer ces preuves.

Les décisions sont reportées dans OBJET, JETON, ARCHITECTURE,
SPECIFICATION, GUIDAGE, MESURE, PLAN, RISQUES, ETAT_DE_LART, GLOSSAIRE
et les README. La présentation originale et les reçus précédents restent
inchangés. **L'étape suivante est l'export natif du pilote choisi** ;
ces fixtures ne sont ni son implémentation ni sa qualification.
