# Reçu J0 — ledger de candidats tronqué, session G4 du 14 août 2026

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=truncated_candidate_ledger`,
`public_status=not_claimed`.

> **Ce reçu n'est pas une mesure de l'objet.** Type `truncated_candidate_ledger`,
> `source_complete=false`. Les chiffres sont produits sous une coupure `--dmax`
> **déclarée et non certifiée** ; deux des six pistes ont été refusées parce que
> cette coupure mord. Ils ne donnent ni taille certifiée de l'objet, ni exposant
> reçu, ni décision de SLO. La réponse d'audit Q20 fixe ce statut.

Zone `europe-west4-ai1a`, cible `ehgp-blackwell-spot-ai1a`, `g4-standard-48`
SPOT, 48 vCPU employés en CPU pur. **Aucun kernel CUDA, aucun débit GPU.**
Arrêt certifié `TERMINATED`.

## 1. La corroboration qui compte

| famille | `smax` | `n` | candidats totaux | par point | secondes (48 cœurs) |
|---|---:|---:|---:|---:|---:|
| `uniform` | 11 | 12 500 | 5 018 956 | 401,52 | 8 |
| `uniform` | 11 | 25 000 | 10 379 223 | 415,17 | 19 |
| `uniform` | 11 | **50 000** | **21 432 482** | **428,65** | 38 |
| `uniform` | 6 | 12 500 | 953 331 | 76,27 | 1 |
| `uniform` | 6 | 25 000 | 1 956 841 | 78,27 | 2 |
| `uniform` | 6 | **50 000** | **4 004 994** | **80,10** | 4 |

La chaîne historique avait publié `21 413 140` `SupportKey` à `50 000` points sur
`uniform`. Cette énumération en trouve `21 432 482`. **L'écart vaut `0,09 %`**,
entre deux chemins qui ne partagent ni structure, ni ordonnancement, ni
prédicats de sélection — l'un par blocs WSPD, l'autre par ancre d'arête
diamétrale. C'est la corroboration la plus forte obtenue jusqu'ici sur la taille
de la population.

Elle ne certifie rien : les deux chemins pourraient partager une erreur de
définition. Elle rend seulement improbable une erreur d'implémentation.

## 2. Le nombre par point est presque plat

| `smax` | 12 500 → 25 000 | 25 000 → 50 000 | exposant global |
|---|---:|---:|---:|
| 11 | `×1,0340` | `×1,0325` | `n^1,047` |
| 6 | `×1,0263` | `×1,0233` | `n^1,035` |

Sous la coupure déclarée et sur cette famille, la population croît donc
**presque linéairement**. Les deux exposants sont très en dessous du seuil
`1,35` du plan de test, et aucun des deux n'est un exposant de sortie reçu.

## 3. Ce que cela dit de l'échelle de repli du SLO

À `50 000` points sur `uniform`, sous coupure :

| profil | supports | pour `p95 < 1 s` | pour `p95 < 100 ms` |
|---|---:|---:|---:|
| `K=10`, `smax=11` | `21,4 M` | `21,4 M/s` | `214 M/s` |
| `K=5`, `smax=6` | `4,0 M` | `4,0 M/s` | **`40 M/s`** |

Le rapport entre les deux profils vaut `5,35`. Le repli fixé par l'utilisateur
change donc l'ordre de grandeur du problème, sans le rendre trivial.

Repère de coût, et il est frappant : l'énumération `K=5` à `50 000` points prend
**`4` secondes sur quarante-huit cœurs CPU**, sans aucun GPU, en produisant
déjà la population. Le facteur restant vers `100 ms` est `40`. Ce n'est pas une
promesse — l'aval n'existe pas, le producteur n'est pas output-sensitive, et
`eight_clusters` n'est pas mesurée — mais c'est la première fois qu'un facteur à
combler tient sur deux chiffres.

## 4. Ce que la session a refusé, et pourquoi c'est le point important

Quatre pistes sur six se sont arrêtées au code `3`.

| famille | `smax` | `n` atteint | `diam_max / dmax` | candidats / retenus |
|---|---:|---:|---:|---:|
| `terrain` | 6 | 12 500 | **`0,940`** | `105 671` |
| `terrain` | 11 | 12 500 | **`0,940`** | `35 965` |
| `eight_clusters` | 6 | 12 500 | — | `36 357` |
| `eight_clusters` | 11 | 12 500 | — | — |

Sur `terrain`, la coupure **mord** : le diamètre réellement atteint vaut `94 %`
de la coupure, très au-dessus du refus à `75 %`. La sonde a donc refusé, comme
elle doit. Les chiffres de ces pistes ne valent rien comme taille d'objet.

Et le rapport candidats/retenus y est catastrophique : `105 671` sur `terrain`
contre `623` sur `uniform`, soit **cent soixante-dix fois pire**. L'énumération
par ancre n'est pas une architecture pour les nappes ; elle n'est même pas un
instrument de mesure utilisable là.

**Conséquence directe.** Les deux familles obligatoires du contrat ne sont pas
toutes deux mesurées : `eight_clusters` s'arrête à `12 500` et n'a pas de
troisième palier. Aucune décision de SLO ne peut donc être prise sur la seule
`uniform`, qui est justement la famille facile.

## 5. Ce qui manque, nommément

1. un certificat de localité qui fonctionne sur les nappes — la restriction aux
   directions admissibles que j'avais proposée est **réfutée**, et sa forme
   corrigée est dans
   [`REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md`](../../audits/REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md) ;
2. `eight_clusters` aux trois tailles, ce qui suppose le point précédent ;
3. `unresolved_pair_mass` et le HWM, absents de ce ledger ;
4. un producteur output-sensitive : `623` candidats par support retenu sur la
   famille la plus favorable reste le vrai coût.

## 6. Provenance

Fichier brut : `rampe_j0.txt`, 78 lignes, conservé intégralement, y compris les
pistes rouges. Transcript de session : `transcript.txt`.

La rampe avait d'abord perdu son brut : le verdict rendait non nul, `set -e`
coupait, et le trap arrêtait la machine avant l'étape de reçu. La recette a été
corrigée — le reçu précède le verdict — et une session de récupération dédiée a
rapatrié le fichier depuis le disque de démarrage, qui persiste à l'arrêt.
