# Contrat numérique de la v12 : coordonnées jusqu'à 32 bits, arithmétique en repère local

7 octobre 2026. Proposition du développeur, **soumise aux auditeurs avant tout code**. Elle met en œuvre la décision
D6 ([`DECISIONS.md`](DECISIONS.md)) : u18 abandonné, u21 au moins, u24 puis u32 visés, un seul profil dans le produit.
Elle prolonge la doctrine de la v11 (`../../morsehgp3D_v11/src/num/budgets.hpp`, voie étroite `kNarrowSpan` de
`src/catalogue/leaf_device_predicates.hpp`) et les décisions G02 et G09 de la conception d'origine
(`../../morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_GENERATEUR.md`).

Historique : première rédaction `e264de6f2`, en réponse au point 6 de l'auditeur Codex
([`AUDIT_CODEX_20261007.md`](../audits/AUDIT_CODEX_20261007.md)) ; **révision** du même jour après la relecture de
l'auditeur Claude ([`AUDIT_CONTRAT_NUMERIQUE_20261007.md`](../receipts/audit_canal_20261007/archives/AUDIT_CONTRAT_NUMERIQUE_20261007.md), `2a7a5f346`,
constats `CST-0108` à `CST-0113`, tous acceptés) : domaine de la garde, recensement et requêtes à centre entier hors
garde, budgets mixtes, filtrage par le parent, et la clé de Morton, qui décidait $S^{*}$ dans la v11. Seconde révision
après le contre-audit de l'auditeur Codex
([`numerique/REPORT.md`](../receipts/audit_contrats_20261007/numerique/REPORT.md), constats `CST-0201` à `CST-0212`,
tous acceptés) : certificats liés à leur domaine, clé de Morton exacte (identité des sites), boîtes fermées à 33 bits,
réservoir du filtre G1, garde des boîtes partielles, mesure de D6.

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

Hypothèse mesurée (`MES-S`, § 8 ; [reçu local](../receipts/mes_m2_local_20261007/README.md)) : sur ng00, ng01 et ng02
(séquence 08, 1 mm, K5 et K10), **toutes** les feuilles ont $s\leq 17$ (au plus 13 sur 430 579 à $s=17$, médiane
10 à 11) et tous les supports $s\leq 15$ (médiane 8 à 10) ; le palier étroit proposé ($s\leq 16$) couvre au moins
99,997 % des feuilles et tous les supports, et les feuilles à $s=17$ passent au palier moyen. Portée : trois trames
d'une séquence, au millimètre ; ni autres séquences, ni dixième de millimètre, ni scènes de plusieurs millions de sites.

## 2. Repères, garde et requêtes

- **Entrée.** Coordonnées entières $0\leq x_i<2^{B}$, $B\leq 32$, stockées en `u32` ; pas de grille déclaré (1 mm par
  défaut) ; positions distinctes (décision D8). **Profils de compilation** : u21, base de mesure de la décision D6 ;
  u24 puis u32, candidats. $B_{\max}=32$ est le candidat de conception, retenu comme profil unique du produit
  seulement s'il est qualifié selon D6 (§ 8, `CST-0207`) ; aucun profil n'est un second chemin produit. Le $B$ effectif
  de l'entrée et son étendue globale sont lus et publiés.
- **`NUM-REPERE` (repère local certifié).** Pour un ensemble fini $E$ de points entiers (sites, coins de boîtes), le
  coin minimal est $m=\min E$ par axe et l'étendue en bits est le plus petit $s$ tel que
  $\max_{i}\max_{x\in E}(x_i-m_i)<2^{s}$. Alors $\lvert x_i-y_i\rvert<2^{s}$ pour tous $x,y\in E$ : c'est l'hypothèse des
  budgets de la v11 avec $M=2^{s}$, quelle que soit l'**origine** $o\in E$ des formules (pour une boule, un site de son
  support, comme dans la v11). Calcul entier exact (un maximum, un comptage de zéros de tête) ; étendue nulle : $s=0$.
  **Boîtes fermées** (`CST-0204`) : la v11 représente une boîte de centres par $[lo,hi)$ avec $hi=\max(\text{site})+1$
  (`boxes.cpp`), qui vaut $2^{32}$ à l'extrémité du domaine u32 ; le repère d'une feuille peut donc exiger $s=33$ ($s=22$
  et $s=25$ aux profils 21 et 24). Les bornes de boîtes vivent dans un type de 64 bits, aucune n'est calculée en `u32`,
  et les paliers couvrent $s\leq 33$.
- **`NUM-COUVERTURE` (règle de couverture).** Un prédicat n'est évalué dans un repère que si **tous** ses arguments
  appartiennent à l'ensemble $E$ qui a défini ce repère :
  - repère d'une **feuille** du catalogue : la fermeture $[lo,hi]$ de sa boîte de centres **et tous les sites de sa
    liste**, y compris ceux qui sont hors de la boîte (l'enveloppe de la voie étroite de la v11) ;
  - le **filtrage G1 d'un enfant** lit les témoins et candidats dans la liste du **parent** (`boxes.cpp`, `filter` et
    `reservoir` dans la v11) : il s'évalue dans le repère du parent, qui contient celui de l'enfant (`CST-0112`) ;
  - repère d'une **boule** : son support $S$ ; repère d'une **partie de descente** : ses $k$ sites ;
  - un site ou une boîte qui n'appartient pas à $E$ (recensement sur l'index, site extérieur interrogé) n'est confronté
    à une boule qu'à travers `NUM-GARDE`.
  - **Aucun plafond d'étendue** : la voie large est exacte, et un refus par étendue serait une incomplétude sur des
    entrées légitimes. Une liste étirée par un seul site lointain est marginale sur les trames mesurées : le site le
    plus lointain ne fixe l'étendue à deux bits ou plus que dans 0,0023 à 0,0085 % des feuilles (`MES-S`). La voie
    par prédicat est donc inutile sur ces trames ; si une autre famille de données la rendait utile, le premier
    remède resterait de subdiviser (l'étendue d'un enfant est au plus celle de son parent).
- **`NUM-CERTIFIEE` (garde réservée aux boules certifiées, `CST-0108`).** Aucun site ni aucune boîte extérieurs ne sont
  confrontés à une boule avant le **certificat exact** de son support. La garde repose sur $c\in\mathrm{conv}(S)$, qui
  est faux pour une proposition flottante non certifiée (centre circonscrit d'un triangle obtus hors du triangle ;
  d'un triangle presque aligné, arbitrairement loin). Tenu par le type : une boule confrontable ne sort que du
  catalogue ou du certificat exact (signes barycentriques) ; mutant « recensement avant certificat ». Une sphère qui
  passe seulement par trois ou quatre sites, sans certificat de positivité, n'en bénéficie pas non plus : pour
  $S=\lbrace(419,0,0),(435,15,0),(434,14,0)\rbrace$ ($s=5$, centre $(419/2,479/2,0)$), le point entier $(0,479,0)$ est
  sur la sphère mais hors du pavé (témoin de l'auditeur Codex). Ces candidates restent dans une voie générique exacte,
  sans la garde.
- **`NUM-GARDE` (garde entière d'une boule certifiée).** Soit $b$ une boule certifiée de support $S$, de coin minimal
  $m$, de largeurs $w_i$ et d'étendue $s$ ; posons $M=2^s$. Son centre $c$ est dans l'enveloppe convexe de $S$, donc
  $m_i\leq c_i\leq m_i+M-1$. Comme tous les sites de $S$ sont sur la sphère, $b$ est la plus petite boule de $S$.
  La boule au milieu de la boîte du support contient $S$, d'où
  $R^2\leq(w_1^2+w_2^2+w_3^2)/4\leq3(M-1)^2/4<M^2$.
  Tout point $x$ de la boule fermée vérifie donc $m_i-M<x_i<m_i+2M$ pour chaque axe (le **pavé ouvert**).
  Cette preuve inclut $s=0$, $M=1$, $R=0$. Conséquences :
  - un **site** hors du pavé est extérieur à la boule fermée, sans arithmétique ; un site dans le pavé est à moins de
    $2M$ de chaque site de $S$ par axe. Le repère d'une requête gardée est celui du support (ancre conservée), sans
    repère commun à toutes les requêtes. Cette réduction du pavé conserve les budgets antérieurs plus larges
    $6s+11$, le certificat au domaine $t=s+2$ et les voies numériques ; leur réduction serait une autre tranche ;
  - une **boîte** de l'index **disjointe** du pavé est extérieure ; pour une boîte qui touche le pavé, le minorant
    (`LEM-LATTICE`) se calcule sur le point **entier** de la boîte le plus proche du centre : plancher et plafond de
    $N_j/D$ (en local), puis $+o_j$, puis saturation à la boîte ; ce point est dans le pavé, jamais calculé depuis le
    centre absolu. Une projection rationnelle du centre n'est pas un minimum sur les points entiers (segment de 0 à 1 :
    minimum continu $-1/2$, minimum entier 0) ;
  - une boîte **non contenue** dans le pavé n'est pas contenue dans la boule : son majorant (coin lointain) est positif
    sans arithmétique ; le coin lointain n'est évalué que pour une boîte contenue dans le pavé, au budget mixte du § 3.
    Une boîte partiellement dans le pavé peut contenir tout le support et le centre (support
    $\lbrace(100,100,100),(102,100,100)\rbrace$, boîte $[0,200]^{3}$) : elle n'est jamais rejetée, elle est raffinée.

  Preuve du minimum : $c=\sum\lambda_j s_j$, $\lambda_j\geq0$, $\sum\lambda_j=1$ et $\|s_j-c\|^2=R^2$ donnent,
  pour tout centre $y$, $\sum\lambda_j\|s_j-y\|^2=R^2+\|c-y\|^2$. Une boule de centre $y$ contenant $S$ a donc un
  rayon au moins égal à $R$. La boule du milieu de la boîte fournit alors la borne de demi-diagonale ci-dessus.
  Cette preuve utilise la positivité certifiée, pas seulement une sphère passant par $S$ ; les candidates génériques
  restent exclues. Tous les sites de la coquille étendue $U$ sont sur la sphère, donc strictement dans le pavé.
  Les signes des bornes entières et les résultats de census sont inchangés ; les compteurs de garde et de voies
  peuvent changer. Aucun gain de temps ni de complexité du census ne découle du seul resserrement du pavé.

  Ce correctif remplace deux usages de la v11 hors garde (`CST-0109`) : `LatticeSphere` (`src/index/census.cpp`,
  chemin chaud) calculait le centre absolu $o_jD+N_j$ ($5B+6$ bits, d'où $B\leq 24$) et évaluait la puissance au coin
  lointain de boîtes situées n'importe où dans le domaine.
- **`NUM-REQUETE` (requêtes à centre entier, `CST-0110`).** Les requêtes des $k$ plus proches d'un site (`core`,
  amorce d'un recensement) n'ont pas de boule avant leurs $k$ premiers candidats. Un écart $\lvert\Delta\rvert<2^{32}$
  donne $\Delta^{2}<2^{64}$ : chaque carré tient dans un `u64`, mais la somme de trois carrés peut déborder à $B=32$
  (elle tient jusqu'à $B=31$). Règle : somme en `u64` contrôlée, recalculée en `u128` au débordement ; la voie native
  est garantie quand l'étendue de la requête (site interrogé et candidats) est au plus $2^{31}$. Dès que $k$ candidats
  sont connus, le rayon courant élague, et le repère de la requête se resserre.

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
| distance du réservoir avant G1, $\sum_j(2x_j-lo_j-hi_j)^{2}$ | $2s+4$ | $s\leq 29$ | toujours | feuille (`CST-0208`) |
| numérateur du centre q3 | $5s+5$ | $s\leq 11$ | $s\leq 24$ | plus petite boule |
| dénominateur du centre q3 | $4s+5$ | $s\leq 14$ | $s\leq 30$ | idem |
| numérateur du centre q4 | $4s+5$ | $s\leq 14$ | $s\leq 30$ | idem |
| dénominateur du centre q4 | $3s+4$ | $s\leq 19$ | toujours | idem |
| côté d'un point du repère | $6s+8$ | $s\leq 9$ | $s\leq 19$ | feuille, certificat |
| orientation avec centre, sites du repère | $7s+9$ | $s\leq 7$ | $s\leq 16$ | feuille (J2, J3) |
| numérateur du niveau | $8s+12$ | $s\leq 6$ | $s\leq 14$ | niveaux |
| dénominateur du niveau | $6s+8$ | $s\leq 9$ | $s\leq 19$ | niveaux |
| comparaison de deux niveaux | $14s+20$ | $s\leq 3$ | $s\leq 7$ | tri des événements |
| **côté d'un site gardé** (budget mixte) | $6s+11$ | $s\leq 8$ | $s\leq 19$ | recensement |
| **orientation avec centre, sites gardés** (budget mixte) | $7s+14$ | $s\leq 7$ | $s\leq 16$ | `LEM-T7`, support canonique |
| orientation d'un quatrième site (`strictly_inside`, sites gardés, sans centre) | $3s+10$ | $s\leq 17$ | toujours | `LEM-T7` |
| triangle strictement aigu (`strictly_acute`) | $2s+7$ | $s\leq 28$ | toujours | q3 |
| **test du milieu** (support à deux sites, canonisation), forme locale | $5s+8$ (sites gardés), $5s+6$ (repère d'une feuille) | $s\leq 11$ | $s\leq 23$ | catalogue, hôte et appareil |

**Budgets mixtes** (`CST-0111`). Le centre $c=o+N/D$ garde les bornes de son repère ($D<24M^{4}$,
$\lvert N_j\rvert<24M^{5}$ pour q3, $M=2^{s}$) ; seuls les sites confrontés s'éloignent :

- côté d'un site gardé $z$ : $\lvert z_j-o_j\rvert<3M$ (garde ; majorant conservé, le pavé resserré de `NUM-GARDE` donne
  $\lvert z_j-o_j\rvert<2M$), d'où
  $\lvert D\lvert z-o\rvert^{2}-2N\cdot(z-o)\rvert<648M^{6}+432M^{6}<2^{11}M^{6}$, soit $6s+11$ bits (q4 et q2 en
  dessous) ;
- orientation de $c$ par rapport au plan de trois sites $p,q,r$ de la coquille (prédicats `orientation` et
  `strictly_inside` de `src/supports/enumerate.cpp`) : les sites sont sur la sphère, à moins de $2R<4M$ l'un de l'autre
  et de $o$, d'où un produit vectoriel $<32M^{2}$ par composante, $\lvert N+D(o-p)\rvert<120M^{5}$, et un total
  $<11\,520M^{7}<2^{14}M^{7}$, soit $7s+14$ bits ; pour trois sites gardés quelconques (écarts sous $5M$, normale sous
  $50M^{2}$, $\lvert N_j+D(o_j-p_j)\rvert<96M^{5}$), le total reste sous $14\,400M^{7}<2^{14}M^{7}$ (addendum de
  l'auditeur) ;
- test du milieu d'un support à deux sites $a,b$ : la v11 calculait $2(Do_j+N_j)=D(a_j+b_j)$ en coordonnées
  absolues ($5B+7$ bits, d'où $B\leq 24$ ; `src/catalogue/support.cpp`, `src/num/predicates.cpp`, et sur l'appareil
  `src/catalogue/leaf_device_predicates.hpp`) ; forme locale $2N_j=D\,((a_j-o_j)+(b_j-o_j))$, de $5s+8$ bits pour des
  sites gardés et $5s+6$ bits dans le repère d'une feuille (`CST-0114`).

La règle grossière (tout au budget de $s+2$) donnerait $6s+20$ et $7s+23$, natifs seulement jusqu'à $s=17$ et $s=14$ ;
les budgets mixtes gardent les seuils du repère ($s\leq 19$ et $s\leq 16$). Contre-lus : addendum
[`ADDENDUM_CONTRAT_NUMERIQUE_20261007.md`](../receipts/audit_canal_20261007/archives/ADDENDUM_CONTRAT_NUMERIQUE_20261007.md) (`f6f65a0d8`).

Conséquences :

- **Trois voies par expression**, comme en v11 : native quand l'étendue du repère la garantit (aucun contrôle) ; sinon
  contrôlée (`__builtin_*_overflow`, tout drapeau abandonne la valeur sans résultat partiel) ; sinon entiers larges à
  largeur fixe, recalculés depuis les coefficients d'origine.
- **Certificats liés à leur domaine** (`CST-0201`). Les certificats de puissance q3 et d'orientation de la v11 ont des
  seuils qui lisent $B$ (`power_certificate.hpp` : $D<2^{123-2B}$ et $\lvert N_j\rvert<2^{124-B}$ ;
  `orientation_certificate.hpp` : $D<2^{124-3B}$ et $\lvert N_j\rvert<2^{124-2B}$). Un certificat de la v12 porte son
  domaine : l'exposant $t$ qui borne toutes les différences qu'il couvre ; le recensement gardé exige $t=s+2$, jamais
  $s$. Témoin : support aigu d'étendue $s=20$ et requête au coin du pavé, dont le premier produit $D\lvert q\rvert^{2}$
  dépasse $2^{127}$ alors que le résultat tient ; le certificat construit pour $s$ l'accepte, celui pour $s+2$ le refuse.
- **Types reconstruits.** Chaque intermédiaire du code porté reçoit le type de son budget en $s$ (la v11 fixait par
  exemple `DotInt` à `i64` et les centres à `i128` d'après $B$) ; aucune conversion rétrécissante avant un
  `__builtin_*_overflow`, qui ne certifierait rien ; un refus recalcule depuis les coefficients d'origine.
- **La voie est uniforme par repère** (une feuille, une boule). Sur le GPU, quand un warp traite une seule feuille, la
  voie est uniforme sur le warp.
- **Les niveaux ne sont plus garantis natifs au-delà de $s=14$**, ni leurs comparaisons au-delà de $s=7$ (les seuils
  de la table sont des garanties suffisantes, pas des impossibilités) :
  comme en v11, le tri des événements passe par les clés F3/F4 à repli exact. Un numérateur de niveau tient dans 192
  bits jusqu'à $s=22$ et dans 320 bits jusqu'à $s=32$ ; une comparaison exacte de repli tient dans 320 bits jusqu'à
  $s=21$ et dans 512 bits jusqu'à $s=32$. La largeur des entiers larges est donc fixée par le palier, jamais par $B$.
- **Paliers proposés** (constantes de compilation communes à l'hôte et à l'appareil) : étroit $s\leq 16$ (orientation,
  côté, centres natifs `i128`), moyen $s\leq 24$ (centres natifs, côté contrôlé ou certifié), large au-delà. La v11 avait
  un seul palier étroit sur le GPU, l'étendue $2^{20}$ ; ces paliers le remplacent s'ils gagnent au microbanc, sinon la
  v12 garde la coupe de la v11.

## 4. Ce qui lit encore le domaine global ou un ordre

| Usage | Bits | Traitement |
| --- | --- | --- |
| lecture et contrôle de l'entrée | $B\leq 32$ | refus explicite au-delà |
| clé de Morton exacte | $3B_{\mathrm{eff}}\leq 96$ (`u64` si $B_{\mathrm{eff}}\leq 21$, `u128` sinon) | identité des sites et ordre interne ; ci-dessous |
| centre absolu $o+N/D$ (export des naissances) | $B+4s+6$ (v11 : $5B+6$ à $s=B$) | entiers larges, à l'export seulement |
| comparaison de deux centres absolus (`compare_centers`, naissances de même rang, seul appelant de la v11) | $9B+11$ en v11 ; en v12, parties entières (64 bits suffisent **pour une boule certifiée**, centre dans l'enveloppe de ses sites ; calculées en `i128`, `Big` au palier large, ce qui couvre aussi les candidates génériques ; dividende $N_j$ de $5s+5$ bits), puis parties fractionnaires sur $8s+10$ bits | en deux temps, sans entier de la taille de $B$ ; même ordre que la v11 |
| test du milieu de la canonisation (v11 : $2(Do_j+N_j)=D(a_j+b_j)$) | $5B+7$ en v11 ; $5s+8$ en forme locale | § 3 (`CST-0114`) |
| export d'un niveau | numérateur $\leq 8s+12$, dénominateur $\leq 6s+8$ | mots de 64 bits, nombre de mots en tête |
| distances carrées des requêtes à centre entier | $2B+2$ | `NUM-REQUETE` |

**Comparaison de centres en deux temps.** L'ordre (niveau, centre) des naissances est invariant par translation ;
seule sa largeur lisait $B$. Pour $c=o+N/D$ avec $D>0$, la partie entière $\lfloor c_j\rfloor=o_j+\lfloor N_j/D\rfloor$
tient sur 64 bits **quand la boule est certifiée** (son centre est dans l'enveloppe de ses sites, donc dans
$[0,2^{32})^{3}$) ; le centre d'une candidate générique peut en sortir (témoin q3 de l'auditeur,
`(0,0,0)`, `(4294967295,4294967294,0)`, `(4294967294,4294967293,0)` : planchers au-delà de `i64`), et
`centers.cpp` calcule la partie entière en `i128` (`Big` au palier large), ce qui couvre les deux cas ; deux centres de parties entières différentes sont ordonnés par elles, sinon par leurs parties
fractionnaires $(N_j-\lfloor N_j/D\rfloor D)/D$, comparées par produits croisés de moins de $8s+10$ bits (natifs
jusqu'à $s=14$, $s$ le plus grand des deux repères). Préconditions : $D>0$ (signe normalisé en q4) et plancher
mathématique pour $N_j<0$. Le dividende $N_j$ du plancher a $5s+5$ bits en q3 : entier large au-delà de $s=24$, même
quand le quotient est petit. Même ordre que `compare_centers` (contre-lu, `f6f65a0d8`).

**Ce que la clé de Morton décidait dans la v11** (`CST-0113`). Le support canonique $S^{*}$ y est le support de
cardinal minimal, puis le premier dans l'ordre lexicographique des `SiteIdx`, qui sont des rangs de Morton
(`src/catalogue/support.cpp`) : sur une coquille à plusieurs supports minimaux (le carré et ses deux diagonales),
$S^{*}$ dépendait de la clé, donc l'ordre des `BallIdx`, les lignes de la sortie `supports` et la convention
`cover_v10` aussi. Et les sections de sites des sorties FULL, `points` et `supports` sont écrites dans l'ordre de Morton
**absolu**, que le lecteur strict exige et hache (`bench/full_semantic.py`, l. 110 à 113). Les ex æquo des plus
proches, les représentants et les propositions ne changent que des compteurs, pas les forêts (théorème D).

**Règles de la v12** :

- **$S^{*}$ ne lit plus aucun ordre interne** : support de cardinal minimal, puis plus petite liste triée des
  positions de ses sites dans l'ordre lexicographique des coordonnées. Cet ordre est invariant par translation et
  équivariant par permutation ; il ne diffère de celui de la v11 que sur les coquilles à plusieurs supports minimaux
  (0,02 à 0,04 % des boules selon la contre-lecture des lemmes T).
- **Clé de Morton exacte sur les coordonnées absolues, identité des sites** (`CST-0202`). La clé entrelace les
  coordonnées absolues sur $3B$ bits, sans troncature : `u64` au profil 21, `u128` (96 bits) aux profils 24 et 32.
  Égalité de clé et égalité de position sont alors équivalentes : la clé reste l'identité des sites, comme dans la
  v11 (`cloud.cpp` regroupe les clés égales en un site), donc la détection des multiplicités (refus D8 ou option
  « sites distincts »). La clé tronquée de la première rédaction est abandonnée : elle fusionnait des positions
  distinctes, et un départage par `PointId` pouvait séparer deux vrais doublons par un troisième site de même clé.
  Une clé prise sur les coordonnées **normalisées** (moins le minimum) a été essayée puis écartée le 7 octobre :
  l'ordre de Morton n'est pas invariant par translation (pour $P=(2,0,0)$ et $Q=(1,1,0)$, de minimum $(1,0,0)$, $Q$
  précède $P$ en clés absolues et le suit en clés normalisées), si bien qu'elle changeait l'ordre des sites que lisent
  les juges du socle (neuf portes au rouge) et la liste parente du parcours, donc le différentiel des feuilles contre
  la v11 (`CST-0113`). La clé absolue garde l'ordre de la v11 ; la coupe de l'index lit les coordonnées absolues ; les
  boîtes de l'index restent exactes, réunies de bas en haut ; la clé n'entre dans aucun ordre publié. Coût : au profil
  32, toutes les clés ont 96 bits, mesuré avec la décision D6.
- **Exports** : la sortie FULL au schéma de la v11, qui ne contient pas $S^{*}$, écrit ses sites dans l'ordre de
  Morton absolu sur `bits` bits, par un tri à l'export (clé absolue de 96 bits si $B>21$), hors du chemin chronométré :
  sur les **mêmes coordonnées absolues**, l'empreinte sémantique de la v11 se reproduit à l'octet. Cette empreinte hache
  des coordonnées et des centres absolus : elle juge la conformité v11/v12, jamais l'invariance par translation, qui a
  sa propre porte (§ 7). Les sorties qui publient $S^{*}$ (`supports`, `cover`)
  changent de schéma ; la comparaison à la v11 passe par un lecteur qui retrie, et chaque écart de $S^{*}$ doit être une
  coquille à plusieurs supports minimaux, vérifiée en exact.

## 5. Flottant

Doctrine F1–F6 de la v11 inchangée : aucune décision en flottant ; noyaux binary64 exacts (F2) quand la somme des valeurs
absolues des termes développés reste sous $2^{53}$ (produits scalaires exacts pour $s\leq 25$) ; clés F3/F4 des niveaux à
repli exact ; filtres de signe F6 à seuil certifié par expression. Les propositions flottantes (plus petite boule de
Welzl, `LEV-MEB-CERT`) se calculent **dans le repère local** (différences exactes en binary64 pour $s\leq 53$) et ne
décident rien : elles sont certifiées par le catalogue (`LEM-T1`, deux inclusions) ou en exact avant toute confrontation
extérieure (`NUM-CERTIFIEE`).

## 6. Appareil (GPU)

Le contrat R7 de la v11 est conservé : sur l'appareil, seules décident les voies natives garanties par le palier du
repère ou par un certificat de fabrique ; tout autre cas rend la feuille « non résolue », rejouée en exact sur l'hôte
avant admission, **en parallèle**, et comptée. Les paliers, les budgets et la garde sont une source unique commune.

## 7. Portes et témoins

- **Bornes de palier** : pour chaque expression de la table, un témoin à l'étendue limite $s^{*}$ et un à $s^{*}+1$
  (par exemple côté q3 à $s=19$ et $s=20$, orientation à $s=16$ et $s=17$, côté gardé à $s=19$ et $s=20$), joués sur
  l'hôte et sur l'appareil ; le repli doit être effectivement emprunté (compteur).
- **Garde** (la sphère est strictement dans le pavé : contact à la sphère et contact au pavé sont deux témoins
  distincts) : site sur la sphère, site au bord intérieur du pavé hors de la sphère, site juste hors du pavé ; boîte
  disjointe du pavé, boîte qui le touche par un coin, boîte partielle qui contient tout le support (support
  $\lbrace(100,100,100),(102,100,100)\rbrace$, boîte $[0,200]^{3}$, à raffiner et jamais rejeter), boîte à cheval sur
  le pavé à $B=32$ (`CST-0109`) ; minimum entier contre minimum continu (segment de 0 à 1) ; candidate non certifiée
  $\lbrace(419,0,0),(435,15,0),(434,14,0)\rbrace$ gardée dans la voie générique ; mutants « garde d'un bit trop
  étroite » et « recensement avant certificat » sur un triangle presque aligné (`CST-0108`), tués.
- **Certificats** : support aigu d'étendue 20 et requête au coin du pavé, premier produit au-delà de $2^{127}$ ; mutant
  « certificat du support seul » tué ; les intermédiaires sont vérifiés, pas seulement le signe final (`CST-0201`).
- **Repère des boîtes** : sites $(0,0,0)$ et $(2^{32}-1,0,0)$, fermeture de boîte à $2^{32}$, $s=33$ (`CST-0204`) ;
  réservoir à $s=30$, boîte $[0,1]^{3}$ et site $(2^{30}-1)^{3}$, au-delà de `i64` (`CST-0208`).
- **Couverture** : une feuille dont un site de la liste est loin hors de la boîte ; un témoin présent dans la liste du
  parent et absent de celle de l'enfant (`CST-0112`) ; mutant « repère pris sur la seule boîte » tué.
- **Requêtes** : deux sites aux coins opposés de $[0,2^{32})^{3}$, dont la distance carrée $3(2^{32}-1)^{2}$ déborde
  `u64` (`CST-0110`).
- **Support canonique** : le carré et ses deux diagonales, puis le même carré translaté et permuté : même $S^{*}$
  (`CST-0113`).
- **Témoins hérités** : `WIT-S21` (triangle aigu d'étendue $2^{21}-1$, premier produit $6\cdot(2^{21}-1)^{6}$ au-delà
  de $2^{127}$), `WIT-U24` étendu à u32 (tétraèdre régulier à l'extrémité du domaine, export du niveau sur plusieurs
  mots), `WIT-F3` et `WIT-F2`.
- **Invariance par translation**, jugée **à translation près** : toute translation entière qui garde l'entrée dans
  $[0,2^{32})$ laisse inchangés les niveaux, les forêts, les verticales et $S^{*}$ ; les centres et les coordonnées sont
  ramenés par la translation inverse, et les sites rangés par `PointId`. Le lecteur strict de la v11, qui hache des
  coordonnées absolues et l'ordre de Morton absolu, ne sert pas à cette porte.
- **Clé et identité** (`CST-0202`) : trois positions dont deux auraient la même clé tronquée ; deux vrais doublons
  séparés dans l'entrée par un troisième site ; permutations à `PointId` stables ; refus D8 et option « sites
  distincts » ; deux amas denses aux extrémités de $[0,2^{32})$ (clés de 96 bits), résultat identique au nuage
  translaté, temps publié.

## 8. Mesures

- `MES-S` : histogramme des étendues locales (feuilles, supports, parties de descente) sur les trames sans sol de
  plusieurs séquences, au millimètre et au dixième de millimètre, et sur les scènes de plusieurs millions de points ;
  part de chaque voie (native, contrôlée, large) ; part des feuilles dont l'étendue vient d'une minorité de sites.
- **Coût du profil, au sens de D6** (`CST-0207`) : binaire u21 de référence contre binaires candidats u24 et u32,
  mêmes trames, régime D1–D3, sources et constructions épinglées, règle statistique écrite avant les prises ; le
  candidat le plus large dont le surcoût reste sous 3 % devient le profil unique du produit.
- **Translation**, banc distinct qui ne décide pas D6 : dans le candidat, la même trame et ses translations jusqu'aux
  deux bords de $[0,2^{32})$, pour la correction (porte à translation près) et le coût.
- La même trame quantifiée au dixième de millimètre est mesurée et publiée, sans objectif.

## 9. Questions ouvertes

1. Les certificats de fabrique de la v11 (puissance q3, orientation), à relire au port.

Les budgets mixtes du § 3 et la comparaison de centres en deux temps du § 4 sont contre-lus (addendum `f6f65a0d8`).
Usages absolus de la v11 relevés et remplacés : `LatticeSphere` (`CST-0109`), `compare_centers` (§ 4), test du
milieu (`CST-0114`) ; selon l'addendum, il n'en reste aucun autre dans `src/num` et `src/catalogue`.
