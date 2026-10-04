# Couverture de l'audit transversal — 4 octobre 2026

Pin produit relu : `0f5e8a207f2974e262cd40a8882b97af1da396af`.
Le développement publié pendant l'audit, `0af635a71`, ajoute les démos de
bouts et leur reçu ; les octets `src`, `bench`, `tools`, `tests` v11 sont
identiques au pin relu (`LATEST_SOURCE.json`). Les chantiers privés restent
des objets de lecture, pas des sources implicitement portées.

`INVENTORY.json` inventorie les 506 fichiers publiés v11 hors audits et
receipts. Les **101 fichiers de src** sont répartis entre les trois
contrelectures et la revue tower centrale ; tous les modules implémentés
ont une relecture de leurs implémentations/interfaces. Les tests sont
inventoriés, leurs portes importantes et leurs résultats sont recoupés ;
ceci ne prétend pas relire chaque ligne de tous les anciens harnesses,
ni prouver chaque entrée possible par une campagne finie.

| Périmètre | Invariants difficiles contrôlés | Preuve et limite |
|---|---|---|
| core (9), cloud (4), sched (4), CMake et IO de banc | statuts sans résultat partiel, budget des Buffer, multiplication et capacités, propriétaires privés, multiplicité/IDs, Pool synchrone et époques | Capsule fondations ; 373 portes natives conservées recoupées sur sources identiques c40, pas rejouées ; lecteur portable431. Session/CLI publics encore absents. |
| num (18), index (6), catalogue (28) | domaines u18/u21/u24, q3/q4 positifs, rationalité, FENV, boîtes fermées, I/U et S* complets, qmin vs présentation, G1/G3/J2, reset des caches, frontier/replay, assemblage/refus | Capsule géométrie, 12 invariants ; source-parité c40, témoin exact340. Pas nouveau benchmark natif. |
| tower (32) | MEB canonique ≤4 supports, q4 sans rejet de son préfixe q3 ; locate du même propriétaire ; descente strictement décroissante et graines datées ; fenêtre p+q−1..p+m et toutes traces strictes | Relecture intégrale des implémentations/interfaces ; théorie et oracles dans capsule math. Mémo et lookup de populations conservent date/graine, jamais top DSU. |
| Forêts, multifusions et verticales | classification/tous ordres, permutation des cohortes exactes, DSU à un écrivain, 2b−1/2b−2 capacités, naissances avant plateaux, liens stricts, activation fermée de chaque enfant | Sorties b872 qualifiées conservées ; tous les src actuels identiques b872 (`NATIVE_PARITY.json`). Ancien juge v10 insuffisant, oracle v11 distinct des diff v10. |
| Pipeline/concurrence/refus | tâches logiques et scratch disjoint, publication release/acquire et ordinal, lecteurs de naissance/parents distincts, pas attente circulaire, admission census possédés et réutilisés | Chemin normal favorable. **Nouveau trou au réveil sur abandon**, test de contrôle exact72 ; pas faux succès FULL ni race TSan reproduite. |
| FULL→points/frontière/statistique | couverture fortement ensembliste, équivariance, deux triangles, propriétaire au plateau fermé, H3 pour k/m fixés, croisement entre k, poids/population/insertion distincts | Capsule math ; nouvelles coupes/Γ/P3 bornées, lemme B3 et comparaison AST. Points actuels : export C++ + calcul Python ; pas module natif points. |
| Hiérarchie→plat | condensation des cohortes tardives, score/EOM exact et égalités, racine/mcs/noise, sélection en antichaîne ; changement de z à calendrier fixé ; complétion distincte | Reçus exacts flat et EOM du même jour déjà clos, complétés par lemme B. Prototype compact O(n) encore plan ; budgets algébriques/refus et format restent à qualifier nativement. |
| Synthétiques, Zoltan et équité | contre-fixtures indépendantes et mutants, exact vs flottant, meilleur bloc vs sélection automatique, HDBSCAN officiel vs tête commune, IoU simultané et univers de blocs | Campagnes conservées et prototypes lus ; nouveau lot360 bouts, 10 séquences, 107..17593 sites,789 observations corrélées. Pas validation de trames entières, de tête plate ou de supériorité statistique. |
| Temps/mémoire/massif | FULL total K5/u21/W48, préparation/sol/IO exclus publiés, sorties identiques appariées, phases qui se recouvrent, Buffer vs RSS, taille des coquilles/sorties | Dernières médianes412/352/381ms pour trois trames sans sol séquence08 ; pas100ms/GPU/dizaines de millions. Coûts sorties potentiellement quadratiques : pas extrapolation d'une primitive ni de 2n−1 points à FULL. |
| G4/preuves/harnais | commit/paquet/binaire/manifest, signaux et timeout, sélection native réelle, profils/sanitizers distincts, verrou commun et arrêt certifié | Revue dédiée G4 ; mock matrix37 et host87 normal/−O ; récent archive427payloads rehachés. Pas nouveau lancement GCP par les auditeurs. |

Les pièges v10 et antérieurs sont suivis par propriété précise, pas par nom
de version : fold incomplet E5, booléen universel faux, arité/qmin confondus,
préfixe obtus rejetant q4, contact perdu, shell tronquée, descente/DSU mémo
non datée, multifusion binaire à égalité, Euler pris pour un juge suffisant,
FENV/clé totale, alias/move, scratch/worker, faux succès de harnais et score
de cohortes. Les constats déjà corrigés ne sont pas présentés comme de
nouveaux tickets. Les capsules indiquent quand une preuve dépend de MEB≤4
et de Γ↔Lk plutôt que d'un second calcul continuum totalement indépendant.

Les limites ouvertes de livraison sont séparées des défauts actuels :
aucune nouvelle non-conformité n'est imputée à un module `points`, `head`,
`api` ou `cli` encore absent. La rigueur du moteur fini à sites unitaires
ne prouve pas un optimum statistique pour une tête plate, ni un coût global
linéaire, ni un résultat sur une population infinie.
