# Audit ciblé après `e7e4d5e` - profondeur avant coquille

Date : 17 août 2026.  
Pin audité : `e7e4d5e58dbc8d0c1b57137b1eba2a9706029328`.

## Verdict

Le census commun par `BallKey` est la bonne architecture. Le reçu indique toutefois que 98 % des boules uniques sont finalement écartées pour profondeur excessive, après avoir payé un census complet.

La primitive exacte nécessaire est déjà présente. Pour

```text
P(z)=A|z|^2+B.z+C, A>0,
```

`ball_census` calcule sur chaque boîte `Z` les extrema exacts `mn(Z)` et `mx(Z)`. Il faut en faire une passe de préflight de profondeur avant toute collecte de coquille.

## 1. Défaut transactionnel actuel

La fonction retourne dès que l'un des deux seuils est rencontré :

```text
|I_B| > interior_cap  -> boule inutile ;
|U_B| > shell_cap     -> resource_exhausted.
```

Si les deux seuils sont dépassés, le statut dépend donc de l'ordre des sous-arbres. Or une boule déjà trop profonde ne sera jamais développée en plateau : sa coquille n'a pas à être matérialisée ni plafonnée.

Le bon ordre logique est :

```text
profondeur d'abord ; coquille seulement pour une boule survivante.
```

## 2. Range-count exact déjà disponible

Les extrema sont séparables et exacts sur la grille entière. La passe de profondeur peut classifier un nœud ainsi :

```text
mx(Z) < 0   -> tout le nœud est strictement intérieur : créditer weight(Z) ;
mn(Z) >= 0  -> aucun intérieur : élaguer ;
autrement  -> descendre.
```

Strictesses indispensables : `mx<0`, car `mx=0` peut contenir une coquille ; `mn>=0`, car la coquille ne compte pas comme intérieur.

Arrêter dès que le compteur dépasse

```cpp
interior_cap = smax_eff - minimal_generator_arity;
```

conformément à l'audit sur le `K_max` dynamique.

## 3. Pipeline conseillé

```text
BallKey RLE
  -> depth_preflight exact par blocs
  -> si trop profond : abandon immédiat
  -> sinon census complet I_B/U_B avec shell_cap
  -> SpherePlateau.
```

La seconde passe ne concerne alors que les boules survivantes. Deux parcours sur environ 2 % des clés coûtent bien moins qu'un parcours complet sur 100 % des clés. Cette structure suit aussi naturellement `count -> preflight -> fill` pour le futur port GPU.

Une variante à une passe peut différer la décision `shell_overflow` jusqu'à connaître la profondeur. Elle corrige le statut, mais ne profite pas aussi clairement des crédits `mx<0` par sous-arbre.

## 4. Portes utiles

Graver une boule qui possède simultanément plus de `interior_cap` intérieurs et plus de `shell_cap` points de bord, répartis dans deux branches Morton. Exécuter les deux ordres d'enfants. Le verdict doit être dans les deux cas :

```text
dead_depth, jamais resource_exhausted.
```

Ajouter un second cas où la profondeur survit mais la coquille dépasse le cap : celui-ci doit rendre `resource_exhausted`.

Mutant :

```text
shell-cap-before-depth
```

qui rétablit le retour immédiat actuel.

Compteurs à publier :

```text
depth_nodes_visited,
depth_all_inside_credits,
balls_dead_depth,
full_census_balls,
full_census_nodes_visited.
```

L'attente structurelle sur `uniform,n=400` est que `full_census_balls/unique_balls` soit proche des 2 % de survivantes annoncées.

## Ordre utile

1. Corriger les `PointId` et le `K_max` dynamique.
2. Scinder immédiatement le census en préflight de profondeur puis remplissage `I_B/U_B`.
3. Refaire la mesure avant d'introduire un index plus complexe ou une pré-clé approximative q4.

## Conclusion

Le prochain gain ne demande pas un nouveau théorème. La forme primitive et l'arbre courant fournissent déjà un range-count exact de profondeur avec crédits de sous-arbres entiers. Il faut seulement décider si une boule est utile avant de payer sa coquille.