# FULL hors encode : déplacer le parallélisme à l'intérieur des ordres

27 septembre 2026, lecture de `a7e80d7f9`, profil grille 1 mm/u18,
`exploration_v9_hors_registre`, `not_claimed`. Aucun moteur modifié,
aucune compilation ni nouvelle mesure, aucun appel GCP. Les chiffres
ci-dessous relisent une capture close ; les transformations proposées
restent à implémenter et à qualifier.

Suite concrète à déléguer : [manifeste et rejeu natif de A](NEXT_MANIFEST.md).
Cette feuille inventorie les prototypes réutilisables et borne le prochain
lot, avant tout nouveau MSF ou port GPU.

## Décision proposée

**La prochaine expérience FULL doit porter sur la fabrication des lots,
pas sur une nouvelle variante d'encodage du même draft.** Garder d'abord
le catalogue et les cibles exactes de phase 0 ; exporter leurs blocs actifs
et comparer un constructeur événementiel à `order_lots`. Ce constructeur
doit rendre le même draft, les mêmes ancres et le même historique, avant
de remplacer les verticales. Il pourra ensuite alimenter l'encodeur
parallèle déjà étudié : produire des millions de nœuds explicites reste
dans le budget, pas dans un décodage différé.

Le graphe temporel, la forêt couvrante minimale et les ancêtres pondérés
ne sont pas des idées nouvelles de cette note : voir
[le modèle de phase A](../b_full_phase_a_work_20260926/README.md) et
[le partage entre ordres](../FULL_PARTAGE_INTER_ORDRES_20260926.md).
Le complément présent fixe le **premier objet d'essai**, les raccords
exacts, la séparation des gains, et les postes qu'un vrai port parallèle
doit effectivement payer. Le modèle Python antérieur utilise Kruskal
séquentiel ; il ne qualifie pas un constructeur CPU/GPU parallèle.

## 1. Les 252–256 ms ne sont pas cinq encodages à additionner

[analyze.py](analyze.py) rejoue l'autorité de la capture
`g4_core_warm_20260927`, via le lecteur du
[chemin critique](../b_critical_path_20260927/README.md), puis expose les
phases de ses premiers passages ON, processus 0 et 3. Une seule trame
08/000000 sans sol, K1..5/s8, 39 885 sites ; pas de nouvelle répétition,
pas de médiane et pas de transfert aux passages chauds.

| phase / fenêtre, ms | passage 0 | passage 3 |
| --- | ---: | ---: |
| validation/indexation du catalogue | 34,774 | 34,010 |
| phase 0, cibles géométriques | 86,012 | 84,618 |
| fin exposée de construction des lots A | 105,554 | 103,964 |
| populations B exposées | 11,550 | 11,140 |
| images C exposées | 15,446 | 15,580 |
| banque | 0,911 | 0,847 |
| reste hors encode, non attribué | 1,395 | 1,375 |
| **tour hors fenêtre d'encodage** | **255,642** | **251,534** |
| fenêtre d'encodage simultané des K | 48,343 | 37,237 |

Les lignes exposées découpent le calendrier actuel ; A, B et C de
différents K se recouvrent déjà. `lots_by_k` vaut par exemple
73,570 / 50,429 / 78,035 / 107,667 / 151,418 ms dans le passage 0 :
**leur somme n'est pas le mur A**. Remplacer A change aussi ce recouvrement
et la contention. Aucune différence du tableau n'est une prédiction de
gain ni un minorant de tous les futurs algorithmes.

Dans la phase 0 du passage 0, collecte 19,247 ms, classes 12,224 ms,
champ historique `static_sort` 3,724 ms, résolution et gather 50,756 ms.
En mode haché, `static_sort` mesure notamment l'index de graines, pas
simplement un tri. Dans la validation, tri des niveaux 9,464 ms,
rangs 4,748 ms et programmes/populations 10,955 ms ; ces étapes sont
déjà parallélisées. Le passage 3 confirme le même ordre de grandeur.
Supprimer le seul tri ne traite donc pas le verrou.

## 2. Lecture du code : ce qui reste sériel et ce qui ne l'est pas

Source principale : [full_ball_tower.hpp](../../src/tower/forest/full_ball_tower.hpp).

| travail | état réel à la base lue | transformation pertinente |
| --- | --- | --- |
| `validate_catalogue`, lignes 1795–2044 | contrôles réguliers, index de clés, niveaux, rangs et scatter des programmes déjà parallèles ; préparation des coquilles étendues encore parcourue en ordre d'index | ne pas refaire ces ports ; mesurer séparément les coquilles étendues, préserver leur table et le premier refus |
| `run_orders_overlapped`, lignes 869–1131 | phase 0 parcourt Kmax→2 en série ; chaque K utilise les workers ; A démarre dès ses cibles prêtes | remplacer le calendrier seulement avec un coût mur apparié, pas en sommant les K |
| `resolve_hashed_order` / `static_terminal`, lignes 2090–2146 et 2360–2539 | classes exactes déjà parallèles ; chaque classe garde une chaîne MEB→intrus→échange séquentielle | états actifs en vagues ou réduction de profondeur, chantier géométrique distinct |
| `order_lots` / `order_block_lean` / `order_lot`, lignes 1240–1406 | les K sont concurrents ; **dans chaque K**, lots, recherche de racines, modifications des successeurs et appends restent séquentiels | graphe événementiel global, sans lancement GPU par niveau |
| `order_populations`, lignes 1511–1542 | noms des premières contributions parcourus sériellement par K ; lignes construites en parallèle | minima d'occurrence + préfixes stables, après IDs canoniques des événements |
| `order_images`, lignes 1557–1591 | DSU inférieure monotone, activation des niveaux et dépendance à `lower_nodes[parent]`, sérielles dans K | naissance descendante + requêtes historiques indépendantes sur l'index de A |
| `finish`, lignes 721–771 | encodages déjà concurrents entre K | utiliser ensuite le raccord intra-K qualifié ; ne pas le présenter comme un nouveau parallélisme inter-K |

Les allocations des groupes de plateau (`owners`, DSU locale,
`vector<vector<size_t>>`, actions), les réserves par ordre, les préfixes
et les destructions doivent rester dans le coût. L'exécution parallèle
des K ne permet que K grandes tâches pour la partie sérielle de A ;
augmenter les CPU de cette boucle seule ne crée pas davantage de travail
indépendant.

### Partage inter-K : garder la bonne frontière

- Une boule régulière, avec m=p+q, émet déjà seulement q≤4 représentants
  à K=m−1, puis une contribution sans représentant à K=m. Pas de
  K copies d'une énumération exponentielle à supprimer.
- Une population de boule est déjà partagée. `first_order`,
  `population_offset` et les références différées évitent sa duplication.
- Les clés, rangs exacts, catalogue et index géométrique sont déjà communs.
  Les programmes sont des vues filtrées de cet ordre global.
- En revanche, `anchors.assign(balls.size(), ...)` est payé dans chaque
  K : remplacer ces K tableaux denses par les seuls blocs actifs est un
  vrai levier mémoire. Le fait qu'une boule existe dans deux ordres ne
  fusionne **jamais** leurs ancres, composantes ou identités terminales.
- Les variantes actuelles repassent sur les blocs pour compter les
  représentants, les émettre et préparer A. Un manifeste commun de blocs
  peut éviter ces répétitions et leurs temporaires, mais leurs coûts
  doivent être mesurés ; aucune économie d'une MEB inter-K n'en découle.
- Les historiques/ancres issus de A, puis la nouvelle DSU de C, puis le
  rejeu structurel de l'encodeur sont trois traitements du même historique
  sous des interfaces différentes. Partager un **index historique
  immuable** est légitime ; partager une racine finale à toutes les dates
  est faux.

## 3. Première couture à implémenter : programmes → mêmes drafts et ancres

### Manifeste possédé et contrôlé, avant de changer la géométrie

Une entrée par `(K, bloc actif)`, plus les sites initiaux de K1. Le premier
prototype garde exactement les cibles de phase 0 actuelle : il ne doit
changer ni MEB, ni intrus, ni identité du terminal. Tous les tableaux
appartiennent au harnais, sont immuables pendant le sidecar, et sont liés
au même propriétaire/index/catalogue. Les seuls drafts finaux ne suffisent
pas : ils omettent les portails silencieux et ne rendent pas toutes les
ancres de blocs.
Ce manifeste ne forge pas un `SealedCatalogue` et ne dispense d'aucune
validation publique actuelle ; sa provenance native est contrôlée par
le harnais, sans étendre la frontière de confiance du moteur.

| champ proposé | fonction exacte |
| --- | --- |
| `order_begin[K]` et ID de bloc actif | domaines disjoints ; arithmétique globale vérifiée en u64 |
| `ball`, `program_ordinal`, `run` | identité originale, ordre canonique dans K ; `run=level_run[ball]+1`, zéro réservé aux sites initiaux de K1 |
| `lot_first_ball` | représentation rationnelle **du premier bloc du lot entier**, conservée même si le premier groupe est silencieux |
| `representative_begin` et `target_block` | CSR des cibles du même K ; rang cible strictement inférieur ; K1 vise les sites initiaux |
| `shell_mask`, `include_interior` | contribution exacte du bloc, pas seulement son existence |
| table d'accès `(K, BallId) → bloc actif` | résout les cibles et les naissances inférieures, sans table dense K×catalogue ajoutée en cachette |

`FullBallStaticTrace` donne déjà cibles et premiers ordinals de classes,
mais pas les fenêtres de blocs, contributions et ancres. Pour le premier
harnais, instrumenter explicitement une copie audit-only épinglée du
Builder pour ces exports, en une seule unité de traduction, ou reconstruire
le manifeste dans un producteur séparé et en vérifier toutes les tranches
contre la trace. Dans ce second cas, compter reconstruction/copies comme
instrumentation : aucune mesure de gain moteur ne doit les effacer puis
prétendre à un coût total nouveau. Ne pas introduire implicitement le
callback batch : il désactive `run_orders_parallel()` dans la base actuelle.

### Calcul de A, sans barrière par niveau

1. Construire le graphe auxiliaire : une arête non orientée par représentant,
   entre le bloc et son terminal ; poids = rang du bloc source. Les sites
   K1 naissent au rang zéro. Les blocs sans arête sont conservés, notamment
   les naissances contributives. Aucune arête entre deux K.
2. Construire une forêt couvrante minimale par poids. Le prototype témoin
   peut commencer par Kruskal ; cela ne qualifie pas le parallélisme. Le
   candidat massif doit payer sa construction parallèle, par exemple
   contractions de Borůvka avec clé totale `(rang, ordinal_arête)` pour
   départager les égalités. Le départage ne devient pas un sous-niveau
   géométrique : les contacts égaux restent simultanés.
3. Enraciner cette forêt, puis construire les tables ancêtre/maximum du
   chemin par doublement. Le poids n'est **pas** nécessairement monotone
   sur une branche de la forêt. Un test sur la seule dernière arête serait
   faux. L'enracinement doit aussi être parallèle dans le bras massif,
   pas une DFS sérielle cachée.
4. Pour chaque bloc au rang r, rechercher son composant **fermé ≤r**.
   Grouper par `(K,r,label_fermé)` ; le groupe est ordonné par son plus
   petit `program_ordinal`. Pour chaque cible de ce groupe, rechercher
   le composant **ouvert <r** et dédoublonner ces labels : ce sont ses
   parents avant le contact simultané, pas des fusions binaires choisies
   par le calendrier.
5. Garder un événement pour chaque groupe, y compris un portail silencieux.
   Pour chaque label parent ouvert, trouver son dernier événement avant r.
   Zéro parent ou au moins deux : l'événement crée un nœud. Un parent :
   il renvoie vers l'événement précédent. Résoudre ces renvois acycliques
   par doublement, puis attribuer les IDs par préfixe dans l'ordre canonique
   `(K,r,premier_bloc_du_groupe)`.
6. Scatter des ancres et du draft. Parents triés par **ID historique de
   nœud**, non par label de la forêt ; contributions par ordre de bloc
   original dans le groupe. Un événement à un parent sans contribution
   ne paraît pas dans le draft ; il reste dans l'index historique. À un
   parent avec contribution, garder la contribution datée, sans créer
   un nouveau nœud. Les lots vides de toute action ne sont pas publiés.

Pourquoi ce remplacement conserve A : une forêt couvrante minimale
conserve les composantes à chaque préfixe de poids ; les coupes ouvertes
et fermées redonnent donc les racines pré-lot et leurs groupes. Dans un
arbre enraciné, le label est le plus haut sommet joignable sous la coupe.
Lorsque ce label disparaît par fusion vers un ancêtre, il ne revient plus.
Son prédécesseur d'événement identifie ainsi l'état historique correct.
Les renvois vont strictement dans le passé ; leur compression retrouve
les IDs des naissances/multifusions, pas seulement la partition finale.
Ce raisonnement est celui des modèles antérieurs : le nouveau travail
à établir est le raccord à **tous les champs natifs et aux vrais programmes**.

### C, B et sortie : élargissement après la première porte A

Une fois A identique, calculer pour chaque nœud une naissance descendante
en suivant son premier parent, par doublement. La boule de cette naissance
possède un bloc à K−1 : une naissance ne peut survenir au rang inférieur
K=p+q_min−1, où le bloc possède nécessairement un représentant.
Interroger l'index A de K−1 à la coupe **fermée du nœud supérieur**, puis
résoudre son événement en ID historique. Vérifier la naturalité pour
**chaque** parent, pas seulement celui choisi pour la naissance.
La propriété utilisée est la transitivité des coupes fermées croissantes ;
une racine finale inférieure ne remplace pas cette requête historique.

Pour B, l'occurrence première d'une boule est le minimum du triplet
`(K, action canonique, contribution dans l'action)`. Minima segmentés puis
préfixes donnent les mêmes IDs de populations que le témoin. Construire
chaque ligne une fois, remapper toutes ses références ensuite. L'ordre
BallId seul ou le seul `first_order` ne suffit pas à fixer les IDs.

Conserver d'abord l'encodeur courant comme juge. Son remplacement par
l'encodeur parallèle qualifié est une expérience suivante, mesurant
**A + C + B + banque + écriture explicite + nettoyage**, pas seulement un
MSF ou le scan d'IDs. Les objets intermédiaires morts doivent être libérés
avant d'allouer les sorties si c'est le contrat du port mesuré.

## 4. La phase 0 est un autre chantier, à ne pas déguiser en partage inter-K

Après le raccord de A, une seconde expérience peut collecter les classes
de tous les K dans un espace de tâches, avec **K dans l'identité** et des
états actifs `{K, classe, facette triée, MEB, premier_consommateur}`.
Une vague fait un échange exact par état, les terminaux sortent, les autres
continuent. Les rounds dépendent de la profondeur des chaînes, pas du
nombre de niveaux du catalogue. Les tailles de vague bornent le scratch,
jamais la recherche. Mesurer effectifs par round, longueurs et déséquilibre.

Ce n'est pas gratuit : le produit utilise déjà les workers à l'intérieur
de K et fait commencer A pendant la résolution des K suivants. Une file
globale peut perdre ce recouvrement ou la localité Morton. Comparer le
**calendrier mur réel**, ne pas soustraire `sum(static_by_k)` à l'ancien
temps de chaîne.

Les preuves restent précises : MEB exacte, terminal du même K dans sa
fenêtre, niveau strict avant le consommateur ; au même rayon, même clé et
décroissance de la coquille sélectionnée ; graine = population entière,
jamais un support ou une coquille partielle. Pour garder exactement les
cibles du témoin, le choix d'intrus conserve le premier rang natif valide,
pas le premier thread GPU qui en trouve un. Un autre intrus ou le saut au
centre peut préserver une composante sans préserver le BallId terminal :
il exige alors la porte supplémentaire d'égalité des racines **pré-lot**.
L'index de selles isolé mesuré négatif ne devient pas une priorité nouvelle.

## 5. Travail, mémoire et portes de réfutation

Noter V blocs actifs, E représentants, G événements, N nœuds FULL,
P incidences de parents et C contributions. E et V viennent du catalogue
et de sa fenêtre : aucune borne sous-quadratique en nombre de points n'est
créée par la parallélisation.

Le premier format à tables d'ancêtres paie O(V log V + E + P + C) espace,
plus sorties et temporaires de tri réellement simultanés. Pour une
implémentation simple de Borůvka : O(log V) rounds avec balayage des E
arêtes ; si l'aplatissement des composantes paie O(V log V) par round,
la borne annoncée doit conserver ce terme, soit O(E log V + V log² V),
avant tris, enracinement et requêtes. Ne pas revendiquer automatiquement
O((V+E) log V) pour un code qui fait ce travail supplémentaire.
Publier les nombres de scans, sauts, comparaisons, lectures, écritures et
capacités ; un nombre logarithmique de rounds n'est pas un coût nul.

Portes dans cet ordre :

1. **Manifeste.** Tous les programmes/cibles/masques, ordinals et niveaux
   du même catalogue ; sites K1 explicites ; coquilles étendues intactes.
   Cible hors K, cible du même niveau, trou/duplication CSR refusés avant
   lecture indirecte ; offsets et produits vérifiés sans quota de recherche.
2. **A différentielle.** Comparer chaque racine pré-lot, groupe canonique,
   draft, ancre de bloc, successeur, naissance et rang. Pas seulement digest,
   nombres de nœuds ou composante finale. Deux enracinements et deux
   départages de forêt doivent donner les mêmes champs natifs.
3. **Contacts/non-régularité.** Égalités rationnelles avec représentations
   différentes, plusieurs groupes au même plateau, groupe dont le premier
   bloc est muet, portails silencieux, ABCZ contributif sans nouveau nœud,
   carré sans continuation et coquille produisant 32 parents. Aucun petit
   tampon de racines fondé sur la seule taille maximale de coquille.
4. **B/C et sortie.** Mêmes IDs/ordre/lignes de populations, masques datés,
   parents/successeurs/verticales ; naturalité sur chaque parent et coupes
   ouvertes/fermées à tous les niveaux exacts des petits oracles. Ne pas
   normaliser les rationnels à l'écriture. API transactionnelle, aucun
   résultat partiel ; conserver priorités des refus et coût payé sur échec.
5. **Mutants causaux.** Parent en coupe fermée ; silencieux omis ; égalités
   binarisées ; labels de MSF utilisés comme IDs ; même ancre entre deux K ;
   naissance inférieure remplacée par racine finale ; contribution de
   continuation supprimée ; premier contributeur confondu avec premier
   bloc du lot. Exiger chaque branche non vacue, Release/sanitizers puis
   TSan sur le bras CPU réellement concurrent.
6. **Mesure.** D'abord même manifeste, candidats/témoin alternés, copies,
   préfixes, tri, allocations et destructions inclus. Puis vraie chaîne
   appariée G4 avec même catalogue et sortie FULL explicite ; enfin
   uniforme 8k/16k/32k et coupes capteur LiDAR avec effectifs observés,
   plusieurs trames, brut, K5/K10 et s8/10/12. Ne pas confondre sommes
   par K, phases recouvertes et mur ; ne pas extrapoler une régression ou
   réussite CPU scalaire en résultat GPU.

**Verdict :** garder les fondements géométriques et les objets FULL ;
remplacer expérimentalement la dépendance chronologique de A, puis celle
de C. Les quelque 105 ms exposées de lots rendent cette expérience plus
directement pertinente que quelques pourcents d'encodage, mais la phase 0,
S3/S4 et le census restent à payer. Aucun objectif 100 ms n'est acquis.

## Provenance et reproduction

Les deux sources moteur sont épinglées dans le lecteur :
`full_ball_tower.hpp` SHA256 `124d3e9b52b1e6155e5a2ffd2a32cda7e5b403dda21155da2949b9ca8c90c0d0`,
`full_coverage_certificate.hpp` SHA256 `8259a3cb8110a9dd8333eaf48455def03bc886014d869f5fcf499898615a7a62`.
Le lecteur historique délégué est lui aussi épinglé avant import et
contrôle son snapshot, ses sources et les reçus fermés. Les lectures ne
créent pas une nouvelle qualification du moteur.
Les deux commandes suivantes ont fini avec code 0 et sorties JSON
identiques, statut `passed`, en modes normal et `-O`.

```sh
python3 -B morsehgp3D_v9/audits/b_full_construction_parallel_20260927/analyze.py /workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/snapshot.tar.gz
python3 -B -O morsehgp3D_v9/audits/b_full_construction_parallel_20260927/analyze.py /workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/snapshot.tar.gz
```
