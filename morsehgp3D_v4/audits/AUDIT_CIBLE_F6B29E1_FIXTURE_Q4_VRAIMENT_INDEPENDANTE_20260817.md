# Audit ciblé après `f6b29e1` — renforcer la fixture d'indépendance q4

Date : 17 août 2026.  
Pin audité : `f6b29e1a57ab9385f54e800e7941dc5e58712158` inclus.

Je ne relève aucun verrou utile dans le nouvel enregistrement `Q3Event`, le comparateur de niveaux `U192` ou le durcissement de l'oracle q3. Les formules, les largeurs et les portes ajoutées sont cohérentes.

Un seul point doit être corrigé avant l'ouverture réelle de q4 : la fixture actuelle prouve que l'ancre finale `(a,b)` est q3-morte et q4-vivante, mais elle ne prouve pas que le tétraèdre est invisible depuis le **flux des événements q3**, contrairement à son commentaire.

## 1. La fixture actuelle contient encore deux événements q3 de profondeur zéro

Le tétraèdre est

```text
a=(100,300,300), b=(300,300,300),
x=(200,160,400), y=(200,160,200).
```

Les faces `abx` et `aby` sont effectivement rendues q3-profondes par les neuf points `z_i` déjà gravés. Mais les deux autres faces `axy` et `bxy` sont elles aussi strictement aiguës, avec arête owner `xy`, et restent des événements q3 peu profonds dans le nuage actuel.

Leurs circumcentres et leur rayon carré sont

```text
c_axy = (6175/37, 7635/37, 300),
c_bxy = (8625/37, 7635/37, 300),
r_face² = 490050/37.
```

Pour le quatrième sommet opposé, la puissance normalisée vaut

```text
|b-c_axy|²-r_face² = 490000/37 > 0,
|a-c_bxy|²-r_face² = 490000/37 > 0.
```

Pour chacun des neuf `z_i` existants, elle est également strictement positive, entre

```text
337125/37 et 380037/37.
```

Ainsi `axy` et `bxy` ont profondeur q3 nulle, aucune coquille supplémentaire et sont bien des événements q3. Une source q4 consommant des événements q3 peut donc encore voir le tétraèdre par ces faces. Le mutant actuel `q4-seeds-from-q3-live` ne simule que la survie de l'ancre `(a,b)` ; il ne tue pas une source fondée sur le flux complet des événements q3.

Ce n'est pas un défaut de la géométrie q4 proposée. C'est seulement une porte plus faible que le contrat annoncé, mais ce contrat est précisément celui qu'il faut fermer avant que q4 ne dépende de la mauvaise source.

## 2. Renforcement entier minimal de la fixture

Conserver les treize points actuels et ajouter les neuf points

```text
w_j = (196+j, 105, 300),  j=0,...,8.
```

Écrivons `t=j-4`, donc `t` varie de `-4` à `4`.

### 2.1 Ils rendent les deux faces restantes q3-profondes

Pour la face `axy`, on a exactement

```text
37 (|w_j-c_axy|²-r_face²) = 37t² + 2450t - 69425 <= -59033 < 0.
```

Pour la face `bxy`,

```text
37 (|w_j-c_bxy|²-r_face²) = 37t² - 2450t - 69425 <= -59033 < 0.
```

Les neuf `w_j` sont donc strictement intérieurs aux deux circum-boules. Les faces `axy` et `bxy` ont désormais profondeur au moins neuf et ne sont plus des événements q3 pour `h_3=9`.

Avec les neuf `z_i` hauts déjà présents, les quatre faces du tétraèdre sont alors q3-profondes : aucune face ne fournit un `Q3Event`.

### 2.2 L'ancre `xy` devient elle aussi q3-morte mais q4-vivante

Le milieu de `xy` est `(200,160,300)` et sa demi-longueur vaut `100`. Pour `w_j`, la distance radiale carrée au milieu vaut

```text
r_j² = 55²+t², donc 3025 <= r_j² <= 3041.
```

Avec `H=10000-r_j²` et `Xi=40000 r_j²`, on obtient sur toute la plage

```text
3H²-Xi >= 23643043 > 0,
2H²-Xi <= -23698750 < 0.
```

Ainsi les neuf `w_j` appartiennent à `W_3(x,y) sans W_4(x,y)`. L'ancre `xy` est donc q3-morte et q4-vivante, exactement comme l'ancre `ab` avec les neuf `z_i` hauts.

Les deux seules arêtes maximales possibles des quatre faces, `ab` et `xy`, sont maintenant toutes deux q3-mortes.

### 2.3 L'événement q4 reste vide et régulier

Le circumcentre q4 reste

```text
c=(200,230,300), R²=14900.
```

Pour chaque `w_j`,

```text
|w_j-c|² = 125²+t²,
```

soit une valeur entre `15625` et `15641`, strictement supérieure à `14900`. Les neuf nouveaux points sont donc extérieurs à la sphère q4, sans coquille. Avec les neuf `z_i` existants, l'événement q4 conserve profondeur zéro.

## 3. Porte réellement discriminante

La fixture renforcée doit vérifier explicitement :

1. les quatre faces `abx`, `aby`, `axy`, `bxy` sont aiguës et ont chacune une profondeur q3 `>=h_3` ;
2. aucune de ces faces n'est publiée comme `Q3Event` ;
3. les deux ancres maximales `ab` et `xy` sont q3-mortes mais q4-vivantes ;
4. le support q4 reste positif, owner `ab`, profondeur zéro et sans extra-shell ;
5. un mutant `q4-seeds-from-q3-events` perd le support ;
6. le mutant `q4-seeds-from-q3-live` perd également le support.

La porte actuelle peut rester comme micro-fixture du découplage des lanes sur une ancre. Mais elle ne doit plus être décrite comme une preuve d'invisibilité depuis les événements q3. La version à vingt-deux points ferme ce contrat sans changer la stratégie recommandée : produire `AcuteSeed` avant tout census q3 et ouvrir q4 directement depuis sa propre lane vivante.

## Conclusion

Le pipeline q3, son événement complet, l'ordre exact des niveaux et l'oracle sont suffisamment solides pour ne plus être retouchés ici. Le seul risque mathématique sérieux avant q4 est une source accidentellement dépendante de q3. La fixture actuelle ne tue qu'une moitié de ce risque ; les neuf points `w_j` ci-dessus la rendent réellement bloquante pour les événements q3 comme pour les ancres q3 vivantes.
