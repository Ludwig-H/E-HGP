# S2 ne ferme pas encore la croissance des formes q3/q4

23 septembre 2026. Relecture B des reçus CPU locaux
[sans sol 8k/16k/32k](../receipts/lidar_scaling_local_20260923/README.md)
et des [trois densités de la trame brute K10](lidar_raw_k10_density_20260923/README.md),
en regard du raccord batch S2 **WIP**. Aucun GCP lancé ici. Ces nuages
8k⊂16k⊂32k sont des **disques emboîtés**, pas les moitiés/quarts
spatiaux par plans du capteur ; la décimation de la trame brute par ID
est encore une autre expérience. Toutes ces mesures sont finies,
mono-séquence 08 et `complete_relative`, sans preuve d'asymptotique.

Sur le sans-sol 08/000200, s8/W8, les ratios aux doublements successifs
sont :

| K | Sites | Paires développées | Formes cœur (`core_sites`) | Formes cover (`cover_sites`) | Supports q3+q4 émis |
| ---: | --- | ---: | ---: | ---: | ---: |
| 5 | 8k→16k | ×2,40 | ×2,72 | ×2,60 | ×1,89 |
| 5 | 16k→32k | **×4,81** | **×8,27** | ×3,81 | ×1,83 |
| 10 | 8k→16k | ×2,20 | ×2,41 | ×2,63 | ×1,88 |
| 10 | 16k→32k | **×4,27** | **×7,25** | ×3,74 | ×1,84 |

Les [JSON bruts K5](../receipts/lidar_scaling_local_20260923/out/s02_k5_w8_r0/)
et [K10](../receipts/lidar_scaling_local_20260923/out/s02_k10_w8_r0/)
donnent les masses ; `core_sites` et `cover_sites` additionnent les
**populations des cœurs/covers construits, extrémités comprises**. Avec
les leviers actifs de ce reçu, `dead_core_form_sites` et
`dead_form_sites` comptent séparément les formes réellement chargées
hors extrémités. À K5, la fraction de la masse de paires résiduelle WSPD
effectivement développée monte de **15,4 %→17,7 %→28,4 %**. Les
formes par charge du cœur montent de **54,0→71,7→191,1** et les
formes par cover de **176,5→230,9→419,4**. À K10, les formes par
charge montent de **94,5→111,3→263,5** ; par cover,
**273,5→354,2→610,7**. Le nombre de supports émis reste doux,
mais ne reflète donc pas le travail du cœur et du cover.

Sur la [trame brute 08/000000](lidar_raw_k10_density_20260923/README.md),
le doublement 61 694→123 389 retours donne encore **×4,34 formes cœur
à K5** et ×3,79 à K10. À pleine taille K10, les comptes bruts sont
**37 868 819 paires** développées, **1 254 254 109 incidences site–cœur**,
**1 074 719 197 incidences site–cover**, **11 387 391 boules** de catalogue,
**8,219 Gio** de RSS et **905,514 CPU·s** locaux. Ce n'est qu'une
trame brute d'une seule séquence, sans GPU, mais c'est déjà un coût
absolu déterminant pour le budget d'une seconde.
Le même `full.stdout` rapporte **1 238 630 455** formes de cœur et
**1 065 598 257** formes de cover effectivement chargées hors extrémités,
soit **2 304 228 712** au total. Ne pas appeler les 2 328 973 306
incidences (extrémités incluses) autant de lectures de formes.

Le filtre S1 GPU accélère la décision des masques ; le batch S2 actuel
**ne réduit pas** les populations des cœurs/covers des paires survivantes,
ni leurs sorties et FULL. Une belle pente des émissions ou 43–107 ms
de filtre ne doivent donc pas être présentés comme une croissance
sous-quadratique **du calcul complet**. Le prochain reçu intégré doit
apparier CPU/GPU sur mêmes octets et juger `R`, masse WSPD, `P`, `S`,
charges de cœur, `core_sites`, `cover_sites`, sorties, catalogue,
digest FULL, CPU·s, mur et RSS/HBM, puis répéter sur plusieurs scènes
brutes et sans sol. Les ratios locaux supérieurs à quatre révèlent des
régimes à traiter ; ils ne prouvent pas non plus une loi quadratique
universelle.

## Prochain shadow à coût borné : le résidu réel après le filtre

**Correction mathématique préalable :** un seul jeu de gardes sur la
boîte entière des centres ne peut fermer une voie S2 survivante si
elle contient le disque nominal d'une de ses arêtes. Cela reproduirait
exactement le témoin « citron » déjà crédité par S2. Voir le
[lemme et sa contre-fixture de frontière](CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md).
Le shadow doit d'abord mesurer le clipping par la boîte réelle du
nuage, puis des **sous-cellules de centres avec gardes distincts** ;
segmenter seulement les arêtes et garder pour chaque segment son
disque entier ne sert pas. Aucune efficacité LiDAR n'est acquise.
Cette première mesure existe maintenant pour la trame brute entière
08/000000/K5 : sur les vrais survivants S2, seulement **1,447 % des
arêtes et 1,494 % des incidences cœur** appartiennent à une voie dont le
disque nominal déborde de la boîte globale réelle
([reçu apparié](bbox_clipping_s2_20260923/README.md)). C'est un
**plafond de potentiel**, et non un rejet effectif : la cellule unique
clippée globalement n'est donc pas prioritaire sur ce cas. Le shadow
utile doit vraiment subdiviser les centres, puis mesurer ses gardes
distincts, coût et formes économisées ; rien n'est encore connu à K10,
sans sol ou `s=10/12`.
Si l'objectif est d'éviter **tout le cœur** d'une arête, le crible est
encore plus sévère : toutes les voies actives doivent déborder, soit
**1,345 % des incidences** au plein, résultat que le script B et le
[reçu A sur une autre trace](precore_cell_screen_20260923/README.md)
retrouvent indépendamment.
Aux densités emboîtées 1/4→1/2→entière de cette même trame/K5, le
plafond d'incidences cœur baisse **2,823 %→2,319 %→1,494 %**. Cette
tendance finie n'est pas une loi sous-quadratique ni une mesure des
coupes spatiales ; elle réduit seulement l'intérêt de la cellule
globale dans ce panel.

La [domination par cellule et gardes](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md)
est déjà prouvée localement, mais son gain sur LiDAR n'est pas mesuré.
Le batch S2 crée précisément l'objet manquant pour le juger : une liste
ordonnée de **vraies arêtes survivantes** `E⊊A×B`, chacune avec son masque
q3/q4. La validation structurelle de `wspd_q34.cpp` parcourt déjà
rectangles et survivants ensemble en `O(R+S)` ; elle détruit ensuite les
rectangles et distribue les survivants par blocs arbitraires de 64. Un
shadow peut, dans ce même parcours, calculer sans nouvelle copie de `E`
les offsets des segments non vides et leurs extrema associatifs
(`a+b`, longueur² maximale, boîtes des deux côtés **réels**). Comparer
ensuite l'enveloppe des centres issue du produit entier avec celle de
`E`, en gardant propriétaire, masque et IDs exacts. Une boîte serrée peut
permettre `K−1` gardes q3 ou `K−2` gardes q4 là où les extrêmes de
`A×B` empêchent tout certificat. Le seuil q4 `K−2` ne protège **pas**
l'atlas partagé q3 ; pour une vue commune, exiger `K−1` gardes ou
conserver séparément la voie q3 entière.

Le [reçu S2 CPU par arête](edge_matched_core_20260923/README.md) précise
où cibler cette tentative sur le **brut** 08/000000/K5 : à pleine
densité, 157 012 arêtes traversant les quarts physiques ne sont que
3,94 % des charges du cœur mais portent 386,518 M de ses 559,662 M
formes ; 382,406 M de ces formes sont calculées sur des arêtes ensuite
fermées par le cœur. Un seuil descriptif de longueur `|ab|≥4 m` cible
80,41 % des formes avec 8,51 % des arêtes, mais n'est **pas** un
certificat et ne doit jamais devenir un rejet. Cibler les *tentatives*
de preuve sur ces charges lourdes, indépendamment de l'axe du capteur,
permet d'évaluer si l'objet `E` sauve vraiment le poste dominant.

**Ne pas confondre avec une nouvelle boule centrale universelle.** Si
`D=|b−a|²`, `m=(a+b)/2` et un témoin réel satisfait
`|g−m|²≤D/16`, alors, pour la paire ponctuelle `a,b`, le filtre
actuel a `4H=D−4|g−m|²≥3D/4` et
`Xi=|(b−a)×(g−m)|²≤D²/16`. Sa condition stricte q4 est donc
`2(4H)²≥18D²/16>16Xi` ; q3 est encore plus facile. De tels
témoins sont **déjà** crédités par la recherche ponctuelle jusqu'aux
feuilles dans la configuration `RectanglePair`/Affine de R12 (ou la
voie est déjà rejetée lorsque son seuil est atteint). Si la voie
q4 survit, il n'y en a pas `K−2` ; si q3 survit, pas `K−1`. Le fait que
94 % des formes du cœur soient ensuite calculées sur des voies fermées
ne justifie donc pas de simplement recompter cette boule centrale :
il faut des gardes dépendant d'une **cellule de centres plus petite**
ou une autre preuve conditionnelle, avec travail mesuré.

La porte est **économique avant d'être chronométrique** : sur les
rectangles lourds choisis par un budget de *tentatives* (jamais un quota
de candidats), publier `R,P,S`, tailles et masques des segments,
tests/gardes/cellules payés, voies certifiées, replis, et surtout les
`dead_core_form_sites` puis `dead_form_sites` des **arêtes réellement
épargnées**. Si une voie est éliminée mais l'autre garde le cœur ou le
cover, compter uniquement la préparation effectivement évitable. Le
shadow laisse le moteur inchangé et doit inclure brut/sans-sol,
8k/16k/32k puis trames entières, s8/10/12. Une baisse de paires déjà
rejetées par le filtre ne rembourse rien dans le cœur. Si le travail
certifié n'amortit pas la recherche des gardes et le transport des
segments, fermer la piste sans port GPU. Le crédit exact par nœuds
essayé **après** construction du cœur/cover a déjà régressé de +27 %
CPU à K5 et +32 % à K10 sur une coupe 16k ; ce shadow différent teste
un partage **entre arêtes avant le cœur**, sans refaire cette fausse
piste. Un résultat favorable demanderait encore une ablation ON/OFF
avec flux complet de candidats, catalogue clé par clé, tour et digest
identiques, plus CPU, mur G4 et HBM/RSS. L'étape industrielle suivante
reste le tuilage borné : la liste globale S2 ne l'est pas.

## Contrelecture du préfixe paresseux

Le [reçu A](lazy_prefix_dead_core_20260923/README.md) mesure directement,
sur le brut 08/000000/K5 aux trois densités, le plus grand ordinal de forme
lu par le prouveur. Ses deux seuls accès à `forms_[id]` sont le balayage
de la frontière et celui de `next`, constitué d'IDs déjà balayés : dans
l'ordre présent, le suffixe après cet ordinal est effectivement omissible.
J'ai recoupé les sommes, les pentes et les SHA du reçu : à pleine densité,
**242 981 833 / 559 661 741** formes du cœur (43,42 %) ne sont pas lues,
mais cela ne représente que **26,30 %** des formes cœur et cover réunies.
Le préfixe lu croît encore avec une pente finie **2,024** au dernier
doublement ; ni gain de temps ni croissance sous-quadratique n'est acquis.

Réserve sur le fichier d'analyse auxiliaire : `analyze.py` ne remplit
pour les classes `intersections` que `loads`, `forms` et le suffixe,
puis leur applique `summarize()` comme si tous les autres compteurs
étaient fournis. Ainsi les champs `post_closed_*`, `loads_full_prefix`,
`loads_short_prefix` et `depth2_*` de ces **sous-classes** sont trompeurs
(dans une classe `post_closed`, `post_closed_loads` sort même à zéro).
Le README et `RESULTS.long4m` emploient les compteurs agrégés corrects ;
ne pas exploiter les autres champs des intersections sans corriger
l'analyseur. Le reçu compact hache ses fichiers conservés, mais les
traces binaires, entrées et analyses détaillées restent sous `/tmp` :
la jointure par arête n'est pas autonome à partir du seul commit.

## Certificat exact avant le cœur — domaine des centres à subdiviser

Le parcours de validation S2 connaît déjà, pour chaque rectangle ouvert,
le segment contigu `E` de ses **arêtes réellement survivantes**, avant de
libérer les rectangles et de distribuer des grains arbitraires de 64.
Accumuler en un passage les boîtes des extrémités **réelles** de `E`
coûte `O(R+S)` au total, sans expanser à nouveau `A×B`. Pour une cellule
convexe fermée `C` couvrant tous les centres q3/q4 admissibles d'`E`,
des gardes `g` réels distincts et chacun de ses sommets `v`, essayer
la stricte comparaison exacte

`2|g−v|² < dist²(v, box(A_E)) + dist²(v, box(B_E))`.                `(†)`

Une enveloppe conservatrice des **présentations positives possédées par
leur plus longue arête** est la boîte des milieux des arêtes d'`E`,
élargie sur chaque axe d'un entier `r` tel que `8r²≥Dmax`, puis
intersectée avec le cube de coordonnées de l'index ; la borne de
variance barycentrique q4 est `|c−m|²≤D/8`, et q3 est plus serrée.
Une subdivision couvrante peut rendre `C` plus utile sans exclure
aucun centre. Cette enveloppe ne prétend rien pour un candidat non
possédé par l'arête considérée.

Le membre droit minore `|a−v|²+|b−v|²` pour toute arête `(a,b)∈E`.
La différence `2|g−c|²−|a−c|²−|b−c|²` est **affine** en `c` ; si `(†)`
tient pour tous les sommets, elle est négative dans toute `C`. À un
centre d'une boule passant par `a,b`, les deux dernières distances
valent le rayon au carré : `g` est donc strictement intérieur. Avec
`K−1` gardes distincts, cela ferme **ensemble** q3 et q4 sur `E`
avant toute forme du cœur ; `K−2` gardes ne ferment que q4. Un garde
pris hors des deux plages de nœuds `A,B` du rectangle ne peut être une
extrémité d'`E` et garantit le non-double-compte. Les gardes et les
boîtes doivent appartenir au même index et au même masque de nuage.

Un second minorant gratuit à ce même passage mérite d'être essayé **en
alternative sommet par sommet**. Stocker la boîte `M_E` des milieux
`m_e=(a+b)/2` et `Dmin=min_E|a−b|²`. L'identité
`(|a−v|²+|b−v|²)/2=|v−m_e|²+|a−b|²/4` donne, pour toute arête,
`L₂(v)=dist²(v,M_E)+Dmin/4 ≤ (|a−v|²+|b−v|²)/2`.
Avec `L₁(v)=[dist²(v,box(A_E))+dist²(v,box(B_E))]/2`, tester
`|g−v|² < max(L₁(v),L₂(v))` à **chaque** sommet de `C`. `L₁` et `L₂`
minorent tous deux la même quantité pour toutes les arêtes ; le maximum
reste sûr et peut sauver un segment d'arêtes longues dont les boîtes
séparées sont lâches. Les milieux demi-entiers et `Dmin/4` doivent être
comparés après multiplication entière commune, sans `float` ni
arrondi vers le mauvais côté. Le calcul des trois boîtes et de `Dmin`
reste `O(S)` ; l'intérêt et les visites de recherche de gardes restent
à mesurer sur les vrais segments S2.

L'[audit A sur les mêmes traces](precore_cell_screen_20260923/README.md)
ajoute un minorant **corrélé** plus fort à chaque sommet :
`F_E(v)=min_(a,b)∈E (|a−v|²+|b−v|²)/2`. Il domine `L₁` et `L₂`
sans rompre la preuve affine ; ses `min` sont réductibles en parallèle.
Une fixture exacte de A fait réussir quatre gardes là où les deux
bornes par boîtes échouent. Mais chaque cellule exige désormais
`|E|×nombre de sommets` évaluations, en plus de la recherche des gardes :
facturer ces opérations sur les vrais segments lourds et ne pas les
confondre avec la préparation de boîtes en un seul passage.
La [proposition B de crédit par nœuds](CERTIFICAT_B_NOEUDS_CORRELES_AVANT_COEUR_20260923.md)
combine ce `F_E` avec un majorant de distance de **tous** les sites
d'une boîte d'index : un seul test strict à chaque sommet crédite une
population entière de gardes distincts. Huit sous-cellules AABB
partagent au plus 27 sommets, donc un balayage `O(27S)` avant recherches
bornées, puis repli exact. C'est une preuve d'admission, **pas** un gain
LiDAR mesuré ni un remède à la masse `S` déjà développée par S2.

Forme d'implémentation exacte, vérifiée indépendamment : à chaque
sommet `v`, poser `w=2v`, `G₂=2g`, et doubler les coordonnées de toutes
les boîtes. Le test combiné devient, sans division,
`2‖G₂−w‖² < max(dist²(w,2A_E)+dist²(w,2B_E),
2dist²(w,2M_E)+2Dmin)`. Les bornes de `2M_E` sont les sommes entières
`a+b` ; conserver le signe **strict**. Si les cellules sont subdivisées
avec des sommets en quarts ou plus fins, augmenter l'échelle commune
et promouvoir les produits **avant** multiplication. Ce certificat ne
s'applique qu'aux centres des supports q3 aigus/q4 positifs possédés
par leur arête maximale ; ne pas l'étendre aux autres présentations ni
au profil float32 sans nouvelle borne. Pour K<3, ne jamais former
`K−2` dans un entier non signé.

Ce lemme est **sûr mais peut être redondant**. Pour une arête intérieure
dont le disque de centres nominal est contenu dans `C_E`, tout garde
strict pour la cellule est déjà un témoin singleton S2. Une voie
encore ouverte n'a pas le nombre requis de tels gardes. Les vrais
essais doivent donc exploiter une restriction certifiée des centres
(au moins l'intersection avec la boîte réelle du nuage) ou une
couverture en plusieurs cellules, chacune avec ses propres gardes.
La [preuve de redondance](CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md)
donne aussi le test entier O(S) pour chiffrer la portion à laquelle
une cellule unique pourrait seulement s'appliquer.

Ce test est seulement **suffisant**. Une cellule de centres trop large,
ou des boîtes d'extrémités trop lâches, peut le faire échouer alors
qu'aucune sortie n'existe ; subdiviser seulement les segments lourds
ou repasser au chemin exact. L'[exemple entier à huit gardes](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md)
montre qu'une division en deux demi-cellules peut fermer K5/q3+q4
alors que le citron ponctuel S2 ne crédite aucun de ces gardes : il
ne s'agit donc pas d'un simple recomptage du milieu. Mais sa fréquence
sur LiDAR, son coût de recherche des gardes et le taux de repli sont
**inconnus**. En shadow, compter les segments/voies effectivement
certifiés, les formes cœur et cover que leurs arêtes auraient réellement
chargées, ainsi que toutes les visites/boîtes/écritures supplémentaires.
Le critère ne remplace pas le tuilage borné du lot S2.
