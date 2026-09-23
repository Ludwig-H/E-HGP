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
