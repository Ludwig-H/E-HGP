# CST-0024 : la porte `diff_v10` déclenchait une course de données de la v10 figée

7 octobre 2026. Constat du développeur (`CST-0024`, [registre](../../audits/CONSTATS.md)) : la porte
`mhgp12_reference_diff_v10` (oracle borné contre les binaires figés de la v10, d'ordinaire 5 s) avait expiré à 300 s
dans la série intégrée u21, puis échoué au premier rejeu, puis passé 13 fois de suite. Cause établie ici, correctif
dans le même commit. GCP non utilisé.

## Cause

Les binaires figés `mhgp10_catalogue` et `mhgp10_tower` sont construits au commit `c764e121a` de la v10 (29 septembre,
10 h 07), **antérieur** au correctif `8e3b76245` (29 septembre, 15 h 18) d'une course du pool de fils : un ouvrier en
retard lit le descripteur du travail que `Pool::parallel_for` pose sur la pile du fil principal, après le retour de
celui-ci (« a late worker could read the next job's counter and fields unsynchronized and run a chunk twice or with
the wrong function »). La porte jouait ces binaires à 1, 2 puis 4 fils, en alternance.

## Preuves (machine locale, 8 cœurs partagés avec quatre agents)

1. **ThreadSanitizer** sur `mhgp10_tower` reconstruit au même commit (`git archive c764e121a`, `-fsanitize=thread`,
   `setarch -R`), sur un nuage de 15 points de la famille `generic`, $K=3$ :
   - à 4 fils, trois prises : 2, 0 et 1 avertissements ; tous `data race` dans `Pool::run_chunks` (`pool.cpp`, lignes 32
     et 33), lecture par un ouvrier d'une case écrite par le fil principal dans `Pool::parallel_for`, « location is
     stack of main thread » ;
   - à 1 fil, cinq prises : aucun avertissement ; le dump d'une prise à 1 fil est identique à l'octet à celui d'une
     prise à 4 fils sans course.
2. **Les deux symptômes reproduits** par la porte elle-même, binaires Release au même commit, trois instances
   concurrentes par tour :
   - un **écart de dump** (code 1) : `clusters_11_2`, $K=4$, tour en entrée core, ligne 22 : le binaire figé publie
     `node 5 6 9 4 4294967295` (un nœud sans parent) au lieu de `node 5 6 9 4 6` ;
   - une **expiration** : un `mhgp10_tower` à 4 fils sur un nuage minuscule tourne à 99 % d'un cœur pendant plus de
     deux minutes, fil principal en exécution, trois ouvriers endormis sur leur futex, jusqu'à l'expiration de la prise.
   Bilan avant correctif : **18 prises** (six tours de trois avant l'arrêt du stress), **16 conformes, 1 écart de dump, 1 expiration** (`timeout 240`, code 124).

## Correctif

`reference/test_dump_v10.py` joue les binaires figés **à un seul fil** (`THREADS = (1,)`), avec la cause en
commentaire : la porte juge les dumps de la v10, pas son parallélisme. Rien d'autre ne change (mêmes nuages, mêmes
compteurs exacts, même ligne). Bilan après correctif, même stress (quinze tours de trois instances concurrentes, mêmes binaires) : **45 prises, 45 conformes**, chacune avec la ligne attendue `reference_diff_v10_ok nuages=190 catalogues=190 tours=640 lignes=26530`. Les 26 portes `diff_v10` (porte, grand lot, refus, mutants et jumelles `-O`) passent par CTest (26/26, 47 s, `-j3`).

## Ce que cela ne dit pas

Le moteur de la v12 n'est pas en cause (aucun de ses fils n'intervient dans cette porte). La v11, qui jouait la même
porte contre les mêmes binaires, portait la même fragilité latente ; ses reçus ne sont pas requalifiés ici.
