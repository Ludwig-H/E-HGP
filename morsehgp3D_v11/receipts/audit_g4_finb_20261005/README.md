# Relecture des portes courtes sanitizer finb — 5 octobre 2026

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
ASan/UBSan s’exécute en u24, TSan en u21.

Session `v11.20261005.claudefinb`, source publiée
`38b76701b9b0198fc1c37afe16e1480e638e513c`. DONE=0,
`completed`, worker=0 ; arrêt ciblé certifié. Reçu, plan, paquet et archive
vérifiés par SHA. Les 635 fichiers utiles livrés sont exactement identiques
à la source figée. Les données sont vérifiées côté worker, sans recopie ici.

| Configuration | Sélection | PASS | Failed | Sans résultat |
|---|---:|---:|---:|---:|
| gcc_asan_ubsan / u24 | 783 | 783 | 0 | 0 |
| gcc_tsan / u21 | 783 | 783 | 0 | 0 |

Les deux inventaires sont uniques, identiques par noms, sans porte
désactivée ; chacun possède ses 783 verdicts CTest terminés PASS. Les JUnit
concordent. Aucun arrêt de budget. Le résumé conserve la liste commune des
783 noms et les empreintes des inventaires, résultats et journaux.

PASS explicites dans les deux profils : les huit `head_unit_*`, dont
`head_unit_huge`, `points_unit_sort_refusal`, `num_roots` normal/−O et
`cli_plat` normal/−O. Huge et sort_refusal sont des portes unitaires natives
uniques ; aucune jumelle Python −O n’est revendiquée pour ces deux groupes.

Aucun marqueur ASan/LSan, TSan ou erreur runtime UBSan n’est trouvé dans
`ctest.log`, `LastTest.log` et `junit.xml` de chaque configuration, avec le
motif exact conservé dans le résumé. Ce contrôle reste limité à ces journaux.

La [contrelecture native ciblée](native_review.json) confirme les flags de
compilation et de lien de la bibliothèque, des unitaires head/points et de
la sonde roots : ASan+UBSan sans récupération en u24, TSan en u21. Elle
conserve les sorties unitaires minimales des portes ciblées, leurs
empreintes et un scan de dix journaux runtime sans diagnostic sanitizer.
Aucune nouvelle compilation ni exécution native par les auditeurs.

Portée exclue par configuration : labels `mutant`, `long`, `scale8000`,
`scale16000`, `scale32000`, `lidar`, ainsi que les noms `^mhgp11_reference_`.
Les lots S d’échelle/LiDAR, les différentiels Python longs et les campagnes
de mutants gardent leurs reçus distincts. Cette capture ferme la portée
courte de B au pin 38b76701b ; elle ne ferme pas la qualification globale,
les échecs de supports_route dans S ni un contrat de temps de tour/GPU.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session` et `--repo` permettent de déplacer les dépendances locales.
Le script contrôle fermeture, hashes, identité des sources utiles,
inventaires et tous les verdicts, puis compare le résultat au résumé figé.
Aucun build, test natif, réseau ou accès cloud. Dépendances : archives
locales de finb et commit Git ; cette capsule n’est pas un reçu autonome.

Aucun journal brut complet, sortie binaire native, identité de compte ou
octet LiDAR copié. Les journaux restent locaux ; leurs empreintes et les
seuls verdicts unitaires ciblés sont publiés.
