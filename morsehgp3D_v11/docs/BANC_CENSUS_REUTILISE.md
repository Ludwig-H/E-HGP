# Banc FULL avec census réutilisé

Tranche du 3 octobre 2026, préparée après `bbb618c14`. Aucun résultat natif ni
gain de temps acquis par ce raccord. La primitive est décrite dans
[FULL_CENSUS_REUTILISE.md](FULL_CENSUS_REUTILISE.md). Les lecteurs, sources et
campagnes historiques figés restent inchangés.

## Option et preuve de mémoire

Le bit 256 active `FullParams.reuse_census_workspace`. Les autres bits gardent
leur signification ; le domaine du pilote est `0..511`, avec la contrainte
bit128 ⇒ bit8. Cela donne 384 modes valides, dont le bit256 seul, les modes
sans mémo et les modes sans lanes. Le bit256 n'active aucun mémo implicitement.

L'événement FULL réussi expose trois nouveaux champs :

- `reuse_census_workspace` : booléen exact conforme au bit demandé ;
- `census_workspaces` : C, nul lorsque l'option est inactive ;
- `census_workspace_reserved_bytes` : exactement `4*n*C` octets Buffer,
  avec n égal au nombre de sites du nuage entier.

En actif, C=1 si Q=0, sinon C=min(W,L,Q), pour tout K demandé, y compris K=1.
Il s'agit des espaces physiques qui peuvent être utilisés simultanément,
non du nombre de lanes mémorisées. La somme des réservations de la tour
conservée, du mémo sériel, des mémos de lanes et de ces espaces doit être
inférieure ou égale au pic FULL mesuré. Le propriétaire et les petits objets
fixes restent hors du compte Buffer ; ce compteur n'est pas le RSS.
Les workspaces sont libérés avant la publication de la tour. Leur construction
fait partie du temps forêt/FULL, et leur mémoire coexiste avec les autres
réservations jusqu'à la fin de la construction.

Le collecteur exige des entiers u64 (jamais des booléens) et le booléen de
mode avant le décodage, y compris avant une réutilisation sémantique. Chaque
sortie binaire est toujours intégralement relue et hachée. Les diagnostics
courants et les comptes de structures sont rejugés ; aucune durée ni issue
d'une ancienne tentative n'est réutilisée.

Les schémas courants deviennent `ehgp.v11.full_campaign.v9` et
`ehgp.v11.full_parallel_campaign.v5`. Le schéma de ces trois champs est
`ehgp.v11.full_census_workspace.v1`. Le format du payload géométrique et le
schéma des compteurs `full_work.v4` ne changent pas.

## Comparaison et calendrier

`full_parallel.py --reuse-census` est exclusif avec `--optimized-catalogue`
et `--parallel-verticals`. Ce dernier flag sélectionne un calendrier ; il
ne décrit pas le booléen natif `parallel_verticals`, actif dans les deux modes
255 et 511 du nouveau calendrier (inactif dans127).

Vingt-neuf invocations sont déclarées, chaque triple dans l'ordre 127, 255 puis 511 :

- trois entrées LiDAR entières, u21 puis u24, W48 : dix-huit invocations ;
- ng00/u21/mode511, W1 puis W8 : deux invocations ;
- les trois tailles uniformes entières en u21, W48 : neuf invocations.

Géométrie et structures restent comparées entre toutes les voies ; les octets
le sont à profil égal. `census_point_tests` compte le travail réellement payé,
donc peut différer entre census possédé et emprunté. Le travail complet est
comparé à masque de descente fixé `399 = 15 | 128 | 256`, entre profils et
nombres de workers. Les comptes des lots réguliers et verticaux sont aussi
comparés. Aucun doublement compensatoire n'est ajouté aux compteurs natifs.
La différence de travail ne prouve pas un gain de temps : allocation, pression
mémoire, MEB, index, catalogue et parties sérielles restent inclus dans FULL.

Le plan [full_census_g4.json](../bench/plans/full_census_g4.json) réserve
850+180+570=1 600 secondes aux trois commandes. En ajoutant les 120 secondes
de préparation déclarées par le préflight, cela donne 1 720 secondes, sous
la fenêtre de 1 737 secondes observée lors de la préparation précédente.
Aucune garde du contrôleur n'est modifiée ; le préflight courant reste requis.
Le banc a son propre budget de 500 secondes et une limite de 60 secondes par
invocation. Il conserve les 29 unités demandées et les omissions explicites
si le budget restant ne permet pas un nouveau lancement. Un échec du premier
mode n'efface pas les deux suivants. La qualification complète reste préalable au banc.
Le plafond de collecte est 16 MiB ; aucun champ de preuve n'est supprimé.

## Contrôles préparés

`full_census_collector_test.py` emploie de petits payloads FULL réels avec
processus factices : espaces exacts pour n distinct de K, trois profils,
W1/W8/W48, chemins sans mémo, inactivité explicite, mémoire au seuil et à
un octet près, corruptions avant hit sémantique, absence de publication après
échec et interruptions avant/après le processus. Il contrôle aussi le
calendrier, les comparaisons, les intentions et le partage exact des 29 unités
entre tentatives et omissions.

La porte native `full_bench_io.py` est préparée pour G4 seulement : 400 appels,
387 succès attendus (384 modes plus trois témoins), 13 refus. Tous les modes
partagent les mêmes octets et la même géométrie sur son petit nuage. Ces
nombres sont des attendus avant exécution native. Les contrôles Python
normal/−O sont transmis séparément avec leurs comptes effectivement observés.

Contrôles Python effectivement passés en normal et `-O` :

- collecteur census : 201 appels factices, 386 corruptions refusées,
  108 décodages, sept calendriers, deux interruptions, onze comparaisons,
  1 892 contrôles ;
- modèle FULL général : 462 appels factices, 391 calendriers,
  une interruption, 36 704 contrôles ;
- collecteurs parallèle, vertical, catalogue, mémo et réutilisation sémantique :
  lignes de verdict inchangées et toujours conformes.

Style tower et analyse AST des nouveaux scripts conformes. Aucun des 400
appels natifs préparés n'a été exécuté localement.

Le juge v5 compare aussi les réussites disponibles entre les voies possédée
et empruntée, par nuage/K et masque143. Tous les autres compteurs de travail
doivent être identiques ; seuls les tests ponctuels du census emprunté sont
multipliés par deux dans la comparaison, jamais dans le résultat produit.
Un essai omis ailleurs ne masque pas une divergence de paire déjà disponible.
Les paires à compteur nul restent valides, mais le nombre de paires positives
est publié séparément. Le schéma de ce diagnostic est
`ehgp.v11.full_census_comparison.v1`. Les captures antérieures gardent leur
lecteur v4 ; cette extension ne leur transfère aucune preuve nouvelle.
