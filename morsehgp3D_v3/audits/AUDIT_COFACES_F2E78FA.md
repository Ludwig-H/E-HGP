# Réfutation constructive du fold cofaces à support canonique

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_and_bounded_oracles`,
`profile=quantized_u16_input_only`, `mode=audit_independant`, aucun statut
public. Note auditée :
[`NOTE_CLAUDE_REPONSE_MASSE_COFACES_20260810.md`](NOTE_CLAUDE_REPONSE_MASSE_COFACES_20260810.md),
committée à `f2e78fadf1fa8012f2d11f35dd76392ec45683a5`.

## Verdict

La réduction `M--Sat(S)--N` est correcte lorsque toutes les `k`-faces porteuses
sont disponibles. Le quatrième fold proposé n'est en revanche pas exact s'il
n'énumère que les faces `S` satisfaisant `|S intersection U|>=q-1` pour **un
seul support canonique** `U` de la boule. Avec plusieurs supports minimaux, une
composante stricte peut n'atteindre cette frontière pour aucun échange strict,
tout en étant fusionnée au niveau de la boule par des supports alternatifs.

Le cas `q<k` n'est donc pas une simple dette de borne `O(q+arité)` : la famille
de candidats écrite par Claude est incomplète. Le certificat `face-owner` de
[`REPONSE_CLAUDE_MASSE_JOIN_50K_20260810.md`](REPONSE_CLAUDE_MASSE_JOIN_50K_20260810.md)
reste la vérité sûre; sa recherche demand-driven peut servir de chemin candidat
si ses coupures sont certifiées.

## Fixture u16 exacte

Une première fixture minimale ne demande que six points. Posons :

| ID | point |
| ---: | --- |
| 0 | `(11025,6050,5525)` |
| 1 | `(6885,10880,5525)` |
| 2 | `(1573,9386,5525)` |
| 3 | `(3185,520,5525)` |
| 4 | `(8330,765,5525)` |
| 5 | `(5525,5525,5525)` |

Les points 0 à 4 sont sur la boule `B` de centre `(5525,5525,5525)` et de
rayon carré `30 525 625`; le point 5 est son centre. L'oracle rationnel choisit
`U={0,1,3}`, `q=3`. À `k=4`, les cinq faces strictes
`0125,0145,0345,1235,2345` sont cinq composantes isolées : toute coface de
taille cinq qui les relie a pour miniboule `B`. Le filtre canonique
`|S intersection U|>=2` reçoit les quatre premières mais manque `2345`, qui ne
rencontre `U` qu'en `3`. Sa miniboule est strictement plus petite, portée par
`{2,4}`, de rayon carré `59 989 345 / 2`. Le lot de `B` doit donc fusionner cinq
racines et le candidat n'en voit que quatre.

La fixture suivante montre que le défaut persiste avec davantage de supports
et qu'un simple changement de support canonique ne le répare pas.

Prenons le centre `c=(2,2,2)`, le rayon carré `5`, et les dix points suivants.

| ID | point |
| ---: | --- |
| 0 | `(2,4,3)` |
| 1 | `(1,0,2)` |
| 2 | `(1,2,0)` |
| 3 | `(2,3,4)` |
| 4 | `(0,2,1)` |
| 5 | `(3,0,2)` |
| 6 | `(3,2,0)` |
| 7 | `(4,1,2)` |
| 8 | `(0,1,2)` |
| 9 | `(4,2,1)` |

Chaque point est exactement sur la sphère `B=(c,r^2=5)`. Sa saturation est le
set entier `M={0,...,9}` et son support minimal canonique est
`U={0,1,2,5}`, de cardinal `q=4`.

À l'ordre `k=6`, le graphe strict interne des six-faces de `M` contient 44
faces et 17 composantes : une composante de 28 faces et 16 singletons. Les 17
faces strictes qui satisfont `|S intersection U|>=3` ne touchent que huit de
ces composantes. Neuf composantes sont donc invisibles au parcours proposé.

Une composante manquée est le singleton
`F={0,2,3,4,6,8}`. En coordonnées relatives au centre, le vecteur
`v=(-3,5,-2)` donne sur ses six points les produits strictement positifs
`8,7,1,8,1,1`. Ils appartiennent à un même demi-espace ouvert : leur
miniboule est strictement plus petite que `B`.

Pourtant chaque ajout possible d'un point extérieur à `F` forme une coface de
niveau exactement `B` :

| point ajouté | support de `B` contenu dans la coface |
| ---: | --- |
| 1 | `{0,1,3,6}` |
| 5 | `{0,2,3,5}` |
| 7 | `{0,2,7,8}` |
| 9 | `{0,3,8,9}` |

Ainsi `F` est isolée à la coupe stricte, puis doit être attachée au lot fermé
de `B`. Elle ne contient que deux éléments du support canonique `U`; aucun des
candidats `|S intersection U|>=3` ne représente sa composante. Le fold proposé
la manque.

La réfutation ne dépend pas d'un mauvais choix isolé de support : la boule
possède 44 supports minimaux possibles et chacun en laisse manquer entre sept
et neuf parmi les 17 composantes strictes.

## Fixture minimale avec `q=k`

Le défaut n'est pas limité au régime `q<k`. Il atteint aussi le cas `q=k`,
que la proposition initiale annonçait comme fermé. Prenons encore le centre
`c=(2,2,2)` et le rayon carré `5`, avec seulement les six points suivants.

| ID | point |
| ---: | --- |
| 0 | `(1,2,4)` |
| 1 | `(2,1,0)` |
| 2 | `(2,3,0)` |
| 3 | `(4,1,2)` |
| 4 | `(2,4,1)` |
| 5 | `(0,2,3)` |

La boule `B` de ces six points a `q=4`; l'oracle choisit le support canonique
`U={0,1,2,3}`. À l'ordre `k=4`, les dix quatre-faces strictes forment six
composantes, de tailles `5,1,1,1,1,1`. Les six candidates qui satisfont
`|S intersection U|>=3` n'en touchent que cinq.

La composante manquée est le singleton `F={0,3,4,5}`. Pour le vecteur
`h=(2,3,5)`, les produits avec les directions `x-c` de ses quatre points sont
`8,1,1,1`, tous strictement positifs; la miniboule de `F` est donc strictement
plus petite que `B`. Pourtant `F` complétée par le point 1 contient le support
alternatif `{0,1,3,4}`, et `F` complétée par le point 2 contient le support
alternatif `{0,2,3,5}`. Ces deux cofaces ont exactement la miniboule `B` et
doivent rattacher `F` au lot fermé.

Les supports minimaux exacts de `B` dans cette coquille sont
`0123,0134,0235,1345,2345`. La perte vient donc bien de la coexistence des
supports et non d'un cas marginal propre à `q<k`. Cette fixture plus petite est
la porte prioritaire du dispatcher : le certificat `principal` doit échouer,
le fallback doit toucher les six composantes, et le mutant qui force le filtre
canonique doit n'en toucher que cinq.

## Deux corrections de formulation

1. Une coface `C` dont la miniboule est `B` contient un support minimal
   **entier** de `B`. Ce sont certaines de ses facettes strictes, obtenues en
   retirant un point du support, qui n'en contiennent que `q-1`. Écrire que la
   coface elle-même est caractérisée par `q-1` points confond les deux objets.
2. Le tuple brut `Sphere{base,num,den}` n'est pas une clé canonique de boule.
   Deux supports de la même sphère changent `base`, peuvent changer l'échelle
   de `num/den`, et la miniboule d'une face peut choisir un support différent
   de celui retenu lors de la canonicalisation de la coquille. Le lemme compact
   reçu dit que `(niveau exact,saturé)` identifie la boule; il ne rend pas la
   représentation relative brute indépendante du support. Un lookup exige une
   clé géométrique normalisée du centre rationnel et du rayon, avec égalité
   exacte de collision, ou un handle de saturé obtenu par un census certifié.

## Ce qui reste constructif

- Énumérer **toutes** les `k`-faces puis appliquer `face-owner` est exact et
  donne déjà un gain `23,5x` à `n=200`; c'est le prochain oracle borné.
- Énumérer tous les supports alternatifs de chaque boule pourrait réparer le
  protocole cofaces, mais réintroduit une masse combinatoire que le catalogue
  canonique ne publie pas.
- La version produit peut parcourir à la demande le trie des combinaisons de
  `M`, intersecter progressivement les postings et couper seulement lorsque la
  liste courante ne contient plus aucune racine extérieure nouvelle. Cette
  coupure possède un certificat d'absence; elle ne promet pas une borne
  `O(q+arité)` au pire.
- La garde suggérée « refuser si une composante stricte n'a pas été atteinte »
  n'est pas calculable par le sujet sans reconstruire précisément les
  composantes qu'il risque d'omettre. Elle est une obligation de juge borné,
  pas une garde produit autonome.

Un fast path exact subsiste sous un certificat plus fort. Supposons qu'un
support minimal `U` de `B` soit **principal** : pour tout sous-ensemble
`A` de `M`, la miniboule de `A` vaut `B` si et seulement si `U` est inclus dans
`A`. La condition `shell(B)=U` suffit. Pour `q<=k` et `|M|>=k+1`, choisissons
un ensemble fixe `T` de `k-q+1` points dans `M` privé de `U`, puis, pour chaque
`u` de `U`, la face `S_u=(U privé de u) union T`.

Toute face stricte `F` omet au moins un `u`. Dans le graphe de Johnson sur les
`k`-sous-ensembles de `M` privé de ce `u`, `F` est reliée à `S_u`; par le
certificat principal, toutes les faces et cofaces de ce chemin sont strictes.
Les racines distinctes des `q` faces `S_u` sont donc exactement toutes les
attaches de `B`. Le cas `|M|=k` est une naissance; le cas `q=k+1` conserve ses
`k+1` facettes de support. Cette séparation donne une route hybride défendable :
`O(q)` lookups sous certificat principal, fallback `face-owner` certifié dans
les coquilles à supports alternatifs.

## Porte permanente conseillée

Graver cette coquille avec l'oracle exact suivant : vérifier la sphère et ses
supports, construire les composantes strictes des six-faces, exiger `17`, puis
comparer les racines touchées par `face-owner` et par le parcours à support
canonique. Un mutant « garder seulement `|S intersection U|>=q-1` » doit perdre
exactement neuf composantes selon l'oracle. Une seconde porte doit permuter les
points afin de changer le support canonique sans sauver le mutant.

GCP non utilisé.
