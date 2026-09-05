# Revue indépendante de la MEB différée — archive du 4 septembre 2026

Archive préparée à **2026-09-04 23:58:09 UTC**, avant mesure C/D.
`public_status=not_claimed`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`.

La [note autonome](../../docs/OPTIMISATION_MEB_DIFFEREE.md) porte la preuve
locale. Cette archive conserve les octets de la contrelecture effectuée
dans `build/v7_meb_review`, pas un nouveau résultat obtenu depuis ce dossier.

## Résultat effectivement obtenu

La révision 2 proposée (header 5214a9a7, test 122807a3, patch d5f273e3) n'a
révélé aucun défaut certain d'équivalence locale à C. La
[contrelecture complète](review.before_integration.txt) expose préconditions,
signes/PGCD, bornes, supports, représentations de niveau et caps.
Elle ne constitue pas un oracle indépendant de l'exactitude globale de C.

Quatorze argv ont été [rejoués](replay.revision2.json) sur les deux binaires
overlay Release et ASAN/UBSAN épinglés : codes et préfixes exacts conformes,
stderr vide. Ce sont des replays, pas une reconstruction de ces cibles.

Le [différentiel de production archivé](production_differential.historical.cpp)
a été compilé séparément en C++20 O3 strict, sans MHGP7_TESTING.
[Commande, sorties et pins](production_result.json) :
11 805 comparaisons conformes sur 168 fixtures, 664 succès,
8 refus de dégénérescence et 11 133 refus cap ; q2/q3/q4 = 53/79/34.
Un garde de compilation interdit TESTING. Les zéros des champs de
reporting de coût ne sont pas des mesures : l'instrumentation est absente.

## Provenance historique explicite

Les fichiers nommés historical sont des copies octet pour octet. Leurs
includes, commandes et chemins sous build/ décrivent **l'emplacement de
l'exécution originale** : ils ne sont pas corrigés silencieusement vers
les sources actuelles et ne sont pas annoncés exécutables en place.
Une reproduction exige de restaurer le contexte et les dépendances épinglés.

[sources.before_integration.sha256](sources.before_integration.sha256)
décrit les 33 sources/dépendances au moment de la revue. Il contient donc
notamment l'ancien header produit C fddde6e2, alors que la proposition
distincte est 5214a9a7. Après le port autorisé, le header réel et le test
réel ont été comparés octet à octet à 5214a9a7/122807a3 avec code 0 ;
le registre ajoute 3 noms et le CMake 7 portes. Cette vérification de raccord
n'est pas une qualification intégrée ; celle-ci appartient au reçu de scale.

[review.historical.SHA256SUMS](review.historical.SHA256SUMS) est le sceau
local d'origine, avec ses chemins historiques. Le nouveau `SHA256SUMS`
scelle les fichiers **de cette archive** ; `manifest.json` explicite le port.
Aucun binaire n'est distribué. La [première révision](review.first_revision.txt)
et ses replays restent séparés, sans transfert vers la révision 2.

## Limites

Aucun temps des overlays n'est repris comme gain pipeline. Les compteurs
de matérialisations sont logiques, pas un chrono de code conservé par le
compilateur. Le contrôle local events.empty ne qualifie pas la publication
transactionnelle. Gamma/API/refus/archive, mesure C/D et qualification
complète intégrée gardent leurs obligations propres.

Aucune promotion exacte globale, aucun SLO 50k/massif, aucun résultat GPU.
GCP non utilisé par cette contrelecture.
