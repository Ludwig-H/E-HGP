# Session G4 C2 : petits nuages rejoués avec le Pool à équipe et le lot T2-d-C (`MES-C`, régime (c))

8 octobre 2026. Session gardée `v12.20261008.mesc2` (`gcp-migration/v12_session.py`, commit `27eca166b`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 07:07:21
à 07:25:44 UTC, **arrêt certifié `TERMINATED`**. Reçu sans identité de compte : [`receipt.json`](receipt.json) ;
sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (voie CPU complète) ; cuda_g4 (catalogue, voie appareil)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

Même paquet et même pilote que la [session C](../g4_mesc_20261008/README.md), avec la règle de cohorte corrigée
(`24dec7b85`). Configurations : voies CPU et appareil, K5 et K10, 4 et 48 fils (le régime à un fil, qui avait expiré,
n'est pas rejoué), trois tours ; familles difficiles seules à 48 fils. Durée : 889 s, toutes les Sessions jouées, GPU
vide avant et après, aucun contrôle manquant.

## Verdict : non tenu

| Critère | Mesure | État |
| --- | --- | --- |
| C1 (ordonnée à l'origine ≤ 2 ms, CPU, K5, 48 fils, nuages réels) | 16,45 ms | non tenu |
| C2 (pente ≤ 3 727,2 ns par site, même configuration) | 11,57 µs par site | non tenu |
| C3 (cohorte difficile complète à K5, deux voies, 48 fils) | quasi-sphère refusée `wide_leaf` à 3 000 et 10 000 sites sur les deux voies | non tenu |

## Comparaison descriptive avec la session C (nuages réels, valeurs chaudes, K5)

| Configuration | t médian, ≤ 150 sites (C → C2) | t médian, ≥ 5 000 sites (C → C2) | a (C → C2) | b (C → C2) |
| --- | ---: | ---: | ---: | ---: |
| CPU, 48 fils | 15,60 → **11,13 ms** | 121,3 → 115,6 ms | 20,34 → 16,45 ms | 11,75 → 11,57 µs |
| CPU, 4 fils | 6,82 → 6,59 ms | 458,5 → 460,4 ms | −0,37 → −0,52 ms | 54,9 → 55,1 µs |
| appareil, 48 fils | 8,49 → **4,54 ms** | 41,5 → 38,6 ms | 9,67 → 6,05 ms | 3,61 → 3,73 µs |
| appareil, 4 fils | — | — | 3,50 → 3,26 ms | 10,54 → 10,39 µs |

Deux changements séparent les sessions : le Pool à équipe dimensionnée (`5b3362bbd`) et le lot T2-d-C (`02b735d6b`,
étage C sur l'appareil). La comparaison est donc descriptive et n'isole aucune cause
([contre-lecture du Pool](../audit_reponses_20261008/pool_equipes/README.md)). Le motif est néanmoins cohérent avec le
Pool : à 48 fils, les plus petits nuages gagnent 29 % sur la voie CPU, où le lot C n'agit pas, et 47 % sur la voie
appareil ; à 4 fils, presque rien ne change.

K10 sur l'appareil, joué cette fois : 48 fils, 8,7 ms + 15,9 µs par site (7,4 ms vers 150 sites, 139 ms au-delà de
5 000) ; 4 fils, 7,8 ms et 620 ms. Réseau entier à K5 : 290 ms (CPU) et 232 ms (appareil) à 10 000 sites, droite
100 à 1 000 sites calculée, empreintes identiques partout.

## Lecture

Le coût fixe des petits nuages baisse mais reste loin des 2 ms : 4,5 ms vers 150 sites sur l'appareil à 48 fils, la
meilleure configuration. Au-delà de la coordination des fils, il vient du catalogue CPU, environ 190 µs par site et
par fil, ce qui explique que la voie appareil l'emporte déjà, et des lancements de la voie appareil. La quasi-sphère
attend la voie large T1-c.

## Ce que cette session n'établit pas

Ni l'effet isolé du Pool (deux changements, aucune session appariée), ni le régime à un fil, ni le budget de 2 ms.
GCP utilisé pour cette seule session, arrêt certifié.
