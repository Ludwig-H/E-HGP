# q4 : borner la canopée ne change PAS l'exposant du travail — correction

Document durable, 30 août 2026, pin `e2ac9da2`. Il **corrige** une conclusion que
`Q4_APRES_Q3.md` laissait espérer et qu'un contrôle adversarial a réfutée.

## Ce qui tient

La canopée **est** le porteur dominant de la masse de complétions de q4 sur
`terrain`. Contrôle apparié à flux préservé (écrêtage **après** tirage, `mt19937`
bit à bit identique — mieux apparié que le mien) : à $n=8000$, borner la canopée
divise les complétions par $2{,}13$, contre $1{,}05$ en bornant les six calottes
de relief au même plafond absolu. À $n=32\,000$, les $647$ points de canopée
($2{,}02\,\%$ du nuage) portent $77{,}2\,\%$ des seeds et $90{,}2\,\%$ des
$256$ M de complétions, et cette part **croît** avec $n$ ($33{,}2 \to 78{,}8 \to
90{,}2\,\%$) alors qu'elle reste proportionnelle au nombre de ces points quand la
canopée est bornée ($4{,}1 \to 4{,}8\,\%$).

## Ce qui tombe, et c'est ma conclusion

**Le gain était publié dans la mauvaise unité.** Les complétions ne représentent
que $3{,}3$ à $10{,}5\,\%$ des tests élémentaires de la lane q4. Le poste
dominant est `q4_core_site_tests` — $48\,\%$ du travail compté à $n=2000$,
$63{,}5\,\%$ à $n=16\,000$ au dépôt, $75{,}5\,\%$ en famille bornée — et il est
proportionnel aux **seeds**, pas aux complétions.

**Le coût aval par seed en q4 n'est ni $11$–$13$ ni plat**, contrairement à q3 :

| $n$ | dépôt | canopée bornée |
|---:|---:|---:|
| 2 000 | 13,81 | 12,37 |
| 4 000 | 16,61 | 13,75 |
| 8 000 | 20,69 | 21,00 |
| 16 000 | **30,63** | **52,79** |

Il **croît dans les deux cas**, et il croît *plus vite* en famille bornée : les
seeds y sont quatre fois moins nombreux, mais les survivants sont les plus durs.

**Conséquence : dans l'unité qui paie, borner la canopée ne change pas
l'exposant.** Exposant local du travail élémentaire total, $8\,000 \to 16\,000$,
graine 3 : **$2{,}215$ au dépôt et $2{,}227$ en famille bornée**.

**Et la pente résiduelle était lue sur un ajustement global.** La série à canopée
bornée a cinq tailles ; ses exposants **locaux** de complétions sont $1{,}065$,
$1{,}068$, $1{,}367$, $1{,}376$ — monotones croissants. Le $1{,}219$ publié est
l'ajustement global. Idem pour les seeds : $1{,}117$, $1{,}181$, $1{,}296$,
$1{,}388$ pour un global de $1{,}245$.

## Ce que cela sépare nettement de q3

| | coût aval par seed | effet d'une canopée bornée |
|---|---|---|
| **q3** | **plat**, $11{,}40$ à $12{,}50$ sur la vraie lane | `seeds/ancre` devient constant $\Rightarrow$ **la lane devient linéaire** |
| **q4** | **croissant**, $13{,}8 \to 52{,}8$ | l'exposant du travail **ne bouge pas** ($2{,}215 \to 2{,}227$) |

Sur q3, seeds plats $\Rightarrow$ travail plat, parce que le coût par seed est
constant. Sur q4 cette implication est fausse.

**Le mur de q4 est donc le scan de cœur par seed, et ce n'est PAS un artefact de
famille.** C'est le premier verrou de cette session qui résiste à la fois au
diagnostic de canopée et au contrôle adversarial.

## La question ouverte, bien posée

Pourquoi le coût par seed du scan de cœur croît-il, alors que celui de q3 est
plat ? Deux causes candidates, non départagées :

1. le scan de cœur compte des témoins **certifiés Jung** — condition bien plus
   rare qu'« intérieur » — et ne sort qu'à $h_4=8$ ; un seed qui survit parcourt
   donc tout `scan_sites` ;
2. la taille de `scan_sites` par ancre croît sur les ancres survivantes.

La première rendrait le coût proportionnel au cover des seeds survivants ; la
seconde le rendrait proportionnel à la croissance du cover. Les distinguer est la
mesure suivante.
