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
341 par arête à 8k/K5, en accord), alors qu'une paire conservée porte 47 à
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
α4 = 2, inégalités strictes, contacts réalisables en u16). **Une phrase de sa
réponse est à préciser** : « utiliser α3 pour q4 aussi ». Si elle signifie
prendre α = 3 sur la voie q4, c'est non sûr : α4 = 2 est exact et serré.
Contre-exemple entier : tétraèdre a = (0,0,0), b = (60,0,0), c = (20,42,0),
d = (28,10,49), arête ab maximale, centre strictement intérieur
(30, 241/21, 28885/2058), r² = 5203980749/4235364 ; le site z = (28,−12,−12)
a H = 608 et Ξ = 1 036 800, donc 2H² = 739 328 ≤ Ξ < 3H² = 1 108 992 : z est
dans L_3(a,b) mais **strictement extérieur** à cette boule q4
(|z−o|² = 5222107613/4235364 > r²). Un front qui le créditerait pour q4
pourrait rejeter à tort l'arête. Vérifié à 4dbe3024 : `spindle/predicates.hpp`
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
autres que a, b, x (a et b ont c = 0, x n'est sur aucune couche),
strictement intérieurs : profondeur ≥ K − 1, boule non émise. Peler le seul
groupe positif suffit pour cette décision ; garder les duaux coïncidents,
les points d'arête et les enveloppes dégénérées, et séparer dans l'API le
seuil de sélection du seuil d'émission (le piège K + 1 → K − 1 signalé est
réel). Contre-fixture du mauvais seuil recalculée exactement : centre
(12, 65/6, 10), r² = 169/36, puissances −7/3 et +2/3, arêtes² 16/13/13,
aigu, ab maximale : conforme. Mais le coût ne suit pas : sur les paires
conservées de mes 18 exécutions, la part des sites de couverture qui sont
des seeds aiguës propriétaires vaut de 0,15 à 0,20 (stable sur les trois
scans, les trois tailles et les deux K), alors que le census par boîtes de
la série 32 coûte environ 35 bornes par seed (67,5 M bornes pour 1,91 M
seeds à 8k) : les couches coûtent (log₂ m + T)·m par arête, soit un seuil
de rentabilité seeds/m ≥ 0,27 à 0,53 selon l'exécution, jamais atteint.
Le filtre coûterait donc 1,5 à 3,3 fois le census qu'il évite. Ne pas le
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
  `front_lanes_lidar_20260921/`, `q3_stream_crosscheck_20260921/` et
  `q4_stream_crosscheck_20260921/`.
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
