# État de livraison v7 — 11 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Chantier sur `main` uniquement. Cette entrée décrit le travail courant ;
les récits des anciens jalons sont retirés, leurs preuves restent liées.

Priorité après `f2bea998` et l'audit `383f8f98` : [objets parallèles de toute la tour](docs/OBJETS_PARALLELES_TOUR_20260911.md).
L'auditeur confirme l'indépendance des horizontales K et le calcul offline
des verticales sans nouvelle MEB. Catalogue unique, atlas des blocs,
graphe daté sur naissances, multifusions et marques séparées remplaceraient
les dépendances temporelles du calendrier actuel. Le [raccord privé complet](receipts/atlas_graph_full_20260911/README.md)
relie désormais les premières briques atlas/calendrier : extraction depuis
vrais census, histoires FULL, contributions datées et verticales offline.
La composition des MSF par fenêtres est exercée avant et après projection
sur les naissances. Les certificats internes peuvent différer, pas la tour.
Aucun remplacement actif ni temps extrapolé. Prochain delta : produire et
consommer les terminales par fenêtres sans garder targets[R], puis distribuer
les réductions et la reconstruction ; mesurer le travail répété et la RAM,
pas seulement la compression des arêtes. Les consultations seules sont CPU1/4.
GCP non utilisé pour ce nouveau jalon architectural.

Complément précédent après `324f6192` : [lots complets, MEB et tentatives G4](docs/QUALIFICATION_BATCH_ET_MEB_20260911.md).
Sonde et gate privées compilées/liées NVCC strict ; la gate O2/SAN compare
10 326 terminales directes et vingt paires physiques. La réduction q2 est
qualifiée séparément sur 6 416 MEB, non intégrée et sans benchmark de vitesse.
Deux préemptions US, puis outils invités manquants au précontrôle européen :
aucun calcul GPU de cette campagne. Trois arrêts ciblés certifiés, clé de
session révoquée ; aucun nouveau temps 50k ni contrat de performance acquis.

Le [nouveau raccord par boules](docs/TOUR_FULL_PAR_BOULES.md) est implémenté :
MEB à coquille libre, ancres fermées, parents pré-lot, plateaux, journal v2
et cartes verticales. Les dix forêts et leurs images adjacentes sont retenues.
Autorité relative aux census complets et exacts fournis ; aucune promotion
automatique de la génération WSPD, des reçus F ou de l'archive industrielle.

Qualification géométrique O2/SAN courante : 28 nuages, 112 ordres, 2 508 coupes,
45 948 images verticales, 170 320 contrôles. Six mutants du delta cache sont
tués, dont croissance et ancre inerte dans le chemin groupé exposé par l'auditeur.
La fixture n8 exerce réellement une descente à rayon égal. MEB séparée :
605 cas, dont 197 extra-shells. Les optimisations validées portent sur les
supports réguliers déclarés, les lots unitaires et le normaliseur temporel
inférieur. Le peigne expose le coût quadratique écarté, sans réécrire les
successeurs historiques. Le [cache exact et ses semis fermés](docs/OPTIMISATIONS_CACHE_ET_GPU_20260910.md)
réduisent les MEB appariées de 1 174 515 à 583 337 à n1000. Le journal réserve
ses arènes exactes et les états morts sont libérés avant la banque finale.
Les brouillons globaux et les résolutions restantes restent coûteux.

Premier delta statique : [voie CPU optionnelle](docs/RESOLUTION_STATIQUE_CPU_20260911.md),
header historique `33e7d05e…`. Tri-unique des représentants stricts, semis géométriques,
une BallId par clé puis restitution des occurrences au calendrier nominal.
Le défaut reste le cache temporel ; `--static-threads=1` ou `4` sélectionne
la nouvelle voie, indépendamment des threads amont. O2/SAN : 30 nuages,
124 ordres, 75 136 comparaisons verticales et 4 498 contrôles physiques
appariés, six mutants réfutés. À 8k/s8, un thread amont et quatre statiques,
même payload, 6 227 265 → 4 185 184 MEB
et 573 011 617 → 364 590 166 supports ; capacités temporaires retenues
307 936 444 octets. Ni ces capacités ni les temps sur hôte chargé ne prouvent
un gain RSS ou de latence contractuel. Le GPU de cette phase reste à porter.
Le [triplet statique](receipts/static_resolution_scale_20260911/README.md) est
clos : mêmes 35 champs de calendrier/sortie et payloads à 8k/16k/32k ;
MEB 4 185 184 / 8 779 465 / 18 244 853, soit −32,8/33,8/34,2 % contre
les comptes nominaux publiés. Exposants locaux du travail MEB 1,069 puis
1,055, uniforme seulement. Totaux partagés 466,761 / 686,049 / 802,278 s,
pas de gain chronométrique revendiqué ; capacités statiques retenues
1 235 849 528 octets à 32k, pas un gain RSS.
La [comparaison statique s8/10/12](receipts/static_s_factors_20260911/README.md)
est close à 8k : mêmes dix forêts et neuf lignes de résolutions par K,
mêmes MEB/supports, seuls les candidats amont et les temps varient.

Le header `6763a877…` a intégré les [semis après échange](docs/SEMIS_APRES_ECHANGE_20260911.md),
dans la même option statique, défaut temporel inchangé. Une recherche exacte
dans la table complète existante évite la dernière MEB lorsqu'elle trouve
la nouvelle facette entière. O2/SAN propres passent, 34 nuages/150 ordres,
87 230 verticales et 5 704 comparaisons physiques dans chaque mode statique.
Le [paquet du delta](receipts/post_exchange_seed_20260911/README.md) conserve
106 commandes, trois mutations réfutées et deux premières tentatives de juge
insuffisant. Micro mono : environ −5,7 % de MEB et −6,5 à −6,8 % de supports
supplémentaires ; pas de gain chronométrique qualifié. Le [build actif](receipts/post_exchange_active_cmake_20260911/README.md)
passe ses 24 CTests, et le [T2 propre](receipts/full_t2_post_exchange_20260911/README.md)
ses 54 tours O2/SAN avec 120 hits/228 recherches, sans reprise des chiffres
historiques ci-dessus. Le [nouveau triplet mono](receipts/post_exchange_scale_20260911/README.md)
est clos, un processus par taille sans compilation concurrente du chantier :
3 947 627 / 8 278 207 / 17 199 233 MEB payées et
237 557 / 501 258 / 1 045 620 évitées à 8k/16k/32k. Les 35 champs/digests
et R/U/S par K coïncident. Totaux 141,366 / 318,968 / 694,459 s ; FULL
58,977 / 134,329 / 292,341 s ; pic RSS 8,687 Gio à 32k. Pas de speedup
historique ou de gain RSS revendiqué, ni de qualification s10/s12 ou 50k
sur ce header.

Le header actif est désormais `83f1c78e…` : [couture optionnelle par lots](docs/PARALLELISATION_PAR_LOTS_20260911.md),
transactionnelle, sans changement du cache par défaut. Les onze CTests
census→tour et cinq CTests du callback sont intégrés ; leurs candidats O2/SAN
passent, puis la [reconstruction active commune](receipts/full_ball_batch_active_cmake_20260911/README.md) passe les 40 CTests ciblés
et la sonde n200 (mêmes sorties/travail, deux capacités de layout distinguées).
Le contexte GPU résident
reste privé, qualifié géométriquement en émulation hôte et compilé sous NVCC
strict après quatre annotations HD restaurées. Aucune nouvelle exécution
device ni mesure 50k. Les économies de transfert/copie ne sont pas des temps.
Les anciennes mesures du header 6763 ci-dessus restent leur propre témoin.

L'[auditeur du journal](audits/receipts_coverage_cpp_20260910/README.md)
a démontré que parent→0 échappait au juge antérieur. La gate renforcée
compare directement parents, successeurs, niveaux et contributions :
823 contrôles O2/SAN avec les réservations, 40 coupes Gamma et 20 pannes
d'allocation contre 34 auparavant ; le mutant est rejeté par `arena.parent_value`.
Une vraie multifusion à quatre parents du carré K2 est permanente. Le schéma
v2 ne change pas. L'injecteur du test cache inclut désormais `new(nothrow)` ;
son premier échec ASan est conservé sans modification des captures.

La nouvelle sonde hybride conserve le constructeur FULL sur CPU et place
seulement prefilter/census sur CUDA. Le contexte inclut les transferts froids
et traite les erreurs sans publier de préfixe. La gate passe dans le stub
hôte ET sur le vrai G4 SM12.0 : 16 627 contrôles, 4 116 boules et 17 rejets.
La compilation NVCC stricte est qualifiée avec son adaptateur de phases.

Les [mesures courantes](docs/RESULTATS_TOUR_CACHE_G4_20260910.md) terminent
les tours 50k à s8 : CPU/hybride 418,873 / 418,921 s K1..10 et
33,853 / 33,569 s K1..5, avec signatures identiques. Elles conservent
27 273 218 / 4 209 792 nœuds et leurs verticales. Dans ces captures du
10 septembre, FULL restait mono-thread et prenait environ 390 / 27 s ;
les kernels census seuls ne font pas le contrat.
Les essais s10/12 G4 ne sont pas lancés faute de temps de clôture restant.

Deux générations SPOT sont closes dans le reçu courant : échec initial NVCC,
puis session optimisée réussie. La cible exacte
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`
est certifiée `TERMINATED` pour chacune. GCE STOP/3600 s et arrêt invité
30 minutes vérifiés ; aucune autre VM modifiée. L'ancien refus de quota
est historique et ne décrit plus l'issue de cette nouvelle campagne.

Les [deux processus G4 historiques K10/K5](docs/RESULTATS_G4_FULL_20260906.md)
restent des refus (21,372 s / 5,646 s), pas des mesures de la nouvelle tour.
Le [rejeu nommé indépendant publié à 223a3897](audits/receipts_cache_commit_20260911/README.md)
retrouve maintenant les parents pré-lot 1/2/2 et l'ancre inerte K10 sur
l'entrée 50k, depuis les octets du moteur du 10 septembre. Ce verrou est
clos sur CPU, sans certifier l'arité finale, la complétude du catalogue,
ni la nouvelle voie statique. Contrats 1 s/100 ms et plusieurs dizaines
de millions de points non atteints. Le jugement du raccord réel
génération/census→tour à K9/K10 dispose maintenant d'une
[nouvelle qualification bornée sur c03f6be8](docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) :
trois géométries n12/n14, deux réindexages, s8/10/12 et cache/statique1/4,
54 tours K1..10 par build O2/SAN. Les inventaires exacts sont jugés avant
13 000 coupes Gamma et 8 103 948 vérifications verticales, avec neuf rejets
et quatre mutants. Les 48 paires physiques supplémentaires complètent ces
six tours de référence, sans promouvoir la complétude universelle WSPD.

Nouvelle session du 11 septembre : [deux primitives CUDA](docs/RESULTATS_PRIMITIVES_GPU_20260911.md)
passent sur la vraie G4 SM12.0, après O2/SAN hôte et compilation stricte.
Sélection MEB : 605 cas, 21 432 contrôles, 44 rejets ; clé/PGCD/division
128 bits : 13 573 cas, 325 752 mots comparés. Deux mutants de transport
sont réfutés sur carte. Aucun résolveur complet ou assemblage FULL GPU
livré par ces gates, aucun nouveau benchmark 50k. La génération SPOT
`2026-09-11T01:20:08.309-07:00` de la même cible est maintenant certifiée
`TERMINATED` ; aucune autre VM active détectée à la clôture.

Depuis cette session, deux raccords supplémentaires sont clos localement :
[MEB + clé primitive](receipts/gpu_meb_key_route_20260911/README.md),
22 245 contrôles, 605 cas, 47 rejets, zéro matérialisation ou puissance hôte
sur le chemin nominal ; [premier intrus exact](receipts/gpu_intruder_primitive_20260911/README.md),
6 861 contrôles, 596 requêtes, O2 et SAN ROOT, six mutants causaux.
La gate autonome de ce dernier compare 14 688 mots pour 612 cas ; pile
Morton 49 effectivement exercée et générations de propriétaires distinctes.
Les deux vrais kernels compilent et se lient sous NVCC strict, mais aucune
nouvelle exécution device n'a eu lieu. GCP non utilisé pour cette reprise.
Le [terminal entier composé](receipts/gpu_static_terminal_host_20260911/README.md)
est maintenant clos en stub O2/SAN : cœur FULL, T2 54 tours et rejets, sur
la baseline c03 sans le nouveau raccourci après échange. La collision réelle
entre deux compteurs de propriétaires est corrigée par réemploi du token de
l'index possédé ; le même juge croisé réfute l'ancien owner et refuse la vue
mixte avant MEB dans le nouveau. Les sources et échecs antérieurs sont
conservés. Pas d'exécution device dans ce paquet ni de raccord au constructeur
actif ; ses gates paient aussi les références et ne sont pas des benchmarks.
Le [paquet CUDA séparé](receipts/gpu_static_terminal_cuda_20260911/README.md)
ferme export exact, O2/SAN ROOT et compilation/lien NVCC stricts, sans aucune
exécution device. Il compare 949 facettes et 1 428 traces sur K2..8,
avec douze rejets et huit corruptions de transport réfutées. Son
[extension K9/K10](receipts/gpu_static_terminal_k10_20260911/README.md) est
maintenant close localement : 1 577 requêtes, 2 763 traces Gram et
15 539 contrôles O2/SAN, puis compilation/lien NVCC SM120. Six rejets du
dernier slot K9/K10 s'ajoutent ; le premier échec NVCC sur droit d'exécution
du script importé est conservé. Le helper reste celui de la référence c03,
sans les semis actifs. Aucun résultat device ni transfert aux nouveaux lots
de tour semés ; leur qualification est distincte.

Le [raccord FULL privé du journal incrémental](receipts/incremental_full_trial_20260911/README.md)
passe les comparaisons physiques O2/SAN dans quatre modes et les refus
transactionnels, mais n'est pas intégré : les allocations et capacités
finales augmentent. Les limites et résultats négatifs sont conservés ;
la suppression des copies doit précéder toute promotion de performance.

## Objet à conserver

La [lecture mathématique](docs/AUDIT_NIVEAUX_GABRIEL_20260905.md),
contre-vérifiée par l'[auditeur](audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md),
fixe FULL sous régularité : feuilles minima Gabriel de cardinal K, niveaux exacts,
multifusions véritables induites par les cofaces Gabriel de cardinal K+1
et parents pré-lot, sans imposer K+1 parents à une fusion. La couverture
se dérive des feuilles, sans fusionner deux identités par égalité de
points. Les portails silencieux résolvent les parents mais ne sont pas
des nœuds de sortie. Aucun Gamma exhaustif n'est nécessaire.

K=n est traité séparément. Hors régularité, le
[supplément de plateau](docs/PLATEAUX_FULL_ET_ANCRES.md) conserve couvertures
initiales, gains datés, parents pré-lot et ancres fermées ; une continuation
ne devient pas une multifusion. Les ancres verticales
et profils pondérés gardent leurs obligations propres. Les minima FULL
ne sont pas toutes les facettes contributrices des poids du manuscrit.
Un validateur structurel et une égalité de digests ne certifient pas la
complétude géométrique d'un fournisseur de catalogues.

L'[étude des sommets Gabriel](docs/SQUELETTE_MINIMA_GABRIEL.md) répond à
la nouvelle question utilisateur : les minima suffisent avec les seuils
des chemins supprimés, pas avec les anciennes adjacences seules. Une
forêt de L−R connexions entre L minima encode les mêmes coupes. La descente
de facettes à cardinal K constant fournit une méthode constructive pour
les parents ; elle est correcte mais pas uniformément moins coûteuse que
le J=1 de l'ancienne lignée. Le nouveau raccord partage les ancres entre
ordres adjacents ; un choix hybride avec J=1 reste à évaluer. Le
[prototype indépendant de tour](audits/receipts_gabriel_vertices_20260906/README.md)
vérifie aussi les images verticales et leur naturalité sur un petit cas.

La [borne de sortie](docs/CROISSANCE_ET_BORNE_DE_SORTIE.md) interdit une
garantie universelle sous-quadratique en n pour une sortie FULL explicite
en 3D. L'objectif pratique reste de limiter le travail intermédiaire et
de mesurer séparément croissance des sorties et temps sur 8k/16k/32k.

## Code actif et prochaine exécution

Entrée : [TOUR_FULL_PAR_BOULES.md](docs/TOUR_FULL_PAR_BOULES.md).
`anchor_meb.hpp` et `full_ball_tower.hpp` forment le nouveau constructeur ;
`full_coverage_certificate.hpp` reste le journal partagé, à réservations exactes.
Les sondes CPU et CUDA-census sont dans `bench/full_ball_tower_probe.*`.
Leurs sorties sont relatives, sans archive industrielle ni claim produit.

Construire dans un répertoire neuf ; 24 CTests ciblés passent sur sources
stables pour le jalon antérieur : MEB, tour, cache, travail temporel, journal,
quotient, front WSPD et simulation de route. La reconstruction active `83f1`
ajoute les seize portes census/batch, soit 40 CTests ciblés ; aucune nouvelle
campagne CMake du moteur n'est revendiquée par les paquets privés suivants.
O2/SAN et vrai device restent des autorités distinctes.
Le worker `gcp-migration/full_ball_worker_v7.py` réutilise le contrôleur
SPOT gardé et le support CPU épinglé. Il vérifie d'abord le vrai device
SM120, puis compare CPU/hybride sur n8 et sur 50k K10/K5, avec s8 puis
s10/s12 selon le temps observé et la fenêtre de fermeture. Aucune installation
CUDA, aucun reboot, aucune mutation d'une autre VM n'est autorisé par ce worker.
Ce worker conserve encore le défaut `static_threads=0`. Le nouveau
[worker terminal par lots](../gcp-migration/README_TERMINAL_BATCH_V7.md) est
distinct : son instantané privé transmet explicitement `--static-threads=1`
et `--batch=1`, puis statique CPU à 48 threads pour la paire complète,
avec validations de configuration.
Ses tests locaux sont clos ; les trois tentatives G4 n'ont pas atteint la
compilation invitée. Reprendre ce worker et son instantané, pas le worker
historique en supposant que le header active automatiquement le GPU.

Le [triplet nominal retenu du 10 septembre](docs/RESULTATS_TOUR_CACHE_G4_20260910.md)
est clos : 235,724 s / 354,144 s / 736,819 s, dix ordres et verticales,
s=8 et un thread ; 17 166 975 nœuds à 32k, pic 9 108 756 KiB.
Les temps sont perturbés par l'hôte partagé : pas de gain apparié revendiqué.
À 8k, s8/10/12 donne exactement le même payload ; aucun optimum s n'est acquis.

Prochaines coutures et prototypes séparés :

- [Résolutions géométriques statiques](docs/RESOLUTION_STATIQUE_CPU_20260911.md) : option CPU intégrée, triplet 8k/16k/32k et s8/10/12 à 8k clos ; suivre maintenant le [port GPU des résolutions](docs/PORT_GPU_RESOLUTIONS.md). Les [clés initiales mesurées](receipts/initial_representatives_20260911/README.md) à 8k ont 50,4 % de doublons ; ce ratio ne vaut pas gain de vitesse, le cache nominal en exploite déjà une partie.
- [Parcours droit des intrus](receipts/rightmost_intruder_20260910/README.md) : prototype de deux lignes non intégré ; 28 nuages O2/SAN et 72 840 requêtes contre un juge de choix. À n1000 : −18,7 % de visites et −3,07 % de MEB, pas de gain contractuel.
- [Journal incrémental](docs/PLAN_JOURNAL_INCREMENTAL.md) : raccord privé FULL O2/SAN correct dans quatre modes, non intégré après micro défavorable sur allocations/rétention finale ; vues plates et suppression des copies restent à évaluer.
- [Primitives de résolutions GPU](docs/RESULTATS_PRIMITIVES_GPU_20260911.md) : MEB par lots et clé/PGCD/division passent sur G4 après O2/SAN ; sources privées, ni terminal ni raccord de tour GPU. Le premier prototype et ses échecs restent conservés séparément.
- [Graphe filtré des boules](docs/GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md) : accord conditionnel de l'auditeur et réduction aux naissances avant forêt couvrante minimale. La [décomposition en objets parallèles](docs/OBJETS_PARALLELES_TOUR_20260911.md) retire aussi la boucle d'exécution des ordres ; contributions, dates d'admission et verticales restent nécessaires. Premières références C++ privées, pas de raccord actif/GPU ni gain mesuré.
- [Front WSPD optionnel](receipts/witness_front_20260910/README.md) : 431 010 contrôles O2/SAN, liaison explicite à l'index et générations vérifiées. Le batch device reste privé et non exécuté sur GPU ; le générateur nominal reste scalaire.

Le premier reçu des [quatre blocs nommés](receipts/full_ball_named_blocks_20260910/README.md)
reste historiquement `NOT_EXECUTED_50K`. Le [nouveau rejeu indépendant](audits/receipts_cache_commit_20260911/README.md)
ferme maintenant ce verrou sur les mêmes octets épinglés, sans modifier le
premier reçu. Aucun temps de composant ni extrapolation 8k/16k/32k ne
qualifie 1 s ou le massif.

Les [notes de la sonde régulière au 6 septembre](docs/HISTORIQUE_SONDE_REGULIERE_20260906.md)
conservent les variantes eager/lazy, quotas retirés, MEB filtrées, mesures
mono/multi-CPU et refus G4. Elles ne décrivent pas le nouveau moteur de tour.
Les [fausses pistes](docs/FAUSSES_PISTES.md) expliquent les abandons ; les
anciens reçus restent conservés, sans copies d'ELF dans la livraison.

## Témoins conservés, non réattribués

- [Port v6 et lecture des fondements](docs/LECTURE_ET_CONTRATS.md), avec [octets consommés](docs/V6_SOURCE_SNAPSHOT.json) : aucune modification du worktree v6 par ce chantier.
- [Qualification F 48/48/339](receipts/witness_stack_integrated_20260905/README.md) et [mesures F](docs/RESULTATS_MONO_F_20260905.md) : objet réduit, distinct de FULL.
- [Premier producteur FULL](receipts/full_gabriel_20260905/README.md) et [mesures eager historiques](docs/RESULTATS_MONO_FULL_20260905.md) : sources et instrument antérieurs, pas bras appariés de la sonde v2.
- [Primitives et autorités mathématiques](docs/QUALIFICATION_S1_PRIMITIVES.md), [MEB à double budget](docs/RESULTATS_MEB_DOUBLE_BUDGET_20260905.md) et [coût local défavorable](docs/RESULTATS_COUT_MEB_20260905.md) : qualification locale, pas accélération de tour intégrée.
- [G4 historique](docs/RESULTATS_G4_20260904.md) et [arrêt certifié historique](receipts/gcp_handoff_20260905.json) : aucune qualification FULL ou nouvelle mesure massive héritée.

Le delta statique CPU du 11 septembre n'utilise pas GCP. Les deux sessions
du 10 septembre et la nouvelle session de primitives GPU du 11 sont closes ;
leur cible E-HGP exacte est certifiée arrêtée pour chaque génération.
Les [trois tentatives suivantes de lots terminaux](receipts/terminal_batch_g4_20260911/README.md)
sont également closes, sans benchmark ni installation. Leur inventaire final
ne détecte aucune autre VM E-HGP active ; aucune autre cible n'a été arrêtée.
Les CTests locaux, la CI et les sessions G4 sont
trois autorités distinctes. Aucun
succès CI d'un ancien commit n'est attribué automatiquement au nouveau.
Le registre officiel reste inchangé. Avant publication : contrôle des
documents, du registre et des octets des reçus effectivement dans l'index.
Ne pas indexer les fichiers de l'auditeur en préparation.
