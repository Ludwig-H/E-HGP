# Question de Claude — V158 : la canopée explique q3 ; explique-t-elle aussi q4 ? Et quelles idées q3 se transportent ?

- **Ancrage :** pin `3c343954`. Deux résultats acquis à vous soumettre avant
  d'ouvrir q4.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Ce qui est acquis sur q3

1. **La pente super-quadratique de `terrain` vient de la famille, pas de
   l'algorithme.** `canopy_lift(1, coord/8)` rend la hauteur des arbres
   proportionnelle à l'emprise ($28$ unités à $n=2000$, $112$ à $n=32\,000$) —
   seul terme non auto-similaire du générateur. Avec une canopée bornée,
   `seeds/ancre` devient **constant** ($5{,}41 \to 5{,}45 \to 5{,}72$ sur un
   facteur $16$ en $n$), au lieu d'exploser ($5{,}97 \to 12{,}62 \to 53{,}21$).
   Le mécanisme est isolé : un point suspendu au-dessus du sol crée une ancre
   dont le fuseau **traverse de l'air**, donc sans témoin, donc qu'aucun
   certificat par témoins ne peut tuer. Signature mesurée : dans l'octave
   $[16,32)$ à $n=32\,000$, cover utile $41{,}2$ points pour seulement $3{,}88$
   témoins $W_3$ — **moins** que les $6{,}36$ de l'octave inférieure.
2. **Le générateur n'a pas de bug** : points complets, densité invariante
   ($1$-NN $2{,}83$, $8$-NN $8{,}3$, $64$-NN $23{,}4$ aux trois tailles),
   dimension locale $2{,}01/2{,}00/1{,}99$, et zéro double comptage sur
   $20{,}7$ M de triplets.

## L'état de q4, mesuré sur les mêmes reçus

Pentes $2\,000 \to 32\,000$, cible produit :

| cohorte | ancres | seeds | **complétions** | candidats | corde tués | **profondeur** |
|---|---:|---:|---:|---:|---:|---:|
| `terrain` | 1,21 | 1,69 | **1,91** | 1,12 | 2,01 | **2,29** |
| `scanline` | 1,43 | 1,33 | 1,45 | 0,82 | 1,33 | 1,61 |
| `eight_clusters` | 1,45 | 1,17 | 1,17 | 1,16 | 1,24 | 1,11 |
| `uniform` | 1,11 | 1,09 | 1,09 | 1,09 | 1,12 | 1,10 |

À $n=32\,000$ sur `terrain` : $50$ M de seeds, **$256$ M de complétions**,
$25{,}6$ M tués par profondeur, et $0{,}21$ M de candidats. Le rapport
complétions/seed passe de $2{,}84$ à $5{,}13$ ($n^{0{,}21}$). La lane q4 porte
$53{,}6\,\%$ du mur à cette taille.

## Mes questions

- **V158.** Attendez-vous que la canopée explique aussi q4 ? Le fuseau $W_4$ est
  plus étroit ($125{,}26$ degrés contre $120$), donc un point suspendu devrait y
  produire le **même** vide de témoins, en pire. Si oui, la pente $2{,}29$ des
  tués par profondeur serait elle aussi un artefact de famille, et je le
  mesurerai avant toute autre chose. Voyez-vous une raison pour que q4 y échappe
  — la complétion introduisant un quatrième point qui, lui, pourrait retomber
  sur la surface ?
- **V159.** Le rapport **complétions par seed** croît en $n^{0{,}21}$ sur
  `terrain` alors qu'il est plat sur `uniform` ($4{,}74$) et `eight_clusters`
  ($6{,}25$). C'est le facteur propre à q4, absent de q3. Est-il gouverné par la
  même géométrie (un seed suspendu ayant plus de complétions admissibles), ou
  est-ce un poste indépendant ?
- **V160.** Des idées q3 de cette session, lesquelles se transportent selon vous,
  et lesquelles sont structurellement q3 ?
  — la cascade $\mathrm{core}+h_a+h_b$ est **déjà** appliquée aux trois lanes
    (`anchors_killed_hist[2]`) ;
  — l'union $W_q$ / résiduelle que j'ai intégrée vaut pour toute lane, mais je ne
    l'ai câblée que sur q3 : faut-il la porter à q4 ?
  — le lemme du **rayon hors axe** ($t_i^2 \leq (\sum_{j\neq i} d_j^2)/12$) est
    dérivé de $R \leq D/\sqrt{3}$, propre à q3 ; l'analogue q4 serait
    $R \leq D\sqrt{3/8}$ (Jung), donc $t_i^2 \leq (\sum_{j\neq i}d_j^2)/8$.
    Est-ce exact, et le center-cover q4 vaut-il d'être re-mesuré avec, ou la
    réfutation q3 (aucun rectangle ne rembourse son crédit) suffit-elle à le
    fermer aussi ?
  — la **couche par ligne** ne rapporte rien à $s=8$ sur q3 ($\lvert A\rvert\lvert B\rvert = 2{,}10$) ;
    l'est-elle davantage sur q4, où $\lvert A\rvert\lvert B\rvert$ vaut $4{,}2$
    sur `eight_clusters` ?
- **V161.** Y a-t-il, propre à q4, un poste que q3 n'a pas et qu'il faudrait
  attaquer en premier — la **corde** (`chord_kill`, pente $2{,}01$ sur `terrain`)
  ou le **cœur de seed** (pente $1{,}42$) ?
