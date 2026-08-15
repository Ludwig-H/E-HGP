# Contre-réception du porteur aigu après `207b542`

Date : 15 août 2026 UTC.

Pins audités :

- `2ce76e0d61988bdb18befe49ba18d02ead75ea9a` — porteur aigu et descente par ancre ;
- `c8e3de7bcf44b4b5aeb895799c69ec9ac2126ab2` — exact-once de la scission et fixture des faces incidentes ;
- `207b542ff1ba011696e7681dc9fd8f6430002a5c` — frontières ponctuelles et cas `D=0`.

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_and_code_audit`,
`profile=quantized_u16_input_only`, `mode=carrier_gateway_review`,
`public_status=not_claimed`. GCP non utilisé.

> [!IMPORTANT]
> **Verdict court.** Je reçois :
>
> - le lemme ponctuel `x porteur aigu <=> x dans la lentille et H<0`, sous
>   l'hypothèse que `ab` est une arête maximale ;
> - les strictes de `pair_lane` et du porteur sur les dix cas entiers de
>   `207b542` ;
> - l'exact-once par `PairId` de la scission récursive, sur les neuf portes de
>   `c8e3de7`.
>
> Je ne reçois pas encore les compteurs d'étages ni les conclusions de
> complexité. Deux erreurs de sémantique sont bloquantes :
>
> 1. le champ imprimé `V4_pair_walive` est en réalité le nombre `S4` de paires
>    survivant au **préfiltre**, et non le vrai `V4` décidé par le fuseau exact ;
> 2. `est_seed` vérifie seulement que `ab` est maximale au sens large. Il
>    n'applique aucun tie-break d'`EdgeKey`, donc `C4_carrier` n'est pas un
>    compteur de carriers possédés et peut compter plusieurs fois une même
>    face quantifiée.
>
> La fixture `D=0` de `207b542` est une bonne porte ponctuelle, mais elle ne
> traverse aucun ledger de doublons. Enfin, la descente par ancre est exacte et
> utile, mais elle n'est pas logarithmique au pire cas, et son ablation ne réfute
> pas le gateway factorisé `A x B x C` proposé par l'audit.

## 1. Ce qui est reçu

### 1.1 Lemme ponctuel du carrier

Pour

```text
D = ||a-b||^2,
E = ||a-x||^2,
X = ||b-x||^2,
H = (x-a) dot (b-x),
```

on a

```text
E + X - D = -2H.
```

Si `D>=E` et `D>=X`, l'angle opposé à `ab` est le plus grand angle du
triangle. Le triangle est donc strictement aigu si et seulement si cet angle
est aigu, soit

```text
H < 0.
```

Ainsi, pour une arête maximale,

```text
carrier aigu <=> E<=D, X<=D et H<0.
```

Les égalités sont importantes. La lentille se partitionne en **trois** parties,
non en deux :

```text
H > 0 : intérieur strict de la boule diamétrale, témoin q2 ;
H = 0 : shell q2, angle droit, non-carrier mais non-témoin strict ;
H < 0 : carrier aigu, sous les deux contraintes de lentille.
```

La phrase « un témoin q2 est exactement un non-porteur » doit donc être
restreinte : un témoin q2 est un non-porteur **strictement intérieur** ; les
points `H=0` sont aussi non-porteurs mais appartiennent au shell. Cette
frontière a une masse non négligeable dans le profil quantifié.

### 1.2 Exact-once de la scission

Les portes de `c8e3de7` utilisent l'oracle de couverture par `PairId`, et non la
seule identité de masse. Pour trois familles et trois caps, elles exigent

```text
oracle_couverture_ko = 0,
oracle_faux_morts = 0,
oracle_ids_doubles = 0.
```

C'est la bonne autorité pour la conservation de la partition. Le fait que la
qualité du minorant change avec la partition n'est pas un défaut d'exact-once :
les paires couvertes restent les mêmes, tandis que les certificats universels
peuvent devenir plus ou moins forts après redécoupage.

### 1.3 Frontières de `pair_lane`

Les fixtures de `207b542` exercent bien :

```text
H=0,
4H^2=ET,
3H^2=ET,
les trois intérieurs stricts,
z=a,
z=b,
a=b,
un carrier H<0.
```

Cette porte reçoit les prédicats ponctuels et leurs strictes.

## 2. P0 : `V4_pair_walive` est actuellement `S4_prefilter_survivor`

Dans la boucle `--seeds`, une paire est retenue par :

```text
budget = h4 - hcore4 - ha4[a]
retenue si budget>0 et hb4[b]<budget.
```

Le code incrémente alors immédiatement :

```text
++seed_ancres;
```

puis balaie les carriers. Aucun scan `pair_lane` ne décide auparavant si la
paire possède réellement moins de huit témoins `W4`. De plus,
`--seeds` et `--verifie-seed` n'activent pas `--vrai-vivant`.

Par conséquent, dans ce mode :

```text
champ actuel V4_pair_walive
  = S4_prefilter_survivor_D0exclu,

C4_carrier
  = carriers candidats issus des paires S4,
  pas carriers issus des seules paires V4 exactes.
```

`two_lines` masque exactement le défaut de nommage : son mou vaut un, donc
`S4=V4=43128`. Les trois autres lignes ne permettent pas cette identification.
Leurs ratios mélangent :

1. le mou du préfiltre universel ;
2. l'absence éventuelle de carrier aigu.

Ils ne mesurent donc pas une contraction causale `V4 -> C4` et ne doivent pas
être comparés aux constantes de Poisson de `V4` ou des vrais supports q4.

### Deux réparations possibles

#### Réparation minimale, immédiatement honnête

Renommer :

```text
S4_prefilter_survivor,
S4_without_carrier,
C4_carrier_candidate_from_S4.
```

Cette version est parfaitement utile comme ledger de coût, à condition de ne
pas la présenter comme une source exacte.

#### Diagnostic exact à petite taille

Pour chaque paire `S4`, faire un seul balayage fusionné :

```text
w4 = 0
c4 = 0
for z:
    lane = pair_lane(a,b,z)
    si lane>=4: ++w4
    si is_face_owner_carrier(a,b,z): ++c4
    si w4 atteint 8:
        paire morte ; abandonner c4 et sortir tôt
si w4<8:
    ++V4_pair_walive
    C4 += c4
```

Une paire vivante exige de toute façon le scan complet ; une paire morte peut
sortir au huitième témoin. Ce chemin est un oracle de compteur, pas encore une
ordonnance 50k.

## 3. P0 : le tie-break d'owner est absent de `est_seed`

La fonction actuelle fait exactement :

```text
si E>D ou X>D : false
sinon : H<0
```

Elle vérifie que `ab` est **une** arête maximale, pas l'owner canonique. Aucun
`EdgeKey` n'apparaît dans le fichier. Sur données continues, les ties sont de
mesure nulle ; sous u16, sur nappes, grilles et multi-échos, ils font partie du
contrat.

### Contre-fixture entière minimale

Prendre :

```text
a=(0,0,0),
b=(1,1,0),
x=(1,0,1).
```

Les trois longueurs carrées valent deux et le triangle est strictement aigu.
La fonction actuelle compte le même triangle sous ses trois arêtes. Une source
possédée doit n'en retenir qu'une, celle dont l'`EdgeKey` est minimale parmi les
arêtes de longueur maximale.

La fixture régulière de `c8e3de7` révèle le même phénomène à l'étage suivant :
le tétraèdre régulier a six arêtes maximales et deux faces incidentes aiguës par
arête. Le compteur candidat peut donc voir douze incidences, alors que :

```text
owner_edge carriers = 2,
centre q4 exact-once = 1.
```

La fixture actuelle choisit seulement la première arête maximale pour compter
ses deux faces ; elle ne confronte jamais ce nombre aux douze propositions que
le producteur sans tie peut réellement émettre.

### Correctif complet et sûr

Utiliser les vrais `PointId`, jamais les rangs Morton. Si `id(i)` est l'identité
stable du point trié, définir :

```text
EdgeKey(u,v) = (min(id(u),id(v)), max(id(u),id(v))).
```

`ab` est l'owner de la face si :

```text
D>E et D>X,

ou D=E>=X et EdgeKey(ab)<EdgeKey(ax),
ou D=X>=E et EdgeKey(ab)<EdgeKey(bx),

avec les deux comparaisons lorsque D=E=X.
```

Plus simplement : construire les trois couples `(longueur^2, -EdgeKey)` avec
l'ordre « longueur maximale, puis EdgeKey minimale » et exiger que `ab` soit le
maximum canonique.

Cette restriction est complète : si `ab` est l'owner global d'un q4, il est en
particulier l'owner canonique de chacune de ses faces incidentes. L'owner sur
les six arêtes reste néanmoins rejoué après ajout de l'apex, car une arête de la
seconde face peut encore gagner.

Le compteur exact doit être nommé au plus :

```text
C4_face_owner_candidate,
```

jusqu'au replay `owner6`.

## 4. La fixture `D=0` ne traverse pas encore la chaîne

`207b542` vérifie correctement :

```text
pair_lane(a,a,z)=0,
est_seed(a,a,z)=false.
```

Mais elle ne produit pas un nuage avec deux `PointId` à la même position. Elle
n'exerce donc pas :

- `paires_D0` et `univers_ancres` ;
- `vivant_degen_lane[q]` ;
- l'accord `legacy/fusion` en présence de doublons ;
- l'exclusion `D=0` dans `seed_ancres` ;
- la conservation de la multiplicité des doublons comme **témoins** d'une
  ancre vers une troisième position.

### Porte demandée

Construire cinq identifiants dont exactement deux partagent une position. Exiger :

```text
paires_indices = C(5,2) = 10,
paires_D0 = 1,
univers_ancres = 9,
```

puis :

```text
legacy == fusion par lane,
degenerées > 0,
S_q et V_q publiés après exclusion de cette unique paire,
seed_ancres n'inclut jamais D0.
```

Une seconde assertion doit montrer que les deux IDs dupliqués comptent encore
séparément comme témoins pour une paire vers une autre position. Une
`deduplication` géométrique globale serait fausse : seule l'ancre endpoint
`D=0` est impropre.

## 5. La descente n'est pas logarithmique au pire cas

Les deux certificats par ancre sont sûrs :

```text
NONE si Box(X) est hors de B(a,|ab|) ou hors de B(b,|ab|),
NONE si Box(X) est entièrement dans la boule diamétrale fermée.
```

Ils donnent environ trente visites par ancre sur `two_lines,n=400`. Cela ne
prouve pas une complexité logarithmique. Une hiérarchie d'AABB ne garantit pas
une requête sphérique en `O(log n)` : une surface de requête peut couper un
nombre linéaire de cellules.

Contre-régime conceptuel : placer de nombreux points sur, ou de part et d'autre
de, la sphère diamétrale d'une même ancre. Chaque boîte interne peut avoir un
coin hors de la boule et contenir un point dedans ; les deux verdicts restent
`MIXED` jusqu'aux feuilles. La visite est alors `Theta(n)` pour cette ancre.

La formulation recevable est :

> « La descente réduit fortement les visites sur les quatre familles mesurées,
> et vaut environ trente nœuds par ancre sur `two_lines,n=400`. »

Avant « logarithmique », mesurer pour chaque famille et quatre tailles :

```text
visites_totales / ancres,
visites_max_par_ancre,
exposants successifs,
fraction NONE_lentille / NONE_diamètre / feuilles.
```

### Unités de coût

`seed_travail_ref` compte un `est_seed` ponctuel. `seed_travail_elag` compte un
test de nœud comme une unité, alors que ce test exécute jusqu'à deux
sphère--AABB et qu'une feuille paie ensuite aussi `est_seed`. Les unités sont
plus honnêtes qu'avant, mais pas identiques instruction par instruction.

Publier séparément :

```text
node_lens_tests,
node_midball_tests,
leaf_seed_tests,
i128_mul_or_square,
time_wall,
HWM.
```

Le ratio courant peut s'appeler `predicate_call_ratio`, pas speedup. Le facteur
`1,20` sur `eight_clusters` peut disparaître au temps de paroi ; le facteur
`13,29` sur `two_lines` est beaucoup plus robuste, mais reste à mesurer.

## 6. L'ablation `A x B` ne réfute pas `AcuteBox24`

Le résultat négatif de `2ce76e0` est utile, mais son périmètre doit être nommé :

```text
A x B fixé par la WSPD,
seul le facteur témoin X descend,
certificat principal H>=0 universel.
```

Le gateway proposé dans
[`NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md`](NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md)
fait davantage :

1. il utilise aussi les deux conditions d'owner `D-E>=0`, `D-X>=0` ;
2. un verdict `MIXED` subdivise le facteur le plus incertain parmi `A`, `B` et
   `C`, au lieu de garder `A x B` figé jusqu'aux feuilles de `C` ;
3. un verdict `ALL_STRICT_OWNER` émet un bloc de carriers sans développer ses
   triples.

Le gain `1,005` réfute donc l'ordonnance « WSPD fixe + split de C seulement ».
Il ne mesure pas le nombre de tâches du gateway triple adaptatif. Cette dernière
question reste ouverte et doit être tranchée par son microprototype autonome.

## 7. Amélioration immédiate : classifieur exact `NONE/ALL/MIXED` à paire fixée

La descente actuelle n'a que des certificats `NONE`. Pour une paire ponctuelle
`(a,b)` et une boîte témoin `X`, on peut aussi certifier **tout le nœud carrier**
en arithmétique entière.

Poser :

```text
D = ||a-b||^2,
M = a+b.
```

Noter `dmin(p,X)^2` et `dmax(p,X)^2` les distances extrêmes exactes d'un point à
une AABB, et utiliser les coordonnées doublées pour le milieu.

### `NONE`

```text
dmin(a,X)^2 > D
ou dmin(b,X)^2 > D
ou max_{x in X} ||2x-M||^2 <= D.
```

Les deux premiers tests excluent la lentille ; le dernier impose `H>=0` partout.
C'est le chemin actuel.

### `ALL_STRICT_OWNER`

```text
dmax(a,X)^2 < D,
dmax(b,X)^2 < D,
min_{x in X} ||2x-M||^2 > D.
```

Alors, pour tout point du nœud :

```text
E<D,
X<D,
H<0.
```

`ab` est strictement plus longue que les deux autres arêtes, donc aucun tie
n'est possible et chaque `PointId` du nœud est un carrier de face possédé. Le
nœud peut être crédité en `O(1)` pour le compteur, ou émis comme
`AcuteCarrierBlock` pour la source.

### `MIXED`

Tout autre cas descend. Les égalités restent `MIXED`, puis sont tranchées avec
l'`EdgeKey` aux feuilles ou dans un bloc dont les IDs permettent un verdict
uniforme.

Ce classifieur complète exactement les deux disjoints de Claude. Il ne résout
pas encore la masse des paires, mais il évite de descendre jusqu'aux carriers
individuels lorsque la sortie elle-même est dense.

## 8. La fixture du facteur deux doit vérifier la positivité q4

La fixture `owner-porteurs` calcule les longueurs et le nombre de faces aiguës,
mais le code exécuté ne vérifie ni le circumcentre annoncé ni ses poids
barycentriques. Le texte donne le bon contre-exemple ; la porte n'en atteste pas
encore la qualité « q4 strictement bien centré ».

Ajouter pour le tétraèdre de l'auditeur :

```text
centre = (83,81,97)/26,
R^2 = 7259/676,
poids = (70,49,109,110)/338,
```

avec :

```text
quatre distances égales,
déterminant affine non nul,
quatre numérateurs barycentriques > 0.
```

Puis tuer un mutant réellement raccordé :

```text
q4_exige_deux_faces_incidentes_aigues.
```

Une fonction qui imprime seulement `1+2=3` ne tue pas encore une source q4 qui
perdrait le support à un seul carrier.

## 9. Ordre recommandé à Claude

### P0

1. renommer `S4` ou calculer le vrai `V4` avant de publier les étages ;
2. appliquer le tie-break de face avec les vrais `PointId` ;
3. ajouter la fixture pipeline des positions dupliquées.

### P1

4. ajouter `ALL_STRICT_OWNER` à la descente par paire et produire des blocs ;
5. remplacer « logarithmique » par une rampe de visites par ancre ;
6. compléter la fixture q4 par barycentriques et mutant causal.

### P2

7. prototyper ensuite le gateway adaptatif `A x B x C`, en gardant l'ablation
   actuelle comme baseline « split de C seulement » ;
8. raccorder `Q4SeedAxisTopR4` seulement après séparation exacte
   `S4 -> V4 -> C4_face_owner_candidate -> M4_apex -> W4_positive -> H4_rank`.

## 10. Statut

Aucun fichier de code, test, reçu ou proposition n'est modifié par cet audit.
Les trois commits de Claude apportent des progrès réels : la scission possède
désormais son exact-once, les frontières sont gravées, et la région carrier à
paire fixée est beaucoup mieux comprise. Les corrections ci-dessus empêchent
simplement que ces bons résultats soient promus sous des noms plus forts que
les objets effectivement comptés.