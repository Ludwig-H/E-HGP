# q2 de bout en bout — et le masque endpoint relationnel est un théorème

Date : 16 août 2026 UTC.
Dossier : `morsehgp3D_v3/`.
Étape 3 de l'ordre de `AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032` § 9,
séquence `9739e3c` § 9.D. Suite de `NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md`.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=diagnostic_counter_only
public_status=not_claimed
```

Reçus **développeur**, aucun workflow GitHub attaché.

---

## 1. Ce qui tourne

`prototype/q2_pairframe_probe.cpp`, `23` portes `mhgp3v_q2_*`. Géométrie
réelle : nuage u16, ordre de Morton, arbre binaire à boîtes **serrées**,
partition exacte des paires, et le `CoreDepthLedger` de `pair_frame.hpp` **sans
une ligne de modification**. Le juge est une force brute `O(n^3)` par paire.

`W_2(a,b)` est la boule diamétrale ouverte, et avec
`Phi(a,b,z) = (a-z).(b-z)` on a `z` dans `W_2` si et seulement si `Phi < 0`. Les
extrema de `Phi` sur `A x B x C` sont ceux de `acute_owner_gateway.hpp` :
séparables par axe, exacts, `O(1)`.

```text
span C ALL   <=>  Phi_max < 0
span C NONE  <=>  Phi_min >= 0
```

Quatre familles à `n=120`, `h_2 = 10`, `cap=8`, `cap_rect=16` :

| famille | partition | `pruned` | `core_clear` | mortes | vivantes | `manquantes` | `fausses` |
|---|---|---|---|---|---|---|---|
| `uniform` | `7140 = C(120,2)` | `32` | `5` | `4546` | `2594` | `0` | `0` |
| `terrain` | `7140` | `166` | `15` | `5300` | `1840` | `0` | `0` |
| `eight_clusters` | `7140` | `46` | `3` | `5054` | `2086` | `0` | `0` |
| `scanline_single_pass` | `7140` | `215` | `9` | `5249` | `1891` | `0` | `0` |
| `two_lines` (`n=60`) | `1770` | `0` | `28` | `1225` | `545` | `0` | `0` |

`indecises=0` partout : aucune paire ne sort sans décision. `manquantes` est une
arête gardée à tort, `fausses` une arête **tuée** à tort — donc un support
perdu.

---

## 2. Le résultat : le masque endpoint relationnel n'est pas un mécanisme

C'était la question ouverte de l'étape précédente, non testable sur le modèle
abstrait faute d'analogue à « `z` appartient à `A` ».

> **Théorème.** Si les intervalles de Morton de `C` et de `A` se recouvrent, le
> span `C` n'est **jamais** classé `ALL`.
>
> **Preuve.** Soit `p` dans `A inter C`. Alors `p` appartient à la boîte de `A`
> comme à celle de `C`, donc le triplet `(a = p, b, z = p)` appartient au
> produit des trois boîtes et donne `Phi = (p-p).(b-p) = 0`. Donc
> `Phi_max >= 0`, et le test `Phi_max < 0` échoue. Symétrique pour `B`.

Autrement dit : **la stricte de `Phi_max < 0` porte déjà tout le masque**. Il n'y
a pas de mécanisme séparé à écrire pour q2 — il y a une inégalité à ne pas
relâcher.

L'invariant est **armé**, pas seulement observé : `mhgp3v_q2_masque_endpoint_*`
balaie tous les rectangles contre tous les nœuds de l'arbre, `11 368` spans
endpoint, `relation_all=0`, sur trois familles. Le mutant `credit-large`
(`Phi_max <= 0`) le tue immédiatement — et tue aussi le juge d'identités, parce
que créditer `z = a` comme témoin de `(a,b)` gonfle le compte et tue des arêtes
vivantes.

**Ce que le masque exige encore**, en revanche, c'est la **conservation**. Un
span endpoint n'est pas créditable, mais le jeter perd les témoins qu'il
contient et qui redeviennent ordinaires dès que `A` ne les contient plus. C'est
le mutant `endpoint-jete`, et il est létal. Votre P0.1 tient donc entièrement,
mais il se scinde en deux : le **non-crédit** est gratuit, la **conservation**
ne l'est pas.

Je ne généralise pas à q3/q4 : là, `W_q` est un cône strictement plus étroit que
la boule diamétrale, `Phi = 0` n'est plus le bord, et l'argument ne se transpose
pas tel quel. Ce sera à établir à l'étape correspondante.

---

## 3. Le ledger décide vraiment, et le sélecteur tient sur géométrie

Sur `uniform` à `n=120` : `46 622` scissions de témoins, `13 184` spans éliminés,
`3 261` continuations, `8 reprises`, `1 001 656` octets de continuation. Le
ledger n'est pas décoratif.

Invariance à la politique sur `terrain` à `n=100`, six politiques — trois
heuristiques, trois caps de tuile, deux caps de frontière, trois lots :

| politique | `splits` | `scan` | `pending` |
|---|---|---|---|
| `buckets`, lot 1 | `23 731` | **`23 731`** | `1 727` |
| `plus-gros`, lot 1 | `23 384` | **`473 444`** | `1 720` |
| `plus-petit`, lot 1 | `23 384` | `473 444` | `1 720` |
| `buckets`, lot 4 | `23 735` | `6 825` | `1 724` |
| `buckets`, caps serrés | `23 731` | `23 731` | `2 342` |
| `buckets`, lot 2, caps serrés | `23 762` | `12 793` | `2 555` |

`divergences=0`. Le sélecteur à buckets coûte **exactement une unité de scan par
scission** ; le vectoriel en coûte vingt fois plus pour un travail géométrique
équivalent. Le lot de quatre amortit encore. Votre § 4 et votre § 5 tiennent sur
la géométrie, pas seulement sur le modèle.

---

## 4. Les mutants, dont deux que je refuse de maquiller

Cinq des sept meurent sur le juge d'identités :

| mutant | nature | tué par |
|---|---|---|
| `credit-large` (`Phi_max <= 0`) | crédite `z = a` | juge **et** invariant du masque |
| `endpoint-jete` | perd les témoins conservables | juge |
| `none-deux-axes` (`k < 2`) | minorant trop grand, spans jetés | juge |
| `mort-large` (`lower > h`) | rate la mort à l'égalité | juge |
| `upper-sature-incremental` | § 3.3 de l'arbitrage | juge |
| `clear-large` | `upper <= h` | juge |
| `boite-cellule` | *conservateur sous u16* | plancher de fermetures |

`boite-cellule` mérite d'être dit précisément. Sous profil u16, la cellule de
Morton calculée ici — le plus petit cube de grille contenant les deux clés
extrêmes — **contient** la boîte serrée, donc la faute est conservatrice : elle
ne fausse pas. Elle détruit en revanche tout pouvoir de décision — `pruned=0`,
`core_clear=0`, `spans_elimines=0`, `118 632` classifications au lieu de
`93 828` — et meurt sur `--min-pruned=1`. C'est un kill honnête ; prétendre
qu'il meurt sur le juge serait faux.

**Un mutant que j'ai retiré.** J'avais écrit `none-large`, qui relâchait
`Phi_min >= 0` en `>= -1`. Il survivait à tout. En cherchant pourquoi : sur des
coordonnées u16 à l'échelle `49`, `Phi_min = -1` ne se produit simplement jamais
— les valeurs de `Phi` sont de l'ordre de la centaine. J'aurais pu le calibrer
pour qu'il meure ; je l'ai remplacé par la faute d'écriture `k < 2` au lieu de
`k < 3`, qui est réelle, létale, et que personne n'a besoin d'accorder. Un
mutant calibré pour mourir n'est pas un mutant, c'est un décor.

---

## 5. Une affirmation que j'avais écrite sans la vérifier

J'ai écrit dans un commentaire que `two_lines` produisait des coordonnées hors
profil u16 et que c'était pour cela que la famille avait révélé une faute de
boîte. **C'est faux** : la famille générée a une emprise de `65536` et des
coordonnées dans `[0, 65535]`. Une porte de refus construite dessus est passée
au vert du mauvais côté et me l'a fait voir.

Le commentaire est corrigé, la porte est remplacée par une porte **positive** —
`two_lines` passe le juge, `1770 = C(60,2)`, `ecart=0`. Le contrôle de profil
u16 reste en entrée comme garde-fou : c'est lui qui garantit que la cellule de
Morton contient la boîte serrée, donc que `boite-cellule` est conservateur et
non létal. Aucune famille déclarée ne le déclenche aujourd'hui, et je grave le
fait mesuré au lieu de ma supposition.

---

## 6. Ce que q2 n'établit pas

- **La sélectivité.** Votre § 7 est confirmé par la mesure, et de la façon la
  plus nette possible. Rampe sur `terrain`, `cap=8`, `cap_rect=16` :

  | `n` | `pruned` | `core_clear` | tuiles | `frontier_candidate_mass_peak` | `witness_splits` |
  |---|---|---|---|---|---|
  | `60` | `11` | `4` | `149` | **`60`** | `5 644` |
  | `90` | `99` | `16` | `289` | **`90`** | `15 806` |
  | `120` | `166` | `15` | `403` | **`120`** | `28 940` |
  | `160` | `479` | `15` | `626` | **`160`** | `52 493` |

  La masse candidate vaut **exactement `n`** à chaque taille : le premier état
  grossier n'exclut rien du tout. Ce n'est pas un défaut d'ordonnancement, c'est
  la géométrie — la boule diamétrale est grosse, et `NONE_W2` (`Phi_min >= 0`)
  est le seul certificat disponible pour q2. Presque tout finit donc en tuile
  exacte (`547` sur `584` rectangles pour `uniform`).

  Les deux verrous sont bien orthogonaux comme vous l'écrivez : q2 ne dit rien
  du premier, et aucun sélecteur ne le résoudra. `NONE_W3` et `NONE_W4` sont
  attendus là, et cette table est le point de comparaison contre lequel il
  faudra les mesurer.

  Le seul mouvement encourageant est `pruned`, qui croît plus vite que `n`
  (`11 -> 99 -> 166 -> 479`) : à densité constante, la fermeture par bloc
  fonctionne de mieux en mieux quand le nuage grossit. Je ne lis aucune pente
  dans quatre points et je ne l'extrapole pas.
- **Le census.** Il n'est pas requis pour q2 puisque `CORE_CLEAR` y équivaut à
  la vivacité exacte. Pour q3/q4 il reste entier.
- **L'échelle.** Le juge est `O(n^3)`, borné à `400` points. Aucune pente n'est
  mesurée ici, et aucune ne doit être lue dans ces chiffres.
- **Le WSPD.** La partition des paires est une self-jointure lockstep avec un
  cap de masse par rectangle, pas un WSPD séparé. Elle est exacte — la masse
  vaut `C(n,2)` à zéro près sur toutes les familles — mais elle n'est pas
  optimisée, et le nombre de rectangles n'a pas à être lu comme un coût de
  référence.

---

## 6 bis. Vos réponses Q1/Q2 et le P0 du cap, appliqués

Les deux audits `08b7007` et `f62d986` sont arrivés pendant que q2 tournait.
Tout est appliqué, et l'un des deux points corrige un défaut réel de ce que
j'avais écrit ici même.

### 6 bis.1 Le P0 du cap : la fonction de coût ne parlait pas de la même boucle

Vous avez raison, et le contre-exemple est direct. `exact_tile_cap` bornait
`pair_mass * frontier_width`, alors que l'exactificateur parcourt tous les
**points** des spans mixtes. Un span de largeur `1` et de population `256` avec
`pair_mass = 64` et `cap = 64` : le scheduler calcule `64`, la boucle exécute
`16 384`. Facteur `256`, et jusqu'à `50 000` sur la cible.

Corrigé des trois façons que vous prescrivez :

- le cap porte sur `pair_mass * frontier_candidate_mass_exact` ;
- la comparaison passe par une **division** (`pair_mass <= cap / masse`), jamais
  par le produit ;
- l'exactificateur initialise `compte = lower` et ne rescane plus les spans
  `ALL` — le coût ponctuel vaut alors *exactement* le produit borné.

Le contrat est armé **des deux côtés** : la fixture d'ABI le refuse a priori, et
le compteur `exact_point_predicate_evaluations` vérifie a posteriori qu'aucune
tuile acceptée n'a dépassé le cap qu'elle a invoqué pour être acceptée. Le
mutant `cout-tuile-largeur` meurt sur les deux.

Effet de bord mesuré, et il est bon : avec le cap corrigé les blocs raffinent
plus longtemps avant d'exactifier, donc `core_clear` passe de `5` à `70` sur
`uniform` et de `15` à `76` sur `terrain`. Le ledger décide davantage.

### 6 bis.2 Q1 : pas de poids par lane — reçu, c'est ce que j'ai fait

Le cap de l'étage `CoreDepth` borne son travail réel,
`pair_mass * mixed_candidate_point_mass`, et rien d'autre. Les handles portent
un cap de mémoire distinct (`frontier_cap`). Les carriers q3, puis carriers /
Jung / axial q4, auront leurs propres `count -> preflight -> fill` et leurs
propres continuations. Un poids de lane peut ordonner une file, jamais
certifier une ressource — d'autant que le nombre de carriers q4 peut être
`Theta(n)` pour une seule arête.

### 6 bis.3 Q2 : `NONE_OPEN` n'est pas `OUTSIDE_CLOSED`, et j'avais tort

C'est le point qui corrige ce document. J'éliminais uniformément les spans
`Phi_min >= 0`. Pour le compte **ouvert** que la descente calcule, c'est sans
conséquence : un point de shell n'est pas dans `W_2` ouvert, et c'est pourquoi
`manquantes=0 fausses=0` restait vrai. Mais `H = 0` est le **shell**, le
`BallCensusLedger` q2 doit distinguer `I_B` de `U_B`, et un span jeté là emporte
l'information de cosphéricité avec lui.

Architecture B de votre § 3.3, avec `Phi = -H` :

```text
Phi_min > 0   -> OUTSIDE_CLOSED, eliminable sans reserve
Phi_min == 0  -> SHELL_POSSIBLE, transfere, JAMAIS jete
Phi_max < 0   -> tout le span est temoin interieur
```

**Ce n'est pas un cas théorique.** Sur u16 quantifié, la cosphéricité exacte est
abondante :

| famille (`n=120`) | spans de shell conservés | masse | triplets cosphériques exacts |
|---|---|---|---|
| `uniform` | `269` | `529` | **`711`** |
| `terrain` | `330` | `1 824` | **`1 314`** |
| `eight_clusters` | `170` | `406` | `354` |
| `scanline_single_pass` | `572` | `1 086` | **`3 019`** |
| `two_lines` (`n=60`) | `80` | `1 732` | `0` |

J'aurais jeté tout cela. `--verifie-shell` vérifie exhaustivement qu'aucun span
éliminé comme `OUTSIDE_CLOSED` ne contient de point cosphérique pour une paire
du rectangle : `perdus=0` partout.

Et le fait de méthode qui compte : le mutant `shell-jete` est **invisible au
juge d'identités** — il rend `manquantes=0 fausses=0` — parce que le shell ne
compte pas dans le fuseau ouvert. Seule la porte de shell le tue. C'est
exactement le genre de défaut qu'un juge de résultat ne peut structurellement
pas voir, et je n'aurais pas su qu'il était là.

`two_lines` sert de **témoin négatif** : deux droites gauches n'ont aucun
triplet cosphérique (`total_cospheriques=0`), et une porte à plancher y échoue
en code `3`. Sans lui, des planchers non nuls partout laisseraient croire que la
cosphéricité est universelle.

### 6 bis.4 Les autres points, appliqués

- **Batch et budget** (P1). `B_effectif = min(B demandé, budget restant, spans
  scindables)`, avec `B = 0` signifiant « tous », borné pareil. Un budget
  booléen ne suffisait plus dès que l'action est batchée.
- **Mutant du majorant causal** (P1 de réception). Vous aviez raison : il
  mourait pour avoir *oublié les enfants*, faute bien plus grossière que le
  piège de saturation. Il rajoute maintenant la population possible des enfants,
  et la fixture exacte que vous donnez — `h=10`, `C0` de `20` restant mixte,
  `C1` de `5` devenant `NONE` — est gravée. Conséquence honnête : il ne meurt
  plus là où il mourait par grossièreté, et meurt là où la configuration se
  produit vraiment, y compris sur trois familles de géométrie réelle.
- **Masse candidate** (P2.1). Elle ne compte plus que les mixtes ; elle était
  incrémentée avant de connaître la classe, donc elle incluait les preuves
  `ALL`.
- **`terminal_checks`** (P2.2). La porte comparait avec `g_c`, qui portait les
  compteurs de la **dernière** politique exécutée. `checks_buckets` est
  maintenant sauvé au même moment que les deux autres.
- **Télémétrie du sélecteur batché** (P2.3). `bucket_mask_probes`,
  `selected_span_handles`, `batch_build_items` publiés ; le coût annoncé est
  `O(mots de bucket non vides + B)` et non plus `O(1)` quand le sélecteur
  énumère un lot.

### 6 bis.5 Le codec fail-closed, § 4 — fait

Vous aviez raison sur le diagnostic : l'aller-retour sur tampon **valide** ne
prouvait rien. Le parseur lisait `na` et `nm` sans préflight, donc un entier
fabriqué provoquait une lecture hors limites au lieu d'un refus typé.

`decode_continuation` préflighte désormais magic, schéma, taille d'en-tête,
taille de charge utile et somme de contrôle, **puis** confronte les cardinalités
aux octets restants — avant la moindre lecture utile ou allocation. Il valide
ensuite la sémantique : domaine des handles, antichaîne, doublons, masse
recalculée, seuil, époque, lane, et refuse tout octet résiduel. Il rend une
`DecodeError` typée, jamais un booléen.

Un point que votre schéma ne disait pas et que l'implémentation a révélé : les
extrémités `A`/`B` et les spans témoins vivent dans **deux domaines distincts**
— la partition des paires d'un côté, l'arbre des témoins de l'autre. Les
confondre relâcherait la validation du plus petit des deux, donc
`ContexteDecodage` porte `nb_noeuds` et `nb_endpoints` séparément.

Campagne de corruption : **122 cas, 122 refus typés, zéro accepté à tort**.
Troncature à *chaque* octet du tampon (pas seulement aux bornes de champ),
cardinalités fabriquées jusqu'à `4 000 000 000`, handle hors domaine, doublon,
parent et descendant ensemble, masse fausse, lane inconnue, endpoint hors
domaine, état déjà terminal, octets finaux.

Vérifié sous `-fsanitize=address,undefined` : aucune lecture hors limites, ni
sur la campagne de corruption, ni sur la reprise croisée. Le codec est sur le
**chemin normal** de la reprise — sans cela ces portes négatives ne testeraient
qu'un code mort.

Ce que je ne revendique pas : il n'y a ni `tree_digest`, ni authentification de
l'identité du nuage. La somme de contrôle détecte une corruption, elle ne
signe rien. C'est `FailClosedContinuationCodec-v1` au sens du refus typé, pas
une continuation authentifiée.

---

## 7. Question

**Q3 — la stricte pour q3 et q4.** Pour q2, le non-crédit d'un span endpoint est
gratuit parce que `z = a` donne exactement `Phi = 0`, c'est-à-dire le **bord** de
`W_2`, et que la stricte l'exclut. Pour q3 et q4, la condition est
`4 H^2 > |e|^2 |t|^2` et `3 H^2 > |e|^2 |t|^2` avec `H = e.t` ; pour `z = a` on a
`e = 0`, donc `H = 0` et les deux membres valent zéro — l'inégalité stricte
échoue encore, donc `z = a` reste exclu.

Mais l'argument de **bloc** que j'utilise ici passe par `Phi_max >= 0` sur le
produit des boîtes, et je n'ai pas d'équivalent immédiat pour un test de la
forme `q H_min^2 > (|e|^2 |t|^2)_max` — un carré n'est pas monotone, et le
majorant du membre droit ne se compose pas aussi simplement. Avez-vous une forme
de bloc pour q3/q4 qui garde la propriété « un span endpoint n'est jamais
crédité » sans test relationnel séparé, ou faut-il accepter que pour q3/q4 le
masque redevienne un **mécanisme** — un drapeau de recouvrement porté par le
span — plutôt qu'une conséquence de la stricte ?

Selon la réponse, `CoreDepthLedger::relation_frontier` est soit un champ mort
pour q2 (ce qu'il est aujourd'hui), soit le champ central de q3/q4.

---

## 8. Étape suivante

Votre séquence donne `NONE_W3` / `NONE_W4` (§ 9.E) puis q3 et q4 complets. Je
prends `NONE_Wq` ensuite : le certificat `4 Hmax^2 <= Emin Xmin` /
`3 Hmax^2 <= Emin Xmin` de `8870e6f`, avec porte annulaire, mutants de stricte
et de coefficients, et parité par identités contre la force brute — la contre-
famille annulaire est déjà gravée et sert de fixture de réfutation.
