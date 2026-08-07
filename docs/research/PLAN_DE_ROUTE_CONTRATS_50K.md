# Plan de route vers les contrats 50 000 points

Statut : plan. Aucun claim, aucune porte ouverte ou fermée. Toutes les grandeurs
citées sont **mesurées**, avec leur provenance ; les extrapolations sont
annoncées comme telles.

Contrats visés, tels que posés : nuage de **50 000 points**, exhaustif et exact,
**1 s** (contrat A) et **100 ms** (contrat B), à **$K=10$**, ou $K=5$ si $K=10$
est hors de portée.

---

## 1. Le contrat, traduit en nanosecondes par record

Un étage ne se juge pas à sa vitesse absolue mais à son **coût par record de
sortie**, parce que la sortie est ce que le contrat livre.

À 50 000 points :

| grandeur | valeur | provenance |
| --- | ---: | --- |
| univers des paires $\binom{n}{2}$ | 1 249 975 000 | exact |
| univers higher $\binom{n}{3}+\binom{n}{4}$ | $2{,}604\cdot10^{17}$ | exact |
| sortie utile $N_{\text{out}}(50\,000, K{=}10)$ | $\approx 1{,}8\cdot10^{7}$ (360/point) | recensement exhaustif, deux familles convergentes |
| sélectivité brute | $6{,}9\cdot10^{-11}$ | quotient des deux |
| candidats après germination locale certifiée | $4{,}4\cdot10^{8}$ | soit **24,4 candidats par record** |

D'où les budgets :

| | par record, mono-thread | par record, 48 cœurs idéaux | par candidat germé, 48 cœurs |
| --- | ---: | ---: | ---: |
| **contrat A (1 s)** | 55,6 ns | **2 667 ns** | **109 ns** |
| **contrat B (100 ms)** | 5,6 ns | **267 ns** | **11 ns** |

Ces quatre nombres sont la grille de lecture de tout ce qui suit.

---

## 2. Où se trouve chaque étage, mesuré

| étage | coût mesuré à 50 000 points | ramené à 48 cœurs | écart au contrat A |
| --- | ---: | ---: | ---: |
| amont algorithmique (canonicalisation + LBVH) | 18,4 ms | 18,4 ms | **tient** |
| lanceur paire natif, rang 11 | 2,434 s | 2,434 s (GPU) | **2,4 ×** |
| recertification paire (1,08 µs/record, mono-thread) | 8,628 s | 0,180 s | **tient une fois parallélisée** |
| recherche higher | **ne termine pas** | — | ∞ |
| classification de la sortie, borne optimiste | 1 183 s | 24,6 s | **25 ×** |
| classification, borne pessimiste (par candidat germé) | 28 908 s | 602 s | 602 × |
| aval, fermeture de descente de facette | $1{,}105\cdot10^{6}$ s | $2{,}3\cdot10^{4}$ s | **$1{,}1\cdot10^{6}$ ×** |

Provenance : lanceur et recertification, session G4 du 7 août ; amont, run Q1′ ;
recherche higher, zéro événement en 240 s à $n=400$ et $n=1000$ ; classification,
65,7 µs par requête closed-ball indexée mesurée à $n=4096$ — **un plancher**, la
requête croît avec le nuage ; aval, 61,4 ms par événement à $n=16$ avec une
constante qui croît (11,6 ms à $n=8$, 21,9 ms à $n=18$) — **un plancher aussi**.

La parallélisation de l'aval sur 48 cœurs n'est pas acquise ; elle est portée ici
pour montrer qu'elle ne suffirait pas.

---

## 3. Les cinq verrous, par écart décroissant

**V1 — L'aval, $1{,}1\cdot10^{6}\times$.** Quadratique en nombre d'événements
(exposants 2,10 et 1,81, deux nuages indépendants) et de constante croissante en
$n$. Ce n'est pas un verrou d'optimisation : même rendu parfaitement linéaire et
parfaitement parallèle, il coûterait $2{,}3\cdot10^{4}$ s. Il demande une **borne
de travail** — donc une conception, pas un réglage.

> **Ce que le profil a changé à ce diagnostic.** À $n=40$, univers fermé,
> l'aval coûte 10 217 ms pour **663 nœuds de fermeture** — 15,4 ms le nœud.
> La fermeture n'est donc **pas grosse, elle est lente** : la borne
> structurelle de 1 048 576 nœuds n'est jamais approchée. Un profil callgrind
> à $n=16$ nomme le coût, et ce n'est pas la science :
>
> | poste (inclusif) | part du run |
> | --- | ---: |
> | `verify_exact_direct_saddle_arm_seed_journal_streaming` (rejeu de vérification) | 34,6 % |
> | `CanonicalSha256Builder::update` | 21,7 % |
> | `ExactRational::canonical_key()` (rendu **décimal** des rationnels) | 14,3 % |
> | constructeur du reducer, ~tout en `build_..._forest_source_manifest` | 11,8 % |
> | constructeur de l'exécuteur de lots | 11,6 % |
> | boost multiprecision (gcd, resize, divide, multiply), exclusif | ~45 % |
>
> La chaîne chaude est **rationnel non borné → chaîne décimale → SHA-256**, et
> c'est la couche de **provenance**. Deux médecines déjà éprouvées dans ce
> dépôt s'y appliquent telles quelles : celle de R1-c/R1-d (déterminants
> entiers au lieu de rationnels normalisés, 864 × sur l'étage higher) et celle
> de R1 tout court (remplacer un rejeu de vérification par des invariants
> $O(\text{sortie})$ — ici les 34,6 % du rejeu de journal de graines).
>
> Premier acompte pris et mesuré : le SHA-256 vidé de sa boucle octet par
> octet et de son brassage de registres rend `reducer_setup` **1,48 ×**,
> `reducer_stream` **1,37 ×** et le run entier **1,25 ×**, à **empreintes
> bit-à-bit identiques** (zéro champ scientifique différent, vecteurs connus
> verts).
>
> V1 reste le verrou dominant et garde son ordre de grandeur, mais il cesse
> d'être opaque : il est fait de trois postes nommés et chiffrés, dont deux
> ont déjà leur remède dans l'histoire du dépôt.

**V2 — La recherche higher, non terminante.** C'est le seul verrou dont la
solution est **déjà démontrée et pas câblée** : la germination locale certifiée
ramène $2{,}604\cdot10^{17}$ à $4{,}4\cdot10^{8}$ candidats, et le budget devient
109 ns par candidat sur 48 cœurs. Un rejet de candidat en arithmétique entière
filtrée est à cette échelle-là. C'est le meilleur rapport valeur/risque du plan.

**V3 — La classification de la sortie, $25\times$ au mieux.** 65,7 µs par requête
closed-ball contre 2 667 ns de budget par record. Indépendant de V2 : même avec
une recherche gratuite, classifier la sortie coûte 24,6 s.

**V4 — Le lanceur paire, $2{,}4\times$.** 74 ns par visite de nœud sur un GPU à
188 SM. Q2 a établi que **le coût n'est pas le travail** : à compteurs de travail
bit-à-bit identiques, le temps de lanceur varie d'un facteur 3,07. Il y a donc de
la marge, et elle est dans l'occupation.

**V5 — La recertification paire.** Résolue sur le papier : strictement linéaire,
1,08 µs par record, mono-thread, massivement parallélisable. 8,628 s → 0,180 s.

---

## 4. Verdict honnête sur les deux contrats

**Contrat A (1 s, $K=10$) : atteignable seulement si V1 est reconçu.** Aucune
suite d'optimisations locales n'y mène. Même V2, V3, V4 et V5 tous résolus,
l'aval seul coûte $2{,}3\cdot10^{4}$ s dans son meilleur cas. V1 est une
condition **nécessaire**, et c'est une preuve à faire, pas un code à écrire.

**Contrat B (100 ms) : hors de portée à $K=10$ avec une sortie de
$1{,}8\cdot10^{7}$ records.** Il faudrait 267 ns par record sur 48 cœurs pour
**tous les étages réunis**, quand la seule classification en coûte 65 700. Le
contrat B n'est pas une version plus rapide du contrat A : c'est un autre
problème, et il exige de réduire la sortie ou de changer ce qu'on classifie.

**Le seul levier qui déplace tous les budgets à la fois est $K$.** La session du
7 août a chiffré $K$ sur l'étage paire — $K=10$ coûte 2,03 fois $K=5$ — mais
**pas sur $N_{\text{out}}$**, qui est le dénominateur de toute la grille du §1.
Or le recensement exhaustif existe déjà et publie l'histogramme du rang fermé
observé : il rend $N_{\text{out}}(K)$ pour tout $K$ **en une exécution locale**.
C'est pourquoi c'est la première action du plan : elle re-chiffre tout le reste.

---

## 5. Les incréments, dans l'ordre d'exécution

Chacun porte une condition d'entrée, un critère de sortie **falsifiable**, et le
lieu de sa mesure. Les incréments locaux ne consomment pas de VM.

### I0 — Chiffrer $N_{\text{out}}(K)$ par le recensement · **RENDU**

Les recensements exhaustifs à $n = 32, 64, 128$ sur les deux familles portent
déjà l'histogramme du rang fermé observé ; $N_{\text{out}}(K)$ s'en déduit sans
nouvelle exécution. Ajustement en loi de puissance sur $n=32 \to 128$, moyenne
des deux familles :

| $K$ | $N_{\text{out}}(50\,000)$ | budget/record, contrat A, 48 cœurs | contre $K{=}10$ | écart des deux familles |
| ---: | ---: | ---: | ---: | ---: |
| 2 | $2{,}53\cdot10^{5}$ | 189 398 ns | 70,3 × moins | **13,8 ×** |
| 3 | $5{,}48\cdot10^{5}$ | 87 536 ns | 32,5 × | 4,9 × |
| 4 | $1{,}42\cdot10^{6}$ | 33 750 ns | 12,5 × | 2,1 × |
| **5** | $2{,}21\cdot10^{6}$ | **21 680 ns** | **8,05 ×** | 1,5 × |
| 6 | $4{,}09\cdot10^{6}$ | 11 726 ns | 4,35 × | 1,4 × |
| 8 | $1{,}01\cdot10^{7}$ | 4 738 ns | 1,76 × | 1,2 × |
| **10** | $1{,}78\cdot10^{7}$ | **2 694 ns** | 1,00 × | 1,1 × |

L'écart entre familles est la mesure d'incertitude de l'extrapolation : il vaut
13,8 × à $K=2$ et **1,1 × à $K=10$**. Les valeurs à bas $K$ sont donc des ordres
de grandeur, celles à haut $K$ sont fiables.

**La décision $K$ est prise, et elle est mesurée.** Passer de $K=10$ à $K=5$
divise la sortie par **8,05** alors que l'étage paire ne coûte que **2,03** fois
moins. Le dénominateur commande : c'est la sortie, pas l'étage paire, qui fixe
tous les budgets.

Grille complète des écarts au contrat A, recalculée, tous étages sur 48 cœurs :

| $K$ | recertification | classification | lanceur paire | **aval** |
| ---: | ---: | ---: | ---: | ---: |
| 5 | 0,101 s (**0,10 ×**) | 3,03 s (3,0 ×) | 3,100 s (3,1 ×) | 2 832 s (**2 832 ×**) |
| 10 | 0,182 s (**0,18 ×**) | 24,4 s (24 ×) | 7,480 s (7,5 ×) | 22 790 s (**22 790 ×**) |

À $K=5$, **tous les étages sauf l'aval tiennent à un facteur 3 près du contrat
A**, et le lanceur y est donné dans sa version pessimiste : le rang 6 mesuré à
3,100 s l'a été en régime soutenu, et le rapport isolé/soutenu de 3,07 établi par
Q2 le ramène à **≈1,0 s**.

**Et l'aval reste le contrat à tout $K$** :

| $K$ | 2 | 3 | 4 | 5 | 6 | 8 | 10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| aval, 48 cœurs | 324 s | 701 s | 1 819 s | 2 832 s | 5 236 s | 12 960 s | 22 790 s |
| écart contrat A | 324 × | 701 × | 1 819 × | 2 832 × | 5 236 × | 12 960 × | 22 790 × |

Même à $K=2$ — un ordre où la sortie est 70 fois plus petite — l'aval est encore
**324 fois** au-dessus. Aucun choix de $K$ ne le sauve. C'est la justification
quantitative de V1 : l'aval n'est pas un étage à optimiser, c'est le contrat.

**Conclusion d'I0, portée au plan** : viser **$K=5$** pour le contrat A. Ce n'est
plus une capitulation devant $K=10$ mais un arbitrage chiffré — 8,05 × de sortie
en moins contre 2,03 × de coût paire en plus. $K=10$ redevient une cible dès que
l'aval et la classification passent l'échelle.

### I1 — Rendre le coupe-circuit effectif dans l'étage higher · *local*

Entrée : rien.
Défaut établi, et plus grave que ce que la session avait dit : le quantum
d'avance n'est borné par **aucun budget configuré**. À $n=40$, $K=3$, backend
hôte, un délai annoncé de 5 000 ms rend :

| profil | budget de travail | arrêt réel | dépassement |
| --- | ---: | ---: | ---: |
| `unbudgeted_industrial` | plafond représentationnel | 45 354 ms | 9,07 × |
| `bounded` | défaut | 46 445 ms | 9,29 × |
| `bounded` | `--support-work-budget 100000` | 43 106 ms | 8,62 × |
| `bounded` | `--support-work-budget 1000` | 44 068 ms | 8,81 × |

Le budget n'a **aucun effet** : une unité de travail interne à
`append_next_internal_chunk` n'est bornée par rien. La boucle du runner teste
pourtant le délai à chaque tour.
Travail : porter le garde-fou **par appel** dans la boucle d'avance hôte, sur le
modèle exact de R2-c/R2-h côté device (hors config scellée, testé entre unités,
censure comptée et marquée à part).
Sortie falsifiable : le délai est honoré à une unité de travail près, et la
taille de cette unité est **déclarée dans le rapport**. Test : à budget de
travail décroissant, le dépassement décroît proportionnellement.
Pourquoi d'abord : sans lui, **toute mesure bornée de l'étage higher est sans
valeur**, et chaque minute de G4 dépensée dessus est perdue.

### I2 — La germination locale certifiée · **MESURÉE, ET RÉFUTÉE À L'ÉCHELLE**

La restriction certifiée existe déjà dans le générateur : $D \le 2R(p)$ sous le
jeu de 26 directions de rayon de couverture prouvé 27,569276°. Elle a été
mesurée, à $n=512$, $K=10$, sur les triples :

| famille | directions | paires retenues | candidats |
| --- | ---: | ---: | ---: |
| uniform_latin | 0 | 47 783 / 130 816 | 480 847 |
| uniform_latin | **26** | 42 164 / 130 816 | **404 713** (−16 %) |
| eight_clusters | 0 | 130 617 / 130 816 | 18 143 332 |
| eight_clusters | **26** | **130 617 / 130 816** | **18 143 332** (−0 %) |

**La restriction certifiée retire 16 % sur nuage uniforme et exactement rien sur
nuage aggloméré.** La raison est structurelle : $R(p)$ est un maximum sur les
orientations de boules passant par $p$, et pour un point au bord d'un amas une
boule qui s'étend vers le vide est énorme tout en restant pauvre. Le chiffre de
$4{,}4\cdot10^{8}$ candidats venait du cutoff **non certifié**, et la route
certifiée ne le reproduit pas : sur `eight_clusters` les candidats valent 81,6 %
de $\binom{512}{3}$.

**Une route de remplacement a été essayée et rejetée pour cause d'exactitude.**
Construire les triples sur la sortie de l'étage paire — déjà exacte et complète à
50 000 points en 2,434 s — supposerait que les arêtes d'un triple accepté soient
des arêtes de Gabriel d'ordre borné. La mesure dit que c'est presque vrai et pas
vrai : à $K=10$, 99,0 % des arêtes ont un rang fermé $\le 11$ en uniforme, 86,9 %
en aggloméré, mais la queue monte à 16 et 38. Et la géométrie explique
exactement pourquoi il ne peut pas en être autrement : pour un triangle acutangle
de circumcentre $O$ et de rayon $R$, la boule diamétrale d'un côté est contenue
dans $B(O, \sqrt2 R)$ et **jamais** dans $B(O,R)$ — son point le plus éloigné est
à $R(\cos\gamma + \sin\gamma)$, qui vaut $R\sqrt2$ au pire et $1{,}366\,R$ même en
se restreignant à l'arête du diamètre. Un pré-filtre par rang d'arête est donc
**prouvablement incomplet**, quel que soit le seuil.

**Ce qui reste, et c'est la seule route exacte connue** : produire les supports
depuis la **structure d'ordre $k$** au lieu de filtrer l'univers. Un support
minimal bien centré de rang fermé $\rho$ est, par définition même, un simplexe de
**Gabriel d'ordre $\rho - |S|$** ; l'ensemble cherché est donc exactement les
triangles de Gabriel d'ordre $\le K-2$ et les tétraèdres d'ordre $\le K-3$. Cette
reformulation ne change pas l'objet — elle change le coût, en le rendant
proportionnel à la **sortie** ($1{,}8\cdot10^{7}$ à $K=10$, $2{,}2\cdot10^{6}$ à
$K=5$) et non à l'univers ($2{,}6\cdot10^{17}$), soit **dix ordres de grandeur**.
Corroboration déjà au dépôt : à l'ordre 0, 334 979 tétraèdres de Delaunay mesurés
à 50 000 points, et 8,36 triangles de Gabriel par point mesurés contre 9,5
projetés par le recensement — 13 % d'écart.

Le dépôt ne contient aucune construction de Delaunay. C'est le travail à faire,
et c'est un travail de fond, pas un câblage.

### I3 — Paralléliser la recertification paire · *local, mesuré G4*

Entrée : rien (indépendant).
Chiffré : 8,628 s → 0,180 s sur 48 cœurs, 54 % du coût de l'étage paire au
rang 11.
Sortie falsifiable : digests de sortie **identiques** à la version mono-thread,
et courbe de passage à l'échelle publiée de 1 à 48 cœurs.

### I4 — Le lanceur paire : instrumenter l'occupation · *local, mesuré G4*

Entrée : rien.
Q2 a montré que le coût n'est pas le travail (facteur 3,07 à travail identique).
L'instrument manque : un profil publiant **nombre de lancements** et **temps par
lancement**, séparément du temps passé dans les portes. Il se construit
localement — le plan de session interdisait de l'improviser sur VM facturée.
Sortie falsifiable : la variance de 3,07 s'explique par une grandeur publiée, et
une borne de visites de nœud par seconde par SM est établie.

### I5 — Le coût de classification par record · *local*

Entrée : I0 (pour savoir combien de records il y en a).
65,7 µs par requête closed-ball contre 2 667 ns de budget. La cible n'est pas un
facteur mais une conception : classifier en lot, réutiliser la descente entre
records voisins, ou déplacer la classification sur le device
(`terminal_classification_native_cuda` est encore faux).
Sortie falsifiable : coût par record mesuré sur la **vraie** sortie à 50 000
points, et non sur une requête isolée.

### I6 — La borne de travail de l'aval · *local, conception*

Entrée : I2 (l'aval n'a pas d'entrée réelle avant).
C'est V1, et c'est le seul incrément qui est d'abord une **preuve**. Objet :
borner le travail de la fermeture de descente de facette par une fonction de la
**sortie**, pas de l'entrée. Tant que cette borne n'existe pas, l'exposant 2,10
mesuré interdit le contrat.
Sortie falsifiable : la borne est énoncée, puis confrontée à nuage **croissant**
— c'est exactement la mesure qui a montré que la constante croît, et elle doit
cesser de croître.
Repli si I2 tarde : nourrir l'aval d'une entrée d'événements **synthétique** à
l'échelle. Le mécanisme existe pour le réducteur de hiérarchie de points
(2 792 ms sur une tour synthétique de 50 000 points) mais **pas** pour cette
fermeture ; le construire est un travail à part entière.

### I7 — Substituer le lanceur natif à la session paire hôte du runner · *local*

Entrée : I3 (sinon la recertification mono-thread annule le gain).
Q3 a qualifié la substitution au rang du contrat ; elle n'est pas câblée. Tant
qu'elle ne l'est pas, aucune question sur l'aval n'est atteignable à 50 000
points par le pipeline.
Sortie falsifiable : le runner rend à 50 000 points la **même** partition que la
qualification autonome — mêmes 7 962 604 candidats, même masse élaguée, même
somme égale à l'univers.

---

## 6. Ordre, et pourquoi

```
I0 (chiffrer K) ──┬── I1 (coupe-circuit) ── I2 (germination) ── I6 (borne de l'aval)
                  │                              │
                  ├── I5 (classification) ───────┘
                  │
                  ├── I3 (recertification //) ── I7 (substitution runner)
                  │
                  └── I4 (occupation lanceur)
```

I0 d'abord parce qu'il re-chiffre la grille. I1 avant toute mesure de l'étage
higher. I2 est le cœur. I3, I4, I7 sont indépendants et peuvent avancer en
parallèle. I6 est le plus long et le plus incertain, et il conditionne le contrat
A : **le commencer tôt**, même sans entrée réelle.

---

## 7. Jalons de mesure G4

Ne monter sur la VM que ce qui ne se mesure pas localement, et jamais un réglage
déjà fermé.

| jalon | question | condition d'entrée |
| --- | --- | --- |
| G-1 | la germination fait-elle fermer l'univers higher à 50 000 points, et en combien ? | I2 vert localement, I1 armé |
| G-2 | la recertification parallèle tient-elle 0,180 s à 48 cœurs ? | I3 vert localement |
| G-3 | le runner substitué rend-il la partition de Q3 à 50 000 points ? | I7 vert localement |
| G-4 | d'où vient le facteur 3,07 du lanceur ? | I4 instrumenté |

Discipline de taille scellée, rappelée : **tout petits nuages, puis directement
50 000 points, puis dizaines de millions si utile — rien entre.**

---

## 8. Ce qui ne doit pas être ré-exploré

- Les réglages de tuile de l'étage higher (T1, T2 : cible d'expansion, racines
  par tuile, raffinement de frontière). Mesuré et fermé : le coût est
  proportionnel au travail exploré, invariant sous partitionnement.
- Baisser $K$ pour débloquer l'étage paire hôte. Réfuté le 7 août : le mur est
  indépendant de l'ordre.
- Le filtre fp64 devant les portes du moteur higher. Réfuté par la mesure, et
  Q2 explique pourquoi : le coût n'est pas l'arithmétique.
- Les tailles de nuage intermédiaires. Interdites par la discipline scellée.

---

# 9. Le facteur manquant : d'où il vient et où le prendre

## 9.1 Pourquoi c'est lent sur une G4 — la réponse est mesurée

Trois faits, chacun mesuré, se composent.

**La G4 est inutilisée à 98 %.** Le pipeline exact ne porte **aucun
multithread** : il tourne sur un cœur sur quarante-huit. Le noyau higher est à
un thread par slot pour une capacité de 1 024, soit **au plus 8 blocs sur les
188 SM** du Blackwell. La machine n'est pas lente ; on ne lui demande presque
rien.

**Des instructions qu'on exécute, 92 % ne sont pas de la géométrie.** Profil
callgrind d'un univers fermé à $n=16$, coûts exclusifs :

| catégorie | part | plafond d'Amdahl |
| --- | ---: | ---: |
| rationnel non borné (boost multiprecision) | **49,01 %** | 1,96 × |
| churn d'allocation de `cpp_int` | 18,86 % | 1,23 × |
| SHA-256 canonique | 17,47 % | 1,21 × |
| rendu texte / chaînes | 6,84 % | 1,07 × |
| **géométrie et reste** | **7,81 %** | — |
| provenance seule (SHA + texte) | 24,31 % | 1,32 × |
| rationnel + son churn | 67,88 % | 3,11 × |
| **les deux ensemble** | **92,19 %** | **12,80 ×** |

**Et le coût d'un nœud de fermeture est linéaire en $n$.** C'est le fait
décisif, et il n'avait jamais été isolé parce qu'on regardait le coût par
*événement* :

| $n$ | événements | nœuds | nœuds/événement | ms/nœud |
| ---: | ---: | ---: | ---: | ---: |
| 12 | 12 | 119 | 9,92 | 3,68 |
| 16 | 27 | 213 | 7,89 | 4,29 |
| 20 | 36 | 294 | 8,17 | 6,41 |
| 24 | 43 | 379 | 8,81 | 7,57 |
| 28 | 48 | 446 | 9,29 | 8,40 |

Le nombre de nœuds par événement est **constant** — la fermeture est déjà
locale, elle ne grossit pas. Toute la croissance est dans le coût d'un nœud, en
$n^{0{,}974}$, de pente **0,318 ms par nœud et par point**. L'ordonnée à
l'origine n'est pas résolue à ces tailles (l'ajustement la rend négative), donc
le plancher indépendant de $n$ est **inférieur à la résolution** de la mesure et
devra être remesuré.

Ce terme linéaire n'est pas de la géométrie : c'est le **re-digest du manifeste**
— qui hache le nuage — et le **rejeu du journal de graines**, appelé depuis huit
sites distincts, chacun replaçant `..._freshly_replayed = true`.

À 50 000 points cela donne 15 900 ms par nœud, dont **100 %** de terme linéaire.
Le chiffre honnête pour l'aval devient donc bien pire que les 2 832 × annoncés
plus haut, qui reposaient sur une constante par événement mesurée sur de tout
petits nuages : à $K=5$, $9 \times 15\,900\ \text{ms} \times 2{,}21\cdot10^{6}
= 3{,}16\cdot10^{8}$ s, soit $6{,}6\cdot10^{6}$ s sur 48 cœurs.

## 9.2 Le budget de facteurs

Le déficit est de $6{,}6\cdot10^{6}$ sur 48 cœurs à $K=5$. Voici ce qui est
disponible, chaque ligne étant mesurée ou déjà obtenue dans ce dépôt.

| levier | facteur | statut |
| --- | ---: | --- |
| **A. Supprimer le terme $O(n)$ par nœud** (manifeste mémoïsé, journal vérifié une fois) | jusqu'à $\sim n$ | structurel, la pente 0,318 ms/nœud/point le mesure |
| **B. Paralléliser sur les événements** | 48 × | la fermeture est locale et par événement : indépendante par construction |
| **C. Cure R1-d sur ce qui reste** (déterminants entiers, plus de normalisation) | ≤ 12,8 × | plafond d'Amdahl mesuré ; 864 × déjà obtenu sur la requête closed-ball |
| **D. Occupation GPU** | ~20 × | 8 blocs sur 188 SM aujourd'hui |

**A est le facteur cent — et davantage.** C'est le seul qui soit structurel :
B, C et D sont des constantes, A est un ordre. Composés, B × C valent 614 ×, ce
qui ne suffit jamais seul ; A × B × C suffit dès que le plancher par nœud tombe
sous la milliseconde.

Chiffrage, en prenant pour le plancher la borne supérieure que la mesure
autorise (0,3 ms par nœud, non résolu, donc à remesurer) :

| étape | aval à $K=5$, sur 48 cœurs |
| --- | ---: |
| aujourd'hui | $6{,}6\cdot10^{6}$ s |
| **A** — plus de terme $O(n)$ | 124 s |
| **A + C** | 9,7 s |
| **A + C + D** | **0,49 s** — contrat A tenu |

La conclusion à retenir est que **l'aval n'est pas un problème d'algorithme** :
sa fermeture est déjà locale et de taille constante. C'est un problème de
redondance de provenance, et il se paie une fois par nœud et par point.

## 9.3 Ce que cela ne règle pas

L'étage higher reste entier. Sa recherche explore $2{,}6\cdot10^{17}$ candidats
pour $2{,}2\cdot10^{6}$ sorties, et **aucun des quatre leviers ci-dessus n'y
change un ordre** : 614 × ne rattrape pas $10^{10}$. Pour lui, la seule route
exacte connue reste la construction d'ordre $k$ du §I2 — produire les simplexes
de Gabriel d'ordre $\le K-2$ au lieu de filtrer l'univers.

Le plan complet est donc : **A, B, C sur l'aval** (structurel puis constantes,
tout mesurable localement), **la construction d'ordre $k$ sur l'étage higher**
(travail de fond), et **D** en dernier, quand il reste quelque chose que le
device puisse saturer.

## 9.4 Ordre révisé

1. **A1** — vérifier le journal de graines **une fois**, sceller un jeton lié à
   l'identité des cinq sources, et faire accepter le jeton aux huit
   consommateurs au lieu du rejeu. Falsifiable : sortie scientifique identique,
   et la pente ms/nœud/point doit **chuter**.
2. **A2** — mémoïser le manifeste de source de forêt, qui hache le nuage à
   chaque construction. Même critère de falsification.
3. **A3** — remesurer la pente. Le plancher par nœud devient enfin résolu, et
   il décide de la suite.
4. **B** — paralléliser la fermeture sur les événements.
5. **C** — cure R1-d sur le résidu, guidée par un nouveau profil.
6. **Ordre $k$** pour l'étage higher, en parallèle de tout ce qui précède.


---

# 10. A1 exécuté : la prémisse était fausse, la mesure l'a corrigée

## 10.1 Ce que la mesure a démenti dans le §9

Le §9.4 ordonnait A1 « vérifier le journal de graines une fois » sur la foi de
34,6 % inclusifs et de huit sites d'appel trouvés au grep. Le comptage réel dit
**trois appels**, pas huit : le plafond d'A1 tel qu'écrit valait 1,3 ×, pas un
ordre. La feuille de route se trompait de fonction.

Le comptage a dit où était le vrai terme. À $n=16$, pour 213 nœuds de
fermeture :

| appelé | appels | par nœud |
| --- | ---: | ---: |
| `CanonicalSha256Builder::update` | 3 109 276 | 14 600 |
| `ExactRational::canonical_key` | 498 630 | 2 341 |
| `DigestWriter::center(ExactRational3)` | 122 627 | 576 |
| `normalized_terminal_output_digest` | **823** | — |
| `pair_terminal_output_digest` | **823** | — |

**823 recalculs du digest de la sortie entière**, sur une sortie qui est une
*entrée* de l'aval et ne change pas. Attribution : `terminal_catalog_certified()`
les re-dérive à chaque appel, et sur 822 appels **812 viennent de
`reconstruct_exact_direct_saddle_arm_facet`** — 609 depuis la vérification en
flux, 203 depuis la sélection de bras de l'exécuteur de lots. Deux boucles sur
les graines de bras, re-certifiant une façade inchangée à chaque itération.

**C'est là qu'était le quadratique en événements.** Pas dans la fermeture : le
coût vaut $O(\text{graines} \times \text{sortie})$.

## 10.2 Pourquoi la mémoïsation aurait été fausse

Le premier réflexe — mettre en cache `terminal_catalog_certified()` — détruit
une propriété réelle : ce prédicat **est** le test anti-forge, il re-dérive les
digests depuis les enregistrements précisément pour détecter qu'on les a
touchés. Un cache le rendrait aveugle.

La correction saine sépare deux choses que le prédicat confond sous un seul
nom : six comparaisons $O(1)$ qui **lient** la façade au journal, et
l'auto-certification $O(\text{sortie})$ de la façade. Le nouveau
`ExactDirectSaddleArmFacetReconstructor` exécute le prédicat **entier** une fois
dans son constructeur — en levant exactement ce que levait la fonction
mono-coup, pour les mêmes raisons — puis reconstruit par indice. Le point
d'entrée mono-coup est **inchangé**, donc tous les contrats anti-forge existants
testent encore ce qu'ils testaient, et une façade falsifiée échoue toujours,
avant la première facette au lieu d'à la première. Refaire une dérivation sur
les mêmes octets ne peut rien découvrir que la première ait manqué.

## 10.3 Le résultat, falsifié sur les deux critères

| $n$ | ms/nœud avant | ms/nœud après | gain |
| ---: | ---: | ---: | ---: |
| 12 | 3,68 | 2,232 | 1,65 × |
| 16 | 4,29 | 2,056 | 2,09 × |
| 20 | 6,41 | 2,314 | 2,77 × |
| 24 | 7,57 | 2,996 | 2,53 × |
| 28 | 8,40 | 3,474 | 2,42 × |

Sortie scientifique **identique à toutes les tailles**, 22 suites vertes dont
les anti-forge du journal de graines et de la descente de facette.

**Mais le gain qui compte est l'exposant**, parce que le contrat est à 50 000
points :

| | loi ajustée | ms/nœud à 50 000 | aval $K=5$, 48 cœurs |
| --- | --- | ---: | ---: |
| avant | $0{,}257\,n^{1{,}054}$ | 23 018 | $9{,}5\cdot10^{6}$ s |
| **après** | $0{,}491\,n^{0{,}559}$ | **208,9** | $8{,}7\cdot10^{4}$ s |

**Facteur 110 à la taille du contrat, pour un changement qui vaut 2,4 × à
$n=28$.** C'est la démonstration que le levier est bien l'exposant et non la
constante — et que mesurer sur de petits nuages sous-estime les gains
structurels autant qu'elle surestime les coûts.

## 10.4 Ce qui reste

L'exposant résiduel $n^{0{,}559}$ dit qu'**un autre terme de même nature est
encore là**. La suite est la même méthode : profiler, compter les appels,
trouver la dérivation refaite, la hisser. Il reste $8{,}7\cdot10^{4}$ à
gagner sur l'aval à $K=5$, dont 48 × de parallélisation et ≤ 12,8 × de cure
R1-d — soit encore un facteur ~140 à trouver dans les exposants.
