# Audit indépendant v7 : résidence et parallélisme

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le tri par permutation satisfait la porte locale de résidence et conserve exactement la sortie stable de référence. Le census utilise une destination privée unique, publiée après validation complète. Aucun défaut de résidence n'est ouvert sur ces deux mécanismes dans le périmètre vérifié. La qualification de mémoire bout en bout et du passage à grande échelle reste à établir.

Le [retour mémoire courant](RETOUR_MEMOIRE_COURANT.md) ajoute deux refus conservatifs évitables et une mesure de coexistence A2/B1. La correction proposée borne la concurrence budgétée par le nombre total d'ordres possibles, conserve les marges et sépare le proxy déclaré des capacités allouées.

Le [mode mono direct](MONO_COURANT.md) est qualifié séparément : `threads=1` et `fold_join_before_next_k=true` exécutent B et ses callbacks sur le thread appelant, sans création de thread dans les fixtures. Les quatre portes dédiées passent. La coexistence A2/B1 concerne la route avec recouvrement ; elle est absente de ce mode direct. Les compteurs d'activité B ne sont pas une mesure des threads natifs créés.

## Source vérifiée et objet mathématique

Les parties I, pages PDF 35–76, puis II, pages PDF 77–134 du manuscrit ont été lues intégralement, ainsi que `README.md`, `audits/ETAT_COURANT.md` et le contrat du census de la v7. Le manuscrit fixe les composantes filtrées du graphe de facettes, puis leur projection vers les points. La proposition 5 permet de se limiter aux cofaces élémentaires pour calculer ces composantes ; elle ne permet pas de remplacer arbitrairement leurs adjacences. Aucune optimisation proposée ici ne construit Gamma exhaustif ni la mosaïque de Delaunay d'ordre supérieur.

Les sources de la compilation sont copiées sous `audits/.work_residence/current/`. Le [reçu courant](receipts_20260904/residence_current.json) contient leurs hashes avant copie, après copie et après vérification, les commandes et sorties brutes. Les octets doivent correspondre à ces hashes pour que les conclusions s'appliquent.

| Source de la campagne historique dans `morsehgp3D_v7/` | SHA-256 vérifié |
| --- | --- |
| `src/parallel/sort.hpp` | `ceb89f8ba42fbc3f44351f4dcbf5e347e7d66a4c85bce08f346e66a9b11f20bd` |
| `src/parallel/pool.hpp` | `5c20aabbe673e2baa1018bea893185592bc3b394d025eadd9be07542453befc6` |
| `src/pipeline/expand.hpp` | `7cafb0341344fbc7d1584001e4685e2e5bf0122fe3b7e37277f5468d5c5e1cf0` |
| `src/pipeline/census.hpp` | `3583680d947a5b28f8a38028bc55f37f12040143fea88d4aecc77a7baf653690` |

## Tri par permutation : objet et allocations

`IndexLess` départage deux enregistrements équivalents par leur indice original. Son ordre sur les indices est donc strict et total. Les tris locaux de la seule route indices utilisent `std::sort`; la route générique conserve `std::stable_sort`. Chaque tranche d'indices a ainsi une suite triée unique, identique à celle d'un tri stable. Les fusions et le ramassage par cycles conservent alors la sortie stable complète, charges utiles comprises.

La [sonde indépendante](sort_residence_probe.cpp) intercepte les allocations C++ ordinaires pendant le tri. Elle compare des clés égales accompagnées de charges utiles distinctes à une référence `std::stable_sort`, puis vérifie un budget de heap déclaré pour cette fixture : `8n + 4096` octets, dont la marge fixe couvre les métadonnées de deux ouvriers. Elle exige aussi l'absence des temporaires locaux de 32 768 octets identifiés dans cette configuration de libstdc++. Elle n'impose aucun rendez-vous d'allocation ni aucune présence de tampon.

Résultat GCC 13.3.0 / libstdc++, `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`, 32 768 enregistrements de 72 octets, deux ouvriers :

```text
n=32768 record_bytes=72 workers=2 stable=1 slice_buffers=0 simultaneous_buffers=0 additional_peak_bytes=262736 heap_budget_bytes=266240 live_after=0 bounded=1
```

Les deux tableaux d'indices coûtent `8n = 262144` octets ; le pic supplémentaire total suivi est de 262 736 octets. La sortie correspond élément par élément à la référence, les temporaires locaux recherchés sont absents et aucune allocation suivie ne subsiste après le tri. Cette mesure porte sur le heap C++ demandé, pas sur le RSS, les piles natives ni les en-têtes de malloc. Les allocations d'un `std::sort` ne sont pas déduites de son nom : elles sont vérifiées pour cette bibliothèque, ce compilateur et cette fixture.

Commandes de reproduction depuis la racine :

```bash
mkdir -p morsehgp3D_v7/audits/.work_residence/tmp
TMPDIR=/workspaces/E-HGP/morsehgp3D_v7/audits/.work_residence/tmp c++ -std=c++20 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -pthread -I morsehgp3D_v7/audits/.work_residence/current morsehgp3D_v7/audits/sort_residence_probe.cpp -o morsehgp3D_v7/audits/.work_residence/sort_residence_current
morsehgp3D_v7/audits/.work_residence/sort_residence_current
```

Compilation et sonde : codes 0. Le code 3 signale un défaut d'objet, un nombre d'ouvriers inattendu ou des allocations résiduelles ; le code 4 signale le dépassement du budget local ou la présence de temporaires locaux recherchés. Pour rejouer sans copie temporaire, employer `-I morsehgp3D_v7` après contrôle des hashes et dépendances. La porte de permutation et ses mutants ciblés sont également exécutés contre la copie courante ; leurs sorties exactes figurent dans le reçu.

La porte `perm_sort_gate` rend 0 sur 241 000 éléments synthétiques, 210 882 paires de clés égales et 30 037 classes multiples ; elle compare les routes à 1, 2, 4 et 8 fils. Elle vérifie aussi 172 199 candidats réels et 168 712 boules de census. Les mutants `perm-apply-scatter`, `perm-apply-partial`, `perm-tie-desc` et `parallel-sort-unstable` rendent chacun 4 sur leur divergence ciblée. Pour le dernier, la route directe diverge six fois et la route permutation zéro fois, conformément au départage total des indices. Les 47 fichiers copiés restent identiques à la source après ces exécutions.

## Acquis de census et de parallélisme

**Census direct.** `census_balls` vide la sortie fournie, possède une destination privée de taille connue, attribue une case par indice de survivante et ne publie qu'après jonction et lecture ordonnée de tous les statuts. Les writes portent sur des cases disjointes. Sur refus explicite ou exception, la destination publique reste vide. Le compteur de capacités observe la destination et tout tableau supplémentaire injecté pendant leur coexistence. Il décrit les capacités `BallData` possédées par le census, pas le RSS du processus.

**Admission des fils.** `run_threads` attend que tous les fils existent avant admission. Son garde RAII joint les fils créés même si le lancement échoue. Le tri ne peut donc pas commencer une phase exigeant des participants dont la création a échoué. Les résultats de la suite globale sur ces contrats sont à lire dans son propre reçu ; l'inspection de ce mécanisme ne constitue pas à elle seule une mesure d'exécution.

**Expansion régulière.** La coquille est triée dans un tableau local borné et convertie en span. Les listes de coquille et d'intérieur sont bornées par le profil. Les plateaux conservent leur expansion complète et leurs refus explicites.

## Qualifications restant à établir

1. **Admission mémoire du pipeline.** Priorité au plafond constant `min(fold_inflight, kmax_eff)` et au précontrôle silencieux proposés dans le retour courant. La garde census compte encore deux `BallData` par candidat ; sa requalification vient séparément. Une formule sur la taille post-RLE ne borne pas la capacité conservée par le tableau de candidats. Compter les capacités encore vivantes, les temporaires et les autres objets annoncés avant de qualifier une admission fondée sur la mémoire ; distinguer refus de budget et échec d'allocation.
2. **Destination directe d'expansion.** L'expansion conserve des shards `ForestEvent` puis une destination fusionnée. Les comptes exacts par tranche et ordre, suivis d'une somme préfixe, peuvent attribuer des intervalles disjoints. Opposer l'ordre et les objets à l'expansion actuelle, avec décalage d'offset et refus tardif ciblés. Les plateaux doivent garder leurs comptes complets.
3. **Coût des reparcours.** Le comptage parcourt les boules, puis chaque expansion d'ordre K reparcourt le tableau. Les plateaux réexécutent leur expansion complète avant filtrage. Évaluer un index compact par ordre ou des comptes réutilisables contre leur coût mémoire ajouté, sans cacher une troncature derrière un cache.
4. **Libération par lots certifiés.** Le tableau `BallData` reste global jusqu'au dernier fold. Qualifier de grandes entrées demande de préciser quelles données sont libérées par lot et comment une incidence ultérieure retrouve un représentant valide. Le certificat doit préserver les composantes et leurs niveaux de naissance ou de fusion. Mesurer ensemble candidats, survivantes, cofaces ajoutées, facettes distinctes et coûts bout en bout.

Ces étapes évitent de matérialiser toutes les facettes possibles et les cellules de Delaunay. Aucun benchmark 8k/16k/32k, aucune mesure GPU et aucune qualification industrielle ne sont rapportés dans cette note. GCP non utilisé.

## Admission des fils et portes intégrées

Le census actuel inclut [AxisBounds](CENSUS_AXIS_COURANT.md) ; le hash du tableau ci-dessus est celui de la campagne de résidence antérieure. Sa destination unique et son contrat `mhgp7-census-direct-v1` sont conservés. `census_merge_peak_bytes` compte la capacité de cette destination dès allocation et celle du tableau réellement ajouté par le mutant `keep-ball-chunks`, sans devenir une borne RSS.

Le tri annule l'admission si une création de thread échoue, joint tous les fils déjà créés puis propage l'exception. Une faute de comparateur fait quitter sa barrière au worker par `arrive_and_drop`, puis tous les fils sont joints avant propagation. `thread_failure_gate` couvre quatre créations partielles après deux fils, quatre fautes de travail, une panne de comparaison inter-tranches et un témoin de réutilisation. L'API transforme la panne en refus de ressources, sans payload public conservé.

La porte de résidence permanente complète la sonde indépendante : 12 scènes sur trois tailles et quatre nombres de fils, 15 refus d'allocation, cinq de comparaison et un lancement partiel. Elle oppose les charges utiles à la référence stable ; les trois mutants de permutation attendent le code 4, une mauvaise CLI le code 2 et un plancher non exercé le code 3. Le rendez-vous de l'ancien tri ne concerne que son témoin négatif ; le tri courant n'exige pas de tampon pour faire passer la porte.

Ces portes sont enregistrées et exécutées dans la [suite D complète de 323 tests](receipts_20260905/release/summary.json). `EXPECT_LINE` impose une ligne entière ; `EXPECT_PREFIX` est une option explicite aussi utilisée par les portes arithmétiques/MEB. Leur résultat est lié au code attendu, sans prétendre couvrir tout mutant imaginable. C6a reste un stub CPU. Les nouveaux deltas gardent leurs propres obligations de qualification.
