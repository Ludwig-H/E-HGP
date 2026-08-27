# Pistes fermées — mémo append-only (hérité v3 + v4, complété v5)

Une piste fermée ne se rouvre qu'avec un nouveau théorème + fixture, jamais sur
un benchmark. Chaque entrée : l'idée, la cause d'abandon (référence), ce qui
survit. Les patterns d'erreur récurrents sont en fin de document.

## Héritées de la v3 (`morsehgp3D_v3/audits/PISTES_FERMEES.md`)

- **Source kNN à petit préfixe pour les ancres q2.** Le rang de voisinage
  d'une arête vivante n'est pas borné (arête vivante joignant a à son
  1001-ième voisin ; fixture 50 000 points gravée). Survit : la WSPD comme
  seule source complète des paires.
- **Cap de population dans le critère terminal de la WSPD.** Force
  $\#\mathrm{rect} \geq \binom{n}{2}/C^2$, quadratique par construction
  (arbitrage du 16 août 2026). Survit : terminal ⟺ séparé ; mutant
  `wspd-cap-terminal`.
- **Scission du facteur le plus peuplé.** −14,7 % de rectangles en scindant
  le plus grand diamètre, et c'est l'invariant de l'argument d'empilement.
  Survit : mutant `wspd-split-heaviest`.
- **Deux arbres spatiaux coexistants.** Une réfutation invalide est née de
  leur confrontation (rétractée le 16 août 2026). Survit : un seul arbre radix.
- **« Arrondir le rayon d'une unité vers le haut est sûr » (cœur-boule).**
  Réfuté par deux fixtures q3/q4 à norme irrationnelle. Survit : arithmétique
  dirigée, distance minorée, rayons majorés, comparaison stricte ; mutant
  `core-ball-ceil-distance`.
- **Majorant de Ξ par composantes « le plus serré possible ».** Retiré par
  contre-audit : sûr mais pas serré. Survit : l'évaluation aux coins.
- **Crédit de groupe sans disjonction d'identités.** `collinear_seven` publie
  un huitième crédit fantôme. Survit : la contre-famille gravée.
- **Cascade RNG–Jung bornée comme source sparse.** Incomplète sur une fixture
  rationnelle de rang fermé 11 (`docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`).

## Héritées de la v4

- **Sélection axiale bornée (seize groupes par seed, sweep à deux côtés,
  `cmp_mu`).** Opt-in NÉGATIF sur CPU : +7 % de `t_gen` à n=1600 malgré
  18,8 M → 0,8 M d'évaluations q4, la baseline rejetant à la lentille en trois
  opérations i64 (reçu axial borné, v4). Conservée « pour le GPU » puis
  jamais activée. **Fermée en v5** : aucun chemin axial ; la piste GPU ne se
  rouvrira qu'avec un kernel mesuré sur G4 et une porte d'égalité post-RLE.
- **`build_forest_legacy` (tri global des incidences) comme témoin figé du
  fold compact.** Une seconde implémentation figée n'est pas un juge : elle
  partage les hypothèses du sujet. Survit : l'oracle borné (`forest_judge`,
  miniboule indépendante, cliques complètes, Kruskal à lots) et les
  invariants structurels de la partition dense.
- **Index par couches convexes pour le scan q3.** Réfuté par les compteurs de
  charge : 10,3 sites par seed, ~830 opérations de construction contre 351
  sites scannés, 4 % du travail sur les ancres à ≥ 128 seeds
  (`ADDENDUM_DESCENTE_WSPD_ET_CHARGE_Q3_20260819`). Survit : le scan plat.
- **Plafond « pic projeté » de résidence.** Deux fois faux : d'abord sur le
  seul flux d'événements (facteur 5,77), puis sur `min(max(budget, max_K m_K),
  Σ m_K)` qui oublie les résultats terminés résidents (audit `bab37b9`).
  Survit : une comptabilité par rôle (persistant / en construction /
  temporaires / amont) et des majorants qui refusent avant allocation.
- **Cover q4 au coefficient 4 pour la génération (v5, première version).**
  Sans effet sur l'objet (le census passe par l'arbre) mais change
  `digest_balls` en tuant des candidats profonds avant l'émission (audit
  bloquant `87e915bd`, 23 boules sur `uniform n=8000`). Survit : coefficient 3
  (la lentille des sommets), fixture `q4_cover_fixture`, mutant
  `q4-cover-coef4`.
- **Étage i64 du préfiltre q4 comme gain de temps.** Aucun gain mesurable au
  banc apparié (médiane 1,0021, 8/20) ; conservé sur un argument compté (~40 M
  de multiplications i128 en moins), jamais chronométré.
- **Refuser les coquilles pour rester en position générale.** Change les
  composantes HGP (carré cocyclique). Survit : le plateau sphérique comme
  quotient exact, `resource_exhausted` au-delà du plafond.
- **Deltas de fusion seuls (`ForestResult` sans naissances ni croissances).**
  Partitions justes mais K-polyèdres faux (une facette absorbée sans fusion
  est une croissance). Survit : `ComponentDelta` ; mutant `drop-nonmerge`.
- **`first_batch` comme entrée du calcul.** Sur un flux sans
  `attach_violations`, redondant ; remplacé par un bit `seen` mis à jour
  après le lot, instrument du seul détecteur.
- **Comparer des constantes entre deux processus.** `t_fold` varie de ±40 %
  d'un processus à l'autre ; seul un banc apparié contrebalancé intra-processus
  (médiane des rapports par paire) conclut.

## Patterns d'erreur récurrents (à reconnaître avant de les reproduire)

1. **Promettre avant de mesurer** : un poste « dominant » désigné par
   raisonnement (scan q3) alors que la mesure montrait la descente WSPD à
   72 % du mur.
2. **Deux bornes qui divergent** : un plafond et un ordonnanceur calculant
   chacun leur majorant ; une seule borne, partagée, ou aucune.
3. **Un cap dans un critère terminal** : une propriété d'ordonnanceur glissée
   dans une définition mathématique.
4. **Un témoin partagé entre sujet et juge** : primitives de production dans
   l'oracle, ou implémentation figée comme « témoin ».
5. **Un statut déclaré au lieu d'une mesure** : « parallélisé » sans compteur
   d'ouvriers ; « pic » sans lecture de RSS ; « conforme » sur un seul digest
   (la campagne appariée v5 ne comparait que `digest_all`).
6. **Un digest qui mesure un filtre, pas l'objet** : `digest_balls` compte les
   candidats profonds ; il grave la force d'un filtre fail-open.
7. **Un vert par vacuité** : un contrôleur documentaire qui ne couvre pas le
   dossier, une porte sans plancher.
8. **Un mutant hors de sa porte** : déclaré sans point d'injection, ou injecté
   sans porte à code 4, ou tué par une divergence préexistante du bras nominal.
