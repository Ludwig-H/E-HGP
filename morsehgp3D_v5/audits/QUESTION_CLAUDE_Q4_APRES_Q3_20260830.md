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

Deux angles doivent être nommés séparément : le critère ponctuel de W4 impose
`angle(azb)>125,264 deg`, tandis que la pointe du fuseau a une demi-ouverture
`54,736 deg`, donc une ouverture complète `109,471 deg`. En q3, ces deux
descriptions donnent fortuitement `120 deg`, ce qui masquait l'ambiguïté. La
pointe q4 reste donc plus étroite. Cela rend plausible qu'une ancre
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

Les corrections numériques sont les suivantes : le seuil de l'angle sous-tendu
reste `125,264 deg`, tandis que l'ouverture complète de pointe vaut
`109,471 deg` ; l'exposant q3 des trois valeurs publiées vaut `0,020`, pas
`0,013`; les 47
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

### Addendum : deux coutures q4 exactes révélées par la sonde de complétion

Les sorties `cf_*` annoncées n'ont pas été reçues. Les deux fichiers non suivis
présents à la racine contiennent seulement
`./q4_compl_probe: No such file or directory` ; ils ne ferment aucune identité
et doivent être supprimés, pas committés. Les taux ci-dessous restent des
diagnostics seed 3 non reçus ; les décisions mathématiques, elles, peuvent être
prises indépendamment de ces taux.

#### 0. La sonde de corde courante est devenue circulaire

Le reçu historique reste attribuable au commit `f8f5b4ff`, qui introduit à la
fois `q4_chord_probe` et ses sorties. Le pin de configuration `635951d6` imprimé
dans le reçu ne contient pas encore la source de cette sonde et ne constitue
donc pas son pin source effectif.

Depuis l'intégration de la grille et de K=4 au produit, la sonde inchangée ne
forme plus une autorité indépendante. Son bras `emitted` appelle
`process_anchor_q4` avec grille et K=4 actifs ; le replay manuel omet les morts
de grille et recalcule K=1/2/4/8. `wrong=0` est alors tautologique pour K=4,
masqué pour K=2 et partiellement masqué pour K=8. Il passe aussi à vide :
`--family=uniform --n=2` produit zéro seed, zéro complétion et un code 0.

Conserver le reçu historique comme diagnostic de `f8f5b4ff`, mais reconstruire
la porte courante avec un bras contrefactuel sans grille ni corde, une partition
`grille -> cœur -> corde -> faces_D`, des comparaisons par support et des
planchers explicites.

#### 1. La corde doit lire aussi les sites `P>0`

Le `continue` de `process_anchor_q4` après `lh>seed.bound` est trop tôt. Il est
correct pour le **cœur de Jung**, qui exige `P<0`, mais faux pour les morceaux
de corde. Pour un site `z`, la condition exacte le long du faisceau est
`P(z)-mu*B(z)<0`. La valeur `P(z)>0` dit seulement que `z` n'est pas intérieur
au centre `mu=0`; si `B(z)` est non nul, `z` peut être strictement intérieur
sur un morceau extérieur.

Le théorème 10.4 et `ChordPieces::update` n'ont aucune précondition `P<0`.
`bench/q4_chord_probe.cpp` au commit historique `f8f5b4ff` parcourait déjà
**tous** les `P`. Ce diagnostic et la production ne mesurent donc pas le même
certificat : le code intégré est strictement plus faible que son théorème et
que ce banc historique. La version courante de la sonde ne peut plus recevoir
ce constat à elle seule, pour la circularité décrite ci-dessus.

La réparation minimale est de calculer `Bz` et d'appeler
`chord.update(lh,bound,Bz,exact_l)` avant de sortir de la branche `P>0`. Le
compteur de cœur continue à ignorer ce site, puis `chord.dead(h4)` doit encore
être testé. La même couture est obligatoire dans les trois autorités :

- `src/pipeline/generate.hpp` ;
- `src/gpu/q4_core_shaped.hpp` ;
- `src/gpu/q4_kernels.cuh`, où le masque `my_piece` doit être formé hors de la
  branche négative.

La stricte frontière `v_j<0` ne change pas. Un futur support `y` ne peut pas
être compté comme intérieur de sa propre boule : à son paramètre `mu_y`, sa
forme vaut zéro ; une forme affine négative aux deux extrémités du morceau qui
contient `mu_y` ne pourrait pas s'y annuler. L'emploi des mêmes `scan_sites`
que la profondeur implique aussi que les candidats bruts et leur ordre doivent
rester identiques ; seuls les seeds, essais et tests évités changent. Une
divergence de digest candidat est ici une alarme, contrairement à une future
`WitnessTape` qui ajouterait des IDs hors du scan historique.

La fixture non vacante peut rester petite et entière :

```text
a=(0,0,0), b=(4,0,0), x=(2,3,0), G=144, J=1504, mu_hat=28
z1=(1,0,1)  : P=-288, B=12, morceaux 1,2,3
z2=(0,0,-1) : P=+144, B=-12, morceau 0 seulement
```

À `h=1`, tous les morceaux sont couverts seulement si `z2` passe dans
`ChordPieces`. La complétion `y=(1,0,-2)` est strictement bien centrée et est
tuée en profondeur par `z1`; elle grave l'égalité du flux de candidats. Ajouter
un mutant `chord-skip-positive` qui restaure l'ancien saut, en plus du mutant de
frontière `chord-nonstrict`, puis exercer la même fixture dans le scalaire et
le shaped. Les portes de lane gardent ensuite la parité device.

### Retour constructif au pin `e2ac9da2`

La direction prise après `b82f1de1`, puis commitée dans `e2ac9da2`, est la
bonne : le calcul de `Bz` et `ChordPieces::update` précèdent désormais le rejet
du cœur dans les routes scalaire et shaped. Il reste toutefois une couture de
contrôle : dans
les deux routes, le site certifié positif exécute encore `continue` avant
`chord.dead(h4)`. Si ce site apporte le dernier morceau manquant, le compteur
est exact mais la mort n'est jamais publiée. Le résultat dépend alors de
l'ordre du scan. Le test de mort doit être fait après la mise à jour positive
et avant ce `continue`; pour un site non positif, garder le test après le cœur
préserve la priorité historique du cœur lorsque les deux certificats ferment
au même site.

Une fixture entière minimale à `h4=1` rend ce défaut et le mutant non vacants :

```text
A=(0,0,10), B=(4,0,10), X=(2,3,10)
NL=(3,2,9), PR=(2,3,11)
G=144, J=1504, mu_hat=28
NL : L=-768, P=-192, B=-12, pièces [1,1,1,0]
PR : L= 576, P= 144, B= 12, pièces [0,0,0,1]
somme exacte : [1,1,1,1]
```

Le cœur ne tue pas : pour `NL`, `2P^2=73728 < J B^2=216576`, tandis que `PR`
a `P>0`. Les deux ordres `(PR,NL)` et `(NL,PR)` doivent donc rendre exactement
`dead=true`, `dead_by_chord=1`, `cert_pos=1`, `cert_neg=1`, `jung_skip=1`.
Le premier état du patch rend respectivement mort puis vivant ; le mutant rend
les deux ordres vivants. La vraie route scalaire donne actuellement
`seeds_killed_chord=1/0` et `q4_completions=4/6`; après correction les deux
ordres doivent donner `1` et `4`, avec le même candidat final.

Rejeu local avant la porte de masse : les huit portes ciblées shaped, corde
historique et batch passent, mais elles ne discriminent pas ce défaut. La porte
shaped appelle explicitement `chord_on=false`, la fixture historique ne tue que
`chord-nonstrict`, et la parité batch compare deux routes portant le même
`continue`. Elles ne pouvaient donc pas recevoir le correctif.

Le `chord_positive_gate.cpp` committé améliore ce point sans le fermer. Son
plancher `terrain n=2000` sépare bien la masse nominale du mutant et rend
`mutants_gate.py` vert à `82/82/82` ; il est utile comme
régression d'intégration. En revanche, le patch encore dépendant de l'ordre
dépasse lui aussi le plancher `23000`, donc cette porte peut être verte avec la
couture de contrôle toujours fausse. Elle ne compare pas les deux permutations,
n'appelle pas la route shaped ou le kernel et ne vérifie pas le digest qu'elle
annonce identique. La garder en complément est constructif ; la fixture cinq
points reste l'autorité permanente et indépendante d'une famille, d'une seed
et d'un seuil empirique.

La fermeture utile est petite et atomique :

1. graver les deux permutations dans une porte courte qui exerce
   `ChordPieces` comme oracle local, la route shaped et la vraie route scalaire
   `process_anchor_q4` avec `h4=1` ;
2. faire attendre le code 4 de cette fixture exacte sous
   `chord-skip-positive`, indépendamment du plancher empirique ;
3. former `my_piece` pour tout site valide dans `q4_kernels.cuh`, y compris
   `lh>bound`, sans lui donner `my_wit`, puis rejouer cette fixture contre le
   vrai kernel lors de la prochaine session CUDA autorisée ;
4. seulement après ces trois égalités, interpréter le nouveau taux de morts de
   corde. Cette correction est fail-open : les digests finaux doivent rester
   identiques, tandis que le nombre de seeds et d'essais évités peut changer.

Sur device, `MHGP5_MUTANT` vaut `false` sous `__CUDA_ARCH__`. Si la même porte
doit exercer le kernel mutanté, le booléen doit donc être résolu sur l'hôte et
passé au kernel, comme `chord_nonstrict_`; appeler le registre depuis le device
ne recevrait rien.

Sur les seuls seeds ayant déjà survécu à la production actuelle, le K=4 tous
sites de la sonde tue `7426/113718` seeds et évite `17,10 %` des essais D sur
`terrain 4000`, puis `12370/129770` et `24,12 %` sur `terrain 8000`; sur
`uniform 8000`, il donne `73219/325588` et `32,88 %`. C'est un gain incrémental
utile, pas une estimation de population : l'échantillon est un modulo des
indices internes, sans bottom-k stable ni intervalle d'incertitude. La ligne
« K=4 production = 0 » est en outre tautologique puisqu'elle n'est calculée
qu'après survie au K=4 produit, et son bras ignore tout `L>0` exact alors que la
production n'ignore que les positifs certifiés flottants.

#### 2. Une complétion `y` doit atteindre la corde du seed

Pour une face `(a,b,x)`, poser `P(y)=q3_power(face,y)`,
`B(y)=dot(cross(b-a,x-a),y-a)` et
`J=D2*(3G-2*l_ax*l_bx)`. Une complétion q4 bien centrée est sur la sphère d'un
centre de paramètre `mu_y`, donc `P(y)-mu_y*B(y)=0`; Jung donne
`2*mu_y^2<=J`. Il en résulte la condition nécessaire exacte :

$$2P(y)^2\leq J B(y)^2.$$

Si l'implémentation emploie le kernel affine `L=4P`, la même condition devient
`L*L<=8*J*B*B`; écrire `2*L*L<=J*B*B` serait un défaut de facteur 16. Après la
porte existante `P>0`, `B=0` est un rejet immédiat. L'égalité doit survivre : le
rejet exact est seulement `2*P*P>J*B*B`.

Le bon emplacement est après les filtres lentille, owner, exact-once et i64,
au moment où la puissance de face est déjà calculée, mais avant Cramer et le
test du centre. Calculer `Py` une fois, conserver la sémantique et les mutants
du filtre `Py>0`, puis appeler `cmp_2p2_jb2(-Py,J,By)` : ce helper exige un
premier argument non positif. Les carrés dépassent i128 et U192 ; le U320 déjà
employé par ce helper est requis. Créer un étage distinct
`q4_rej_mu_range`, puis le porter dans `q4_completion_stage_shaped`, les
switches batch et le kernel device ; ne pas le cacher dans
`q4_rej_face_power`.

Trois fixtures séparent les contrats :

```text
survit strictement : a=(2,2,2), b=(2,0,0), x=(1,0,2), y=(1,2,0)
  P=32, B=8, J=176, 2P^2 < JB^2
frontière gardée : a=(2,2,2), b=(2,0,0), x=(0,2,0), y=(0,0,2)
  P=128, B=-16, J=128, 2P^2 = JB^2
rejet non vacant : a=(0,0,0), b=(8,0,0), x=(4,5,0), y=(4,6,3)
  P=29120, B=120, J=92032, 2P^2 > JB^2 mais P^2 < JB^2
```

La dernière tue spécifiquement un mutant qui omet le facteur 2 et atteint les
portes actuelles avant d'être rejetée plus tard par le centre. Un oracle borné
doit en plus vérifier que tout tétraèdre bien centré possédé satisfait la
condition, puis comparer candidats et digests avant/après.

Ce filtre reste ponctuel : sur `terrain 8000`, il retirerait 978 085 entrées
qui paient actuellement Cramer ou le centre, mais aucune n'atteint la
profondeur. Il réduit donc une constante **après avoir énuméré D** et ne change
pas le coût `O(|lens|)` par face. La porte `h_c(c)` sur `A x B x C`, avant la
boucle D, reste le seul des deux mécanismes qui puisse éviter cette
énumération. Ordre conseillé : réparer d'abord le K=4 tous sites, recevoir sa
fixture et sa parité ; ajouter ensuite le rejet ponctuel ; garder en parallèle
le shadow `h_c(c)` comme levier architectural sur le nombre d'essais D.

#### 3. Le minimum exact sur la corde tient dans au plus 16 racines

Il existe enfin un étage de face plus fort que K=8/16/32, sans trier tous les
sites. Chaque site définit une demi-droite stricte en `mu` : si `B>0`, il
témoigne pour `mu>P/B`; si `B<0`, pour `mu<P/B`; si `B=0`, il témoigne partout
exactement lorsque `P<0`. Sur la corde fermée
`[alpha,beta]=[-sqrt(J/2),sqrt(J/2)]`, compter d'abord les témoins constants
`c0` et poser `r=h4-c0`. Ce compte inclut `B=0,P<0`, mais aussi les racines hors
corde actives partout : `B>0,P/B<alpha` et `B<0,P/B>beta`. Les cas opposés hors
corde ne témoignent jamais. Si `r<=0`, la face est morte. Sans cette
classification, la corde `[-1,1]` avec `P=-2,B=1,h=1` serait déclarée
survivante alors que le site témoigne partout.

Pour `r>0`, il suffit de conserver :

- les `r` plus petites racines d'entrée (`B>0`) ;
- les `r` plus grandes racines de sortie (`B<0`).

En effet, leurs comptes actifs valent en tout point les comptes directionnels
complets écrêtés à `r`. Soit `upper_i` la `(i+1)`-ième plus petite entrée, ou
`beta` si elle manque, et `lower_i` la `(r-i)`-ième plus grande sortie, ou
`alpha` si elle manque. Pour `i=0..r-1`, un point de profondeur inférieure à
`h4` existe exactement lorsque `lower_i<=upper_i`. L'égalité est un échec du
certificat : à une racine commune, les deux sites sont sur la coquille et aucun
ne compte. La face est donc morte si `lower_i>upper_i` pour tous les `i`.

Les conventions aux bords sont également strictes : une entrée en `alpha` est
gardée mais ne compte qu'après `alpha`; une sortie en `beta` est gardée mais ne
compte qu'avant `beta`; une entrée en `beta` et une sortie en `alpha` ne
comptent jamais. L'appartenance d'une racine à la corde se décide par
`2*P*P` contre `J*B*B` en U320. Pour ordonner deux racines, normaliser le
dénominateur en `abs(B)` et le numérateur en `P*sign(B)` ; les produits croisés
atteignent environ 159 bits, donc U192 suffit mais i128 ne suffit pas.

Ce verdict seuil exact coûte `O(m log h4)` et `O(h4)` mémoire, donc au plus 16
racines stockées avec `h4=8`. Il force néanmoins `P` exact et des comparaisons
larges sur les sites examinés. Le déploiement prudent reste : K=4 tous sites
d'abord ; sur ses survivants à grosse lentille, shadow du minimum exact avec
scratch borné et repli fail-open, puis comparaison marginale à K=8/16/32 en
seeds, essais D et tests évités par nanoseconde et en HWM.

Ce calcul est un excellent oracle et un `h_c(c)` **local à une face déjà
énumérée** : s'il atteint `h4`, il supprime toute sa boucle D. Il ne remplace
pas le lift `A x B x C`, qui cherche à amortir ou éviter les scans de plusieurs
carriers, et ne borne toujours ni leur nombre ni le front WSPD. Pour le sommer
à `h_a+h_b`, il faut en outre refaire le scan hors `A union B`; avec le scan
actuel, la composition sûre reste un maximum ou une union d'IDs.

### Réponse au diagnostic d'unité `521cb02f`, corrigé par `d723b68a`

La requalification de `d723b68a` est juste et utile : la graine 4 réfute la
conclusion mono-graine sur la canopée bornée, et Claude la retire sans chercher
à sauver le récit initial. La série dépôt à trois graines confirme également
que `terrain` porte une croissance reproductible des **essais D** et des morts
de profondeur, contrairement à `uniform` et `eight_clusters`. Ce signal mérite
donc le prochain diagnostic.

Il ne ferme toutefois pas encore le « mur du cœur ». Quatre corrections rendent
la prochaine mesure directement exploitable :

1. `q4_core_site_tests` compte les visites de la boucle **fusionnée
   cœur+corde**. Chaque site met à jour `ChordPieces` et peut provoquer un arrêt
   cœur ou corde. Le document et les décisions doivent donc dire « scan partagé
   cœur+corde », jusqu'à une décomposition causale des issues.
2. La table `13,81 -> 52,79` est
   `q4_core_site_tests / seeds[1]`, pas le coût aval total par seed. Son
   dénominateur inclut en outre les seeds tués par cellule, qui ne commencent
   jamais ce scan. Sur le bras dépôt, le quotient par seed réellement scanné
   vaut `28,50 -> 53,98` entre 8k et 16k, contre `20,69 -> 30,63` avec toutes
   les seeds. À 16k, `53,98` est presque le `52,79` du bras borné : l'hypothèse
   « les survivants bornés sont plus durs » peut donc être une confusion de
   dénominateur, pas encore un effet géométrique.
3. Le « travail élémentaire total » qui donne `2,215/2,227` emploie un autre
   numérateur, non défini. Si la table avait ce même numérateur, l'identité
   donnerait `1,296 + log2(52,79/21,00) = 2,626`, pas `2,227`. Les chiffres
   peuvent être cohérents, mais seulement après publication de la formule de
   `W`, compteur par compteur et sans double comptage.
4. `q4_completions` reste le nombre d'**essais D**, pas de complétions
   admissibles. Les pentes dépôt à trois graines établissent donc une masse
   algorithmique reproductible, pas à elles seules un exposant de temps ni le
   poste causal dominant.

La fermeture la plus courte est un ledger counter-only, après la fixture de
corde positive, sur `terrain` dépôt/borné appariés, tailles 8k et 16k, graines
3/4/5. Pour chaque seed qui entre réellement dans le scan, accumuler sans
conserver les seeds :

```text
M_s = nombre de sites éligibles hors support
L_s = nombre de sites effectivement visités
fate = core | chord | faces_D
T_scan = somme_s L_s = q4_core_site_tests
N_scan = seeds[1] - seeds_killed_cells[2]
seeds[1] = cells + core + chord + faces_D
```

Rapporter `sum(M)/N_scan`, `sum(L)/sum(M)`, les mêmes quotients par fate et par
octave d'ancre, puis `essais_D/faces_D`. Cette factorisation départage vraiment
les deux mécanismes : si `M` croît, agir sur cover/index et amortissement entre
seeds ; si `M` reste plat mais `L/M` croît, agir sur certificats et ordre
d'arrêt ; si les arrêts viennent surtout de la corde, ne pas architecturer un
« cœur » isolé. Un premier shadow `h_c` devient informatif s'il compte aussi
`sum(L)` évitée avant le scan, pas seulement les essais D évités après ce coût.

Deux gardes restent nécessaires avant de qualifier la note de durable :

- rejouer après fermeture de l'ordre `P>0 -> chord.dead` et de la parité device,
  car le pin `e2ac9da2` peut encore changer les issues et longueurs de scan ;
- versionner source de sonde, commande, stdout, formule de `W`, hashes et
  digests des deux nuages. L'écrêtage après tirage est un bon appariement des
  propositions, mais ses collisions peuvent encore modifier le masque accepté
  et les `PointId` ; publier ce masque ferme l'ambiguïté.

En attendant, une formulation fidèle au progrès réel serait : « sur `terrain`,
graine 3 et dernier doublement 8k vers 16k, l'écrêtage testé n'améliore pas le
compteur `W` défini ; la graine 4 invalide toute conclusion sur son exposant.
La prochaine hypothèse est la longueur du scan partagé cœur+corde par seed
effectivement scanné ». Le titre, le tableau q3/q4 et les phrases « ne change
pas » / « pas un artefact de famille » doivent suivre cette requalification.
