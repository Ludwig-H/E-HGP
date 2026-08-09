# Audit de promotion et de traçabilité — commit `7fa39b1`

Date : 9 août 2026 UTC.

> [!CAUTION]
> **La promotion annoncée par le message du commit n'est pas soutenue par son contenu.** Le commit `7fa39b1d8c9d3b566bcd098bb4bdd2dbc107d7af` ajoute un oracle expérimental non intégré et embarque simultanément [`AUDIT_ORDER_K_BFS_A8111F0.md`](AUDIT_ORDER_K_BFS_A8111F0.md), qui conclut **NO-GO produit** avec un contre-exemple exact encore ouvert. Il ne peut donc ni « décider l'architecture », ni « retirer la source d'ancres ».

## 1. Contradictions internes au snapshot

| claim du message de commit | état vérifiable dans le même commit |
| --- | --- |
| « architectural decision » et retrait de la source d'ancres | `README.md` reste en M1/M2.1 et dit qu'il n'existe pas encore de v3; `PROPOSITION.md`, CMake et l'oracle ne sélectionnent pas ce chemin |
| le catalogue est « exactement » le niveau shallow relevé | `order_k_vertices` ne retourne que des sommets portés par quatre points; les arités un à trois, le bon centrage, le shell complet et le statut public sont absents |
| le coût suit la sortie au lieu de $n^2$ | chaque sommet visité lance quatre pinceaux dans deux directions et rescane le nuage; en position générique, le travail est exactement $8(n-4)V$, soit $\Theta(nV)$, avec `seen` et `visited` globaux |
| le niveau varie seulement de $\pm 1$ | la formule implémentée autorise aussi une variation nulle; le commentaire mathématique est plus fort que le code |
| le germe de face convexe est nécessairement de niveau zéro | la fixture u16 bien centrée de l'audit inclus montre qu'un témoin coplanaire constant est ignoré : le code publie le support `0124` au niveau 0 au lieu du niveau 1 et manque les vrais sommets shallow |
| accord exhaustif sur 275 nuages uniformes et LiDAR | aucun driver, target CMake, test oracle, reçu, commande, graine, digest d'entrée, sortie brute ou sidecar de cette campagne n'est présent dans le commit |
| signe `InSphere` calibré sur un tétraèdre explicite | la correction de signe est réelle, mais la « calibration » n'existe que dans un commentaire du header; aucun test unitaire indépendant ne la verrouille |

Le header reconnaît lui-même que la connexité du sous-graphe shallow n'est pas démontrée. Or le parcours refuse d'enfiler tout voisin au-dessus du plafond : cette preuve ou un mécanisme complet multi-germe est une condition de complétude, pas une amélioration ultérieure.

## 2. Absence d'intégration et de preuve exécutable

Au snapshot audité :

- SHA-256 de `prototype/order_k_bfs.hpp` : `a8111f02f76e458912e2a2e1e1ff2d4ee0b71bba31af7993975f49fa6c792a3c`;
- SHA-256 de `CMakeLists.txt` : `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874`;
- SHA-256 de `oracle/oracle_main.cpp` : `ed0fe1c1b86a5d0b4dd1a96a6ab00ccd094f0dbd1f3e5abcff83b27029989dbc`.

Une recherche hors Markdown ne trouve `order_k_bfs`, `order_k_vertices` ou `OrderKStatistics` que dans le header lui-même. Aucun exécutable du dépôt ne le compile ou ne l'appelle. Les 275 nuages annoncés ne sont donc ni rejouables par CTest, ni vérifiables depuis un reçu versionné. Un résultat de probe sous `/tmp` peut guider la recherche; sans provenance et sans porte indépendante, il ne qualifie pas une architecture.

La correction de signe doit devenir une fixture exécutable comparant `InSphere` à une autorité indépendante sur témoins intérieur, shell et extérieur, avec permutations d'orientation. Un différentiel dont les deux côtés partagent le prédicat ne peut pas détecter une inversion globale — le message de commit le reconnaît lui-même.

## 3. Statut correct et action attendue

Le statut défendable de `order_k_bfs.hpp` à `7fa39b1` est : **oracle expérimental borné, non intégré, non complet et non qualifiant**. L'encodage entier de l'ordre d'un pinceau reste un résultat utile, mais il ne ferme ni les quatre arités, ni `RelevantGP`, ni la connexité, ni le coût produit, ni le contrat 50 k.

Avant toute nouvelle promotion, il faut au minimum :

1. figer le contre-exemple coplanaire et la calibration `InSphere` dans des tests indépendants;
2. intégrer un sujet oracle explicite avec reçus complets et reproduire la campagne annoncée;
3. fermer la preuve de complétude et les arités un à trois;
4. supprimer le facteur $\Theta(nV)$ et démontrer que les structures globales évitées restent effectivement non matérialisées;
5. documenter une ouverture de phase cohérente avec `README.md` et le statut d'implémentation.

Jusqu'à ces portes, la promotion du message de commit doit être lue comme une hypothèse de recherche réfutée par le propre audit du commit, jamais comme une décision d'architecture acquise.

GCP non utilisé.
