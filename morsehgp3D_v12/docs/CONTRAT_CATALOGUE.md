# Contrat de la tranche T1 : le catalogue

7 octobre 2026. Proposition du développeur, **à relire avant tout code** ([`PLAN.md`](PLAN.md) § 0 : contrat, puis
témoins et oracle, puis le natif). Entrée de la tranche : `MES-M2` jugé sur G4 (feuille J3 adoptée, variante
`j3_r168`, [reçu](../receipts/g4_t0a_20261007/README.md)) ; `MES-M5` (parcours des boîtes en largeur sur le GPU) en
cours. Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `public_status=not_claimed`.

## 1. L'objet

Le catalogue $\mathrm{Cat}_K$ est l'ensemble des boules critiques $b$ telles que $p+q\leq K+1$ (`CAT-`,
[`OBJET_ET_CONTRAT_MATHEMATIQUE.md`](OBJET_ET_CONTRAT_MATHEMATIQUE.md)), chacune avec son support canonique $S^{*}$,
son intérieur strict $I$ ($p$ sites), sa coquille $U$ ($m$ sites, dont $q_{\min}=\lvert S^{*}\rvert$), son niveau exact.
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
produit est la même source J3, jouée sur un warp simulé (1,4 à 1,9 fois le coût de la feuille DFS de la v11 à un fil,
mesure locale de `MES-M2`), répartie sur les fils ; la feuille DFS de la v11 ne reste que comme **oracle de test**,
jamais comme second produit. La voie CPU sert la référence exacte et les petits nuages (décision D5).

## 3. Numérique

Tout le contrat numérique s'applique ([`CONTRAT_NUMERIQUE.md`](CONTRAT_NUMERIQUE.md)) : repère d'une feuille = fermeture
de sa boîte et tous les sites de sa liste ; filtrage G1 d'un enfant dans le repère du parent ; budgets par palier
d'étendue (sur les trames mesurées, toutes les feuilles ont $s\leq 17$ et tous les supports $s\leq 15$ : le palier étroit
couvre tout) ; certificats liés à leur domaine ; test du milieu local ; réservoir $2s+4$ ; boîtes fermées jusqu'à
$s=33$. Sur l'appareil (contrat R7 de la v11) : seules décident les voies garanties par le palier ou par un certificat ;
toute autre feuille est **non résolue**, comptée, et rejouée en exact sur l'hôte avant admission. Aucune décision en
flottant ; les clés F3 ne servent qu'à trier, avec repli exact.

## 4. Capacité, mémoire, refus

- **Comptage puis réservation** (`CST-0211`) : chaque lot de feuilles compte exactement ses boules et ses incidences
  avant de les écrire ; la réservation est contrôlée ; un dépassement rend `resource_exhausted` sans rien publier. La
  prévision par les lois par site (33 boules et 153 incidences par site à K5 sur le LiDAR) ne sert qu'à dimensionner les
  lots.
- **Domaines d'indices** (`CST-0212`) : sites, boules et feuilles sur 32 bits avec refus à la vraie limite ; décalages
  et compteurs sur 64 bits.
- **Profondeur** du parcours au plus $3B$ (`CST-0205`) ; nombre de nœuds budgété à part.
- **Flux** : une trame de 60 000 sites tient en un lot ; une scène de plusieurs millions de sites passe par lots de
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

1. **Différentiel contre la v11** : catalogue complet (boules, $S^{*}$, $p$, $m$, $q_{\min}$, $I$, $U$, niveaux) égal
   à celui de la v11 gelée sur ng00–02 à K5 et K10 et sur les uniformes de 8 000, 16 000 et 32 000 sites, au départage
   de $S^{*}$ près, chaque écart de $S^{*}$ vérifié en exact comme coquille à plusieurs supports minimaux.
2. **Oracle borné** (`reference/`, $n\leq 14$) : égalité du catalogue sur la suite rapide.
3. **Témoins** : `WIT-FEUILLES` (cube et son centre), `WIT-SPHERE50`, `WIT-T1-CARRE` (côté catalogue : table
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

## 8. Questions pour les auditeurs

1. La règle « une seule implantation » appliquée à la feuille sur l'hôte (warp simulé, plus lent que la DFS de la v11)
   vous paraît-elle le bon compromis pour la voie CPU, sachant qu'elle sert surtout la référence et les petits nuages ?
2. Le départage de $S^{*}$ par positions change la sortie `supports` et la convention `cover_v10` : faut-il un lecteur
   de transition qui compare v11 et v12 en ne tolérant que les coquilles à plusieurs supports minimaux ?
