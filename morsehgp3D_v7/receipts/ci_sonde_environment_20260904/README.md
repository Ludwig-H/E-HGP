# Correctif de la porte sonde sous environnement setup-python

`public_status=not_claimed`. Reçu de correction du harnais Python seulement,
sans changement du moteur, de CMake, du workflow ou du garde du lanceur.
Aucun commit, relancement GitHub Actions ou appel GCP effectué par cet agent.

## Cause observée

Le [run GitHub 33924177970](https://github.com/Ludwig-H/E-HGP/actions/runs/33924177970),
commit `d9e4ee0152435d4394bedfe1dc5134d5125a65a8`, construit correctement
le CPU puis termine CTest en échec : 291/292 portes passent. Seule
`mhgp7_sonde_ablation_gate` échoue, avec trois contrôles d'inventaire rouges.
Les étapes suivantes sont ignorées, pas validées. Les dix portes mono/Axis
passent dans ce run. Les métadonnées et extraits bruts figurent dans
[github_run.json](github_run.json) et [github_failed_excerpt.log](github_failed_excerpt.log).

L'environnement CTest porte explicitement
`LD_LIBRARY_PATH=/opt/hostedtoolcache/Python/3.12.14/x64/lib`.
Le [lanceur](../../bench/sonde_ablation_reduce.sh), lignes 207–213,
refuse volontairement cette variable avant toute opération, y compris
`--inventaire` (lignes 297–300). Ce refus rc=2 est correct.

Dans [l'avant](sonde_ablation_gate.before.py), les deux `subprocess.run`
de la scène (n), lignes 1034–1035 et 1041–1042, héritent de l'environnement.
Ils contournent ainsi le nettoyage déjà effectué par `Porte.lancer`.
Leurs stdout vides expliquent les trois contrôles rouges ; ce n'est pas
une divergence mathématique, un timeout ou un échec de compilation.

## Correction et fixture permanente

[Le diff](change.patch) ne change que
[tests/sonde_ablation_gate.py](../../tests/sonde_ablation_gate.py).
Les deux inventaires nominaux reçoivent maintenant une copie nettoyée des
seules `VARIABLES_REFUSEES`, en conservant les autres variables utiles.

La scène (n) réintroduit temporairement **seulement** `LD_LIBRARY_PATH`
dans l'environnement ambiant via `patch.dict(..., clear=True)`.
Elle exige d'abord un inventaire brut rc=2, stdout vide, avec le motif
spécifique de cette variable. Puis les deux inventaires nettoyés doivent
rendre rc=0 et les ensembles exacts attendus. Omettre à nouveau `env=`
hériterait donc systématiquement de la variable, même sur une machine
locale dont l'environnement initial est propre. Le contexte restaure
l'environnement initial à sa sortie. Aucun contrôle ne repose sur
`assert` ; la scène (y) de rejets explicites reste active.

Le chemin sentinelle local est non vide et n'a pas besoin d'exister :
le garde teste sa présence, sans charger une bibliothèque. Les autres
variables refusées sont retirées avant le témoin brut pour qu'aucune
ne masque sa cause. Le garde du lanceur demeure octet pour octet inchangé.

## Résultats locaux

Python local 3.12.1 ; Python CI 3.12.14. Les commandes exactes sont dans
[commands.txt](commands.txt). Chaque exécution parcourt les 23 scènes.
Les temps sont ceux de la porte et de ses faux binaires, pas un benchmark HGP.

| Source | LD_LIBRARY_PATH externe | Python | Code de porte | Temps réel |
| --- | --- | --- | --- | --- |
| Avant exact d9e4ee01 | Présent | Normal | 1, trois erreurs attendues | 29,78 s |
| Après | Présent | Normal | 0 | 29,40 s |
| Après | Présent | -O | 0 | 29,51 s |
| Après | Absent | Normal | 0 | 29,60 s |
| Après | Absent | -O | 0 | 30,30 s |

Le refus rc=2 est celui du lanceur ; le code 1 de la première ligne est
celui de la porte qui attendait incorrectement deux inventaires rc=0.
Le [rejeu avant exact](before_dirty_normal_exact.log) emploie la copie
strictement identique au commit, hash `77b47261…`. La copie après,
hash `acca76c1…`, est strictement identique au test intégré.

[before_dirty_normal.log](before_dirty_normal.log) conserve aussi le
premier diagnostic capturé (29,21 s, mêmes trois erreurs) : sa copie avait
perdu uniquement la ligne vide terminale, hash `53374f61…`. Ce diagnostic
n'est pas substitué au rejeu byte-identique final. Les quatre logs après
et ce log préliminaire ont été comparés aux captures originales ; ils
conservent stdout/stderr combinés et les trois lignes de `time -p`.

## Portée et vérification des artefacts

[manifest.json](manifest.json) fixe les codes, les durées, les sources et
les logs. [sources.sha256](sources.sha256) fixe sept fichiers depuis la
racine du dépôt ; `SHA256SUMS` fixe les artefacts de ce dossier. Les copies
avant/après sont des preuves historiques, pas des portes CTest ajoutées.

```bash
sha256sum -c morsehgp3D_v7/receipts/ci_sonde_environment_20260904/sources.sha256
sha256sum -c morsehgp3D_v7/receipts/ci_sonde_environment_20260904/SHA256SUMS
```

Cette matrice ciblée ne prétend ni relancer la suite C entière, ni rendre
vert le run GitHub historique, ni qualifier une performance ou l'exactitude
du moteur. La porte affiche toujours sa limite héritée sur la cible réelle
`build/v6/mhgp7_profile_sonde` absente ; cette limite n'est pas levée ici.
GCP non utilisé.
