# Session G4 cache2 : le cache de blocs de la Session rejoué avec le pilote apparié fermé — adopté

8 octobre 2026. Session gardée `v12.20261008.cache2b` (`gcp-migration/v12_session.py`, commit `bdfca8fb1`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 10:24:05
à 10:35:47 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 10:36:27 UTC. Une première
tentative (`v12.20261008.cache2`) avait été refusée par le lanceur avant tout démarrage, faute de place sur le disque
du codespace : aucune VM n'a tourné. Reçu sans identité de compte : [`receipt.json`](receipt.json) ; sorties sous
`resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Pourquoi rejouer.** La [session M](../g4_fullm_20261008/README.md) avait adopté le cache avec un pilote apparié
encore ouvert. Rejugée avec les fermetures de l'auditeur (`85db49890`), sa campagne est refusée strictement : la
fermeture du binaire n'y était pas relevée. Cette session rejoue la même question avec le pilote fermé : journaux
d'identité relus, résumés recalculés, cohortes fermées, empreinte de la sonde égale au début et à la fin.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (`ctest -LE long`, porte native de terminaison comprise) | ok, 730 portes sur 730 | 141 s |
| `apparie_cache_ferme` ([`pilote_apparie.py`](../../microbancs/mes_apparie/pilote_apparie.py), fermé) | ok, campagne jugée, aucun refus | 262 s |

## Verdict de `REGLE_APPARIEE` : cache **adopté**

Référence et A/A : `--cache=0`. Bras `cache` : défaut de la Session (8 Gio). ng00–02 à K5, appareil, 48 fils,
10 tours × 10 passes. Empreinte de la sonde `a72884f5…` au début et à la fin.

| Bras | ng00 | ng01 | ng02 | Verdict |
| --- | --- | --- | --- | --- |
| `cache` | 0,922 (0,917–0,927) | 0,925 (0,920–0,931) | 0,912 (0,906–0,917) | **adopté** |
| `aa` | 0,995 (0,985–1,005) | 1,008 (1,002–1,014) | 0,995 (0,988–1,002) | contrôle, dans la fenêtre |

Murs médians (ms), sans cache → cache : 94,7 → 87,2 (ng00), 77,8 → 71,8 (ng01), 96,4 → 88,1 (ng02) ; temps CPU par
passe −9 %. Session `v12set`, information : médiane 160,2 → 147,8 ms, maximum 318,9 → 297,5 ms.

La [session M](../g4_fullm_20261008/README.md) donnait 0,929 / 0,924 / 0,918 : l'adoption est reproduite, cette fois
avec toutes les preuves. Le cache reste le défaut de la Session (`72f622a55`).

## Ce que cette session n'établit pas

Ni le contrat FULL (le cache retire 7 à 9 % du mur ; la médiane des 37 trames reste vers 148 ms en information), ni
l'effet du cache sur les petits nuages, mesuré sans effet dans la [session C3](../g4_mesc3_20261008/README.md). GCP
utilisé pour cette seule session, arrêt certifié.
