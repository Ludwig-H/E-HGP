# Relecture R4 — reprise ASan/UBSan u24

Campagne du 5 octobre 2026, close et relue le 6 octobre. Cadre :
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Exécution explicitement ASan/UBSan u24.

Session `v11.20261005.clauderepriser4`, source publiée
`98a00955083d483306c4f92b9031e382e81b0e59`. DONE=0,
`completed`, worker=0 ; arrêt ciblé certifié. Reçu, plan, paquet et archive
vérifiés par SHA. Les 635 fichiers utiles du paquet sont exactement
identiques à Git au pin. Les données ont été vérifiées côté worker.

| Lot ASan/UBSan u24 | Sélection | PASS | Failed | Sans résultat |
|---|---:|---:|---:|---:|
| gcc_asan_scale32000 | 11 | 11 | 0 | 0 |
| gcc_asan_lidar_ng01_ng02 | 24 | 24 | 0 | 0 |
| gcc_asan_scale16000 | 10 | 10 | 0 | 0 |
| gcc_asan_scale_rest | 35 | 35 | 0 | 0 |
| Total | 80 | 80 | 0 | 0 |

Les quatre inventaires sont uniques et sans porte désactivée ; les lots
sont disjoints. Leur union reproduit exactement les 80 noms ASan de
`claudefins`, sans ajout ni manque. Les 80 verdicts CTest sont terminés
PASS et les JUnit concordent. Aucun arrêt de budget.

Les **six API supports_route auparavant rouges sous ASan/u24** ont un
PASS explicite au pin corrigé : synthétiques 8k/16k/32k et trames
ng00/ng01/ng02 à K5. Le résumé conserve leurs verdicts nominatifs et les
empreintes des anciennes erreurs de fins ; aucune extrapolation depuis
les PASS ordinaires R2. Les six portes S9 points, les six S10 plat et
les six num_roots_cost sont aussi directement PASS.

Ce reçu ferme la portée normale d’échelle/LiDAR ASan/UBSan u24. Les
journaux et flags de compilation/lien sont relus dans un complément
natif distinct qui peut être joint ; la présente capsule établit
inventaires, identités et verdicts.

Labels `mutant` et `long`, références `^mhgp11_reference_` et jumelles
`_opt` sont exclus de R4. Les portes courtes huge/sort_refusal/num_roots
et cli_plat gardent le reçu finb ; les différentiels Python, les portes
longues, les profils TSan et les mutants gardent leurs propres reçus.
Aucun transfert entre profils, contrat de temps de tour/GPU ni
qualification globale n’est revendiqué par cette seule capsule.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session`, `--baseline-session` et `--repo` permettent de déplacer les
dépendances locales. Le script contrôle fermeture, hashes, sources
utiles, options u24/sanitizer, inventaires et tous les verdicts. Il relit
les métadonnées de fins pour vérifier l’union exacte des 80 noms et les
six erreurs anciennes, puis compare le résultat au résumé figé. Aucun
build, test natif, réseau ou accès cloud. Dépendances : archives locales
R4/fins et commits Git ; cette capsule n’est pas un reçu autonome.

Aucun journal brut, sortie binaire native, identité de compte ou octet
LiDAR copié. Les journaux restent locaux ; seules leurs empreintes sont
conservées dans la capsule.
