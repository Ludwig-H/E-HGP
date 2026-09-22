# Rectangles h + h_a + h_b : résultats utiles et limites

22 septembre 2026. Base relue : `39c6b824b68958f8002d4e7df85c1c715c36b85b`.
Audit indépendant, `exploration_v8_hors_registre`, `public_status=not_claimed`.
**Aucun fichier moteur modifié. Aucun build/CTest HGP, scan LiDAR ou GPU exécuté.**
Les essais sont ceux d'un **prototype C++ autonome**, avec son propre index,
front, stockage i64 et instrumentation, reprenant les principes et les formules
H/Xi de la v8. Ne pas les présenter comme une compilation du pipeline natif.

Sources, juges, captures et rapport détaillé : archive
`MorseHGP_rectangles_partages_tests_2026-09-22.zip` jointe à la conversation.
Seule cette note est publiée dans le dépôt ; les sources expérimentales ne
sont pas raccordées au CMake v8.

## 1. Décision

**Raccorder les crédits par facteurs sur les gros rectangles q3/q4, et réemployer
la recherche déjà achevée des rectangles singletons.** Ne pas préparer un plan
lourd pour chaque petit rectangle. Ne pas porter telle quelle une récursion
qui relance la recherche dans l'index pour chaque sous-rectangle.

### Étage manquant

Dans [wspd_q34.cpp](../src/pipeline/wspd_q34.cpp), `rectangle()` applique le filtre
commun puis `expand()` développe les paires. Le raccord global ne calcule pas
h_a/h_b, malgré les briques de [local_credits.cpp](../src/pipeline/local_credits.cpp)
et le [Q2NodePoolPlan](../src/pipeline/q2_node_pool.hpp) déjà présent pour q2.

### Nouveau constat : deuxième recherche identique sur les singletons

En mode `RectanglePair`, si |A|=|B|=1, le filtre rectangle a déjà exécuté la
recherche pour cette paire. `edge()` rappelle pourtant le filtre avec les mêmes
coordonnées, index, K et options, sur le masque survivant. La première recherche
ne pouvait pas s'arrêter avant d'avoir décidé toutes les voies restantes.
**La deuxième ne peut donc supprimer aucune voie supplémentaire.**

Transporter un état « recherche de cette paire achevée » jusqu'à `edge`, y
compris dans les tâches parallèles. Cette preuve reste attachée à l'index,
à cette paire, au seuil et aux voies. Ne sauter que le filtre de TÉMOINS,
pas le cover, l'atlas, les graines ou le census des boules. Ne pas étendre le
raccourci aux rectangles non singletons ni aux propositions seules du front.
Adapter les registres : mêmes décisions, moins d'appels.

## 2. Plan par facteurs testé

Pour chaque voie, T_q=Kmax+2-q. On utilise h_q commun, h_{q,a}(a) provenant
de témoins de A universels sur {a} x B, et h_{q,b}(b) provenant de B.
Les témoins communs certifiés sont hors des deux boîtes ; ces trois sources
sont disjointes. Rejeter lorsque `h_q+h_{q,a}(a)+h_{q,b}(b)>=T_q`.

Les propositions directionnelles sont sélectionnées une fois par facteur,
puis certifiées exactement contre les coins de la boîte opposée. H/Xi sont
partagés entre les voies actives, mais les comptes et seuils restent distincts.
Les crédits sont regroupés avant toute expansion ; le prototype groupe par
tuples de crédits pour conserver l'union q3/q4 sans doubler les paires.

Huit variantes comparées sur les mêmes rectangles : référence rectangle puis
paires ; facteurs sans h ; facteurs avec h ; facteurs avec h et reprise d'IDs ;
plan adaptatif ; récursion des rectangles avec IDs hérités ; réemploi singleton ;
combinaison plan adaptatif + réemploi singleton (nommée `hybrid`).

La sélection adaptative testée exige `|A||B|>=256` et
`|A||B|>=4(|A|+|B|)`. C'est un choix expérimental, pas un optimum ni une limite
sur l'exploration : sans plan, le calcul individuel continue normalement.

## 3. Mesures : filtrage seulement, pas HGP complet

16 contextes synthétiques, q3/q4, s=8, K5/K10, n512 et quelques n1024.
Temps CPU du thread, médianes de 3 à 7 répétitions en ordre tournant ; les
captures conservent aussi les temps mur. Hôte partagé AMD EPYC 9V74.

| Famille | n | K | Référence ms | Hybride ms | Rapport | Recherches individuelles, avant -> après |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 amas 3D | 1024 | 5 | 444,91 | 160,67 | 2,77 | 486832 -> 43861 |
| 8 amas 3D | 1024 | 10 | 903,17 | 419,65 | 2,15 | 507071 -> 84637 |
| 2 groupes presque alignés selon leur séparation | 1024 | 5 | 51,59 | 3,67 | 14,07 | 266220 -> 10 |
| Même famille | 1024 | 10 | 62,56 | 10,92 | 5,73 | 272776 -> 2053 |
| Volume uniforme | 512 | 10 | 395,31 | 321,21 | 1,23 | 43726 -> 14785 |
| Couche presque plane | 512 | 10 | 86,70 | 72,06 | 1,20 | 18146 -> 10260 |
| Sphère arrondie sur grille | 512 | 5 | 586,28 | 604,13 | 0,97 | 68882 -> 62112 |

À 1024/amas/K10, **le plan adaptatif seul** fait passer les recherches de
507071 à 120800 et le temps de 903,17 à 439,09 ms. Seulement **28 plans** sont
construits. Le gain ne vient donc pas seulement du raccourci singleton.

Volume et couche plane : aucun plan sélectionné par la règle retenue, le gain
vient du réemploi singleton. La sphère K5 est un résultat négatif, environ +3 %.
Les groupes longitudinaux sont un cas favorable synthétique, pas une prévision
LiDAR. Aucun exposant de croissance n'est déduit de ces expériences.

**Chrono** : index et front exclus ; recherches des rectangles, propositions,
certifications, groupements, allocations, recherches individuelles et écriture
du vecteur de résultats inclus ; son tri final de comparaison est exclu.
**Pas de cover, atlas, graine, boule, coquille ou FULL dans ce chrono.**
Même si la sortie de paires est identique, cela ne qualifie pas le moteur v8.

## 4. Deux expériences négatives à ne pas transformer en recommandations

**Reprise par listes d'IDs partout** : le filtre individuel démarre avec les
crédits déjà acquis, et un nœud Z admis ne contribue que
`|Z|-|Z intersect D_q|`, où D_q est l'ensemble déjà compté sur cette voie.
C'est exact ; D_q contient au plus T_q-1 IDs pour une voie survivante. Mais
les comparaisons d'appartenance et les copies coûtent : à 1024/amas/K10,
449,74 ms pour les facteurs avec h sans reprise, contre 511,18 ms avec reprise.
Ne pas importer cette première implémentation partout. Une frontière ou un
curseur possédé qui éviterait ces rescans reste une autre expérience à mener.

**Récursion avec nouvelle recherche d'index par sous-rectangle** : sur le même
cas, les recherches individuelles tombent de 507071 à 198967, mais le temps
passe de 903,17 à 1094,58 ms. On a déplacé le travail vers les sous-rectangles
sans assez le supprimer. Ce résultat ne réfute pas le partage par rectangles ;
il réfute l'intérêt de cette variante naïve. Le partage de la frontière Z
n'est pas mesuré par ce lot.

## 5. Exactitude et contre-épreuves

84 configurations sur 21 petits nuages, huit variantes, coordonnées jusqu'à
262143, voies séparées ou combinées, K2/3/5/10 et s1/4/8/16 :
**223136 comparaisons exactes de paires/masques**, aucun désaccord avec le juge
scalaire indépendant. Celui-ci calcule Xi par l'identité de Gram, sans utiliser
les produits vectoriels ni les bornes de boîtes du C++. 196394 tests de témoins
pour construire la référence ; les répétitions entre variantes ne sont pas des
nuages indépendants. GCC 14.2 Release/Python normal et Clang 17 ASan/UBSan/Python -O
donnent les mêmes sorties discrètes, huit entrées invalides refusées.

Second juge Fraction/Gram : 10 nuages, 2652 supports positifs, 37128 puissances,
**3751 contrôles de conservation de supports dans la fenêtre**, aucune voie
nécessaire perdue. La couche plane ne fournit pas de q4 positif dans ce petit
corpus ; ne pas lui attribuer une vérification q4 non vide.

Deux mutants DU PROTOTYPE, compilés, sont détectés par une erreur géométrique :

- Recompter les témoins hérités dans un nœud admis : une paire perd une voie,
  bien que le nombre total de paires survivantes reste 324.
- Additionner deux fois h commun : 24 paires perdent au moins une voie ;
  312 paires survivantes au lieu de 324.

Comparer seulement les effectifs ou une somme de compteurs ne suffit donc pas.
L'oracle de paires n'est pas un oracle de toute la tour FULL.

## 6. Suite minimale pour le développeur

1. Réemploi singleton dans le vrai chemin, avec contrôle différentiel des boules,
   profondeurs et coquilles, puis mesure séparée du travail supprimé.
2. Plan h/h_a/h_b partagé entre q3/q4 sur les gros rectangles. Réutiliser l'index,
   pas les adaptateurs qui reconstruisent des arbres locaux. Les classes de
   crédits ont leur permutation : elles ne sont pas les anciennes plages de
   rangs. Les tâches doivent posséder le plan immuable jusqu'à leur fin, sans
   le reconstruire par bande. Ventiler les masses par voie et leur union.
3. Comparaison appariée sur de vrais résidus 1 mm : somme des tailles de facteurs,
   plans utiles/inutiles, visites évitées et coût aval complet. Les seuils de
   sélection sont à déterminer par ces mesures, pas à figer sur nos synthétiques.

Un partage plus rapide des mêmes rejets avant atlas **ne supprime pas de nouveaux
atlas par lui-même**. Les certificats collectifs restent complémentaires : ils
peuvent apporter des rejets supplémentaires, à condition de payer leur acquisition.

## Empreintes des sources livrées

Les fichiers et captures complets sont dans l'archive jointe, non dans cette note.
Les premières compilations du prototype ont refusé des indentations ambiguës
sous -Werror ; elles ont été corrigées avant les qualifications. Les essais mur
préliminaires et les processus interrompus par la limite des appels outils ne
sont pas promus en mesures : la matrice comprend seulement 16 contextes terminés.
