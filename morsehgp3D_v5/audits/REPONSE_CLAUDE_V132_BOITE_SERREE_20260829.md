# Réponse de Claude — V132 à V135 : la boîte des centres était lâche d'un facteur 3, le crédit par nœud est réfuté, et la route gagne un facteur 17

- **Ancrage :** suite de `REPONSE_CLAUDE_V131_CONFLIT_RESOLUTION_20260829.md` au
  pin `f4e345a4`. Deux des résultats ci-dessous viennent d'une **exploration
  parallèle** que j'ai lancée et **répliquée moi-même** dans la sonde committée ;
  je le dis explicitement, ce ne sont pas mes idées.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## V132 — lemme du rayon hors axe : la boîte des centres était lâche d'un facteur 3

**Lemme.** Soit $(a,b,x)$ un triangle strictement aigu d'arête maximale $ab$, de
circumcentre $c$. Avec $d=b-a$, $D=\lVert d\rVert$, $m=(a+b)/2$, $t=c-m$ : l'angle
opposé à $ab$ est le plus grand donc $\geq 60$ degrés, la loi des sinus donne
$R\leq D/\sqrt{3}$, et comme $t\perp d$ on a
$\lVert t\rVert^2=R^2-D^2/4\leq D^2/12$. De $t\cdot d=0$ et Cauchy-Schwarz :
$t_i^2 d_i^2 \leq (\lVert t\rVert^2-t_i^2)(D^2-d_i^2)$, donc

$$t_i^2 \leq \frac{D^2-d_i^2}{12} = \frac{\sum_{j\neq i} d_j^2}{12}.$$

**La dilatation de l'axe $i$ n'utilise que l'étendue HORS AXE.** L'ancienne boîte
dilatait isotropiquement de $R_{\max}=\sqrt{\mathrm{maxdist}^2/3}$ et ignorait
cette annulation. La boîte serrée est **prouvée incluse** dans l'ancienne — la
différence sur un axe vaut
$(\sqrt{M}/(2\sqrt{3}))\left(2\sin(\theta+30^\circ)-2\right)\leq 0$ — c'est donc
un raffinement, jamais un concurrent, et aucune intersection n'est nécessaire.

Implémenté à l'**échelle 2** ($\mathrm{lo}_2 = A_{\mathrm{lo}}+B_{\mathrm{lo}}-\rho_2$),
patches à l'échelle $2K$, tous les prédicats déjà paramétrés par l'échelle.
Environ dix lignes, aucun coût supplémentaire.

Effet, `terrain` $n=2000$, graine 3, 300 rectangles (réplication exacte, chiffre
pour chiffre, de la mesure indépendante) :

| $K$ | boîte ancienne | **boîte serrée** |
|---:|---:|---:|
| 2 | 0,02 % | **12,14 %** |
| 4 | 13,92 % | **34,16 %** |
| 8 | 32,40 % | **38,08 %** |

**La boîte serrée à $K=4$ bat l'ancienne à $K=8$, pour trois fois moins
d'évaluations.** Rapport de volumes mesuré : $2{,}8$ à $3{,}3$ selon la cohorte,
$23$ à $44$ sur les contre-familles dégénérées.

Sûreté : $0$ centre exact hors de la boîte sur $5$ cohortes $\times$ $3$ graines
(plus un balayage indépendant de $747\,854$ centres sur $7$ cohortes), et le
mutant `rho-moins-un` (un cran de moins sur le rayon hors axe) sort en **code 3**
— porte `mhgp5_q3_patch_block_rho_mutant`.

## V133 — la porte `core` : ne paver que là où $W_3$ a presque réussi

`core` dit gratuitement de combien de témoins $W_3$ a manqué son coup. Combiné à
la boîte serrée, `terrain` $n=2000$, $K=4$ :

| porte | rectangles pavés | évaluations / rect | seeds retirés | **rapport** |
|---|---:|---:|---:|---:|
| aucune | 315 | 2 928 | 34,16 % | 0,0248 |
| `core` $\geq 5$ | 110 (35 %) | 1 147 | **34,16 %** | **0,0634** |
| `core` $\geq 6$ | 75 (24 %) | 745 | 33,01 % | **0,0942** |
| `core` $\geq 7$ | 48 (15 %) | 503 | 25,90 % | **0,1096** |

**`core` $\geq 5$ est gratuit** : coupe identique pour $2{,}55$ fois moins cher.
C'est un pur choix de coût — un rectangle non pavé n'est pas élagué, l'objet ne
change pas.

## V134 — le crédit par NŒUD est réfuté comme réduction de coût, et votre `U_W` a une correction

Je vous avais annoncé un facteur $5$ à $10$ à attendre de la version nœud. **C'est
faux, et dans le mauvais sens.**

- La borne $L_S$ de votre note est **exacte** : elle égale le minimum énuméré de
  $\Phi_S$ sur $\mathrm{Vert}(Q)\times(\mathrm{Box}(A)\cap\mathbb{Z}^3)\times(\mathrm{Box}(W)\cap\mathbb{Z}^3)$
  — vérifié par énumération exhaustive, mutant `box-support-corners-only` tué.
- La version nœud **ne peut pas perdre de pouvoir de coupe** (le crédit nœud est
  un sur-ensemble du crédit point, car le point minimise sur la boîte réelle et
  non sur le réseau). Mesuré : rapport de coupe $1{,}0000$ partout, $0$ désaccord.
- **Mais elle coûte $2{,}9$ à $5{,}6$ fois plus cher** en temps réel ; la meilleure
  variante hybride reste à $1{,}16$–$1{,}43$.
- Cause mesurée : seulement $2{,}2$ à $3{,}3\,\%$ des sites du cover sont crédités
  pour un patch donné, et $0{,}12$ à $0{,}40\,\%$ des handles le sont
  **entièrement**. Le créditeur en bloc moyen ne porte que $2{,}8$ sites. **`ALL`
  ne se déclenche quasiment jamais.**

**Correction à votre note.** Vous posez
$U_W=\min(U_S(Q,A,W),U_S(Q,B,W))$ et `NONE` ssi $U_W\leq 0$. Le prédicat de crédit
d'un site étant $L_A(z)>0$ **ou** $L_B(z)>0$, son majorant sur $W$ est
$\max(U_A,U_B)$, pas le $\min$. Votre $\min$ élague donc plus souvent que la
preuve ne l'autorise. Il reste **sûr** (il ne peut que sous-compter $g_{AB}$, donc
laisser vivre des patches, jamais tuer un seed peu profond), mais il est
strictement moins puissant en théorie ; sur l'échantillon mesuré les deux critères
donnent le même nombre de `NONE`.

**Le levier n'est pas `ALL`, c'est `NONE`.** Mesure du plafond : un prédicat de
saut parfait au niveau du handle ne laisserait que $11{,}7$ à $16{,}2\,\%$ des
sites à scanner (gain $6$ à $8$ fois) ; la borne AABB actuelle en laisse $57{,}6$
à $74{,}1\,\%$.

## V135 — où en est la route, honnêtement

Rapport gain/coût sur `terrain`, du matin à maintenant, tout mesuré :

| étape | rapport |
|---|---:|
| $K=8$ uniforme, boîte lâche (ce matin) | 0,0055 |
| $+$ porte `core` | 0,0292 |
| $+$ boîte serrée, $K=4$, `core` $\geq 6$ | **0,0942** |

**Un facteur $17$**, sans rien perdre du pouvoir de coupe et avec les invariants à
zéro. L'écart au seuil de rentabilité passe de deux ordres de grandeur à environ
$10$. Une piste nommée reste non évaluée — le pavage **tourné** sur $u$, estimé à
$8$–$11$ fois en volume — et le levier `NONE` en vaut $6$ à $8$ sur l'autre face.

Je ne conclus pas que la route est viable : $0{,}094$ reste une perte, et les deux
leviers restants sont des plafonds, pas des mesures. Mais elle n'est plus
disqualifiée, et c'est un changement d'état par rapport à ce que je vous écrivais
il y a deux heures.

## Questions

- **V136.** Acceptez-vous la correction $U_W=\max(U_A,U_B)$ ?
- **V137.** Le pavage tourné sur $u$ bute sur la non-intégralité des sommets d'une
  cellule tournée. À l'échelle $2N$ un sommet vaut $\hat q = N(a+b)+2(iu+jv)$ avec
  $u,v$ de `bisector_basis`, donc entier — mais $u,v$ dépendent de l'**ancre**, pas
  du rectangle. Faut-il alors abandonner le niveau rectangle, que ma mesure
  d'amortissement ($\lvert A\rvert\lvert B\rvert=2{,}10$) montre de toute façon
  presque vide, et poser le certificat directement sur l'ancre ?
- **V138.** Le plafond `NONE` ($6$ à $8$ fois) demande une borne supérieure plus
  serrée qu'une AABB — boule englobante du contenu, ou extension directionnelle le
  long de l'axe centre-ancre. En voyez-vous une qui reste exacte en entiers ?
