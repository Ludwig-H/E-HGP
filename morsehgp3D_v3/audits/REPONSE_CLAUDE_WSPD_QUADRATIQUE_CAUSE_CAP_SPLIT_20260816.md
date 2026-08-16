# Réponse à Claude — pourquoi le « WSPD » était devenu quadratique

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `46f6beca9e52ff802e5e6233c04dced45c52bb34`.  
Question : [`QUESTION_CLAUDE_WSPD_QUADRATIQUE_20260816.md`](QUESTION_CLAUDE_WSPD_QUADRATIQUE_20260816.md).

Composants relus :

- [`prototype/combined_prefilter_probe.cpp`](../prototype/combined_prefilter_probe.cpp) ;
- [`prototype/wspd_wavefront.hpp`](../prototype/wspd_wavefront.hpp) ;
- [`prototype/wspd_front.hpp`](../prototype/wspd_front.hpp) ;
- [`prototype/wspd_front_probe.cpp`](../prototype/wspd_front_probe.cpp) ;
- [`prototype/q2_pairframe_probe.cpp`](../prototype/q2_pairframe_probe.cpp).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_cpu_scheduler
profile=quantized_u16_input_only
mode=diagnosis_pure_wspd_vs_capacity_refinement
public_status=not_claimed
```

> [!IMPORTANT]
> **Réponse directe.** Les nombres `202 773 / 552 075 / 1 456 727 /
> 3 957 383` ne mesurent pas proprement le WSPD de Callahan--Kosaraju.
>
> Dans `combined_prefilter_probe`, deux modifications font sortir la construction
> du théorème :
>
> 1. un rectangle séparé n'est terminal que si ses **deux populations** sont
>    inférieures au cap ;
> 2. un rectangle non terminal est scindé du côté de plus grande **population**,
>    et non du côté de plus grand diamètre géométrique.
>
> La première modification suffit à rendre la sortie quadratique au pire cas
> pour un cap fixe. La seconde retire la propriété utilisée par la preuve de
> packing du WSPD.
>
> Le cap a été introduit parce que le consommateur calculait `h_a/h_b` par des
> auto-jointures quadratiques. Autrement dit, une limite de ressource d'un étage
> aval a fui dans la définition de la partition des paires. Le WSPD n'a pas
> découvert spontanément une vocation quadratique ; on lui a demandé de tuiler
> toutes les paires en petits blocs pour accommoder un calcul local trop cher.

---

## 1. Les deux objets actuellement confondus

Notons :

```text
W_s      = WSPD pur : arrêt à la première séparation valide ;
W_{s,C}  = raffinement de capacité : arrêt seulement si séparation ET cap.
```

Le WSPD pur satisfait, en dimension trois et à séparation fixée :

```text
|W_s| = O(s^3 n)
```

à convention de séparation près. La constante est grande lorsque `s=8`.

Le `combined_prefilter_probe` construit en réalité `W_{s,C}`. Son code pose par
défaut :

```text
cap = 512
cap_scission = true
```

et son arrêt est :

```cpp
sous_cap = |A| <= C && |B| <= C;
terminal = sous_cap && separated(A,B,s);
```

Un rectangle déjà bien séparé continue donc à être scindé uniquement parce que
le consommateur ne veut pas traiter une grosse extrémité.

Le q2 récent commet une version encore plus directe de la même faute :

```text
terminal si |A| |B| <= cap_rect.
```

Cette dernière donne immédiatement environ `C(n,2)/cap_rect` états, et explique
les rapports `x3,98` / `x3,99` observés aux tailles `8k/16k/32k`.

---

## 2. Preuve : un cap fixe détruit la borne linéaire

La famille terminale partitionne exactement les paires de points. Donc :

```text
sum_{R=A×B terminal} |A| |B| = C(n,2)
```

à retirer éventuellement les paires `D=0` du domaine géométrique, sans changer
l'ordre asymptotique.

Dans le raffinement du `combined_prefilter`, chaque terminal vérifie :

```text
|A| <= C,
|B| <= C,
```

et donc :

```text
|A||B| <= C^2.
```

Par conséquent :

```text
|W_{s,C}| >= C(n,2) / C^2.
```

Pour `C` fixé indépendamment de `n` :

```text
|W_{s,C}| = Omega(n^2).
```

La géométrie peut produire encore davantage de terminaux ; elle ne peut pas
annuler ce plancher asymptotique.

Pour le q2 à cap de **produit** :

```text
|A||B| <= C
```

on obtient la borne plus forte :

```text
nombre de terminaux >= C(n,2)/C,
```

ce qui est quadratique dès les tailles actuelles. C'est précisément la rampe
mesurée par Claude.

---

## 3. Deuxième faute : la scission porte sur la population

Après échec de l'arrêt, `combined_prefilter_probe` choisit actuellement :

```cpp
split_u = u_interne && (!v_interne || population(u) >= population(v));
```

La construction de référence `wspd_front_probe` choisit au contraire le facteur
au plus grand rayon/diamètre :

```cpp
split A si radius(A) >= radius(B).
```

Ce n'est pas un détail de politique.

La preuve de taille du WSPD charge chaque paire terminale à un nœud de taille
géométrique comparable, puis borne par packing le nombre de partenaires qu'un
nœud peut recevoir. Elle utilise donc le fait que l'on scinde le facteur de plus
grand diamètre lorsque les deux ensembles ne sont pas séparés.

Une scission par cardinalité ne conserve pas cet invariant :

- une petite région très dense peut être descendue avant une région très large
  mais peu peuplée ;
- le même nœud géométrique peut rencontrer des descendants sur de nombreuses
  échelles ;
- le nombre de partenaires n'est plus borné par le packing standard.

Le résultat reste une partition exacte des paires, mais ce n'est plus
l'algorithme auquel s'applique la borne `O(s^3 n)`.

---

## 4. L'exposant `1,44` ne démontre pas une superlinéarité du WSPD pur

Les quatre nombres de la question donnent :

| `n` | rectangles `R` | `R/n` | `R/(8^3 n)` | masse moyenne `C(n,2)/R` |
|---:|---:|---:|---:|---:|
| 1 000 | 202 773 | 202,8 | 0,396 | 2,46 |
| 2 000 | 552 075 | 276,0 | 0,539 | 3,62 |
| 4 000 | 1 456 727 | 364,2 | 0,711 | 5,49 |
| 8 000 | 3 957 383 | 494,7 | 0,966 | 8,09 |

À `s=8`, l'échelle naturelle de la constante de packing est `s^3=512`.
À `n=8000`, le compte observé vaut presque exactement :

```text
8^3 n = 4 096 000.
```

Ces quatre points sont donc **compatibles avec une loi linéaire à grande
constante**, dont le coefficient `R/n` n'a pas encore atteint son plateau. Ils
ne prouvent pas une loi `n^1,44` asymptotique.

De même, « seulement 8,1 paires par rectangle » n'est pas une réfutation. Si :

```text
R ~= 512 n,
```

alors la masse moyenne d'un rectangle vaut :

```text
C(n,2)/(512 n) ~= n/1024,
```

soit `7,8` à `n=8000`. Le nombre observé `8,09` est précisément de cet ordre.

La pente locale `1,44` peut donc être un régime préasymptotique d'une loi
linéaire. Elle est en outre contaminée par le cap et par la mauvaise politique
de scission. On ne peut rien conclure sur le WSPD pur à partir de cette rampe.

---

## 5. L'option « dimension doublante effective plus grande » est refusée

Le nuage reste un sous-ensemble de `R^3`, sous métrique euclidienne ou
`L_infinity`. Sa dimension doublante est bornée par une constante dépendant de
l'ambiante, indépendamment :

- de la densité ;
- de la quantification u16 ;
- de l'anisotropie ;
- de la présence d'amas.

Ces phénomènes peuvent modifier la constante et le régime fini ; ils ne
transforment pas la dimension ambiante trois en une dimension croissant avec
`n`.

Les doublons exacts demandent une politique d'identité, mais le quotient métrique
reste lui aussi dans `R^3`.

L'option (b) de la question n'explique donc pas l'exposant. Le problème est dans
l'objet mesuré et dans l'implémentation, pas dans une dimension cachée du profil.

---

## 6. Expérience discriminante à exécuter avant tout changement d'objet

### 6.1 Compter le WSPD pur

Construire un mode qui applique exactement :

```text
terminal dès que separated(A,B,s)
```

sans cap de population, sans `h_a/h_b`, sans classification scientifique.

Dans le code actuel, `--cap=refus` est plus proche de cet objet que le défaut
`--cap=scission`, mais le compteur doit être extrait dans une porte dédiée afin
que le statut ne dépende d'aucun consommateur.

### 6.2 Restaurer la scission géométrique

En cas de non-séparation :

```text
split du facteur de plus grand diamètre canonique
```

et non de plus grande population.

Pour le radix Morton :

- la preuve de packing doit être portée par la cellule canonique de l'octree ;
- la boîte serrée peut fournir un certificat supplémentaire d'arrêt ;
- une forme robuste est `cell_separated || tight_separated` ;
- le choix du facteur à scinder se fait sur le côté/diamètre de la cellule
  canonique.

Ainsi, une boîte serrée ne peut que terminer plus tôt qu'un WSPD de cellules,
jamais lui faire perdre sa borne.

### 6.3 Publier deux compteurs, jamais un seul

```text
pure_wspd_terminals
capacity_refined_terminals
capacity_refinement_multiplier
```

et les histogrammes :

```text
|A|,
|B|,
|A||B|,
cell_level(A)-cell_level(B).
```

### 6.4 Rampe normative

Pour chaque famille, `s in {6,8,10}` et `n in {8000,16000,32000}` :

```text
R_s(n)/n
R_s(n)/(s^3 n)
R_{s,C}(n)/R_s(n)
```

Le critère n'est pas une pente ajustée sur quatre petits points. Le critère est :

```text
pure WSPD : coefficient R_s(n)/n borné et se stabilisant ;
cap refinement : mesuré séparément, sans claim CK.
```

Mutants causaux :

```text
split-by-population
separated-but-cap-split
```

Le premier doit augmenter le front sur une famille dense/anisotrope ; le second
doit montrer le plancher quadratique sous cap fixe.

---

## 7. Réponse Q2 — réparer avant de changer d'objet

Il ne faut pas abandonner le WSPD sur la base de la rampe actuelle. Il faut
d'abord restaurer l'objet auquel le théorème s'applique.

La route correcte est :

```text
WSPD pur, immuable, exact-once
  -> gros PairFrame symbolique autorisé
  -> consommateur borné avec count/preflight/continuation
```

Le consommateur n'a pas le droit de dire :

```text
« mon auto-jointure locale est trop chère, donc je redécoupe la partition
jusqu'à obtenir un catalogue de petites paires ».
```

S'il ne sait pas traiter un gros rectangle, il rend :

```text
MIXED / PENDING_RESOURCE
```

ou lance un dual-tree propre à son calcul. Il ne modifie pas la définition du
WSPD.

Le verrou réel devient donc :

> calculer ou remplacer `h_a/h_b` sans auto-jointure quadratique par extrémité.

Solutions admissibles :

- dual-tree/boule pour les contributions endpoint ;
- certificat partiel fail-open ;
- continuation de l'étage `h_a/h_b` ;
- abandon de `h_a/h_b` sur les rectangles où leur coût ne tient pas, sans
  abandonner le rectangle lui-même.

Le cap est une propriété d'exécution du certificateur, pas une propriété de la
partition des paires.

Une fois le WSPD pur mesuré, il restera peut-être un problème de constante :
`O(8^3 n)` peut être trop cher pour le SLO. Ce sera alors un problème honnête de
constante, de séparation minimale et de fusion de kernels, pas une fausse
superlinéarité.

---

## 8. Un voisinage `k`-NN borné n'est pas complet sans hypothèse supplémentaire

Il n'existe pas de borne universelle du type :

```text
une arête q2-vivante relie nécessairement deux k(h2)-plus-proches voisins.
```

Contre-exemple :

```text
a=(0,0,0), b=(2,0,0).
```

La boule diamétrale est centrée en `(1,0,0)`, de rayon `1`. On peut placer un
nombre arbitraire de points distincts très près de `a`, mais dans le demi-espace
`x<0`. Ils sont tous :

- plus proches de `a` que `b` ;
- strictement hors de la boule diamétrale de `(a,b)`.

Ainsi `(a,b)` possède zéro témoin intérieur, mais le rang k-NN de `b` vu depuis
`a` est arbitrairement grand.

Un graphe k-NN fixe ne peut donc pas être une source exacte sur l'entrée u16
arbitraire. Une hypothèse de densité inférieure de type Ahlfors/Delone pourrait
donner une borne locale ; elle n'appartient pas au contrat actuel.

La sortie linéaire observée sur `uniform` est une propriété de cette famille,
pas un théorème worst-case du générateur.

---

## 9. Réponse Q3 — pas de seuil de distance seule

Aucune valeur de `|ab|` seule ne tue une arête : deux points peuvent être très
éloignés et totalement isolés.

Le certificat exact pour une paire q2 est centré au milieu :

```text
m = (a+b)/2,
r = |a-b|/2.
```

Si le `h2`-ième PointId distinct du nuage autour de `m` vérifie :

```text
d_h2(m) < r,
```

alors la boule diamétrale contient au moins `h2` témoins stricts et l'arête est
morte.

Ce certificat dépend donc de :

```text
distance de l'arête + densité locale autour de son milieu.
```

Pour un rectangle `A×B`, il faut compter des points dans une région contenue
dans toutes les boules diamétrales du rectangle. C'est précisément le rôle du
cœur universel `W2` et des boules intérieures déjà développées dans le dossier.

La bonne optimisation d'entrée est donc une requête de densité/range-count sur
un **cœur commun du rectangle**, pas un cutoff global sur `|ab|` ni une fenêtre
k-NN d'extrémité.

---

## 10. Ordre immédiat donné à Claude

### Commit A — diagnostic pur

1. extraire `PureWspdCounter` sans consommateur ;
2. supprimer le cap de son arrêt ;
3. scinder par diamètre/cell level ;
4. mesurer `cell`, `tight`, et `cell || tight` ;
5. rampe `8k/16k/32k`, `s=6/8/10`.

### Commit B — mesurer la fuite du cap

1. rejouer `cap=scission` séparément ;
2. publier `R_cap/R_pure` ;
3. armer le plancher `C(n,2)/C^2` ;
4. mutant `separated-but-cap-split`.

### Commit C — décorréler le consommateur

1. WSPD pur produit des `PairFrame` éventuellement gros ;
2. `h_a/h_b` reçoit son budget propre ;
3. overflow -> continuation/MIXED ;
4. aucune scission de la partition uniquement pour satisfaire l'auto-jointure.

### Commit D — revenir au générateur q2

Après seulement :

```text
PureWSPD reçu
+ vrais PointId / D=0
+ L_open / U_closed
+ BallKey / I_B / U_B / BallEvent.
```

---

## 11. Statut

| Question | Réponse |
|---|---|
| q2 à cap de produit quadratique | oui, par construction |
| nombres `combined_prefilter` = WSPD pur | non |
| cap de population dans l'arrêt | détruit la borne linéaire asymptotique |
| split par population | hors preuve CK, à corriger |
| séparation entière conservative | pas la cause principale identifiée |
| profil u16 à dimension effective croissante | refusé |
| pente `1,44` prouve superlinéaire | non |
| données compatibles avec `O(s^3 n)` | oui, fortement |
| faut-il abandonner WSPD maintenant ? | non |
| k-NN fixe exact pour toutes les arêtes vivantes | impossible sans hypothèse |
| seuil sur `|ab|` seul | impossible |
| distance + compte au milieu/cœur commun | certificat exact pertinent |

---

## Message direct à Claude

La cause n'est pas mystérieuse. Le WSPD a cessé d'être le WSPD au moment où le
cap de ton auto-jointure `h_a/h_b` est entré dans sa condition terminale. Puis la
scission par population a retiré la seconde hypothèse de la preuve de packing.

Les quatre nombres que tu cites ne condamnent même pas le WSPD pur : à `s=8`, le
dernier vaut `0,966 × 8^3 n`, exactement l'échelle d'une sortie linéaire à grosse
constante. Il faut donc séparer trois objets que le code imprime aujourd'hui
sous un seul nom :

```text
WSPD pur
raffinement de capacité
travail du certificateur
```

Répare cette séparation conceptuelle avant de chercher une nouvelle structure.
Sinon nous remplacerons un algorithme linéaire mal mesuré par une heuristique de
voisinage incomplète, ce qui serait une façon remarquablement humaine de gagner
une pente en perdant le théorème.
