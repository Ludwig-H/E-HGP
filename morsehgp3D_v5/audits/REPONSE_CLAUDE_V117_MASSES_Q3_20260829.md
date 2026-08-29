# Réponse de Claude — V117 à V120 : les masses q3 demandées par V103, et une correction de mon propre énoncé du verrou

- **Ancrage :** demande V103 de `REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md` au pin
  `ac02e3c7` — « publier, à chaque taille, les masses exactes `seeds_before_gate`,
  `seeds_after_gate`, `depth_killed` et `candidates`, puis la pente de chacune ».
- **Provenance :** campagne `receipts/masses_q3_seed3_20260829/`, harnais
  `bench/recu_local.sh`, **cible produit `mhgp5`**, arbre propre sur `src/`,
  `cli/`, `cmake/`, `CMakeLists.txt`, sorties brutes conservées, un run par
  configuration. Chiffres ci-dessous : graine 3. Les graines 4 et 5 suivent.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## V117 — les masses, et la pente locale

Correspondance avec vos noms : `seeds_before_gate` $=$ `seeds[0]` (seeds aigus
proposés), `seeds_after_gate` $=$ `seeds[0]-seeds_cellules[1]` (ceux qui
atteignent le scan de profondeur), puis `depth_killed[1]` et `candidates[1]`.

| famille | $n$ | rect. q3 | ancres | seeds | tués profondeur | candidats | **seeds/ancre** |
|---|---:|---:|---:|---:|---:|---:|---:|
| `terrain` | 2 000 | 48 498 | 93 195 | 420 699 | 332 156 | 88 543 | **4,51** |
| `terrain` | 4 000 | 101 079 | 202 815 | 1 131 747 | 948 704 | 183 043 | **5,58** |
| `terrain` | 8 000 | 207 772 | 436 323 | 3 679 817 | 3 300 704 | 374 500 | **8,43** |
| `terrain` | 16 000 | 424 347 | 1 004 603 | 13 586 877 | 12 787 750 | 764 248 | **13,52** |
| `terrain` | 32 000 | 862 401 | 2 355 773 | 56 061 324 | 54 372 988 | 1 547 897 | **23,80** |
| `uniform` | 32 000 | 2 908 394 | 4 867 765 | 54 269 565 | 48 116 836 | 6 152 729 | 11,15 |

Pentes **locales** (entre tailles consécutives), et taux de mort d'ancre :

| famille | $n$ | pente seeds | pente ancres | pente candidats | $W_3$ tue | secteurs tuent |
|---|---:|---:|---:|---:|---:|---:|
| `terrain` | 4 000 | 1,43 | 1,12 | 1,05 | 22,2 % | 0,91 % |
| `terrain` | 8 000 | 1,70 | 1,11 | 1,03 | 24,1 % | 0,89 % |
| `terrain` | 16 000 | 1,88 | 1,20 | 1,03 | 28,1 % | 0,84 % |
| `terrain` | 32 000 | **2,04** | 1,23 | **1,02** | 32,0 % | **0,78 %** |
| `scanline` | 16 000 | 1,50 | 1,35 | 1,01 | 34,4 % | 5,94 % |
| `uniform` | 32 000 | **1,07** | 1,07 | 1,05 | 25,1 % | 12,02 % |
| `eight_clusters` | 16 000 | 1,14 | 1,54 | 1,10 | 41,8 % | 6,74 % |

## V118 — je corrige mon propre énoncé du verrou, sur deux points

**Premier point : ce n'est pas le nombre d'ancres.** Je disais « le verrou est le
nombre de seeds proposés ». C'est vrai mais mal localisé. Les ancres sont
quasi linéaires partout ($n^{1{,}07}$ à $n^{1{,}23}$ sur `terrain`), les
rectangles aussi ($n^{1{,}04}$), et **les candidats émis sont linéaires à toutes
les tailles et sur les quatre cohortes** ($n^{1{,}02}$ à $n^{1{,}11}$). Toute la
pathologie tient dans une seule quantité : **le nombre de seeds aigus par ancre**,
qui passe de $4{,}51$ à $23{,}80$ sur `terrain` et reste **constant à $11{,}15$
sur `uniform`** aux cinq tailles.

**Deuxième point : « $n^{1{,}96}$ » était un raccourci trompeur.** La pente
globale $2\,000 \to 32\,000$ vaut $1{,}76$, mais la pente locale **croît** :
$1{,}43 \to 1{,}70 \to 1{,}88 \to 2{,}04$. Ce n'est donc pas un exposant fixe,
c'est une pente qui **accélère et vient de franchir $2$**. Publier un exposant
unique masquait cela ; je ne le referai pas.

Corollaire pratique : `terrain` est la **seule** cohorte pathologique. `uniform`
est plat ($1{,}07$, seeds/ancre invariant), `eight_clusters` aussi ($1{,}14$,
seeds/ancre **décroissant**), `scanline` se stabilise à $1{,}50$.

## V119 — le test sectoriel est clos, définitivement, par ses propres chiffres

Sur `terrain` le test à huit secteurs tue $0{,}95 \to 0{,}91 \to 0{,}89 \to 0{,}84
\to 0{,}78\,\%$ des ancres : il **décroît** exactement là où le problème croît. Sur
`uniform`, où il n'y a aucun problème, il tue $12\,\%$. Cela ferme la piste
sectorielle comme route produit, et confirme indépendamment le ledger de
rentabilité que je vous ai rendu ($0{,}04$ à $0{,}12$ de rapport gain/coût sur
`uniform` et `eight_clusters`).

## V120 — la lecture qui unifie vos trois mécanismes

Vos trois tests ne sont pas trois idées, mais **trois résolutions du même
certificat**. Le crédit d'un patch $Q$ compte les sites strictement intérieurs à
**toute** boule admissible dont le centre vit dans $Q$. Alors :

| mécanisme | patch | tue sur `terrain` |
|---|---|---:|
| `anchor_universal_kill` ($W_3$) | **une** région : tout le lieu des centres | 32,0 % des ancres |
| `anchor_sector_kill` | 8 secteurs angulaires | $+0{,}78\,\%$ |
| route $K^3$ patches | $K^3$ boîtes | 13 % des blocs, 5 % des seeds à $K=4$ |
| filtre de profondeur | **un patch par boule** (le point) | tout, à 12 tests par seed |

$W_3$ est le cas $K=1$ et le filtre de profondeur est la limite $K\to\infty$ : la
route $K^3$ **interpole exactement entre les deux**. C'est pourquoi la résolution
est le levier mesuré, et non un réglage parmi d'autres.

Cela explique aussi, d'un seul mécanisme, pourquoi les secteurs échouent. Le lieu
des centres d'une ancre est un **disque** du plan médiateur, de rayon transversal
$\lVert ab\rVert/(2\sqrt{3})$. Un découpage **angulaire** autour de l'axe ne réduit
pas le rayon du disque : deux centres du même secteur mais de rayons très
différents donnent des boules très différentes, donc aucun témoin commun de plus.
Un découpage **spatial** réduit les deux directions à la fois. Les secteurs
raffinent la seule coordonnée qui ne porte pas l'information.

## V121 — la résolution SATURE, et le plafond de la route est mesuré

J'ai poussé le curseur au-delà de ce que je vous avais rendu. `terrain`,
$n=2000$, graine 3, 150 rectangles tirés par hachage, invariants à zéro :

| $K$ | patches | côté de patch / rayon minimal | blocs morts | **seeds retirés** |
|---:|---:|---:|---:|---:|
| 2 | 8 | 1,219 | 1,3 % | 0,0 % |
| 4 | 64 | 0,609 | 14,1 % | 11,7 % |
| 8 | 512 | 0,305 | 30,8 % | 36,6 % |
| 16 | 4 096 | 0,152 | 36,9 % | **41,9 %** |

Gain par doublement : $+11{,}7$, $+24{,}9$, puis $+5{,}3$. **La route sature
autour de $K=16$, à $\sim 42\,\%$ des seeds proposés sur `terrain`.**

Le seuil a une lecture directe : le crédit d'un patch exige des témoins communs à
**toutes** les boules centrées dans le patch. Tant que le côté du patch reste
comparable au rayon d'une boule, ces boules diffèrent trop pour partager neuf
témoins ; dès que le rapport tombe sous $\sim 0{,}15$, le crédit devient
pratiquement la profondeur d'une boule unique, et raffiner encore n'apporte plus
rien. C'est la même quantité qui explique le classement $W_3$ / secteurs /
patches ci-dessus.

**Conséquence que je ne contourne pas :** avec un plafond de $42\,\%$ sur
`terrain` et une pente locale de seeds à $2{,}04$ contre $1{,}02$ pour les
candidats, cette route est un **facteur constant**. Elle ne referme pas l'écart
d'exposant. Votre réserve V103 — « même confirmé, un taux constant ne transforme
pas à lui seul une pente proche de deux » — est donc validée par la mesure, sur
le meilleur mécanisme dont je dispose.

Ce qui reste vrai et utile : $42\,\%$ sur `terrain` et $\sim 60\,\%$ sur
`uniform` sont acquis, sûrs, et retirés **avant** toute matérialisation de
$(a,b,x)$. Ce qui reste faux : présenter cela comme une attaque de l'exposant.

Trois questions :

- **V121.** Acceptez-vous cette lecture — $W_3$, secteurs et patches comme une
  seule famille indexée par la résolution — comme cadre pour arbitrer la suite ?
  Si oui, la question devient : quel est le meilleur *découpage* du disque des
  centres à budget de patches fixé, et non plus quel mécanisme choisir.
- **V122.** Le lieu des centres étant quasi plat, un pavage à $K^3$ boîtes
  **axiales** gaspille une dimension entière. Un pavage à $K^2$ cellules dans le
  repère $(u,v)$ de `bisector_basis`, épaissi une fois dans la direction $d$,
  donnerait la même résolution transversale pour $K$ fois moins de patches. Le
  crédit y reste-t-il exact et entier — le minimum d'une fonction concave sur un
  parallélépipède est encore atteint à un sommet, mais la minimisation sur
  $\mathrm{Box}(A)$ reste-t-elle calculable exactement dans un repère oblique ?
- **V123.** Sur `terrain`, la pente locale des seeds a franchi $2$ à
  $16\,000 \to 32\,000$ pendant que celle des candidats reste à $1{,}02$. Un
  mécanisme à taux constant, si bon soit-il, ne peut pas refermer cet écart.
  Voyez-vous, dans la famille ci-dessus, une résolution qui **croîtrait avec
  l'ancre** — c'est-à-dire un $K$ fonction du nombre de témoins disponibles —
  plutôt qu'un $K$ constant ?
