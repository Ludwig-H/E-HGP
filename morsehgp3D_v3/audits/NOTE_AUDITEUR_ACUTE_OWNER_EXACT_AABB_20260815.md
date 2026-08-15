# Note de l'auditeur — extrema AABB exacts pour l'owner du carrier aigu

Date : 15 août 2026 UTC.

Complément et amélioration de
[`NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md`](NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md).

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=quantized_u16_input_only`, `mode=acute_owner_exact_continuous_aabb`,
`public_status=not_claimed`.

> [!IMPORTANT]
> La première note rendait exacts les extrema de l'acuité
>
> ```text
> Phi=(a-x) dot (b-x),
> ```
>
> mais proposait seulement des intervalles sûrs pour les deux conditions
> d'owner. Ces deux extrema sont en réalité eux aussi **exacts sur les AABB
> continues**, avec une formule plus simple que l'enclosure de produits.
>
> Pour
>
> ```text
> D=||a-b||^2,
> E=||a-x||^2,
> X=||b-x||^2,
> ```
>
> les fonctions `D-E` et `D-X` sont des sommes séparables. Le carré de la
> variable commune s'annule, rendant l'expression linéaire dans cette variable.
> Il suffit donc d'évaluer ses deux extrémités et des distances carrées
> intervalle--point.
>
> Le gateway complet
>
> ```text
> NONE_ACUTE_OWNER / ALL_STRICT_OWNER / MIXED
> ```
>
> peut ainsi être exact sur l'enveloppe cartésienne de `A x B x C`, en i64, sans
> faux `NONE` ni dépendance d'intervalle inutile.

## 1. Primitive unidimensionnelle

Pour un intervalle fermé entier ou réel

```text
I=[l,h]
```

et un scalaire `t`, définir :

```text
d2_min(I,t) = min_{u in I} (u-t)^2,
d2_max(I,t) = max_{u in I} (u-t)^2.
```

Les formules exactes sont :

```text
d2_min(I,t) = 0                         si l<=t<=h,
              min((l-t)^2,(h-t)^2)      sinon;

d2_max(I,t) = max((l-t)^2,(h-t)^2).
```

Aucun flottant ni racine n'intervient.

## 2. Extrema exacts de `D-E`

Sur un axe :

```text
delta_E(a,b,x)
  = (a-b)^2-(a-x)^2
  = b^2-x^2-2a(b-x).
```

Pour `b,x` fixés, cette expression est linéaire en `a`. Son maximum et son
minimum sur `A=[Alo,Ahi]` sont donc atteints à une extrémité de `A`.

Pour `a` fixé, les variables `b` et `x` sont indépendantes :

```text
delta_E = (b-a)^2-(x-a)^2.
```

Ainsi, exactement :

```text
delta_E_hi
 = max_{a in {Alo,Ahi}}
     [d2_max(B,a)-d2_min(C,a)],

delta_E_lo
 = min_{a in {Alo,Ahi}}
     [d2_min(B,a)-d2_max(C,a)].
```

Le passage des extrema est licite directement :

```text
max_{a,b,x} = max_{b,x} max_{a endpoint}
            = max_{a endpoint} max_{b,x},
```

et de même pour le minimum.

## 3. Extrema exacts de `D-X`

Symétriquement :

```text
delta_X(a,b,x)
  = (a-b)^2-(b-x)^2
  = a^2-x^2-2b(a-x).
```

La variable commune est cette fois `b`. On obtient :

```text
delta_X_hi
 = max_{b in {Blo,Bhi}}
     [d2_max(A,b)-d2_min(C,b)],

delta_X_lo
 = min_{b in {Blo,Bhi}}
     [d2_min(A,b)-d2_max(C,b)].
```

## 4. Passage exact en dimension trois

Les AABB sont des produits cartésiens et les trois coordonnées sont
indépendantes. On peut donc choisir simultanément, axe par axe, les triplets qui
réalisent chaque extremum scalaire. Par conséquent :

```text
Delta_E_hi = sum_axis delta_E_hi(axis),
Delta_E_lo = sum_axis delta_E_lo(axis),
Delta_X_hi = sum_axis delta_X_hi(axis),
Delta_X_lo = sum_axis delta_X_lo(axis).
```

Ce sont les extrema exacts sur les boîtes **continues** `A x B x C`. Ils sont
donc des enclosures sûres des vrais points stockés, éventuellement plus larges
que leurs extrema discrets si les AABB sont peu remplies, mais sans relaxation
algébrique supplémentaire.

## 5. Classifieur complet

Réutiliser les extrema exacts de `Phi` de la première note :

```text
Phi_hi = max_{A x B x C} (a-x) dot (b-x),
Phi_lo = min_{A x B x C} (a-x) dot (b-x).
```

Puis :

```text
NONE_ACUTE_OWNER
  si Phi_hi <= 0
  ou Delta_E_hi < 0
  ou Delta_X_hi < 0;

ALL_STRICT_OWNER
  si Phi_lo > 0
  et Delta_E_lo > 0
  et Delta_X_lo > 0;

MIXED
  sinon.
```

Les signes sont volontairement asymétriques :

- `Phi` doit être strictement positif ; l'égalité est un angle droit, donc
  `NONE` ;
- `Delta_E/Delta_X` peuvent être nuls, car un tie de longueur peut encore être
  gagné par l'`EdgeKey` ; on ne rend donc `NONE` que pour un maximum strictement
  négatif ;
- `ALL` exige les deux différences strictement positives afin d'éviter tout tie
  au niveau bloc ; les égalités descendent jusqu'à un domaine où l'owner
  lexical est exact.

## 6. Largeurs entières

Sous coordonnées u16 :

```text
|u-t| <= 65535,
(u-t)^2 < 2^32.
```

Une différence de deux carrés tient sous 33 bits signés et la somme de trois
axes sous 35 bits signés. `int64_t` suffit largement pour les deux deltas.

Les extrema de `4*Phi` de la première note tiennent également en i64. Le
classifieur complet n'a donc besoin ni de i128, ni de division, ni de racine,
ni de flottant.

## 7. Pseudocode minimal

```cpp
struct D2Range { int64_t lo, hi; };

D2Range d2_interval_point(int64_t lo, int64_t hi, int64_t t) {
  const int64_t dl = lo - t;
  const int64_t dh = hi - t;
  const int64_t mx = std::max(dl * dl, dh * dh);
  const int64_t mn = (lo <= t && t <= hi)
                       ? 0
                       : std::min(dl * dl, dh * dh);
  return {mn, mx};
}

D2Range delta_E_axis(Box1 A, Box1 B, Box1 C) {
  int64_t lo = INT64_MAX, hi = INT64_MIN;
  for (int64_t a : {A.lo, A.hi}) {
    const auto db = d2_interval_point(B.lo, B.hi, a);
    const auto dc = d2_interval_point(C.lo, C.hi, a);
    lo = std::min(lo, db.lo - dc.hi);
    hi = std::max(hi, db.hi - dc.lo);
  }
  return {lo, hi};
}

D2Range delta_X_axis(Box1 A, Box1 B, Box1 C) {
  int64_t lo = INT64_MAX, hi = INT64_MIN;
  for (int64_t b : {B.lo, B.hi}) {
    const auto da = d2_interval_point(A.lo, A.hi, b);
    const auto dc = d2_interval_point(C.lo, C.hi, b);
    lo = std::min(lo, da.lo - dc.hi);
    hi = std::max(hi, da.hi - dc.lo);
  }
  return {lo, hi};
}
```

La version finale doit utiliser une primitive de carré sans overflow implicite
avant promotion. Sous le profil u16 validé, `int64_t` en entrée suffit ; la
porte doit néanmoins refuser toute coordonnée hors profil avant ce kernel.

## 8. Oracle et mutants

### Oracle exhaustif

Sur des petites boîtes entières :

1. énumérer tous les triplets `(a,b,x)` de points de grille ;
2. comparer les extrema discrets aux bornes continues ;
3. vérifier qu'ils sont inclus dans `[lo,hi]` ;
4. sur des intervalles où l'optimum continu est entier, exiger l'égalité ;
5. vérifier directement les verdicts `NONE/ALL` sur tous les triplets.

Une seconde autorité rationnelle peut optimiser les polynômes sur les boîtes
continues et exiger l'égalité exacte des formules.

### Mutants causaux

```text
owner-utilise-Dmax-moins-Emin,
owner-oublie-annulation-a2,
owner-evalue-seulement-Alo,
owner-min-distance-aux-seuls-bords,
owner-rejette-le-tie,
owner-all-accepte-le-tie.
```

La première mutation est particulièrement utile : soustraire deux intervalles
de distances calculés indépendamment perd la corrélation du point commun et
peut transformer des blocs évidents de `two_lines` en `MIXED`.

## 9. Conséquence d'implémentation

`AcuteCarrierGateway-q4` peut désormais être prototypé comme un microkernel
entièrement exact sur AABB :

```text
AcuteBox24/12 pour Phi
+ OwnerD2Exact pour Delta_E,Delta_X
= NONE / ALL_STRICT_OWNER / MIXED.
```

Il reste fail-open relativement aux vrais ensembles de points parce qu'une AABB
peut contenir des positions absentes, mais aucune largeur supplémentaire ne
vient de l'arithmétique. Cette précision est exactement celle requise pour que
`two_lines` meure au niveau bloc sans ouvrir ses `m^2` paires et pour que les
familles positives ne soient pas noyées dans des splits causés par une
soustraction d'intervalles trop grossière.
