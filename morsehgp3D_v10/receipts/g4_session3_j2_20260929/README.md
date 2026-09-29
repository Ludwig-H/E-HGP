# Reçu : session G4 de performance après J2, CPU seul (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. **Aucun calcul sur GPU.**

## Session

- Code au commit `82fc2a6b5`, qui contient J2 (`5565f94fb`, filtre des nœuds en forme D-loc). Build Release par
  défaut, sans `MHGP10_MARCH`.
- Plan [`plan_s3_j2.json`](plan_s3_j2.json) : celui de la session 2 (`receipts/g4_session2_perf_20260929`), plus le
  catalogue à 1 fil à K = 10. 21 commandes, toutes réussies. La porte `ctest -L fast` fait 2 tests sur 2 : l'unitaire,
  et la porte de multiplicité en Python nu.
- Mêmes données qu'en session 2 : les trois trames LiDAR entières sans sol, de 39 885, 35 551 et 45 845 sites.
- VM `g4-standard-48` SPOT (AMD EPYC 9B45, 48 fils, g++ 11.4).
- **Arrêt certifié TERMINATED** sur la cible exacte : génération `06:24:40.195-07:00`, arrêt
  `06:29:55.928-07:00`, code 0 ; clé OS Login retirée.
- Le statut `failed_remote` vient seulement de l'absence de pip sur la VM.
- `receipt.json` et `preflight.json` sont copiés avec l'adresse du compte masquée ; sha256 des originaux dans
  `ORIGINAUX.sha256`.

## Résultats (dernière passe chaude)

**Catalogue de la trame 02, avant J2 (session 2) et après :**

| K | Fils | Session 2 (s) | J2 (s) | Gain | Boîtes, session 2 (s) | Boîtes, J2 (s) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 1 | 8,514 | 4,819 | ×1,77 | 7,979 | 4,326 |
| 5 | 12 | 0,782 | 0,482 | ×1,62 | 0,654 | 0,362 |
| 5 | 24 | 0,481 | 0,324 | ×1,49 | 0,369 | 0,220 |
| 5 | 48 | 0,314 | 0,235 | ×1,33 | 0,214 | 0,143 |
| 10 | 1 | — | 19,161 | — | — | — |
| 10 | 24 | 1,630 | 1,189 | ×1,37 | 1,258 | 0,823 |
| 10 | 48 | 1,098 | 0,840 | ×1,31 | 0,760 | 0,516 |

À 48 fils, le gain est moindre qu'à 1 fil : l'assemblage, la frontière et l'ordonnancement, que J2 ne touche pas,
y pèsent davantage. De 1 à 48 fils, le catalogue gagne maintenant ×20 (×27 avant J2).

**Trames entières à 48 fils (catalogue + tour, sans attaches) :**

| Trame | K | Session 2 (s) | J2 (s) | Catalogue (s) | Tour (s) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 00 | 5 | 0,373 | 0,283 | 0,201 | 0,082 |
| 01 | 5 | 0,305 | 0,231 | 0,163 | 0,068 |
| 02 | 5 | 0,374 | 0,285 | 0,196 | 0,089 |
| 00 | 10 | 1,541 | 1,291 | 0,846 | 0,445 |
| 01 | 10 | 1,234 | 1,015 | 0,682 | 0,333 |
| 02 | 10 | 1,498 | 1,237 | 0,827 | 0,411 |

**Chaîne complète jusqu'aux étiquettes** (`mhgp10_cluster`, K = 5, couverture, EOM z = 3, mcs = 200) :

| Trame | Catalogue (s) | Tour (s) | Tête (s) | Total (s) | Session 2 (s) | Amas |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 00 | 0,232 | 0,057 | 0,173 | 0,462 | 0,547 | 66 |
| 01 | 0,186 | 0,046 | 0,148 | 0,380 | 0,459 | 45 |
| 02 | 0,238 | 0,060 | 0,177 | 0,475 | 0,558 | 46 |

Les nombres d'amas sont ceux de la session 2 : J2 ne change pas le catalogue.

## Lecture

- Le gain local de l'agent se retrouve sur G4 à 1 fil : ×1,84 sur l'étage des boîtes, contre ×1,64 sur le
  codespace ; ×1,77 sur le catalogue entier, contre ×1,59.
- À K = 5 et 48 fils, catalogue et tour prennent 0,23 à 0,29 s par trame.
- Dans la chaîne complète, la tête pèse maintenant 0,15 à 0,18 s sur 0,38 à 0,48 s. C'est le prochain levier du
  temps de bout en bout, avec la feuille du catalogue (J3).
- Pour viser 100 ms à K = 5, il reste un facteur 2,3 à 2,9 sur le catalogue et la tour, et la tête à
  paralléliser.
