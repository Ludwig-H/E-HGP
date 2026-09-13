# Comparaisons appariées : préserver l'identité des répétitions

13 septembre 2026, quatrième passe après `65ac5ee6`.
Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Sources en construction ; les empreintes successives sont
distinguées dans le [reçu](PAIRED_PROVENANCE_CHECKS.json).

**État à la publication : les trois défauts ci-dessous sont corrigés et
contre-vérifiés sur leurs versions épinglées.** Les contre-exemples restent
conservés ; leurs clôtures sont décrites dans chaque section.

## P1 clos — les médianes mélangeaient des builds différents

Le nouveau lecteur apparié vérifie individuellement ses campagnes,
mais son regroupement des répétitions ignore l'identité de leur build
et de leur machine. Le défaut est reproduit avec **deux vraies captures**
du même code C++ figé : l'une compilée en Release avec `-O2`, l'autre
en Debug sans optimisation. Leurs SHA256 exécutables sont différents.
Chaque capture utilise n128 et les deux ordres d'exécution du batch.

Les quatre lignes sont admises et regroupées en deux configurations
portant chacune `repeats=2`, avec des médianes mélangeant les deux builds.
Le lecteur initial `b6339805…` puis sa révision `433ebb5d…` ont ce
comportement, en Python normal et `-O`. Les sources, résultats et hashes
ne sont pas falsifiés ; le manque se situe dans la clé de regroupement.

**Correction proposée :** refuser les provenances hétérogènes, ou les
partitionner avant tout résumé par identité du build et de la machine.
Conserver cette identité dans les résumés eux-mêmes. Une même fixture et
les mêmes sources ne suffisent pas à faire de deux mesures des répétitions
appariées ; le compilateur, ses options et l'environnement de mesure font
partie du protocole. Les deux ordres restent des configurations distinctes.

Ce cas est construit pour éprouver le lecteur. Aucun mélange n'est
allégué dans une campagne réelle du développeur, et les petits temps
obtenus n'établissent aucun classement de performance.

Le lecteur corrigé `f4148e22…` accepte la campagne Release homogène,
conserve l'identité du build dans ses résumés et rejette l'union
Release/Debug pour provenance hétérogène. Ces contrôles passent en
normal/−O sur les captures déjà conservées, sans nouvelle mesure ni
recompilation. La politique porte sur les métadonnées enregistrées ;
elle ne prétend pas identifier un hôte physique de manière unique.

## P2 clos — le marqueur de checksum axial n'était pas contrôlé

Un proxy remplace seulement le champ `checksum_kind` d'une vraie sortie
axiale par `fnv1a64_be_u64_axis_plan_v0`. Le runner et le lecteur acceptent
la campagne. Le validateur initial `d6f1ce17…` puis sa révision
`948751c8…` gardent cette omission. Le hash annoncé n'a plus une convention
admise et vérifiée, même si les autres champs sont cohérents.

Ajouter dans `validate_axis` l'égalité avec
`fnv1a64_le_u64_axis_plan_v1`, comme pour la convention du batch, puis
garder le mutant du marqueur. Ce contrôle de schéma ne prétend pas
recalculer le plan géométrique à partir du seul digest.

Le validateur corrigé `212efc6e…` accepte la ligne réelle et rejette
la même ligne au marqueur erroné, en normal et `-O` : quatre contrôles,
avec le diagnostic précis de convention incorrecte. L'addendum de
clôture conserve les valeurs et empreintes historiques.

## Corrections et contrôles positifs reconnus

L'ancien P2 sur `sheet` inactive est **clos** avec le lecteur single
`0ebf0c55…`. Une vraie capture n8/dual/q3/sheet/Kmax1/s8, seuil et
résidu nuls, passe désormais en normal et `-O`. Le reçu conserve aussi
le refus initial ; il ne réécrit pas les preuves antérieures.

Les comparaisons batch vérifient les crédits, IDs, ordres et blocs exacts
sur un même propriétaire avant publication du checksum. Les deux ordres
et la préparation commune sont explicites. Les vraies petites sorties
batch et axis sont admises ; les mutants de tuple et de JSON tronqué
sont invalidés avec leurs sorties brutes conservées.

Le [reproducteur](paired_provenance_checks.py) fige ses sources et refuse
leur changement au lieu de transférer les constats. Le reçu sépare ce
premier snapshot des vérifications ultérieures des lecteurs corrigés,
sans les transformer en qualification des nouveaux C++ en cours. La CLI
axiale n'était pas encore enregistrée dans le CMake capturé ; elle a été
compilée explicitement dans le snapshot, commande conservée. Ce détail
du brouillon n'est pas présenté comme un défaut de publication.
GCP non utilisé.
