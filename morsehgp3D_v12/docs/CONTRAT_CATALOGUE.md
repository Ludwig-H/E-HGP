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

## 10. Voie appareil (tranche T1-b) et fin d'étage partagée (7 octobre 2026), décisions à contre-lire

Travail d'agent pour le développeur, à relire ; **rien n'a encore tourné sur un GPU** : tout ce qui suit est construit et
joué sur l'hôte (même code), la session G4 est préparée (plan, pilote et juge ci-dessous). Cadre : `backend=cuda_g4`
pour le catalogue, `cpu_reference` pour l'identité, `public_status=not_claimed`.

1. **Une implantation, deux exécuteurs.** Le pilote du parcours (`traversal_driver.hpp`, port de `Driver<B>` de
   `MES-M5`), la fin d'étage (`finish_*.hpp`) et les lots de feuilles de la voie appareil (`device_*.hpp`) sont écrits
   une fois : l'exécuteur Pool (`exec_host.hpp`, warps simulés) les joue pour la voie CPU et pour les portes locales de
   la voie appareil ; l'exécuteur CUDA (`device_cuda.cu`, `device_leaf.cu`, option `MHGP12_ENABLE_CUDA`, `sm_120`) les
   joue sur l'appareil. La voie CPU garde ses feuilles (`LeafStage`) ; sa fin d'étage devient la fin d'étage
   partagée, parallèle (demande du coordinateur après la session F2, `receipts/g4_t1f_20261007`).
2. **Fin d'étage.** Niveau exact de chaque boule en entiers à mots, mêmes formules que `num::Sphere::through`
   (numérateurs et dénominateurs non réduits identiques, porte `level_words`) ; clé F3 et marge F4 de la v11 ; clé des
   positions de $S^{*}$ par les rangs lexicographiques des sites ; tri par base stable (positions, puis clé F3) ; toute
   paire de voisins non certainement ordonnée par F4 est comparée en exact ; une chaîne de voisins incertains mal
   ordonnée (niveaux égaux de représentations différentes dont les clés s'inversent) est retriée en exact sur l'hôte
   (repli compté, `chains_repaired`), puis tout est revérifié ; rangs, CSR $I$ puis $U$ et niveaux des rangs par sommes
   préfixes par tuiles ; table $S^{*}\to$ boule par tri par base des `SiteIdx` et dichotomie. Mêmes sorties qu'avant :
   empreinte de ng00 à K5 égale à celle de F2 en local, portes du catalogue vertes (§ 6), toutes à rejouer sur G4.
3. **Voie appareil, hybride.** L'appareil joue les feuilles d'au plus 32 sites et d'étendue locale d'au plus 16 bits ;
   les autres sont reprises sur le CPU, exactement (compactées, rapatriées, rejouées par `LeafStage`, puis remontées) ;
   les plafonds de 256 candidats et de 64 sites de coquille restent ceux de la voie CPU. Une construction aux profils
   24 ou 32 ne signifie donc pas un traitement intégral sur l'appareil. Feuilles gardées d'un niveau à l'autre jusqu'à
   un lot de $2^{17}$ (une trame de 60 000 sites à K5 tient en un lot) ; par lot : repère et cause de chaque feuille,
   ordre par taille décroissante (`MES-M2`), feuille J3 étroite (`j3_r168`, un warp par feuille, cases de 64 émissions
   comme la voie CPU), décalages exacts par sommes préfixes ; feuilles non résolues comptées par cause, reprises sur le
   CPU **avant** l'admission du lot ; admission (arène des boules agrandie dans le budget) seulement après ces comptes ;
   copie des cases, puis réécriture par la même source des seules feuilles qui débordent leur case, aux places du
   comptage. Comptes publiés : reprises par cause (`replayed_wide`, `replayed_span`), réécritures par lieu
   (`rewritten_device`, `rewritten_host`), dont la somme est `leaves_rewritten`, égal à celui de la voie CPU (porte
   `pipeline_witnesses`). Première faute : la plus petite profondeur, puis la fusion des refus.
4. **Capacité et Session.** Tout tableau de l'appareil et la mémoire épinglée de transit sont réservés dans le budget
   (`BudgetReservation`) avant `cudaMalloc` ou `cudaMallocHost` ; un refus rend `memory_budget` sans rien publier. Le
   contexte `CatalogueDevice` garde ses tableaux d'un appel à l'autre (décision D1) ; diagnostics : réservations faites
   pendant l'appel, octets de l'appareil et épinglés, lots, feuilles et boules rejouées. Raisons nouvelles :
   `device_unavailable` (`resource_exhausted`) et `device_fault` (`invariant_violated`, contexte inutilisable ensuite).
5. **Mutants.** Trois de plus dans `tests/mutants/catalogue.json`, tués sur l'hôte par le même code (feuille non
   résolue admise sans rejeu, palier étroit dépassé sur l'appareil, fin d'étage sans départage exact) ;
   `ordre_par_rang_de_morton` visé sur la nouvelle clé des positions. « Un fil par feuille » (§ 6.6) n'a de sens que
   sur l'appareil : `bench/g4_catalogue_mutants.json`, joué par le pilote G4, via le mode série de `simt.hpp`
   (`MHGP12_SIMT_SERIAL`, un fil joue tout le warp ; dormant dans le produit).
6. **Session G4 préparée.** Pilote `bench/g4_catalogue_device.py` et juge `bench/g4_catalogue_judge.py` (règle écrite
   d'avance dans le pilote, portes `mhgp12_catalogue_g4_judge` et `mhgp12_catalogue_g4_mutants`) : construction CUDA,
   portes rapides (dont `device_open`, vraie voie appareil contre voie CPU sur neuf témoins), identité à l'octet sur
   ng00–02 à K5 et K10 et sur les uniformes, déterminisme d'un appel à l'autre, voie CPU après correction sur les douze
   cas de F2 (comparée à F2), temps de l'étage C à chaud (5 processus × 10 passes par trame), mutants ; « adopté »
   seulement si l'identité tient partout et si médiane et maximum des médianes par processus restent sous 45 ms.
7. **Constats de l'audit de performance (`f601b36ac`).** CST-0233 : la fin d'étage partagée suit le schéma de la
   preuve de l'auditeur (`receipts/audit_performance_20261007/assemblage/`, balayage par blocs avec halo, 3 537
   confrontations) ; ses quatre mutants ont leurs analogues, tués sur l'hôte (`finition_sans_halo` par
   `finish_plateau`, `finition_formes_au_lieu_des_valeurs` et `finition_dernier_representant` par `finish_ties`,
   `finition_prefixe_inclusif` par `finish_random`) ; la table $S^{*}\to$ boule est sa proposition B (tri des quatre
   `SiteIdx`, ici par base). CST-0234 : correctif séparé, empilé sur celui-ci (sur l'hôte, chaque couple évalué une
   fois, census arrêté au rejet ; émissions et registres inchangés, noyau de l'appareil inchangé). CST-0235 : le
   pilote publie séparément parcours, feuilles, émission, fin d'étage, transferts du raccord complet (toute copie hôte
   ↔ appareil de l'appel, chronométrée après l'attente des noyaux en file, avec ses octets par sens), publication et
   total à chaud (médiane et maximum) contre le budget ; les étapes sont nettes des transferts, et le juge refuse une
   passe dont les étapes dépassent le total.

Décisions à contre-lire : (a) le mode série dormant de `simt.hpp` ; (b) les cases de 64 émissions par feuille et les
lots de $2^{17}$ feuilles (192 Mio de cases sur l'appareil) au lieu de l'arène à curseurs atomiques du microbanc, pour
compter avant de réserver ; (c) le repli des chaînes mal ordonnées sur l'hôte (une seule sur les cas joués en local :
ng02 à K5, retriée en exact, sortie identique) ; (d) la clé des positions par rangs lexicographiques ; (e) deux refus
de natures différentes à la même profondeur dans deux lots de la voie CPU peuvent se départager autrement (profondeur
puis fusion ici, premier lot de 16 384 feuilles là-bas) ; (f) l'ancien tri par blocs de la voie CPU (`sort.cpp`)
retiré au profit du tri par base partagé.

## 11. Transferts et publication sur l'appareil (tranche T2-d-C, 8 octobre 2026), à juger sur G4

Travail d'agent pour le développeur, livré sur `main` `902041f66` ; **rien n'a tourné sur un GPU** : la voie appareil
est jouée sur l'hôte par l'exécuteur Pool et par un exécuteur à transit simulé (même code, comptes de l'exécuteur
CUDA), la construction CUDA est vérifiée sans avertissement aux profils 21, 24 et 32. Constat des sessions I et K : à
ng00 K5, 134,3 Mo rapatriés en 9,2 ms (14,6 Go/s contre 56,8 Go/s pour une copie épinglée, `MES-M6`) et 3,0 ms de
publication ; le reste du temps est fait sur l'hôte : copies de la mémoire épinglée vers des `Buffer` neufs (fautes de
page au premier toucher), un tampon intermédiaire de 52 Mo pour les niveaux, 66 petites lectures. **Le catalogue ne
change pas** (export `MHGP12DP`, niveaux publiés et table identiques à l'octet ; voie appareil = voie CPU). Quatre
leviers déclarés d'avance, chacun coupé par une constante (bras d'ablation du pilote, jamais une option) :

1. **Flux de sortie** (`transfer_meter.hpp`, `finish_outputs.hpp`) : les sorties de la fin d'étage sont remises à l'hôte
   par tranches d'au plus 8 Mio dans la mémoire épinglée de transit et consommées tranche par tranche (copie
   parallèle vers le `Buffer` final ; niveaux matérialisés en `num::Level` directement depuis la tranche, sans tampon
   hôte intermédiaire ; la publication n'est plus qu'une adoption). Voie CPU : tableaux de taille exacte adoptés sans
   copie, comme avant.
2. **Double tampon** (`kStagingSlots` = 2) : la copie de la tranche suivante recouvre la consommation de la tranche
   courante (un événement par case ; une case n'est relancée qu'après la consommation de sa tranche).
3. **Sorties anticipées** (`kAnticipateOutputs`) : boules, décalages, populations et table sont réservés à leur
   taille exacte dès les comptes du dernier lot, les niveaux dès le balayage des débuts de rang, puis leurs pages sont
   peuplées sur le Pool (`MADV_POPULATE_WRITE`, repli : un octet écrit par page de l'intervalle) **pendant** que
   l'appareil écrit le lot, puis pendant `Emit`. Une sortie anticipée d'une autre taille est un invariant violé. Avec
   `kAnticipateOutputs = false`, la fin d'étage réserve chaque sortie à sa prise, sans premier toucher, comme la base :
   la constante retire la réservation anticipée **et** le premier toucher.
4. **Repli compact** (`kChainWindow` = 64) : le repli exact des chaînes ne rapatrie plus verdicts, ordre et clés des
   n boules (16 n octets, 22,5 Mo à ng02 K5 pour une chaîne de 9 éléments) mais les positions des M paires mal
   ordonnées, compactées sur l'exécuteur, puis les clés par fenêtres de 64 doublées jusqu'aux bornes de chaque chaîne,
   puis l'ordre et les clés des R éléments des chaînes : lectures directes de 4 M + 8 Σ|fenêtre lue| + 12 R octets,
   plus les contrôles du balayage et les transferts communs du tri exact (contre-lecture Codex) ; `bounds` réserve 2 M
   entrées. Beaucoup de petites chaînes peuvent coûter plus que la lecture d'avant : aucun gain n'est garanti.

**Mémoire (prélecture Codex, point 1).** Le flux demande `min(kStagingSlots × kStreamChunk, plus grand segment)`
octets de transit (`stream_staging_bytes`, au moins 1 Mio, `kStagingMin`), soit 16 Mio dès que la plus grande sortie
les dépasse. Avant T2-d, chaque sortie était copiée d'un seul tenant par une mémoire de transit de `min(64 Mio,
sortie)` octets ; chaque demande de la tranche est donc au plus une demande d'avant pour les mêmes appels, et la
mémoire épinglée gardée par le contexte (le maximum des demandes, jusqu'à sa destruction) n'est jamais plus grande.
Les sorties hôte vivent **plus tôt** : dès le dernier lot (boules, décalages, populations, table), dès `Emit` (niveaux),
au lieu de la prise des sorties ; le tampon des mots de niveaux (48 octets par rang) et la copie d'un seul tenant
disparaissent. Le pic du budget de l'hôte change donc de valeur **et de moment**, et un petit budget peut refuser à un
autre endroit qu'avant (au dernier lot ou pendant `Emit` au lieu de la prise) : toujours un refus entier
(`memory_budget`, rien de publié), jamais au-delà de la limite. Comptes locaux (exécuteur « comptes CUDA » de
l'essai, budgets hôte et appareil séparés, non mesurés sur G4) : pic de l'hôte 255,9 → 168,5 Mo à ng00 K5 et 219,4 →
145,9 Mo à ng01 K5, transit 52,1 Mo → 16 Mio ; pic de l'appareil inchangé. Portes : `pipeline_budget` (hôte et
appareil séparés comme `CatalogueDevice::open(budget, device)` : pic de chaque budget mesuré sur un appel réussi ; au
pic exact, catalogue identique à la voie CPU ; un octet de moins, trois quarts, moitié, un huitième, 4 Kio et zéro :
`memory_budget` ; limites tenues ; tout rendu à la destruction), `pipeline_budget_reuse` (le contexte sert encore
après un refus), `finish_budget` (chaîne longue du repli sous budget serré), `device_open_budget` (la vraie voie
appareil sur G4 : hôte de 512 Kio, appareil de 1 Kio, puis pic et pic moins un octet de chaque budget),
`stream_staging` (demande de transit du flux).

**Chronos (point 2).** `transfer_ns` porte les copies chronométrées après l'attente des noyaux en file et, pour le
flux de sortie, le mur du flux moins la durée de ses consommateurs de publication (matérialisation des niveaux,
comptée dans `publish_ns`) : c'est une **partition du mur, pas une mesure du DMA** (une copie recouverte par un
consommateur n'y paraît pas). `outputs_ns` porte la réservation anticipée et le premier toucher, en durée murale de
l'hôte, même quand un noyau avance pendant ce temps : les étapes nettes ne sont alors pas la durée propre des noyaux.
Étapes disjointes, leur somme tient dans le total de la passe.

**Diagnostics et sonde.** `outputs_ns`, `outputs_bytes`, `stream_chunks` ; la sonde du catalogue les écrit sur une
ligne `sorties` et les empreintes des niveaux et de la table sur une ligne `digest_complet`, seulement sur demande
(`--sorties`, `--digest-complet`) ; `--cache` donne au budget un cache de blocs. Autres portes : `pipeline_staged`
(séquence des tranches sur le transit simulé, cases de 256 octets empoisonnées au lancement et copiées à l'attente,
une à trois en vol), `finish_long_chain` (chaîne de 202 éléments, deux élargissements de fenêtre), `prefault_pages`,
octets des sorties anticipées dans `pipeline_witnesses`, juge, lecteur et substitutions du pilote G4 ; neuf mutants
(`tests/mutants/catalogue.json`, plancher 24) et un mutant appareil joué sur G4 (`flux_sans_attente_appareil`).

**Session G4 préparée** : pilote `bench/g4_catalogue_flux.py`, lecteur strict `bench/g4_catalogue_flux_lecteur.py`,
juge `bench/g4_catalogue_flux_judge.py`, auto-test `bench/g4_catalogue_flux_selftest.py`, tableaux
`bench/g4_catalogue_flux_tables.py` ; règle `REGLE_T2D_C` écrite d'avance (adopté si les empreintes sont identiques
partout et si la borne haute **non arrondie** de l'IC 95 % du rapport après/avant de l'étage C est sous 1 sur chacune
de ng00, ng01, ng02 à K5 ; rejeté sinon ; refusé si une prise manque ou sort de sa commande, si le mutant n'est pas
comparé, ou si le contrôle A/A sort de [0,985 ; 1,015]). Bras par substitution, nommés par ce qu'ils retirent :
`sans_anticipation`, `sans_double_tampon`, `repli_cles_entieres` (fenêtre infinie : toutes les clés par chaîne, garde
la compaction des positions ; ce n'est pas l'ancien repli, qui n'a pas de bras) et `flux_et_repli_selectif` (les trois
substitutions : il isole le flux sur ng00 et ng01, le flux et le repli sélectif sur ng02). Contre-lecture
d'admission Codex (`receipts/audit_reponses_20261008/t2d_c_admission/`) traitée : sorties natives liées à la commande
(séquence, statuts, indices, voie, profil, K, feuille, fils, options, u64 hors booléens, blocs complets ; un champ
absent n'est jamais une mesure nulle, la sonde de la base sans ligne `sorties` étant une exception déclarée), mutant
sans comparaison refusé, A/A écrit, bornes non arrondies ; un cas d'auto-test par défaut relevé (39 injections dont
25 d'admission).
