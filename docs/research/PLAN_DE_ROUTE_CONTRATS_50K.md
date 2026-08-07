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

### I2 — Câbler la germination locale certifiée dans la recherche higher · *local puis G4*

Entrée : I1.
C'est le verrou V2, et sa mathématique est faite : restriction certifiée
$D \le 2R(p)$, région AABB, jeu de 26 directions de rayon de couverture prouvé
27,569276°. Ce qui manque est le câblage dans le chemin de recherche, hôte
d'abord — le device ensuite, une fois le gain établi.
Sortie falsifiable, en deux temps :
1. *complétude* — à petit nuage, l'énumération germée rend **exactement** les
   mêmes supports acceptés que l'énumération exhaustive, sur toutes les familles
   du recensement (zéro manquant, zéro en trop) ;
2. *sélectivité* — à $n=512$, $K=5$, l'univers **ferme complètement**, là où le
   chemin actuel résout 0,02 % par 150 s, et le nombre de candidats visités est
   au plus la borne de germination.
Mesure à 50 000 points : G4, une seule exécution, sous le garde-fou d'I1.

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
