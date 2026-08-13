# Note de Claude — une décomposition en triplets AIGUS pour q3 : pertinence, forme, coût

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

Proposition soumise à votre jugement **avant** implémentation. Elle ne s'insère
pas dans l'ordre que vous avez fixé — `ProjectiveWindowCounter`, puis
`LocalShallowBall`, puis census — et je ne la coderai pas sans votre accord sur
sa place.

## 1. Le problème qu'elle vise, chiffré

Le moteur traite q3 par un **produit**. Pour chaque ancre `a`, il balaie ses
partenaires, et pour chacun il balaie la lentille. High-water mesurés à
`n=3 000` : `partenaires = 2 019`, `lentille = 230`. À `n=50 000`, avec la
fenêtre certifiée `kept \approx 474` par ancre :

$$474 \times 230 \times 50\,000 \;\approx\; 5{,}4\cdot 10^{9}\ \text{triplets candidats}$$

pour une sortie q3 d'environ `12` M supports. **Un facteur `450` de
sur-couverture**, et chaque candidat paie une circumboule et un census.

## 2. Pourquoi « bien séparé » serait le mauvais outil, et ce qu'il faut à la place

Une décomposition en triplets **bien séparés** au sens de Callahan–Kosaraju ne
sert à rien ici : un support q3 a `ab` pour arête maximale, donc
`\lVert ax\rVert\le\lVert ab\rVert` et `\lVert bx\rVert\le\lVert ab\rVert`. Le
triplet est **compact**, jamais séparé.

Ce que le certificat demande n'est pas l'éloignement mutuel mais que les boîtes
soient **petites devant le diamètre du triplet**. J'appelle cela **bien
échelonné** : `\max(r_A,r_B,r_C)\le D/s` avec `D` le diamètre, les boîtes
pouvant être adjacentes.

## 3. Les trois conditions du porteur n'en font qu'une

L'identité `(a-x)\cdot(b-x)=\lVert x-m\rVert^2-D^2/4` donne
`M_0>0 \iff H<0 \iff` **angle aigu en `x`** `\iff x` hors de la boule
diamétrale. Et `ab` maximale place le plus grand angle en `x`. Donc

> `ab` arête maximale **et** angle aigu en `x` `\iff` le triangle est **aigu**.

Les deux inégalités de longueur et l'acuité ne sont pas trois contraintes mais
une seule, et la région des porteurs est exactement **la lentille privée de la
boule diamétrale**. C'est déjà ce que `rect_carrier_by_margins` décide, avec son
juge exhaustif vert sur `30 000` triplets de boîtes.

## 4. Ce que l'acuité offre, et qui change tout le calcul

**Le circumcentre d'un triangle aigu est intérieur au triangle.** Donc il est
dans l'enveloppe convexe de `\lbrace a,b,x\rbrace`, donc dans le **hull des
trois boîtes** — un encadrement **affine, sans division**. Ma crainte du
dénominateur qui s'annule ne concernait que les triangles obtus, précisément
ceux qu'on exclut.

Et `ab` maximale plus acuité donnent

$$\frac{D}{2}\;\le\;R\;\le\;\frac{D}{\sqrt3},$$

un intervalle de rapport `1{,}155` seulement.

**Conséquence directe, et c'est le point.** Un `z` intérieur à *toute*
circumboule du bloc doit être à distance `< R_{\min}` de *tout* centre possible,
donc de tout point du hull. Le témoin universel d'un triplet de boîtes est donc

$$\max_{c\in \mathrm{hull}(A,B,C)}\lVert z-c\rVert^2 \;<\; \frac{D_{\min}^2}{4},$$

où `D_{\min}` est le minimum exact de `\lVert b-a\rVert^2` sur `A\times B`. Le
membre de gauche est un **maximum de distance carrée entre deux boîtes** —
séparable, entier, huit coins, exactement `rect_maxsq`. Le certificat de
triplet est donc **aussi bon marché que celui de paire**, et il rend `ALL`,
`NONE` et `MIXED` par le même mécanisme.

## 5. Le cardinal, et l'endroit exact où je doute

Pour chaque paire `(A,B)` du front WSPD — `O(s^3n)` par théorème — les nœuds
porteurs vivent dans la lentille, de volume `O(D^3)`. Des nœuds de rayon
`\sim D/s` y ont un volume `\sim (D/s)^3`, donc par empilement il y en a
`O(s^3)`. D'où

$$\#\text{triplets aigus} \;=\; O(s^6\,n).$$

Avec mon front mesuré de `51` records par point à `s=2`, cela donne de l'ordre
de `400` à `1 500` triplets par point, soit `20` à `75` M à `n=50 000` — contre
`5,4\cdot 10^9` candidats aujourd'hui. **Un facteur `70` à `270`.**

**Mon doute est sur l'empilement**, et c'est exactement le point où vous m'avez
repris ce matin. L'argument suppose des nœuds **disjoints** et de rayon
comparable. Il est valide pour des **cellules de Morton**, qui sont alignées et
disjointes par construction ; il ne l'est pas automatiquement pour mes **boîtes
serrées**, qui sont incluses dans les cellules mais peuvent être bien plus
petites. Je ne sais pas si la borne survit au passage aux boîtes serrées.

## 6. Ce que la construction NE fait pas

Elle **élimine** des blocs de non-supports ; elle ne **produit** aucun support.
Les blocs `MIXED` doivent être raffinés, et la positivité, le rang, l'owner
canonique et le census restent par sortie. Le gain porte donc sur le facteur
`450` de sur-couverture, pas sur les `12` M sorties elles-mêmes.

Elle ne remplace pas non plus le certificat de témoins : c'est un **second**
élagage, sur un objet différent — le triplet, et non la paire.

## 7. Trois questions

1. **L'empilement tient-il sur boîtes serrées**, ou faut-il faire porter la
   sélection des nœuds porteurs sur les **cellules** de Morton pour garder la
   borne, quitte à certifier ensuite sur la boîte serrée incluse ?
2. Le certificat de la section 4 majore le rayon possible par `R\ge D_{\min}/2`.
   Est-ce recevable tel quel, ou exigez-vous le majorant exact
   `R\ge \lVert b-a\rVert/2` par paire, ce qui interdirait de factoriser sur le
   rectangle ?
3. **Où placez-vous ceci dans votre ordre ?** Je penche pour « après le census
   par `BallKey` », comme une ablation mesurée du facteur `450`, et non comme un
   préalable. Mais si vous jugez que le produit `partenaires × lentille` est le
   verrou du contrat, il faudrait le remonter.

## 8. Non-claims

Rien n'est implémenté. Les `400` à `1 500` triplets par point sont une
**estimation** dérivée de mon front mesuré et d'un argument d'empilement dont je
doute moi-même. Le facteur `450` est mesuré ; le facteur `70` à `270` ne l'est
pas. Le relèvement à q4 avec `R\le D\sqrt{3/8}` est mentionné mais non étudié.
