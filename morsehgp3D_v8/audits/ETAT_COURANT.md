# État de l'audit v8

13 septembre 2026. Audit constructeur, avec contrelectures parallèles.
Ce dossier n'est pas l'auditeur indépendant propriétaire des audits v7.

État constructeur courant, troisième tranche P0 : mode axial additif,
intersection intégrée avec un plan local q2 et suppression des allocations
aussitôt remplacées. `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`, hors registre. 26 CTests Release
et Clang ASan/UBSan ; [648 mesures valides](../receipts/additive_q2_20260913/README.md),
sans expansion des grandes candidates, census ou FULL. Le résidu des
nappes alignées diminue mais leur sélection ralentit. L'intersection
est plus sélective que Pool seul, dont la préparation reste plus rapide.
Lire le [contrat](../docs/P0_ADDITION_ET_INTERSECTION.md). Les contrelectures
mathématiques des deux auditeurs sont intégrées ; leurs prototypes de
census et composition restent distincts des résultats du produit courant.
La prochaine étape doit mesurer le coût complet du census q2 partagé.
P0, complexité générale, contrats 50k et massif restent ouverts. GCP non utilisé.

## Historique : deuxième tranche P0 publiée à 8e406f9b

Préparation Tubes partagée
entre q2/q3/q4 et filtre axial q2 par colonnes exactes et index B.
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`, hors registre. Vingt et un CTests Release/sanitizers et
[594 mesures appariées](../receipts/shared_axis_20260913/README.md), sur
un rectangle à la fois, sans census ni FULL. Le partage garde les mêmes
plans ; le filtre axial est sûr mais spécialisé et peut être inopérant
après rotation. Voir le [contrat](../docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).
L'auditeur a fait corriger l'affectation après panne mémoire et deux
omissions des reçus appariés. Ses propositions additives et de queues
A/B restent distinctes des optimisations effectivement mesurées.
P0, contrats 50k, GPU et massif ouverts ; GCP non utilisé.

## Historique : première tranche P0 publiée à 3589a2c9

Implémentation ouverte ensuite par l'utilisateur le 13 septembre :
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=implementation_v8_p0`,
`public_status=not_claimed`. Première brique implémentée : crédits
certifiés et sélection de paires résiduelles sur rectangle séparé,
avec Pool, DualBlocks et Tubes. Huit CTests locaux passent en Release
GCC et sous Clang ASan/UBSan. Les
[729 mesures mono](../receipts/p0_local_credits_20260913/README.md) distinguent
préparation et résidu ; aucune mesure aval ni qualification FULL.
Le [contrat actif](../docs/P0_CREDITS_LOCAUX.md) fixe les preuves, limites
et consignes de raccord. P0 reste ouverte, notamment sur les nappes.
Les défauts signalés sur la copie/affectation du propriétaire, les alias
mutables du tampon d'entrée et l'admission des reçus sont corrigés et ont
des contre-tests permanents. Les coordonnées sont copiées avant certification.
Les premières captures sont conservées à part ; la qualification active
porte sur les builds r3 et les nouveaux reçus uniquement.
La porte d'entrée est satisfaite par les lectures et décisions consignées
ci-dessous ; l'historique suivant décrit l'audit documentaire initial.

La porte d'entrée documentaire est ouverte : demande explicite de refonte,
cadre et périmètre déclarés dans [README](../README.md), base v7 publiée
`dc57ffd5`, état local sale distingué. Les règles du dépôt restent en vigueur.
La lecture intégrale antérieure des parties I/II est consignée dans
[la lecture v7](../../morsehgp3D_v7/docs/LECTURE_ET_CONTRATS.md) ; les
définitions et preuves utiles sont réexaminées pour cet audit.

Trois contrelectures ont traité séparément WSPD/supports/témoins,
contrats et mesures, architecture/CPU/GPU/tests, puis confronté leurs
rapports. La synthèse et les fondements du constructeur ont aussi été
contrelus. Les précisions de régularité, voies actives, compteurs et
maturité ont été intégrées. Aucun avis indépendant futur n'est anticipé.

La [synthèse](../docs/AUDIT_V7_SYNTHESE.md) et le
[périmètre vérifiable](PERIMETRE_ET_PREUVES.md) constituent la livraison
d'audit. Les états « prouvé sous hypothèses », « testé borné », « mesuré »,
« proposé » et « manquant » sont distingués. L'audit couvre la chaîne et
ses contrats, sans prétendre relire chaque ligne des 26 777 fichiers ni
relancer les suites C++. Six lecteurs et deux recalculs exacts sont clos.

Verdict public inchangé : `not_claimed`. Contrats 50k et massif ouverts.
La CLI reste F, les sondes FULL et les prototypes privés sont distingués.
Aucun moteur v8 lors de cet audit initial, aucun usage GCP. Les contrôles documentaires et d'index
de livraison sont conservés dans les reçus v8, sans valeur de preuve moteur.

Décision de priorité ultérieure à l'audit, 13 septembre : l'utilisateur
place en **P0** la suppression des histogrammes systématiques
O(|A|²+|B|²). Le [plan de refonte](../docs/PLAN_DE_REFONTE.md) compare
plusieurs familles d'architectures ; le petit ensemble de témoins ne
constitue pas un choix définitif. Minorants certifiés, coût des résidus
et absence de déplacement du carré sont des critères obligatoires.
C'est une orientation ouverte, pas un nouveau résultat mathématique,
un test moteur ou une qualification de complexité globale.

Complément de passation demandé le 13 septembre :
[VERROUS_ARCHITECTURE](../docs/VERROUS_ARCHITECTURE.md) organise les cinq
autres verrous B1–B5 avec références, changements à comparer et critères
de validation. La note distingue coûts cumulés dangereux, constante liée
à K, sérialisation et résidence, sans confondre ces constats avec la borne
intrinsèque de sortie FULL. P0 reste premier ; aucun moteur ni benchmark
n'est introduit par cette formalisation documentaire.
