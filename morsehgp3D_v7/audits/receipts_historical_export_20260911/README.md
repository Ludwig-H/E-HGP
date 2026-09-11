# Retrouver l’encodage historique après les horizontales indépendantes

11 septembre 2026. Suite de [la composition par fenêtres](../receipts_composable_msf_20260911/README.md), après publication constructeur `679f4a6f`. Source historique visée : `full_ball_tower.hpp`, SHA `83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366`. Écritures d’audit uniquement ; GCP non utilisé.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

**On peut conserver une banque unique et retrouver ses anciens indices après calcul indépendant des horizontales.** Le couplage de numérotation est une contrainte d’export, pas une obligation de construire K banques ou de rejouer le calendrier géométrique. Il demande des requêtes historiques, des minima par groupe et des tris. Cette note distingue leur preuve de la vitesse d’une implantation parallèle.

## 1. Le point de conformité est réel

Le [second auditeur](../NOTE_CLAUDE_DECOUPE_TOUR_20260911.md), §4ter, identifie correctement les portes `paired.bank_rows`, `paired.exact_contribution` et `bank.shared_across_orders`. Un digest de populations déréférencées n’est pas un substitut à ces contrôles. `same_payload` compare aussi les numéros de nœuds, parents, successeurs, images verticales et les octets des niveaux rationnels.

Le [raccord constructeur](../../receipts/atlas_graph_full_20260911/README.md) crée déjà une banque unique avant sa boucle K. Il compare FULL par une bijection des naissances natives et assume une nouvelle convention numérique. Notre contrelecture est favorable dans ce domaine : aucune erreur ne lui est imputée pour une égalité physique qu’il ne revendique pas. Ses lecteurs normal/−O passent ; 114 census, 29 784 coupes et 15 594 832 contrôles verticaux restent des qualifications bornées, pas un résultat industriel.

Le but supplémentaire ici est de reproduire l’ancienne convention **sans refaire la découverte temporelle**. Les clés suivantes se calculent depuis le catalogue, les rôles, φ et les histoires horizontales complètes déjà certifiées contre leurs graphes. Elles ne certifient pas ces prémisses à leur place.

## 2. Retrouver les groupes de fermeture depuis les histoires terminées

Pour chaque rôle `(K,B)`, conserver sa date λ_B, son rang exact t_B et son rang r(B) dans l’ordre **BallKey**, distinct du BallId d’entrée. Le programme historique trie d’abord par BallKey, puis trie stablement par date ; à une date fixée, son ordinal de bloc est donc croissant en r(B).

Soit u_B l’ancêtre historique de φ(B), à la coupe **fermée** λ_B. La partition des rôles d’un même lot par u_B est exactement celle de `close_lot` : les blocs qui partagent une composante avant lot, directement ou par chaîne de blocs, appartiennent à la même composante après fermeture. Les naissances sans parent restent isolées les unes des autres. Les continuations sont rattachées à leur ancienne composante, sans créer un nœud fictif.

On peut obtenir tous les u_B par les chaînes lourdes déjà qualifiées. Le groupage utilise `(K,t_B,u_B)`, jamais u_B seul : une composante persistante peut recevoir des contributions à plusieurs dates. Il ne dépend pas des numéros historiques des nœuds ; toute bijection préservant les identités de naissances, niveaux et parents donne les mêmes groupes.

Calculer ensuite deux minima différents :

$$g(K,t,u)=\min_{B:(K,t_B,u_B)=(K,t,u)}r(B),\qquad h(K,t)=\min_{B:(K,t_B)=(K,t)}r(B).$$

Le premier ordonne les groupes du lot ; le second identifie le premier bloc du lot entier. **Tous les blocs participent**, y compris ceux qui ne portent aucune contribution. Ces deux résumés peuvent être calculés par tris et réductions segmentées ; des fragments de fenêtres se combinent par minimum, avant finalisation des résultats.

## 3. Une seule banque, dans l’ordre exact de première utilisation

Dans `close_lot`, les unions DSU choisissent toujours le plus petit ordinal de bloc. Les groupes sont parcourus par cet ordinal minimal, puis leurs blocs par ordinal croissant. Le chemin singleton respecte le même ordre. Un appel à `population(B)` a lieu exactement lorsque le masque de contribution est non nul ou que l’intérieur contribue.

L’ordre exact de ces appels est donc la clé lexicographique :

$$c(K,B)=(K,t_B,g(K,t_B,u_B),r(B)).$$

Retenir le minimum de c(K,B) pour chaque **BallId utilisé**, puis trier ces minima. Ce tri donne l’ordre de création des populations de boules. Le préfixe est constitué des n populations singleton K1, en PointId croissant. Chaque BallId possède au plus un rôle par K et un niveau fixe : le premier K où il contribue peut même être déterminé depuis l’atlas avant les histoires.

Cette procédure reproduit la table mémoïsée `population_ids`, qui reste globale entre ordres. Elle ne dédoublonne pas les BallId par contenu de population et n’alloue pas une banque par K. Les références intermédiaires gardent leur BallId jusqu’à la permutation finale. Construire alors un propriétaire immuable unique et transmettre **le même** `shared_ptr` à tous les encodeurs.

Contre-fixture abstraite minimale : à un même `(K,λ)`, trois blocs de rangs `0<1<2` ont respectivement les anciennes racines `{p}`, `{q}`, `{p}`. Le bloc 0 est silencieux ; 1 et 2 contribuent pour la première fois. Le calendrier visite le groupe `{0,2}`, puis `{1}`, donc crée la population **2 avant 1**. Un tri direct par BallKey donne 1 avant 2. Un minimum limité aux blocs contributeurs commet la même erreur. Aucune réalisation 3D de ce témoin abstrait n’est supposée.

Les rôles silencieux peuvent être libérés après consommation des deux minima, pas au seul motif qu’ils n’écrivent rien dans la banque.

## 4. Nœuds, contributions et niveaux bruts

La banque seule ne suffit pas à passer `same_payload`. Pour chaque K, les nœuds historiques sont créés en suivant les groupes par `(t,g)` et en sautant les continuations. À K1, les n singletons PointId précèdent ces groupes. Une naissance ou une multifusion possède un unique groupe à sa date de création ; lui attribuer un indice par tri puis scan reproduit son NodeId historique.

Transporter ensuite les parents par cette permutation et les trier, calculer leurs offsets, remapper les segments des contributions et les successeurs. Les contributions suivent l’ordre `(t,g,r(B))`, avec leur masque et leur indicateur d’intérieur d’origine. Les images verticales déjà calculées se transportent par les permutations de K et K−1 ; aucune nouvelle MEB ni dépendance aux verticales inférieures n’apparaît.

Le `FullCoverageBatch` historique porte le **niveau brut du premier bloc du lot K**. Deux `ExactLevel` peuvent être rationnellement égaux sans avoir les mêmes numérateurs et dénominateur. Le rang global ne choisit pas nécessairement le bon représentant ; celui d’un contributeur non plus. Utiliser le bloc h(K,t), silencieux compris, pour toutes les dates de nœuds et contributions de ce lot. Conserver `{{0,0,0},1}` pour les singletons initiaux.

Cette reconstruction est une passe de convention physique sur une histoire déjà obtenue. Elle ne retrouve pas les coûts, compteurs de cache ou chemins d’exécution de l’ancien Builder, et elle ne promet pas l’identité des arêtes MSF internes.

## 5. Travail supplémentaire et portée du parallélisme

Avec A rôles, N nœuds et C contributions, la réalisation directe ajoute A consultations historiques, des groupages/minima et les permutations d’export. Les chaînes lourdes donnent O(log N) par consultation ; un tri comparatif simple des rôles coûte O(A log A), avec des tableaux temporaires de taille O(A). Ces coûts restent à payer et à mesurer. La copie des lignes de populations et la taille de la sortie explicite restent présentes.

Les requêtes sont indépendantes une fois les histoires disponibles. Minima par groupe, premières utilisations par BallId, offsets et écritures aux indices attribués sont eux aussi distribuables. Une version fenêtrée doit fusionner les minima des groupes traversant plusieurs fenêtres avant d’émettre leurs positions définitives. La preuve ne justifie ni de garder tous les temporaires sans budget ni de publier une banque partiellement numérotée.

Le plafond somme/max de dix calendriers, discuté dans le §4bis du second auditeur, concerne une variante qui conserve leurs dépendances internes. [La proposition a97ee819](../receipts_parallel_objects_20260911/README.md), §2 et §4, vise également le calcul interne à chaque K par graphes, MSF et reconstruction d’arbres. Aucun facteur 11x ou 20x n’est transféré à cette architecture. Sa reconstruction parallèle et son export doivent être implémentés et chronométrés.

## 6. Qualification C++ bornée O2/SAN

La [gate indépendante](historical_export_gate.cpp) passe O2 strict et ASan/UBSan/LSan, avec les mêmes sources et sorties : dix entrées, soixante ordres, 2 184 nœuds, 1 390 contributions, 2 056 références verticales et 1 388 lignes de banque cumulées. Elle compare tous les champs de `same_payload` et contrôle séparément l’unique propriétaire partagé de chaque tour. Les 2 704 consultations de blocs comptent un export normal par entrée, sans ajouter les rejeux de diagnostic ou de mutants.

Le corpus comporte cinq nuages fixes de 3, 4, 5, 8 et 16 points, chacun dans deux variantes de PointId, ordre d’entrée, census et coquilles. Le census vient de génération→préfiltre→census, s=8. Sur les deux entrées à cinq points, une unique fraction de rayon carré 4 est ensuite multipliée par 2 au numérateur et au dénominateur : la géométrie reste identique, les bornes sont contrôlées et le vrai Builder admet ces métadonnées. Huit entrées utilisent donc le census sans ce recodage, deux exercent sa représentation rationnelle équivalente. K=n est exercé sur les petits nuages ; n=1 n’est pas une nouvelle fixture de ce paquet.

| Transformation fautive | Réfutation dans la gate |
| --- | --- |
| Banque triée par BallId d’entrée | Dix rejets `physical.bank_rows` sur les entrées géométriques. |
| Représentant rationnel global au lieu du premier bloc du lot K | Deux rejets `physical.raw_node` sur la fixture de recodage exact. |
| Numéros de l’histoire interne conservés | Deux rejets `physical.raw_contribution` ; ils ne reproduisent pas l’ordre physique attendu. |
| Minimum de groupe limité aux contributeurs | Le corpus géométrique ne l’exerce pas : zéro groupe contributif à minimum silencieux. Le témoin abstrait du §3 reste explicitement séparé et réfute cette simplification. |

Les quatre témoins abstraits de la gate sont des vérifications de structure et d’encodage, sans claim de réalisation 3D. Les mutants confrontés aux entrées géométriques sont testés à l’intérieur de `--selftest` par comparaison causale ; ils ne sont pas présentés comme quatre commandes en échec. Argument absent ou inconnu : code 2, flux vides. Chaque capture comporte huit commandes, sources avant/après identiques ; aucune erreur de compilation ou de sanitizer dans ces captures. Les lignes `case=...` sur stderr sont la progression attendue des fixtures.

Le reconstructeur est séquentiel : tris, minima et écritures servent de témoin, pas de backend parallèle. La gate réutilise les verticales de `graph_full::build`, puis transporte explicitement leurs indices. Elle conserve donc un export sémantique intermédiaire et la référence pour comparaison ; ce n’est ni le plan mémoire du produit ni une nouvelle preuve géométrique. Le port industriel pourra transporter directement les réponses HLD et construire sa seule banque finale. La copie de Builder fournie par le paquet constructeur ne diffère de la source de référence que par sa déclaration friend de lecture, contrôlée par son lecteur parent. Aucune duplication de source constructeur n’est ajoutée à ce paquet.

## 7. Lecture, reproduction et entretien

Les [captures O2](o2.json), [captures SAN](san.json) et [résultats](result.json) sont scellés. Le lecteur contrôle les sources consommées, le paquet constructeur et son parent, les commandes, les sorties et les non-vacuités ; il ne compile ni n’exécute la gate.

```bash
python3 -B morsehgp3D_v7/audits/receipts_historical_export_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_historical_export_20260911/verify.py
python3 -B morsehgp3D_v7/audits/receipts_historical_export_20260911/record.py --out morsehgp3D_v7/audits/.work_export_replay_o2
python3 -B morsehgp3D_v7/audits/receipts_historical_export_20260911/record.py --out morsehgp3D_v7/audits/.work_export_replay_san --san
```

Chaque sortie doit être nouvelle et rester sous `audits/`. Le recorder extrait physiquement le paquet constructeur scellé, compile avec C++20 et `-Wall -Wextra -Wpedantic -Werror`, puis garde les commandes et résultats, y compris si une commande échoue. Les 42 fichiers consommés (gate comprise) sont épinglés avant/après ; le compilateur et le binaire le sont également. Les headers et runtimes système ne constituent pas une fermeture hermétique ; aucun ELF, Boost ou vendor n’est versionné ici. Le lecteur s’appuie sur les sources scellées, sans exiger la survie des chemins temporaires des anciennes commandes.

Le résultat ferme une possibilité constructive de conformité à l’encodage `83f1c78e`. Son intégration au nouveau producteur fenêtré reste à effectuer et à mesurer. Les reçus historiques, contre-fixtures et notes du second auditeur restent sous leurs autorités propres ; les variantes globales D–Q sont inchangées. GCP non utilisé.
