# Audit de rupture v8 → v9 : algorithme k-Gabriel local

22 septembre 2026. Audit indépendant A sur le moteur v8 `a74e90f2`,
publié pour l'ouverture v9 `3595725a` ;
`phase=exploration_v9_hors_registre`, `backend=none`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.
Périmètre lu : parties I et II du [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
dont les définitions 20–22 et le théorème 2 (PDF 83–87), les
[fondements et contre-preuves](../../morsehgp3D_v8/audits/FONDEMENTS_ET_OBJET.md),
la reprise u18 `morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md`
(encore non commise au pin lu),
les sources et reçus v8 indiqués ci-dessous. Le moteur local lu part de
`a74e90f2` ; les audits complémentaires publiés sur `main` jusqu'à
`12294241` sont repris ci-dessous. Les travaux de reprise et de q3 global
float32 encore présents dans l'arbre partagé sont des états en cours, pas
une qualification v9.
GCP non utilisé.

Cette note donne un choix d'architecture et ses critères de décision. Les
détails mathématiques q3, q4 et le bilan systèmes sont dans les trois
notes associées : [q3](Q3_STRUCTURE_ET_BORNES.md),
[q4](Q4_STRUCTURE_ET_BORNES.md) et
[contrat/coûts/parallélisme](CONTRAT_COUTS_ET_PARALLELISATION.md).
Les échecs et reçus historiques restent à leur endroit v7/v8, sans copie ici.

## Verdict utile au constructeur

La v8 possède une bonne factorisation géométrique : supports positifs de
taille au plus quatre, propriété canonique de la plus longue arête,
témoins stricts par blocs, même index global, comptes et coquilles séparés,
atlas q4 à fragments exacts, clés de boule partagées et tâches CPU
possédées. **Le prochain saut doit empêcher le travail avant rejet.** Une
file plus fine, un tri plus rapide ou un autre facteur constant d'atlas
distribueraient encore des milliards de classifications.

La proposition v9 est de générer les boules k-Gabriel à partir de **petites
régions certifiées de centres de miniballs**, construites paresseusement
seulement là où le front laisse des supports possibles. Chaque région
partage un compte intérieur strict et une frontière de sites encore
ambigus. Les supports q3/q4 de cette région ne peuvent prendre leurs
autres sommets que dans cette frontière. C'est un certificat local de
famille de miniballs ; il ne construit pas la mosaïque de Delaunay ou le
diagramme de Voronoï d'ordre K du nuage entier. Ses cellules peuvent
néanmoins proliférer : leur **coût total** et celui de la frontière sont
des conditions à mesurer, jamais une borne gagnée d'avance.
Il manque encore l'invariant de génération qui couvre **chaque** arête
propriétaire, bloc de complétions et centre exact avant toute omission de
cellule. Jusqu'à sa preuve, le générateur v8 exhaustif reste le repli
complet. La paresse seule n'est pas une justification de complétude.

## Ce que la thèse fixe et ce que la v7 corrige

Au rayon r, la vraie cible est la composante de
`L_K(r)={c : au moins K sites sont dans B(c,r)}`. Le graphe Γ_K du
manuscrit a pour sommets **tous les ensembles de K sites apparus**, dont
les isolés ; ses adjacences viennent des unions couvertes au même rayon.
Les régions témoins de ces ensembles sont convexes, et leur graphe
d'intersection donne les composantes. Une seule hiérarchie de points ou
un graphe k-NN usuel n'est donc pas la tour demandée.

Pour la sortie FULL réduite, les minima Gabriel servent de feuilles et
les événements critiques de fusion sont Gabriel sous les hypothèses
régulières déclarées. Mais le graphe induit sur ces seules feuilles est
faux : une connexion silencieuse peut porter le parent et la date d'une
fusion ultérieure. La contre-fixture E5 réfute même la proposition 6
littérale du manuscrit, comme égalité des ensembles de points. La v9
doit recalculer les rattachements de facettes et les histoires datées,
conserver les plateaux exacts et les applications entre K, sans restaurer
un catalogue exhaustif Γ. Voir les [contre-preuves v7](../../morsehgp3D_v7/audits/receipts_gabriel_vertices_20260906/README.md)
et le [statut mathématique](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).
La généralité des grilles non régulières n'est pas un théorème acquis :
si une coquille supplémentaire pertinente échappe au domaine prouvé,
le constructeur doit refuser explicitement `unsupported_degeneracy`
jusqu'à une extension certifiée. Les fixtures de plateau doivent être
des portes FULL, non des exemples décoratifs.

Les seuils de rejet d'une famille de supports q sont
`h_q=Kmax+2−q` sites **strictement** intérieurs, disjoints et certifiés :
q2/q3/q4 valent 10/9/8 pour Kmax10, ou 5/4/3 pour Kmax5.
La puissance nulle reste un contact. Un support positif q≤4 ne borne ni
la coquille ni le nombre de minima. La
[construction v7](../../morsehgp3D_v7/audits/receipts_probe_meb_review_20260906/full_output_growth.md)
donne Ω(n²) feuilles FULL pour tout K fixé≥2 dans des nuages réguliers
à précision croissante. Le sous-quadratique **universel** de la sortie
explicite est donc impossible ; la v9 vise un coût sous-quadratique
mesuré et justifié **sur les régimes LiDAR déclarés**, sensible au volume
réel de sorties. Même des points sur deux arcs courts peuvent atteindre
la borne : « surface ou courbe » n'est pas à elle seule une hypothèse de
complexité suffisante.

## Les coûts à supprimer, dans leur vrai périmètre

| Preuve | Entrée et configuration | Ce qu'elle établit | Limite |
| --- | --- | --- | --- |
| [G4 spatial v8](../../morsehgp3D_v8/receipts/q34_spatial_20260921/README.md) | trames brutes entières u16/2 cm 08/000000,100,200 ; K5/s8/W48 CPU | 165,214 / 34,319 / 505,479 s pour le seul flux q3/q4 ; 4,19 / 11,13 / 1,93 CPU logiques occupés en moyenne ; 244,805 / 122,737 / 273,138 M boules q3 construites | une répétition/scène ; ni GPU, ni FULL, ni profil float32 |
| [sans sol phase 1+2](../../morsehgp3D_v8/receipts/ground_phase1_20260921/README.md) | trois trames sans sol u16/2 cm ; K5/K10, hôte local partagé | sur scène0 au calme, K5/W8 108,0 s et 741,5 CPU·s ; K10/W8 323,0 s et 2212,1 CPU·s ; identités de sorties/compteurs aux configurations appariées | captures 01/02 concurrentes ; flux seulement, pas contrat G4 |
| [premier u18 sans sol, inventorié à l'ouverture](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md) | 08/000000 entier sans sol, 39 885 sites à 1 mm, K5/s8/W8 | 104,63 s mur, 812,82 CPU·s, 691 284 q3 et 158 496 q4 émis ; atlas 3,252 G bornes de blocs, 7,316 G tests points, 5,547 G IDs de frontière copiés | reçu v8 non commis au pin, une seule ligne, option `saturate_deep` inactive, aucun W1/W8 apparié |
| [tour v7](../../morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md) | uniforme u16 50k sur G4, K1..10 | 418,873 s pour la tour, dont 389,668 s de constructeur FULL ; catalogue 21,47 M boules, tour 27,27 M nœuds ; 3,90 G essais de supports aval | autre nuage/architecture, jamais temps v9 |

Le [diagnostic spatial](../../morsehgp3D_v8/receipts/q34_spatial_20260921/README.md)
trame0→moitié x+ a déjà un exposant observé 2,502 pour les bornes q3
et 2,332 pour les bornes de blocs q4, alors que les sorties de cette
campagne croissent plus lentement. Une coupe change aussi la géométrie :
ce sont des alertes structurelles, pas une borne asymptotique.

Les [audits LiDAR complémentaires du 22 septembre](https://github.com/Ludwig-H/E-HGP/blob/12294241/morsehgp3D_v8/audits/SYNTHESE_PRIORITES_LIDAR_20260922.md)
offrent un **raccord tactique** avant cette rupture : présélection négative
des blocs pour le témoin universel, plans de crédits disjoints par facteurs,
et réemploi des rectangles singletons. Sur 960 rectangles échantillonnés
de trois trames 08 sans sol à 1 mm, les masques/paires restent identiques
et le temps du **seul filtrage** baisse d'un facteur 2,55 à 4,46 ; ce
n'est ni un temps par trame ni une réduction d'atlas. Les
[certificats collectifs d'arêtes](https://github.com/Ludwig-H/E-HGP/blob/12294241/morsehgp3D_v8/audits/CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md)
peuvent, eux, rejeter q3/q4 sans témoin individuel : la fixture de cinq
triangles donne dix crédits à K10. Sur 763 arêtes LiDAR stratifiées, 221
rejets q4 ont un aval q4 de référence vide, mais l'atlas pouvait encore
servir q3 et aucun gain du pipeline combiné n'est établi. Porter ces
options avec ablations et grand-livre complet ; ne pas multiplier leurs
ratios ni les présenter comme solution sous-quadratique.

La cible de temps de [l'ouverture v9](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md)
est la **tour FULL** K1..10 en moins de 1 s sur G4, repli K1..5, puis
100 ms, d'abord sur les trames entières **sans sol, grille 1 mm/u18**.
Le contrat v8 antérieur gardait le float32 original comme entrée par défaut
et la trame brute entière comme obligation principale ; l'ouverture v9 cite
une instruction ultérieure de poursuivre d'abord le **jalon de temps** sur
u18 sans sol et de suspendre le développement float32. Cela n'efface pas
l'obligation brute ; sa portée temporelle v9 reste une question explicite
du développeur. Nos deux régimes et leurs coûts restent séparés. Aucune
hypothèse de calage entre passages ou d'alignement des points n'entre dans
les certificats.
Aucun des temps ci-dessus ne qualifie cette cible. Les trois scans
disponibles sont tous de la séquence 08.

## Lemme local commun à q2, q3 et q4

Fixons un site de support `a` et une boule de centre `c` passant par `a`.
Pour tout site `z`, posons

`Δ_a,z(c)=|z−c|²−|a−c|²=|z|²−|a|²−2c·(z−a)`.

La boule a exactement `p(c)=#{z : Δ_a,z(c)<0}` sites strictement
intérieurs. Cette identité est indépendante de q, du choix des autres
sommets et du rayon une fois `a` sur la coquille. Vue comme un rang de
site, elle sert ici à **certifier localement des miniballs**, sans
préconstruire des cellules de Voronoï d'ordre supérieur.

Pour une petite cellule fermée de centres C, classifier des nœuds
disjoints de l'index spatial :

`I_C={z : max_{c∈C} Δ_a,z(c)<0}` ;
`E_C={z : min_{c∈C} Δ_a,z(c)>0}` ;
`A_C=P\(I_C∪E_C)`.

Si `|I_C|≥h_q`, **toute** la famille de supports q dont le centre est
dans C est rejetée. Si le fragment est complet, pour chaque centre c∈C :

`p(c)=|I_C| + #{z∈A_C : Δ_a,z(c)<0}` et
`coquille(c)={z∈A_C : Δ_a,z(c)=0}`.

La raison essentielle pour l'énumération est que tout autre sommet d'un
support passant par a satisfait `Δ=0` à son centre. Il est donc
**nécessairement dans A_C**, même si le sommet n'aurait pas été accepté
comme graine à une autre étape. Pour q3, les complétions de l'arête
canonique sont cherchées dans cette frontière ; pour q4, les deux
complétions incidentes à l'arête y sont cherchées ou balayées ensemble.
Une cellule rejetée peut cesser de classifier Z aussitôt que h_q est
certifié ; elle ne transmet alors **aucun** compte ou fragment exact à
un balayage positif. Dans une cellule non rejetée, `I_C`, `E_C` et
`A_C` doivent constituer une partition complète du **nuage original**
ou d'un cover dont la complétude pour ces boules est prouvée.
Ce fragment livre le **compte** des nœuds uniformément intérieurs, pas
nécessairement leurs IDs. Le catalogue FULL demandant les intérieurs,
il faut conserver une représentation de ces IDs ou les recollecter
une fois **par boule canonique distincte** après déduplication, avec
le coût et les plateaux d'égalité explicitement comptés.

Pour une boîte de sites Z et une cellule polygonale C, `Δ` est convexe
en z et affine en c. Son maximum sur `Z×C` est atteint sur leurs sommets :
un maximum **strictement négatif** y crédite tous les sites du nœud Z.
Le minimum demande un minorant certifié, pas le minimum des seuls coins
de Z (fonction convexe) ; une incertitude conserve le nœud dans A_C.
Avec u18, les comparaisons doivent rester dans les bornes entières
qualifiées ; avec float32 exact, filtre conservateur puis repli dyadique.
Les égalités `Δ=0` demeurent actives. Les crédits issus de plusieurs
nœuds s'additionnent uniquement lorsque leurs populations sont disjointes.

Ce lemme prouve **la sûreté d'une famille élaguée et la complétude des
complétions dans une cellule couverte**. Il ne prouve pas que le nombre
de cellules, de sites actifs ou de couples résiduels est petit. L'atlas
par arête de la v8 en est un ancêtre utile ; la généralisation partageable
par ancre et par région doit être créée **à la demande**, avec durée de
vie, préparation, copie et éviction mesurées. Construire un atlas complet
pour chaque a paierait potentiellement le carré avant les graines.

Un second certificat local, détaillé dans la [note q4](Q4_STRUCTURE_ET_BORNES.md),
peut réduire cette frontière **avant** son expansion. Pour q4 à seuil
maximal K, choisir `T=K−2` gardes distinctes et poser
`U_C=max_{garde g} max_{c∈C}|g−c|²`. Toute boule q4 admissible de centre
dans C a `R²≤U_C` : au-delà, les T gardes seraient strictement intérieures.
Tout bloc spatial Z avec `distance²(C,Z)>U_C` est alors strictement extérieur
aux **supports, intérieurs et contacts** de ces boules, sans examen de ses
sites. Les analogues q3/q2 emploient respectivement `T=K−1` et `T=K`.
L'inégalité doit être stricte et le même sous-nuage fournit les gardes et
les supports. Une grande cellule ou des gardes lointaines rendent `U_C`
large : l'économie sur les vrais LiDAR reste une mesure, non une promesse.

## Route d'optimisation après une première tour FULL

Le [plan v9 ouvert](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/PLAN_V9.md)
place à juste titre un premier flux **bout-à-bout**, catalogue et parents
compris, avant d'attribuer un gain au contrat. Les briques ci-dessous
peuvent être prouvées en parallèle, mais leur intégration optimisée suit
ce premier reçu FULL ou un échec de base explicite et borné.
Le reçu 1 mm a déjà **23,687 millions de paires développées** et environ
**2,044 millions de covers** avant le travail d'atlas : bâtir une cellule
locale après chaque paire ne retirerait pas ce terme. Le front doit garder
des **produits de familles** jusqu'à ce qu'un certificat les rejette ou
justifie leur expansion ; compter séparément rectangles, masse résiduelle,
paires réellement ouvertes, covers et cellules.

1. **Port sûr, court, depuis l'existant.** Si le centre q3 est dans une
   feuille **exacte** de l'atlas q4 de la même arête, du même propriétaire
   et du même cover prouvé pour q3, son compte intérieur et sa frontière
   complète peuvent servir au census q3 et à la coquille. Le reste de
   Z ne doit pas être revisité ni le crédit compté deux fois. Une feuille
   `saturate_deep` à K−2 est un **certificat de rejet q4**, pas un fragment
   q3 : elle rejette aussi q3 seulement si sa profondeur certifiée atteint
   K−1 ; sinon repli global ou raffinement propre. Un centre q3 hors du
   domaine couvert par l'atlas garde le census global. Ceci répond à la
   [question constructeur](../../audits/COORDINATION_MORSEHGP3D_V8.md)
   sans faire passer une optimisation locale pour le saut de complexité.
2. **Mesurer avant de généraliser.** Pour chaque arête réellement active,
   compter `sommes |A_C|`, pics de frontière, nombre de cellules et tests
   Z partagés, refus par voie, cas q3 hors atlas, chemins de repli et
   duplications de cellules touchées par plusieurs arêtes. Comparer sur
   trames entières sans sol puis brutes, K5/K10 ; aucun quota de graines,
   de témoins ou de coquille n'est admis. Si le cache par ancre prépare
   plus de tests et copie davantage de sites qu'il n'évite, garder le
   fragment par arête et étudier un autre regroupement.
3. **q3 : compter une famille avant ses graines.** Partir des blocs de
   complétions possédés déjà préparés et de leurs enveloppes de centres.
   Diviser X seulement si une feuille Z ambiguë empêche le crédit commun.
   Le ticket `(compte, curseur)` se fige pour tous les frères ; les petites
   graines reprennent la forêt résiduelle sans racine globale répétée.
   Le [relais d'audit fermé](../../morsehgp3D_v8/audits/q3_prefix_relay_20260921/README.md)
   exerce cette couture, et le [census float32 partagé](../../morsehgp3D_v8/docs/CENSUS_Q3_FLOAT32_PARTAGE_20260921.md)
   la porte sur une arête, pas encore sur tout le front. Confronter cette
   voie au fragment exact déjà payé par q4 : leurs coûts d'entrée peuvent
   rendre l'une ou l'autre préférable selon l'arête.
4. **q4 : sélectionner des événements peu profonds une fois par frontière.**
   Dans le plan bissecteur d'une arête, chaque autre site donne une droite
   affine de puissance nulle. Une intersection de deux telles droites est
   une candidate de centre q4 ; à profondeur élevée, elle doit être
   rejetée sans énumérer les graines individuellement. La
   [fenêtre exacte de la v8](../../morsehgp3D_v8/docs/Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md)
   conserve déjà au plus `2H−2` événements intérieurs triés par graine,
   mais paie encore O(r) lectures pour chacune. Exploiter l'arrangement
   **local** de seulement `A_C` lignes, ou un index de chaînes/familles
   réutilisé, pour produire directement ses intersections de faible
   profondeur ; vérifier le coût de découverte des extrêmes, des lignes
   coïncidentes et des contacts groupés. Les algorithmes de niveaux peu
   profonds d'arrangements de lignes donnent une piste théorique, **pas**
   une borne transférée au pipeline v9 ; voir la
   [publication primaire sur les shallow cuttings](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.SOCG.2015.719).
5. **FULL dans la même enveloppe de coût.** Dédupliquer par clé de boule
   après exactitude, conserver `q_min`, intérieurs, coquille et les
   **incidences de facettes/parents nécessaires** au FULL, puis reconstruire
   les vrais parents et les cartes verticales. Le flux v8 de toutes les
   présentations de supports reste un oracle différentiel ; l'imposer comme
   sortie industrielle pourrait rendre cubique une coquille cosphérique
   représentable par une seule clé. Une réduction précoce exige toutefois
   la preuve qu'au moins une présentation de chaque boule utile survit. Les
   [prototypes de forêts datées v7](../../morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md)
   et leurs oracles sont des points de départ à requalifier. Le temps de
   catalogue, résolutions de facettes, historique et sérialisation compte
   dans le 1 s/100 ms ; un digest rapide de q3/q4 ne suffit pas.

Une mosaïque globale d'ordre K est une comparaison de recherche, pas la
route industrielle retenue ici. Même des algorithmes modernes la
construisent de proche en proche via des mosaïques pondérées et sa taille
en dimension au moins 3 dépend fortement des données : voir
[Edelsbrunner–Osang, Algorithmica 2023](https://research-explorer.ista.ac.at/download/12086/12322/2023_Algorithmica_Edelsbrunner.pdf).
L'objet de travail k-Gabriel par miniball permet des tâches indépendantes
`(arête canonique, cellule de centres, sous-arbre témoin)` et un arrêt
certifié avant l'expansion ; il faut encore établir que leur **somme**
reste économiquement sous-quadratique sur le LiDAR visé.

## Contrat de parallélisation et de preuve

Un job portable ne contient que IDs/rangs de support, handle vers index
et contexte de cellule possédés, masque q3/q4, compte certifié et racines
Z disjointes encore indécises. Les frontières partagées sont immuables ;
les buffers et sorties sont privés par worker ou par lot. Une tâche
transférée ne peut pointer dans la pile du producteur. Scinder seulement
les sous-produits non visités ; file pleine ⇒ poursuite locale. Un lot
GPU borné comprend comptage, compactage, offsets exclusifs, calcul et
repli exact des seuls signes incertains. Le lot n'est pas une limite de
recherche, et tous les transferts, replis et sorties sont payés.

La première porte v9 doit confronter, **sur les mêmes entrées**, une voie
de référence complète et la voie groupée : sur petits nuages, toutes
les présentations de supports comme oracle ; sur le chemin FULL, clés
uniques, `q_min`, intérieurs, coquilles, incidences utiles, puis
parents/verticales. Les
fixtures obligatoires sont les contacts `Δ=0` aux frontières de cellule,
coquille30, cosphéricités, support q4 sans q3 accepté, ticket q3 avec
compte mais mauvais curseur, atlas saturé pris pour un fragment, et la
famille d'arcs à sortie quadratique. Les mutants doivent être tués par
une **différence géométrique**, pas par une assertion de compteur.
Les petites scènes utilisent un oracle rationnel indépendant ; sur les
grandes, les digests et différentiels ne prouvent pas seuls la complétude.

Le grand-livre sépare préparation de l'index, rectangles, paires, visites
Z, cellules, bornes/blocs, copies de frontières, graines, boules construites,
replis, événements, sorties, clés, catalogue, parents, mémoire simultanée,
travail CPU/GPU et mur complet. Trames **entières** de plusieurs séquences,
sans sol u18 d'abord, puis avec sol ; le profil float32 d'entrée reste
séparé et son développement temporel v9 en veille ; K5/K10
et s8/10/12 appariés. Les sept coupes spatiales servent au diagnostic,
pas au contrat. Pour un suivi de croissance, publier les ratios réels
de cardinalités et du travail de chaque poste, y compris les défavorables,
sans appeler trois coupes d'une scène une borne asymptotique. Si les
certificats réduisent le coût d'une voie mais déplacent le carré vers
`Σ|A_C|`, les copies ou FULL, le verrou reste ouvert. Ajouter un diagnostic
de densité et de captations LiDAR superposées, sans supposer leur alignement :
mesurer notamment la croissance des frontières actives, coquilles et sorties
lorsque plusieurs passages occupent des voisinages proches.
