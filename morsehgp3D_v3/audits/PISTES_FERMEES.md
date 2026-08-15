# Pistes fermées — mémo court

`public_status=not_claimed`. Rien ici n'est une autorité. Le verdict mutable est
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

Ces tentatives ont été menées, mesurées, puis fermées. Leurs textes ont été
supprimés le 15 août 2026 — ils étaient absorbés ou réfutés, et les garder
entiers coûtait plus que ce qu'ils apprenaient. Ce mémo garde ce qui vaut
d'être su : l'idée, ce qui l'a tuée, et ce qui en survit dans le code actuel.
Le détail reste dans l'historique Git.

## Front, localité, chambres

| tentative | ce qui l'a tuée | survit |
| --- | --- | --- |
| front de Jung coalescé par dual-tree d'ancres | `4,85` → `23,84` M de visites q3 entre `n=500` et `1000`, pentes `2,30`/`2,33` contre une porte à `1,35` | front coalescé à `141,18 n` |
| banque directionnelle de chambres Yao-48 | la chambre fait `54,74°` quand un témoin de Jung en exige `< 35,26°` (q3) et `< 31,13°` (q4) ; la condition q4 délimite un **anneau**, pas un préfixe radial | `3 D_i^2 < D_j^2` place `b_i` dans la boule diamétrale, exact en `i64` |
| génération locale exacte par cône (`certified_locality_probe`) | faux vert : `681/795/174` contre `681/884/202` au juge exhaustif ; `4,65 s` contre `1,39 s` pour le scan remplacé | contre-fixture extra-shell / support non unique |
| cône cible par endpoint alimenté par banque k-NN | aucune série ne ferme deux pentes `<= 1,35` ; `39,2` M de tests témoin-nœud à `n=2000` | **le noyau ponctuel `H>0, 4H^2>E_2X_2` / `3H^2>E_2X_2` et la porte `ALL` par huit coins** — repris tel quel |

## Cellules de centres

| tentative | ce qui l'a tuée | survit |
| --- | --- | --- |
| Source S par listes imbriquées de cellules de centres | snapshot non transférable au source live ; supersédée par `CKPairTape` | **le lemme profondeur–cellule** `beta <= R_p(C)` |
| juge rationnel indépendant des cellules | porte vacueuse : driver sans `--judge`, refus en code 2 converti en vert par `WILL_FAIL` ; six vérités manquantes une fois le flux muté soumis | positivité barycentrique stricte = centre dans `relint conv(U)` |
| sentinelle top-`(12-q)` hors support, parallélisée | ne réduit ni les cellules ni les `839 582 666` occurrences, **et casse la télémétrie** : `7 012` occurrences contre `22 543` lifts, code retour zéro | le théorème de la sentinelle et sa minimalité |
| pentes vertes de `uniform` comme propriété du générateur | binaire non gelé, `wall_s` relevés sous charge concurrente | `coord = sqrt(25 n)` fait croître la boîte de `terrain` en `n^1,5` |

## Source par ancre, lentille aiguë

| tentative | ce qui l'a tuée | survit |
| --- | --- | --- |
| coupure de lentille aiguë fermant des chambres de paires | carrier aigu sur **`300/300`** paires échantillonnées à `eight_clusters,n=50000` | **le théorème de face adjacente aiguë**, encore porté par la lane q4 |
| couple de carriers dans la lentille ⟹ ancre diamétrale | fixture à cinq points : `||x-y||^2 = 144 > D^2 = 100`, le centre sort de l'ellipse, rang `4` au lieu de `5` | la correction `||x-y||^2 <= D^2`, posée avant le produit q4 |

## Ledger, owner, GPU

| tentative | ce qui l'a tuée | survit |
| --- | --- | --- |
| premier ledger des causes de lifts | ne ferme pas : `130 033` occurrences sans attribution ; quotients divisant trois populations par les seules acceptations | rejets owner `96,1 / 91,7 / 92,1 %`, qui motivent le groupement avant lift |
| histogramme de multiplicité `SupportKey` | ses trois issues sont un **stade maximal**, pas des propriétés orthogonales | la fermeture à écart nul par arité |
| réemploi du prune Yao48 `P1a` de la ligne enregistrée | **aucun prune ne survit au portage** : `dist2 >= 3D` est faux comme preuve de dix intérieurs stricts — un témoin tombe sur la coquille | deux fixtures q2 gravées |
| déduplication `SupportKey` avant géométrie | diagnostic devenu la route : `39,24` géométries par support et `81,6 %` de rejets owner à `n=50 000` | le théorème du minimum auto-centré, qui fonde le « q3 par droite » |
| certificat de Helly sur le disque de Jung | reste **ponctuel** ; `F_k` autour de `180` bits sous u16, hors `i128` | le sous-certificat de taille au plus trois |
| cœur universel de Jung sur arête maximale | ne borne ni le nombre d'ancres ni le coût ; pire cas quadratique | **les relaxations `3||U||^2 < D^2` et `15||U||^2 <= 4D^2`** — les lanes q3/q4 actuelles |
| gate à trois voies comparant trois certificats | juxtapose `n=12 500`, `150`, `600` avec des ELF et univers différents, sans union commune ni pente | la récursion `A x A` en trois cas, avec la porte `paires_couvertes == C(n,2)` |

## Ordre k, Gabriel, front inverse

| tentative | ce qui l'a tuée | survit |
| --- | --- | --- |
| K-graphe de Gabriel brut | fixture `E5` : deux non-Gabriel rattachent une facette sans nouveau `PointId`, deux composantes subsistent jusqu'à `24` | l'étoile silencieuse : `<= k-1` attaches au lieu d'une clique |
| route sparse « directes + gateways » | pivot dans l'union des supports **faux sur un carré cosphérique** ; les `68,07` records par point sont des supports proposés, pas des cofaces reçues | la clé de niveau `beta = N/(4D)` et ses bornes u16 |
| borne de degré q2 par chambre canonique | **treize** partenaires q2 dans une seule chambre, sans plateau ni cosphère — tout cap 12 est réfuté | la contre-fixture, et le feu vert au filtre flottant certifié |
| Source S comme front inverse par transitions | le graphe **n'est pas connexe** : deux q4 de niveau zéro ne partagent qu'une arête, jamais une facette | quatre contre-fixtures, dont `plateau_carre_multifusion` |
| fenêtre top-`M` par ancre | zéros limités aux `SupportKey` et deux cardinalités ; `1 277` supports jamais proposés sur `eight_clusters` | **la fixture d'égalité `delta_out^2 = 100 = 4R^2`**, qui impose l'inégalité stricte |
| parcours de l'arrangement relevé (BFS puis GPU) | ses trois énoncés fondateurs sont faux hors position simple ; volume quadratique là où la sortie est linéaire ; `1 270` sommets par point à `n=800` pour `300` sphères | `order_k_flats.hpp`, qui le remplace |

## Les deux motifs d'échec

1. **La porte vacueuse.** Verte sans rien prouver : refus converti en succès par
   `WILL_FAIL`, regex qui ignore le code de retour, quantificateur `{n}` que
   `cmsys::RegularExpression` lit littéralement. Toute porte doit exhiber son
   plancher de couverture et son mutant tué.
2. **Le certificat qui coûte plus qu'il ne rapporte.** Évalué à chaque nœud
   visité, il ne peut pas économiser plus de visites qu'il n'en coûte — sort
   commun de `SOC64`, `BlockJungDual`, `HCBlockDepth`. Un gain se mesure
   **apparié**, contre une exécution désarmée.

## Réouverture

Nouveau théorème de complétude, fixture qui falsifie le motif d'abandon sans
casser les contre-exemples, architecture sans structure globale interdite, porte
de coût distincte. Un bon rappel empirique ne suffit jamais.

## `h_a` par région au lieu d'auto-jointure — 15 août 2026

**L'idée.** Le contre-audit du préfiltre combiné demandait de remplacer les
auto-jointures ponctuelles `A x A` et `B x B`, de coût annoncé
`O(|A|^2 + |B|^2)`, par des requêtes de région hiérarchiques (question Q23).

**Ce qui a été construit.** La région exacte de `h_a` est un **cône d'apex `a`**
de demi-ouverture `gamma_q = theta'_q - arcsin((r_B + 2 r_A)/D)`, et non la
boule du cœur — celle-ci est centrée à l'équateur du fuseau, loin de `A`. La
boule inscrite dans ce cône est en forme close, prouvée, gravée par deux
fixtures et trois mutants, et sûre : `oracle_faux_morts = 0`.

**Ce qui l'a fermée.** La prémisse. L'auto-jointure sort dès `h_q <= 10`
atteint, donc elle coûte `O(|A| h_q)` et non `O(|A|^2)` : ces deux postes ne
pèsent que `14,6 %` du travail sur `uniform`, la famille la plus lente. La
boule réduit le travail de `28 %` sur `terrain` mais l'augmente de `20 %` sur
`uniform`, le temps de paroi est plus mauvais dans les trois cas, et la
fermeture q4 tombe de `61,3 %` à `50,1 %` sur `eight_clusters`.

**Ce qui survit.** `spindle_core_ball.hpp` en entier, ses portes, et le chemin
`--ha=boule` gardé compilé et exercé. Surtout : le compteur `travail_ha`, qui
dit que le travail est dans la **descente du cœur** — `43` à `85 %` du total —
et non dans les auto-jointures. C'est là qu'il faut optimiser.

