# Réception v6 : identités de cache, voies et coquilles à compléter

23 septembre 2026, produit `a1d7a9bc`. Le correctif de réception ferme les
quatre lacunes v5 suivies dans la [contrelecture B](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md).
Les **21 selftests protocolaires** passent en Python normal et sous `-O`
sur un `HEAD` stable ; le vrai préflight local de 1 500 sites exerce les
quatre leviers. Cet audit n'a pas utilisé GCP. La
[tentative R4](../receipts/g4_tower_r4_preempted_20260923/README.md) s'est arrêtée avant
le worker et ne donne aucun reçu LIVE de tour v6.
Les mutations ci-dessous sont acceptées par `validate_probe` avec statut
`complete_relative` sur le JSON local K5/W8/s8
`build/v9-runs/dead_20260923/cache_s00_k5.json` (nuage sans sol
08/000000, non versionné). Les mutations citées séparément ne changent
qu'un compteur ; les deux cas groupés sont indiqués explicitement.

| Mutation isolée acceptée | Pourquoi elle contredit le produit | Garde minimale pour une tour complète |
| --- | --- | --- |
| `witness_cache_queries=0` alors que `witness_cache_rejected_pairs=16 524 694` ; `witness_cache_node_tests=0` est aussi accepté | Une paire ne peut être rejetée par le cache sans une requête ni sans tester un nœud. | `cache_rejected_pairs≤cache_queries≤expanded_pairs` ; `cache_rejected_pairs>0 ⇒ cache_node_tests>0`. |
| `both_edges=q3_edges+1`, ou même `both_edges=cover_builds+1` | Une arête aux deux voies appartient à chacun des ensembles q3 et q4 et à celui des covers. | `both_edges≤min(q3_edges,q4_edges)` et `q3_edges+q4_edges−both_edges≤cover_builds`. |
| `dead_q3_proved=cover_builds+1` ; mettre les deux comptes `dead_q3_proved=dead_q4_proved=0` est aussi accepté par la validation d'un cas | Une voie q3 ne peut être prouvée qu'une fois par cover. Chaque cover construit portait au moins une voie avant la preuve. | Pour `i∈{3,4}`, `dead_qi_proved+dead_qi_open≤cover_builds` ; `q3_edges+q4_edges−both_edges+dead_q3_proved+dead_q4_proved≥cover_builds`. |
| `catalogue.shell_over_12=1`, `catalogue.max_shell=13`, ou un transfert d'une unité de `by_shell[2]` vers `by_shell[16]` | `tower_chain.cpp` refuse transactionnellement toute coquille de plus de 12 avant de rendre `complete_relative`. | `shell_over_12=0`, `max_shell≤12`, `by_shell[j]=0` pour `j>12`. |

Ces gardes sont nécessaires, pas une preuve générale de complétude du
catalogue. Les imposer dans le lecteur hôte sur **chaque cas complet** et
les tuer par des mutations ciblées de la porte v6 ; conserver le refus
explicite pour les cas dégénérés. Le faux préflight teste déjà une activité
non nulle, mais ne corrige pas les compteurs impossibles d'un cas LiDAR.
La preuve de garde archivée a en outre une lacune distincte, documentée par
[B](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md) : son calendrier et certains
champs du mark ne sont pas entièrement rejugés contre le contexte hôte.
