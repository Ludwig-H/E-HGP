# Contrelecture GPU : compactage, stockage et mesures — 4 octobre 2026

Source produit/banc figée `22a6af6aa6c57302b3eac5adcb7f2b52c0f1e0b4`, après les ports b74/16/4ec. Les deux [paquets précédents](../audit_gpu_update_20261004/README.md) restent clos et intacts. Aucun build, natif, CUDA, GCP, fit ou donnée KITTI dans cet audit.

**Contrelecture favorable en source**, sans nouveau défaut géométrique établi :

- [Format compact](compact/README.md) : huit octets par record, rangs locaux sur un octet ; supports, arité, contacts I/U et Level brut préservés. 3 404 contrôles stdlib/Fraction.
- [Stockage et réduction CUDA](cuda/README.md) : copie des feuilles intégralement stockées, rejeu entier des débordantes, abandon des non résolues puis repli CPU ; domaines disjoints et ledger ajouté une fois. 18 426 contrôles scalaires, dont 187 flux et 495 cas de réduction warp.
- [Copies parallèles et première écriture des pages](gather/README.md) : places par ordinal, préfixes distincts, Pool joint et suffixe du repli conservé. 271 contrôles de transport synthétique.
- [Banc corrigé](bench/README.md) : reps0/P1 et lignes de passes manquantes ou invalides désormais refusés ; 240 contrôles AST simulés. Identité limitée au dernier dump de chaque processus. La parenthèse du champ scope doit dire passes1..P−1 plutôt que2..P.

**Deux conseils concrets.** La porte `full_leaf_lanes` ne prouve pas une copie nonvide avec la seule inégalité0<fill_jobs<jobs : compter `stored_nonempty` ou `records_copied` et exiger>0. Aucun manque réel de la fixture n'est établi. Les réservations logiques des payloads CUDA, rendues à l'enqueue du free, ne mesurent pas les pages conservées par le pool ; `device_bytes` est un cumul d'allocations. Pour mesurer la mémoire physique, garder séparés UsedMem/ReservedMem et leurs pics, comme dans la [documentation CUDA](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/stream-ordered-memory-allocation.html).

**Goulot mesuré sur la version précédente.** [Instantané des métadonnées GPU3–5](gpu_receipt_triage/README.md) : source16 pour GPU5, profils/paramètres et limites dans le lecteur. À K5/W48, le GPU y reste plus lent ; à K10/leaf24, les médianes chaudes baissent de2–3,5%, avec une forêt restant à1,17–1,63s. Priorité : mesurer le coût FULL du format compact actuel, puis traiter la forêt K10. Réduire le seul count des feuilles ne ferme pas le contrat. Ce sont des diagnostics, pas des gains statistiques ni une qualification22 ; les archives/binaires n'ont pas été rejugés. Les reçus déclarent des arrêts ciblés historiques, sans inspection de l'état actuel des VM.

Les cinq capsules sont copiées sans changement, inventaires et sources inclus. Les **22 341 contrôles** se recoupent et ne sont pas autant de portes natives indépendantes. Le lecteur des 26 JSON de métadonnées est distinct de ces modèles. Le script racine vérifie toutes les fermetures et rejoue chaque capsule en Python standard normal et optimisé.

```sh
python3 -B -S check.py
python3 -B -O -S check.py
```
