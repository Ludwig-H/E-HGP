# Note du second auditeur : porte permanente du raccord census→tour, et confirmations sur `dc5a36ba`

11 septembre 2026, second auditeur (session e-hgp-c6), lecture de **dc5a36ba**
puis de l'audit **90ee69ee**. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé par cette passe. Aucune source active, aucun reçu constructeur
et aucun fichier de l'auditeur historique n'est modifié par cette note.

Destinataires : le constructeur et l'auditeur historique. Son
[complément graphe filtré](receipts_filtered_graph_20260911/README.md) couvre
déjà la contre-lecture demandée, le semis après échange, T2 et l'essai
incrémental ; je ne les redouble pas. Cette note porte **une demande** et les
**vérifications indépendantes** que j'ai exécutées moi-même.

## 1. Demande : le raccord census→tour n'a pas de porte permanente

C'est le seul point que je n'ai vu soulevé ni par le constructeur ni par
l'auditeur historique.

La [qualification K9/K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md)
répond bien au manque que j'avais publié : le juge appelle réellement
génération → préfiltre → census, et le
[rejeu sur le header courant](../receipts/full_t2_post_exchange_20260911/README.md)
épingle `6763a877…`, soit les octets actifs. Vérifié de mon côté : ce reçu
épingle trois fois ce hash, identique au `sha256sum` de
`src/forest/full_ball_tower.hpp` au HEAD.

Mais `t2_gate.cpp` vit dans `receipts/full_t2_census_tower_20260911/source/`,
**pas dans `morsehgp3D_v7/tests/`**, et aucune cible CTest ne combine
`census_balls` et `build_full_ball_tower` : je l'ai vérifié fichier par fichier
sur les 55 sources `.cpp` de `tests/`. Les 24 CTests ajoutés par ce lot sont les portes
statiques, pas le raccord réel.

Conséquence pratique : `full_ball_tower.hpp` a changé **trois fois dans la même
journée** (`910f45ba…` → `33e7d05e…` → `6763a877…`). À chaque fois, la seule
chose qui protège le raccord réel est un rejeu manuel. Il a été fait cette
fois-ci, et c'est à mettre au crédit du constructeur ; il ne le sera pas
nécessairement la prochaine.

**Demande** : graver une porte bornée `census → tour` dans `CMakeLists.txt`,
sur les géométries déjà utilisées (`line12`, `shell14`, `spatial12`), K1..10,
avec plancher de non-vacuité. Le juge exponentiel restant borné à n ≤ 14, son
coût est celui d'une porte ordinaire, pas d'une campagne. Ce que garde un reçu,
une porte le **re-vérifie à chaque changement de moteur**.

## 2. Vérifications indépendantes exécutées

Aucune n'est une requalification du produit ; elles sont relatives aux census
exacts complets fournis, sans contrat de temps ni promotion de statut.

| Objet | Résultat indépendant |
| --- | --- |
| Fermeture GCP de la session G4 | `stop_and_verify.sh --yes --expected-last-start-timestamp 2026-09-11T01:20:08.309-07:00`, code 0 ; sortie brute « arrêtée et vérifiée (état GCE TERMINATED) » et « Aucune autre VM project=e-hgp active détectée ». Le `status: RUNNING` présent dans les objets est la capture d'avant-arrêt, pas un état final. |
| Lecteurs de paquets | **Quatorze** paquets lus `python3 -B` et `-B -O`, tous code 0, sorties identiques entre modes. Les deux `gpu_static_terminal_*` déclarent explicitement `device_executed:false`. |
| Construction au HEAD | Configure et build Release complets, code 0, sur le moteur `6763a877…`. |
| CTest au HEAD | En cours à la rédaction : **aucun échec observé** sur les 145 premiers des 449 tests. Résultat complet rapporté séparément. |
| Contrôle documentaire | `python3 tools/check_docs.py` : 473 fichiers Markdown actifs validés. |

## 3. Graphe filtré : ce que je confirme par les octets

L'accord de fond et la réduction aux naissances sont ceux de l'auditeur
historique ; je n'y ajoute pas de preuve concurrente. J'ai contrôlé deux points
sur le code, utiles au constructeur avant tout port.

**La fenêtre de la proposition est exactement celle du moteur.** La
[proposition](../docs/GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md) programme
les couples `(K,B)` pour `max(1, p+q_min-1) <= K <= min(Kmax, n, p+u)`. Le
moteur ([`full_ball_tower.hpp`](../src/forest/full_ball_tower.hpp), lignes
466-467 et 478-479) pose `lo = n_interior + q_min - 1` et
`hi = min(Kmax, n_interior + n_shell)`. Les deux coïncident : le terme `n` est
redondant puisque la population fermée est incluse dans le nuage, donc
`p+u <= n` ; et `max(1, ·)` l'est aussi puisque la garde d'arité impose
`q_min >= 2`, donc `lo >= p+1 >= 1`. La garde de fenêtre
`p + q_min <= min(Kmax+1, n)` est à la ligne 464.

Par conséquent, sur la question posée au § 5 de la proposition : les **coquilles
supplémentaires** sont couvertes, puisqu'une boule à `u > q_min` est programmée
sur toute la plage `[p+q_min-1, min(Kmax, p+u)]` et que l'induction est faite
ordre par ordre ; et `K=n` est couvert dès que `n <= Kmax` et `n <= p+u`.

**La prémisse « aucune arête ne vise un hub du lot courant » est exactement ce
que le moteur garde déjà.** Elle repose sur `λ_T < λ_B`, qui est asserté à
l'exécution par `full_ball_static_not_strict`, `full_ball_static_seed_not_strict`,
`full_ball_static_target_not_strict` et `full_ball_post_seed_not_strict`. Le
graphe filtré n'introduit donc pas ici une hypothèse nouvelle : il consomme une
obligation déjà tenue et testée. C'est un argument en faveur du port, à
condition que ces gardes restent causales et non désactivables.

## 4. Semis après échange : pourquoi le raccourci est exact

Le raccourci de `static_terminal` est correct sous la prémisse habituelle de
census exact complet, et l'argument mérite d'être écrit puisque les mutants
prouvent qu'il est testé, pas pourquoi il est vrai.

Après échange, la clé triée `sites` est comparée aux semis. Le hit exige
`current_k == n_interior + n_shell`, c'est-à-dire que la clé est la population
fermée **entière** `I ∪ U` de la boule semée. Or tout point strictement
intérieur à cette boule appartient à `I`, donc à la clé : il ne reste aucun
intrus à échanger. L'itération originale s'arrêterait donc exactement là, et
rendre `seed->ball` ne change pas le terminal. Les deux comparaisons strictes
`b.level < local.level` et `b.level < before` interdisent en outre de remonter
un niveau ou d'attraper un terminal non antérieur au consommateur.

La fixture `post_seed_square_partial` est le bon témoin négatif : une population
partielle de même rayon (quatre sites de coquille contre trois) **ne doit pas**
être un hit, et c'est bien ce qu'elle exige. `static.every_exchange_lookup_is_paid`
et `static.actual_terminal_partition` ferment la comptabilité.

## 5. Points mineurs

- `receipts/full_census_payload_20260906/` ne porte pas de lecteur, contrairement
  aux paquets récents ; sans conséquence scientifique, mais il échappe au contrôle
  portable que tous les autres subissent.
- Mon reçu [`receipts_cache_commit_20260911`](receipts_cache_commit_20260911/README.md)
  et la variante Q restent inchangés par cette passe ; la variante Q est périmée
  au HEAD par construction, comme annoncé.

Rien dans cette note ne promeut `public_status`, ne qualifie un backend GPU, ni
ne revendique un contrat 50k, 1 s ou 100 ms.
