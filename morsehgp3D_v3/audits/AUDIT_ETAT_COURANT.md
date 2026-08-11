# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur et périmètre

`HEAD` à l'ouverture de cet audit :
`232470cbaf2449e5e68c92f2c42c532c4df20458`.

Le worktree est concurrent et non propre. Les changements locaux de source et
de CMake ne sont ni reçus ni qualifiés par ce document. Le dernier snapshot de
code sidecar audité bit à bit est le commit
`9483b1cd5ff691bc53f51eb2776aaba77b011e43`; son verdict détaillé est
[`AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md`](AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md).
Toute nouvelle empreinte exige un nouveau rejeu avant réception.

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
- Le sidecar du commit `9483b1c` est une livraison déclarée, non reçue. Il
  peut devenir un oracle CPU borné après correction; son census `O(G*n)` ne
  doit pas devenir l'architecture produit.

## Réfutation du sidecar v0

Les défauts suivants sont bloquants sur `9483b1c` :

1. le constructeur public du reçu permet de sceller une table amputée avec
   ses propres digests ;
2. le pipeline redérive toujours la prétention depuis `smax`, `n` et le statut
   d'énumération, puis détruit le sidecar avant le fold ;
3. la clé de rayon carre des entiers pouvant atteindre environ 90 bits dans
   `i128`, donc peut déborder sur des entrées u16 valides ;
4. `q_min` et le support propre positif ne sont pas reconstruits
   indépendamment ;
5. le FNV de l'image mémoire brute n'est ni canonique ni une preuve exacte ;
6. le rescan nuage--générateur et la miniboule de chaque saturé sont
   incompatibles avec la cible 50 k.

Une réparation locale en worktree n'est reçue qu'après compilation, fixtures
hostiles, mutants et nouvel audit de ses empreintes. Il faut notamment tuer
`fresh_receipt_on_amputated_catalogue`, les supports redondants ou dépendants,
le champ `Sphere.support` incohérent et une clé u16 qui dépasse 128 bits.

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

La source candidate commune aux lanes q2/q3/q4 est un self-join canonique du
LBVH. Il partitionne toutes les paires non ordonnées par blocs, puis applique
un `center-cover` fail-open avec les seuils témoins `10/9/8`. Les ancres
résiduelles seulement alimentent les lanes terminales. Le ledger ferme
`pruned + microtile = C(n,2)`; une paire n'est jamais omise par une heuristique.

L'ancien prototype `center-cover` a dépassé 600 secondes à 50 k sans JSON. Il
est rejeté comme implémentation. La proposition courante exige une nouvelle
sonde par blocs, sans scan ancre--nuage, et publie le nombre d'ancres `a`,
`M=sum_e m_e`, les files, ambiguïtés, octets et visites. Ces masses, et non la
seule preuve locale, décideront la route.

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

### Supports q3 et q4

Choisir comme ancre canonique la plus petite paire parmi les diamètres du
support positif canonique, après census et RLE de la boule. Si `D` est sa
longueur, le centre d'un support q3 positif appartient
au disque du plan médiateur défini par `h^2<=D^2/12`; pour q4, le théorème de
Jung en dimension trois donne `h^2<=D^2/8`. Dans ce plan, chaque autre point
définit une droite d'égalité et un demi-plan d'intérieur. Les q4 pertinents
sont des sommets de profondeur stricte au plus `7-c_e`, où `c_e` compte les
points intérieurs sur tout le disque. Pour `m_e` cordes, leur nombre vérifie
`Z_e<=m_e*(8-c_e)<=8*m_e`. Le constructeur doit bâtir ces niveaux directement;
former d'abord toutes les `C(m_e,2)` intersections est un NO-GO.

Cette piste n'est pas encore une architecture reçue. Le self-join est complet,
mais sa parcimonie résiduelle n'est pas prouvée. Il manque aussi le constructeur
exact des parallèles, concurrences et ex æquo, ainsi qu'une admission mesurée
de `a`, `M` et `sum_e Z_e`.

## Ordre des prochaines portes

1. Stabiliser puis réauditer le sidecar comme oracle borné; le fold doit
   consommer le type validé jusqu'au bout.
2. Recevoir la sémantique exacte du prune de cellule : « branche
   `beta<=R` vide », jamais « aucun support », avec contact et
   contre-fixture haute.
3. Construire la sonde commune `self-join -> center-cover` et fermer l'identité
   de toutes les paires; refuser la route si `source-cover + cordes > 400 ms`
   chaud sur G4 ou si la majorité de la masse atteint les microtuiles.
4. Recevoir les lanes q2 et cordes shallow q3/q4 sur des oracles exhaustifs
   bornés; abandonner le triple-pencil global et tout terme `sum_e m_e^2`.
5. Recevoir `BallActivation`, le resolver silencieux et le quotient H0 contre
   Gamma exhaustif à petit `n`, puis seulement mesurer source+fold+payload.
6. Porter sur G4 uniquement les routes admises et publier `warm_e2e` complet.

GCP non utilisé.
