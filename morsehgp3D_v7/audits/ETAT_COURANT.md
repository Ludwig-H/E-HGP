# État courant de l’audit v7

11 septembre 2026, après publication constructeur **069bb6a2**. La priorité
reste la parallélisation de toute la tour et la maîtrise des intermédiaires.

**Nouvelle simplification prouvée : contracter les pivots pendant la
consommation des terminales et appliquer Kruskal directement aux naissances.**
La [preuve et son modèle indépendant](receipts_birth_stream_20260911/README.md)
exploitent l’ordre du flux réel et l’antériorité stricte des terminales.
φ reste une identité native stable ; le DSU travaille sur les seules L
naissances. Le certificat intermédiaire sur A hubs, sa projection et sa
compaction disparaissent. Atlas, φ, fenêtres, marques et sortie restent à payer.

Sur les compteurs publiés n8000, le travail du nouveau réducteur se déduit à
7 342 931 tentatives d’union, contre 48 390 815 dans la pile actuelle et
3 910 849 dans sa compaction native. **Cette déduction n’est pas un nouveau
benchmark.** Le raccord C++, le RSS et le temps de cette spécialisation restent
à qualifier. Les lots parallèles hors ordre gardent leur contrat distinct.

Le [producteur réellement fenêtré](../receipts/streaming_graph_20260911/README.md)
a reçu une contrelecture favorable, lecteurs normal/−O compris. Sa première
qualification bornée est close ; son résultat mono n8000 reste négatif :
188,638 s référence contre 250,408 s flux, index jusqu’à FULL retenu, avec un
RSS légèrement supérieur. Corriger le réducteur avant de prolonger cette
variante à n16k/n32k est la bonne suite. Aucune promotion de performance.

Les acquis précédents restent consultables dans leurs preuves :
[export physique historique](receipts_historical_export_20260911/README.md),
[raccord FULL constructeur](../receipts/atlas_graph_full_20260911/README.md),
[composition MSF](receipts_composable_msf_20260911/README.md),
[décomposition parallèle](receipts_parallel_objects_20260911/README.md) et
[support certifié](receipts_certified_support_20260911/README.md).
Les demandes déjà closes ne sont plus développées ici.

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
