# Quatre questions à l'utilisateur : jusqu'où va le principe des deux triangles ?

30 septembre 2026, soir. Synthèse de la révision « cible des deux triangles » (`NOTE_REVISION_CIBLE.md`).

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=synthese_revision_cible
public_status=not_claimed
GCP non utilisé. Aucun moteur modifié.
```

**Ce que vous avez fixé.** Deux triangles ABC et DEF, C et D face à face : d'abord ABC | DEF, puis la fusion. Nous
l'appliquons aussi à la version où le pont est plus court de 0,1 % (P2). Cela exclut la première couverture et
l'ancrage persistant P_κ (recommandation précédente).

**Ce qui reste ouvert.** Deux familles de règles continues donnent vos triangles. Elles ne font pas la même chose
dès que l'écart n'est plus une quasi-égalité :

- **masse des sites** (MMg = MMp à K = 2, et la majorité de paires `maj_paires`) : le point va au groupe qui a le plus
  de voisins dans sa bande ;
- **temps de couverture** (MMt) : le point va à la structure qui le couvre le plus longtemps dans sa bande. Elle
  rejoint le témoin stable P_2 dès que l'écart est net.

Sur votre figure idéale (coordonnées dans Q(√3)), l'oracle exact de l'auditeur indépendant donne à C deux tiers de
ses votes dans ABC, avec une marge de 1/6 : c'est la lecture « masse ». L'auditeur continu a justifié, sous des
hypothèses explicites, qu'une telle majorité à dénominateur figé reste laminaire ; elle n'est pas stable pour autant.

Chaque question isole une décision. Les quatre fixtures sont exactes, à coordonnées entières, et recalculées ici
(`synthese/recus/tables_normal.txt`).

**Lecture.**

- r est le rayon des boules ; le niveau est r² (convention de la thèse). Coupes fermées.
- Une hiérarchie est donnée par ses blocs d'au moins deux points, intervalle de rayon par intervalle de rayon. Les
  points non cités sont seuls.
- HDBSCAN apparaît en contrôle externe, avec la convention de la thèse : niveau = distance de liaison / 2.
- L'option recommandée est en premier. Notre règle de recommandation : **la masse ne tranche que les quasi-égalités**
  (votre cible) ; ailleurs, on suit le cœur quand il contraint, et le témoin stable P_2. C'est une proposition, pas un
  résultat : vos réponses la confirment ou la réfutent.

## Q1. Le pont nettement plus court (K = 2)

```text
 A                                                      E
   \  1999,96                                  1999,96  /
    \                                                  /
      C ----------------- 1700 ----------------- D
    /                                                  \
   /                                                    \
 B                                                      F
 AB = EF = 2000 ; pont CD = 1700 (15 % plus court)
```

Sites (K = 2) : A(268,3000,0) B(268,1000,0) C(2000,2000,0) D(3700,2000,0) E(5432,3000,0) F(5432,1000,0).

FULL_2 (rayons) : CD naît à 850 ; AC, BC, DE, DF à √999956 ≈ 999,978 ; AB, EF à 1000 ; ABC et DEF à
√(249978000484/187489) ≈ 1154,684 ; fusion globale (trois enfants) à √3194656 ≈ 1787,360 = |AD|/2. d_2(C) = d_2(D) =
1700 : **le cœur réunit C et D à 1700, avant la fusion.** Même FULL que vos triangles : seules les dates changent.

| Option | Hiérarchie (blocs, par intervalle de rayon) | Règles qui la donnent (date d'entrée de C) |
| --- | --- | --- |
| (a) attendre, puis la paire | [1154,684 ; t[ : AB \| EF ; [t ; 1787,360[ : AB \| CD \| EF, avec t ≤ 1700 | P_2 (t = 1487,404) ; MMt(4, 2/3) (t = 1507,574) |
| (b) les triangles | ABC \| DEF avant la fusion, dès 1154,684 ou après une attente | maj_paires[1/4] et [1/2], majorités de bande du catalogue, S (durée propre), P (taille) : 1154,684 ; MMg_3 : 1522,352 ; MMg_2 : 1610,688 |
| (c) le pont d'abord | [850 ; 1154,684[ : CD ; [1154,684 ; 1787,360[ : AB \| CD \| EF | première couverture A1, A5, E (excès de masse) : 850 ; MMt(2, 1/4) : 901,561 ; progressive z = 5 : 1202,473 |

Témoins hors options : core (CD à 1700 seulement, AB à 1999,956) ; HDBSCAN (thèse) : CD à 850, puis les six points
dès 999,978.

**Question.** Le pont CD mesure 1700, 15 % de moins que les côtés. Entre la naissance des triangles (1154,7) et la
fusion (1787,4), quelle hiérarchie attendez-vous ?

1. **(a) C et D restent seuls tant que le triangle et le pont les réclament, puis forment la paire CD au plus tard à
   1700.** Recommandée. Raisons : (b) viole le respect du cœur sur [1700 ; 1787,4[ (théorème C-CR : la cible étendue
   et le cœur sont incompatibles pour tout pont plus court que L* ≈ 1868,3) ; les deux majorités continues n'ont ici
   qu'une marge faible et attendent toutes deux ; (a) est ce que donnent ensemble le témoin stable P_2 et MMt(4, 2/3),
   qui garde vos triangles à 1998 et 2000.
2. **(b) C rejoint ABC et D rejoint DEF avant la fusion.** La masse l'emporte même sur un pont nettement plus court.
   Garde MMg, MMp et maj_paires ; écarte MMt(4, 2/3) et P_κ ; coûte le respect du cœur.
3. **(c) C et D forment la paire CD dès la naissance du pont.** Le pont est un amas ; les triangles se réduisent à AB
   et EF. Garde une bande étroite (MMt(2, 1/4)) ; plus précoce que (a).

**Où chaque règle bascule** (famille T1_L : même figure, pont de longueur L ; lettre = option ci-dessus ; * : la
réponse viole le cœur, car L < L*) :

| Règle | 2000 | 1998 (P2) | 1869 | 1868 | 1800 | 1700 | 1600 | 1400 | 1000 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MMt(4, 2/3) | b | b | b | b* | b* | a | c | c | c |
| MMt(2, 1/4) | b | b | a | a | c | c | c | c | c |
| MMg_3 (= MMp_3) | b | b | b | b* | b* | b* | a | c | c |
| maj_paires[1/4] | b | b | b | b* | b* | b* | b* | c | c |
| maj_paires[1/2] | b | b | b | b* | b* | b* | b* | b* | c |
| P_2 | b | a | a | a | a | a | a | c | c |
| A1, A5 | b | c | c | c | c | c | c | c | c |

La cible fixée (2000 et 1998) est dans la colonne b de toutes les règles candidates. MMt(4, 2/3) entre dans ABC à
1305,17 pour L = 1869 : juste après 1300. P_2 n'a b qu'à L = 2000, et à 1931,783, après la fenêtre de la cible.

## Q2. Un voisin proche contre deux voisins un peu plus loin (K = 2)

```text
        b1  b2          b1 et b2 : paire serrée (16,03)
          \/
           \  120,4 et 120,5
            \
             x ------------- 100 ------------- a
   |a b1| = 150 ; |a b2| = 150,06
```

Sites (K = 2) : x(1000,1000,1000), a(1100,1000,1000), b1(1010,1120,1000), b2(1010,1119,1016).

FULL_2 (rayons) : b1b2 naît à 8,016 ; xa à 50 ; xb1 à 60,208 et xb2 à 60,243 ; le triangle {x, b1, b2} à
√(540976005/148484) ≈ 60,360 ; ab1 à 75 et ab2 à 75,028 ; fusion à 75,122 ; racine à 25√145/4 ≈ 75,260.
d_2(x) = 100 dépasse la racine : **le cœur ne contraint rien ici.** C'est la décision de Q1 (une paire née nettement
plus tôt, ici 17 %, contre un triangle), sans le cœur.

| Option | Hiérarchie (blocs, par intervalle de rayon) | Règles qui la donnent (date d'entrée de x) |
| --- | --- | --- |
| (a) x avec son plus proche voisin | b1b2 dès ≈ 8 ; [t ; 75,260[ : {x, a} \| {b1, b2} | A1, A5, S (durée propre) : 50 ; MMt(2, 1/4) : 53,033 ; P_2 : 100 − 2√3625 + √(90625/16) ≈ 54,844 ; MMt(4, 2/3) : 60,093 ; progressive z = 5 : 61,155 |
| (b) x avec le couple | b1b2 dès ≈ 8 ; [t ; 75,122[ : {x, b1, b2}, a seul (a les rejoint à 75,122 ou à 75,260 selon la règle) | maj_paires[1/4] et [1/2], majorités du catalogue, P, E : 60,360 ; MMg_3 : 68,233 ; MMg_2 : 70,575 |
| (c) x attend la fusion | b1b2 seul bloc jusqu'à 75,12 | aucune règle calculée |

Témoins hors options : core (b1b2 à 16,031, puis tout à 100) ; HDBSCAN (thèse) : b1b2 à 8,016, {x, a} à 50, tout à
60,208. Bassins d'un noyau gaussien (vérificateur statistique, flottants, illustratif) : x avec a pour h dans
[46 ; 47,5[, jamais avec {b1, b2}.

**Question.** x a un voisin a à 100 et deux voisins b1, b2 à 120, serrés entre eux. Entre 61 et 75, avec qui mettez-vous
x ?

1. **(a) Avec a.** Recommandée. Raisons : a est nettement plus proche (100 contre 120,4) ; la paire {x, a} vit de 50
   à la racine, le triangle {x, b1, b2} ne naît qu'à 60,36 ; votre cible ne tranchait qu'une quasi-égalité (0,1 %) ;
   P_2, MMt et les bassins d'un noyau gaussien donnent a.
2. **(b) Avec b1 et b2.** Deux voisins valent plus qu'un, même 20 % plus loin : c'est la lecture « masse des sites »
   de votre cible. Garde MMg, MMp et maj_paires ; écarte MMt.
3. **(c) Seul jusqu'à la fusion.** x est une vraie ambiguïté ; aucune règle testée ne le fait.

## Q3. Filament contre amas (K = 2)

```text
 amas c0..c7 (pas ~200)      filament x, f1..f5 (pas ~700)

 c5 c6 c1
 c7 c2 c0 .... 900 .... x --700-- f1 --700-- f2 .. f5
 (c3, c4 : +-200 en z)

 x prolonge le filament ; l'amas est 22 % plus loin
```

Sites (K = 2), O = (10000, 10000, 10000) : x = O ; f1 = O + (700,3,0), f2 = O + (1401,−2,0), f3 = O + (2100,4,0),
f4 = O + (2802,0,0), f5 = O + (3500,−3,0) ; c0 = O + (−900,0,0), c1 = O + (−880,200,0), c2 = O + (−880,−200,0),
c3 = O + (−880,0,200), c4 = O + (−880,0,−200), c5 = O + (−1100,0,0), c6 = O + (−1080,150,100),
c7 = O + (−1080,−150,−100).

FULL_2 (rayons) : l'amas c0..c7 est une seule composante dès 141,428 ; les lentilles du filament naissent de 349,003 à
351,006 (xf1 à 350,003) ; x est couvert par l'amas dès 450 (lentille xc0, puis xc1 à xc4) ; l'amas absorbe ces
lentilles à 453,471 ; le filament devient une seule composante à 700,501 ; fusion globale à 796,117.
d_2(x) = 700,006 : **le cœur met x avec le filament à 700,006, avant la fusion.**

| Option | Hiérarchie (blocs, par intervalle de rayon) | Règles qui la donnent (date où x rejoint son bloc) |
| --- | --- | --- |
| (a) continuation | amas dès 141,428 ; x avec f1 dès t, puis le filament entier dès 700,501 | A1, A5 : 350,003 ; MMt(2, 1/4) : 696,429 ; MMt(4, 2/3) : 697,458 ; P_2 : 699,489 ; maj_paires[1/4], bande 1/4, progressive z = 5 : 700,501 |
| (b) masse | amas dès 141,428 ; x avec l'amas dès t ; filament f1..f5 à part | maj_paires[1/2], bande 1/2, maj_unif, S, P, E : 453,471 ; MMg_3 : 524,201 ; MMg_2 : 614,840 |
| (c) attente | x seul jusqu'à 796,117 | maj_paires pour η dans [0,285702 ; 0,289190[ seulement (vérificateur statistique) |

Témoins hors options : core (x avec f1 à 700,006 : continuation) ; HDBSCAN (thèse) : x avec f1 à 350,003, filament à
351,006, puis tout à 450 (le filament entier rejoint l'amas). Bassins d'un noyau gaussien (flottants) : l'amas.

**Question.** x prolonge un filament régulier (un voisin à 700) et fait face à un amas dense (cinq voisins à 900).
Entre 705 et 790, où mettez-vous x ?

1. **(a) Dans le filament.** Recommandée. Raisons : (b) viole le respect du cœur sur [700,006 ; 796,117[ ; le filament
   est la structure la plus proche (700 contre 900) ; sur LiDAR, une règle de masse arracherait les extrémités des
   objets filiformes (poteaux, troncs) aux objets denses voisins ; P_2, MMt et maj_paires[1/4] donnent (a).
2. **(b) Dans l'amas.** La masse (cinq voisins contre un) l'emporte. Garde MMg, MMp et les bandes larges ; écarte MMt
   et maj_paires[1/4] ; coûte le respect du cœur.
3. **(c) Seul jusqu'à la fusion.** Vraie ambiguïté ; fenêtre de paramètres très étroite.

## Q4. Une chaîne entre deux tétraèdres (K = 3)

```text
 P   Q   R                                P2  Q2  R2
   \ | /                                    \ | /
     C ----- 692,8 ----- m ----- 692,8 ----- D
 {C,P,Q,R}         C, m, D alignés       {D,P2,Q2,R2}
 tétraèdres d'arête 1414

 K = 3 : chaîne à 692,8 ; faces à 816,5 ; tétraèdres à 866
```

Sites (K = 3) : C(3000,3000,3000), P(4000,4000,3000), Q(4000,3000,4000), R(3000,4000,4000), m(2600,2600,2600),
D(2200,2200,2200), P2(1200,1200,2200), Q2(1200,2200,1200), R2(2200,1200,1200).

FULL_3 (rayons) : chaîne CmD née à √480000 ≈ 692,820 ; huit faces à √(2000000/3) ≈ 816,497 ; tétraèdres {C,P,Q,R} et
{D,P2,Q2,R2} à √750000 ≈ 866,025 ; triplets mixtes (m avec une face) à √1020000 ≈ 1009,950 ; fusions {C,P,Q,R,m} et
{m,D,P2,Q2,R2} à √(208080000/179) ≈ 1078,174 ; racine à √1780000 ≈ 1334,166. La chaîne vit de 692,8 à la racine.
d_3(C) = 1385,6 dépasse la racine : **le cœur ne contraint rien ici.** C'est votre figure à K = 3 : C a trois faces
d'un côté, une chaîne née 15 % plus tôt de l'autre.

| Option | Hiérarchie (blocs, par intervalle de rayon) | Règles qui la donnent |
| --- | --- | --- |
| (a) attendre, puis la chaîne | [866 ; t[ : PQR \| P2Q2R2 ; [t ; 1334,166[ : CmD \| PQR \| P2Q2R2 | progressive z = 5 (t = 1053,145) ; P_2 (1086,814) ; MMt(4, 2/3) (PQR \| P2Q2R2 dès 922,557, CmD dès 1244,770) |
| (b) les tétraèdres, m seul | CPQR \| DP2Q2R2 avant la racine | MMg_3, S (durée propre), P (taille), bande du catalogue 1/4 : dès 866,025 ; MMg_2 : 926,625 ; bande 1/2, maj_unif : 1078,174 |
| (c) la chaîne d'abord | CmD dès sa naissance ; puis CmD \| PQR \| P2Q2R2 | A1, A5, E : 692,820 ; MMt(2, 1/4) : 734,847 |

Hors options : maj_paires et MMp (votes de paires d'ordre 3) mettent {C, D} ensemble **sans m** (dès 692,820 ; dès
1039,230 pour maj_paires[1/2]) : le vote de paire C–D a pour milieu m ; à K ≥ 3, l'univers des paires scinde la
chaîne. Core : rien avant la racine.
HDBSCAN (thèse, min_samples = 3) : CmD à 692,820, puis tout à 707,107.

**Question.** À K = 3, une chaîne serrée C–m–D (pas 692,8, née la première) relie deux tétraèdres (arête 1414).
Entre 866 et 1334, quelle hiérarchie attendez-vous ?

1. **(a) C, m et D attendent tant que les faces et la chaîne les réclament, puis la chaîne CmD se forme avant la
   racine.** Recommandée. Raisons : la chaîne naît 15 % avant les faces, comme le pont de Q1, et ses points sont deux
   fois plus serrés que ceux des tétraèdres ; c'est la réponse de P_2 et de MMt(4, 2/3), cohérente avec (a) en Q1.
2. **(b) C rejoint son tétraèdre, D le sien, m reste seul.** Transposition littérale de vos triangles (trois faces
   contre une chaîne). C'est la convention des cibles K ≥ 3 du catalogue commun (famille `simplexes_et_ponts`), écrite
   avant votre réponse. Garde MMg ; écarte MMt et P_κ.
3. **(c) La chaîne est un amas dès sa naissance.** Première couverture, ou bande étroite (MMt(2, 1/4)) ; plus précoce
   que (a).

## Ce que vos réponses décident

| Réponses | Règle à retenir pour le juge final | Ce qu'elle sacrifie |
| --- | --- | --- |
| (a) partout | MMt(4, 2/3) : temps de couverture, intrinsèque, locale, continue | respect du cœur ailleurs (fixture FX-MC12) ; la cible atomique adverse (voir la note, § 3.3) |
| (b) partout | MMg_3 (Γ_K ; = MMp_3 à K = 2) : masse, date continue ; maj_paires[1/2] donne (b) en Q1 à Q3 | respect du cœur à K = 2 ; intrinsèque ; à K ≥ 3, un univers de votes reste à choisir (Γ_K combinatoire ; les paires scindent la chaîne de Q4) ; continuité pour maj_paires |
| (c) en Q1 et Q4, (a) ailleurs | MMt à bande étroite (2, 1/4) | la cible atomique (échoue sur les trois nuages atomiques) ; marge faible sur vos triangles (κ_min = 1,936) |
| (a) en Q1 et Q3, (b) en Q2 et Q4 | « la masse, sauf contre le cœur » : aucune règle connue ; construction ouverte | — |

Vos triangles (P1 et P2) passent avec chacune de ces règles : sur les 20 variantes exactes de la figure, MMt(4, 2/3),
MMt(2, 1/4), MMg_3, maj_paires[1/4] et [1/2] réussissent 20 fois sur 20 ; P_2 échoue 20 fois.

La sélection finale (condensation, EOM) est un étage séparé : sur l'arbre ABC | DEF, mcs = 2 ou 3 garde les deux
triangles et mcs = 4 les retire (porte de l'auditeur indépendant).
