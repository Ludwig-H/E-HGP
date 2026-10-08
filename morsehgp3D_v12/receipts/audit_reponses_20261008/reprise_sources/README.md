# Reprise de trois sources A depuis le secours — 8 octobre 2026

`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. Récupération proposée de travail du développeur,
aucune modification du produit ni du scratch par l'auditeur. Le worktree d'audit est détaché sur `8da450ab7` ; le
prototype A restauré annonce la base `8dc5d6b16`. Ce patch cible **la racine de A/work**, contenant
`morsehgp3D_v12/`, et non directement ces commits : les fichiers pipeline sont ceux du prototype non livré.

Le secours `sources_t2d.tar.gz` daté par son manifeste du 8 octobre à 04:08:29 UTC est intact : **1 199 fichiers**
(A 402, B 396, C 401), tailles et SHA-256 tous conformes, sans doublon, entrée manquante/supplémentaire ni lien.
Archive de 3 471 291 octets, SHA `bedaf467e2d62736820510c83653293da43f6d5af997d27bd02f460cc635e517`.
Le [manifeste de vérification](pins.json) épingle aussi le manifeste externe et les trois états avant/après.
Cette sauvegarde ciblée ne reconstitue pas les qualifications : builds, données, journaux et la plupart des JSON
(notamment manifestes mutants) n'y sont pas conservés.

À 04:33:41 UTC, la copie rétablie était plus ancienne sur deux fichiers et dépourvue du troisième. Le
[patch exact](reprise_a.patch) restitue seulement leurs octets archivés :

| Source relative à `morsehgp3D_v12/` | Copie rétablie | Archive à restituer |
| --- | --- | --- |
| `src/tower/pipeline.cpp` | `6291a100374d…` | `44e83374113a…` |
| `src/tower/pipeline.hpp` | `bf379d8d8a8e…` | `6b279ea85988…` |
| `tests/tower/pipeline_fault.cpp` | absent | `c92e6dbeb1df…` |

Les deux premiers retirent `noexcept` de `open_session`, permettant aux allocations de propager `bad_alloc`
jusqu'au `guarded` public ; c'est la réponse au
[risque déjà signalé](../prelecture_t2d_corps/README.md). Le troisième est un **test en préparation**, conservé
sans retouche : injection des trois allocations levantes annoncées et balayage des allocations sans exception.
Il n'est pas enregistré dans `tests/tower/tests.cmake`, identique entre les deux copies
(`af1ba26245fa…`). Ni compilation ni exécution ni validité complète de ce test ne sont attestées ici.

Vérification effectuée sur une copie temporaire isolée : `git apply --check`, application, comparaison des trois
sorties octet pour octet à l'archive, puis `git apply --reverse --check`. Les trois sources vivantes, l'archive
et son manifeste sont restés identiques avant/après. Aucun build, moteur, GPU ou GCP exécuté.

Pour reprendre, comparer d'abord les empreintes complètes dans `pins.json`, puis lancer depuis une copie isolée
de A/work `git apply --check <chemin-vers-reprise_a.patch>`. L'enregistrement et la qualification du nouveau test,
puis le report vers la base produit actuelle, restent au développeur. Aucun résultat ancien n'est transféré par
la seule récupération des sources.
