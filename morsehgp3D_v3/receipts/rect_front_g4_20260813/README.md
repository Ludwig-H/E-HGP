# Reçu G4 du 13 août 2026 — le budget était sous la profondeur de l'arbre

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Session `gcp-migration/session_rect_front_g4.sh`, scripts gardés, instance
`ehgp-blackwell-spot-ai1a`, `g4-standard-48` SPOT, `maxRunDuration=3600 s`,
arrêt invité secondaire à `45 min`. Arrêt certifié par le trap sur exactement
la génération démarrée. `CUDA_COMPILE=OK`.

**Aucun débit GPU n'est mesuré et aucun SLO n'est qualifié.** Les quarante-huit
cœurs lancent quinze runs mono-thread en parallèle. Le NO-GO device reste
entier.

## 1. Le refus, et sa cause

À budget `24`, **les quinze runs refusent** la règle des deux pentes, avec des
pentes de `1,4` à `2,4`. Le balayage de budget sur `eight_clusters`, la famille
la plus dure, donne la cause :

| budget | fermé à `50 000` | front/pt | pentes | verdict |
| ---: | ---: | ---: | :---: | :---: |
| `8` | `2,82 %` | `781` | `1,990 / 1,992` | REFUSÉ |
| `16` | `75,70 %` | `213` | `1,884 / 1,955` | REFUSÉ |
| `24` | `95,18 %` | `54,9` | `1,436 / 1,365` | REFUSÉ |
| `48` | `97,41 %` | `31,6` | `1,190 / 1,301` | **VERT** |
| `96` | `97,69 %` | `28,2` | `1,150 / 1,246` | **VERT** |
| `192` | `97,73 %` | `27,8` | `1,148 / 1,214` | **VERT** |

Atteindre un nœud témoin au niveau des feuilles coûte environ
`2\log_2(n/\text{leaf})` classifications, soit `25` à `n=50 000` et `27` à
`n=100 000` pour `leaf=8`. **Un budget de `24` est sous ce seuil**, et le
certificat n'échoue alors pas par géométrie mais par profondeur : les arbres
plus grands sont plus profonds, donc plus pénalisés, ce qui fabrique
mécaniquement une pente supérieure à un.

Le rendement sature nettement au-delà de `48` — `97,41 %` puis `97,69 %` puis
`97,73 %`. Le budget utile est donc `\Theta(\log n)` à petite constante, et non
une ressource à augmenter sans fin.

## 2. Les quatre familles à budget 24, pour mémoire

Fractions fermées à `50 000` puis `100 000`, lane q2 :

| famille | `12 500` | `25 000` | `50 000` | `100 000` | front/pt à `100 000` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scanline_single_pass` | `99,71 %` | `99,84 %` | `99,87 %` | `99,84 %` | `2,51` |
| `scanline_overlap_multiecho` | `98,18 %` | `98,52 %` | `98,54 %` | `98,54 %` | `21,05` |
| `terrain` | `96,99 %` | `98,13 %` | `98,35 %` | — | `19,58` à `50 000` |
| `eight_clusters` | `88,93 %` | `92,51 %` | `95,18 %` | `96,28 %` | `81,59` |

Les lanes q3/q4 ferment aussi : `95,38 %` et `93,50 %` sur
`scanline_overlap_multiecho` à `100 000`, `99,61 %` et `99,19 %` sur
`scanline_single_pass`. Seule `eight_clusters` q4 reste basse à `48,20 %`.

## 3. Ce que ce reçu NE décide pas

Les pentes publiées ici sont celles d'un budget sous-dimensionné ; elles ne
réfutent pas le certificat, elles mesurent une ressource insuffisante. Aucune
campagne à budget `\Theta(\log n)` n'a encore couvert les quatre familles sur la
rampe longue, et c'est la mesure qui décide.

Aucun octet, aucun high-water, aucune pente de source, aucune tranche
`SupportKey -> BallKey -> census -> fold`. La descente témoin repart de la
racine à chaque rectangle et n'est donc pas la continuation persistante exigée
par l'audit `96be8e0`. Le contrat `50 000` reste entièrement ouvert et G4 reste
NO-GO pour toute qualification.
