# Addendum : budgets mixtes et comparaison de centres en deux temps

7 octobre 2026. Claude, auditeur de contre-lecture. Ce document contre-lit les deux points du § 9 du
[contrat numérique révisé](../docs/CONTRAT_NUMERIQUE.md) (`8865e32c1`, blob `39872129beac`), et relève un dernier usage
absolu de la v11 (`CST-0114`). Aucun code exécuté hormis la porte `reference/test_witness_t1.py`, rejouée sous
`python3 -S -O` : 0 conforme, 4 sur le mutant `sans_inclusion`, 2 sur une option inconnue.

```text
phase=exploration_v12_hors_registre
public_status=not_claimed
GCP non utilisé
```

## 1. Budgets mixtes : justes

Hypothèses : $M=2^{s}$ est l'étendue du support, l'origine $o$ est un site du support, et l'on a $D<24M^{4}$ et
$\lvert N_j\rvert<24M^{5}$ (q3, `budgets.hpp` de la v11). q4 et q2 restent en dessous.

- **Côté d'un site gardé.**
  - La garde donne $\lvert z_j-o_j\rvert<3M$, puisque $o$ est dans la boîte du support.
  - On a $D\lvert z-o\rvert^{2}<24M^{4}\cdot 27M^{2}=648M^{6}$ et $\lvert 2N\cdot(z-o)\rvert<432M^{6}$. Chaque somme
    partielle reste sous le total $1080M^{6}<2^{11}M^{6}$.
  - D'où $6s+11$ bits : natif `i128` jusqu'à $s=19$, `i64` jusqu'à $s=8$.
- **Orientation du centre par rapport à trois sites.** L'expression est celle de la v11 (`center_orientation`),
  $\sum_j (N_j+D(o_j-p_j))\,\mathrm{normale}_j$.
  - Sites de coquille : votre majorant $11\,520M^{7}<2^{14}M^{7}$ est juste.
  - Il vaut aussi pour **trois sites gardés quelconques**, pas seulement de coquille. Les écarts entre sites du pavé
    sont inférieurs à $5M$, d'où une normale de moins de $50M^{2}$ par composante. On a aussi
    $\lvert N_j+D(o_j-p_j)\rvert<24M^{5}+72M^{5}=96M^{5}$. Le total est inférieur à $14\,400M^{7}<2^{14}M^{7}$.
  - La ligne de la table peut donc dire « sites gardés » : $7s+14$, natif jusqu'à $s=16$.
- **`strictly_inside`** (`center_inside` de la v11) ajoute, par face, l'orientation du quatrième sommet : quatre sites,
  sans centre. Sur des sites gardés, $6\cdot(5M)^{3}<2^{10}M^{3}$, soit $3s+10$ bits, natif `i64` jusqu'à $s=17$.
  `strictly_acute` vaut $2s+7$ bits. Ces deux expressions manquent à la table, sans enjeu de seuil.

## 2. Comparaison de centres en deux temps : juste

- **Même ordre que `compare_centers`.** Pour $c_j=I_j+f_j$, avec $I_j=o_j+\lfloor N_j/D\rfloor$ et
  $f_j=r_j/D \in [0,1)$, la décomposition est unique. On compare d'abord les $I_j$, puis $r_jD'$ et $r'_jD$, axe par
  axe dans l'ordre lexicographique, ce qui donne le même ordre que `compare_centers`.
- **Largeurs.**
  - $I_j$ tient sur 64 bits signés, puisque $\lvert c_j-o_j\rvert<M$.
  - Le **dividende** $N_j$ du plancher fait $5s+5$ bits en q3 : entier large au-delà de $s=24$, même si le quotient est
    petit.
  - Les produits croisés font moins de $8s+10$ bits pour $s$ le plus grand des deux repères : natifs jusqu'à $s=14$.
- **Préconditions.** $D>0$ (signe normalisé en q4) et plancher mathématique pour $N_j<0$ (la v11 a `floor_division`).

## 3. `CST-0114` : un troisième usage absolu dans la v11

Le test du milieu calcule en coordonnées **absolues**
$2(Do_j+N_j)=D(a_j+b_j)$, avec un `static_assert` à $5B+7\leq 127$, donc $B\leq 24$. Il sert à trouver un support à deux
sites pendant la canonicalisation, sur l'hôte (`src/catalogue/support.cpp`, l. 25, puis `src/num/predicates.cpp`,
l. 235) comme sur l'appareil (`src/catalogue/leaf_device_predicates.hpp`, l. 176). Il est donc sur le chemin du
catalogue.

- **Forme locale** : $2N_j=D\,((a_j-o_j)+(b_j-o_j))$. Elle tient en $5s+8$ bits pour des sites de coquille ou gardés,
  et en $5s+6$ bits dans le repère d'une feuille.
- **Autres tests vérifiés.** Le test « centre dans la boîte » ($N+D(o-\text{borne})$) est déjà local. Il ne reste,
  hors `LatticeSphere` et `compare_centers`, aucun autre usage du centre absolu dans `src/num` et `src/catalogue`.
