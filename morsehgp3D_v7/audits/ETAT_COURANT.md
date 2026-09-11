# État courant de l’audit v7

11 septembre 2026, lecture de `34ad933d`, puis publications constructeur
`0db1e775` et second auditeur `b5271aad`. **La priorité est désormais la
parallélisation de toute la tour par changement de structures.** La
[proposition courante](receipts_parallel_objects_20260911/README.md) articule
boules partagées, rôles clairsemés, graphes sur naissances, arbres de multifusions
et requêtes historiques sur marques datées.

Deux dépendances sont levées mathématiquement : les horizontales des K peuvent
se construire indépendamment ; leurs verticales peuvent ensuite se calculer
toutes ensemble, sans utiliser celles de K−1. Les chaînes lourdes donnent
O(log N) par requête avec stockage O(N). Des algorithmes publiés construisent
le dendrogramme en parallèle ; les égalités doivent redevenir des multifusions
FULL atomiques. La génération et les descentes adaptatives conservent leurs
propres dépendances de découverte.

Le modèle Python normal/−O passe 11 cas, 234 coupes, 5 178 requêtes comparées
au BFS, 12 nœuds verticaux et cinq mutants. Le constructeur d’arbre est un
juge séquentiel borné ; aucun parallélisme C++/GPU ni temps de tour nouveau
n’est qualifié. Le dossier donne aussi la formule de deux scans exacts pour
le sweep q4 et les transformations des phases de préparation et d’export.

Le [graphe filtré précédent](receipts_filtered_graph_20260911/README.md) et la
[canonisation par support certifié](receipts_certified_support_20260911/README.md)
conservent leurs preuves et tests. Le second auditeur a publié un premier
histogramme de coquilles sur flux réel ; cette demande est close dans son
domaine, sans gain transféré à un raccord encore absent.

Le raccord transactionnel et ses 40 portes ciblées ont été publiés dans
324f6192 ; notre lecture des paquets actifs, T2 et HD reste dans
l’[entretien](ENTRETIEN.json). Les nouvelles tentatives G4 documentées par le
constructeur gardent son autorité ; cet audit n’utilise pas GCP. Les demandes
anciennes closes sont condensées dans le [dialogue](DIALOGUE_COURANT.md).
Archive industrielle, complétude globale et contrats de temps restent ouverts.

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
