# Addendum après `7420355` — deux filtres q4 purement `i64` avant même `q3_power`

Date : 19 août 2026.  
HEAD relu : `74203554f524a433b4da9b41134aee4d94571f4f`.

## Verdict

Le contre-audit `CONTRE_AUDIT_5B89BC_Q4_REUTILISER_Q3_POWER_20260819.md` est correct et couvre déjà la meilleure optimisation : tester d'abord

```cpp
q3_power(f3s, py) > 0
```

pour la face seed `abx`, puis mesurer l'apport marginal des autres puissances de faces.

Je n'ajoute ici que deux conséquences nécessaires du bien-centrage, calculables avec **les six longueurs carrées déjà en registre, sans multiplication large ni i128**. Elles sont plus faibles que la puissance de face, mais leur coût est de quelques `max/add/compare`; elles peuvent donc être placées avant elle.

---

## 1. Filtre du sommet `y`

Soit `c` le circumcentre strictement intérieur du tétraèdre et

\[
u_i=p_i-c,\qquad \|u_i\|=R.
\]

Comme `c` est intérieur, il existe des barycentriques strictement positives

\[
\sum_i \lambda_i u_i=0,\qquad \lambda_i>0.
\]

En prenant le produit scalaire avec `u_i`, il est impossible que tous les
`u_i·u_j`, `j!=i`, soient positifs ou nuls. Il existe donc `j` tel que

\[
u_i\cdot u_j<0.
\]

Alors

\[
d_{ij}^2=\|u_i-u_j\|^2=2R^2-2u_i\cdot u_j>2R^2.
\]

Si `D²=|a-b|²` est l'arête owner maximale, c'est une corde de la même sphère, donc

\[
D\le 2R,
\qquad 2R^2\ge D^2/2.
\]

Ainsi chaque sommet possède une arête incidente vérifiant

\[
\boxed{d_{ij}^2>D^2/2.}
\]

Pour `a,b`, c'est automatique grâce à `ab`. Pour `x`, c'est déjà impliqué par l'acuité stricte du seed `abx` :

\[
l_{ax}+l_{bx}>D^2
\]

donc l'une des deux dépasse `D²/2`.

La seule vérification nouvelle est donc :

```cpp
if (2 * std::max({l_ay, l_by, l_xy}) <= D2) {
    ++q4_rej_center_i64_vertex_y;
    return false;
}
```

Sous u16, `2*l` reste trivialement dans `i64`.

---

## 2. Filtre du couple `(x,y)`

Posons

\[
w=u_x+u_y.
\]

Comme `x,y` forment une arête du tétraèdre et que le circumcentre est strictement intérieur, ils ne peuvent pas être antipodaux : sinon `c` serait le milieu de l'arête `xy`, donc sur la frontière du tétraèdre.

Ainsi

\[
w\cdot u_x=w\cdot u_y
=2R^2-\frac{d_{xy}^2}{2}>0.
\]

Mais

\[
\sum_i\lambda_i(w\cdot u_i)=0.
\]

Comme les contributions de `x,y` sont strictement positives, au moins l'un de `a,b` doit vérifier

\[
w\cdot u_z<0.
\]

Or, pour `z∈{a,b}` :

\[
w\cdot u_z
=2R^2-\frac{d_{xz}^2+d_{yz}^2}{2}.
\]

Donc pour au moins l'un des deux sommets :

\[
d_{xz}^2+d_{yz}^2>4R^2\ge D^2.
\]

D'où la condition nécessaire :

\[
\boxed{
\max(l_{ax}+l_{ay},\ l_{bx}+l_{by})>D^2.
}
\]

Implémentation :

```cpp
if (std::max(l_ax + l_ay, l_bx + l_by) <= D2) {
    ++q4_rej_center_i64_pair_xy;
    return false;
}
```

Là encore, seulement deux additions et une comparaison.

---

## 3. Ordre conseillé dans l'entonnoir

Je testerais :

```text
self
→ lentille
→ owner
→ exact-once
→ det/volume nul
→ filtre i64 sommet y
→ filtre i64 couple xy
→ q3_power(f3s,y) / filtre affine certifié
→ autres puissances de faces si rentables
→ q4_form seulement pour les survivants
```

Les deux filtres i64 sont **nécessaires mais non suffisants**. Ils ne doivent jamais remplacer la porte de puissance de face; leur seule raison d'être est que leur coût marginal est presque nul.

La réception utile est donc empirique : ajouter

```text
q4_rej_center_i64_vertex_y
q4_rej_center_i64_pair_xy
```

et conserver le filtre seulement s'il retire une masse mesurable avant `q3_power` sans ralentir le banc apparié.

## Conclusion

L'idée structurante reste celle déjà documentée au HEAD : les puissances équatoriales de faces. Ces deux inégalités constituent seulement un étage zéro très bon marché. Si elles capturent même quelques pourcents des ~71 millions de rejets de centre à `n=8000`, leur rapport coût/bénéfice sera probablement excellent; sinon elles se suppriment sans toucher au reste de l'architecture.
