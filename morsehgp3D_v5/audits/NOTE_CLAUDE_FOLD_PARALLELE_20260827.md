# Note — parallélisation du fold (point 1 après la session G4)

- **Date :** 27 août 2026 — **pin :** le commit qui contient cette note
- Mesures sur cette machine (8 vCPU, 31 Go, **partagée** avec la suite ASan pendant les mesures) ; aucun claim de SLO.

## Ce qui a été fait (sortie bit-identique : digests v4 inchangés, portes vertes)

1. **Internement partitionné par empreinte** (`prepare_fold`, 64 partitions fixes par les six bits hauts de l'empreinte, table privée par partition, tid locaux composés `partition << 26 | tid`) ; fid finaux par tri des clés uniques puis **fusion k-aire parallèle par rangs de valeurs** (séparateurs pris dans la plus grosse partition, `lower_bound` par partition, offsets de sortie connus avant la fusion).
2. **Tri stable parallèle** des événements par niveau exact (`src/parallel/sort.hpp`, identique à `std::stable_sort` par contrat — les ex æquo d'un lot gardent l'ordre d'entrée, dont dépend l'ordre d'émission des deltas).
3. **Comptage par K parallèle** (`count_events_by_k`).
4. **SHA-256 accéléré** (SHA-NI, répartition dynamique) : le digest de conformité à `uniform` 8000 passe de 10,7 s à 2,9 s ; égalité bit à bit avec le chemin portable (`mhgp5_sha256_gate`).
5. **Pipeline à deux étages** dans `run_pipeline` : étage B (réduction séquentielle, digest, callback, libération) de l'ordre K dans un fil d'arrière-plan pendant l'étage A (expansion, tri, internement, fusion) de K+1 ; un seul B à la fois, joint avant le suivant ; résidence bornée à deux ordres (+0,9 Go à 8000).

## Mesures

| mesure | valeur |
|---|---|
| fold seul, K=10, `uniform` 4000, banc apparié contrebalancé 10 paires ABBA, 8 fils | **médiane ×3,27**, 10 victoires sur 10, p = 0,001 (signature identique à chaque paire) |
| `uniform` 8000, temps mur complet, 1 fil → 8 fils | **147,0 s → 35,4 s** (×4,2) ; RSS max 1,9 → 2,8 Go |
| détail 8 fils (cumuls par étage) | gen 15,7 s ; préfiltre 4,7 ; census 4,2 ; fold : tri 0,8, intern 3,5, fusion 0,9, **réduction 6,2 (séquentielle, recouverte)** ; digest 2,9 |

Pour mémoire, la v4 mettait 50,5 s à `uniform` 8000 à 8 fils avec 5,7 Go de RSS (reçu `campagne_v5_210571fba29c`, colonne v4).

## Ce qui reste séquentiel, et pourquoi

La réduction (union-find à macro-lots, rôles par lot, deltas) dépend de l'ordre des lots : elle reste un seul fil, mais elle est **recouverte** par l'étage A de l'ordre suivant. Sur les familles régulières, la borne devient max(A, B) par ordre plutôt que la somme. Une réduction parallèle (union-find concurrent) changerait l'ordre d'émission des deltas d'un lot — donc le digest v4 — et n'est pas engagée.

## À rejouer sur G4

Les 50 000 points (219 s `uniform`, 375 s `eight_clusters` avec le fold séquentiel) sont à re-mesurer par une nouvelle session gardée ; la campagne locale de conformité 8000/16000/32000 est relancée d'abord.
