# État courant de l’audit v7

11 septembre 2026, après **e3903b2a**. Priorité : paralléliser toute la tour
et maîtriser les intermédiaires.

**Suite constructive : étendre les rangs certifiés aux gardes du scatter.**
Le [dialogue courant](DIALOGUE_COURANT.md) donne les trois substitutions
exactes, avec les mêmes contrôles de domaine et d’admission. Sur le témoin
ordonné 32k, cela concerne 90 662 398 comparaisons de scatter et 26 901 500
de semis initiaux. Ce comptage logique est reproductible ; aucun gain de
temps ni retrait de MEB n’est revendiqué. Les gardes des boules dynamiques
restent exactes, distinctes des rangs du catalogue.

La lecture du pool CPU persistant et du raccord parallèle est favorable :
une fenêtre, scratch privé, barrière avant scatter et consommation ordonnée
de φ/DSU. Le tri, la préparation et la réduction restent séquentiels. Le
runner des fixtures concurrentes doit encore borner ses commandes par un
timeout. La capture dense privée O2 **87eb210e** a été contre-vérifiée en
lecture : 26 commandes, 114 census, 456 essais et trois fautes réellement
injectées. Le constructeur prépare la publication O2/SAN et du pool ; ces
qualifications restent distinctes de notre lecture et du moteur actif.

Le [partage de préparation et les semis liés](receipts_prepared_catalogue_20260911/README.md)
sont acceptés par le développeur comme delta suivant. Le détail de leur
preuve, les conventions de masques et la contre-fixture de seuil restent
dans le paquet ; les recommandations déjà reprises ont été condensées.
Le [réducteur ordonné](../receipts/ordered_streaming_20260911/README.md),
la [contraction](receipts_birth_stream_20260911/README.md),
l’[export](receipts_historical_export_20260911/README.md), la
[composition MSF](receipts_composable_msf_20260911/README.md) et les
[objets parallèles](receipts_parallel_objects_20260911/README.md)
conservent leurs preuves et leurs limites, sans redemander les premières portes.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La lecture des parties I–II du manuscrit, PDF 35–134, reste acquise. La [cible FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md) distingue minima, multifusions, portails, K=n, régularité, verticale et poids. La [vraie tour K-NN](receipts_gabriel_vertices_20260906/README.md) demande les bonnes connexions entre minima ; restreindre les anciennes adjacences aux seules facettes Gabriel ne suffit pas.

| Autorité | Résultat conservé |
| --- | --- |
| Q/P, cache ad7ffd28 et ancres d188e3de (second auditeur) | [Rejeu commité et blocs nommés 50k](receipts_cache_commit_20260911/README.md), [contrelecture initiale](receipts_raccord_ancres_20260910/README.md) ; aucune réattribution au header courant |
| Journal v2, `1fbe49d3` | [Arènes complètes, multifusions, lots mixtes et coupes larges](receipts_coverage_cpp_20260910/README.md) ; qualification de composant, pas nouvelle variante moteur |
| O, raccord FULL `20b28b1d` | [116 ordres, budgets, K9/K10 et deux mutants](receipts_full_meb_20260906/README.md) ; builds indépendants, captures constructeur 30+30 contre-vérifiées |
| N, filtre privé publié par `62e5cd76` | [Captures R2, frontières MAX et ordre admissible](receipts_filtered_review_20260906/README.md) ; qualification locale historique distincte |
| M, publication et captures `5633bc5a` | [29 comparaisons / 204 ordres, rejeu s8 et diagnostic du refus MEB](receipts_followup_20260906/README.md) ; lectures seules, aucune nouvelle qualification C++ ou de performance |
| L, successeurs v2 `85c27ab9` | [114 ordres, 912 sorties et 69 120 coupes par build ; 3 851 appels du helper](receipts_full_successor_20260905/README.md), deux mutants ; captures constructeur 20+20 contre-vérifiées |
| K, lot unitaire `21b77d29` | [114 ordres, 912 sorties et 69 120 coupes par build](CACHE_FULL_COURANT.md) ; mutation du quatrième parent ; captures constructeur 17+17 contre-vérifiées |
| J, lazy `13c6cc72` | [109 ordres et 67 920 coupes par build O2/ASan-UBSan](CACHE_FULL_COURANT.md), quatre politiques, budgets, trois mutants ; 14+14 CTests propres, admission n=8 de la sonde et first-C contre-vérifiés |
| I/H, EAGER `e02d163c` | [100 ordres indépendants](PRODUCTEUR_FULL_GABRIEL_COURANT.md) ; [trois réussites mono 8k et deux refus d’alias](MONO_FULL_COURANT.md), sans transfert de leurs temps vers lazy |
| G, lecteur FULL | [Qualification structurelle](CERTIFICAT_FULL_CPP_COURANT.md), sans certification géométrique |
| D/E/F, réduit et primitives | [Qualifications distinctes](AUDIT_QUALIFICATION_20260905.md) ; aucun reçu réduit réinterprété FULL |

Le [manifeste](validation_current.json) conserve les variantes D–Q sans réattribuer leurs résultats au header courant. La liaison du journal publié avec son reçu borné est acquise ; cette passe ne crée pas une nouvelle qualification globale de la tour. Les écarts de source restent visibles dans le contrôle de fraîcheur.

Le [dialogue](DIALOGUE_COURANT.md) ne répète plus les demandes satisfaites ; les [questions secondaires](QUESTIONS_SECONDAIRES.md) restent regroupées. Les preuves et échecs scientifiques sont inchangés ; l’[entretien](ENTRETIEN.json) garde la provenance des condensations. GCP non utilisé par cet audit.
