# Contrat numérique de la v12 : coordonnées jusqu'à 32 bits, arithmétique en repère local

7 octobre 2026. Proposition du développeur, **soumise à l'auditeur avant tout code** (`audits/`). Elle met en œuvre la
décision D6 ([`DECISIONS.md`](DECISIONS.md)) : u18 abandonné, u21 au moins, u24 puis u32 visés, un seul profil dans le
produit. Elle prolonge la doctrine de la v11 (`../../morsehgp3D_v11/src/num/budgets.hpp`, voie étroite
`kNarrowSpan` de `src/catalogue/leaf_device_predicates.hpp`) et les décisions G02 et G09 de la conception d'origine
(`../../morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_GENERATEUR.md`). Elle répond au point 6 de
la note d'ouverture de l'auditeur ([`AUDIT_CODEX_20261007.md`](../audits/AUDIT_CODEX_20261007.md)) : budgets, centres
absolus, mots de Morton, exports, et site extérieur à un repère.

## 1. Le constat

Dans la v11, chaque budget est une fonction du **domaine global** $B$ (convention de `budgets.hpp` : une expression
« de $b$ bits » est de valeur absolue $<2^{b}$, sous l'hypothèse $\lvert\Delta\rvert<M=2^{B}$ pour toute différence de
coordonnées). Le côté d'un point par rapport à une sphère q3 y exige $6B+8$ bits, le numérateur d'un niveau $8B+12$, la
comparaison de deux niveaux $14B+20$ ; à $B=32$ : 200, 268 et 468 bits.

Or **tous les prédicats géométriques ne lisent que des différences de coordonnées** (plus petite boule, centre, côté
d'un point, dominance de deux sites sur une boîte, orientation, appartenance d'un centre à une boîte), et un niveau est
invariant par translation. La largeur utile d'un calcul est donc fixée par le rapport entre l'**étendue locale** des
objets qu'il touche et le pas de la grille, pas par le domaine. Deux conséquences :

- **un grand domaine ne coûte rien** : une scène agrégée de plusieurs kilomètres au millimètre (u24) ou une trame
  translatée au bord de $[0,2^{32})$ garde les étendues locales d'une trame u21 ;
- **une grille plus fine coûte** : au dixième de millimètre, toutes les étendues locales gagnent 3,3 bits. Une grille
  plus fine que la précision du capteur (1 à 2 cm pour le LiDAR) ne change pas l'objet utile et alourdit tout le calcul ;
  la v12 la mesure et la publie, sans l'optimiser.

Hypothèse à mesurer (`MES-S`, § 8) : sur les trames SemanticKITTI au millimètre, l'étendue d'une feuille ou d'un
support de boule critique tient presque toujours sous $2^{17}$ (131 m).

## 2. Repères

- **Entrée.** Coordonnées entières $0\leq x_i<2^{B}$, $B\leq 32$, stockées en `u32` ; pas de grille déclaré (1 mm par
  défaut) ; positions distinctes (décision D8). Le binaire compile **un seul profil** ($B_{\max}=32$) ; le $B$ effectif de
  l'entrée et son étendue globale sont lus et publiés.
- **`NUM-REPERE` (repère local certifié).** Pour un ensemble fini $E$ de points entiers (sites, coins de boîtes),
  l'ancre est le minimum par axe $a=\min E$ et l'étendue en bits est le plus petit $s$ tel que
  $\max_{i}\max_{x\in E}(x_i-a_i)<2^{s}$. Alors $\lvert x_i-y_i\rvert<2^{s}$ pour tous $x,y\in E$ : c'est exactement
  l'hypothèse des budgets de la v11 avec $M=2^{s}$. Calcul entier exact (un maximum, un comptage de zéros de tête).
- **`NUM-COUVERTURE` (règle de couverture).** Un prédicat n'est évalué dans un repère que si **tous** ses arguments
  appartiennent à l'ensemble $E$ qui a défini ce repère. En particulier :
  - repère d'une **feuille** du catalogue : la fermeture $[lo,hi]$ de sa boîte de centres **et tous les sites de sa
    liste**, y compris ceux qui sont hors de la boîte (c'est déjà l'enveloppe de la voie étroite de la v11) ;
  - repère d'une **boule** : son support $S$ ;
  - repère d'une **partie de descente** : ses $k$ sites ;
  - un site ou une boîte qui n'appartient pas à $E$ (census sur l'index, site extérieur interrogé) n'est confronté à la
    boule qu'à travers la garde `NUM-GARDE`.
- **`NUM-GARDE` (garde entière d'une boule).** Soit $b$ une boule critique de support $S$, d'ancre $a$ et d'étendue
  $s$. Son centre $c$ est dans l'enveloppe convexe de $S$, donc dans $[a,a+2^{s}-1]$ par axe, et son rayon vérifie
  $R\leq\mathrm{diam}(S)<\sqrt{3}\cdot 2^{s}<2^{s+1}$. Tout point $x$ de la boule fermée vérifie donc, pour chaque
  axe, $a_i-2^{s+1}<x_i<a_i+2^{s}+2^{s+1}$. La garde rejette, par deux comparaisons entières par axe, tout site (ou toute
  boîte de l'index) qui sort de ce pavé : il est extérieur à la boule fermée, sans arithmétique. Un site qui passe la
  garde est, avec $S$, dans un repère d'étendue $s+2$ (différences $<3\cdot 2^{s}<2^{s+2}$) : le prédicat de côté se
  calcule avec le budget de $s+2$, jamais avec une étendue dynamique. Une boîte de l'index qui passe la garde est
  ramenée à son point le plus proche du centre (projection par axe), qui est dans le même pavé ; le minorant
  `LEM-LATTICE` s'y applique dans le même repère.

Preuve des deux faits utilisés : $c=\sum\lambda_j s_j$ avec $\lambda_j\geq 0$, $\sum\lambda_j=1$, d'où $c$ dans le pavé
de $S$ et $\lvert c-s_k\rvert\leq\sum\lambda_j\lvert s_j-s_k\rvert\leq\mathrm{diam}(S)$ ; et $R=\lvert c-s_k\rvert$ pour
tout $s_k\in S$ (le support est sur la sphère). Le centre d'une boule critique est dans l'enveloppe convexe de son
support : q2 (milieu), q3 (triangle aigu), q4 (centre intérieur au tétraèdre), propriété de toute plus petite boule
englobante. À contre-lire par l'auditeur.

## 3. Budgets en fonction de l'étendue locale

Les formules de `budgets.hpp` restent vraies avec $s$ à la place de $B$. Seuils de type natif : `i64` si l'expression
a au plus 63 bits, `i128` si elle en a au plus 127.

| Expression (v11) | Bits | Natif `i64` si | Natif `i128` si | Usage chaud |
| --- | --- | --- | --- | --- |
| différence | $s$ | toujours | — | partout |
| produit scalaire | $2s+2$ | $s\leq 30$ | toujours | partout |
| produit vectoriel | $2s+1$ | $s\leq 31$ | toujours | q3, q4 |
| déterminant | $3s+3$ | $s\leq 20$ | toujours | q4, orientations |
| dominance G1 sur une boîte | $2s+3$ | $s\leq 30$ | toujours | feuille |
| numérateur du centre q3 | $5s+5$ | $s\leq 11$ | $s\leq 24$ | plus petite boule |
| dénominateur du centre q3 | $4s+5$ | $s\leq 14$ | $s\leq 30$ | idem |
| numérateur du centre q4 | $4s+5$ | $s\leq 14$ | $s\leq 30$ | idem |
| dénominateur du centre q4 | $3s+4$ | $s\leq 19$ | toujours | idem |
| côté d'un point | $6s+8$ | $s\leq 9$ | $s\leq 19$ | census, certificat |
| orientation avec centre | $7s+9$ | $s\leq 7$ | $s\leq 16$ | feuille (J2, J3) |
| numérateur du niveau | $8s+12$ | $s\leq 6$ | $s\leq 14$ | niveaux |
| dénominateur du niveau | $6s+8$ | $s\leq 9$ | $s\leq 19$ | niveaux |
| comparaison de deux niveaux | $14s+20$ | $s\leq 3$ | $s\leq 7$ | tri des événements |

Conséquences :

- **Trois voies par expression**, comme en v11 : native quand l'étendue du repère la garantit (aucun contrôle) ; sinon
  contrôlée (`__builtin_*_overflow`, tout drapeau abandonne la valeur sans résultat partiel) ; sinon entiers larges à
  largeur fixe (192 ou 320 bits), recalculés depuis les coefficients d'origine. Les certificats de puissance q3 et
  d'orientation de la v11 sont calculés sur les coefficients réels : ils ne lisent ni $B$ ni $s$ et restent valides ; ils
  élargissent la voie native au-delà de la table.
- **La voie est uniforme par repère** (une feuille, une boule). Sur le GPU, quand un warp traite une seule feuille, la
  voie est uniforme sur le warp.
- **Les niveaux ne sont jamais natifs au-delà de $s=14$**, et leurs comparaisons ne le sont jamais au-delà de $s=7$ :
  comme en v11, le tri des événements passe par les clés F3/F4 à repli exact. Un numérateur de niveau tient dans 192
  bits jusqu'à $s=22$ et dans 320 bits jusqu'à $s=32$ ; une comparaison exacte de repli tient dans 320 bits jusqu'à
  $s=21$ et dans 512 bits jusqu'à $s=32$. La largeur des entiers larges est donc fixée par le palier, jamais par $B$.
- **Paliers proposés** (constantes de compilation communes à l'hôte et à l'appareil) : étroit $s\leq 16$ (orientation,
  côté, centres natifs `i128`), moyen $s\leq 24$ (centres natifs, côté contrôlé ou certifié), large au-delà. La v11 avait
  un seul palier étroit sur le GPU, l'étendue $2^{20}$ ; ces paliers le remplacent s'ils gagnent au microbanc, sinon la
  v12 garde la coupe de la v11.

## 4. Où le domaine global intervient encore

| Usage | Bits | Traitement |
| --- | --- | --- |
| lecture et contrôle de l'entrée | $B\leq 32$ | refus explicite au-delà |
| clé de Morton | 63 | voir ci-dessous |
| centre absolu $a+N/D$ (export, départage) | $B+4s+6$ (v11 : $5B+6$ à $s=B$) | entiers larges, hors chemin chaud |
| comparaison de deux centres absolus | $B+8s+11$ (v11 : $9B+11$) | idem ; rare |
| export d'un niveau | numérateur $\leq 8s+12$, dénominateur $\leq 6s+8$ | mots de 64 bits, nombre de mots en tête |

**Clé de Morton.** Dans la v11, l'index (arbre radix de Karras) ne lit la clé que pour couper une plage au bit le plus
haut qui diffère ; ses boîtes sont **exactes**, réunies de bas en haut sur les coordonnées. La clé ne décide donc rien :
elle ordonne. La v12 garde **une seule clé de 63 bits** : les 21 bits de poids fort de chaque axe, pris sur l'**étendue
globale de l'entrée** (coordonnées moins le minimum, décalées de $\max(0,B_{\mathrm{eff}}-21)$ bits). Les égalités de
clé, possibles dès que $B_{\mathrm{eff}}>21$, sont départagées par le `PointId` (ordre total, équivariant par
permutation) ; une plage de clés toutes égales est coupée en son milieu. La correction ne dépend pas de la clé ; la
localité, si (deux amas denses aux extrémités du domaine) : une porte de temps dédiée la surveille.

**Ordre canonique.** Exigence : il ne lit aucun ordre interne (rang de tri, indice de site), de sorte que la clé
tronquée ne change rien à l'objet publié. La v12 doit reproduire l'**empreinte sémantique** de la v11 (lecteur strict `bench/full_semantic.py`, [`MESURE.md`](MESURE.md) § 4). Si le
départage canonique de la v11 compare des centres absolus, la v12 garde cette comparaison, en entiers larges, hors du
chemin chaud.

## 5. Flottant

Doctrine F1–F6 de la v11 inchangée : aucune décision en flottant ; noyaux binary64 exacts (F2) quand la somme des valeurs
absolues des termes développés reste sous $2^{53}$ (produits scalaires exacts pour $s\leq 25$) ; clés F3/F4 des niveaux à
repli exact ; filtres de signe F6 à seuil certifié par expression. Les propositions flottantes (plus petite boule de
Welzl, `LEV-MEB-CERT`) se calculent **dans le repère local** (différences exactes en binary64 pour $s\leq 53$) et ne
décident rien : elles sont certifiées par le catalogue (`LEM-T1`) ou en exact.

## 6. Appareil (GPU)

Le contrat R7 de la v11 est conservé : sur l'appareil, seules décident les voies natives garanties par le palier du
repère ou par un certificat de fabrique ; tout autre cas rend la feuille « non résolue », rejouée en exact sur l'hôte
avant admission, **en parallèle**, et comptée. Les paliers, les budgets et la garde sont une source unique commune.

## 7. Portes et témoins

- **Bornes de palier** : pour chaque expression de la table, un témoin à l'étendue limite $s^{*}$ et un à $s^{*}+1$
  (par exemple côté q3 à $s=19$ et $s=20$, orientation à $s=16$ et $s=17$), joués sur l'hôte et sur l'appareil ; le
  repli doit être effectivement emprunté (compteur).
- **Garde** : site exactement sur la sphère à la limite du pavé, site juste hors du pavé, boîte de l'index qui touche le
  pavé par un coin ; mutant « garde d'un bit trop étroite » tué.
- **Couverture** : une feuille dont un site de la liste est loin hors de la boîte (l'étendue du repère est celle du
  site, pas de la boîte) ; mutant « repère pris sur la seule boîte » tué.
- **Témoins hérités** : `WIT-S21` (triangle aigu d'étendue $2^{21}-1$, premier produit $6\cdot(2^{21}-1)^{6}$ au-delà
  de $2^{127}$), `WIT-U24` étendu à u32 (tétraèdre régulier à l'extrémité du domaine, export du niveau sur plusieurs
  mots), `WIT-F3` et `WIT-F2`.
- **Invariance** : toute translation entière qui garde l'entrée dans $[0,2^{32})$ ne change ni la tour, ni les niveaux,
  ni l'empreinte sémantique ; la porte joue chaque nuage de test translaté jusqu'aux deux bords.
- **Clé** : deux amas denses aux extrémités de $[0,2^{32})$ (égalités de clé massives) ; résultat identique au nuage
  translaté, temps publié.

## 8. Mesures

- `MES-S` : histogramme des étendues locales (feuilles, supports, parties de descente) sur les trames sans sol de
  plusieurs séquences, au millimètre et au dixième de millimètre, et sur les scènes de plusieurs millions de points ;
  part de chaque voie (native, contrôlée, large).
- Coût du profil : la même trame u21 translatée en u24 et en u32 doit coûter moins de 3 % de plus que l'original
  (décision D6) ; la même trame quantifiée au dixième de millimètre est mesurée et publiée, sans objectif.

## 9. Questions pour l'auditeur

1. `NUM-GARDE` : la preuve « centre dans l'enveloppe convexe du support » couvre-t-elle toutes les boules que le
   moteur confronte à des sites extérieurs, y compris les boules proposées en flottant puis rejetées, et les boules de
   coquille étendue (`LEM-T7`) ?
2. `NUM-COUVERTURE` pour la feuille : faut-il aussi borner l'étendue de la liste (refus ou découpe d'une feuille dont la
   liste s'étend trop), ou la voie large suffit-elle ?
3. La clé de 63 bits sur l'étendue de l'entrée, départagée par le `PointId` : voyez-vous un usage de la clé dans la v11
   (hors coupe de l'index et ordre du tri) qui décide quelque chose ?
4. Le départage canonique de la v11 compare-t-il des centres absolus ailleurs que pour des naissances de même niveau ?
