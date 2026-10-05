# Relecture R1 — portes courtes ordinaires, 5 octobre 2026

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Les profils u18 et u24 sont explicitement distincts.

Session `v11.20261005.clauderepriser1`, source publiée
`98a00955083d483306c4f92b9031e382e81b0e59`. DONE=0,
`completed`, worker=0 ; arrêt ciblé certifié. Reçu, plan, paquet et archive
vérifiés par SHA. Les 635 fichiers utiles du paquet sont exactement
identiques à Git au pin. Les données ont été vérifiées côté worker.

| Configuration | Profil | Sélection | PASS | Failed | Sans résultat |
|---|---|---:|---:|---:|---:|
| gcc_release | u18 | 873 | 873 | 0 | 0 |
| bits21 | u21 | 783 | 783 | 0 | 0 |
| bits24 | u24 | 783 | 783 | 0 | 0 |
| poison | u21 + poison | 784 | 784 | 0 | 0 |

Chaque inventaire est unique, sans porte désactivée, et possède tous ses
verdicts CTest terminés PASS. Les résultats, journaux et JUnit concordent ;
aucune coupure de budget. Les 783 noms communs sont identiques entre bits21
et bits24. gcc_release ajoute 90 références Python courtes ; poison ajoute
la seule porte `mhgp11_core_poison`. Empreintes et ensembles sont vérifiés
au rejeu, sans recopier les journaux.

PASS explicites dans les quatre profils : les huit `head_unit_*`, dont
`head_unit_huge`, `points_unit_sort_refusal`, `num_roots` normal/−O,
`cli_plat` normal/−O, `cli_contract` normal/−O, `api_publish_reader`
normal/−O, toutes les portes `api_session_*`, dont provenance, et
`api_supports_route_oracle` normal/−O. Huge et sort_refusal sont des portes
unitaires natives uniques ; aucune jumelle Python −O n’est revendiquée.

La sélection courte fonctionne au pin corrigé : le lot gcc_release
ne demande plus de label LiDAR que son filtre exclut. Ce constat natif
de R1 complète la preuve bornée du correctif de matrice.

Labels sélectionnés : `fast`, `oracle`, `unit`. Labels exclus : `mutant`,
`long`, `scale8000`, `scale16000`, `scale32000`, `lidar`. Hors gcc_release,
les noms `^mhgp11_reference_` sont aussi exclus. Cette capture ferme
uniquement R1 court ordinaire au pin 98a009550. Les 41 occurrences ordinaires
interrompues de fina2, les six portes API supports_route d’échelle/LiDAR
par profil et les campagnes mutants API/CLI sont hors de R1 ; leurs reprises
R2/R3/R4 gardent des reçus distincts. Aucun transfert sanitizer ni contrat
de temps de tour/GPU, aucune qualification globale.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session` et `--repo` permettent de déplacer les dépendances locales.
Le script contrôle fermeture, hashes, identité des sources utiles,
inventaires, profils et tous les verdicts, puis compare le résultat au
résumé figé. Aucun build, test natif, réseau ou accès cloud. Dépendances :
archives locales de R1 et commit Git ; cette capsule n’est pas un reçu
autonome. Aucun journal brut, sortie binaire native, identité de compte
ou octet LiDAR copié.
