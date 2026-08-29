# Réponse de Claude — V124 à V127 : la prémisse d'amortissement `WSPDRect × Handle` est fausse en mesure — plus d'un rectangle vivant sur deux ne porte qu'une seule ancre

- **Ancrage :** § « Certificat sûr : center-cover conditionné par $C$ » et V103 de
  `REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md`. Mesures au pin `b0827725`, sondes de
  travail dérivées de `bench/q3_patch_block_probe.cpp` (committée), invariants de
  sûreté à zéro partout.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## V124 — le crédit est monotone sous raffinement : c'est un théorème, et il est vérifié

$L_A(Q,z)=\min_{o\in Q} f_A(o;z)$ décroît quand $Q$ grandit, donc pour
$Q'\subseteq Q$ on a $L_A(Q',z)\geq L_A(Q,z)$ : **tout patch mort le reste après
subdivision**, et $g_{AB}$ est monotone non décroissante sous raffinement. Une
descente qui s'arrête dès qu'un nœud meurt rend donc exactement le même verdict
feuille que la grille uniforme de même profondeur.

Vérifié : grille uniforme $K=16$ et descente adaptative de profondeur 4 donnent
**le même** $36{,}9\,\%$ de blocs morts et $41{,}9\,\%$ de seeds retirés.

## V125 — mais la descente adaptative est plus CHÈRE, et je la retire

Même verdict, coût **triplé** : $18{,}9$ M évaluations de témoins contre $6{,}0$ M
pour la grille uniforme, sur 150 rectangles de `terrain` $n=2000$.

La cause est une asymétrie de coût que je n'avais pas vue. Un patch qui **meurt**
est bon marché : le scan sort dès le neuvième témoin crédité. Un patch qui **vit**
est cher : il paie le scan entier. Or un gros nœud a un crédit faible — c'est
exactement ce que dit la mesure $K=2$, qui retire $0{,}0\,\%$ des seeds. Les
niveaux hauts de l'arbre ne tuent donc presque rien ($3{,}3\,\%$ des nœuds
internes) et se paient plein tarif. La descente ajoute des niveaux entiers de
scans complets pour un gain nul.

J'ai proposé cette idée et je la retire dans la même heure, sur ma propre mesure.

## V126 — le vrai chiffre : la route coûte 130 à 480 fois ce qu'elle rapporte, et l'écart s'aggrave avec $n$

Unités comparables, par rectangle : coût $=$ évaluations de témoins réellement
payées ; gain $=$ tests de sites évités, à $12$ par seed retiré (votre valeur basse).

| configuration | évaluations / rect | tests évités / rect | **rapport gain/coût** |
|---|---:|---:|---:|
| `terrain` $n=2000$, $K=8$ | 9 759 | 76 | **0,0078** |
| `terrain` $n=8000$, $K=8$ | 10 229 | 57 | **0,0055** |
| `terrain` $n=2000$, $K=16$ | 42 300 | 90 | **0,0021** |

Et l'unité m'est défavorable une seconde fois : une évaluation de témoin vaut
jusqu'à huit distances aux sommets, un test de site vaut un produit scalaire.

Une version **nœud** ne peut pas combler cela. Le scan sort déjà au neuvième
témoin, à $\sim 19$ évaluations par patch en moyenne à $K=8$ : un crédit par nœud
ramènerait cela à quelques unités, soit un facteur $5$ à $10$ — il en faut $200$.

## V127 — la cause est structurelle : il n'y a rien à amortir

Toute la route repose sur « calculer une fois par rectangle, amortir sur
$\lvert A\rvert\lvert B\rvert$ ancres ». J'ai mesuré cette base d'amortissement.

`terrain`, $n=8000$, graine 3 : 207 772 rectangles vivants q3, 436 323 ancres,
**moyenne $\lvert A\rvert\lvert B\rvert = 2{,}10$**.

| $\lvert A\rvert\lvert B\rvert \geq$ | rectangles cumulés | part des rectangles | ancres cumulées | part des ancres |
|---:|---:|---:|---:|---:|
| 128 | 9 | 0,0 % | 1 489 | 0,3 % |
| 16 | 873 | 0,4 % | 26 121 | 6,0 % |
| 8 | 5 616 | 2,7 % | 72 204 | 16,5 % |
| 4 | 28 410 | 13,7 % | 179 753 | 41,2 % |
| 2 | 93 237 | 44,9 % | 321 788 | 73,7 % |
| 1 | 207 772 | 100 % | 436 323 | 100 % |

`uniform`, $n=8000$ : moyenne $1{,}65$, et $62{,}6\,\%$ des rectangles vivants
sont des **singletons**.

**Plus d'un rectangle vivant sur deux ne porte qu'une seule ancre.** « Une fois
par rectangle » est donc, pour l'essentiel de la masse, « une fois par ancre ».
La WSPD par vagues descend jusqu'à des paires quasi ponctuelles, et il ne reste
aucun facteur $\lvert A\rvert\lvert B\rvert$ à amortir.

C'est une réfutation de la **prémisse**, pas de mon implémentation, et elle porte
au-delà du center-cover : elle vaut pour tout calcul de la forme « une fois par
rectangle WSPD, réutilisé sur ses ancres », donc aussi pour les requêtes saturées
censées remplacer `corner_histograms`. Corollaire : `corner_histograms` en
$O(n_A^2+n_B^2)$ n'est pas un poste de coût, puisque $n_A,n_B\approx 1$ à $2$ —
le remplacer ne peut rien rapporter.

Ce qui reste vrai et acquis : le pouvoir de coupe est réel, sûr, mesuré
($42\,\%$ des seeds sur `terrain` au plafond, $\sim 60\,\%$ sur `uniform`), et
retiré avant toute matérialisation de $(a,b,x)$. Ce qui tombe : l'espoir que ce
pouvoir soit payable au niveau du rectangle.

## Questions

- **V128.** Contestez-vous la mesure d'amortissement ? Elle est reproductible en
  quelques lignes sur `alive_rectangles`, et elle me paraît décisive pour toute
  la nomenclature `WSPDRect × Handle`. Si elle tient, la seule granularité qui
  amortisse quelque chose est le **handle** ($\sim 7$ blocs par rectangle) ou un
  niveau **au-dessus** du rectangle, pas le rectangle.
- **V129.** Y a-t-il un niveau de la descente WSPD, *avant* la terminaison en
  paires quasi ponctuelles, où un center-cover serait à la fois assez serré pour
  couper et assez gros pour amortir ? Autrement dit : le certificat doit-il être
  posé sur les rectangles **non terminaux**, quitte à être plus lâche ?
- **V130.** Avec un plafond de coupe à $42\,\%$ et une pente locale de seeds à
  $2{,}04$ contre $1{,}02$ pour les candidats, aucun mécanisme à taux constant ne
  referme l'écart. La seule quantité dont la mesure montre une croissance
  utilisable est le taux de mort de $W_3$ ($19{,}7 \to 32{,}0\,\%$). Voyez-vous
  une raison de penser qu'il continue de croître, ou faut-il chercher ailleurs ?
