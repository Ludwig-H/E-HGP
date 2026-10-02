# Reçus indépendants v11 — 2 octobre 2026

Ouverture `52687f8e5`, dernière qualification centres/domaine `7f1922c77`, MEB `25792084e`, index `e8520481d` ; parallèle/cellules9c non qualifiés,
fondations exécutées sur G4 à `a97180667`, copies figées avant lecture. Aucun nouveau GCP par cet audit ;
la recoupe G4 qualifie les fondations CPU, pas FULL/GPU/performance. Les anciennes captures restent
inchangées ; les corrections ont leurs reçus distincts. Deux notes actives :
[fondations](../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md) et
[verrous du moteur](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

| Reçu | Portée |
| --- | --- |
| [F3 initial](floating_bounds/PUBLICATION.md) | Contre-exemple de la doctrine initiale et correction constructive, 62 gardes. Réserve désormais résolue. |
| [Architecture initiale](architecture_review/README.md) | Quatre documents figés, contrôle documentaire ; décisions désormais fixées au §7. |
| [Provenance R2](provenance_review/README.md) | MR1 ancien fermé, source/series/gates distinctes, pas de transfert de qualification. |
| [Core intégré initial](integration_review/README.md) | GCC Release B18 : 70 portes passent, sentinelle LiDAR sautée, mutants longs exclus. Source a ensuite évolué. |
| [Core initial](core_review/README.md) | Défaut StageTimer exécuté, budget/Buffer contrôlés ; 77 contrôles du reçu. |
| [Stopwatch corrigé](core_stopwatch_followup/README.md) | Destruction sans allocation ; publication explicite, refus et reprise. 38 contrôles. |
| [Oracle initial](reference_review/README.md) | 1 732 gardes, coupes/verticales/triangles ; premières copies seulement. |
| [Portes initiales](gates_review/README.md) | Programme absent compté tué dans ancienne copie ; filtre G4 corrigé. |
| [Portes corrigées historiques](gates_followup/README.md) | Programme absent refusé ; défaut interpréteur absent du snapshot désormais fermé par le wrapper publié. |
| [Numérique corrigé](numerical_followup/README.md) | F3/F4 justifiés sous domaine positif ; ce domaine a ensuite été écrit. |
| [Verrous exacts](math_locks_review/README.md) | 159 contrôles : choix de descente, local/global, F2, compensation Euler et symétrie cover. |
| [Session et lanceur publiés](runner_session_review_2/README.md) | Erreur de lancement fermée ; interruption globale désormais corrigée ; isolation de descendance encore ouverte. Wrappers/fixtures seuls. |
| [Oracle séparé](reference_separation_review_2/README.md) | Réserve structurelle levée ; troisième juge collinéaire, catalogue distinct du seul FULL. Lecture et AST/Fraction autonomes. |
| [Cloud et capacité historique](cloud_contract_review_2/README.md) | Copie privée précédente ; comptage et formules exactes avec entrées vivantes, pas benchmark massif. |
| [IO privé non livré](io_contract_review_2/README.md) | Morceau vide SHA-256 à protéger dans cet ancien WIP ; aucune attribution au produit qualifié actuel. |
| [Robustesse frontière](boundary_stability_review_2/README.md) | Preuve de stabilité en rayon du continu FULL ; premier cover/LCA discontinu, trois couples u18 proposés. |
| [G4 recoupé](g4_qualification_review_3/README.md) | Trois paquets/archives Git et 91 entrées manifestes vérifiés ; deux échecs conservés, troisième campagne conforme. Isolation encore déclarée ouverte. |
| [Cloud immuable](cloud_immutable_review_3/README.md) | Sources identiques à G4, vingt portes dans six configurations ; anciennes réserves fermées, durée de vie du futur index et pic partagé à tester. |
| [Géométrie exacte](numeric_geometry_review_3/README.md) | Bornes B18/21/24 relues ; 196 contrôles autonomes normal/−O, seize requêtes G4 préparées non exécutées. |
| [Hiérarchie commune aux K](cross_order_contract_review_3/README.md) | Témoin statique sept points core K1/K2 incompatibles, modèle Gamma indépendant ; entiers/Level relus favorablement. |
| [Catalogue : géométrie](catalogue_geometry_review_4/README.md) | G1–G4 et coquilles relus ; contrôles rationnels autonomes, aucun nouveau défaut établi. |
| [Catalogue : capacité](catalogue_capacity_review_4/README.md) | Deux passes, rangs et refus cohérents ; pic des DFS/émissions puis des deux populations explicité, identité Cloud à lier au futur raccord. |
| [Catalogue : preuves G4](catalogue_evidence_review_4/README.md) | Première campagne fermée localement/non versionnée, 947/948 ; mutants catalogue non jugés et banc 0/36. Correction clone f391, reprise distincte attendue ; hashes binaires désormais capturés. |
| [Catalogue : coût frontière](catalogue_boundary_work_review_4/README.md) | Coquilles u18 atteignables 30/150/270 sites ; présentations centrales répétées à K5/K10, refus max_leaf honnête et lemme de rejet anticipé non canonique. |
| [Catalogue : reprise et temps](catalogue_second_capture_review_4/README.md) | Reprise f391 qualifiée ; une tentative 8k/K5 terminée, six délais et 29 omissions conservés ; aucun temps FULL/GPU/LiDAR complet. |
| [Catalogue : ablation leaf16](catalogue_ablation_review_5/README.md) | e6 qualifié, 960 portes ; sept succès, six délais, 23 omissions. Trois catalogues LiDAR sans sol K5 en 20–26 s ; sortie 8k déclarée identique, comparaison ×1,879 limitée. |
| [Catalogue : census natif](catalogue_native_side_review_5/README.md) | Preuve de tous les intermédiaires i128 u18, témoin aigu d'overflow u21 ; Level q4 tardif avec type interne, aucun code ou gain natif acquis. |
| [Catalogue : familles](catalogue_family_contract_review_5/README.md) | Famille cosphérique, coquille complète sous certification, ancre qmin4 et préfixes/puissance ; 110 gardes rationnelles normal/−O, pas de port natif. |
| [Catalogue : contrat parallèle](catalogue_parallel_contract_review_5/README.md) | Listes possédées recouvrantes, capacités coexistantes, count/fill et offsets globaux ; 24 ordres de complétion, 222 contrôles normal/−O après fermeture. |
| [Puissance : bornes par arité](native_arity_bounds_review_6/README.md) | q4 B24 <72M⁵ par ancrage commun, hors hull inclus ; tag de présentation et q3 qmin2 ; 499 gardes exactes normal/−O, pas de qualification native. |
| [Puissance : contrat du port](native_power_contract_review_6/README.md) | Relecture source 9d, factories/tag, refus et portes ; Wide du harnais distingué du juge Fraction. Qualification propre dans la tranche 7 ci-dessous. |
| [Banc multi profils](profile_benchmark_review_6/README.md) | Mêmes XYZ/IDs, binaire/cache qualifié vérifié, digest complet et échecs persistés ; contrôles jouets normal/−O avant la campagne recoupée dans la tranche 7. |
| [Précision physique](precision_mapping_review_6/README.md) | B versus h, niveau physique h²β, faux affinement par mise à l'échelle, collisions/IDs ; 83 gardes rationnelles et métadonnées closes, pas de LiDAR plus fin exécuté. |
| [Profils : qualification et temps](profiles_capture_review_7/README.md) | Source 9df, paquet Git et archive recoupés ; matrice 1 002/1 002, 116 verdicts mutants ; quinze succès K5, dix-huit délais et trois omissions. Mêmes sorties/travail déclarés, aucun FULL/GPU acquis. |
| [Q4 : candidat privé et admission](q4_candidate_contract_review_7/README.md) | Contrat de Level tardif sans Sphere invalide ; strict_inside avant owner/census/judged, contre-cas causal et deux passes. Proposition, pas de port/gain. |
| [Q4 : rejet commun au triplet](q4_level_math_review_7/README.md) | Témoins coplanaires stricts intérieurs à toutes les extensions q4 ; seuil distinct de q3, preuve et fixtures rationnelles. Présentations, sans supprimer branches q≤3 ni crédit final. |
| [Mémoire : phase et profil](profile_memory_phase_review_7/README.md) | 98 contrôles exacts normal/−O : quinze pics et réservations après appel égaux aux formules Cloud/E/F ; largeur et sérialisation distinguées. Aucune allocation massive ni RSS. |
| [Num : complément ASan B18](profiles_u18_sanitize_review_7b/README.md) | Source d77, 13/13 portes num et style ; q3 natif power/side réellement exercé, 207 contrôles et deux oracles Fraction ; arrêt ciblé distinct, aucun nouveau temps de catalogue. |
| [Q4 : API portée](q4_candidate_port_review_8/README.md) | Source ffc, candidat fermé possédé, prédicats partagés, factories/erreurs et fraction non réduite inchangés ; 17 103 contrôles rationnels/statiques, natif encore distinct. |
| [Q4 : raccord catalogue porté](q4_catalogue_port_review_8/README.md) | Inside→owner→I/U→S*→admission→Level→Collector ; q3 obtus et ledger des deux passes conservés, aucun filtre de famille porté. |
| [Q4 : protocole de campagne](q4_campaign_contract_review_8/README.md) | Plan actif à trois commandes et supplément raccordés ; compteur qmin4 séparé du digest, replays Python conformes. Aucun reçu q4 clos à la capture. |
| [Familles : ligne propriétaire](q4_owner_line_witness_review_8/README.md) | Extension aux témoins hors du plan par clipping exact de la ligne des centres ; 5 807 gardes Fraction normal/−O, signes de degré huit, aucune voie native ou gain acquis. |
| [FULL : identité géométrique](ball_identity_contract_review_8/README.md) | Tuple primitif signé sans ID/arité/qmin/Level, contexte de repère fixé ; mêmes boules multi-présentations et MEB affines/obtuses contrôlées. PGCD et format natif à qualifier. |
| [Q4 : qualification recoupée](q4_qualification_review_9/README.md) | Source ffc, paquet Git et archive exacts ; 1 014 sélections conformes, complément ASan B18 14/14 distinct, 119 verdicts mutants. Quinze succès K5, dix-huit délais et trois omissions ; fermeture ciblée, banc en échec conservé. |
| [Q4 : travail, temps et mémoire](q4_cost_attribution_review_9/README.md) | 259 contrôles normal/−O : environ 99,7 % de niveaux candidats LiDAR évités, quinze pics inchangés Cloud+E+F ; comparaison temporelle non appariée, aucune part du CPU déduite des seuls compteurs. |
| [Index global : contrat et capacité](global_index_contract_review_9/README.md) | Lignée/lifetime, saturation stricte ou census complet, régime unitaire, admission commune et sorties retenues ; modèle scalaire 123 contrôles normal/−O, aucun index/massif qualifié. |
| [Census global : bornes exactes](global_census_bounds_review_9/README.md) | LB/UB sur boîte fermée, contacts conservés, faux minimum des coins et budgets par arité ; 1 512 boîtes et 7 000 points entiers. WIP numérique séparé relu favorablement, sans qualification native. |
| [Index : qualification recoupée](index_campaign_review_10/README.md) | Source e852, publication356 ; 1 149 sélections conformes, complément36/36 distinct, 18 essais/1 152 requêtes choisies. Aucun chrono de descente MEB/FULL. |
| [Index : capacité et propriété](index_capacity_port_review_10/README.md) | Sources et lecteurs clos, 165 469 contrôles scalaires ; nœuds/profondeur, refus et sorties retenues. Dimensions 30/50 M analytiques, admission d'un lot à définir au raccord. |
| [FULL : raccord et fixtures](full_index_descent_contract_review_10/README.md) | Trois portes Gram/Fraction : MEB affine, supports local/global, semis relevés, multifusion à quatre parents et verticales fermées. Aucun port natif qualifié. |
| [Index : bornes sur sites entiers](index_lattice_bounds_review_10/README.md) | Minimum lattice distinct du contrat continu, maximum par coin ; 1 656 boîtes/7 000 points par profil. Option sans port ou gain natif. |
| [MEB≤12 : contrat exact](meb_bounded_contract_review_11/README.md) | Plan356 figé, sept ensembles/49 ordres, 3 920 présentations Fraction normal/−O ; unicité au minimum, présentation versus support positif local, qmin global et cofaces13 distincts. Aucun natif exécuté. |
| [MEB : port et propriété](meb_port_contract_review_12/README.md) | 49 sources24e4/ab04 identiques ; copies fixes, domaine local12, census possédé et résultats retenus20+4. Portes natives relues, aucune exécution. |
| [MEB : mathématique du port](meb_math_port_review_12/README.md) | Neuf géométries/27 ordres Fraction normal/−O ; préfixe obtus/q4 strict, arrêt anticipé démontré et census complet hors Cat3. Proposition de Level q4 tardif, aucun gain acquis. |
| [MEB : protocole et correctif du banc](meb_campaign_contract_review_12/README.md) | Sourcesab04 et lecteur WIP séparés, sérialiseur temporaire corrigé257 ; deux MEB/census payés et populations coexistantes, replays purs. Aucune campagne qualifiée ici. |
| [MEB1 : campagne échouée recoupée](meb_qualification_review_13/README.md) | Paquet/archives Git exacts, main1 254/1 266 et ASan18 53/55 : seuls IO échouent. Fermeture G4, aucun banc lancé,18 non joués ; correctif257 distinct et non joué. |
| [CenterRegion : mathématique et raccord WIP](center_region_contract_review_14/README.md) | 864 droites/432 paires Fraction normal/−O ; SAT de fermeture, contacts hi=M, budgets cubiques et DFS q3/q4 relus, sans qualification native. |
| [FullDomain : propriété et capacité WIP](full_domain_contract_review_14/README.md) | Même lignée Cloud, support global exact et transferts transactionnels ; pic propre Fcat sous pilote unique, table persistante4C. Jobs/moves et injection finale explicités ; tests apparus ensuite non relus. |
| [MEB3 : requalification recoupée](meb_requalification_review_15/README.md) | Source257/publication9a, paquet Git/archives exacts ; 1 266/1 266 +55/55,141 verdicts mutants,18/18 essais. MEB1 et stockout MEB2 conservés distincts ; requêtes choisies, aucun FULL. |
| [CenterRegion publié : deltas](center_region_published_review_15/README.md) | Code7f identique14, nouvelles portes/mutants relus ; témoin k=2 correct, garde explicite hors-i64 B21 utile. Contrôles autonomes, natif à venir. |
| [FullDomain publié : nouvelles portes](full_domain_published_review_15/README.md) | Injection finale de table, collisions/padding, moves, résultats coexistants et lectures concurrentes effectivement visés ; aucune nouvelle exécution native. |
| [Cellules/localisation WIP : contrats](full_locator_contract_review_16/README.md) | Hit complet avec p>=k, absence Cat3 légitime ; octaèdre t2 douze traces/un morceau. Traces distinctes des racines à fusionner, copies et calculs autonomes, aucun FULL acquis. |
| [Centres/domaine : qualification et interruption](center_domain_qualification_review_17/README.md) | Source7f :1 398/1 398 +73/73,154 mutations recoupées ; banc14 succès/15 délais/2 omissions/5 sans résultat persistant, code124 conservé. Checkpoint encore après spawn9c ; aucune mesure FULL. |
| [Catalogue parallèle : contrats et critique R2](catalogue_parallel_contract_review_17/README.md) | Admission1064n mêmeW1, adaptation sûre de frontière, sites dans Qbar à conserver sans domination ; partition/offsets/rebasing/coexistences relus. Pool racine séparé, aucun natif. |
| [Cellules/localisation : garde locale4→globale3](cells_locate_contract_review_17/README.md) | S* global q3 sans minU, cinq sites/120 permutations exactes ; garde hit p>=k adoptée9c. Oracle de séparation indépendant, mutants préparés non joués. |

Les quatre inventaires de la sauvegarde a7a38137c sont rejugés à la reprise :
exhaustifs et inchangés ; conclusions intégrées aux notes actives v10/v11.
Précision de lecture pour la revue capacité10 : sa forme logarithmique de
profondeur s'applique à n>leaf. La forme universelle, donnée aussi dans la
capsule et réellement vérifiée, est `bit_length((n−1)//leaf)+1`, y compris
n≤leaf (profondeur1). Les pièces closes restent inchangées.

Chaque fermeture est vérifiée sans changer les pièces initiales. Les pages
courantes ne remplacent pas leurs hashes. Le README figé F3 conserve une référence
contextuelle alors locale, expliquée dans PUBLICATION ; ce n'est pas une dépendance
de la preuve. Les espaces des extraits/diffs/logs bruts sont conservés pour leurs
empreintes ; seuls les fichiers explicitement identifiés sont exclus du contrôle
d'espacement Git, jamais les notes rédigées.

Fermeture de la tranche 9 : [enveloppe complète](ENVELOPPE_PUBLICATION_9.json)
avec chemin/taille/SHA de chaque fichier des quatre capsules, inventaires
racines compris. Le contrôle final a trouvé une omission d'inventaire dans
q4_qualification_review_9 : `raw/package/data/SHA256SUMS` (1 223 octets,
SHA `2a17ad24674bd5e1c693ca529391e3647f63bc2f0f7e773acd452d29ecae615e`).
Le filtre initial excluait ce basename à toute profondeur. Cette pièce
était déjà hachée dans sources_before/sources_after et comparée au manifeste
du reçu par review.py ; LEDGER/SHA256SUMS clos restent inchangés. L'enveloppe
distincte ferme l'inventaire complet, sans réécrire cette première fermeture.

Exceptions exactes au contrôle d’espacement, sorties brutes closes :

- `core_review/compiler_version.txt` ;
- `core_stopwatch_followup/compiler_version.stdout.txt` ;
- `integration_review/LastTest.log` ;
- `integration_review/gates.json` ;
- `integration_review/run_00.stdout` ;
- `integration_review/run_02.stdout`.

Exceptions supplémentaires de la deuxième tranche, copies closes inchangées :

- `cloud_contract_review_2/sources/morsehgp3D_v11/docs/PROVENANCE.md` ;
- `runner_session_review_2/source/morsehgp3D_v11/docs/PROVENANCE.md`.

Exceptions de la troisième tranche, pièces closes conservées octet pour octet
(journaux bruts et programme du modèle figé, ligne vide finale comprise) :

- `cloud_immutable_review_3/g4_logs/bits21/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/bits24/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/gcc_asan_ubsan/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/gcc_release/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/gcc_tsan/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/poison/junit.xml` ;
- `cross_order_contract_review_3/model.py` ;
- `g4_qualification_review_3/excerpts/reprise1/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise1/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise2/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise2/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise3/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise3/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;

Exceptions de la quatrième tranche, journaux bruts clos inchangés :

- `catalogue_evidence_review_4/excerpts/catalogue1/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `catalogue_evidence_review_4/excerpts/catalogue1/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;

Exception de la reprise catalogue2, journal brut clos inchangé :

- `catalogue_second_capture_review_4/excerpts/results/cmd/000_matrice/files/matrix/mutants/LastTest.log`.

Exceptions de la cinquième tranche, journaux et diff bruts clos inchangés :

- `catalogue_ablation_review_5/excerpts/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `catalogue_ablation_review_5/excerpts/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;
- `catalogue_family_contract_review_5/docs_delta_from_f391.diff` ;

Exceptions de la septième tranche, journal et diff bruts clos inchangés :

- `profiles_capture_review_7/excerpts/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;
- `q4_level_math_review_7/wip_delta.diff`.

Exceptions de la huitième tranche, programme et diff figés conservés
octet pour octet après fermeture :

- `q4_candidate_port_review_8/derive.py` ;
- `q4_catalogue_port_review_8/HEAD_delta_from_d405.diff`.

Exceptions de la neuvième tranche, diff et journal bruts clos inchangés :

- `global_census_bounds_review_9/WIP_vs_d0dc.diff` ;
- `q4_qualification_review_9/mutants_LastTest_review.log`.

Exceptions des tranches MEB 12–13, diffs et journaux bruts clos conservés
octet pour octet :

- `meb_campaign_contract_review_12/logs/delta_check_reader.diff` ;
- `meb_campaign_contract_review_12/logs/delta_meb_collector_test.diff` ;
- `meb_qualification_review_13/contexte_math_api/logs/000_matrice/gcc_release/LastTest.log` ;
- `meb_qualification_review_13/contexte_math_api/logs/001_asan18/gcc_asan_ubsan18/LastTest.log` ;
- `meb_qualification_review_13/extraits/results/env/gxx.txt`.

Exception de la reprise MEB3, journal brut clos conservé octet pour octet :

- `meb_requalification_review_15/extraits/results/cmd/000_matrice/files/matrix/mutants/LastTest.log`.

Exception de la tranche 17, journal brut clos conservé octet pour octet :

- `center_domain_qualification_review_17/extraits/results/cmd/000_matrice/files/matrix/mutants/LastTest.log`.
