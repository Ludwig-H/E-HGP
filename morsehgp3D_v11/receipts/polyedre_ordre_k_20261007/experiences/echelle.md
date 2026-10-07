# Taille et coût du complexe alpha d'ordre k aux tailles d'intérêt

6 octobre 2026, 22 h 55 UTC – 7 octobre 2026, 00 h 14 UTC (heures relevées par `date -u`). Rôle : expérimentateur « taille et coût ».

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (prototype Python ; mhgp11 07428324e pour la tour)
profile=quantized_u21_input_only (grille de 1 mm)
public_status=not_claimed
GCP non utilisé
```

Prédictions écrites avant toute mesure : [PREDICTIONS.md](PREDICTIONS.md) (scellées par `PREDICTIONS.sha256`, 23 h 02 UTC).
Verdicts au § 6. Rien ici n'est qualifié ; les nombres sont des mesures (comptes exacts, temps du codespace partagé
sous charge, 2 fils) ou des extrapolations déclarées comme telles.

## 1. Ce qui est mesuré

**Objet.** Pour un nœud v de la tour FULL (MHGP11SP v2) à l'ordre k, la composante A_{k,v}(r) du complexe alpha
d'ordre k (mosaïque de Delaunay d'ordre k, filtration a_σ = min_{F_σ} d_k², valeur `R` de
`approche_mosaique/mosaique.py`), à trois niveaux de la vie [b_v, d_v) du nœud : naissance b_v (coupe fermée),
milieu géométrique √(b_v d_v) (fermée), fin de vie d_v^- (a_σ < d_v² strict). Puis sa réduction L par effondrements
polyédraux à sommets protégés (proposition de l'auditeur, 28d70f8ab, § 2–3).

**Construction locale certifiée** (`mesure_locale.py`, `campagne.py`). Y = P ∩ (cœur ⊕ B_ρ), cœur = sites des S* du
sous-arbre, ρ = 2 d_v + 1 mm ; mosaïque COMPLÈTE d'ordre k de Y (ordres 1 à k, `mosaique.construire`) ; composante
désignée par le sommet de naissance (k-ensemble = population exacte d'une boule de naissance du sous-arbre ; à K = 1,
le site de la feuille, `leaf_site` du lecteur officiel) ; **certificat de localité** : tout site de P à au plus 2 d_v
d'une étiquette de la composante à d_v^- est dans Y (sinon Y est agrandi). Sous ce certificat, la composante de Y
est celle de P, avec les mêmes cellules, niveaux et étiquettes (argument : les k plus proches voisins de tout point
de la composante, et de son voisinage immédiat, sont dans Y).

**Contrôle strict de chaque mosaïque** (pas un volume) : chaque 2-face a une ou deux 3-cellules ; chaque 2-face à une
seule 3-cellule est sur un plan d'appui de l'enveloppe des barycentres (décision entière) ; caractéristique d'Euler
de la mosaïque = 1 ; refus de propositions de chaque ordre relevés. Le trou de 29 220 mm³ de la critique précédente
laisserait des 2-faces intérieures orphelines, hors de tout plan d'appui : il serait signalé (raisonnement ; le cas de
08/000100 à k = 2 n'a pas été rejoué ici).

**Attribution par labels et incidences, comparée à FULL** : tous les sommets de naissance des boules de naissance du
sous-arbre doivent être dans la composante à d_v^-, aucun sommet de naissance extérieur (niveau < d_v², population
dans Y) ne doit y être (K = 1 : sites de la composante = feuilles du sous-arbre). Aucune position de barycentre
n'est utilisée.

**Réduction** : paires (σ, τ), σ facette de τ, σ libre dans le complexe COURANT de toute la plage [0, d_v)
(A_{k,v}(d_v^-)), même naissance EXACTE (égalité des flottants correctement arrondis, puis même face réalisant le
minimum ou égalité de `Fraction`), aucun sommet retiré ; priorité : dimension de τ décroissante, diamètre de τ
croissant, clés (I', U') triées (reproductible, pas canonique). **Vérificateur indépendant** (`verifier`) : rejoue
le journal sur des ensembles d'incidences, recalcule la naissance par l'étoile (autre chemin de code), contrôle
facette, liberté, maximalité, sommets, résultat et fermeture de L ; puis, à chaque niveau, χ(L_r) = χ(A_r) et L_r
connexe. Limite d'un calcul local : le certificat vaut pour r < d_v ; au-delà (vie du parent), une face libre peut
recevoir une coface : il faut recertifier sur le complexe du parent.

**Mesures** : cellules par dimension, faces exposées (2-faces de bord + 2-faces, arêtes et sommets isolés), points
couverts P ∩ (C ⊕ B_r) (réunion des étiquettes), par nœud et par point couvert ; D_v(r) = plus grand diamètre de
cellule et θ = D_v / r ; |Y| et temps de construction, de réduction, de vérification.

**Jeux d'échelle** (`jeux_echelle.py`) : `kitti200_n8000/16000/32000` = les n sites de `trame_08_000200` les plus
proches du capteur (emboîtés ; mêmes PointId et étiquettes ; tour recalculée par mhgp11 sur chaque sous-ensemble) et
`trame_08_000200` entière (45 845 sites, tour du cache du harnais) ; `tuile_b1/b2/b4` = 1, 2, 4 blocs translatés de
(synth_rue + synth_deux_nappes_10m + synth_velo_occulte_10m), 8 027 / 16 054 / 32 108 sites. Tirages : meilleur nœud
oracle d'objets (borne, pas méthode), plus haut ancêtre d'IoU >= 0,5 (« haut de vie » de l'objet), nœuds uniformes
(graine déclarée). **Jeux communs** (`campagne_scenes.py`) : les sept scènes synthétiques de l'audit à K = 1, 2, 3,
5, mosaïque entière (petits nuages : oracle et ordres de grandeur, jamais une pente), meilleurs nœuds des objets et
des parties et ancêtres de la partie jusqu'à l'objet ; la découpe réelle en image seulement.

## 2. Résultats d'échelle

Toutes les mosaïques locales passent le contrôle strict (417 nœuds mesurés, 417 mosaïques complètes au sens
du § 1 ; des propositions refusées existent à certains ordres, jusqu'à 328 sur une scène, sans jamais laisser de 2-face
intérieure orpheline). Tous les nœuds sont conformes à FULL (417/417 : naissances du sous-arbre toutes dans la
composante, aucune naissance extérieure, une seule composante). Les 414 journaux de réduction passent le vérificateur
indépendant (jusqu'à 209 101 paires) ; aux 1 242 couples (nœud, niveau), χ(L_r) = χ(A_r) et L_r est connexe. Trois
composantes de plus de 600 000 faces (hauts de vie d'objets à K = 5) n'ont pas été réduites (borne déclarée).

**Densité de la représentation par événements** (la mosaïque complète d'ordre k, chaque cellule une fois avec sa
date ; mosaïques locales de Y >= 425 points et scènes de 575 points et plus) : K = 1 : 26 à 27 faces par site
(6,0 à 6,2 tétraèdres) ; K = 2 : 132 à 138 (23,5 à 24,4 3-cellules) ; K = 3 : 333 à 353 (56,6 à 60) ; K = 5 :
1 003 à 1 086 (168 à 181). Pour une trame de 45 845 sites : environ 6,2 millions de faces à K = 2 et 48 millions à
K = 5. La tour FULL publie 4,5 à 4,8 nœuds par site à K = 2 et 13,3 à 16,1 à K = 5 (KITTI) : la mosaïque compte 28 à 30 fois
(K = 2) et 62 à 82 fois (K = 5) plus d'éléments que la tour.

**Taille d'un nœud** (fin de vie d_v^-). Meilleurs nœuds d'objets : K = 2 : 1 768 à 76 523 faces, 3,6 (nappes minces) à
118 faces par point couvert ; K = 5 : 6 133 à 558 462 faces, 76 à 483 faces par point couvert (la voiture de 1 157
points : 558 462 faces dont 86 565 3-cellules). Hauts de vie (plus haut ancêtre d'IoU >= 0,5) : K = 2 : 93 179 à
142 825 faces (123 à 129 par point) ; K = 5 : 343 417 (vélo, 405 points) à 1 084 689 (voiture et voisinage à 1,7 m),
848 à 983 faces par point couvert. Nœuds uniformes : à K = 5, 8 sur 15 sont un seul sommet (naissances qui meurent
aussitôt) ; de 1 à 65 651 faces (K = 2) et de 1 à 54 016 (K = 5). Les trois niveaux d'un même nœud diffèrent
peu : rapport des faces fin / naissance médian 1,02 sur 146 nœuds oracle (40 % à moins de 1 %, 66 % à moins de 5 %,
au plus 6) ; la vie d'un meilleur nœud est courte (d_v / b_v médian 1,013). Les « niveaux » d'un objet sont portés par
sa chaîne d'ancêtres : de la plus grande partie à l'objet, la taille est multipliée par 1,3 à 130 (jusqu'à 8 700
depuis la plus petite partie), puis par 1,9 à 3,3 jusqu'au haut de vie (tables des scènes et de localité).

**Localité exacte** : un même nœud certifié a exactement les mêmes comptes à toutes les tailles où il existe : voiture
à 16 000, 32 000 et 45 845 sites (76 523 et 142 825 faces à K = 2 ; 558 462 et 1 084 689 à K = 5 aux deux tailles
mesurées) ; vélo des tuiles b1 = b2 = b4 (47 945, 110 832, 343 417). Le sous-ensemble de 8 000 sites tronque la voiture
(720 points au lieu de 1 157) : c'est un autre nœud (273 176 faces). La taille d'un nœud ne dépend pas de n ; ce qui en
dépend est le nombre de nœuds (proportionnel à n) et la profondeur de l'arbre (table ci-dessous).

**Réduction à sommets protégés** (fin de vie). L / A : K = 5 : 0,16 à 0,45 pour les objets (0,20 à 0,68 sur les
nœuds des scènes) ; K = 2 : 0,21 à 0,82 ; K = 1 : 0,28 à 1,0. Il reste 0,5 à 21 % des 3-cellules (K = 5) ; sommets et
arêtes font 56 à 100 % de L : le plancher est celui des sommets protégés. Faces de L par point couvert : 32 à 124
(meilleurs nœuds, K = 5), 3 à 24 (K = 2). **Le dessin ne diminue pas** : rapport des faces exposées L / A de 0,49 à
5,8 (médiane 1,0) sur 49 nœuds d'objets à K = 5 ; au haut de vie, A est un bloc (faces exposées = 1,2 % de A) et L en
expose 5,8 fois plus.

**Limite du certificat local** (`limite_locale.py`, `limite_locale_k5.py`, `resultats/limite_locale.jsonl` ; 57
couples nœud → parent : parties oracle et deux ancêtres sur quatre scènes à K = 2 et 5, la voiture de
`kitti200_n16000` à K = 2 et 5 sur des mosaïques certifiées pour le parent). La suite d'effondrements de v, certifiée
sur [0, d_v), **cesse d'être exécutable telle quelle sur la vie du parent** dans 47 cas sur 57 : 2 582 des 306 676
paires (0,8 % ; par nœud médiane 1,3 %, au plus 31 %) voient σ recevoir une coface NOUVELLE (apparue entre d_v et d_p),
et le rejeu ordonné échoue dès la paire 0 à 836 (médiane 74) ; c'est le cas AB/ABD de l'auditeur, à l'échelle. Mais
**la réduction certifiée pour le parent, restreinte à v, vaut celle de v** : |L_p ∩ A_v| / |L_v| = 1,000 en médiane
(au plus 1,032), χ égal et L connexe dans les 57 cas, et le journal du parent reprend 74 à 100 % (médiane 99,3 %) des
paires de v. Conséquence pratique : certifier une seule fois, au sommet de la chaîne voulue (haut de vie d'un objet, ou
toute la scène), puis restreindre aux descendants, donne des représentants emboîtés le long de la hiérarchie pour un
surcoût de 0 à 3 % ; des réductions indépendantes nœud par nœud ne sont ni emboîtées ni prolongeables.

**Budget géométrique** θ = D_v / r (plus grand diamètre de cellule de la composante sur le rayon) : maximum 2,00 (K = 1),
1,96 (K = 2), 1,32 (K = 3), 0,79 (K = 5) : toujours sous la borne générique 4/k (k >= 2) et 2 (k = 1).

**Temps** (Python, 2 fils, `nice`, codespace chargé ; indicatifs) : construction locale 0,4 à 1,0 ms par point de Y à
K = 1, 1,7 à 6,8 à K = 2, 7 à 14 à K = 3 (scènes), 38 à 75 à K = 5 pour les objets (12 à 41 pour de très petits Y ;
136 sous forte charge : la même construction de 1 157 points a pris 56,8 puis 157,5 s) ; réduction 15 à 25 µs par
face, vérification 5 à 12 µs par face, contrôle FULL au plus 0,75 s. Y / points couverts : 1,0 à 3,2 pour les objets
(sol retiré : Y est presque l'objet), jusqu'à 20 pour des naissances minuscules (rayon de certificat 2 d_v).

### Tours FULL

| jeu | K | sites | nœuds | nœuds par site | mhgp11 (s, 2 fils, codespace) |
| --- | --- | --- | --- | --- | --- |
| kitti200_n8000 | 2 | 8 000 | 38 407 | 4.80 | 0.24 |
| kitti200_n8000 | 5 | 8 000 | 128 670 | 16.08 | 1.61 |
| kitti200_n16000 | 2 | 16 000 | 75 131 | 4.70 | 0.54 |
| kitti200_n16000 | 5 | 16 000 | 233 366 | 14.59 | 2.96 |
| kitti200_n32000 | 2 | 32 000 | 148 937 | 4.65 | 0.92 |
| kitti200_n32000 | 5 | 32 000 | 436 075 | 13.63 | 6.80 |
| tuile_b1 | 2 | 8 027 | 32 165 | 4.01 | 0.28 |
| tuile_b1 | 5 | 8 027 | 74 983 | 9.34 | 1.22 |
| tuile_b2 | 2 | 16 054 | 64 350 | 4.01 | 0.44 |
| tuile_b2 | 5 | 16 054 | 150 190 | 9.36 | 1.94 |
| tuile_b4 | 2 | 32 108 | 128 702 | 4.01 | 1.10 |
| tuile_b4 | 5 | 32 108 | 300 424 | 9.36 | 5.41 |
| kitti200_n8000 | 1 | 8 000 | 15 917 | — | 0.12 |

### Nœuds mesurés (fin de vie d_v^-), agrégats par jeu, K et tirage

| jeu | K | tirage | nœuds | Y médian | construction médiane (s) | ms par point de Y | faces de A (médiane / max) | 3-cellules (méd.) | exposées (méd.) | faces de A par point couvert | L / A (méd.) | faces de L par point couvert | θ max | FULL | vérif. | complètes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| kitti200_n16000 | 2 | oracle | 1 | 1 157 | 5.86 | 5.1 | 76 523 / 76 523 | 10 921 | 8 435 | 66.1 | 0.361 | 23.9 | 1.73 | 1/1 | 1/1 | 1/1 |
| kitti200_n16000 | 2 | uniforme | 3 | 171 | 0.69 | 4.0 | 882 / 26 952 | 34 | 243 | 14.5 | 0.612 | 8.9 | 1.93 | 3/3 | 3/3 | 3/3 |
| kitti200_n16000 | 5 | oracle | 1 | 1 157 | 67.27 | 58.1 | 558 462 / 558 462 | 86 565 | 26 736 | 482.7 | 0.251 | 121.2 | 0.69 | 1/1 | 1/1 | 1/1 |
| kitti200_n16000 | 5 | uniforme | 3 | 98 | 3.56 | 36.3 | 1 / 19 275 | 0 | 1 | 0.2 | 1.000 | 0.2 | 0.63 | 3/3 | 3/3 | 3/3 |
| kitti200_n32000 | 2 | oracle | 2 | 696 | 3.46 | 4.7 | 41 667 / 76 523 | 5 990 | 4 448 | 72.2 | 0.305 | 21.7 | 1.73 | 2/2 | 2/2 | 2/2 |
| kitti200_n32000 | 2 | uniforme | 3 | 9 | 0.03 | 5.0 | 7 / 12 974 | 0 | 1 | 2.3 | 0.714 | 1.7 | 1.82 | 3/3 | 3/3 | 3/3 |
| kitti200_n32000 | 5 | oracle | 2 | 719 | 43.33 | 56.7 | 299 134 / 558 462 | 46 402 | 14 190 | 467.5 | 0.214 | 100.7 | 0.69 | 2/2 | 2/2 | 2/2 |
| kitti200_n32000 | 5 | uniforme | 3 | 67 | 4.09 | 27.3 | 1 / 54 016 | 0 | 1 | 0.2 | 1.000 | 0.2 | 0.75 | 3/3 | 3/3 | 3/3 |
| kitti200_n8000 | 2 | oracle | 1 | 720 | 4.37 | 6.1 | 28 130 / 28 130 | 3 066 | 5 535 | 39.2 | 0.451 | 17.7 | 1.90 | 1/1 | 1/1 | 1/1 |
| kitti200_n8000 | 2 | uniforme | 3 | 65 | 0.30 | 5.4 | 407 / 31 992 | 14 | 113 | 8.0 | 0.583 | 4.9 | 1.90 | 3/3 | 3/3 | 3/3 |
| kitti200_n8000 | 5 | oracle | 1 | 720 | 53.84 | 74.8 | 273 176 / 273 176 | 40 634 | 19 356 | 379.4 | 0.328 | 124.3 | 0.77 | 1/1 | 1/1 | 1/1 |
| kitti200_n8000 | 5 | uniforme | 3 | 64 | 5.24 | 41.0 | 1 / 790 | 0 | 1 | 0.2 | 1.000 | 0.2 | 0.60 | 3/3 | 3/3 | 3/3 |
| tuile_b1 | 2 | oracle | 2 | 344 | 1.73 | 4.7 | 5 429 / 9 090 | 592 | 980 | 26.1 | 0.595 | 10.5 | 1.75 | 2/2 | 2/2 | 2/2 |
| tuile_b1 | 2 | uniforme | 3 | 12 | 0.02 | 2.0 | 11 / 65 570 | 0 | 3 | 2.2 | 0.818 | 1.8 | 1.76 | 3/3 | 3/3 | 3/3 |
| tuile_b1 | 5 | oracle | 2 | 166 | 7.71 | 45.2 | 20 805 / 24 953 | 2 666 | 2 752 | 137.4 | 0.294 | 40.2 | 0.70 | 2/2 | 2/2 | 2/2 |
| tuile_b1 | 5 | uniforme | 3 | 42 | 1.97 | 25.0 | 1 / 1 049 | 0 | 1 | 0.2 | 1.000 | 0.2 | 0.39 | 3/3 | 3/3 | 3/3 |
| tuile_b2 | 2 | oracle | 2 | 420 | 1.98 | 4.5 | 5 852 / 9 090 | 598 | 1 122 | 26.3 | 0.584 | 10.6 | 1.76 | 2/2 | 2/2 | 2/2 |
| tuile_b2 | 2 | uniforme | 3 | 130 | 0.52 | 4.0 | 520 / 65 651 | 0 | 161 | 4.7 | 0.762 | 3.6 | 1.76 | 3/3 | 3/3 | 3/3 |
| tuile_b2 | 5 | oracle | 2 | 348 | 18.00 | 51.8 | 116 746 / 122 661 | 17 494 | 7 632 | 346.9 | 0.252 | 86.8 | 0.74 | 2/2 | 2/2 | 2/2 |
| tuile_b2 | 5 | uniforme | 3 | 27 | 0.98 | 17.8 | 1 / 994 | 0 | 1 | 0.2 | 1.000 | 0.2 | 0.39 | 3/3 | 3/3 | 3/3 |
| tuile_b4 | 2 | oracle | 2 | 210 | 0.71 | 3.5 | 6 868 / 9 090 | 772 | 1 210 | 34.2 | 0.410 | 13.5 | 1.51 | 2/2 | 2/2 | 2/2 |
| tuile_b4 | 2 | uniforme | 3 | 3 536 | 19.26 | 5.4 | 24 522 / 65 630 | 415 | 8 200 | 7.7 | 0.664 | 5.1 | 1.76 | 3/3 | 3/3 | 3/3 |
| tuile_b4 | 5 | oracle | 2 | 194 | 8.64 | 41.6 | 64 397 / 122 661 | 9 895 | 3 218 | 247.9 | 0.344 | 67.8 | 0.74 | 2/2 | 2/2 | 2/2 |
| tuile_b4 | 5 | uniforme | 3 | 29 | 1.07 | 21.7 | 1 / 1 049 | 0 | 1 | 0.2 | 1.000 | 0.2 | 0.39 | 3/3 | 3/3 | 3/3 |

### Localité : le même objet aux trois tailles (et la trame entière)

Meilleur nœud oracle de l'objet (`oracle`) et plus haut ancêtre d'IoU >= 0,5 (`oracle_haut`), à d_v^- ; objet 1 (« voiture ») de `trame_08_000200` dans ses sous-ensembles, objet 0 (vélo de synth_rue, bloc 0) des tuiles. Temps : codespace partagé (la même construction de 1 157 points a pris 56,8 s puis 157,5 s selon la charge).

| jeu | K | tirage | nœud | b_v (mm) | d_v (mm) | Y | points couverts | faces de A | 3-cellules | exposées | L / A | θ | construction (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| tuile_b1 | 2 | oracle | 31759 | 144.6 | 971.5 | 412 | 405 | 47 945 | 8 249 | 841 | 0.206 | 0.79 | 1.7 |
| tuile_b1 | 5 | oracle | 70671 | 151.1 | 151.5 | 405 | 405 | 110 832 | 15 946 | 9 678 | 0.262 | 0.72 | 18.1 |
| tuile_b1 | 5 | oracle_haut | 73782 | 200.5 | 975.4 | 418 | 405 | 343 417 | 56 621 | 4 009 | 0.158 | 0.32 | 19.1 |
| tuile_b2 | 2 | oracle | 63528 | 144.6 | 971.5 | 412 | 405 | 47 945 | 8 249 | 841 | 0.206 | 0.79 | 2.0 |
| tuile_b2 | 5 | oracle | 141470 | 151.1 | 151.5 | 405 | 405 | 110 832 | 15 946 | 9 678 | 0.262 | 0.72 | 16.3 |
| tuile_b2 | 5 | oracle_haut | 147692 | 200.5 | 975.4 | 418 | 405 | 343 417 | 56 621 | 4 009 | 0.158 | 0.32 | 18.4 |
| tuile_b4 | 2 | oracle | 127066 | 144.6 | 971.5 | 412 | 405 | 47 945 | 8 249 | 841 | 0.206 | 0.79 | 1.4 |
| tuile_b4 | 5 | oracle | 283068 | 151.1 | 151.5 | 405 | 405 | 110 832 | 15 946 | 9 678 | 0.262 | 0.72 | 16.0 |
| tuile_b4 | 5 | oracle_haut | 295512 | 200.5 | 975.4 | 418 | 405 | 343 417 | 56 621 | 4 009 | 0.158 | 0.32 | 19.1 |
| kitti200_n8000 | 2 | oracle | 37695 | 83.1 | 83.3 | 720 | 718 | 28 130 | 3 066 | 5 535 | 0.451 | 1.90 | 2.6 |
| kitti200_n8000 | 2 | oracle_haut | 38103 | 126.5 | 2 198.2 | 722 | 720 | 93 179 | 16 290 | 1 121 | 0.237 | 0.65 | 2.7 |
| kitti200_n8000 | 5 | oracle | 127168 | 134.5 | 134.8 | 720 | 720 | 273 176 | 40 634 | 19 356 | 0.328 | 0.77 | 32.2 |
| kitti200_n8000 | 5 | oracle_haut | 128584 | 2 227.6 | 2 231.3 | 2 036 | 724 | 711 299 | 118 190 | 5 106 | non réduit (> 600 000 faces) | 0.79 | 129.6 |
| kitti200_n16000 | 2 | oracle | 74018 | 154.8 | 167.0 | 1 157 | 1 157 | 76 523 | 10 921 | 8 435 | 0.361 | 1.73 | 4.2 |
| kitti200_n16000 | 2 | oracle_haut | 74703 | 320.6 | 1 706.0 | 1 170 | 1 157 | 142 825 | 24 761 | 2 503 | 0.231 | 1.23 | 4.4 |
| kitti200_n16000 | 5 | oracle | 229437 | 258.2 | 258.4 | 1 157 | 1 157 | 558 462 | 86 565 | 26 736 | 0.251 | 0.69 | 56.8 |
| kitti200_n16000 | 5 | oracle_haut | 233070 | 1 720.3 | 1 721.7 | 2 405 | 1 158 | 1 084 689 | 179 477 | 10 475 | non réduit (> 600 000 faces) | 0.50 | 163.0 |
| kitti200_n32000 | 2 | oracle | 145487 | 154.8 | 167.0 | 1 157 | 1 157 | 76 523 | 10 921 | 8 435 | 0.361 | 1.73 | 4.5 |
| kitti200_n32000 | 2 | oracle_haut | 147525 | 320.6 | 1 706.0 | 1 170 | 1 157 | 142 825 | 24 761 | 2 503 | 0.231 | 1.23 | 5.6 |
| kitti200_n32000 | 5 | oracle | 421623 | 258.2 | 258.4 | 1 157 | 1 157 | 558 462 | 86 565 | 26 736 | 0.251 | 0.69 | 157.5 |
| kitti200_n32000 | 5 | oracle_haut | 434652 | 1 720.3 | 1 721.7 | 2 470 | 1 158 | 1 084 689 | 179 477 | 10 475 | non réduit (> 600 000 faces) | 0.50 | 156.7 |
| trame_08_000200 | 2 | oracle | 188439 | 154.8 | 167.0 | 1 157 | 1 157 | 76 523 | 10 921 | 8 435 | 0.361 | 1.73 | 4.4 |
| trame_08_000200 | 2 | oracle_haut | 200417 | 320.6 | 1 706.0 | 1 170 | 1 157 | 142 825 | 24 761 | 2 503 | 0.231 | 1.23 | 4.4 |
| trame_08_000200 | 5 | — | — | — | — | — | — | budget de temps | — | — | — | — | — |

### Profondeur des arbres (coût d'une coupe par nœud stockée séparément)

| jeu, K | nœuds par site | profondeur moyenne | profondeur max | naissances : moyenne, médiane, q90 |
| --- | --- | --- | --- | --- |
| kitti200_n8000_k2 | 4.80 | 1 071 | 2 469 | 1 081, 973, 2 239 |
| kitti200_n8000_k5 | 16.08 | 5 117 | 11 066 | 5 188, 5 198, 9 902 |
| kitti200_n16000_k2 | 4.70 | 1 068 | 2 568 | 1 069, 880, 2 218 |
| kitti200_n16000_k5 | 14.59 | 4 829 | 12 015 | 4 869, 4 727, 9 279 |
| kitti200_n32000_k2 | 4.65 | 2 061 | 5 666 | 2 059, 1 240, 5 295 |
| kitti200_n32000_k5 | 13.63 | 9 275 | 24 793 | 9 280, 6 314, 22 066 |
| trame_08_000200_k2 | 4.53 | 1 924 | 5 981 | 1 921, 1 121, 5 489 |
| trame_08_000200_k5 | 13.29 | 9 996 | 27 548 | 10 007, 7 567, 24 034 |
| tuile_b1_k2 | 4.01 | 1 230 | 2 795 | 1 265, 1 045, 2 762 |
| tuile_b1_k5 | 9.34 | 3 752 | 6 697 | 3 810, 4 309, 6 638 |
| tuile_b2_k2 | 4.01 | 1 239 | 2 805 | 1 274, 1 054, 2 771 |
| tuile_b2_k5 | 9.36 | 3 839 | 6 789 | 3 896, 4 400, 6 730 |
| tuile_b4_k2 | 4.01 | 1 239 | 2 805 | 1 275, 1 054, 2 771 |
| tuile_b4_k5 | 9.36 | 3 839 | 6 791 | 3 894, 4 401, 6 731 |

## 3. Jeux communs (petites tailles : oracle, pas de pente)

### Mosaïques entières des scènes communes (petits nuages : ordres de grandeur, pas de pente)

| scène | K | points | s | ms/point | sommets | arêtes | 2-faces | 3-cellules | faces/point | complète | K = 1 : égal à gudhi |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| synth_anneau_perce_05m | 2 | 170 | 0.6 | 3.5 | 1 075 | 6 070 | 8 079 | 3 083 | 108 | oui | — |
| synth_anneau_perce_05m | 3 | 170 | 2.1 | 12.5 | 2 594 | 14 896 | 19 635 | 7 332 | 262 | oui | — |
| synth_anneau_perce_05m | 5 | 170 | 6.4 | 37.9 | 7 324 | 42 430 | 55 707 | 20 600 | 742 | oui | — |
| synth_anneau_perce_10m | 2 | 52 | 0.1 | 2.8 | 285 | 1 508 | 1 943 | 719 | 86 | oui | — |
| synth_anneau_perce_10m | 3 | 52 | 0.4 | 7.4 | 634 | 3 502 | 4 518 | 1 649 | 198 | oui | — |
| synth_anneau_perce_10m | 5 | 52 | 1.1 | 20.2 | 1 594 | 8 939 | 11 542 | 4 196 | 505 | oui | — |
| synth_velo_05m | 2 | 575 | 2.2 | 3.8 | 4 083 | 24 509 | 33 747 | 13 320 | 132 | oui | — |
| synth_velo_05m | 3 | 575 | 8.2 | 14.2 | 10 585 | 63 290 | 85 262 | 32 556 | 333 | oui | — |
| synth_velo_05m | 5 | 575 | 31.2 | 54.3 | 32 125 | 191 835 | 256 108 | 96 397 | 1 003 | oui | — |
| synth_velo_10m | 2 | 203 | 0.6 | 2.7 | 1 332 | 7 855 | 10 723 | 4 199 | 119 | oui | — |
| synth_velo_10m | 3 | 203 | 1.6 | 7.8 | 3 407 | 20 141 | 26 990 | 10 255 | 299 | oui | — |
| synth_velo_10m | 5 | 203 | 6.8 | 33.6 | 10 188 | 60 125 | 79 802 | 29 864 | 887 | oui | — |
| synth_pieton_05m | 2 | 649 | 2.7 | 4.2 | 4 762 | 28 884 | 39 910 | 15 787 | 138 | oui | — |
| synth_pieton_05m | 3 | 649 | 7.8 | 12.0 | 12 574 | 75 487 | 101 833 | 38 919 | 353 | oui | — |
| synth_pieton_05m | 5 | 649 | 38.9 | 59.9 | 38 899 | 232 804 | 311 110 | 117 204 | 1 079 | oui | — |
| synth_deux_nappes_10m | 2 | 1 139 | 4.4 | 3.9 | 8 170 | 48 636 | 66 720 | 26 253 | 132 | oui | — |
| synth_deux_nappes_10m | 3 | 1 139 | 13.2 | 11.5 | 20 710 | 125 381 | 169 999 | 65 327 | 335 | oui | — |
| synth_deux_nappes_10m | 5 | 1 139 | 63.6 | 55.8 | 65 821 | 397 476 | 533 605 | 201 949 | 1 053 | oui | — |
| synth_velo_occulte_10m | 2 | 581 | 2.2 | 3.8 | 4 194 | 25 386 | 35 017 | 13 824 | 135 | oui | — |
| synth_velo_occulte_10m | 3 | 581 | 6.4 | 11.0 | 11 122 | 67 035 | 90 538 | 34 624 | 350 | oui | — |
| synth_velo_occulte_10m | 5 | 581 | 30.1 | 51.9 | 35 251 | 210 219 | 280 310 | 105 341 | 1 086 | oui | — |
| synth_anneau_perce_05m | 1 | 170 | 0.1 | 0.5 | 170 | 1 075 | 1 738 | 832 | 22 | oui | 4/6 niveaux |
| synth_anneau_perce_10m | 1 | 52 | 0.0 | 0.4 | 52 | 285 | 437 | 203 | 19 | oui | 5/6 niveaux |
| synth_velo_05m | 1 | 575 | 0.5 | 0.8 | 575 | 4 083 | 6 962 | 3 453 | 26 | oui | 5/6 niveaux |
| synth_velo_10m | 1 | 203 | 0.1 | 0.5 | 203 | 1 332 | 2 224 | 1 094 | 24 | oui | 4/6 niveaux |
| synth_pieton_05m | 1 | 649 | 0.6 | 1.0 | 649 | 4 762 | 8 157 | 4 043 | 27 | oui | 0/6 niveaux |
| synth_deux_nappes_10m | 1 | 1 139 | 0.8 | 0.7 | 1 139 | 8 170 | 13 984 | 6 952 | 27 | oui | 0/6 niveaux |
| synth_velo_occulte_10m | 1 | 581 | 0.3 | 0.6 | 581 | 4 194 | 7 137 | 3 523 | 27 | oui | 1/6 niveaux |

### Nœuds des scènes communes (fin de vie), agrégats

| scène | K | tirage | nœuds | faces de A (méd. / max) | exposées (méd.) | faces de A par point couvert | L / A | θ max | FULL | vérif. |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| synth_anneau_perce_05m | 2 | oracle_objet | 1 | 960 / 960 | 257 | 5.6 | 0.715 | 1.88 | 1/1 | 1/1 |
| synth_anneau_perce_05m | 3 | ancetre_partie_objet | 4 | 1 380 / 1 477 | 368 | 8.6 | 0.589 | 1.13 | 4/4 | 4/4 |
| synth_anneau_perce_05m | 3 | oracle_objet | 1 | 1 542 / 1 542 | 414 | 9.1 | 0.589 | 1.12 | 1/1 | 1/1 |
| synth_anneau_perce_05m | 3 | oracle_partie | 1 | 1 210 / 1 210 | 316 | 8.6 | 0.577 | 0.66 | 1/1 | 1/1 |
| synth_anneau_perce_05m | 5 | oracle_objet | 1 | 7 844 / 7 844 | 1 729 | 46.1 | 0.311 | 0.52 | 1/1 | 1/1 |
| synth_anneau_perce_10m | 1 | ancetre_partie_objet | 4 | 46 / 193 | 22 | 2.0 | 1.000 | 2.00 | 4/4 | 4/4 |
| synth_anneau_perce_10m | 1 | oracle_partie | 1 | 13 / 13 | 6 | 1.9 | 1.000 | 2.00 | 1/1 | 1/1 |
| synth_anneau_perce_10m | 2 | ancetre_partie_objet | 4 | 180 / 419 | 52 | 5.2 | 0.691 | 1.41 | 4/4 | 4/4 |
| synth_anneau_perce_10m | 2 | oracle_objet | 1 | 901 / 901 | 240 | 17.3 | 0.303 | 0.77 | 1/1 | 1/1 |
| synth_anneau_perce_10m | 2 | oracle_partie | 1 | 7 / 7 | 3 | 1.4 | 1.000 | 1.00 | 1/1 | 1/1 |
| synth_anneau_perce_10m | 3 | ancetre_partie_objet | 4 | 266 / 763 | 76 | 7.9 | 0.578 | 0.84 | 4/4 | 4/4 |
| synth_anneau_perce_10m | 3 | oracle_objet | 1 | 1 431 / 1 431 | 373 | 27.5 | 0.275 | 0.73 | 1/1 | 1/1 |
| synth_anneau_perce_10m | 3 | oracle_partie | 1 | 19 / 19 | 3 | 3.2 | 0.684 | 0.61 | 1/1 | 1/1 |
| synth_anneau_perce_10m | 5 | ancetre_partie_objet | 4 | 308 / 661 | 84 | 8.7 | 0.490 | 0.57 | 4/4 | 4/4 |
| synth_anneau_perce_10m | 5 | oracle_objet | 1 | 2 129 / 2 129 | 503 | 40.9 | 0.243 | 0.50 | 1/1 | 1/1 |
| synth_anneau_perce_10m | 5 | oracle_partie | 1 | 25 / 25 | 4 | 2.8 | 0.680 | 0.40 | 1/1 | 1/1 |
| synth_deux_nappes_10m | 1 | oracle_objet | 2 | 1 658 / 1 883 | 1 088 | 2.9 | 1.000 | 2.00 | 2/2 | 2/2 |
| synth_deux_nappes_10m | 2 | oracle_objet | 2 | 2 192 / 2 615 | 777 | 3.8 | 0.808 | 1.76 | 2/2 | 2/2 |
| synth_deux_nappes_10m | 3 | oracle_objet | 2 | 11 766 / 13 382 | 2 850 | 20.7 | 0.516 | 0.61 | 2/2 | 2/2 |
| synth_deux_nappes_10m | 5 | oracle_objet | 2 | 46 930 / 48 283 | 14 903 | 41.7 | 0.458 | 0.40 | 2/2 | 2/2 |
| synth_pieton_05m | 1 | ancetre_partie_objet | 10 | 2 552 / 9 865 | 888 | 7.3 | 0.752 | 2.00 | 10/10 | 10/10 |
| synth_pieton_05m | 1 | oracle_partie | 6 | 439 / 1 901 | 163 | 6.8 | 0.797 | 2.00 | 6/6 | 6/6 |
| synth_pieton_05m | 2 | ancetre_partie_objet | 17 | 8 445 / 55 027 | 1 348 | 34.0 | 0.367 | 1.88 | 17/17 | 17/17 |
| synth_pieton_05m | 2 | oracle_objet | 1 | 56 041 / 56 041 | 3 426 | 86.3 | 0.283 | 1.71 | 1/1 | 1/1 |
| synth_pieton_05m | 2 | oracle_partie | 6 | 2 010 / 4 331 | 644 | 17.8 | 0.667 | 1.80 | 6/6 | 6/6 |
| synth_pieton_05m | 3 | ancetre_partie_objet | 17 | 18 583 / 130 817 | 4 736 | 96.2 | 0.296 | 1.29 | 17/17 | 17/17 |
| synth_pieton_05m | 3 | oracle_objet | 1 | 133 271 / 133 271 | 6 027 | 205.3 | 0.250 | 1.16 | 1/1 | 1/1 |
| synth_pieton_05m | 3 | oracle_partie | 6 | 4 492 / 6 215 | 1 296 | 36.4 | 0.538 | 1.15 | 6/6 | 6/6 |
| synth_pieton_05m | 5 | ancetre_partie_objet | 20 | 79 546 / 364 905 | 10 196 | 182.4 | 0.246 | 0.77 | 20/20 | 20/20 |
| synth_pieton_05m | 5 | oracle_objet | 1 | 369 519 / 369 519 | 11 528 | 569.4 | 0.223 | 0.76 | 1/1 | 1/1 |
| synth_pieton_05m | 5 | oracle_partie | 6 | 9 599 / 11 870 | 2 255 | 69.1 | 0.443 | 0.73 | 6/6 | 6/6 |
| synth_velo_05m | 1 | ancetre_partie_objet | 10 | 3 602 / 8 121 | 988 | 6.9 | 0.616 | 2.00 | 10/10 | 10/10 |
| synth_velo_05m | 1 | oracle_partie | 5 | 393 / 7 003 | 173 | 2.5 | 0.905 | 2.00 | 5/5 | 5/5 |
| synth_velo_05m | 2 | ancetre_partie_objet | 15 | 15 394 / 41 106 | 3 099 | 29.7 | 0.404 | 1.96 | 15/15 | 15/15 |
| synth_velo_05m | 2 | oracle_objet | 1 | 45 652 / 45 652 | 3 510 | 79.4 | 0.284 | 1.63 | 1/1 | 1/1 |
| synth_velo_05m | 2 | oracle_partie | 5 | 501 / 3 090 | 140 | 11.0 | 0.585 | 1.85 | 5/5 | 5/5 |
| synth_velo_05m | 3 | ancetre_partie_objet | 16 | 43 529 / 94 048 | 6 384 | 77.1 | 0.303 | 1.29 | 16/16 | 16/16 |
| synth_velo_05m | 3 | oracle_objet | 1 | 103 164 / 103 164 | 6 645 | 179.4 | 0.248 | 1.10 | 1/1 | 1/1 |
| synth_velo_05m | 3 | oracle_partie | 5 | 3 182 / 44 011 | 958 | 19.1 | 0.517 | 1.24 | 5/5 | 5/5 |
| synth_velo_05m | 5 | ancetre_partie_objet | 16 | 91 527 / 225 483 | 11 574 | 161.3 | 0.273 | 0.77 | 16/16 | 16/16 |
| synth_velo_05m | 5 | oracle_objet | 1 | 236 133 / 236 133 | 14 143 | 410.7 | 0.230 | 0.72 | 1/1 | 1/1 |
| synth_velo_05m | 5 | oracle_partie | 5 | 8 400 / 71 228 | 1 975 | 62.4 | 0.340 | 0.77 | 5/5 | 5/5 |
| synth_velo_10m | 1 | ancetre_partie_objet | 11 | 625 / 2 330 | 206 | 3.8 | 0.848 | 2.00 | 11/11 | 11/11 |
| synth_velo_10m | 1 | oracle_partie | 3 | 75 / 116 | 37 | 2.3 | 1.000 | 1.99 | 3/3 | 3/3 |
| synth_velo_10m | 2 | ancetre_partie_objet | 12 | 4 318 / 9 232 | 986 | 21.7 | 0.454 | 1.88 | 12/12 | 12/12 |
| synth_velo_10m | 2 | oracle_objet | 1 | 9 332 / 9 332 | 1 448 | 46.0 | 0.384 | 1.60 | 1/1 | 1/1 |
| synth_velo_10m | 2 | oracle_partie | 3 | 509 / 972 | 136 | 12.1 | 0.564 | 1.37 | 3/3 | 3/3 |
| synth_velo_10m | 3 | ancetre_partie_objet | 13 | 9 098 / 19 621 | 1 743 | 45.9 | 0.392 | 1.32 | 13/13 | 13/13 |
| synth_velo_10m | 3 | oracle_objet | 1 | 20 342 / 20 342 | 2 551 | 100.2 | 0.331 | 1.17 | 1/1 | 1/1 |
| synth_velo_10m | 3 | oracle_partie | 4 | 2 191 / 6 303 | 518 | 32.1 | 0.388 | 1.07 | 4/4 | 4/4 |
| synth_velo_10m | 5 | ancetre_partie_objet | 13 | 18 833 / 43 498 | 2 694 | 100.2 | 0.380 | 0.77 | 13/13 | 13/13 |
| synth_velo_10m | 5 | oracle_objet | 1 | 43 555 / 43 555 | 4 618 | 214.6 | 0.318 | 0.75 | 1/1 | 1/1 |
| synth_velo_10m | 5 | oracle_partie | 4 | 3 312 / 11 665 | 642 | 46.9 | 0.358 | 0.70 | 4/4 | 4/4 |
| synth_velo_occulte_10m | 1 | ancetre_partie_objet | 5 | 336 / 1 173 | 109 | 3.7 | 0.774 | 1.99 | 5/5 | 5/5 |
| synth_velo_occulte_10m | 1 | oracle_objet | 1 | 3 915 / 3 915 | 162 | 20.9 | 0.276 | 0.67 | 1/1 | 1/1 |
| synth_velo_occulte_10m | 1 | oracle_partie | 3 | 494 / 888 | 139 | 5.3 | 0.676 | 2.00 | 3/3 | 3/3 |
| synth_velo_occulte_10m | 2 | ancetre_partie_objet | 11 | 1 819 / 5 141 | 440 | 20.7 | 0.443 | 1.81 | 11/11 | 11/11 |
| synth_velo_occulte_10m | 2 | oracle_objet | 1 | 9 090 / 9 090 | 1 325 | 48.6 | 0.371 | 1.44 | 1/1 | 1/1 |
| synth_velo_occulte_10m | 2 | oracle_partie | 3 | 965 / 1 037 | 259 | 12.3 | 0.536 | 1.41 | 3/3 | 3/3 |
| synth_velo_occulte_10m | 3 | ancetre_partie_objet | 11 | 4 563 / 11 703 | 915 | 47.6 | 0.367 | 1.23 | 11/11 | 11/11 |
| synth_velo_occulte_10m | 3 | oracle_objet | 1 | 21 981 / 21 981 | 2 362 | 117.5 | 0.300 | 1.17 | 1/1 | 1/1 |
| synth_velo_occulte_10m | 3 | oracle_partie | 3 | 1 908 / 2 909 | 431 | 34.6 | 0.377 | 1.07 | 3/3 | 3/3 |
| synth_velo_occulte_10m | 5 | ancetre_partie_objet | 14 | 10 586 / 28 265 | 1 544 | 119.9 | 0.362 | 0.78 | 14/14 | 14/14 |
| synth_velo_occulte_10m | 5 | oracle_objet | 1 | 45 891 / 45 891 | 4 302 | 245.4 | 0.280 | 0.77 | 1/1 | 1/1 |
| synth_velo_occulte_10m | 5 | oracle_partie | 4 | 5 070 / 5 920 | 950 | 61.9 | 0.342 | 0.75 | 4/4 | 4/4 |

## 4. Témoin K = 1

**A_1(r) est exactement le complexe alpha** (vérifié borné, simplexe par simplexe, `verif_k1_gudhi.py`,
`resultats/verif_k1_gudhi.txt`) : sur `synth_anneau_perce_05m` (3 815 simplexes), `synth_velo_10m` (4 853) et le
voisinage Y de 721 points de la voiture de `kitti200_n8000` (19 539), les ensembles de simplexes de la mosaïque
d'ordre 1 et de `gudhi.AlphaComplex(precision='exact')` sont identiques et tous les niveaux coïncident à un écart
relatif au plus de 2,2e-16 (un ulp). Le niveau de la mosaïque est le flottant correctement arrondi du rationnel exact
(contrôlé par `Fraction`) ; celui de gudhi en diffère parfois d'un ulp : les quelques écarts de comptes de la colonne
« égal à gudhi » des tables sont des simplexes situés exactement au niveau choisi. Sur les trois scènes où au moins
cinq sites sont cosphériques (grille de 1 mm : 4, 10 et 38 coquilles), la mosaïque garde la cellule polyédrale et
gudhi la triangule (sur `synth_pieton_05m` : 18 simplexes de gudhi absents de la mosaïque, 6 faces polyédrales de la
mosaïque absentes de gudhi ; tous les simplexes communs au même niveau à un ulp près) : même espace filtré,
combinatoires différentes, sans simulation de simplicité de notre côté.

Témoin K = 1 à l'échelle (`kitti200_n8000`, tour d'ordre 1) : 5 nœuds (4 uniformes, la voiture) mesurés localement,
conformes à FULL (sites de la composante = feuilles du sous-arbre), mêmes comptes que gudhi aux niveaux b_v, d_v et
4 d_v². Un défaut de MON code a été trouvé en route : la feuille i de l'arbre d'ordre 1 n'est pas le site i mais
`leaf_site[i]` (sites triés par coordonnées, lecteur officiel) ; les premières mesures K = 1 des scènes désignaient la
mauvaise composante (FULL en échec, à juste titre) ; corrigé, refait (`mesures_scenes_k1.jsonl`), 100 % conformes. Ce
n'est pas une contradiction mathématique.

## 5. Images

Planches (`png/planche_*.png`) : ligne du haut A_{k,v}(r), ligne du bas L_r réduit, colonnes naissance, milieu, fin
de vie ; rendu `harnais.rendre_png` des faces exposées (2-faces de bord et isolées en vert, arêtes isolées en bleu,
sommets isolés en orange) ; **même caméra pour les six vues d'un nœud** : chaque vue porte tous les sommets de
A_{k,v}(d_v^-) (ceux qui ne sont pas dessinés le sont comme arêtes de longueur nulle gris très clair), ce qui fixe le
centre, l'orientation et l'étendue calculés par `rendre_png`.

- `planche_synth_velo_10m_k1_partie0.png` (roue arrière, K = 1 : le complexe alpha) ;
- `planche_synth_velo_10m_k2_objet0.png`, `planche_synth_velo_10m_k2_partie0.png` ;
- `planche_synth_velo_10m_k5_objet0.png`, `planche_synth_velo_10m_k5_partie0.png` ;
- `planche_synth_anneau_perce_05m_k2_objet0.png`, `planche_synth_anneau_perce_05m_k5_objet0.png` ;
- `planche_decoupe_08_002852_deux_velos_6_51_instances_k2_objet0.png` et `..._k5_objet0.png` (image seulement :
  aucun réglage choisi sur cette découpe).

Lecture : à K = 2 et 5, A remplit les roues dès la fin de vie du nœud du vélo (ouverture absente de la région dense
à ce rayon, comme l'avait relevé l'audit) ; L garde tous les sommets et devient un mélange de triangles et d'arêtes
isolées (« poils » bleus) : plus petit en faces, pas plus lisible ; les trois niveaux d'un meilleur nœud sont presque
identiques (vie courte).

## 6. Prédictions : verdicts

| prédiction | verdict | mesure |
| --- | --- | --- |
| P1 localité | **confirmée** (identité exacte) ; la seconde moitié (médianes à 1,5 près) **non testable** | même nœud certifié : mêmes comptes à 16 000, 32 000 et 45 845 sites et dans les trois tuiles ; médianes des nœuds uniformes (3 par taille, queue lourde) de 7 à 882 faces à K = 2 : pas de comparaison significative |
| P2 densité de la mosaïque | **confirmée** | K = 1 : 6,0–6,2 tétraèdres par site ; K = 2 : 23,5–24,4 3-cellules, 132–138 faces ; K = 5 : 168–181 3-cellules, 1 003–1 086 faces |
| P3 taille d'un nœud | **en partie réfutée** | naissance = 1 face : oui ; 3-cellules par point couvert à K = 2 : 0,01 (nappes) à 20 (hors de 5–20 pour les objets minces) ; K = 5 : 9 à 75 (sous la fourchette 30–120 pour les petits objets) ; nœuds uniformes à K = 5 : médiane 1 face (oui) |
| P4 faces exposées | **en partie** | K = 5 : 0,007 (haut de vie, bloc) à 0,15 (objets d'échelle), 0,03 à 0,32 (objets des scènes, nappes minces en haut) : sous 0,05 pour les grands objets et les hauts de vie |
| P5 réduction | **en partie** | K = 5 : L / A = 0,16–0,45 (prédit 0,15–0,40), 3-cellules gardées 0,5–21 % (prédit < 15 %) ; K = 2 : 0,21–0,82 (prédit 0,25–0,50) ; K = 1 : 0,28–1,0 (prédit 0,4–0,7) ; sommets + arêtes >= 56 % de L (prédit >= 60 %) |
| P6 vérification | **confirmée** | 414/414 journaux, 1 242/1 242 niveaux (χ égal, L_r connexe) |
| P7 budget géométrique θ | **réfutée à K = 2**, confirmée à K = 1 et 5 ; dépassement de 4/k **réfuté** | θ max : 2,00 (K = 1), 1,96 (K = 2, prédit 0,6–1,2 : j'avais oublié que 4/k = 2 à k = 2), 0,79 (K = 5) ; aucun cas au-dessus de 4/k |
| P8 FULL | **confirmée** | 417/417 (après correction de mon erreur feuille → site à K = 1) |
| P9 temps | **confirmée** (Y / couverts **réfuté**) | K = 5 : 38–75 ms par point de Y pour les objets (12 à 41 pour les très petits Y, 136 sous forte charge) ; K = 2 : 1,7–6,8 ; K = 1 : 0,4–1,0 ; réduction 15–25 µs par face ; Y / points couverts : 1,0 à 3,2 pour les objets (prédit 2 à 6 : objets isolés, sol retiré), jusqu'à 20 pour des naissances minuscules |
| P10 tour | **en partie** | K = 2 : 4,0–4,8 (oui) ; K = 5 : 13,6–16,1 (KITTI, 16,1 au-dessus de 15) et 9,4 (tuiles, sous 11) |
| P11 bornes | **réfutée (dans le bon sens)** | aucun refus par borne : objets à Y <= 2 470 points ; seule borne atteinte : réduction non faite au-delà de 600 000 faces (3 hauts de vie à K = 5) et le budget de temps (trame entière à K = 5) |
| P12 budget LiDAR | **confirmée** (extrapolation, § 7) | A_5 entier d'une trame ≈ 48 M faces, 62–82 fois les éléments de la tour |

## 7. Budget LiDAR (extrapolation déclarée)

Faits mesurés ici : la tour (mhgp11, 2 fils, codespace) coûte 0,2 ms par site à K = 5 et 0,03 ms à K = 2 ; les reçus
G4 de la v11 la donnent autour de 100 ms par trame de 30 000 à 60 000 sites à K = 5. La mosaïque d'ordre 5 coûte en
Python 38 à 75 ms par site (180 à 375 fois la tour sur la même machine) et compte 62 à 82 fois plus d'éléments que la
tour (1 003 à 1 086 faces par site contre 13,3 à 16,1 nœuds) ; à K = 2, 28 à 30 fois (132 à 138 contre 4,5 à 4,8).

Extrapolation, sous l'hypothèse **H** (non vérifiée) qu'un constructeur natif atteigne le même coût par élément produit
que la tour sur G4 (environ 165 ns de temps mural par nœud à K = 5) :

- **A_5 de toute la trame** (représentation par événements, 48 M faces) : 6 à 8 s, soit 62 à 82 fois le budget de
  100 ms ; doublé si l'on compte les ordres 1 à 4 que la construction itérative exige (environ 2 200 faces par site
  au total) ; mémoire de l'ordre du gigaoctet à 16–32 octets par face ;
- **A_2 de toute la trame** (6,2 M faces) : de l'ordre de 1 s, environ 10 fois le budget ;
- **un objet** : la voiture à K = 5 (558 462 faces, et 1,08 M au haut de vie) coûte à elle seule l'équivalent de la
  tour entière (0,61 M nœuds) ; à K = 2 (76 523 faces), environ un dixième ;
- **la réduction L** ne diminue pas ce coût (il faut A pour la certifier ; 15 à 25 µs par face en Python) et ne
  diminue pas le dessin (faces exposées) ;
- **une coupe par nœud stockée séparément** coûte la profondeur de l'arbre fois la représentation par événements :
  profondeur moyenne 1 070 à 2 060 (K = 2) et 4 830 à 10 000 (K = 5, croissante avec n sur KITTI) ; il faut donc
  stocker chaque cellule une fois (date, nœud d'activation) et obtenir une coupe par requête de sous-arbre.

Conclusion de budget : le complexe exact, même réduit, n'entre pas dans 100 ms pour toute la hiérarchie ; il peut
accompagner, en aval et à la demande, quelques jetons à K = 2 (dizaines de millisecondes chacun sous H) ; à K = 5, un
seul grand objet coûte déjà le budget de la tour. Rien de cela n'est mesuré sur G4 ni en natif.

## 8. Ce qu'il faudrait exporter de FULL pour un calcul local

Pour construire A_{k,v}(r) localement, sans mosaïque globale ni catalogue de k-parties, il faut, par nœud v :

1. **ses niveaux exacts** b_v et d_v (rationnels ; d_v = niveau de la première boule du parent, déjà dans MHGP11SP) ;
2. **une désignation de la composante** : le k-ensemble d'un sommet de naissance de son sous-arbre (population
   I_b ∪ U_b d'une boule de naissance : k indices de sites). MHGP11SP ne publie que S* et (p, m) : la population est ici
   recalculée par arbre k-d (`harnais.populations`, décisions entières dans la bande) ; l'exporter (k entiers par
   naissance, ou par nœud) supprimerait ce recalcul ; à K = 1, `leaf_site` suffit ;
3. **les points couverts à d_v^-** (étiquettes, P ∩ (C ⊕ B_r)) : ils donnent directement Y = P ∩ (étiquettes ⊕ B_{2 d_v})
   et le certificat de localité sans itération ; la pendaison de MHGP11PT (propriétaire et date de chaque point dans
   l'arbre d'ordre K) devrait les fournir — à vérifier, non utilisé ici ;
4. **le nuage** (sites et index spatial) : la mosaïque locale d'ordre k se construit sur Y par les ordres 1 à k ;
   c'est elle qui coûte, et FULL ne la contient pas ;
5. pour les contrôles : les boules de naissance du sous-arbre (rattachement `noeud_de`, rôles) et le postordre de
   l'arbre (appartenance au sous-arbre).

Rien d'autre : ni la mosaïque d'ordre supérieur globale, ni un tableau par paire. Pour la réduction certifiée au-delà
de d_v, il faut le complexe du parent (sa propre localité) : la réduction locale d'un nœud n'est pas réutilisable telle
quelle par ses ancêtres.

## 9. Échecs, limites, ce qui n'est pas établi

- **Petits échantillons de nœuds** : 3 nœuds uniformes et au plus 2 objets par (taille, K) dans la campagne d'échelle,
  plus les objets forcés et les scènes (417 mesures au total, dont des doublons volontaires pour la localité) ; les
  médianes sont indicatives, la distribution des tailles est à queue lourde.
- **Pas de juge global de |A_k(r)| à niveau fixé** (somme sur tous les nœuds vivants) : seules la densité de la
  représentation par événements (toutes les cellules) et les tailles par nœud sont mesurées.
- **Certificat de réduction local** : valable pour r < d_v seulement ; sur la vie du parent, la suite de v cesse d'être
  exécutable dans 47 cas sur 57 (0,8 % des paires reçoivent une coface nouvelle ; une première mesure, fausse, comptait
  aussi les cofaces déjà retirées par la suite de v et annonçait 81 % : corrigée), alors que la réduction du parent
  restreinte à v ne coûte que 0 à 3 % de plus (§ 2) ; mesuré sur 57 couples (scènes, et une voiture KITTI à K = 2 et 5)
  ; trois composantes de plus de 600 000 faces non réduites.
- **Temps** : Python sur un codespace partagé (charge de 7 à 15) ; la même construction a varié d'un facteur 2,8 ; pas
  de G4, pas de natif ; l'extrapolation du § 7 repose sur l'hypothèse H.
- **Jeux** : les sous-ensembles KITTI sont centrés sur le capteur (densité décroissante avec la portée) et ne
  contiennent qu'un à trois objets ; le sous-ensemble de 8 000 sites tronque la voiture ; la famille synthétique
  est une translation de blocs (localité triviale entre blocs, par construction) ; la trame entière n'a pas été
  mesurée à K = 5 (budget de temps).
- **Nœuds des meilleurs objets** : vie très courte, donc trois niveaux presque identiques ; les niveaux le long de la
  hiérarchie sont couverts par les chaînes partie → objet → haut de vie, pas par la vie d'un seul nœud.
- **K = 1** : la racine n'est pas mesurée (d_v infini ; borne finie non déclarée) ; les objets uniques des scènes à
  K = 1 sont la racine.
- **Égalité exacte des naissances** : décidée par égalité des flottants correctement arrondis puis même face réalisant
  le minimum ou `Fraction` ; elle s'appuie sur les niveaux propres de `mosaique.py` (validés contre l'oracle borné
  dans le travail précédent, pas re-validés ici au-delà du témoin K = 1).
- **Rendu** : registre des sommets par arêtes de longueur nulle pour fixer la caméra (contournement, pas une option du
  harnais) ; police par défaut corrigée pour les en-têtes de planches.
- **Erreurs trouvées et corrigées dans mon code** : correspondance feuille → site à K = 1 (FULL en échec, à raison) ;
  rendu interrompu par un tableau irrégulier (nœuds refaits) ; nœuds de moins de k + 4 points (Y complété). Aucune
  contradiction mathématique : aucune fixture nouvelle.
- **Disque** : environ 85 Mo écrits (sorties mhgp11 des sous-ensembles : 75 Mo, à retirer après lecture si besoin).

## Fichiers

- `PREDICTIONS.md`, `PREDICTIONS.sha256` : prédictions scellées ;
- `jeux_echelle.py` (jeux, tours, lecteur sans cache), `mesure_locale.py` (construction, contrôle strict, niveaux
  exacts, composantes, réduction, vérificateur), `campagne.py` (nœuds d'échelle, rendus), `campagne_scenes.py`
  (jeux communs, témoin gudhi), `temoin_k1.py`, `verif_k1_gudhi.py`, `limite_locale.py` et `limite_locale_k5.py`
  (limite du certificat local), `profondeur.py`, `synthese.py`, `tables.py`, `planches.py`, `remplir_readme.py`
  (ce fichier, depuis `README.gabarit.md` et `textes/`) ; chaînes `chaine*.sh` ;
- `resultats/` : `tours.json`, `mesures_*.jsonl` (une ligne par nœud), `synthese.json`, `profondeur.json`,
  `verif_k1_gudhi.txt`, `limite_locale.jsonl`, `tables.md` ; `journaux/` ; `png/` (vues et planches) ; `entrees/`,
  `sorties/` (entrées des sous-ensembles et, des sorties mhgp11, les manifestes et lignes d'état seulement : les
  fichiers `supports.mhgp11sp` ont été retirés après les mesures pour rendre 75 Mo au disque partagé ; ils se
  reproduisent à l'identique par `python jeux_echelle.py`, empreintes dans les manifestes) ; aucune donnée
  SemanticKITTI hors de `build/`.
