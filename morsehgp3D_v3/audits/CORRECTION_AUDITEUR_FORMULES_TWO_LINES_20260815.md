# Correction de l'auditeur — domaine des formules exactes de `two_lines`

Date : 15 août 2026 UTC.

Cette note corrige la section 2 de
[`AUDIT_RECEPTION_SCISSION_CAP_5CE2634_20260815.md`](AUDIT_RECEPTION_SCISSION_CAP_5CE2634_20260815.md).

> [!CAUTION]
> Les trois formules publiées y sont exactes sur toute la rampe mesurée
> `n=300/600/1200/2400`, mais leur domaine a été énoncé trop largement.
> Pour q3/q4, le seul fait `m<=H` ne garantit pas que toutes les paires croisées
> soient `W`-vivantes. Les conditions exactes ci-dessous doivent accompagner le
> golden analytique.

## 1. Famille

Posons `n=2m` et

```text
A_i=(i,0,0),
B_j=(0,j,H),
1<=i,j<=m.
```

Les seuils sont

```text
h_2=10,
h_3=9,
h_4=8.
```

## 2. Témoins croisés q3/q4

Pour la paire `(A_i,B_j)` et un témoin `A_k` avec `k<i`, le prédicat ponctuel
se réduit à

```text
q3 : 3 k^2 > j^2+H^2,
q4 : 2 k^2 > j^2+H^2.
```

Pour un témoin `B_l` avec `l<j`, on obtient symétriquement

```text
q3 : 3 l^2 > i^2+H^2,
q4 : 2 l^2 > i^2+H^2.
```

Le plus grand indice possible d'un témoin situé avant son endpoint est `m-1`,
et le second indice minimal vaut un. Ainsi aucune paire croisée ne possède de
témoin q3 si et seulement si

```text
3 (m-1)^2 <= H^2+1,
```

et aucune n'en possède en q4 si et seulement si

```text
2 (m-1)^2 <= H^2+1.
```

Sous ces conditions, les `m^2` paires croisées sont toutes vivantes dans la
lane correspondante.

## 3. Formules corrigées et leurs domaines

Pour `m>=10`, q2 reste exactement

```text
V_2 = 10 n - 55.
```

Cette formule ne dépend pas de `H` : une paire croisée `(A_i,B_j)` possède
exactement `i+j-2` témoins q2 et survit si `i+j<=11`, soit cinquante-cinq paires
croisées.

Pour q3, si `m>=9` et

```text
3 (m-1)^2 <= H^2+1,
```

alors

```text
V_3 = n^2/4 + 9 n - 90.
```

Pour q4, si `m>=8` et

```text
2 (m-1)^2 <= H^2+1,
```

alors

```text
V_4 = n^2/4 + 8 n - 72.
```

Les termes linéaires sont les paires vivantes internes aux deux droites : sur
une droite de `m` points, leur nombre vaut

```text
h m - h(h+1)/2.
```

## 4. Domaine du générateur courant

Avec `H=65535` :

```text
q3 : m<=37837, donc n<=75674,
q4 : m<=46341, donc n<=92682.
```

Les quatre tailles publiées sont donc très largement dans le domaine exact.
Le golden analytique peut les remplacer sans réserve, à condition que la porte
vérifie aussi l'hypothèse sur `(m,H)` ou refuse au-delà.

Lorsque l'hypothèse échoue, la famille reste une contre-famille géométrique
utile, mais les comptes q3/q4 ne sont plus donnés par les polynômes ci-dessus :
des témoins de la même droite que l'un des endpoints entrent alors dans le
fuseau de certaines paires croisées.

## 5. Ce qui ne change pas

La preuve d'absence de **carrier aigu owner** donnée dans
[`NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md`](NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md)
ne dépend pas de ces inégalités ni de la hauteur `H` :

- pour un carrier placé avant l'endpoint de sa droite, `Phi<=0` ;
- pour un carrier placé après, l'arête croisée `ab` n'est pas maximale.

Ainsi la cible architecturale reste bien

```text
W-vivant potentiellement quadratique,
carrier aigu owner exactement nul,
```

sur tout le domaine valide du générateur, même lorsque la formule fermée du
premier compteur change.
