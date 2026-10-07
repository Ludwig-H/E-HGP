# Expérience « rendu des niveaux » : faces exposées, strates isolées, ombre, offset

6 octobre 2026, de 22 h 14 à 23 h 18 UTC (heures lues par `date -u`). Expérimentateur du workflow « généraliser le
complexe alpha à l'ordre K pour représenter robustement les niveaux d'un polyèdre ». Prédictions gelées AVANT toute
mesure : [`PREDICTIONS.md`](PREDICTIONS.md) (22 h 19, sha256 dans `PREDICTIONS.sha256`).

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (prototype Python en aval de mhgp11 07428324e ; au plus 2 fils, nice -n 10)
profile=quantized_u21_input_only (grille de 1 mm)
public_status=not_claimed
GCP non utilisé. Aucun commit, aucune écriture hors de build/. Dépôt lu à origin/main = 5ca12e8dd ;
aucun commit « audit » postérieur à 28d70f8ab pendant l'expérience.
```

Étiquettes : [P] prouvé par l'auditeur (cité, non refait) ; [V] vérifié borné (oracle exact, petits nuages) ;
[M] mesuré ; [J] jugement visuel déclaré ; [C] conjecture. **Toutes les mesures sont sur de petits nuages (52 à
1 139 points) : coût, taille et sélectivité ne sont PAS mesurés à n = 8 000 / 16 000 / 32 000.**

## 0. En bref

1. **L'objet de l'auditeur se dessine, exactement et sans position générale, sur les huit jeux communs** [V/M] :
   mosaïque d'ordre K complète (importée d'`approche_mosaique`, jamais modifiée) passée à un **contrôle strict**
   nouveau (incidences 2-face/3-cellule, côtés opposés, faces de bord portées par l'enveloppe avec aires entières
   égales plan par plan, volume exact en entiers ; mutant « une cellule retirée » tué) ; composantes attribuées par
   **labels de naissance et incidences** ; **0 écart π0** avec l'arbre FULL d'ordre K à tous les niveaux critiques
   (32 couples jeu × K) ; trace de l'ombre sur P = amas discret (labels) sur **100 %** des couples (nœud, niveau).
2. **K = 1 redonne le complexe alpha** [M] : mêmes simplexes et mêmes niveaux que `gudhi.AlphaComplex` (exact) sur
   5 jeux sur 8 ; sur les 3 autres, les seules différences sont les cellules à coquille cosphérique (5 points ou plus
   sur la grille de 1 mm) que la mosaïque garde en polytopes et que gudhi triangule par perturbation symbolique ;
   0 écart de niveau sur les faces communes.
3. **Le rendu « dual exposé » est petit devant la mosaïque et garde les ouvertures des parties** [M] : à la fin de vie
   du nœud objet, 1,8 (K = 1), 5,6 (K = 2), 10,4 (K = 3), 18,8 (K = 5) faces exposées par point couvert (médianes),
   soit 2 à 7 % des faces de la mosaïque entière (24 à 1 080 faces par point). Ouvertures des roues conservées au
   niveau des nœuds « roue » : dual 6/7 à K = 5, ombre 3/7, offset 1/7, supports 4/7.
4. **Reconnaissabilité** [J] : la roue se reconnaît (anneau) à son propre niveau pour tous K ; **le vélo se reconnaît
   le mieux dans le dual A_K des ancêtres intermédiaires de la chaîne roue → vélo** (K = 5, r ≈ 77–89 mm sur
   synth_velo_05m : deux anneaux, cadre, selle, guidon), pas au nœud vélo de l'oracle (né à 151 mm, roues
   remplies par moyeu et rayons). Ombre : même topologie, plus épaisse (pièces jusqu'à 2r), remplit plus tôt.
   Offset : un bloc au niveau objet pour le vélo et le piéton (anneau épais pour l'anneau percé à 5 m), donc
   une enveloppe et non une forme. Supports S* : lisibles à K = 1–2 (arbre), confus à K = 5.
   Le vélo réel (découpe 08/002852) n'est reconnaissable dans aucun rendu à K = 5 (bloc), squelette partiel à K = 1.
5. **Les « niveaux » d'un nœud sont presque toujours dégénérés** [M] : 95 nœuds sur 754 (non racines) ont une vie
   d_v / b_v > 1,05 ; à K = 5 la marge relative médiane de la coupe intérieure vaut 0,1 % et seuls 16 nœuds sur 237
   ont une marge > √3 mm (bruit de quantification apparié). La forme évolue le long de la CHAÎNE des nœuds, pas
   pendant la vie d'un nœud : la bande de vignettes utile est celle de la chaîne (ou d'un arbre condensé).
6. **Robustesse et liens entre ordres** : voir §§ 5 et 6 (233 nœuds K = 5 liés, 233 cohérents ; 695 paires de chaîne, 695 commutent ; déplacement apparié de ±1 mm : d_B(H0) de 1.01 à 1.41 mm pour δ_max = 1,73 mm (6 cas sur 6 sous δ), composantes égales à marge > 2δ, mais faces exposées instables (jusqu'à un facteur 5) ; aberrant à 10 m sans effet).
7. **Sur la question ouverte (petit + preuve de topologie + applications de la hiérarchie + qualité mesurée)** :
   ce rendu donne exactement les deux premières garanties d'information (topologie : sous-complexe exact de la
   mosaïque contrôlée, lemme du nerf de l'auditeur [P] ; hiérarchie : π0, emboîtements, liens K = 5 → 2, 100 %) et une
   qualité mesurée (d_H(A, C) ≈ 0,83 à 1,0 r ; couverture des retours 87 à 99 %, précision 65 à 91 %), mais il n'est
   **ni petit** (≈ 19 faces par point couvert à K = 5, ≈ 10^6 par niveau sur une trame, extrapolé non mesuré) **ni
   stable comme dessin** (§ 6). L'ombre n'est pas plus petite (une pièce par cellule maximale) ; l'offset n'est pas
   un représentant. La suite utile est la réduction certifiée à sommets protégés de l'auditeur, mesurée contre ce
   rendu exact, et une sélection de nœuds à vie longue (arbre condensé) pour que les « niveaux » aient des marges.

## 1. Ce qui a été construit (`rendu.py`, `campagne.py`)

**Mosaïque.** `approche_mosaique/mosaique.construire(jeu.xyz, K, K_min=K)` (importé) sur le jeu entier : oracle
borné en aval de la tour, **hors du calcul de la hiérarchie** (invariant respecté ; coût dit au § 7).

**Contrôle strict** (`rendu.controle_strict`), parce que le certificat de volume à tolérance 1e-6 du module est aveugle
(trou de 29 220 mm³ connu) : (a) chaque 2-face bordée par 1 ou 2 cellules de dimension 3 ; (b) toute 2-face bordée
par une seule cellule portée par un plan de facette de l'enveloppe des sommets (facettes qhull vérifiées en entiers),
et aires entières projetées égales plan par plan ; (c) toute 2-face intérieure sépare ses deux cellules (signes
entiers) ; (d) somme exacte des volumes des cellules (cônes depuis un sommet, entiers) = volume exact de l'enveloppe.
(a)+(b)+(c) rendent la multiplicité de recouvrement localement constante, (d) la fixe à 1. Mutant : retirer une
3-cellule fait échouer (b) et (d) (mutant tué : 4 faces de bord hors de l'enveloppe, volume différent).

**Attribution.** Pour un nœud v et un niveau a = r² : sommets de naissance du sous-arbre (K = 1 : sites des feuilles ;
K ≥ 2 : k-partie P_b = I_b ∪ U_b de chaque boule de naissance, populations exactes du harnais) → composante du
1-squelette {R ≤ a} → toutes les faces de niveau ≤ a dont un sommet est dans la composante. **Jamais par position.**
Contrôles à chaque (nœud, niveau) : naissances toutes dans une seule composante (2 277/2 277), aucune naissance absente.

**Niveaux.** Naissance b_v ; coupe intérieure au milieu géométrique √(b_v d_v), marges publiées ; fin de vie
A_v(d_v^−) = {a_σ < d_v²} (plus grand niveau de la mosaïque strictement sous d_v², comparaison de flottants
correctement arrondis des mêmes rationnels). Racine : d_v = 2 b_v **déclaré** (borne d'échelle finie).

**Quatre rendus du même nœud au même niveau.**

| Rendu | Construction | Statut |
| --- | --- | --- |
| dual A_{K,v}(r) | 2-faces de bord (une seule 3-cellule active), strates isolées : 2-faces sans 3-cellule (orange), arêtes sans 2-face (bleu), sommets sans arête (magenta) | exact (sommets = barycentres rationnels) |
| ombre S_v(r) | réunion des conv(Q réels) des cellules maximales actives (3-cellules et strates isolées), sommets sur les données | trace sur P exacte ; dessin en voxels (pas r/8, ≥ 6 mm, demi-espaces décalés de h√3/2 : sur-ensemble déclaré) |
| offset C_v(r) ⊕ B_r | ε-net de grille E_v de C_v(r) (pas r/5, ≥ 8 mm ; d_K(y) ≤ r ; attribution par l'ensemble STRICT des K plus proches voisins → sommet → composante), puis E_v ⊕ B_r par transformée de distance | déclaré : E_v ⊕ B_r ⊆ offset ; ε non certifié ; les C de mesure nulle (naissances exactes) manquent |
| supports S* | conv(S*) du sous-arbre (référence, `H.Polyedre.depuis_noeud`) | constant sur la vie (MHGP11SP v2) |

**Caméra.** Une caméra FIXE par jeu (plan principal et cadrage des points des objets, directions de `H._base_vue`,
marge 600 mm) pour tous les candidats, tous K et tous niveaux : `H.rendre_png` recadre selon le polyèdre et ne permet
pas de fixer la caméra ; le rendu Pillow de `rendu.dessiner_panneau` reprend sa projection.

## 2. Oracles et contre-épreuves (exacts, petits nuages)

Oracle `oracle_bas.py` : force brute sur toutes les k-parties, facettes inférieures exactes des relevés
(Σ Q, Σ |q|²), faces identifiées par leurs familles de k-parties, niveaux par la règle des faces (`fixtures.py`,
`six_points.py` ; `resultats/fixtures.json`, `resultats/six_points.json`). Outil 3D (`mosaique.py`) inapplicable aux
nuages plats : ces cas sont traités en 1D/2D, l'exemple à six points en 2D (déclaré).

| Contre-épreuve | Attendu (auditeur, thèse) | Mesuré [V] |
| --- | --- | --- |
| P = {0, 2, 4}, k = 2, r = 1 | Ω = {1} ∪ {3} ; ombres [0, 2] et [2, 4] qui se touchent en 2 ; pas de fusion | 2 composantes ; labels {0, 2} et {2, 4} ; site 2 partagé ; ombres en contact, nœuds distincts |
| P = {0, 1, 2, 11}, k = 3, r² = 441/16 | Ω = [−13/4, 21/4] ∪ [23/4, 25/4] ; sommets 1 et 14/3 sans arête ; 14/3 dans la composante de GAUCHE | identique ; attribution par position : les deux barycentres dans l'intervalle de gauche (faux) ; par labels : {0, 1, 2} et {1, 2, 11} (juste) |
| P = {0, 1, 10}, k = 3, r = 5 | Ω = {5}, A = {11/3} | identique ; ombre [0, 10] ; trace sur P = {0, 1, 10} = labels |
| tétraèdre régulier, k = 2 | une cellule octaédrique de six barycentres ±e_i | 1 cellule ; 6 sommets, 12 arêtes, 8 triangles ; a = 3 |
| six points du § 6.1, K = 2 (2D, √3 · 1000 arrondi à 1732 : écart relatif des côtés ≤ 2,2e-5) | 7 amas au niveau r0 (AB, AC, BC, CD, DE, DF, EF), 3 au niveau 2√3/3 r0 ({A,B,C}, {C,D}, {D,E,F}), recouvrements | 7 → 3 → 1 composantes à r = 1 000, 1 155, 2 000 mm ; ombres AB…EF, puis ABC, CD, DEF qui se recouvrent en C et D sans fusion (figures 6.2 et 6.5 retrouvées) |
| six points, K = 3 | — | 0 composante à r0, 2 (ABC, DEF) à 1 155 et 1 500 mm, 2 (ombres ABCD et CDEF qui se CROISENT) à 2 000 mm : ombres sécantes, nœuds distincts |
| six points, K = 1 | complexe alpha | 6 sommets, 9 arêtes, 4 cellules dont 2 quadrilatères (A, C, D, E et B, C, D, F cocycliques par symétrie) que gudhi triangule (23 simplexes) ; 17 faces communes, 0 écart de niveau |

Planches : [K = 1](png/six_points_k1.png), [K = 2](png/six_points_k2.png), [K = 3](png/six_points_k3.png) (colonnes :
Ω_K(r) en raster coloré par composante, dual exact, ombre conv(Q), offset C_v ⊕ B_r en raster de 12 mm). Limite du
raster : une composante de mesure nulle (naissance exacte, ex. les sept points-lentilles à r = r0) n'apparaît ni
dans Ω ni dans l'offset ; elle apparaît dans le dual et l'ombre.


## 3. Contrôles sur les huit jeux communs

Dernière exécution retenue par couple (jeu, K) ; « strict » : contrôle strict du § 1 ; « écarts π0 » : nombre de
niveaux critiques où le nombre de composantes de A_K(r) diffère du nombre de nœuds vivants de l'arbre FULL d'ordre K
(`jeton.bijection`) ; « alpha = gudhi » : K = 1, mêmes simplexes et niveaux que `gudhi.AlphaComplex(precision='exact')`.

| jeu | K | n | 3-cellules | faces/point | strict | écarts π0 | alpha=gudhi | nœuds mesurés | s |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 1 | 279 | 1613 | 25.3 | True | 0 | True | 2 | 12 |
| decoupe_08_002852_deux_velos_6_51_instances | 2 | 279 | 6288 | 127.7 | True | 0 | — | 2 | 15 |
| decoupe_08_002852_deux_velos_6_51_instances | 3 | 279 | 15383 | 324.8 | True | 0 | — | 2 | 19 |
| decoupe_08_002852_deux_velos_6_51_instances | 5 | 279 | 45500 | 977.0 | True | 0 | — | 2 | 34 |
| synth_anneau_perce_05m | 1 | 170 | 832 | 22.4 | True | 0 | True | 1 | 4 |
| synth_anneau_perce_05m | 2 | 170 | 3083 | 107.7 | True | 0 | — | 1 | 3 |
| synth_anneau_perce_05m | 3 | 170 | 7332 | 261.5 | True | 0 | — | 13 | 6 |
| synth_anneau_perce_05m | 5 | 170 | 20600 | 741.5 | True | 0 | — | 1 | 10 |
| synth_anneau_perce_10m | 1 | 52 | 203 | 18.8 | True | 0 | True | 13 | 4 |
| synth_anneau_perce_10m | 2 | 52 | 719 | 85.7 | True | 0 | — | 13 | 4 |
| synth_anneau_perce_10m | 3 | 52 | 1649 | 198.1 | True | 0 | — | 12 | 4 |
| synth_anneau_perce_10m | 5 | 52 | 4196 | 505.2 | True | 0 | — | 11 | 4 |
| synth_deux_nappes_10m | 1 | 1139 | 6952 | 26.6 | True | 0 | False | 2 | 16 |
| synth_deux_nappes_10m | 2 | 1139 | 26253 | 131.5 | True | 0 | — | 2 | 25 |
| synth_deux_nappes_10m | 3 | 1139 | 65327 | 334.9 | True | 0 | — | 2 | 35 |
| synth_deux_nappes_10m | 5 | 1139 | 201949 | 1052.5 | True | 0 | — | 2 | 120 |
| synth_pieton_05m | 1 | 649 | 4043 | 27.1 | True | 0 | False | 23 | 21 |
| synth_pieton_05m | 2 | 649 | 15787 | 137.7 | True | 0 | — | 52 | 52 |
| synth_pieton_05m | 3 | 649 | 38919 | 352.6 | True | 0 | — | 56 | 95 |
| synth_pieton_05m | 5 | 649 | 117204 | 1078.6 | True | 0 | — | 69 | 213 |
| synth_velo_05m | 1 | 575 | 3453 | 26.2 | True | 0 | True | 35 | 31 |
| synth_velo_05m | 2 | 575 | 13320 | 131.6 | True | 0 | — | 51 | 47 |
| synth_velo_05m | 3 | 575 | 32556 | 333.4 | True | 0 | — | 59 | 121 |
| synth_velo_05m | 5 | 575 | 96397 | 1002.5 | True | 0 | — | 59 | 276 |
| synth_velo_10m | 1 | 203 | 1094 | 23.9 | True | 0 | True | 24 | 14 |
| synth_velo_10m | 2 | 203 | 4199 | 118.8 | True | 0 | — | 34 | 22 |
| synth_velo_10m | 3 | 203 | 10255 | 299.5 | True | 0 | — | 46 | 28 |
| synth_velo_10m | 5 | 203 | 29864 | 886.6 | True | 0 | — | 46 | 44 |
| synth_velo_occulte_10m | 1 | 581 | 3523 | 26.6 | True | 0 | False | 10 | 16 |
| synth_velo_occulte_10m | 2 | 581 | 13824 | 135.0 | True | 0 | — | 33 | 27 |
| synth_velo_occulte_10m | 3 | 581 | 34624 | 349.9 | True | 0 | — | 34 | 41 |
| synth_velo_occulte_10m | 5 | 581 | 105341 | 1086.3 | True | 0 | — | 47 | 206 |

Sur les trois jeux où « alpha = gudhi » est faux, `verif_alpha.py` établit que les faces de la mosaïque absentes de
gudhi sont TOUTES non simpliciales (15, 6, 59 faces) et que les simplexes de gudhi absents de la mosaïque sont TOUS
intérieurs à ces cellules dégénérées (45, 18, 181) ; 0 écart de niveau sur les faces communes. La grille de 1 mm crée
des quintuplets cosphériques (panneaux réguliers des nappes) : la mosaïque les garde en polytopes (subdivision
régulière exacte, sans simulation de simplicité), gudhi les triangule.

**Défaut trouvé et corrigé dans MON contrôle strict** : la première version calculait le volume de l'enveloppe en
cônes depuis son centroïde multiplié par le nombre de sommets, ce qui dépasse int64 ; elle a déclaré à tort
« incomplète » la mosaïque K = 5 du vélo occulté (écart de volume 80 %, alors que le certificat flottant du module
passait). Corrigé (cônes depuis un sommet, garde |sommes| < 2^19) puis rejoué (rejoué à 23 h 10 : complet, 0 écart π0). Le mutant « une
3-cellule retirée » reste tué après correction (4 faces de bord hors de l'enveloppe, volume différent).


## 4. Mesures (R1)–(R5) au niveau des nœuds

Nœuds : meilleur nœud oracle de chaque objet et de chaque partie (≥ 5 points), et ancêtres d'une partie jusqu'au
nœud objet (au plus 12 par partie, régulièrement en rang) ; trois niveaux chacun. Sélection par la vérité terrain :
une borne, pas une méthode.

**(R1) Topologie et (R2) hiérarchie, contrôles par couple (nœud, niveau)** [M] :

```text
couples (nœud, niveau) : 2277 erreurs : 0 []
P4 trace = labels : 2277 / 2277
P11 emboîtement précédent : 1518 / 1518 ; amas croissant : 1518 / 1518
P11 emboîté dans le parent : 754 / 754
P11 vie d/b > 1,05 (nœuds non racines) : 95 / 754
marges K=1 : 105 nœuds non racines ; marge > √3/2 mm : 37 ; > √3 mm : 28 ; dont objets/parties : 24, marge > √3 mm : 12 ; marge relative médiane 0.0142
marges K=2 : 188 nœuds non racines ; marge > √3/2 mm : 45 ; > √3 mm : 26 ; dont objets/parties : 29, marge > √3 mm : 7 ; marge relative médiane 0.0042
marges K=3 : 224 nœuds non racines ; marge > √3/2 mm : 35 ; > √3 mm : 24 ; dont objets/parties : 30, marge > √3 mm : 8 ; marge relative médiane 0.0018
marges K=5 : 237 nœuds non racines ; marge > √3/2 mm : 27 ; > √3 mm : 16 ; dont objets/parties : 30, marge > √3 mm : 5 ; marge relative médiane 0.0010
naissances manquantes (somme) : 0 ; naissances dans plusieurs composantes : 0
```

Emboîtements : A_v(naissance) ⊆ A_v(milieu) ⊆ A_v(fin) en faces et amas discret croissant (100 %) ; A_v(d_v^−) ⊆
composante du parent à d_v (100 %). Ce sont des conséquences de la construction (sous-complexes {a_σ ≤ r²}, attribution
par naissances) : elles testent l'implémentation, pas un théorème. La caractéristique d'Euler χ des composantes
objet varie de −962 à 6 (panneaux : χ très négatif, la région dense y est une feuille percée de centaines de petits
trous à ce rayon ; vélo : χ de −13 à 1) ; aucune
homologie certifiée (β non calculés) : le certificat topologique reste celui de l'auditeur [P] (lemme du nerf sur la
subdivision complète), appliqué ici à une mosaïque contrôlée strictement.

**(R1)/(R5) Faces exposées, strates isolées, ombre, ombres qui se touchent** [M] (P5 : part des nœuds dont l'amas
discret recoupe celui d'un AUTRE nœud vivant au même niveau ; P6 : faces exposées par point couvert du nœud objet
à la fin de vie ; P7 : part des strates isolées parmi les éléments rendus ; P8 : pièces de l'ombre / 3-cellules) :

| K | P5 : part des nœuds dont l amas recoupe un autre nœud vivant (fin) | P6 : faces exposées / point couvert, objets (fin), médiane | P7 strates (naissance, nœuds de naissance) | P7 strates (milieu, parties) | P7 strates (fin, objets) | P8 pièces ombre / 3-cellules (fin) | χ objets (fin) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 0.000 (110) | 1.8 | None | 0.7875 | 0.1988 | 2.8293 | [-591, -447, -12, -5, 0, 1] |
| 2 | 1.000 (188) | 5.5875 | None | 0.4021 | 0.0977 | 1.5271 | [-105, -80, -46, -12, -4, -2, 0, 1, 3] |
| 3 | 1.000 (224) | 10.4425 | None | 0.2143 | 0.0542 | 1.1742 | [-962, -762, -78, -11, -4, 0, 1, 5] |
| 5 | 1.000 (237) | 18.788 | None | 0.1094 | 0.0379 | 1.0608 | [-935, -864, -19, -13, -3, -2, 0, 1, 6] |

À K = 1 aucun amas ne recoupe un autre (deux composantes de Ω_1(r) sont à plus de 2r : prouvé, retrouvé) ; à K ≥ 2,
TOUS les nœuds mesurés ont, à la fin de vie, un amas qui recoupe celui d'au moins un autre nœud vivant, sans que la
tour les fusionne (0 écart π0) : « des ombres qui se touchent ne fusionnent pas les nœuds » est la règle, pas
l'exception, et l'ombre ne peut donc jamais servir d'identité.

**(R3) Géométrie** [M] — Hausdorff rapporté à r (échantillonné : A → C borné par la distance à l'ε-net E_v ;
C → A sur les points de E_v, distance exacte au solide ; ombre en voxels ; non certifié) :

| K | d_H(A,C)/r médiane | d_H(ombre,C)/r médiane | d_H(supports,C)/r médiane | n |
| --- | --- | --- | --- | --- |
| 1 | 0.9998 | 1.0017 | 0.9999 | 58 |
| 2 | 0.952 | 0.9126 | 0.9585 | 58 |
| 3 | 0.9035 | 0.8786 | 0.9269 | 60 |
| 5 | 0.8278 | 0.8534 | 0.8627 | 60 |

d_H(A, C) est dominé par C → A : la région dense déborde des barycentres d'environ r ; il vaut ≈ r à K = 1 et
descend à ≈ 0,83 r à K = 5 (borne prouvée : ≤ r [P]). L'ombre n'est pas plus proche de C que le dual ; l'offset est
à distance r de C par définition.

Fidélité aux retours de la cible (δ = 30 mm, `H.fidelite` pour dual et supports ; voxels pour ombre et offset,
« précision » = part de la frontière voxel à ≤ δ + h√3/2 des retours) :

| K | rendu | couverture (retours cible à <= 30 mm) | précision |
| --- | --- | --- | --- |
| 1 | dual | 0.9896 | 0.9058 |
| 1 | ombre | 0.9896 | 0.9946 |
| 1 | offset | 0.9924 | 0.0016 |
| 1 | supports | 0.9896 | 0.978 |
| 2 | dual | 0.9828 | 0.8405 |
| 2 | ombre | 0.9924 | 0.9095 |
| 2 | offset | 1.0 | 0.0195 |
| 2 | supports | 0.9924 | 0.8918 |
| 3 | dual | 0.9212 | 0.7811 |
| 3 | ombre | 1.0 | 0.8081 |
| 3 | offset | 1.0 | 0.0047 |
| 3 | supports | 0.9962 | 0.8106 |
| 5 | dual | 0.8702 | 0.647 |
| 5 | ombre | 1.0 | 0.6751 |
| 5 | offset | 1.0 | 0.0067 |
| 5 | supports | 0.9972 | 0.6586 |

Ouvertures des roues (part du disque d'ouverture à ≤ 10 mm du rendu ; conservée si < 5 % ; pour un nœud partie
« roue », seule SON ouverture compte ; ombre et offset en voxels, ± h/2) :

| K | rendu | objets : ouvertures conservées (fin) | parties roue : conservées (milieu) | parties roue : conservées (fin) |
| --- | --- | --- | --- | --- |
| 1 | dual | 2 / 8 | 5 / 7 | 5 / 7 |
| 1 | ombre | 2 / 8 | 5 / 7 | 6 / 7 |
| 1 | offset | 0 / 8 | 4 / 7 | 4 / 7 |
| 1 | supports | 7 / 8 | 7 / 7 | 7 / 7 |
| 2 | dual | 2 / 8 | 7 / 7 | 7 / 7 |
| 2 | ombre | 1 / 8 | 6 / 7 | 6 / 7 |
| 2 | offset | 1 / 8 | 3 / 7 | 3 / 7 |
| 2 | supports | 2 / 8 | 6 / 7 | 6 / 7 |
| 3 | dual | 2 / 8 | 8 / 8 | 8 / 8 |
| 3 | ombre | 1 / 8 | 5 / 8 | 6 / 8 |
| 3 | offset | 1 / 8 | 4 / 8 | 4 / 8 |
| 3 | supports | 2 / 8 | 6 / 8 | 6 / 8 |
| 5 | dual | 2 / 8 | 6 / 7 | 6 / 7 |
| 5 | ombre | 1 / 8 | 3 / 7 | 3 / 7 |
| 5 | offset | 0 / 8 | 1 / 7 | 1 / 7 |
| 5 | supports | 2 / 8 | 4 / 7 | 4 / 7 |

Au niveau objet (vélos), aucun rendu ne garde les roues ouvertes (moyeu et rayons du générateur remplissent le
disque dès r ≈ 140–170 mm) sauf les deux anneaux percés ; au niveau des nœuds « roue », le dual garde l'ouverture
dans 26 cas sur 29 (tous K, fin de vie), l'ombre 21, les supports 23, l'offset 12.


## 5. Les niveaux d'un nœud, la chaîne roue → vélo et les ordres

**Vie des nœuds** [M] : seuls 95 des 754 nœuds non racines mesurés vivent plus de 5 % au-delà de leur naissance
(d_v / b_v > 1,05). Marges de la coupe intérieure (minimum des deux côtés) :

| K | nœuds non racines | marge > √3/2 mm | marge > √3 mm | marge relative médiane |
| --- | --- | --- | --- | --- |
| 1 | 105 | 37 | 28 | 1,4 % |
| 2 | 188 | 45 | 26 | 0,42 % |
| 3 | 224 | 35 | 24 | 0,18 % |
| 5 | 237 | 27 | 16 | 0,10 % |

Exemple : le nœud vélo K = 5 de synth_velo_10m vit de 167,449 à 167,547 mm (marges de 0,049 mm, sous l'arrondi au
millimètre) ; ses trois vignettes sont identiques ([planche](png/synth_velo_10m_k5_noeud1377_niveaux.png)). Une
« coupe intérieure avec marges > δ » n'existe que pour une minorité de nœuds ; pour les autres, la famille
A_{K,v}(r) se réduit à un état et la vie du nœud est plus courte que le bruit de quantification. Exemple de vie
longue : la roue de synth_anneau_perce_10m à K = 2 (55,4 → 141,5 mm) : le dual passe d'un « C » fin à un « C » épais,
l'offset d'un « C » à un disque ([planche](png/synth_anneau_perce_10m_k2_noeud120_niveaux.png)).

**La forme évolue le long de la chaîne des nœuds** [J] : planches « chaîne » (dual et ombre de la partie à l'objet,
chacun à sa coupe intérieure) : [vélo 5 m, K = 5](png/synth_velo_05m_k5_chaine_vélo_roue_avant.png) (roue avant en
anneau à 46,6 mm ; vélo à deux anneaux, cadre, selle et guidon à 77–89 mm ; roues qui se remplissent à 111 mm ;
silhouette à disques pleins au nœud vélo à 155 mm), [vélo 5 m, K = 2](png/synth_velo_05m_k2_chaine_vélo_roue_avant.png),
[piéton 5 m, K = 5](png/synth_pieton_05m_k5_chaine_piéton_jambe_gauche.png) (jambe à 45 mm, corps entier à 88 mm),
[vélo 10 m, K = 5](png/synth_velo_10m_k5_chaine_vélo_roue_avant.png).

**Entre ordres (K = 5 → 2)** (`liens.py`) : pour chaque nœud K = 5 à sa coupe intérieure r, témoins EXACTS de sa
composante (centre de la sphère propre d'une face active : sommet de Voronoï d'une 3-cellule, ou face de dimension
inférieure si la composante n'a pas de volume ; admissibilité et niveau ≤ r² vérifiés en entiers) ; ensemble STRICT
des 2 plus proches voisins du témoin (comparaisons entières) → sommet de la mosaïque d'ordre 2 → composante de
A_2(r) → nœud vivant de l'arbre K = 2 (par ses sommets de naissance). C'est l'inclusion Ω_5(r) ⊆ Ω_2(r) évaluée sur
des points de Ω_5(r) : aucune projection, aucun plus proche barycentre. Contrôles : tous les témoins d'une composante
tombent dans le même nœud K = 2 (« cohérent ») ; le long d'une chaîne enfant → parent (rayons r_e < r_p),
l'ancêtre vivant à r_p de l'image de l'enfant est l'image du parent (« commute »).

| jeu | nœuds K = 5 liés | cohérents (tous les témoins dans un seul nœud K = 2) | paires de chaîne (par partie, répétitions comprises) | commutent |
| --- | --- | --- | --- | --- |
| synth_anneau_perce_05m | 1 | 1 | 0 | 0 |
| synth_anneau_perce_10m | 11 | 11 | 10 | 10 |
| synth_pieton_05m | 69 | 69 | 212 | 212 |
| synth_velo_05m | 59 | 59 | 236 | 236 |
| synth_velo_10m | 46 | 46 | 138 | 138 |
| synth_velo_occulte_10m | 47 | 47 | 99 | 99 |
| total | 233 | 233 | 695 | 695 |

Planches K = 5 (haut) et image K = 2 au même rayon (bas), même caméra : [vélo 5 m](png/synth_velo_05m_liens_k5_k2.png),
[vélo 10 m](png/synth_velo_10m_liens_k5_k2.png), [piéton 5 m](png/synth_pieton_05m_liens_k5_k2.png). Ce lien vient
des mosaïques, pas des verticales de la tour FULL (la sortie supports n'en publie pas) : un lien qualifié contre
FULL reste à faire. Les solides K = 5 et K = 2 ne sont pas inclus l'un dans l'autre (seuls les π0 le sont) : la
planche montre deux représentants, un par ordre, comme le demande l'auditeur.


## 6. Robustesse (R4)

Trois sens de « robuste » (auditeur, 28d70f8ab) ; ici, épreuves (a) déplacement apparié et (b) modification de
population (`robustesse.py`, `resultats/robustesse.json`). (a) Chaque coordonnée déplacée de −1, 0 ou +1 mm (graine
1 ; δ_max = √3 mm ; aucun site confondu, donc bijection conservée), deux mosaïques complètes contrôlées strictement,
diagrammes H0 de la filtration A_K (règle de l'aîné, en rayon), distance d'étranglement (gudhi) ; nombres de
composantes et de faces exposées de TOUT le niveau aux milieux des intervalles entre événements H0. (b) Un point
ajouté 10 m au-dessus du nuage.

| jeu | K | δ_max (mm) | d_B(H0) (mm) | d_B ≤ δ | rayons comparés | à marge > 2δ : composantes égales | écart relatif max des faces exposées (tout le niveau) | rayons à écart > 10 % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| synth_anneau_perce_10m | 1 | 1.732 | 1.167 | True | 45 | 3 / 3 | 0.114 | 2 |
| synth_anneau_perce_10m | 2 | 1.732 | 1.224 | True | 110 | 4 / 4 | 4.000 | 17 |
| synth_anneau_perce_10m | 5 | 1.732 | 1.008 | True | 114 | 6 / 6 | 0.375 | 18 |
| synth_velo_10m | 1 | 1.732 | 1.142 | True | 145 | 3 / 3 | 0.060 | 0 |
| synth_velo_10m | 2 | 1.732 | 1.413 | True | 543 | 3 / 3 | 3.000 | 63 |
| synth_velo_10m | 5 | 1.732 | 1.175 | True | 1390 | 2 / 2 | 4.000 | 47 |

| aberrant (anneau 10 m) | K | distance (mm) | coupes : r (mm), A_K(r) identique (faces, labels, niveaux) |
| --- | --- | --- | --- |
| point ajouté 10 m au-dessus | 2 | 10005 | 500 : True (2687 faces); 1000 : True (3721 faces); 2001 : True (4025 faces); 4002 : True (4189 faces) |
| point ajouté 10 m au-dessus | 5 | 10005 | 500 : True (11723 faces); 1000 : True (19235 faces); 2001 : True (21911 faces); 4002 : True (23623 faces) |

Lecture [M] : **la filtration est stable, le dessin ne l'est pas.** d_B(H0) ≤ δ_max dans 6 cas sur 6 (1,0 à 1,4 mm
pour δ_max = 1,73 mm), conforme à l'entrelacement prouvé [P] ; aux rayons dont la marge aux événements dépasse 2δ,
les nombres de composantes sont égaux (21 sur 21). Mais à rayon fixé, le nombre de faces exposées change de plus de
10 % sur 0 à 4 % des rayons à K = 1 et sur 3 à 16 % des rayons à K = 2 et 5, jusqu'à un facteur 5 : un déplacement d'un millimètre peut faire
apparaître ou disparaître des cellules entières sans changer le type d'homotopie (le triangle de l'auditeur, à
l'échelle d'une scène). Le point aberrant lointain laisse A_K(r) identique (faces, labels, niveaux) jusqu'à
r = 4 m à K = 2 et 5 (énoncé de l'auditeur : un groupe de moins de K points à plus de 2r ne change rien).
Non fait : fusion de sites par la quantification (modèle pondéré), ajouts proches (décalage de K), sous-échantillonnage
avec contrat de masse.


## 7. Taille et coût (R5)

Tout est mesuré sur le codespace partagé (2 fils au plus, `nice -n 10`), prototype Python, **petits nuages
seulement : non mesuré à l'échelle** (n = 8 000 / 16 000 / 32 000 non atteints par cette expérience).

| | K = 1 | K = 2 | K = 3 | K = 5 |
| --- | --- | --- | --- | --- |
| faces de la mosaïque complète par point (8 jeux) | 19–27 | 86–138 | 198–353 | 505–1 086 |
| 3-cellules par point (jeu le plus gros, 1 139 points) | 6,1 | 23 | 57 | 177 |
| faces exposées par point couvert, nœud objet, fin de vie (médiane) | 1,8 | 5,6 | 10,4 | 18,8 |
| part des faces de la mosaïque gardées par le rendu exposé (objet) | ≈ 7 % | ≈ 4–5 % | ≈ 3 % | ≈ 2 % |
| mosaïque complète, 1 139 points (s) | 0,9 | 6,5 | 14 | 63 |

La mosaïque d'ordre 5 pèse environ 1 000 faces par point ; le rendu exposé d'un nœud objet en garde ≈ 20 par point,
la scène entière à un niveau (toutes composantes) du même ordre (vélo à 10 m : 4 484 faces exposées pour 203 points
à K = 5 et r = 162 mm, 1 079 à K = 2 et r = 90 mm ; `resultats/robustesse.json`). Extrapolation NON mesurée : une trame de 40 000 sites
donnerait ≈ 4·10^7 faces de mosaïque à K = 5 (hors de portée du prototype Python et de l'invariant d'architecture
pour le calcul de la hiérarchie ; permis seulement en aval, borné) et ≈ 10^6 faces exposées par niveau. Le rendu
exposé est donc un affichage d'oracle, pas encore un jeton compact : la réduction certifiée (sommets protégés,
effondrements sur toute la plage) est la suite nécessaire, et l'ombre n'est pas plus petite (pièces / 3-cellules
≥ 1,06).


## 8. Reconnaissabilité, candidat par candidat [J]

Critères fixés avant de regarder les planches : **roue** = boucle fermée avec ouverture visible ; **vélo** = deux roues
distinctes + cadre (ouverture des roues non exigée : le générateur met moyeu et rayons) ; **piéton** = deux jambes,
tronc, tête ; **nappes** = deux panneaux séparés ; **vélo occulté** = pas de surface inventée sur la bande cachée ;
**réel** = un vélo identifiable. Jugement de l'expérimentateur sur les planches à caméra fixe ; « coupe » = coupe
intérieure du nœud ; ✓ reconnaissable, ~ partiel, ✗ non.

| Cible (nœud) | dual A_K exposé | ombre S_v | offset C_v ⊕ B_r | supports S* |
| --- | --- | --- | --- | --- |
| roue, anneau percé (objet), K = 1…5 | ✓ anneau à tous K (ouverture gardée) | ✓ anneau épais (5 m) ; ~ « C » épais (10 m, K = 2 fin) | ✓ anneau épais (5 m, K = 5) ; ✗ disque (10 m, K = 2 fin) | ✓ anneau |
| roue d'un vélo (nœud partie), fin de vie | ✓ anneau (26/29 ouvertures gardées) | ~ anneau qui se remplit (21/29) | ✗ le plus souvent (12/29) | ✓ à K ≤ 2, ~ à K = 5 (23/29) |
| vélo, ancêtres intermédiaires (K = 5 : 77–89 mm ; K = 1 : 28–47 mm ; vélo 5 m) | ✓ **le meilleur rendu** : deux anneaux, cadre, selle, guidon | ✓ même dessin, plus épais | non rendu | — |
| vélo, nœud objet de l'oracle (5 m et 10 m) | ~ silhouette (deux disques pleins + cadre) à K = 2–5 ; ✗ à K = 1 10 m (né à 151 mm) | ~ silhouette empâtée | ✗ bloc | ~ à K = 1 (arbre), ~ K = 2, ✗ K = 5 |
| piéton, chaîne jambe → piéton (K = 2 et 5) | ✓ jambe, puis corps entier | ✓ | ✗ bloc | ~ |
| deux nappes (K = 1…5) | ✓ deux panneaux distincts (strates de dimension 2 à K ≤ 2) | ✓ deux rectangles | ✓ deux dalles (qui se recouvrent à l'écran, nœuds distincts) | ✓ |
| vélo occulté (K = 5, chaîne roue avant → vélo) | ~ deux morceaux séparés jusqu'à 163 mm ; au nœud vélo (196 mm) un pont mince de cellules franchit la bande cachée (juste pour la région dense, inventé pour la surface observée) | ✗ au nœud vélo : bloc qui recouvre la bande cachée | ✗ bloc | ~ |
| découpe réelle 08/002852 (deux vélos) | ~ squelette à K = 1 ; ✗ bloc à K ≥ 2 | ~ K = 1 ; ✗ K ≥ 2 | ✗ | ~ K = 1 ; ✗ K ≥ 2 |

Classement observé au niveau objet : **dual A_K exposé ≳ supports S* (K ≤ 2) > ombre > supports (K = 5) > offset**,
mais le facteur dominant n'est pas le rendu : c'est **le nœud et son rayon**. Un même rendu passe de « anneaux +
cadre » à « bloc » le long de la chaîne roue → vélo, parce que le nœud qui contient tout l'objet naît à l'échelle de
la plus grande lacune de l'objet (à K = 1 : 151 mm pour le vélo à 10 m, 98 mm pour le vélo à 5 m ; à K = 5 :
151 mm pour le vélo à 5 m), où les roues sont
déjà remplies. La hiérarchie de polyèdres reconnaissable est donc : roue (anneau) au nœud partie, vélo (anneaux +
cadre) aux ancêtres intermédiaires, silhouette pleine au nœud objet. Planches comparatives à caméra fixe :
`png/candidats_<jeu>_objet<o>.png`.


## 9. Prédictions : bilan

| # | Prédiction gelée (résumé) | Mesure | Verdict |
| --- | --- | --- | --- |
| P1 | K = 1 : niveaux = gudhi sur tous les simplexes, même nombre, 7/7 | identique sur 5 jeux sur 8 ; sur 3 jeux, différences limitées aux cellules cosphériques (polytopes contre triangulation), 0 écart de niveau | réfutée telle qu'écrite (comptes), tenue sur les niveaux |
| P2 | contrôle strict : ≥ 30 mosaïques sur 32 | 32 sur 32 après correction de MON contrôle (dépassement int64) | tenue |
| P3 | 0 écart π0 | 0 écart, 32 sur 32 | tenue |
| P4 | trace de l'ombre = labels, 100 % | 100 % des couples (nœud, niveau) | tenue |
| P5 | K = 1 : 0 % ; K ≥ 2 : ≥ 30 % des nœuds recoupent un autre nœud vivant | 0 % ; 100 % | tenue (plus fort que prévu) |
| P6 | faces exposées / point couvert : 2–6, 8–40, 15–80, 30–200 (K = 1, 2, 3, 5) ; < 20 % de la mosaïque | 1,8 ; 5,6 ; 10,4 ; 18,8 ; 2 à 7 % | réfutée (surestimé d'un facteur 1,5 à 2) ; borne de 20 % tenue |
| P7 | strates : 100 % à la naissance d'un nœud de naissance ; ≥ 10 % (parties, K = 5, coupe) ; < 30 % (objets, fin) | non mesuré ; 10,9 % ; 3,8 % | tenue sur ce qui est mesuré |
| P8 | pièces de l'ombre ≥ 0,8 × 3-cellules | 1,06 à 2,83 | tenue |
| P9 | ouvertures : K = 1 dual ≥ 50 % ; K = 5 dual ≤ 30 %, ombre ≤ 20 %, offset 0 % | objets : 2/8, 2/8, 1/8, 0/8 ; nœuds roue : 5/7, 6/7, 3/7, 1/7 | tenue au niveau objet ; réfutée au niveau des nœuds roue (le dual garde l'ouverture) |
| P10 | d_H(A, C)/r dans [0,2 ; 1] ; médiane 0,4–0,8 à K = 5 ; ombre plus loin que le dual | dans [0,6 ; 1,0] ; 0,83 ; ombre plus loin à K = 5 seulement | réfutée de peu |
| P11 | emboîtements 100 % ; vie > 1,05 pour au moins la moitié des nœuds | 100 % ; 95 sur 754 (13 %) | réfutée (vies courtes) |
| P12 | classement : dual K = 1 > dual K = 2 > supports ≈ ombre (K = 5) > dual K = 5 > offset ; roue fermée à K = 5 ; piéton en bloc à K = 5 | dual K = 5 meilleur que l'ombre et les supports K = 5 ; roue ouverte à son nœud à tous K ; piéton lisible à K = 5 | réfutée en partie (le dual d'ordre élevé est meilleur que prévu) |
| P13 | roue ⊆ vélo (faces) ; liens K = 5 → 2 cohérents et commutatifs, 100 % | 100 % ; 233 nœuds K = 5 liés, 233 cohérents ; 695 paires de chaîne, 695 commutent | tenue |
| P14 | composantes égales à marge > 2δ ; faces exposées : écart > 10 % à au moins un rayon ; aberrant lointain sans effet | 21/21 ; oui (jusqu'à un facteur 5) ; identique jusqu'à 4 m | tenue |
| P15 | mosaïque K = 5 de 1 139 points < 90 s ; rendu d'un nœud < 5 s ; rien à l'échelle | 63 s ; 1 à 4 s par nœud, mesures comprises ; non mesuré à l'échelle | tenue |


## 10. Échecs, défauts, limites

- **Échelle** : petits nuages (52 à 1 139 points) ; aucune conclusion de coût, de taille ou de mémoire n'est tirée
  pour n = 8 000 / 16 000 / 32 000.
- **Défaut de mon contrôle strict** (dépassement int64 dans le volume de l'enveloppe), trouvé sur synth_velo_occulte_10m
  K = 5, corrigé et rejoué ; les contrôles passés avant la correction (vélo 5 m K = 5, vélo occulté K = 1–3, piéton
  K = 1) l'ont été avec l'ancienne somme, sans dépassement (égalité exacte obtenue).
- **Ombre et offset dessinés en voxels** : sur-ensemble déclaré (demi-espaces décalés de h√3/2) pour l'ombre ;
  ε-net de grille non certifié pour C_v(r) (composantes minces ou de mesure nulle manquées) et offset sous-estimé
  d'au plus ε ; leurs mesures d'ouverture et de fidélité ont une erreur de ± h/2.
- **Hausdorff échantillonné** (pas une borne certifiée) ; aucune homologie calculée au-delà de χ.
- **Niveaux comparés en flottants correctement arrondis** des mêmes rationnels (tour et mosaïque) ; deux rationnels
  distincts arrondis au même flottant seraient confondus (risque déclaré, non observé : 0 écart π0).
- **Liens entre ordres** obtenus par les mosaïques (témoins exacts), pas lus dans les verticales de la tour FULL.
- **Robustesse** : déplacement apparié de ±1 mm et un aberrant lointain seulement ; ni fusion de sites par la
  quantification, ni sous-échantillonnage avec contrat de masse (non faits).
- **Reconnaissabilité** : jugement visuel de l'expérimentateur, sans protocole en aveugle ; vue « de face » = plan
  principal des points des objets (vue de profil pour les vélos et le piéton) ; la découpe réelle n'a servi qu'à
  l'image, aucun réglage n'a été choisi dessus.
- **Sélection oracle** (vérité terrain) : borne de ce que contient l'arbre, pas une méthode. Aucun nœud de naissance
  (genre 1) n'est dans la sélection : la prédiction P7 « 100 % de strates à la naissance d'un nœud de naissance » n'est
  pas mesurée (elle découle de la construction : un seul sommet).
- **Caméra** : `H.rendre_png` recadre selon le polyèdre ; le rendu utilise sa projection (`H._base_vue`) avec une
  caméra fixe par jeu. Les planches de levels du vélo occulté ont été refaites après le passage au cadrage sur les
  objets (le poteau rendait la vue de face rasante).


## 11. Fichiers

Dossier `build/v11-persist/polyedres_ordre_k/experience_rendu/` (≈ 20 Mo) :

- `PREDICTIONS.md`, `PREDICTIONS.sha256` (gelées à 22 h 19), `README.md` (ce rapport).
- Code : `rendu.py` (contrôle strict, étiquetage des composantes, attribution par naissances, ombre, ε-net et offset,
  caméra fixe, panneaux), `campagne.py` (jeu × K : mosaïque, contrôles, nœuds, niveaux, mesures, planches),
  `liens.py` (K = 5 → 2), `robustesse.py`, `fixtures.py` et `oracle_bas.py` (contre-épreuves exactes 1D/2D/3D),
  `six_points.py`, `verif_alpha.py`, `synthese.py`, `montage.py`, `lignes_sortie.py`, `ecrire_readme.py`,
  `lancer*.sh` ; `blocs/` (sections du rapport).
- Résultats : `resultats/mesures.jsonl` (une ligne par exécution ; la dernière par (jeu, K) fait foi),
  `synthese.json`, `fixtures.json`, `six_points.json`, `liens_<jeu>.json`, `robustesse.json`, `verif_alpha.json`,
  `lignes_sortie.json`, `journal.txt`.
- Images (`png/`) : `<jeu>_k<K>_noeud<v>_niveaux.png` (lignes : dual exposé, ombre, offset, supports ; colonnes :
  naissance, coupe intérieure, fin de vie ; même caméra pour tous K d'un jeu), `<jeu>_k<K>_chaine_<partie>.png`
  (chaîne partie → objet, dual et ombre), `<jeu>_liens_k5_k2.png`, `six_points_k{1,2,3}.png`,
  `candidats_<jeu>_objet<o>.png` (planches comparatives des candidats).

