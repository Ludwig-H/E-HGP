# Note de Claude — le mur est la lentille, et c'est un effet de dimension

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

Diagnostic du mur que vous aviez annoncé et que ma session a mesuré.

## 1. Le mécanisme, isolé

Vous relevez que l'ordonnance est `for i in lens : for j > i`, donc un coût en
`\lvert lens\rvert^2/2` par paire. Voici le high-water `lens` mesuré :

| famille | `6 250` | `12 500` | `25 000` | `50 000` | pente |
| --- | ---: | ---: | ---: | ---: | :---: |
| `uniform` | `256` | `254` | `306` | `318` | **`\approx 0`** |
| `scanline_overlap_multiecho` | `1 596` | `3 311` | `5 389` | — | `0,99 / 0,70` |
| `terrain` | `2 895` | `4 349` | `7 811` | — | `0,59 / 0,85` |
| `eight_clusters` | `3 373` | `5 631` | — | — | `0,74` |

**Sur `uniform` la lentille est BORNÉE** — `256` puis `318` alors que `n` est
multiplié par huit. Sur les trois autres elle croît en `n^{0,6}` à `n^{1,0}`.

Le carré rend la pente du temps immédiatement : `terrain` mesure `3,19 / 2,69`,
et `1 + 2\times 0,85 = 2,7`. Le mur est donc **entièrement** dans la lentille,
et non dans le nombre d'ancres, de partenaires ou de sorties.

À `terrain, n=25 000`, une seule paire coûte
`7811^2/2 \approx 3,05\cdot 10^{7}` couples q4.

## 2. Pourquoi la lentille explose là et pas ailleurs

C'est un **effet de dimension**, et il vise exactement le régime cible.

Sur `uniform` la densité est **volumique** : les paires survivantes sont à
l'échelle du plus proche voisin, et la lentille — région de diamètre
`\sim D` — contient `O(1)` points.

Sur `terrain` et `scanline` le nuage est une **nappe quasi bidimensionnelle**.
Le même nombre de voisins y couvre une distance `3D` bien plus grande, et la
lentille, qui reste une région **volumique**, capture une **plaque entière** de
la surface. Sa population croît alors comme le carré du rayon en unités de
surface, non comme son cube en unités de volume.

Autrement dit : **le producteur suppose implicitement une densité volumique**,
et les nuages LiDAR — le régime que le contrat vise — ne l'ont pas.

## 3. Ce que ce diagnostic implique pour la correction

Il exclut toute réparation qui garderait l'énumération ponctuelle de la
lentille : `\lvert lens\rvert` n'est pas borné, donc `\lvert lens\rvert^2` non
plus, à aucune constante près.

Il désigne en revanche une propriété exploitable. Sur une nappe, les points de
la lentille occupent `O(s^2)` cellules d'octree à l'échelle `D/s`, et non
`O(s^3)` : la structure **s'adapte** à la dimension intrinsèque du nuage, là où
l'énumération ponctuelle ne le fait pas. Une décomposition en blocs paie donc
`O(s^2)` sur les nappes et `O(s^3)` au pire, alors que l'énumération ponctuelle
paie `\lvert lens\rvert^2` dans les deux cas.

C'est précisément ce que fait la proposition de
[triplets aigus](NOTE_CLAUDE_DECOMPOSITION_TRIPLETS_AIGUS_20260813.md) : elle
remplace `for i : for j > i` sur des **points** par une énumération de **blocs**
`(A,B,C)`. Sur `terrain` à `n=25 000`, cela remplacerait `3,05\cdot 10^{7}`
couples par quelques dizaines de blocs par paire.

Je ne présente pas cela comme une mesure : c'est un argument de structure, et la
question du pavage que je vous posais reste entière.

## 4. Ce que le diagnostic ne dit pas

Il ne dit pas que la décomposition en blocs suffit — la positivité, le rang,
l'owner et le census restent par sortie, et la sortie elle-même vaut `21,4` M
supports à `uniform 50 000`. Il ne dit pas non plus que `uniform` est
représentatif : sa lentille bornée est précisément ce qui le rend facile, et
c'est la seule famille où le producteur tient.

## 5. Une question

Ce diagnostic change-t-il votre verdict sur les triplets aigus ? Vous les
receviez comme une « troisième voie » possiblement à écarter. Si le mur est bien
la lentille, et si la structure de blocs est ce qui s'adapte à la dimension
intrinsèque, alors ce n'est plus une variante mais **la** réparation de
l'ordonnance que vous réfutez — et sa place dans votre ordre remonte.

## 6. CORRECTION — la lentille est le symptôme, pas la cause

J'ai intitulé cette note « le mur est la lentille ». C'est faux, et ma propre
mesure le montre.

D'abord, la **linéarité** que je proposais est vérifiée : `1 199 967` triples
`(a,b,z)` avec un `t` perpendiculaire tiré au hasard, comparés à la définition
brute `\lVert z-c\rVert^2<\lVert a-c\rVert^2`, **zéro désaccord**. Chaque site
donne bien un demi-plan de l'espace des paramètres.

Ensuite, j'ai mesuré la lentille et le `\le 7`-niveau **en fonction du rang du
voisin**, sur une nappe de `6 000` points — `a` tiré au hasard, `b` son `k`-ième
plus proche voisin :

| rang de `b` | lentille | sommets de niveau `\le 7` |
| ---: | ---: | ---: |
| `1` – `2` | `\approx 1` | `0` |
| `4` | `2` | `28` |
| `8` | `3` | `125` |
| `32` | `13` | **`1 883`** |
| `128` | `56` | `448` |
| `512` | `251` | **`0`** |

Deux faits, et ils renversent le diagnostic :

1. **Pour les paires qui produisent des supports, la lentille vaut `2` à `56`,
   jamais `7 811`.** Le high-water `lens=7811` mesuré sur `terrain` provient
   donc de paires **bien au-delà** du rang où quoi que ce soit existe.
2. **Les supports disparaissent avant que la lentille ne grossisse.** Le
   `\le 7`-niveau culmine au rang `32` et vaut **zéro** au rang `512`, où la
   lentille vaut déjà `251` — soit `31\,500` couples parcourus pour **aucune**
   sortie.

Le mur n'est donc pas `\lvert lens\rvert^2` : c'est que le moteur **traite des
paires qui ne peuvent rien produire**. Sa liste de partenaires monte à `4 380`
sur `terrain` à `n=25\,000`, alors que tout se joue sous le rang `\approx 128`.

## 7. Ce que cela change à la réparation

Ma proposition de triplets aigus, et même l'arrangement de la section
précédente, réparent le **mauvais étage**. Ils rendraient moins cher un travail
qui ne devrait pas avoir lieu.

La réparation est **en amont** : fermer ces paires, ou ne pas les proposer. Or
c'est exactement l'office du certificat de front — et le fait que `terrain`,
`scanline` et `eight_clusters` murent dit que le front **n'y ferme pas assez**,
pas que la lentille y soit intrinsèquement grosse.

Cela recoupe une mesure que j'avais déjà et dont je n'avais pas tiré cette
conséquence : à `s=3`, les pentes du degré résiduel valent `1,858 / 1,887 /
1,931` sur `eight_clusters`. Le front y laisse passer une masse quasi
quadratique de paires, et le moteur la paie au carré.

**La question à vous poser n'est donc plus « comment accélérer la lentille »
mais « pourquoi le front ne ferme-t-il pas les paires longues sur les nuages
structurés ».** Je retire la conclusion de ma section 3, qui désignait la
décomposition en blocs comme la réparation : elle en était une pour un mur qui
n'est pas le bon.
