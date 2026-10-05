# B — sanitizers G4 partiels, 5 octobre 2026

La session `v11.20261005.claudequalb` a exécuté
`b319efc8477fec234afc0b31e86f8a43e3023641`, de 15:23:34 à 15:58:35 UTC.
Le reçu final dit `failed_remote`, worker code 1 ; le marqueur du contrôleur
est `DONE=3`. Les deux CTests ont été coupés au budget global de 2100 secondes.
Ce sont des résultats incomplets, aucun échec individuel n'est consigné.

| Configuration | Passed / sélectionnés | Échecs / sans résultat |
| --- | ---: | ---: |
| GCC ASan/UBSan u24 | 774 / 824 | 0 / 50 |
| GCC TSan u21 | 759 / 824 | 0 / 65 |

Le résumé est finalisé (`complete=true`), mais non conforme
(`conforming=false`). Les deux profils excluent `^(mutant|long)$` et
`^mhgp11_reference_`. Les noms des 50/65 portes sans résultat figurent dans
`summary.json`. En particulier, les dix portes CLI lourdes sans résultat
dans A, ensuite passées par A2 en Release/poison, n'ont pas de résultat dans B.
Les identités FULL K10 scale32000/ng00 restent hors sélection.

Dans les deux profils, les 27 portes `api_*`, 24 `io_*`, `cli_contract`
normal/`-O`, oracle supports normal/`-O` et sphères5/9 ont des verdicts Passed
explicites. Le rejeu les extrait des lignes CTest terminées et rapproche les
comptages de l'inventaire et du résultat officiel. Sous sanitizer, les
préchargements du contrat CLI sont omis par `tests/cli/tests.cmake:21` : ce
succès ne requalifie pas le crochet I/O variadique.

La fermeture ciblée est certifiée : cible TERMINATED, générations et nom de
cible concordants, clé supprimée, réservation libérée. Empreintes de
l'archive, du paquet, du plan et du reçu vérifiées localement ; pas de membre
de résultat ignoré ni flux tronqué. Archive SHA-256 :
`8faaacf3c58cf1b6ebf5ff96e2d454de5b4c684f881fb28ce7015a6bdafae3c7`.

La session `v11.20261005.claudequall` restait ouverte, sans DONE ni reçu final
à cette capture ; elle est exclue. Main observé `ac5fc58d5` n'est pas la
source exécutée : aucun transfert aux changements ultérieurs, dont S8.
Aucun chrono de contrat de tour ni qualification sanitizer complète acquis.

`manifest.json` conserve les champs sûrs des reçus/résumés, les SHA des
preuves locales et le plan B exact consommé. Ni compte, clé, log brut ni
nuage recopié. `SHA256SUMS` ferme les quatre fichiers.

```sh
python3 /chemin/de/cette/capsule/replay.py
python3 -O /chemin/de/cette/capsule/replay.py
```

Rejeu stdlib en lecture seule, sans réseau, build ni appel natif. Il dépend
des archive, reçu, paquet, plan et DONE locaux sous
`/workspaces/.ehgp-sessions/v11.20261005.claudequalb/` : cette contre-lecture
ne constitue pas un reçu autonome. Les capsules antérieures sont inchangées.
