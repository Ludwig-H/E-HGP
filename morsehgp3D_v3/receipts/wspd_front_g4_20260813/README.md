# Reçu G4 du 13 août 2026 — le front WSPD est linéaire, mesuré

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Session `gcp-migration/session_rect_front_g4.sh`, scripts gardés,
`g4-standard-48` SPOT, `maxRunDuration=3600 s`, arrêt déclenché par le trap.
`CUDA_COMPILE=OK`, `14/14` portes rejouées sur la VM. Aucun débit GPU mesuré,
aucun SLO qualifié : les quarante-huit cœurs lancent des runs mono-thread en
parallèle.

## 1. Le fait principal

`WspdFrontLowerBound-v1`, cinq familles x trois séparations, rampe
`12 500 / 25 000 / 50 000 / 100 000`. **Quatorze configurations sur quinze
tiennent la règle des deux pentes**, avec des pentes `front_records` comprises
entre `0,97` et `1,23`. La seule refusée est `eight_clusters` à `s=4`
(`1,435` puis `1,586`).

C'est la première fois de ce chantier qu'un compteur **physique** dominant
passe la règle sur la rampe longue, et il la passe parce qu'il est linéaire par
théorème, non parce qu'un réglage l'y amène.

## 2. Front et fermeture à `n=100 000`

| famille | `s` | front/pt | q2 | q3 | q4 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scanline_overlap_multiecho` | `4` | `8,38` | `99,90 %` | `46,41 %` | `41,61 %` |
| `scanline_single_pass` | `4` | `9,03` | `99,94 %` | `79,16 %` | `62,81 %` |
| `terrain` | `4` | `22,22` | `96,06 %` | `12,22 %` | `7,03 %` |
| `eight_clusters` | `4` | `88,57` | `82,91 %` | `1,01 %` | `0,57 %` |
| `uniform` | `4` | `111,09` | `82,02 %` | `8,91 %` | `4,50 %` |
| `scanline_overlap_multiecho` | `2` | `3,83` | `92,64 %` | `0,28 %` | `0,14 %` |
| `terrain` | `2` | `8,57` | `51,16 %` | `0 %` | `0 %` |
| `uniform` | `2` | `29,35` | `41,72 %` | `0 %` | `0 %` |

Sur les familles de type LiDAR, `s=4` donne **environ neuf enregistrements par
point** avec `99,9 %` de fermeture q2 — soit `450 000` records à `50 000`
points.

## 3. Ce que ce reçu NE décide pas

La fermeture q3/q4 y est un `PRUNED_MAX_EDGE_ANCHOR` **sous obligation** : la
précondition `owner = max_edge_canonical` n'est pas établie par ce sujet.

Aucun temps, aucun octet, aucun high-water, aucun `p95`. Aucune tranche
`SupportKey -> BallKey -> census -> fold`. La banque Morton n'est pas exercée
ici. Le contrat `50 000` reste entièrement ouvert et G4 reste NO-GO.

## 4. Deux défauts de la session, déclarés

Le front de rectangles a tourné avec `--budget=24` et sans `--core` : mes
éditions successives du script avaient écrasé les continuations `\`, et mes
remplacements suivants ont échoué **en silence**. Ses chiffres sont donc ceux
d'un budget constant, pas du budget-profondeur annoncé.

La session s'est terminée en `rc=127` sur une ligne de commentaire cassée par la
même cause. Le trap a fonctionné et l'arrêt a été déclenché sur la génération
démarrée.
