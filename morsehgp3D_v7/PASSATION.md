# État de livraison v7 — 10 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Chantier sur `main` uniquement. Cette entrée décrit le travail courant ;
les récits des anciens jalons sont retirés, leurs preuves restent liées.

Le [nouveau raccord par boules](docs/TOUR_FULL_PAR_BOULES.md) est implémenté :
MEB à coquille libre, ancres fermées, parents pré-lot, plateaux, journal v2
et cartes verticales. Les dix forêts et leurs images adjacentes sont retenues.
Autorité relative aux census complets et exacts fournis ; aucune promotion
automatique de la génération WSPD, des reçus F ou de l'archive industrielle.

Qualification géométrique O2/SAN : 24 variantes, 100 ordres, 2 136 coupes,
35 462 images verticales, 130 734 contrôles et quatre mutants causaux.
La fixture n8 exerce réellement une descente à rayon égal. MEB séparée :
605 cas, dont 197 extra-shells. Les optimisations validées portent sur les
supports réguliers déclarés, les lots unitaires et le normaliseur temporel
inférieur. Le peigne expose le coût quadratique écarté, sans réécrire les
successeurs historiques. Les brouillons et les résolutions MEB restent coûteux.

L'[auditeur du journal](audits/receipts_coverage_cpp_20260910/README.md)
a démontré que parent→0 échappait au juge antérieur. La gate renforcée
compare directement parents, successeurs, niveaux et contributions :
837 contrôles O2/SAN, 40 coupes Gamma, 34 pannes d'allocation ; le mutant
est rejeté par `arena.parent_value`. Une vraie multifusion à quatre parents
du carré K2 est permanente. Le header nominal du journal n'a pas changé.

La nouvelle sonde hybride conserve le constructeur FULL sur CPU et place
seulement prefilter/census sur CUDA. Le contexte inclut les transferts froids
et traite les erreurs sans publier de préfixe. Stub hôte : 16 627 contrôles,
4 116 boules et 17 rejets ; aucun résultat NVCC/device encore obtenu.

Tentative G4 du 10 septembre : refus `GPUS_ALL_REGIONS`, limite 1,
occupée par `cracksam-frangigraph-g4-spot-ew8c` en `europe-west8-c`
(autre charge de travail, non modifiée). La cible exacte E-HGP
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`
est vérifiée `TERMINATED`. Le contrôleur a conservé son refus de clôture
sans génération nouvelle ; la vérification ciblée ultérieure confirme
l'arrêt et l'absence de nouveau `lastStartTimestamp`. Aucun worker distant
n'a commencé. Ne pas relancer dans une autre zone : le quota est global.

Les [deux processus G4 historiques K10/K5](docs/RESULTATS_G4_FULL_20260906.md)
restent des refus (21,372 s / 5,646 s), pas des mesures de la nouvelle tour.
Les parents globaux réels 1/2/2 prouvés par l'auditeur restent à retrouver
par le nouvel instrument sur l'entrée 50k entière. Contrats 1 s/100 ms et
plusieurs dizaines de millions de points non atteints.

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
`full_coverage_certificate.hpp` reste le journal partagé, inchangé.
Les sondes CPU et CUDA-census sont dans `bench/full_ball_tower_probe.*`.
Leurs sorties sont relatives, sans archive industrielle ni claim produit.

Construire dans un répertoire neuf ; les quatorze CTests ciblés concernent
MEB, tour, travail temporel, journal, quotient local et simulation de route.
Le worker `gcp-migration/full_ball_worker_v7.py` réutilise le contrôleur
SPOT gardé et le support CPU épinglé. Il vérifie d'abord le vrai device
SM120, puis compare CPU/hybride sur n8 et sur 50k K10/K5, avec s8 puis
s10/s12 selon le temps observé et la fenêtre de fermeture. Aucune installation
CUDA, aucun reboot, aucune mutation d'une autre VM n'est autorisé par ce worker.
Le quota global doit être libéré avant cette reprise.

Le [triplet retenu 8k/16k/32k](docs/RESULTATS_TOUR_BOULES_20260910.md)
est clos : 215,169 s / 417,627 s / 965,053 s, dix ordres et verticales,
s=8 et un thread ; 17 166 975 nœuds à 32k, pic 10 559 316 KiB.
Ordre de travail restant : réduire résolutions et résidence,
qualifier la route sur G4, puis porter
les témoins universels WSPD par lots. À 8k, la sonde compte déjà 3 976 472
nœuds sur dix ordres : coût des sorties et coûts intermédiaires doivent
être rapportés ensemble. Aucun temps de composant ne satisfait le contrat.

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

Tentative G4 refusée avant démarrage ; cible E-HGP vérifiée arrêtée,
autre VM non modifiée. Les CTests locaux, la CI et les sessions G4 sont
trois autorités distinctes. Aucun
succès CI d'un ancien commit n'est attribué automatiquement au nouveau.
Le registre officiel reste inchangé. Avant publication : contrôle des
documents, du registre et des octets des reçus effectivement dans l'index.
Ne pas indexer les fichiers de l'auditeur en préparation.
