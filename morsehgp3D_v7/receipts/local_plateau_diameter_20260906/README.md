# Quotient local : supports complets et premier rang à diamètre

6 septembre 2026. Delta après `main` 22003315, limité à `src/forest/local_plateau.hpp` et `tests/local_plateau_gate.cpp`. `public_status=not_claimed`. Aucun raccord FULL, aucune suppression du refus des coquilles supplémentaires, aucun résultat de performance50k ou GPU.

## Delta qualifié localement

La porte construit désormais l'ensemble complet des masques minimaux depuis sa table Gram rationnelle indépendante : un masque vrai est minimal si toutes ses faces immédiates sont fausses. Cet ensemble doit être exactement égal à `minimal_supports()`, sans doublon ; la propriété « chacun des supports retournés est minimal » ne suffit plus. Le carré porte explicitement ses deux diamètres, masques5 et10. La mutation privée retire seulement le deuxième diamètre de cette clé de carré, tout en laissant la table booléenne intacte : le nouveau contrôle d'exhaustivité la réfute.

Pour `q_min=2`, `u>=3`, `K=p+1`, `rank()` émet directement une composante de représentant I plus le premier point U, couverture I∪U et membres réduits égaux aux u singletons. Les deux tableaux de DSU ne sont pas construits. La preuve est le graphe complet privé des paires antipodales : ces arêtes manquantes forment un couplage, dont le complément est connexe à partir de trois sommets. `u=2` est expressément exclu ; ses deux classes strictes restent distinctes. Le census partagé, la table des supports et tous les autres rangs sont inchangés.

Autorité lue : `audits/receipts_plateaux_full_20260906/COVERAGE_THRESHOLDS.md`, SHA256 `ddea165f228a405c4bfec0001eaaf18057e1df071a19ec4824f68079ba810dfe`. Le supplément autorise ce raccourci, pas la reconstruction des parents à partir d'une couverture ni la suppression d'une ancre.

## Champs et unités

`analytic_diameter_hub` indique ce nouveau chemin analytique. `inert_sufficient` conserve le seuil générique historique K≤p+q_min−2 ; le nouveau bool apporte une condition suffisante supplémentaire. Un rang dont la couverture et la composante sont déjà présentes peut toujours demander son ancre fermée : le cas réel p=9,K=10 est conservé.

Les compteurs sont du travail effectivement réalisé, pas les cardinaux d'un graphe non parcouru :

- `reduced_vertices` : sommets réduits émis ; u sur le raccourci.
- `strict_cofaces` : cofaces strictes effectivement traitées ; zéro sur le raccourci.
- `union_attempts` : tentatives d'union des étoiles ; zéro sur le raccourci.
- `dsu_mask_slots` : somme des tailles des deux tableaux de masques effectivement initialisés ; zéro sur les deux chemins analytiques et sur un rang absent, 2·2^u sur le chemin général.

Le raccourci est placé avant les deux constructions de tableaux. Il alloue toujours le descripteur de composante et sa liste de u membres ; ce n'est pas une API sans allocation. La préparation de la table complète coûte toujours O(u·2^u), partagée entre les rangs. Aucun gain de temps global ne se déduit de ces comptes.

## Vérifications bornées

O2 passe avec les mêmes 18 tables, 96 rangs, 96 composants/représentants, 14 cas du seuil générique et 26 naissances locales. Les supports q2/q3/q4 comptés restent20/14/4. Les quatre cas réels gardent leurs40 rangs et leurs IDs externes originaux. Les18 ensembles complets Gram sont comparés ; 17 chemins analytiques diamètre et68 chemins DSU sont effectivement exercés. Le cas p=5000 interdit le raccourci pour u=2 ; la coquille maximale u=12 l'exerce sans changer ses autres rangs témoins. Tous les compteurs physiques sont contrôlés branche par branche.

Les mutants sont des copies sources isolées, sans macro produit :

| Mutation | Code attendu et obtenu O2 | Cause exacte |
| --- | ---: | --- |
| Retirer le seul diamètre10 du carré | 1 | `support.complete_Gram_minima_set` |
| Désactiver le raccourci | 1 | `diameter_hub.dispatch` |
| Autoriser u=2 à tort | 1 | `diameter_hub.forbidden_domain` |
| Supprimer les unions du chemin général | 1 | `quotient.strict_component_count` |

Les18 commandes de préparation sont closes : six compilations, leurs dépendances, O2 0/2 et les quatre refus causaux. Reçu `prepare.receipt.json` SHA256 `e9fb6d2a0dbd14fb79eae5145b074c23d43519313cc6f8e64c10932e4ea43afd`. Le premier essai SAN est un échec d'environnement ptrace/LeakSanitizer, code1 sans résultat de test ; il est conservé tel quel dans `san.receipt.json`, SHA256 `da8d9ed120d764728a5f6a8acf2aee44eedc63974947034fad4ccf0a579c53dd`. La reprise séparée, exécutée par ROOT, est close avec codes0/2 : même ELF, détection de fuites conservée, aucune recompilation. Son reçu `san_resume/receipt.json` est SHA256 `b801e66ef00f3df9a978112da83d79bb7f49964b8899ac6d7a4e107dac1f89a6`. Les sorties O2/SAN sont littéralement égales, SHA256 `16b5503d88d77ef3ede1ecb58aad7104cb96999418f8ff8937b385c12fdf01ff`. Cette reprise ne modifie pas le statut de la capture ptrace failed.

Sources gelées : helper `df56fbf33ea3088218174f88a646c65702d97a0366ec848887f1846ae7666f2e`, gate `22bb006e26a7846c332d032f14a2fb09478efb6dfdaa9b134911187f6a44fce2`, oracle inchangé `7a002853749784bb14a8db178fbfe637244bd3019a08ae1daf92fd20aeae670d`. Les dépendances réelles sont épinglées avant/après chaque compilation. Les nouvelles sources sont copiées ; les anciens headers produit et Boost restent des dépendances liées par hash, pas des copies massives. Les mutations reproduisent chacune un remplacement physique unique du helper gelé.

Les commandes utilisent une attente directe, sans plafond temps/CPU/fichier ajouté. GCP non utilisé. Aucune branche, opération Git, modification d'audit, CMake ou document d'entrée par cet agent.
