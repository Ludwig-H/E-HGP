# Réponse de Claude — V140 : je mesurais le mauvais verdict ; le bon rapproche la route d'un facteur 3 du seuil

- **Ancrage :** correction de `REPONSE_CLAUDE_V139_ROUTE_REFUTEE_20260829.md` au
  pin `1c5ba207`, qui concluait « la route est réfutée à toute résolution et sous
  toute règle de porte ». Cette conclusion portait sur le **verdict bloc**. Le
  verdict qui compte est **par seed**, et il change tout.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Le bon verdict

Je tuais un **bloc** $(A,B,C)$ quand **tous** les bits de son masque étaient des
patches morts. C'est presque impossible : le masque déclare $33{,}4$ bits
faisables là où les centres des vrais seeds n'occupent que $2{,}14$ patches
(facteur $15{,}6$), et il suffit d'un patch faux positif vivant pour sauver le
bloc entier.

Le verdict par seed n'a pas besoin de cela. **Si le patch qui contient le centre
d'un seed est mort, alors toute boule centrée dans ce patch porte $h_3$ témoins
strictement intérieurs — donc celle-là aussi.** Le seed est profond, on saute son
scan sans le faire. C'est sûr, et mon oracle le vérifiait déjà sans que j'en tire
la conséquence : `violations_credit = 0` signifie exactement que ce verdict n'a
jamais menti.

Son application dans la lane est immédiate : `q3_form` est de toute façon calculé
avant le scan, localiser le centre coûte quelques comparaisons entières, et le
crédit des patches est calculé une fois par rectangle.

Pouvoir de coupe, boîte serrée, `terrain` :

| configuration | verdict **bloc** | verdict **par seed** |
|---|---:|---:|
| $n=2000$, $K=2$ | 12,1 % | **51,2 %** |
| $n=2000$, $K=4$ | 34,2 % | **78,0 %** |
| $n=8000$, $K=4$ | 18,1 % | **81,8 %** |
| `uniform` $n=2000$, $K=4$ | 73,0 % | **87,2 %** |

## Le bilan, avec tous les coûts facturés

Unité unique, le test de site. Sont facturés : chaque **sommet** évalué par le
crédit ($1{,}56$ par témoin testé), le **tri radial** des témoins à $2m$ — ce
qu'un tri par comptage en 32 classes radiales coûte réellement, comme
`anchor_cover_from_handles` le fait déjà — et la **localisation du centre** de
chaque seed jugé, à $6K$ comparaisons. Gain : $13$ tests par seed non scanné.

| configuration | coût (sommets) | seeds tués | **rapport** |
|---|---:|---:|---:|
| $K=2$, tous rectangles | 338 694 | 51,2 % | 0,110 |
| $K=2$, `core` $\geq 5$ | 177 346 | 75,6 % | 0,176 |
| $K=2$, `core` $\geq 6$ | 119 410 | 79,7 % | 0,202 |
| $K=2$, `core` $\geq 7$ | 78 799 | 84,8 % | 0,246 |
| **$K=2$, `core` $\geq 8$** | **42 822** | **89,2 %** | **0,295** |

Deux effets nouveaux, tous deux mesurés à coupe **identique** :

- **$K=2$ bat $K=4$.** Huit patches suffisent au verdict par seed, et c'est le
  point de fonctionnement le moins cher — cohérent avec le fait que le pavage ne
  s'amortit pas ($\lvert A\rvert\lvert B\rvert = 2{,}10$).
- **Le tri radial des témoins** fait tomber le scan de $85$ à $66$ témoins par
  patch. Le crédit scannait en ordre de handle ; le filtre de profondeur doit sa
  brièveté ($13$ tests) au tri radial de son cover. Même remède, même effet.

## Où en est la route, dans la bonne unité

| étape | rapport gain/coût sur `terrain` |
|---|---:|
| $K=8$ uniforme, boîte lâche, verdict bloc (ce matin) | 0,0055 |
| boîte serrée $+$ porte `core` | 0,0942 (unité fausse) $\to$ 0,054 corrigé |
| **$+$ verdict par seed, $K=2$, tri radial, `core` $\geq 8$** | **0,295** |

**Un facteur $54$ sur la journée, et il reste un facteur $3{,}4$.** Ce n'est
toujours pas rentable et je ne propose pas de l'écrire dans le chemin produit.
Mais « réfutée à toute résolution et sous toute règle de porte », que j'écrivais
il y a une heure, était faux : je n'avais mesuré qu'un verdict, et le mauvais.

Reste, non mesuré par moi, le **pavage oblique** aligné sur le repère de l'ancre,
qu'une exploration parallèle mesure à $1{,}66$ fois moins de seeds survivants pour
$0{,}97$ fois le coût, précisément à $K=2$. S'il tient, le rapport passerait à
$\sim 0{,}5$ et il resterait un facteur $2$.

## Ce que je retire

« La route center-cover est réfutée comme chemin produit, à toute résolution et
sous toute règle de porte » : faux, énoncé sur le verdict bloc seulement.

Ce qui reste vrai de cette note : le verdict **bloc** est bien réfuté, la borne
inférieure « un patch coûte des dizaines de scans de seed » vaut toujours, et
c'est justement pourquoi $K=2$ — huit patches — est le bon point.

## Question

- **V140.** Le verdict par seed suppose de localiser le centre exact de chaque
  seed dans le pavage, donc de calculer `q3_form` avant de décider. La lane le
  fait déjà. Voyez-vous une raison, côté exactitude ou côté lots GPU, de ne pas
  poser le fate à cet endroit précis — entre `q3_form` et
  `ensure_anchor_scan_affine` ?
