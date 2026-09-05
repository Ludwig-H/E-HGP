# Préparation q2 E — qualification locale bornée du 5 septembre 2026

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce répertoire scelle l'overlay relu avant port. Le `candidate.patch` vise seulement quatre fichiers produit : `silent_incidence.hpp`, registre des mutants, `meb_lazy_gate.cpp` et une nouvelle porte CMake. Les autres sources sont des dépendances privées de compilation ; ce répertoire n'est pas une distribution CMake complète. Aucun moteur de tour ni GCP n'a été lancé ici. Le port est coordonné séparément par root.

## Delta et portée

Le prédicat q2 est le produit scalaire i64 `(z-a)·(z-b)`, exactement égal à la puissance de la clé primitive q2. Chaque différence a un module au plus 65535, chaque produit au plus 4294836225 et chaque somme partielle au plus 12884508675 : tous les intermédiaires tiennent dans i64. La comparaison reste strictement `>0`, les zéros passent. La charge des caps, l'ordre des supports, les représentations littérales de clé/niveau et le contrôle final de coquille restent inchangés.

Une seule clôture matérialise la clé et le niveau q2 après acceptation du prétest ; le `accept` original reste appelé. Compteurs q2 et branche eager sont sous `MHGP7_TESTING`. Un nouveau mutant remplace `>0` par `>=0`, et le mutant eager historique exerce maintenant q2, q3 et q4. Aucun champ public ni structure globale supplémentaire ; les deux prétests ajoutés aux sites d'une paire retenue peuvent coûter plus cher. Aucun gain universel ni SLO n'est inféré.

## Qualification exécutée

- `test_runs.json` : compilation O3 stricte puis huit argv directs, codes attendus 0/4/4/4/4/2/2/2. Il ne s'agit pas d'une exécution CTest.
- `sanitized_runs.json` : huit essais sandbox échoués, tous conservés. LeakSanitizer refuse ptrace ; aucun de ces essais ne vaut succès.
- `sanitized_unsandboxed_runs.json` : après autorisation explicite d'escalade, même binaire ASan/UBSan, `detect_leaks=1` conservé, huit argv aux codes attendus, aucun diagnostic sanitizer.
- `production_runs.json` : nouveau différentiel D/E compilé séparément sans `MHGP7_TESTING`, 11 816 comparaisons et 675 164 identités de puissance. Le header E est adapté uniquement par namespace/import dans `variant.hpp` ; la référence D est le snapshot historique explicitement inclus dans `build/v7_meb_q2_review/source_D.hpp`.

La porte permanente compare 11 816 MEB : 668 succès, 12 refus de dégénérescence et 11 136 refus cap, sur 170 scènes, tous les caps jusqu'au coût plus un et l'ordre inversé. Les sorties sont initialisées par une sentinelle non nulle pour observer les refus. Elle vérifie tous les champs de clé, le niveau littéral, le support ordonné, le bool/statut/raison et les 13 statistiques publiques. Les conteneurs d'événements vides ne prouvent que le contrat local de `miniball`, pas une transaction de complétion entière.

La non-vacuité q2 comprend 514 identités indépendantes sur coins/extrêmes, une valeur positive au-delà de i32, une valeur négative au-delà de i32, 96 zéros étrangers et deux rejets de profil avant prédicat. Le nominal observe 421 459 candidats q2 rejetés, 224 matérialisations et zéro matérialisation rejetée. Le mutant eager conserve tous les objets mais matérialise ces 421 459 rejets ; ce sont des comptes logiques, pas des cycles ni une distribution pipeline. Le mutant q2-shell diverge causalement sur une paire minimale dont D réussit avec q=2, code 4.

## Provenance et limites des copies

`preintegration.txt` épingle les quatre sources D, HEAD et les binaires protégés C/D. `overlay_sources.sha256` épingle les sources effectivement compilées. Parmi les 15 dépendances privées, 11 copies ont uniquement leurs lignes vides finales retirées par la préparation, inventoriées dans `judgement.json`; aucune transformation sémantique ni modification produit correspondante. Les quatre fichiers destinés au port ne comportent que le delta de `candidate.patch`. La compilation du différentiel production emploie, elle, les dépendances D originales.

Le sceau historique `build/v7_meb_q2_review/SHA256SUMS` et ses dépendances ont été vérifiés sans réécriture. Son sous-dossier `microprobe` conserve source, binaire, dépendances et pins avant/après identiques, mais **aucun stdout/stderr, code de sortie ni reçu de commande exécutée**. Aucun résultat temporel historique n'en est publiable ; il n'est pas reconstruit. Aucun nouveau microchronométrage n'a été lancé ici.

Le GO obtenu porte sur ce changement local q2 et sur son intégration pour qualification complète ultérieure, pas sur l'exactitude globale, la tour entière, le contrat 50k/1 seconde, la cible 100 ms ou les dizaines de millions.
