# Revue de porte Phase 11 — tour verticale réduite

> [!IMPORTANT]
> Cette revue ferme administrativement la Phase `11` sur deux autorités séparées : l'oracle Gamma exact de Phase 1 et la couture directe conditionnelle 11A. Elle ne ferme ni G4 produit, ni M.1, ni la fidélité horizontale globale du flot direct, et ne publie aucun `VerticalMap` v2.

## Décision

- Phase : `11` — tour verticale réduite.
- Backend de référence : `reference_cpu`.
- Profil : `hgp_reduced`.
- Mode : `certified`.
- Déploiement direct : `architecture_only`.
- Statut direct : `conditional_vertical_candidate`, `public_status=not_claimed`.
- Porte d'entrée : satisfaite par la fermeture conditionnelle de Phase 10.
- Verdict de sortie : satisfait administrativement; l'exactitude verticale de référence reste portée par Gamma exhaustif, tandis que le chemin direct peut être absent, partiel ou localement vérifié.
- Effet opérationnel : la Phase 15 devient prête; aucune qualification finale de Phase 14 ou G4 n'en découle.

## Séparation des autorités

L'oracle de Phase 1 construit les inclusions entre composantes Gamma, vérifie l'unicité des cibles et ferme les carrés entre toutes les coupes exactes consécutives. Cette branche reste l'autorité verticale exacte bornée.

11A ne reconstruit pas Gamma. Pour chaque groupe atomique du journal horizontal direct, il trie et déduplique les clés de bras, conserve seulement le plus petit indice de binding représentatif, puis reçoit une proposition externe absente, non résolue ou résolue. Une cible résolue doit appartenir à l'ordre inférieur adjacent et être née au plus au niveau du groupe; un sweep DSU de la seule forêt cible la normalise dans l'état fermé du même niveau.

Les naissances $q_R=0$ ancrent leur nœud lorsque tous leurs labels concordent. Les continuations $q_R=1$ propagent le checkpoint courant ou posent un checkpoint tardif explicitement incomplet. Les multifusions $q_R\geq2$ comparent séparément chaque enfant disponible à la cible commune; un carré déjà vérifiable reste compté même si un autre enfant manque. Toute cible présente contradictoire, future ou de mauvais ordre rejette le journal atomiquement.

## Évaluation de la porte

| critère | preuve | décision |
|---|---|---|
| images Gamma de référence uniques | oracle exhaustif et revue de Phase 1 | go dans le domaine borné |
| carrés Gamma de référence | tests d'unicité et naturalité à toutes les coupes exactes | go dans le domaine borné |
| flèches directes | absentes, non résolues ou candidates vérifiées relativement aux graines fournies | go conditionnel |
| état fermé partagé | sweep de la forêt cible avec activation `<= niveau du groupe` | go |
| composition multiordre | trace locale bornée, avec arrêt explicite si un checkpoint manque | go conditionnel |
| périmètre réduit | compte par famille des naissances isolées d'ordre supérieur omises; une cible absente n'est jamais reclassée comme isolée | go |
| structures globales | aucune facette globale, coface, incidence, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur | go |
| autorité externe et M.1 | non rejoués | obligation ouverte |
| statut produit | `all_naturality_squares_replayed=false`, `vertical_maps_complete=false`, `public_status=not_claimed` | pas de promotion |

## Validation volontairement courte

- C++ Release strict : build de `morsehgp3d_hierarchy_direct_morse_vertical_journal_tests`, réussi.
- CTest ciblé : `morsehgp3d.hierarchy_direct_morse_vertical_journal`, `1/1` réussi en `0,01 s`.
- Cas discriminants : $q_R=0,1,\geq2$, ordre des clés opposé à l'ordre des bindings, labels absents et non résolus, un carré vérifié à côté d'un carré non vérifiable, checkpoint tardif, familles adjacentes vides, naissances isolées omises et trace `3 -> 2 -> 1`.
- Rejets : cible future, mauvais ordre, conflit relatif, proposition étrangère, budgets juste insuffisants et forêts dont les arènes, parents ou racines ne forment pas les partitions attendues.
- Oracle Python de Phase 1 : deux tests ciblés d'unicité et naturalité, `2/2` réussis en `0,128 s`.

Aucun benchmark long, sanitizer, CUDA ou oracle combinatoire supplémentaire n'est utilisé pour cette fermeture.

## Coût et structures évitées

Pour une paire d'ordres, le journal conserve les nœuds de forêt, les groupes, les labels représentatifs et les checkpoints. Le scratch de tri est local à un groupe et le sweep cible est libéré avant la famille suivante. Les clés et `PointId` ne sont pas recopiés dans les résolutions.

Le chemin produit ne matérialise ni Gamma, ni univers de facettes, ni cofaces ou incidences globales, ni cellules, ni mosaïque de Delaunay d'ordre supérieur. L'oracle Gamma peut falsifier ou recertifier une petite sortie, mais il n'est jamais l'architecture par défaut.

## Limites maintenues

- Le journal source de Phase 10 reste un candidat horizontal conditionnel à la fidélité globale des carriers.
- Les graines de cible proviennent d'une autorité externe que 11A ne rejoue pas.
- Les identifiants de nœuds sont locaux au journal et ne sont pas des identifiants publics durables.
- Les contrôles élémentaires directs ne remplacent pas les carrés Gamma de toutes les coupes.
- M.1, `full_pi0`, G4 produit, le SLO 50 k et la capacité 10 M+ ne sont pas fermés par cette revue.

## GCP

GCP non utilisé.
