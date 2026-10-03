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
sont en qualification dans **graph4**, source91890b457. La matrice principale
passe3483/3483, le supplément ASan18 passe aussi. Premier calendrier leaf16 :
20/20 FULL conformes ; mode4095 W48 LiDAR1,043–1,407 s. Les premières
mesures leaf8 montent à3,9–5,0 s ; garder16 pour la prochaine campagne.
Ces observations sont provisoires jusqu'au rapatriement et à la clôture.
Le graphe garde son repli au-delà de32 sites et256 octets par espace physique.

[graph2](../receipts/full_pair_graph_20261003/graph2_failure/README.md) conserve
3410/3483+297/309 portes et40 FULL non démarrés. Deux défauts de tests :
comparaison `==` indisponible pour Wide, puis injection dans une allocation
singleton devenue inexistante. La correction compare séparément les deux
entiers bruts et exige un vrai census sur un carré avant injection. Lecteurs
normal/−O :11 témoins et75 corruptions ; contre-revue indépendante concordante.
Aucun défaut produit ni chrono FULL déduit de cet échec. Les précontrôles
[graph3/graph3r2](../receipts/developpement_20261002/graph3_preflight_refusals/proof.json)
ont ensuite refusé le manque d'espace local, avant toute mutation GCP ; le SHA
court n'était pas la cause. Les archives déplacées restent identiques et lisibles.

Trois ports attendent la prochaine qualification native :
[poids q4](../docs/Q4_POIDS_PRESENTATION.md),
[contacts certifiés](../docs/CATALOGUE_CONTACTS_SUPPORT.md),
[MEB différé](../docs/MEB_CONSTRUCTIONS_DIFFEREES.md). Le premier conserve
le prédicat générique sur d'autres tétraèdres ; le second garde tous les tests
hors support ; le troisième garde ordre, supports et sept compteurs logiques.
Modèles normal/−O et contre-revues passent. Ces compteurs ne dénombrent pas
les opérations arithmétiques effectivement évitées. Aucun gain natif attribué.

Le [banc cible compilateur](../docs/FULL_CIBLE_COMPILATEUR.md) prépare24 FULL
appariés baseline/v3, deux répétitions et deux profils, sans IPO. Contre-revue :
le premier collecteur acceptait potentiellement une sélection de tests tronquée.
Les inventaires et sélecteurs exacts sont maintenant exigés ;562 contrôles et58
corruptions passent normal/−O. Qualification native préalable puis essai séparé.

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
