# Note Claude — le producteur par ancre maximale existe, il est exact

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Cette note rapporte une implémentation et des mesures locales. Elle ne reçoit
aucune porte, ne qualifie aucun SLO et ne modifie aucun statut public. Elle
demande un contre-audit.

## 1. Ce qui a été construit

`prototype/anchor_envelope.hpp` et `prototype/anchor_source.cpp`, cible
`mhgp3v_anchor_source`. C'est la route des Théorèmes 4 et 5 de
[`AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md`](AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md),
spécifiée dans
[`NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md`](NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md),
avec **une différence d'ordonnance assumée** : aucun front n'est matérialisé.
Le certificat de front, la décision de lane et l'extension partagent la même
descente et la même liste de voisins.

La lane « cellules de centres » n'est plus poursuivie comme chemin produit.

## 2. Émission exacte-une-fois, sans RLE ni lift

Un support n'est publié que depuis son **arête maximale canonique** — la plus
longue, et à égalité le plus petit couple `(min PointId, max PointId)`. Comme
tout support propre positif possède au moins une arête de longueur maximale, la
couverture est totale, et comme l'arête canonique est unique, l'émission l'est
aussi. Le reçu vérifie l'identité `occurrences = cles_uniques` et refuse en
code 3 si un doublon apparaît.

Il n'y a donc plus ni `SupportKey` RLE, ni lift, ni owner de cellule, ni point
location : les `39,242` occurrences par support du point gelé n'ont pas
d'équivalent ici, par construction et non par optimisation.

## 3. Ce qui a été rendu entier

Trois quantités que la note de solution laissait en flottant sont désormais
entières :

1. **Les trois lanes sont trois seuils sur une seule marge.** Avec
   `u = 2z-a-b` et `g = D^2 - u\cdot u`, les témoins universels du Lemme B
   s'écrivent `g>0` (q2), `3g>2D^2` (q3), `15g>11D^2` (q4). Une seule
   soustraction donne les trois lanes et le census q2 exact, extra-shell
   comprise par l'égalité `4s = 16D^2`.
2. **L'amplitude de la marge sur le disque de Jung est encadrée par une racine
   entière.** Avec `Q = (u\cdot u)D^2-(u\cdot d)^2` et `s = isqrt(2Q)+1`, les
   bornes `Llow = g-s` et `Uhigh = g+s` encadrent `L` et `U^{*}`; le filtre
   `Uhigh < theta` est donc exact et fail-open, sans aucune comparaison de
   racines.
3. **Le certificat de front est entier.** Pour un noeud de boîte `[lo,hi]`, de
   diagonale `ext` et de distance minimale `Dmin` à l'ancre, le milieu de toute
   paire du produit est à au plus `ext/4` du centre `z_0=(a+c_B)/2`. Les trois
   rayons `R_q = Dmin/c_q - ext/4` sont minorés par
   `(\lfloor k_q\,\mathrm{isqrt}(Dmin^2)/10^4\rfloor - \mathrm{isqrt}(ext^2)-1)/4`
   avec `k_2=20000`, `k_3=11547<4\sqrt{12}^{-1}10^4` et
   `k_4=10327<4\sqrt{15}^{-1}10^4`. Le produit n'est fermé que si les **trois**
   lanes atteignent leurs seuils `10/9/8` séparément.

Le point 3 corrige une perte d'un facteur deux sur le rayon : la première
version exigeait dix témoins dans `Dmin/(2\sqrt{15})`, ce qui repoussait la
coupure à environ `10,4\rho^{-1/3}` au lieu de `4,8\rho^{-1/3}`, soit huit fois
trop de candidats.

## 4. Trois économies supplémentaires, toutes exactes

- **`always_inside` et `always_outside`.** Un site de `Llow>0` est intérieur à
  toute boule du disque : il est compté sans être testé. Un site de `Uhigh<0`
  est extérieur à toute boule : il n'est jamais chargé. Seuls les sites dont la
  marge change de signe subissent le prédicat exact. En prime, `always_inside`
  au-delà du budget tue l'ancre avant toute lentille.
- **Le carrier doit croiser le disque.** Un carrier est exactement sur la
  sphère, donc `Llow \leq 0 \leq Uhigh`. Cette condition retire de la lentille
  les sites qui ne peuvent être que dedans ou que dehors.
- **Le Théorème 5 restreint la boucle q4.** Un q4 positif d'arête maximale
  `(a,b)` possède au moins une face positive parmi `abx` et `aby`; la boucle
  saute donc les paires dont aucune des deux faces n'est aiguë.

## 5. Ce qui est mesuré

**Exactitude.** Le mode `--verify` rejoue la même extension sur **toutes** les
paires, sans aucun certificat, et compare les deux ensembles de supports clé
par clé et `p` par `p`. Accord exact sur `uniform`, `terrain`,
`scanline_single_pass` et `scanline_overlap_multiecho`, plusieurs graines,
`n` de 60 à 120. Ce différentiel valide les certificats de front, de lane, de
disque et d'enveloppe d'un seul coup : n'importe lequel qui couperait à tort
ferait chuter le compte.

**Travail.** Le compteur décisif est `candidate_pairs / n`, c'est-à-dire le
nombre de paires **non ordonnées** que le certificat de front laisse passer par
point. Sur `uniform` il vaut `227` à `n=500`, `351` à `n=1 000` et `465` à
`n=2 000`.

**Claim retiré.** Cette note comparait ces valeurs à `(4\pi/3)(4,8)^3\simeq463`.
C'est faux : cette quantité est un degré **dirigé**, et la baseline pointwise
est sa moitié, environ `231,6`; la dérivation exacte du front q4 par boule de
milieu donne `232,379n` et la coalescence des trois lanes `233,807n`. Les
valeurs observées sont donc environ **deux fois** la baseline, pas égales à
elle. Le certificat de nœud laisse encore passer un facteur deux, attribuable à
la granularité des feuilles — il ne peut pas couper plus fin qu'une boîte. Ce
facteur doit être mesuré en fonction de `--leaf`, pas expliqué. La correction
est due au contre-audit du 12 août.

**Sortie.** À `n=4 000`, `1 459 968` supports, soit `365` par point, à comparer
à la baseline bulk Poisson d'environ `440`. L'écart est encore l'effet de bord.

Aucun temps n'est publié comme mesure : la machine de développement a deux
cœurs et les campagnes se chevauchent.

## 6. Portes raccordées et mutants

Le sujet porte désormais des portes CMake. Huit mutants cassent chacun **une**
décision exacte ; le sujet muté est comparé à la référence exhaustive **non**
mutée, donc toute différence tue le mutant en code 4. Un mutant sans juge est
refusé en code 2.

| mutant | ce qu'il casse | ce qui le tue |
| --- | --- | --- |
| `theta-no-fail-open` | filtre d'enveloppe sur `Llow` au lieu de `Uhigh` | `uniform,n=500` |
| `owner-min-edge` | ancre sur l'arête minimale | `uniform,n=500` |
| `lens-strict` | lentille en `<` au lieu de `<=` | `uniform,n=500` |
| `census-no-always-inside` | census ignorant les toujours-intérieurs | `uniform,n=500` |
| `positivity-loose` | accepte un centre sur une face | `uniform,n=500` |
| `owner-no-tiebreak` | toute arête maximale se croit owner | fixture `ties` |
| `front-no-ext` | boule témoin sans la soustraction `ext/4` | `terrain` et `scanline_overlap_multiecho` |
| `front-q4-only` | ferme un produit sur la seule lane q4 | fixture `q4only` |

Deux enseignements de mutants sont durables :

- `front-no-ext` **survit sur `uniform`**. Le terme `ext/4` ne se voit que sur
  des boîtes allongées : les familles anisotropes sont donc la porte, pas un
  choix de confort. Une campagne `uniform` seule aurait laissé passer une
  boule témoin trop grande.
- `front-q4-only` exige une fixture gravée. La fixture `q4only` place huit
  points au milieu exact d'une longue paire : la lane q4 y meurt, alors que le
  support q2 reste pertinent avec `p=8` puisque son seuil est dix. Fermer un
  produit sur la seule lane q4 le détruit. Aucune famille aléatoire testée ne
  produit ce motif.

La fixture `ties` est le tétraèdre régulier `(0,0,0),(2,2,0),(2,0,2),(0,2,2)`,
de côté carré huit, de circumcentre exact `(1,1,1)` : ses six arêtes sont
maximales et ses quatre faces équilatérales. Sans règle canonique, il est émis
plusieurs fois; le reçu le voit en `doublons=25`.

## 7. Portage device

`prototype/anchor_pipeline.hpp` contient l'intégralité du travail d'un point
d'ancre, sans STL, sans allocation et sans récursion, en `MHGP_HD`.
`prototype/anchor_source_kernel.cu` n'implémente **aucune** géométrie : il
appelle `run_anchor_point`, exactement la fonction que l'hôte compile. La
parité hôte/device n'est donc pas un accord à espérer entre deux
implémentations, c'est la même fonction sur deux matériels; le différentiel
sert à détecter une divergence de compilateur, d'arrondi entier ou de matériel.

La chaîne de garde est : exhaustif ↔ moteur de référence ↔ moteur `pipeline`
(portes CMake locales), puis `pipeline` hôte ↔ `pipeline` device (porte
`mhgp3v_anchor_device_parite`, active seulement sous `MHGP3V_ENABLE_CUDA`).
La porte device compare la liste triée des clés **avec** leur census
`(p, extra)` **et** les vingt-cinq compteurs de travail, qui sont
déterministes par point et donc indépendants du partitionnement.

Toutes les capacités sont fixes et préflightées; un dépassement lève un
drapeau et fait refuser la campagne en code 3, il ne tronque jamais une sortie.

## 8. Ce que cette note ne dit pas

Elle ne publie aucun `warm_e2e`, ne construit ni `BallActivation`, ni gateways,
ni resolver, ni fold H0, ni payload. Aucune mesure device n'existe encore : le
noyau est écrit et contrôlé syntaxiquement, il n'a jamais été exécuté. Le
domaine dégénéré n'est pas fermé : les positions dupliquées sont refusées en
code 2, l'extra-shell est comptée mais pas routée, et le cas terminal `k=n`
n'est pas produit. Il n'existe toujours aucun juge **indépendant** : le
différentiel exhaustif partage les primitives du sujet.

## 9. Objectif d'échelle inscrit

L'utilisateur a fixé l'objectif suivant, **après** le contrat `50 000/1 s` :
traiter sur la même G4 des nuages de **dizaines de millions de points**. Deux
invariants sautent à cette taille et doivent être anticipés :

- l'index dense `u16` et la `SupportKey` de 64 bits (quatre `u16`) sont cassés
  dès `n > 65 535`; il faut `DensePointIndex:u32` et une clé de 128 bits, ou un
  fingerprint routeur suivi d'une comparaison exacte en bucket;
- Source S ne peut plus être matérialisée : à `10^7` points la baseline donne
  environ `4,4\times10^{9}` supports. Le fold H0 doit donc consommer les
  supports **en flux**, par lots de niveau, sans catalogue global.

Le producteur par ancre est compatible avec ces deux contraintes : son travail
est local par point d'ancre et sa sortie est exacte-une-fois, donc streamable
sans déduplication globale. C'est un argument de conception, pas un résultat.

## 10. Questions à l'auditeur

1. Le certificat de front de la section 3.3 est-il correct sur le point
   suivant : pour une requête **mono-arbre** (l'ancre est un point, pas un
   nœud), la borne `|m - z_0| \leq ext/4` est-elle bien exacte, `ext` étant la
   diagonale de la boîte et `z_0=(a+c_B)/2` avec `c_B` le centre de la boîte ?
2. Le Théorème 5 est-il utilisable tel que je l'emploie : « un q4 positif
   d'arête maximale `(a,b)` a au moins une face positive parmi `abx` et `aby` »
   suffit-il à restreindre la boucle q4 aux paires dont au moins un membre
   donne un triangle `ab\cdot` aigu ? La preuve de l'audit du verrou porte sur
   les projections dans le plan médiateur; je voudrais qu'elle soit confirmée
   sans hypothèse de position générale.
3. Le filtre `theta` est calculé sur le disque de Jung **q4**, donc sur le plus
   grand des deux disques, pour servir q3 et q4 avec une seule enveloppe. Cette
   mutualisation est-elle sans perte d'exactitude ?
4. Le census `q2` est lu directement sur `g` pendant la sonde de lane, avec
   `g=0` comme extra-shell. Y a-t-il un cas `u16` où un membre du shell
   diamétral n'est pas capturé par cette égalité ?
5. Quelles portes l'auditeur exige-t-il en priorité sur ce sujet : fixtures
   gravées, mutants (« pas de tie-break d'arête maximale », « arête minimale »,
   « filtre theta sans fail-open », « certificat de front sans la soustraction
   `ext/4` »), planchers, ou juge indépendant d'abord ?

GCP non utilisé pour cette note.
