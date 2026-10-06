# Portes des mutants coopératifs — corrections proposées avant G4

Source de base `3b76a3fcf0ca14dd08f005e1e1ae8e3418dd247e`, WIP du développeur capturé par les SHA256 dans `proof.json`. Aucun fichier développeur modifié, aucun build/CTest/native/GCP exécuté. Le correctif `suggested_targets.patch` est une proposition à intégrer et qualifier.

Les six nouveaux mutants donnent tous `porte = mhgp11_catalogue_leaf_coop`. La déclaration actuelle utilise `GROUPS` (`tests/catalogue/tests.cmake:49–50`) : `cmake/gates.cmake:279–289` enregistre quatre noms suffixés et l'inventaire, jamais ce nom de base. Le runner choisit le nom exact (`tests/support/mhgp11_gate.py:152–158`). Deuxième blocage indépendant : le manifeste omet `construction`, donc le runner configure uniquement `MHGP11_MODULES=tower` (`run_mutants.py:126,243`). Les dépendances de la bibliothèque contiennent catalogue, mais `CMakeLists.txt:279–284` inclut uniquement les portes des unités explicitement demandées ; les portes catalogue ne seront donc pas enregistrées, même avec leurs noms corrigés. Ajouter `construction: ["tower", "catalogue"]` au manifeste.

Le témoin non muté doit passer ces portes avant les mutants (`tests/mutants/run_mutants.py:311–324`) ; il refusera donc la campagne avant leur jugement.

| Mutant | Porte proposée |
| --- | --- |
| `coop_paire_sautee` | `mhgp11_catalogue_leaf_coop_sizes` |
| `coop_candidats_restants` | `mhgp11_catalogue_leaf_coop_witness_q3_obtuse_q4` |
| `coop_cache_par_paire` | `mhgp11_catalogue_leaf_coop_cache_hits` |
| `coop_compteurs_fill` | `mhgp11_catalogue_leaf_coop_witness_q3_obtuse_q4` |
| `coop_publication_partielle` | `mhgp11_catalogue_leaf_coop_near_max`, option `-DMHGP11_COORD_BITS=21` |
| `coop_profondeur1_masque` | `mhgp11_catalogue_leaf_coop_sizes`, avec la fixture de boîte étroite proposée |

Deux causes expliquent les derniers choix. `near_max` exige actuellement `partial>0` dans tous profils (`leaf_coop_test.cpp:149–161`), mais ses trois fixtures u18 restent certifiées : puissance native globale `6B+8=116`, largeur q4 <2^20 et orientations q2/q4 admises ; aucun besoin d'orientation q3 sur coquille étendue. Aucune feuille n'est non résolue, donc aucune émission avant refus. Réserver `partial>0` aux profils u21/u24 et vérifier `unresolved==0` en u18. Forcer u21 pour le mutant de publication partielle garantit que sa porte témoin couvre effectivement l'annulation, y compris dans une campagne de base u18. Cette cause a été confirmée indépendamment par les auditeurs mathématique et natif, sans exécuter le produit.

Toutes les fixtures de cette porte hôte contiennent tous leurs sites dans leur boîte (`leaf_coop_test.cpp:110,128,141,157`). Pour deux sites distincts, la différence des distances a des signes opposés aux deux sites ; une dominance universelle stricte sur une telle boîte est impossible. Tous les masques `dom` sont donc nuls. Supprimer `dom[pair.i]` est équivalent sur cette porte : ajouter à `sizes` la fixture causale de cinq sites proposée par l'auditeur mathématique : `(0,3,0),(1,3,0),(3,1,4),(3,0,6),(5,6,6)`, boîte `[2,5)^3`, K3 et cache activé. Elle exige `dom[0]=bit1` et `dom[3]=bit2`, puis compare la voie coopérative à la séquentielle. Le modèle mathématique observe `region_line_tests=3` pour la référence et `4` pour le mutant. Cette preuve de modèle ne constitue pas encore une mise à mort native acquise.

`replay.py` dérive les noms de la déclaration `GROUPS` capturée et applique la sélection exacte du runner : zéro résolution pour les six anciens noms, une pour chaque cible proposée. Il vérifie aussi les unités explicitement construites et l'option u21. Rejeux normal et `-O` identiques. Ce contrôle stdlib ne remplace pas CMake ni le témoin natif. Aucun résultat de qualification CUDA ou performance n'est revendiqué.
