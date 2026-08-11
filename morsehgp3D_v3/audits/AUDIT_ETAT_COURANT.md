# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur et périmètre

`HEAD` à l'ouverture de cet audit :
`cbac109a09c2575cdf875b19de1570265bd5bf08`.

Le worktree est concurrent et non propre. Il ajoute notamment une sonde q2 et
des mises à jour documentaires; ces changements ne sont pas confondus avec le
commit. Le sidecar committé est réaudité dans
[`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md), avec ses empreintes exactes. Toute nouvelle
empreinte exige un nouveau rejeu avant réception.

## Verdict

Le contrat n'est pas rempli. La cible spécifiée principale est un p95
`warm_e2e < 100 ms` à 50 000 points et `K=10`; la seconde est la cible
secondaire et le jalon immédiat demandé. Aucun backend public exact n'est
qualifié.

Le verrou dominant est la source géométrique exacte. Les reçus G4 existants
n'ont formé aucun tuple : ils mesurent seulement top-t, dilations et comptes.
Le pipeline borné peut comparer un fold relatif à une table fournie; il ne
prouve pas que cette table contient toutes les activations requises.

Deux contrats restent strictement distincts :

1. Gamma/v2 exhaustif, avec facettes, cofaces, incidences silencieuses et
   applications verticales ;
2. le candidat `hgp_reduced_normalized_h0_v3`, limité aux composantes
   horizontales et unions de `PointId`, avec quotient certifié des blocs H0
   inertes.

Le second n'est encore ni spécifié comme produit ni reçu. Une omission licite
pour ce quotient ne peut jamais être présentée comme une source Gamma complète.

## Livraisons reçues et limites

- Le fast path principal des lots ex æquo a été reçu au commit `84ba459` sous
  la garde `q<=k+1`, avec lookups stricts pré-lot et fallback pour `q>k+1`.
  Cette réception est relative à une table complète fournie; elle ne crée pas
  sa propre preuve de fermeture.
- `prefix-all` reste un juge exact du fold relativement à la même
  `GeneratorTable`. Il ne certifie jamais la complétude géométrique de la
  table.
- La porte `k=1` compare les partitions strictes et fermées au single linkage
  porté par un EMST exact. Elle autorise l'étude d'une route EMST dédiée; elle
  ne qualifie pas encore un producteur G4 sous la seconde.
- Les commits `3c13cbd` et `4b9d9a1` reçoivent les mesures mass-only et leurs
  sorties brutes, pas une lane de production.
- Le commit `cbac109` ferme plusieurs défauts du sidecar `9483b1c`, mais ne le
  rend pas recevable : le reçu reste forgeable, l'unicité des boules exactes
  reste fausse, une entrée hostile déclenche UBSan et le digest de confiance
  reste incomplet. Son census `O(G*n)` en fait au mieux un oracle CPU borné
  après correction.

## Sidecar `cbac109` : corrections et blocages

Les corrections suivantes sont réelles : le fold reçoit le sidecar typé,
aucun carré de niveau n'est formé dans `i128`, le support minimal est
revérifié et le digest ne lit plus la structure `CriticalSphere` entière.

Les blocages d'exactitude restent :

1. le token vide est trivialement copiable par `std::bit_cast`; le
   constructeur public du reçu scelle alors un catalogue amputé avec ses
   digests et certifie toutes les fermetures ;
2. l'index trie les boules de même centre par indice, puis compare seulement
   les niveaux voisins. `[r1,r2,r1]` accepte donc deux handles identiques ;
3. `nx=INT128_MIN, den=1` atteint `-INT128_MIN` dans `sidecar_gcd`
   avant tout refus ;
4. le support minimal est valide, mais son tie-break canonique n'est pas
   reconstruit ;
5. les entiers sont pliés dans l'endianness native avec FNV-1a 64 bits, sans
   schéma ni framing contractuel. Le digest catalogue lie le support déclaré,
   mais le digest final ne lie pas séparément les preuves de suppression,
   `maximum_order` ou les fermetures.

Le pipeline hybride ne peut par ailleurs accepter que `n<=32`, car il exige
`smax>=n` tout en refusant `smax>32`. Il énumère deux fois le catalogue et
refuse le producteur parallèle. Ce comportement convient à un oracle borné,
pas à la route 50 k.

## Ce que la G4 a effectivement mesuré

Sur trois familles et deux pas, les temps count-only vont de 0,174 à
29,153 secondes. Après le prune d'axe :

| lane | masse minimale | masse maximale | admission |
| --- | ---: | ---: | --- |
| q2 | 465 371 500 | 2 862 879 000 | non admise |
| q3 | 14 667 530 000 | 131 762 100 000 | rouge |
| q4 | 330 437 400 000 | 9 968 861 000 000 | rouge |

Les réductions observées atteignent 136,3x sur R4 terrain et 584,3x sur R4
multiecho au pas 6. Aucun reçu ne porte 750x sur R3. q2 est seulement la lane
la moins rouge : sans octets, fill, census, fold et temps bout en bout, le mot
« admissible » serait faux.

Le pinceau q4 est également rouge. Dans une cellule q4 survivante de taille
`m_C`, même le schéma canonique qui prend les trois plus petits identifiants
doit considérer `C(m_C-1,3)` triples. Comme
`C(m_C,4)=m_C*C(m_C-1,3)/4`, les reçus au pas 6 imposent avant toute requête
plus de `2,74e9` triples sur `terrain`, `1,063e10` sur scanline simple et
`1,020e9` sur multi-écho. Ces bornes utilisent la dilation q4 et non la masse
R3 d'une autre lane.

## Lemme exact de cellule, avec sa vraie portée

Pour une cellule half-open `C`, utiliser sa fermeture dans les bornes. Pour un
point `x`, noter `l_C(x)` la distance carrée minimale à cette fermeture et
`u_C(x)` la distance carrée maximale. Pour la lane `q`, poser
`t_q=K+2-q`, choisir canoniquement les `t_q` plus petites valeurs `u_C`, poser
`R_q(C)` égal à leur maximum et
`A_q(C)={x : l_C(x)<=R_q(C)}`.

Pour une boule de support propre positif `q` dont le centre owner est dans
`C` :

- si `beta>R_q(C)`, les `t_q` témoins sont strictement intérieurs et
  `p+q>=K+2`; le théorème 4.2 rend le bloc inerte seulement pour le quotient
  H0 normalisé ;
- si `beta<=R_q(C)`, tout le saturé fermé, donc support, intérieur et coquille,
  appartient à `A_q(C)`.

L'égalité doit rester dans la seconde branche. Une séparation stricte de la
fermeture de `C` et de `conv(A_q(C))` exclut donc une activation non inerte de
la seconde branche. Elle ne prouve jamais qu'aucun support n'existe : la
contre-fixture entière de la réponse pont conserve précisément un support dans
la branche inerte.

Une subdivision est monotone : pour un enfant `C'`,
`A_q(C')` est inclus dans `A_q(C)`. Cela permet de réutiliser les listes, mais
ne fournit aucune borne de travail. Une cosphère massive peut conserver
`|A_q(C)|=Theta(n)` à toute profondeur et faire tester `Theta(n^q)` vues pour
une seule `BallKey`. Toute profondeur maximale exige un fallback exhaustif ou
un refus explicite; jamais un drop. La route cellule reste donc un diagnostic
branch-and-bound, pas encore le chemin industriel.

## Direction de source à tester

La sonde concurrente q2 partitionne toutes les paires non ordonnées par blocs
d'un arbre AABB médian. Dix témoins communs stricts certifient qu'un bloc
entier est H0-inerte pour q2; le ledger ferme
`pruned+microtile=C(n,2)`. Ce principe est exact, mais le différentiel
bruteforce compare seulement des comptes compensables et aucun CTest ne porte
la cible. À `n=2400`, les runs locaux visitent 17,7 à 60,5 millions de nœuds
témoins selon la famille; un run terrain `n=50 000, leaf=64` n'a pas terminé
dans sa fenêtre locale mono-thread de 60 s. Ces diagnostics ne sont ni G4 ni
`warm_e2e`. Le prune ne fournit pas la source d'ancres q3/q4.

L'ancien prototype `center-cover` a dépassé 600 secondes à 50 k sans JSON. Il
est rejeté comme implémentation. La grille de cellules et son plan séparateur
restent des diagnostics mass-only.

### Ordre un

Router `k=1` vers un EMST/Boruvka exact, avec distances u16 exactes, lots
d'égalité atomiques et différentiel par partitions à chaque niveau. Cette
route évite le catalogue Morse et toute mosaïque d'ordre supérieur pour
l'ordre un.

### Supports q2

Une paire `(x,y)` non inerte possède au plus `K-1=9` points dans l'intérieur
de sa boule diamétrale. Le prédicat exact est
`(z-x) dot (z-y)<0`. Un produit dual-tree de boîtes peut supprimer un produit
de paires seulement lorsqu'il certifie dix témoins distincts strictement
intérieurs pour toutes ses paires. Les feuilles restantes calculent le compte
exact, la coquille, l'owner et la `BallKey`, une seule fois par paire non
ordonnée.

Cette lane ne construit ni cellules de centres, ni tableau global de paires.
Elle reste output-sensitive avec un pire cas quadratique : son premier jalon
est donc une sonde count-only publiant produits visités, produits prunés,
paires terminales, témoins, doublons, octets et high-water. Aucun chrono de
microkernel ne remplace ces masses.

Ce prune est strictement q2. Une paire dont la boule diamétrale contient dix
témoins peut rester le diamètre d'un support q3 ou q4 dont la sphère décalée
ne contient aucun de ces témoins. Retirer les paires prunées de la source
d'ancres supérieures serait donc faux.

### Supports q3 et q4

Choisir comme ancre canonique la plus petite paire parmi les diamètres du
support positif canonique, après census et RLE de la boule. Si `D` est sa
longueur, le centre d'un support q3 positif appartient
au disque du plan médiateur défini par `h^2<=D^2/12`; pour q4, le théorème de
Jung en dimension trois donne `h^2<=D^2/8`. Dans ce plan, chaque autre point
définit une droite d'égalité et un demi-plan d'intérieur. Les q4 pertinents
sont des sommets de faible profondeur. En position générale, les bornes
d'arrangements shallow peuvent retirer le carré local si le constructeur bâtit
directement ces niveaux. Elles ne couvrent pas encore les parallèles,
concurrences, ex æquo et grandes coquilles; former d'abord toutes les
`C(m_e,2)` intersections est un NO-GO.

Cette piste n'est pas encore une architecture reçue. Il manque une source
sparse complète des ancres, le constructeur exact des dégénérescences et une
admission globale mesurée.

## Ordre des prochaines portes

1. Tuer la forge fraîche, `[r1,r2,r1]` et `INT128_MIN`; reconstruire le
   tie-break du support, remplacer le digest de confiance et recevoir le
   sidecar comme oracle borné `n<=32`, jamais comme source 50 k.
2. Recevoir la sémantique exacte du prune de cellule : « branche
   `beta<=R` vide », jamais « aucun support », avec contact et
   contre-fixture haute.
3. Recevoir la sonde q2 avec un rejeu non compensable de chaque bloc pruné,
   puis convertir microtuiles, visites et octets en budget mesuré.
4. Prouver une source sparse complète des ancres q3/q4 et recevoir son
   constructeur shallow sur des oracles bornés; abandonner le triple-pencil
   global et tout terme quadratique construit avant prune.
5. Recevoir `BallActivation`, le resolver silencieux et le quotient H0 contre
   Gamma exhaustif à petit `n`, puis seulement mesurer source+fold+payload.
6. Porter sur G4 uniquement les routes admises et publier `warm_e2e` complet.

GCP non utilisé.
