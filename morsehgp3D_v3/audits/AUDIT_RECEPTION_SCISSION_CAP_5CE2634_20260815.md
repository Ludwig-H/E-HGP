# Réception positive de la scission du cap et de `two_lines`

Date : 15 août 2026 UTC.

Pin audité : `5ce2634cc6e1e5fa9dedc3b9736ce799802d40a5`.
Base fonctionnelle : `7493deca8edb7e453e67d8df6e9fd2fcee2abfc7`.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

> [!NOTE]
> **Verdict court.** Le commit répond utilement au ré-audit. Je reçois le cœur
> mathématique de la scission récursive du cap : elle conserve la partition des
> paires, termine et ne semble introduire aucune nouvelle fausse fermeture dans
> le chemin nominal. Le maintien explicite de `--cap=refus`, le branchement des
> familles scanline, la rétractation sur `two_lines` et la correction
> `inf/NA` du mou sont de bonnes décisions.
>
> Le commit est donc une amélioration réelle. Il reste toutefois trois portes
> bloquantes avant de prendre ses nouveaux chiffres comme autorité :
>
> 1. les tests de scission vérifient la **masse totale**, pas l'exact-once par
>    `PairId` ;
> 2. la gestion des positions dupliquées n'est cohérente que dans une partie du
>    reçu et n'est exercée par aucune fixture ;
> 3. `two_lines` prouve dans le même binaire que le scan dit « sans `O(n^3)` »
>    est bien cubique au pire cas.
>
> Ce sont surtout des défauts de portes, d'unités et de budget. Je ne rejette pas
> la scission elle-même.

## 1. La scission du cap est mathématiquement saine

### 1.1 Conservation de la couverture

La décomposition diagonale initiale émet, pour chaque nœud interne `v`, le
produit

```text
L(v) x R(v).
```

Chaque paire non ordonnée de feuilles possède un unique plus petit ancêtre
commun ; elle apparaît donc dans exactement un de ces produits.

La scission remplace ensuite, par exemple,

```text
U x V
```

par

```text
U_g x V  disjoint-union  U_d x V.
```

Les deux produits sont disjoints et leur union est le produit parent. La même
propriété vaut lorsqu'on coupe `V`. Par induction sur les scissions, la
couverture exacte est donc conservée, indépendamment du test de séparation.

### 1.2 Retester la séparation est la bonne décision

Le code ne suppose pas qu'un rectangle séparé reste séparé après raffinement.
C'est prudent et correct : les sphères employées sont celles des AABB serrées,
et ces sphères ne sont pas emboîtées avec les boîtes. Une boîte fille peut être
incluse dans la boîte mère tout en ayant une sphère circonscrite non incluse
dans celle du parent.

Le statut `séparé/non séparé` est donc recalculé sur chaque enfant ; il ne sert
qu'à décider si le produit devient terminal. Il ne modifie pas l'identité des
paires couvertes.

### 1.3 Terminaison

À chaque étape non terminale, le code coupe une extrémité interne. Sa population
strictement décroît sur chaque branche. Le cas final est un produit de deux
feuilles, nécessairement sous tout cap positif. La récursion termine donc.

**Réception :** je ne trouve pas de défaut structurel dans ce mécanisme.

## 2. `two_lines` donne mieux qu'une pente : une formule exacte

Le branchement de cette contre-famille est très utile. Les quatre mesures sont
correctes, mais les exposants log-log sont ici superflus : le générateur admet
un compte fermé.

Posons `n=2m`, avec

```text
A_i=(i,0,0),
B_j=(0,j,H),
1 <= i,j <= m,
```

et les seuils actuels

```text
h_2=10,
h_3=9,
h_4=8.
```

Sous le réglage par défaut `H=65535`, les comptes sont exactement

```text
V_2 = 10 n - 55,
V_3 = n^2/4 + 9 n - 90,
V_4 = n^2/4 + 8 n - 72.
```

### Preuve

Sur chacune des deux droites, une paire séparée par `d` pas possède exactement
`d-1` témoins intérieurs dans les trois lanes. Sur une droite de `m` points, le
nombre de paires ayant moins de `h` témoins vaut

```text
h m - h(h+1)/2.
```

En sommant les deux droites, on obtient les termes linéaires.

Pour une paire croisée `(A_i,B_j)`, aucun autre point n'appartient à `W_3`, donc
aucun à `W_4`. En particulier, pour un candidat `A_k` situé avant `A_i`, le test
q3 impose après simplification une inégalité du type

```text
3(i-k)^2 > j^2 + H^2,
```

impossible dans le domaine courant ; après `A_i`, le signe de `H(a,b,z)` est
négatif. Le raisonnement est symétrique sur l'autre droite. Les `m^2` paires
croisées sont donc toutes q3/q4-vivantes.

En q2, la paire croisée possède exactement `i+j-2` témoins du demi-espace ; elle
survit si `i+j<=11`. Il y a exactement `55` telles paires. D'où les trois
formules.

Elles rendent bien :

| `n` | q2 exact | q3 exact | q4 exact |
| ---: | ---: | ---: | ---: |
| `300` | `2 945` | `25 110` | `24 828` |
| `600` | `5 945` | `95 310` | `94 728` |
| `1 200` | `11 945` | `370 710` | `369 528` |
| `2 400` | `23 945` | `1 461 510` | `1 459 128` |

Je recommande d'en faire un golden analytique plutôt qu'une simple ligne à
`n=600`. Une formule exacte tue beaucoup plus proprement une régression qu'une
pente qui, comme souvent, arrive munie de trois décimales et d'aucun alibi.

## 3. Portes bloquantes

### P0.1 — La porte de scission ne vérifie pas l'exact-once

Les nouveaux CTests à `cap=4` exigent

```text
masse = C(n,2),
masse_non_decide = 0.
```

Cela ne suffit pas : une paire dupliquée et une paire absente peuvent se
compenser dans la somme. Le dépôt possède déjà la bonne autorité,
`oracle_couverture_ko`, qui matérialise les `PairId` et exige une occurrence
exacte.

Correctif minimal : ajouter `--oracle=200` aux portes de scission forcée, sur
les six familles :

```text
uniform,
terrain,
eight_clusters,
scanline_single_pass,
scanline_overlap_multiecho,
two_lines.
```

La sortie doit imposer

```text
oracle_couverture_ko=0.
```

Correctif causal : ajouter deux mutants limités à la scission du cap :

```text
cap-oublie-enfant,
cap-duplique-enfant.
```

Le premier conserve au mieux une sous-masse ; le second peut être ajusté avec
une omission compensatrice pour démontrer précisément que la somme seule est
insuffisante.

### P0.2 — Les doublons géométriques ne sont pas encore traités de bout en bout

Le reçu calcule correctement

```text
univers_ancres = C(n,2) - somme_x C(m_x,2).
```

Le mode fusionné retire aussi les paires `D=0` du numérateur du mou. Mais quatre
incohérences restent :

1. le mode `legacy` saute `D=0` sans alimenter `vivant_degen_lane` ; son
   `S_D0exclu` reste donc faux ;
2. les lignes ordinaires
   `lane q* survivantes/fermees/ferme_pct` utilisent toujours `C(n,2)` ;
3. la garde finale compare `L.survivantes[q]` à `V_q`, non le résiduel corrigé
   de `D=0` ; une masse dégénérée pourrait masquer une paire valide manquante ;
4. l'échantillonneur tire et normalise encore sur `C(n,2)`, paires `D=0`
   comprises.

En outre, aucune famille courante n'exerce ce chemin : toutes dédupliquent les
triplets exacts. Les multi-échos partagent parfois `(x,y)`, mais pas le même
`z`.

Fixture recommandée : cinq `PointId`, dont deux à la même position, avec
`smax=32`. Comme `h_q>n-2`, toutes les ancres de diamètre positif doivent être
vivantes :

```text
paires_D0 = 1,
univers_ancres = 9,
V_2=V_3=V_4=9.
```

Un mutant `vivant-inclut-D0` doit rendre `10/10/10` et mourir. Une seconde porte
doit exercer `--refuse-doublons` et obtenir le code `2`.

Pour ce probe diagnostique, deux politiques sont acceptables :

- soutenir réellement les multiplicités et employer `univers_ancres` partout ;
- refuser les doublons par défaut jusqu'à cette mise en cohérence.

Le milieu actuel, où certaines lignes les retirent et d'autres non, est le seul
choix difficile à interpréter.

### P0.3 — Le scan exact reste cubique au pire cas

Le commentaire source annonce encore le `W`-vivant « exactement et sans
`O(n^3)` ». `two_lines` vient de fournir le contre-exemple exécutable :

```text
|S_4| = Theta(n^2),
```

et les paires croisées ne trouvent pas `h_4` témoins. Chacune parcourt donc
`Theta(n)` sites. Le coût est

```text
O(n^2 + n |union_q S_q|),
```

soit `Theta(n^3)` sur cette famille.

La borne `n<=40000` n'est pas un budget : elle autorise en théorie des dizaines
de milliers de milliards de visites. Il faut une porte de travail, par exemple

```text
--max-vivant-visites,
```

et idéalement séparer :

1. une passe `O(n^2)` qui compte l'union des paires résiduelles et publie une
   borne de travail ;
2. le balayage exact seulement si cette borne tient ;
3. sinon, refus explicite ou échantillonnage avec intervalle binomial.

L'invariant maintenant naturel doit être gaté :

```text
evals == travail,
travail <= (n-2) * paires_uniques.
```

Le champ `paires` du legacy compte des paire-lanes, alors que celui du mode
fusionné compte l'union des paires ; les unités doivent être nommées
séparément.

### P0.4 — La matérialisation de `rects` peut elle-même devenir quadratique

En mode `cap=scission`, un cap très petit peut produire presque une cellule par
paire. Le code stocke toute la liste avant de la traiter. À `n=65535`,
`C(n,2)` dépasse deux milliards de rectangles : le problème survient bien avant
le scan du `W`-vivant.

À court terme :

```text
--max-rectangles,
--max-wspd-front,
```

avec refus avant allocation excessive.

À moyen terme : traiter les rectangles terminaux en flux ou par vagues GPU,
sans construire un vecteur global dont la taille peut être celle de l'univers
des paires.

## 4. La scission est exacte pour la couverture, pas neutre pour le filtre

Le nouveau cap modifie la partition WSPD. Or le minorant

```text
h_coeur + h_a + h_b
```

n'est pas manifestement monotone sous raffinement.

Exemple conceptuel : un site `z` appartenant à un sous-nœud frère de `a` peut
être crédité dans `h_a(a;B)` au niveau parent. Après scission de `A`, `z` sort de
l'extrémité courante. Il n'entre dans le cœur du rectangle enfant que s'il est
universel pour **tous** les `a'` de l'enfant, condition plus forte. Le crédit
peut donc disparaître. Inversement, les boîtes plus petites permettent de
nouveaux certificats.

Ainsi `cap=scission` n'est pas simplement `cap=refus` auquel on aurait retiré la
masse indécise : c'est une nouvelle partition et potentiellement un nouveau
résiduel.

Campagne demandée à petit/moyen `n` :

```text
cap in {1,4,16,64,512},
```

avec matérialisation des `PairId` survivants. Publier :

```text
|S_cap1|, |S_cap4|, ...,
intersections,
differences orientees,
V_q identique pour tous les caps,
oracle_faux_morts=0.
```

Les anciens chiffres de séparation/cap ne doivent pas être mélangés aux nouveaux
sans cette distinction.

## 5. Corrections P1 utiles

### 5.1 `coord=65536` n'est pas rejouable

`two_lines` choisit par défaut une étendue `65536` afin d'autoriser les
coordonnées `0..65535`, mais la CLI refuse `--coord=65536`. Le reçu publie donc
une valeur qu'on ne peut pas recopier pour rejouer la campagne.

Deux solutions propres :

- accepter une **étendue** `65536` tout en gardant les coordonnées sous u16 ;
- distinguer dans le reçu `coord_extent=65536` de la valeur maximale
  `coord_max=65535`.

### 5.2 Les scanlines doivent entrer dans les juges métamorphiques

Leur simple non-vacuité q4 est un bon début. Il faut désormais les ajouter à :

```text
fusion == legacy,
cap-scission + oracle exact-once,
rampe de plusieurs graines,
histogrammes de lentille.
```

### 5.3 Commentaires devenus faux

Trois récits anciens survivent encore dans le source :

- « sans `O(n^3)` », réfuté par `two_lines` ;
- la prédiction de lentille obtenue par simple multiplication du ratio de
  volumes par `h`, alors que la moyenne conditionnelle correcte est donnée dans
  l'audit Poisson ;
- la variance de l'échantillonneur encore dite inexpliquée dans un commentaire,
  alors que sa normalisation a déjà été rétractée.

Le complément en dimension intrinsèque est désormais dans
[`NOTE_AUDITEUR_POISSON_DIMENSION_INTRINSEQUE_20260815.md`](NOTE_AUDITEUR_POISSON_DIMENSION_INTRINSEQUE_20260815.md) :
la référence q4 vaut `34,6244` ancres par point et `14,2305` candidats de
lentille sur un plan, contre `139,0696` et `47,8917` dans un volume homogène.
Ces deux régimes expliquent beaucoup mieux `terrain` et `uniform` que la phrase
unique `O(h)`.

## 6. Ordre d'implémentation recommandé

1. Ajouter `--oracle=200` aux scissions forcées et tuer un mutant de couverture.
2. Graver la fixture `D=0`, puis harmoniser tous les dénominateurs et les deux
   modes de balayage.
3. Remplacer toute revendication « sans cube » par le coût output-sensitive
   exact et ajouter un budget de visites.
4. Faire le sweep apparié des caps avec ensembles de `PairId`.
5. Remplacer la pente `two_lines` par les trois formules exactes.
6. Seulement ensuite relancer les rampes de performance du nouveau défaut
   `cap=scission`.

Le commit `5ce2634` est donc reçu comme une bonne correction architecturale et
une honnête rectification expérimentale. La prochaine étape n'est pas de
resserrer encore le certificat : elle consiste à rendre la scission causalement
prouvée, à borner son travail, puis à faire disparaître la masse quadratique de
`two_lines` par la positivité aiguë avant toute allocation de supports.
