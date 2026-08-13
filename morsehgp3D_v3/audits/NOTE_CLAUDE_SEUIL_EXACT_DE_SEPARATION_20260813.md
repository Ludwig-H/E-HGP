# Note de Claude — le mur avait un seuil, et il est en forme close

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

> **Contre-audit live — corrections encore requises.** Les seuils ci-dessous
> sont suffisants pour garantir une boule inscrite positive, jamais nécessaires
> à une fermeture : une fixture ferme q4 à `s=3` avec `rho_lb<0`. Les phrases
> « le moteur ne pouvait rien fermer », « linéaire/borné » et les deux verdicts
> « Fermé » restent donc des surclaims à lire comme observations mono-seed sur
> une plage finie. La boule idéale q4 n'est ni `CentralBall209` ni le spindle
> anisotrope déjà présent dans `spindle_cone.hpp`. Enfin, les portes vertes ne
> reçoivent pas encore la finalité : le nominal avec banque passe avec
> `fenetre_finale=NON`, et les modes non-VWave peuvent annoncer `OUI` après un
> abandon. Réponse et fixtures :
> [`AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md`](AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md).

Cette note fait trois choses : elle rétracte le chiffre central de ma note
précédente, elle livre le compteur `EdgeWindowRangeAdd-v0` que l'audit avait mis
en tête de la directive, et elle donne la raison — exacte, close, vérifiable —
pour laquelle le moteur ne fermait rien en q3 ni en q4.

## 1. Rétractation : mon plafond de `84 %` portait sur la mauvaise lane

Ma note `b96751c` écrivait : « le certificat factorisé plafonne à `84 %` sur
`eight_clusters`, et le reste est quadratique ». Ce `84 %` est une mesure de la
lane **q2**. Je l'ai présenté comme le plafond du certificat, donc de q4.

Mesuré maintenant lane par lane, `uniform`, `n=12 500`, `s=3`, budget `512`,
`pending=0` :

| lane | masse ouverte |
| ---: | ---: |
| q2 | `3,98 %` |
| q3 | `87,49 %` |
| q4 | `96,62 %` |

Le certificat ne plafonnait pas à `84 %` en q4. Il y fermait `3 %`.

## 2. La raison : chaque lane a son propre seuil de séparation, en forme close

Le certificat central compare `V^2 = \lVert 2z-a-b\rVert^2` à
`D^2 = \lVert b-a\rVert^2`. Les trois lanes sont trois seuils sur le même
rapport, et je note `tau_q` la racine du seuil :

| lane | test | `tau` |
| :--- | :--- | ---: |
| q2 | `V^2 < D^2` | `1` |
| q3 | `3 V^2 < D^2` | `1/\sqrt{3}` |
| q4 | `V^2/D^2 < 2-\sqrt{3}` | `\sqrt{2-\sqrt{3}}` |

Le cœur universel d'une **paire** est donc la boule de centre `m=(a+b)/2` et de
rayon `tau_q D/2`. Pour q4 ce rayon vaut `0,2588 D`, alors que `a` et `b` sont
à `D/2`.

**Proposition (minoration, corrigée par l'audit).** Soit un rectangle `A x B`
de rayons circonscrits `r_A, r_B` et de distance entre centres `d`. Le cœur
commun à toutes les paires de `A x B` **contient** la boule centrée en
`m_0=(c_A+c_B)/2` et de rayon

```text
rho_lb = (tau/2) (d - r_A - r_B) - (r_A + r_B)/2.
```

*Preuve.* `D_min \ge d - r_A - r_B`. Le milieu `m` d'une paire de `A x B`
décrit une boule de rayon `(r_A+r_B)/2` autour de `m_0`, donc pour un site `z`,
`V_max \le 2(\lVert z-m_0\rVert + (r_A+r_B)/2)`. Le certificat
`V_max \le tau D_min` est donc **impliqué** par
`\lVert z-m_0\rVert \le rho_{lb}`. ∎

**Ce que la proposition ne dit pas.** L'audit a raison de refuser ma
formulation initiale. C'est une **inclusion**, pas une égalité :

- `rho_lb > 0` ne fournit aucun témoin — la boule garantie peut ne contenir
  aucun point du nuage, encore moins huit `PointId` distincts ;
- `rho_lb \le 0` ne prouve **aucune** vacuité : la minoration a seulement cessé
  de parler. Deux feuilles singleton ont `r_A=r_B=0` et un cœur non vide quelle
  que soit la valeur de `s`.

L'énoncé correct est donc : **en dessous du seuil, le seul invariant de
séparation ne garantit plus une boule inscrite de rayon strictement positif dans
le pire cas.** J'avais écrit « le cœur est vide pour tout nuage », ce qui est
faux, et le contre-exemple des deux singletons suffit à le montrer.

**Corollaire.** Sous `d \ge (s+2) r` et `r_A+r_B \le 2r`, il vient
`rho_{lb} \ge r(tau s/2 - 1)`, donc un rayon garanti positif exige `s > 2/tau`.

Il faut en outre distinguer le seuil **idéal** de celui du certificat
**réellement codé**, qui emploie l'approximation rationnelle sûre
`209 V^2 \le 56 D^2`, donc `tau_{code} = \sqrt{56/209}` :

| lane | seuil idéal | valeur | test entier pour `s=p/q` |
| :--- | :--- | ---: | :--- |
| q2 | `2` | `2` | `p > 2q` |
| q3 | `2\sqrt{3}` | `3,4641016` | `p^2 > 12 q^2` |
| q4 idéal | `\sqrt{6}+\sqrt{2} = 4\cos(\pi/12)` | `3,8637033` | — |
| q4 **codé** | `\sqrt{209/14}` | `3,8637510` | `14 p^2 > 209 q^2` |

L'écart entre `3,8637033` et `3,8637510` est infime, mais un audit exact ne peut
pas les identifier, et c'est le second qui gouverne le code. Ces trois tests
sont des **planchers de résolution** : ni une fermeture, ni un motif de rejet
d'un rectangle, ni un optimum de coût.

Toute la campagne du reçu G4 a tourné à `s=3`. À `s=3`, q3 et q4 sont l'une et
l'autre **sous leur seuil**, donc l'invariant de séparation seul n'y garantit
plus aucune boule inscrite positive. Ce que j'appelais « le mur » était d'abord
cela. Attention à ne pas en faire plus qu'il n'y a : le seuil est suffisant,
jamais nécessaire, et une fermeture reste possible en dessous sur les
rectangles dont la distance réelle dépasse la borne garantie.

### Vérification, `uniform`, `n=3 000`, masse ouverte par lane

| `s` | q2 | q3 | q4 |
| ---: | ---: | ---: | ---: |
| `2` | `56,93 %` | `99,96 %` | `99,99 %` |
| `3` | `14,91 %` | `96,87 %` | `99,16 %` |
| `4` | `6,49 %` | `73,09 %` | `87,91 %` |
| `5` | `4,32 %` | `47,44 %` | `63,57 %` |

Chaque lane ne commence à fermer massivement qu'une fois `s` passé **son**
seuil : q3 reste quasi inerte jusqu'à `3,46`, q4 jusqu'à `3,86`. Les deux
transitions sont **cohérentes** avec les seuils, sans les démontrer : la
minoration n'interdit pas de fermer en dessous, et c'est bien ce qu'on observe —
`0,8 %` de q4 ferme à `s=3`, sur des rectangles dont la distance réelle dépasse
largement la borne garantie.

## 3. `EdgeWindowRangeAdd-v0` : la fenêtre d'arêtes, exacte, en `O(F+n)`

J'avais écrit que le maximum de `E_q(a)` exigeait de développer `|A||B|` par
terminal. C'est faux, et l'audit `ab32c9d` §5 en donne la raison : les nœuds du
radix tree sont des intervalles disjoints de `GenerationRank`, donc tout
terminal satisfait un ordre **total** entre ses deux plages, et l'orientation
canonique en fait un range-add — deux écritures et un scan préfixe.

Le ledger est implémenté et gardé. Ce qu'il publie, par lane : terminaux
ouverts, masse ouverte, `sum E_q`, `max E_q`, p50/p95/p99, et `pending` par
lane, avec `fenetre_finale=OUI` seulement si aucune continuation ne reste
pendante.

Portes ajoutées. **Correction : ma première version de cette note écrivait
« toutes vertes » alors que je n'avais lancé que les six portes courtes.**
Le contre-audit a rejoué les huit et trouvé `7/8` : `fenetre_pente_uniform`
sortait rouge après `132 s` parce que l'ancienne porte `front_records` rendait
`3` **avant** l'impression de `pente sum_E4`, et `fenetre_pente_mord` pouvait
être satisfaite par cette même ancienne porte au lieu de la sienne. Les deux
défauts sont réparés : toutes les mesures sont désormais publiées **avant** tout
verdict, chaque refus porte un motif distinct (`REFUS DE PENTE sum_E4`), et les
deux tests désarment explicitement les portes antérieures par `--max-slope=9`.
La liste ci-dessous est celle des **dix-neuf** portes de la famille
`wspd_wavefront`, rejouées ensemble avec le binaire final : `19/19`, dont les
cinq portes antérieures que le refactor des sorts aurait pu casser.

- `fenetre_oracle` et `fenetre_oracle_amas` : l'oracle développe chaque
  `PairId` **exactement une fois**, oriente par `GenerationRank` et compare
  **tout** le vecteur de degrés, sur `uniform` et sur `eight_clusters` ;
- `fenetre_mutant_orientation` (code `4`) : le range-add sur des intervalles de
  `PointId`. Trois portes indépendantes le mordent, et le reçu dit laquelle :
  domaine des degrés `1 503`, identité de somme, oracle `1 800` désaccords ;
- `fenetre_mutant_cote` (code `3`) : créditer toujours le côté stocké en
  premier. **L'identité de somme y survit intacte** — les deux plages ont la
  même masse — et c'est pourquoi l'équivariance par échange des côtés est une
  porte séparée ;
- `fenetre_plancher_mord` (code `3`) et `fenetre_refus_mutant_sans_oracle`
  (code `2`) ;
- `fenetre_oracle_banque` : l'oracle sur une relation **non triviale**. Les deux
  précédents n'activent aucune banque et comparent donc le range-add à la
  relation complète `C(n,2)`, ce qui ne juge aucune sélection par sort. Ce
  nominal exige `10 000` terminaux fermés **et** `10 000` ouverts en q4 ;
- `fenetre_pente_uniform` et `fenetre_pente_mord` (code `3`) : la pente de
  `sum E_4` est gatée, et la porte est prouvée mordante à `s=3`, là où la
  minoration du §2 ne garantit plus aucun rayon positif ;
- `fixtures_spindle` : les deux fixtures gravées du §5 bis, dont celle de
  l'audit `b96751c` §1.2, jugées par un balayage du disque de Jung qui n'emploie
  aucune des deux algèbres certifiées ;
- `fixtures_rang` : la fixture de l'audit `b96751c` §2, qui ferme **ma propre
  proposition**. J'avais demandé si borner la liste de partenaires par un rang
  supprimerait le `\lvert lens\rvert^2` sans rien changer au front. La réponse
  est non, définitivement : le support q4 de sommets `(5000,40000,30000)`,
  `(55000,40000,30000)`, `(30000,5000,40000)` et `(30000,5000,20000)` est
  positif, de profondeur **zéro**, d'arête maximale **unique**, et ses `4 381`
  satellites `(5000,40000+j,30000)` sont tous strictement plus proches de `a`
  que ne l'est `b`. Le second endpoint est donc au **rang 4 382**, et la fixture
  le vérifie aux coordonnées exactes. Aucun seuil de rang ne peut être exact.

Le ledger est aussi devenu **massiquement exclusif** : `fermée + pendante +
ouverte = C(n,2)` par lane, gaté. Ma version précédente comptait les pendants
dans la masse ouverte et n'utilisait `pend` nulle part.

Le drapeau `fenetre_finale` a lui aussi été réparé sur signalement du
contre-audit live : les modes non-VWave annonçaient `OUI` **après un abandon**.
La fenêtre Morton, en particulier, n'examine jamais tout le nuage — c'est une
proposition bornée autour d'une clé, jamais une preuve d'absence — et la
descente peut sortir sur un tas non vide ou un débordement. Les deux marquent
désormais leurs lanes non fermées comme pendantes, et deux portes exigent le
verdict dans les **deux** sens (`fenetre_finale_non`, `fenetre_finale_oui`).

Enfin, l'ancien degré symétrique parcourait encore ses deux plages, si bien que
le wall du probe n'était pas `O(F+n)` malgré le nouveau ledger — l'audit le
relève et il a raison. Ce compteur est lui aussi devenu un range-add : le degré
symétrique étant la somme des deux orientations, il vaut exactement **deux**
range-adds, donc quatre écritures par terminal. Tout le ledger est maintenant en
`O(F+n)`, et `nsum`/`nmax` sont inchangés par construction.

Le producteur émettant structurellement la plage basse en premier — mesuré,
`A<B = 17 444` et `B<A = 0` —, la seconde branche de l'ordre total serait du
code mort. Chaque fenêtre est donc calculée **deux fois**, la seconde sur les
terminaux dont les côtés sont échangés, et les deux vecteurs doivent être
identiques.

## 4. Ce que le ledger mesure une fois `s` au-dessus du seuil

`s=8`, boîte serrée, budget `512`, `pending=0` partout.

| famille | `n` | q4 ouvert | `sum E_4` | `max E_4` |
| :--- | ---: | ---: | ---: | ---: |
| `uniform` | `3 000` | `22,84 %` | `1 027 538` | `1 337` |
| | `6 000` | `13,30 %` | `2 394 081` | `1 366` |
| | `12 000` | `7,91 %` | `5 693 663` | `1 537` |
| | `24 000` | `3,99 %` | `11 490 601` | `1 485` |
| `terrain` | `3 000` | `12,60 %` | `566 926` | `718` |
| | `24 000` | `3,76 %` | `10 838 433` | `7 367` |
| `eight_clusters` | `3 000` | `89,93 %` | `4 045 644` | `2 912` |
| | `24 000` | `73,71 %` | `212 268 176` | `21 551` |

Pentes :

| famille | pentes `sum E_4` | pentes `max E_4` |
| :--- | :--- | :--- |
| `uniform` | `1,220 / 1,250 / 1,013` | `0,031 / 0,170 / -0,050` |
| `terrain` | `1,330 / 1,394 / 1,532` | `1,304 / 1,071 / 0,983` |
| `eight_clusters` | `1,904 / 1,907 / 1,903` | `0,988 / 0,933 / 0,967` |

Une réserve sur la dernière ligne : le run `eight_clusters` sort en code `3`,
refusé par l'**ancienne** porte `degre_residuel` (`1,458` puis `1,502`) avant
l'impression des pentes `sum E_4`. Les quatre tailles ont bien terminé et leurs
quatre lignes `fenetre q4` sont dans le reçu, mais ces trois pentes sont
**calculées à la main** depuis ces quatre valeurs, non imprimées par le binaire.
Le reçu le dit aussi.

Trois régimes nets. `uniform` rend un résiduel **linéaire** et un `max E_4`
**borné** : c'est exactement la propriété dont `LocalShallowBall` a besoin, et
elle est mesurée, pas supposée. `terrain` monte. `eight_clusters` est
**quadratique**, `1,90` sur les trois transitions.

Sur `terrain`, monter à `s=16` améliore sans réparer : pentes `sum E_4`
`1,158 / 1,235` sur `3 000 -> 6 000 -> 12 000`.

## 5. À qui la perte : mesure par paire, sans aucun jeu de rectangle

Deux causes possibles, que ma note précédente confondait. Ou bien la paire
possède ses témoins universels et c'est la **factorisation** qui les perd ; ou
bien elle n'en a aucun, et alors aucun certificat central ne la fermera jamais.

J'échantillonne donc dans la masse ouverte **en lane q4**, et je compte
exactement, par balayage du nuage, les sites du cœur q4 de la **paire**
(`209 V^2 \le 56 D^2`, aucune boîte, aucun rectangle). `n=6 000`, `s=8`,
`4 000` tirages :

| famille | cœur q4 moyen | cœur vide | `\ge 8` témoins |
| :--- | ---: | ---: | ---: |
| `uniform` | `13,61` | `6,2 %` | `57,5 %` |
| `eight_clusters` | `43,79` | `13,9 %` | `71,6 %` |
| `terrain` | `71,61` | `4,6 %` | `74,1 %` |

**La perte est bien la factorisation, et le verdict est net.** De `57 %` à
`74 %` des paires que le rectangle laisse ouvertes sont individuellement
fermables **par le même certificat**. J'avais avancé, avant de mesurer, que les
paires inter-amas auraient un cœur vide parce que leur milieu tombe dans un
vide : c'est faux, et `eight_clusters` a au contraire le cœur moyen le plus
**gros** des trois — un cœur de rayon `0,2588 D` avec `D` inter-amas attrape
d'autres amas.

Le rapport des rayons chiffre exactement l'écart. Cœur de la paire :
`tau D/2 \approx tau (s+2) r/2`. Cœur du rectangle : `r(tau s/2 - 1)`. À `s=8`,
le rapport vaut `2,417` en rayon, donc `14,1` en volume.

## 5 bis. Et le cœur que nous testons n'est pas le cœur : c'est sa boule inscrite

En cherchant pourquoi `eight_clusters` résistait, j'ai relu la dérivation de
`rect_front.hpp`. Elle est explicite : le test vient de la condition universelle
`c H^2 > E_2 X_2` **en supprimant le terme `-4 (d\cdot v)^2`**. Ce terme est
favorable, et il est maximal **sur l'axe de l'arête**.

Le cœur universel exact se dérive directement. Avec `u = z-m`, le site est
intérieur à toute sphère admissible si et seulement si, pour tout `t` du disque
de Jung de rayon `D/(2\sqrt{2})` orthogonal à `d`,
`\lVert u-t\rVert^2 < D^2/4 + \lVert t\rVert^2`. Le minimum de `t\cdot u` sur ce
disque vaut `-D \lVert u_{\perp}\rVert/(2\sqrt{2})`, d'où

```text
||u||^2 + (D/sqrt(2)) ||u_perp|| < D^2/4,
```

et en élevant au carré, avec `4H = D^2-V^2` et `d\cdot v = 2 (d\cdot u)` :

```text
(D2 - V2)^2 > 2 (V2 D2 - (d.v)^2)      et      D2 > V2.
```

Tout est entier, tout tient en `i128` sous u16. Deux vérifications :

- à `d\cdot v = 0` — sur le plan médiateur — la condition redonne
  **exactement** `V^2/D^2 < 2-\sqrt{3}`, c'est-à-dire le test implémenté. Le
  test implémenté est donc le **pire cas directionnel** : la boule inscrite ;
- sur l'axe, `(d\cdot v)^2 = V^2 D^2`, le membre droit s'annule et la condition
  redevient `H > 0`. Le vrai cœur atteint la **boule diamétrale entière** le
  long de `ab`.

Le même calcul avec le disque q3, de rayon `D/(2\sqrt{3})`, donne
`3 (D2-V2)^2 > 4 (V2 D2 - (d\cdot v)^2)`, qui redonne `3V^2 < D^2` à
`d\cdot v = 0`. Les deux lanes testent aujourd'hui leur boule inscrite.

Mesure du même échantillon, `n=6 000`, `s=8`, `4 000` tirages dans la masse
ouverte q4 :

| famille | inscrite : moyen / vide / `\ge 8` | **exact** : moyen / vide / `\ge 8` |
| :--- | :--- | :--- |
| `uniform` | `13,66` / `6,1 %` / `57,4 %` | `23,42` / `3,9 %` / `72,4 %` |
| `eight_clusters` | `43,46` / `13,8 %` / `71,3 %` | `245,18` / `0,9 %` / `95,0 %` |
| `terrain` | `72,66` / `4,7 %` / `73,2 %` | `155,44` / `2,3 %` / `84,4 %` |

Le gain est le plus fort exactement là où le résiduel était quadratique. Sur
`eight_clusters`, le cœur exact porte `5,6` fois plus de témoins, la fraction de
cœur vide s'effondre de `13,8 %` à `0,9 %`, et `95 %` du résiduel devient
fermable. C'est le mécanisme prédit : pour une paire inter-amas, les témoins
sont **près de l'axe**, dans les amas des deux extrémités — précisément ce que
la boule inscrite jette.

Ce n'est pas encore une réparation du moteur. Le passage au rectangle demande un
**minorant** de `(d\cdot v)^2` sur `A \times B \times C`, donc l'intervalle de
`d\cdot v` ; quand il enjambe zéro, le minorant sûr est zéro et l'on retombe sur
la boule inscrite — fail-open, donc sain. Le cas favorable est celui où `C` est
aligné avec `d`, c'est-à-dire exactement le cas inter-amas.

**Portée exacte du mot « exact ».** L'audit exige la qualification et il a
raison : ce certificat est exact **sur le disque de Jung et sous owner
maximal**, jamais « exact pour toute sphère passant par la paire ». Le domaine
réel des centres peut être plus petit que le disque, ce qui le rend suffisant et
jamais complet. L'audit nomme les trois niveaux séparément —
`CentralBall209-v0`, `JungSpindleSingleton-v0`, `CageFlower-v0` — et je reprends
sa nomenclature.

**Deux fixtures gravées**, portées par `--fixtures-spindle` :

- **A, sûreté** — la fixture de l'audit `b96751c` §1.2 : `a=(100,100,100)`,
  `b=(200,100,100)`, `x=(150,30,120)`, `y=(150,30,80)`, et les dix
  `z_i=(150+i,140,100)` pour `i=-4..5`. Les dix sont strictement dans la boule
  diamétrale et strictement **hors** de la circumsphère q4 de centre
  `(150,80,100)` et `R^2=2900`. Aucun des deux certificats ne les crédite, et le
  balayage indépendant du disque de Jung exhibe un centre excluant pour les dix.
  C'est exactement la confusion que je faisais : « dix témoins q2 » n'est pas un
  certificat q4 ;
- **B, non-vacuité** — `z=(110,100,100)`, sur l'axe. La boule inscrite le
  rejette, le spindle le crédite, et le balayage ne trouve aucun centre excluant.
  Sans cette fixture, un spindle qui n'accepterait jamais rien passerait A.

## 6. Ce que cela ouvre, ce que cela ferme

**Aucun de ces deux points n'est « fermé »** — le contre-audit live a raison, et
je reformule.

Affaibli : « le résiduel q4 est intrinsèquement quadratique ». Sur `uniform`,
une graine, quatre tailles de `3 000` à `24 000`, `s=8`, la pente de
`sum E_4` descend à `1,013` et `max E_4` reste dans `[1 337, 1 537]`. C'est une
**observation sur une plage finie**, pas une preuve de linéarité ni de
bornitude ; elle suffit à réfuter mon ancienne affirmation de quadraticité
universelle, pas à lui substituer une loi.

Affaibli aussi : « mesurer q3 ou q4 à `s \le 3` ne peut rien rendre ». La
proposition du §2 est **suffisante, jamais nécessaire** : elle garantit une
boule inscrite positive au-dessus du seuil, elle n'interdit rien en dessous.
Une fixture ferme q4 à `s=3` avec `rho_{lb}<0`, et la mesure le montre aussi —
`0,8 %` de la masse q4 ferme à `s=3`. L'énoncé défendable est donc : à `s=3`,
**l'invariant de séparation seul** ne garantit plus rien aux lanes q3 et q4, et
la fermeture y devient accidentelle plutôt que structurelle.

Ouvert, et c'est le vrai nœud : `eight_clusters` reste à `1,90` alors que
`71,6 %` de son résiduel est individuellement fermable. Le `s` global ne
rattrape pas cet écart — il coûte le front en `s^{1,5}` mesuré et laisse la
pente quadratique. L'audit `360ea7c` avait déjà écrit la direction : refus de
`s=8` global, raffinement **local** guidé par `Vbest`. Ma mesure la chiffre
enfin : le raffinement local n'a pas à gagner un facteur inconnu, il a à
récupérer les `71,6 %` que la granularité du rectangle perd.

La tendance du plafond par paire, `eight_clusters`, `s=8`, boule inscrite :

| `n` | cœur moyen | vide | `\ge 8` témoins |
| ---: | ---: | ---: | ---: |
| `3 000` | `22,63` | `15,2 %` | `62,5 %` |
| `6 000` | `43,46` | `13,8 %` | `71,3 %` |
| `12 000` | `85,91` | `12,7 %` | `78,0 %` |

Sous la boule inscrite, la masse non fermable par paire croît encore en
`n^{1,52}`, et la masse à cœur vide en `n^{1,78}` : le raffinement local seul,
poussé jusqu'à la paire, ne suffirait pas. Sous le cœur exact, la fraction vide
tombe à `0,9 %` dès `n=6 000`. C'est le certificat, pas la granularité, qui
décide ici.

## 7. Ce que l'audit a répondu, et ce qui reste ouvert

Mes trois questions ont reçu réponse dans
[`AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md`](AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md)
avant que je ne publie cette version. Je les enregistre ici plutôt que de les
reposer :

1. **Oui** au raffinement local des seuls terminaux ouverts, **non** à
   `rho_{lb}>0` comme critère d'arrêt : c'est une minoration, pas une décision.
   L'arrêt exact est `credit_4 \ge 8` ; un cap donne `PENDING_CONTINUATION` et
   jamais `OPEN` final. `rho_{lb}` peut prioriser une tâche, jamais décider un
   sort.
2. Les `13,9 %` ne sont **pas** un résiduel irréductible : ils portent sur la
   petite boule rationnelle. Le spindle de Jung est plus grand — je le mesure
   au §5 bis, `0,9 %` — et même son vide n'exclut pas les **cages**, où le
   témoin varie avec le centre.
3. **Oui** à une tâche partagée `(ANode,BNode,lane\_mask)` avec critères locaux
   par lane, **non** au choix automatique « juste au-dessus du seuil » : les
   seuils éliminent les valeurs structurellement peu informatives, ils ne
   choisissent pas l'optimum d'un coût composé.

## 7 bis. Audit de l'auditeur : j'ai vérifié, ils ont raison

Trois de leurs claims portent le poids de ce qui vient ensuite. Je ne les ai pas
reçus sur parole.

**L'identité du terme directionnel** (`§6`). `T = d\cdot v = \lVert
z-a\rVert^2 - \lVert z-b\rVert^2` : vérifiée algébriquement, les deux membres
valent `2(b-a)\cdot z + a\cdot a - b\cdot b`. Elle **débloque exactement** la
question que je posais, car elle est séparable par axe — `a`, `b` et `z`
choisissent leurs coordonnées indépendamment dans des boîtes alignées, donc
l'intervalle de la somme est la somme des intervalles.

Leur prescription de candidats — bornes de `C`, ruptures de la distance, et les
deux entiers voisins du milieu qui sépare les endpoints les plus lointains — est
implémentée et **comparée à une énumération exhaustive de tout
`A \times B \times C`** sur trois mille triplets de boîtes tirées :
`3 000` cas, `3 000` intervalles non triviaux, **zéro désaccord**. Leur
contre-exemple des coins se reproduit exactement : sur `A=[0,1]`, `B=[0,3]`,
`C=[0,2]`, le maximum vaut `4` en `(0,2,2)` et les huit extrémités donnent `3`.
Porte `fixtures_terme_t`.

**La cosphère à 384 points** (`§1.2`). Recalculée indépendamment :
`N=826408505=5\cdot 13\cdot 17^2\cdot 29\cdot 37\cdot 41` donne exactement
`384` représentations, toutes dans `[4021,61515]` donc dans le cube u16 ;
`\binom{384}{3}=9 363 584` triples se répartissent en `6 967 680` obtus,
`73 344` rectangles et `2 322 560` aigus. **Les cinq nombres sont exacts.**

**La fixture de rang** (`§2`). Vérifiée aux coordonnées : les quatre sommets
sont exactement sur la sphère de `R^2=725000000`, `ab^2=2\,500\,000\,000` est
l'unique maximum, les `4 381` satellites sont tous strictement hors de la
circumsphère et strictement plus proches de `a` que ne l'est `b`. Le partenaire
est au rang `4 382`. Porte `fixtures_rang`.

Je n'ai trouvé aucune erreur dans ces trois blocs.

Ce qui reste ouvert de mon côté :

- le passage du spindle au rectangle est **débloqué** par leur `§6` mais pas
  encore implémenté dans le moteur : le test sûr est `Dlo > Vhi` et
  `(Dlo-Vhi)^2 > 2(Dhi\,Vhi - Tabs^2)`, avec `Tabs=0` si l'intervalle de `T`
  traverse zéro. Ils notent eux-mêmes qu'il est fail-open et non sans perte,
  parce que `D`, `V` et `T` sont corrélés et que leurs extrema séparés perdent
  du rappel ;
- le ledger reste un compteur d'arêtes candidates sous hypothèse d'arête
  maximale : aucun oracle n'établit encore que l'arête maximale canonique de
  chaque vrai q4 tombe dans la fenêtre centrale. L'oracle actuel juge le ledger,
  pas la géométrie scientifique. L'audit le relève et je ne le conteste pas.

## 8. Non-claims

Les pentes sont mesurées sur trois ou quatre tailles, une graine, un binaire
CPU, sans répétition ni p95 ; ce ne sont pas des pentes reçues au sens du
contrat. `scanline_overlap_multiecho` et `scanline_single_pass` n'ont pas été
mesurées. La proposition du §2 borne le cœur commun sous la séparation
**garantie** `d \ge (s+2)r` ; un rectangle dont la distance réelle dépasse cette
borne peut fermer sous le seuil, et c'est ce qui explique les `1,5 %` fermés à
`s=3` plutôt que zéro. Aucun `BallKey`, aucun census, aucun fold, aucun payload
n'est produit ici : le ledger compte des arêtes candidates, il n'en construit
aucune. Le contrat reste `p95 warm_e2e < 100 ms` à `n=50 000`, et rien ici n'est
un temps.

Le cœur exact du §5 bis est **mesuré par paire seulement**. Aucune version
rectangle n'est implémentée, aucun compteur du moteur ne l'utilise, et le
certificat en production reste la boule inscrite. Sa dérivation suppose que tout
support q4 dont `ab` est l'arête maximale a son centre dans le disque de Jung de
rayon `D/(2\sqrt{2})` : c'est la correction que l'audit `590683c` §2 a apportée
à ma note, et je l'emploie telle qu'elle a été reçue.
