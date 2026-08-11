# Audit épinglé — `order_k_flats` à `9c587e6`

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> [!NOTE]
> Snapshot conservé parce que `prototype/order_k_flats.hpp` cite ses
> contre-fixtures. Il ne décrit pas l'implémentation courante.

## Descente de rayon réfutée

| identifiant | coordonnées |
| --- | --- |
| `A` | `(0,0,0)` |
| `B` | `(0,3,0)` |
| `C` | `(2,1,0)` |
| `P` | `(1,1,0)` |
| `Q` | `(1,1,2)` |

Dans `z=0`, `in_circle_coplanar(A,B,C,P)=-72`, donc `P` est strictement
intérieur, mais les quatre rayons sont égaux :

$$R^2(ABC)=R^2(ABP)=R^2(BCP)=R^2(CAP)=\frac{5}{2}.$$

Sur les 120 permutations du snapshot, 90 construisaient un germe et 30
rendaient `germe_non_certifie`. Autoriser l'égalité sans ordre bien fondé
peut cycler : il faut une construction planaire exacte avec terminaison
prouvée, ou une recherche exhaustive bornée, puis tester les permutations.

Le plafond historique `q*q+8` n'avait pas de preuve de complétude et
débordait un `int` signé dès `q>=46341`. Toute borne analogue doit être
justifiée et évaluée dans un type assez large.

GCP non utilisé.
