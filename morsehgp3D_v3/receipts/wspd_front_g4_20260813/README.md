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
tiennent la règle des deux pentes** — refus après deux pentes consécutives
supérieures ou égales à `1,35`.

CORRECTION (contre-audit `33df59d`). La première rédaction de ce reçu nommait
la mauvaise famille et résumait un mauvais intervalle. La configuration refusée
est **`scanline_single_pass` à `s=4`** (`0,982 / 1,435 / 1,586`), tandis que
`eight_clusters` à `s=4` **passe** (`1,215 / 1,225 / 1,182`). Et l'intervalle
annoncé `0,97..1,23` était faux : `scanline_single_pass` à `s=1` est vert avec
une pente de `1,349`. Les cardinaux et les pourcentages du tableau ci-dessous
concordent, eux, avec le transcript ; c'étaient le nom du refus et la borne
résumée qui étaient faux.

C'est la première fois de ce chantier qu'un compteur **physique** passe la
règle sur la rampe longue.

Le mot « dominant » n'est PAS soutenu et il est retiré : aucun temps de phase
n'est publié ici. Le même transcript montre au contraire que la passe témoin
consomme presque tout son quantum — `62,5` à `63,0` dépilages par record — et
que `rect_classify` est encore appelé une fois par bit de lane ouvert, si bien
que le nombre d'appels arithmetiques reels peut approcher le triple. La WSPD
borne le nombre de terminaux ; elle ne borne ni cette reprise a `C=root`, ni la
source carrier, ni le census, ni le fold.

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

## 3. Le choix de `s` n'est PAS justifié par ce reçu

Les pourcentages q2/q3/q4 ci-dessus proviennent de la DFS capée à `64`, non de
la banque `W=32 / L=16`. Ils sont des diagnostics de couverture d'un **autre**
chemin et ne permettent pas de choisir la séparation du futur kernel.

Passage de `s=2` à `s=4` à `n=50 000`, en records :

| famille | `s=2` | `s=4` | facteur |
| --- | ---: | ---: | ---: |
| `scanline_overlap_multiecho` | `174 259` | `364 869` | `2,09` |
| `scanline_single_pass` | `149 164` | `300 674` | `2,02` |
| `terrain` | `428 018` | `1 100 550` | `2,57` |
| `eight_clusters` | `1 188 612` | `3 904 810` | `3,29` |
| `uniform` | `1 392 028` | `5 143 451` | `3,69` |

Pour `uniform` à `s=2`, une banque `W=32 / L=16` a déjà une enveloppe de
`44,5` M lectures et `22,3` M recertifications avant la source ; à `s=4` elle
passe à `164,6` M et `82,3` M. La baseline reste donc `s=2`, avec ablation
`s=1`, puis raffinement **local** des seuls records ouverts.

## 4. Ce que ce reçu NE décide pas

La fermeture q3/q4 y est un `PRUNED_MAX_EDGE_ANCHOR` **sous obligation** : la
précondition `owner = max_edge_canonical` n'est pas établie par ce sujet.

Aucun temps, aucun octet, aucun high-water, aucun `p95`. Aucune tranche
`SupportKey -> BallKey -> census -> fold`. La banque Morton n'est pas exercée
ici. Le contrat `50 000` reste entièrement ouvert et G4 reste NO-GO.

## 5. Deux défauts de la session, déclarés

Le front de rectangles a tourné avec `--budget=24` et sans `--core` : mes
éditions successives du script avaient écrasé les continuations `\`, et mes
remplacements suivants ont échoué **en silence**. Ses chiffres sont donc ceux
d'un budget constant, pas du budget-profondeur annoncé.

La session s'est terminée en `rc=127` sur une ligne de commentaire cassée par la
même cause. Le trap a fonctionné et l'arrêt a été déclenché sur la génération
démarrée.
