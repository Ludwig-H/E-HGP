# État courant de l’audit v7

11 septembre 2026, reprise sur **abc960ac**. **Une réduction exacte de la canonisation MEB est prête à reprendre** : après certification par un support positif S et confinement complet, tester chaque candidat sur S suffit ; si la coquille U égale S, rendre directement le support trié. La [preuve et le helper C++](receipts_certified_support_20260911/README.md) préservent le support canonique, la coquille, la clé et le niveau, sans changer les trajectoires.

Qualification bornée O2/SAN : 21 cas, neuf retours directs, six certificats q4 redonnant un support canonique q2, deux rejets des préconditions. Les puissances de canonisation passent de 460 à 123 sur ce corpus ; les 171 puissances du certificat sont comptées dans chaque voie. Le proposeur et la tour ne sont pas chronométrés. Le cas direct ne demande aucune régularité globale du nuage.

Le raccord transactionnel et ses portes permanentes sont **publiés dans 324f6192**. Notre nouvelle contrelecture couvre les lecteurs normal/−O des [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md), du [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et de la [variante HD corrigée](../receipts/gpu_terminal_batch_hd_20260911/README.md). Captures et stabilité des empreintes sont dans l’[entretien](ENTRETIEN.json) ; les lecteurs ne réexécutent ni C++ ni device. La comptabilité cumulative hôte est close ; 65 cas callback signifient 64 rejets et un cas positif.

La [contre-fixture K7](receipts_meb_boundary_20260911/README.md) garde sa valeur historique : le second auditeur a réparé le proposeur depuis. Ses notes et mesures restent sous son autorité. Les demandes de premier essai incrémental, de semis après échange et de porte census→tour K10 sont également closes dans le [dialogue](DIALOGUE_COURANT.md).

Le [graphe filtré réduit aux naissances](receipts_filtered_graph_20260911/README.md) conserve sa preuve et son modèle sur 267 cas ; son raccord C++ reste à qualifier. Le [second auditeur](receipts_cache_commit_20260911/README.md) conserve ses blocs 50k et sa qualification relative ad7ffd28. Archive industrielle, complétude globale et contrats 1 s/100 ms ou massifs restent ouverts, sans transfert des temps historiques vers les nouvelles variantes.

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
