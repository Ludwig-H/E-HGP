# État de l'audit complémentaire v8

13 septembre 2026, cinquième passe après la publication `8e406f9b`.
Intervenant **AUDITEUR_COMPLEMENTAIRE**, distinct du développeur et de
l'auditeur historique. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le partage Tubes et le filtre axial par maximum sont publiés et leurs
sources correspondent aux snapshots déjà audités. La troisième tranche,
addition des colonnes et intersection avec les crédits locaux, est en
construction : ses capacités et ses mesures ne sont pas héritées de
cette publication.

## Suites actives transmises au développeur

| Priorité | Constat vérifié et aide concrète |
| --- | --- |
| P0, coût aval | Le [consommateur indexé q2](P0_CONSOMMATION_INDEXEE_Q2.md) paie les candidates publiées : à n32k/Kmax10, 6 483 670 requêtes, 455 418 paires sous le seuil, 295 540 004 visites. Comparer filtres et partage des recherches sur leur travail total. Temps bruts non qualifiés. |
| P0, intersection | La [composition par rangs](P0_INTERSECTION_RESIDUS.md) construit l'intersection exacte en O(h|B|+D), sans expansion des paires ; 48 plans, 172 800 paires vérifiées, deux mutants rejetés. Alternative à comparer au parcours intégré du constructeur, dont la règle de bornes est mathématiquement correcte. |
| P0, orientation | Une [rotation isométrique u16](P0_AXES_ET_ROTATIONS.md) fait passer le filtre axial publié de 6 483 670 à 256 millions de candidates à n32k/h10. Le repli est compact ; la recherche générale de directions reste ouverte. |
| P0, raccord | Les [fixtures de census](P0_CENSUS_Q2_ET_COQUILLE.md) distinguent profondeur stricte, coquille, supports et boules. Elles complètent les [bornes de l'autre auditeur](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches), lues dans sa publication 9633d8ef. |
| P0, autres pistes | La [somme des colonnes](P0_SOMME_TEMOINS_AXIAUX.md), les [nappes 2D](P0_NAPPES_2D.md), l'[ordre des témoins](P0_ORDRE_TEMOINS.md) et les [groupes recouvrants](P0_GROUPES_RECOUVRANTS.md) restent des apports bornés, à comparer sans qualification globale. |

Les réponses et la coordination figurent dans le
[journal partagé](../COORDINATION_MORSEHGP3D_V8.md). **P0 reste ouverte** :
les résidus généraux, les requêtes de census, q3/q4 complets et la tour
FULL ne sont pas payés par un gain sur les seules nappes alignées.
Aucun consommateur produit ni contrat 50k n'est qualifié par nos prototypes.

## Contre-vérifications de cette passe

Le [reçu des 594 mesures publiées](SHARED_AXIS_MEASURES_CHECKS.json)
confirme 504 configurations, cinq matrices, les sources et la provenance
communes, les comptes et les tableaux du README. Les lecteurs normal/−O
passent ; aucune mesure, compilation ou suite CTest relancée pour cette
lecture. Aucun nouveau défaut de protocole relevé sur `8e406f9b`.

Le [consommateur indépendant](Q2_INDEXED_CONSUMER_CHECKS.json) passe 16 cas
O2 et UBSan, 3 076 448 tests ponctuels par exécution, trois vrais mutants
C++ réfutés. Le runner normal/−O concorde. Les six grandes consommations
sont réellement exécutées, sans oracle exhaustif de leurs produits.
Les [fixtures de coquille](Q2_CENSUS_FIXTURE_CHECKS.json) ajoutent 192
requêtes et quatre contre-modèles ; elles ne prétendent pas fournir FULL.
L'[intersection](RESIDUAL_INTERSECTION_CHECKS.json) a son propre snapshot
embarqué, ses rejets de propriétaires/voies et ses mutants de rangs.

Le [delta d'allocation](ALLOCATION_DELTA_CHECKS.json), cpp `522009ad…`,
conserve les garanties des plans : batch passe 10 872 contrôles, affectation
1 110, dont 20 pannes avec cible conservée et 24 accès déplacés refusés.
Ce contrôle GCC strict -O1/UBSan porte uniquement sur l'initialisation
conditionnelle des tableaux. La compilation supplémentaire -O2/UBSan
refusée par GCC est conservée séparément ; aucun succès O2 n'en est déduit.
Les nouveaux changements axiaux restent hors de ce contrôle.

## Points clos et preuves conservées

L'[affectation après panne](P0_PLAN_ASSIGNMENT.md) est corrigée dans les
sources publiées : le [reçu du partage](BATCH_EXCEPTION_REVIEW.json)
conserve 20 injections, 195 lots / 585 voies, puis 3 051 voies du juge
Tubes avec préparation comptée une fois. Les hashes publiés sont ceux
déjà rejoués en normal/−O. Les défauts antérieurs de copie du propriétaire
et d'alias du tampon d'entrée restent aussi clos.

Les [trois constats de protocole](P0_RECUS_APPARIES.md) sont clos sur leurs
pins : mélange Release/Debug refusé et provenance conservée, marqueur de
checksum axial contrôlé, vraie nappe inactive acceptée. Le
[reçu avec addenda](PAIRED_PROVENANCE_CHECKS.json) conserve les preuves.
Le [runner R3](CAMPAIGN_RECEIPT_R3_CHECKS.json), les [729 tuples R3](CAMPAIGN_R3_MEASURES_CHECKS.json)
et les [captures initiales](CAMPAIGN_INITIAL_CHECKS.json) restent des
objets distincts ; aucun mélange réel de campagnes n'est allégué.

- [Filtre axial](AXIS_Q2_IDENTITY_CHECKS.json) : 131 plans, 24 143 paires,
  874 128 tests du census ; quatre mutants C++ rejetés.
- [Rotations](AXIS_ROTATION_CHECKS.json) : 27 configurations × trois plans
  par mode, neuf petites avec census ; 3 709 380 tests ponctuels.
- [Nappes 2D](SHEET_RECTANGLES_CHECKS.json) : 42 cas par mode, C++20
  strict/UBSan ; 3 533 502 tests du juge, mutant sans certificat réfuté.
- [Rangées transverses](P0_RESIDU_TRANSVERSE.md), [rails](P0_RAILS.md),
  [quantificateurs](P0_QUANTIFICATEURS.md), [prédicats](PREDICATES_CHECKS.json),
  [tubes](TUBES_CHECKS.json) et [ordre Dual](DUAL_ORDER_CHECKS.json) :
  preuves et limites propres aux octets épinglés, conservées intactes.

## Rejeu et entretien

Les notes donnent les commandes exactes. Les nouveaux tests utilisent
le commit publié `8e406f9b` ou les snapshots embarqués dans leurs reçus.
Les anciennes gates peuvent donc refuser légitimement le worktree courant.
Une correction demande une nouvelle qualification, jamais la réécriture
d'un reçu clos. Les sujets résolus quittent la liste active ; les preuves
reproductibles restent accessibles dans leurs notes et reçus.

Aucun fichier constructeur ou de l'autre auditeur n'entre dans nos commits.
La porte documentaire générale exclut ces audits : nos Markdown et le
journal sont donc aussi validés explicitement. GCP non utilisé.
