# Suivi développeur et audit FULL → points

État courant du 3 octobre 2026. Ce fichier remplace ses résumés successifs ;
les preuves et premiers échecs restent dans les reçus liés.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_full_forests`, `not_claimed`.

Priorité : FULL K1..5 ≤200 ms G4, puis K1..10 ; hiérarchie des points et
HDBSCAN/Zoltan ensuite. **Le contrat 200 ms reste ouvert.**

## Dernier lot clos

[reuse1](../receipts/full_regular_vertical_20261003/reuse1/README.md),
source `ae817d09e` : **3339/3339 portes, 299/299 ASan18 et 29/29 FULL**,
aucune omission ni divergence. Release, ASan/UBSan24, TSan21, u21/u24,
poison21 et complément ASan18 passent ; Clang est absent, pas qualifié.
Les six groupes appariés ont les mêmes objets canoniques. Les hashes des
29 grands dumps sont enregistrés ; ces fichiers supprimés après décodage
ne sont pas présentés comme encore rehachables. Lecteurs normal/−O :19 témoins et126 corruptions ;292 mutants, dont290
code/ligne et2 constructions attendues. Le calendrier ne contient aucune
paire census possédé/emprunté, donc aucune nouvelle preuve inter-voies.
La VM ciblée est arrêtée,
la clé retirée et la réserve libérée, sans erreur ni avertissement.

FULL K1..5 W48, mode2047 (lookup dense et réemploi vertical) :

| Nuage entier | u21 | u24 |
|---|---:|---:|
| ng00 | 1,463 s | 1,483 s |
| ng01 | 1,155 s | 1,154 s |
| ng02 | 1,515 s | 1,531 s |

Un processus par configuration, sans répétition statistique. Sur ng00u21,
les modes511/1023/2047 donnent1,622/1,436/1,463 s : le dernier gagne sur
les forêts (815/705/658 ms), mais le catalogue varie (807/730/805 ms).
Ne pas annoncer chaque option comme un gain FULL acquis. Mode2047 W1/W8 :
15,307/2,512 s. Uniformes8k/16k/32k W48 :377/839/1844 ms.
Les 29 FULL cumulent53,437 s ; processus69,972 s, collecte sémantique
226,133 s, campagne297,688 s. Ces coûts ne sont pas interchangeables.

Données : sous-nuages1mm entiers sans sol de08/000000,100,200,
39885/35551/45845 sites, donc une seule séquence. Cloud, Pool, lecture et
sérialisation sont séparés ; segmentation/préparation hors ligne exclues.
Calcul CPU sur G4, aucune accélération GPU. Mémoire Buffer, pas RSS.
Aucun transfert au profil brut float32 ou à plusieurs séquences.

Le précédent [census2](../receipts/full_census_20261003/census2/README.md)
garde1,280–1,793 s, 3141/3141+266/266 portes,29/29 FULL et son juge
inter-voies. Les autres paliers restent consultables dans les reçus ; leurs
preuves ne se transfèrent pas implicitement au code courant.

## Travail actif

Le [graphe exact des petites feuilles](../docs/CATALOGUE_SMALL_PAIR_GRAPH.md)
et la [classification régulière avec terminal singleton](../docs/FULL_REGULAR_CLASSIFICATION_SINGLETON.md)
sont préparés en `a75763a4f`, hors source de reuse1. Modèles et relectures
statiques passent ; mutants, budgets et sorties attendent le prochain G4.
Le graphe conserve le repli au-delà de32 sites et compte256 octets par
espace physique. Le nouveau banc sépare tous les comptes catalogue des
comptes forêt et distingue census, lookup et singleton dans les pas.

Le catalogue reste dominant :505–673 ms de génération dans reuse1.
Le banc est gelé en `b7b244942`. Son premier précontrôle est refusé sans
mutation GCP : durée demandée4200 s, cible configurée3600 s. L’adaptation
du script gardé de durée est limitée à cette cible exacte. Les79 scénarios
simulés passent normal/−O ; la relecture après configuration confirme4200 s,
SPOT/STOP et la même génération arrêtée. `graph2`, source245eee1ae, est
close en échec avec3410/3483 portes et297/309 ASan18,40 FULL non lancés.
Résultats rapatriés, arrêt ciblé/clé/réserve clos, aucune erreur de fermeture.
Sa qualification échoue sur deux défauts de tests : `==`
absent pour Wide dans la nouvelle sonde singleton, puis injection dans une
allocation q1 devenue inexistante. Les corrections gardent les comparaisons
séparées du numérateur/dénominateur et exigent un vrai census sur le carré
avant d'y injecter la panne. Aucun chrono FULL de graph2 n'est acquis.
Prochaine comparaison : graphe OFF/ON et feuilles16/8, sorties exactes
identiques exigées ; jamais imposer des comptes géométriques identiques
entre tailles de feuilles différentes. La classification et les singletons
ont leurs portes indépendantes, sans leur attribuer le gain du graphe.

Le port [poids de présentation q4](../docs/Q4_POIDS_PRESENTATION.md) est
préparé séparément et exclu des sources graph2/graph4. Il recycle les
coefficients du tétraèdre initial, avec certificat i128 ou repli Wide,
sans remplacer le prédicat générique sur d'autres tétraèdres. Modèle
normal/−O :1761 requêtes,53877 contrôles,114 corruptions. Le nouveau juge
masquait encore stderr/code ; correction et13 scénarios de processus/58
contrôles avant qualification. Aucun gain natif attribué à ce port.

La proposition générale MEB support+extérieur, inspirée de la v10, est
écartée pour K5 : sur108 petites fixtures le modèle augmente présentations
341→392 et tests828→1294. Elle progresse pour les grandes parties, sans
chrono LiDAR. Une proposition q3 directe différente réduit ces comptes
à291/770 sur le même lot, mais régresse dans trois cas ; aucun port ni
gain natif acquis. Toute voie doit recertifier toute la partie et conserver
le support local d'arité minimale puis lexicographique, même cosphérique.

## Critique des audits et échecs conservés

[dense1](../receipts/full_dense_20261003/dense1_failure/README.md) garde
3255/3267+285/287 et20 FULL non lancés. Sa sonde d'orientation utilisait
les coordonnées d'un Point temporaire détruit en C++20. Correction e937,
puis requalification verte dans reuse1 ; aucun prédicat produit modifié.
Le harnais avait perdu stderr de l'enfant : aucune trace ASan ne doit être
inventée. Le correctif d60 conserve désormais les diagnostics des refus.

[census1](../receipts/full_census_20261003/census1_failure/README.md) garde
le mutant de cohorte invalide à la compilation, distinct d'une mort prévue.
[vertical1](../receipts/full_parallel_20261003/vertical1_failure/README.md)
garde la fixture sans aucun miss saturé ; six points ont rétabli la
non-vacuité, sans retirer la garde. Les nouveaux tests de classification
ont aussi révélé une fixture q4 absente et une macro void dans un helper
non void, corrigées avant compilation. Une affirmation d'auditeur ne
remplace ni preuve, ni compilation, ni exécution.

Autres corrections de lecture : catalogue entier ≠ assemblage ; un
intérieur non vide ne signifie pas cellule étendue ; doublement de buffers
refusé sans admission de la mémoire simultanée ; cohorte de trois centres
préservée malgré une non-naissance intercalée. Les premiers échecs sont
conservés dans les reçus, sans encombrer ce suivi avec leurs anciennes étapes.

## FULL → points : verrous conservés

Le [suivi exact](../receipts/audit_full_hierarchie_20261002/suivi_verrous/README.md)
et les fixtures de référence établissent :

- `cover` et MR₂-bord ne sont pas équivalents. Pour `{0,2,5}`, K2,
  cover contient AB ; MR₂-bord publie seulement ABC à la jonction.
  IoU maximal pour AB :1 contre2/3, sans supériorité générale sur HDBSCAN.
- Plusieurs K peuvent produire des groupes core croisés :
  `{0,10,11}` à K1 et `{10,11,26,27}` à K2 dans la fixture à sept sites.
  Une projection laminaire doit exposer les groupes qu'elle perd.
- Séparer groupes présents dans FULL, pertes de projection,
  incompatibilités et pertes du sélecteur. Le meilleur bloc pour une cible
  ne donne pas une partition simultanée. Core/cover ensemblistes et
  projection LCA exclusive restent distincts.

Les lemmes B–F restent soumis au quotient local, à la surjection globale,
au raffinement exhaustif, aux représentants valides et à la déduplication
avant plateau. Un terminal mémoïsé `(b,k)` vaut à coupe fermée `a≥λ_b`,
ou ouverte `a>λ_b`. Conserver `λ_b≤β(R)<λ_parent` avant un parent.
Une boule admise n'est pas forcément nécessaire à π₀ ; Euler ne certifie pas.
Plateaux et cohortes restent atomiques ; `mcs` s'applique même avec
`allow_single`. Masse recouvrante et exclusive sont distinctes. ER0h à
arbre/activations fixes ne prouve pas la stabilité aux déplacements.
Pondérations FULL, EOM près des égalités et condensation restent ouverts.

## Limites de preuve et de fonctionnement

Le [FULL initial](../receipts/full_20261002/README.md) garde42 comparaisons
exactes v10 sur petites fixtures ; le différentiel LiDAR entier reste ouvert.
Aucun label SemanticKITTI n'est utilisé. Les rapports privés absents restent
recensés dans le [rapprochement des sources](../receipts/audit_full_hierarchie_20261002/suivi_verrous/commit_reconciliation.json).

`MemoryBudget::admit` exige un pilote unique. Dans la matrice fonctionnelle,
les descendants sont clos en fin de commande ; `isolation=not_certified`.
Le banc démarre dans une commande séparée. Les intentions persistées avant
subprocess ne prouvent pas à elles seules PID ou lancement. Toute compilation,
tout test et chrono natifs passent exclusivement par une session G4 gardée.
Le sparse checkout et les déplacements d'archives closes ne modifient ni
les sources Git, ni les reçus : octets hachés et chemins de lecture conservés.
Aucun contrat FULL200ms, résultat GPU ni avantage général sur HDBSCAN acquis.
