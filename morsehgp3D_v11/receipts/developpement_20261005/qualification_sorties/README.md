# Qualification G4 de la sortie `supports` (L1 et L2) et mesure de la règle de L2

5 octobre 2026. Six sessions gardées, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié après chacune. Source : commits poussés `00bd979ac` (première session) et `b319efc84` (les autres), qui
contiennent S3, S5 (avec la correction de provenance), S6a, S6b et S7. Cadre : `exploration_v11_hors_registre /
cpu_reference / quantized_u21_input_only / not_claimed`. Aucune mesure ne promeut un statut public.

## Matrice

| Session | Configurations | Résultat |
| --- | --- | --- |
| `claudequalmatrice` (`00bd979ac`) | toutes | mutants **conformes** (459 mutants, 1 508 s) ; style conforme ; GCC Release 929/941 sans échec, le reste coupé par l'échéance derrière une porte `long` |
| `claudequalA` (`b319efc84`) | Release u18, u21, u24, empoisonnement | aucun échec ; les mêmes 12 portes non jouées derrière `mhgp11_cli_full_identity_scale32000_k10` (environ 1 500 s) |
| `claudequala2` (`b319efc84`, portes `long` retirées) | Release u18, u21, u24, empoisonnement | **conformes** : 914/914, 824/824, 824/824, 825/825 |
| `claudequalb` (`b319efc84`) | ASan+UBSan (u24), TSan (u21) | aucun échec : 774/824 et 759/824 ; les portes non jouées sont toutes des portes d'échelle et LiDAR en série, coupées par l'échéance sous sanitizer |
| `claudequall` (`b319efc84`) | `release_long` (u21) | **conformes** : toutes les portes `long` réelles (oracle de référence complet, Euler K10, identités K10 dont CLI 32 000 points 765 s et trame ng00 323 s) ; les quatre campagnes de mutants étiquetées `long` non rejouées, déjà conformes en première session |

**Périmètre déclaré.** Les portes d'échelle et LiDAR ne sont qualifiées qu'en Release (trois profils et
empoisonnement), pas sous sanitizer. Clang est absent de la VM.

## Mesure appariée de la règle de L2 (`claudequalmesure`, `b319efc84`)

Étage `tree`, médiane de trois prises, K = 5 ; FULL au masque 16 379, `--sortie=supports` au masque 7 035.

| Trame | W1 FULL | W1 supports | W48 FULL | W48 supports | rapport W48 |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 | 3,77 s | 1,99 s | 150 ms | 182 ms | 1,21 |
| ng01 | 2,78 s | 1,41 s | 117 ms | 138 ms | 1,18 |
| ng02 | 3,27 s | 1,58 s | 168 ms | 184 ms | 1,09 |

Règle écrite d'avance (`docs/SORTIES.md` § 11) : l'arbre d'ordre K seul reste le défaut si son étage `tree` ne
dépasse pas celui de `full` de plus de 10 % à W48 sur au moins deux trames. Une seule trame (ng02) la respecte :
**décision `livrer_L2b`**, journal des graines posé dans la voie concurrente de `build_full`.

Hors de la règle, diagnostic : à W48 sur ng00, durée totale médiane 0,75 s pour `--sortie=supports` (rattachement
74 ms, assemblage 51 ms, écriture 220 ms) contre 2,56 s pour `--sortie=full` (écriture de `MHGP11FUL1` 2,16 s).

**Invariance à W48** (complément de l'audit `4acbae533`) : `cli_supports_scale_verdict conforme` sur ng02 et ng00,
14 appels chacun (W1, W4, W48, répétition, permutation et réétiquetage à W48, FULL à W48, boîte cosphérique).

## Pièces

Par session : `receipt.json` (reçu du contrôleur), `launch.json`, `matrix_summary.json` et `result_<config>.json` ;
pour la mesure, `sorties_g4.json` et les sorties des deux contrôles W48. Aucune donnée ni coordonnée LiDAR.
`SHA256SUMS` couvre tous les fichiers sauf lui-même.
