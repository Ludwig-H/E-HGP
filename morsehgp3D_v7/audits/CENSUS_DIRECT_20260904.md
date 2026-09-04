# Census direct v7 : contrat courant

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Le census possède une destination privée de taille égale au nombre de
survivantes. Chaque travail écrit à l'indice global de sa survivante, dans
un intervalle disjoint. L'ordre ne dépend donc pas de l'achèvement des fils.
Tous les travaux sont joints avant la lecture ordonnée des statuts. Un
refus de coquille ou une contradiction count-only invalide la destination ;
seul un résultat complet la publie par `swap`.

`census_merge_peak_bytes`, sous le contrat
`kCensusStorageVersion=mhgp7-census-direct-v1`, mesure les capacités des
tableaux `BallData` possédés simultanément par le census. La destination
est comptée dès son allocation. Le mutant `keep-ball-chunks` possède une
copie supplémentaire réelle, comptée avant destruction. Cette métrique ne
borne ni le RSS ni les autres étapes du pipeline.

Le calcul des tranches est un plan sans lancement de fils. L'expansion
régulière trie la coquille dans un tableau borné ; les plateaux gardent leur
expansion complète. Le tableau global de boules reste vivant pendant les
folds, et les sorties de l'expansion nécessitent encore leur propre étude
de résidence.

La source, les acquis et les actions de qualification sont détaillés dans
[l'audit de résidence courant](AUDIT_RESIDENCE_20260904.md). Les compteurs
de stockage doivent être jugés avec leur version d'instrument. Aucune mesure
RSS ou résultat de grande échelle ne découle de la suppression des shards.
Aucune mosaïque de Delaunay ni population combinatoire nouvelle n'est créée.

GCP non utilisé par l'auditeur.
