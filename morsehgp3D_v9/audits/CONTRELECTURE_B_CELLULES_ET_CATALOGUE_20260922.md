# Contrelecture B — cellules locales, coût global et sortie réellement nécessaire

22 septembre 2026. Audit indépendant en lecture seule du moteur ; ce fichier
ne qualifie aucun algorithme v9. Il complète les notes [q3](Q3_STRUCTURE_ET_BORNES.md),
[q4](Q4_STRUCTURE_ET_BORNES.md) et [état A](ETAT_COURANT.md), sans les modifier.
Le [rapport de base v8](../../audits/morsehgp3D_v9/AUDIT_INITIAL_V8_20260922.md)
et les reçus cités fixent le périmètre. Aucun GCP ni benchmark lourd lancé.

## Verdict

Les deux lemmes locaux proposés sont **sûrs sous leurs préconditions** :
la différence de puissances `Δ_a,z(c)` est affine en centre ; `T=K−2`
gardes distinctes du même sous-nuage bornent le rayon des q4 admis dans
une cellule fermée. Contact `Δ=0` et égalité du gap restent actifs. Ces
lemmes ne constituent pas encore un **générateur complet**, ni une borne
sur le nombre de cellules, les sélections de gardes ou les sorties.

**Contrat précisé pendant cette contrelecture.** Les instructions
[AGENTS.md](../../AGENTS.md) historiques conservaient le float32 brut
par défaut et la grille 1 mm en option. La réponse explicite de
l'utilisateur du 22 septembre tranche pour la v9 : **grille entière
1 mm comme profil principal à qualifier**, float32 comme objectif
secondaire. Le préambule v9 d'AGENTS.md publié à `c399808e` donne
désormais préséance à cette décision ; les paragraphes v8 plus bas
restent historiques. Le [plan v9](https://github.com/Ludwig-H/E-HGP/blob/c399808e/morsehgp3D_v9/docs/PLAN_V9.md)
place le **sans-sol** en premier pour le contrat temps et le brut avec
sol dans une phase séparée ultérieure. Une réussite sans sol ne qualifie
donc pas la ligne brute ; une sonde q3/q4 ne qualifie aucune tour FULL
K1..Kmax. Ni les anciens reçus float32 ni ceux à 2 cm ne deviennent
des preuves 1 mm.

La v8 1 mm sans sol laisse 23,687 M paires développées, prépare 2,044 M
covers et 1,872 M atlas q4 sur 08/000000/K5/s8/W8, dont 1,545 M
partagés avec q3 ; 38,795 M cellules
coûtent 3,252 G bornes de blocs, 7,316 G tests ponctuels et 5,547 G
insertions d'IDs de frontière, avant un catalogue FULL. La cascade de
rectangles auditée récemment accélère son **filtre** mais conserve les
mêmes paires et masques : elle ne réduit donc pas ces nombres d'aval
sans certificat additionnel. La source est le
[reçu 1 mm](../../morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/only_probe_01_s00_k5_w8.json)
(`sha256=89be317e0459f179f04a4224160490812b3f9edaa52c75abec275349c644792c`),
(`work.q4_edges` et `work.local28.geometry.preparations` pour le total,
`work.q3_atlas.edges_with_atlas` pour la part q3), et la
[synthèse du filtre](https://github.com/Ludwig-H/E-HGP/blob/12294241/morsehgp3D_v8/audits/SYNTHESE_PRIORITES_LIDAR_20260922.md).
Ce reçu a été versionné au commit `3f0d188f` après la première lecture ;
il atteste **une seule ligne CPU q3/q4**, non une campagne de croissance,
un catalogue FULL ou un chrono G4.

## P0 mathématique : définir la couverture avant la paresse

Une création « à la demande » des cellules est correcte seulement avec
un invariant de couverture initial et une règle de subdivision complète.
Pour chaque support positif q≤4 admis, il faut prouver qu'il possède :

1. une arête maximale propriétaire, départagée par IDs en cas d'égalité ;
2. un produit du front contenant cette paire et non rejeté sur la voie q ;
3. un bloc de complétions contenant les autres sommets ;
4. une cellule possédée contenant son **centre exact**, dans une
   partition du domaine faisable de cette arête ;
5. soit une feuille qui examine ce support, soit un **certificat terminal
   valide pour toute la cellule** qui le rejette.

La note q3 propose parfois `a=min(ID des trois sommets)` comme ancre
d'émission, alors que le front route d'abord par **plus longue arête** :
ces deux choix ne coïncident pas toujours. Un partage de cellules par
ancre avant les graines singleton doit donc router ou dédupliquer les
trois possibilités avec preuve et coût explicites. Pour les tuiles
demi-ouvertes, les faces maximales de la boîte globale doivent appartenir
à une dernière tuile fermée (ou à une règle lexicographique équivalente),
sinon un centre exactement sur la borne supérieure n'a pas de propriétaire.
Fixture minimale à conserver : IDs 0,1,2 aux coordonnées respectives
`(10,13,10)`, `(8,10,10)`, `(12,10,10)`. Le triangle est strictement
aigu ; l'arête maximale est **1–2** (carré 16 contre 13), tandis que
l'ancre `minID` est **0**. Son centre exact est `(10,10+5/6,10)`.

La note q3 énonce une partie de cette obligation ; la note q4 prouve le
filtre d'une cellule **déjà fournie** mais ne fournit pas encore le
mécanisme exhaustif qui crée toutes les cellules utiles. La voie v8 peut
rester l'oracle/repli complet pendant le port. Une palette de gardes,
une limite de profondeur, une file pleine ou une cellule « difficile »
ne ferment jamais une tâche sans la preuve géométrique. Les cellules
fermées servent aux bornes ; une règle demi-ouverte distincte choisit
l'émetteur unique sur leurs frontières.

La cellule initiale proposée comme boîte englobante globale du nuage
illustre le verrou : **chaque boîte Z de l'index est déjà dans C**, donc
`gap²(C,box Z)=0`. Le certificat de rayon ne peut écarter **aucun** Z
à cette racine, quels que soient les gardes. Le gain commence après
localisation ou subdivision des centres ; il faut expliquer comment ces
petites cellules naissent **sans avoir déjà payé** toutes les graines et
atlas v8. Une cellule non demandée n'est omissible que si la couverture
des supports prouve qu'elle ne contient aucune sortie admissible.

Il faut publier également **la somme** des produits et sous-domaines
ouverts, pas seulement le coût moyen d'une cellule favorable. Sinon on
peut déplacer le carré de la liste des arêtes vers la construction des
cellules, leurs consultations d'index ou les mêmes sites actifs répétés.
Le compteur doit inclure les **incidences produit×cellule avec
multiplicité**, les tentatives d'enveloppe, routages et centres localisés,
pas seulement les cellules distinctes mises en cache.

## Proposition B : enveloppe de centres et rejet au niveau du rectangle

Il existe une racine de centres **plus locale que la boîte du nuage entier**
qui ne demande pas de développer `A×B`. Pour un support positif de `q`
sites (`q=3` ou `4`), soit `ab` son arête propriétaire la plus longue,
`D=|a−b|²`, `m=(a+b)/2`, et `c` son centre. Ses poids barycentriques
strictement positifs donnent

`R² = (1/2) Σ_{i,j} λ_i λ_j |p_i−p_j|² ≤ D(q−1)/(2q)`.

Comme `a,b` sont sur la même sphère,
`R²=D/4+|c−m|²`. Ainsi
`|c−m|²≤D(q−2)/(4q)` : **D/12 pour q3, D/8 pour q4**. Les bornes sont
atteintes par le triangle équilatéral et le tétraèdre régulier ; les
égalités de longueur d'arête ne gênent pas si la propriété est départagée
par IDs. Ce raisonnement ne dépend pas d'un choix de troisième/quatrième
sommet ni du nombre de paires du rectangle.

Pour le produit WSPD `A×B`, la boîte `M` des milieux est calculée
directement par coordonnées
`[(A_lo+B_lo)/2,(A_hi+B_hi)/2]`. Soit `D_max` un majorant certifié des
carrés `|a−b|²` sur le **produit des boîtes continues**. Tous ses centres
q4 positifs sont dans
la **somme de Minkowski** `C=M⊕Ball(0,r)` avec `r²=D_max/8` ; pour q3,
`r²=D_max/12`. Cette enveloppe euclidienne arrondie se calcule depuis
les boîtes du produit **sans parcourir ses paires**. Une boîte XYZ
extérieure peut servir à parcourir l'index, mais ne doit pas remplacer
`C` dans le certificat de gardes ci-dessous. Le centre positif appartient
aussi à l'enveloppe convexe de ses supports, donc à la boîte globale du
nuage. La localisation ne prouve **ni** la génération des complétions
**ni** une borne sur le nombre de sous-cellules.

Cette enveloppe fournit aussi un rejet universel exact à essayer **avant**
`A×B`. Soit `D_min` un minorant certifié de `|a−b|²` sur le produit, et
`T_q=Kmax+2−q>0` gardes distinctes du même nuage. Pour chaque garde
`g`, poser `v_g=max_{m∈coins(M)}|g−m|²` et `w=r²` : le maximum exact sur
`C` est `(√v_g+√w)²`. Poser `U_C` comme maximum de cette expression sur
les `T_q` gardes.
Si **`D_min/4>U_C`**, toute boule de la voie q passant par une paire du
produit a `R²≥|a−b|²/4≥D_min/4>U_C` ; les `T_q` gardes sont donc toutes
strictement intérieures. Le rectangle entier est rejeté **sur cette voie
seulement**. Un contact ou une égalité dans ce test conserve le produit.
Même si une garde appartient à `A` ou `B`, le test ne peut alors réussir
pour une paire qui l'utilise comme support : elle serait sur la coquille,
ce qui contredirait l'inégalité stricte. Aucun crédit n'est transmis à
une autre voie, ni ajouté aux `h`, `h_a`, `h_b` existants sans preuve de
disjonction.

Ce rejet se décide **sans calculer une racine** : pour chaque garde,
`δ=D_min/4`, exiger `δ−v_g−w>0` puis
`(δ−v_g−w)²>4v_g w`, en rationnels exacts. Une garde qui échoue laisse
le produit vivant. Les gardes doivent être des sites géométriques
distincts ; si le sous-nuage en contient moins que `T_q`, il faut un
repli. `D_min` peut être le gap carré des boîtes A/B, parfois très lâche.
Sur la grille entière, une forme sans division pour q4 est disponible :
`V_g=4v_g` calculé aux huit coins de `M`,
`L_g=2D_min−2V_g−D_max` ; exiger
`L_g>0` et `L_g²>8V_gD_max`. Pour q3, poser
`L_g=3D_min−3V_g−D_max` et exiger
`L_g>0` et `L_g²>12V_gD_max`. Ces produits demandent une preuve de
largeur et de domaine avant leur port u18 ; le float32 secondaire exige
ses rationnels dyadiques exacts, pas le recyclage de cette preuve.
Petite fixture arithmétique non vacueuse :
`A={(-10,0,0),(-9,0,0)}`, `B={(10,0,0),(11,0,0)}` et les trois
gardes distinctes `(0,0,0)`, `(0,1,0)`, `(0,0,1)` donnent
`D_min=361`, `D_max=441`, `V_g≤8`. Pour q4/Kmax=5, le pire cas a
`L_g=265`, donc `L_g²=70225>28224=8V_gD_max` : le rejet du rectangle
réussit. Cette fixture teste l'arithmétique du certificat, **pas** son
gain sur un scan LiDAR.

**Fausse piste éliminée par la contrelecture mathématique :** gonfler
chaque axe de `M` de `t` avec `t²≥D_max/8` (q4) crée un cube dont, pour
*toute* garde, le maximum de distance carrée est au moins
`3t²≥3D_max/8>D_min/4`. Le test strict ne peut donc **jamais** rejeter
sur cette AABB non coupée. En q3, `t²≥D_max/12` donne déjà
`3t²≥D_max/4≥D_min/4`, même impossibilité. L'intersection avec une
boîte globale particulière peut supprimer le cube, mais n'est pas une
base d'algorithme général. Garder l'enveloppe euclidienne exacte pour
ce certificat et réserver l'AABB à un parcours conservateur de l'index.

**Rapport au citron v8 : pas un crédit géométrique nouveau.** Pour une
paire `ab` fixe, le centre q3/q4 faisable est dans le **disque du plan
bissecteur**, de rayon carré `D/12` ou `D/8`. Poser
`H=(z−a)·(b−z)=D/4−|z−m|²` et
`Ξ=|(b−a)×(z−a)|²`. Le maximum de la puissance du témoin `z` sur ce
disque-relaxation est `−H+√(Ξ/3)` en q3, `−H+√(Ξ/2)` en q4. La condition
de témoin strict universel est donc exactement `H>0` et
`3H²>Ξ` ou `2H²>Ξ`, **le prédicat citron déjà présent** dans la v8.
La capsule `M⊕Ball3(r)` autorise même des centres hors du plan
bissecteur : sur une paire singleton, notre test est nécessairement
plus conservateur. Plus fort : à boîtes A/B et garde `z` **identiques**,
il est dominé par le
[`box_witness` v8](../../morsehgp3D_v8/src/spindle/predicates.hpp),
qui teste exactement le témoin citron sur le produit de deux boîtes
continues par coins successifs. Il est même dominé par la **recherche
rectangulaire native** q3/q4 actuelle lorsqu'elle visite ses feuilles :
pour une garde singleton, le test capsule implique `H_min≥δ−v` et la
borne d'implémentation `Ξ_high≤2vD_max`, où `δ=D_min/4`,
`v=max_M|z−m|²`, `w=D_max/(4α_q)` et `α_3=3`, `α_4=2`. Son inégalité
`δ>(√v+√w)²`, avec `δ≤α_q w`, entraîne `v<w` puis
`δ−v>3√(vw)>√(8vw)`. Ainsi
`α_q H_min²>2vD_max≥Ξ_high` : la v8 admet déjà cette garde à la feuille.
Avec `T_q` gardes, elle rejette déjà la voie sur le même rectangle.
La cascade h/h_a/h_b ne peut que rejeter davantage.
La borne sur `Ξ_high` suit composante par composante de
`|d_j t_k−d_k t_j|≤p_j q_k+p_k q_j`, avec
`p_i=max_{m∈M}|z_i−m_i|` et `q_i=max_{a∈A,b∈B}|b_i−a_i|` : la somme des
trois carrés est au plus `2(Σp_i²)(Σq_i²)=2vD_max`. Pour `Z={z}`,
les extrema des deux produits d'une composante utilisent des coordonnées
indépendantes, comme dans le calcul d'intervalles v8.
La domination est stricte : avec `A={(0,0,0)}`,
`B={(10,0,0)}` et gardes `(7,0,0)`, `(8,0,0)`, `(9,0,0)`,
q4/Kmax=5, le citron v8 crédite les trois (`H>0`, `Ξ=0`) et rejette.
La capsule échoue dès la première, car
`(2+√(100/8))²>100/4`. C'est un gate négatif pour toute prétention
de certificat géométriquement plus fort.

Il ne peut donc produire **aucun rejet/atlas supplémentaire après un h
terminal complet**. Intérêt éventuel : court-circuiter à moindre coût une
recherche h, ou rejeter plus tôt un produit interne dont le front n'a
sondé qu'une fenêtre de témoins. L'ablation appariée doit trouver
**zéro** rejet `capsule_seule` *après h complet* ; sinon, chercher un
défaut ou une configuration différente. Ne pas le porter avant un gain
de temps net sur h/h_a/h_b. Aucun crédit de gardes ne s'ajoute au h
existant : ce sont potentiellement les mêmes sites intérieurs. La vraie
priorité reste les complétions **discrètes** après échec de h.
Cette conclusion s'applique à la première ligne 1 mm citée, dont le reçu
active `witness_mode="rectangle-pair"` et parcourt le même index complet.
Elle ne s'applique pas à une exécution qui laisse le mode API par défaut
`Disabled`, ni au seul front WSPD à fenêtre de propositions limitée.
Ce verdict ne condamne **pas** le certificat `U_C` appliqué à une
**cellule locale d'atlas** pour retrancher des nœuds actifs : là, il borne
le rayon des seules boules encore admissibles et vise les milliards de
copies de frontière, pas un rejet supplémentaire du rectangle. Cette
autre utilisation demande l'expérience shadow décrite plus bas.

Le citron continu n'est pas nécessaire pour les **seuls supports du
nuage fini**. Exemple q3 : `a=(-10,0,0)`, `b=(10,0,0)`,
`x=(0,15,0)`, `z=(0,8,0)`. `abx` est aigu, `ab` sa plus longue arête,
son centre est `(0,25/6,0)` et `z` est strictement intérieur ; l'autre
triangle `abz` est obtus. Pourtant `H=36`, `Ξ=25600` pour `z`, donc
`3H²=3888<Ξ` : aucun certificat universel sur **tout** le disque ne
crédite `z`. C'est une raison précise de conserver un traitement plus fin
des complétions résiduelles après les filtres de rectangle, pas un défaut
d'exactitude du citron.

Ce certificat peut échouer sur beaucoup de rectangles ; il n'est ni une
preuve de coût sous-quadratique ni un remplacement du front actuel. Son
expérience décisive est l'**ablation appariée** : coûts de choix des
gardes et de l'enveloppe, visites de h évitées, rejets **plus tôt dans le
front** avant le h terminal, et multiplicité des subdivisions en cas
d'échec. Sur les rectangles déjà traités par h complet, les paires,
covers, atlas et sorties doivent rester identiques : un écart est un
signal de défaut, non un gain. La preuve numérique
u18 exige des moitiés, carrés et produits du test sans racine en largeur
vérifiée ; le calcul flottant ordinaire ne peut pas décider un rejet.

## Couture Local28 : cellule 2D, certificat de rayon 3D

La `Q4LocalCell` existante est un carré de coordonnées `(α,β)` dans le
plan bissecteur de l'arête, **pas** une boîte XYZ 3D
([type](../../morsehgp3D_v8/src/lanes/q4_local_partition.hpp)). Son image
par la carte affine des centres est un parallélogramme 3D. La première
expérience avec les gardes doit donc :

- calculer `U_C` sur les **quatre coins 3D mappés** : la distance carrée
  à une garde est convexe et son maximum sur ce parallélogramme est
  atteint à un coin ;
- pour `gap²(C,box Z)`, utiliser au minimum la boîte XYZ englobant ces
  quatre coins. Son gap est un **minorant** sûr du vrai gap, donc un
  rejet strict reste valide, mais peut être beaucoup plus faible ;
- prouver séparément les largeurs entières/rationnelles de cette nouvelle
  expression. Les preuves i128 de Local28 ne sont pas transférées par
  simple changement de noms, et le profil float32 a ses propres replis.

Ne jamais appliquer les formules de boîte XYZ directement aux coordonnées
`(α,β)`. Mesurer les rejets perdus par l'englobante avant d'investir dans
une distance exacte parallélogramme–boîte.

## P0 coût : condition de passage du certificat par gardes

Le complément A publié à `e8ce8f33` dans la
[note q4](Q4_STRUCTURE_ET_BORNES.md) certifie le rejet local de blocs de
graines **avant** la partition Z, sous couverture de toutes les graines
possibles et contact conservé. La présente contrelecture ajoute surtout
le trajet global de complétude et l'obligation de juger le **coût net** ;
elle ne revendique pas un certificat local concurrent.
Le [contre-audit pré-atlas B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md)
précise pourquoi un `NoSeed` q4 ne vaut pas un certificat de profondeur
q3 et pourquoi les graines q4 comptées **après** atlas ne dimensionnent
pas le travail du futur filtre pré-atlas.

Sur la ligne 1 mm ci-dessus, rechercher même trois gardes par scan des
39 885 sites pour chacune des 38,795 M cellules ferait environ
**1,55×10¹² examens**. Huit IDs stockés dans chaque cellule feraient
**2,48 Go d'écritures logiques** supplémentaires. La sélection doit donc
réutiliser les gardes des parents ou employer l'index, et son coût doit
être soustrait du travail qu'elle évite. Une garde peut être bonne pour
une arête et très mauvaise pour une autre ; un cache par ancre n'est
utile que si ses hits paient effectivement son acquisition.

Expérience d'audit avant GPU : choisir par hash des IDs des arêtes
stratifiées selon cover/atlas, **sans modifier leur sortie v8**, et tester
toutes leurs cellules. Avant `Q4LocalFragment::child` et tout parcours de
`Z`, tester explicitement les **blocs de complétions `X` contre les cellules
de centres `C`**. Cette expérience *shadow* ne doit ni sauter un bloc ni
changer un support émis ; elle compte produits `X×C`, cellules et fragments
réellement évitables, y compris le prix de préparation des blocs. Un mode
actif ne suivrait qu'après preuve du trajet
`produit → arête propriétaire → X → centre exact → cellule possédée`, avec
repli v8 pour chaque cas indécis. Le raccord d'un fragment q4 exact à q3
peut économiser un census q3, mais ne supprime pas à lui seul les milliards
de visites de l'atlas q4. Publier pour chaque option de gardes : temps et
visites de sélection ; `U_C` ; nœuds et sites écartés **en plus** du
fragment v8 ; `Σ_C |S_C|`, p50/p95/p99/max de `|S_C|` ; sites/cellules
répétés entre arêtes ; événements, tests et copies réellement évitables ;
nombre et coût des replis. Une faible moyenne de `|S_C|` ne suffit pas si
une queue de cellules coûteuses domine. Refaire ensuite le bilan sur les
trames **entières**, sans sol puis brutes, K5/K10, s8/10/12 et profils
float32/1 mm séparés.

Le registre doit distinguer visites de nœuds, tests de points,
`frontier_ids_copied`, réservations, octets écrits et durées : la même
ligne fabrique **26,23 M fragments**. Leurs `6,00 G` slots d'entrée
réservés représentent au moins **48,0 Go de capacité demandée
cumulativement**, et `5,547 G` insertions d'IDs de 8 octets imposent
au moins **44,4 Go d'écritures logiques**. Ni l'un ni l'autre n'est un
pic RAM ou une mesure de débit DRAM ; les insertions ne prouvent pas
autant de lectures d'IDs source. À 100 ms, ces seules écritures
demanderaient 444 Go/s si le chemin restait inchangé : la v9 doit éviter
ce travail plutôt que le distribuer tel quel. Le reçu utilise le mode
`digest`, sans records conservés : le catalogue FULL n'y est pas payé.

## Sortie : boule canonique ≠ toutes ses présentations

Il faut distinguer trois objets :

- **tâche de calcul**, avec ID propre pour reprise exactement une fois ;
- **présentation de support**, telle que le flux v8 q3/q4 l'émet ;
- **boule canonique**, identifiée par sa clé géométrique exacte, avec
  profondeur, liste des intérieurs, coquille complète et `q_min`.

Le [constructeur FULL v7 par boules](../../morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md)
accepte **un record par boule**, sous hypothèse d'un catalogue complet
et exact ; il calcule les facettes, rattachements et plateaux nécessaires.
Il ne demande pas en entrée toutes les présentations positives q3/q4
d'une même clé. Sur une coquille cocyclique/cosphérique, le flux de
supports peut être énorme alors que le nombre de clés est faible.
Il serait donc dangereux de poser « conserver toutes les incidences de
supports » comme contrat de sortie FULL sans le justifier. L'exactitude
du FULL réduit exige les **incidences de facettes et parents pertinentes**,
pas forcément chaque présentation géométrique redondante.
Le [contrat d'objets v8](../../morsehgp3D_v8/docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md)
sépare déjà le flux de toutes présentations de l'interface catalogue.
Une porte peut conserver le flux exhaustif comme **oracle différentiel**
sur petits nuages, sans imposer son coût au chemin industriel FULL.

Piste constructive pour q4 : à arête propriétaire `ab` fixée, les centres
vivent dans son plan bissecteur et chaque autre site trace une droite
exacte `Δ_a,z(c)=0`. Une boule q4 distincte correspond à un **centre
d'intersection**, pas nécessairement à une unique paire de droites : une
grande coquille peut rendre de nombreuses paires concurrentes au même
centre. Énumérer les **sommets de faible profondeur distincts**, regrouper
leurs droites incidentes et décider une fois la boule/sa clé serait plus
proche du catalogue requis que le balayage de chaque présentation.
Cette route reste une hypothèse : il faut trouver ces sommets sans
générer d'abord tous les croisements profonds, vérifier qu'une paire de
supports strictement positive et propriétaire existe, puis mesurer le
coût des concurrences, contacts, clés et plateaux. La simple réduction
de sortie après `O(|A_C|²)` intersections ne règle rien.

Cela **n'autorise pas** une déduplication aveugle à la première clé :
le générateur doit prouver qu'au moins une présentation de chaque boule
requise survit, déterminer `q_min` global, et conserver assez de données
pour ses intérieurs, coquille et plateaux non réguliers. Le constructeur
v7 est seulement relatif à un catalogue fourni et garde un plafond de
coquille 12 ; ce plafond n'est pas une solution massive. Mesurer séparément
présentations examinées, clés uniques, multiplicité par clé et coût du
traitement de coquille ; tester coquille30, grande coquille, E5 et les
vrais parents contre l'oracle Γ borné.

Le [quotient local v7](../../morsehgp3D_v7/src/forest/local_plateau.hpp)
emploie actuellement des masques de coquille et des tableaux de taille
exponentielle en `u=|U|` ; **une seule BallKey ne garantit donc pas** un
plateau massif rapide. Le remplacement industriel devra soit calculer
les composantes/ancres de plateau sans énumérer tous les sous-ensembles,
soit déclarer explicitement une limite de domaine. L'économie de flux
et le coût du quotient sont deux verrous distincts.

Une réduction d'objet est déjà visible dans le
[`visit_block` FULL v7](../../morsehgp3D_v7/src/forest/full_ball_tower.hpp) :
pour chaque rang K, il ne lit du quotient local que la **contribution
non couverte** et **un représentant par composante stricte**. Les listes
`reduced_members`, les masques de couverture propres à chaque composante
et les comptes DSU du prototype servent aux tests/statistiques, pas à
construire la tour réduite. Il faut encore déterminer **exactement** les
composantes du graphe des t-sous-ensembles stricts reliés par une
coface stricte de taille t+1, puis l'union de leurs couvertures ; supprimer
la liste explicite sans algorithme de composantes ne résout pas ce verrou.
Mais elle ne doit pas être érigée en sortie obligatoire de la v9.
La [note de quotient sphérique](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
donne une représentation exacte candidate en `O(u²)` **régions
combinatoires**, avec preuve de correspondance des composantes et
contre-exemple à leur fusion naïve ; aucun temps/RSS n'en est encore
déduit.

La feuille **exacte** de l'atlas q4 peut finir profondeur et coquille q3
pour son centre, mais ne stocke pas les IDs de ses intérieurs uniformes :
`inside_count` n'est pas une liste I pour le catalogue FULL. Une option
économe est de recollecter I/U **une fois par clé de boule distincte**
après déduplication, avec coût publié. Un certificat profond à K−2
rejette q4 mais pas nécessairement q3 ; un certificat terminal K−1
rejette les deux sans devenir une feuille exacte.

Pour le parallélisme, `task_id`, `BallKey` et éventuel `incidence_key`
restent distincts. Un retry GPU valide un lot **une fois** ou l'annule
avant publication ; dédupliquer par `BallKey` ne doit pas servir à masquer
une tâche perdue, ni à fusionner deux vrais objets de sortie différents.

## Questions fermées à résoudre avec le développeur et l'auditeur A

1. Quelle famille initiale de tâches couvre formellement **tous** les
   supports q3/q4 sans matérialiser chaque A×B ? Quel invariant de
   raffinement prouve qu'aucune cellule de centre admissible n'est perdue ?
2. Le premier objectif v9 est-il le **catalogue canonique pour FULL** ou
   le flux exhaustif de présentations q3/q4 v8 ? Le premier suffit au
   constructeur v7 *relatif*, mais sa complétude doit être prouvée ; le
   second peut imposer une sortie inutilement immense.
3. Quelle fraction des coûts de Local28 est réellement supprimée par les
   gardes une fois leur acquisition, les copies, les cellules difficiles
   et les collectes I/U payées ? Un bilan q4 seul ne vaut pas bilan q3/q4
   si l'atlas partagé reste nécessaire à q3.
4. Quelle représentation conserve les plateaux et `q_min` pour des
   coquilles bien supérieures à 12, sans énumérer tous les supports ?

Ces réponses précèdent toute annonce de sous-quadratique. Les séries
8k/16k/32k et les coupes spatiales donnent un diagnostic de croissance ;
seules les tours FULL de trames entières multi-séquences sur G4 peuvent
qualifier le contrat 1 s, puis 100 ms.
