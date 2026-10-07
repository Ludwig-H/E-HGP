
## 8. Constats

Gravité : **bloquant** = rend faux ou invalide un résultat ou un contrat ; **majeur** = à traiter dans la conception de la v11 ; **mineur** ; **info**. Aucun bloquant dans cette lentille.

### L02_MATH_TOUR-01 — majeur — Le théorème de la tour est vrai mais n'est écrit nulle part ; le registre des preuves n'a aucune ligne v10

- **Fait.** L'énoncé « naissance ou jonction de morceaux locaux, représentants rattachés par descente » figure dans `SPEC_V10.md:49-63`, `tower.hpp:3-14` et la docstring de `reference/hgp10_ref.py:11-18`, sans preuve. Les preuves sont des esquisses d'une ligne dans une conception **hors dépôt** (`build/v10-persist/design/TOWER_v1.md:632-648`), reprises par renvoi dans `TOWER_v2.md:779-780` (« à rédiger en note avant le code correspondant »). `CONCEPTION_V10.md:807-809` exige l'inscription au registre « avant le code qui en dépend » (lignes V10-R01 à V10-R39) ; `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (1 320 lignes) ne contient aucune occurrence de « v10 ». La preuve de `TOWER_v2` § 9.1 porte sur un résolveur (saut depuis le centre dans tous les cas) qui n'est pas celui du code.
- **Preuve.** `grep -n "v10\|V10" docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` : aucune ligne ; lignes citées. L'audit géant l'avait relevé (RG1) ; contrôlé ici indépendamment.
- **Vérification.** Lu.
- **Conséquence v11.** Écrire la note mathématique avant le code : le § 4 de ce rapport en est une rédaction complète (théorèmes A à J, obligations restantes), à faire contre-lire par un tiers, puis à inscrire au registre avec ses fixtures.

### L02_MATH_TOUR-02 — majeur — La conception normative `TOWER_v2` n'a pas été implémentée ; aucun document ne dit ce qui en reste normatif

- **Fait.** `CONCEPTION_V10.md:61` déclare la conception « normative pour la suite » et prévoit (l. 104, § 9.4) la « réécriture de la tour selon TOWER_v2 ». L'historique de `src/tower/` ne contient que la tour d'ouverture (`6b4b4ccee`, `9af04985c`) et son accélération (`025fb7782`). Sont absents du code : l'index $(\pi, J)$ et la requête `WA`, la règle cartésienne, les ancres par cellule, les contributions, la numérotation canonique et les digests, Euler (I2), le refus d'une sphère admissible absente (I5), les quotients circulaire et pondéré, les multiplicités, les modes `full` et `sampled`, les juges J-CENSUS, J-DESC, J-MF, les 24 mutants, les fixtures épinglées, le différentiel v9. Des huit portes prévues pour la tour (`CONCEPTION_V10.md:888-899`), une seule existe, à 73 exécutions au lieu de 5 000 nuages.
- **Preuve.** `git log -- morsehgp3D_v10/src/tower/` (13 commits) ; `grep -rn "jblock\|leaf_lo\|tower_digest\|merkle" src/tower` : rien ; `CMakeLists.txt:66-135` (13 portes, dont une pour la forêt).
- **Vérification.** Lu.
- **Nuance.** `SPEC_V10.md` et `PASSATION.md` décrivent fidèlement le code livré ; c'est la conception qui est restée en arrière.
- **Conséquence v11.** Un seul document d'objet, tenu égal au code. Reprendre de `TOWER_v2` ce qui est prouvé et utile (règle cartésienne, Euler, juges, mutants par copies) et l'écrire comme obligations de la v11, pas comme héritage implicite.

### L02_MATH_TOUR-03 — majeur — La porte `mhgp10_tower_oracle` n'établit que le nombre de composantes et la partition $C \cap X$ : cinq mutants sur six la traversent

- **Fait.** § 6.1 et 6.3. La porte ne compare ni les parents, ni l'arité, ni les populations des naissances ; elle relève elle-même les nœuds publiés avant de comparer (`top()`), ce qui masque toute erreur de hauteur.
- **Preuve.** `tests/oracle/test_tower_oracle.py:83-87, 136-146, 153` ; journaux `mutants_porte_du_depot/*.log` (cinq fois `tower_oracle_checks 73 fails 0 cuts 23444`, code 0) ; journaux du juge L02 sur les mêmes mutants.
- **Vérification.** Lu, exécuté. Recoupe le constat JG1 de l'audit géant (non contre-vérifié à l'époque) et le constat 03 du rapport L06, établis séparément.
- **Conséquence v11.** La porte d'oracle de la v11 doit comparer l'arbre étiqueté (contrôles B, M, V, P du § 6.2), avec des planchers par propriété (fusions à trois parents ou plus, niveaux à plusieurs événements, coquilles étendues par taille, ordres jusqu'à $K_{\max}$) et des mutants tués par construction.

### L02_MATH_TOUR-04 — info — La forêt C++ est l'arbre de fusion exact de $\Gamma_k$ sur toutes les entrées jugées, verticales comprises

- **Fait.** @@TOT_CLOUDS@@ nuages et 15 fixtures, 0 écart sur @@TOT_BIRTHS@@ naissances, @@TOT_MERGES@@ fusions, @@TOT_VERT@@ verticales, @@TOT_PTS@@ attaches, @@TOT_EULER@@ identités d'Euler (§ 6.2). Coquilles étendues jusqu'à 12 sites, ordres jusqu'à 10, fixtures dégénérées comprises.
- **Preuve.** `campagnes/*.log`, `fixtures.log`.
- **Vérification.** Exécuté (oracle borné, donc établissant la vérité sur ces entrées).
- **Limite.** Descentes courtes à ces tailles (§ 6.2).
- **Conséquence v11.** La sémantique de la v10 se porte telle quelle ; le juge L02 peut servir de point de départ à la porte T2 de la v11 et d'ancre pour la campagne appariée v10/v11.

### L02_MATH_TOUR-05 — majeur — Aucun invariant global d'échelle n'est une porte, alors qu'Euler et l'ordre 1 contre l'EMST tiennent partout où ils ont été mesurés

- **Fait.** § 7. Euler vrai aux ordres 1 à 5 sur 18 entrées (trois trames, quinze synthétiques de 8 000 à 32 000 points) et aux ordres 1 à 10 sur une trame ; ordre 1 égal à l'arbre couvrant minimal de scikit-learn, structure N-aire comprise, sur les 18 entrées ; invariants linéaires (A), (C), (D) à 0 violation ; ils tuent les cinq mutants survivants.
- **Preuve.** `lidar/*.json`, `echelle/*.json`, `neutralite/*`.
- **Vérification.** Mesuré.
- **Conséquence v11.** Faire d'Euler (catalogue à admission $p \leq K - 1$, $\times 1{,}48$ en boules sur 08/000200), de l'ordre 1 contre l'EMST (structure, pas seulement les poids) et de (A), (C), (D) des portes d'échelle aux tailles 8 000, 16 000, 32 000 et sur les trames ; (C) et (D) coûtent un parcours des nœuds et peuvent être des invariants de produit.

### L02_MATH_TOUR-06 — majeur — « Continuations datées » et K-polyèdres annoncés, non calculés

- **Fait.** `README.md:21-23` et `CONCEPTION_V10.md` § 2.1 placent les « continuations datées » dans l'objet. Le code classe `inert` toute cellule à un seul morceau (`tower.cpp:631`) et ne publie ni contribution ni couverture. Hors régularité, une composante peut gagner un point sans fusion (proposition G, fixture à quatre points) : les K-polyèdres de la thèse ne sont pas reconstructibles depuis la seule forêt et les populations des naissances.
- **Preuve.** `grep -rn -i "contrib\|continuation" src cli` : un commentaire sans rapport ; fixture `continuation_avec_gain_de_couverture` (ordre 3, niveau 25) ; compteur « gain de couverture sans fusion » du juge : @@GROWTH@@ occurrences dans les campagnes.
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Décider de l'objet avant d'écrire : soit la tour est l'arbre abstrait de $\pi_0$ plus la relation « boule couvrante $\to$ nœud » (ce que fait la v10, à dire ainsi), soit elle porte les contributions datées. Ne plus annoncer ce qui n'est pas produit.

### L02_MATH_TOUR-07 — majeur — Le Théorème 5 du manuscrit est faux en général ; la v11 ne doit pas se rabattre sur le K-graphe de Gabriel

- **Fait.** § 4.11 : contre-exemple exact à cinq points du plan en position générale, $K = 2$ ; désaccord aussi sur E5.
- **Preuve.** `l02_these_th5.py` et `these_th5.log`.
- **Vérification.** Exécuté (arithmétique exacte), relu à la main pour l'exemple plan.
- **Conséquence v11.** La réduction correcte est celle de la v10 (toutes les sphères de la fenêtre de rang, rattachement de chaque représentant par descente). Graver le contre-exemple en fixture permanente et inscrire la ligne au registre ; le signaler à l'auteur de la thèse (énoncé à corriger : il faut rattacher les facettes nées dans une coface non-Gabriel). Toute implémentation de l'Algorithme 1 en hérite : aucune ne peut servir d'oracle de hiérarchie (HGP-old n'a pas été contrôlé ici).

### L02_MATH_TOUR-08 — majeur — L'exactitude est relative au catalogue, et rien dans le produit ne témoigne de sa complétude

- **Fait.** Quand la boule minimale d'une partie est une sphère en fenêtre **absente** du catalogue, la descente continue avec un représentant calculé à la volée (`tower.cpp:951`, puis 978-989) ; seul le cas « naissance absente » est refusé (`census_mismatch`). Le contrôle I5 de la conception n'existe pas, Euler non plus, et il n'y a pas de statut `catalogue_incomplete`. Une sphère de jonction manquante retarde ou supprime une fusion sans refus tant qu'il reste une racine.
- **Mesure.** Catalogues amputés (§ 7.5) : sur 3 062 retraits d'une boule admissible, 295 forêts fausses publiées sans refus (9,6 %) ; Euler les détecte toutes. Mutant `m_onepiece` : sur 64 nuages, 44 refus `root_count` et 6 forêts fausses publiées avec une seule racine.
- **Preuve.** Lignes citées ; `src/core/reasons.def` (aucune raison de complétude) ; `amputation.log` ; `mutants_juge_l02.log`.
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Publier « exact relativement au catalogue » tant qu'un témoin indépendant n'a pas tourné ; prévoir un mode certifié (Euler) et le refus d'une sphère en fenêtre absente. Sur les 18 entrées d'échelle de cet audit, Euler est vrai : rien n'indique un catalogue incomplet aujourd'hui.

### L02_MATH_TOUR-09 — mineur — Extension non régulière par énumération à budgets ; multiplicités refusées ; quotients de la conception absents

- **Fait.** Coquille étendue : énumération de toutes les parties de taille $t$, puis test par paires en $O(s^{2})$ sur les $s$ parties séparables (`tower.cpp:581-631`) ; refus au-delà de 24 sites ou de 20 000 parties séparables. Ce sont des budgets, pas des bornes de l'objet. Sur les trois trames, $m \leq 5$ et 204 à 865 boules étendues sur 2,1 à 2,7 millions : aucun effet mesuré. Une entrée à sites répétés est refusée.
- **Preuve.** Lignes citées ; `lidar/*_k5_kcat7.json` (`ext_m_hist`).
- **Vérification.** Lu, mesuré.
- **Conséquence v11.** Garder l'énumération tant que $m$ reste petit sur les entrées du contrat, mais publier $m$ maximal et refuser par boule ; décider du sort des doublons avant le contrat « trames entières » (question ouverte 1).

### L02_MATH_TOUR-10 — mineur — « Dumps identiques au binaire figé » et « 1 fil = 4 fils » : journal privé, pas des portes ; rejoués et confirmés

- **Fait.** § 6.1. Rejoué ici : même empreinte que le journal du 29 septembre pour la trame 01 à $K = 5$ ; identique à 1, 2 et 4 fils ; identique pour deux variantes neutres de la descente.
- **Preuve.** `neutralite/lidar01_k5_hashes.txt` ; `build/v10-persist/g4/verify_tower.log` (piste).
- **Vérification.** Exécuté.
- **Conséquence v11.** Porte de déterminisme sur le dump de la tour aux tailles d'intérêt ; porte de neutralité (deux règles de descente valides), qui est le seul contrôle à l'échelle du régime des descentes longues.

### L02_MATH_TOUR-11 — info — La règle cartésienne de `TOWER_v2` (PO-T18) est correcte et disponible

- **Fait.** § 4.12.
- **Preuve.** `l02_cartesian_rule.py` : 2 000 essais, 0 écart.
- **Vérification.** Exécuté ; preuve de la conception relue.
- **Conséquence v11.** Option sûre si le Kruskal par lots séquentiel devient le plafond du contrat de temps.

### L02_MATH_TOUR-12 — mineur — La référence Python ne modélise pas les verticales et reste hors CTest

- **Fait.** `reference/hgp10_ref.py` n'a aucune fonction verticale ; `reference/test_ref.py` s'arrête à 8 points et $K = 4$ ; il n'est pas enregistré dans `CMakeLists.txt` (lancé à la main ; « ≈ 6 min » selon `README.md:75`). Rejoué ici au HEAD : @@TESTREF@@.
- **Preuve.** Fichiers cités.
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Une référence qui porte tout l'objet (verticales, attaches), enregistrée comme porte.

### L02_MATH_TOUR-13 — mineur — `mhgp10_tower --no-points --dump=…` meurt par signal

- **Fait.** Le dump lit `point_node[s]` même quand les attaches sont désactivées (`cli/mhgp10_tower.cpp:186-194`) : SIGSEGV sur toute entrée.
- **Preuve.** `cli_no_points_dump.log` (quatre points, code de retour 139) ; rencontré d'abord en lançant le juge de l'ordre 1 sur les trois trames.
- **Vérification.** Exécuté. Recoupe le constat CL1 de l'audit géant.
- **Conséquence v11.** Les sondes sont des portes : sorties validées, aucun signal.

## 9. Ce qui est solide et mérite un port explicite en v11

1. **La sémantique des cellules** : fenêtre $[\max(1, p + q - 1), \min(K, p + m)]$, admission $p + q \leq K + 1$, naissance si aucune partie de taille $t$ n'est séparable, jonction des morceaux sinon, règle analytique des coquilles régulières (théorème B). Jugée exacte ici.
2. **La descente comme fonction pure**, avec sa garde de décroissance stricte, et le fait prouvé que la sortie ne dépend d'aucun choix valide (théorème D) : la règle du saut et du représentant peut être choisie pour son coût.
3. **Les plateaux atomiques** (racines pré-lot figées, un nœud par groupe ; théorème C).
4. **Les verticales à la coupe fermée**, par descente d'une $(k-1)$-partie de la boule de naissance, avec contrôle de naturalité sur tous les enfants (théorème F).
5. **Le critère de Gordan exact** `center_in_closed_hull` et le support canonique (`src/catalogue/support.hpp`), sans flottant.
6. **Les invariants de produit existants** : une racine par ordre, décroissance stricte, naturalité, recensement échantillonné.
7. **Les fixtures** : E5, carré, cube, et celles de ce rapport (contre-exemple du Th. 5, gain de couverture, cinq points de `BALL_ANCHORS`, cercle de 12 points, coins u18).
8. **Le juge par arbre étiqueté** et **les invariants d'échelle** de cet audit, comme base des portes.
9. **Les nombres d'ancrage** : 1 407 885 boules à $K = 5$ sur 08/000200 ; naissances par ordre du § 7.2 ; empreinte `6ebb1eb4…` du dump de 08/000100 à $K = 5$.

## 10. Ce qu'il ne faut pas refaire

1. Juger une forêt par des **comptes** et des partitions relevées par le juge lui-même.
2. Annoncer comme acquis une propriété sans porte (« jamais binarisées », « plateaux atomiques », « image à la coupe fermée »).
3. Laisser une conception « normative » diverger du code sans le dire ; laisser des preuves à l'état d'esquisses « à rédiger avant le code ».
4. Décrire dans l'objet ce qui n'est pas produit (continuations datées).
5. Prendre « sorties identiques à une version antérieure » pour une preuve d'exactitude, ou garder une telle vérification dans un journal privé.
6. Revenir au K-arbre couvrant du graphe de Gabriel (Th. 5), ou à tout repli qui ignore les facettes rattachées silencieusement.
7. Traiter un budget d'énumération comme une frontière de l'objet sans le publier boule par boule.
8. Lancer une réécriture pour la performance avant d'avoir figé les juges qui diront si elle calcule le même objet.

## 11. Questions ouvertes

1. **Doublons.** Les trames du contrat lues ici n'en ont pas ($n$ = nombre de sites) ; qu'en est-il des trames avec sol et des autres séquences ? Sémantique pondérée (multiensembles) ou dédoublonnage déclaré ?
2. **Objet publié.** Arbre abstrait plus relation de couverture, ou contributions datées donnant les K-polyèdres exacts ?
3. **Complétude.** Euler en porte seulement, ou mode certifié du produit (coût : catalogue $\times 1{,}48$ à $K = 5$) ?
4. **Juge indépendant à l'échelle pour $K \geq 2$.** Il n'en existe pas : Euler juge le catalogue, la neutralité juge la cohérence des descentes. Un juge de descente indépendant (échange d'intrus, prévu par `TOWER_v2` § 13.4) ou un différentiel avec une seconde implémentation est-il exigé avant de revendiquer l'exactitude à l'échelle ?
5. **Coquilles étendues.** Quelle borne de $m$ sur les entrées réelles (mesuré : 5) ; faut-il le quotient par arrangement de grands cercles, ou l'énumération suffit-elle avec un refus par boule ?
6. **Longueur des descentes.** Aucune borne prouvée ; faut-il un plafond de pas publié ?
7. **Rangs exacts vers la tête.** `point_dendrogram` publie des niveaux en double et fusionne les rangs de niveaux exacts distincts dont les doubles coïncident (`tower.cpp:1828-1840`) : la v11 transmet-elle les rangs exacts ? (Frontière avec la lentille des points.)
8. **Thèse.** L'auteur souhaite-t-il corriger l'énoncé du Th. 5 (rattachement des facettes nées dans une coface non-Gabriel) ?

## 12. Recommandations pour la v11

1. **Note mathématique d'abord.** Reprendre le § 4 (définitions, lemmes 1 à 4, théorèmes A à J), le faire contre-lire, l'inscrire au registre avec les fixtures de l'annexe B. Statut de départ proposé : A `theorem_external` ; B à F `proved_here` après contre-lecture ; H `proved_here` ; I `false_in_general` ; J `proved_here`.
2. **Objet unique et canonique.** Par ordre : naissances (boule, population), fusions N-aires, niveau exact, image verticale à la coupe fermée ; identité canonique indépendante de l'ordre des boules et du nombre de fils ; digest.
3. **Porte d'oracle T2 par arbre étiqueté**, bornée à 14 points, avec planchers : fusions à trois parents ou plus, niveaux à plusieurs événements, coquilles étendues par taille jusqu'à 12, naissances de population supérieure à $k$, ordres 1 à $K_{\max}$, familles dégénérées, et les six mutants de cet audit tués.
4. **Portes d'échelle** (8 000, 16 000, 32 000 et trames) : Euler ; ordre 1 contre l'EMST, structure N-aire ; (A), (C), (D) ; neutralité de deux règles de descente ; déterminisme du dump en fils ; préfixe ($K = 5$ contre $K = 10$).
5. **Statuts honnêtes.** `exact_relative_to_catalogue` par défaut ; mode certifié nommé quand Euler a tourné ; refus typé d'une sphère en fenêtre absente.
6. **Liberté de conception prouvée.** Le choix du saut, du représentant et du découpage en morceaux (tout découpage plus fin que les morceaux convient) est libre : l'utiliser pour le coût, sous la porte de neutralité.
7. **Ne rien promettre sur les doublons, les couvertures et les coquilles larges avant d'avoir tranché les questions 1, 2 et 5.**

## Annexe A — Reproduction

Dossier de calcul : `/tmp/v11-audit/l02_math_tour/`. Sources de la v10 copiées dans `v10src/` depuis `build/v11-worktree/morsehgp3D_v10` (HEAD `afb081774`).

```text
cmake -S v10src -B build_ref -DCMAKE_BUILD_TYPE=Release && cmake --build build_ref -j3
python3 -B v10src/tests/oracle/test_tower_oracle.py build_ref                 # porte du dépôt : 73 exécutions, 0 écart
g++ -std=c++20 -O2 -I v10src/src tools/l02_dump.cpp build_ref/libmhgp10_core.a -lpthread -o tools/l02_dump
python3 -B tools/l02_judge.py tools/l02_dump 11 240 generic,grid,plane,clusters,sphere,circle,line,cube 7-11 5
python3 -B tools/l02_judge.py tools/l02_dump 21 200 generic,grid,plane,clusters,sphere,circle,line,cube 10-12 8
python3 -B tools/l02_fixtures.py tools/l02_dump
python3 -B tools/l02_amputation.py tools/l02_dump 64 4                      # catalogues amputés
python3 -B tools/make_mutants.py v10src .      # puis cmake de chaque copie, porte du dépôt et juge sur chacune
tools/l02_dump TRAME.u32le --k=5 --kcat=7 --threads=4 --no-points          # Euler + statistiques
tools/l02_dump TRAME.u32le --k=5 --kcat=5 --threads=2 --coherence --no-euler # invariants (A) à (D)
python3 -B tools/l02_emst_k1.py build_ref TRAME.u32le 2                     # ordre 1 contre scikit-learn
build_ref/mhgp10_tower TRAME.u32le --k=5 --threads=W --dump=F ; sha256sum F  # neutralité, fils
python3 -B tools/l02_these_th5.py ; python3 -B tools/l02_cartesian_rule.py 2000
```

## Annexe B — Pièces déposées (`build/v11-persist/audit_v10/preuves_l02_math_tour/`)

- `outils/` : `l02_dump.cpp`, `l02_judge.py`, `l02_fixtures.py`, `l02_coverage.py`, `l02_amputation.py`, `l02_emst_k1.py`, `l02_these_th5.py`, `l02_cartesian_rule.py`, `make_mutants.py`, `lint.py`, scripts de lancement.
- `porte_du_depot_head.log` : la porte `mhgp10_tower_oracle` rejouée au HEAD.
- `campagnes/` : journaux du juge L02 (graine 11 ; graines 21 et 22), couverture.
- `fixtures.log` : les 15 fixtures gravées.
- `mutants_porte_du_depot/` : sortie de la porte du dépôt sur chaque mutant ; `mutants_juge_l02.log` : verdicts du juge ; `mutants_invariants_echelle.txt`.
- `lidar/` : Euler et statistiques ($K = 5$, catalogue à 7 ; $K = 10$, catalogue à 12), invariants (A) à (D), ordre 1 contre l'EMST, comptes du catalogue par $(q, p)$. Aucun octet de nuage.
- `echelle/` : les 15 entrées synthétiques (Euler, ordre 1).
- `neutralite/` : empreintes des dumps (fils, variantes) et grands-livres des trois builds.
- `amputation.log` : catalogues amputés.
- `these_th5.log`, `regle_cartesienne.log`, `test_ref_head.log`, `cli_no_points_dump.log`.
