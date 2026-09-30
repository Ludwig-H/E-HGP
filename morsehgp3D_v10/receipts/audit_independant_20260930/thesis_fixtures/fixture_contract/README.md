# Contrat des fixtures des deux triangles — 30 septembre 2026

Reçu indépendant, borné, clos après vérification des neuf entrées nommées. Il juge les petites captures historiques de `/tmp/deux_triangles`, **sans relancer l'exporteur ni un moteur natif**. Le commit déclaré par les exports est `e9eab2754f3d9f2a9e16d8c542b908c19b725c9a` ; le HEAD du checkout lors de la copie est consigné dans [SOURCE_BEFORE.json](SOURCE_BEFORE.json). Ceci ne qualifie pas une nouvelle intégration, une règle de tête, le profil large ou une performance. Aucun GCP/GPU.

## Géométrie effectivement livrée

Les six sites sont distincts, de poids un, coplanaires en `z=0`, dans u18. `r=1000` est exprimé en unités de grille ; les fichiers u32le n'encodent pas le pas physique, l'origine ou cette désignation des points. Les lignes binaires A..F sont conservées et leur correspondance avec les PID exportés est vérifiée. La variante courte translate tout DEF de −2 en x.

| Fichier | AB² = EF² | AC² = BC² = DE² = DF² | CD² |
| --- | ---: | ---: | ---: |
| aretes_plus_courtes | 4 000 000 | 3 999 824 | 4 000 000 |
| pont_plus_court | 4 000 000 | 3 999 824 | 3 992 004 |
| pont_plus_long | 4 000 000 | 3 999 824 | 4 000 000 |

`aretes_plus_courtes` et `pont_plus_long` ont les mêmes octets, binaires **et** JSON. Les quatre côtés obliques valent `sqrt(3 999 824) ≈ 1999,9559995`, mais les bases restent égales au pont de longueur 2000. La variante courte a un pont de longueur 1998. Ce sont deux géométries quasi équilatérales quantifiées, avec des plateaux distincts ; le libellé « arêtes plus courtes de 0,04 » ne décrit pas les deux bases.

Dans un plan `z=0` à coordonnées rationnelles, une aire non nulle est rationnelle, alors qu'un triangle équilatéral d'un côté de carré rationnel a une aire `sqrt(3) × côté² / 4` : une réalisation exacte est donc impossible dans ce repère. Cette restriction **ne s'étend pas aux triangles entiers en 3D** : `(0,0,0),(a,a,0),(a,0,a)` a trois côtés de carré `2a²`, un rayon de carré `2a²/3` et trois poids barycentriques `1/3`. Ce témoin exact est vérifié pour a=1 et 1000. Il ne réalise pas à lui seul la disposition coplanaire complète du manuscrit.

La fixture idéale de la thèse doit garder sa propre arithmétique algébrique, par exemple Q(sqrt(3)), et ses vrais plateaux. `vx.py` sait manipuler des **dates** radicales, mais sa factory `Tower` convertit les **coordonnées** par `int(c)` (ligne 256 de la copie) : le contrôle minimal `Fraction(3,2)` devient 1. C'est hors de son domaine entier déclaré, pas un défaut constaté dans ce domaine. Cette voie ne convient pas à l'oracle idéal sans modification explicite du contrat et de l'arithmétique des coordonnées.

## Couvertures, cohortes et cible de projection

L'oracle [check.py](check.py) construit exactement les 15 paires et les 20 triples de Γ₂, en énumérant les supports de MEB, y compris les sous-supports des triangles obtus. Il calcule également toutes les sphères critiques à supports positifs de taille 2..4, déduplique centre/rayon, puis applique l'admission `p + q_min ≤ 3`. Les **13 boules** de chaque capture coïncident, avec leurs S*, I et U complets, sans doublon. Les 10 nœuds archivés donnent les mêmes couvertures de composantes à toutes les coupes d'événements ouvertes et fermées ; aucune lecture double ne décide une égalité.

Entre les rayons 1300 et 1700, la couverture FULL comporte **ABC, CD et DEF**, donc deux sites ont plusieurs composantes incidentes. `ABC|DEF` est la cible de la **projection des points** demandée par l'utilisateur. Elle ne doit pas remplacer l'oracle FULL par une partition disjointe arbitraire.

La première cohorte exacte de C est **AC et BC** dans la base, mais **CD seulement** dans la variante au pont court ; celle de D suit la même asymétrie. Les trois témoins AC, BC et CD sont présents ensemble à un rayon ultérieur. La phrase de la réponse développeur selon laquelle ils couvrent C « dès sa première date » s'applique au plateau idéal, pas à ces deux entrées quantifiées. Une majorité sur la première cohorte et une majorité sur une bande fixe testent deux contrats différents.

Une bande de rayon `η=1/1000` contient exactement AC, BC et CD pour C, et CD, DE et DF pour D, dans les deux variantes. Le test est rationnel : `β_F ≤ (1001/1000)² α²`. La borne minimale pour inclure le pont dans la base est `sqrt(1 000 000 / 999 956)−1`; pour inclure les deux arêtes dans le cas du pont court, elle est `sqrt(999 956 / 998 001)−1`. Une bande plus petite peut donc changer l'univers. Ce choix fixe ne détermine pas un η universel pour d'autres nuages.

Le diagnostic de majorité stricte, à dénominateur fixé avant la coupe, donne ABC|DEF dans cet intervalle avec poids uniformes ou `1/β`. Les témoins non encore nés restent au dénominateur. Son emboîtement est contrôlé à toutes les coupes d'événements ouvertes puis fermées de ces petits nuages. Il s'agit d'un oracle de cette règle candidate, **pas d'un raccord natif livré**. La première couverture donne ABC|DEF dans la base et AB|CD|EF dans le cas du pont court.

Les dates d'entrée core sont `D₂²=3 999 824` pour tous les points dans la base ; dans la variante courte, C et D ont `D₂²=3 992 004`. Aucun point core n'est donc entré entre 1300 et 1700. Ce résultat respecte le contrat C∩X du moteur ; il explique pourquoi cette projection ne satisfait pas la nouvelle cible. Ce reçu ne prétend pas exécuter HDBSCAN.

## Contrôles à conserver pour les futures fixtures

1. Séparer « idéal algébrique », « grille u18 exactement jugée » et « triangle entier exact en 3D ». Publier h, origine commune, traduction/arrondi, six PID, carrés exacts des côtés/pont et liste complète des vrais plateaux ; refuser une conversion silencieuse vers `int` dans l'oracle idéal.
2. Juger FULL par toutes les K-parties et leurs unions de K+1, puis **séparément** la projection attendue à des coupes avant/après chaque plateau et au milieu de l'intervalle cible. Comparer le cover comme famille de composantes éventuellement chevauchantes ; juger S*, I/U et contacts complets, pas les seuls supports qui ressemblent à des arêtes de la figure.
3. Pour un vote, fixer précisément son univers, η, poids, dénominateur et seuil strict. Tester l'égalité au bord de la bande et les perturbations qui changent la première cohorte. Une réussite sur η=.001 dans cette fixture ne certifie ni la résolution des ambiguïtés générale ni les grands K.
4. Garder les PID à travers permutations et similitudes permises par la grille. Ici les 720 permutations sont jugées à rayon 1500 pour core, première couverture et majorité uniforme ; deux similitudes par permutation d'axes, translation et échelle entière positive vérifient les dates MEB mises à l'échelle. Une rotation quelconque **suivie d'un nouvel arrondi** peut changer les plateaux : elle exige un autre oracle de quantification.
5. Pour K=3..10, annoncer le nombre de sites, leurs poids, le vrai niveau de pont et les sous-supports positifs (taille ≤4 en 3D). De petits nuages peuvent garder l'énumération des K-parties bornée ; ils ne prouvent ni le coût du générateur ni la robustesse sur LiDAR. Ne pas introduire mcs/EOM dans ce jugement de projection.

[normal.stdout.json](normal.stdout.json) et [optimized.stdout.json](optimized.stdout.json) sont identiques : 35 060 vérifications explicites, sans `assert` supprimable par −O, stderr vides. Les décimales servent uniquement à l'affichage. [SOURCE_AFTER.json](SOURCE_AFTER.json) vérifie l'absence de changement des neuf fichiers lus ; [SHA256SUMS](SHA256SUMS) ferme ce reçu. Les sorties des premières lectures partielles sont décrites dans [EXECUTION.md](EXECUTION.md).
