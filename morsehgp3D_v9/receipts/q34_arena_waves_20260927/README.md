# Reçus du consommateur q34 par vagues

Audit CPU isolé, base `fd1a2c7ee`, grille entière 1 mm, `not_claimed`.
Aucun moteur ou GCP modifié ; aucune mesure de tour ou de trame LiDAR.
Voir [la synthèse et la portée exacte](../../audits/b_q34_arena_waves_20260927/README.md).

`capture.json` / `summary.json` : quinze commandes closes, Release et
Clang ASan/UBSan/LSan, 175 lots et 1 050 consommations ; trois mutations
natives par build rejetées avec le motif attendu. `checks/checks.json` :
lectures normale/`-O` identiques et huit corruptions causales rejetées.

Commande de provenance, exécutée hors du contexte ptrace :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_arena_waves_20260927/run.py --capture morsehgp3D_v9/receipts/q34_arena_waves_20260927 --build-prefix /workspaces/E-HGP/build/v9-audit-q34-arena-waves-20260927
```

Les builds `v9-audit-q34-arena-waves-20260927_{release,sanitize}` sont
épinglés. Ne pas écraser ces chemins pour une nouvelle compilation.
L'arène collective figée à `fd1a2c7ee` et les archives générateur immuables
du 26 septembre sont explicitement réutilisées, hashes avant/après inclus.

Relectures :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_arena_waves_20260927/run.py --readback morsehgp3D_v9/receipts/q34_arena_waves_20260927
python3 -O -B morsehgp3D_v9/audits/b_q34_arena_waves_20260927/run.py --readback morsehgp3D_v9/receipts/q34_arena_waves_20260927
```

Le script `check_capture.py` a en plus enregistré les lectures et mutations
dans `checks/`. Ce dossier existe déjà et le script refuse de l'écraser.
Les preuves restent LIVE, dépendantes des sources/builds locaux, pas une
archive autonome exportable par simple copie.

## Préflight conservé dans la chronologie, hors autorité

Le build mutable `/workspaces/E-HGP/build/v9-audit-q34-waves-preflight-20260927`
a servi à la mise au point avant gel. Son premier `--gate` a quitté avec
code1 et ce motif :

```text
mhgp9 gen WSPD requires a mask in 1..7 intersecting available lanes
```

Le test appelait le producteur q34 à K1 alors qu'aucune voie q34 n'y est
active. Correction du gate : utiliser des rectangles géométriques valides
produits à K2, puis vérifier leur fermeture par le consommateur à K1,
comme le fait `run_q34_filter_batch_cpu`. Le consommateur et le moteur
n'ont pas été modifiés pour contourner ce refus.

Le premier préflight corrigé couvrait174 lots, mais aucun cas E>0/S=0 ;
ce trou a été fermé avec une fixture native découverte exhaustivement.
La capture qualifiée distincte couvre175 lots. Deux boucles incorrectes sur
sortie vide avaient par ailleurs été corrigées par lecture avant tout gel ;
ne pas prétendre qu'un sanitizer avait découvert ce défaut à l'exécution.
Les sorties du préflight ne remplacent aucune commande de la capture close.
