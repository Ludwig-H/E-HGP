# Suivi développeur et audit FULL → points

État courant du 3 octobre 2026. Ce fichier remplace ses résumés successifs ;
les preuves et premiers échecs restent dans les reçus liés ci-dessous.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_full_forests`, `not_claimed`.

Priorité utilisateur : FULL K1..5 ≤200 ms G4, puis K1..10 ; la hiérarchie
et HDBSCAN/Zoltan viennent après. Inspiration critique de toute la v10 autorisée.
**Le contrat 200 ms reste ouvert.**

## État mesuré

La [capture census2](../receipts/full_census_20261003/census2/README.md),
source`cc93360a3`, est close : **3141/3141 portes et266/266 ASan18**,
29/29 essais FULL, aucune omission ni divergence. Lecteurs normal/−O :
16témoins/107corruptions ;281mutants, dont279code/ligne et2constructions
attendues. Les29hashes FULL sont enregistrés ; les grands dumps supprimés
ne sont pas rehachables lors de cette relecture. FULL K1..5 W48, avec
verticales parallèles et census réutilisé, mesure **1,280–1,793 s** :

| Nuage entier | u21 | u24 |
|---|---:|---:|
| ng00 | 1,588 s | 1,680 s |
| ng01 | 1,280 s | 1,282 s |
| ng02 | 1,770 s | 1,793 s |

Sur ng00u21, modes127/255/511 :2,929/1,732/1,588 s. Pour le dernier,
domaine837,462 ms et forêts750,462 ms. L'égalité des sorties et le travail
census entre voies sont jugés séparément. Un processus par configuration,
sans répétition statistique ; ces trois trames restent une seule séquence.
Arrêt ciblé, retrait des clés et réserve certifiés.

La [capture combined3](../receipts/catalogue_single_full_20261003/combined3/README.md),
source`90dd48bd2`, conserve le palier précédent :2799/2799 portes,
209/209 ASan18,27/27 essais conformes, FULL2,482–3,243 s. Ses preuves ne
se transfèrent pas aux nouveaux changements.

La [capture forest3](../receipts/full_parallel_20261003/forest3/README.md),
source `3dbfd1c32`, garde le palier précédent : FULL4,358–6,163 s,
18 essais conformes mais uniforme32k omis avant lancement ; `failed_remote`.
Ses portes2673/2673 et201/201 ASan18 ne qualifient aucune conformité19/19.

Les paliers antérieurs restent dans [memo1](../receipts/full_memo_20261003/memo1/README.md)
et [assembly1](../receipts/catalogue_assembly_20261003/assembly1/README.md),
avec leurs sources et omissions propres. Les payloads supprimés ne sont pas
présentés comme encore rehachables ; les mutants code/ligne restent séparés
des refus de construction attendus.

Ces captures utilisent les mêmes sous-nuages1mm entiers sans sol de
08/000000,100,200 (39885/35551/45845 sites), donc une seule séquence.
Cloud, Pool, lecture et sérialisation sont séparés des API mesurées ;
segmentation/préparation hors ligne ne sont pas chronométrées ici.
Calcul CPU sur G4, sans accélération GPU. Réservations Buffer, pas RSS.

## Développement en cours

Trois raccourcis sont préparés, sans qualification native héritée :
[graphe exact des petites feuilles](../docs/CATALOGUE_SMALL_PAIR_GRAPH.md),
[classification régulière et terminal singleton](../docs/FULL_REGULAR_CLASSIFICATION_SINGLETON.md).
Leurs modèles et relectures statiques passent ; leurs mutants, refus,
budgets et sorties attendent le prochain lot G4. Le graphe conserve le
repli au-delà de 32 sites et facture 256 octets par espace physique.
Une erreur de macro dans un test et une fixture manquante ont été corrigées
avant compilation, sans réduire les obligations de couverture. L'audit
reste contradictoire ; une affirmation d'auditeur ne vaut pas preuve.

La [forêt parallèle régulière](../docs/FULL_PARALLEL.md) conserve publication
DSU exacte et plateaux atomiques. L'échec [forest1](../receipts/full_parallel_20261003/forest1_failure/README.md)
venait d'une fixture ligne024 prétendue étendue alors que m=qmin=2 ;
la reprise ajoute un diamant étendu nécessaire et une q4 régulière.
Les refus locaux d'espace restent archivés. Le sparse checkout de mon seul
worktree omet des copies historiques, sources Git conservées. Des archives
source closes occupent le stockage local plus grand avec hashes identiques
et chemins de lecture conservés ; reçus, résultats et gardes restent intacts.

Le [catalogue en une passe](../docs/CATALOGUE_SINGLE_PASS.md), port608aecc75,
et le [q3 i128 contrôlé](../docs/PREDICATS_I128_CONTROLES.md), portb6729d827,
sont inclus dans combined3, sans transfert aux changements ultérieurs.
[combined1](../receipts/catalogue_single_full_20261003/combined1_failure/README.md)
garde2774/2799 et205/209, zéro FULL et zéro mutant tower jugé : accesseur
inexistant et somme flottante non portable Python3.10/3.12. Correction90dd ;
combined2 garde son refus local d'espace avant toute mutation GCP.

Les [verticales parallèles](../docs/FULL_VERTICAL_PARALLEL.md), port7e7af48fc,
le [census emprunté](../docs/CENSUS_EMPRUNTE_UNE_PASSE.md), port5c90e52cb,
son [raccord FULL](../docs/FULL_CENSUS_REUTILISE.md), port5e39d2726, et les
[cohortes de naissances](../docs/FULL_BIRTH_RUNS.md), port220047c07,
sont conservés dans l'échec clos [vertical1](../receipts/full_parallel_20261003/vertical1_failure/README.md),
source5e39d2726 :3068/3075 portes et254/255 ASan18, huit builds réussis.
La porte descents échoue sur sa non-vacuité :1338 contrôles mais aucun miss
saturé pour ses nuages≤5 sites àKmax4. Aucune divergence observée dans la
parité ; aucun mutant tower jugé et aucun benchmark lancé. Fermeture certifiée.
Le correctif372c296c3 ajoute six points avec quatre intérieurs hors catalogue,
sans retirer l'assertion. Lecteur d'échec normal/−O :7positifs/65corruptions.
Le plan suralloué1800s pour fenêtre1737s est conservé ; correction95d178314
à1720s pour les sessions suivantes, gardes intactes.

Le [certificat q3](../docs/PREDICATS_Q3_CERTIFICAT.md), port3d3e3cd20,
et le [banc census](../docs/BANC_CENSUS_REUTILISE.md), portc6954f231,
sont dans [census1](../receipts/full_census_20261003/census1_failure/README.md),
échec clos :3128/3129 portes et264/264 ASan18. Toutes les configurations
fonctionnelles passent ; `forest_cohort_nonbirth_reset` échoue à compiler.
Les87 autres mutants tower sont tués ; aucune des29 mesures prévues lancée.
Ce mutant invalide reste distinct d'une mort attendue à la compilation.
Correctioncc93360a3 : même défaut logique, sans reset d'optional, qualifié
par la reprise `census2` avec juge inter-voies renforcé. Capsule d'échec :
lecteurs normal/−O,11témoins/80corruptions ; contrelecture indépendante.

Le [certificat d'orientation](../docs/PREDICATS_ORIENTATION_CERTIFICAT.md),
portf238f5b8c, et la [table directe des naissances](../docs/FULL_DENSE_BIRTH_LOOKUP.md),
port584666a5f, sont dans l'échec clos [dense1](../receipts/full_dense_20261003/dense1_failure/README.md),
source768070ddb :3255/3267 portes et285/287 ASan18,20 FULL non démarrés.
Arrêt ciblé, clés et réserve certifiés. La sonde d'orientation y échoue : range-for sur les coordonnées d'un Point
temporaire détruit en C++20. Correction isolée e937aa72f par Point local,
aucun prédicat produit modifié. La campagne d'origine reste conservée ;
la qualification et les chronos de ces deux ports restent attendus.
Le harnais a perdu stderr du processus ASan : diagnostic statique de
durée de vie confirmé, aucune trace ASan conservée ne doit être inventée.
Instrumentation corrigée end60ba2981, cinq tests de processus simulés
normal/−O. Capsule d'échec :11témoins/66corruptions,288mutants détaillés.
Le [réemploi des cellules régulières](../docs/FULL_REGULAR_VERTICAL_REUSE.md)
est implémenté opt-in en14e017edc, encore hors qualification native : graine basse
réutilisée puis coupe fermée exacte, table temporaire4M, cellules étendues
inchangées. Deux contrelectures statiques favorables ne remplacent pas les
tests G4, y compris mémoire et concurrence. Il est absent de `dense1`.
La reprise gardée `reuse1`, sourceae817d09e, qualifie ces changements
et compare29FULL en modes511/1023/2047 ; aucun résultat encore publié.
Le graphe de couples J2 et la classification régulière directe restent
des travaux séparés, exclus de cette source.

La critique s'applique aussi aux auditeurs : proposition de doublement de
buffers rejetée après confrontation à ARCHITECTURE§7.1 ; durée du catalogue
entier corrigée lorsqu'elle était présentée comme durée d'assemblage ;
non-régularité distinguée d'un intérieur non vide ; gardes des diagnostics
resserrées lorsque des traces pouvaient rester sans cellule explicative.
Une contrelecture a aussi renforcé la fixture des cohortes : une non-naissance
intercalée doit conserver le groupe maximal de trois centres, pas seulement
deux groupes de même taille. Toute correction garde son premier échec.

## Preuves antérieures utiles

[FULL initial](../receipts/full_20261002/README.md) garde42 comparaisons
exactes v10 sur petites fixtures ; le différentiel LiDAR entier reste ouvert.
[Balayage](../receipts/full_sweep_20261002/README.md),
[cache J2](../receipts/catalogue_optimizations_20261002/README.md),
[catalogue parallèle](../receipts/catalogue_parallel_20261002/README.md)
et [premiers échecs](../receipts/developpement_20261002/README.md)
gardent sources, premiers refus et périmètres propres, sans transfert de preuves.

## FULL → points : verrous conservés

Le [suivi exact](../receipts/audit_full_hierarchie_20261002/suivi_verrous/README.md)
et les fixtures de la référence conservent les faits suivants :

- `cover` et MR₂-bord ne sont pas des hiérarchies équivalentes. Pour
  `{0,2,5}`, K2, cover contient AB ; MR₂-bord ne publie que ABC au plateau
  de jonction. Le meilleur IoU de la cible AB vaut 1 contre 2/3. Les dates
  d'attache diffèrent aussi : aucun facteur global ne les identifie.
  Ce témoin ne prouve aucune supériorité générale de cover sur HDBSCAN.
- Les groupes core de plusieurs K peuvent se croiser. La fixture
  `{0,10,11,26,27,45,46}` garde `{0,10,11}` à K1 et `{10,11,26,27}` à K2,
  avec leurs naissances et parents exacts. Une hiérarchie commune doit
  rendre explicites les pertes dues à la contrainte laminaire.
- Séparer présence d'un groupe dans FULL, pertes de projection, incompatibilité
  de groupes et pertes du sélecteur. Un score au meilleur bloc borne la
  sélection dans cet arbre ; il ne fournit pas une partition simultanée.
  Core/cover ensemblistes et projection LCA exclusive restent distincts.

La qualification FULL sur petites fixtures ne ferme ni le différentiel LiDAR
entier, ni la hiérarchie sur les points, ni la condensation.

## Gardes du futur raccord FULL

La contrelecture des lemmes B–F est favorable avec Q1 corrigé : quotient
local, surjection vers les composantes globales, raffinement exhaustif,
représentants valides et déduplication des racines avant le plateau.
Un terminal mémoïsé `(b,k)` est valable à coupe fermée `a≥λ_b`, mais à
coupe ouverte seulement `a>λ_b`. Avant un parent, conserver
`λ_b≤β(R)<λ_parent` ; ne pas remplacer trop tôt tous les représentants.
La terminaison utilise les niveaux de toutes les k-parties. Une boule
admise n'est pas forcément nécessaire à π₀ ; Euler reste un diagnostic.

Conserver plateaux et cohortes de départ atomiques, `mcs` même avec
`allow_single`, mémoire simultanée complète et distinction masse
recouvrante/exclusive. La stabilité ER0h à arbre et activations fixes
n'est pas une stabilité générale sous déplacement des coordonnées.
Le modèle pondéré FULL et les comparaisons EOM proches d'une égalité
restent ouverts ; aucun label SemanticKITTI n'est utilisé.

## Constats antérieurs clos et limites restantes

Les défauts F3/F6, Result sur refus, arrêts anormaux usurpés, collecteurs
et mutants sont corrigés avec leurs premiers échecs conservés dans les
[reçus de développement](../receipts/developpement_20261002/README.md).
La précision numérique est attachée aux profils réellement testés ;
q3 garde son repli large en21/24 ; q1/q2/q4 emploient leurs bornes i128 propres.
Le test ASan24 ne couvre pas q3 natif 18 : d'où son complément distinct.

`MemoryBudget::admit` exige un pilote unique. Dans la matrice fonctionnelle,
les descendants ne sont fermés qu'en fin de commande par le worker ; les
sondes gardent `isolation=not_certified`. Le banc démarre dans une commande
séparée. Les builds, tests et chronos natifs se font exclusivement sur G4,
via une session gardée avec arrêt ciblé et retrait des clés certifiés.
P2 traçabilité de l’audit17 : les pilotes persistent maintenant l’intention
avec argv/profil/hashes avant subprocess.run, puis le résultat avant décodage.
Une intention seule ne prouve ni PID ni lancement ; modèles d’interruption verts.

L'audit v10 est consolidé sans prétention d'exhaustivité : les rapports
privés absents restent recensés dans le
[rapprochement des sources](../receipts/audit_full_hierarchie_20261002/suivi_verrous/commit_reconciliation.json).
Aucun gain de performance, contrat FULL, résultat GPU ni avantage général
sur HDBSCAN n'est déduit de ces audits.
