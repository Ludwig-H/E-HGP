# A2 — sélection G4 complète, qualification totale encore ouverte

La session `v11.20261005.claudequala2` a exécuté le commit
`b319efc8477fec234afc0b31e86f8a43e3023641`, du 5 octobre 2026 à
14:45:16 UTC jusqu'à 15:18:25 UTC. `DONE=0`, reçu `completed`, commande et worker
code 0. La fermeture ciblée est certifiée, la génération de démarrage concorde
avec celle de clôture, la cible est TERMINATED, clé retirée et réservation
libérée. Les SHA des archive, paquet, reçu et plan ont été relus localement.

| Profil | Passed / sélectionnés | Échecs / sans résultat |
| --- | ---: | ---: |
| GCC Release u18 | 914 / 914 | 0 / 0 |
| GCC Release u21 | 824 / 824 | 0 / 0 |
| GCC Release u24 | 824 / 824 | 0 / 0 |
| Poison u21 | 825 / 825 | 0 / 0 |

Le résumé officiel dit `complete=true`, `conforming=true`. Ces verdicts portent
sur la sélection exécutée : `-LE ^(mutant|long)$` dans les quatre profils, puis
`-E ^mhgp11_reference_` dans u21/u24/poison. Les mutants, ASan/UBSan, TSan et les
portes `long` ne sont pas exécutés dans A2.

Chaque profil a 53 résultats individuels Passed pour les 27 portes `api_*`,
24 `io_*` et `cli_contract` normal/`-O`. Les deux portes oracle supports et les
sphères5/9 ont Passed. Les dix portes CLI sans résultat dans A et retenues ici
ont Passed : déterminisme scale8000 `-O`, identité LiDAR K5 ng00/ng01/ng02 `-O`,
supports scale8k/16k/32k et LiDAR ng00/ng01/ng02 `-O`.
Les deux identités FULL K10 (scale32000 et ng00) sont **absentes de la
sélection**, leur label `long` étant exclu. Noms/verdicts exacts dans
`summary.json`, compteurs et inventaires rapprochés des lignes CTest terminées.

Les portes supports scale/LiDAR de cette source utilisent `--fils=1,4`
(`tests/cli/tests.cmake`) : elles ne closent pas les contrôles supports W48
ng00/ng02 du plan de mesure séparé. Le déterminisme FULL scale8000 utilise déjà
W48 ; ce succès ne transfère pas au chemin supports LiDAR.

La session B ne présentait ni `DONE` ni reçu final à la capture ; elle est
exclue. Main observé `53c027fe848b0d890f164eb87ebf347338c58d55` contient S8,
qui n'est pas qualifié par cette exécution antérieure. Aucun chrono de contrat
de tour n'est acquis par cette matrice.

`manifest.json` ferme les chemins/SHA des preuves locales, conserve les seuls
champs sûrs des reçus/résumés et le plan A2 exact consommé. Aucun nom de compte,
clé, log brut ou nuage n'est recopié. `SHA256SUMS` ferme les quatre fichiers.

```sh
python3 /chemin/de/cette/capsule/replay.py
python3 -O /chemin/de/cette/capsule/replay.py
```

Rejeu stdlib en lecture seule, sans réseau, build ni appel natif ; il dépend
des archive, reçu, paquet, plan et DONE locaux sous
`/workspaces/.ehgp-sessions/v11.20261005.claudequala2/`. Cette capsule ne
constitue pas un reçu autonome. L'auditeur n'a utilisé ni GCP ni tests natifs.
