# Réponse de Claude — V131 : pourquoi la famille center-cover ne peut pas payer, en une seule quantité

- **Ancrage :** suite de `REPONSE_CLAUDE_V124_AMORTISSEMENT_20260829.md` au pin
  `19e6b99c`. Mesures : `terrain`, $n=2000$, graine 3, 150 rectangles tirés par
  hachage, invariants de sûreté à zéro.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Le conflit, mesuré

Un certificat de patch ne paie que s'il couvre **plusieurs** seeds : c'est un seul
scan de témoins qui en remplace plusieurs. Mais il ne **tue** que si le patch est
petit devant le rayon d'une boule. Ces deux exigences tirent en sens contraire, et
la mesure donne le taux de change exact. Sur le même échantillon, $18{,}3$ seeds
aigus par rectangle :

| $K$ | patches / rect | **patches par seed** | côté / rayon | seeds retirés |
|---:|---:|---:|---:|---:|
| 2 | 8 | **0,44** | 1,219 | 0,0 % |
| 4 | 64 | **3,5** | 0,609 | 11,7 % |
| 8 | 512 | **28,0** | 0,305 | 36,6 % |
| 16 | 4 096 | **223,8** | 0,152 | 41,9 % |

Le point d'équilibre « un patch par seed » tombe vers $K\approx 2{,}6$, où le
retrait vaut $\sim 3\,\%$. **Pour retirer une fraction utile, il faut
sur-résoudre d'un facteur $3$ à $220$**, c'est-à-dire payer $3$ à $220$ scans de
patch pour économiser une fraction d'un scan de seed. C'est la forme fermée du
rapport $0{,}0021$ à $0{,}0078$ que je vous ai rendu.

Un scan de patch n'est d'ailleurs pas moins cher qu'un scan de seed : il lit le
même cover, sort au même neuvième témoin, mais teste chaque témoin contre huit
sommets au lieu d'une seule forme. Mesuré : $19$ évaluations par patch à $K=8$,
contre $11$ à $13$ tests par seed dans la vraie lane.

Cela unifie les trois mécanismes de votre nomenclature en une seule courbe :

- $W_3$ ($K=1$) couvre **tous** les seeds de l'ancre : amortissement maximal, et
  c'est pourquoi il paie — il tue $19{,}7 \to 32{,}0\,\%$ des ancres pour un
  scan ;
- les secteurs raffinent la coordonnée angulaire, qui ne réduit pas le rayon du
  patch : ils héritent du coût sans gagner le pouvoir de coupe ($0{,}78\,\%$) ;
- les $K^3$ patches gagnent le pouvoir de coupe en détruisant l'amortissement.

## Une sortie partielle du conflit, mesurée : ne paver que là où $W_3$ a presque réussi

Le conflit ci-dessus suppose qu'on pave **tous** les rectangles. Or `core`
($h_{\mathrm{coeur}}$ du rectangle, déjà calculé, donc gratuit) dit exactement de
combien de témoins $W_3$ a manqué son coup. Le pavage n'a de valeur que là où il
en manque peu. Même échantillon, `terrain` $n=2000$, invariants à zéro :

| configuration | rectangles pavés | évaluations / rect | seeds retirés | **rapport** |
|---|---:|---:|---:|---:|
| $K=4$, tous | 150 | 2 639 | 11,7 % | 0,0098 |
| $K=8$, tous | 150 | 9 318 | 36,6 % | 0,0086 |
| $K=16$, tous | 150 | 40 001 | 41,9 % | 0,0023 |
| $K=8$, `core` $\geq 5$ | 58 | 3 976 | 35,9 % | 0,0199 |
| **$K=8$, `core` $\geq 6$** | **39** | **2 684** | **35,6 %** | **0,0292** |
| $K=8$, `core` $\geq 7$ | 30 | 2 149 | 31,3 % | **0,0320** |

À **coût égal** au $K=4$ uniforme ($2\,684$ contre $2\,639$ évaluations), la porte
retire $35{,}6\,\%$ au lieu de $11{,}7\,\%$. Seuls $26\,\%$ des rectangles méritent
d'être pavés et ils portent $97\,\%$ de la coupe atteignable. La porte est un pur
choix de coût : un rectangle non pavé n'est simplement pas élagué, l'objet ne
change pas.

C'est cohérent avec la lecture ci-dessus, et c'en est même la conséquence : le
certificat de patch ne vaut d'être payé que là où $W_3$ est à trois témoins du
seuil. Le conflit résolution/couverture n'est pas levé, il est **contourné** en
ne payant que sur la fraction où le solde est favorable.

Crédit : cette idée ne vient pas de moi. Elle m'a été rendue par une exploration
parallèle, et je l'ai répliquée ici sur ma propre sonde après avoir corrigé un
bug de ma porte (`patch_mort` vide indexé sur un rectangle non pavé).

## Le budget de la meilleure version concevable — projection, pas mesure

Je le donne explicitement comme une **projection**, à partir de quantités
mesurées, pour ne pas laisser croire qu'une variante non essayée sauverait la
route.

Deux améliorations sont géométriquement fondées et cumulables :

1. **Paver le disque de l'ancre, pas la boîte du rectangle.** Le lieu des centres
   d'une ancre est un disque **plan** de rayon $\lVert ab\rVert/(2\sqrt{3})$. Un
   pavage $K^2$ de ce disque a un côté de $\lVert ab\rVert/(\sqrt{3}K)$, soit un
   rapport au rayon minimal de $1{,}155/K$, contre $2{,}44/K$ mesuré pour la
   boîte axiale du rectangle. Donc $K=4$ par ancre vaut $K\approx 8$ par
   rectangle, avec $16$ cellules au lieu de $512$. Les sommets restent **entiers**
   à l'échelle $2N$ : $\hat q = N(a+b) + 2(iu+jv)$ avec $u,v$ de
   `bisector_basis`. Gain de coût projeté : $\times 15$ à $\times 30$.
2. **Crédit par nœud au lieu de par point.** Le scan sortant déjà au neuvième
   témoin à $\sim 19$ évaluations, un crédit en bloc ne peut rendre qu'un facteur
   $5$ à $10$.

Budget cumulé à partir du **meilleur point mesuré** ($0{,}032$, $K=8$ avec
`core` $\geq 7$) : $0{,}032 \times 30 \times 7 \approx 6{,}7$, ou
$0{,}032 \times 15 \times 5 \approx 2{,}4$ dans l'hypothèse basse. La route
deviendrait donc **favorable d'un facteur $2$ à $7$** — mais ce chiffre empile
deux facteurs qui ne sont **pas mesurés** et que j'ai estimés par majorants
optimistes. Il justifie de mesurer le pavage par ancre et le crédit par nœud ;
il ne justifie pas d'écrire la route dans le chemin produit.

## Ce que je retiens, et ce que je ne retiens pas

Retenu, mesuré et sûr : le pouvoir de coupe existe et il est important
($42\,\%$ des seeds proposés sur `terrain` au plafond, $\sim 60\,\%$ sur
`uniform`), il est exact, et il agit **avant** toute matérialisation de
$(a,b,x)$. Ce n'est pas rien : c'est la première quantité de la journée qui
attaque la bonne cible.

Non retenu **pour l'instant** : que ce pouvoir soit payable. Il ne l'est pas au
niveau du rectangle uniforme ($\lvert A\rvert\lvert B\rvert = 2{,}10$, rapport
$0{,}0055$ à $0{,}0098$), et le conflit résolution/couverture explique pourquoi.
La porte `core` en récupère un facteur $4$ ($0{,}032$), et deux améliorations
géométriques non mesurées pourraient récupérer le reste. L'écart n'est donc plus
de deux ordres de grandeur mais d'**un seul**, et il est adressable ; il reste à
le fermer par la mesure, pas par le budget.

## Question

- **V131.** Cette lecture — un unique arbitrage entre amortissement et pouvoir de
  coupe, dont $W_3$ occupe déjà l'optimum mesuré — vous paraît-elle juste ? Si
  oui, la conséquence est que la marge restante n'est pas dans un certificat
  **plus fin** sur le même lieu de centres, mais dans un certificat **d'une autre
  nature** : quelque chose qui décide sans lire le cover, ou qui décide pour un
  ensemble de seeds défini autrement que par une région de centres.
