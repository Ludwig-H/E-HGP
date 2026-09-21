# Dialogue courant de l'auditeur indépendant B (v8)

21 septembre 2026, après **4dbe3024** (tranche 31 commise à 08:18 UTC pendant
la mesure ci-dessous, épinglée à c5308651 ; `src/wspd/front.cpp` n'a pas
changé entre les deux), sur main. Canal rouvert : l'auditeur B reprend son rôle d'auditeur indépendant
après avoir été constructeur du 17 au 20 septembre (tranches 19 à 21) ; il ne
requalifie jamais son propre code de cette période. Écritures limitées à
`morsehgp3D_v8/audits/` ; l'auditeur A conserve
[DIALOGUE_COURANT.md](DIALOGUE_COURANT.md) et ses notes P0_*, l'auditeur
complémentaire `audits/morsehgp3D_v8_complementaire/`, le constructeur ses
sources, docs, reçus et [ETAT_COURANT.md](ETAT_COURANT.md).
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

Cadre fixé par l'utilisateur pour cette reprise : l'objet est le clustering
hiérarchique par polyèdres des niveaux K-NN (la tour HGP par niveaux) ; on
audite son **calcul** (exactitude, complétude, coût sur les régimes visés,
SemanticKITTI en tête), **pas encore sa pertinence** : aucune confrontation à
une vérité terrain n'est menée pour l'instant. L'oracle indépendant
[oracle_q3q4_20260915/](oracle_q3q4_20260915/README.md) reste prêt ; A mène
le contrat global exhaustif dans `q34_global_contract_20260921/` et B ne le
duplique pas.

## Incident du 21 septembre, 11:00 UTC : mon commit 4c3cdb0c a emporté la tranche 34

Faute de l'auditeur B, à signaler avant tout le reste : le commit
`4c3cdb0c` (« audit: measured stateless relay… ») contient, outre mes cinq
fichiers de `relais_temoins_20260921/`, **300 fichiers du constructeur** que
l'index partagé portait au moment de mon `git commit` : les reçus
`receipts/q4_seed_cells_20260921/` (qualifications, sanitize, mutations r2,
performance r2, compatibilité), `src/pipeline/wspd_q34.{cpp,hpp}`,
`tests/q4_seed_cells_gate.cpp`, `tests/q4_seed_cells_mutations.py`,
`tests/wspd_q34_gate.cpp` et le reste de la tranche 34. Ma chaîne affichait
`git diff --cached --stat` mais ne s'arrêtait pas sur un index non vide.
Le commit suivant du constructeur, `d6e1bd9e` (« prune dead q4 atlases and
qualify seed-cell joins »), ne porte donc plus que le journal. Aucun octet
n'a été modifié ni perdu, l'historique partagé n'est pas réécrit : **la
tranche 34 est celle du constructeur, ses sources et reçus sont ceux de
`4c3cdb0c` et sa section de journal celle de `d6e1bd9e`** ; toute citation
de la tranche doit nommer ces deux commits. Les reçus que j'ai ainsi
commis n'ont pas été relus par moi et ne valent aucune contrelecture.
Procédure corrigée pour la suite : `git diff --cached --quiet || exit` avant
tout `git add`, jamais une simple impression.

## Réponses aux deux questions du journal du 21 septembre

Mesure d'appui : [front_lanes_lidar_20260921/](front_lanes_lidar_20260921/README.md),
18 exécutions (scans 0/100/200, n = 8k/16k/32k, K = 5/10, s = 8) sur les
sources c5308651 épinglées, 2 000 paires jugées exactement par voie et par
exécution, reçu rejoué en `python3` et `python3 -O`, aucun flottant.

### Question pratique : K témoins autour du pivot, ou crédits h_a/h_b par blocs ?

**La fenêtre de K témoins autour du pivot est la faiblesse dominante ; les
crédits par blocs n'apporteraient rien.** Les témoins exacts situés dans A ou
B représentent 1 à 4 % des témoins du citron, sur les deux voies et les 18
exécutions. À l'inverse, un proposeur ponctuel parfait (compte saturant des
sites universels pour les boîtes, par descente de l'index) retirerait 82 à
91 % de la masse résiduelle q3 et 75 à 89 % de la masse q4, quand la fenêtre
2K n'en retire que 36 à 56 % (q3) et 6 à 55 % (q4, 6 à 16 % à K = 5), et la
fenêtre 4K à peine plus. Élargir la fenêtre ne rattrape donc pas le plafond :
il faut chercher les témoins par **descente saturante de l'index** (les bornes
conjointes de boîte existent déjà), pas par rangs autour du pivot.

Au-delà du plafond de boîte, 88 à 97 % des paires résiduelles sont rejetables
exactement (au moins h_q sites dans leur citron), part qui croît avec n ; 40
à 67 % de leurs témoins sont hors A∪B sans être universels pour les boîtes.
Le reste de la réduction passe donc par un test **par paire**, mesuré
ci-dessous, jamais par un scan local quadratique.

### Question prioritaire : census q3 par boîtes, certificats avant les clés

**Le census par boîtes de la puissance exacte est validé, mais ce n'est pas
lui qui supprime les 780 M seeds : c'est le rejet exact de la paire avant sa
couverture.** Sur les 18 exécutions, une paire rejetable porte 2 000 à
13 000 sites de couverture et 350 à 1 950 seeds aigus (le constructeur mesure
341 par arête développée à 8k/K5, soit 363 par arête q3, en accord), alors qu'une paire conservée porte 47 à
792 sites de couverture en moyenne (maximum 11 225) et 8 à 125 seeds. Rejeter
la paire d'abord divise la masse de seeds par 87 à 1 519 selon l'exécution
(tableau des coûts du reçu). Mais les survivantes ne se censurent à bon
compte naïvement qu'aux petites tailles : leurs couvertures et leurs seeds
croissent de ×2,3 à ×2,8 par doublement, et la projection sur la masse
résiduelle (part conservée × masse × moyennes par paire) donne à 8k/K5 sur
le scan 0 1,37 M seeds et 60 M tests naïfs au lieu de 779 M seeds (le
constructeur en mesure 780,66 M : la projection tombe juste) et 361 Md
tests, mais encore 251 M seeds et 334 Md tests naïfs à 32k/K10 sur le scan
200. Le census par boîtes saturant n'est donc pas facultatif pour les
survivantes ; il vient après le rejet par paire, pas à sa place. Ordre
recommandé, avant covers et seeds :

1. Par rectangle résiduel : descente saturante de l'index avec les bornes de
   boîte (le « plafond » mesuré), coût total 46 à 851 M visites de nœuds par
   exécution dans mon harnais mono-fil, soit 3 à 48 s pour les deux voies.
2. Par paire restante : descente saturante avec les boîtes singleton de a et
   b, enfant le plus proche du milieu de ab visité en premier : 43 à 148
   visites par paire rejetable q3 et 63 à 222 en q4 selon l'exécution (quatre
   à six fois moins qu'en préordre), décision identique au balayage complet
   sur les 72 000 paires jugées.
3. Couverture serrée pour q3 : |2z−a−b|² ≤ 3|b−a|² suffit (centre à moins de
   D/√3 du milieu et rayon au plus 2D/√3, D demi-longueur, donc tout intérieur
   à moins de √3·D du milieu), 1,54 fois moins de volume que 4|b−a|² ; en q4
   la couverture 4|b−a|² reste nécessaire ((1/√2 + √(3/2))·D < 2D).
4. Census par seed sur les survivantes, par boîtes de la puissance,
   saturant, sous-arbre du centre d'abord (le naïf ne tient pas à 32k/K10).

Sur le census par boîtes lui-même (proposition du contrat 31) : la fonction
A|z|² + B·z + C est convexe et séparable, son minimum entier par axe est
atteint au plancher ou au plafond du sommet −B_i/(2A) rabattu dans l'axe, son
maximum à une extrémité ; minimum > 0 écarte le nœud, maximum < 0 admet son
cardinal, saturation à h_q, chaque nœud compté une fois : exact. Deux
précisions : (i) séparer les deux phases, un compte saturant des intérieurs
stricts qui écarte aussi les nœuds à minimum = 0 (les contacts ne servent
qu'à l'acceptation), puis, pour les seules boules acceptées, une seconde
descente des nœuds à minimum ≤ 0 ≤ maximum pour récupérer la coquille
entière ; raffiner les égalités pour toutes les boules rejetées paierait pour
rien. (ii) Visiter d'abord le sous-arbre du centre : la saturation d'une
boule profonde se joue près du centre, l'ordre préordre gauche-droite est
quatre à six fois plus cher, comme mesuré ici pour les citrons.

Certificat de famille avant les clés : pour une paire survivante, le compte
c < h_q du citron est un plancher d'intérieurs commun à toutes ses boules
propriétaires ; chaque seed n'a plus à trouver que h_q − c intérieurs dans la
couverture serrée. Avec 8 à 125 seeds par paire survivante, un certificat
plus fin (secteurs du disque des centres dans le plan bissecteur) n'est pas
nécessaire aujourd'hui ; ne pas construire d'index par seed.

**Lemme du citron, vérification exécutable (q3)** : sur 1 800 paires
rejetables et tous leurs seeds aigus propriétaires, aucune circumboule n'a de
profondeur < h_q (0 violation). Confirmation par instances de la preuve de A,
pas une preuve ; q4 n'est pas vérifié de cette façon.

### Citron : accord avec A, une phrase à préciser

Accord avec la preuve de A par la variance (R² ≤ D²(q−1)/(2q), α3 = 3,
α4 = 2, inégalités strictes, contacts réalisables en u16). **Une phrase de son
brouillon non commis du 21 septembre est à préciser** (elle n'apparaît pas dans
la version commise de son dialogue) : « utiliser α3 pour q4 aussi ». Si elle
signifie prendre α = 3 sur la voie q4, c'est non sûr : α4 = 2 est exact et
serré.
Contre-exemple entier : tétraèdre a = (0,0,0), b = (60,0,0), c = (20,42,0),
d = (28,10,49), arête ab maximale, centre strictement intérieur
(30, 241/21, 28885/2058), r² = 5203980749/4235364 ; le site z = (28,−12,−12)
a H = 608 et Ξ = 1 036 800, donc 2H² = 739 328 ≤ Ξ < 3H² = 1 108 992 : z est
dans L_3(a,b) mais **strictement extérieur** à cette boule q4
(|z−o|² = 5222107613/4235364 > r²). La même configuration translatée de
(0, 12, 12), a = (0,12,12), b = (60,12,12), c = (20,54,12), d = (28,22,61),
z = (28,0,0), est une entrée u16 valide aux mêmes H, Ξ et distances. Un front
qui créditerait z pour q4 pourrait rejeter à tort l'arête. Vérifié à 4dbe3024 : `spindle/predicates.hpp`
renvoie 3 pour Q3 et 2 sinon (ligne 125) ; rien à changer dans le code.

### Clôture 31 et pilote R3 : d'où vient le ×10 par doublement

Le journal de clôture donne, à K5/s8 sur G4 avec 48 workers, 1k/2k/4k/8k =
0,863/8,470/59,274/614,744 s, census q3 ×10,8/×10,1/×10,8 par doublement,
et 3,23 CPU moyens malgré 48 workers. Ma mesure décompose ce facteur sur
8k → 32k : masse résiduelle des paires ×2,4 à ×3,2 par doublement (la part
résiduelle décroît moins vite que 1/n), seeds par paire rejetable ×1,6 à
×2,3, couverture par paire rejetable ×1,6 à ×2,2 ; le produit vaut ×6 à
×16, en accord avec ×10. Rien n'est sous-quadratique tant que les paires
rejetables sont développées ; après leur rejet, les paires conservées
croissent de ×2,1 à ×2,4 par doublement et leurs seeds de ×2,3 à ×2,8,
d'où l'ordre ci-dessus. Les 3,23 CPU sur 48 disent que le grain d'une arête
atomique (2 237 sites de couverture en moyenne, maximum 8 000) laisse
l'équipe inactive : le rejet par paire retire les grosses arêtes, mais la
queue lourde des survivantes (couvertures jusqu'à 11 225 sites) demande un
grain plus fin que l'arête, par blocs de seeds, avant tout pilote 50k.

### Lecture du raccord 31 (`pipeline/wspd_q34.cpp` à 4dbe3024) et fixtures

Voie q3 relue : test d'acuité strict sur les trois angles, propriété de
l'arête ab avec égalité admise et départage par la plus petite clé d'IDs,
bornes de nœud correctes (min de distance à a ou b > D² écarte, max de la
somme ≤ D² écarte l'angle en x), seuil de rejet `depth ≥ K − 1` conforme,
saturation du census, cover fermé qui contient tous les intérieurs. Aucun
défaut d'exactitude vu. Deux remarques : (i) la coquille émise est la
coquille **complète**, support compris (`q34_seed.hpp` : « complete-shell
parts ») : trois IDs par émission q3, ce qui explique les 327 815
comparaisons de tri pour 93 914 émissions et fait compter deux fois le
support dans `payload_shell_ids` ; dire explicitement que le consommateur doit
retirer le support de la coquille, ou l'exclure à l'émission. (ii) Le point
d'insertion du rejet par paire est `Engine::edge`, **avant**
`Q34EdgeCover::make` : une descente saturante par voie active avec les
boîtes singleton de a et b (bornes `Q2JointPreparedBounds` et `xi_bounds`
déjà écrites, feuilles jugées par le citron exact), retrait de la voie dont
le compte atteint h_q, et pas de cover si aucune voie ne survit ; la
descente « milieu d'abord » de mon harnais (`front_lanes_probe.cpp`, § 4 bis)
en est le patron mesuré. Le pilote parallèle (jobs du front tirés par un
compteur atomique, moteur privé par slot, jointure avant réduction) est
correct ; son grain reste l'arête atomique à l'intérieur d'un job.

Fixtures proposées pour le census par boîtes (contrat 31), recalculées en
rationnels exacts : la clé de Triangle((0,0,0),(2,2,0),(2,0,2)) est bien
[3, −8, −4, −4, 0] ; **celle de Triangle((0,0,0),(2,0,0),(1,1,1)) n'est pas
[2, −4, −2, −2, 0] mais [2, −4, −1, −1, 0]** (centre (1, 1/4, 1/4),
r² = 9/8 ; avec la clé écrite, |o−c|² vaudrait 1/2 contre 3/2 pour a). Les
sommets des paraboles sont donc 1, 1/4, 1/4 (pas demi-entiers), et la boîte
x = z = 0, y ∈ [0, 1] contient un seul contact, a lui-même ((0,1,0) a une
puissance 1 > 0, extérieur). La clé de
Triangle((65535,65534,65533),(0,0,65532),(2,65531,0)) est
[27665049883098415167, −1208778244396181609578829,
−2417279841248645846597407, −2417335173459171585359629,
39606828108042427812408262620] (65, 80, 81, 81 et 95 bits ; B² dépasse
bien 2^128), triangle aigu d'arête ab maximale. Les fixtures u16 de A
(équilatéral et tétraèdre régulier d'arête 72, contacts z à Ξ = 3H² et
Ξ = 2H², centre (33, 33, 27) strictement intérieur) sont exactes.

### Flux q3 du raccord 31 : accord exact avec une énumération indépendante (1k à 4k)

Reçu [q3_stream_crosscheck_20260921/](q3_stream_crosscheck_20260921/README.md) :
la sonde `mhgp8_wspd_q34_probe` construite à 4dbe3024 (masque 2, quatre
workers, mode `records`) et mon harnais en mode exhaustif (toutes les paires
résiduelles q3 du même front, citron exact, seeds avec la règle de propriété
du raccord, census entier des circumboules sur la couverture des paires
conservées) donnent, sur les préfixes 1k/2k/4k du scan 0 à K5 et 1k/2k à
K10, **les mêmes masses résiduelles, les mêmes seeds q3, les mêmes boules
émises et le même multiensemble (support, profondeur)** : 10 481, 21 948,
45 151, 42 876 et 92 994 boules ; la voie q3 ne dépend pas du masque demandé
(1k et 2k relancés au masque 6). Le lemme du citron est vérifié seed par
seed sur **toutes** les paires rejetables à 1k (78 419 paires K5 et 167 992
paires K10, 13,2 M seeds, 0 violation) et sur 2 000 puis 1 000 paires aux
tailles suivantes. C'est un contrôle du calcul (même objet, deux codes
indépendants pour tout ce qui suit le front), pas une qualification : trois
petites tailles, un scan, pas de q4.

Deux lectures de coût sur ces lignes : les seeds du moteur croissent de
×5,5 à ×6 par doublement (3,68 M → 21,9 M → 121,7 M à K5, soit n^2,5) quand
les boules émises croissent linéairement (10,5 k → 21,9 k → 45,2 k, ≈ 11 n à
K5, ≈ 46 n à K10) ; à 4k/K5 le census q3 du moteur fait 33,6 Md tests
ponctuels là où le harnais n'en fait que 86,7 M sur les paires conservées,
le reste étant réglé par le citron (facteur 387). Le rejet par paire avant
la couverture est donc mesuré ici sur le moteur réel, pas seulement projeté.

### Flux q4 (Local28) du raccord 31 : accord exact avec une énumération indépendante (1k à 4k)

Reçu [q4_stream_crosscheck_20260921/](q4_stream_crosscheck_20260921/README.md) :
la sonde à 4dbe3024 (masque 4, Local28, mode `records`) et mon harnais, qui
n'appelle aucune brique q4 du moteur (pour toutes les paires résiduelles de
la voie q4 : citron exact, puis pour chaque paire conservée l'énumération de
tous les tétraèdres propriétaires de ab au sens de `owned` de `q4_local.cpp`,
positivité stricte par les quatre coordonnées barycentriques du centre en
i128, profondeur exacte sur la couverture), donnent **le même ensemble de
boules distinctes (clé entière réduite, profondeur)** sur les préfixes
1k/2k/4k du scan 0 à K5 et 1k/2k à K10 : 876, 2 113, 4 798, 8 033 et 20 194
boules, chacune émise une fois de chaque côté (pas de plateau cosphérique sur
ces préfixes), chaque support de la sonde étant sur sa propre sphère ; la
voie q4 ne dépend pas du masque demandé (1k et 2k relancés au masque 6). Le
lemme du citron pour q4 (α4 = 2) est vérifié tétraèdre par tétraèdre sur
toutes les paires rejetables à 1k (45 918 paires à K5, 1,06 M tétraèdres
positifs, 0 violation ; toutes les paires rejetables aussi à K10) et sur
2 000 puis 500 paires aux tailles suivantes. Les coquilles ne sont pas
comparées. Avec le reçu q3, la totalité de ce qui suit le front dans le
raccord 31 est donc confrontée à un second calcul indépendant sur ces
tailles ; ce n'est toujours pas une qualification (trois petites tailles, un
scan, pas de FULL).

### Brouillon de la tranche 32 (non commis) : lecture sans objection

Le brouillon `docs/Q34_TEMOINS_INDEXES_ET_CENSUS_BOITES_20260921.md`,
`lanes/q34_witness_search.cpp` et `lanes/q3_ball_census.cpp` (état du
worktree à la lecture, pas une version publiée) porte exactement les deux
mécanismes recommandés ci-dessus : recherche saturante de témoins sur
l'index avant toute couverture (rectangle puis paire, bornes conjointes de
4H et majorant de Ξ, enfant le plus proche du milieu d'abord, un nœud crédité
une seule fois par voie par retrait du bit du masque transmis aux enfants,
identité de conservation `q3_edges + rectangle_q3_pairs + pair_q3_pairs =
masse résiduelle q3` dans `validate_completion`), et census q3 par boîtes en
deux phases (compte saturant des intérieurs stricts où `minimum ≥ 0` écarte
le nœud, puis coquille des seules boules acceptées par les nœuds à
`minimum ≤ 0 ≤ maximum`), minimum entier par axe au plancher ou au plafond
du sommet rabattu dans l'axe, maximum aux extrémités, enfants visités par
minimum croissant. Le brouillon dit explicitement qu'employer 3 à la place
de 2 pour q4 serait incorrect. Aucun défaut vu à cette lecture ; à sa
commission, les deux harnais de flux (q3 et q4) seront rejoués sur le
nouveau commit avec les nouvelles options, car ils jugent le flux émis,
pas le mode interne.

### Réponses aux questions A/B du brouillon 32 (journal non commis, lu le 21 septembre)

**Couches duales 29 pour filtrer les seeds q3 à T = K − 1 : valide, mais non
rentable ici.** La preuve tient : pour une seed x strictement aiguë,
c_x > 0 (x hors de la boule diamètre), donc son centre q3 t_x vérifie
L_x(t_x) = 0 avec t_x ≠ 0 ; si x est strictement intérieure aux T couches du
groupe positif, la forme linéaire non constante 1 + p·t_x prend sur chaque
couche un minimum strictement inférieur à 0, d'où T sites retenus distincts,
autres que a, b, x (a et b ont la forme affine nulle, x n'est sur aucune couche),
strictement intérieurs : profondeur ≥ K − 1, boule non émise. Peler le seul
groupe positif suffit pour cette décision ; garder les duaux coïncidents,
les points d'arête et les enveloppes dégénérées, et séparer dans l'API le
seuil de sélection du seuil d'émission (le piège K + 1 → K − 1 signalé est
réel). Contre-fixture du mauvais seuil recalculée exactement : centre
(12, 65/6, 10), r² = 169/36, puissances −7/3 et +2/3, arêtes² 16/13/13,
aigu, ab maximale : conforme. Mais le coût ne suit pas : sur les paires
conservées de mes 18 exécutions, la part des sites de couverture qui sont
des seeds aiguës propriétaires vaut de 0,15 à 0,21 (stable sur les trois
scans, les trois tailles et les deux K), alors que le census par boîtes de
la série 32 coûte environ 35 bornes par seed pour le compte seul (67,5 M
bornes de compte pour 1,91 M seeds à 8k ; 72,9 M avec la coquille) : les couches coûtent (log₂ m + T)·m par arête, soit un seuil
de rentabilité seeds/m ≥ 0,27 à 0,53 selon l'exécution, jamais atteint.
Le filtre coûterait donc 1,5 à 3,4 fois le census qu'il évite, en comptant
une borne de boîte et une comparaison de couche pour une unité chacune. Marge
à connaître : 1,34 seulement entre le ratio maximal (0,204) et le seuil
minimal (0,273) ; avec le seul groupe positif et la couverture serrée le
modèle frôle 1 à 8k/K5, mais le noyau 29 réellement mesuré par le
constructeur (Window30) coûte 2,1 à 2,5 fois ce modèle, ce qui rend la
conclusion conservatrice. Ne pas le
porter pour q3 sur ces régimes ; le réexaminer seulement si un régime donne
seeds/m au-dessus du seuil ou si le coût par boule du census remonte.

**Spécialisation de Ξ à endpoints fixes et rejet de voie par nœud : oui.**
Pour a, b fixes, Ξ(z) = |d × (z − a)|² avec d = b − a est une somme de trois
carrés de formes affines en z ; l'intervalle de chaque forme sur une boîte
s'obtient par les signes de ses coefficients, son carré minimal vaut 0 si
l'intervalle contient 0, sinon le plus petit carré des bornes, et la somme
des trois minima minore Ξ sur la boîte. Si α·max(0, H_max)² ≤ Ξ_min, aucun
z de la boîte n'est témoin de la voie (pour tout z, α·H(z)² ≤ α·max(0,H_max)²
≤ Ξ_min ≤ Ξ(z)) : rejet sûr, qui complète H_max ≤ 0. Il vise les nœuds de
la ceinture de la boule diamètre hors du citron, que la descente actuelle
ouvre jusqu'aux feuilles ; même logique avec les bornes basses de Ξ sur
A × B × Z au niveau rectangle.

**Héritage des témoins entre produits subdivisés : oui, par monotonie.**
Pour A' ⊆ A et B' ⊆ B, h_min(A', B', Z) ≥ h_min(A, B, Z) et
Ξ_max(A', B', Z) ≤ Ξ_max(A, B, Z) : un nœud Z admis pour une voie sur A × B
l'est sur tout sous-produit, un nœud exclu (H_max ≤ 0) le reste, et seule la
frontière des nœuds ambigus est à retester chez les enfants. L'état à
transmettre par voie est donc (crédits des nœuds admis, frontière des
nœuds ambigus), sous-arbres disjoints, jamais un test ambigu pris pour un
certificat ; la paire est le dernier sous-produit et repart de la frontière
de son rectangle au lieu de la racine. C'est ce que mesurait
[PROPAGATION_TEMOINS_20260914.md](PROPAGATION_TEMOINS_20260914.md) sur la
voie q2 (résidu ×0,40 à ×0,60 à 32k uniforme, 0 rejet non sûr). Sur mes
mesures, la descente par paire coûte 43 à 148 visites « milieu d'abord »
dont la plus grande part est le tronc commun aux paires d'un même
rectangle : c'est là que l'héritage porte, et il répond au ×5,4 des bornes
par paire relevé de 16k à 32k.

Série 32 en brouillon (8k/16k/32k, K5, non commise) lue : 1,91 M seeds q3 à
8k contre 1,37 M projetés par mon échantillon (écart d'échantillonnage sur
une moyenne à queue lourde, sens et ordre de grandeur confirmés), temps
locaux 4,96/18,1/60,7 s au lieu de 1 361 s à 8k ; les visites q4 ×4,0 puis
×5,6 par doublement restent le poste dominant à surveiller.

### Tranche 32 commise (d1b4dbc6) : rejeu des deux flux, accord exact et rejet égal au citron

Reçu [q34_stream_crosscheck_t32_20260921/](q34_stream_crosscheck_t32_20260921/README.md) :
la sonde à d1b4dbc6 en modes `rectangle-pair` et `boxes` (masque 6, Local28,
mode `records`) contre les deux harnais de la tranche 31 (copies à l'octet
près), sur les mêmes préfixes 1k/2k/4k (K5) et 1k/2k (K10) du scan 0 : mêmes
boules q3 (support, profondeur) et mêmes boules q4 distinctes (clé,
profondeur) que les énumérations indépendantes, et que la tranche 31 (à 1k/K5
la sonde relancée en `disabled`/`scalar` donne les mêmes comptes, empreintes
et IDs de coquille). En outre, **la recherche de témoins rejette exactement
les paires du citron** : sur chaque ligne, `rectangle_q3_pairs +
pair_q3_pairs` égale mon compte de paires q3 rejetables, `q3_edges` mon compte
de paires conservées, `q3.seeds` les seeds de mes seules paires conservées, et
de même pour q4 (à 1k/K5 : 72 938 + 5 481 = 78 419 paires q3 rejetées,
17 297 conservées ; 39 863 + 6 055 = 45 918 paires q4 rejetées, 15 637
conservées). Temps de la sonde sur ces préfixes, sous charge et sans valeur
de contrat : 0,3 / 0,9 / 2,1 s à K5 et 1,3 / 3,3 s à K10, là où la tranche
31 demandait 1,1 + 1,2 / 12 + 8 / 90 + 52 s (q3 puis q4) et 4 + 14 / 30 + 32 s.
Rien n'est transféré aux tailles 8k et plus, que mesure la série 32 du
constructeur (4,96 / 18,1 / 60,7 s à K5), ni aux coquilles.

Sur l'analyse de croissance 32 du constructeur : le poste super-quadratique
qui reste est la recherche **par paire** (bornes H ×5,4, Ξ ×7,4 de 16k à
32k à K5), exactement le point visé par les deux réponses ci-dessus (rejet
de nœud par Ξ_min à endpoints fixes, héritage de la frontière du rectangle
vers ses paires) ; les visites q4 (×4,0 puis ×5,6 par doublement) sont le
second poste, hors de portée de ces deux mesures.

### Tranche 32 à 8 000 points : accord exact des deux flux à la première taille d'intérêt

Reçu [q34_stream_crosscheck_t32_8k_20260921/](q34_stream_crosscheck_t32_8k_20260921/README.md) :
mêmes harnais, mêmes égalités exigées, sur le scan 0 à 8 000 points entier,
K5 et K10, sonde à d1b4dbc6 en modes `rectangle-pair`/`boxes`. À K5 :
93 914 boules q3 et 10 756 boules q4 identiques (support ou clé, profondeur)
à l'énumération indépendante, et identiques au flux des anciens modes
relancés à 8k (mêmes comptes, empreintes et IDs de coquille) ; 1 940 894
paires rejetées par rectangle et 167 441 par paire, 344 856 paires
développées et 1 911 457 seeds q3, exactement les paires rejetables,
conservées et seeds de mon harnais et exactement les compteurs publiés par
la série 32 du constructeur (masses de l'union des voies ; par voie, q3 :
1 868 974 + 115 604, q4 : 1 040 041 + 163 679). À K10 : 409 195 boules q3 et
116 985 boules q4 identiques, 4 171 837 + 324 331 paires rejetées (union),
717 777 développées,
8 099 443 seeds q3, mêmes égalités (les 116 985 boules q4 sont les boules
distinctes ; les deux côtés émettent 116 988 présentations, trois d'entre
elles partageant une même boule cosphérique). Les deux lemmes du citron restent sans
violation sur 300 (q3) et 200 (q4) paires rejetables par ligne. La sonde
met 4,7 s à K5 et 19,2 s à K10 sur cet hôte partagé (la tranche 31 mettait
1 361 s à K5). Tout ce qui suit le front dans le raccord est donc confronté
à un second calcul indépendant à 8k, à deux valeurs de K, sur un scan ;
16k et 32k ne le sont pas (le harnais exhaustif y coûterait des heures), et
rien n'est qualifié.

### Question q3 du journal 33 (témoins communs d'un bloc de seeds) : formule confirmée, réponse de A suffisante

Vérification exacte indépendante sur 2 999 triangles entiers non colinéaires
tirés au hasard : avec d = b − a, D = |d|², w = 2x − a − b, v = 2z − a − b,
J = D|w|² − (d·w)² = |d × w|², P = Dw − (d·w)d, qx = |w|² − D et
qz = |v|² − D, on a bien F = J·qz − qx·(P·v) = 4J·puissance(z) dans la
circumboule de (a,b,x) (la sphère dont le centre est dans le plan du triangle) et
centre (a+b)/2 + qx·P/(4J), sans exception ; l'identité est vide à J = 0 (P = 0,
F = 0, centre indéfini). J > 0 est la non-colinéarité, pas la positivité ;
qx = 4(a−x)·(b−x) > 0 est l'acuité en x.
La note de A [q3_seed_block_power_20260921/](q3_seed_block_power_20260921/README.md)
(boîte de centres par bloc X grâce à λ = DQ/J ∈ (0, 2/3] sous acuité et
propriété, puis six paraboles en z par nœud Z) est l'alternative que
j'aurais proposée aux bornes cubiques en x : le seul pas non linéaire est
l'enveloppe des centres du bloc, et la puissance (z − a)·(z + a − 2c) = |z|² −
|a|² − 2c·(z − a) est ensuite affine en c et quadratique convexe en z :
maximum aux coins de Z × C, minimum au sommet borné z_i = clamp(c_i, Z_i) pour
chaque extrémité de C_i (corrigé le 21 septembre : ma première rédaction
disait « bilinéaire en (centre, z), donc bornée aux coins », ce qui est faux
pour le minimum ; fixture et conséquence dans la relecture SharedPrefix
ci-dessous). Je n'y ajoute qu'une discipline de compte : les témoins communs
d'un bloc X proviennent de nœuds Z d'une partition disjointe, s'ajoutent une
fois au compte de chaque seed de X et saturent à K − 1 par seed ; un nœud Z
ambigu pour le bloc est retesté par seed, jamais hérité comme certificat ;
aucun crédit commun n'initialise le census d'une boule acceptée (sa coquille
et sa profondeur exactes sont recalculées).

### Tranche 33 commise (2629a536, bornes `affine`) : rejeu des deux flux, accord exact

Reçu [q34_stream_crosscheck_t33_20260921/](q34_stream_crosscheck_t33_20260921/README.md) :
sonde à 2629a536 en modes `rectangle-pair`, `boxes` et bornes de témoins
`affine` (Ξ spécialisée aux endpoints fixes, exclusion de nœuds), mêmes
harnais et mêmes égalités que pour les tranches 31 et 32, sur les préfixes
1k/2k/4k (K5) et 1k/2k (K10) du scan 0 : mêmes boules q3 et q4, mêmes
profondeurs, rejets par rectangle et par paire égaux aux paires rejetables
du citron, seeds égaux, lemmes sans violation, même flux qu'en modes
`disabled`/`scalar`/`legacy` à 1k/K5. Les bornes 33 ne changent pas l'objet
émis ; leur coût et leur croissance restent ceux que mesurent les reçus 33
du constructeur, non jugés ici.

### Relais sans état par bloc (demande du journal 34) : proposition mesurée

Reçu [relais_temoins_20260921/](relais_temoins_20260921/README.md). Le relais
proposé ne stocke rien par paire ni par bloc : pour un rectangle survivant à
la recherche saturante, une recherche exhaustive sur l'index produit un
**crédit commun** U (nœuds universels) et une **liste de candidats** C (feuilles
ni universelles ni exclues) ; chaque paire du rectangle teste C en saturant,
puis C est libérée. L'identité « compte du citron = U + citron sur C » tient
par monotonie des bornes de boîte (les nœuds exclus pour le rectangle le sont
pour chaque paire, les nœuds universels le sont aussi) ; elle est vérifiée
sans saturation contre le balayage complet sur les paires tirées qui tombent
dans un rectangle survivant (172 à 510 par voie et par exécution, sur 2 000
tirées ; les autres ne reçoivent que le contrôle « compte ≥ h_q » du rectangle
rejeté), 0 désaccord sur 8 exécutions (scan 0 à 8k/16k/32k, K5 et K10,
scans 100 et 200 à 8k/K5). Le même schéma vaut pour le relais bloc de seeds →
seeds du census q3 (crédit commun, liste transitoire, saturation par seed,
rien d'hérité qui ne soit un certificat), non mesuré ici.

Mesure : 9,6 à 21,7 candidats par rectangle survivant en q3 (12,7 à 30,8 en
q4), maximum 1 100 à 4 221, pour un surcoût de 74 à 120 visites par rectangle
survivant. Par paire, les tests saturants sur C valent 0,30 à 0,66 fois les
visites de la descente saturante depuis la racine (q3 ; 0,39 à 0,84 en q4),
et en travail total le relais vaut 0,71 à 0,87 fois la référence en q3 et
0,74 à 1,04 en q4 : gain réel mais modéré, parce que la descente par paire
est déjà courte après le rejet par rectangle. Ce qui compte pour la
croissance : les candidats par rectangle passent de 10 à 18 (q3, K5, 8k →
32k) quand les visites par paire depuis la racine passent de 88 à 252 ; le
relais absorbe donc la part super-linéaire observée par le constructeur
(×5,4 des bornes par paire de 16k à 32k), sans réduire le nombre de paires
à juger, qui reste le vrai poste. Unités hétérogènes (visite ≠ test), donc
comptes et non temps ; la part « paires » de la référence est extrapolée
depuis 172 à 510 paires échantillonnées par exécution, à distribution lourde
et sans intervalle de confiance ; à comparer au coût mesuré du port avant de
choisir.

### Tranche 34 (sources dans 4c3cdb0c par ma faute, journal dans d6e1bd9e) : rejeu des deux flux, accord exact

Reçu [q34_stream_crosscheck_t34_20260921/](q34_stream_crosscheck_t34_20260921/README.md) :
sonde construite à d6e1bd9e en modes `rectangle-pair`, `boxes`, `affine` et
atlas q4 `joined` (cellules de seeds jointes, atlas sans feuille sautés),
mêmes harnais et mêmes égalités que pour les tranches 31 à 33, sur les
préfixes 1k/2k/4k (K5) et 1k/2k (K10) du scan 0 : mêmes boules q3 et q4,
mêmes profondeurs, rejets égaux au citron, seeds égaux, lemmes sans
violation, même flux qu'en modes anciens à 1k/K5. Le mode `joined` ne change
pas l'objet émis ; ses coûts sont ceux des reçus 34 du constructeur, que je
n'ai pas relus.

### Lecture du brouillon du protocole spatial (non commis) : deux points à corriger, trois risques de lecture

Lecture par quatre relectures indépendantes (méthode, code de préparation,
lanceur et analyseur, outillage d'audit), toutes en lecture seule sur l'état
du worktree du 21 septembre ; ce qui suit n'est pas une relecture des reçus.

- **Erreur de documentation** : `Q34_MESURES_SPATIALES` affirme que « tous
  les compteurs sont conservés, avec les temps et capacités séparés », mais
  `level_sums` de l'analyseur additionne `timings_ms.*` et `parallel.*` entre
  morceaux sans étiquette (sur la fixture, `parallel.requested_workers` vaut
  8 au niveau des quarts et 4 au niveau de la scène : somme de constantes).
  Exclure ces genres des sommes par niveau, ou les étiqueter.
- **Erreur de lecture à prévenir** : les 513 compteurs aplatis reçoivent tous
  une décision « quadratique », y compris les constantes et les compteurs nuls
  (14 constantes sortent « below » avec exposant 0, 213 compteurs « zero_work »).
  Désigner la courte liste des compteurs de tête à lire
  (`local28.atlas.partition.node_visits`, `q3_blocks.count_node_visits`,
  `witness.*.node_visits`, `front.work.product_visits` avec son préfixe
  parallèle), et ne jamais publier un ratio « compteurs sous-quadratiques ».
- **Risque méthodologique principal** : l'exposant par relation parent/enfant
  est dominé par la différence de profil de densité, pas par n. Sur la scène 0,
  un compteur exactement local et linéaire (paires à moins de 0,5 m, calcul
  exact par grille) donne 58,0 paires par site sur la scène, 54,1 et 60,1 sur
  les moitiés, 63,3 / 45,0 / 67,9 / 51,3 sur les quarts (×1,51 entre extrêmes),
  et `growth()` lui attribue des exposants de 0,78 à 1,27 selon la relation,
  pour un poste qui vaut 1. Estimateur à préférer : l'exposant poolé par
  parent (déjà calculable depuis `level_sums`), les six exposants par relation
  publiés comme dispersion, et l'écart entre frères (mêmes effectifs à ±3 %)
  comme barre d'erreur ; une bande d'indécision |α − 2| < δ plutôt qu'un bit.
- **Ce que les coupes ne peuvent pas voir** : à densité conservée, une lecture
  « sous-quadratique » est presque tautologique pour tout algorithme local ;
  elle ne détecte pas la croissance pilotée par la densité, qui est
  précisément l'axe où le moteur a ses postes au-dessus de ×4 (census q3 ×4,3,
  atlas q4 ×4,1 à ×6,5 au doublement des préfixes). Garder un axe densité
  compagnon (préfixes hachés imbriqués d'un même morceau, ou superposition de
  trames), annoncé comme non substituable ; les reçus 8k/16k/32k restent le
  seul lien de comparabilité entre versions.
- **Effet de frontière** : réel, petit, concentré dans la zone dense : 3,4 %
  des sites à moins de 0,5 m du plan x (7,9 % à moins de 1 m, aux distances
  2 à 10 m du capteur où le degré local est double de la moyenne) ; 1,49 % des
  paires à 0,5 m traversent la coupe scène → moitiés, 0,20 % et 0,80 % pour
  moitiés → quarts. Publier par compteur le déficit croisé 1 − ΣW_enfants/W_parent
  et les effectifs par tranche de distance au plan, en interprétant le signe
  selon l'étage (élagage ou acceptation).
- **Reçus et pins** : la campagne de performance en cours (`spatial_9kscvyt0`)
  tourne avec `--repeats 1` sous concurrence (1,74 cœur effectif sur 4 pour le
  premier quart), en partie à cause de mes propres harnais (voir ci-dessous) :
  n'y lire aucun exposant sur les temps, seulement sur les compteurs
  déterministes ; l'analyseur n'est pas dans `PROTOCOL_SOURCES` et le
  manifeste enregistre un worktree sale (scripts non suivis) ; le selftest du
  lecteur (préfixe, mauvais morceau, hash, compteurs altérés) existe mais
  n'est exercé par aucune porte ; la relecture dépend du répertoire courant ;
  `TEST_PLAN` § 3.1 n'est pas amendé (les morceaux le satisfont par n ≥ 8 000,
  pas à la lettre) ; « build R2 qualifié » désigne un reçu smoke `candidate`.
  Détails vérifiés (lignes, essais synthétiques) disponibles sur demande.
- Ce qui tient : `growth()` décide exactement sur les entiers dans tous les cas
  limites ; les six relations et la conservation des effectifs sont
  correctes ; les chiffres des deux portes (14 appels, 238 records, 688
  triangles, 1 964 tétraèdres, 54 positifs) sont exacts, mais la fixture à 16
  sites est un oracle de flux, jamais une mesure de croissance.
- **Préparation** (`prepare_lidar_spatial.py`, commis à 759ce2b0 avec le
  protocole et ses reçus ; seuls lanceur, analyseur et leur porte sont encore
  non suivis) : aucune erreur ; quantification exacte vérifiée sur les
  demi-cellules et 430 000 float32 denses (refus corrects hors grille et non
  finis, −0,0 → 32768). Nuances : en coordonnées brutes la coupe effective est
  x ≥ −0,01 m et non x ≥ 0 (les retours de [−0,01, 0) passent du côté non
  négatif, 46 sur la scène 0, biais d'une demi-cellule unidirectionnel, à
  écrire dans le protocole) ; un morceau réduit à un seul site serait invoqué
  puis refusé par le chargeur natif (n ≥ 2) ; les champs `*_overlap_sites = 0`
  et `*_equals_full = True` du manifeste sont des constantes littérales, pas
  des mesures ; une réflectance non finie refuse tout le scan (champ
  inutilisé) ; les reçus embarquent des chemins absolus vers un `data/`
  ignoré par Git et le hash exact du script : relecture possible seulement sur
  cette machine, à documenter.
- **Mes propres outils, corrigés à la lecture** : ma première version V2 des
  harnais ne gardait aucun gain (couverture descendue pour toutes les paires,
  trois descentes du citron par paire) et ma relance de contrôle validait
  l'atlas `joined` alors que la campagne chronométrée tourne en `live` ; les
  deux sont corrigés (couverture seulement pour les paires conservées ou
  soumises au lemme, une seule descente de décision, relance de la sonde en
  `live` sur chaque morceau), sorties toujours identiques à la V1. Mes sept
  morceaux sont identiques octet pour octet aux sept fichiers du constructeur
  (`receipts/lidar_spatial_20260921/spatial_tbmhj_zx/scene_00_000000/`) : le
  contrôle croisé porte sur le même objet que sa campagne. Sa campagne
  chronométrée (`spatial_9kscvyt0`, `--repeats 1`) a chevauché mes harnais
  (1,74 cœur effectif sur 4 pour son premier quart) : ses temps ne valent pas,
  ses compteurs si.

### Voie q4 à l'échelle de la scène : chaque record émis est une boule q4 valide (protocole bilatéral)

Reçu [README_Q4_BILATERAL.md](q34_stream_crosscheck_spatial_20260921/README_Q4_BILATERAL.md) :
là où l'énumération exhaustive de la voie q4 est hors de portée, chaque record
q4 émis par la sonde (moteur 34 gelé, atlas `live`, le mode de la campagne
chronométrée) est rejugé indépendamment (arête propriétaire, positivité
stricte en i128, profondeur exacte par descente sur la couverture de l'arête,
coquille complète comparée aux IDs), et la complétude est testée sur 2 000
paires tirées par morceau (énumération de toutes les boules propriétaires des
paires conservées). Sur les sept morceaux de la scène 0 à K5 : 189 486 records
sur les quarts, 190 180 sur les moitiés et **190 405 sur la scène entière,
tous valides** (0 propriétaire faux, 0 tétraèdre non positif, 0 profondeur ou
coquille différente), 0 boule manquante sur les paires tirées ; le harnais
coûte 1 à 72 s par morceau. À K10
([README_Q4_BILATERAL_K10.md](q34_stream_crosscheck_spatial_20260921/README_Q4_BILATERAL_K10.md)) :
2 143 012 records sur les quarts, 2 154 843 sur les moitiés et **2 158 063 sur
la scène entière, tous valides**, 0 boule manquante sur les paires tirées (1,36 Go
de records lus en flux depuis le disque ; le harnais y coûte 39 s). La
direction « complétude » reste celle d'un
échantillon (1 à 12 boules énumérées par morceau à K5, car peu de paires
tirées sont conservées) : l'énumération exhaustive des quarts, en cours,
la complète. En parallèle, la voie q3 exhaustive sur la moitié x+ (59 953
sites) donne les 528 575 boules du registre du constructeur, identiques à
l'énumération indépendante (ligne observée, reçu à venir avec la moitié x− et
la scène) ; le harnais y coûte 100 min, bien plus que projeté, car les paires
conservées à pleine densité portent des couvertures et des seeds nombreux.
Énumération exhaustive des deux voies sur le quart x+y+ (29 926 sites, K5,
ligne observée, reçu à la clôture des quatre quarts) : 270 202 boules q3 et
28 481 boules q4 distinctes identiques aux énumérations indépendantes, rejets
égaux au citron, lemmes sans violation ; le moteur émet 28 854 présentations
q4 pour ces 28 481 boules (373 plateaux cosphériques sur un vrai quart à
pleine densité, contre 3 sur le préfixe 8k/K10) et mon harnais 28 973
tétraèdres propriétaires positifs ; la comparaison porte sur les boules
distinctes, comme prévu. Le harnais q4 y a coûté trois heures (énumération
des tétraèdres à pleine densité) : les trois autres quarts prendront la nuit,
et la voie q4 des moitiés et de la scène reste couverte par le seul protocole
bilatéral.

### Protocole spatial (brouillon du constructeur) : préparation indépendante et harnais adaptés

Le protocole spatial demandé par l'utilisateur (scène brute dans son repère
capteur, moitiés x < 0 / x ≥ 0, quarts par y, tous mesurés) remplace les
préfixes sous-échantillonnés comme expérience principale de croissance. Pour
le contre-vérifier sans hériter de la préparation du constructeur, le dossier
[q34_stream_crosscheck_spatial_20260921/](q34_stream_crosscheck_spatial_20260921/SPATIAL_PIECES_SCAN0.json)
contient une préparation indépendante de la scène 0 (`prepare_spatial_b.py` :
quantification floor(50·x + 32768 + 1/2) en rationnels exacts depuis le
float32, déduplication globale avant découpe, plans qx = qy = 32768 avec les
sites du plan du côté ≥) : 119 142 sites uniques et 4 247 fusions, exactement
les effectifs de la préparation A du 14 septembre ; moitiés 59 189 et 59 953
sites, quarts 29 128 / 30 061 / 30 027 / 29 926 ; 80 sites sur le plan x et 54
sur le plan y. Ces sept effectifs et empreintes sont à confronter au manifeste
du constructeur dès qu'il sera commis ; toute différence sera un écart de
convention à expliquer, pas une qualification.

Les deux harnais de flux ont une version V2 à descentes d'index (décision du
citron par la descente saturante « milieu d'abord », couverture par descente
avec la borne inférieure séparable de |2z − a − b|² sur une boîte) qui
reproduit à l'octet près les sorties V1 (supports et profondeurs q3, clés et
profondeurs q4) sur les préfixes 1k/2k/4k à K5 et 1k/2k à K10 ; leur coût par
paire ne dépend plus de n, ce qui met un quart de scène (≈ 30 000 sites) à
portée d'un contrôle croisé exact en moins d'une heure par voie, les moitiés
en quelques heures, la scène entière hors de portée. La campagne sur les
quatre quarts à K5 (moteur 34 gelé, modes `rectangle-pair`/`boxes`/`affine`/
`joined`, relance en `live`) démarre après la clôture de la capture
chronométrée du constructeur ; ses reçus suivront.

Processus de B sur la machine : ces harnais occupent un cœur par campagne
(annoncées ici avec leur heure de départ : quarts, deux voies, et moitiés
et scène, voie q3 seule, relancés à 19:09 UTC après qu'un redémarrage de
session vers 17:30 UTC a interrompu les campagnes du 12:26 sans en garder
les reçus, les lignes observées jusque-là étant citées comme telles dans
ce dialogue ; les runners écrivent désormais un reçu partiel après chaque
ligne ; durée attendue de plusieurs heures chacune) ; en dehors de ces campagnes B ne laisse
aucun processus actif. Une fenêtre libre pour les chronos isolés du
constructeur peut être demandée dans ce dialogue.

### Briques float32 natives (028a0f1d, 12d885d8, 9923a6b9) : relecture, dérivation manquante fournie

Quatre relectures indépendantes en lecture seule (clé de boule, ordre des
événements q4, exactitude des prédicats, question SharedPrefix du journal),
toutes consignées ici ; verdict d'ensemble : aucune erreur dans les trois
commits, une erreur dans mon propre dialogue, quatre points durs sur la
proposition SharedPrefix.

**Clé de boule** : aucune erreur. Le vecteur primitif (A, B, C) de A|Q|² + B·Q + C
en unité 2^−149, pgcd 1 et A > 0, est l'unique représentant de la sphère ;
le retrait des bits nuls terminaux par coefficient est réversible (le nombre
de bits retirés est stocké, encodage préfixe et bijectif), donc deux supports
d'une même sphère donnent la même clé et deux sphères distinctes des clés
différentes, ce que des exemples à exposants hétérogènes confirment
(0 écart entre Fractions et sonde). Bornes publiées valides mais lâches
(q3 : A ≤ 9M⁴ plutôt que 12M⁴, |C| ≈ 61M⁶ plutôt que 144M⁶ ; tout < 2^1677
pour une capacité de 2^1728). Deux réserves de provenance : le champ
`git_commit` des captures `float32_identity_20260921` vaut 12d885d8, commit
où les sources testées (`float32_ball_key.*`, `float32_q4_events.*`, la porte
et le lecteur) n'existent pas encore (elles entrent à 9923a6b9) ; et la
relecture exige les répertoires `build/v8_float32_identity_*` non versionnés :
reçus rejouables ici seulement, à dire dans leur README. Nuance : les « dix
boules communes aux trois arités » sont une seule configuration (centre 0,
rayon 5) sous dix similitudes ; ajouter une boule tri-arité à exposants
hétérogènes dans un même support.

**Ordre des événements q4** : aucune erreur ni contre-exemple (3 000 tirages
exacts, 300 égalités cosphériques construites, 600 perturbations d'un ulp,
signes justes ; degrés et largeurs annoncés exacts : Δ de degré 5 à sommes
partielles < 2^1397, produit naïf de degré 9 ≈ 2^2511 hors capacité). Mais la
note IDENTITE_FLOAT32 renvoie à ce dialogue pour une identité « acquise
mathématiquement ici » qui n'y figurait pas. La voici. Pour une seed aiguë
(a,b,x), d = b − a, u = x − a, n = d × u, G = |n|² > 0, et deux quatrièmes
points z₁, z₂ avec v_i = z_i − a, B_i = det(d, u, v_i) et P_i le numérateur
de la position du centre sur l'axe (t_i = P_i / (2G·B_i)), soit Δ le
déterminant 4 × 4 des lignes (w, |w|²) pour w = d, u, v₁, v₂. Avec c le centre
de (a,b,x,z₁) rapporté à a, l'opération de colonne C₄ ← C₄ − 2c·(C₁, C₂, C₃)
laisse Δ inchangé et met en quatrième colonne la puissance pow(w) = |w|² −
2c·w, nulle pour d, u, v₁ ; d'où Δ = pow(v₂)·det(d, u, v₁) = pow(v₂)·B₁. Or
pow(v₂) = P₂/G − 2t₁B₂ et t₁ = P₁/(2G·B₁), donc G·Δ = P₂B₁ − P₁B₂ et
sign(t₁ − t₂) = −sign(Δ)·sign(B₁)·sign(B₂), ce qu'implémente
`float32_q4_events.cpp`. L'égalité Δ = 0 est exactement la cosphéricité de z₁
et z₂ avec la seed. Deux points pour la suite : le corpus de 960 événements
n'a aucune mantisse aléatoire (petits entiers sous un 2^e commun ; sur des
mantisses aléatoires aux exposants LiDAR, 0 repli exact sur 182 tirages, sur
toute la plage 14 replis sur 70) : ajouter une famille à mantisses et
exposants aléatoires par coordonnée et publier le taux de repli ; et au
futur tri, classer les sites à B = 0 avant tout `std::sort`.

**Prédicats float32** : aucune erreur. Chaque mot binary32 fini devient un
entier exact dans l'unité globale 2^−149 (|entier| < 2^277, NaN et infinis
refusés deux fois, zéros signés confondus), l'arithmétique est un entier
signe-magnitude de 54 mots (1 728 bits) sans i128 ni flottant sur le chemin
exact, chaque opération vérifie sa capacité et lève une exception plutôt que
de tronquer ; bornes recalculées : puissance q3 ≤ 144M⁶ (1 676 bits),
accumulateur q4 ≤ 396M⁶ (1 677 bits), déterminant réduit ≤ 72M⁵ (1 397 bits),
maxima observés 1 671 bits sur des mots-coins : les float32 arbitraires
(exposants 2^−149 à 2^127, signes mêlés) tiennent, pas seulement une scène à
échelle commune. Dégénérescences décidées exactement (colinéaire, coplanaire,
centre sur facette, angle droit, doublons, sous-normaux, −0,0) ; le filtre
d'intervalles (nextafter, serrage à ±DBL_MIN) ne certifie jamais un zéro et
tient sous tout mode d'arrondi et FTZ/DAZ. Rejeu indépendant des binaires
épinglés sur 13 340 boules, 11 720 clés et 6 044 événements : 0 désaccord.
Deux risques : la discipline de compilation flottante (`-ffp-contract=off`,
`-fno-fast-math`, `-frounding-math`) n'existe que dans les lecteurs Python,
pas dans une cible CMake : un garde de compilation est requis avant tout
raccord au build ou au GPU ; et le repli entier local recalcule tous les
coefficients à 1 728 bits à chaque repli (plusieurs Ko de pile, produit
quadratique) : mesurer le taux de repli sur les trames réelles avant de
généraliser. Nuance : le prédicat q2 emploie une seconde arithmétique exacte
(unité 2^−298, accumulateur à 18 mots) ; deux schémas coexistent sur le
chemin de contact, à dire dans la note.

**Question SharedPrefix (enveloppe conditionnelle aux graines positives)** :
la proposition « non portée » du journal existe déjà dans l'arbre de travail
comme fichiers non suivis (`src/core/float32_q3_block.*`,
`src/lanes/float32_q3_census.*`), sans reçu ni test ; relus sans compilation,
contrôlés en rationnels exacts. Réponse à la question : oui, conv(a,b,X) est
une enveloppe valide de toutes les graines positives, sans hypothèse de
propriété d'arête. Avec d = b − a, u = x − a, D = |d|², E = |u|², F = d·u et G
= DE − F² = |d × u|², les coordonnées barycentriques du centre circonscrit
sont α = F|x − b|²/(2G), β = E(D − F)/(2G), ξ = D(E − F)/(2G) ; strictement
intérieur ⟺ F, D − F, E − F > 0 avec G > 0, exactement le test de validité de
`float32_ball.cpp` ; triangle rectangle : une coordonnée nulle (centre au
milieu de l'hypoténuse, sur le bord) ; obtus : une coordonnée négative, le
centre peut sortir de conv(a,b,X). Le resserrement par W/(2G) est valide aussi
(W = E(D − F)d + D(E − F)u, c = a + W/(2G) et non m + W/(2G) ; 0 violation sur
297 boîtes contenant une graine valide), et une intersection vide conv ∩ (a +
W/(2G)) prouverait qu'aucune graine positive n'existe dans X : le code replie
sur le hull et perd ce certificat gratuit. Quatre points durs.

(1) Erreur dans ce dialogue, corrigée ci-dessus (réponse à la question q3 du
journal 33) : « bilinéaire en (centre, z), donc bornée aux coins » est faux
pour le minimum. (z − a)·(z + a − 2c) est affine en c (les termes en c²
s'annulent) mais quadratique convexe en z : maximum aux coins de Z × C,
minimum au sommet borné z_i = clamp(c_i, Z_i) pour chaque extrémité de C_i.
Fixture : a = (0,0,0), b = (10,0,0), x = (5,6,0) (aigu, c = (5, 11/12, 0)), Z
= [0,10] × {0} × {0} : les 64 évaluations aux coins donnent [0, 0] (les coins
sont a et b), la vraie plage est [−25, 0], atteinte en z = (5,0,0), témoin
strict ; un port « coins seulement » sous-compterait et accepterait une boule
à tort. Le README de A et le code non suivi (minimum au sommet, maximum aux
extrémités) sont justes ; graver cette fixture (sommet intérieur à Z) dans
toute porte de bloc.

(2) L'enveloppe hull ∩ (a + W/(2G)) est nettement plus lâche que celle de A,
parce que W est évalué par produits d'intervalles corrélés puis divisé par un
G lui-même corrélé. La structure exacte est c = m + ξ·h(x) avec h(x) = x − a −
(F/D)d affine (intervalle exact sur une boîte, coefficients constants D·I −
dd^T) et ξ = D(E − F)/(2G) = λ/2 scalaire ; identités vérifiées sur 3 000
triangles : J = 4G, Q = 4(E − F), P = 2(Du − Fd), λ = DQ/J = 2ξ, (c − m)·d =
0. Fixture axiale de A (a = (20,20,20), b = (40,20,20), X = {(30, y, 20) : 32
≤ y ≤ 37}) : vrai hull des centres {30} × [21,833 ; 25,559] × {20},
constructeur c_x ∈ [24,983 ; 40], c_y ∈ [20,914 ; 31,156] ; puissance en z =
(30,31,20) : vraie plage [−101,29 ; −19,33], rendue par l'enveloppe resserrée
de A (décidée), constructeur [−424,44 ; 101,25] (indécis), A universelle
[−103,67 ; 21] (indécis). Rapports de largeur sur 297 boîtes, constructeur sur
A universelle : min 0,29, médiane 1,92, max 72 ; constructeur sur A resserrée
: min 1,25, médiane 4,75, max 155. Remplacer les trois quotients par axe par c
= m + ξ·h(X), ξ resserré par D(E − F)/(2G) quand G est certifié positif, en
conservant la relation J = DQ + R de A.

(3) La borne λ ∈ (0, 2/3] de A exige la propriété d'arête ; la voie
SharedPrefix n'en impose aucune, et y porter l'enveloppe universelle de A avec
2/3 serait faux : sous acuité seule, sup λ = 1 (non atteint), soit ξ < 1/2 ;
formule fermée ξ = cos X/(cos(A − B) + cos X), qui tend vers 1/2 quand A → 90°
et X → 0. Fixtures : a = (0,0,0), b = (4,0,0), x = (2,10,0) : λ = 24/25 ; b =
(2,0,0), x = (1,−51,−52) : λ = 5304/5305 ; sur la bissectrice x = (1, n, 0), b
= (2,0,0) : λ = 1 − 1/n². Sous propriété, maximum observé 245/377 ≈ 0,650 sur
60 000 tirages (2/3 à l'équilatéral). Graver la fixture à 24/25 dans toute
porte de bloc de la voie sans propriété, ou introduire la propriété avec un
test explicite.

(4) Repli quand G est ambigu (`gram.low ≤ 0`) ou petit : l'enveloppe est le
hull entier et le quotient ne resserre plus rien après intersection. X = (30,
[y_lo, 40], 20) sur la même seed : y_lo = 36 → c_y ∈ [23,12 ; 31,72] ; y_lo =
30 → c_y = [20, 40] = hull ; y_lo = 21 → c_x ∈ [20,02 ; 40], c_y = [20, 40] ;
y_lo = 20 → G.low = 0, hull [20, 40]² ; A universelle donne c_x = {30}, c_y ∈
[20, 30] dans tous ces cas (vrai hull c_y ∈ [20,955 ; 27,5]). Le repli m + [0,
1/2]·h(X) reste fini, sans division, et contient les centres de toutes les
graines aiguës : le prendre à la place du hull, ou l'y intersecter.

Nuances. (a) Largeurs : la puissance exacte q3 est de degré 6 (< 144M⁶ =
2^1676, 53 mots) ; le test (z − a)·(z + a − 2c) avec des extrémités de centre
dyadiques arrondies vers l'extérieur sur la grille 2^−149 est de degré 2
(trois termes < 2^559 en unité 2^−298, 18 mots : l'accumulateur du prédicat
q2), mais `fixed_signed.hpp` n'offre que `divided_exact` (reste non nul
refusé) : un chemin entier des bornes de bloc exigerait une division avec
reste ; sinon garder « ambigu ⇒ raffinement ou relais ». (b) Census
SharedPrefix relu (215 + 169 lignes) : a et b sautés seulement dans COUNT
(puissance nulle, aucun bloc les contenant ne peut être certifié intérieur),
graines invalides conservées comme témoins, feuille ambiguë ⇒ scission de X
avant consommation avec ticket (compte, curseur) gelé pour chaque enfant :
aucune erreur produit trouvée. Ne pas sauter les feuilles de X dans Z, elles
sont témoins les unes des autres (a = (0,0,0), b = (2,0,0), x₁ = (1, 3/2, 0),
x₂ = (1, 10, 0), toutes deux aiguës : puissance de x₁ dans la boule (a,b,x₂) =
−68/5 ; puissance nulle dans sa propre boule, donc tout Z ∋ x₁ a max ≥ 0 et
min ≤ 0 pour X ∋ x₁). (c) Mesurer `shared_splits` et `shared_frames` contre le
nombre de graines valides de X ; le journal l'annonce lui-même. (d) Documenter
c = a + W/(2G) = m + ξ·h(x) et λ = 2ξ dans la note q3 float32 pour relier les
notations (W, G) du constructeur et (P, J, Q, λ) de A. Combinaison recommandée
: enveloppe m + ξ·h(X), puis bornes par six paraboles ; fixtures d'égalité :
la fixture axiale de A (décision attendue [−101,29 ; −19,33]) et sa version
tournée par ((2,−2,1),(1,2,2),(−2,−1,2)) + 500.

Les fixtures ci-dessus (huit familles : sommet, axiale et sa rotation, λ, G
ambigu, témoins mutuels, barycentriques, identités) sont gravées en rationnels
exacts et rejouables dans
[q3_bloc_float32_fixtures_20260921/](q3_bloc_float32_fixtures_20260921/README.md)
(`run`/`read`, aucun assert), avec les valeurs attendues des trois enveloppes
et des décisions : matière directe pour la porte de bloc que le journal
annonce (« tests Fraction à venir »). Ma plage resserrée exacte pour la
fixture axiale est [−1722/17 ; −58/3], la vraie plage ; celle du README de A,
−486226/4800, est un peu plus lâche par ses conditions entières.

### Commit a005f8aa (census q3 float32 partagé, protocole LiDAR sans sol) : relecture à quatre lentilles

Quatre relectures indépendantes en lecture seule, sans cmake ni natif relancé
: sources commises contre mes quatre points durs, protocole sans sol, sources
web citées, reçus et analyse de croissance. Verdict : aucune affirmation
fausse ni défaut produit ; trois risques de portée et une dizaine de nuances,
ci-dessous.

**Sources commises** (`core/float32_q3_block.*`, `lanes/float32_q3_census.*`,
porte, mutations, sonde) : les huit points sont vérifiés ligne par ligne.
Enveloppe = boîte de hull(a,b,X), resserrée par a + W/(2G) seulement si G.low
> 0, replis sur le hull sans certificat (block.cpp 89-119) ; bornes par six
paraboles, minimum au sommet clamp(c, Z_i), maximum aux extrémités, par
extrémité de C_i, arrondi extérieur, 18 évaluations comptées et imposées par
la porte ; a et b sautés seulement comme feuilles Z du préfixe partagé et
comme graines au relais, graines invalides et sites de X conservés comme
témoins ; ticket (nœud X, compte, curseur) copié figé pour chaque enfant et
chaque graine, feuille Z ambiguë ⇒ scission de X avant consommation ; coquille
= traversée globale depuis la racine, exclusions strictes seulement ; identité
G − D(U − E) = E(D − E) > 0 ⇒ 0 < ξ < 1/2 exacte ; oracle Fraction indépendant
(système de Gram résolu par élimination, barycentriques strictement
positives), aucun assert nu, deux mutants tués par géométrie à code 0 ; les
trois options flottantes sont dans les deux lanceurs et les manifestes. Mes
fixtures rejouées sous -O : PASS ; F1 par le schéma du code rend [−25, 0] en
rationnels et, en double avec les mêmes primitives, [−25,000000000000366 ;
+6,75e−13] avec classify 0 (correct, a et b ∈ Z). Deux nuances : aucun mutant
compilé ne vise `float32_q3_block.cpp` (sommet → extrémité, intersection hull
omise, `gram.low < 0`, dénominateur G) alors que F1 et la fixture ±2 les
tueraient ; le plancher de couverture de la porte n'inclut ni
`gram_unresolved` ni `center_intersection_fallbacks`. Un risque chiffré : sur
les fixtures de la capture Release, 843 des 1 642 préparations partagées (51
%) tombent en repli hull et 85 intersections sont vides ; mon point dur (4)
n'est pas théorique. Publier ces taux dans la matrice 8k/16k/32k et les
comparer au repli m + [0, 1/2]·h(X). Formulation : « les seuls témoins sautés
sans borne sont a et b » vaut dans le préfixe partagé ; au compte individuel
ils sont testés exactement.

**Reçus et analyse de croissance** : les cinq commandes de lecture du README
rejouées telles quelles rendent 0 et reproduisent octet pour octet READBACK et
MUTATION_READBACKS (lecteurs LIVE : les trois builds épinglés non versionnés
sont requis, présents ici) ; les quatre SHA256 sont exacts ; tous les nombres
d'ANALYSE_CROISSANCE sont reproduits par un recalcul indépendant en Fractions
depuis les 36 records (12 séries, 24 doublements, 122 postes, maxima
×2,116563, ×2,000250, ×2,141703, ×2,217511, aucun ratio ≥ 4, aucun zéro →
non-zéro, tableau K10, deux tableaux de temps). Deux risques de portée. (1) La
matrice mesure le coût de rejet des graines saturées d'une seule chaîne
emboîtée (intérieur de la boule i = {j < i}, seul site frontière = la graine)
: K − 1 supports constants, Individual paie n − 2 préparations dont n − 6
saturées ; en partagé, `outside` = 0, coquilles de 3 IDs, coût additif (+32
visites, +2 préparations par doublement : une descente racine → feuille).
Avant toute lecture de « forte réduction de travail », ajouter une famille où
le nombre de supports acceptés croît avec n (graines réparties en 3D autour de
l'arête) et une ligne de couverture à l'échelle (outside partagés, taux de
repli exact, `gram_unresolved`). (2) Les chronos ont été pris pendant les
propres captures concurrentes du constructeur (release, sanitize et mutations
lancées 19:29:59, matrice 19:30:28 ; 13 des 36 chronos chevauchent la capture
de mutations) ; bruit ×1,35 à ×1,77 sur un travail d'index identique (colonne
32k : 13,19 à 23,32 ms) : aucun rapport de temps n'est lisible, ce que le
texte dit déjà mais en sous-décrivant la charge. Nuances : la ligne «
évaluations paraboliques » est `power_evaluations` (`axis_parabolas` vaut 672
732 → 1 440 792, même ratio par structure) ; `slab` ne diffère de `column` que
pour le pré-tri et le préfixe partagé (compteurs Individual et digests
identiques ; x jusqu'à ±0,49 pour une arête de longueur 2) ; « l'index reste
payé en O(n log n) » est invoqué du tri, pas établi par deux doublements
(ratios 2,12 à 2,22 contre 2,14 à 2,15 prédits) ; `git_commit` vaut e2b09f94
pour release et sanitize, 74fb0a6a pour la matrice, absent des mutations, et 8
des 16 hashes épinglés sont absents de ces deux commits : les sources testées
sont celles de a005f8aa, à écrire ; `read` et `selftest` du lecteur matrice
sont le même chemin de code (quatre lectures × deux modes Python, pas huit
relectures).

**Protocole LiDAR sans sol** (`docs/LIDAR_SANS_SOL_PROTOCOLE_20260921.md`,
décision utilisateur reprise dans `AGENTS.md`) : cohérent avec les règles
contrôlées (float32 et IDs d'origine conservés, aucune reconstruction, labels
hors du calcul, GCP non utilisé, s ∈ {8, 10, 12}, 8k/16k/32k gardés comme
diagnostics) ; les 14 pages citées ont été relues (deux requêtes indépendantes
pour les chiffres) : fréquences, CPU, threads, IoU 94,78 moyenne des séquences
00-10, grille 0,33 m, licences BSD-2 et BSD-3, API sans en-tête ROS, défauts
discordants (th_seeds 0,5 contre 0,125 ; elevation_thr), classes
SemanticKITTI, protocoles d'évaluation : tout vérifié, rien de contredit.
Trois risques. (1) Le protocole subordonne les chronos HGP à une évaluation de
segmentation contre les fichiers `.label`, qui n'existent pas localement
(`labels_downloaded = False`), alors que la consigne que j'ai reçue est « pas
de confrontation à la vérité terrain pour l'instant, clustering seulement » ;
découpler : campagnes HGP appariées brute/sans sol dès maintenant sur un
masque à paramètres fixés a priori (défauts effectivement chargés publiés,
masque haché), l'évaluation contre labels devenant un diagnostic séparé,
ultérieur, soumis à la décision de télécharger les labels ; et régler méthode
et paramètres sur d'autres séquences que 08, qui est le split de validation.
(2) L'exposant par relation parent/enfant du protocole spatial est repris tel
quel ; ma critique (biais dominé par le profil de densité, estimateur poolé
par parent, dispersion entre frères comme barre d'erreur, déficit croisé de
frontière, axe densité compagnon) s'applique davantage encore, le retrait du
sol modifiant précisément les profils de densité. (3) Aucune métrique brute →
sans sol sur un même morceau n'est définie, alors que c'est la question du
régime : publier par morceau les effectifs et le rapport de travail par
compteur de tête, et un compteur explicite des boules acceptées sans sol dont
la profondeur brute dépassait le seuil (candidats créés par le retrait), avec
une fixture gravée à petite taille. Nuances : « bits XYZ float32 » = après
normalisation de −0 en +0 (comptée, nulle sur les trames présentes) ; sept
scans sont présents (000000 à 000004 consécutifs avec poses KITTI odometry,
000100, 000200), trois préparés, `data/` non versionné ; destination de
l'adaptateur et licences des dépendances liées à nommer (bench v8 sous MIT,
hors produit) ; cadre de statut absent en tête du document ; format et hachage
du masque non spécifiés, et la politique des doublons XYZ n'est exerçable par
aucune trame disponible (fixture à graver) ; renvois de section trop larges
pour GroundGrid (§IV-A, IV-D) et URL de branches mobiles à épingler par SHA et
date. La ligne « A/B : vigilance sur le changement de témoins » du journal est
une remarque du constructeur, pas une lecture d'audit : la présente section en
tient lieu.

## Erreurs et points durs relevés (à 4dbe3024)

1. **Session G4 R2 : diagnostic non établi.** La capture
   `receipts/lidar_global_20260921/gcp_r2_gate_failure/` montre
   `mhgp8_wspd_q34_gate --selftest` sans aucune sortie pendant 350,6 s puis
   tué (exit −15) par la session, sans chien de garde par commande ni trace de
   pile ; arrêt certifié `TERMINATED`. La suspicion « rational/int == 0 sous
   ancien Boost/C++20 » est plausible mais non prouvée : avant de conclure,
   reproduire sur la chaîne de la VM avec `timeout -s ABRT` et une trace
   (`gdb -batch`), ou un chien de garde par commande dans le worker. La
   réécriture par `numerator() == 0` est équivalente pour un rationnel
   normalisé (dénominateur > 0 garanti par Boost), donc sans risque, mais elle
   ne vaut correction que si une session passe la même porte ; la clôture 31
   déclare R3 `COMPLETED` (cinq mesures, arrêt certifié) sans dire si cette
   porte y a été rejouée : le préciser.
2. **Chiffre sans reçu.** « 210 987 arêtes, 83,307 M incidences, environ
   10,5 s » (journal, pilote 1000/K10) n'a pas de capture dans
   `receipts/lidar_global_20260921/` : le doter d'un reçu ou le retirer.
3. **Formulation d'ETAT_COURANT** : « la contrelecture A confirme
   indépendamment le citron » désigne une preuve sur papier ; la campagne
   exécutable de A est en préparation (son essai LSan a échoué), la mienne
   ci-dessus couvre q3 par instances. Écrire « preuve indépendante de A,
   vérification exécutable q3 par B, q4 en attente ».
4. **Compteur `owner_tests`** (`wspd_q34.cpp`, deux incréments par seed aigu,
   un par test de propriétaire) : sémantique acceptable, à noter dans le
   registre pour que `owner_tests ≥ acute_seeds` ne soit pas lu comme une
   anomalie.
5. **Budgets par défaut de Local28** exercés par la porte sur deux appels
   seulement : ajouter des fixtures bornées où le budget par défaut mord
   (frontière active saturée) et où il ne mord pas, avec planchers.
6. **Archive de 28 256 326 octets** (`global_vwtz76da.tar.gz`) dans les reçus :
   sa relecture n'est pas `--check-live` et ses entrées ne sont pas dans le
   dépôt ; garder l'archive hors Git (empreinte seule dans le reçu) pour ne
   pas alourdir l'historique sans rejouabilité.
7. **`--check-live` des tranches 22 à 30** échoue sur l'arbre courant (sources
   et CMake modifiés par la tranche 31, attendu) : chaque README de reçu
   devrait nommer le commit auquel sa relecture vivante s'applique, pour que
   le lecteur sache faire `git worktree add --detach <commit>`.
8. **Clé de fixture fausse** dans le contrat 31 (Triangle((0,0,0),(2,0,0),(1,1,1)),
   voir ci-dessus) : à corriger avant de graver la fixture.
9. **Rédaction** : Q3_Q4_COVERS_PARTAGES (§ couverture) parle d'« un minimum
   de distance [qui] dépasse D² » ; comparer des carrés à des carrés
   (|2z−a−b|² à 4D²) comme dans `edge_cover.hpp`.

Journal et sessions GCP : la mention « aucune VM démarrée » précède les
sessions R1 (07:17 UTC) et R2 (07:25 UTC) que l'entrée suivante déclare ;
lecture chronologique correcte, rien à corriger. Les deux captures sont
conservées et les arrêts certifiés `TERMINATED` sur leur cible ; GCP non
vérifié en direct par B (pas de `gcloud` ici).

## Entretien du dossier

- Fichiers de B : ce dialogue ; les notes datées
  [REGIME_WSPD_20260914.md](REGIME_WSPD_20260914.md),
  [PROPAGATION_TEMOINS_20260914.md](PROPAGATION_TEMOINS_20260914.md),
  [CREDITS_TERMINAUX_20260914.md](CREDITS_TERMINAUX_20260914.md),
  [BUDGET_CONTRAT_50K_20260914.md](BUDGET_CONTRAT_50K_20260914.md),
  [PORTES_ET_TESTS_20260914.md](PORTES_ET_TESTS_20260914.md),
  [VERROUS_MATHEMATIQUES_20260914.md](VERROUS_MATHEMATIQUES_20260914.md),
  [SEPARATION_20260914.md](SEPARATION_20260914.md), chacune coiffée ce jour
  d'un statut (historique ou acquis) ; les reçus immuables
  `wspd_regime_20260914/`, `propagation_temoins_20260914/`,
  `credits_terminaux_20260914/`, `chaine_q2_20260914/`,
  `separation_20260914/`, `surproposition_20260915/`,
  `plafond_proposeur_20260915/`, `oracle_q3q4_20260915/`,
  `front_lanes_lidar_20260921/`, `q3_stream_crosscheck_20260921/`,
  `q4_stream_crosscheck_20260921/`, `q34_stream_crosscheck_t32_20260921/`,
  `q34_stream_crosscheck_t32_8k_20260921/`, `q34_stream_crosscheck_t33_20260921/`,
  `q34_stream_crosscheck_t34_20260921/`, `relais_temoins_20260921/` et
  `q34_stream_crosscheck_spatial_20260921/`.
- Rien n'est supprimé ni déplacé : chaque ancien fichier est cité par un reçu
  immuable, une note du constructeur ou le journal (précédent
  `P0_OWNER_CHECKS.json` à ne pas répéter). Les sections antérieures de ce
  dialogue (14 au 17 septembre) sont condensées ci-dessous ; leur texte
  intégral se lit par `git show 36bef318:morsehgp3D_v8/audits/DIALOGUE_AUDITEUR_B.md`.
- Ne pas déplacer `P0_INPUT_ALIAS_CHECKS.json`, `p0_q2_census_bounds_probe.py`
  ni `p0_collective_probe.py` (épinglés ou importés ailleurs).
- Contrôles : Markdown de ce dossier validés par la fonction `validate` de
  `tools/check_docs.py` ; reçus rejoués en `python3 -O` ; aucun fichier des
  autres acteurs modifié.

## Historique condensé de l'audit B (14 au 17 septembre)

Chaque tranche q2 publiée a été confrontée à la force brute exacte sur des
sources identiques, haché par haché, aux blobs du commit indiqué ; reçus
dans `chaine_q2_20260914/`, rejouables par `git archive <commit>`.

| Tranche | Commit | Reçu | Résultat |
| --- | --- | --- | --- |
| Front + census q2, frère, ordre Complement | e3af11a7 | `CHAINE_Q2_CHECKS.json` | 86 × 8 combinaisons, 104,7 M paires, 0 désaccord |
| Census conjoint A × B | b2106c3c | `CHAINE_Q2_JOINT_CHECKS.json` | 86 × 18, 235,7 M paires, 0 désaccord, admission conjointe nulle comme prédit |
| Filtre Pool terminal | ba11e3ab | `CHAINE_Q2_POOL_CHECKS.json` | 86 × 25, 327,3 M paires, 0 désaccord ; seuil 64 = 99,94 % de la masse filtrable |
| Workers du front et du census | b268cf6f | `CHAINE_Q2_PARALLEL_CHECKS.json`, `…_BALANCE_…` | 4 348 appels, W ∈ {1,2,3,4,8}, sorties identiques ; ×4,1 à ×4,9 à W = 8 |
| Redistribution dynamique | 4e878754 | `CHAINE_Q2_DONATE_CHECKS.json` | 13 044 appels, 1,95 M dons repris, 0 blocage |
| Continuations à ancre unique | d09e2207 | `CHAINE_Q2_RESUME_CHECKS.json` | 258 624 continuations, 0 désaccord |
| Détachement intérieur | 897085f8 | `CHAINE_Q2_DETACH_CHECKS.json` | 63 120 lignées, 504 960 appels parallèles, 0 désaccord |
| Équipe persistante | beee3341 | `CHAINE_Q2_COOP_CHECKS.json`, `…_COOP_SCALE_…` | 5 304 appels, 0 désaccord, 258 exceptions propagées |
| Plages d'ancres et Pool partagé | 2741d614 | `CHAINE_Q2_RANGES_CHECKS.json` | 5 256 appels, 0 désaccord |
| Lots singleton (sources en chantier) | — | `CHAINE_Q2_BATCHED_*` | obligations des feuilles B compactées tenues |

Mesures et résultats acquis à côté : régime WSPD v4 et convention de
séparation (`wspd_regime_20260914/`), lentille réfutée et propagation
chiffrée du premier front (`propagation_temoins_20260914/`), crédits
terminaux et survivantes de Pool sur les amas (`credits_terminaux_20260914/`),
séparation s ∈ {8, 10, 12} : même objet, s = 8 confirmé
(`separation_20260914/`), bilan net de la surproposition sur copie patchée
(fenêtre 2K : temps q2 à 42 à 68 % de la référence hors rangées,
`surproposition_20260915/`), plafond de tout proposeur de témoins q2
(`plafond_proposeur_20260915/`), oracle q3/q4 i128 identique au catalogue
rationnel de `reference/` sur 315 petits nuages (`oracle_q3q4_20260915/`),
campagne adversariale sur 1bf806f0 sans défaut d'exactitude survivant.
Corrections acquittées : chiffres de la note des crédits, invariant I1,
argument des rangées (cordes 2u, témoins W3 pour u > D/√3). Les tranches 19
à 21 (8d615cfd, 8190e7ab, 3e94c868) ont été livrées par B en tant que
constructeur et relèvent des contrelectures de A et de l'auditeur
complémentaire, pas de ce canal.
