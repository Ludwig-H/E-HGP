# Périmètre, preuves et limites de l'audit d'ouverture

13 septembre 2026. Ce rapport rend l'audit vérifiable sans confondre
ampleur de couverture, lecture exhaustive des octets et certification.

## Base et changements exclus

Base v7 publiée : `dc57ffd5fec5b73aff9bc7f79f280fb8bc92a6d1`.
Arbre Git v7 : `6bcd23739cb9b93cef0b19c3a253dfb281cc2c7e`.
L'[inventaire JSON](../receipts/audit_v7_20260913/INVENTORY.json)
dénombre 26 777 chemins suivis, dont 57 fichiers source, 86 tests,
24 fichiers bench, 66 documents, 23 399 fichiers de reçus et 3 136
fichiers d'audits. Les archives de preuves représentent donc l'essentiel
du nombre de fichiers, pas autant de modules du moteur.

La sélection détaillée contient 240 fichiers : sources, tests, oracles,
bench, CLI, cmake, manifeste CMake et docs. Leurs blobs Git, SHA256 publiés
et SHA256 locaux sont enregistrés. 238 sont identiques à la base ; deux
documents locaux diffèrent. L'inventaire consigne aussi les autres
modifications locales, notamment README/PASSATION v7.

Ces deltas, le paquet fused_history non committé, les modifications v6 et
le journal v7 préexistant restent intacts et **hors du commit v8**.
Les citations de leurs sections anciennes ne promeuvent pas leurs ajouts
locaux en résultats publiés. Les sources privées publiées sont citées
par manifeste/objet, pas par dépendance à un `build/` éphémère.
Le registre `docs/implementation_status.toml` n'est pas modifié.

## Lecture mathématique et sources premières

Le manuscrit est [conservé dans le dépôt](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
SHA256 `579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.
Les parties I/II, PDF 35–134, ont été lues intégralement dans la phase
v7 antérieure ; cette lecture est consignée dans
[LECTURE_ET_CONTRATS](../../morsehgp3D_v7/docs/LECTURE_ET_CONTRATS.md).
Nous ne prétendons pas avoir refait une lecture intégrale de ces cent
pages pendant cette seule passe d'audit.

Passages réexaminés directement ici : PDF 83–87 (définitions et connexité),
110–117 (Gabriel, connexions, réductions), 125–127 (poids et algorithme1).
Les entrées v7 et la
[décision indépendante sur FULL](../../morsehgp3D_v7/audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md)
ont été relues. Les contre-preuves E5, sommets Gabriel, plateaux/ancres et
sortie quadratique sont reliées dans le [volet mathématique](FONDEMENTS_ET_OBJET.md).

La source originale Callahan–Kosaraju est utilisée pour distinguer la
preuve de sa WSPD des propriétés effectivement prouvées de l'arbre v7.
Les références primaires RCTT et PANDORA éclairent les propositions de
contraction parallèle ; leurs temps ne sont pas transférés à HGP.
Les liens et la portée de chaque emprunt figurent à l'endroit des
arguments dans le [volet WSPD](WSPD_Q2_Q3_Q4.md) et le
[plan de refonte](../docs/PLAN_DE_REFONTE.md).
Ce complément ciblé n'est pas une revue exhaustive de toute la littérature.

## Couverture de l'implémentation

| Domaine examiné | Ce que l'audit trace | Rapport |
| --- | --- | --- |
| Entrée, exactitude, index | Profil u16, clés, identités, bornes d'arithmétique et de représentation | Implémentation |
| WSPD et témoins | Couverture, séparation, noyau universel, h/h_a/h_b, coûts des facteurs | WSPD |
| q2/q3/q4 | Ancrage canonique, seeds, covers, intervalles/racines et plateaux | WSPD + algorithme expliqué |
| Tri/census/catalogue | Canonisation, exactitude d'une entrée versus complétude, copies/transport | Implémentation + mesures |
| Rattachements FULL | Builder, cache/statique, MEB, intrus, facettes uniques, Atlas/flux privés | Implémentation + mathématiques |
| Histoires/contributions/verticales | Égalités, identités, graphe daté, reconstructions et consultations | Implémentation + mathématiques |
| Backends | Mono, quatre workers, 48 CPU amont, kernels réels/compilés/hôte | Implémentation + mesures |
| Contrats | 50k1..10/1..5, 100 ms, triplets, s8/10/12, massif | Mesures |
| Livraison | CLI F versus FULL, archive, reprise, mémoire/offsets, CI et tests | Implémentation |

Chaque rapport fournit ses fichiers et, lorsque utile, ses lignes ou
empreintes. Cette couverture traverse la chaîne entière. Ce n'est pas
une annotation ligne par ligne des 240 fichiers, ni une revue de chaque
fichier opaque de reçu. Les gates sont examinées par familles de preuve
et par pièces pertinentes, pas toutes réexécutées.

## Contrelectures et indépendance

Trois agents ont travaillé séparément sur WSPD, mesures et architecture ;
le constructeur principal a traité fondements, synthèse et plan.
Ils ont ensuite contrelu les rapports voisins et les nouveaux exposés.
Corrections retenues : distinguer une MEB d'une descente, préciser les
compteurs CPU/GPU communs, le rôle des singletons, les conditions de
régularité, les voies actives, le volume partagé/cumulé et les index résidents.

Cette équipe est une contrelecture du constructeur, **pas l'auditeur
indépendant propriétaire de `morsehgp3D_v7/audits/`**. Ses rapports
antérieurs sont des sources séparées. Le nouveau
[journal de coordination](../../audits/COORDINATION_MORSEHGP3D_V8.md)
pose les questions ouvertes ; aucune réponse future n'est anticipée.

## Commandes réellement réexécutées

[CONTRACT_CHECKS.json](../receipts/audit_v7_20260913/CONTRACT_CHECKS.json)
conserve six commandes Python normal/`-O`, codes0 : lecteurs des paquets
50k du 10 septembre, rank_guard du 11 et tentatives G4 terminal du 11.
Ils vérifient les captures closes et leurs liens, pas une nouvelle
exécution C++ ou une nouvelle interrogation GCP. Le champ historique
`CTests` retourné par un lecteur ne compte pas des CTests relancés aujourd'hui.

[MATH_CHECKS.json](../receipts/audit_v7_20260913/MATH_CHECKS.json)
conserve deux recalculs en arithmétique exacte du témoin de sortie,
normal/`-O`, codes0. La sortie est filtrée pour éviter de recopier toutes
les coordonnées ; `pipefail` garde l'échec du calcul propagé. Les pins et
planchers non vacus sont conservés. Un premier affichage intégral a été
tronqué par l'outil et n'a pas servi de preuve enregistrée ; ces deux
commandes compactes sont les captures exploitables.

Les contrôles documentaires, du registre et de découverte du périmètre
v8 sont consignés séparément dans
[la vérification de livraison](../receipts/audit_v7_20260913/PUBLICATION_CHECKS.json).
Ces succès ne certifient ni les preuves mathématiques ni un moteur v8.

## Ce qui n'a pas été fait, et pourquoi

Aucun benchmark lourd local ni cloud, aucune compilation C++, aucun CTest
du moteur, aucun replay complet de toutes les captures et aucun contrôle
de run CI distant. La demande prioritaire était l'audit avant de recoder ;
les mesures existantes suffisent à établir les écarts et localiser les
problèmes. Acheter une VM pour rechronométrer le même chemin n'était pas
nécessaire à ce diagnostic.

Le contrat 50k récent sur la future chaîne, les familles FULL autres
qu'uniforme aux trois grandes tailles, s10/s12 sur G4, le terminal composé
device, la chaîne GPU entière et les dizaines de millions restent donc
ouverts. Aucune conclusion d'exactitude industrielle ou de complexité
universelle n'est tirée des seules vérifications documentaires.
GCP non utilisé.
