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
