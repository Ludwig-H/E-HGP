# État courant de l’audit v7

Le [complément des gardes du journal](receipts_journal_guards_20260910/README.md) transforme les trous de tests du second auditeur en fixtures causales : dix cas valides, neuf rejets, six lectures et neuf mutants détectés. Le WIP figé 7608e70e passe O2/SAN et toutes ses cinq pannes d’allocation sur la fixture mixte, contre 14 allocations pour la source publiée. Aucun défaut nominal démontré ; intégration des nouvelles fixtures dans la gate constructeur encore à faire. Les sources et l’échec initial SAN sont conservés.

La nouvelle [tour retenue](../docs/TOUR_FULL_PAR_BOULES.md), publiée par **d188e3de**, conserve K1..10 et ses cartes verticales. Le second auditeur a publié sa [suite cache/front](receipts_raccord_ancres_20260910/suite_cache_20260910/README.md), dont un défaut d’injecteur ASan corrigé dans sa copie. La correction de capacité du cache donne 12 Gio à dix millions de points, sans allocation exécutée. Notre [complément de résidence](receipts_tower_cost_review_20260910/README.md) établit à 32k : 642 249 936 octets de tableaux libérables avant encodage, au moins 2 193 190 400 octets d’en-têtes de brouillons, et une réduction de représentation future pour les naissances. Ce sont des déductions statiques sur captures et ABI locale, sans gain RSS ou temps mesuré.

Le [raccord FULL par ancres de boule](receipts_raccord_ancres_20260910/README.md), publié par `d188e3de` avec les octets déjà lus comme WIP, est contre-lu par le second auditeur : lecture statique conforme aux autorités de plateau, portes rejouées O2 et ASan/UBSan, K1 égal au single-linkage exact jusqu'à n=4 000, 2 507 nuages entiers aléatoires sans divergence contre un juge Gamma rationnel, normaliseur temporel identique au lecteur immuable sur 11,9 millions de requêtes. Il n'est **pas reçu** : deux mutants causaux du reçu constructeur, réappliqués au chemin des lots groupés du header publié, survivent à la porte (vert par vacuité, fixture correctrice fournie), et aucun contrôle nommé ne confronte le raccord aux parents réels 1/2/2 des blocs 50k. Aucun temps, 50k, GPU ou statut public n'en découle.

Le [journal v2](receipts_coverage_cpp_20260910/README.md) garde sa qualification indépendante O2/SAN : 184 cas et 2 976 coupes. La lacune de contrôle du tableau des parents est **corrigée** par la [gate constructeur](../receipts/coverage_parent_array_20260910/README.md), 837 contrôles et mutant réfuté. L’échec initial LeakSanitizer de notre campagne reste conservé séparément de sa reprise réussie. Le journal structurel ne certifie pas la complétude du producteur.

Les [parents réels 50k](receipts_plateaux_full_20260906/GLOBAL_PARENTS.md), repris par le constructeur, restent 1/2/2 pour les trois blocs concernés. La [vraie tour K-NN](receipts_gabriel_vertices_20260906/README.md) se reconstruit sur un quotient des minima avec les bonnes connexions ; la restriction aux seules adjacences Gabriel est réfutée. Les [ancres partagées](receipts_shared_anchors_20260906/README.md) préservent les identités et coupes des ordres distincts.

Les qualifications historiques du raccord MEB filtré et du moteur réduit gardent leurs autorités séparées ci-dessous. Les [mesures courantes](MONO_FULL_COURANT.md) distinguent désormais la tour retenue des sondes horizontales anciennes. L’export industriel, la complétude à grande échelle et les contrats 50k/1s restent ouverts.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La lecture intégrale des parties I et II du manuscrit, PDF 35–134, reste acquise. La [décision FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md) distingue minima, multifusions et rattachements silencieux, K=n, régularité, verticale et poids.

| Autorité | Résultat conservé |
| --- | --- |
| Raccord par ancres `d188e3de` (second auditeur) | [Contre-lecture, corpus aléatoire, normaliseur temporel ; deux verrous P1, non reçu](receipts_raccord_ancres_20260910/README.md) |
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

Le [manifeste](validation_current.json) conserve O sur le publié `20b28b1d`, ainsi que D–N ; P épingle la contrelecture du second auditeur sur d188e3de, sans réception du raccord. Les écarts ultérieurs des documents, tests et du générateur sont affichés par le contrôle de fraîcheur, sans requalification implicite. Les nouvelles modifications WIP du journal et de la tour restent distinctes de ces sources publiées et des snapshots qualifiés. La réutilisation terminale q2 possède ses captures constructeur, contre-lues sans nouvelle exécution ; elle reste hors de O. Les sondes sans quotas et multi-CPU ne sont pas qualifiées par O. Le juge se rejoue sur les captures sans moteur.

Le [dialogue actif](DIALOGUE_COURANT.md) porte les fixtures de garde à intégrer ; les demandes déjà reprises ont été raccourcies. Les autres questions sans incidence immédiate restent [regroupées](QUESTIONS_SECONDAIRES.md).

Les reçus bruts et échecs restent conservés ; les anciennes synthèses sont accessibles par le [registre d’entretien](ENTRETIEN.json). GCP non utilisé.
