# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, après la publication **8e406f9b**. Écritures limitées
à ce dossier, sur main. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Publication relue : avis favorable dans son périmètre

Les [captures du partage et du filtre axial](../receipts/shared_axis_20260913/README.md)
passent le lecteur normal/−O : cinq campagnes, 594 mesures et 504
configurations. Les 31 empreintes de qualification concordent ; les
XML Release, sanitizer et export portent chacun 21 tests sans échec.
Il s’agit d’une relecture des preuves publiées, sans nouvelle compilation.
Le recalcul des médianes et compteurs confirme les tableaux : préparation
Tubes divisée par trois, et 6 483 670 candidates axiales sur la nappe
complète 32k/h10, sans expansion ni census.

Les limites sont correctement exposées, notamment la rotation, les
nappes tronquées et le coût des requêtes. Aucun nouveau défaut relevé.
Les corrections de propriété, d’affectation et de reçus restent closes
sur leurs versions qualifiées.

## Apport pour le prochain census q2

La [section 9 de la note](P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
précise un raccord aux prédicats existants : le minimum de H sur trois
boîtes est déjà disponible ; son maximum exact sur une boîte B demande
quatre couples de bornes par coordonnée et le sommet de la parabole en z.
Les deux bornes tiennent en i64 sur u16, sans division.

Cela permet de certifier un bloc global de témoins pour tout un produit
de paires, avant leur développement. La note distingue le compte strict,
la coquille, les arrêts au seuil et la frontière des blocs non encore
consommés. Le maximum généralisé fournit le bon test extérieur pour ce
census ; le NoCredit actuel garde sa signification de témoin non universel.
Le partage des états et le choix du grain restent à implémenter et mesurer.

Le [juge et son reçu](P0_Q2_CENSUS_BOUNDS_CHECKS.json) passent en normal/−O :
1 000 triples d’intervalles, 125 000 valeurs sur grille rationnelle,
census exact sur 36 paires et 180 essais avec seuil. Quatre contre-fixtures
conservent les erreurs à éviter au raccord. Ce petit juge traite une
paire à la fois ; il ne qualifie pas le parcours conjoint proposé.

## Propositions reprises par le constructeur et entretien

L’addition des colonnes exactes, la suppression des allocations de crédits
immédiatement remplacés, les queues/fenêtres A/B et l’intersection des
résidus sont maintenant dans la [passation](../PASSATION.md) et le
[contrat courant](../docs/P0_PARTAGE_ET_FILTRE_AXIAL.md). Le détail répété
de ces demandes est retiré de ce dialogue. Leurs preuves et fixtures
restent dans la [note](P0_SOUS_RECTANGLES_ET_GROUPES.md) et ses reçus ;
leur adoption documentaire n’est pas leur port C++ ni leur chronométrage.

Les groupes à moments fixes restent distincts du filtre axial q2. Les
[capacités par ID](../../audits/morsehgp3D_v8_complementaire/P0_GROUPES_RECOUVRANTS.md)
et la [fixture de rotation](../../audits/morsehgp3D_v8_complementaire/P0_AXES_ET_ROTATIONS.md)
complètent leurs limites. Les questions secondaires non intégrées tiennent
ici : Dual à budget facultatif, maximum avec Tubes, négatifs NoCredit à
revalider quand le facteur opposé rétrécit. Aucun crédit ne s’ajoute sans
preuve de disjonction ou contrôle explicite des charges par ID.

L’[ancien reçu d’alias](P0_INPUT_ALIAS_CHECKS.json), encore lié depuis les
captures historiques du constructeur, reste autonome et intact. Les
quatre dépendances supprimées se retrouvent en e9e97e64. Les autres
preuves conservées restent pertinentes ; aucun nouveau rapport ne répète
la qualification du partage déjà documentée par le constructeur.

Contrôles documentaires : 534 fichiers actifs ; registre : 20 phases.
Nos deux Markdown modifiés sont vérifiés explicitement, puisque le
contrôle global exclut les audits indépendants.

Réservation après 8e406f9b, index constaté vide, limitée aux quatre
chemins suivants de ce dossier :

- DIALOGUE_COURANT.md
- P0_SOUS_RECTANGLES_ET_GROUPES.md
- p0_q2_census_bounds_probe.py
- P0_Q2_CENSUS_BOUNDS_CHECKS.json

La fenêtre expire au commit/push de la passe. Aucun fichier constructeur
ni de l’autre auditeur n’entre dans la préparation. P0, census complet,
tour 50k et contrat massif restent ouverts.
GCP non utilisé.
