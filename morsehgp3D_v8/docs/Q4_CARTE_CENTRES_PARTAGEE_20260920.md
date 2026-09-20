# Une carte de certificats commune aux familles d'une arête

20 septembre2026, tranche27 après2920b8b5. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `public_status=not_claimed`.

## Ce qui change

Les faces incidentes à une arête cherchent leurs centres dans un même
plan. Au lieu de reconstruire une recherche indépendante sur chacune,
une carte conserve les régions où assez de témoins sont toujours intérieurs.
Une face peut alors traverser plusieurs régions déjà certifiées, même
si les témoins changent d'une région à l'autre.

Le port suit la [proposition indépendante A2920b8b5](../audits/q4_center_blocks_20260920/README.md),
sans reprendre sa préparation scalaire du nuage ni hériter de ses mesures.
La comparaison constructeur garde le **même pool spatial** que26 : le
prototype A utilisait tous les sites du cover. Son gain ne se transfère
donc pas à cette composition. Les témoins de blocs entiers restent une
suite distincte, pas un travail gratuitement acquis.

L'entrée explicite `run_q34_mapped_edge_candidates` applique d'abord le
filtre26. La carte n'est construite qu'à la première famille q4 encore
vivante, puis réutilisée par toutes les suivantes. q3 est indépendant.
Une carte qui ne sait pas rejeter une famille conserve le calcul exact
à zéro, coquilles complètes ; aucun compte de carte n'y est ajouté.
Un budget de carte nul reproduit le chemin26, pas une sortie tronquée.

Cette carte est **un filtre, pas un census local réutilisable**. Elle peut
retirer un témoin dont le minimum est nul, car il ne fournit aucun crédit
strict. Pour un futur balayage local, ce même témoin peut porter un
événement ou une coquille : il devra rester actif. De même, le certificat
d'un parent comprimé ne constitue pas un compte exact. Le futur payload
devra donc être distinct et requalifié, sans transformer ces listes de
filtre en partition exacte de tous les sites du cover.

## Géométrie commune, sans flottants

Pour v=b−a, D=|v|², w=2z−a−b et t=2c−a−b :

$$t\cdot v=0,\qquad 4\Pi_c(z)=|w|^2-D-2w\cdot t.$$

La positivité et l'arête maximale donnent R²≤3D/8, donc |t|²≤D/2.
Une base entière A,B de v⊥ choisit l'axe dominant k, h=|v_k|, puis
A=h e_i−sign(v_k)v_i e_k et B=h e_j−sign(v_k)v_j e_k. Ses valeurs
propres de Gram sont h²,D ; D≤3h². Le carré [−2,2]² contient donc le
disque utile dans les coordonnées t=Aξ+Bη, sans racine ni normalisation.

Chaque témoin fournit une forme affine F(ξ,η). Sur une cellule fermée,
son maximum strictement négatif donne un crédit ; son minimum positif
ou nul retire le témoin sans crédit ; sinon il reste indécis. La droite
F_x=0 de la face n'est exclue que si les deux bornes ont un signe strict
commun. Les contacts aux frontières ne sont jamais éliminés.

## Domaine positif préparé par blocs

`Q4PositiveDomain` partage le cover et son index. Il parcourt les boîtes
spatiales pour construire l'AABB exacte des sites hors a,b dans les deux
boules fermées de rayon²D. Une boîte entièrement admise et sans extrémité
possible contribue d'un seul coup. Les boîtes indécises sont subdivisées.
Les coordonnées et IDs ne sont ni copiés ni parcourus systématiquement
un à un. Coût O(V), V≤2n−1, mémoire fixe ; ce n'est pas une borne globale
sur toutes les arêtes.

Les complétions **obtuses** sont conservées. Les huit coins projetés de
cette AABB, plus l'origine, donnent une enveloppe à neuf sommets au plus.
La positivité exprime t comme combinaison convexe des projections des
deux complétions et de0 ; cette enveloppe contient donc tous les centres
utiles. Un hull dégénéré entraîne un repli au disque, jamais un domaine
vide supposé. Moins de deux complétions exclut q4 seulement.

Une cellule sort du domaine si une facette est strictement violée par
tous ses points. La boîte3D des coins donne aussi un minorant de |t|².
Ces deux tests peuvent garder trop de cellules mais ne perdent pas de
centre admissible. La lentille borne les complétions, **pas les témoins** :
un point hors lentille peut compter dans le census et reste dans le cover.

## Raffinement à la demande et invariants de mémoire

Une cellule possède son compte et les indices de pool encore indécis.
Les enfants héritent de ce compte et de ces seuls indices ; un témoin
retiré n'est jamais reproposé ou crédité une seconde fois le long d'une
branche. Des frères ne cumulent pas leurs crédits. Les listes sont des indices
dans le pool immuable de l'arête, pas des coordonnées dupliquées.

La requête raffine seulement les cellules rencontrées par sa droite.
Un échec d'intersection est une réponse **locale à cette droite**, jamais
un état global « vide ». Les enfants non interrogés restent indécis.
Un parent ne peut être comprimé que lorsque ses quatre enfants sont
globalement profonds ou hors domaine. Leurs crédits ne s'additionnent pas.
La compression conserve les descripteurs déjà alloués et ne prétend pas
restituer cette capacité mémoire.
La première cellule indécise impose le repli de la famille ; aucune liste
de tous les morceaux de sa droite n'est nécessaire.

L'ordre des faces peut changer le raffinement et le nombre de rejets
certifiés, pas les sorties du calcul exact. Une carte est mutable et
exclusive : pas de requêtes concurrentes sur la même instance. Des arêtes
indépendantes peuvent posséder leur propre carte. Cette tranche ne livre
ni ordonnanceur multithread q3/q4 ni carte GPU ; une carte figée et des
plages de requêtes distribuables devront avoir leur qualification propre.

La limite de cellules/profondeur borne le travail de certification, pas
la recherche géométrique : épuisement ⇒ indécision ⇒ repli exact.
Le tableau de cellules croît amorti, sans réserver tout le budget.
Les indices copiés, listes non vides créées et agrandissements du tableau
ont leurs compteurs : les frères alloués mais jamais interrogés ont un
coût même sans test de témoin.
Les capacités des listes sont suivies en O(1), pas par scan du tableau
à chaque ajout. Les capacités parentales encore vivantes pendant un split
sont comptées ; allocations transitoires et métadonnées restent exclues.
Une exception interne pendant une requête acquise rend la carte non
reprenable ; les arguments invalides rejetés avant cette acquisition
n'empoisonnent pas la carte.

Le pic englobant distingue capacité maximale pendant la requête et
capacité conservée pendant le repli. Il les couple au workspace26 puis
aux buffers de cette **même face**, avant maximum sur l'arête ; il ne
somme pas aveuglément des maxima mesurés sur des faces différentes.
Nuage/index/pool partagés, callbacks et RSS restent hors de ce compteur.

## Largeurs arithmétiques et coût

Le dénominateur fixe q=2⁴⁴ représente exactement les subdivisions jusqu'à
la profondeur44. |α|,|β|≤2q ; les bornes de norme sont majorées par
128M²q²<2¹²⁷ avec M=65535. Les coefficients affines restent i64 ; les
évaluations dyadiques, produits projetés et orientations sont i128.
Les orientations du hull sont bornées par1152M⁶<2¹⁰⁷. Les facettes sont
stockées en3D pour éviter des produits de gros déterminants. Cette preuve
ne couvre pas un futur clipping de corde rationnelle propre à chaque seed.

Pour C témoins et T cellules créées, le coût de certification est au plus
O(T(C+1)), les requêtes coûtent leurs visites réellement exécutées, et le
stockage est au plus O(T(C+1)), y compris si C=0. S'ajoutent domaine, génération des faces,
filtre26 et traitement exact des familles restantes. Ces expressions
ne bornent ni T, ni le résidu utile en fonction de n sous-quadratiquement.

Cette composition conserve le préfixe26 par face, y compris ses tris
lorsqu'ils sont exécutés, avant la requête partagée. Avec le même pool,
le minimum collectif26 est déjà exact sur sa corde extérieure arrondie.
La carte Disk change surtout la surcouverture ; Positive enlève aussi
des centres impossibles. Ni l'une ni l'autre ne gagne de nouveaux témoins.
Une carte commune n'implique donc pas un gain dans cette composition.

## Qualification close et décision

88 CTests Release passent ; six gates et24 sondes passent sous Clang
ASan/UBSan, pas la suite complète88 sous sanitizers. La nouvelle gate
effectue1956 contrôles : census rationnel de la lentille, racines positives,
tangences, ordre des requêtes, coquille30, rotations, extrêmes u16 et
quatre allocations fautives injectées. Deux compressions profondes sont
exercées ; aucune compression entièrement hors domaine ne l'est.
Trois mutations compilées sont tuées causalement. Les20 comparaisons
avec le binaire26 gardent tous les champs non temporels identiques.
Les170 sources et les builds `v8_q4_center_map_20260920` et
`v8_q4_center_map_sanitize_20260920` sont désormais épinglés.

Les [reçus](../receipts/q4_center_map_20260920/README.md) ferment128 mesures
principales, dont48 grands fonds à8k/16k/32k, et48 filtres denses auxiliaires
distincts. Les lecteurs normal/−O concordent. À n256/C64, Positive/profondeur7
retire deux familles q4 supplémentaires à K5 :10 667→10 155 lectures,
mêmes14 sorties ; K10 garde21 266 lectures et54 sorties. Le surcoût de
carte est payé ; **aucun gain de vitesse stable n'est acquis**.

À32k/C64, le préfixe dense garde185,344M/357,472M lectures minimales
futures du repli à K5/10 ; la grille permutée123,776M/300,608M.
Ce repli n'est pas exécuté. Les derniers ratios du préfixe sont×9,694/×6,210 :
aucune garantie sous-quadratique. Le domaine positif ne paie que sept
visites de blocs et zéro test scalaire dans ces cas, mais cela ne clôt
ni le travail par face ni le résidu.

Décision : conserver l'entrée explicite comme expérience contrôlée,
sans changer le défaut. La suite doit combiner **blocs témoins et balayages
exacts locaux du résidu**, pas simplement augmenter profondeur ou pool.
Il faut un nouveau contrat : intérieurs certains max<0, extérieurs
certains min>0, contacts et indécis conservés, compte exact et identités
disjointes. Les comptes comprimés de ce filtre ne suffisent pas ; payer
aussi les copies d'actives, les frontières de cellules, le tri local et
les coquilles. Le dialogue mathématique avec A se poursuit sur ce point.

Les résultats du prototype indépendant restent séparés du produit.
FULL, raccord WSPD q3/q4 global, catalogue, intérieurs après regroupement,
GPU et contrats G4 restent ouverts. Le s8/10/12 ne figure pas dans cette
primitive à arête fournie. GCP non utilisé.
