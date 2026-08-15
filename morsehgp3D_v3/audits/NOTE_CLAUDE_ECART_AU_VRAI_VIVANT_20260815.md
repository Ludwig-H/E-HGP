# Note de Claude — l'écart au vrai vivant, mesuré exactement

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé.

C'est la mesure que je reportais depuis le matin, et elle borne tout le reste :
elle dit combien il reste à prendre, donc combien tout resserrement de
certificat peut valoir au maximum.

## 1. La méthode, et pourquoi elle n'est pas en `O(n^3)`

Le préfiltre est **fail-open** : toute ancre vraiment vivante est parmi les
survivantes. Il suffit donc de décider exactement les survivantes, qui sont bien
moins nombreuses que `C(n,2)`.

Coût : `C(n,2)` tests de budget — trois additions — plus `survivantes x n`
évaluations avec sortie anticipée dès `h_q` témoins trouvés. Le coût suit donc
le **résiduel**, pas le cube de `n`.

Deux contrôles. Sur `uniform, n=600`, ce chemin reproduit **à l'unité**
l'énumération exhaustive de `C(600,2)` : `17 479 / 42 294 / 45 913`. Et le vrai
vivant est **identique** à `s=6` et `s=8` — `148 077` sur `terrain, n=4 000` —
comme il doit l'être, puisqu'il ne dépend d'aucun paramètre du préfiltre.

## 2. Un estimateur que je retire, et pourquoi

J'avais commencé par échantillonner `K` paires. Il ne ratait aucune paire, et
chacune était décidée exactement. Mais sa **variance n'est pas expliquée**. Sur
`uniform, n=600`, contre le compte exact `45 913`, trois graines et trois
tailles donnent

```text
K= 5000 :  +2,85 %   -3,63 %   +1,44 %     ecart-type binomial  0,62 %
K=20000 :  -0,21 %   -1,42 %   +1,84 %                          0,31 %
K=80000 :  -0,20 %   +0,55 %    0,00 %                          0,15 %
```

soit trois à douze écarts-types. J'ai identifié une cause — un `xorshift64`
réduit par `% n`, donc sur ses bits les plus faibles — et l'ai remplacé par
`splitmix64` avec réduction par multiplication haute. **Cela n'a pas suffi.**

Extrapoler sur une variance qu'on ne comprend pas ne vaut rien. L'estimateur
reste dans le code, documenté hors du chemin de mesure, et aucun chiffre publié
n'en dépend. Il redeviendra le seul recours si le résiduel dépasse un jour ce
que l'exact absorbe.

## 3. Le mou entre `n=4 000` et `n=8 000` — et une contradiction que je signale

> [!CAUTION]
> **Le titre de cette section disait d'abord « le mou ne dépend pas de `n` ».
> Mes propres mesures le contredisent au-delà de `n=8 000`**, et je l'avais
> publié sans le voir. Le détail est en section 6 ; les chiffres ci-dessous
> restent exacts, mais leur portée s'arrête à `n=8 000`.

Lane q4, chemin courant — cœur `corner64`, `h_a` par dual-tree :

| famille | `n` | `s=6` | `s=8` |
| --- | ---: | ---: | ---: |
| `terrain` | `4 000` | `1,537` | `1,288` |
| `terrain` | `8 000` | `1,609` | `1,327` |
| `uniform` | `4 000` | `1,747` | `1,327` |
| `uniform` | `8 000` | `1,759` | `1,327` |
| `eight_clusters` | `4 000` | `3,652` | `2,779` |
| `eight_clusters` | `8 000` | `3,619` | `2,631` |

`uniform` donne `1,327` aux deux tailles, au millième. **Entre `n=4 000` et
`n=8 000`, le mou est donc une fonction de (famille, séparation) et non de la
taille.** Il aurait été confortable d'en conclure que les mesures à `n=4 000`
prédisent la cible `50 000`. La section 6 montre que c'est faux.

## 4. Ce que le mou est, et ce qu'il borne

C'est la **taxe d'universalité**. `h_coeur`, `h_a` et `h_b` exigent qu'un témoin
le soit pour **toutes** les paires du rectangle, alors que la vérité est par
paire : un point témoin de quatre-vingt-dix-neuf pour cent des paires compte
pour zéro. Le mou mesure exactement l'écart entre « décider par rectangle » et
« décider par paire ».

D'où une lecture qui n'était pas la mienne ce matin : **`s` est déjà le bouton
de raffinement**. Augmenter la séparation rétrécit les cellules, donc le domaine
du quantificateur universel. Le raffinement adaptatif que j'envisageais n'est
pas une idée nouvelle, c'est la version locale de ce que `s` fait globalement.

Et une borne dure : à `s=8`, sur `terrain` et `uniform`, le préfiltre est à
**33 % du plancher**. Aucun resserrement de certificat ne peut rendre plus que
ces trente-trois pour cent. Les quatre certificats adoptés aujourd'hui —
`corner64`, l'autorité à huit coins, le dual-tree, la borne couplée — ont donc
pris l'essentiel de ce qui était disponible sur ces deux familles.

`eight_clusters` est le seul cas ouvert : `2,6` à `2,8` fois le plancher. C'est
là, et seulement là, qu'un raffinement local se justifierait.

## 5. La séparation fait converger le mou vers un

Balayage exact à `n=4 000`, lane q4 :

| famille | `s=6` | `s=8` | `s=10` | `s=12` | `s=16` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `terrain` | `1,537` | `1,288` | `1,174` | `1,110` | `1,050` |
| `uniform` | `1,747` | `1,327` | `1,169` | `1,092` | — |

Le préfiltre **converge vers l'exactitude**. À `s=16` sur `terrain` il est à cinq
pour cent du plancher. Cela confirme la lecture de la section 4 : `s` est le
bouton de raffinement, et il ne plafonne pas.

Le coût suit, mais moins vite qu'annoncé. De `s=6` à `s=16` sur `terrain`, les
rectangles passent de `271 933` à `1 060 282` — un facteur `3,9` là où `s^3`
prédirait `19` — et le temps de `7,8` à `20,9` secondes, facteur `2,7`. Le
résiduel, lui, tombe de `227 640` à `155 414`.

## 6. LA CONTRADICTION, et je ne sais pas encore de quel côté elle tombe

Les sections 3 et 5 mesurent jusqu'à `n=8 000`. Or j'ai par ailleurs mesuré, sur
`terrain` à `n=32 000`, `58 684 461` survivantes à `s=6` et `9 117 531` à `s=8`.

Ces deux séries ne peuvent pas être vraies ensemble sous l'hypothèse implicite
que j'avais faite. Si le mou valait encore `1,327` à `n=32 000, s=8`, les vraies
ancres vivantes seraient `6,87` millions, soit `215` par point contre `39,2` à
`n=8 000` — un exposant de `2,23` entre les deux tailles, donc **quadratique**.

De deux choses l'une, et je ne sais pas laquelle :

1. **le mou explose entre `n=8 000` et `n=32 000`** — auquel cas ma section 3 ne
   se prolonge pas, et le préfiltre se dégrade bien à l'échelle, ce que le fait 2
   du reçu annonçait déjà pour `terrain, s=6` ;
2. **le vrai vivant devient quadratique** — auquel cas l'objet lui-même explose,
   et l'espoir d'une production output-sensitive tombe avec lui.

Les deux comptent, et la seconde serait beaucoup plus grave. La mesure à
`n=16 000` discrimine ; elle est en cours. Je ne prolonge aucune des deux
lectures avant de l'avoir.

Ce que je retiens de ma propre erreur : j'ai publié « le mou ne dépend pas de
`n` » sur deux tailles, alors qu'une troisième mesure déjà en ma possession la
contredisait. Deux points ne font pas une pente — c'est la règle du plan de test,
et je l'ai enfreinte dans le sens le plus confortable.

## 7. Un fait que je ne revendique pas encore

Les vraies ancres vivantes q4 passent de `148 077` à `313 806` quand `n` double
sur `terrain`, soit un exposant `1,084` ; `1,093` sur `uniform` et `1,148` sur
`eight_clusters`. Par point : `37,0 -> 39,2`, `103,0 -> 109,9`, `110,6 -> 122,6`.

Si cela tenait, **l'objet que le préfiltre doit produire serait quasi linéaire**,
et une production output-sensitive serait possible en principe — le préfiltre à
`s=8` en étant alors à un facteur `1,33`.

**Mais c'est un seul exposant par famille, et le plan de test en exige trois
successifs.** Je ne publie donc aucune pente ; la mesure à quatre tailles est en
cours. Et la section 6 montre que cet exposant `1,08` est précisément ce qui est
en cause : s'il tenait à `n=32 000`, le mou y vaudrait `6,5` et non `1,33`.
