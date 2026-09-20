# Compatibilité du chemin q4 shallow29 après ajout window30

## Reprise après correction du test worker

Autorité finale : [differential_3gm_bctv](differential_3gm_bctv/COMPLETION.json),
close PASS sur les189 sources après correction de la gate de reçus q2.
Même helper, mêmes deux builds/binaires, mêmes20 paires/40 commandes,
nouveau répertoire et pins de source. Tous les champs hors `timings`
restent identiques. Les quatre relectures normal/`-O`, historiques/live,
passent : [DIFFERENTIAL_REPRISE_READBACK.json](DIFFERENTIAL_REPRISE_READBACK.json).
Manifeste SHA256 :
`05872b5fb943e6fc48cfa755c45d1509da761a486641016c0284d670d5785f3f`.

La [correction et sa preuve](../preflight/WORKER_DIGEST_FIX.md) changent
seulement un test de l'inventaire189, pas les moteurs. La capture initiale
ci-dessous et ses lectures restent intactes, historiques sur l'ancien pin ;
ses anciennes lectures live ne qualifient pas le nouvel instantané.

## Capture initiale conservée

Capture [differential_i0b0kw1p](differential_i0b0kw1p/COMPLETION.json) close PASS :
20 paires, 40 commandes Release, tous les champs JSON identiques sauf `timings`.
Les quatre lectures normal/`-O`, historiques/avec `--check-live`, passent ;
leurs sorties exactes figurent dans [DIFFERENTIAL_READBACK.json](DIFFERENTIAL_READBACK.json).

La sonde **ancienne voie** `mhgp8_q4_shallow_probe` compare le build29
`build/v8_q4_shallow_20260920` au build30 `build/v8_q4_window_20260920`,
avec option32 identique. Matrice : far/cap, K5/10, n8000/16000/32000,
puis adversarial, K5/10, n32/64/128/256. Ce différentiel ne mesure pas le
gain de window30 : il vérifie que son ajout ne change pas le chemin29.

L'autorité antérieure est la capture
[scale__b3aquyb](../../q4_shallow_20260920/scale__b3aquyb/COMPLETION.json).
Ses manifeste/fermeture sont archivés dans la nouvelle capture et vérifiés.
Elle épingle bien l'ancienne sonde et son cache CMake, **pas son archive** :
les deux archives sont seulement contrôlées avant/après cette comparaison.
Les189 sources, les six artefacts, le helper et les deux fichiers d'autorité
sont épinglés et inchangés à la fermeture. Les lecteurs vérifient aussi
commandes exactes, sortie brute/base64, JSON re-parsé et les20 égalités.

Helper utilisé sans modification, SHA256
`92ff774fd42484b37676f376b07d91566aebab8c47c3184c52b9d90f9fef030e`.
Manifeste de la capture :
`62c7af7142824c24e6bd7c6464bf648d81c0f53d55a0874fe26d64e0c17d0010`.

Commande de capture :

```sh
python3 morsehgp3D_v8/receipts/q4_window_20260920/mutants/run_default_differential.py run --build build/v8_q4_window_20260920
```

Portée : compatibilité sur une arête fournie et ces20 cas, aucune qualification
de performance, sous-quadratique global, tour FULL, G4 ou GPU. GCP non utilisé.
