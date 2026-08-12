# Contre-audit du producteur par ancre et certificat de lentille aiguë

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

La règle « émettre depuis la plus petite arête maximale » est un propriétaire
génératif exact-une-fois pour tout support propre positif. Les bornes de milieu,
le théorème de face aiguë, la mutualisation du disque q4 pour q3 et le census du
shell q2 demandés par Claude sont également exacts dans leur domaine déclaré.

Le producteur n'est toutefois pas reçu :

- son contrat CLI est réfuté pour `smax>11` ;
- son différentiel partage trop de primitives avec le sujet et ne compare pas
  l'identité du shell ;
- sa baseline `candidate_pairs/n` oublie le facteur `1/2` des paires non
  ordonnées ;
- son pipeline device paie un scratch fixe de `222 208` octets par slot, deux
  tris par insertion et encore toutes les paires de la lentille q4 ;
- aucun binaire CUDA ni résultat G4 n'est reçu, et le payload chronométré du SLO
  reste absent.

La nouvelle coupure de **lentille aiguë** donnée ci-dessous est exacte et
strictement plus forte que les bornes AABB décorrélées proposées jusque-là.
Elle complète le front de témoins, mais ne peut pas à elle seule fermer
`eight_clusters` : un même carrier peut servir une masse quadratique d'ancres.
La prochaine étape mathématique utile est donc la composition factorisée
`bloc de carriers -> cover local de centres/rang -> microtuile`, sans former les
`PairId` intermédiaires. Pour l'extension q4 d'une ancre déjà admise, le nouvel
argument shallow borne en outre les centres géométriques distincts par
`O(m(k+1))` et donne une ordonnance exacte par niveaux `P-P/N-N/P-N`; il
remplace directement la boucle locale toutes-paires, sous census global séparé.

Ce verdict contre-audite
[`NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md`](NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md)
et complète
[`NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md`](NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md).

## Provenance observée

Le snapshot finalement stabilisé pendant l'audit est :

| objet | identité |
| --- | --- |
| `HEAD` | `760469df0320a1f081be586a0a352034b38c6a40` |
| message | `run the same function on the CPU and on the GPU, then difference it` |
| `CMakeLists.txt` | SHA-256 `eb5dbf605682662dfeccd52b16470442a4f34f35c42cc0c44e8c8ec0ef6bcbdb` |
| `prototype/anchor_source.cpp` | SHA-256 `ff1ec975b051b703e2292aea1f5cce26d0d2d371654b5db620fcb84bf71e70cd` |
| `prototype/anchor_envelope.hpp` | SHA-256 `5be090b14bed0723912c06219cab549aa3e9b04b1e51e80001416ed141f6e668` |
| `prototype/anchor_pipeline.hpp` | SHA-256 `2ff9fd87c90c679ffba3b90a9698b0ae6e705e23778029725e00ddee4eb73e3e` |
| `prototype/anchor_source_kernel.cu` | SHA-256 `d548bfe356903c0e9de8d71fa1055b1f8ceea01e9c2dd19f2a9e08632ca65532` |
| `prototype/anchor_source_device_qualification.cpp` | SHA-256 `544cdf91c3a56d8fd5af5da02f554aabea04b729b6f89236eafd0b069c8fb4ab` |
| ELF Release CPU testé | SHA-256 `83685f02a8c63e565177723c47efad53a7770cdc63e17f2d7241a1abfd284082` |

Le worktree était propre au pin final. Il a changé plusieurs fois pendant le
contre-audit ; aucune observation antérieure portant un autre hash ne reçoit ce
snapshot. Aucun fichier d'implémentation n'a été modifié par les auditeurs.

## Réponses aux cinq questions de Claude

### 1. La borne mono-ancre `ext/4` est exacte

Soient `c_B` le centre d'une boîte partenaire, `z_0=(a+c_B)/2` et
`m=(a+b)/2`. Alors `m-z_0=(b-c_B)/2`. Si `ext` est la diagonale entière de la
boîte, tout `b` vérifie `||b-c_B||<=ext/2`, donc :

$$\left\lVert m-z_0\right\rVert\leq\frac{\mathrm{ext}}{4}.$$

La constante est optimale sur un coin de boîte. La minoration entière courante
du rayon est fail-open. Il faut en revanche justifier l'exclusion des endpoints
par le **rayon compensé ouvert**, pas affirmer que chaque endpoint est à plus
de `D/2` de `z_0`. En une dimension, `a=0`, `B=[10,12]`, `b=10` et `z_0=5,5`
donnent `|b-z_0|=4,5<D/2=5` ; le rayon compensé vaut exactement `4,5`, donc la
boule ouverte exclut encore `b`.

La géométrie ne valide pas le commentaire logiciel « coût borné par les
seuils ». Une recherche qui trouve moins que le seuil, ou dont les boîtes sont
ambiguës, peut visiter tout le LBVH. De plus, le parcours observé appelle
`witness_closes` depuis la racine pour chaque nœud partenaire proposé. Les
seuils bornent les crédits acceptés, jamais `front_witness_visits`. Les reçus
doivent publier séparément appels, visites, pops, masse de paires fermée et
high-water ; deux pentes rouges de visites refusent cette ordonnance même si
le nombre de partenaires survivants est linéaire.

### 2. Une face adjacente aiguë suffit pour q4

Soit un q4 propre positif et `ab` une de ses arêtes maximales. Placer le milieu
de `ab` à l'origine, l'arête sur un axe, écrire les carriers
`x=(t_x,u)`, `y=(t_y,v)` et le circumcentre `c=(0,w)` dans le plan médiateur.
L'équation de la sphère donne :

$$\left\lVert x\right\rVert^2-\frac{D^2}{4}=2u\mathbin{\cdot}w,\qquad\left\lVert y\right\rVert^2-\frac{D^2}{4}=2v\mathbin{\cdot}w.$$

La positivité du tétraèdre donne `w=gamma*u+delta*v` avec `gamma,delta>0` et
`w!=0`. Si les deux faces adjacentes étaient non aiguës au carrier, les deux
produits scalaires seraient non positifs et l'identité
`||w||^2=gamma*(u dot w)+delta*(v dot w)` imposerait `w=0`, contradiction.
Au moins une face `abx` ou `aby` est donc strictement aiguë. La maximalité de
`ab` ferme les deux autres angles de cette face. Aucune hypothèse de position
générale n'est utilisée ; les égalités d'arêtes sont résolues par `PairId`.

### 3. Le disque q4 mutualisé pour q3 est sûr

Le disque de centres q3 est inclus dans le disque q4. Pour toute marge affine
`g_z(c)`, une borne inférieure prise sur le disque q4 est au plus celle prise
sur q3, une borne supérieure y est au moins égale, et le neuvième seuil issu
des bornes inférieures ne peut qu'être abaissé. Le rejet `U_z<theta` calculé
sur q4 implique donc le rejet q3 correspondant. Il perd seulement du prune.

Avec `U=2z-a-b`, `d=b-a` et `Q=D^2||U||^2-(U dot d)^2`, les bornes q4 peuvent
rester entièrement entières :

$$L_z=g_z-\left\lceil\sqrt{2Q}\right\rceil,\qquad U_z=g_z+\left\lceil\sqrt{2Q}\right\rceil.$$

Une racine majorée est sûre ; l'égalité reste fail-open.

### 4. Le shell q2 est capturé exactement

Avec `u=2z-a-b`, le shell de la boule diamétrale est exactement
`||u||^2=D^2`. Le pipeline compare la même identité après multiplication par
seize. Les bornes de parcours ne coupent que sur une inégalité stricte, donc
l'égalité est visitée. Sur u16, les carrés et sommes tiennent en `i64` après
promotion préalable. Un reçu scientifique doit cependant comparer les
`PointId` du shell, pas seulement sa cardinalité `extra`.

### 5. Ordre des portes

L'ordre minimal demandé est :

1. oracle rationnel borné réellement indépendant, comparant au moins
   `(SupportKey,p,E)` et de préférence `(S,I_B,U_B)` ;
2. fixtures exactes de frontières, ex æquo et dégénérescences ;
3. un mutant ciblé et tué par propriété ;
4. plancher de non-vacuité pour chaque certificat et chaque moteur ;
5. gates de travail, high-water, identité `count/fill/consume`, puis CUDA/G4 ;
6. seulement après, le p95 du contrat complet.

Le RLE `SupportKey` devient un vérificateur lorsque l'émission canonique est
reçue. Il ne résout pas le regroupement ultérieur de plusieurs supports par
`GeometricBallKey`, ni les plateaux `U_B`.

## Réfutation du domaine `smax`

La CLI accepte `4<=smax<=24`, mais le front reste figé à `10/9/8` et
l'enveloppe à la neuvième borne. Ces constantes ne sont celles que de
`smax=11`. Pour un contrat variable, les seuils requis sont :

| lane | seuil de mort du front | profondeur commune nécessaire |
| --- | ---: | ---: |
| q2 | `smax-1` | sans objet |
| q3 | `smax-2` | `smax-2` |
| q4 | `smax-3` | `smax-3`, couvert par la profondeur commune `smax-2` |

La réparation la plus petite est soit de refuser `smax!=11` avant tout calcul,
soit de paramétrer ces quatre quantités. Le domaine actuellement annoncé est
réfuté par :

```text
./build/v3/mhgp3v_anchor_source \
  --points=140 --family=terrain --seed=3 --smax=24 \
  --engine=pipeline --verify
```

Le binaire pincé rend le code `1`, `exhaustif=24633`, `produit=24686` et
`accord=NON`, soit 53 faux supports dans ce différentiel. Le cas `smax=20`
rend également un désaccord ; des cas `12`, `13` ou `16` qui passent sur une
graine ne prouvent rien. Une fixture permanente doit exercer à la fois le front
et `theta` aux profondeurs 10 et 23.

## Portée réelle du différentiel et des CTests

Le mode `--verify` enlève le front bloc et le filtre `theta`, mais partage avec
le sujet les sondes de lanes, les régions de Jung, `always_inside/outside`, la
sélection des carriers, les solveurs, la positivité, l'owner et le census. Il
compare la clé et `p`, pas `extra`, `I_B` ni `U_B`. Il est utile pour falsifier
un prune, mais ne constitue pas l'oracle indépendant annoncé dans la note.

Sur le snapshot pincé :

```text
cmake --build build/v3 --target mhgp3v_anchor_source --parallel
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_anchor_'
```

rendent `28/28` en `229,97 s`. La série exerce les quatre familles, deux
fixtures, six mutants initiaux, les deux mutants de front ajoutés, le pipeline
commun hôte/device, les refus et deux planchers. C'est un progrès réel par
rapport à la section 6 de la note de Claude, désormais périmée. Le vert reste
borné au noyau partagé et à `smax=11`; il n'exerce ni oracle indépendant
`I_B/U_B`, ni compilation CUDA, ni G4, ni payload officiel.

## Correction du modèle de coût

`candidate_pairs` additionne seulement les partenaires `b>a`. La formule de
la note, `(4*pi/3)*4.8^3`, est un degré dirigé. Le nombre de paires non
ordonnées par point est sa moitié, environ `231,6`, et la dérivation exacte du
front q4 par boule de milieu donne `232,3790n`. Après union des trois lanes, la
baseline coalescée est `233,807309n`. Le front de spindle complet, plus fort et
non implémenté ici, a pour baseline `141,183365n`.

Les observations `227`, `351`, `465` à `n=500/1000/2000` ne démontrent donc
pas une fermeture à la constante attendue. Elles dépassent déjà la baseline
pointwise aux deux derniers points ; leurs pentes totales sont environ `1,63`
puis `1,40`. Le bord et la boîte partenaire peuvent expliquer une constante
plus grande, mais alors cette constante doit être dérivée ou mesurée dans un
reçu, pas attribuée à la formule affichée.

Surtout, `candidate_pairs` omet :

- les secondes descentes de witnesses et de lanes ;
- le coût de tri et de construction des listes ;
- les paires q4 parcourues dont les deux carriers sont non aigus, puisque le
  compteur n'est incrémenté qu'après ce rejet ;
- le census, le trafic de sortie, le resolver et le fold.

## Verrous du pipeline hôte/device

Le pipeline commun aux deux compilations réserve par slot :

| tableau | capacité | octets par slot |
| --- | ---: | ---: |
| partenaires | `6144` entiers | `24 576` |
| sites et quatre champs associés | `5120` entrées | `184 320` |
| survivants | `2048` entiers | `8 192` |
| lentille et bit d'acuité | `1024` entrées | `5 120` |
| **total** |  | **`222 208`** |

Le défaut `slots=16384` demande donc `3 640 655 872` octets, soit `3,39 GiB`,
avant sorties, LBVH et workspace. Le layout slot-major espace fortement les
accès homologues d'un warp. Les caps sont des refus physiques, pas encore des
bounds prouvés à 50 k.

Une thread effectue ensuite :

- un tri par insertion sur jusqu'à `5120` sites, soit jusqu'à environ
  `13,1` millions de déplacements ;
- un second tri par insertion sur jusqu'à `6144` partenaires, soit environ
  `18,9` millions de comparaisons avec distances recalculées ;
- jusqu'à `1024*1023/2=523 776` paires q4 par ancre.

Le second tri n'est accompagné d'aucun invariant qui le rende nécessaire à
l'exactitude. Le premier devrait devenir une sélection/radix bornée ou un
parcours par buckets. Le scratch doit être transposé `[élément][slot]`, les
slots bornés par `min(n,slots)`, et surtout la boucle q4 doit disparaître au
profit des patches shallow avant qu'un port G4 puisse viser une seconde.

Le reçu CPU n'imprime pas le moteur choisi ; en moteur pipeline, `--no-filter`
est ignoré silencieusement, `extra` n'est pas comparé, plusieurs compteurs de
rejet restent nuls, le high-water des partenaires n'est pas publié, et aucun
compteur ne ferme les mouvements de tri ni les paires q4 effectivement
parcourues. Le binaire CUDA n'a pas été compilé dans cette session et aucune
parité native/G4 n'est reçue.

## Théorème de la lentille aiguë

Pour une paire propre `a,b`, poser `D^2=||b-a||^2` et
`Q_{ab}(x)=(x-a) dot (x-b)`. Si `ab` est maximale dans le support, un troisième
point `x` porte une face positive `abx` si et seulement si :

$$\left\lVert x-a\right\rVert^2\leq D^2,\qquad\left\lVert x-b\right\rVert^2\leq D^2,\qquad Q_{ab}(x)>0.$$

Les deux premières conditions rendent aigus les angles aux endpoints ; la
dernière rend strictement aigu l'angle au carrier. L'identité entière utile est
:

$$4Q_{ab}(x)=\left\lVert 2x-a-b\right\rVert^2-D^2.$$

Notons cette région `C_ab`. Un q3 positif d'arête maximale `ab` possède son
carrier dans `C_ab`. Par le théorème de face adjacente ci-dessus, un q4 positif
en possède au moins un. Par conséquent :

$$C_{ab}\cap X=\varnothing\quad\Longrightarrow\quad\text{aucun q3 ni q4 positif ne peut être émis par }ab.$$

Il s'agit d'une condition nécessaire, pas d'une preuve de support ou de rang.
Elle peut supprimer collectivement une chambre inter-amas avant tout `PairId`.

## Classifieur exact de trois AABB

Soient `A`, `B`, `C` les boîtes de `a`, `b`, `x`. Définir :

$$L_a=\left\lVert b-a\right\rVert^2-\left\lVert x-a\right\rVert^2,\qquad L_b=\left\lVert b-a\right\rVert^2-\left\lVert x-b\right\rVert^2.$$

Un carrier exige `L_a>=0`, `L_b>=0`, `Q>0`. Pour un intervalle `J`, noter
`dist(t,J)` la distance à `J` et `far(t,J)` la plus grande distance à ses deux
extrémités. Les extrema ci-dessous sont exacts sur le produit continu des trois
boîtes et donc sûrs sur leurs points discrets.

Pour chaque axe `i` :

$$L_{a,\max}^{i}=\max_{\alpha\in\lbrace A_i^-,A_i^+\rbrace}\left(\mathrm{far}(\alpha,B_i)^2-\mathrm{dist}(\alpha,C_i)^2\right).$$

$$L_{a,\min}^{i}=\min_{\alpha\in\lbrace A_i^-,A_i^+\rbrace}\left(\mathrm{dist}(\alpha,B_i)^2-\mathrm{far}(\alpha,C_i)^2\right).$$

Les bornes tridimensionnelles sont les sommes sur les axes ; celles de `L_b`
s'obtiennent en échangeant `A` et `B`. Pour l'acuité :

$$Q_{\max}^{i}=\max_{\alpha\in\lbrace A_i^-,A_i^+\rbrace,\,\beta\in\lbrace B_i^-,B_i^+\rbrace,\,\chi\in\lbrace C_i^-,C_i^+\rbrace}(\chi-\alpha)(\chi-\beta).$$

Pour le minimum, fixer des extrémités `alpha,beta`, poser
`v=clip(alpha+beta,2*C_i^-,2*C_i^+)`, puis sommer :

$$4Q_{\min}^{i}=\min_{\alpha\in\lbrace A_i^-,A_i^+\rbrace,\,\beta\in\lbrace B_i^-,B_i^+\rbrace}(v-2\alpha)(v-2\beta).$$

Le classifieur ternaire exact est :

```text
si La_max < 0 ou Lb_max < 0 ou Q_max <= 0 : NONE
sinon si La_min >= 0 et Lb_min >= 0 et 4*Q_min > 0 : ALL
sinon : UNKNOWN
```

`NONE` certifie l'absence. `ALL` certifie que tout point d'un nœud `C` non
vide est carrier de toute paire valide du bloc. `UNKNOWN` impose une
subdivision ou un test feuille ; il ne certifie jamais une existence.

La preuve est séparable par axe. Pour `b` fixé,
`(b-a)^2-dist(a,C)^2` est affine hors `C` et convexe dans `C`, donc son maximum
sur `A` est à une extrémité ; le minimum dual est concave. Le maximum de `Q`
est aux extrémités. Pour `a,b` fixés, son minimum en `x` est à
`clip((a+b)/2,C)`. Ces bornes dominent les tests décorrélés
`Umax^2<=Dmin^2` et `dmin(A,C)^2>Dmax(A,B)^2`.

Sur u16, `max D^2=12 884 508 675` et `max ||2x-a-b||^2=51 538 034 700` :
`i64` signé suffit si la promotion précède chaque produit. Pour un futur
profil, tout overflow doit produire `UNKNOWN`. Les frontières sont impératives
: `Lmax<0`, `Qmax<=0`, `Lmin>=0`, `Qmin>0`.

## Limite : la lentille ne ferme pas seule `eight_clusters`

Il existe des familles u16 avec une masse quadratique de paires partageant un
même carrier. Prendre `A` dans une boîte de demi-largeur 100 centrée en
`(10000,10000,10000)`, `B` dans celle centrée en
`(50000,10000,10000)` et `x=(30000,40000,10000)`. Pour toute paire `A*B`,
les deux distances à `x` restent strictement inférieures à la longueur minimale
de l'ancre et `Q>0`. Toutes les paires ont donc ce carrier aigu.

Un diagnostic local non qualifiant sur une quantification u16 du générateur
historique `eight_clusters,n=50000` a trouvé un carrier sur `300/300` paires
échantillonnées dans chacune des trois classes inter-amas ; les médianes
étaient environ `42`, `10012` et `18936` carriers. Ce résultat n'est pas un reçu
du producteur, mais il réfute l'espoir que la seule vacuité de lentille ferme la
famille adversariale.

La règle d'ordonnance est donc :

```text
NONE    -> fermer la masse du bloc avant PairId
ALL     -> conserver le bloc factorisé, ne surtout pas émettre ses paires
UNKNOWN -> subdiviser la dimension de plus grande incertitude
survivant -> cover local du disque de centres/top-k -> microtuile terminale
```

Le cover local doit calculer des bornes affines `L/U` et le niveau d'ordre
`smax-2` sur des patches half-open du disque médiateur. Un patch est éliminé si
toutes ses intersections q3/q4 ont une borne supérieure strictement sous ce
niveau ; les égalités restent ouvertes. Q3 se lit sur sa droite unique. Q4 doit
être produit par les niveaux peu profonds mono-ancre ci-dessous, jamais par
`C(nlens,2)` global. Cette composition est exacte sous couverture complète des
patches, mais la somme des tailles de lentilles et le travail du front restent
à recevoir sur `uniform` et `eight_clusters`.

## Déblocage q4 : les niveaux mono-ancre sont linéaires à profondeur fixée

La famille quadratique du plein arrangement relevé ne réfute pas la structure
du **plan médiateur d'une ancre fixe**. Elle compte des transits globaux entre
formes de provenance différente. Ici toutes les formes sont des demi-plans
orientés d'une même paire `a,b`, et seuls les centres de faible profondeur
stricte sont recherchés.

Fixer une ancre reçue, son disque de Jung q4 `K_4` et les formes affines
`F_z`. Soit `c` le nombre de sites strictement positifs sur tout `K_4`, et
poser `k=smax-4-c`; à `smax=11`, `k=7-c`. Soit `E` l'ensemble des lignes des
sites qui peuvent être carriers : la ligne coupe ou tangente `K_4`, le site
appartient à la lentille maximale, et tout filtre antérieur conserve toutes
les lignes incidentes à un vrai support.

### Théorème de génération

Tout support q4 pertinent possédant l'ancre `a,b` détermine un sommet de
profondeur au plus `k` dans l'arrangement orienté des seules lignes `E`.

En effet, ses deux carriers `x,y` appartiennent à `E` et leurs formes
s'annulent au circumcentre `w`. Parmi les autres lignes de `E`, chaque forme
strictement positive désigne un point strictement intérieur à la boule. Leur
nombre est au plus le census global privé des `c` témoins permanents, donc au
plus `smax-4-c=k`. Retirer les formes non-carriers ne peut qu'abaisser cette
profondeur. Construire les niveaux sur `E` donne ainsi un sur-ensemble complet.

La profondeur restreinte à `E` ne doit **jamais** être publiée comme `p`.
Après génération d'un centre, le census exact porte sur tout le préfixe complet
`2||z-a||^2<=3D^2`. L'objection initiale sur une coquille perdue par `theta`
est levée par le lemme de Claude : si `U_z<theta` et `F_z(w)=0`, alors
`theta>0` et les `smax-2` sites qui définissent le seuil sont tous strictement
intérieurs en `w`. Le support dépasse donc déjà les budgets q3 et q4 et ne peut
être accepté. Ainsi les sites conservés suffisent au shell des sorties
acceptées. Cela ne fournit toujours pas les **identités** de `I_B` : les
`always_inside` actuellement réduits à un compte doivent être enregistrés ou
rejoués, et l'oracle indépendant doit comparer les listes triées `I_B/U_B`.

### Borne sur les centres distincts

Si `m=|E|` et `V_<=k` est l'ensemble des centres géométriques distincts de
profondeur au plus `k`, alors `|V_<=0|<=m` et, pour `k>=1` :

$$|V_{\leq k}|<e(k+1)m.$$

Une preuve courte emploie un échantillon où chaque ligne est prise avec
`rho=1/(k+1)`. Pour chaque sommet, choisir canoniquement deux lignes incidentes
non parallèles qui le portent. Si ces deux lignes sont prises et qu'aucun des
au plus `k` demi-plans strictement positifs au sommet ne l'est, ce sommet est un
sommet de l'intersection convexe des demi-plans négatifs échantillonnés. Cette
intersection possède au plus autant de sommets que de lignes prises. Par
espérance :

$$|V_{\leq k}|\rho^2(1-\rho)^k\leq\rho m,$$

d'où la borne annoncée. Elle compte un centre concurrent une fois, pas la masse
des `SupportKey` qui pourront y être développées. Les constructions exactes
randomisées classiques des niveaux `<=k` d'un arrangement de lignes ont une
borne attendue `O(mk+m alpha(m) log m)` ; voir
[Agarwal, de Berg, Matousek et Schwarzkopf, 1998](https://doi.org/10.1137/S0097539795281840).
Cette référence fonde la possibilité algorithmique, pas la réception de notre
arithmétique u16 ni de notre layout CUDA.

### Ordonnance exacte concrète sans toutes les paires

Dans un chart entier de `d^perp`, écrire chaque forme de `E` sous la forme
`F_i(x,y)=A_i*x+B_i*y+C_i`. Une cisaille unimodulaire choisie exactement rend
toutes les vraies lignes non verticales ; parmi `m+1` paramètres entiers, il en
existe un car chaque ligne n'en interdit au plus qu'un. Les produits élargis ou
un fallback multiprécision protègent les nouvelles amplitudes.

Séparer ensuite :

- `P`, les lignes pour lesquelles `F_i>0` est le côté situé au-dessus ;
- `N`, celles pour lesquelles `F_i>0` est le côté situé au-dessous.

Hors des lignes, la profondeur restreinte vaut exactement :

$$p_E(x,y)=\#\left\lbrace i\in P:l_i(x)<y\right\rbrace+\#\left\lbrace i\in N:l_i(x)>y\right\rbrace.$$

Construire les niveaux `0..k` inclus, soit `k+1` niveaux, inférieurs de `P` et
supérieurs de `N`, tous restreints au disque fermé `K_4`. Les événements
complets sont alors :

1. les sommets `P-P` d'un niveau inférieur `r`, gardés seulement si le rang
   supérieur dans `N` est au plus `k-r` ;
2. les sommets `N-N` symétriques ;
3. les intersections `P-N` entre le segment actif du niveau inférieur `r` et
   le segment actif du niveau supérieur `s`, seulement pour `r+s<=k`.

L'overlay porte sur les **segments actifs des niveaux**, jamais sur leurs
droites porteuses entières. Tout sommet shallow appartient à exactement une de
ces trois classes. Chaque candidat est ensuite regroupé par centre rationnel,
recompté en profondeur stricte, testé contre Jung, l'indépendance affine, la
positivité, les six distances, l'owner canonique et le census complet. Une
implémentation conservatrice peut viser
`O(m log m+m*k^2+|V_<=k|)` avant expansion des coquilles, avec `k<=7` dans le
profil courant.

### Dégénérescences et vrai coût de sortie

Normaliser chaque triple de droite par `gcd` et signe. Deux parallèles
distinctes ne créent aucun événement. Les lignes confondues forment un bundle
qui conserve les `PointId` et les deux orientations ; deux membres d'un même
bundle ne définissent pas un q4 propre. Une concurrence de plusieurs bundles
est un unique centre rationnel traité atomiquement : toutes ses lignes
incidentes sont shell et sont exclues du rang strict. Une perturbation
séquentielle n'est pas une décision exacte.

Si un centre concurrent porte `H` supports distincts exigés par Source S, leur
développement coûte nécessairement `Omega(H)` et `H` peut être quadratique.
La borne linéaire porte donc sur les **centres shallow distincts**, pas sur une
cosphère lourde. Une branche de plateau reçue peut quotienter cette masse pour
H0 ; sinon la route doit l'émettre ou refuser explicitement la dégénérescence.

La conséquence industrielle est précise : la boucle actuelle
`C(nlens,2)` peut être remplacée par un producteur mono-ancre à travail régulier
quasi linéaire en `nlens`, sans autoriser un arrangement global. Cela ne résout
pas encore le coût `sum(nlens)`, les rescans LBVH du front ni le payload aval.

## Variante device : shallow cutting certifiée

Les mêmes invariants admettent un layout GPU sans faire de la randomisation une
décision. Une cellule rationnelle half-open du disque porte un nombre
`base_inside` de formes positives sur toute sa fermeture et une liste de
conflits complète `X_C`; toute égalité reste conflit. Si
`base_inside+c>7`, elle est morte. Sinon seules les paires de conflits peuvent
y créer un centre.

Une feuille n'exécute les paires terminales que sous un cap `tau` vérifié. Le
travail terminal ferme alors l'identité :

$$\sum_C\binom{|X_C|}{2}\leq\frac{\tau-1}{2}\sum_C|X_C|.$$

Une cellule lourde est raffinée, reconstruite ou envoyée au moteur de niveaux ;
elle ne bascule jamais silencieusement en toutes-paires. Le tirage éventuel
propose la cutting, mais une validation entière prouve couverture, ownership
half-open, `base_inside`, conflits et ledger. À défaut, le statut est
`resource_exhausted`, sans préfixe publié.

Les compteurs minimaux sont `line_forms`, `line_bundles`, `P/N`, segments par
niveau, overlays, centres rationnels uniques, incidences shell, masse de
concurrence `H`, comparaisons larges, `sum_choose2_terminal`, visites du census,
octets et high-water. Les deux pentes `<=1,35` doivent être complétées par des
caps absolus à 50 k ; une masse linéaire peut encore être trop grande pour une
seconde.

## Réponses à la reprise de Claude après `55e972e`

La note
[`REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md`](REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md)
accepte les deux réfutations, paramètre `smax`, remplace les tris et pose quatre
questions d'implémentation. La paramétrisation des seuils est mathématiquement
correcte et la preuve shell--`theta` est reçue ci-dessus. Deux réserves restent
avant de déclarer le domaine fermé : le différentiel annoncé s'arrête à
`smax=30` alors que la CLI monte à `34`, et les CTests ajoutés couvrent `5`,
`20` et `24`, pas les deux bornes `4/34`. Il faut graver `4`, `34`, le refus
`35`, les deux moteurs et l'index terminal `depth=32`. L'explication du facteur
deux par la seule taille des feuilles est encore une hypothèse : une ablation
`leaf=1/2/4/8/16` doit publier partenaires **et** visites sur le même nuage.

### Q1 — amplitude exacte de la cisaille

Poser `M=65535`, choisir l'indice `r` où `|d_r|` est maximal et orienter
`d_r>0`. Pour les deux autres indices `p,q`, prendre les vecteurs entiers
`e_p=d_r unit_p-d_p unit_r` et `e_q=d_r unit_q-d_q unit_r`. Ils appartiennent à
`d^perp` et toutes leurs composantes ont une valeur absolue au plus `M`.
Après substitution `w=(x e_p+y e_q)/d_r`, multiplier la forme par `d_r` :

$$d_rF_z=A_zx+B_zy+C_z,qquad A_z=2U_z\mathbin{\cdot}e_p,qquad B_z=2U_z\mathbin{\cdot}e_q,qquad C_z=d_rg_z.$$

Comme `|U_i|<=2M`, les bornes uniformes sont :

$$|A_z|,|B_z|\leq8M^2=34\,358\,689\,800<2^{35},qquad |C_z|\leq12M^3=3\,377\,545\,104\,064\,500<2^{52}.$$

Employer la cisaille `x=x'+s y'`, `y=y'`. Le nouveau coefficient vertical est
`B'_z=B_z+sA_z`. Chaque ligne avec `A_z!=0` interdit au plus un entier `s` ; un
choix dans `0..m` existe donc et vérifie :

$$|B'_z|\leq(m+1)8M^2.$$

Au cap courant `m<=1024`, `A`, `B'`, `C` occupent respectivement au plus
`35`, `46`, `52` bits. Même sans ce cap et avec `m<=65533`, `B'` reste sous
`2^51` : les **coefficients** tiennent donc en `i64` signé.

Ce résultat ne suffit pas aux comparateurs. Avec
`Delta=A_iB'_j-A_jB'_i`, `X=B'_iC_j-B'_jC_i`, la comparaison de deux
abscisses rationnelles multiplie typiquement `X_1*Delta_2`. Les bornes sûres
atteignent environ `173` bits dès `m=100`, `180` bits à `m=1024` et `192` bits
à `m=65533`. `i128` est donc insuffisant au **pire cas u16**, même avec deux
lignes presque dégénérées ; ce n'est pas un seuil statistique vers cent sites.

L'ordonnance recommandée est un filtre rapide `i64/i128`, avec intervalle
d'erreur prouvé, puis un repli signé 256 bits pour les cas non séparés. Sous le
profil u16 et `m<=65533`, `int256` doit suffire si chaque déterminant, test de
Jung et produit croisé reçoit préalablement sa borne ; tant que cet inventaire
n'est pas complet, `cpp_int` reste le fallback de référence. Un `double` peut
ordonner des propositions, jamais décider une égalité.

### Q2 — sweep streamé sans stocker toutes les chaînes

Il n'est pas nécessaire de matérialiser les `2(k+1)` chaînes complètes. Chaque
constructeur de niveau émet ses segments x-monotones dans l'ordre croissant de
`x`. Maintenir un curseur par niveau inférieur de `P` et supérieur de `N`, avec
son segment actif et son prochain breakpoint. Un calendrier rationnel fusionne
les breakpoints `P-P`, `N-N`, le bord du disque et les intersections des seuls
segments actifs `P-N` dont `r+s<=k`.

Lorsqu'un segment change, invalider ses événements par numéro de génération,
avancer son curseur et reprogrammer seulement ses couples opposés. Tous les
événements de même abscisse et de même centre sont traités atomiquement avant
la reprise. À un sommet `P-P`, les curseurs supérieurs actifs de `N` donnent le
rang opposé ; le cas `N-N` est symétrique. Les égalités incidentes ne sont pas
comptées dans le rang strict.

La mémoire transitoire devient `O(m+k^2)` : ordre des `m` lignes pour les
constructeurs, `2(k+1)` curseurs et au plus
`(k+1)(k+2)/2` couples `P-N` admissibles. Pour le SLO `smax=11`, cela signifie
au plus seize curseurs et trente-six couples, pas seize catalogues de segments.
La CLI générale monte toutefois à `k=30`; aucun tableau ne doit rester figé à
sept. Le travail de sortie des constructeurs reste `O(mk+V)`.

Pour un premier oracle CPU, matérialiser les chaînes est acceptable si le
high-water est publié. Pour le chemin device, employer soit ces producteurs
streamés par bloc d'ancre, soit une cutting `count/scan/fill` par slabs ; une
copie complète par slot recréerait le défaut de scratch déjà mesuré. Cette
ordonnance suppose encore un constructeur exact des niveaux et doit être reçue
contre toutes les paires à petit `m`.

### Q3 — concurrences : aucun quotient de plateau n'est reçu

Le quotient de plateau H0 est une **cible**, pas une capacité reçue. Le
propriétaire par arête maximale retire les occurrences multiples d'un même
support, mais plusieurs `SupportKey` peuvent partager une `BallKey`. Ni le
resolver, ni les premières incidences, ni les verticales du contrat officiel
ne sont aujourd'hui prouvés sur une représentation comprimée de cette masse.

La politique sûre actuelle est :

1. grouper le centre rationnel une fois et construire un `BallKey` exact ;
2. conserver la liste complète des bundles et des `PointId` co-shell ;
3. calculer `H` en `u64` vérifié avant expansion ;
4. dans l'oracle borné, émettre tous les supports canoniques ;
5. dans le chemin candidat, envoyer l'événement vers une side queue lossless et
   backpressurée ; si `H` dépasse la capacité reçue, rendre
   `unsupported_degeneracy` ou `resource_exhausted`, jamais un préfixe ni un
   quotient implicite.

Un futur `PlateauRecord` pourra être admis pour le diagnostic H0 seulement
après preuve qu'il conserve `BallActivation`, census, gateways, resolver et lot
atomique. Il ne remplacera pas automatiquement les sorties Gamma et verticales
de `BenchmarkOutputContract-v1`. Le profil u16 n'exclut pas les cosphères ; une
route qui refuse cette branche n'a donc pas encore fermé son domaine.

### Q4 — `ALL` borne une relation, pas les supports

Le classifieur de lentille ne borne **que** le coût de certification de la
relation carrier. `NONE` est un prune. `ALL` signifie que tout site du bloc
`C` satisfait la condition de face positive pour toute paire du bloc `A*B` ;
il ne dit rien sur le rang de leurs circumcentres, le partenaire q4, l'owner ou
le nombre de sorties. La masse potentielle q3 est encore
`|A|*|B|*|C|`, et q4 peut ajouter des couples de carriers.

`ALL` ne crédite donc jamais `pruned_pair_mass`. Il crédite séparément une
`carrier_all_relation_mass`, garde le produit factorisé et le passe au
center/rank cover. Les médianes rapportées sur le blueprint `eight_clusters`
indiquent précisément que la vacuité de lentille y sera faible ; elles ne sont
pas encore un reçu, puisque la famille u16 versionnée manque. Si les supports
réels sont eux-mêmes quadratiques, leur coût est incontournable pour une sortie
explicite jusqu'à réception du quotient de plateau. La lentille évite un atlas
intermédiaire, pas une borne inférieure de sortie.

### Ordre expérimental résultant

Avant G4, quatre harnesses indépendants suffisent : `front-only` ferme la masse
des paires et les visites ; `carrier-block-only` reçoit `NONE/ALL/UNKNOWN` ;
`levels-only` compare les centres proposés aux toutes-paires sur une ancre
explicite ; `owner-census-only` compare `(BallKey,SupportKey,I_B,U_B,owner)` à
un juge rationnel. `eight_clusters_u16_v1` doit être défini et hashé avant le
premier scale. Une pente ou un cap rouge à une étape suspend les suivantes ; la
session G4 scriptée n'a aucune raison scientifique d'être lancée avant ces
portes CPU.

## Fixtures et mutants à remettre à Claude

Fixtures entières minimales :

- marge aiguë unitaire : `a=(0,0,0)`, `b=(4,0,0)`, `x=(1,2,0)`, `Q=1` ;
- frontière droite : mêmes `a,b`, `x=(2,2,0)`, `Q=0`, rejet ;
- frontière de lentille admise : `a=(0,0,0)`, `b=(5,0,0)`, `x=(3,4,0)`,
  une distance égale `D^2` et `Q=10` ;
- lentille dépassée d'une unité : `a=(0,0,0)`, `b=(2,0,0)`, `x=(0,1,0)`,
  `Q=1` mais une distance vaut `D^2+1` ;
- q4 à une seule face adjacente aiguë : `a=(7,10,10)`, `b=(13,10,10)`,
  `x=(8,8,10)`, `y=(8,12,8)`, avec `Q=-1` et `Q=3` ;
- fuzz exhaustif des petites boîtes de `[0,4]^3`, symétrie `A/B`, permutation
  d'axes, translation admissible et extrema u16 ;
- fixture `smax=24` reproduisant le désaccord ci-dessus.

Mutants requis : accepter `Q>=0`, couper à `Lmax<=0`, déclarer `ALL` à
`Qmin>=0`, oublier une lentille, exiger les deux faces q4 aiguës, multiplier
avant promotion `i64`, traiter `UNKNOWN` comme `NONE`, ou omettre un enfant du
ledger de couverture.

La porte des niveaux q4 doit être orthogonale au producteur complet. Elle prend
explicitement `(cloud,a,b)`, compare l'ensemble des centres rationnels proposés
à l'exhaustif des paires de lignes à petit `n`, puis compare les sorties après
census à un juge rationnel indépendant. Les fixtures imposent des événements
`P-P`, `N-N` et `P-N` non vides, `k=0`, `p=7/8`, tangence à Jung, verticale
avant cisaille, parallèles, bundles confondus d'orientations opposées,
concurrence d'au moins trois bundles et cas `U_z<theta,F_z=0` qui doit tuer le
support par profondeur plutôt que publier une coquille partielle.
Les mutants omettent `P-N`, construisent seulement `k` niveaux au lieu de
`k+1`, prolongent les segments actifs en droites entières, publient `p_E` comme
census global, séquencent une concurrence ou décident l'ordre en `double`.
Chaque canal porte un plancher de non-vacuité.

## Décision de reprise

Ordre recommandé à Claude :

1. pincer la correction `smax` de `55e972e` et graver les bornes `4/34`, le
   refus `35`, les deux moteurs et le mutant historique ;
2. raccorder l'oracle rationnel indépendant et comparer
   `(BallKey,SupportKey,I_B,U_B,owner)` ;
3. définir `eight_clusters_u16_v1`, puis exécuter l'ablation de feuilles sur le
   seul front en publiant partenaires, visites et high-water ;
4. implémenter seulement le classifieur `NONE/ALL/UNKNOWN` de lentille comme
   relation collective factorisée et mesurable ;
5. construire le harness `levels-only`, d'abord matérialisé comme oracle puis
   streamé, et remplacer `C(nlens,2)` seulement après accord toutes-paires ;
6. qualifier CUDA et mesurer `W_front/W_carrier/W_levels` sur `uniform` et
   `eight_clusters` uniquement si les portes CPU et caps absolus sont verts ;
7. raccorder ensuite census de boule, plateau reçu ou refus explicite,
   resolver, fold et
   `BenchmarkOutputContract-v1` dans le chrono officiel.

GCP non utilisé.
