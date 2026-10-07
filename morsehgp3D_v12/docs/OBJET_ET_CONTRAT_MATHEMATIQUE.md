# Objet et contrat mathématique de la v12

7 octobre 2026. Ce document dit **quoi porter** du contrat mathématique de la v11, sous quels identifiants, avec quels
témoins, et ce qui reste ouvert. Les énoncés eux-mêmes et leurs preuves sont dans
`../../morsehgp3D_v11/docs/MATHEMATIQUES.md` ; leur lecture de bout en bout, rattachée à la thèse, est au § 1–2 de
l'audit géant (`../../morsehgp3D_v11/docs/AUDIT_GEANT_V11.md`). Toutes les preuves y ont été refaites : aucune erreur
de fond, mais des degrés de rédaction inégaux et un registre lacunaire.

## 1. L'objet

- **Entrée** : $n$ sites distincts de poids un sur $[0,2^{B})^{3}$, $B=21$ ; ordre maximal $K\leq\min(n,12)$.
- **FULL** : pour chaque $k\leq K$, la forêt $T_k$ des composantes de $L_k(a)$ quand $a$ croît ; fusions N-aires à
  plateau atomique ; coupes fermées et ouvertes ; verticales vers l'ordre $k-1$ à la coupe fermée ; niveaux = rayons
  carrés rationnels exacts, non réduits, comparés par produits croisés ; numérotation canonique (naissances par niveau
  puis centre exact, fusions par niveau puis plus petite naissance).
- **Modèle fini** : $\Gamma_k(a)$ ($k$-parties de plus petite boule englobante $\beta\leq a$, reliées par les
  $(k+1)$-parties de même seuil) a les composantes de $L_k(a)$ (T1). Il définit l'objet et l'oracle borné ; il n'est
  jamais matérialisé à l'échelle.
- **Rapport à la thèse** : $T_k$ a pour ensembles de points les $K$-polyèdres de la Déf. 22 (Th. 2). La tour
  multi-ordres et les verticales ne sont pas dans la thèse. La Prop. 6 et le Th. 5 de la thèse sont faux (E5, L02) ;
  l'Alg. 1 calcule une variante « Gabriel ».
- **Vues** : `hgp_reduced` (composantes d'au moins deux sommets, c'est la qualification $\Pi_{k+1}$ des points), la
  hiérarchie de points $H^{r}_{K+1}$, la sortie plate, l'arbre couvrant d'ordre K (squelette), les polyèdres d'ordre k
  (en aval).

## 2. Identifiants

Chaque énoncé, témoin, levier ou mesure de la v12 reçoit un identifiant **unique et préfixé**. La v11 réutilisait les
mêmes lettres pour des objets différents (J2, J3, P1, G4, V3, R, E1, D2, T1, U2), ce qui a causé des erreurs de lecture.

| Préfixe | Domaine | Exemple |
| --- | --- | --- |
| `OBJ-` | objet et théorèmes de structure | `OBJ-T2` (trace stricte) |
| `CAT-` | catalogue et complétude conditionnelle | `CAT-G3` (élagage d'un support partiel) |
| `TOW-` | tour : plateaux, descentes, graines, verticales, forêt | `TOW-P` (plateau) |
| `PTS-` | des forêts aux points | `PTS-P5` (stabilité en rayon) |
| `SUP-` | sortie squelette (supports) | `SUP-F` (supports minimaux) |
| `JUG-` | juges et filets | `JUG-EULER` |
| `NUM-` | doctrine numérique | `NUM-F3` |
| `LEM-` | lemmes de calcul (raccourcis exacts) | `LEM-POP` (table de populations) |
| `WIT-` | témoins exacts | `WIT-E5` |
| `LEV-` | leviers de performance | `LEV-MEB-CERT` |
| `MES-` | mesures et microbancs | `MES-M3` |

Chaque identifiant est relié à une ligne du registre des preuves (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`), à un
endroit du code, à une porte et, s'il en a un, à un mutant.

## 3. Énoncés à porter

| Identifiant v12 | Énoncé (v11) | Preuve dans `MATHEMATIQUES.md` | Registre racine | À faire dans la v12 |
| --- | --- | --- | --- | --- |
| `OBJ-M1`, `OBJ-M2` | plus petite boule unique ; certificat combinatoire | complète | aucune ligne v11 | ligne au registre |
| `OBJ-T1` | modèle fini $\Gamma_k$, coupes fermées et ouvertes, verticales | complète | via le Th. 2 de la thèse | ligne propre |
| `OBJ-T2` | trace stricte ⇔ séparable (remplace la position générale) | complète | aucune ligne v11 | ligne ; c'est la clé de voûte |
| `OBJ-T3` | événement d'une boule à l'ordre $k$ ; fenêtre $[p+q-1,p+m]$ | complète | aucune | ligne |
| `TOW-P` (ancien T4) | plateau atomique, énoncé comme le lemme P du § 10.3 | complète au § 10.3 ; recette au § 5 | `proved_here` (§ 10) | énoncer T4 **comme** le lemme P |
| `TOW-T5` | descente valide, terminaison, terminal non unique | complète | aucune | ligne |
| `TOW-T6` | suffisance constructive, verticales comprises | esquissée | aucune | rédiger en théorème complet, hypothèses : catalogue complet par rang, graines de date initiale $<\lambda$ |
| `CAT-G1`…`CAT-G4` | listes K-certifiées, recensement local, élagage, complétude **conditionnelle** | complètes | aucune | lignes ; G4 reste conditionnel (ni terminaison ni borne de travail) |
| `PTS-P1`…`PTS-P5` | core, cover, témoins forts, laminarité à ordre fixé, stabilité en rayon | complètes | trilemme 3ε (V11) | lignes ; garder les contre-exemples de P4 et P5 |
| `JUG-J1` | restriction de $\mathrm{Cat}_{K'}$ | complète | V11 (Euler) | — |
| `JUG-EMST` (ancien J2) | ordre un = arbre couvrant euclidien minimal coupé à $4a$ | complète | aucune | **l'implanter** : c'est le juge d'échelle le moins coûteux, jamais implanté en v11 |
| `JUG-EULER` (ancien J3) | identité d'Euler par ordre ; $\mathrm{Cat}_{K+2}$ suffit ; filet, pas certificat | résumée ; complète en v9 | `proved_here` | importer la preuve par le nerf (v9, `559c8ab84`) |
| `SUP-A`…`SUP-H`, `SUP-W` | rattachement, rôles, branches, graines, juge, supports minimaux, comptes, polyèdres datés, périmètre (Th. 4 sans position générale) | complètes | `proved_here` (§ 10) | réparer la ligne E fusionnée à D2 (`\n` littéral, l. 1370) |
| `SUP-KRUSKAL` | les boules de naissance et de fusion déterminent $T_K$ ; sélection de Kruskal au plateau | conclusion juste, citation fausse (C.3 au lieu de P.3) | **absente** | corriger la citation, ajouter la ligne |

## 4. Lemmes de calcul

Aucun n'a de ligne au registre en v11 ; ils sont dispersés dans des documents de conception. La v12 les rassemble dans un
seul contrat, chacun avec sa fixture d'égalité.

| Identifiant v12 | Énoncé | Source v11 | Usage |
| --- | --- | --- | --- |
| `LEM-POP` | une $k$-partie égale à $P_b$ est un pas terminal ($p<k$, $t=m$, $b\in\mathrm{Cat}_K$) | `src/tower/population_lookup.*` | arrêt des descentes |
| `LEM-R` | dominateur d'un générateur = intérieur ; dominé = extérieur | `docs/CATALOGUE.md` | census par masques dans la feuille |
| `LEM-LATTICE` (ancien V3) | minorant exact de la puissance sur les **points entiers** d'une boîte ; **réservé aux sites** | `docs/INDEX.md` | census sur l'index |
| `LEM-DIAM` | test du diamètre de la paire la plus éloignée | `docs/MEB_DIAMETRE.md` | plus petite boule |
| `LEM-CLIQUE` | un support centré dans la boîte est une clique du graphe de paires | `docs/CATALOGUE_SMALL_PAIR_GRAPH.md` | feuille |
| `LEM-ZONO` (ancien lemme Z) | droite équidistante de trois points ∩ boîte ⇔ zonogone contenant 0 | `docs/CENTER_REGION.md` | filtre J2 de la feuille |
| `LEM-M3`, `LEM-E4` | enveloppes du triangle médian et du tétraèdre | `docs/CATALOGUE.md` | filtres purs (coûteux sur CPU) |
| `LEM-DEFER` | niveaux q3 et q4 matérialisés après l'admission | `docs/MEB_CONSTRUCTIONS_DIFFEREES.md` | feuille |
| `LEM-VREUSE` | réemploi vertical régulier ($m=q$) | `docs/FULL_REGULAR_VERTICAL_REUSE.md` | verticales |
| `LEM-MSTC` | sur les étoiles de graines pondérées par rang, tout arbre couvrant minimal a les composantes par seuil ; contracter les chaînes de fusions de même rang redonne la forêt N-aire ; **le MST ne transporte pas les compteurs logiques** | `receipts/audit_plan_gpu_20261006/mathematics/REPORT.md` | forêt parallèle (second recours) |

**Énoncés de la conception d'origine** (`../../morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_TOUR.md`,
annexe A ; prouvés par l'auteur, **contre-lus le 7 octobre 2026** par l'auditeur
([`AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md`](../receipts/audit_canal_20261007/archives/AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md) : les six preuves
tiennent, avec les hypothèses ci-dessous) et inscrits au registre des preuves
(`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, section V12). La première rédaction de `LEM-T1` dans ce tableau omettait
l'inclusion $S\subseteq F$ : elle était fausse (constat `CST-0101`, témoin `WIT-T1-CARRE`).

| Identifiant v12 | Énoncé | Usage prévu |
| --- | --- | --- |
| `LEM-T1` | certificat combinatoire de plus petite boule : si $S$ est un support de la sphère critique $b$ et $S\subseteq F\subseteq P_b$, alors $B(F)=b$ ; le moteur prend $S=S^{*}$ lu au catalogue et **teste les deux inclusions** sur les identifiants | `LEV-MEB-CERT` (76 % des plus petites boules des descentes sont au catalogue, mesure de la v10) |
| `LEM-T3` | arrêt d'une descente sur la première cellule de fenêtre, pointeurs strictement descendants suivis après coup ; la cible d'un pointeur ne vaut qu'à partir de $\beta(F_0)$ ; **aucune** garde $\beta(F_0)\leq\ell(r_b-1)$ | mémo de cellule déterministe ; porte avec `WIT-D2` et `WIT-MEMO` (`CST-0104`) |
| `LEM-T4` | noyau union-find sans lots : les classes d'événements binaires liés de même rang sont exactement les multifusions ; énoncé comme le lemme P.3 sous trois ponts : (H2) avec les cibles de `LEM-T3`, (H4) avec le lemme C.1, liste des jonctions égale aux cellules de $W_k$ (boules faibles et cellules inertes comprises) | `LEV-FOREST-D-F1`, contraction parallèle ; rend **caduque** l'obligation `REG:243`, sans la prouver (`CST-0103`) ; clé (rang, plus petite naissance) lue dans la numérotation canonique du § 1 (`CST-0107`) |
| `LEM-T5` | requêtes d'ancêtre par l'historique d'attache, profondeur au plus $\log_2$ du nombre de naissances ; `component_at` **sous l'hypothèse** $\mathrm{rang}(\ell)\leq r$, listes par survivant dans l'ordre de traitement | verticales et rattachements (`CST-0105`) |
| `LEM-T6` | image d'une naissance depuis le sommet laissé par la jonction de la même boule à l'ordre $k-1$ | verticales en $O(1)$ |
| `LEM-T7` | quotient local des coquilles étendues par la famille des fenêtres des plans par le centre, qui contient tous les séparables maximaux **sans s'y réduire** ; $O(m^{3})$ prédicats ; (iv) donne l'union couverte, pas un compte de sites nouveaux | coquilles étendues sans énumération (`CST-0106`) |

## 5. Témoins à graver d'abord

Chaque défaut grave de la v11 s'est vu sur un témoin de quelques sites ou à une borne exacte. La v12 grave ces témoins
**avant** le code, dans un catalogue lisible par une machine (`tests/fixtures/regressions/` à la racine, ou un catalogue
propre à la v12), chacun avec : l'énoncé réfuté ou fixé, les coordonnées, les attendus exacts, une commande de rejeu en
Python nu, une porte native et une ligne de registre. **Tout producteur et tout lecteur de hiérarchie** doit les passer.

| Identifiant | Données | Ce qu'il réfute ou fixe | Où il est aujourd'hui |
| --- | --- | --- | --- |
| `WIT-E5` | $(0,0,7),(0,9,6),(1,4,0),(0,0,1),(4,1,2)$, $K=2$ | Prop. 6 et Th. 5 de la thèse ; toute réduction par Gabriel ; retirer les liaisons hors $W_K$ | `tests/fixtures/regressions/gabriel_point_set_counterexample.json` ; `reference/test_supports.py` (`e5_window`) |
| `WIT-L02` | plan : $A=(-100,0)$, $C=(100,0)$, $z=(1,-90)$, $y=(30,-85)$, $w=(3,300)$, $K=2$ | Th. 5 : le graphe de Gabriel reste déconnecté pour toujours | **non gravé** : `../../morsehgp3D_v11/receipts/conception_v11_20261002/audit_v10/L02_MATH_TOUR.md` § 4.11 |
| `WIT-SIX` | deux triangles équilatéraux (thèse § 6.1) | HDBSCAN fusionne à $K=2$ ; fusion ternaire simultanée | `tests/fixtures/exact/chapter6_six_points.json` |
| `WIT-SEPT` | axe : $\lbrace 0,10,11,26,27,45,46\rbrace$ | une hiérarchie de points commune aux ordres | `reference/test_projection_contracts.py` |
| `WIT-024` | $\lbrace 0,2,4\rbrace$ puis $\lbrace 0,2,4+\delta\rbrace$, $k=2$ | stabilité de la projection par plus petit ancêtre commun ; propriétaire équivariant | idem |
| `WIT-025` | $\lbrace 0,2,5\rbrace$, $k=2$ | cover = MR₂-bord | idem |
| `WIT-CERCLE` | cercle unité, quatre points, puis un point déplacé ; immersions $t=1/n$ | stabilité de la réalisation par supports ; saut de $S^{*}$ | `reference/test_supports.py` (`circle_witness`) |
| `WIT-D2` | $(2,10),(18,10),(10,20),(9,3),(11,3)$, $K=2$ | « une trace stricte naît avant le rang précédent » | `reference/test_supports.py` (`d2_witness`), `tests/tower/attach_test.cpp` |
| `WIT-MEMO` | $\lbrace 0,2,4,6\rbrace$, $k=2$ | mémo valide dès la date terminale | `tests/tower/memo.cpp` |
| `WIT-TRI-EQ` | $(0,0,0),(1,1,0),(1,0,1)$, $K=1$ | « les boules de fusion forment un arbre couvrant » (cycle de plateau) | `tests/cli/supports_spanning_reader_gate.py` |
| `WIT-TRANSL` | $(0,1,1),(1,0,1),(1,1,0)$, $K=1$, translaté | équivariance par translation du squelette publié | **non gravé** : script de l'audit, `../../morsehgp3D_v11/receipts/audit_geant_v11_20261007/scripts/B/spv2_translation.py` |
| `WIT-FEUILLES` | cube $\lbrace 0,16\rbrace^{3}$ et son centre, K5, feuilles 8/32 | « feuilles ≤ sites » (80 feuilles pour 9 sites) | **modèle seulement** : `receipts/audit_gpu_euler_20261004/centre_leaf_counterexample/` |
| `WIT-EULER5` | cinq sites portant un triangle $+1$ et une paire $-1$ au niveau 25 | Euler comme certificat | `tests/catalogue/euler_limits.py` |
| `WIT-F3`, `WIT-F2` | $N=2^{53}+1$ ; $x=32767$, $A=512x^{3}$ | bornes flottantes par nombre d'instructions et par degré | reçus `audit_independant_20261002/floating_bounds`, `math_locks_review` |
| `WIT-S21` | triangle aigu à $s=2^{21}-1$ | « une borne sur le résultat borne les intermédiaires » | `docs/PREDICATS_I128_CONTROLES.md` |
| `WIT-U24` | tétraèdre régulier en u24 | export du niveau sur trois mots (196/148 bits) | porte `mhgp11_tower_points_export_width` |
| `WIT-RACINE` | $2^{127}-1$ | conversion d'une racine u128 en i128 | `tests/head/head_test.cpp` (`huge`) |
| `WIT-RADICAUX` | $(2,162,50)$ et $(8,98,32)$, tous deux $5\sqrt{2}$ | départage de sommes de radicaux en décimal ou en binary64 | `bench/points_gate.py` |
| `WIT-POLY` | trois réfutations du polyèdre d'ordre k | aberrants, chaînes contractées, emboîtement | `tests/fixtures/regressions/polyhedron_order_k_counterexamples.json` |
| `WIT-SPHERE50` | 84 sites entiers de $x^{2}+y^{2}+z^{2}=50$ ; coquille de 270 sites | capacité des coquilles étendues (refus explicite) | `docs/MATHEMATIQUES.md` § 10.6 |
| `WIT-T1-CARRE` | carré $(0,0,0),(2,0,0),(2,2,0),(0,2,0)$ ; $F$ un côté, proposition $S^{*}=\lbrace A,C\rbrace$ ; puis $F$ et proposition la diagonale $\lbrace B,D\rbrace$ | « $S=S^{*}(b)$ et $F\subseteq P_b$ suffisent » (`CST-0101`) ; support non canonique : recherche en échec, chemin exact | `reference/test_witness_t1.py` (mutant `sans_inclusion` tué) |
| `WIT-T7-CERCLE25` | cercle $x^{2}+y^{2}=25$ de $z=0$ : $(5,0,0),(4,3,0),(-3,4,0),(-3,-4,0)$ | « la famille de `LEM-T7` est celle des séparables maximaux » (`CST-0106`) | **à graver** avec la porte de `LEM-T7` |
| témoins du § 10.11 | carré à K1–K4, triangle droit, `growth_ABCZ`, passagère, ligne, triangle équilatéral faible, tétraèdre à intérieurs (K5), cube et octaèdre | rôles, comptes, polyèdres datés | `docs/MATHEMATIQUES.md` § 10.11 ; oracle S1 |

## 6. Doctrine numérique à porter

- **F1** : aucune décision en flottant. **F2** : noyaux binary64 exacts si la somme des valeurs absolues des termes
  développés reste sous $2^{53}$. **F3** : erreur propagée **par expression**, jamais par nombre d'instructions.
  **F4** : comparaison de clés à constante $c=1-2^{-40}$, repli exact sinon. **F5** : auto-test, sans valeur de
  preuve. **F6** : filtre de signe à seuil certifié, repli exact dans la bande et à l'égalité.
- **Budgets `constexpr` par expression** (`src/num/budgets.hpp` de la v11) : un débordement est une erreur de
  compilation. Avec $M=2^{B}$ : produit scalaire $2B+2$, déterminant $3B+3$, côté (puissance) $6B+8$, niveau
  $8B+12$ / $6B+8$, comparaison de niveaux $14B+20$, comparaison de centres $9B+11$ bits. Seul le côté d'un support q3
  ($216M^{6}$) dépasse l'`i128` natif en u21 : certificat calculé une fois, sinon voie contrôlée, sinon entiers larges.
- **Le flottant propose, l'entier décide** : c'est ce qui autorise la plus petite boule **proposée** en flottant puis
  **certifiée** (`LEV-MEB-CERT`), à condition que la proposition ne décide rien.
- **Pièges payés** : une borne sur le résultat ne borne pas les intermédiaires ; un minimum sur les points entiers
  d'une boîte n'est pas le minimum continu ; un invariant supposé (« feuilles ≤ sites ») est un défaut en attente ; un
  mutant compilé dans un profil où sa branche est éliminée est vide.

## 7. Questions ouvertes

| Priorité | Question | Pourquoi |
| --- | --- | --- |
| haute | identité des nœuds sous quantification : 61 à 67 % des nœuds K5 vivent moins de $2\delta$ ($\delta=\sqrt{3}/2$ mm) | seule une vie supérieure à $2\delta$ garantit une image stable (P5) ; les « séparations fugaces » sont sous ce seuil |
| haute | départage canonique invariant par translation ($S^{*}$, ordre de Kruskal) | le squelette publié par la v11 ne l'est pas |
| haute | contrat des compteurs logiques, indépendants de l'ordre de visite | préalable à toute forêt parallèle (`LEM-MSTC`) |
| haute | contre-lecture de `LEM-T1`, `LEM-T3`–`LEM-T7` | ils portent les changements d'algorithme de la tour |
| moyenne | contrat pondéré (modèle par copies) : T2, T3, G1–G3 avec poids, naissances nulles, fenêtres non contiguës | déjà implanté dans l'oracle ; à prouver avant tout port |
| moyenne | borne de travail et terminaison de la subdivision des boîtes | G4 reste conditionnel |
| moyenne | masses du § 9.1 sur FULL : famille de faces, tension avec `WIT-SIX` | condensation de la thèse |
| moyenne | règle de points stable par insertion, locale, qui passe T0 et Q1–Q4 ; synthèse multi-K par décalage en $k$ | hiérarchie de points |
| basse | polyèdre d'ordre $k$ : un représentant petit, certifié et robuste (huit questions à l'auditeur) | en aval |
| basse | partition T > 0 compatible avec u24 | sans effet mesuré sur LiDAR |

## 8. Registre des preuves : ce que la v12 y ajoute à l'ouverture

- Une section V12 avec une ligne par identifiant des § 3 et § 4 (statut, preuve citée par reçu ou commit, jamais par une
  note vivante).
- Les contradictions de la v11 qui n'ont pas leur ligne : cycle de Kruskal au plateau, « feuilles ≤ sites », F3 par
  nombre d'instructions, F2 par degré, mémo valide dès la date terminale, sept sites, $\lbrace 0,2,5\rbrace$, squelette
  et translation, L02.
- La réparation de la ligne E (l. 1370) et un contrôle des `\n` littéraux dans `tools/check_docs.py`.
- La clôture, avec sa portée, de l'obligation « contraction des plateaux » (`REG:243`) par le lemme P, `LEM-MSTC` et
  `LEM-T4`.
