# Contre-audit de `879b37d` — la borne flottante dynamique est sûre

Date : 18 août 2026.  
Pin de code audité : `879b37d4c5987ff7b5d95a4122eaab72d76d0c00`.  
Note de réconciliation reçue : `90f8dc67ea7fe2c13cd5ab466c266224a4bed823`.

## Verdict

Je reçois positivement les développements récents :

- le fold compact conserve la sémantique des macro-lots, des canoniques,
  des naissances, des croissances et des multifusions ;
- le cœur de seed de Jung reste exact ;
- le filtre flottant du **signe de `q3_power`** est fail-closed : il ne
  remplace l'exact que lorsqu'une borne d'erreur certifie le signe ;
- le repli exact reste l'autorité sur la bande d'incertitude.

Le raffinement introduit par `879b37d` était explicitement soumis à
contre-audit :

```text
E_f = 2^-48 · (G_d S_max + ||W_d||_1 v_max),
S_max = 2 D²,
v_max = sqrt(S_max)+1.
```

Cette borne est **mathématiquement sûre** pour le programme FMA figé dans
`q3_power_float_sign`, sous le contrat annoncé : binaire64, valeurs finies,
arrondi au plus proche et absence de `fast-math`. Le facteur `2^-48` est
nettement plus large que nécessaire ; il n'y a donc pas de correction de
production à demander.

---

## 1. Le majorant géométrique `S_max = 2D²` est correct

Le cover de coefficient trois satisfait

```text
|2z-a-b|² <= 3D²,
D² = |b-a|².
```

Avec `m=(a+b)/2`, cela donne

```text
|z-m| <= sqrt(3) D / 2,
|a-m| = D/2.
```

Donc

```text
|z-a|²
 <= ((sqrt(3)+1)/2)² D²
 = (1 + sqrt(3)/2) D²
 < 2D².
```

Ainsi, pour `v=z-a`,

```text
S=|v|² < S_max=2D²,
|v_i| <= sqrt(S_max).
```

Le `+1` dans `v_max` absorbe très largement l'arrondi éventuel de la
racine carrée. Sous le profil u16, `S_max<2^35`, donc `S_max` est représenté
exactement en binaire64 et toutes les valeurs restent très loin de tout
sous-flux ou débordement flottant.

---

## 2. Erreur des conversions de coefficients

Posons

```text
u = 2^-53,
g = fl(G),
w_i = fl(W_i).
```

Pour un entier non nul converti en binaire64 sous arrondi au plus proche,

```text
|g-G| <= u |G|,
|w_i-W_i| <= u |W_i|.
```

En exprimant cette erreur avec les coefficients flottants eux-mêmes :

```text
|g-G| <= u/(1-u) |g|,
|w_i-W_i| <= u/(1-u) |w_i|.
```

Pour un site fixé, définir

```text
M(v) = |g| S + sum_i |w_i| |v_i|.
```

L'erreur provenant des seules conversions est donc bornée par

```text
E_coeff <= u/(1-u) M(v).
```

Cette formulation traite correctement les fortes annulations : la borne est
absolue, fondée sur la somme des modules, jamais relative à la petite valeur
finale de `P`.

---

## 3. Erreur du programme FMA réellement exécuté

Le code calcule exactement la séquence suivante :

```cpp
r0 = fl(w0 * v0);
r1 = fma(w1, v1, r0);
r2 = fma(w2, v2, r1);
ph = fma(g, S, -r2);
```

Il y a donc quatre arrondis : une multiplication et trois FMA. Le lemme
classique des produits de facteurs `(1+delta)` donne

```text
|ph - (gS - sum_i w_i v_i)| <= gamma_4 M(v),
gamma_4 = 4u/(1-4u).
```

En ajoutant l'erreur de conversion :

```text
|ph-P|
 <= (gamma_4 + u/(1-u)) M(v)
 < 6u M(v).
```

La constante six est volontairement arrondie vers le haut. La constante
réelle est voisine de cinq.

---

## 4. Le seuil calculé par le code reste supérieur à `31u M(v)`

Pour tout site du cover,

```text
M(v)
 <= |g| S_max + (|w0|+|w1|+|w2|) v_max
 =: M_max.
```

Le code évalue cette expression uniquement avec des opérations positives :

```cpp
w1 = fabs(w0)+fabs(w1)+fabs(w2);
Mhat = fma(g,S_max,w1*v_max);
bound = 2^-48 * Mhat;
```

Les deux additions, la multiplication et la FMA peuvent arrondir légèrement
vers le bas, mais sans annulation. On a conservativement

```text
Mhat >= (1-u)^4 M_max.
```

La multiplication par `2^-48`, puissance de deux, est exacte ici. Comme

```text
2^-48 = 32u,
```

on obtient

```text
bound >= 32u(1-u)^4 M_max > 31u M(v).
```

Or l'erreur totale démontrée est `<6u M(v)`. La marge est donc supérieure à
un facteur cinq :

```text
ph < -bound  => P < 0,
ph >  bound  => P > 0.
```

Cette preuve reçoit également l'usage du même filtre dans le cœur q4 : un
`P>0` certifié peut être écarté ; lorsqu'il faut comparer `2P²` à `JB²`, le
code recalcule encore `P` exactement. Aucun carré d'approximation flottante
n'entre actuellement dans une décision de Jung.

---

## 5. Ce que la porte doit continuer à garantir

Les mesures `kFloatVerify` sont utiles, mais elles deviennent une porte de
régression de l'implémentation, pas le fondement de la preuve. Elles doivent
continuer à exercer :

```text
signes positifs certifiés,
signes négatifs certifiés,
replis exacts,
P exact nul sous forte annulation,
formes proches des largeurs u16 maximales.
```

Le mutant `float-threshold-too-small` est causal : il détruit la marge
prouvée et permet au bruit d'arrondi de certifier un exact nul.

Deux contrats doivent rester explicites dans le build CPU et le futur build
CUDA :

```text
pas de fast-math ;
séquence FMA non réassociée.
```

Si le moteur devient une bibliothèque appelée depuis un environnement qui
peut modifier le mode d'arrondi, vérifier `FE_TONEAREST` à l'entrée est le
raccord le plus propre. Ce n'est pas nécessaire pour les exécutables actuels,
qui démarrent dans le mode standard.

---

## 6. Suite : ne pas réutiliser aveuglément cette borne pour Jung ou les quotients

La réception ci-dessus porte seulement sur le signe affine de

```text
P = G|v|²-W·v.
```

Pour les prochains étages :

- Jung doit utiliser un intervalle `P in [ph-E_P,ph+E_P]`, puis propager les
  bornes dans `2P²-JB²` ;
- l'ordre axial doit borner séparément le déterminant
  `A1B2-A2B1` ;
- une valeur non certifiée doit toujours retomber sur U192/U320 exact.

La note de réconciliation `90f8dc6` adopte déjà cet ordre. Je confirme donc
qu'il n'est pas nécessaire de remplacer le seuil dynamique actuel avant de
passer au kernel affine par ancre et aux intervalles de Jung.

## Conclusion

Le filtre de `879b37d` n'est pas seulement empiriquement sans désaccord : sa
constante dynamique est rigoureusement conservatrice pour le programme
arithmétique effectivement écrit.

L'erreur totale est inférieure à `6u M`, tandis que le seuil utilisé reste
supérieur à `31u M`. Le facteur `2^-48` peut donc être conservé. Les futurs
filtres de Jung et de quotient devront recevoir leurs propres preuves ; la
prudence humaine consistant à réutiliser une constante qui a bien marché une
fois n'est pas, à ce jour, un théorème d'analyse numérique.
