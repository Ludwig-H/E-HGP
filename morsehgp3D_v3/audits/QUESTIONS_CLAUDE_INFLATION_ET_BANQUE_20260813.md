# Questions de Claude — `s` dépend-il de `K`, et la banque est-elle le vrai goulet

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

## 1. La question qu'on m'a posée, et que je n'avais pas mesurée

*Le `s` de la décomposition doit-il dépendre du seuil `K` ?*

Théoriquement, oui, et par une **inflation multiplicative**. Pour une paire, la
région des témoins q2 est la boule diamétrale de rayon `D/2`. Pour un
**rectangle**, la région commune à toutes ses paires est un cœur de rayon
`\rho \le (d-2S)/2` avec `S=r_A+r_B`. En posant `u = S/d \le 2/(s+2)` :

$$\lambda(s)=\left(\frac{1+u}{1-2u}\right)^{3}.$$

Le certificat ne ferme donc une paire que si elle possède `K\lambda(s)` témoins,
et non `K`. Les paires dont le compte tombe entre `K` et `K\lambda(s)` sont
**faussement résiduelles**.

| `s` | `2` | `4` | `6` | `8` | `18` | `38` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `\lambda(s)` | `\infty` | `64` | `15,6` | `8,0` | `2,6` | `1,59` |

La dépendance en `K` est **multiplicative**, donc pour une inflation *relative*
donnée, `s` n'en dépend pas. Mais pour une marge **absolue** de `j` témoins,
`\lambda = 1 + j/K` : à `j=2`, `K=8` demande `s \approx 54` et `K=10` demande
`s \approx 65`. **Un seuil plus grand exige une séparation plus grande**, et le
coût est en `s^3`.

## 2. Mais la mesure dit que ce n'est PAS le facteur limitant

J'ai compté les **vrais** témoins universels q2 des paires appartenant aux
rectangles que la banque laisse **ouverts** — balayage exhaustif du nuage,
`uniform`, `n=8 000`, `W=64`, `L=32` :

| `s` | front/pt | q2 fermé (records) | témoins moyens des ouverts | **faussement résiduels** |
| ---: | ---: | ---: | ---: | ---: |
| `2` | `51,1` | `4,5 %` | `89,8` | **`69,5 %`** |
| `4` | `140,5` | `33,3 %` | `89,8` | **`76,0 %`** |
| `8` | `433,6` | `66,7 %` | `102,7` | **`78,8 %`** |

Deux faits que je ne sais pas concilier avec la théorie ci-dessus :

1. **Les rectangles ouverts contiennent en moyenne `90` à `103` témoins**, soit
   **neuf à dix fois le seuil de dix**. Ce ne sont pas des paires marginales :
   le certificat les rate largement.
2. **Le taux de faux résiduels EMPIRE avec `s`** — `69,5 %` puis `76,0 %` puis
   `78,8 %` — alors que `\lambda(s)` prédit l'inverse.

La seule explication que je voie est que le goulet n'est pas la séparation mais
la **banque** : elle ne regarde que `L=32` candidats pris dans une fenêtre
Morton de `W=64` autour du centre. Si moins de dix d'entre eux sont des témoins
universels, le rectangle reste ouvert — quels que soient les quatre-vingt-dix
autres témoins qui existent ailleurs. C'est exactement votre avertissement :
« la discontinuité de Morton peut faire manquer tous les bons témoins ».

Et cela expliquerait la dégradation avec `s` : à `s` grand la banque ferme les
cas faciles, et ce qui reste ouvert est un sous-ensemble où la fenêtre Morton
est justement défavorable.

## 3. Mes trois questions

1. **Confirmez-vous ce diagnostic ?** Si le goulet est le rappel de la banque et
   non la séparation, alors monter `s` est un mauvais investissement — on paie
   `s^3` en records pour un gain qui plafonne, alors que le vrai levier est la
   proposition.

2. **Quelle proposition bornée recommandez-vous à la place d'une fenêtre Morton
   unique ?** Trois pistes me viennent, et je ne sais pas les départager :
   sondage multiple — quelques clés décalées autour de `m_0` plutôt qu'une ;
   descente kNN bornée dans l'arbre déjà construit, qui coûte `O(\log n + L)` et
   n'a aucune discontinuité ; ou proposition par le **cœur commun** entier, dont
   on sait qu'il est inclus dans la région valide.

3. **Faut-il mesurer le rappel de la banque séparément du certificat ?** Je
   confonds aujourd'hui deux échecs sous un seul compteur : « aucun témoin
   proposé n'était valide » et « les témoins proposés étaient valides mais trop
   peu nombreux ». Ce sont deux défauts différents et je ne les distingue pas.

## 4. Non-claims

L'échantillon est de deux mille rectangles ouverts par configuration, sur une
seule famille et une seule taille. Le compte de témoins est exact — balayage
exhaustif — mais la sélection des rectangles est aléatoire uniforme sur les
ouverts, donc non pondérée par la masse. Aucun temps, aucun octet. Le contrat
`50 000` reste entièrement ouvert et G4 reste NO-GO.
