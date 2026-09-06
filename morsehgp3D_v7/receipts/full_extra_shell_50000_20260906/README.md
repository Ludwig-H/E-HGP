# Diagnostic extra-shell local 50k — captures closes

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [run r3](run_r3/receipt.json) contient 33 contrôles O2 et SAN, une fixture carrée identique entre ces modes, les codes 0/2 attendus, puis deux micros de huit ordres FULL aux mêmes digests avec et sans diagnostic. Le flux de la sonde reste `mhgp7-full-gabriel-probe-v6` ; seul l'opt-in ajoute les métadonnées et les traces `mhgp7-extra-shell-diagnostic-v1`.

Le [cas local n=50000, s=8, Kmax=10](run_r3/n50000_k10.stdout), avec huit workers de pipeline et construction FULL séquentielle, reste refusé code 2 : `probe_rank_relevant_extra_shell`, avant tout ordre FULL. Ses [quatre traces](run_r3/n50000_k10.stderr) sont quatre enregistrements BallData, pas nécessairement quatre plateaux distincts. Le temps GNU time est celui d'un diagnostic jusqu'au refus, pas une mesure d'une tour réussie. Aucun contrat 50k, résultat GPU ou traitement des plateaux n'est acquis. GCP non utilisé pour cette capture locale.

Les échecs sont conservés : [r1](run_r1/receipt.json), erreur de compilation `-Werror=ignored-attributes`, et [r2](run_r2/receipt.json), erreur LeakSanitizer sous ptrace, même si ses contrôles avaient écrit `passed` avant cette erreur. Le gate historique r1 et la version historique de son recorder ne sont pas disponibles : leurs octets ne sont pas reconstruits ni réattribués au recorder courant. Le recorder archivé décrit r3 ; les journaux et pins réellement capturés restent l'autorité de chaque tentative.

Les [deux CTests](ctest_r1/junit.xml) passent. Le [reçu initial](ctest_r1/receipt.json) reste `failed` à cause d'une erreur de lecture du depfile après les tests ; la [clôture de métadonnées](ctest_r1/metadata_close/receipt.json) vérifie les mêmes captures sans compilation ni exécution moteur supplémentaires. Les 338 dépendances système n'ont été hachées qu'après compilation ; seule la stabilité avant/après des 20 sources projet/contrôles est établie dans cette clôture.

[publication.json](publication.json) référence les 43 dépendances réelles du probe. Seuls le nouveau diagnostic, le probe modifié, le gate et CMake sont copiés ; les autres sources pointent vers leurs octets déjà scellés dans les paquets voisins. Les ELF sont exclus, leurs empreintes conservées. Les intentions redondantes sont omises quand le journal de commande complet existe ; les chemins absolus de ces journaux sont historiques, jamais exécutés par le lecteur.

Depuis ce dossier (avec les paquets frères de sources), rejeu hors ligne :

```bash
python3 -B verify.py
python3 -B -O verify.py
sha256sum -c BASE_SHA256SUMS
```

Le lecteur vérifie transport, empreintes, retours et portée des résultats ; il ne requalifie ni les ELF ni la géométrie. Le supplément `reader/`, publié séparément, porte la lecture rationnelle indépendante des traces. `BASE_SHA256SUMS` exclut ce supplément ; le manifeste global est fermé par ROOT après son ajout.
