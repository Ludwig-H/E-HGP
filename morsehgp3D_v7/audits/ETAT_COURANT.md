# État courant de l’audit v7

11 septembre 2026, reprise sur **99b4d3b1**, puis piste MEB **d2ed4d48**. Une [contre-fixture de sept points u16](receipts_meb_boundary_20260911/README.md) réfute le remplacement total par ce prototype : sa proposition oublie un point, sa canonisation refuse. Le repli sur `anchor_meb`, déclenché pendant le balayage de coquille, retrouve les champs exacts O2/SAN ; aucune modification du moteur actif ni nouveau gain mesuré.

La contrelecture des quatre nouveaux paquets privés est favorable : couture CPU, contexte hôte semé, extension K9/K10 c03 et candidat CTest census→tour. Lecteurs normal/−O et [empreintes](ENTRETIEN.json) conservés. Le raccord cumulatif consomme bien Builder 83f1 et adaptateur 993786f3 : demande close, [contre-fixture initiale](receipts_batch_work_20260911/README.md) inchangée. Notre lecture de la porte couvre onze CTests privés O2/SAN ; le constructeur annonce désormais 40/40 CTests sur son raccord actif, en préparation de publication.

Le [graphe filtré réduit aux naissances](receipts_filtered_graph_20260911/README.md) garde sa preuve conditionnelle et son modèle indépendant sur 267 cas. Cette proposition permet de conserver coupes, parents, contributions et ancres ; son raccord C++ reste à qualifier.

Le [semis après échange](../docs/SEMIS_APRES_ECHANGE_20260911.md) est désormais intégré, header 6763a877, avec qualification O2/SAN et 24 CTests. Le triplet mono 8k/16k/32k évite 237 557 / 501 258 / 1 045 620 MEB supplémentaires, à sorties égales ; pas de speedup apparié aux anciens temps. Le [vrai census→tour à K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) est maintenant jugé sur n12/n14, puis rejoué sur ce header : 54 tours par build, 120 hits après échange. Sept lecteurs de paquets passent normal/-O dans notre complément.

Le [premier raccord FULL incrémental](../receipts/incremental_full_trial_20260911/README.md) a été essayé et qualifié, puis non retenu pour ses allocations et capacités. Les [gardes](receipts_journal_guards_20260910/README.md), [préfixes indépendants](receipts_incremental_prefix_20260911/README.md) et [calcul net de résidence](receipts_incremental_review_20260911/README.md) restent ses preuves historiques ; la demande d’essai est close. Aucun défaut nominal du journal daté n’est ouvert.

Le [second auditeur](receipts_cache_commit_20260911/README.md) conserve sa qualification relative ad7ffd28, variante Q : lots groupés et contrôle nommé 50k clos sur CPU. Les [tours 50k CPU/hybride](../docs/RESULTATS_TOUR_CACHE_G4_20260910.md) restent des diagnostics sur d’autres octets. Archive industrielle, complétude globale et contrats 1 s/100 ms ou massifs restent ouverts ; la contrelecture hôte des nouveaux prototypes ne qualifie ni leur exécution device ni leur vitesse.

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
