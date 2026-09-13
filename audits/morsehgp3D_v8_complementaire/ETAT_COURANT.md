# État de l'audit complémentaire v8

13 septembre 2026, sixième passe après la publication `f5430f57`.
Intervenant **AUDITEUR_COMPLEMENTAIRE**, distinct du développeur et de
l'auditeur historique. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

L'addition des colonnes et l'intersection avec les crédits locaux sont
publiées. La relecture des sources et des 648 mesures est favorable.
Le développeur ouvre maintenant le census q2 partagé avec collecte des
IDs : nos modèles et notre lecture de son API ne qualifient pas encore
cette nouvelle implémentation.

## Suites actives transmises au développeur

| Priorité | Constat vérifié et aide concrète |
| --- | --- |
| P0, partage aval | Le [parcours partagé](P0_FRONTIERE_PARTAGEE_Q2.md) consomme les résidus f5430f57 : à n32k sur nappe additive, 189 649 460 visites par paire contre 147 292 056 classifications partagées, mais 145 720 627 écritures de continuations. Aucun temps mesuré. Le curseur proposé par l'autre auditeur peut éviter ces écritures sous ordre Z fixe. |
| P0, sortie et réutilisation | Le [contrat entre supports et seuils](P0_CENSUS_PARTAGE_ET_SEUILS.md) précise la clé nuage/IDs/boule, le surplus à conserver lors d'une reprise et les supports retrouvés par antipodes. La première API produit a un seul seuil et annonce correctement des vues temporaires, des coquilles complètes et des incidences distinctes. |
| P0, orientation | La [rotation isométrique u16](P0_AXES_ET_ROTATIONS.md) reste une contre-épreuve de généralisation des colonnes. Le repli compact peut laisser tout A×B ; aucune recherche générale de directions n'est qualifiée. |
| P0, autres comparaisons | La [composition par rangs](P0_INTERSECTION_RESIDUS.md), les [nappes 2D](P0_NAPPES_2D.md), l'[ordre des témoins](P0_ORDRE_TEMOINS.md) et les [groupes recouvrants](P0_GROUPES_RECOUVRANTS.md) gardent leurs preuves et limites. Le census et la tranche FULL minimale priment sur une optimisation indéfinie du seul cas axial. |

Les réponses figurent dans le [journal partagé](../COORDINATION_MORSEHGP3D_V8.md).
**P0 reste ouverte** : coût des sorties, résidus généraux, q3/q4 complets,
WSPD et tour FULL restent à payer. Nos prototypes ne qualifient aucun
contrat 50k. Les modèles de l'autre auditeur conservent leur autorité
propre dans sa [note de bornes et curseurs](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md).

## Contre-vérifications de cette passe

Les [648 mesures additives](ADDITIVE_MEASURES_CHECKS.json) passent la
lecture indépendante : 576 configurations, trois matrices, provenance,
36 pins et tableaux concordants, lecteurs normal/−O. Aucun benchmark,
build ou CTest relancé pour cette lecture ; aucune anomalie significative.
Le rapport constructeur distingue bien réduction du résidu et temps total.

La [propriété des nouveaux plans](AXIS_LIFETIME_CHECKS.json) passe 50 pannes
de construction, quatre déplacements sans allocation et 12 accès déplacés
refusés. Les 96 plans restreints confrontent keeps et fragments sur
345 600 paires ; un mutant de déplacement perdant la restriction est
réfuté. Aucun défaut moteur trouvé. Copie et affectations axiales restent
interdites par cette API, contrairement à CreditPlan.

La [preuve du minimum transversal](P0_PROPRIETE_ET_MINIMUM_Q2.md) écarte
une optimisation sans cas actif : avec les crédits internes q2 actuels,
un plan vide équivaut à h=0, déjà traité avant les tris. Le raccourci
restriction vide proposé devient pertinent seulement si ce contrat change,
par exemple avec des témoins extérieurs. Cette limite ne garantit pas
une vraie survivante au census. 42 restrictions à besoin positif et six
à besoin nul confirment le lemme sur les fixtures.

Le [parcours partagé C++](Q2_SHARED_FRONTIER_CHECKS.json) passe 57 petits
cas en O2/UBSan, 8 278 451 tests ponctuels indépendants, trois vrais mutants
réfutés ; normal/−O concordent. Les six grandes consommations sont terminées,
sans collecte des IDs ni oracle exhaustif géométrique. Les [42 demandes
entre seuils](Q2_SHARED_CENSUS_CHECKS.json) et cinq contre-modèles ajoutent
un contrat de réutilisation, pas une API produit nouvelle.

## Points clos et preuves conservées

Les [594 mesures](SHARED_AXIS_MEASURES_CHECKS.json), le [premier consommateur
indexé](Q2_INDEXED_CONSUMER_CHECKS.json), les [fixtures de coquille](Q2_CENSUS_FIXTURE_CHECKS.json)
et le [delta d'allocation local](ALLOCATION_DELTA_CHECKS.json) gardent leurs
pins et périmètres antérieurs. Les nouvelles gates ne réécrivent pas ces
reçus ; les échecs exploratoires et calibrations y restent explicités.
La [somme des colonnes exactes](P0_SOMME_TEMOINS_AXIAUX.md) est désormais
portée et qualifiée localement dans la troisième tranche publiée.

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
le commit publié `f5430f57` ou les snapshots embarqués dans leurs reçus.
Les anciennes gates peuvent donc refuser légitimement le worktree courant.
Une correction demande une nouvelle qualification, jamais la réécriture
d'un reçu clos. Les sujets résolus quittent la liste active ; les preuves
reproductibles restent accessibles dans leurs notes et reçus.

Aucun fichier constructeur ou de l'autre auditeur n'entre dans nos commits.
La porte documentaire générale exclut ces audits : nos Markdown et le
journal sont donc aussi validés explicitement. GCP non utilisé.
