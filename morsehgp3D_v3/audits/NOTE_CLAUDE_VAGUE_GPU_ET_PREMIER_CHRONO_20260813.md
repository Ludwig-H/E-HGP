# Note de Claude — une WSPD en forme de kernel, et le premier chrono du chantier

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

## 1. La WSPD se met en forme de kernel, et les trois obstacles tombent

**L'arbre.** La borne de Callahan–Kosaraju ne demande pas le *fair split tree* :
elle vaut pour un **octree comprimé**, qui se construit par tri radix des codes
de Morton puis **arbre radix de Karras** — un thread par nœud interne, aucune
récursion. Ses nœuds sont des cellules alignées de côté `2^k`, donc d'aspect
exactement un.

**Le cas diagonal se déroule.** `WSPD(v,v) \to \lbrace (L,L),(R,R),(L,R)\rbrace`
n'est rien d'autre, une fois déplié, que « pour chaque nœud interne `v`, émettre
la graine `(gauche(v), droite(v))` ». Exactement `n-1` graines, **un seul kernel
plat**. Toute la récursion diagonale disparaît. Vérifié par l'oracle : la vague
partitionne les paires exactement, `1128` et `2016` clés, aucune diagonale,
aucun doublon, aucune manquante.

**La récursion croisée** devient une vague `count -> scan -> fill`.

Trois propriétés gravées comme portes :

| | valeur |
| --- | --- |
| `tests / front` | **`1,98`** — le travail vaut deux fois la sortie |
| vagues | `29` à `37` — borné par `2\log_2 n` |
| vague maximale | `46` paires par point |

## 2. Cellule contre boîte serrée

| boîte | `n=8 000` | `n=32 000` | pente |
| --- | ---: | ---: | :---: |
| cellule de Morton | `230,9`/pt | `276,3`/pt | `1,130` |
| boîte serrée | `51,1`/pt | `58,4`/pt | `1,096` |

La cellule porte la borne mais elle est grossière — facteur `6,5`. La boîte
serrée lui est **incluse**, donc la séparation ne peut qu'être plus facile, et
la partition reste garantie par la **structure de la récursion**, non par le
test. Elle ne coûte que `11 %` de front de plus que la récursion séquentielle.

## 3. Le premier chrono, `n=50 000`, mono-thread

| phase | temps |
| --- | ---: |
| arbre Morton, Karras plus boîtes serrées | **`8,7` à `26,1` ms** |
| vague WSPD seule, `3 040 527` terminaux | **`414,9` ms** |
| plus banque `W=32, L=16` — `48,6` M recertifications | `2 589` ms |
| plus banque `W=64, L=32` — `97,3` M recertifications | `3 706` ms |

**Sortir `D_{lo}` de la boucle de candidats a divisé le temps par `2,6`** —
`6 618` puis `2 589` ms. C'était votre spécification, et je l'avais implémentée
à l'intérieur de la boucle.

Deux enseignements de coût :

1. **la vague elle-même est bon marché** — `415` ms pour trois millions de
   terminaux, soit `69` ns par test, dominé par les accès mémoire ;
2. **la banque domine**, et son coût vaut exactement `W \times front`. Le levier
   est donc le front ou la fenêtre, pas la constante arithmétique.

Votre porte de falsification demande
`p95(\text{banque} + \text{masque} + \text{compactage} + \text{handoff}) \le 200` ms
à `50 000` points. Mono-thread j'en suis à `2,2` s pour la configuration
`s=2, W=32, L=16`. Un facteur trente sur device rendrait `73` ms. **C'est une
extrapolation d'ingénierie, pas une mesure**, et je ne la présente pas
autrement.

## 4. Ce que je vous demande

1. La constante de la borne pour l'octree **comprimé** en 3D : je ne l'ai pas
   vérifiée, et je m'attends à ce qu'elle soit meilleure que celle du fair split
   puisque les cellules sont alignées — mais c'est une attente.
2. **Le seul risque réel que je vois.** La compression saute les niveaux vides,
   donc deux nœuds voisins peuvent avoir des niveaux très différents. La
   récursion « couper le plus gros » reste correcte, mais l'argument
   d'empilement demande `\lvert niveau(A)-niveau(B)\rvert \le 1`, ce que la
   compression peut violer. La borne tient-elle malgré cela ?
3. Est-il licite de tester la séparation sur la **boîte serrée** tout en
   invoquant la borne établie sur la **cellule** ? La partition est garantie par
   la récursion, et la boîte serrée est incluse — mais la borne de cardinal,
   elle, a été prouvée sur des objets d'aspect un.

## 5. Non-claims

Aucun `p95` device, aucun octet, aucun high-water. Le tri Morton n'est pas
inclus dans le chrono de l'arbre. La fraction fermée rapportée ici est en
**records**, non en masse — `38,5 %` de records à `s=4` correspond à une part de
masse bien supérieure, et les deux ne doivent pas être confondues. Le contrat
`50 000` reste entièrement ouvert et G4 reste NO-GO.
