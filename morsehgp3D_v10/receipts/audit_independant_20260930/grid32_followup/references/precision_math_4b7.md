# Audit numérique indépendant u18 → domaine entier plus large

Lecture seule de R=`/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10`.
Sources ciblées hachées dans `SOURCES.sha256` ; pas de qualification du
nouveau profil, moteur/compilation/GCP non utilisés. `bounds.py` fait seulement
des calculs entiers exacts et deux familles de supports positifs ; normal/−O
code0, sorties identiques. Les bits ci-dessous sont des majorants suffisants
de MAGNITUDE, pas des maxima atteints ni les bits d'un entier signé natif.

## 1. Choix minimal de profil, avant de changer les types

Un fichier `u32le` ne dit ni la précision physique ni le domaine numérique.
À pas h=0,1mm, u18 couvre26,2143m, u24 couvre1,6777215km, u32 couvre429,4967295km
par axe. Un premier profil B≤24/h0,1mm, dans contenantu32, est bien plus
léger que le domaine u32 complet et suffit si les étendues déclarées y tiennent.
Ne PAS le nommer «u32-full». La décision h/f32 sans perte reste utilisateur.

La grille doit être créée depuis les coordonnées float32 ORIGINALES, pas
par multiplication par10 des anciens entiers1mm. Publier h exact, origine,
mode de départage, unités des rayons, IDs et fusions ; refuser dépassement de
domaine, ne pas agrandir h automatiquement. Une quantification au plus proche
donne erreur par point≤√3h/2 ; elle ne prouve pas la stabilité d'un head dur.
Pour grille décimale exacte, décoder f32 dyadique et arrondir son quotient
par h rationnel exactement : un double représentant approximativement h
ne doit pas arbitrer les demi-grilles. Coordonnée-bits ≠ résolution.

## 2. Bornes géométriques pour E<2^B

E borne TOUTES les différences de sites du support et de ses requêtes.
Les centres sont relatifs à un site. On conserve les majorants prudents de
GEN_v2, sans parier sur un pgcd ou une annulation ultérieure.

| opération | borne magnitude | bits B24 | bits B32 | type suffisant B32 |
|---|---|---:|---:|---|
| distance² / dot |3E²|50|66|u128 / i128|
| composante cross |2E²|49|65|i128, pas P3 i64|
| orientation de4sites |6E³|75|99|i128|
| centre q3 N / D |36E⁵ /24E⁴|126/101|166/133|Wide3 /Wide3|
| centre q4 N / D |18E⁴ /12E³|101/76|133/100|Wide3 /i128|
| clé side q3 |288E⁶|153|201|Wide4|
| clé side q4 |144E⁵|128|168|Wide3|
| orient-centre q3 |360E⁷|177|233|Wide4|
| orient-centre q4 |180E⁶|152|200|Wide4|
| test midpoint q3/q4 |120E⁵ /60E⁴|127/102|167/134|Wide3|
| centre dans boîte, T6 |64·60E⁵ /64·30E⁴|132/107|172/139|Wide3|
| niveau q3 num /den |27E⁶ /48E⁴|149/102|197/134|Wide4 /Wide3|
| niveau q4 num /den |972E⁸ /144E⁶|202/152|266/200|Wide5 /Wide4|
| comparaison niveaux |139968E¹⁴|354|466|Wide8 magnitude|

Pour B24, Center conserve ses champs i128 ; dot/cross restent i64. Toutefois
side q3, side q4 (128bits MAGNITUDE, donc trop pour i128 SIGNÉ), orientation
centre, pont boîte et niveaux doivent changer. Level commun Wide4/Wide3
donne produitWide7, déjà autorisé. Pour B32, LevelWide5/Wide4 donne produit
Wide9, actuellement interdit par `Wide<L>:L≤8`, même si sa valeur tient
sur8mots : soit temporaireWide9 exact puis comparaison, soit multiplication
versWide8 contrôlée et prouvée, jamais suppression silencieuse du mot haut.

### Violations effectives sur supports positifs

Prendre M=2^B−1 et tetra régulier
`a=(0,0,0), b=(M,M,0), c=(M,0,M), d=(0,M,M)`.
Le triangle abc est strictement aigu ; le tetra a centre(M/2,M/2,M/2),
barycentriques1/4. Les formules réelles du code donnent :

- q3 N=(4M⁵,2M⁵,2M⁵), D=6M⁴ ; rayon num8M⁶, den12M⁴.
- q4 N=(2M⁴,2M⁴,2M⁴), D=4M³ ; rayon num12M⁸, den16M⁶.

À B32 : q3 N162bits/D131bits, num195bits ; q4 N129bits, num260bits/den196bits.
À B24 déjà, q4 num196bits>192 et den148bits>128. Donc élargir seulement le
census ou retirer le refus B18 serait réellement incorrect, même sur
supports réguliers et positifs. Ces exemples homothétiques ont un grand
pgcd ; le chemin produit ne le calcule pas, et un pgcd1 reste possible en
général. Ne pas remplacer des bornes par une réduction espérée.

## 3. Points de port causaux, sources actuelles

- geometry.hpp : `dot/cross` calculent en i64 AVANT conversion ; q3/q4
  `Center`, `side_key`, `side`, `orient_center`, `is_midpoint` n'ont pas
  la capacité B32. Séparer vecteur de positions i64 et cross i128.
- geometry.cpp : q3 `uu*vv` multiplie en u128 (peut exiger132bits), puis
  `Wide1::from_u128(dd)` jette silencieusement les bits hauts d'une distance
 66bits. q4 D² peut exiger200bits ; `resize/add` ignorés sont sûrs uniquement
 sous les bornes actuelles. Propager/checker les résultats du nouveau port.
- `Level::approx` a des convertisseurs à3/2mots : adapter toutes conversions
 et exports. Les bandes de tri ne réparent une mauvaise clé que sous une
 borne d'erreur REPROUVÉE ; un dénominateur tronqué ne se répare pas ainsi.
- generator.cpp, T6 : les POSITIONS de boîte restent i64 (≤39bits), mais
 `X2/lx2`, clés du réservoir, produits dominance et bissectrices deviennent
 i128 en u32. Le maximum prudent du réservoir27E'² a81bits, E'=2^38.
 Le lemmeZ final72E'³ a121bits : i128 suffit, avec promotion AVANT chaque
 produit (cross/P0/P1 ne restent pas i64). Le pont centre→boîte devientWide3.
- support.hpp et generator::canonical_support : tous les midpoint/dot/
 cross/orient exacts suivent les nouveaux types ; pas de nouvelles epsilon,
 contacts et supports strictement positifs gardés inchangés.
- tower.cpp : Sphere/set_approx, orient_center_filtered, verify_meb,
 Scratch.near (clés side), level_at_most, point_level et RankIndex doivent
 suivre leurs domaines. RankIndex compte des niveauxu32, pas des coordonnées.
 `level_at_most` doit accepter distanceu128 en B32 ; produire e·den enWide6,
 puis comparaison/resize contrôlé versnumWide5. Ne pas garder Wide1(e).
- Les filtres flottants ont des hypothèses u18 : w exact en double dans
 orient_center_filtered, centre/distance avec marge fixe dans SiteTree/
 Sphere. w peut avoir65bits en B32. Reprouver encadrement/conversion/FTZ/
 FENV et repli exact ; garder initialement ExactOnly/refus, pas .02 arbitraire.
- Pas de pgcd sur chemin chaud v10 : identitéS* et mémo de coquille sont
 réutilisables. Pgcd d'export/clé seulement si besoin, avec division exacte
 et capacité certifiée, pas comme remède au débordement préalable.

## 4. Cloud/index/entrée, contre-audit de verify_new_receipts_0930

Préparateur actuel1..21bits ; catalogue/tour/CLI restent B18. La branche
`bits==32` est inatteignable. Morton64/spread21 masque les bits≥21 et fusion
par égalité de clé seule : (0,0,0) et(2^21,0,0) seraient fusionnés si l'on
levait seulement le plafond. CléMorton96 dansu128 (historique≤21préservé),
ou clé tronquée MAIS égalité/ordre completxyz pour tri et regroupement.

SiteTree distance² maxu32=55340232195358851075,66bits. Produits actuelsi64
puis castu64 déborderaient avant le cast. Propageru128 aux API/sentinelles,
KNN/rayon, point_level et exports. Requêtes rationnelles ont leur propre
domaine/refus. Les extrémités de boîteu32 et leurs copiesdouble restent
exactement représentables ; pas besoin de changer les médianes/bbox.

## 5. Roadmap minimale, sans transfert de qualification

1. Choisir profil explicite h/étendue : B24 intermédiaire ou u32 complet,
   float32 brut distinct. Préparer depuis bruts, préserver tous les IDs.
2. Porter identitéMorton, domaineCloud, distances/index et capacités avant
   de lever les refus catalogue/tour. Oracle exact, mutants bits hauts/
   cast tardif, frontières0/UINT32_MAX, doublons, permutations.
3. Porter centres/prédicats/niveaux/pont/tri/export selon table, sans gcd
   chaud ; tester supports ci-dessus, contacts, annulations, near-coplanar,
   comparaisons inter-arités et égalités. Pas de premier benchmarkG4 avant
   gates CPU Release/SAN, lecteurs normal/−O et refus exacts.
4. Garder palier rapide local : q3/side i128 sûr si E<2^19 ; Bglobal32 ne
   force pas toutes les feuilles à cette largeur. Déterminer l'étendue
   AVANT arithmetic. Dominancei64 reste sûre B+T≤29. La plupart des étendues
   LiDAR0,1mm peuvent rester petites ; taux et coût du repli à mesurer.
5. Requalifier la chaîne FULL/G4 au nouveau pas sur trames entières et
   coupes capteur appariées, avec/sans sol ; aucune croissance/100ms héritée.

## 6. Réemploi float32 v8 : ce qui est utile et ce qui ne l'est pas

Utiles : intervalles extérieurs + ExactOnly, états privés, entier fixe
1728bits contrôlé, décodage dyadique, classes de contact, quatre poids q4,
clé primitive globale/pgcd hors chemin chaud, comparateur événements q4 de
degré5 avec DEUX signes de dénominateur. Voir float32_ball/key/q4_events.
Ce comparateur de racines n'est PAS un comparateur de rayons FULL.

Ne pas convertir des entiersu32 enfloat32 (perte au-delà24bits), ni
réinterpréter u32le enmotsIEEE. Réutiliser algorithmes/invariants ou adapter
un décodage entier direct ; pas copier une qualification.

Float32 sans perte général : déplacements exacts dans2^-149 peuvent
atteindre278bits. Les prédicats degré6 de v8 tiennent1728bits, mais le niveau
q4 naïf requiert num2234bits/den1676bits et produits croisés3910bits sous les
mêmes majorants ! Ni576bits q2 ni1728bits ball-side ne qualifient ce tri.
Index/front/q3 ancien ne constituent pas une chaîne FULL v10. C'est un
chantier plus large, à ne pas présenter comme équivalent au profil grille.
