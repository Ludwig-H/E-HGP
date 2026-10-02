# Conception et ordre de livraison du moteur v11

Décisions de développement au 2 octobre 2026, base documentaire `b104028f5`.
Ce texte complète [ARCHITECTURE.md](ARCHITECTURE.md), sans modifier ses domaines, transactions ou
budgets, et utilise les lemmes de [MATHEMATIQUES.md](MATHEMATIQUES.md). Les conceptions privées du
générateur et de la tour sont des sources de propositions ; leurs prototypes ne sont pas des ports
qualifiés. L'[inventaire de l'audit v10](AUDIT_V10_SYNTHESE.md) conserve cette séparation.

## 1. Contrat et prochaine tranche

Livrer la tour FULL exacte des ordres $1..K$, verticales comprises, puis les entrées de points « core »
et « cover » ensembliste. Le profil initial est entier 18 bits, à sites de poids un. Le module cloud
conserve les identifiants de tous les retours et leurs multiplicités ; le premier moteur refuse les
poids supérieurs à un au plus tôt. La grille déclarée est un profil distinct des coordonnées float32
brutes. Ni le port d'une primitive ni une mesure sur grille ne qualifient le profil brut.

La cible utilisateur du 2 octobre est la trame LiDAR entière, prioritairement sans sol, sur G4 :
objectif 100 ms pour la tour jusqu'à $K=5$, et si possible jusqu'à $K=10$. Un seul ordre, un sous-nuage spatial, un catalogue
seul ou une exécution sans verticales ne remplace pas FULL. Préparation, segmentation du sol, moteur,
projection et tête ont des durées séparées, avec une durée de chaîne déclarée. La segmentation peut
être approximative ; l'exactitude HGP porte alors sur le sous-nuage explicitement publié.

**Prochaine tranche : catalogue exact après fermeture des portes de num.** Une première voie exacte,
vérifiable et déterministe précède les variantes vectorisées. Livrer ensemble le module, son format
canonique, son oracle, ses refus et ses mutants géométriques. Ne pas ajouter simultanément une nouvelle
projection, un nouveau sélecteur et une nouvelle représentation numérique : leurs écarts deviendraient
impossibles à attribuer.

| Tranche | Résultat public attendu | Condition de sortie |
| --- | --- | --- |
| N | Primitives numériques utilisées par le catalogue | Budgets de bits, domaine de chaque filtre, exact/oracle, refus et mutations |
| C | $\mathrm{Cat}_K$ avec populations, supports et niveaux exacts | G1–G4, populations contre scan global, différentiel et portes d'échelle |
| T | FULL $1..K$, plateaux N-aires et verticales | Comparaison intégrale à la définition indépendante, pas seulement nombre de composantes |
| P | core, $A_k(x)$, tout $E_k(x)$, relation boule forte → nœud | Dates et ensembles à coupe fermée, égalités et continuations étendues |
| H | Projection exclusive nommée, condensation puis sélection | Contrats distincts et juges de perte à chaque étage |

Cette table est un ordre de livraison, pas une déclaration que ces tranches sont présentes ou qualifiées.
Les qualifications des fondations restent celles des fichiers et portes datés dans
[PROVENANCE.md](PROVENANCE.md).

## 2. Frontières de données

PointId, SiteIdx, BallIdx, LevelRank, NodeIdx et offsets ne sont pas interchangeables.
Un BallIdx canonique n'est pas une position dans un tampon d'émission temporaire. Si un stockage
spatial est essayé, sa permutation vers le domaine canonique doit être explicite et budgétée.

Le catalogue publie au minimum le support canonique, les nombres $p,m,q$, les incidences complètes
de $I$ et $U$, le niveau exact ou sa référence, et son rang dense. L'ordre canonique est donné par
(niveau exact, support canonique). Les offsets sont u64 ; les plafonds d'identifiants u32 et de
capacités produisent des refus avant troncature. Des niveaux rationnels égaux ont un même rang,
même si leur représentation non réduite diffère.

Un format de compatibilité v10 peut demander une autre présentation du niveau ou une renumérotation.
Cet adaptateur doit être nommé et testé. Une égalité octet pour octet avec le format v10 ne remplace
pas la comparaison mathématique au format canonique v11, et inversement.

Chaque résultat possède ses tableaux ou emprunte un propriétaire dont la durée de vie est explicite.
Les vues de feuilles, supports et requêtes privées ne traversent pas une file asynchrone par simple
copie de pointeurs. La Session doit survivre aux résultats comme l'exige ARCHITECTURE.

## 3. Numérique : fermer l'expression réellement utilisée

La voie entière est la référence de décision. Pour chaque centre, prédicat de côté, dominance,
intersection de boîte et comparaison de rayons : écrire l'expression, son budget de bits et les
hypothèses de chaque opérande avant son port. Un témoin extérieur à une feuille n'hérite pas de la
largeur locale de cette feuille.

Pour F2, borner toutes les sommes partielles et tous les produits en valeur absolue sous $2^{53}$.
Pour F3, propager les exposants par expression : un carré lit deux fois l'erreur de son opérande.
Pour F6, développer avant annulation et inclure dans l'exposition toute conversion non exacte en
binary64. Certifier aussi la représentabilité du seuil $2^{q+e-51}$, les résultats intermédiaires
de l'inverse, l'absence de débordement et le domaine sans sous-flux requis. Hors domaine du filtre,
revenir à l'entier exact ; à l'égalité du seuil aussi. Ces précisions reprennent le
[suivi numérique](../audits/AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md), sans transformer un auto-test
en preuve des expressions.

Les portes numériques portent sur les expressions du produit : zéros, signes $\pm1$ au voisinage de
grands permanents, réutilisations, parenthésages permis, quatre arrondis, FMA active/inactive, limites
u18 et repli effectivement emprunté. Compiler en 21/24 bits est nécessaire mais ne qualifie pas ces
profils. Une borne de distance centre–boîte qui soustrait des approximations exige sa propre preuve
d'erreur absolue ; elle n'entre pas automatiquement dans F3.

## 4. Catalogue : chemin exact de référence

1. Préparer le propriétaire des sites et la boîte racine, vérifier domaine, poids et paramètres.
2. Propager des listes K-certifiées G1. Une dominance est stricte sur la boîte fermée ; les contacts
   restent candidats. Ajuster les boîtes sans perdre les centres extrêmes ; subdiviser avec propriété
   demi-ouverte, progression et borne de ressources explicitement contrôlées.
3. Dans une feuille, préparer les dominateurs distincts, puis énumérer les supports partiels avec les
   seuils G3. Conserver les triplets non aigus susceptibles d'appartenir à un tétraèdre.
4. Certifier le support complet, le centre propriétaire et le recensement. Un arrêt anticipé sur trop
   d'intérieurs est un rejet de candidat ; une sortie acceptée contient toute la coquille. Canonicaliser
   sur tous ses supports minimaux et appliquer l'admission $p+q\leq K+1$.
5. Compter/réserver/remplir les sorties, comparer exactement les niveaux, former leurs rangs puis
   publier une seule transaction. Les variations de nombre de fils changent l'ordonnancement, jamais
   le catalogue ni ses compteurs discrets définis indépendamment de cet ordonnancement.

Une taille de feuille est un réglage de coût, pas un axiome. Une taille trop proche de K peut faire
exploser la subdivision ; une grosse feuille rend l'énumération coûteuse. Les seuils privés 32/64,
les régimes SIMD et les boîtes très fines restent des choix à mesurer. Un plafond de coquille ou de
profondeur doit être annoncé et refuser proprement ; ne pas réduire implicitement le domaine v10.

Un filtre de droite équidistante ou de boîte de centre peut être ajouté seulement avec un lemme et
une porte des égalités. Retirer un filtre nécessaire préserve la complétude mais peut augmenter le
travail ; changer son ordre n'est avantageux qu'après une ablation du pipeline complet.

**Portes minimales de C, à livrer avec le module :**

| Porte | Juge indépendant et cas obligatoires |
| --- | --- |
| Petits catalogues | Énumération brute de supports/rationnels ; q2/q3/q4, toutes les dimensions affines, coquilles étendues, limites u18, K1..10 avec planchers par strate |
| Listes et élagages | Scan global au centre des boules émises ; candidat canonique conservé ; contacts sur chaque face/arête/coin et borne supérieure d'ajustement |
| Restrictions | CatK contre CatK+2 filtré, après renumérotation ; présence de niveaux distincts trop proches pour un flottant |
| Déterminisme | Entrées permutées avec IDs conservés, nombres de fils différents, reprise d'un échec de tâche ; mêmes sorties et verdicts |
| Ressources | Échec de chaque allocation pertinente, capacité/offset limite, refus sans sortie partielle, budget revenu à son état initial |
| Mutants causaux | Stricte devenue large, témoin compté deux fois, seuil décalé, q3 aigu imposé au préfixe q4, coquille tronquée, support/plateau mal canonisé |

Les oracles de petite taille jugent la correction ; les tailles 8k/16k/32k et les trames complètes
jugent croissance, mémoire et durées. Euler élargi, EMST et scans ciblés y sont complémentaires,
sans devenir une preuve de complétude. Conserver le plancher de chaque strate et les premiers échecs.

## 5. Index et tour : obligations avant les raccourcis

L'index interroge le propriétaire global. La liste d'une feuille certifiée pour les centres de cette
feuille ne certifie pas le recensement au centre d'une MEB descendue hors de la boîte. Une requête
saturante peut rendre K intérieurs stricts quelconques lorsqu'ils suffisent au saut ; sinon elle
doit rendre le recensement exact, avec toute la coquille. K borne la taille de ce premier témoin,
pas le nombre de visites de recherche. Une requête promettant les K plus proches a un contrat plus fort.

Les cellules régulières se lisent directement dans $(p,q,m)$ ; les cellules étendues utilisent d'abord
une référence exhaustive bornée. Un quotient plus compact doit couvrir toutes les parties séparables,
antipodes et dégénérescences compris. Un représentant par morceau ou par raffinement **exhaustif**
suffit ; dédupliquer les racines globales avant l'union.

Un rapprochement par empreinte accélère la recherche d'un semis, mais seule l'égalité exacte des
populations décide. Le certificat M2 évite un calcul de MEB si support inclus et population englobante
sont vérifiés. Un mémo ou pointeur de cellule porte sa date de validité : jamais de partage de terminal
entre morceaux distincts avant leur niveau de jonction. Une MEB absente du catalogue **dans sa fenêtre
requise** provoque un refus d'invariant ; sa présence hors fenêtre n'est pas exigée.

Construire les plateaux par lots, ou par événements binaires suivis d'une contraction exacte ; ne
publier aucun nœud intermédiaire de même niveau que son parent. L'historique d'une union par taille
peut accélérer les requêtes de composante passée, à condition de conserver l'historique immuable et
de traiter les égalités de rang à coupe fermée. Les raccourcis T3–T7 de la conception privée ont des
arguments et prototypes propres ; leur port doit encore fournir ces garanties sur les types v11.

Les portes T comparent **toute la forêt** à l'étage de définition de reference : naissances,
ensembles d'enfants de chaque multifusion, niveaux, coupes ouvertes/fermées, verticales et attaches.
Inclure les chemins extérieurs reliant plusieurs morceaux locaux, les terminaux de descente distincts,
le raffinement exhaustif, les plateaux réordonnés, les coquilles étendues et K10. Muter séparément les
niveaux, les enfants, une verticale et une naissance pour vérifier la sensibilité du juge. Le juge
constructif reste utile, mais ne peut partager avec le produit toutes les fonctions qu'il prétend juger.

## 6. Points et tête : rendre chaque perte observable

Publier d'abord les dates core, les dates cover $A_k(x)$ et les ensembles complets $E_k(x)$,
avec la relation des cellules fortes vers les nœuds vivants. Le choix d'un propriétaire exclusif est
une transformation nommée qui vient ensuite. La projection LCA publie sa date retardée ; elle ne
réécrit pas la date de première couverture. Les deux triangles, le site médian symétrique et une
continuation étendue gagnant un point sont des portes permanentes.

Comparer successivement la présence d'un groupe géométrique, sa conservation après projection, la
compatibilité simultanée de plusieurs groupes, puis ce que sélectionne la tête. Le meilleur IoU
d'une cible sur tous les nœuds borne le potentiel de cette hiérarchie pour cette cible ; plusieurs
meilleurs nœuds peuvent se chevaucher et ne forment pas nécessairement une partition réalisable.

La condensation doit balayer les cohortes de départs et tous les nœuds d'un même plateau simultanément,
y compris une attache qui entre exactement au niveau du parent. Les masses se conservent. L'option
allow_single autorise la sélection de la racine, sans l'exempter de min_cluster_size. Définir séparément
la stabilité, EOM/leaf, les égalités et le bruit ; contrôler les groupes finaux par leur masse exclusive.
Un correctif v10 par cohortes et ses preuves bornées existent : ils inspirent des portes, sans
qualifier un port v11 ni la règle vote.

Le domaine de $\lambda$, les poids, les produits poids × durée et leurs sommes doivent être traités
ensemble. Une garde « lambda finie » n'empêche pas le débordement d'une stabilité. Les cas zéro,
infini, exposant z, poids et racine exigent un contrat explicite et des refus avant publication.

La comparaison HDBSCAN utilise l'implémentation scikit-learn réelle, version et paramètres épinglés.
Les graphes témoins MR1/MR2 et leurs attaches cœur/bord sont des ablations nommées ; ils ne remplacent
pas cette référence. Le témoin exact $\{0,2,5\}$, K2, distingue déjà cover et MR2-bord avant EOM ;
des moyennes proches ne prouvent ni l'égalité des hiérarchies ni leur équivalence statistique.

## 7. Leviers priorisés et mesure de fermeture

| Priorité | Levier | Invariant à conserver | Preuve manquante avant promotion |
| --- | --- | --- | --- |
| 1 | Voie entière exacte, listes et catalogue canonique | G1–G4, ressources et déterminisme | Port C complet et juges indépendants |
| 2 | Ordre des tests, réutilisation de coefficients, filtres F2/F6 par lots | Mêmes supports et décisions exactes, domaines par opérande | Expressions réelles, mutants, ablation globale G4 |
| 3 | Préfixes d'émission et réduction des copies, stockage/permutation | Même domaine canonique ; toutes capacités coexistantes comptées | Pic mémoire mesuré et égalité de sortie |
| 4 | Cellules régulières analytiques, jointure de semis vérifiée, certificat M2 | Même population, aucune décision par hash/proposition flottante | Collisions forcées et différentiel des descentes |
| 5 | Pointeurs datés, noyau de plateau contracté, historique d'attache | T4–T6, nœuds vivants aux égalités de rang | Forêts/verticales contre oracle, prototypes raccordés |
| 6 | Quotient polynomial des coquilles étendues | Couverture exhaustive et mêmes morceaux | Dégénérescences, profils de capacité, oracle au-delà des petits cas |
| 7 | Projection et tête, puis voie un seul ordre | Objets nommés et diagnostics de perte séparés | Qualité et coût de chaque chaîne réellement servie |

Ces priorités ne ferment pas une borne globale. L'hérédité « toutes les arêtes q2 d'un q3 admis sont
admises » est fausse ; elle ne peut servir de réduction du catalogue. La seule croissance favorable
d'un index, d'un noyau ou d'une sortie observée ne borne pas toute la chaîne.

Les estimations privées de cycles par boule sont des modèles pour choisir des expériences. Elles
ne prouvent aucun plancher matériel universel et n'établissent pas l'impossibilité de K10 en 100 ms.
Mesurer le pipeline réel, avec sorties et verticales, avant de réviser une cible. Une voie GPU demande
un port et sa qualification propres ; des chronos CPU sur une machine G4 ne sont pas des chronos GPU.

Chaque clôture indique source/commande, domaine, juge, plancher atteint, écarts, mutants survivants,
temps mur, travail total et pic de mémoire. Les lecteurs Python passent en normal et sous -O.
Les grandes matrices se déroulent dans le harnais G4 autorisé, à partir d'une source figée ; aucun
benchmark local sous charge ne devient une promesse de temps sur les trames.
