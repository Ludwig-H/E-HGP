# Contrelecture B — correctif de réception `1f048aae`

23 septembre 2026. Relecture **du commit exact** `1f048aae`, sans utiliser
le WIP du développeur ni GCP. Le selftest du lecteur passe **43/43** en
Python normal et sous `-O` ; la matrice LiDAR archivée déclarée (six
campagnes, 60 cas) se revalide sans erreur. Le lien réciproque
`reason=chain_catalogue_euler_violated` ↔ `euler.status=fails` côté G4
et le confinement de `resolve_input()` au sous-arbre v8 sont réels.
La porte CMake des mutants exige maintenant la raison exacte, mais la
porte C++ `chain_euler_gate.cpp:60–61` renvoie encore immédiatement sur
un refus de chaîne sans vérifier elle-même `kInvariantViolated` ni
`euler_status==kFails`. La source produit les définit correctement à
`tower_chain.cpp:597–603` ; le résidu est une preuve mutante incomplète,
pas un défaut observé du chemin produit.

Deux trous de réception distincts restent reproductibles :

1. `run_lidar_scaling.py:515–535` ne passe **pas** `record['argv'][0]` par
   `resolve_input()` et ne le compare pas au chemin d'entrée reconstruit.
   Pour un morceau, les **deux derniers composants** du chemin suffisent ;
   pour un disque emboîté, seul le basename suffit. Dans une revalidation
   en mémoire, remplacer l'argument du cas `s00_k5_s8_w8_r0_piece_full`
   par `/tmp/evil/scene_00_grid/full.u32le` laisse réussir **60/60** cas
   et `failures=0` avec les options `--expect-campaigns`,
   `--expect-probe-sha256`, `--expect-commit`. Aucun fichier extérieur
   n'est lu pendant ce test : le défaut concerne la **fidélité de la
   commande archivée**, pas une preuve de géométrie fausse ni une fuite.
   Le bon `resolve_input()` aux lignes 142–152 n'est simplement pas appelé
   par ce chemin ; `expected_from_argv()` ignore aussi `argv[0]`.
2. Le lecteur LiDAR v13 appelle le lecteur G4 pour Euler, occupation et
   phases de la tour, mais **pas** pour toute sa validation de sonde
   (`run_lidar_scaling.py:197–209`, contre `tower_worker_v9.py:612–645`).
   Sur la fixture v13 synthétique du selftest, chacun des mutants
   `times_ms.prepare="bad"`, `tower_work.meb_accounting="bad"` et
   `generator.q34_expanded_pairs="bad"` reste accepté localement,
   alors que le lecteur G4 le refuse. Les nouvelles sections v13
   partagées sont bien contrôlées ; l'équivalence **intégrale** des deux
   schémas ne l'est pas. Cela affecte les preuves de comptes/temps, pas
   une omission de `BallKey` démontrée.

La matrice attendue est facultative en CLI. Sans `--expect-*`, un seul
groupe de dix cas retourne code 0 ; la docstring le dit explicitement :
ce mode prouve seulement la cohérence des campagnes **présentes**. Ce
n'est pas un défaut de la porte CTest, qui impose sa matrice, mais tout
reçu présenté comme campagne complète doit fournir ces attentes et
les conserver dans son manifeste.

Corrections proposées : lier l'argument archivé au chemin canonique de
chaque morceau (et à l'identité/provenance du disque emboîté) avant de
le juger, puis réutiliser le validateur G4 complet pour v13 ou prouver
et tester une équivalence champ par champ. Ajouter ces trois mutants au
selftest local et un test `--revalidate` avec `argv[0]` hors dépôt. Ne pas
confondre ce raccord documentaire avec une qualification FULL/GPU.
