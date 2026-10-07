# Oracle exact borné du complexe alpha d'ordre k, A_k(r)

6 octobre 2026 (travail entre 22 h 14 et 23 h 29 UTC, heures lues par `date -u`). Rôle : constructeur de l'oracle
exact borné de A_k(r), selon les § 1 et 7 de la note de l'auditeur
(`morsehgp3D_v11/receipts/audit_hartigan_delaunay_20261006/README.md`, commit d2be6bdc7) et sa suite
(`audit_hartigan_robustesse_20261006`, commit 28d70f8ab).

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (oracle Python, Fractions et entiers)
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Oracle de CORRECTION sur petits nuages entiers (n <= 60 déclaré, ordres k <= 8) : il établit la vérité de A_k(r) sur
de petits cas et sert de juge ; il ne mesure ni coût ni échelle (aucune conclusion de coût n'est tirée ici : non
mesuré à l'échelle). Il n'entre pas dans le calcul de la hiérarchie : la mosaïque d'ordre supérieur n'est construite
qu'ici, en aval, bornée (invariant d'architecture respecté).

## 0. En bref

- **Objet** : A_k(r), complexe alpha d'ordre k sur la subdivision régulière duale aux domaines de Voronoï d'ordre k
  pleins, filtré par a_sigma = min sur F_sigma de d_k^2 (note de l'auditeur, § 1), avec labels, faces duales,
  incidences, niveaux certifiés (témoin + multiplicateurs KKT exacts), plateaux, composantes, attribution par
  incidences, couverture par les labels et offset exact par pièces convexes.
- **Garanties de l'oracle** : exactitude arithmétique (Fractions, entiers), aucune position générale supposée, aucune
  simulation de simplicité, contrôle strict de chaque mosaïque (sinon refus explicite).
- **Validation** : FULL (mhgp11) sans désaccord, nœud par nœud, sur 91 ordres ; prototype identique sur 138 473 faces
  en rang 3 ; `oracle_mosaique.py` et gudhi (k = 1) conformes ; Gamma_K, offset, réduction, robustesse pi0 conformes ;
  14 fixtures et 4 capsules de l'auditeur rejouées en `python3` et `python3 -O`.
- **Prototype** : fiable seulement en rang 3, à étendue modérée, et après contrôle strict ; trou de 08/000100 (k = 2)
  localisé à 2 cellules et reproduit sur 13 sites (§ 8 bis) ; échec explicite sur nuages plans ou alignés.

## 1. Fichiers

| Fichier | Rôle |
| --- | --- |
| `oracle_ak.py` | le module (Python 3 standard ; numpy et scipy facultatifs, seulement comme accélérateurs vérifiés) |
| `fixtures_oracle.py` | 14 fixtures exactes (contre-épreuves de l'auditeur, défaut corrigé de l'oracle, trou minimal du prototype), 66 contrôles, sans `assert` |
| `epreuves_oracle.py` | couverture contre offset exact, Gamma_K, verticales, juge des effondrements et mutants |
| `comparaisons.py` | gudhi (k = 1), `oracle_mosaique.py`, prototype `mosaique.py`, tour FULL `mhgp11 --sortie=supports` |
| `comparaisons_lidar.py` | extraits de 40 sites de la trame 08/000100 (grille de 1 mm) : oracle, prototype, FULL |
| `controle_prototype.py`, `trou_minimal.py` | contrôle strict d'une mosaïque du prototype (trame entière à k = 2) ; oracle exact local du trou ; reproduction minimale |
| `epreuves_robustesse.py` | entrelacements pi0 exacts : positions appariées, ajouts, suppressions |
| `mesure_temps.py`, `synthese.py` | temps de l'oracle ; agrégation de `resultats/` en `resultats/synthese.json` |
| `rejeu_auditeur/` | les quatre capsules de preuves de l'auditeur extraites par `git show`, rejouées |
| `resultats/` | sorties JSON (résumés et détails) ; `avant_correctif/` : première passe, avant le correctif du § 8 |

## 2. Utilisation (juge pour les autres agents)

```python
import sys
sys.path.insert(0, '/workspaces/E-HGP/build/v11-persist/polyedres_ordre_k/oracle')
import oracle_ak as O
from fractions import Fraction as Fr

N = O.Nuage(points, kmax)              # points : liste de triplets entiers distincts ; n <= 60, kmax <= 8
M = N.ordre(k)                         # mosaïque d'ordre k : construite, contrôlée strictement, niveaux certifiés
M.faces                                # dict par face : id, dim, I, U (clé), fam (étiquettes), som (sommets), a, z, w
M.cle[(I, U)]                          # face de clé (I', U') ; sommet Q : M.cle[(Q, ())]
M.etiq_list[e], M.pleine[e]            # étiquette e (k-partie triée) ; domaine plein (sommet de la mosaïque) ou non
M.bord[fid], M.cofaces[fid]            # facettes ; cofaces (toutes dimensions)
M.niveaux                              # niveaux distincts (Fractions, r^2) ; plateaux = faces de même niveau
comp, etiq_comp, face_comp = M.composantes(a)      # A_k(r) à la coupe fermée a = r^2 (strict=True : a_sigma < a)
M.couverture(a)                        # P inter (C_v + B_r) par composante (réunion des étiquettes pleines)
M.evenements()                         # naissances et fusions (plateaux d'un bloc, coupe fermée)
M.vertical(a, N.ordre(k - 1))          # pi0(A_k(r)) -> pi0(A_{k-1}(r)), bien défini (vérifié)
M.betti_mod2(a), M.euler_coupe(a)      # homologie mod 2 du complexe cellulaire A_k(r)
M.strates_maximales(a), M.faces_exposees(a)        # strates isolées de dimension 0, 1, 2 ; faces exposées du solide
M.dans_offset(x, e, a)                 # x dans C_Q(r) + B_r (pièce convexe de l'offset), exact
M.certifier(face, z, a)                # juge un témoin proposé : z dans F_sigma, d_k(z)^2 = a, multiplicateurs KKT
O.verifier_effondrements(M, paires)    # juge d'un journal d'effondrements (plage de filtration entière, sommets protégés)
O.juger_sous_complexe(M, a, L)         # juge d'un représentant proposé L à la coupe a : fermeture, sommets, pi0 par nœud, Betti, D_v
O.diametres2_composantes(M, a)         # D_v(r)^2 par composante (borne de Hausdorff L <-> A à sommets protégés)
O.gamma_k(P, K, a)                     # composantes et K-polyèdres de Gamma_K (thèse, Déf. 21), exhaustif borné
M.plateaux()                           # niveau -> faces de ce niveau
M.niveaux_dtm()                        # ATTRIBUT b_sigma = min_{F_sigma} f_k^2 (DTM), b <= a <= k b vérifié
O.application_temoins(M1, a, M2, d2)   # pi0 A^{1}(r) -> pi0 A^{2}(r + delta), delta^2 = d2 (entrelacements exacts)
O.coupe_sous(M, a, d2), O.racine_le(x, a, d2)      # plus grand niveau <= (sqrt(a) + sqrt(d2))^2, décidé exactement
M.face_duale(face)                     # F_sigma en contraintes linéaires entières (égalités, inégalités)
```

Ligne de commande : `python oracle_ak.py --points '[[0,0,0],[1,0,0],[2,0,0],[11,0,0]]' --k 3 --coupe 441/16 [--faces]`
(résumé JSON exact). `ORACLE_AK_PUR=1` désactive numpy et qhull (Python pur, mêmes sorties).

Toute violation d'un contrôle lève `O.ErreurOracle` : aucune mosaïque partielle n'est rendue comme complète.

## 3. L'objet calculé

Sites P entiers distincts, poids unitaires ; d_k(y) = distance au k-ième plus proche site ; boules et coupes fermées ;
niveau stocké a = r^2. Pour une k-partie Q, V_Q est son domaine de Voronoï d'ordre k ; seuls les Q de domaine plein
(dimension de aff(P)) sont des sommets. La mosaïque est la subdivision régulière duale (barycentres c_Q = somme(Q)/k,
relevés par la puissance des barycentres pondérés, BCY Th. 4.8).

- **Clé d'une cellule** sigma : sa famille est l'ensemble de TOUTES les k-parties dont le barycentre est sur sigma
  (sommets ou non) ; I' = intersection, U' = réunion moins intersection. Vérifié pour chaque face : famille
  = {I' u J : J dans U', |J| = k - |I'|}, 0 < k - |I'| < |U'|, et dim sigma = dim aff(U') (dualité).
- **Face duale** : F_sigma = {y : |y - u| égaux pour u dans U', I' à distance <= , les autres à distance >= }.
- **Niveau** : a_sigma = min sur F_sigma de d_k^2. Calcul : minimum des niveaux propres admissibles des cofaces
  (centre de la boule minimale de Q pour un sommet, circoncentre de U' sinon, admissible s'il est dans la face duale
  fermée), propagé des cofaces vers les faces. **Certificat indépendant** pour chaque face : témoin z exact dans
  F_sigma, d_k(z)^2 = a_sigma recalculé, et multiplicateurs exacts w (KKT du problème convexe) :
  z = somme w_s s, somme w_s = 1 sur les sites de la sphère S(z, a), w >= 0 sur les sites « dedans » de la clé,
  w <= 0 sur les sites « dehors », libres sur U'. Ce sont les multiplicateurs de Lagrange des contraintes actives
  au signe près (lambda_i = w_i >= 0 pour |y - i| <= rayon, lambda_o = -w_o >= 0 pour |y - o| >= rayon, libres pour
  les égalités de E(U')). Les conditions KKT suffisent (fonction convexe sur un polyèdre).
- **Coupe** : A_k(r) = {sigma : a_sigma <= r^2}, sous-complexe (monotonie vérifiée sur chaque incidence) ;
  composantes par le 1-squelette ; plateaux traités d'un bloc ; événements H0 : naissance, fusion, continuation.
- **Attribution** : toute étiquette (pleine ou non) d'une face active va à la composante de cette face ; l'unicité
  est vérifiée (C_Q(r) est convexe). Jamais par la position d'un barycentre (fixture `zero_un_deux_onze`).
- **Couverture** : P inter (C_v(r) + B_r) = réunion des étiquettes pleines actives de la composante (§ 1.3 de la note).
- **Offset exact par pièces convexes** : C_v(r) + B_r = réunion des C_Q(r) + B_r ; l'appartenance de x à une pièce
  est décidée exactement par min sur V_Q de max sur Q u {x} de |y - s|^2 <= r^2 (V_Q décrit par les arêtes de la
  mosaïque en c_Q ; ensembles actifs énumérés par taille croissante, premier point KKT = minimum global).

## 4. Construction exacte et contrôle strict

1. Rang affine r de P (0 à 3, nuages plans et alignés compris). Les cellules maximales (dimension r) sont duales des
   sphères passant par r + 1 sites affinement indépendants, de centre dans aff(P), avec |In| < k < |In| + |On|.
   **Énumération exhaustive** des (r+1)-parties ; en rang 3 un filtre entier numpy int64 sous borne prouvée
   (coordonnées centrées |x| <= 1000 : toute quantité <= 4896 B^5 < 2^63), chaque sphère gardée re-décidée en
   entiers Python ; dédoublonnage par **(centre, rayon)**.
2. Cellule maximale = conv des barycentres des I u J ; treillis exact de ses faces : plans d'appui exacts (force brute
   jusqu'à 12 positions, sinon propositions qhull vérifiées exactement), fermeture (chaque arête dans exactement deux
   facettes) et Euler ; repli en force brute si une proposition échoue.
3. Contrôle global strict, à chaque ordre : cohérence clé, famille, dimension de chaque face partagée ; appariement
   des facettes (deux cellules maximales, ou une et alors sur le bord de l'enveloppe globale, vérifié exactement) ;
   **enveloppe inférieure des relevés** : à chaque facette intérieure, les sommets de la cellule voisine sont
   strictement au-dessus de l'hyperplan d'appui (puissance en z) et les étiquettes d'une cellule sont coplanaires
   (convexité locale, donc régularité globale) ; caractéristique d'Euler de la mosaïque = 1 ; juge d'échantillon exact
   (200 points rationnels : k plus proches voisins uniques => sommet, sinon clé (In, On) => face) ; recouvrement
   exact (60 points intérieurs : exactement une cellule maximale fermée quand ils sont intérieurs stricts à l'une).

## 5. Fixtures gravées (`fixtures_oracle.py`, 66 contrôles)

Résultat : conforme ; sorties identiques à l'octet en `python3`, `python3 -O` et en Python pur (`ORACLE_AK_PUR=1` :
ni numpy ni qhull), empreinte sha256 `edbc7e56…` (`resultats/fixtures_{normal,O,pur}.txt`).

| Fixture | Contenu exact vérifié |
| --- | --- |
| `zero_un_dix` | P = {0, 1, 10}, k = 3 : seul sommet, niveau 25, témoin (5, 0, 0), multiplicateurs {0 : 1/2, 10 : 1/2} ; d_3(11/3)^2 = 361/9 ; rayon carré DTM 43/9 ; actif exactement à r = 5 |
| `zero_un_deux_onze` | P = {0, 1, 2, 11}, k = 3 : arête 121/4, date DTM 251/12 ; deux composantes à 441/16 ; les k-ppv du barycentre 14/3 de Q = {1, 2, 11} sont T = {0, 1, 2} : il est dans la composante de T (attribution par position FAUSSE) ; couvertures {0,1,2} et {1,2,11} ; offset exact : bords 21/2 (T) et 1/2 (Q) |
| `tetraedre_octaedre` | tétraèdre régulier, k = 2 : six barycentres +-e_i, f = (6, 12, 8, 1), cellule octaèdre de clé (vide, 4 sites), niveau 3 |
| `carre_degenere` | 4 sites cocycliques, k = 2 : une 2-cellule carrée, diagonales {0, 2} et {1, 3} non pleines, attribuées par incidence |
| `grilles` | coins du cube (k = 1..7, une seule cellule à coquille de 8), grille 3x3x3 (k <= 3), grille plane 3x3 (k <= 4) |
| `contact_point` | P = {0, 2, 4}, k = 2 : contact ponctuel au point 2, arête au niveau 4, fusion à 4 exactement (coupe fermée) ; à r = 1 le site 2 est couvert par les deux nœuds |
| `face_libre_future_coface` | k = 1, A, B, C, D de la note : AB et ABC à 25/4, ABD à 100/9 |
| `vie_noeud` | P = {0, 2, 10}, k = 2 : naissances à 1 et 16, fusion à 25 |
| `aberrants` | k = 2, r = 2 : {0, 6, 12} vide ; + 1 : une composante ; {0, 2, 5} : deux ; + 3 : une |
| `triangle_saut` | k = 1, triangle obtus : AB et ABC à 377/16 ; à la même coupe, 99 P dessine le triangle plein, 101 P ses deux petits côtés ; tous deux contractiles |
| `k_egal_n` | k = n = 4, {0, 1, 2, 10} : niveau 25, témoin 5, barycentre 13/4 (écart dual 7/4, cellule de diamètre nul) |
| `gamma_these` | grille 2x2x2 + 2 sites : à chaque niveau critique, Gamma_K et A_k ont les mêmes composantes et K-polyèdres (k = 1, 2, 3) |
| `spheres_concentriques` | défaut de l'oracle trouvé et corrigé (§ 8) : ligne {0, 1, 2, 4, 7, 11, 16} ; deux sphères concentriques en (1, 1, 1) dans la grille 3x3x3 |
| `trou_prototype_minimal` | 13 sites : le quintuplet presque cosphérique de 08/000100 et les coins d'un cube de demi-côté 100 000 mm ; 90 cellules exactes dont les 3 que le prototype perd ; cinq sphères distinctes (§ 8 bis) |

## 6. Rejeu des preuves de l'auditeur

Les quatre capsules (`proofs`, `proofs_ball_union`, `robustesse`, `ombre_et_niveaux`) ont été extraites par
`git show origin/main:…` dans `rejeu_auditeur/` : `sha256sum -c SHA256SUMS` conforme, `python3 -B -I replay.py` et
`python3 -O -B -I replay.py` donnent le code 0 et des sorties identiques à l'octet ; `proofs` (265 contrôles) et
`robustesse` (108 contrôles) redonnent exactement `proof.json` et `proof.optimized.json` ; les deux autres impriment
leur verdict (`critical_ball_geometry_verdict conforme cases4`, `PASS: shadow traces/contact; …`).

## 7. Comparaisons et épreuves (vérifié borné, `resultats/`)

| Référence | Portée | Résultat |
| --- | --- | --- |
| gudhi `AlphaComplex(precision='exact')`, k = 1 | 12 nuages aléatoires, n = 10 à 43 | 12/12 : mêmes simplexes, mêmes niveaux (écart relatif <= 2,2e-16 : arrondi du double de gudhi) ; pour k = 1 l'oracle redonne exactement le complexe alpha |
| `lecture_maths/oracle_mosaique.py` | 8 nuages en position générale, n = 9 à 12, K = 1 à 4 | 32/32 ordres : mêmes sommets, mêmes arêtes, rayons d'Edelsbrunner-Osang égaux exactement à a_sigma |
| prototype `approche_mosaique/mosaique.py` | 13 nuages : 6 aléatoires, 7 dégénérés (coins du cube, grilles 3x3x3 et 2x3x3, octaèdre centré, grille bruitée, plan 4x3, ligne), k <= 4 | 11/13 : 43 ordres, 61 231 faces : 0 manquante, 0 en trop, 0 niveau différent (flottant correctement arrondi = valeur exacte), 0 coupe de composantes en désaccord ; nuage plan et nuage aligné : le prototype échoue (`ValueError`), sans sortie fausse |
| tour FULL `mhgp11 --sortie=supports` (07428324e) | les mêmes 13 nuages, K <= 4 (50 ordres) | 11 288 coupes, 0 désaccord : à chaque coupe (réunion des niveaux des deux côtés), nœuds vivants de FULL = composantes de A_k(r), identifiés par les k-parties de leurs naissances (bijection, pas seulement des comptes) ; chaque naissance FULL est un sommet de même niveau exact ; 1 638 naissances et 799 fusions, comptes égaux à ceux de l'oracle |
| tour FULL, ordres élevés (`comparaisons_full_hautk.py`) | 4 nuages (aléatoires n = 16 et 20, grille 3x3x3, cube + 2 sites), K <= 6 (5 pour n = 20), 23 ordres | 6 894 coupes, 0 désaccord (même critère d'identité) |
| Gamma_K (thèse, Déf. 21, Th. 2) | fixture + 15 couples (nuage, k) | 1 431 coupes (niveaux de A_k ET niveaux propres de Gamma_K : aucun écart transitoire possible) : mêmes composantes, identifiées par les k-parties, et mêmes K-polyèdres que la couverture par les labels |
| offset exact (pièces convexes) | 5 nuages, k <= 3 | 1 609 tests (site, composante) : appartenance exacte à un C_Q(r) + B_r de la composante <=> site dans une étiquette de la composante ; 0 désaccord |
| juge des effondrements | 15 couples (nuage, k), réduction gloutonne de test à sommets protégés | 1 271 paires, 4 063 cellules -> 1 521 ; 1 074 coupes : mêmes composantes et mêmes Betti mod 2 que A_k(r) ; 43/43 mutants refusés ; la paire (AB, ABC) de l'exemple A, B, C, D est refusée (ABD naît plus tard) |
| verticales pi0(A_k) -> pi0(A_{k-1}) | 906 coupes | application bien définie partout |
| DTM (attribut) | fixture + les 15 couples (nuage, k) des épreuves (4 063 faces) | b <= a <= k b vérifié face par face ; arête de {0, 1, 2, 11} : 251/12 |
| LiDAR, trame 08/000100 (grille de 1 mm, sans sol) | 6 extraits : les 40 sites les plus proches d'un site tiré au hasard (étendues 4302, 1601, 2752, 2529, 333, 317 mm), k <= 3 | prototype : 18 ordres, 77 242 faces, 0 écart (faces, clés, niveaux, composantes) ; FULL : 18 ordres, 19 811 coupes, 0 désaccord |

## 8. Défauts trouvés pendant la construction (corrigés, gravés)

1. **Oracle, sphères concentriques** (22 h 40 UTC). La première version dédoublonnait les sphères par leur seul
   centre. Deux sphères de même centre et de rayons différents sont distinctes : sur l'axe, {4, 7} et {0, 11} ont le
   même milieu 11/2 ; la seconde remplaçait la première et l'arête de Delaunay {4, 7} disparaissait. Le contrôle
   d'Euler l'a refusé (caractéristique 2) au lieu de rendre une mosaïque trouée. Correctif : clé (centre, rayon),
   vérifiée ; fixture `spheres_concentriques` (ligne, et deux sphères concentriques en (1, 1, 1) dans la grille
   3x3x3). Défaut d'implémentation de l'oracle, pas une contradiction mathématique : aucun énoncé du registre n'est
   touché. Les résultats antérieurs sont gardés dans `resultats/avant_correctif/`.
2. **Comparaison FULL, naissances dégénérées**. La première passe identifiait une naissance FULL à la seule k-partie
   I_b u U_b ; quand la coquille est plus grande que k - p (coins du cube, grilles, plan), la naissance est une
   cellule entière dont tous les sommets naissent au même niveau. Les « désaccords » de la première passe (cube et
   grilles à K = 3, plan à K = 3 et 4) venaient de cette lecture, pas de FULL ni de l'oracle : avec les k-parties
   I_b u J (J dans U_b), 0 désaccord.

## 8 bis. Le prototype `mosaique.py` / `jeton.py` : fiable à quelles conditions, où il échoue

**Trame entière 08/000100 à k = 2** (`controle_prototype.py`, 35 551 sites, construction du prototype 313 s, un fil ;
contrôle strict 1,7 s) : 2 288 550 2-faces, dont 478 dans une seule 3-cellule ; **8 ne sont pas sur le bord de
l'enveloppe** et la caractéristique d'Euler vaut **3** au lieu de 1, alors que le certificat de volume du prototype
répond « vrai » (déficit 29 220 mm^3 sur 9,6e13 mm^3, soit 3e-10 en relatif, sous sa tolérance de 1e-6).
L'oracle exact sur les 40 sites à moins de 450 mm du centre du trou (6,2 s) donne 255 cellules exactes pour la trame
(boule incluse dans le voisinage) ; **le prototype en manque exactement deux** : I = {11963} et I = {12028}, chacune
avec U = les quatre autres retours du quintuplet presque cosphérique (11768, 11912, 11963, 12017, 12028 ; rapports
des d^2 = 1 + 8e-9). Leurs 8 facettes sont les 8 2-faces libres trouvées par le contrôle strict.

**Reproduction minimale** (`trou_minimal.py`, gravée dans `fixtures_oracle.py`, `f_trou_prototype_minimal`) :
le quintuplet (coordonnées relatives exactes (0,0,0), (69,69,-210), (91,14,-97), (119,67,-100), (125,106,-102)) et
les 8 coins d'un cube de demi-côté L. À L = 1 000 et 10 000 mm : aucune perte. À **L = 100 000 mm** (échelle d'une
trame) : 13 sites, le prototype perd 3 cellules sur 90, son certificat de volume reste « vrai », le contrôle strict
trouve 12 facettes libres hors du bord et Euler 2. La cause est l'échelle des relevés flottants dans qhull face à un
amas presque cosphérique de 25 cm : les propositions dégénérées sont refusées par la vérification exacte et rien ne
re-propose les vraies cellules. Le quintuplet seul, ou son voisinage de 40 sites, ne reproduit pas le trou (comme
l'avait constaté la critique sur des sous-nuages de 12 à 50 points) : c'est l'étendue globale qui le déclenche.

**Verdict.**

- Fiable, sur tout ce qui a été testé, **à trois conditions** : nuage de rang 3 ; étendue modérée (petits nuages,
  extraits LiDAR de 40 sites, voisinages locaux) ; et, dans tous les cas, **contrôle strict passé** (appariement des
  2-faces et Euler = 1, `controle_prototype.controle_strict`, coût linéaire). Dans ces conditions : mêmes faces, mêmes
  clés, mêmes niveaux (flottants correctement arrondis égaux aux valeurs exactes), mêmes composantes que l'oracle,
  coquilles cosphériques de 8 à 12 sites comprises (son chemin exact des cellules dégénérées est correct).
- **Échoue** (a) sans sortie sur les nuages plans ou alignés (`ValueError`) : un voisinage local plan ou aligné
  (`jeton.construction_locale`) n'a pas de mosaïque ; (b) **en silence** à grande étendue, autour d'amas presque
  cosphériques : cellules perdues, trou non vu par le certificat de volume ni par le juge d'échantillon flottant.
  Toute mosaïque du prototype sur une trame ou une grande étendue doit donc passer le contrôle strict ou être
  déclarée partielle ; la correction de fond est de re-proposer la cellule d'une proposition refusée par l'oracle
  local exact (sphères exactes des sites voisins), ce que fait `oracle_ak` sur le voisinage.
- `jeton.bijection` compare des **comptes** de composantes et de nœuds vivants, pas une bijection nœud à nœud ;
  `comparaisons.vs_full` fournit la comparaison par identité (naissances FULL -> k-parties -> composantes).
- `jeton.py` n'a pas été testé séparément : ses composantes et polyèdres reposent sur une mosaïque du prototype
  (fiable sous les conditions ci-dessus) et sur le certificat de localité (2 sqrt(a)), même raisonnement que
  l'oracle local du trou.


## 9. Statuts

- **Prouvé (par l'auditeur, invoqué)** : identité nerf / mosaïque même dégénérée, couverture par les labels,
  inclusions, KKT suffisant pour un problème convexe, effondrements élémentaires (Forman). L'oracle ne re-démontre
  rien : il vérifie ses propres sorties contre ces énoncés sur des cas bornés.
- **Vérifié borné** : tout ce qui est dans `resultats/` (fixtures, épreuves, comparaisons) ; décisions exactes au sens
  arithmétique seulement.
- **Mesuré** : temps de l'oracle sur petits nuages (un seul fil, machine partagée, `nice -n 10`).
- **Rien n'est exact au sens du registre** ; aucune conclusion d'échelle (8 000 / 16 000 / 32 000 : non mesuré).

## 10. Limites déclarées

- **Bornes** : n <= 60, k <= 8 (refus au-delà) ; familles de cellule <= 20 000 étiquettes ; coût en temps mesuré
  au § 12 (un fil). C'est un oracle de correction : aucune conclusion de coût, de taille ou d'échelle n'en découle
  (8 000 / 16 000 / 32 000 : non mesuré).
- **Complétude** : elle vient de l'énumération exhaustive des (r+1)-parties (définition des sommets de Voronoï) ; les
  contrôles (appariement, convexité locale, Euler, juge et recouvrement par échantillon) sont un filet contre les
  fautes d'implémentation, pas une preuve indépendante de complétude : le juge et le recouvrement sont des
  échantillons (200 et 60 points par ordre).
- **Accélérateurs** : numpy (filtre int64 sous borne prouvée, re-décidé en entiers) et qhull (propositions de
  facettes vérifiées exactement, fermeture exigée, repli en force brute). Sans eux (`ORACLE_AK_PUR=1`), sorties des
  fixtures identiques à l'octet ; seules les fixtures ont été rejouées dans ce mode.
- **Offset exact** : appartenance d'un point à une pièce C_Q(r) + B_r par énumération d'ensembles actifs (<= 4) ;
  correct pour un problème convexe (premier point KKT = minimum global) mais combinatoire : réservé aux petits cas.
- **FULL** : comparaison sur la sortie `supports` (arbre d'ordre K seul) ; les verticales entre ordres de FULL ne
  sont pas comparées (seule leur bonne définition côté oracle est vérifiée).
- **Pas de géométrie de C_v elle-même** : l'oracle donne A_k(r), ses niveaux, ses labels, la couverture et
  l'appartenance exacte à l'offset ; la frontière sphérique de C_v n'est pas maillée.
- **Réduction** : `reduction_gloutonne` est un générateur de test du juge, pas une proposition de réduction ; aucune
  minimalité, aucune taille de L n'est revendiquée.

## 11. Robustesse au niveau pi0 (vérifié borné, `epreuves_robustesse.py`, `resultats/robustesse.json`)

Trois épreuves distinctes, comme le demande la note 28d70f8ab, par applications exactes entre composantes (témoins des
sommets, `application_temoins`), sur 3 tirages chacune :

| Épreuve | Nuages | Coupes | Composée = inclusion | Nombre de composantes différent à (k, r) fixés |
| --- | --- | --- | --- | --- |
| positions appariées (déplacement entier, delta^2 <= 3), k <= 3 : g o f = inclusion de r dans r + 2 delta | 3 x 12 sites | 1566 | 1566 | 581 |
| ajout d'un site : Omega_k^P dans Omega_k^{P u O} dans Omega_{k-1}^P, k = 3 | 3 x 11 sites | 609 | 609 | 116 |
| suppression d'un site : Omega_{k+1}^P dans Omega_k^Q dans Omega_k^P, k = 2 | 3 x 12 sites | 818 | 818 | non mesuré |


Lecture : l'entrelacement à homotopie près (ici sur pi0) est exact sur tous les cas ; l'invariance à (k, r) fixés est
fausse, comme annoncé par l'auditeur (le nombre de composantes change à r fixé dans une part notable des coupes).
Ce sont des vérifications de formules sur cas bornés, pas des mesures de stabilité géométrique du dessin.

## 12. Temps de l'oracle (mesuré, un fil, `nice -n 10`, machine partagée ; `resultats/temps.json`)

| n | ordre k | faces (toutes dimensions) | faces par site | temps de l'ordre (s) |
| --- | --- | --- | --- | --- |
| 20 | 1 | 273 | 13.7 | 0.3 |
| 20 | 2 | 1187 | 59.4 | 1.5 |
| 20 | 3 | 2587 | 129.3 | 4.1 |
| 20 | 4 | 4175 | 208.8 | 7.7 |
| 40 | 1 | 763 | 19.1 | 1.0 |
| 40 | 2 | 3673 | 91.8 | 6.2 |
| 40 | 3 | 8627 | 215.7 | 19.8 |
| 60 | 1 | 1231 | 20.5 | 1.8 |
| 60 | 2 | 6035 | 100.6 | 11.9 |
| 60 | 1 | 1309 | 21.8 | 1.8 |
| 60 | 2 | 6285 | 104.8 | 12.7 |
| 60 | 3 | 15231 | 253.8 | 72.1 |

Temps totaux (sphères comprises) : n = 20, k <= 4 : 13.6 s ; n = 40, k <= 3 : 27.2 s ; n = 60, k <= 2 : 14.5 s ; n = 60, k <= 3 : 87.3 s. Nuages aléatoires entiers dans [0, 200)^3 (graine n x 100 + kmax : les deux lignes n = 60 sont deux nuages différents). Les faces par site de ces petits nuages sont dominées par le bord : ce ne sont pas des densités d'échelle.

Ces temps fixent la borne utile de l'oracle ; ils ne disent rien du coût du produit.

## 13. Provenance

- Moteur : `/workspaces/E-HGP/build/v11-persist/videos/build-v11-sp3/mhgp11` (compilé à 07428324e), lecteur officiel
  `mhgp11_formats.py` du worktree `build/v11-videos-20261004` (d8c015dbd). Prototype et oracle borné lus dans
  `build/v11-persist/polyedres_reconnaissables/` (importés, jamais modifiés). Trame 08/000100 lue par le harnais.
- Les campagnes finales (comparaisons et LiDAR rejouées à partir de 23 h 05 UTC sur `oracle_ak.py` `b52fddd4…` ;
  épreuves, robustesse, temps, ordres élevés lancés entre 22 h 59 et 23 h 03) ont tourné sur des versions qui ne
  diffèrent de la finale que par des fonctions de juge ajoutées (`face_duale`, `juger_sous_complexe`), sans effet
  sur la construction ni sur les niveaux ; les fixtures ont été rejouées sur la version finale (même empreinte de
  sortie `edbc7e56…`). Les passes antérieures sont gardées (`resultats/avant_correctif/`, `resultats/passe_intermediaire/`) ;
  la passe finale redonne exactement les mêmes comptes que la passe intermédiaire. Après les campagnes, seuls des
  nettoyages pyflakes (imports et variables inutilisés) ont touché `comparaisons.py`, `epreuves_oracle.py` et
  `controle_prototype.py`.
  Empreintes des sources : `resultats/SHA256_SOURCES.txt` (après la dernière modification).
- Écritures : seulement dans ce dossier (moins de 1 Mo) ; aucune donnée brute SemanticKITTI hors de `build/` (seuls des
  indices de lignes et les coordonnées relatives des cinq retours du trou sont écrits) ; aucun commit, aucun GCP.
