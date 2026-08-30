# Réponse de Claude — V144 : `core` et $h_a/h_b$ sont gouvernés par $s$ en sens opposés, et le réglage par défaut annule les deux crédits d'extrémité

- **Ancrage :** pin `0ad70c23`. Sonde de travail counter-only, vérité terrain par
  énumération du cover, échantillon de rectangles tiré par hachage.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## La décomposition des trois crédits, pondérée par la masse de seeds

Rappel : l'ancre $(a,b)$ meurt dès que $\mathrm{core} + h_a(a) + h_b(b) \geq h_3$,
les trois sources étant disjointes (corridor hors $A\cup B$, témoins dans $A$,
témoins dans $B$). $n = 2000$, graine 3, $s=8$ :

| cohorte | masse tuée | `core` moyen chez les tuées | $h_a$ | $h_b$ |
|---|---:|---:|---:|---:|
| `terrain` | 11,5 % | 7,61 | 0,95 | 0,90 |
| **`eight_clusters`** | **30,3 %** | 6,41 | **2,60** | **2,09** |
| `uniform` | 12,0 % | 7,92 | 0,51 | 0,87 |
| `scanline` | 27,0 % | 7,60 | 1,66 | 2,15 |

L'hypothèse est confirmée et chiffrée : **`core` fournit $6{,}4$ à $7{,}9$ des $9$
requis, $h_a+h_b$ fournissent le solde de $1{,}4$ à $4{,}7$**. Ils sont
complémentaires, pas concurrents — `core` est le crédit de masse, les crédits
d'extrémité sont les finisseurs. Et sur `eight_clusters` les finisseurs sont
$2{,}5$ à $4$ fois plus forts qu'ailleurs, ce qui débloque $30{,}3\,\%$ de la
masse contre $11{,}5\,\%$ sur `terrain`.

(`core` seul ne tue jamais parmi les rectangles vivants : c'est tautologique,
ceux de $\mathrm{core}\geq h_3$ sont tués pendant la descente et n'y figurent pas.)

## $s$ gouverne les deux crédits en sens opposés

| `terrain` | $\lvert A\rvert\lvert B\rvert$ | `core` | $h_a$ | $h_b$ | **masse tuée** |
|---:|---:|---:|---:|---:|---:|
| $s=2$ | 34,60 | 4,21 | **8,46** | **9,26** | **78,5 %** |
| $s=4$ | 4,17 | 7,26 | 1,08 | 1,92 | 23,1 % |
| $s=6$ | 2,56 | 7,48 | 0,83 | 1,34 | 17,9 % |
| $s=8$ | 1,92 | 7,72 | 1,11 | 0,78 | 11,2 % |
| $s=12$ | 1,36 | 7,94 | 0,47 | 0,88 | 11,7 % |

| `eight_clusters` | $\lvert A\rvert\lvert B\rvert$ | `core` | $h_a$ | $h_b$ | **masse tuée** |
|---:|---:|---:|---:|---:|---:|
| $s=2$ | 27,16 | 1,11 | **8,91** | **14,36** | **71,2 %** |
| $s=4$ | 14,35 | 1,15 | 6,35 | 7,35 | 51,7 % |
| $s=8$ | 4,21 | 5,89 | 2,27 | 2,12 | 30,3 % |
| $s=12$ | 2,56 | 6,94 | 1,26 | 1,49 | 22,3 % |

**La cause est structurelle : $h_a \leq \lvert A\rvert - 1$.** À $s=8$ on a
$\lvert A\rvert \approx 1{,}4$, donc les deux crédits d'extrémité sont
**plafonnés près de zéro par construction**. Le réglage par défaut de la
séparation rend inopérants exactement les crédits qui portent le régime en amas.

Le gain des extrémités écrase la perte du cœur : la masse tuée par le test
histogramme passe de $11{,}2\,\%$ à $78{,}5\,\%$ sur `terrain` et de $30{,}3\,\%$
à $71{,}2\,\%$ sur `eight_clusters` en passant de $s=8$ à $s=2$.

## Les morts sont PAR POINT, pas par paire

À tout $s$, la quasi-totalité des morts vient de $h_a(a) \geq \mathrm{need}$
**seul** (ou $h_b(b)$ seul), pas de la somme :

| cohorte, $s$ | points $a$ tués | points $b$ tués | ancres tuables **par point** | tuées **par paire** |
|---|---:|---:|---:|---:|
| `terrain`, $s=2$ | 14,7 % | 17,4 % | 10 685 | 8 504 |
| `terrain`, $s=8$ | 2,5 % | 1,3 % | 47 | 46 |
| `eight_clusters`, $s=2$ | 7,7 % | 8,5 % | 5 686 | 5 217 |

Or `generate.hpp:1003` applique le test **dans la boucle interne**, donc
$\lvert A\rvert\lvert B\rvert$ fois :

```cpp
const u64 need = h_of[1] - ar.core;
for (i32 ua = ra.first; ua <= ra.last; ++ua)
  for (i32 ub = rb.first; ub <= rb.last; ++ub) {
    ++ls->anchors[1];
    if (sc.ha[ua - ra.first] + sc.hb[ub - rb.first] >= need) { ...; continue; }
```

Hisser $h_a(a) \geq \mathrm{need}$ hors de la boucle interne tue $\lvert B\rvert$
ancres d'un seul test, sans changer l'objet — la mesure ci-dessus dit que la
perte serait marginale.

Note au passage : le compteur `anchors_killed_hist` existe mais **n'est imprimé
nulle part**. Ce poste, qui retire $11{,}5$ à $30{,}3\,\%$ de la masse, est
invisible dans les reçus.

## La décomposition que j'en tire

Il ne faut pas changer la forme du rectangle : il faut **ajouter une couche PAR
POINT entre le rectangle et l'ancre**.

| niveau | crédit porté | veut des boîtes |
|---|---|---|
| rectangle | `core` | **petites** |
| **point** | $h_a(a)$ — un test tue $\lvert B\rvert$ ancres | **grosses** |
| ancre | le solde $\mathrm{core}+h_a+h_b$ | — |

C'est cette couche qui rend les grosses boîtes payables, et les grosses boîtes
sont ce qui fait vivre $h_a/h_b$. Sans elle, baisser $s$ multiplie les ancres
matérialisées ($436$ k à $s=8$ contre $10{,}3$ M à $s=2$) et le gain est mangé.

## Ce que je ne prétends pas

Le résidu final est **invariant en $s$** : $3\,679\,817$ seeds sur `terrain`
$n=8000$ pour $s \in \lbrace 2,\dots,32\rbrace$, digest identique. Ces morts
supplémentaires sont donc **redondantes** avec le test $W_3$ par ancre qui suit :
le gain est en **coût**, pas en résidu — tuer au stade histogramme (une addition)
plutôt qu'au stade $W_3$ (un balayage du cover). Et bout en bout, $s=2$ est
aujourd'hui **plus lent** ($31{,}0$ s contre $23{,}2$ s) : c'est précisément ce
que le hissage par point doit corriger, et ce n'est pas mesuré.

## Questions

- **V144.** Le hissage par point change les compteurs (`anchors[1]` cesse de
  compter les ancres jamais matérialisées) mais pas l'objet. Faut-il un compteur
  séparé `anchors_skipped_by_point` pour que le grand-livre reste fermé ?
- **V145.** Le seuil de non-dégénérescence du citron commun vaut $s > 4$ pour q2,
  $s > 6{,}93$ pour q3, $s > 7{,}73$ pour q4, et l'optimum de $h_a/h_b$ est à
  $s$ petit. Les trois lanes n'ont donc pas le même $s$ optimal, et le pipeline
  en impose un seul. Une descente unique avec **trois niveaux de récolte** — une
  hauteur d'arbre par lane — est-elle acceptable, ou brise-t-elle une propriété
  du grand-livre post-séparation ?
