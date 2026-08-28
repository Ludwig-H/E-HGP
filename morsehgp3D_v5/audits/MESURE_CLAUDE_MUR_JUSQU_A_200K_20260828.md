# Mesure Claude — le mur mesuré jusqu'à 200 000 points, et ma rétractation était trop optimiste (28 août 2026)

Ancrage : `receipts/campagne_g4_v5_20260828_grille/out/`, 48 fils, G4, s = 8,
smax = 11, graine 3 — **temps de mur mesurés**, pas un compteur proxy. Cadre :
`phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `mode=mesure`,
`public_status=not_claimed`.

## 0. Ce que je corrige, pour la troisième fois aujourd'hui

- **Premier état** : « `terrain` échoue, il manque 0,86 d'exposant » — faux,
  fondé sur `jung_cert_skip`, un proxy étroit.
- **Rétractation** : « la génération n'est pas le mur » — **trop optimiste**,
  fondée sur `completions_q4`, qui n'existe que jusqu'à 50 000 points.
- **État présent** : des reçus portent le **mur mesuré jusqu'à 200 000**. Je
  ne les avais pas exploités. Ils disent que la génération **est** le mur sur
  la famille LiDAR, et plus durement que mon premier état ne le prétendait.

Leçon : j'ai deux fois extrapolé depuis la plage où la donnée existait, alors
qu'une plage quatre fois plus large dormait dans `receipts/`.

## 1. Le mur, mesuré

Exposants locaux (8 000 → 16 000 → 32 000 → 100 000 → 200 000) :

| famille | grandeur | 8 000 | 32 000 | 200 000 | exposants |
|---|---|---|---|---|---|
| `uniform` | mur total | 7 442 ms | 34 390 | 253 259 | 1,10 / 1,11 / 1,09 / **1,09** |
| | lanes (rects) | 714 | 2 958 | 21 294 | 0,98 / 1,07 / 1,12 / 1,01 |
| | µs-fil par ancre q4 | 26,7 | 24,8 | **26,3** | plat |
| | part des lanes dans le mur | 9,6 % | 8,6 % | **8,4 %** | — |
| `eight_clusters` | mur total | 6 694 | 36 383 | 353 057 | 1,22 / 1,22 / 1,19 / **1,32** |
| | lanes | 1 130 | 8 039 | 132 394 | 1,49 / 1,34 / 1,41 / 1,73 |
| | part des lanes | 16,9 % | 22,1 % | **37,5 %** | — |
| `scanline_single_pass` | mur total | 1 289 | 6 622 | 267 701 | 1,06 / 1,30 / 1,59 / **2,72** |
| | **lanes** | 245 | 2 586 | 239 579 | 1,55 / 1,85 / 2,07 / **3,14** |
| | ancres q4 | 804 786 | 6 951 708 | 191 710 560 | 1,39 / 1,72 / 1,71 / 1,98 |
| | **candidats q4 (la sortie)** | 47 520 | 146 222 | 710 211 | 0,82 / 0,80 / 0,85 / **0,89** |
| | µs-fil par ancre q4 | 14,6 | 17,9 | **60,0** | — |
| | part des lanes | 19,0 % | 39,1 % | **89,5 %** | — |
| | mur du fold | 545 | 2 024 | 14 269 | 0,93 / 0,96 / 1,06 / **1,07** |

## 2. Les trois faits qui en découlent

1. **`uniform` est linéaire de bout en bout** (mur $n^{1{,}09}$, coût par
   ancre plat à 26 µs-fil, lanes 8,4 % du mur). Rien à y corriger.
2. **`scanline_single_pass` explose, et c'est la génération** : le mur croît
   en $n^{2{,}72}$ entre 100 000 et 200 000, les lanes en $n^{3{,}14}$, et
   elles passent de 19 % à **89,5 % du mur**. Le fold, lui, reste linéaire
   ($n^{1{,}07}$) et ne pèse plus que 5,3 % : à 200 000 points le problème
   n'est plus du tout celui que j'avais identifié à 50 000.
3. **La sortie est SOUS-LINÉAIRE** ($n^{0{,}89}$) : 710 211 candidats pour
   191 710 560 ancres à 200 000 points. **Le travail croît d'un facteur 238
   par unité de sortie entre 8 000 et 200 000.** C'est le rendement, pas la
   taille, qui s'effondre.

## 3. Ce que cela impose comme contrat

Pour tenir 10 M en une session gardée de 8 h, depuis le mur mesuré à
200 000 :

| famille | mur à 200 000 | exposant mesuré | exposant maximal admissible | verdict |
|---|---|---|---|---|
| `uniform` | 253 s | 1,09 | 1,21 | **tient** (5,0 h extrapolées) |
| `eight_clusters` | 353 s | 1,32 | 1,13 | échoue de 0,19 (17,1 h) |
| `scanline_single_pass` | 268 s | **2,72** | 1,20 | **échoue de 1,52 (130 jours)** |

La mémoire, elle, est moins critique que je ne l'avais dit : le RSS par point
**décroît** sur `scanline` ($n^{0{,}87}$ ; 54,3 ko par point à 200 000 contre
73,9 à 50 000), ce qui donne ≈ 319 Go à 10 M — un facteur 1,8 au-dessus des
180 Gio de la VM, pas 4. Sur `uniform` il reste à ≈ 2,6 To.

## 3 bis. La formulation définitive : la fraction de paires survivantes

La WSPD énumère toutes les paires ; l'élagage de rectangle en tue une partie ;
le reste devient des ancres. **Le nombre d'ancres est linéaire en $n$ si et
seulement si la fraction survivante décroît en $n^{-1}$.** Cette fraction est
directement mesurable — ancres q4 divisées par $\binom{n}{2}$ :

| famille | 8 000 | 16 000 | 32 000 | 100 000 | 200 000 | exposant de la fraction |
|---|---|---|---|---|---|---|
| `uniform` | 4,010 % | 2,125 % | 1,120 % | 0,379 % | 0,195 % | −0,92 / −0,92 / −0,95 / **−0,96** |
| `eight_clusters` | 11,909 % | 8,345 % | 6,618 % | 3,495 % | 2,151 % | −0,51 / −0,33 / −0,56 / −0,70 |
| `scanline_single_pass` | 2,515 % | 1,652 % | 1,358 % | 0,971 % | 0,959 % | −0,61 / −0,28 / −0,29 / **−0,02** |

**Sur `scanline`, entre 100 000 et 200 000, la fraction survivante a cessé de
décroître ($n^{-0{,}02}$).** Elle est bloquée à ≈ 0,96 % de toutes les paires.
C'est, à la lettre, la définition de la quadraticité : un nombre d'ancres
proportionnel à $\binom{n}{2}$.

Sur `uniform` la même fraction décroît en $n^{-0{,}96}$ — presque exactement le
$n^{-1}$ qu'exige la linéarité, ce qui explique que la famille soit saine.

**L'objectif devient donc un énoncé unique, mesurable et falsifiable :**

> faire décroître la fraction de paires survivantes de `scanline` en
> $n^{-1}$ au lieu de $n^{-0{,}02}$.

C'est un objectif d'**élagage au niveau du rectangle**, puisque c'est lui qui
détermine cette fraction. Ni la WSPD (linéaire), ni le fold (linéaire), ni la
sortie (sous-linéaire) n'y entrent.

## 3 ter. Les compteurs sont indépendants de la machine — la mesure peut se faire localement

Vérification faite : un run local à **8 fils** et le reçu G4 à **48 fils**
donnent, à $n = 100\,000$ sur `scanline`, **exactement** les mêmes nombres —
48 557 755 ancres q4 et 384 464 candidats q4. Les compteurs sont donc
déterministes et citables hors G4 ; seuls les **temps** dépendent de la
machine. Mesuré localement à 8 fils, `scanline` donne entre 100 000 et
150 000 : mur $n^{2{,}10}$, ancres $n^{1{,}99}$, candidats $n^{0{,}87}$,
RSS $n^{0{,}92}$ — cohérent avec le reçu G4.

Conséquence pratique : les campagnes d'exposant sur les **compteurs** ne
demandent pas de session payante. Seules les mesures de **temps** et de RSS à
grande échelle en demandent une.

## 4. Où porter l'effort, désormais sans ambiguïté

La cible est **le coût par ancre de `scanline`** : 14,6 → 60,0 µs-fil quand
$n$ passe de 8 000 à 200 000, alors qu'il est **plat sur `uniform`** (26 µs) et
quasi plat sur `eight_clusters`. Ce n'est ni la WSPD (rectangles linéaires),
ni le fold (linéaire), ni la sortie (sous-linéaire) : c'est le travail dépensé
par ancre pour ne rien produire.

Les deux termes candidats, déjà nommés et mesurés ailleurs, sont le **cover
par ancre** (rebalayé pour chaque ancre du rectangle) et son **amplification**
$\lvert \text{cover} \rvert / \lvert P \cap W_q \rvert$, mesurée à 0,48 stable
sur `uniform` contre 15,3 et croissante sur `scanline`. Je ne propose pas
encore de mécanisme : la mesure qui manque est la décomposition du coût par
ancre à 100 000 et 200 000 points, et elle demande une session G4 puisque la
sonde n'a jamais tourné au-delà de $n = 16\,000$.

## 5. Réserves

- L'exposant 2,72 est une pente locale entre **deux** points (100 000 et
  200 000). Elle est cohérente avec la suite croissante 1,06 / 1,30 / 1,59,
  mais deux points ne font pas une loi.
- `terrain` n'a **aucun** run à 100 000 ni 200 000 : sa pente s'arrête à
  50 000 et je ne sais pas ce qu'il fait au-delà.
- Les temps sont ceux d'une VM partagée à 48 fils ; les rapports d'un même
  run sont fiables, la comparaison entre runs l'est moins.
- L'extrapolation à 10 M suppose l'exposant constant sur deux décades, ce
  qu'aucune mesure n'établit.
