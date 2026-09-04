# AxisBounds : proposition isolée, 4 septembre 2026

Statut : overlay NON intégré. Aucune source réelle, aucun CMake, aucun test
réel ni banc réel n'a été modifié par cette proposition. Ce jalon ne promeut
ni l'exactitude horizontale globale, ni la disponibilité industrielle, ni
le contrat 50 000 points / tour K1..K10 en une seconde. Le palier 100 ms ne
vient qu'après validation du même contrat à une seconde.

## Objet et preuve locale

Le census utilise la forme $P(z)=a\sum_{i=1}^{3}z_i^2+\sum_{i=1}^{3}b_i z_i+c$,
avec $a>0$. Il calcule les extrêmes sur la boîte entière de réseau de chaque
nœud, pas seulement sur les points présents. Pour chaque axe, poser
$f(t)=at^2+bt$, $q=\lfloor\frac{-b}{2a}\rfloor$ et $r=-b-2aq$.
Alors $0\leq r<2a$ et $f(q+1)-f(q)=a-r$.
Un minimiseur entier global est donc $m=q+\mathbf{1}_{r>a}$ ; lorsque $r=a$,
les deux voisins sont minimisants et le choix de $q$ est déterministe.

La différence discrète $f(t+1)-f(t)=a(2t+1)+b$ croît strictement avec $t$.
La suite décroît avant son ensemble de minimiseurs puis croît après. Sur
un intervalle entier non vide $[\ell,h]$, un minimiseur est donc
$\mathrm{clip}(m,\ell,h)$. Cette propriété vaut aussi lorsque le minimiseur
global n'appartient pas à l'intervalle. Toutes les boîtes du CloudIndex
admis sont incluses dans $[0,65535]^3$. Par conséquent on peut d'abord
calculer $m_0=\mathrm{clip}(m,0,65535)$, puis utiliser
$\mathrm{clip}(m_0,\ell,h)=\mathrm{clip}(m,\ell,h)$.

L'overlay calcule et conserve ces trois $m_0$ une fois par AxisBounds,
c'est-à-dire une fois par boule et par passe. Le minimum sur chaque boîte
demande ensuite une seule évaluation quadratique par axe, contre quatre
candidats clippés dans la source initiale. Le maximum reste celui des deux
extrémités. La somme des extrêmes par axe est exacte sur le produit
cartésien de réseau. Le census ne change aucun signe de frontière :
`mn >= 0` exclut l'intérieur strict dans depth ; `mn > 0` seul élague le
census complet ; `mx < 0` seul autorise le range-add strict.

Il n'y a ni changement du nombre d'incidences, ni approximation, ni
nouvelle structure globale. L'objet AxisBounds reste local à une requête
et conserve une référence à la clé immuable ; seules trois valeurs i64
s'ajoutent sur la pile. Les piles de descente partagées restent inchangées.

## Largeurs et domaine

Préconditions conservatrices du tableau § 4 de
`morsehgp3D_v7/docs/QUALIFICATION_S1_PRIMITIVES.md` :
$0<a<2^{68}$, $|b_i|<2^{87}$, $|c|<2^{105}$ ; les coordonnées sont u16.
Elles incluent les commentaires historiques plus étroits de `keys.hpp`
($|b_i|<2^{86}$ et $|c|<2^{104}$), sans dépendre de leur éventuel raffinement.
La réconciliation est constructive : dans `q3_form` et `q3_ball_form`,
$G<2^{68}$ et $|W_i|<2^{86}$ impliquent
$|B_i|=|2Ga_i+W_i|<2^{87}$ et
$|C|\leq G\sum_i a_i^2+\sum_i|W_i a_i|<2^{105}$.
Les formes q2/q4 ont des bornes plus petites. La division par le PGCD
positif ne peut augmenter le module d'aucun coefficient.
Alors $2a<2^{69}$, la négation de $b_i$ est sûre et
$|2aq|\leq |b_i|+2a<2^{88}$. La formation du reste ne déborde donc pas
i128. $q$ et $m$ restent dans i128 ; la conversion en i64 n'a lieu
qu'APRÈS le clip u16. Cela évite de convertir un centre lointain en i64
et surtout d'évaluer un polynôme au centre lointain. Sur le réseau u16,
on a $|at^2|<2^{100}$ et $|b_i t|<2^{103}$ ; la somme des trois axes et de
$c$ a donc module $<2^{107}$, comme le tableau conservateur. Chaque
intermédiaire reste représentable dans i128, pas seulement le résultat.

Ce n'est pas une garantie pour un BallKey arbitraire occupant tout i128 :
par exemple `b = INT128_MIN` invaliderait la négation. Aucun nouveau garde
produit n'est inventé. Les tests hors domaine vérifient leur rejet par le
validateur de FIXTURES avant appel, et le rejet effectif d'un point négatif
par CloudIndex. Ils ne prétendent pas que AxisBounds valide ses propres
préconditions. Des coefficients synthétiques aux limites servent à tester
les largeurs ; leur présence ne prouve pas qu'une lane les produit.

## Porte indépendante et résultats observés

`tests/axis_bounds_gate.cpp` emploie OBig à limbes 32 bits, copie octet pour
octet de l'oracle existant. Il énumère chaque entier des intervalles et,
sur des petites boîtes, chaque point du volume 3D. Il ne partage ni
division, ni calcul de sommet, ni règle d'arrondi avec AxisBounds.
Tout débordement de l'oracle interdit le verdict (code 3). Les planchers
de non-vacuité sont explicites et ne reposent pas sur `assert`.

Release GCC 13.3.0, `-O3 -std=c++20 -Wall -Wextra -Wpedantic -Werror
-DMHGP7_TESTING=1` : code 0. ASAN + UBSAN avec détection des fuites,
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` : code 0.
Sortie identique dans les deux cas :

```text
OK axis_bounds boxes=1212 axis_points=454697 volume_points=31720 census_queries=45 max_bits=106 rejected_fixtures=10 extended_profile_boxes=4
```

La porte vérifie aussi census complet / profondeur sur 729 points,
plusieurs rayons avec coquilles entières, les plafonds intérieur/coquille,
et les cas rayon nul, intervalle singleton, centre extérieur, voisins
entiers égaux, coordonnées 0/65535 et coefficients larges. Quatre boîtes
franchissent SIMULTANÉMENT les deux anciennes bornes B/C ; ce compteur est
un plancher obligatoire. Les valeurs exactes $|B|=2^{86}$ et $|C|=2^{104}$
des deux signes sont désormais des cas positifs, pas des rejets. Les dix
rejets visent les limites conservatrices (B/C des deux signes incluses),
les A non positifs ou trop larges et les boîtes hors u16 ou inversées.

Les cinq mutants sont réellement dans le header overlay, inscrits dans
son registre overlay, et tous tués par une divergence min/max (code 4) :

| Mutant | Première divergence | Champ |
| --- | ---: | --- |
| `axis-argmin-floor-only` | boîte 1 | minimum |
| `axis-argmin-ceil-always` | boîte 1 | minimum |
| `axis-argmin-no-clip` | boîte 7 | minimum |
| `axis-argmin-narrow-coefficient` | boîte 8 | minimum |
| `axis-argmin-max-min` | boîte 1 | maximum |

Ni le choix opposé à égalité exacte, ni le simple remplacement de floor
par trunc lorsque le centre négatif est de toute façon clippé à zéro ne
sont présentés comme des défauts observables du domaine u16. Le mutant
OBig au-delà de 128 bits n'est pas revendiqué tué par cette porte à 106 bits.

## Microbench borné, pas une mesure de tour

`bench/axis_bounds_microbench.cpp` compare la copie explicite de la source
initiale, le seul cache du plancher, et l'argmin entier de l'overlay. Cette
copie initiale est un comparateur de performance, JAMAIS l'oracle.
256 clés mixtes petites/larges, centres dans u16, une entrée identique par
bras, cinq essais dans un ordre cyclique ; allocations/copies hors région
chronométrée. Un changement déterministe de `c` par passe évite que le
compilateur supprime les répétitions. Les digests des trois bras doivent
coïncider. Les variantes `min_max` et `min_only` rendent observable la
possibilité d'élimination du maximum inutilisé dans le census complet.

Hôte local : AMD EPYC 7763 virtualisé, 8 vCPU ; GCC 13.3.0 `-O3 -DNDEBUG`,
sans `MHGP7_TESTING`. Affinité `taskset -c 6`. Commandes exécutées :

```bash
taskset -c 6 build/v7_axis_bounds_fix/axis_bounds_microbench 1 32
taskset -c 6 build/v7_axis_bounds_fix/axis_bounds_microbench 8 32
taskset -c 6 build/v7_axis_bounds_fix/axis_bounds_microbench 64 32
```

Résultats bruts : `microbench.jsonl` (90 observations). Médianes locales :

| Boîtes par boule | Mode | Initial ms | Cache plancher ms | Argmin ms | Ratio initial/argmin |
| ---: | --- | ---: | ---: | ---: | ---: |
| 1 | min + max | 0,535 | 0,537 | 0,437 | 1,22 |
| 1 | min seul | 0,557 | 0,512 | 0,362 | 1,54 |
| 8 | min + max | 3,300 | 3,087 | 1,932 | 1,71 |
| 8 | min seul | 2,586 | 2,493 | 1,068 | 2,42 |
| 64 | min + max | 23,077 | 24,418 | 13,955 | 1,65 |
| 64 | min seul | 19,419 | 20,047 | 4,992 | 3,89 |

Les durées sub-milliseconde sont particulièrement sensibles au bruit. Ces
observations sur un microbench synthétique ne prouvent ni gain global,
ni p95, ni qualification de machine dédiée. Le cache du seul plancher
n'apporte pas de gain stable ici ; le compilateur pourrait déjà déplacer
ce calcul hors des boucles. L'attribution formelle exigerait inspection
d'assemblage/profil. Le gain de l'argmin observé suffit à justifier une
paire end-to-end mono B/C après autorisation, pas à l'annoncer acquise.

Le microbench a précédé la réconciliation du domaine : son header est
reconstructible depuis `census.before.hpp` et `census.microbench.patch`
(SHA `a152e17fd4e2c378b348340a2a1396b86f46e3cdf1ac0bddb5bdcdede7ec191a`).
La réconciliation ne change que les commentaires du header et étend la
porte OBig ; elle ne change aucune instruction d'AxisBounds. Les chiffres
restent des observations de ce binaire antérieur, pas un nouveau run.

## Intégration proposée, non appliquée

Les fichiers `census.patch`, `mutants.patch`, `gate.patch` et
`microbench.patch` ciblent la v7 réelle. `git apply --check` des quatre
ensemble : code 0 sur la source gelée observée. Les CTests restent à
enregistrer par le responsable du build après GO : porte nominale code 0,
cinq portes avec nom mutant en premier argument, code 4 ET préfixe
`DIVERGENCE axis_bounds` exigés. Ne pas accepter n'importe quel échec.
Le microbench ne doit pas devenir un CTest avec seuil temporel instable.

Avant promotion locale de l'optimisation : requalifier les portes census,
les lanes consommatrices, les digests de la route complète et la paire
mono K1..K10. Aucun GCP utilisé par cet overlay.
