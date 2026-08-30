# Réponse de Claude — V150 : un crédit d'extrémité étendu, sûr sans rayon ni cône, qui triple la masse éliminée — mais qui ne gagne rien en pouvoir, seulement en placement

- **Ancrage :** pin `6699be42`. Réponse à mes propres questions V147–V148, que je
  peux trancher.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## La formulation qui évite les deux défauts

Les deux implémentations réfutées butaient sur la **disjonction** : élargir le
domaine de $h_a$ à une boule fait compter deux fois avec le cœur, et deux boules
$B(a,d_{\min}/2)$ se recouvrent à l'égalité. Les deux problèmes disparaissent si
l'on n'essaie **pas** de rendre les domaines disjoints par la géométrie. Poser

$$h_a^{\mathrm{ext}}(a)=\#\left\lbrace z\notin A\cup B \ :\ z\in W_3(a,b')\ \ \forall\, b'\in\mathrm{Box}(B)\right\rbrace,$$

c'est-à-dire **exactement le prédicat de `corner_histograms`**
(`universal_over_corners`), appliqué au domaine **hors** des deux boîtes au lieu
de dedans. Alors :

- $h_a^{\mathrm{ext}}$ et $h_b^{\mathrm{ext}}$ partagent leur domaine : on ne peut
  pas les additionner ;
- mais `core` exige l'universalité sur **toutes** les paires de $A\times B$, donc
  son ensemble est **inclus** dans celui de $h_a^{\mathrm{ext}}(a)$ pour tout $a$ ;
- et $h_a$, $h_b$ gardent leur domaine **dans** les boîtes, disjoint des deux.

D'où la borne sûre, sans aucun rayon à régler :

$$\mathrm{depth}(a,b,x)\ \geq\ \max\left(h_{\mathrm{coeur}},\,h_a^{\mathrm{ext}}(a),\,h_b^{\mathrm{ext}}(b)\right)+h_a(a)+h_b(b),$$

qui **domine $h_{\mathrm{coeur}}+h_a+h_b$ par construction**.

## Mesure — sûre, et forte

$n=2000$, $s=8$, graine 3, 600 rectangles tirés par hachage. L'invariant est
vérifié par la **vraie forme `q3_power`** sur chaque seed de chaque ancre
nouvellement tuée :

| cohorte | base moyenne `core` → étendue | morts d'ancre | **masse de seeds éliminée** | violations |
|---|---|---:|---:|---:|
| `eight_clusters` | 3,64 → **6,42** | 14,4 → 54,3 % | **28,3 → 82,8 %** | **0** |
| `uniform` | 4,01 → 5,03 | 6,2 → 27,6 % | **17,9 → 56,4 %** | **0** |
| `scanline_single_pass` | 3,46 → 4,63 | 6,9 → 27,4 % | **11,1 → 50,9 %** | **0** |
| `terrain` | 3,82 → 4,77 | 5,8 → 21,6 % | **10,9 → 41,9 %** | **0** |

Un facteur $2{,}9$ à $4{,}6$ sur la masse éliminée **à la porte histogramme**,
à $s=8$, dans le domaine admis.

## Ce que je dois dire contre ma propre proposition

**Le test $W_3$ par ancre domine cette cascade.** Preuve, en quatre pas :

1. $h_a^{\mathrm{ext}}(a)\leq\left\lvert W_3(a,b)\cap(\mathrm{cover}\setminus(A\cup B))\right\rvert$ — l'universalité sur $\mathrm{Box}(B)$ implique l'appartenance pour ce $b$ ;
2. $h_a(a)\leq\left\lvert W_3\cap(A\setminus\lbrace a\rbrace)\right\rvert$ et symétriquement ;
3. $h_a^{\mathrm{ext}}$ et $h_b^{\mathrm{ext}}$ vivant tous deux hors $A\cup B$, leur $\max$ y est majoré par $\left\lvert W_3\cap(\mathrm{cover}\setminus(A\cup B))\right\rvert$ ;
4. tout témoin de $W_3$ est dans le cover de coefficient 3, puisque $W_3\subset W_2$ donne $\lVert 2z-(a+b)\rVert^2<D^2\leq 3D^2$.

Donc la somme est majorée par
$\left\lvert W_3\cap(\mathrm{cover}\setminus\lbrace a,b\rbrace)\right\rvert$,
c'est-à-dire exactement le compte de `anchor_universal_kill`. **Le crédit étendu
n'apporte aucune mort nouvelle.**

Sa valeur est donc de **placement**, pas de pouvoir : $h_a^{\mathrm{ext}}(a)$ ne
dépend que de $a$ et de $\mathrm{Box}(B)$, donc se calcule **une fois par point**
et sert les $\lvert B\rvert$ ancres de ce $a$ — alors que le test $W_3$ est
**par ancre** et exige d'avoir construit `anchor_cover_from_handles`. Sur
`eight_clusters` $n=8000$, `ancres_w3` vaut $1\,148\,935$ : autant de covers
d'ancre construits puis balayés. En déplacer $40\,\%$ au niveau du point est un
gain réel — mais son amortissement est $\lvert B\rvert\approx 1{,}4$ à $s=8$,
et le prédicat aux coins coûte huit évaluations par témoin contre une pour
`in_spindle`.

Je ne le propose donc **pas** comme incrément produit sur ce seul argument : il
faut d'abord mesurer le coût amorti réel, avec sortie anticipée à $h_3$ et cover
trié radialement, ce que ma sonde ne fait pas (elle balaie sans sortie et compte
$911$ tests par ancre, chiffre inutilisable comme coût).

## Réponses à mes propres questions

- **V147** — oui, on peut élargir sans toucher au cœur : il suffit de ne pas
  chercher la disjonction, et de composer par $\max$ avec le cœur au lieu de
  l'addition. Le domaine élargi **contient** celui du cœur, ce qui rend le $\max$
  exact et non conservateur.
- **V148** — la question du rayon disparaît : il n'y a plus de boule. Pour
  mémoire, si l'on tenait à la boule, $\rho < \lVert a-b\rVert/2$ **strict** et
  rapporté à $\lVert a-b\rVert$ (par ancre) et non à $d_{\min}$ (par rectangle)
  suffirait à écarter le contre-exemple du milieu.

## Question

- **V150.** Le crédit étendu ne gagne rien en pouvoir sur le test $W_3$ par
  ancre, mais il se calcule au niveau du **point**. La vraie question devient
  donc : à $s\geq 8$ où $\lvert B\rvert\approx 1{,}4$, existe-t-il un niveau
  intermédiaire — un ancêtre de $B$ dans l'arbre, servant plusieurs rectangles —
  auquel $h_a^{\mathrm{ext}}$ s'amortirait vraiment ? L'universalité sur une
  $\mathrm{Box}(B)$ plus grosse est plus exigeante, donc le crédit y est plus
  faible : y a-t-il un optimum, et se calcule-t-il une fois pour toutes pendant
  la descente plutôt que par rectangle ?
