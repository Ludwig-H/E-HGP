# Audit d'architecture : q3/q4 et tour FULL explicite en 100 ms sur G4

24 septembre 2026. Note **audit-only**, sans modification du moteur ni appel GCP.
Lecture au HEAD `e447847ae` du clone isolé :
[`PLAN_CRITIQUE_100MS_FULL_20260924.md`](PLAN_CRITIQUE_100MS_FULL_20260924.md),
[reçu G4 R20](../receipts/g4_tower_r20_20260924/README.md),
`src/gen/pipeline/wspd_q34.cpp`, `src/gpu/filter_runner.cu`,
`src/gpu/{certificate,lanes}.hpp`, `src/chain/tower_chain.cpp` et
`src/tower/forest/full_ball_tower.hpp`. Le reçu R20 exécute le moteur au
commit `48791e72`, non celui de cette note. Les idées qui suivent sont des
**hypothèses d'architecture et des portes de réfutation**, pas un gain acquis.
Contrat visé : **tous** les ordres K=1..5 d'une trame SemanticKITTI entière
sans sol, grille 1 mm, tableaux FULL explicites et prêts à consommer dans
100 ms de latence chaude sur une G4. Préparation du masque hors chrono HGP
mais mesurée séparément ; aucune compression à décoder hors des 100 ms.

## Le travail que le parallélisme actuel ne fait pas disparaître

R20 `08/000000`, 39 885 sites, K5/s8/W48, produit 1,109 s pour la chaîne.
Les chiffres ci-dessous proviennent de
[`vm/probe_0.stdout`](../receipts/g4_tower_r20_20260924/vm/probe_0.stdout) ;
les durées superposées ne s'additionnent pas mécaniquement.

| Étape | Exposition R20 | Mur/noyau marquant |
| --- | ---: | ---: |
| Front q3/q4 sur CPU | 3 133 819 rectangles | 106,454 ms |
| S2 rectangle puis paire, GPU | 103 861 099 paires couvertes ; 23 686 751 développées ; 2 043 612 survivantes | 105,397 / 63,652 ms |
| S3 certificats, GPU | 2 043 612 cœurs ; 359 707 275 sites de cœur ; 1 143 235 arêtes entièrement fermées ; 900 377 covers restants | 117,056 / 90,105 ms |
| S4 voies, GPU | 708 686 arêtes demandées ; 2 009 427 tâches ; 849 780 présentations renvoyées | 111,947 / 56,540 ms, dont 29,964 ms de « transfert » composite |
| Catalogue + census global | 1 306 696 clés ; 100 689 614 visites de nœuds et 6 097 121 tests feuille | 27,911 ms de fusion + 10,056 d'index + 98,852 de census |
| FULL explicite | 3 621 785 représentants ; 1 289 447 MEB ; 31 708 173 visites d'index | 421,527 ms, dont validation 70,516, cibles statiques 198,500, lots 56,518 |

Le filtre, le certificat et les voies utilisent **déjà** le GPU, avec
3 008 warps annoncés pour S3/S4 ; augmenter seulement le nombre de
workers CPU n'est donc pas une refonte crédible. Le seul front CPU dépasse
les 100 ms. Même un Kruskal instantané laisserait q3/q4, census et cibles
statiques. À l'inverse, si q3/q4 tenait 35 ms, q2 actuellement recouvert
à 103,500 ms redeviendrait le chemin critique. L'enveloppe expérimentale
du [plan critique](PLAN_CRITIQUE_100MS_FULL_20260924.md) est 10 ms
préparation, 35 ms maximum de q2 et q3/q4, 15 ms catalogue/census, 35 ms
FULL, 5 ms de marge : **aucun étage n'est aujourd'hui qualifié** à ce
plafond. Les trois trames R20 sans sol de la seule séquence 08 prennent
1,010/1,109/1,260 s à K5, pas moins d'une seconde.

La dépendance géométrique réelle est : propriétaire/index immuables →
décomposition en rectangles WSPD → rejet universel de voies par rectangle
→ rejet S2 par paire → certificats S3 par arête survivante → graines q3,
événements q4 et census des présentations → clé globale dédupliquée avec
intérieur strict **et coquille entière** → FULL. La phase S4 q4 peut
réutiliser les graines q3 possédées d'une arête ; elle ne devient pas
indépendante par un simple changement de thread. q2 démarre après le
front q3/q4 et peut recouvrir les appels appareil, mais la fusion et FULL
attendent les deux catalogues. Aujourd'hui les hôtes imposent aussi des
frontières `rectangles → survivors → certified masks → records` :
`run_wspd_q34_batched()` matérialise puis vérifie chaque tableau, et les
trois appels GPU ont chacun leurs allocations/copies de listes et leurs
retours d'état. Cette barrière **logicielle** est attaquable ; les
dépendances de preuve ci-dessus ne le sont pas.

## Refonte A — graphe de tâches sur produits de nœuds, masques prouvés et résidence appareil

Remplacer le « front CPU de 3,1 M rectangles, téléchargement, S2 de
23,7 M paires, téléchargement, S3 » par une file **sur appareil** de
tâches `(nœud A, nœud B, masque q3/q4, ordinal stable)`, issue de la même
partition WSPD. Les niveaux de la file se construiraient par compte,
préfixe et dispersion en vrac ; pas de kernel ni de synchronisation hôte
par rectangle. Un rejet universel exact conserve seulement les bits
ouverts. Quand une tâche demeure trop massive, couper un seul de ses
facteurs en deux sous-rectangles disjoints et retester ces bits : les
témoins de la *nouvelle* tâche peuvent être choisis localement dans le
nuage, avec `h=K` au plus, puis `h_a/h_b` si leur coût est justifié.
L'échec du certificat ne supprime **aucune** paire et n'ajoute aucun
crédit ; le terminal descend jusqu'à des tuiles de paires, traitées par
warps, puis au chemin exact S2/S3/S4. La subdivision ne doit jamais
réintroduire `O(|A|²+|B|²)` pour construire les témoins : sélection par
index partagé, bornes par nœud et coût payé par tâche sont à mesurer.

Une preuve suffisante pour fermer un bit sur une tâche est : `T3=K−1`
ou `T4=K−2` **sites distincts** dont l'intérieur strict est garanti pour
**tous** les couples de la tâche et **tous** les centres admissibles de
la voie. Les seuils sont donc 4/3 à K5. Des bornes entières englobantes
peuvent être grossières, jamais optimistes ; les extrémités des couples
ne sont pas des témoins stricts. Les sous-tâches partitionnent exactement
le produit du parent et héritent seulement de son *masque*, jamais d'un
compte de témoins additionné sans identité/disjonction prouvée. Garder
les ordinals parent/enfant, les IDs originaux et les masques individuels
q3/q4 ; un bit fermé ne justifie pas de supprimer l'autre. Le module
S2 actuel est un oracle de ce préfiltre, pas la source d'une nouvelle
heuristique de signe flottant. En cas de borne incertaine, le traitement
exact actuel est obligatoire.

Une deuxième preuve **optionnelle après S2 et avant tout cœur** peut
compter des nœuds BVH du nuage entier uniformément intérieurs sur une
cellule de centres. Le [contre-audit des 32 coins](CONTRELECTURE_FORMES_BVH_AVANT_S3_20260924.md)
prouve le test `max f<0`, l'antichaîne de plages et les seuils séparés ;
il ne donne pas une borne de temps. Une fermeture des deux bits évite
`make_diametral`, `load()` et tout cover S3. Une fermeture d'un seul bit
laisse le cover de l'autre ; la masse `F_e` n'est alors **pas** entièrement
épargnée. Sur le pilote **K10** de 256 arêtes stratifiées, 160 035 tests
nœud-cellule, 5,12 M coins et 2,93 M tests ponctuels n'ont ajouté que
43 fermetures complètes au core, de masse `F=1 544` :
[`b_spatial_block_probe_plan_20260924/README.md`](b_spatial_block_probe_plan_20260924/README.md).
Porter **ce** parcours tel quel sur G4 n'est pas économiquement plausible.
K5 a d'autres seuils : son [trace S2/S3 propre](b_s2_trace_k5_20260924/README.md)
fournit 27 099 survivants et `ΣF=298 205` sur un quartier physique de
1 288 sites. Sa [sonde BVH K5 séparée](b_spatial_block_probe_k5_20260924/README.md)
ferme 133/256 arêtes contre 87 pour le core, mais paie **6,842 M**
évaluations exactes pour `ΣF=5 634` dans l'échantillon : même en supprimant
les 2,536 M tests de sites aux centres témoins, les 4,262 M coins restent
très au-dessus de la masse de cœur éligible. Aucun gain intégré.
Un bloc-témoin doit gagner du
**travail total** avant toute montée G4 ; on ne peut extrapoler le
pourcentage de fermetures d'un échantillon au nuage entier.

La sortie de A serait une suite déterministe de présentations exactes
sur l'appareil. Trier/réduire par **clé canonique complète**, arité puis
support, conserver le représentant minimal et vérifier profondeur/coquille
cohérentes pour toutes les présentations d'une clé, comme
`gather_presentations()`. Ensuite seulement, lancer un census global
batché des clés uniques contre un index partagé : les 1,307 M clés et
leurs coquilles ne peuvent pas être déduites du seul support émetteur.
Des tuiles de clés × BVH avec repli exact doivent publier intérieur,
**coquille entière** et IDs d'origine ; aucun K-troncage de coquille.
Le catalogue n'est pas « gratuit » parce que les présentations résident
sur GPU. Un appel à `ball_census` par clé recopié tel quel reste une
charge potentiellement incompatible avec 15 ms. L'API doit fixer si les
tableaux FULL finaux sont sur hôte ; si oui, leurs copies font partie des
100 ms, même avec un état GPU persistant et des buffers épinglés.

Le test de décision pour A n'est **pas** le nombre de rectangles supprimés
mais `temps(front+S2+S3+S4+catalogue+census)`, volumes et repli compris,
avec *même ensemble de clés et mêmes supports*. Il faut constater la
baisse de `paires développées`, survivants, `ΣF` réellement évitée **avant
chargement**, tâches S4 et visites du census, sans explosion des tâches
subdivisées ni de VRAM. À 000000/K5 les trois noyaux S2/S3/S4 seuls
totalisent 210,297 ms en série : réduire les copies sans réduire leur
travail ne satisfait pas la fenêtre de 35 ms q3/q4.

## Refonte B — cibles terminales batchées et graphe d'événements FULL

La géométrie des cibles statiques est **indépendante de la forêt vivante** :
`static_terminal()` ne lit que catalogue/index immuables, rang et graines
fermées ; `prepare_static_order()` groupe déjà les facettes par clé,
résout une cible par groupe, puis redonne le BallId aux ordinals. Préparer
sur GPU (ou CPU/GPU avec files de repli) **toutes les clés de facettes
uniques de K2..5**, en lots indépendants, avec MEB et recherche d'intrus
exactes. Le noyau GPU peut traiter les cas simples, mais tout débordement
de largeur/capacité ou ambiguïté doit rester en file explicite et être
résolu exactement **avant** publication ; `FullBallBatchResolver` tel
qu'il est câblé désactive actuellement `run_orders_parallel()`
(`full_ball_tower.hpp`, constructeur vers ligne 403). Un callback GPU
isolé sans nouveau planificateur pourrait donc **ralentir** la tour.
Épingler `(K, clé facette, premier consumer, ordinal)` et valider cible,
fenêtre de rang et niveau strict pour chaque demande.

Le second objet est un graphe d'événements par K : un sommet par bloc
`(K,BallId)`, plus les sites initiaux à K1 ; chaque facette résolue donne
une arête vers son terminal de niveau strictement inférieur, étiquetée
par le **rang exact du niveau du bloc source**. Les composantes au seuil
strict `<λ` donnent les parents et au seuil fermé `≤λ` les groupes de
blocs. Grouper par `(K, λ, composante fermée)`, attribuer les actions dans
l'ordre canonique du premier bloc, puis construire nœuds, parents,
successeurs et contributions par comptes/préfixes/dispersions. Le
[lemme du maximum d'ID marqué](PHASE_A_MAX_ID_COMPOSANTE_20260923.md)
explique comment retrouver les IDs historiques des racines sans rejouer
chaque petit plateau, **à condition** de disposer de requêtes exactes de
composantes ouvertes/fermées. Une forêt minimale pondérée peut conserver
tous les préfixes, mais **ne remplace ni les facettes/cibles, ni les
contributions, ni l'ordre de première population, ni les images
verticales**. Une construction en `O(E × nombre_de_niveaux)` ou un
kernel par niveau serait rédhibitoire. Le choix de l'algorithme GPU de
connectivité temporelle reste un verrou, pas une simple formalité.

Pour chaque K, les quatre dépendances de FULL à préserver sont :
résolution géométrique des cibles avant graphe ; composantes `<λ` avant
parents et `≤λ` avant groupe ; IDs/actions déterministes avant liens
`next` ; puis, entre ordres, populations attribuées au **premier** usage
canonique et images verticales prises aux coupes fermées exactes de K−1.
On peut former ces dernières en lots après les cinq histoires, mais pas
prendre la racine finale de K−1 à la place de la racine à l'ancien
niveau. Le résultat doit matérialiser les mêmes `FullNode`, CSR parents,
successeurs, contributions, banque et `lower_nodes` que R20. Le volume
utile R20/K5 est 207 496 040 octets pour les cinq tableaux connus, plus
26–74 Mo logiques de banque selon les comptes publiés
([audit des octets](CONTRE_AUDIT_FULL_R20_OCTETS_100MS_20260924.md)).
Ce volume **ne démontre pas** un plancher supérieur à 100 ms, mais impose
d'inclure la copie appareil→hôte si l'API hôte demeure le contrat.

La porte économique pour B est le mur de **tout** FULL, non celui de
Kruskal/DSU : R20 mesure 198,500 ms de cibles, 70,516 de validation,
56,518 de lots et 95,882 ms cumulés pour populations/images/banque/
encodage. Supprimer uniquement le tri, même parfaitement, ne règle pas
ces postes. Il faut montrer `≤35 ms` pour cibles exactes + connectivité
temporelle + attribution + expansion **ensemble**, sur le même catalogue.
À défaut d'une connectivité pondérée exploitable sans rescans des niveaux,
la proposition B doit rester sidecar de recherche, pas remplacer le
chemin exact de production.

## Ordre des portes, avant dépense G4

1. **Oracles et transaction.** Sur petits nuages adversariaux, comparer
   chaque masque rectangle/paire/pré-cœur, toutes les clés, intérieurs,
   coquilles et IDs, puis la tour FULL littérale contre le produit. Tester
   coordonnées doubles, frontières `f=0`, cellules tangentes, coquilles
   étendues, plateaux d'égal niveau, mêmes clés par supports différents,
   plusieurs K, refus de capacité et reprise. Une comparaison de digests
   ou `complete_relative` seule n'est pas une preuve de complétude.
2. **Panneaux appariés sans GCP.** Pour les coupes physiques 8k/16k/32k
   d'une même trame selon deux plans passant par le capteur, et densités
   appariées, mesurer séparément `R`, masse WSPD, paires S2 développées,
   survivants, `ΣF`, formes calculées, covers, tâches/événements q4,
   clés uniques, visites du census, facettes/cibles FULL, événements,
   octets, CPU·s et mur. Calculer les ratios aux doublements **du travail
   total**, pas seulement d'un noyau ; répéter K5/K10 et `s=8/10/12`.
   Une pente sous-quadratique observée sur ces panneaux est un diagnostic
   de régime, jamais une borne générale ni un succès de trame entière.
3. **Sidecar FULL sur catalogue épinglé.** Comparer *par ordinal* cibles
   de facette, labels `<λ`/`≤λ`, groupe, parents, IDs, successeurs,
   contributions, population première occurrence, banque, images et
   tableaux encodés. Capturer histogrammes de tailles de lots, niveaux,
   longueurs de chaînes et pics RSS/VRAM ; comparer coût total du sidecar
   au chemin courant avec mêmes entrées. Les 3 000 histoires abstraites
   du lemme ne suffisent pas sur LiDAR.
4. **G4 seulement après préflight vert.** Le [préflight R21/v26](CONTRE_AUDIT_R21_PREFLIGHT_20260924.md)
   était encore rouge à cette lecture ; le réparer et le rejouer avec
   hashes épinglés avant un SPOT. Puis même entrée/sortie explicite,
   latence chaude **et** froide, plusieurs trames de plusieurs séquences
   sans sol ; annoncer maximum et répétitions, K1..5 sous 1 s puis
   100 ms. Publier séparément segmentation, HGP sur masque figé et total.
   Tester aussi K1..10 et trames brutes avec sol, `s=8/10/12`, et garder
   float32 brut comme objectif secondaire. Pour 10 M+ à plusieurs dizaines
   de millions de sites, vérifier compteurs/ordinals 64 bits, arènes,
   tuilage, refus sans sortie partielle, RSS/VRAM et temps sans extrapoler
   les 100 ms de 40k sites.

**Verdict.** Les deux refontes sont complémentaires : A attaque le nombre
de paires, les cores et les barrières hôte/appareil ; B attaque les millions
de cibles et les petits plateaux FULL. Aucune n'est actuellement qualifiée,
et la sonde BVH K10 naïve plaide *contre son port direct*. Si A ne réduit
pas à la fois travail de S2/S3/S4 **et** coût du census, ou si B doit
rescanner toutes les facettes par niveau, 100 ms sur une G4 n'est pas une
projection crédible à partir de R20. La sortie FULL explicite et exacte
reste le juge, jamais la seule vitesse d'un tri.
