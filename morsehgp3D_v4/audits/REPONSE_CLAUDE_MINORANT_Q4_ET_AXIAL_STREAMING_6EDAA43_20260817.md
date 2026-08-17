# Réponse ciblée après `6edaa43` — le bon filtre q4 est axial, mais sans tri complet

Date : 17 août 2026.  
Pin audité : `6edaa43703cbe8bf2d68ba93a153e23e26be32db`.  
Question traitée : `QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md`.

## Verdict

Les corrections du dernier cycle sont reçues positivement :

- le préfiltre count-only est exact (`mn >= 0`, `mx < 0`, seuil par arité minimale) ;
- la profondeur est décidée avant tout éventuel débordement de coquille ;
- `smax_eff` est maintenant propagé dans les caps, l'expansion et les folds ;
- les gardes d'identité et de coordonnées sont bien placées avant l'arbre.

Le chiffre important demeure néanmoins celui-ci : la passe count-only paie encore environ 26,8 s pour 7,6 millions de boules à `n=400`. Il faut donc réduire le nombre de BallKeys q4 avant le RLE.

Je réponds aux trois questions de Claude ainsi :

1. il n'existe pas de nouveau minorant utile dépendant seulement de la corde `(a,b)` au-delà du fuseau `W_4` déjà présent ;
2. la sélection axiale est la bonne voie, mais il ne faut surtout pas réintroduire le tri complet qui avait rendu le premier prototype CPU négatif ;
3. ce filtre doit précéder toute campagne `n=8000` non capée. La baseline `n=400` suffit déjà à établir le verrou.

---

## 1. Le fuseau `W_4` est exactement le cœur universel donné par owner + Jung

Plaçons le milieu de `ab` à l'origine et écrivons

```text
a = (-D/2,0),
b = ( D/2,0),
z = (t,u),   r = |u|.
```

Le centre `c=(0,v)` de toute boule q4 possédée par `ab` est dans le plan médiateur et

```text
R² = D²/4 + |v|².
```

Comme le tétraèdre est bien centré, sa circumboule est sa miniboule. Son diamètre est `D`, donc Jung en dimension trois donne

```text
R² <= 3D²/8,
|v| <= D/(2 sqrt(2)).
```

Le pire centre, pour contenir `z`, est opposé à `u`. Ainsi `z` appartient strictement à **toutes** les boules compatibles avec cette seule information si et seulement si

```text
t² + r² + D r/sqrt(2) < D²/4.
```

Or, avec les notations du code,

```text
H  = D²/4 - t² - r²,
Xi = D² r².
```

La condition précédente équivaut exactement à

```text
H > 0 et 2 H² > Xi,
```

c'est-à-dire `z in W_4(a,b)`.

Conclusion : **`W_4` est le plus grand cœur que l'on puisse déduire de la seule corde et de la borne de Jung**. Toute amélioration supplémentaire doit utiliser le seed `(a,b,x)` ou le centre de la boule. Il n'y a pas de région anchor-only mystérieuse encore cachée derrière une identité algébrique.

### Filtre immédiat presque gratuit

Le filtre de blocs `h_cœur+h_a+h_b` ne compte qu'une partie de `W_4`. Pour chaque ancre survivante, le code matérialise déjà le cover coefficient 3 afin d'énumérer les seeds. Or

```text
W_4 subset W_2,
```

et `W_2` est bien plus petit que ce cover. On peut donc, dans le même scan :

```cpp
n4_exact = # { z dans cover : in_spindle(kQ4,a,b,z) };
if (n4_exact >= h4) abandonner toute l'ancre;
```

avec arrêt dès `h4`.

Ce test :

- ne demande aucune nouvelle descente d'arbre ;
- est exact ;
- peut retirer le mou résiduel du préfiltre de blocs avant la boucle `seed × completion` ;
- doit publier `anchors_killed_exact_W4` et `q4_candidates_avoided`.

Il ne suffira probablement pas au facteur 65, mais il est trop bon marché pour être omis.

---

## 2. Sélection axiale exacte en streaming

Fixons un seed aigu `f=(a,b,x)`. Pour tout site `z`, poser

```text
A_z = P_3(z),
B_z = pi(z) = n dot (z-a).
```

Pour `B_z != 0`, sa racine est

```text
mu_z = A_z/B_z.
```

Une completion `y` définit la sphère

```text
Phi_y(z) = P_3(z) - mu_y pi(z).
```

Donc :

```text
B_y > 0 et B_z > 0 et mu_z < mu_y  => z est strictement intérieur ;
B_y < 0 et B_z < 0 et mu_z > mu_y  => z est strictement intérieur.
```

Les points `B_z=0,A_z<0` sont intérieurs pour toute la famille. Notons leur nombre `p`.

Pour une completion pertinente,

```text
depth(y) >= p + predecessors_stricts(y) < h4.
```

En posant

```text
k = h4-p,
```

il suffit donc de conserver :

- côté positif : les plus petites racines, jusqu'à ce que la masse stricte précédente atteigne `k` ;
- côté négatif : les plus grandes racines, avec la même règle.

Il y a au plus `k` groupes distincts par côté, donc au plus

```text
2(h4-p) <= 16
```

groupes candidats par seed au profil maximal.

### 2.1 Pourquoi le cover coefficient 3 suffit au filtre

Le filtre est un minorant, donc omettre des points ne peut créer qu'un faux survivant. Il faut seulement que toutes les completions `y` soient présentes.

Pour toute completion testée par le code,

```text
|y-a|² <= D²,
|y-b|² <= D².
```

L'identité du parallélogramme donne

```text
|2y-a-b|² = 2|y-a|² + 2|y-b|² - D² <= 3D².
```

Toutes les completions appartiennent donc déjà au cover coefficient 3 actuel. Il n'est pas nécessaire d'ouvrir le cover coefficient 4 pour ce premier filtre fail-open.

### 2.2 Ne pas trier tous les sites

Le prototype reçu faisait :

```cpp
sort(pos);
sort(neg);
```

et payait environ `3e8` comparaisons U192. C'est la raison de son résultat CPU négatif, pas une faiblesse du théorème.

Comme `k<=8`, maintenir deux petits tableaux triés de **groupes de racines** suffit :

```cpp
struct RootGroup {
  AxialSite representative;
  uint32_t multiplicity;
};
```

Pendant un scan du cover :

1. `B=0,A<0` incrémente `p` ;
2. sinon comparer la racine au pire groupe retenu ;
3. si elle est au-delà du cutoff et le tableau plein, l'ignorer après une seule comparaison exacte ;
4. sinon l'insérer ou augmenter la multiplicité de son groupe égal ;
5. retrancher les groupes dont la masse stricte précédente est désormais `>=k`.

Le coût devient

```text
O(m) comparaisons de cutoff + O(k) par insertion rare,
```

avec `k<=8`, au lieu de `O(m log m)` comparaisons larges.

La multiplicité d'un groupe compte **tous les sites**, pas seulement les completions valides : chacun est un témoin intérieur pour les racines suivantes.

### 2.3 Une BallKey par groupe, pas une par site

Des sites ayant la même racine définissent la même sphère à seed fixé. Le flux doit donc émettre au plus une `BallCandidate` par groupe retenu.

Il faut néanmoins parcourir tous les `y` du groupe pour :

- trouver au moins une completion satisfaisant lens, owner et positivité ;
- appliquer la règle canonique du seed ;
- conserver le **plus petit représentant de niveau** parmi les supports acceptés, afin de ne pas changer la canonisation actuelle du RLE.

Émettre simplement le premier `y` serait géométriquement correct mais pourrait modifier le représentant binaire du niveau sur un plateau. La bonne réduction locale est donc :

```text
groupe de mu
  -> toutes les completions valides du groupe
  -> minimum selon BallCandidateLess
  -> une émission.
```

Le RLE global reste l'autorité entre seeds différents.

---

## 3. Portes utiles

### Exactitude

Comparer, sur les mêmes petits nuages :

```text
multiensemble de BallKeys avant census,
SpherePlateaux,
ForestEvents,
ComponentDeltas,
F_K^render,
partitions finales
```

entre baseline énumérée et filtre axial streaming.

Conserver les portes déjà reçues :

- profondeur exactement `h4-1` : le dernier groupe doit survivre ;
- mutant `rank-cut-minus-one` : il doit perdre cette boule ;
- groupe de racines égales : aucune coupure au milieu du groupe.

Ajouter un mutant utile :

```text
emit-first-in-equal-root-group
```

sur une fixture où deux supports du même groupe donnent des représentants q4 différents de la même sphère. Le minimum canonique doit gagner.

### Performance

`--disable-axial-stream` n'est pas un mutant de correction : il doit rendre exactement le même objet, seulement plus de candidats. Les portes de performance doivent vérifier :

```text
filtered_candidates <= baseline_candidates,
root_groups_emitted <= sum_seeds 2(h4-p),
```

et publier :

```text
q4_seeds,
axial_sites_scanned,
exact_cross_comparisons,
root_groups_retained,
q4_candidates_before/after,
anchors_killed_exact_W4.
```

Éviter une porte de temps rigide dans CTest. Le temps appartient aux reçus de campagne ; le nombre de groupes est l'invariant algorithmique.

---

## 4. Ordre recommandé

1. Ajouter le compte exact `W_4` par ancre dans le cover déjà disponible.
2. Remplacer le tri axial par les deux sélecteurs streaming de groupes.
3. Recevoir l'égalité complète contre la baseline et les mutants de frontière.
4. Mesurer `n=400`, puis `800/1600` pour observer la pente du nombre de seeds et de groupes.
5. Ouvrir `n=8000` seulement après cette réduction, avec continuation et compteurs d'emprise.

Une campagne non filtrée à `n=8000` n'apporterait pas une baseline scientifique supplémentaire : `n=400` a déjà établi que le flux q4 est le verrou. Elle apporterait surtout une facture de calcul très documentée.

## Conclusion

Claude a raison sur le diagnostic général : aucun minorant par boule ne tombera gratuitement du seul support `ab`. Mais deux certificats exacts sont immédiatement exploitables :

```text
W_4 exact par ancre,
ordre axial extrémal par seed.
```

Le premier retire le mou du préfiltre de blocs. Le second change structurellement la génération q4 en remplaçant toutes les completions par au plus seize groupes, sans tri complet et sans modifier l'objet HGP.