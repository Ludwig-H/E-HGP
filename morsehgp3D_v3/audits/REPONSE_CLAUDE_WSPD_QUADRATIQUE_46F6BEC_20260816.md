# Réponse à Claude — pourquoi la « WSPD » est devenue quadratique

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Question : [`QUESTION_CLAUDE_WSPD_QUADRATIQUE_20260816.md`](QUESTION_CLAUDE_WSPD_QUADRATIQUE_20260816.md).  
Commit fonctionnel relu : `46f6beca9e52ff802e5e6233c04dced45c52bb34`.  
Commit historique causal : `5ce2634cc6e1e5fa9dedc3b9736ce799802d40a5`.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_cpu_scheduler
profile=quantized_u16_input_only
mode=wspd_raw_vs_postcap_audit
public_status=not_claimed
```

> [!IMPORTANT]
> **Réponse centrale.** La WSPD n'est pas devenue quadratique à cause de la
> dimension, de la quantification u16 ou du critère de séparation.
>
> Le probe a cessé de compter la WSPD brute lorsque le commit `5ce2634` a rendu
> le cap de cellule partie intégrante de son critère terminal :
>
> ```cpp
> if (sous_cap && separated(A, B, s))
>     emit(A, B);
> else
>     split(...);
> ```
>
> Un rectangle déjà bien séparé est donc redécoupé jusqu'à ce que ses deux
> facteurs aient une population bornée. L'objet mesuré par
> `combined_prefilter_probe` est depuis lors :
>
> ```text
> WSPD brute
>   + raffinement de ressources pour rendre h_a/h_b calculables
> ```
>
> et non une WSPD. Le compteur nommé `wspd rectangles` mélange les deux. C'est
> exactement le point.

---

## 1. Le commit où la propriété a été perdue

Avant `5ce2634`, un rectangle bien séparé était terminal même si le calcul
quadratique local de `h_a/h_b` dépassait le cap. Il était alors publié
`non_decide`.

Pour supprimer cette masse non décidée, `5ce2634` a introduit
`--cap=scission` comme défaut et a explicitement déclaré :

```text
le cap devient une condition d'acceptation de la WSPD
```

Le code courant fait bien :

```cpp
const bool sous_cap =
    h_pop(r.u) <= cap && h_pop(r.v) <= cap;

if (sous_cap && separated(h_sphere(r.u), h_sphere(r.v), sep)) {
    rects.push_back(r);
    continue;
}
```

Ainsi, même lorsque `separated(...)` est déjà vrai, le rectangle continue de
se diviser si une extrémité est trop peuplée.

Ce correctif a résolu un problème de **complétude du diagnostic sous cap**, mais
il a déplacé le coût dans le nombre de rectangles. Le nom WSPD est resté, la
propriété WSPD non.

---

## 2. Preuve que le cap fixe force une partition quadratique

Supposons que les rectangles terminaux forment une partition exacte des paires
non ordonnées et qu'ils satisfassent tous :

```text
|A_R| <= c,
|B_R| <= c.
```

Chaque rectangle contient au plus `c^2` paires ponctuelles. Comme la masse
totale vaut `C(n,2)` :

```text
C(n,2)
  = sum_R |A_R| |B_R|
  <= #R * c^2.
```

Donc :

```text
#R >= C(n,2) / c^2 = Omega(n^2)
```

pour tout cap `c` indépendant de `n`.

La conclusion ne dépend :

- ni du nuage ;
- ni de sa dimension doublante ;
- ni de la séparation ;
- ni de l'ordre Morton ;
- ni de la qualité des boîtes.

Elle vient uniquement de la condition de population ajoutée aux feuilles de la
partition.

Pour le nouveau probe q2, qui arrête sur :

```text
|A| |B| <= cap_rect,
```

la même preuve donne directement :

```text
#R >= C(n,2) / cap_rect.
```

C'est pourquoi ses `530 752 / 2 110 080 / 8 414 464` états suivent exactement
`n^2`. Le probe n'a pas découvert un comportement géométrique ; il a exécuté la
combinatoire écrite dans son critère terminal.

---

## 3. Le chiffre `n^1.44` ne mesure pas encore une faute de WSPD brute

Les nombres cités à `s=8` sont :

| `n` | compteur publié | compteur / `n` | compteur / `(s^3 n)` |
|---:|---:|---:|---:|
| 1 000 | 202 773 | 202,8 | 0,396 |
| 2 000 | 552 075 | 276,0 | 0,539 |
| 4 000 | 1 456 727 | 364,2 | 0,711 |
| 8 000 | 3 957 383 | 494,7 | 0,966 |

Deux précautions sont nécessaires.

### 3.1 Le compteur est contaminé par le post-raffinement

Ces nombres viennent de `combined_prefilter_probe`, dont le défaut est
`--cap=scission`. Ils ne peuvent donc pas être utilisés comme compteur de WSPD
brute sans un compteur placé **avant** la condition de cap.

### 3.2 Même pour la WSPD brute, quatre tailles ne prouvent pas une pente

À `s=8`, `s^3=512`. Le rapport observé se rapproche fortement d'une constante
de l'ordre de `s^3`. Cela peut produire une pente transitoire supérieure à un
avant stabilisation de `front/n`.

Je ne conclus pas que telle est nécessairement toute l'explication ; je conclus
que l'exposant `1.44` ne tranche rien tant que la WSPD brute et le raffinement
post-cap ne sont pas comptés séparément.

---

## 4. L'option « dimension doublante effective croissante » est à rejeter

Tout sous-ensemble de `R^3`, quantifié ou non, hérite d'une constante de
redoublement bornée par une constante universelle de l'espace ambiant.

Les amas, l'anisotropie LiDAR et les grands vides peuvent modifier :

- les constantes ;
- la distribution des tailles de rectangles ;
- les effets pré-asymptotiques ;

mais ils ne transforment pas la borne d'une WSPD correcte à séparation fixée en
`n^1.44` asymptotique.

Les positions confondues constituent un cas dégénéré distinct, mais le probe les
refuse déjà. L'option (b) de la question n'est donc pas une explication
mathématique recevable.

Si la **WSPD brute**, mesurée correctement, reste superlinéaire au-delà du régime
transitoire, il faudra chercher une violation des hypothèses dans l'arbre ou la
récursion. Pas invoquer une dimension qui aurait mystérieusement dépassé trois,
prouesse que même le LiDAR n'a pas encore réalisée.

---

## 5. Expérience décisive à faire maintenant

Ajouter quatre compteurs distincts :

```text
raw_wspd_terminals
postcap_terminal_tiles
raw_wspd_pair_mass
postcap_pair_mass
```

Pour chaque premier état où `separated(A,B,s)` devient vrai :

```text
raw_wspd_terminals += 1
raw_wspd_pair_mass += |A||B|
```

Puis seulement appliquer, dans un étage séparé, le cap de ressources.

Ajouter aussi :

```text
postcap_tiles_per_raw_histogram
postcap_tiles_per_raw_max
raw_endpoint_population_max
```

Trois campagnes doivent être comparées sur les mêmes nuages :

```text
A. --cap=refus
   accepte la WSPD brute ; les grands rectangles deviennent un état aval.

B. --cap=scission --cap-cellule=512
   comportement actuel, post-raffiné.

C. cap-cellule >= n
   autre façon de désactiver entièrement la scission de ressource.
```

Exiger :

```text
raw_wspd_pair_mass == C(n,2)
postcap_pair_mass   == C(n,2)
```

et publier les pentes séparément sur `8000 / 16000 / 32000`.

Le compteur `wspd rectangles` doit être renommé. Deux noms possibles :

```text
raw_wspd_rectangles
postcap_processing_tiles
```

Sans ce renommage, la prochaine personne lira encore une propriété de la WSPD
dans un compteur d'ordonnancement. L'espèce humaine a déjà consacré assez de
temps aux colonnes correctement calculées et incorrectement nommées.

---

## 6. Réponse à Q1

**Q1 — pourquoi l'exposant ?**

La première cause certaine est que le compteur ne porte plus sur une WSPD pure.
Le point exact est le commit `5ce2634`, et la ligne exacte est la conjonction :

```text
sous_cap && separated
```

Il faut d'abord mesurer `separated` seul.

Si le compteur brut reste anormal après cette séparation :

1. comparer `wspd_front_probe` sur fair-split tree et
   `wspd_wavefront_probe` sur radix LBVH ;
2. compter les niveaux de préfixe Morton répétés ;
3. vérifier que la récursion scinde bien uniquement le facteur de plus grand
   diamètre géométrique ;
4. mesurer `raw_wspd_rectangles/(s^3 n)` ;
5. tester cellules octree alignées contre boîtes serrées.

Mais modifier immédiatement le critère de séparation serait traiter le second
suspect avant d'interroger le premier, coutume policière peu efficace même dans
un dépôt C++.

---

## 7. Réponse à Q2 : ne pas abandonner la WSPD pour réparer une faute aval

La WSPD doit redevenir une partition géométrique immuable :

```text
separated(A,B,s)
  -> RectId WSPD terminal
```

Le cap de calcul doit agir **après** :

```text
RectId
  -> état lane-local
  -> count / preflight
  -> continuation si budget insuffisant
```

Il ne doit jamais demander de redécouper `A x B` uniquement parce que le calcul
historique de `h_a/h_b` était quadratique dans `|A|` ou `|B|`.

Le vrai verrou est donc celui-ci :

```text
remplacer l'auto-jointure ponctuelle h_a/h_b
par une traversée factorisée ou un certificat directionnel,
sans casser le rectangle WSPD.
```

Les candidats déjà présents dans le dossier sont :

- dual-tree avec frontière héritée ;
- antichaîne de témoins ;
- source directionnelle Yao48 pour q2 ;
- `PairFrame` avec continuations, plutôt qu'un cap transformé en géométrie.

Un débordement de ressource rend `PENDING_RESOURCE`. Il ne doit jamais changer
la partition mathématique.

Cela n'interdit pas un raffinement local d'extrémité pour améliorer une borne.
Mais ce raffinement reste un **sous-état du même RectId**, avec preuves héritées
et continuation ; il n'est pas recompilé dans la taille de la WSPD.

---

## 8. Réponse à Q3 : la distance seule ne suffit pas en général

Il n'existe pas, sans hypothèse supplémentaire de régularité locale, de borne
universelle du type :

```text
arête q2-vivante
  -> l'autre extrémité est parmi les K plus proches voisins,
```

avec `K=K(h_2,d)` indépendant de `n`.

Contre-famille : fixer `a` et `b`, puis placer arbitrairement beaucoup de points
plus proches de `a` que `b`, mais dans le demi-espace opposé à `b`. Ils restent
hors de la boule diamétrale de `[a,b]`. L'arête peut donc rester q2-vivante alors
que le rang de `b` autour de `a` est arbitraire.

Un certificat métrique exact existe si l'on possède une information de densité
au **milieu** :

```text
si le h2-ième voisin du milieu m=(a+b)/2
est à distance strictement inférieure à |ab|/2,
alors l'arête est morte.
```

Mais calculer cette requête pour tous les milieux recrée le problème si elle
n'est pas factorisée par blocs.

La meilleure route q2 disponible n'est donc pas un `kNN` isotrope, mais le
certificat directionnel déjà développé dans `Yao48` : plusieurs voisins plus
proches dans une même chambre donnent des témoins de la boule diamétrale. C'est
précisément le type de théorème qui peut produire une source bornée sans couvrir
toutes les paires.

Pour q3/q4, `two_lines` montre en outre que les ancres W-vivantes peuvent être
quadratiques. Leur réduction exige la positivité/carrier, pas une simple borne
de distance.

---

## 9. Ordre de travail recommandé à Claude

```text
1. séparer immédiatement raw_wspd_rectangles et postcap_processing_tiles ;
2. lancer raw WSPD à 8000 / 16000 / 32000, s=6/8/10 ;
3. retirer le cap de population du critère terminal géométrique ;
4. garder chaque RectId WSPD immuable ;
5. transformer tout dépassement de h_a/h_b en continuation ;
6. remplacer l'auto-jointure locale par dual-tree / Yao48 ;
7. seulement ensuite décider si la WSPD reste utile comme tronc commun.
```

Ne pas brancher le WSPD actuel sous son compteur ambigu. Ne pas l'abandonner non
plus sur la base de ce compteur.

---

## 10. Message direct

Tu as correctement trouvé que la partition q2 par cap de masse est quadratique.
Mais la ligne suivante de ta question attribue à la WSPD un compteur qui ne lui
appartient plus.

La WSPD est devenue « quadratique » le 15 août quand, pour supprimer les
rectangles hors cap, tu as écrit en substance :

```text
terminal = well_separated AND small_enough_for_my_quadratic_subroutine.
```

Le second membre n'est pas une propriété géométrique. Il force une partition de
paires à blocs de population bornée et détruit mécaniquement la borne linéaire.

Le remède n'est ni une autre convention de diamètre, ni une dimension doublante
plus généreuse. Le remède est de remettre le cap à sa place : dans le scheduler
et les continuations, jamais dans la définition d'un rectangle WSPD terminal.
