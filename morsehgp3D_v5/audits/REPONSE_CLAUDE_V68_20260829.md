# Réponse Claude — V68 tranchée : la vacuité est la LENTILLE à 82–97 %, et la moitié non classée l'est aussi (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Pin de mesure : `1ff39ab9`, `worktree_modifie=non`, quatre familles, $n = 8000$,
3 000 blocs à pas constant, `mhgp5_block_witness_probe`.

## 1. La cause réelle de la vacuité, décomposée

`is_acute_seed` se décompose exactement en **lentille** → **acuité stricte** →
**owner canonique**. En attribuant chaque bloc vide à l'étage le plus profond
qu'un de ses rôles atteint, on partitionne la vacuité réelle sans recouvrement :

| cause réelle | `scanline` | `terrain` | `uniform` | `eight_clusters` |
|---|---|---|---|---|
| aucun rôle | 0 | 7 | 0 | 0 |
| **lentille** | **1 218 (82 %)** | **1 309 (82 %)** | **1 555 (96 %)** | **1 551 (97 %)** |
| acuité | 248 (17 %) | 269 (17 %) | 65 (4 %) | 50 (3 %) |
| owner | 16 (1 %) | 6 (0,4 %) | 7 (0,4 %) | 1 (0,06 %) |

**Votre hypothèse d'une cause structurelle unique (`NONE_OWNER` ou identités
distinctes) est réfutée par la mesure :** l'owner explique de 0,06 % à 1 % de la
vacuité, jamais davantage. C'est la **lentille** qui explique presque tout.

## 2. Et la moitié non classée est de la lentille elle aussi

| parmi les NON classés par les boîtes | `scanline` | `terrain` | `uniform` | `eight_clusters` |
|---|---|---|---|---|
| **lentille** | 482 (65 %) | 548 (69 %) | **755 (91 %)** | **716 (93 %)** |
| acuité | 238 (32 %) | 238 (30 %) | 65 (8 %) | 50 (7 %) |
| owner | 16 (2 %) | 6 (0,8 %) | 7 (0,8 %) | 1 (0,1 %) |

**Ce n'est donc pas une cause manquante, c'est un lâche de boîtes sur le test
qui marche déjà.** La moitié non reconnue est majoritairement de la lentille que
le certificat de boîtes ne parvient pas à prouver à la granularité du handle.
Votre seconde hypothèse — « le lâche des boîtes suffit à l'expliquer » — est la
bonne.

## 3. Une borne meilleure existe, et elle est prouvablement INERTE ici

J'ai cru tenir le correctif. Le test découplé compare deux minima atteints en
des points différents ; l'identité du parallélogramme les couple :

$$\lVert x-a \rVert^{2} + \lVert x-b \rVert^{2} = 2\lVert x-m \rVert^{2} + \frac{D^{2}}{2},$$

d'où $\max(\lVert x-a \rVert^{2}, \lVert x-b \rVert^{2}) \ge \lVert x-m \rVert^{2} + D^{2}/4$.
La lentille exige ce maximum $\le D^{2}$, donc la condition **nécessaire**
$\lVert 2x-a-b \rVert^{2} \le 3D^{2}$ — le coefficient 3 du cover, **une seule
quantité séparable** au lieu de deux.

Je l'ai implémentée. **Elle ne change aucun des quatre comptes**, et la cause
est démontrable, pas empirique : `rect_cover_handles` élague sur
`gap2 > coef * dmax2` avec `coef = 3`, où `gap2` est **exactement** ce
$\mathrm{lower}\lVert 2x-a-b \rVert^{2}$ et `dmax2` **exactement** ce
$\mathrm{upper}\,D^{2}$. **Le test couplé EST le critère de sélection des
handles** : aucun handle retourné ne peut le satisfaire. Ma « meilleure borne »
est celle qui a produit la liste que j'essayais de filtrer.

Le code garde le commentaire et pas le test : un mort dans une sonde vaudrait
une seconde implémentation fantôme.

## 4. Ce que cela dit du premier incrément

Trois conséquences, toutes négatives pour ce que je proposais en V69 :

1. **`NONE_ACUTE` est à retirer de la nomenclature de boîtes** : 0 à 31 blocs
   sur quatre familles, et l'acuité réelle n'explique que 3 à 17 % de la
   vacuité ;
2. **`NONE_OWNER` ne mérite pas un fate de boîtes** : 0,06 % à 1 % ;
3. **`NONE_MAX_EDGE` est le seul qui morde, il plafonne à ≈ 50 %, et ce
   plafond ne se relève pas par une meilleure borne au même grain.** Le reste
   de la lentille n'est prouvable qu'en **découpant $C$** — c'est-à-dire en
   payant un split, pas un test.

Autrement dit : la vacuité est massivement une seule cause, la moitié en est
gratuite, et l'autre moitié coûte un split. **Ce n'est plus une question de
certificat, c'est une question de politique de subdivision**, et son coût doit
être mesuré avant d'être promis — ce que votre V64 disait déjà.

## 5. Questions

- **V70.** Étant donné le § 3, retirez-vous `NONE_ACUTE` et `NONE_OWNER` de la
  nomenclature des fates de boîtes, ou les gardez-vous comme emplacements que
  q4 remplirait différemment ?
- **V71.** Le plafond de ≈ 50 % à granularité de handle se relève-t-il par un
  **split borné de $C$** (un ou deux niveaux, comme le raffinement
  post-séparation que j'avais mesuré à +34 % de mur pour un gain nul) ? J'ai
  déjà payé une fois une subdivision qui tuait plus tôt ce qui mourait déjà :
  avant de recommencer, je voudrais votre avis sur ce qui distingue ce cas-ci
  du précédent.
- **V72.** Confirmez-vous que la bonne unité de décision reste le compteur
  d'appels réellement évités par étage, et non le nombre de blocs classés ?
  C'est ce qui m'a déjà trompé deux fois.
