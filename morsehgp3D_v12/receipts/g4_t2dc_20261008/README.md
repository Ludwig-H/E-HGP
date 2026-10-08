# Session G4 T2-d-C : transferts et publication du catalogue sur l'appareil — adopté

8 octobre 2026. Session gardée `v12.20261008.t2dc` (`gcp-migration/v12_session.py`, commit `02b735d6b` = `main`
avec le patch T2-d-C intégré, preuve `pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`,
`--max-run-seconds 4200`. VM de 06:44:12 à 06:59:58 UTC, **arrêt certifié `TERMINATED`**. Pilote
[`bench/g4_catalogue_flux.py`](../../bench/g4_catalogue_flux.py) (716,9 s), juge et lecteur strict du chantier. Reçu
sans identité de compte : [`receipt.json`](receipt.json) ; rapport, tableaux et journaux natifs sous `resultats/` ;
empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue, voie hybride)
objet=full_pi0 (catalogue Cat_K, puis tour FULL pour les informations)
quantification=quantized_u21_input_only
public_status=not_claimed
```

Bras : `avant` = archive de `902041f66` (SHA-256 `2e760983…` épinglé dans le plan), `apres` = le produit joué, et
quatre ablations par substitution d'une constante (`sans_anticipation`, `sans_double_tampon`, `repli_cles_entieres`,
`flux_et_repli_selectif`), plus un bras `avant_bis` pour l'A/A. Sept bras en processus alternés, 10 tours × 10 passes,
48 fils, ng00–02 à K5. Règle `REGLE_T2D_C` écrite d'avance : adopté si les empreintes sont identiques partout et si la
borne haute **non arrondie** de l'IC 95 % (bootstrap, 10 000 tirages) du rapport après/avant est sous 1 sur chacune
des trois trames ; refusé si une prise manque, si le mutant n'est pas comparé ou si l'A/A sort de [0,985 ; 1,015].

## Verdict : adopté

| Levier (de → vers) | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| **lot** (avant → après) | **0,786** (0,781–0,791) | **0,798** (0,793–0,802) | **0,737** (0,734–0,740) |
| flux (avant → flux et repli sélectif) | 0,853 | 0,864 | 0,821 |
| double tampon (sans → après) | 0,946 | 0,943 | 0,939 |
| sorties anticipées (sans → après) | 0,992 | 0,987 | 0,982 |
| fenêtres du repli (clés entières → après) | 1,001 (publié) | 0,996 (publié) | 0,979 |
| A/A (avant → avant bis) | 0,992 | 1,004 | 1,003 |

Mutant appareil `flux_sans_attente_appareil` : tué (empreinte). Identité : MHGP12DP, niveaux, table et comptes égaux
entre bras et entre voies ; FUL1 égale à celle de la session K.

## Étage C à chaud, K5, millisecondes

| Trame | avant | après | dont transferts (avant → après) | publication (avant → après) | mémoire épinglée | pic de l'hôte |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 34,29 | **26,90** | 9,06 → 3,26 | 4,04 → 1,66 | 52,1 → 16,8 Mo | 994 → 907 Mo |
| ng01 | 29,86 | **23,76** | 7,90 → 2,94 | 3,19 → 1,37 | 45,2 → 16,8 Mo | 828 → 754 Mo |
| ng02 | 36,52 | **26,85** | 10,29 → 3,47 | 3,99 → 1,63 | 52,8 → 16,8 Mo | 1 011 → 922 Mo |

« Transferts » est une partition du mur du flux, pas une mesure du DMA ; « sorties » (2,2 à 2,7 ms après) porte la
réservation et le premier toucher des tableaux de sortie.

**Informations (ne décident rien).** Étage C à K10 : 135,9 → 102,7 ms (ng00), 112,0 → 84,0 (ng01), 145,3 → 98,2
(ng02). Mur FULL K5 : 160,2 → 153,9 ms (ng00), 128,0 → 122,7 (ng01), 164,9 → 155,6 (ng02). Sur les 37 trames
`v12set`, l'étage C baisse de 16,0 à 26,5 % (trame la plus lourde : 90,0 → 71,7 ms). Cache de blocs : environ −1 ms
de plus sur C.

## Lecture

Le lot T2-d-C retire 21 à 26 % de l'étage C (7 à 10 ms à K5, 33 à 47 ms à K10) sans changer le catalogue. Le mur FULL
ne baisse que de 5 à 6 ms, car G et T, M, V, R restent en série. Ce sont les chantiers T2-d-A (recouvrement) et
T2-d-B (coût interne de G), en cours. Le contrat de 100 ms n'est pas tenu.

## Ce que cette session n'établit pas

Ni le contrat FULL, ni la voie appareil sur les grandes scènes (la mémoire de la carte y reste le mur : le flux des
sorties ne borne pas l'arène ni la fin d'étage), ni le levier facultatif de libération avant croissance, livré à part
et non joué. GCP utilisé pour cette seule session, arrêt certifié.
