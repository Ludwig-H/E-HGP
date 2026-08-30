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

## Réponse des auditeurs — 30 août 2026

### Précondition : le résultat canopée n'est pas encore « acquis »

Le mécanisme est plausible et mérite le contre-factuel q4, mais `3c343954` ne
suit que le Markdown. Les deux sondes annoncées, leur commande, leurs sorties,
leur règle d'échantillonnage et le reçu des nuages modifiés sont absents. Les
caps cités contredisent en outre la source : `coord=223/894` donne par division
entière `coord/8=27/111`, pas `28/112`. Enfin, le `11,15` de comparaison est
`seeds[0]/anchors[1]` dans les reçus ; `anchors[1]` précède plusieurs portes et
n'est pas le nombre d'ancres survivantes. La pente q3 historique à trois graines
est reçue comme exposant sécant local ; les chiffres du bras borné restent un
diagnostic counter-only.

### V158 — oui comme hypothèse à tester, non comme transfert de verdict

L'ouverture complète q4 est `109,47 deg`, pas `125,26 deg`; elle est donc bien
plus étroite que les `120 deg` de q3. Cela rend plausible qu'une ancre
sol--point suspendu ait encore peu de témoins communs. Ce n'est pas un
« a fortiori » sur la lane : le seuil q4, le carrier, la corde et les
complétions diffèrent, et un faible compte W4 universel n'exclut pas un
certificat dépendant de la face ou du carrier.

Jouer d'abord le même bras canopée apparié en q4 est la bonne décision. Garder
`terrain` inchangé comme adversaire et ajouter une contre-famille distincte ;
figer les tirages latents avant de remapper le lift. À chaque taille et graine,
publier séparément W4, ancres atteignant les seeds, seeds, complétions, morts
cœur/corde/profondeur, candidats, mur et HWM. La pente `2,29` des morts de
profondeur est un compte seed 3, pas encore un artefact causal.

### V159 — poste q4 distinct, causalité non identifiable dans l'agrégat

Le passage `2,84 -> 5,13` et l'exposant sécant `0,21` se recalculent pour ce run.
Ils peuvent venir du lift, mais aussi de la taille du cover, de la composition
des seeds survivants ou des sorties anticipées. Le quotient agrégé ne sépare
pas ces mécanismes. Le probe q4 doit stratifier `complétions/seed` par octave de
`|ab|`, type d'ancre (sol--sol, sol--canopée, canopée--canopée), taille du cover
et fate amont, puis comparer les caps appariés. Avant cela, traiter ce poste
comme indépendant dans le grand-livre.

### V160 — ce qui se transporte

- La cascade `h_coeur+h_a+h_b` se transporte à la porte d'ancre W4, avec seuil
  q4 et provenance propre. Elle ne se somme pas ensuite à des témoins de seed,
  corde ou complétion sans IDs dédupliqués ou strates prouvées.
- L'union du W4 résiduel avec les IDs déjà certifiés est le raccord prometteur.
  La porter d'abord en shadow sous forme de `WitnessTape` canonique ; toute
  précharge de corde repasse par le vrai `ChordPieces::update`, jamais par quatre
  crédits scalaires aveugles.
- La borne continue q4 est bien le facteur `/8` : Jung donne le rayon hors axe,
  puis l'orthogonalité à `ab` donne la borne coordonnée par coordonnée. Elle
  exige encore son arrondi dirigé, ses égalités et un mutant q4. Le résultat
  négatif q3 ne ferme pas le center-cover q4, dont le disque et l'économie sont
  différents ; il interdit seulement de le promouvoir sans shadow apparié.
- Le passage de `2,10` à `4,2` partenaires ne suffit pas à rouvrir la couche par
  lignes comme priorité. Les clés exactes déjà relues se répètent peu. Mesurer
  d'abord requêtes uniques, sites évités, mur et HWM sur les seuls survivants
  q4 ; sans non-vacuité nette, ne pas construire de cache.

### V161 — ne pas choisir entre cœur et corde avec des compteurs de morts

Les pentes `chord_kill` et `core_kill` comptent des décisions, pas du temps. Le
seul probe disponible agrège encore cœur+corde (`2258/2410 ms` sur
terrain/scanline à `n=8000`) et les place devant les complétions (`1057/939 ms`),
mais il n'est pas reçu et ne départage pas les deux sous-étages.

Le prochain incrément utile est donc partagé : instrumenter séparément sites et
temps du cœur, mises à jour de corde, complétions et profondeur, puis raccorder
le `WitnessTape` W4 qui peut éviter du travail dans les deux étages et en aval.
Une fois ce split reçu, optimiser le sous-étage dominant. Cette séquence aide q4
sans inventer aujourd'hui une priorité à partir de deux compteurs non causaux.

Verdict borné : mesurer d'abord la canopée q4 et le split cœur/corde ; conserver
`s=8`, les trois graines et les digests appariés. Aucun claim de complexité,
d'exactitude Gamma ou de résultat GPU n'en découle.

### Addendum après `d280fb2c`

Le nouveau tableau q4 rend la piste des ancres longues plus concrète, mais il
reste une sonde non reçue : `octq4.cpp`, commande, stdout, hashes et reçu ne sont
pas versionnés. Les 2 500 rectangles q4 et les 8 000 rectangles q3 ne sont pas
appariés par `AnchorKey`; on ne peut donc pas affirmer que les 47 ancres q4 sont
précisément celles que q3 tue.

Les corrections numériques sont les suivantes : l'angle reste `109,47 deg` ;
l'exposant q3 des trois valeurs publiées vaut `0,020`, pas `0,013`; les 47
survivantes valent `0,71 %` des 6 663 ancres échantillonnées, `1,34 %` des 3 509
survivantes ou `2,72 %` des 1 726 ancres longues, jamais `5,5 %`. Les 50 % de
masse sont cohérents **dans cet échantillon seed 3** et justifient un banc ciblé,
pas encore l'énoncé « le mur tient dans 47 ancres ».

Le contrat géométrique exact est plus étroit que le récit : `W4` est inclus
dans `W3`, donc `N4<=N3` pour une même ancre et seulement `N3=0` implique
`N4=0`. Avec `h3=9` et `h4=8`, les décisions ne s'emboîtent pas. La fixture déjà
permanente `q4_source_fixture` réalise précisément `(N3,N4)=(9,0)` et tue le
mutant `q4-seeds-from-q3-live`; elle doit rester l'autorité de ce résidu.

Enfin, le quotient de complétion publié emploie tous les `seeds[1]`, y compris
ceux tués par cellule, cœur ou corde avant la boucle D. Le dénominateur exact est

```text
faces_D = seeds[1] - seeds_killed_cells[2]
        - seeds_killed_core - seeds_killed_chord
```

Sur les reçus seed 3, `terrain` passe ainsi de `1 293 473 / 102 835 = 12,58`
essais par face D à 2 000 points à
`256 463 974 / 2 692 030 = 95,27` à 32 000 points. Le facteur est `7,57`,
exposant sécant local `0,73`. La conclusion « ne pas chercher côté complétion »
est donc retirée. Recevoir en parallèle le certificat ciblé des ancres longues
et la décomposition `cellule -> cœur -> corde -> faces_D -> essais D` est la
voie qui départage réellement les deux leviers.
