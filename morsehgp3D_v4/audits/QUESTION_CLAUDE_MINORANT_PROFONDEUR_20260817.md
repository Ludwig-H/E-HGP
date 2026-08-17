# Question de Claude — un minorant de profondeur par boule, à la génération q4 ?

Date : 17 août 2026. Contexte : le pré-filtre de profondeur est exécuté
(descente comptante exacte, mutant tué, reçu
`ADDENDUM_PREFILTRE_PROFONDEUR_20260817.md`) et son résultat est
honnêtement NEUTRE (~3 % de census à n=400). La donnée qui reste : à
n=400, le générateur q4 émet 6 858 491 candidats sur 7 597 781 boules
uniques, pour 104 802 événements finaux — un facteur ~65 d'excès que
tout l'aval paye (tri RLE, census, pré-filtre). Le poste de coût est le
NOMBRE de boules, pas leur prix unitaire.

## La question mathématique

Existe-t-il un MINORANT DE PROFONDEUR PAR BOULE, calculable à la
génération d'un candidat q4 `B = circonscrite(a, b, x, y)` (ancre
`(a,b)` arête maximale, complétion `(x,y)`), SANS descente d'arbre, qui
permette de refuser `|I_B| > 9` avec certificat ? Ce que je sais :

1. Le lemme de complétude sous les seuils `h_q` borne les témoins de
   fuseau de l'ANCRE — il protège l'ancre d'un plateau pertinent, mais
   ne borne pas `|I_B|` : la boule déborde le fuseau, les témoins
   minorent du mauvais côté.
2. La boule d'un support minimal q4 d'ancre `(a,b)` a
   `R² <= |ab]²/2` (Jung, § 4.5) : `B` est incluse dans la boule de
   centre `milieu(a,b)` et de rayon `~1,23·|ab|` — un compte local
   autour de l'ancre MAJORE donc `|I_B|`... par le mauvais côté aussi
   (il faudrait un minorant pour REFUSER).
3. Pour minorer `|I_B|`, il faut exhiber des points DANS `B` : la seule
   région certifiée sans géométrie fine est le voisinage du segment
   `[a,b]` (corde de `B`) — mais la boule diamétrale de `(a,b)` n'est
   PAS incluse dans `B` (contre-exemple : corde plate), et je ne vois
   pas de région canonique incluse dans TOUTE boule de corde `(a,b)`
   autre que le segment lui-même (mesure nulle).

Ma conclusion provisoire : un minorant PAR BOULE sans structure
auxiliaire n'existe probablement pas sous cette forme — la profondeur
dépend de la position du centre, que seule la complétion détermine.
Contre-argument bienvenu.

## L'alternative que je propose (et que je compte prendre, sauf avis)

Re-dériver la SÉLECTION AXIALE comme PRÉ-CLÉ DE CANDIDATS plutôt que
comme sélection : lors de l'énumération des complétions `(x,y)` d'une
ancre, un rang axial certifié (§ 4.6) élimine les complétions dont la
boule ne peut pas être de support minimal pertinent, AVANT d'émettre le
candidat. Son économie a changé depuis sa réception négative : comme
SÉLECTION elle coûtait plus cher que la baseline énumérée (6,8 s →
31,7 s, reçu axial) parce qu'elle payait sa propre machinerie par ancre ;
comme FILTRE elle économise désormais ~4 µs de tri + census aval PAR
CANDIDAT ÉVITÉ — à 6,86 M de candidats q4, le point d'équilibre est à
~1 µs/candidat de coût de filtre pour un taux de rejet même modeste.

Trois questions précises :

1. Voyez-vous un minorant de profondeur par boule certifié que j'aurais
   manqué (point 3 ci-dessus) ?
2. La pré-clé axiale comme filtre de candidats vous semble-t-elle la
   bonne voie, et sous quelle porte (baseline énumérée jugée + mutant
   qui désactive le filtre et doit rester EXACT mais plus lent, ou
   plancher de rejet mesuré) ?
3. Ordre : ce filtre AVANT la première campagne n=8000 (le flux actuel
   à n=400 laisse craindre des heures à n=8000), ou une campagne
   n=8000 d'abord pour graver la baseline de coût ?

Je pars sur la pré-clé axiale comme filtre sauf contre-ordre, après la
prochaine relecture de vos audits.
