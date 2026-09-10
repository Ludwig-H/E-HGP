# Réflexion d'auditeur : où est le facteur mille entre la tour actuelle et le contrat d'une seconde

10 septembre 2026, second auditeur. Analyse à partir des compteurs scellés du
constructeur ; aucune mesure nouvelle, aucun temps revendiqué, aucun statut
promu. `public_status=not_claimed`.

## 1. Les ordres de grandeur, tels que scellés

À 32 000 points, `uniform`, s=8, K1..10, un fil
(`receipts/full_ball_runs_20260910`) : 965 s au total, dont WSPD 290 s,
tri/RLE 11 s, préfiltre 51 s, census 39 s, construction FULL 566 s, empreinte
9 s. Objets : 13,5 millions de boules, 17,2 millions de nœuds, 10,4 millions
de contributions, 45,5 millions de représentants résolus, 52,0 millions
d'appels MEB, 6,8 millions de requêtes d'intrus, 34,2 millions de demandes au
normaliseur inférieur. À 50 000 points (refus historique
`full_extra_shell_50000_20260906`) : 21,7 millions de candidats, 21,5 millions
de boules, **4,95 milliards de nœuds d'index visités par la WSPD**,
1,29 milliard d'évaluations de coins, 2,19 milliards de nœuds visités par le
census, 55 s de génération sur huit fils.

Le contrat demande toute la tour K1..10 à 50 000 points **sous une seconde**,
puis sous 100 ms. Entre 965 s à 32k et une seconde à 50k, le facteur à trouver
est de l'ordre de 1 500 à 2 000. Aucun étage n'en porte seul la moitié : la
génération vaut 30 %, la construction FULL 59 %.

## 2. Ce que la sortie impose, indépendamment de l'algorithme

Dix forêts à 17,2 millions de nœuds et 10,4 millions de contributions à 32k,
donc de l'ordre de 27 millions de nœuds à 50k. Même parfaitement produite,
cette sortie pèse quelques centaines de mégaoctets ; l'écrire en mémoire hôte
coûte des dizaines de millisecondes, pas des secondes. La borne de sortie
quadratique (`CROISSANCE_ET_BORNE_DE_SORTIE.md`) n'interdit pas la seconde
sur `uniform` 50k ; elle interdit de la promettre pour tout nuage. Le contrat
doit donc nommer sa famille, et la sortie doit rester sur le device jusqu'à
l'export si l'on vise 100 ms.

## 3. Où le parallélisme est déjà prouvé exact

- **Témoins WSPD par lots** : le front par lots égale le front scalaire à
  l'ordre, au grand-livre et au travail près (`suite_cache_20260910` § 4).
  Les 4,95 milliards de visites d'index à 50k sont des requêtes indépendantes
  par rectangle : c'est l'étage le plus naturellement device, et c'est là que
  le constructeur porte ses efforts.
- **Préfiltre et census** : route device existante, simulée hôte, non encore
  qualifiée sur device (`census_route.cuh`).
- **Résolution des facettes** : phase statique prouvée
  (`NOTE_PHASE_STATIQUE_MEB.md`) ; les 52 millions d'appels MEB d'un ordre sont
  indépendants entre représentants, et le choix des intrus est libre.
- **Phase temporelle** : union-find par lots, normaliseur monotone en
  O((N+Q)α) ; à 32k, 34 millions de demandes et 81 millions de pas find : de
  l'ordre de la seconde CPU, à réduire par tri par niveau des arcs plutôt que
  par requêtes individuelles.

## 4. Les trois leviers que je recommande d'instruire, dans l'ordre

1. **Dédoublonner les représentants avant toute MEB.** 45,5 millions de
   représentants pour 13,5 millions de boules à 32k : chaque boule régulière
   demande q facettes, et une même K-facette est un retrait de plusieurs
   boules. Le mémo direct-mapped de 16n entrées capture une partie de ce
   partage au prix de 12 Gio à dix millions de points ; un tri-unique des clés
   de représentants par ordre, dans la phase statique, capture tout le partage
   sans résidence évictive ni aléa de collision, et il est lui-même un tri par
   lots. La mesure à 8 000 points de la suite dit quelle fraction des 11,96
   millions d'appels MEB disparaît (§ 5).
2. **Ne calculer qu'une MEB par facette résolue, puis suivre des arcs.**
   Après dédoublonnage, la chaîne d'une facette ne dépend ni de l'état ni de
   l'intrus ; elle se calcule une fois par facette et par ordre, hors ligne. Le
   reste est de la normalisation.
3. **Compter avant de porter.** Les compteurs existent (`representatives`,
   `resolver_meb_calls`, `intruder_queries`, `wspd_witness_nodes`) ; un port
   GPU se juge d'abord sur l'égalité de l'objet et des compteurs logiques, puis
   seulement sur le temps, sur hôte réservé, deux échauffements et dix nuages
   frais par famille (`CONTRAT_PERFORMANCE.md`).

## 5. Ce que la mesure à 8 000 points dit (`mesure_cache_8k/`, header publié `0b72b4e9` + cache WIP, 4 fils, diagnostic)

| Famille | boules | représentants | requêtes cache | hits | taux | évictions | semis | appels MEB | sans cache |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `uniform` | 3 113 381 | 10 456 312 | 10 396 562 | 5 616 343 | 54 % | 6 185 343 | 2 396 646 | 6 227 265 | 11 957 768 |
| `scanline_overlap_multiecho` (15 060 coquilles supplémentaires, 66 pas à rayon égal) | 165 868 | 422 112 | 394 924 | 318 660 | 81 % | 23 784 | 144 509 | 485 081 | — |

Le partage des représentants entre boules est réel : plus de la moitié des
requêtes retrouvent une facette déjà résolue, alors même que le mémo
direct-mapped (131 072 emplacements pour 8 000 points) évince 6,2 millions de
fois. Les appels MEB sont divisés par 1,9 ; la construction de la tour passe de
122 à 71 s dans ces conditions non appariées. Un tri-unique des clés de
représentants par ordre, dans la phase statique, capturerait tout le partage
sans éviction ni résidence de 16n : au moins 3,3 millions des 10,5 millions de
résolutions (31 %) sont des doublons exacts de clés déjà stockées, et les
évictions en cachent d'autres. Le levier 1 vaut donc son coût ; il se mesure
par le nombre de clés uniques par ordre, que la phase statique fournit
gratuitement.

## 6. Ce qui ne bougera pas

La complétude S1 de la génération, les quatre coquilles supplémentaires de
50k et les parents 1/2/2, l'égalité des dix forêts entre routes, les refus
transactionnels et l'absence de tout préfixe publié. Aucun temps de composant
ne satisfait le contrat ; seule la tour entière, mesurée et scellée, le fera.
