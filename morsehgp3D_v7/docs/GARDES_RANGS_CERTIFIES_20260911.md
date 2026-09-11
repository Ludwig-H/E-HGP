# Gardes de catalogue par rangs certifiés

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Delta privé du [raccord CPU persistant](CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md),
sans remplacement du moteur actif. GCP non utilisé.

## Ce qui change, et seulement cela

L'Atlas construit les rangs par tri exact des niveaux, regroupe leurs
égalités, puis vérifie chaque liaison boule→rang→niveau. Après cette
validation, les rangs ont les mêmes relations d'ordre que les niveaux.
Cela reste vrai pour des fractions brutes distinctes mais équivalentes.
Les fractions originales restent disponibles, sans renumérotation des
plateaux ni changement de leur représentation historique.

Trois gardes répétées du traitement par fenêtres utilisent désormais ces
rangs entiers :

| Usage | Condition conservée | Nombre sur une exécution complète |
| --- | --- | ---: |
| Semis initial de la facette | rang terminal < rang premier consommateur | ΣSw |
| Dispersion dans un groupe | rang premier consommateur ≤ rang consommateur | R |
| Terminale K≥2 | domaine boule, admission K, puis rang terminal < rang consommateur | R−R_K1 |

R compte les occurrences, Sw les réponses par semis initiaux dans chaque
fenêtre. Il ne s'agit ni du nombre de semis stockés, ni du nombre de MEB.
Les trois compteurs `rank_guard_work` sont séparés des comparaisons déjà
entières du réducteur (`BirthWork.native_date_comparisons`). Les nouveaux
helpers vérifient leurs domaines avant d'accéder aux rangs. Les compteurs
rapportés concernent les succès complets, pas le travail avant exception.

À K1, la dispersion conserve sa garde entre consommateurs, mais la
terminale est un indice géométrique de point : aucun accès BallId ne lui
est appliqué. Les gardes exactes des MEB initiales et intermédiaires, les
tests géométriques, les admissions et le niveau brut transmis au resolver
restent inchangés. Aucune nouvelle structure globale n'est construite.
L'espace occupé par les compteurs est constant par ordre ; leurs incréments
et le travail remplacé sont linéaires en occurrences et en réponses par
semis, pas en paires de points.

## Prémisse de réutilisation

Le même catalogue, index et Atlas doivent rester immuables et vivants
pendant tout le calcul. Un helper recevant une référence const ne prouve
pas cette propriété à lui seul. Il ne recertifie ni l'identité du
propriétaire ni la complétude géométrique du census. Les producteurs
extérieurs doivent établir leurs propres prémisses ; des métadonnées
forgées ne deviennent pas fiables grâce à leurs rangs.

La [contrelecture indépendante](../audits/receipts_prepared_catalogue_20260911/README.md)
autorise ces trois substitutions. Le comparateur de programme de l'Atlas
pourrait également changer, mais ce quatrième site reste volontairement
exact dans ce delta, pour isoler la mesure.

## Qualification et mesures

Les [reçus propres](../receipts/rank_guard_streaming_20260911/README.md)
ferment O2/SAN sur le raccord FULL : 114 census, 912 essais, 506 448
terminales comparées et 48 775 524 contrôles par build. Tous les anciens
champs scientifiques du parent sont égaux ; les 4 368 contrôles nouveaux
vérifient les compteurs par ordre. Gardes ciblées O2/SAN : 18 census,
36 Atlas, 2 796 couples de boules, 12 882 contrôles et sept mutations
réellement exécutées/refusées. Domaines, plateaux, liaisons de rangs et
fractions équivalentes sont discriminants. Aucun nouveau verdict TSan.

Le mono8k est clos à 206,224207575 s (phase atlas/géométrie/réduction
62,453518709 s). Le triplet CPU4 donne 96,401057050 / 223,446536141 /
478,615724962 s pour toute la tour 8k/16k/32k ; la phase modifiée prend
32,134279819 / 73,197374312 / 158,710478073 s. RSS respectifs :
2 874 208 / 5 807 080 / 11 607 552 KiB. Mêmes digests, sorties et MEB
que les parents ; 25 907 749 / 55 773 201 / 117 563 898 gardes désormais
entières. Les petites comparaisons physiques s8/10/12 à n800 passent.
Douze captures, 102 commandes closes, toutes réussies.

Le témoin mono parent donnait 187,214349752 s. Les comparaisons temporelles
ne sont pas appariées ; des sondes utilisateur ont chevauché le mono et
les premiers runs CPU4, et l'amont non modifié varie aussi. La phase 32k
est favorable contre le parent (171,384→158,710 s), mais aucun gain de
latence reproductible n'est établi. Les comptages d'occurrences/MEB et
les rapports de croissance figurent dans le paquet ; ils restent identiques
au parent et ne prouvent pas une borne tous régimes.

Le [triplet parent scellé](../receipts/parallel_birth_streaming_20260911/README.md)
reste la référence historique CPU4 : 92,963 / 215,381 / 489,601 s pour
8k/16k/32k. Ce ne sont pas les temps des nouvelles gardes. La sortie FULL
explicite, les histoires et l'export restent volumineux ; aucune borne
sous-quadratique universelle ni promesse 50k sous une seconde ne découle
de ce remplacement local.

## Prochains coûts distincts

L'auditeur a aussi établi que les marques fermées produites par les
histoires peuvent répondre aux consultations contributives, et que deux
index de chaînes adjacents peuvent être réutilisés (19 préparations→10
pour K1..10). Ces raccords restent séparés : conserver les dates des
contributions, leur ordre source et les consultations propres des
verticales. Ils ne sont pas inclus dans le delta de gardes.
