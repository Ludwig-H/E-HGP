# Portes et tests v8 : ce qui mord, ce qui ne mord pas encore

14 septembre 2026, après **85015a8c**, complété après **e3af11a7**. Auditeur indépendant B.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Résultat d'ensemble : **aucun défaut du produit** n'a été trouvé par la
campagne adversariale (huit dimensions, harnais C++ contre la bibliothèque
publiée et juges Python en entiers exacts, mutants compilés hors dépôt).
Les constats ci-dessous portent sur la couverture des portes et sur leur
déterminisme, avec les fixtures exactes qui les fermeraient. Les harnais
des agents sont hors dépôt ; chaque entrée ci-dessous est reproductible à
la main avec les coordonnées données.

## 1. Ce qui est confirmé par exécution

| Objet | Contrôle exécuté | Résultat |
| --- | --- | --- |
| Builds neufs | Release GCC 13.3 et Clang 18 ASan/UBSan à 1bf806f0 ; Release à 85015a8c | 34/34, 34/34 ; 37 tests dont un échec intermittent (§2.7) |
| Prédicats et bornes de blocs | 5 006 triplets de boîtes u16 (petites, pleine plage, extrêmes 0/65535, sommets demi-entiers, égalités 3H²=Ξ et 2H²=Ξ mises à l'échelle), 512 coins × 3 lanes par cas, comparés à une réimplémentation exacte indépendante | 0 écart ; `h_minimum`, `h_maximum_times_four`, `xi_bounds`, `Q2PreparedBounds` exacts ou sûrs |
| Crédits Pool/DualBlocks/Tubes | 1 728 plans (nappes, rails, grilles à 60 000, facteurs jusqu'à 790 sites, Kmax 1–10, s 8–12) contre un juge brute force i128 | DualBlocks exact saturé ; Pool et Tubes jamais surcrédités ; 327 309 paires tuées ont toutes ≥ h_q témoins distincts |
| Tubes à la frontière D = 10R | 1 151 rectangles dont 997 à égalité `d² = 100·diam²`, 297 444 ancres | crédit ≤ compte exact partout, marge minimale 0 (contrôle non vacant) |
| Filtre axial et census q2 | ≈8 300 petits nuages adversariaux (colinéaires, cosphériques, grilles, colonnes longues, extrêmes), 8 variantes de plan × 2 modes de census, ≈118 000 paires | ensembles de paires, intérieurs, coquilles et clés identiques à la force brute ; partition des blocs exacte |
| Pleine échelle u16 | 1 357 rectangles aléatoires sur tout le cube, 144 711 rejets locaux, 112 930 rejets de préfiltre, 60 672 supports | 0 désaccord avec un oracle Python entier |
| Mutants tués par les portes | `h<=0`→`h<0` (point_witness), `Δ>0`→`Δ>=0` (tubes), coquille oubliée (collect), saturation un cran trop tôt (census), curseur réinitialisé à la racine, égalité au bord du slab rejetée, bornes préparées faussées | tous tués par au moins une porte C++ enregistrée |

### 1 bis. Chaîne front + census q2 (tranches 8 à 10, e3af11a7) : vérifiée bout en bout

Le point d'entrée `run_wspd_q2_census` a été confronté à une force brute
exacte sur tous les sites, pour huit combinaisons de modes (front Pure ou
MidpointSamples, census Pairwise ou SharedBlocks, frère Disabled ou
Saturating, ordre GlobalDfs ou ComplementFirst) : toute paire non
ordonnée à moins de Kmax intérieurs stricts doit être émise exactement
une fois, avec ses IDs intérieurs, toute sa coquille (extrémités comprises)
et sa clé ; aucune autre paire ne doit l'être. Familles : sept nuages
adversariaux (grille 5³, deux sphères cosphériques concentriques de
rayons 5 et 13 plus centre et voisins, colinéaires avec extrêmes u16,
coins d'un cube avec doublons de centre, extrêmes 0/65535 et milieu
demi-entier, coordonnées impaires, boule dense) à Kmax 1/2/5/10 et
s 8/12, puis uniforme, terrain, amas et rangées à n = 800 (Kmax 1/3/10,
s 8/12, deux graines) et n = 2 000 (uniforme, amas, Kmax 10).
Résultat : **86 exécutions × 8 combinaisons, 104 736 960 paires contrôlées,
3 359 624 paires vivantes toutes émises, 0 désaccord** (intérieurs, coquilles
et clés identiques ; aucune émission en double ni en trop). Les modes
optionnels ont bien travaillé : le certificat du frère a rejeté 4 212 717
paires sur 133 exécutions et l'ordre complément a changé de phase
1 421 827 fois sur 258. À n = 2 000 : 515 712 supports uniformes et
412 928 supports d'amas, tous exacts.
Reçu et harnais : [chaine_q2_20260914/](chaine_q2_20260914/CHAINE_Q2_CHECKS.json),
compilés contre une extraction de e3af11a7 ; ce contrôle porte sur
l'exactitude et la complétude du flux de supports q2, pas sur les temps
ni sur la tour.

## 2. Lacunes de couverture, avec les fixtures qui les ferment

### 2.1 Égalité W3/W4 créditée : mutant survivant à `mhgp8_p0_gate`

Le mutant `spindle_detail::square(h_min) >= xi.high` en
`src/spindle/predicates.hpp` (Credit sur l'égalité du fuseau) passe la porte
entière (148 587 contrôles, 390 plans). C'est la seule inégalité qui protège
« un point du bord n'est jamais témoin » au niveau des blocs ; elle n'est
exercée que sur des boîtes dégénérées. Fixtures d'égalité à ajouter dans
`block_gate` avec l'exigence explicite `NoCredit` pour la lane concernée
(et `Credit` pour q2) :

- q3 : A = {(0,0,0)}, B = {(1,2,1)}, Z = {(1,1,0)} (3H² = Ξ = 3) ;
- q4 : A = {(0,0,0)}, B = {(2,1,1)}, Z = {(1,0,0)} (2H² = Ξ = 2) ;
- les mêmes multipliées par 32 767.

La sonde `predicates_probe.cpp` de l'auditeur complémentaire tue ce mutant
mais n'est pas un CTest.

### 2.2 Fixture « maximum en z = 1,5 » non décisive

La fixture de `block_gate` commentée « the maximum in z is at 1.5 »
(A = {0}, B = {3}, Z = [1,2]) donne `h_max4 = 9` avec le code et 8 avec un
mutant qui remplace le sommet clampé par l'extrémité haute : décision
identique, mutant survivant. Fixture décisive : A = {(0,0,0)}, B = {(3,0,0)},
Z = [0,2]×[0,3]×{0}, attendu `h_max4 = 9` et décision `Uncertain` pour les
trois lanes ; le mutant rend −28 et `NoCredit`, alors que z = (1,0,0) est
témoin universel de a. Le contrat de `classify_witness_block` serait violé
sans que la porte le voie (sous-crédit conservateur, pas de paire perdue).

### 2.3 Tubes : aucune fixture à la frontière exacte avec crédits non nuls

`tube_gate` confronte bien chaque crédit à l'histogramme exact, mais ses
fixtures sont loin de la frontière (rapports `d²/(100·diam²)` de 3,9 à
74 074, ou 0,27 pour le repli). Fixture gravable, calculée avec la
bibliothèque à 85015a8c : A = {(7,11,13), (10,11,13), (7,15,13), (10,15,13),
(8,11,13), (10,13,13)}, B = A + (15,20,0), Kmax = 10, s = 1 ; alors
`d² = 100·diam² = 2 500` exactement, aucun repli, et les crédits sont
q2 A = [3,2,0,0,3,1], B = [0,1,1,2,0,2] ; q3 A = [2,1,0,0,2,0],
B = [0,0,1,1,0,2] ; q4 A = [2,1,0,0,1,0], B = [0,0,1,1,0,1] ; 36 candidates
dans les trois lanes.

### 2.4 Repli de séparation invisible dans un `CreditBatch`

Quand `direction² < 100·diam²`, les crédits tubes sont nuls (sûr) et le seul
signal est `tube_separation_fallbacks`. Dans `make_credit_batch` il n'est
chargé qu'à `shared_work()` : sur A = {(100,100,100), (106,108,100),
(103,104,100)}, B = A + (49,0,0), Kmax = 10, s = 1, `shared_work` compte 2
replis et `plan(lane).work()` en compte 0 pour les trois lanes. Un
consommateur d'un plan seul ne distingue pas « lemme inapplicable » de
« aucun témoin ». Exposer le repli sur le plan ou documenter que seul le
lot le porte, et ajouter l'assertion côté lot dans `tube_gate`.

### 2.5 Contrôle vacant : `frontier_restarts == 0`

`q2_census_gate` exige `work.frontier_restarts == 0`, mais le compteur n'est
incrémenté qu'immédiatement avant un `throw` dans `root_start`, et `cover`
n'appelle `root_start` qu'avec 0 : la condition est toujours vraie. Soit
retirer compteur et assertion, soit tester la garde par une fixture qui
attend `logic_error`.

### 2.6 Directions conservatrices non discriminées

Les seules mutations qu'aucune porte ne peut voir remplacent une décision
certifiée par `Uncertain` ou une scission (`h_max4 <= 0` → `< 0`,
`<= 16·xi.low` → `<`, `maximum4 <= 0` → `< 0`, `> b_diagonal` → `>=`) : elles
augmentent le travail sans changer l'objet. C'est acceptable par doctrine
(« Uncertain n'est pas un rejet »), mais une régression de ce type ne se
verrait que dans les reçus de campagne. Si l'on veut la verrouiller,
fixtures d'égalité exacte `h_max4 = 0` et `maximum4 = 0` avec attente sur la
décision.

### 2.7 `mhgp8_campaign_gate` est intermittent

Reproduit ce jour sur 85015a8c : un échec dans la suite complète lancée
avec `ctest -j 4` (« RuntimeError: failed attempt stdout lost: interrupted »,
`tests/campaign_gate.py` ligne 208), puis 0 échec sur 8 relances isolées.
Un agent avait observé 3 échecs sur environ 50 exécutions à 1bf806f0. La
fixture « interrupted » envoie SIGINT au runner après avoir imprimé ; la
ligne enregistrée a le bon statut mais `stdout_base64` vide selon
l'ordonnancement. Rendre la fixture déterministe (accusé de lecture avant
SIGINT, ou lecture jusqu'à EOF avant propagation) et journaliser les bruts
dans le message d'échec ; un reçu qui relance jusqu'au vert masquerait une
régression réelle du runner.

### 2.8 Portes Python liées à la disposition du dépôt

Copié sous un autre nom, le chantier fait échouer 16 des 34 tests sans
changement de code (chemins construits depuis le nom du dossier,
`git rev-parse`, `git ls-tree` d'un commit épinglé, `tools/check_docs.py`
attendu à la racine) ; 3 échouent encore après `git init`. La CI GitHub ne
construit pas la v8, donc rien ne le détecte. Dériver les chemins de
`Path(__file__).resolve().parents[1]`, échouer explicitement quand git ou
l'historique manque, passer le chemin de `check_docs.py` par option CMake.

### 2.9 Reçu complémentaire des tubes épinglé sur d'anciens octets

`audits/morsehgp3D_v8_complementaire/TUBES_CHECKS.json` épingle
`tube_credits.hpp` = `a850a442…` (3589a2c9) ; la version courante est
`8485c7a5…` (8e406f9b, `PreparedTubes` partagée) et la sonde correspondante
a quitté le worktree. Seule `tube_gate.cpp` juge les octets actuels ; les
997 rectangles frontière ci-dessus les couvrent sans faute, mais hors dépôt.
À signaler à l'auditeur complémentaire.

## 3. Ce qui n'est pas dans le périmètre

Aucune porte de bout en bout n'existe encore (aucun objet FULL). La
vérité terrain utilisable pour la tranche FULL minimale est décrite dans
[DIALOGUE_AUDITEUR_B.md](DIALOGUE_AUDITEUR_B.md) : la chaîne
`build_exhaustive_hierarchy` de `reference/` accepte les plateaux
cosphériques, alors que `run_oracle` et le contrat v2 les refusent.
