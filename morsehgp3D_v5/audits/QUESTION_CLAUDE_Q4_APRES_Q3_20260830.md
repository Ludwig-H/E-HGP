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

Le quatrième point `y` ne répare pas lui-même ce déficit pour la boule qu'il
complète : c'est un support, donc sa puissance est nulle, tandis qu'un témoin
de profondeur doit être strictement intérieur. Il peut contraindre le centre,
servir de témoin à une autre boule et multiplier les essais ; aucun de ces
effets ne fournit un transfert simple de q3 vers q4.

Jouer d'abord le même bras canopée apparié en q4 est la bonne décision. Garder
`terrain` inchangé comme adversaire et ajouter une contre-famille distincte ;
figer les tirages latents avant de remapper le lift. À chaque taille et graine,
publier séparément W4, ancres atteignant les seeds, seeds, complétions, morts
cœur/corde/profondeur, candidats, mur et HWM. La pente `2,29` des morts de
profondeur est un compte seed 3, pas encore un artefact causal.

### V159 — le bon dénominateur révèle un facteur q4 bien plus fort

Dans `process_anchor_q4`, `q4_completions` est incrémenté avant les rejets de
distance, owner, exact-once, i64, puissance de face, déterminant, centre et
profondeur. Ce compteur mesure donc des **essais D**, pas des complétions
admissibles. `seeds[1]` contient en outre toutes les faces aiguës, y compris
celles tuées avant la boucle D. Le nombre de faces qui l'atteignent se déduit :

```text
faces_D = seeds[1] - seeds_killed_cells[2]
        - seeds_killed_core - seeds_killed_chord
```

Le quotient publié mélange deux mécanismes opposés :

$$\frac{\text{essais D}}{\text{seeds}}=\frac{\text{faces D}}{\text{seeds}}\frac{\text{essais D}}{\text{faces D}}.$$

Le recalcul direct de
`receipts/masses_q3_seed3_20260829/out/*_prod_r1.txt` montre que, sur `terrain`,
entre 2 000 et 32 000 points, la première fraction tombe de `0,2258` à
`0,0539`, tandis que la seconde monte de `12,58` à `95,27`, soit un facteur
`7,57` et un exposant sécant local proche de `0,73`. Leur produit donne le petit
`0,21` rapporté et masque donc la croissance conditionnelle. Le même calcul
donne `20,37 -> 64,24` sur `scanline`, contre `19,49 -> 20,31` sur `uniform` et
`29,92 -> 34,94` sur `eight_clusters`.

Il faut ajouter le compteur explicite `q4_faces_reaching_completions`, fermer
l'identité `seeds = cells + core + chord + faces_D`, et conserver l'identité
existante `essais = somme des rejets + profondeur + candidats`. Publier ensuite
essais par face D, taux cumulé de chaque rejet, entrées et tests de profondeur,
mur et HWM, stratifiés par octave et type d'ancre. La boucle de complétion est
donc un poste q4 distinct **ouvert**, possiblement important aussi sur
`scanline` ; `d280fb2c` ne permet pas de la déprioriser.

### V160 — ce qui se transporte

- La cascade `h_coeur+h_a+h_b` se transporte à la porte d'ancre W4, avec seuil
  q4 et provenance propre. Elle ne se somme pas ensuite à des témoins de seed,
  corde ou complétion sans IDs dédupliqués ou strates prouvées.
- Porter `EndpointCredit` au W4 est une première couture exacte si le scan
  résiduel exclut **tout** `A union B`, pas seulement `a,b`, puis s'arrête au
  besoin `h4-credit`. Ne jamais ajouter ce crédit au compte W4 complet, qui
  reconnaît déjà des siblings d'extrémité. Sur les survivantes, collecter les
  IDs résiduels effectivement vus dans une `WitnessTape` canonique ; l'union
  certifiée totale en porte au plus sept pour `h4=8`. Ne pas inventer les IDs
  derrière les scalaires `core`, `h_a` ou `h_b` tant que leur provenance typée
  n'existe pas.
- Toute précharge de corde repasse par le vrai `ChordPieces::update`, jamais par
  quatre crédits scalaires aveugles. Pour isoler la première ablation et
  préserver le **flux de propositions** historique, la tape doit être
  intersectée avec le scan downstream et garder les mêmes exclusions. Employer
  des IDs trouvés par la requête mais absents de ce scan est mathématiquement un
  certificat plus fort : il peut changer les candidats émis et exige alors
  requalification, oracle final et nouveau reçu. Le digest final doit rester
  identique dans les deux variantes ; il n'existe pas de « digest brut des
  candidats » à préserver silencieusement.
- La borne continue q4 est bien le facteur `/8` : Jung donne le rayon hors axe,
  puis l'orthogonalité à `ab` donne la borne coordonnée par coordonnée. Elle
  exige encore son arrondi dirigé, ses égalités et un mutant q4. Le résultat
  négatif q3 ne ferme pas le center-cover q4, dont le disque et l'économie sont
  différents ; il interdit seulement de le promouvoir sans shadow apparié.
- Le passage de `2,10` à `4,2` partenaires ne suffit pas à rouvrir la couche par
  lignes comme priorité. Les clés exactes déjà relues se répètent peu. Mesurer
  d'abord requêtes uniques, sites évités, mur et HWM sur les seuls survivants
  q4 ; sans non-vacuité nette, ne pas construire de cache.

La généralisation la plus directe reste asymétrique : conserver la WSPD binaire
`A x B`, prendre les handles de taille au plus 32 comme blocs `C`, puis traiter
chaque `c` comme une face avant d'énumérer `D`. Un `h_c(c)` q4 strict doit être
valable pour toutes les complétions du patch de centres considéré. `h_core` et
`h_c` comptent des positions uniques hors `A union B` et hors supports ; `h_c`
porte sur la même fibre valide non vide. Sans IDs ou strates disjointes,
composer `h_a+h_b+max(h_core,h_c(c))`, jamais la somme nue du cœur et de `h_c`.
Avec une tape, prendre l'union dédupliquée. Ce niveau face est précisément celui
qui peut supprimer les `12,58 -> 95,27` essais D par face vivante ; le prototype
doit rester CPU, counter-only et ciblé d'abord sur les ancres longues du
diagnostic.

La couture implémentable ne demande pas encore d'auto-jointure globale. Pour
chaque handle `H_i` de taille `m_i<=32`, chaque carrier `c` et chaque patch q4
faisable `j`, énumérer localement les `z!=c` de ce handle, hors `A union B`, et
compter jusqu'au besoin résiduel ceux qui satisfont strictement sur tout le
patch :

```text
Phi32(q,c,z) = 2*q.(z-c) + 32*(||c||^2-||z||^2) > 0
```

Pour `c,z` fixés, le minimum en `q` est atteint à un coin du patch, donc le
test reste entier et borné. Si `g_i,j` est le crédit patch-global dans la strate
du handle et `g_not_i,j` celui des autres strates, composer exactement :

```text
b_i,j(c) = max(rect_core4, g_not_i,j + max(g_i,j, h_c,j(c)))
tau_i(c) = max over feasible j of max(0, h4 - b_i,j(c))
face (a,b,c) closed iff h_a(a) + h_b(b) >= tau_i(c)
```

`rect_core4` est le cœur porté par `AliveRect`, jamais le cœur de Jung propre à
la face. Le maximum sur les patches exprime le pire cas alternatif ; il ne
somme pas leurs témoins. Un masque faisable vide reçoit un fate d'absence
séparé, jamais `tau=0`. Avec au plus 64 patches, le coût plat vérifie
`64*sum_i(m_i^2) <= 2048*sum_i(m_i)`. Cette borne locale rend le premier shadow
viable ; elle ne borne toujours ni le nombre total de handles ni le front WSPD.

Pour mémoire, la borne `/8` se déduit sans heuristique : si `d=b-a`, `D^2` est
sa norme carrée et `t` le déplacement du centre depuis le milieu, alors
`t` est orthogonal à `d` et Jung donne `t` de norme carrée au plus `D^2/8`.
Cauchy fournit donc
pour chaque coordonnée :

$$t_i^2\leq\frac{D^2-d_i^2}{8}=\frac{\sum_{j\neq i}d_j^2}{8}.$$

### V161 — ne pas choisir entre cœur et corde avec des compteurs de morts

Les pentes `chord_kill` et `core_kill` comptent des décisions, pas du temps. Le
seul probe disponible agrège encore cœur+corde (`2258/2410 ms` sur
terrain/scanline à `n=8000`) et les place devant les complétions (`1057/939 ms`),
mais il n'est pas reçu et ne départage pas les deux sous-étages.

Le prochain incrément utile est donc partagé : instrumenter séparément sites et
temps du cœur, mises à jour de corde, faces D, essais D et profondeur, puis
raccorder le `WitnessTape` W4. En parallèle, le shadow `h_c(c)` sur
`A x B x C` mesure combien de faces et d'essais D il ferme avant la complétion.
Le compteur conditionnel montre qu'il ne faut choisir ni une micro-optimisation
de corde ni l'abandon de la complétion avant ce split.

Verdict borné : mesurer la canopée q4, le split cœur/corde et le coût
`essais_D/faces_D` ; prototyper ensuite `WitnessTape -> h_c(c) -> D`, avec
`s=8`, trois graines et reçus appariés. Aucun claim de complexité, d'exactitude
Gamma ou de résultat GPU n'en découle.

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

Enfin, V159 corrige la sémantique de `q4_completions`, donne le dénominateur
`faces_D` et retire la conclusion « ne pas chercher côté complétion ». Il faut
recevoir en parallèle le certificat ciblé des ancres longues et la
décomposition `cellule -> cœur -> corde -> faces_D -> essais D` pour départager
réellement les deux leviers.

### Garde immédiate sur la sonde q4 en cours

Le scratch `q4lanep.cpp` relu pendant son exécution ne peut pas encore recevoir
un effet causal de canopée : changer la borne de `uniform_int_distribution`
peut changer la consommation du MT, puis les collisions de `z` changent la
déduplication et l'arrêt à `n`. Il faut figer les propositions latentes et
accepter un même ensemble d'indices dans tous les bras, ou déclarer des
contre-familles indépendantes avec incertitude, jamais des paires.

L'échantillonneur courant hache le rang `ir` dans `alive`. Ce rang et la liste
WSPD changent avec le nuage ; les rectangles des bras ne sont donc pas appariés.
En outre, le test modulo sélectionne un effectif aléatoire autour de la cible,
alors que la sortie imprime la cible comme si elle était réalisée. Employer un
bottom-k exact sur des `AnchorKey` stables après appariement des PointId, ou
publier `sel.size()`, les probabilités d'inclusion et l'incertitude groupée.

Trois corrections de grand-livre sont nécessaires avant usage des sorties :

- imprimer `faces_D` et `q4_completions/faces_D`, pas seulement
  `q4_completions/seeds[1]` ;
- imprimer `depth_killed/q4_depth_entries` et
  `q4_power_tests/q4_depth_entries`, pas `depth_killed/q4_completions` ;
- refuser si la partition des ancres ou des seeds ne ferme pas, au lieu de
  masquer un éventuel sous-dépassement par une soustraction saturée.

Enfin, la copie du wrapper produit omet actuellement l'alimentation de
`q4_covers_built`, `q4_cover_visits` et `q4_cover_sites`; ces trois valeurs du
bloc `PROFIL` sortiront donc à zéro et ne mesurent rien. Le bras historique
bit-identique est une bonne garde, mais le cap constant de référence exact à
2 000 points est 27, non 28. Versionner source, commande, stdout, hash du
binaire, `HEAD` et état du worktree avant tout nouveau verdict.
