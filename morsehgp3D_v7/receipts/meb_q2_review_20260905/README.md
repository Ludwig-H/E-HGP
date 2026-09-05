# Revue q2 E — reçu local du 5 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce reçu qualifie un overlay **avant port**. Il ne qualifie pas encore un build intégré ni une tour entière. La [note mathématique](../../docs/OPTIMISATION_MEB_Q2.md) explique le prétest q2 et ses limites ; le [patch proposé](candidate.patch) vise seulement le header MEB, le registre des mutants, le test MEB existant et une nouvelle porte CMake.

## Résultats conservés

| Exécution | Résultat | Bruts |
| --- | --- | --- |
| O3 strict, `MHGP7_TESTING` | Huit argv directs conformes, codes 0/4/4/4/4/2/2/2 | [Build](build_runs.json), [runs](test_runs.json) |
| ASan/UBSan dans le sandbox | Huit échecs LeakSanitizer sous ptrace, code 1 ; aucun succès inféré | [Build](sanitized_build.json), [échecs](sanitized_runs.json) |
| Même binaire ASan/UBSan hors sandbox, après autorisation explicite | Huit argv conformes, mêmes codes attendus, `detect_leaks=1` conservé ; aucun diagnostic sanitizer dans le flux capturé | [Runs](sanitized_unsandboxed_runs.json) |
| Différentiel D/E compilé sans `MHGP7_TESTING` | 11 816 comparaisons, 675 164 identités de puissance | [Build et run](production_runs.json) |

Ces argv ne sont pas huit CTests. Les JSON préservent commandes, codes et **sortie combinée stdout/stderr** telle que rendue par l'outil ; aucun flux séparé n'a été capturé. Les durées de l'outil sont celles du juge complet, pas des mesures comparatives D/E.

Le [test permanent proposé](morsehgp3D_v7/tests/meb_lazy_gate.cpp) compare 170 scènes : 668 succès, 12 dégénérescences et 11 136 refus cap, tous les caps jusqu'au coût plus un et ordre inversé. Sentinelles non nulles, clé, niveau littéral, support ordonné, statut/raison et 13 statistiques sont comparés. Le q2 nominal compte 421 459 rejets, 224 matérialisations et zéro matérialisation rejetée. Le mutant eager conserve les objets mais réintroduit ces matérialisations ; le mutant shell q2 est tué causalement sur une paire minimale réussie par la référence. Ces comptes sont logiques, sans traduction en cycles.

## Provenance et reproduction

[manifest.json](manifest.json) décrit **39 copies octet pour octet**, sans projection de fin de ligne. Aucun binaire n'est distribué. Le `SHA256SUMS` de ce reçu couvre tous ses artefacts ; [overlay.historical.SHA256SUMS](overlay.historical.SHA256SUMS) conserve séparément le sceau de travail original, avec ses chemins et binaires historiques.

Le [README de l'overlay](overlay.README.historical.md), le [jugement](judgement.json), les [pins avant port](preintegration.txt) et les [sources réellement compilées](overlay_sources.sha256) sont conservés. Les 15 dépendances privées de la porte sont incluses : onze ont uniquement leurs lignes vides finales retirées lors de la préparation de l'overlay, adaptation explicitement listée dans le jugement. La publication copie exactement cet état compilé, pas les sources originales avant cette adaptation non sémantique.

Le [différentiel production](meb_q2_production_gate.cpp) emploie une [référence D historique](source_D.historical.hpp) et le [header E adapté par namespace](variant.hpp). Ses dépendances D ordinaires restent celles de HEAD `e6d33698e62ebecf74dff01c16d7de17149d7a4e` ; la compilation de la porte TESTING utilise les copies privées incluses. Les commandes, includes et fichiers `.d` gardent **leurs emplacements historiques sous build/** : ils ne sont pas silencieusement corrigés ni annoncés exécutables depuis ce reçu. Rejouer implique de restaurer ces chemins et les dépendances épinglées. Le CMake inclus est une source du patch, pas une configuration complète exécutable dans cette archive.

L'ancien microprobe conserve source, binaire, dépendances et pins avant/après identiques, mais aucun brut de run, code de sortie ni reçu de commande exécutée. Son inventaire est dans le manifeste : **aucun temps historique n'en est attribuable**. Aucun nouveau microchronométrage n'est effectué par ce reçu.

L'équivalence locale ne démontre pas l'exactitude globale HGP. Les obligations intégrées, la tour K=1..10, le repli K=1..5, 50k/1 seconde, 100 ms et plusieurs dizaines de millions restent séparées. GCP non utilisé pour cette revue.
