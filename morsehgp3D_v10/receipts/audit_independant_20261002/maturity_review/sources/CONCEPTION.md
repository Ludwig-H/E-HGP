# Existence mûre : définitions, preuves, fixtures

2 octobre 2026. Idée étudiée : entre « couvert » (date α_K) et « cœur » (date d_K), un site devient **mûr** dans sa
lignée à une date intermédiaire ; une composante n'est un cluster qu'une fois qu'elle possède mcs sites mûrs.
L'appartenance reste précoce : seule l'**existence** des clusters est retardée (§ 2 à 9, forme causale `M[θ]`).
Extension, § 10 : la maturité **valide** les clusters sans les retarder (forme rétroactive `R[θ]`).

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=benchmark_only (règle candidate)
public_status=not_claimed
```

GCP non utilisé. Aucun moteur modifié. Le juge du verrou, la bibliothèque des règles, `vote_condense_v2` et `er0h` sont
importés, jamais copiés ni modifiés. Graines : espace `dev_tour_points` et nuages de mise au point hors de tout plan.

Vocabulaire des statuts : **prouvé** (argument complet ci-dessous), **contrôlé** (vérifié sur un nombre de cas donné),
**réfuté** (contre-exemple exécuté), **observé** (constat sans garantie). Reçus : `recus/`.

## 1. Réponse courte

| Question | Réponse |
| --- | --- |
| La règle | `M[θ, projection]` : la projection (cover ou ER0h) donne le propriétaire et la date d'entrée ; un nœud compte ses sites possédés dont la date de maturité est passée ; un site est membre de son cluster dès max(entrée, naissance du cluster) |
| Forme close (théorème F) | date d'entrée t(x) = mcs-ième plus petite valeur, sur les sites y, de max(m(y), u_P(x, y)) ; hauteur de réunion = max(u_P(x, y), t(x), t(y)). u_P : hauteur de réunion dans la projection ; m : date de maturité. **Prouvé**, contrôlé en exact sur 1 296 000 dates (recoupe) |
| θ = 0 | c'est la condensation par cohortes de la projection : **prouvé**, contrôlé (168 formes canoniques ; mêmes étiquettes plates que cover et que ER0h sur 50 unités de scènes réelles, 450 coupes chacune) |
| θ = 1 | existence aux dates de cœur, propriétaire de la projection ; mêmes clusters que `cdelay[1]` condensé, appartenances plus précoces : **prouvé**, contrôlé |
| Les 125 jugements ancrés, juge tel quel | θ = 0 : 74 (cover), 125 (ER0h). θ = 1/16 : **125** (`er0ha`). θ = 1/8 : 115. θ = 1/4 : 105. θ = 1/2 : 65. θ = 3/4 : 55. θ = 1 : 45. Jusqu'à θ = 1/4 tous les échecs de `er0ha` sont des **retards** (au plus 11,5 %), aucun échec de structure |
| Seuils exacts en θ (`er0ha`) | Q4 : 0,0829. Q1bis à mcs 2 : 0,1547. Q2 : 0,2079. T0 : 0,2995 à 0,3000. Q1 à mcs 3 : 0,6975. Les deux triangles restent deux clusters avant la fusion jusqu'à θ = 0,7874 (pont court) et 0,9318 (pont égal au côté) |
| Forme (ii), géométrique | seuil des triangles τ > 0,1189 (pont court), comme annoncé (0,119). À K = 2 les deux formes **coïncident** pour la lignée cover, avec θ = (1 − τ) / (1 + τ) : 0,1189 ↔ 0,7874 |
| Continuité | l'étage d'existence n'ajoute **aucune** discontinuité : M[θ] est aussi continue que sa projection (théorème C, **prouvé**). Avec cover : sauts de cover (4 608 pour 1 mm). Avec ER0h : aucun saut sur six familles, pas de 12 par millimètre au plus. Avec un vote : sauts du vote (486, 288, 250) |
| « Un point de bord seul ne crée pas de cluster » | tenu sur la cellule ancrée pour tout θ. **Réfuté** pour θ < 1 sans le filament : le cluster naît à (1 − θ) α + θ d_K ; à θ = 1 le site qui complète le compte est un site de cœur |
| Aperçu local (16 scènes, pas une mesure) | la tour et les hiérarchies de points ne changent pas avec θ (mêmes meilleurs blocs, même compatibilité). À la coupe plate, mcs = K = 5, z = 1 : le nombre de clusters baisse avec θ ; l'IoU moyen passe de 0,69 (cover) et 0,75 (ER0h) à 0,80 pour θ ≥ 3/4, HDBSCAN 0,78. Les θ qui passent le juge (≤ 1/16) ne changent presque rien |
| La tension, forme causale | elle demeure, mais elle devient un curseur : le juge tel quel borne θ à 0,083 ; l'effet sur la coupe à mcs = K apparaît vers θ = 7/16 à 1/2 |
| Extension : validation rétroactive `R[θ]` (§ 10) | la maturité **valide** les clusters par cohortes de la projection, sans les retarder. Même arbre de clusters que `M[θ]` (**prouvé**). Les 125 jugements passent pour θ < 0,4963 (au lieu de 0,083). Prix : règle non causale ; écart à `M[0]` borné par θ (d_K − α_K) (**prouvé**), saut de 345 pour 1 mm sur la famille E2 |
| Où est le gain | à la **sélection**, pas dans la hiérarchie : `R[θ]` et `M[θ]` ont la même tour et le même cluster par point ; la coupe de `M[θ]` est celle de `R[θ]` avec des stabilités datées de la maturité (identité contrôlée, 1 152 cas). Hiérarchie `R[7/16]`, coupe de `M[7/16]` : 125 jugements (seuil 0,4963), et à l'aperçu 0,787 (`er0ha`) ou 0,796 (`er0h`) à z = 1, mcs = K, HDBSCAN 0,779. À mesurer |

## 2. Objets

- X : n sites entiers distincts (grille de 1 mm). K fixé. `L_K(r)` : points ayant au moins K sites à distance ≤ r.
- FULL_K : arbre de fusion des composantes de `L_K(r)`. Nœud v : naissance `b_v`, mort `d_v` (naissance du parent,
  infini à la racine). Niveaux : rayons carrés rationnels exacts. Coupe fermée : v vit à β si `b_v ≤ β < d_v`.
- `c_x(v)` : premier niveau de la vie de v où v couvre x. L'ensemble des nœuds qui couvrent x est clos vers le haut.
- α_K(x) : rayon de la plus petite boule fermée contenant x et K − 1 autres sites. d_K(x) : distance de x à son
  K-ième plus proche site, x compté. α_K ≤ d_K ≤ 2 α_K. À K = 2 : α_2 = d_2 / 2.
- **Projection** P : à chaque site x, une date d'entrée e(x) (rayon) et un nœud propriétaire o(x), vivant à e(x) et
  qui couvre x à e(x). La hiérarchie de points de P : x est seul avant e(x), puis suit les ancêtres de o(x).
- **Hauteur de réunion dans P** : u_P(x, y) = max(e(x), e(y), naissance du plus proche ancêtre commun de o(x) et
  o(y)) ; u_P(x, x) = e(x). C'est le premier rayon où x et y sont dans un même bloc.

| Projection | Date d'entrée | Propriétaire | Source |
| --- | --- | --- | --- |
| `cover` | α_K(x) | composante de la première boule couvrante (départage natif) | entrées natives de l'export |
| `er0h` | date à marge de ER0h(1, 12) | propriétaire de ER0h | règle du juge final (`verdict/er0h.py`, exact) ; `er0h_certifie` (rapide) |
| `er0ha` | la même que `er0h` | le même que `er0h` | idem ; seule la date de maturité change (§ 3.1) |

## 3. Forme (i) : maturité interpolée

### 3.1 Date de maturité

θ rationnel de [0, 1]. Dates en rayons.

| Projection | Date de maturité m(x) | Lecture |
| --- | --- | --- |
| `cover` | (1 − θ) α_K(x) + θ d_K(x) | interpolation entre première couverture et cœur |
| `er0h` | e(x) + θ (d_K(x) − e(x))₊ | lecture littérale de la consigne : interpolation à partir de la date d'entrée de la projection |
| `er0ha` | max(e(x), (1 − θ) α_K(x) + θ d_K(x)) | maturité **ancrée sur α_K** : date propre au site, plancher e(x) |

- Toujours e(x) ≤ m(x) ≤ max(e(x), d_K(x)). θ = 0 : m = e. θ = 1 : m = max(e, d_K).
- Pour cover les deux formules coïncident (e = α_K ≤ d_K).
- ER0h entre plus tard que cover (dates de 1 à 2,83 α). La lecture littérale retarde donc deux fois : § 7.
- **Nœud de maturité** μ(x) : ancêtre de o(x) vivant à m(x).

### 3.2 Taille mûre, tour condensée

- **Taille** d'un nœud v au niveau β de sa vie : s(v, β) = nombre de sites y avec μ(y) sous v (v compris) et
  m(y)² ≤ β. Un site ne compte que sur **une** lignée : celle de son propriétaire dans la projection.
- **Admission** a(v) : premier niveau de la vie de v où s(v, ·) ≥ mcs. Une égalité suffit. v est **grand** si a(v)
  existe. La racine est grande par convention (si n < mcs, elle est admise à sa naissance).
- Vraies scissions, têtes, clusters, absorption : **exactement** la tour condensée de
  `vote_condense_v2/CONCEPTION.md` § 3 (classe `Condense` importée). Une fusion est une vraie scission si elle a au
  moins deux enfants grands. Un cluster est la chaîne des grands nœuds de même tête. Un nœud petit est absorbé par
  le cluster de son premier ancêtre grand.
- **Naissance d'un cluster** : a(bas de sa chaîne) = max(naissance du nœud, mcs-ième plus petite date de maturité
  de sa lignée).

| HDBSCAN | cohortes sur la projection (θ = 0) | existence mûre |
| --- | --- | --- |
| taille = points de la composante | points entrés dans le bloc | points entrés **et mûrs** |
| un point compte dès d_K | dès e(x) | dès m(x) |
| un point est membre dès d_K | dès e(x), si le bloc a mcs points | dès e(x), si la lignée a mcs points mûrs |

### 3.3 Propriétaire et date d'un site

| Propriétaire | Règle | Nom |
| --- | --- | --- |
| **P1**, face unique de la projection | g(x) = premier ancêtre grand de o(x), lui compris. Date t(x) = max(e(x), a(g(x))). Ensuite x suit les ancêtres de g(x) | `M[θ,proj]` |
| vote W1 | routage descendant de `vote_condense_v2` sur CETTE tour : témoins forts, poids 1/r, majorité stricte, dénominateur `sc`, date `un` | `M[θ,proj,W1]` |
| vote T~1, marge c12 | temps de couverture, date `maj`, date à marge κ = 12 | `M[θ,proj,T~1+c12]` |

Pour un vote, la projection ne sert qu'à la taille (lignée et date de maturité).

### 3.4 Sorties

| Forme | Construction | Garantie |
| --- | --- | --- |
| (b) hiérarchie de points | date t(x) et nœud propriétaire, puis ancêtres | laminaire |
| `a1`, tour directe | clusters de la tour condensée, membres = points propriétaires | P1 : tout cluster a au moins mcs membres (P4) |
| `a2`, recondensée | cohortes sur (b), au même mcs | tout cluster a au moins mcs membres |

Formats : `condense_pr` / `tvp_core` (T = rank, parent, target, entry, weight ; `Condensed` ; `FlatTree`). Le juge
des fixtures lit (b) : à chaque rayon, clusters = blocs d'au moins mcs points.

### 3.5 Variantes

| Variante | Taille | Propriétaire | Pourquoi la garder |
| --- | --- | --- | --- |
| `M[θ,cover]` | lignée cover | cover | la moins chère : entrées natives seules. Hérite du pont court et des sauts de cover |
| `M[θ,er0h]` | lignée ER0h, interpolation depuis e | ER0h | lecture littérale de la consigne |
| `M[θ,er0ha]` | lignée ER0h, maturité ancrée sur α_K | ER0h | la meilleure sur les fixtures ; date de maturité indépendante du retard de la projection |
| `M[θ,cover,W1]` | lignée cover | vote W1 | analogue de `VC[coeur,W1]` avec une taille intermédiaire |
| `M[θ,cover,T~1+c12]` | lignée cover | vote T~1, marge | analogue de la variante VC retenue |

θ ∈ {0, 1/4, 1/2, 3/4, 1}, plus 1/16 et 1/8 dans le juge et l'aperçu : ce sont les deux valeurs dyadiques autour du
seuil des fixtures (0,083). Elles viennent des jugements ancrés, pas des étiquettes d'une scène.

Noms canoniques, lus par la chaîne de mesure : `VC[mur<θ>:<proj>,<votes>,abs,<décision>,<dénominateur>,<date>]`.
Exemple : `M[1/4,er0ha]` = `VC[mur1/4:er0ha,P1,abs,maj,sc,un]`.

Extension (§ 10) : `R[θ,proj]`, mêmes clusters que `M[θ,proj]`, dates de la projection.

## 4. Forme close : théorème F

**Théorème F (prouvé).** Soit n ≥ mcs, une projection P, des dates de maturité m ≥ e, et la variante P1. Alors

- t(x) = mcs-ième plus petite valeur, sur tous les sites y (x compris), de max(m(y), u_P(x, y)) ;
- g(x) = ancêtre de o(x) vivant à t(x) ;
- hauteur de réunion de x et y dans M[θ] : u_M(x, y) = max(u_P(x, y), t(x), t(y)).

*Preuve.* Notons N_x(β) le nombre de sites y avec max(m(y), u_P(x, y))² ≤ β.

1. **Lemme A.** Pour β ≥ e(x)², soit v l'ancêtre de o(x) vivant à β. Alors s(v, β) = N_x(β).
   Si m(y)² ≤ β et u_P(x, y)² ≤ β : l'ancêtre commun w de o(x) et o(y) est né avant β, donc w est sous v ; o(y) est
   sous v ; μ(y), vivant à m(y) ≤ β sur la lignée de o(y), est sous v : y est compté. Réciproquement, si μ(y) est
   sous v et m(y)² ≤ β, alors o(y) et o(x) sont sous v, donc w aussi et b_w ≤ b_v ≤ β ; e(y) ≤ m(y) ; e(x)² ≤ β :
   u_P(x, y)² ≤ β.
2. **Lemme B.** Soit β* le plus petit niveau où N_x ≥ mcs : c'est le carré de la mcs-ième plus petite valeur.
   Comme u_P(x, y) ≥ e(x), β* ≥ e(x)². Soit v* l'ancêtre de o(x) vivant à β*. Par le lemme A, s(v*, β*) ≥ mcs : v*
   est grand. Un nœud de la lignée strictement sous v* meurt avant β* ; sa taille vaut N_x < mcs sur sa vie à partir
   de e(x), et au plus s(o(x), e(x)) = N_x(e(x)) < mcs avant : il est petit. Donc g(x) = v*. Sur la vie de v*, à
   partir de e(x), la taille vaut N_x : a(v*) = β* si a(v*) ≥ e(x)² ; sinon v* = o(x), N_x(e(x)) ≥ mcs et β* = e(x)².
   Dans les deux cas max(e(x), a(g(x))) = β*.
3. **Ultramétrique.** u_M(x, y) = max(t(x), t(y), naissance de l'ancêtre commun de g(x) et g(y)). Cet ancêtre est
   celui de o(x) et o(y), ou bien g(x) ou g(y) lui-même, né au plus tard à sa propre admission, donc avant
   max(t(x), t(y)). ∎

*Lecture.* t(x) est une « distance de cœur » à mcs voisins, mesurée dans l'ultramétrique de la projection, où un
voisin y ne compte qu'une fois mûr. À θ = 0, m(y) = e(y) ≤ u_P(x, y) : t(x) est le rayon où le bloc de x atteint mcs
points. C'est la condensation par cohortes.

*Contrôle.* Tests : 6 480 dates et 3 000 hauteurs de réunion au moins, en rangs exacts (`test_formule_close…`).
Recoupe (`recus/recoupe_n300.json`) : la formule est un troisième chemin, indépendant des tours condensées :
1 296 000 dates, 0 désaccord. À l'échelle, elle sert de juge d'échantillon (README § 6).

## 5. Forme (ii) : maturité géométrique

**Définition.** τ ∈ [0, 1]. Le site x est mûr dans la composante C au niveau r si dist(x, C_r) ≤ τ r. τ = 1 :
couvert. τ = 0 : dans la composante. La taille s_τ(C, r) compte les sites mûrs. Elle croît le long d'une lignée
(C_r croît, τ r aussi). Elle ne dépend d'aucune projection. Un site peut être mûr dans plusieurs composantes.

**Ce qui est calculable** (`code/em_geo.py`, oracle borné).

| Objet | Calcul | Nature |
| --- | --- | --- |
| « x est mûr relativement à la K-partie S au niveau β = r² » | min sur y de max(max_s \|y − s\|² − β, \|y − x\|² − τ² β) ≤ 0. Tous les rayons carrés sont rationnels ; le minimiseur est rationnel (support d'au plus quatre contraintes, système linéaire) | décision **exacte** en Fraction |
| taille s_τ(C, β) à un niveau rationnel | réunion sur les K-parties dont la composante au niveau β est C | exacte, par K-parties exhaustives (n ≤ 12 environ) |
| date de maturité de x relativement à S | boule minimax pondérée min_y max(max_s \|y − s\|, \|y − x\| / τ). Son carré est racine d'un polynôme de degré 2 à coefficients rationnels (sphères d'Apollonius). Le rayon est un radical imbriqué | **encadrement** certifié par bissection (chaque étape est une décision exacte) ; forme close si K = 2 et x ∈ S : \|x s\| / (1 + τ) |
| date dans une lignée | min sur S de max(date relative à S, niveau où la composante de S rejoint la lignée) | encadrement |
| règle complète (tour condensée, juge) | les dates sortent de la classe « somme de racines de rationnels » des `Rayon` du juge | **non implémentée** : il faudrait une arithmétique de nombres algébriques |

**Bornes (prouvées).** d_K(x) / (1 + τ) ≤ date géométrique de x ≤ 2 α_K(x) / (1 + τ), la borne haute pour la lignée
de la première boule couvrante.

- Borne basse, toute composante. Si p ∈ L_K(r), la boule B(p, r) contient K sites, tous à moins de r + |x − p| de x.
  Donc d_K(x) ≤ r + |x − p| et dist(x, C_r) ≥ d_K(x) − r. Être mûr exige d_K − r ≤ τ r.
- Borne haute, lignée cover. La première boule couvrante B(c, α) contient x et K sites. Pour r ≥ α, tout point à
  moins de r − α de c est dans L_K(r), dans la composante de c. Donc dist(x, C_r) ≤ (2 α − r)₊.
- **Relation avec la forme (i).** Posons θ = (1 − τ) / (1 + τ). Les deux bornes deviennent (1 + θ) d_K / 2 et
  (1 + θ) α_K. La date interpolée (1 − θ) α_K + θ d_K est dans le même intervalle, de largeur (1 + θ)(α_K − d_K / 2).
- **K = 2.** α = d / 2 : l'intervalle est un point. Les deux formes coïncident pour la lignée cover :
  d_2 / (1 + τ) = (1 + θ) α_2.

*Contrôle.* `recus/geo.json` : 120 couples (site, τ) sur quatre nuages (K = 2 : 9 points ; K = 3 : 8 points),
τ ∈ {0, 1/8, 1/2, 1}. Aucune borne violée ; égalité des deux bornes aux 72 cas de K = 2 ; encadrements de largeur
relative 1e-12.

**Différence qui reste.** La forme (ii) compte un site dans toute composante dont il est proche. Sur le pont court à
mcs = 2, la lentille du pont porte C et D mûrs dès 1 700 / (1 + τ) : elle est grande, donc une vraie scission de
plus. Avec le propriétaire de ER0h elle n'a aucun membre ; la forme recondensée l'efface. Il faut de toute façon
une règle de propriétaire.

## 6. Propriétés

### P1. La taille croît le long d'une lignée : prouvé, contrôlé

Sur la vie d'un nœud, s(v, ·) croît par définition. À une fusion, tout site compté dans un enfant u (μ(y) sous u,
m(y)² < d_u) est compté dans le parent dès sa naissance b_v = d_u. Les sous-arbres des enfants sont disjoints. Donc
les grands nœuds sont clos vers le haut et a(parent) = b_parent dès qu'un enfant est grand : la condensation est
bien définie.

*Contrôle* (`recus/proprietes.json`, 8 nuages de 36 points, K = 2 et 3, mcs = 2, 3, 5, 8, trois projections, six θ :
1 008 cas). Admission recalculée par la définition sur 138 564 nœuds : 0 différence. Objet d'un descendant
postérieur à la naissance du parent : 0.

### P2. Laminarité : prouvée, contrôlée

Chaque point a une date et un nœud vivant à cette date, fixés une fois ; il suit ensuite les ancêtres (théorème T1 du
mémo d'ancrage). Contrôle : `Hierarchie.valider` sur chaque règle des invariants et du juge.

### P3. Tout bloc est dans l'amas discret de sa composante : prouvé, contrôlé

Si g(x) = o(x), le propriétaire couvre x depuis e(x) ≤ t(x) (propriété de la projection : c'est α_K pour cover, la
« NP ancrée » pour ER0h). Si g(x) est un ancêtre strict, il couvre x dès sa naissance, antérieure à son admission
donc à t(x). Pour les votes : preuve P2 de `vote_condense_v2`.

*Contrôle.* 497 640 contrôles (P2 et P3 ensemble) sur les 1 008 cas, votes compris : 0 violation.

### P4. Aucun cluster sous mcs membres

| Forme | Statut |
| --- | --- |
| `a2`, recondensée | garanti par la condensation par cohortes ; contrôlé : 6 122 clusters, 0 sous mcs |
| `a1`, tour directe, P1 | **prouvé** : a1 = a2. Avant a(v) aucun point n'est entré sous v. À a(v), les mcs sites mûrs de la lignée sont entrés (mûr implique entré). Un nœud petit n'a aucun membre : ses points appartiennent au premier ancêtre grand. Mêmes vraies scissions, mêmes clusters |
| `a1`, votes | **réfuté** : un vote envoie des points ailleurs |

*Contrôle.* P1 : formes canoniques a1 et a2 égales dans 1 008 cas sur 1 008. Votes, tour directe : W1, 335 clusters
sous mcs dont 27 vides, sur 6 122 ; T~1+c12, 27 sous mcs ; a1 = a2 dans 773 et 813 cas sur 1 008.

### P5. « Un point de bord seul ne crée pas de cluster »

Par le théorème F, un site y complète le compte de la lignée de x au rayon max(m(y), u_P(x, y)).

| Énoncé | Statut |
| --- | --- |
| θ = 1 : le site qui complète le compte est un site de cœur à ce rayon (m ≥ d_K) | prouvé |
| θ < 1 : un site couvert, pas encore de cœur, peut compléter le compte | **réfuté** comme garantie |
| cellule ancrée Q-Π2 (amas de 8 points, filament, x ; mcs = 9) | tenue pour tout θ et les trois projections : x appartient au filament |

Fixture du vérificateur, sans le filament (amas de 8 points, x à 900, un groupe lointain) : α(x) = 450, d_K(x) = 900.
Premier cluster de 9 points :

| θ | 0 | 1/8 | 1/4 | 1/2 | 3/4 | 1 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `M[θ,cover]`, rayon de naissance de {amas, x} | 453,471 | 506,25 | 562,5 | 675 | 787,5 | 900 |
| x est-il un site de cœur à cette date ? | non | non | non | non | non | oui |

Le cluster naît à max(453,471 ; (1 − θ) α + θ d_K). θ retarde le point de bord sans l'écarter. Seul θ = 1 l'écarte,
et il échoue alors les triangles, comme la taille `coeur`.

### P6. Les deux limites

**θ = 0 : c'est la condensation par cohortes de la projection. Prouvé.** m = e, donc s(v, β) est la masse du bloc de
v dans la hiérarchie de la projection. Par le théorème F, x entre au rayon où son bloc atteint mcs points : c'est le
rang de paiement des cohortes.

| Contrôle | Volume | Écarts |
| --- | --- | ---: |
| forme canonique de la tour de `M[0,proj]` contre `condense_pr.cohort_condense` de la projection, exact | 168 cas (8 nuages, K = 2 et 3, 4 mcs, 3 projections) | 0 |
| mêmes étiquettes plates que le témoin cover de la campagne et que l'arbre ER0h de `er0h_certifie` (EOM z = 1 et z = 2) | 32 unités de 300 points : 16 à K = 5, 8 à K = 2, 8 à K = 3 ; mcs = K et 10 | 0 |
| idem, plus les feuilles, mcs = K, 10, 20 ; et deux scènes de 2 000 points | `recus/limites.json` : 50 unités, 450 coupes contre cover, 450 contre l'arbre ER0h | 0 |
| dans la chaîne de mesure : `VC[poss,A1]` = cover (porte de `mvc_run`) et `M[0,cover]` = `VC[poss,A1]` | 2 unités (n = 300, n = 8 000) | 0 |

**θ = 1 : existence aux dates de cœur, propriétaire de la projection.** m = max(e, d_K). Pour tout θ, la tour
condensée a les mêmes clusters que la condensation par cohortes de `cdelay[θ]` (le site entre à sa maturité dans son
nœud de maturité) : la taille mûre est la masse de cette hiérarchie. Les appartenances diffèrent : un site couvert
tôt et mûr tard est membre du cluster fin dans M[θ], du cluster parent dans `cdelay[θ]`.

| Contrôle | Volume | Résultat |
| --- | --- | --- |
| mêmes clusters (niveaux de séparation, parents) que `cdelay[θ]` condensé, exact | 1 008 cas, six θ | 1 008 égaux |
| appartenances différentes de `cdelay[θ]` | les mêmes | 840 cas diffèrent : le contrôle n'est pas vide |
| `M[1,cover]` contre `tvp_core.tree_delay(…, 1)` de la campagne, scènes réelles | `recus/limites.json` : 150 tours, 3 445 clusters | mêmes clusters, mêmes niveaux de séparation : 150 sur 150 ; coupes plates différentes : 442 sur 450 |

Ce n'est **pas** la taille `coeur` de `vote_condense` : celle-ci compte un site dans la composante qui le contient ;
ici il compte dans la lignée de son propriétaire. Les deux nœuds diffèrent dès K = 3.

### P7. Continuité

**Théorème C (prouvé).** Soient deux configurations des mêmes sites. Si les hauteurs de réunion de la projection
diffèrent d'au plus δ (dates d'entrée comprises) et les dates de maturité d'au plus δ′, alors les dates t(x) et
toutes les hauteurs de réunion de M[θ] (P1) diffèrent d'au plus max(δ, δ′).

*Preuve.* Le maximum et une statistique d'ordre sont 1-lipschitziens pour la norme sup. Le théorème F écrit t(x) et
u_M comme maxima et statistique d'ordre des u_P(x, y) et des m(y). ∎

- m est 1-lipschitzienne en (e, d_K), ou en (e, α_K, d_K) pour `er0ha`. α_K est 1-lipschitzienne ; d_K l'est pour le
  déplacement d'un site (2-lipschitzienne si tous bougent). Donc δ′ ≤ max(δ, 2 ε) pour un déplacement ε.
- **L'étage d'existence n'ajoute aucune discontinuité.** M[θ] est continue partout où la projection l'est, avec le
  même module, borné inférieurement par 2 ε.
- Avec cover : la projection saute aux égalités de première couverture ; M[θ,cover] saute donc.
- Avec ER0h : la continuité de la projection est **observée**, non prouvée (module en racine, non lipschitzien :
  rapport de l'agent tiers, § 7.2). M[θ,er0h] et M[θ,er0ha] héritent de ce statut, rien de plus.
- La naissance d'un cluster est l'une des dates t(x) de ses membres : continue aux mêmes conditions. Quand elle
  rattrape la mort du cluster, le cluster disparaît à durée nulle ; les hauteurs de réunion restent continues
  (famille E1). L'**identité** des clusters et la coupe plate, elles, ne sont jamais continues : un seuil entier
  les sépare, comme dans HDBSCAN.
- Votes : le routage par majorité de `vote_condense` garde ses sauts ; la tour mûre n'y change rien.

*Contrôle* (`recus/proprietes.json`, exact ; hauteur de réunion de deux sites, en unités de grille ; saut = plus
grand écart entre deux configurations voisines de 1 mm).

| Famille | cover, P1 | ER0h, P1 (`er0h` et `er0ha`) | vote W1 | vote T~1+c12 |
| --- | ---: | ---: | ---: | ---: |
| Q7, mcs 2 : L et x | **4 608** (tout θ) | 12,01 (tout θ) | **4 608,5** | 12,01 |
| Q7, mcs 3 : Lf et L (naissance du cluster de gauche) | **4 608** à θ = 0 ; 3 840 ; 2 560 ; 1 280 ; 1 à θ = 1 | 12,01 à 1 | **3 840** sur lignée cover ; 12,01 sur lignée `er0ha` | **3 840** sur lignée cover ; 12,01 sur lignée `er0ha` |
| CF1 : x et a | 0,46 | 0,46 | **287,8** | **486,3** |
| CF2 : x et a | **947,8** | 6,51 | 0 | 0 |
| CF3 : x et m | **250** | 6,51 | 0 | **250** |
| E1 : A et B (la naissance du cluster croise sa mort) | 0,5 à 1 | 0,5 à 1 | 0,5 à 1 | 0,5 à 1 |

Lecture, composante par composante :

| Composante de la règle | Saute-t-elle ? |
| --- | --- |
| date de maturité (interpolation de α_K, d_K, e) | non : lipschitzienne |
| naissance d'un cluster, à lignées fixées | non (théorème C ; famille E1) |
| lignée de comptage = propriétaire **cover** | **oui** : Q7, CF2, CF3. À Q7 et mcs 3, x passe d'un côté à l'autre ; le cluster de gauche perd son troisième site mûr et n'existe plus. Le saut vaut (fusion − maturité de x) ; il s'éteint à θ = 1 |
| lignée de comptage = propriétaire **ER0h** | non sur ces familles : pas de 12,01 au plus, soit κ fois le déplacement. Le site change bien de lignée à l'égalité, mais sa date tend alors vers la fusion |
| routage par vote W1 | **oui** : Q7, CF1 |
| routage par vote T~1, date `maj` à marge | **oui** : CF1, CF3 |

Réponse à la question posée : remplacer le vote par le propriétaire de la projection ER0h supprime les sauts de Q7 et
des trois contre-fixtures. Avec cover ils restent, et un vote sur une tour comptée par la lignée cover saute aussi
(Q7 à mcs 3), par la taille et non par le routage.

Les lignes `cover_A1` et `ER0h[1,12]` du reçu sont les hiérarchies **sans** condensation ; leurs versions condensées
sont `M[0,cover]` et `M[0,er0h]`.

## 7. Les 125 jugements ancrés

Juge du verrou importé tel quel (`cellules_natif.py`, par `vc_juge` de `vote_condense_v2`) ; seule la fabrique de
hiérarchies reçoit la famille `mur`. 69 règles (6 témoins, 63 variantes). Empreinte `8543573d1ac32ca7`, identique
pour la référence exacte en normal, sous `-O`, et pour le chemin rapide. Reçus : `recus/juge_famille_*.json`.

Le juge classe chaque échec : **retard** (la cible devient conforme à un rayon postérieur à la borne gauche ; retard
relatif r0 / a − 1) ou **structure** (la fenêtre n'est jamais conforme).

### 7.1 Témoins et propriétaire P1

| Règle | T0 | Q1 | Q1bis | Q2 | Q3 | Q-Π2 | Q4 | Total | Retards (max) | Structure |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| `cover_A1` (témoin) | 14/40 | 5/10 | 0/10 | 5/5 | 25/25 | 5/5 | 20/30 | 74 | 0 | 51 |
| `core` (témoin) | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 0 | 80 |
| `ER0h[1,12]` (témoin) | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 30/30 | 125 | 0 | 0 |
| `VC[coeur,W1]` (témoin) | 0/40 | 5/10 | 0/10 | 0/5 | 0/25 | 5/5 | 10/30 | 20 | 0 | 105 |
| `VC[couv,W1]` (témoin) | 40/40 | 10/10 | 10/10 | 0/5 | 0/25 | 0/5 | 10/30 | 70 | 0 | 55 |
| `VC[couv,T~1,abs,maj,sc,maj+c12]` (témoin) | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 30/30 | 125 | 0 | 0 |
| `M[0,cover]` | 14/40 | 5/10 | 0/10 | 5/5 | 25/25 | 5/5 | 20/30 | 74 | 0 | 51 |
| `M[1/16,cover]` | 14/40 | 5/10 | 0/10 | 5/5 | 25/25 | 5/5 | 20/30 | 74 | 0 | 51 |
| `M[1/8,cover]` | 14/40 | 5/10 | 0/10 | 5/5 | 25/25 | 5/5 | 10/30 | 64 | 10 (2,9 %) | 51 |
| `M[1/4,cover]` | 14/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 12/30 | 61 | 15 (11,5 %) | 49 |
| `M[1/2,cover]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 20/30 | 55 | 27 (28,8 %) | 43 |
| `M[3/4,cover]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 20/30 | 55 | 10 (46,0 %) | 60 |
| `M[1,cover]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 0 | 80 |
| `M[0,er0h]` | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 30/30 | **125** | 0 | 0 |
| `M[1/16,er0h]` | 40/40 | 10/10 | 5/10 | 5/5 | 25/25 | 5/5 | 20/30 | 110 | 15 (4,5 %) | 0 |
| `M[1/8,er0h]` | 35/40 | 10/10 | 5/10 | 5/5 | 25/25 | 5/5 | 20/30 | 105 | 20 (9,1 %) | 0 |
| `M[1/4,er0h]` | 5/40 | 10/10 | 5/10 | 0/5 | 25/25 | 5/5 | 20/30 | 70 | 55 (18,2 %) | 0 |
| `M[1/2,er0h]` | 0/40 | 10/10 | 5/10 | 0/5 | 25/25 | 5/5 | 12/30 | 57 | 66 (36,6 %) | 2 |
| `M[3/4,er0h]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 9 (5,2 %) | 71 |
| `M[1,er0h]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 0 | 80 |
| `M[0,er0ha]` | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 30/30 | **125** | 0 | 0 |
| `M[1/16,er0ha]` | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 30/30 | **125** | 0 | 0 |
| `M[1/8,er0ha]` | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 20/30 | 115 | 10 (2,9 %) | 0 |
| `M[1/4,er0ha]` | 40/40 | 10/10 | 5/10 | 0/5 | 25/25 | 5/5 | 20/30 | 105 | 20 (11,5 %) | 0 |
| `M[1/2,er0ha]` | 0/40 | 10/10 | 5/10 | 0/5 | 25/25 | 5/5 | 20/30 | 65 | 58 (29,9 %) | 2 |
| `M[3/4,er0ha]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 20/30 | 55 | 20 (46,0 %) | 50 |
| `M[1,er0ha]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 0 | 80 |

### 7.2 Propriétaire par vote sur la tour mûre

| Règle | θ = 0 | 1/16 | 1/8 | 1/4 | 1/2 | 3/4 | 1 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `M[θ,cover,W1]` | 57 | 57 | 57 | 52 | 25 | 20 | 20 |
| `M[θ,cover,T~1+c12]` | 107 | 107 | 97 | 87 | 60 | 55 | 45 |
| `M[θ,er0h,W1]` | 75 | 70 | 65 | 35 | 30 | 20 | 20 |
| `M[θ,er0h,T~1+c12]` | 125 | 110 | 105 | 70 | 57 | 45 | 45 |
| `M[θ,er0ha,W1]` | 75 | 75 | 75 | 70 | 30 | 20 | 20 |
| `M[θ,er0ha,T~1+c12]` | 125 | 125 | 115 | 105 | 65 | 55 | 45 |

- `M[0,cover,W1]` = `VC[poss,W1]` (57) et `M[0,cover,T~1+c12]` = `VC[poss,T~1,…,maj+c12]` (107) : mêmes totaux que
  `vote_condense_v2`.
- Sur la lignée ER0h, le vote T~1+c12 donne les mêmes totaux que le propriétaire P1, à chaque θ. Il coûte plus cher
  et saute (P7). Il n'apporte rien ici.
- Le vote W1 garde ses échecs de structure : Q2 (deux faces plus lointaines battent la plus proche), Q3 (l'amas offre
  plus de faces que le filament), Q4.

### 7.3 Mécanisme de chaque échec

| Échec | Observé | Mécanisme |
| --- | --- | --- |
| Q4, θ ≥ 1/8, toutes projections | retard : conforme dès 891,2 (θ = 1/8), 965,9 (1/4), 1 115,4 (1/2), 1 264,8 (3/4) ; borne gauche 866,025 | la fenêtre commence à la naissance des tétraèdres ; les faces PQR ont α = 816,5 et d_K = 1 414,2 : elles ne sont mûres qu'à 816,5 + 597,7 θ |
| Q1bis, mcs 2, θ ≥ 1/4 (`er0ha`) | retard : conforme dès 1 250 (θ = 1/4), 1 500 (1/2) ; borne gauche 1 154,684 | la fenêtre commence à la naissance du triangle (rayon circonscrit) ; A et B sont mûrs à 1 000 (1 + θ) |
| Q1bis, `er0h`, tout θ > 0,001 | retard de 4,5 % dès θ = 1/16 | ER0h fait entrer A et B à 1 153,9, juste avant le triangle ; l'interpolation part de là |
| Q2, θ ≥ 1/4 | retard : {x, a} conforme dès 62,5 ; borne gauche 61 | α = 50, d_K = 100 : mûrs à 50 (1 + θ). À θ = 1/2 la paire naît à 75, fin de la fenêtre |
| T0, θ ≥ 1/2 (`er0ha`), θ ≥ 1/4 (`er0h`) | retard : triangles dès 1 500 (θ = 1/2) ; borne gauche 1 300 | α = 1 000, d_K = 2 000 |
| T0, θ = 3/4 | structure pour le juge : rien avant 1 700, fin de la fenêtre | les triangles naissent à 1 750, avant la fusion (1 931,8), mais hors fenêtre |
| T0, Q1, Q1bis, θ = 1 | structure : aucun triangle | maturité = cœur (2 000) après la fusion : l'échec de la taille `coeur` |
| T0, Q1, Q1bis avec cover, tout θ | structure : C D groupés, « clusters attendus réunis » | cover donne C et D au pont : échec connu de cover, que la maturité ne corrige pas |
| Q4 avec cover, θ = 1/4 | 8 échecs de structure | lecture stricte : deux de C, m, D groupés à la naissance des tétraèdres |
| Q2, Q3 avec W1 | structure : {b1, b2, x}, {c0…c7, x} | décompte de faces (`vote_condense_v2` § 7) |

### 7.4 Seuils exacts en θ

Méthode : balayage θ = k / 64, puis bissection de chaque transition par la règle exacte en des θ rationnels (largeur
2⁻⁴⁰) ; forme close cherchée parmi les sites. 47 seuils encadrés. Reçu : `recus/seuils.json`.

Lecture **J** : le juge tel quel (les cinq variantes de la fixture, toutes ses fenêtres). Lecture **S** : sur chaque
variante, les deux triangles sont deux clusters de composition exacte à un rayon antérieur à la fusion.

| Cellule | mcs | `er0ha`, J | `er0ha`, S | `er0h`, J | `er0h`, S | Forme close du seuil J de `er0ha` |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| T0, arêtes courtes (pont = côté) | 2 et 3 | 0,300000 | 0,931827 | 0,1727 et 0,1718 | 0,9195 et 0,9194 | (1 300 − 1 000) / (2 000 − 1 000) |
| T0, pont court de 2 mm | 2 et 3 | 0,300000 | 0,930861 | 0,1727 et 0,1721 | 0,918334 | idem |
| T0, plan à égalités | 2 et 3 | 0,299480 | 0,896608 | 0,2995 et 0,0665 | 0,8966 et 0,8632 | (1 300 − 1 000,4) / (2 000,8 − 1 000,4) |
| T0, 3D équilatéral | 2 et 3 | 0,300000 | 0,732051 | 0,1719 et 0,1717 | 0,6830 et 0,6829 | (919,239 − 707,107) / (1 414,214 − 707,107) |
| Q1 (pont de 1 700), fenêtre stricte | 3 | 0,697536 | 0,787360 | 0,642557 | 0,748724 | (1 697,536 − 1 000) / (2 000 − 1 000) |
| Q1bis (pont de 1 700) | 2 | 0,154709 | 0,787360 | 0,000936 | 0,748724 | (1 154,684 − 999,978) / (1 999,956 − 999,978) |
| Q2 | 2 | 0,207921 | — | 0,207921 | — | (61 − 50,5) / (101 − 50,5) |
| Q4 | 2 et 3 | 0,082863 | — | 0 | — | (866,025 − 816,497) / (1 414,214 − 816,497) |
| Q3, Q-Π2 | toutes | tout θ | — | tout θ | — | |

- Forme générale : θ* = (a − s) / (d_K − s). a : borne gauche de la fenêtre (J) ou rayon de la fusion (S). s : départ
  de l'interpolation, α_K pour `er0ha` et cover, date d'entrée pour `er0h`. d_K : celui du site qui lie.
- **Borne d'interpolation de la consigne** (d = 2 000, α = 1 000, fusion à 1 787) : (1 787,36 − 1 000) / 1 000 =
  0,78736. Retrouvée : lecture S de Q1 et Q1bis.
- **Tous les jugements passent si et seulement si θ ≤ 0,082863** (`er0ha`). Avec la lecture littérale `er0h` : θ = 0
  seulement (Q4 dès θ > 0, Q1bis dès 0,000936).
- cover : aucun seuil pour T0, Q1, Q1bis, Q4 ; ces cellules échouent déjà à θ = 0.

Forme (ii), K = 2, lignée cover : date d / (1 + τ).

| Fixture | Fusion | d_K d'un sommet | Seuil τ (structure) | θ équivalent | Contrôle par l'oracle |
| --- | ---: | ---: | ---: | ---: | --- |
| Q1, pont de 1 700 | 1 787,360 | 1 999,956 | **0,11894** | 0,78740 | à τ = 0,121 le nœud du triangle porte A, B, C mûrs juste sous la fusion ; à τ = 0,116 aucun |
| T0, arêtes courtes | 1 931,827 | 1 999,956 | 0,03527 | 0,93187 | idem, τ = 0,038 et 0,033 |
| T0, pont court | 1 930,861 | 1 999,956 | 0,03578 | 0,93090 | idem |
| T0, plan à égalités | 1 897,367 | 2 000 | 0,05409 | 0,89737 | idem, τ = 0,057 et 0,052 |
| T0, 3D équilatéral | 1 224,745 | 1 414,214 | 0,15470 | 0,73205 | idem, τ = 0,157 et 0,152 |

Le θ équivalent égale le seuil S de `er0ha` sur la variante de base ; le seuil S du tableau précédent est le minimum
sur les cinq variantes. Pour les fenêtres du juge : τ ≥ d / a − 1, soit 0,538 (T0), 0,732 (Q1bis), 0,178 (Q1).

### 7.5 Ce que disent ces seuils

Les fenêtres du juge commencent près de la première couverture : à la naissance de la composante qui couvre le
groupe (Q4, Q1bis) ou à 1,3 α (T0). Elles encodent « un groupe de mcs points **couverts** est un cluster ». Tout
retard d'existence les viole, par construction. La lecture « avant la fusion » de la consigne est bien plus large :
θ < 0,787.

## 8. Mutants

| Mutant | Effet | Tué par |
| --- | --- | --- |
| `date_entree` | la date de maturité devient la date d'entrée (θ = 0) | cellules ancrées : 105 → 125, 55 → 74, 115 → 125 (code 4) ; fixture isocèle, chemins exact et rapide |
| `date_coeur` | la date de maturité devient la date de cœur (θ = 1) | cellules ancrées : 105 → 45, 55 → 45, 115 → 45 (code 4) ; fixture isocèle |
| `appartenance_tardive` | le site n'entre qu'à sa maturité, dans son nœud de maturité (c'est `cdelay[θ]`) | fixture isocèle, exact et rapide. **Non tué par les cellules ancrées** : mêmes totaux et mêmes cellules (code 0, publié) |
| `taille_couverte` | la taille compte les sites couverts | cellules ancrées : 105 → 125, 55 → 74, 115 → 125 (code 4) ; fixture isocèle |
| `sans_borne`, `sans_carre` (numériques) | le flottant décide seul ; le repli compare des flottants | cas gravé de contact (niveaux 1 ; 16 + 2⁻⁶⁰ ; 16 + 2⁻⁵⁹ ; 64) |
| portes de la recoupe : `noeud`, `date`, `maturite` | un point corrompu dans la sortie rapide | la recoupe rend le code 1 |
| juge d'échantillon : date décalée d'un rang | une date corrompue dans la sortie rapide | `em_formule` rend le code 1 |
| `admission_mure`, `sans_validation`, `date_par_noeud` (extension R) | § 10.4 et 10.6 | cellules ancrées (code 4), fixtures isocèle et E2, définition relue |
| portes sans mode mutant intégré (propriétés, limites, aperçu, forme (ii)) | chaque porte est rejouée saine puis sous mutant | `em_portes.py` : README § 6 |

Fixture isocèle gravée (K = 2, mcs = 2, θ = 1/2, cover) : paire A B de côté 2 000, C au sommet, paire lointaine D E.
Maturités : A et B à 1 500, C à 2 019,4. États condensés attendus : D E à 225 ; A B C à **1 500** ; tout à 9 581,9.
C, couvert depuis 1 346,3, est membre dès 1 500. Les quatre mutants donnent quatre suites différentes de celle-ci.

Les 125 jugements ne distinguent donc pas l'appartenance précoce de l'appartenance tardive. Les scènes, elles, le
font : 840 cas sur 1 008 (P6), et le rappel du meilleur bloc (à mesurer).

## 9. Limites

| Sujet | État |
| --- | --- |
| Fixtures contre mcs = K, forme causale | aucune valeur de θ ne passe le juge tel quel **et** ne change la coupe à mcs = K : le juge borne θ à 0,083 ; l'aperçu ne bouge qu'à partir de 1/4 à 1/2. Suite : § 10 |
| Continuité avec ER0h | héritée d'une propriété observée, non prouvée, de ER0h |
| Forme (ii) | oracle de taille et encadrements seulement ; règle complète non implémentée (nombres algébriques) |
| `er0ha` | variante ajoutée ici, hors de la consigne littérale ; motivée par T0 à θ = 1/4 (5/40 contre 40/40) |
| Point de bord | la maturité le retarde, ne l'écarte pas (P5) |
| Votes | ils gardent les sauts et les échecs de `vote_condense` ; sur la lignée ER0h, T~1+c12 ne change aucun total |
| Aperçu | 16 scènes de 300 points : un mécanisme, pas une mesure ; aucune affirmation de supériorité |
| Lemme L0 de `vote_condense` | non utilisé ici : la taille mûre est additive, son admission se lit sans hypothèse de position générale |

## 10. Extension : validation rétroactive `R[θ, projection]`

Hors de la consigne littérale. Motif : § 7.5 et § 9, première ligne. La forme causale retarde l'existence, et le juge
refuse tout retard. Ici la maturité ne retarde rien : elle **valide**. Code : `code/em_retro.py`.

### 10.1 Définition

Même projection (o, e) et mêmes dates de maturité m que `M[θ,proj]`.

- **Taille d'entrée** s₀(v, β) : nombre de sites y avec o(y) sous v et e(y)² ≤ β. Admission a₀(v). Nœud **grand par
  entrée**. C'est la taille mûre à θ = 0 : la tour condensée par entrée est celle de `M[0,proj]`. Ses clusters sont les
  clusters par cohortes de la projection, ici « clusters d'entrée ».
- Un nœud est grand par entrée si et seulement s'il possède mcs sites sous lui : un site possédé entre avant la mort
  de son propriétaire. a₀(v) = max(b_v, mcs-ième plus petite date d'entrée sous v).
- Un cluster d'entrée C, de bas w et de tête h, est **valide** si h est grand par maturité : sa lignée porte mcs sites
  mûrs avant sa mort. Forme close : la mcs-ième plus petite date de maturité des sites possédés sous h précède d_h.
- **Propriétaire** g′(x) : premier ancêtre de o(x), lui compris, qui appartient à un cluster d'entrée valide.
  **Date** t(x) = max(e(x), a₀(g′(x))). Ensuite x suit les ancêtres de g′(x).

Lecture : `R[θ]` est la condensation par cohortes de la projection, moins les clusters qui meurent avant de porter
mcs sites mûrs. Leurs points attendent la fusion avec un cluster valide. À θ = 0 rien n'est retiré.

| | `M[0,proj]` | `R[θ,proj]` | `M[θ,proj]` |
| --- | --- | --- | --- |
| clusters | par cohortes de la projection | les mêmes, **validés** par la maturité | ceux de la taille mûre |
| arbre de clusters | celui de θ = 0 | celui de `M[θ]` | celui de `M[θ]` |
| date d'un membre | t₀ | t₀ si son cluster d'entrée est valide | max(e, a) : attend la maturité de la lignée |
| causale | oui | **non** | oui |
| continuité | celle de la projection | écart borné à `M[0]` : θ (d_K − α_K) | celle de la projection |

Noms : `R[θ,proj]` = `VC[retro<θ>:<proj>,P1,abs,maj,sc,un]`. Propriétaire de la projection seulement.

### 10.2 Propriétés

**Lemme R0.** s ≤ s₀ : un site mûr sous v est entré sous v (o(y) est sous μ(y), e ≤ m). Donc grand par maturité
implique grand par entrée, et a₀ ≤ a. Les ancêtres d'un nœud grand par maturité le sont (P1) : les clusters d'entrée
au-dessus d'un cluster valide sont valides, et la racine l'est dès que n ≥ mcs. g′(x) existe.

Notons g₀(x) le propriétaire dans `M[0]` (premier ancêtre grand par entrée) et g(x) celui dans `M[θ]`. Sur la lignée
de o(x) : g₀(x), puis g′(x), puis g(x), de bas en haut (g(x) est dans un cluster valide).

**R1 (prouvé). Même arbre de clusters que `M[θ]`.**

1. Les nœuds de la lignée strictement sous g(x) ne sont pas grands par maturité : dans la tour de `M[θ]` ils sont
   absorbés par le cluster de g(x). Donc x a le même cluster propriétaire dans `R[θ]` et dans `M[θ]`. Mêmes masses.
2. Blocs de la hiérarchie de points de `R[θ]`. Soit u un nœud d'un cluster valide. À a₀(u), les mcs sites entrés sous
   u sont membres du bloc de u : pour un tel y, g′(y) est sous u ; si g′(y) = u, t(y) = a₀(u) ; sinon
   t(y) < d_{g′(y)} ≤ b_u. Donc tout bloc non vide a au moins mcs points. Un nœud hors de tout cluster valide a un
   bloc vide.
3. Une fusion p réunit deux blocs non vides si et seulement si deux enfants de p sont dans des clusters valides.
   Ces deux enfants sont alors grands par entrée : p est une vraie scission d'entrée, ses enfants grands par entrée
   sont des têtes, et une tête est dans un cluster valide si et seulement si elle est grande par maturité. Les
   vraies scissions de la forme recondensée sont donc celles de `M[θ]`. ∎

**R2 (prouvé). Encadrement.** t₀(x) ≤ t_R(x) ≤ t_M(x). Si le cluster d'entrée de x est valide, t_R(x) = t₀(x).
*Preuve.* Si g′ = g : a₀(g) ≤ a(g). Sinon t_R < d_{g′} ≤ b_g ≤ t_M. Si g₀ = g′ les dates coïncident ; sinon
t₀ < d_{g₀} ≤ b_{g′} ≤ t_R. ∎

**R3 (prouvé). Laminarité, mcs membres, a1 = a2.** t(x) est dans la vie de g′(x) ; x suit ensuite les ancêtres : la
hiérarchie est laminaire, et g′(x) couvre x à t(x) (comme P3). Par R1, points 2 et 3 : aucun bloc non vide sous mcs
points, et la tour directe (arbre de `M[θ]`, dates de `R[θ]`) est la forme recondensée.

**R4 (prouvé). Un cluster d'entrée non valide est bref.** Soit C un cluster d'entrée de bas w et de tête h, et Δ le
mcs-ième plus petit écart d_K(y) − α_K(y) parmi les sites entrés sous w à a₀(w). Si C n'est pas valide, sa durée
d_h − a₀(w) est au plus θ Δ.
*Preuve.* Soit Y l'ensemble des mcs sites de plus petit écart parmi ceux-là. Pour y dans Y : e(y) ≤ a₀(w) et
m(y) − e(y) ≤ θ (d_K(y) − α_K(y)), pour les trois projections (e ≥ α_K). Donc la mcs-ième plus petite maturité sous
h est au plus a₀(w) + θ Δ. Si elle précédait d_h, C serait valide. ∎

Conséquences :

- **`R[θ]` reste près de `M[0]`.** t_R(x) − t₀(x) est au plus la somme des durées des clusters d'entrée non
  valides emboîtés au-dessus de x : chacune au plus θ Δ, et Δ ≤ α_K ≤ rayon de naissance du cluster. Un cluster
  d'entrée qui vit plus de θ Δ est gardé avec ses dates.
- **`R[θ]` n'est pas continue.** Quand la mcs-ième maturité d'un cluster croise sa mort, le cluster disparaît alors
  qu'il avait une durée non nulle : ses points passent de t₀ à la fusion. Le saut est au plus θ Δ. `M[θ]` n'a pas ce
  saut (théorème C) ; `M[0]` non plus. Famille E2 ci-dessous.
- **`R[θ]` n'est pas causale.** L'état à un rayon dépend de ce qui arrive ensuite, jusqu'à la mort du cluster. C'est
  le propre d'une sélection, non d'une hiérarchie.
- **Le point de bord (P5) n'est plus retardé.** Dans `R[θ]` un site compte dès son entrée ; la maturité exige
  seulement que le cluster vive jusqu'à la maturité de son mcs-ième site. Sur la figure sans filament du § P5,
  {amas, x} naît à 453,471 pour tout θ et les trois projections (`M[θ,cover]` : 506,25 à 900). La cellule ancrée
  Q-Π2 reste tenue pour tout θ. Reçu : `recus/retro_bord.json`.

### 10.3 Contrôles des propriétés

`recus/retro_proprietes.json` : 8 nuages de 36 points, K = 2 et 3, mcs = 2, 3, 5, 8, trois projections, six θ.
1 008 cas, 36 288 points. Référence exacte.

| Propriété | Contrôle | Résultat |
| --- | --- | --- |
| R1 | cluster propriétaire égal à celui de `M[θ]` ; nœud propriétaire sur la lignée de celui de `M[θ]` | 0 écart |
| R1 | squelette (niveaux de fusion, parents) et masses de la tour émise égaux à ceux de `M[θ]` | 1 008 sur 1 008 |
| R1, θ = 0 | dates et nœuds de `R[0]` égaux à ceux de `M[0]` | 0 écart |
| R2 | t₀ ≤ t_R ≤ t_M, en exact | 0 violation ; 12 584 points avancés sur `M[θ]`, 1 348 retardés sur `M[0]` |
| R2 | cluster d'entrée valide : date et nœud de `M[0]` | 34 940 points, 0 écart |
| R3 | laminarité, couverture, admission : 203 667 contrôles | 0 violation |
| R3 | forme recondensée : 6 122 clusters ; tour directe = forme recondensée | 0 sous mcs ; 1 008 sur 1 008 |
| R4 | durée d'un cluster d'entrée non valide ≤ θ Δ, signes exacts de sommes de racines | 452 clusters non valides, 0 hors borne (6 424 valides) |
| définition relue | troisième chemin, sans tour condensée : date et nœud de chaque point | 36 288 points, 0 désaccord |
| mutants | `admission_mure` rend `M[θ]` ; `sans_validation` rend `M[0]` ; `date_par_noeud` est entre R et M | 1 008 sur 1 008 chacun ; différents de la règle saine dans 751, 209 et 582 cas |

Chemin rapide contre référence exacte, et juge d'échantillon à l'échelle : README § 6.

### 10.4 Les 125 jugements ancrés

Juge importé tel quel, comme au § 7. 22 règles. Empreinte `5475161113357e5d`, identique pour la référence exacte en
normal, sous `-O`, et pour le chemin rapide. Reçus : `recus/retro_juge_*.json`.

| Règle | T0 | Q1 | Q1bis | Q2 | Q3 | Q-Π2 | Q4 | Total | Retards | Structure |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `R[θ,er0ha]`, θ = 1/16, 1/8, 1/4, 3/8 | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 30/30 | **125** | 0 | 0 |
| `R[1/2,er0ha]` | 40/40 | 10/10 | 10/10 | 3/5 | 25/25 | 5/5 | 30/30 | 123 | 0 | 2 |
| `R[3/4,er0ha]` | 30/40 | 10/10 | 10/10 | 0/5 | 25/25 | 5/5 | 30/30 | 110 | 0 | 15 |
| `R[1,er0ha]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 0 | 80 |
| `R[θ,er0h]`, θ = 1/16, 1/8, 1/4, 3/8 | 40/40 | 10/10 | 10/10 | 5/5 | 25/25 | 5/5 | 30/30 | **125** | 0 | 0 |
| `R[1/2,er0h]` | 40/40 | 10/10 | 10/10 | 3/5 | 25/25 | 5/5 | 30/30 | 123 | 0 | 2 |
| `R[3/4,er0h]` | 30/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 29/30 | 94 | 0 | 31 |
| `R[1,er0h]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 0 | 80 |
| `R[θ,cover]`, θ = 1/16, 1/8, 1/4, 3/8 | 14/40 | 5/10 | 0/10 | 5/5 | 25/25 | 5/5 | 20/30 | 74 | 0 | 51 |
| `R[1/2,cover]` | 14/40 | 5/10 | 0/10 | 3/5 | 25/25 | 5/5 | 20/30 | 72 | 0 | 53 |
| `R[3/4,cover]` | 10/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 20/30 | 65 | 0 | 60 |
| `R[1,cover]` | 0/40 | 5/10 | 0/10 | 0/5 | 25/25 | 5/5 | 10/30 | 45 | 0 | 80 |

- **Aucun retard**, à aucun θ. Tous les échecs sont de structure : le cluster attendu n'est pas valide, il fusionne
  avant de porter mcs sites mûrs. C'est R2 : un cluster valide garde les dates de la projection.
- Avec cover, les échecs de θ ≤ 3/8 sont ceux de cover elle-même (74) : la validation ne corrige pas une projection.
- θ = 1 : 45, comme `core` et `M[1]`. Un cluster n'existe que si mcs de ses sites deviennent des sites de cœur avant
  sa mort.

Seuils exacts (même méthode qu'au § 7.4 ; 27 seuils encadrés ; `recus/retro_seuils.json`) : plus grand θ pour lequel
tous les jugements de la cellule passent.

| Cellule | mcs | `R[θ,er0ha]` | `R[θ,er0h]` | Forme close pour `er0ha` |
| --- | ---: | ---: | ---: | --- |
| T0, arêtes courtes | 2 et 3 | 0,931827 | 0,919476 et 0,919400 | (1 931,827 − 1 000) / (2 000 − 1 000) |
| T0, pont court de 2 mm | 2 et 3 | 0,930861 | 0,918334 | (1 930,861 − 1 000) / (2 000 − 1 000) |
| T0, plan à égalités | 2 et 3 | 0,896608 | 0,896608 et 0,863155 | (1 897,367 − 1 000,4) / (2 000,8 − 1 000,4) |
| T0, 3D équilatéral | 2 et 3 | 0,732051 | 0,683013 et 0,682944 | (1 224,745 − 707,107) / (1 414,214 − 707,107) |
| Q1 et Q1bis (pont de 1 700) | 3 et 2 | 0,787360 | 0,748724 | (1 787,360 − 1 000) / (2 000 − 1 000) |
| Q2 | 2 | **0,496279** | **0,496279** | (75,562 − 50,5) / (101 − 50,5) |
| Q4 | 2 et 3 | 0,864699 | 0,756185 et 0,744896 | non identifiée parmi les candidats |
| Q3, Q-Π2 | toutes | tout θ | tout θ | |

- Forme générale : θ* = (fusion − s) / (d_K − s). Ce sont les seuils de la lecture « structure » du § 7.4 : `R[θ]`
  passe une cellule dès que ses clusters sont validés. La borne d'interpolation de la consigne (0,78736) est le seuil
  de Q1.
- **Les 125 jugements passent si et seulement si θ < 0,496279**, pour `er0h` comme pour `er0ha`. La cellule qui lie
  est Q2 : la paire {x, a} (α = 50,5 ; d_K = 101) fusionne à 75,562, soit 1,496 α.
- La forme causale exigeait θ ≤ 0,082863 (§ 7.4). cover : échec dès θ = 0.
- Sur la grille dyadique : θ = 3/8 est la plus grande valeur k / 8 qui passe ; 7/16 la plus grande k / 16.

Mutants, par les cellules ancrées (`recus/retro_juge_mutant_*.json`, code 4 : tués) :

| Mutant | `R[1/2,er0ha]` | `R[3/4,er0ha]` |
| --- | ---: | ---: |
| règle saine | 123 | 110 |
| `admission_mure` (c'est `M[θ]`) | 65 | 55 |
| `sans_validation` (c'est `M[0]`) | 125 | 125 |
| `date_par_noeud` (première forme, § 10.6) | 113 | 100 |

### 10.5 Continuité : famille E2

Famille E2, gravée dans `em_retro.py` (K = 2, mcs = 3). Triangle A B C de côté 2 000 : α = 999,98, d_K = 1 999,96,
lentilles réunies à 1 154,68. Amas de cinq sites à la verticale du centre, à la hauteur Z. La fusion du triangle
avec l'amas croît avec Z ; la maturité du triangle vaut 999,98 (1 + θ). Quand la fusion précède la maturité, le
triangle n'est pas valide.

| θ | Z, avant et après | réunion de A et B, `R[θ]` | saut de `R[θ]` pour 1 mm | borne θ (d_K − α_K) | saut de `M[θ]` |
| --- | --- | --- | ---: | ---: | ---: |
| 1/4 | 1 914 et 1 915 | 1 249,655 puis 1 154,684 | **94,97** | 250,0 | 0,36 |
| 3/8 | 2 245 et 2 246 | 1 374,688 puis 1 154,684 | **220,00** | 375,0 | 0,39 |
| 1/2 | 2 553 et 2 554 | 1 499,726 puis 1 154,684 | **345,04** | 500,0 | 0,42 |
| 3/4 | 3 133 et 3 134 | 1 749,797 puis 1 154,684 | **595,11** | 750,0 | 0,44 |

- Le saut vaut (fusion − naissance du cluster) ; la fusion égale alors la maturité, 999,98 (1 + θ), à 1 mm près.
  Il respecte la borne de R4.
- `M[0]` ne saute pas (1 154,684 des deux côtés). `M[θ]` passe continûment de la fusion à la maturité.
- Sur Q7, CF1, CF2, CF3 et E1, `R[θ]` a exactement les sauts de sa projection, pour θ = 1/4, 3/8, 1/2, 3/4 : 4 608,
  947,8 et 250 avec cover ; 12,01 et 6,51 avec ER0h ; 0,46 ; 0,5. La validation n'y ajoute rien.

Reçu : `recus/retro_proprietes.json`. La porte exige le saut de `R[θ]` et l'absence de saut de `M[θ]`.

### 10.6 Première forme essayée : la date par nœud

Première tentative : garder g(x), le propriétaire de `M[θ]`, et le dater de a₀(g(x)). Défaut : toute fusion change
de nœud, même avec une composante qui ne possède presque rien, et la date repart de la naissance du nouveau nœud.
Fixture isocèle (K = 2, mcs = 2, θ = 1/2, cover) : la lentille {A, B}, née à 1 000, rejoint à 1 450 la lentille de C,
qui ne possède qu'un site. A et B sont mûrs à 1 500 : le nœud {A, B} n'est pas grand par maturité, son parent l'est.
La date par nœud donne {A, B, C} à 1 450 ; la validation par cluster garde {A, B} depuis 1 000, comme `M[0]`.
Cette forme est gardée comme **mutant** (`date_par_noeud`) : tuée par la fixture isocèle et par les cellules
ancrées, où elle perd dix jugements (113 et 100 au lieu de 123 et 110, § 10.4).

### 10.7 Ce que l'extension change

| Constat | Source |
| --- | --- |
| `R[θ]` passe les 125 jugements pour θ < 0,4963, sans aucun retard | § 10.4 |
| `R[θ]` a l'arbre de clusters de `M[θ]` et le même cluster par point | R1 ; 1 008 cas exacts ; 1 152 cas sur scènes réelles |
| La tour et les hiérarchies de points portent la même information pour tout θ, `M` comme `R` | aperçu, étages A, B, O (README § 7.1 et 7.2) |
| La coupe plate de `R[θ]` (EOM de la campagne) reste celle de la projection : 0,747 à 0,752 pour θ ≤ 1/2, contre 0,746 à θ = 0 | aperçu, z = 1, mcs = K |
| La coupe plate de `M[θ]` bouge : 0,764 à θ = 3/8, 0,787 à 7/16, 0,792 à 1/2 (`er0ha`) ; 0,792, 0,796, 0,801 (`er0h`) ; HDBSCAN 0,779 | aperçu, z = 1, mcs = K |
| À z = 2 et mcs = K l'effet est plus lent : 0,713 à θ = 7/16, HDBSCAN 0,779 | aperçu |

**Lecture.** `M[θ]` et `R[θ]` ne diffèrent que par les dates. L'excès de masse pèse chaque membre depuis sa date
d'entrée dans le cluster. Avec les dates de la projection (R), un petit cluster né tôt reste stable et il est
retenu. Avec les dates de maturité (M), sa stabilité baisse et son parent l'emporte. Le gain de la forme causale à
mcs = K est donc un effet de **sélection**. Il n'est pas dans la hiérarchie.

**Identité (prouvée par R1, contrôlée).** Une antichaîne de clusters étiquette les points par le cluster
propriétaire de chacun. `R[θ]` et `M[θ]` ont le même arbre et le même cluster par point. Donc la sélection calculée
avec les stabilités de `M[θ]`, appliquée à la tour de `R[θ]`, rend exactement la coupe plate de `M[θ]`.
Contrôle : `recus/apercu_ordonne_K5.json`, 16 scènes, mcs = K et 10, trois projections, six θ, z = 1 et 2 : 1 152
cas sur 1 152 (mêmes parents, mêmes masses, même cluster par point, mêmes étiquettes).

**Ce que cela permet.** Découpler les deux exigences, sur un seul objet, la tour condensée de θ :

| Exigence | Objet | État |
| --- | --- | --- |
| hiérarchie de points : existence précoce, jugements ancrés | `R[θ,er0h]` ou `R[θ,er0ha]`, θ < 0,4963 | 125 sur 125 |
| coupe plate à mcs = K : moins de petits clusters | sélection par stabilités mûres = coupe de `M[θ]`, même θ | effet à l'aperçu dès θ = 3/8 (`er0h`) ou 7/16 (`er0ha`) ; **à mesurer** |

θ = 7/16 est la plus grande valeur k / 16 sous le seuil 0,4963 des jugements : elle vient des cellules ancrées, pas
des étiquettes d'une scène. Rien ici n'est une mesure : 16 scènes de 300 points. La chaîne de mesure prend les deux
familles par leur nom (README § 5).

Ce qui reste vrai : `R[θ]` n'est pas causale et saute de θ (d_K − α_K) au plus quand un cluster cesse d'être valide.
La coupe par stabilités mûres hérite du défaut de z = 2 à mcs = K. Aucune des deux ne corrige une projection : avec
cover, les échecs de cover demeurent.
