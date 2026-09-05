# Qualification F/pile témoins intégrée

Trois qualifications fermées sont publiées : **48/48 Release ciblées, 48/48 ASAN/UBSAN et 339/339 Release complètes**. Les sorties sont relues, pas les moteurs réexécutés. `public_status=not_claimed`, `backend=cpu_reference`, `profile=quantized_u16_input_only`. GCP non utilisé par cet export.

Les répertoires `release_receipts`, `sanitized_receipts` et `full_receipts` conservent les résultats séparément. Les sources, binaires, liaisons de compilation, inventaires, JUnit et journaux LastTest/fences sont vérifiés avant copie puis à la frontière finale. F Release est lié au SHA ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85; le CLI instrumenté a son propre SHA. Les baselines historiques C/C/D/E ne deviennent jamais F. Le build complet est incrémental et les exécutions des portes sont fraîches. Les onze commandes de compilation ciblées sont contrôlées ; la liaison full porte sur le CLI, pas sur les flags de toutes les cibles.

`protocol/` conserve la préparation F, son diff depuis E et ses selftests bruts. Les trois révisions E, y compris les faux rejets UTC/kernel, restent conservées dans le reçu historique `meb_q2_integrated_20260905`, référencé par manifeste et sommes épinglés. Ces tests de protocole ne sont pas des qualifications moteur. Les snapshots `.py.txt` et `.md.txt` restent inertes et leurs chemins sources sont historiques ; les manifestes `SOURCE_*` ne désignent pas les noms de cette projection. `dependencies.json` référence les autorités historiques par SHA, sans réutilisation de leurs résultats.

`provenance.json` donne chaque correspondance source/public, SHA et taille. Les copies sont byte-exactes, sans LF ajouté. Un grand LastTest est compressé en gzip déterministe ; le SHA et la taille décompressés égalent exactement le brut conservé. Aucun journal n'est tronqué ni publié sous une extension `.log` ignorée. `SHA256SUMS` couvre tout le contenu livré et le manifeste, hors lui-même.

Ces portes ne démontrent ni exactitude globale industrielle, ni SLO 50k/1 s/100 ms, ni passage massif/GPU.
