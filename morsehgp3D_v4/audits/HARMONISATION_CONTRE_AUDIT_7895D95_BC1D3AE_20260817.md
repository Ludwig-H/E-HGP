# Harmonisation des deux audits du 17 août 2026

Date : 17 août 2026.  
Pin de code audité : `5072e235ba1194132f84a16420600f767fd7f811` inclus.  
Notes rapprochées :

- `REPONSE_A_CLAUDE_CENSUS_Q3_ET_BORD_20260817.md`, commit `7895d95` ;
- `AUDIT_MATHEMATIQUE_0FB32C3_A_5072E23_20260817.md`, commit `bc1d3ae`.

Cette note est l'arbitrage à lire en priorité lorsque les deux documents
emploient des formulations différentes. Il n'y a pas de désaccord de fond :
les deux audits reçoivent `R_coup`, la descente q2/q3/q4 fusionnée, le filtre
`h_a/h_b` et la géométrie de la forme q3. Ils refusent tous deux de qualifier
le probe actuel de producteur transactionnel complet d'événements.

Les précisions ci-dessous corrigent deux formulations trop larges de
`bc1d3ae`, retiennent les meilleures propositions des deux audits et donnent à
Claude un ordre de travail unique. Il fallait bien une troisième note pour
empêcher deux notes largement concordantes de devenir artificiellement une
controverse, coutume académique dont le rendement reste difficile à mesurer.

---

## 1. Verdict harmonisé

### Reçu mathématiquement

1. Le rayon couplé

   \[
   R_{\mathrm{coup},q}
   =\kappa_q d-
   \sqrt{(4\kappa_q^2+1)(r_A^2+r_B^2)/2}
   \]

   est une boule universelle sûre ; les arrondis de `core_ball` sont dirigés
   dans le bon sens.
2. `h_cœur+h_a+h_b` est un **minorant calculé exactement** et fail-open de la
   profondeur. L'inégalité stricte de l'histogramme est correcte.
3. `count_universal_witnesses_234` ne présente pas de source identifiée de
   fausse mort : masques locaux, saturation, exclusion des extrémités et
   comparaisons strictes sont cohérents.
4. La forme de Gram q3

   \[
   P(z)=G\|z-a\|^2-W\cdot(z-a)
       =G\bigl(\|z-c\|^2-r^2\bigr)
   \]

   est exacte, et les bornes de boîte de `q3_ball_depth` sont orientées
   correctement.
5. Le commit `5072e23` est reçu comme **énumérateur des supports q3 réguliers
   peu profonds**, confronté par identités internes à l'oracle exhaustif qui
   partage encore son arithmétique de puissance.

### Non encore reçu comme contrat public exact

Le probe q3 ne publie encore ni vrais `PointId`, ni `BallKey`, ni
`ExactLevel`, ni liste des intérieurs, ni hyperincidence. Il saute les
coquilles externes puis retourne succès, et il n'applique pas encore
`h_a/h_b` malgré son commentaire de tête. Le terme correct reste donc
« supports q3 réguliers peu profonds », pas « événements HGP complets ».

---

## 2. Correction 1 : cellule exacte d'un préfixe Morton

La note `bc1d3ae` mentionne à un endroit un rapport d'aspect maximal `4` pour
la cellule binaire exacte. La valeur correcte est **`2`**.

La clé utile comporte 48 bits, lus depuis le bit fort. Avec l'entrelacement
actuel, les bits résiduels d'un triplet fixent successivement `z`, puis `y`,
puis `x`. Pour

```text
used = prefix_length - 16,
full = used / 3,
rem  = used % 3,
```

le nombre de bits de coordonnées fixés est

```text
fixed_z = full + (rem >= 1),
fixed_y = full + (rem >= 2),
fixed_x = full.
```

Deux longueurs de côté ne diffèrent donc que d'un facteur deux au plus.
`cell_of_prefix` ignore actuellement `rem` et remplace cette cellule
rectangulaire par le cube extérieur du niveau octree précédent.

### Prescription WSPD retenue

1. construire la cellule **rectangulaire exacte** du préfixe ;
2. choisir le facteur à scinder par diamètre de cette cellule ;
3. terminer lorsque

   ```text
   cell_separated(A,B) || tight_box_separated(A,B)
   ```

4. comparer à une récursion ombre qui emploie les mêmes graines et scissions,
   mais seulement `cell_separated`.

La récursion réelle ne fait qu'élaguer plus tôt la récursion ombre. Son nombre
de rectangles est donc au plus celui du front cellulaire auquel s'applique le
packing `O(s³m)`, avec `m` positions distinctes. On conserve ainsi les bonnes
constantes des boîtes serrées sans leur demander de porter seules une preuve
qui dépend du mécanisme de scission.

---

## 3. Correction 2 : ce qui peut réellement être hérité dans la descente

Sous raffinement `A'⊆A`, `B'⊆B`, la région de témoins universels grandit :

\[
\bigcap_{a\in A,b\in B}W_q(a,b)
\subseteq
\bigcap_{a\in A',b\in B'}W_q(a,b).
\]

Il en résulte une asymétrie importante.

### Parent vers enfant WSPD

- Un sous-arbre géométriquement `ALL` pour le parent reste `ALL` pour
  l'enfant.
- Un sous-arbre `NONE` ou `MIXED` pour le parent peut devenir `ALL` pour
  l'enfant ; il doit donc être rouvert.
- Un point retiré du compte parce qu'il appartenait à `A∪B` peut devenir un
  témoin de cœur après scission.

La première cache sûre doit donc transporter seulement une antichaîne de
plages `ALL` **entièrement disjointes de `A∪B`**. Une plage créditée après
soustraction partielle d'extrémités ne peut pas être simplement réutilisée
comme un entier opaque.

### Tentative interne vers tentative terminale pour le même `(A,B)`

- Les nœuds élagués par `Hmax≤0` peuvent être mémorisés comme vrais `NONE` pour
  les trois lanes, puisque la géométrie ne change pas.
- Les nœuds extérieurs à une boule-cœur ne sont pas `NONE` pour le fuseau : ils
  restent **indécis** et doivent être revus par l'autorité des coins.
- Les plages déjà créditées `ALL` peuvent être reprises avec leur masque de
  lane et leur règle exacte d'exclusion d'identités.

Une continuation correcte contient donc au minimum des plages `ALL`, les
nœuds encore indécis et les relations d'intersection avec les extrémités. Elle
ne doit jamais transformer « hors de la boule suffisante » en « hors du
fuseau ».

---

## 4. Raccord immédiat de `h_a/h_b` à l'instruction q3

Le commentaire de `q3_events_probe.cpp` annonce le filtre complet, mais le
code expanse actuellement tous les couples `ua×ub` des rectangles qui ont
survécu au seul cœur.

La phase déjà mesurée sur `eight_clusters, n=8000` conserve 74,7 % des ancres
q3 après `h_a/h_b`. La brancher avant toute requête de lentille est donc le
gain immédiat le moins risqué.

L'énumération ne requiert pas de produit cartésien matérialisé. Avec
`r=h_q-h_cœur≤9` :

1. ranger les `b` dans les seaux de leur valeur saturée `h_b` ;
2. pour chaque `a`, calculer `t(a)=r-min(h_a(a),r)` ;
3. émettre les `b` des seaux `0,…,t(a)-1`.

Le coût vaut

\[
O(|A|+|B|+\#\text{ancres survivantes}).
\]

La primitive devrait être partagée entre le probe q234 et les instructions
q3/q4, faute de quoi les commentaires continueront probablement à exécuter
plus d'étapes que les fonctions.

---

## 5. Q3 : cover commun et census partagé

Soit `D=||b-a||` la longueur de l'arête owner. Pour un triangle aigu dont
`ab` est une arête maximale :

\[
r\le \frac{D}{\sqrt3},
\qquad
\|c-m\|\le \frac{D}{2\sqrt3},
\qquad m=\frac{a+b}{2}.
\]

Tout porteur et tout point intérieur à sa circum-boule appartiennent donc au
cover commun

\[
B\!\left(m,\frac{\sqrt3}{2}D\right).
\]

Ce rayon `≈0,866D` remplace avantageusement la borne `0,966D` évoquée dans la
question de Claude, qui relevait de q4.

### Objet exact dans le plan médiateur

Avec

\[
T=2c-a-b,\qquad u_z=2z-a-b,
\]

on a `T·(b-a)=0` et

\[
\ell_z(T)=\|u_z\|^2-D^2-2u_z\cdot T.
\]

Le site `z` est intérieur, sur la coquille ou extérieur selon que
`ℓ_z(T)` est négatif, nul ou positif. Le census de tous les porteurs d'une
ancre est donc un problème de **niveaux peu profonds d'un arrangement de
droites** dans le plan médiateur.

### Ordre d'implémentation harmonisé

#### A. Prototype prioritaire : arbre des centres-porteurs

1. construire tous les centres rationnels `T_x` des porteurs survivants ;
2. les organiser dans un petit LBVH/quadtree 2D temporaire ;
3. parcourir une seule fois les sites du cover commun ;
4. pour chaque site, les extrema de `ℓ_z` sur une boîte de centres sont aux
   coins ;
5. `max ℓ_z<0` donne un `range-add(+1)`, `min ℓ_z>0` élague, l'égalité reste
   ouverte pour détecter les coquilles ;
6. masquer les centres dès saturation à `h_3`.

Ce prototype est plus simple que le dual-tree complet et partage déjà le
travail entre tous les porteurs de l'ancre.

#### B. Voie radiale exacte pour les directions rentables

Pour

\[
V_x=D^2(2x-a-b)-((2x-a-b)\cdot(b-a))(b-a),
\]

les centres de porteurs de même direction primitive `P` sont sur une même
droite `m+τP`. Chaque site y donne une racine rationnelle

\[
\tau_z=\frac{\|2z-a-b\|^2-D^2}{4P\cdot(2z-a-b)}.
\]

Si `p` sites sont intérieurement permanents, un porteur de profondeur
`<h_3` se trouve parmi les `h_3-p` premières racines d'entrée ou les
`h_3-p` dernières racines de sortie. Cette voie réutilise directement la
primitive de racines extrémales de q4, mais elle ne sera rentable que lorsque
plusieurs porteurs partagent une direction.

#### C. Dual-tree complet seulement si nécessaire

Si le prototype A reste trop mélangé sur `eight_clusters`, passer à
`CenterBlock×SiteNode` avec bornes dirigées, crédit en bloc et saturation.
L'arrangement peu profond explicite reste une dernière option pour les ancres
très chargées, pas le premier morceau de code à écrire.

---

## 6. Passage du support q3 à l'événement complet

Les formules exactes sont déjà disponibles :

\[
\operatorname{ExactLevel}=r^2
=\frac{D\,E\,X}{4G},
\qquad X=\|b-x\|^2,
\]

et

\[
c=\frac{2Ga+W}{2G}.
\]

Le prochain enregistrement doit contenir :

- `SupportKey` en vrais `PointId` ;
- `BallKey` canonique ;
- `ExactLevel` réduit ;
- profondeur ;
- `InteriorIds` ;
- éventuels `ShellIds` ;
- ordre `K=depth+2` ;
- facettes de connexion et de rendu ;
- payload de multifusion.

Comme `h_3≤9`, un événement survivant possède au plus huit intérieurs. La
collecte d'identités n'est donc pas le mur mémoire : conserver des handles de
plages `ALL`, puis les expanser uniquement après confirmation de survie,
suffit.

### Trois portes obligatoires

1. **Identités stables.** `ua,ub,ux` sont des rangs Morton. Employer

   ```text
   id(u) = ix.bucket_ids[ix.bucket_start[u]]
   ```

   après refus des doublons, puis faire évoluer l'API vers des couples
   `{PointId,position}`.
2. **Refus réellement transactionnel.** En mode exact, une coquille externe
   produit `unsupported_degeneracy` et aucune publication. Le comportement
   actuel doit être nommé `regular_subset_diagnostic` s'il est conservé.
3. **Exact-once visible.** Publier `raw_supports`, `unique_supports` et
   `duplicate_supports`. La porte exige `duplicate_supports=0`; `sort/unique`
   ne doit pas réparer silencieusement une violation de l'owner ou de la
   partition des paires.

---

## 7. Oracle indépendant : maintenant, avant q4

Le juge actuel est excellent pour la complétude WSPD/owner, mais le sujet et
le juge partagent `q3_form` et `q3_ball_depth`. Un micro-oracle indépendant à
`n≤40–60` doit :

1. tester l'acuité par les trois produits scalaires d'angles ;
2. employer de vrais `PointId` ;
3. résoudre rationnellement le système du circumcentre avec `cpp_int` ;
4. comparer exactement toutes les distances centre-site ;
5. rendre
   `(SupportKey,BallKey,ExactLevel,InteriorIds,ShellIds)` ;
6. tuer les mutants de signe, de stricte inégalité, d'owner Morton et d'oubli
   de coquille.

Il faut l'écrire avant que q4 ne réutilise les seeds q3 et n'empile une couche
supplémentaire de déterminants sur une vérité encore commune au sujet et au
juge.

---

## 8. Bord de cube : les deux calculs sont complémentaires

Les deux audits s'accordent sur le point essentiel : à densité fixée, le
rapport aux constantes de volume infini converge vers `1`, avec une correction
principale `O(n^{-1/3})`. La fraction de bord n'est pas invariante.

Les deux développements ne calculent simplement pas le même morceau :

- `7895d95` traite exactement, jusqu'aux arêtes et coins, la **disponibilité
  des deux extrémités** dans le cube, en gardant encore le volume plein du
  fuseau témoin ;
- `bc1d3ae` traite au premier ordre de face la **troncature du fuseau de
  témoins**, tout en laissant les arêtes et coins dans `O(L^{-2})`.

Ils ne doivent donc ni être opposés, ni additionnés intégralement. En partant
du développement « disponibilité des paires » et en ajoutant seulement le
terme de face positif dû au fuseau tronqué, on obtient une approximation
hybride sans double comptage. À `n=2000`, elle donne indicativement

```text
q2 : 32,75
q3 : 87,13
q4 : 95,72
```

contre `32,3 / 86,3 / 94,9` observés. L'accord est suffisamment bon pour
valider l'interprétation, pas pour graver une porte dure sur un échantillon de
réseau sans remise.

La porte recommandée reste le tore continu à `R<L/2`, où la cible finie à
`n` fixé s'écrit par bêta incomplète régularisée. Sur la grille actuelle, elle
sert de limite statistique à haute résolution, ou doit être remplacée par un
petit oracle hypergéométrique discret.

---

## 9. Ordre de travail unique proposé à Claude

### P0 : raccords courts et vérité indépendante

1. Brancher réellement `h_a/h_b` dans q3 et énumérer les survivantes par
   seaux.
2. Remplacer les rangs Morton par les vrais `PointId` dans tous les owners et
   `SupportKey`.
3. Rendre le refus de coquille transactionnel, ou nommer explicitement le
   mode sous-ensemble régulier.
4. Ajouter `raw/unique/duplicate_supports`.
5. Écrire le micro-oracle rationnel indépendant.

### P1 : événement q3 complet

6. Publier `BallKey`, `ExactLevel`, profondeur et `InteriorIds`.
7. Ajouter les facettes `F_K^conn/F_K^render` et le payload de multifusion.
8. Ajouter la parité bit à bit entre descente fusionnée et trois descentes
   séparées.
9. Fermer à la racine les lanes q3/q4 de rayon nul lorsque les coins sont
   désactivés.

### P2 : census partagé

10. Cover commun `B(m,√3D/2)`.
11. Prototype « sites vers arbre 2D des centres », avec `range-add` et
    saturation.
12. Groupes radiaux exacts lorsque la multiplicité de direction le justifie.
13. Dual-tree complet ou arrangement peu profond seulement si le prototype
    reste insuffisant.

### P3 : preuve de taille et portes statistiques

14. Cellules de préfixe exactes, scission cellulaire et arrêt
    `cell || tight`.
15. Porte torique ou régression contrôlée en `n^{-1/3}`.
16. Continuation déterministe par unités de travail reproductibles ; une
    sortie partielle reste marquée `incomplete` jusqu'au digest final.

---

## Conclusion

La contre-audition ne rétracte aucun acquis récent. Elle confirme que Claude a
franchi deux verrous sérieux : le front sait désormais tuer collectivement des
ancres, et q3 possède une chaîne géométrique exacte jusqu'aux supports
réguliers peu profonds.

Le prochain verrou est désormais proprement isolé : partager, pour une ancre,
le niveau d'un ensemble de formes affines dans le plan médiateur. La stratégie
la plus prudente est de brancher d'abord les filtres déjà disponibles, puis de
construire l'arbre temporaire des centres, avant toute sophistication q4. Cela
préserve l'exactitude, donne une primitive GPU plausible et évite de résoudre
un arrangement complet lorsque quelques `range-add` bien placés suffiront
peut-être à ramener `eight_clusters` dans un régime raisonnable.
