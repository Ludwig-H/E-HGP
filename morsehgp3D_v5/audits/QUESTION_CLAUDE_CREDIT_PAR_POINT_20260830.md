# Question de Claude — V151 : le crédit d'extrémité est recalculé 65 à 189 fois par point ; peut-il devenir directionnel et précalculé ?

- **Ancrage :** pin `dc9cc309`. Suite de V147 (100 % des morts d'ancre exigent
  $h_a+h_b$) et de V150 (le crédit étendu ne gagne qu'en placement).
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Le fait

$n=2000$, $s=8$, graine 3, sur les rectangles vivants q3 :

| cohorte | rectangles vivants | **rectangles par point** | ancres par point | $\lvert B\rvert$ par rectangle |
|---|---:|---:|---:|---:|
| `terrain` | 48 498 | **65,5** (max 154) | 93,2 | 1,92 |
| `eight_clusters` | 113 552 | **188,6** (max 342) | 479,9 | 4,23 |
| `uniform` | 144 791 | **180,6** (max 363) | 232,0 | 1,60 |

`corner_histograms` calcule $h_a(a)$ **une fois par (rectangle, point)**. Chaque
point traversant $65$ à $189$ rectangles vivants, le même crédit d'extrémité est
donc recalculé autant de fois. La base d'amortissement d'un crédit **par point**
serait $34$ à $113$ fois celle d'un crédit par rectangle — et c'est exactement le
facteur qui manque à toutes les routes mesurées depuis deux jours, où
$\lvert A\rvert\lvert B\rvert \approx 2$ ne laissait rien à amortir.

## Pourquoi c'est plausible géométriquement

Le citron a une pointe de $60$ degrés en $a$, orientée vers $b$. Un témoin $z$
proche de $a$ y appartient si et seulement si $z-a$ pointe vers $b$ à moins de
$60$ degrés, dans une portée radiale bornée par $\lVert ab\rVert$. Donc
$h_a^{\mathrm{ext}}(a)$ ne dépend de $\mathrm{Box}(B)$ que par **une direction**
et **une portée** — pas par la boîte entière. Un histogramme directionnel par
point le capturerait.

La sûreté est obtenue en stockant un cône **rétréci** : si le cône stocké est
contenu dans le vrai cône pour toute direction du seau, le compte stocké minore
le vrai compte, et un minorant est exactement ce qu'un crédit doit être.

## Ce que la mesure dit du seau

Les directions vers la boîte opposée sont **dispersées** : seules $19$ à
$25\,\%$ des paires de directions d'un même point tiennent dans $30$ degrés, et
$50$ à $53\,\%$ dans $60$ degrés. Un scalaire par point ne suffirait donc pas ;
il faut un histogramme à $16$–$32$ cônes, ce qui coûte $60$ degrés moins la
demi-largeur du seau en rétrécissement — soit encore $45$ à $50$ degrés utiles à
$32$ cônes.

## Mes questions

- **V151.** Un précalcul **par point** est-il admissible dans l'architecture ? Le
  volume est $O(n\cdot k)$ avec $k$ le nombre de cônes — donc pas un catalogue en
  $\binom{n}{k}$, mais à $30$ M de points et $32$ cônes en demi-octet cela fait
  $\sim 480$ Mo, à confronter au plafond mémoire. Une alternative paresseuse —
  calculer au premier usage et mémoïser, le point servant $65$ à $189$ fois —
  est-elle préférable, et compatible avec le déterminisme par ouvrier ?
- **V152.** La portée radiale de la pointe dépend de $\lVert ab\rVert$, qui varie
  d'un rectangle à l'autre. Faut-il stocker par (cône, coquille radiale) et
  sommer les coquilles sous la borne conservatrice, ou fixer une portée unique
  par la plus petite $\lVert ab\rVert$ du point — ce qui perdrait les rectangles
  lointains ?
- **V153.** Ce crédit par point serait, d'après V147, la seule pièce dont
  $100\,\%$ des morts d'ancre dépendent, et d'après V150 il ne gagne rien en
  pouvoir sur le test $W_3$ par ancre — seulement en placement. Est-ce que cela
  vous suffit pour l'instruire, ou exigez-vous d'abord une mesure du coût amorti
  avec sortie anticipée à $h_3$ et cover trié radialement, que ma sonde actuelle
  ne fait pas ?
