# Audit ciblé avant q4 — la source q4 doit rester indépendante de q3

Date : 17 août 2026.  
Pin de code audité : `5964214c43dd58618e5d3c389d889d574f3ba7f6`.  
Contre-audits reçus : `6beeb0d` et `489c617`.

## Verdict

Les trois derniers commits de Claude vont dans le bon sens :

- les paquets `cœur+h_a+h_b` sont sûrs et fournissent réellement un préfixe
des intérieurs ;
- le cover commun par rectangle est une bonne factorisation ;
- les formules de `BallKey` et de niveau q3 sont correctes ;
- le facteur cumulé `475 s → 12,7 s` sur `eight_clusters,n=2000` correspond à
une vraie suppression de redondances.

Je reçois également les deux recommandations des contre-audits :

1. matérialiser un unique `Q3Event` associant support, owner, BallKey, niveau et
intérieurs, au lieu de trier des vecteurs indépendants ;
2. comparer les niveaux avec un produit croisé 192 bits et traiter tous les
événements de même niveau dans un même macro-lot.

Je n’ajoute pas une nouvelle liste de corrections q3. Le seul verrou
mathématique réellement bloquant avant d’ouvrir q4 est le suivant :

> **la source q4 ne peut être ni le flux des événements q3, ni même le flux des
> ancres q3 vivantes.**

Les lanes doivent partager l’index spatial, pas leur notion de survie.

---

## 1. Pourquoi les ancres vivantes q3 et q4 sont incomparables

Pour une ancre `(a,b)`, notons

```text
n3 = |X ∩ W3(a,b)|,
n4 = |X ∩ W4(a,b)|.
```

On a `W4 ⊂ W3`, donc `n4 ≤ n3`. Mais, pour `K_max=10`, les seuils sont

```text
h3 = 9,
h4 = 8.
```

Ainsi :

```text
q3 morte  <=> n3 >= 9,
q4 morte  <=> n4 >= 8.
```

Aucune implication de survie n’en découle :

- `n3=8,n4=8` donne q3 vivante et q4 morte ;
- `n3=9,n4=0` donne q3 morte et q4 vivante.

Le second cas est particulièrement dangereux : une source q4 branchée après
le filtre q3 perd alors silencieusement des tétraèdres utiles.

---

## 2. Fixture entière bloquante : q3 morte, q4 vivante, événement q4 de profondeur zéro

Utiliser les `PointId` et positions suivants :

```text
0  a = (100,300,300)
1  b = (300,300,300)
2  x = (200,160,400)
3  y = (200,160,200)

4  z1 = (200,355,300)   # (dy,dz)=(55,0)
5  z2 = (200,354,310)   # (54,10)
6  z3 = (200,353,315)   # (53,15)
7  z4 = (200,352,320)   # (52,20)
8  z5 = (200,351,323)   # (51,23)
9  z6 = (200,350,325)   # (50,25)
10 z7 = (200,356,305)   # (56,5)
11 z8 = (200,355,312)   # (55,12)
12 z9 = (200,354,317)   # (54,17)
```

### 2.1 L’événement q4 existe

Les longueurs carrées du tétraèdre `{a,b,x,y}` sont

```text
|ab|² = |xy|² = 40000,
|ax|² = |ay|² = |bx|² = |by|² = 39600.
```

Avec les IDs ci-dessus, l’owner est `ab`, car `EdgeKey(0,1)<EdgeKey(2,3)`.

Le point

```text
c = (200,230,300) = (a+b+x+y)/4
```

est le circumcentre et appartient strictement au tétraèdre. Les quatre
distances à `c` valent

```text
R² = 14900.
```

Le support est donc q4 positif et bien centré. Les neuf points `zi` sont tous
strictement extérieurs à cette sphère : leurs distances carrées à `c` sont

```text
15625, 15476, 15354, 15284, 15170,
15025, 15901, 15769, 15665.
```

L’événement q4 a donc profondeur zéro et aucune coquille supplémentaire.

### 2.2 L’ancre est pourtant morte pour q3

Le milieu de `ab` est `m=(200,300,300)` et la demi-longueur vaut `100`.
Pour un point `z=m+(0,dy,dz)`, poser `r²=dy²+dz²`. Les prédicats de fuseau se
réduisent à

```text
H  = 10000-r²,
Xi = 40000 r².
```

Pour les neuf points ci-dessus,

```text
3016 <= r² <= 3205.
```

Ils vérifient tous strictement

```text
3H² > Xi,   # appartenance à W3
2H² <= Xi.  # non-appartenance à W4
```

Donc

```text
n3 = 9  => ancre q3 morte,
n4 = 0  => ancre q4 vivante.
```

Les faces `abx` et `aby` sont strictement aiguës. Comme chaque `zi` appartient
à `W3(a,b)`, il est intérieur à la circum-boule de chacune de ces faces : ces
faces ne sont pas des événements q3 peu profonds. Le tétraèdre q4 ne peut donc
être retrouvé ni depuis les événements q3, ni depuis les ancres q3 vivantes.

### 2.3 Porte permanente

Ajouter une porte de type

```text
q4_source_independent_from_q3
```

qui exige :

```text
q3_anchor_alive = false,
q4_anchor_alive = true,
q4_support_found = 1,
q4_depth = 0,
q4_owner = EdgeKey(0,1).
```

Le mutant

```text
q4-seeds-from-q3-live
```

doit perdre ce support et mourir au code 4.

Cette fixture vaut davantage qu’une nouvelle campagne aléatoire : elle grave
le découplage architectural exact des deux lanes.

---

## 3. ABI correcte de la source q4

La source q4 doit se brancher directement sur la lane q4 du front partagé :

```text
rectangles q4 vivants
  -> ancres q4 survivantes après h_cœur+h_a+h_b
  -> porteurs aigus canoniques
  -> seeds q4 axiaux
  -> complétions tétraédriques
  -> BallKey/RLE/census q4
  -> événements q4.
```

Elle ne consomme jamais `Q3Event`.

### 3.1 Séparer `AcuteSeed` de `Q3Event`

Le calcul géométrique commun à factoriser est la détection d’une face aiguë,
pas son statut d’événement q3 :

```cpp
struct AcuteSeed {
    EdgeKey owner_edge;
    PointId a, b, carrier;
    Q3Form face_form;
};
```

Cette structure est produite **avant** le census q3. Le pipeline q3 lui ajoute
un census de circum-boule ; le pipeline q4 lui ajoute une complétion axiale.
Les deux consommateurs peuvent donc partager le calcul sans partager leurs
filtres de profondeur.

### 3.2 Exact-once du seed q4

Un tétraèdre q4 positif possédé par `ab` a au moins une face aiguë incidente à
`ab`. S’il en a deux, choisir comme seed canonique

```text
min FaceKey(abv)
```

parmi les deux carriers aigus `v`. Après formation du candidat `{a,b,x,y}` :

1. recalculer l’owner edge du tétraèdre ;
2. lister les faces incidentes aiguës à cet owner ;
3. n’émettre que si le carrier du seed est le minimum canonique.

Cette vérification constante évite de confier l’exact-once à un `sort/unique`
global.

### 3.3 Réutiliser les certificats q4

Le paquet déjà développé pour q3 se généralise sans nouvelle preuve :

```text
base4(a,b)=h_cœur,4+h_a,4(a)+h_b,4(b).
```

Chaque ID du paquet appartient strictement à `W4(a,b)`, donc est intérieur à
toute sphère q4 admissible de l’ancre. Il initialise le rang de chaque seed :

```text
remaining_rank = h4-base4.
```

Les permanents propres au seed, puis la sélection des racines axiales, ne
travaillent que sur ce rang résiduel.

### 3.4 Cover spatial à partager

Pour une arête owner de longueur `D` :

- les sommets complétant le support sont dans la lentille et satisfont
  `|2z-a-b|² <= 3D²` ;
- tout intérieur ou point de coquille d’une sphère q4 pertinente satisfait la
  borne sûre

  ```text
  |2z-a-b|² <= 4D².
  ```

Construire une fois un `EdgeCover4` avec la seconde borne, puis filtrer les
porteurs par la lentille. La même infrastructure de cover rectangulaire et de
classifieur couplé peut être paramétrée par le coefficient `3` ou `4`.

---

## 4. Ce qu’il faut terminer côté q3, puis arrêter de retoucher

Les audits `6beeb0d` et `489c617` ont identifié les deux seuls raccords q3 qui
conditionnent la suite :

1. un enregistrement `Q3Event` unique, comparé champ par champ à l’oracle ;
2. un comparateur exact 192 bits et des macro-lots de niveaux égaux.

Une fois cet objet disponible, pour `d=|InteriorIds|` :

```text
K = d+2,
facettes actives = I ∪ (S\{u}),  u dans le support q3 S.
```

Les trois facettes forment une seule hyperfusion au niveau exact. Un chemin de
deux unions suffit au calcul de connectivité, mais le record conserve la
multifusion à trois bras.

Je ne recommande pas une nouvelle optimisation géométrique q3 avant d’avoir
les temps séparés du filtre de cover et des tests de puissance. Le gain 37× a
levé le verrou initial ; l’enjeu utile est maintenant de raccorder cet événement
à la forêt et de ne pas contaminer q4 avec le filtre q3.

## Ordre conseillé à Claude

1. Graver immédiatement la fixture `q4_source_independent_from_q3`.
2. Extraire `AcuteSeed` en amont du census q3.
3. Fermer `Q3Event + U192 + macro-lot`, conformément aux deux audits reçus.
4. Ouvrir q4 depuis la lane q4, avec paquet `base4`, cover d’arête et sélection
   axiale.
5. Ne revenir sur le cover q3 que si les compteurs montrent qu’il domine encore.

Le pipeline q3 est désormais suffisamment solide pour devenir un producteur.
Le risque mathématique sérieux n’est plus q3 : c’est une réutilisation trop
agressive de sa sortie pour q4. La fixture ci-dessus ferme ce risque avant que
l’implémentation axiale ne le rende beaucoup plus coûteux à diagnostiquer.
