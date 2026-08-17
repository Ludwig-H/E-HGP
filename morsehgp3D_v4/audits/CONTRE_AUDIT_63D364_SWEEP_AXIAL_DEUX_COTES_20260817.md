# Contre-audit après `63d364a` — factoriser exactement la profondeur q4 par un sweep axial à deux côtés

Date : 17 août 2026.  
Pin de code audité : `63d364a46de1bbd5f02a98e259492021bf0c538b`.  
Pin de protocole également reçu : `3792d56db71399fcb6ac70cf1562589ae6a40107`.

## Verdict

Les développements récents sont reçus positivement.

- Le scan strict du cover par boule candidate est un minorant de profondeur sûr.
- Le compte exact de `W_4(a,b)` tue correctement une ancre entière avant `seed × completion`.
- La sélection axiale bornée conserve les groupes de frontière, choisit le minimum canonique par groupe et est appariée à la baseline.
- Le choix de laisser la baseline comme chemin CPU par défaut après le résultat `+7 %` à `n=1600` est honnête et correct.
- La campagne G4 est désormais transactionnelle et distingue latence isolée de débit sous contention.

Je ne trouve pas de nouvelle faute mathématique dans ces commits.

Il reste toutefois une factorisation exacte dans le chemin axial. Le code sélectionne actuellement les groupes par un rang sur leur propre côté, puis paie encore, pour chaque groupe retenu, un scan complet

```cpp
depth_dead(f4)
```

sur le cover. Ce second scan est redondant : la profondeur stricte sur le cover se lit exactement dans les deux ordres axiaux, positif et négatif, déjà calculés pour le seed.

Ce n'est pas une raison de promouvoir prématurément l'axial sur CPU. C'est la correction structurelle à tester avant de conclure définitivement que son surcoût ne peut pas être amorti, et c'est surtout la forme naturelle du futur kernel GPU.

---

## 1. Identité exacte de profondeur sur le cover

Fixons un seed aigu `(a,b,x)`. Pour tout site `z` du cover, poser

```text
A_z = P_3(z),
B_z = pi(z) = n dot (z-a).
```

Pour `B_z != 0`, définir

```text
mu_z = A_z / B_z.
```

La sphère de paramètre `mu` dans le faisceau du seed a pour forme

```text
Phi_mu(z) = A_z - mu B_z.
```

Comme son coefficient quadratique est positif, `Phi_mu(z) < 0` signifie exactement que `z` est strictement intérieur à la boule.

On obtient donc :

```text
B_z > 0 : z intérieur ssi mu_z < mu,
B_z < 0 : z intérieur ssi mu_z > mu,
B_z = 0 : z intérieur pour tout mu ssi A_z < 0.
```

Notons

```text
p = nombre de sites B_z = 0 et A_z < 0,
P_<(mu) = nombre de sites B_z > 0 et mu_z < mu,
N_>(mu) = nombre de sites B_z < 0 et mu_z > mu.
```

Alors le nombre exact d'intérieurs stricts de la boule de paramètre `mu` parmi les sites du cover est

```text
d_cover(mu) = p + P_<(mu) + N_>(mu).
```

Cette identité est exactement le résultat du scan actuel `q4_power(f4,z) < 0`, pas seulement une borne plus faible. Les points de même `mu` sont sur la coquille et sont exclus par les inégalités strictes.

Le cover n'est qu'un sous-ensemble du nuage, donc `d_cover` reste un minorant de la profondeur globale. Mais, relativement au cover déjà matérialisé, aucun second scan de puissance q4 n'est nécessaire.

---

## 2. Le préfixe unilatéral est nécessaire, pas suffisant

La sélection actuelle utilise :

```text
p + prédécesseurs stricts du même côté < h_4.
```

C'est un filtre fail-open correct. Il ignore cependant les témoins de l'autre côté du plan du seed.

Pour une completion `y` avec `B_y > 0`, les sites négatifs satisfaisant

```text
mu_z > mu_y
```

sont eux aussi strictement intérieurs à sa boule. Symétriquement, une completion négative reçoit les contributions positives situées avant elle.

Le code courant finit par les retrouver dans `depth_dead(f4)`. La bonne optimisation consiste donc à ne pas supprimer ce contrôle, mais à le remplacer par le sweep exact à deux côtés ci-dessus.

---

## 3. Intervalle viable et borne de `2(h_4-p)` groupes

Poser

```text
h = h_4,
k = h-p.
```

Si `p >= h`, le seed est mort.

Sinon, soit :

```text
U = k-ième plus petite racine positive, multiplicité comprise,
L = k-ième plus grande racine négative, multiplicité comprise.
```

Lorsqu'un côté contient moins de `k` racines, le seuil correspondant est infini.

Toute racine viable vérifie simultanément :

```text
mu <= U,
mu >= L.
```

En effet, `mu > U` implique déjà au moins `k` racines positives strictement antérieures ; `mu < L` implique au moins `k` racines négatives strictement postérieures.

Les racines candidates vivent donc dans

```text
[L,U].
```

Il y a au plus `k` groupes positifs distincts dans cet intervalle et au plus `k` groupes négatifs distincts. Après fusion des égalités entre côtés :

```text
nombre de groupes distincts <= 2k = 2(h_4-p) <= 16.
```

Au profil `K_max=5`, cette borne tombe à six.

Les égalités de frontière sont conservées entièrement : une forte multiplicité au seuil peut produire beaucoup de points de coquille, mais un seul groupe de racine.

---

## 4. Sweep exact recommandé

Après le balayage qui calcule les `(A_z,B_z)` :

1. compter `p` ;
2. déterminer `L` et `U` par les deux sélecteurs bornés déjà écrits ;
3. construire une table UNIQUE de groupes exacts de `mu` dans `[L,U]`, en fusionnant les membres des deux signes ;
4. trier ces au plus `2k` groupes par `mu` ;
5. stocker pour chaque groupe les multiplicités

   ```text
   n_pos[j], n_neg[j] ;
   ```

6. calculer les préfixes positifs et suffixes négatifs, saturés à `h` ;
7. pour le groupe `j`, décider exactement

   ```text
   d_j = p + pos_before[j] + neg_after[j].
   ```

Le terme constant contient les racines positives situées strictement avant `L` et les racines négatives situées strictement après `U`. Les groupes de même `mu` ne se comptent jamais eux-mêmes.

Si `d_j >= h`, supprimer le groupe avant :

```text
owner,
Cramer q4,
test du centre,
formation de BallKey,
scan depth_dead.
```

Pour chaque groupe restant :

- parcourir ses membres des deux signes ;
- trouver les completions satisfaisant les prédicats de support ;
- conserver le minimum `ball_candidate_less` ;
- émettre une seule candidate.

Le scan `depth_dead(bestf4)` devient alors inutile dans le chemin axial. Pendant la réception, il peut rester derrière une assertion de recoupement :

```text
d_j == nombre de q4_power < 0 dans le cover.
```

### Deux gains locaux supplémentaires

Le code courant construit les groupes séparément par côté. Une même sphère ayant des points de coquille des deux côtés peut donc être émise deux fois avant le RLE global. Une table commune par `mu` retire cette duplication.

Surtout, les groupes tués uniquement par les témoins du côté opposé ne paient plus `valid_completion` ni la formation q4. C'est le raccord que la sélection unilatérale actuelle ne peut pas effectuer.

---

## 5. Fixture entière qui discrimine le côté opposé

Prendre :

```text
a  = (10,10,10),
b  = (20,10,10),
x  = (15,17,10),
y  = (15,10,17),
z1 = (15,13, 9),
z2 = (14,13, 9),
z3 = (16,13, 9).
```

Le tétraèdre `{a,b,x,y}` possède `ab` :

```text
|ab|^2 = 100,
|ax|^2 = |bx|^2 = |ay|^2 = |by|^2 = 74,
|xy|^2 = 98.
```

Son circumcentre et son niveau sont :

```text
c = (15,82/7,82/7),
R^2 = 1513/49.
```

Les poids barycentriques de `c` sont :

```text
lambda_a = lambda_b = 25/98,
lambda_x = lambda_y = 12/49.
```

Ils sont tous strictement positifs : le support est bien q4.

Les trois points supplémentaires sont strictement intérieurs :

```text
|z1-c|^2 = 442/49,
|z2-c|^2 = |z3-c|^2 = 491/49,
```

tous inférieurs à `1513/49`.

Ils ne sont pourtant pas dans `W_4(a,b)`. Pour `z1` :

```text
H = 15,
Xi = 1000,
2H^2 = 450 < 1000.
```

Pour `z2,z3` :

```text
H = 14,
Xi = 1000,
2H^2 = 392 < 1000.
```

Le filtre exact par ancre ne tue donc pas ce cas.

Pour le seed `(a,b,x)`, la completion `y` est du côté positif et les trois `z_i` sont du côté négatif. Comme ils sont intérieurs à la boule de `y` :

```text
mu_zi > mu_y.
```

La completion `y` a zéro prédécesseur positif : le filtre unilatéral la conserve. Le sweep à deux côtés calcule immédiatement :

```text
d_cover(mu_y) = 3.
```

Exiger :

```text
smax=6, h_4=3 : groupe tué avant valid_completion/q4_form ;
smax=7, h_4=4 : groupe conservé, événement K=6 au niveau 1513/49.
```

Cette fixture isole exactement la contribution du côté opposé sans être absorbée par le filtre `W_4` de l'ancre.

---

## 6. Portes utiles

### 6.1 Identité du compteur

Sur les petits nuages jugés, pour chaque groupe retenu par l'axial :

```text
two_sided_depth == scan actuel q4_power < 0 sur le cover.
```

Comparer le nombre exact, pas seulement le verdict mort/vivant.

### 6.2 Égalité fonctionnelle

Comparer ancien axial contre sweep à deux côtés :

```text
candidats post-RLE : BallKey, arité, représentation,
I_B/U_B,
ForestEvents,
ComponentDelta,
RenderResult,
partition finale.
```

### 6.3 Groupes mixtes et ex æquo

Sur une cosphère, graver :

```text
même mu sur les deux côtés,
un seul groupe local,
une seule candidate locale,
tous les points égaux retrouvés dans U_B par le census.
```

### 6.4 Mutants

```text
axial-ignore-opposite-side,
axial-reverse-negative-inequality,
axial-depth-nonstrict,
axial-drop-boundary-ties,
axial-first-valid-representative.
```

Une fois le scan `depth_dead` retiré, les trois premiers deviennent des mutants de correction et doivent mourir sur les fixtures dédiées.

### 6.5 Mesures

Publier séparément :

```text
t_axial_AB,
t_thresholds,
t_grouping,
t_valid_completion,
t_depth_scan_legacy,
groupes_tues_par_cote_oppose,
doublons_locaux_de_mu_fusionnes.
```

Le verdict CPU ne doit être reconsidéré qu'après cette décomposition. Si le calcul de `(A,B)` demeure dominant, garder l'axial opt-in est la bonne décision.

---

## 7. Raccord avec le fold `sort/reduce`

Je reçois également le contre-audit `d0edac9` :

```text
existed_before_batch ssi first_touch < batch
```

permet bien de remplacer les maps de rôles par une table d'occurrences internée et des réductions segmentées. Le fold reste séquentiel entre niveaux, mais il n'a aucune raison de construire plusieurs arbres rouges-noirs à l'intérieur de chaque lot.

Les deux travaux sont indépendants :

```text
sweep axial deux côtés : réduit le coût par seed q4 ;
table d'incidences     : réduit le coût par événement conservé.
```

L'ordre raisonnable est :

1. intégrer le sweep exact dans le chemin axial opt-in et mesurer ;
2. poursuivre en parallèle la table d'incidences `sort/reduce`, qui est déjà le verrou CPU certain à `n=8000` ;
3. ne changer le chemin CPU par défaut que sur mesure isolée favorable ;
4. conserver l'axial borné comme candidat GPU même s'il reste légèrement négatif sur CPU.

## Conclusion

La sélection de `63d364a` est exacte et son résultat CPU négatif est honnêtement interprété. Il reste néanmoins un scan redondant dans cette expérience : la profondeur sur le cover est déjà entièrement codée par les deux ordres de racines du faisceau.

La formule

```text
d_cover(mu) = p + #positifs strictement avant mu + #négatifs strictement après mu
```

permet de fusionner les deux côtés, de borner le nombre de groupes à `2(h_4-p)`, de tuer les groupes par le côté opposé avant tout calcul q4 et de supprimer le scan de puissance par représentant.

Ce raffinement peut encore rester négatif sur CPU. Il doit être mesuré, pas cru. Mais c'est la version algorithmique complète du théorème axial, et la bonne interface pour décider proprement entre référence CPU et kernel GPU.
