# État courant de l’audit v7

11 septembre 2026. **ce842a3f publie la résolution statique CPU optionnelle**, les comptages R/U et les tours 8k/16k/32k, sur les octets 33e7d05e déjà contre-lus. Sept paquets passent leurs lecteurs normal/-O. Le [complément indépendant](receipts_static_followup_20260911/README.md) ajoute un rejeu SAN réussi de la panne après admission et une piste de réutilisation des semis après échange, prouvée mais non implémentée. Le [second auditeur publie à 223a3897](receipts_cache_commit_20260911/README.md) sa contrelecture ad7ffd28 et le contrôle CPU réussi des quatre blocs nommés 50k. Ses deux lecteurs passent ; la première tentative interrompue reste conservée. Ce contrôle ferme la demande pré-lot, sans qualifier la complétude ni la vitesse.

Le [journal réservé 7608e70e](receipts_journal_guards_20260910/README.md) garde ses dix positifs, neuf rejets, six lectures et neuf mutants indépendants O2/SAN. Les gardes sont reprises dans la qualification du prototype incrémental ; son raccord FULL reste distinct. Le [journal historique](receipts_coverage_cpp_20260910/README.md) conserve ses 184 cas et 2 976 coupes ; CSR et multifusion carrée K2 sont permanents. Aucun défaut nominal du journal n’est ouvert, son autorité reste structurelle.

La [gate incrémentale indépendante](receipts_incremental_prefix_20260911/README.md) qualifie désormais les anciennes coupes du prototype privé b526b895/76885ecd : 20 préfixes O2/SAN, 5 616 requêtes racine/couverture, 16 suffixes refusés et trois mutants causaux. Elle réalise la régression du [lemme publié](receipts_incremental_review_20260911/README.md), dont le calcul net de résidence reste séparé. Aucun raccord FULL ni gain RSS n’en découle.

La [factorisation statique](receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md) possède maintenant son [raccord CPU optionnel](../docs/RESOLUTION_STATIQUE_CPU_20260911.md), sous census exact complet. Les choix d’intrus préservent les composantes, pas nécessairement les terminaux ni le travail. Le journal incrémental reste un prototype séparé, publié avec 104 cas appariés constructeur et les mêmes headers que notre gate indépendante.

Les [mesures publiées de tour retenue](../docs/RESULTATS_TOUR_CACHE_G4_20260910.md) terminent à 8k/16k/32k et à 50k K1..10/K1..5, avec verticales et paires CPU/hybride identiques. Ce sont des diagnostics du constructeur, contre-vérifiés par lecteurs de captures, sans nouvelle exécution par nous. Archive industrielle, complétude globale et contrats 1 s/100 ms ou massifs restent ouverts.

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
| P, raccord par ancres `d188e3de` (second auditeur) | [Contrelecture historique](receipts_raccord_ancres_20260910/README.md), prolongée par le [rejeu ad7ffd28](receipts_cache_commit_20260911/README.md) ; lots groupés et contrôle nommé 50k clos, sans nouvelle variante globale |
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

Le [manifeste](validation_current.json) conserve les variantes D–P sans réattribuer leurs résultats à ad7ffd28. La liaison du journal publié avec son reçu borné est acquise ; cette passe ne crée pas une nouvelle qualification globale de la tour. Les écarts de source restent visibles dans le contrôle de fraîcheur.

Le [dialogue](DIALOGUE_COURANT.md) ne répète plus les demandes satisfaites ; les [questions secondaires](QUESTIONS_SECONDAIRES.md) restent regroupées. Les preuves et échecs scientifiques sont inchangés ; l’[entretien](ENTRETIEN.json) garde la provenance des condensations. GCP non utilisé par cet audit.
