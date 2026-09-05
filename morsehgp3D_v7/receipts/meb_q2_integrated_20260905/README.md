# Qualification E/q2 intégrée

Trois qualifications fermées sont publiées : **33/33 Release ciblées, 33/33 ASAN/UBSAN et 324/324 Release complètes**. Les sorties sont relues, pas les moteurs réexécutés. `public_status=not_claimed`, `backend=cpu_reference`, `profile=quantized_u16_input_only`. GCP non utilisé par cet export.

Les répertoires `release_receipts`, `sanitized_receipts` et `full_receipts` conservent les résultats séparément. Les sources, binaires, liaisons de compilation, inventaires, JUnit et journaux LastTest/fences sont vérifiés avant copie puis à la frontière finale. E Release est lié au SHA df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6; le CLI instrumenté a son propre SHA. Les baselines historiques C/C/D ne deviennent jamais E. Le build complet est incrémental et les exécutions des portes sont fraîches.

`protocol/` conserve les trois révisions de préparation, y compris les faux rejets de précision UTC/kernel et leurs sorties brutes. Ces tests de protocole ne sont pas des qualifications moteur. Les snapshots `.py.txt` et `.md.txt` restent inertes et leurs chemins sources sont historiques ; les manifestes `SOURCE_*` ne désignent pas les noms de cette projection. `dependencies.json` référence les autorités historiques par SHA, sans réutilisation de leurs résultats.

`provenance.json` donne chaque correspondance source/public, SHA et taille. Les copies sont byte-exactes, sans LF ajouté. Un grand LastTest est compressé en gzip déterministe ; le SHA et la taille décompressés égalent exactement le brut conservé. Aucun journal n'est tronqué ni publié sous une extension `.log` ignorée. `SHA256SUMS` couvre tout le contenu livré et le manifeste, hors lui-même.

Ces portes ne démontrent ni exactitude globale industrielle, ni SLO 50k/1 s/100 ms, ni passage massif/GPU.
