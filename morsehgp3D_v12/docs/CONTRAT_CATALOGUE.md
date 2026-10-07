# Contrat de la tranche T1 : le catalogue

7 octobre 2026. Proposition du développeur, **contre-lue par l'auditeur Codex**
([`contrat/README.md`](../receipts/audit_session_t1_20261007/contrat/README.md), pin `4147c5460`) et révisée en
conséquence le même jour ; à relire encore avant tout code ([`PLAN.md`](PLAN.md) § 0 : contrat, puis
témoins et oracle, puis le natif). Entrée de la tranche : `MES-M2` jugé sur G4 (feuille J3 adoptée, variante
`j3_r168`, [reçu](../receipts/g4_t0a_20261007/README.md)) ; `MES-M5` (parcours des boîtes en largeur sur le GPU) en
cours. Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `public_status=not_claimed`.

## 1. L'objet

Le catalogue $\mathrm{Cat}_K$ est l'ensemble des boules critiques **positives** ($q=q_{\min}\geq 2$ ; les naissances des
sites au niveau zéro n'en sont pas) telles que $p+q\leq K+1$ (`CAT-`,
[`OBJET_ET_CONTRAT_MATHEMATIQUE.md`](OBJET_ET_CONTRAT_MATHEMATIQUE.md)), chacune avec son support canonique $S^{*}$,
son intérieur strict $I$ ($p$ sites), sa coquille $U$ ($m$ sites), $q_{\min}=\lvert S^{*}\rvert$, son niveau exact.
La complétude est **conditionnelle** (`CAT-G4`) : listes $K$-certifiées des boîtes, recensement local, élagage. Le
catalogue de la v12 est **le même ensemble** que celui de la v11 gelée (`ac081a06f`), à un seul écart déclaré près :
$S^{*}$ se départage par la liste triée des positions de ses sites (ordre lexicographique des coordonnées), et non plus
par les rangs de Morton ([contrat numérique](CONTRAT_NUMERIQUE.md) § 4, `CST-0113`) ; l'écart ne touche que les
coquilles à plusieurs supports minimaux.

## 2. L'algorithme (changements déclarés d'avance)

| Étage | v11 | v12 | Preuve d'égalité |
| --- | --- | --- | --- |
| parcours des boîtes de centres | récursif sur l'hôte (frontière 20,7 ms, passe unique 37,5 ms sur ng00 K5) | **en largeur sur le GPU**, tous les nœuds d'un niveau à la fois (`MES-M5`) | même ensemble final de feuilles (boîtes, listes) |
| feuille | un fil par feuille sur le GPU (témoin de `MES-M2`) ; DFS sur l'hôte | **J3 par phases sur un warp**, source unique `__host__ __device__`, consommée **en flux** pendant le parcours | identité exacte, 15 compteurs et émissions par feuille (`MES-M2`, 2 748 544 feuilles) |
| fin d'étage | sur l'hôte | **sur l'appareil** : tri par base des clés F3, chaînes de voisins non certainement ordonnés résolues en exact, rangs, CSR des populations ($I$ puis $U$), table $S^{*}\to$ boule | différentiel du catalogue complet contre la v11 |
| repli | `unresolved` rejoué en série sur l'hôte | rejoué **en parallèle** et budgété (`CST-0009`) | même décision que la voie exacte |

**Une seule implantation par noyau** ([`ARCHITECTURE.md`](ARCHITECTURE.md), règle 2) : sur l'hôte, la feuille du
produit est la même source J3, jouée sur un warp simulé, répartie sur les fils ; la feuille DFS de la v11, gelée, et
l'oracle borné indépendant restent comme **témoins de test** (une source commune peut porter le même défaut sur l'hôte
et l'appareil), jamais comme second produit. La voie CPU sert la référence exacte et les petits nuages (décision D5).
Conditions d'adoption, accordées par l'auditeur : le surcoût local de la feuille simulée (1,4 à 1,9 fois la DFS de la
v11 à un fil, `MES-M2`) ne décide pas seul ; on mesure le **catalogue CPU entier** et la chaîne des petits nuages
(`MES-P` : 100 à 10 000 sites, froid et chaud), qui fixe aussi le seuil entre voie CPU et voie GPU ; en cas d'échec,
la décision de conception est révisée ouvertement, jamais par une seconde implantation discrète. Le **repli exact** de
l'hôte rejoue une feuille refusée avec la même structure J3 mais des opérations exactes plus larges : rejouer sur le
même domaine arithmétique ne résoudrait rien.

## 3. Numérique

Tout le contrat numérique s'applique ([`CONTRAT_NUMERIQUE.md`](CONTRAT_NUMERIQUE.md)) : repère d'une feuille = fermeture
de sa boîte et tous les sites de sa liste ; filtrage G1 d'un enfant dans le repère du parent ; budgets par palier
d'étendue (sur les trames mesurées, toutes les feuilles ont $s\leq 17$ et tous les supports $s\leq 15$ ; la voie étroite
historique de la v11, d'enveloppe au plus $2^{20}$, les couvre toutes ; le palier étroit proposé, $s\leq 16$, en couvre au
moins 99,997 %, et les feuilles à $s=17$, au plus 13 par cas, passent au palier moyen, l'orientation y valant 128 bits ;
des supports à $s\leq 15$ ne couvrent pas d'office les autres sites interrogés) ; certificats liés à leur domaine ; test du milieu local ; réservoir $2s+4$ ; boîtes fermées jusqu'à
$s=33$. Sur l'appareil (contrat R7 de la v11) : seules décident les voies garanties par le palier ou par un certificat ;
toute autre feuille est **non résolue**, comptée, et rejouée en exact sur l'hôte avant admission. Aucune décision en
flottant ; les clés F3 ne servent qu'à trier, avec repli exact.

## 4. Capacité, mémoire, refus

- **Comptage puis réservation** (`CST-0211`) : chaque lot de feuilles compte exactement ses boules et ses incidences
  avant de les écrire, **repli des feuilles non résolues compris, avant admission** ; décalages et sommes contrôlés ;
  le budget couvre les listes du front du parcours et la fin d'étage ; la trame entière s'engage de façon transactionnelle,
  et un dépassement rend `resource_exhausted` sans rien publier. Le résultat partiel et les compteurs d'une tentative
  non résolue sont jetés : une feuille ne compte qu'une fois, après succès, et les deux passes (comptage, écriture) ne
  doublent pas les compteurs logiques ; leurs coûts physiques, et ceux des tentatives, sont publiés à part. La
  prévision par les lois par site (33 boules et 153 incidences par site à K5 sur le LiDAR) ne sert qu'à dimensionner les
  lots.
- **Domaines d'indices** (`CST-0212`) : sites, boules et feuilles sur 32 bits avec refus à la vraie limite ; décalages
  et compteurs sur 64 bits.
- **Profondeur** du parcours au plus $3B$ (`CST-0205`) ; nombre de nœuds budgété à part.
- **Flux** : une trame de 60 000 sites tient en un lot (prévision de régime, non conséquence de $n$ : l'admission
  certifiée prévaut) ; une scène de plusieurs millions de sites passe par lots de
  feuilles dans l'ordre de Morton, l'appareil étant réutilisé d'un lot à l'autre ([`ARCHITECTURE.md`](ARCHITECTURE.md)
  § 4.6).
- **Refus** : entrée invalide ; multiplicités par défaut (décision D8, option « sites distincts ») ; coquille étendue
  au-delà du plafond déclaré (`WIT-SPHERE50`) ; ressources. Jamais de jitter, jamais un préfixe publié.

## 5. Compteurs

Les compteurs **logiques** de la feuille (les quinze de `leaf.cpp`, dont la forme close est prouvée par le lemme des
faces, relu le 7 octobre) et ceux du parcours sont indépendants de l'ordre de visite, de la voie et du nombre de fils ;
ils entrent dans les empreintes. Les diagnostics **physiques** (paquets, remplissage des voies, feuilles non résolues
par cause, temps) sont publiés à part, jamais dans une empreinte.

## 6. Portes

1. **Différentiel contre la v11, par un lecteur de transition gravé avant le catalogue** : identité des sites par
   position et des boules par centre exact et rayon carré (jamais par $S^{*}$), bijection exigée (absence, doublon ou
   boule de trop sont des échecs) ; $p$, $m$, $q_{\min}$, $I$, $U$ et niveaux comparés exactement, par valeur ; pour tout
   $S^{*}$ différent, même boule, même cardinal minimal et minimum lexicographique propre à chaque convention vérifiés
   ($m>q$ seul ne prouve pas plusieurs supports admissibles) ; ordre publié et renumérotations recalculés. Sur ng00–02
   à K5 et K10 et sur les uniformes de 8 000, 16 000 et 32 000 sites. **Ordre parent du parcours** : l'égalité des
   feuilles avec la v11 ne vaut qu'à liste parente ordonnée identique (réservoir départagé par le rang dans la liste
   parente). La v12 garde l'ordre des sites de la v11 (clé de Morton exacte sur coordonnées absolues,
   [contrat numérique](CONTRAT_NUMERIQUE.md) § 4) : la liste parente est la même, et l'égalité des feuilles et des
   quinze compteurs avec la v11 est exigée, comme `MES-M5` l'a établie à ordre fixé. Une normalisation d'ordre future
   changerait la partition en feuilles sans perdre de boule (modèle exact à six sites de l'auditeur) : elle se
   jugerait sur $\mathrm{Cat}_K$, écarts de feuilles et de coûts publiés.
2. **Oracle borné** (`reference/`, $n\leq 14$) : égalité du catalogue sur la suite rapide.
3. **Témoins** : le triangle $(0,1,1),(1,0,1),(1,1,0)$ (`WIT-TRANSL`) dont les trois boules diamétrales de niveau $1/2$
   changent d'ordre entre conventions, ce qui change les choix de Kruskal et de `cover` sans changer la forêt (le
   lecteur de transition des sorties `supports` et `cover` refait la sélection canonique de chaque convention et vérifie
   la relation de couverture **et** le choix prescrit) ; `WIT-FEUILLES` (cube et son centre), `WIT-SPHERE50`, `WIT-T1-CARRE` (côté catalogue : table
   $S^{*}\to$ boule), témoins de palier du contrat numérique, profondeur 60 et 63 (`CST-0205`), boîte fermée à
   $2^{32}$ (`CST-0204`).
4. **Filets** : restriction J1 et Euler à $K+2$ (`JUG-EULER`), sur les trames et les uniformes.
5. **Déterminisme** : sorties identiques à l'octet entre la voie CPU et la voie GPU, et quel que soit le nombre de fils.
6. **Mutants causaux** : témoin perdu du réservoir, G1 dans le repère de l'enfant, feuille non résolue admise sans
   rejeu, $S^{*}$ départagé par rang de Morton, et « un fil par feuille », qui doit **perdre son budget de temps**.
7. **Échelle** : invariants globaux et juge d'échantillon à 8 000, 16 000 et 32 000 sites, puis sur les découpes de 1 à
   8 millions de sites (`MES-E`), jamais un juge exhaustif.

## 7. Budget et règle de sortie

Budget de l'étage C à K5 sur une trame de type ng00, G4, à chaud : **35 à 45 ms**, transferts et fin d'étage compris
([`ARCHITECTURE.md`](ARCHITECTURE.md) § 3). Point de départ mesuré : noyau de feuille `j3_r168` 11,6 ms (ng00 K5/24) ;
le reste se mesure par `MES-M5` et à l'intégration. Sortie de T1 : portes vertes et budget atteint, ou écart publié avec
sa cause ; à K10, objectif publié (le noyau de feuille seul y vaut 37,2 ms sur ng00).

## 8 bis. Format d'échange et lecteur de transition

Le catalogue de la v12 s'exporte, hors du chemin chronométré, au format `MHGP12DP` version 1 de genre « catalogue »
déjà écrit par l'outil de vidage de la v11 des microbancs de la tour
([`common/format.hpp`](../microbancs/mes_m3_m4_tour/common/format.hpp)) : en-tête (profil, $K$, nombre de sites),
section des sites (coordonnées ; la version 1 n'a pas de `PointId`, l'identité d'un site étant sa position), section des
boules (rang de niveau, $p$, $m$, $q_{\min}$, $S^{*}$ en indices de sites) dans l'ordre canonique, puis les populations
$I$ et $U$ en CSR. Les deux versions partagent ainsi un format et
le même ordre des sites (clé de Morton exacte sur coordonnées absolues).

Le lecteur de transition (`reference/transition_catalogue.py`, bibliothèque standard, rationnels exacts) lit deux
vidages, ramène chaque indice à sa position, **recalcule** le centre exact et le rayon carré de chaque boule à partir des
positions de son $S^{*}$ (plus petite boule de 2, 3 ou 4 points), puis applique la règle du § 6.1 : bijection des
boules par (centre, rayon carré) ; $p$, $m$, $q_{\min}$, $I$ et $U$ égaux comme ensembles de positions ; rangs recalculés
et comparés par valeur ; pour tout $S^{*}$ différent, même boule, même cardinal, et minimum propre à chaque convention
(rangs de Morton pour la v11, positions lexicographiques pour la v12). Il est gravé et jugé sur ses témoins (triangle
`WIT-TRANSL`, carré `WIT-T1-CARRE` et ses deux diagonales, coquille à plusieurs supports, mutants de chaque règle) avant
de juger le premier catalogue de la v12.

**Gravé le 7 octobre 2026** ([`reference/README.md`](../reference/README.md), « Lecteur de transition du catalogue ») :
six témoins, 49 cas et 25 mutants en portes CTest (`mhgp12_reference_transition*`). Le carré `WIT-T1-CARRE` garde
$S^{*}=AC$ dans les deux conventions (son premier site l'est en Morton comme en positions, la clé de Morton croissant
avec chaque coordonnée) ; le changement de $S^{*}$ se grave sur le même carré tourné de 45 degrés (`WIT-CARRE-TOURNE`),
puis sur un triangle (`WIT-CERCLE-Q3`) et un tétraèdre (`WIT-SPHERE-Q4`). Deux précisions de lecture : la règle de
convention s'applique à **toute** coquille étendue, pas seulement aux $S^{*}$ qui diffèrent (un candidat qui garderait
le départage de la v11 a souvent le même $S^{*}$ que la référence) ; à niveau égal, l'ordre publié de la v12 range les
boules par listes triées des positions de leur $S^{*}$ (bourrage par un élément plus grand que tout, sans effet : à
niveau égal, un $S^{*}$ n'est jamais préfixe d'un autre), ordre que l'exportateur de la v12 doit suivre.

## 8. Questions pour les auditeurs

1. La règle « une seule implantation » appliquée à la feuille sur l'hôte (warp simulé, plus lent que la DFS de la v11)
   vous paraît-elle le bon compromis pour la voie CPU, sachant qu'elle sert surtout la référence et les petits nuages ?
2. Le départage de $S^{*}$ par positions change la sortie `supports` et la convention `cover_v10` : faut-il un lecteur
   de transition qui compare v11 et v12 en ne tolérant que les coquilles à plusieurs supports minimaux ?

## 9. Voie CPU de référence livrée (7 octobre 2026) et décisions à contre-lire

Module `src/catalogue/` ([reçu du développeur](../receipts/developpement_20261007/catalogue_cpu_RAPPORT.md)) :
parcours en largeur et feuille J3 en source unique sur le warp simulé, repli exact plus large, fin d'étage sur l'hôte,
table $S^{*}\to$ boule, export `MHGP12DP`. **Différentiel conforme** à la v11 gelée sur ng00, ng01, ng02 et les uniformes
de 8 000, 16 000 et 32 000 sites à K5 (profils 21 et 32 ; vingt compteurs logiques égaux ; deux $S^{*}$ changés sur ng02,
exigés par la convention) et sur ng00 à K10 (5 512 670 boules, un $S^{*}$ changé), par le lecteur de transition ; oracle
borné conforme (342 nuages) ; filets Euler et J1 conformes ; mêmes octets à 1, 4 et 8 fils. Décisions prises dans la
latitude du contrat, **à contre-lire** :

1. **Feuilles de 33 à 256 sites** : même source J3, jouée sur l'hôte par un warp virtuel de 256 voies (jamais sur
   l'appareil) ; leurs compteurs reproduisent ceux de la voie DFS de la v11 par une forme close. Sans cela, la coquille
   de 48 sites de `CST-0205` serait refusée.
2. **Plafond de coquille à 64 sites** : au-delà, refus `shell_capacity` (`WIT-SPHERE50`, où la v11 calculait 435
   boules) : écart déclaré.
3. **Deux politiques arithmétiques de feuille** au lieu de trois paliers : natif jusqu'à l'étendue 16, entier exact de
   320 bits au-delà (une feuille au palier moyen sur 123 581 à ng00).
4. **Pas de quota de nœuds séparé** : le budget des tableaux du front et les indices de 32 bits en tiennent lieu.
5. **Option « sites distincts » (D8)** non faite : seul le refus par défaut des multiplicités existe.

Coût local indicatif (machine chargée, aucune décision) : 1,1 à 1,3 fois la voie CPU de la v11 à un fil. La règle de
`MES-P` et le budget de l'étage C se jugent sur G4, avec la voie appareil (T1-b), qui doit rendre les mêmes octets.
