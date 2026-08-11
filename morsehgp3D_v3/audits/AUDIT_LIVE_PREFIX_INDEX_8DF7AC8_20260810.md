# Audit live constructif — premier fallback préfixe--préfixe

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=complete_bounded`, `mode=hybrid_prefix`,
`public_status=not_claimed`.

Base committée : HEAD `8df7ac8`. Worktree audité par SHA-256 :

| fichier | empreinte |
| --- | --- |
| `prototype/prefix_index.hpp` | `296525ea3be45a44c02c02d6d33f26d8601d290def8992f4c3ab8df4d0a46c98` |
| `prototype/prefix_index_gate.cpp` | `8dccf5e3140d1aa6f16050fcc4fefeff16f2975d08072bb82ab62462359d1e26` |
| `prototype/prefix_mass_probe.cpp` | `5a0da948d9965d1b58fb7507957f30f25704f9a712b40657ce944683a88e9329` |
| `prototype/saturated_fold_hybrid.hpp` | `219feecf675890d391b709a6163f13835e0ac68621273d733e30b927506bbb64` |
| `prototype/postings_join_gate.cpp` | `273aaa356dc83691bddcf810a7d983c3cdacf815ea49de87e873b2991f78bd3a` |
| `prototype/saturated_pipeline.cpp` | `3b237bb2f4d337e834631a09122edabc1a68661f1390e4c7a008f30fc0c9eb3d` |

## Verdict positif

Claude a correctement transformé le théorème de
[`NOTE_SOLUTION_GPU_INDEX_PREFIX_PREFIX_20260810.md`](NOTE_SOLUTION_GPU_INDEX_PREFIX_PREFIX_20260810.md)
en une sixième vérité CPU du fold :

- les membres triés en `PointId` imposent bien un ordre total commun;
- le lot exact entier est indexé avant la première requête et les futurs restent
  invisibles;
- le vrai couple de générateurs est recertifié avant toute projection DSU;
- le fallback préfixé concorde avec `G2`, postings, global, `face-owner` et
  hybride sur les campagnes bornées;
- les mutants préfixe trop court, dernier posting omis, recertification sautée
  et projection racine précoce meurent.

C'est le bon oracle d'intégration. Il ne faut pas le jeter pour aller sur GPU :
il devient la vérité byte-à-byte de la future CSR device.

## P0 SLO — posséder chaque paire une fois

Le fold live interroge tous les candidats depuis chaque fallback
([`saturated_fold_hybrid.hpp`](../prototype/saturated_fold_hybrid.hpp#L433)).
Deux fallback du même lot se trouvent dans les deux directions. L'idempotence
de la DSU conserve la partition, mais double recertification, attaches tentées
et trafic. La gate abstraite transforme ensuite les paires en `std::set`
([`prefix_index_gate.cpp`](../prototype/prefix_index_gate.cpp#L43)); elle ne voit
donc pas cette multiplicité.

Pour le lot courant, calculer d'abord `is_query` et un `ActivationId`
canonique. Une requête $M$ conserve le candidat $N$ si :

- $N$ appartient à un lot strictement antérieur; ou
- $N$ est non-requête dans le lot courant; ou
- $N$ est requête et `ActivationId(N)<ActivationId(M)`.

Ainsi `Q--Q` est possédé une fois, `Q--R` est toujours présent et `R--R` reste
l'obligation du sidecar principal. La gate doit comparer le multiensemble
canonique et exiger multiplicité un, pas seulement son support ensembliste.

## P0 SLO — préflighter les hits, pas seulement les entrées

La masse persistante $L_k$ est bien reçue, mais elle ne borne pas les lectures.
Poser $d_{x,k}=\lvert I_x^{(k)}\rvert$ et $q_{x,k}$ le nombre de requêtes dont
le préfixe contient $x$. Le nombre de hits avant unique est exactement :

$$H_Q^{(k)}=\sum_x q_{x,k}d_{x,k}.$$

En tout-requête, il devient $\sum_x d_{x,k}^2$. Ce scan de degrés doit être fait
en entier vérifié avant allocation; il donne aussi une distribution exacte du
travail par requête et permet les chunks sous arène.

La sonde tout-requête actuelle confirme pourquoi ce préflight est prioritaire.
Sur `--points 48 --smax 11 --max-order 3 --seed 20260810 --threshold 1` :

| famille | générateurs | ordre | entrées | hits | candidats hors self | temps requête+recertification CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| uniform | 4 777 | 1 | 36 857 | 42 944 187 | 15 484 340 | 2,999 s |
| uniform | 4 777 | 2 | 32 080 | 33 713 012 | 14 038 670 | 2,742 s |
| uniform | 4 777 | 3 | 27 351 | 25 916 423 | 12 169 768 | 2,199 s |
| terrain | 1 778 | 1 | 12 690 | 4 609 706 | 1 580 340 | 0,233 s |
| terrain | 1 778 | 2 | 10 912 | 3 661 168 | 1 431 752 | 0,258 s |
| terrain | 1 778 | 3 | 9 182 | 2 918 266 | 1 271 248 | 0,196 s |

Le terrain réduit fortement la masse, pas le théorème : le tout-requête reste
le mauvais produit. La sonde doit ajouter `--mask=hybrid` et publier
`p50/p95/p99/max` de $H_Q$ par requête et par lot.

La variante $t>1$ est exacte, mais n'est pas automatiquement plus rapide. Sur
le même terrain, passer à $t=k$ retire tous les faux candidats à $k=2,3$, mais
monte les hits de `3,66 M` à `4,58 M`, puis de `2,92 M` à `4,50 M`; le temps
CPU monte de `0,258 s` à `0,314 s`, puis de `0,196 s` à `0,267 s`. Garder
$t=1$ comme baseline et choisir $t$ seulement depuis les masses mesurées.

## P1 — forme GPU directement dérivable

Le `PrefixIndex` live est un `vector<vector<int>>`
([`prefix_index.hpp`](../prototype/prefix_index.hpp#L53)); chaque requête
concatène, alloue et trie
([`prefix_index.hpp`](../prototype/prefix_index.hpp#L74)). C'est approprié pour
l'oracle CPU, pas pour 50 k.

La forme device recommandée est :

1. ordonner les générateurs par `(niveau_exact,ActivationId)`;
2. construire, ordre par ordre, une vraie CSR dont chaque posting est trié par
   cet identifiant;
3. trouver par recherche binaire la fin inclusive du lot visible;
4. affecter un warp par requête et fusionner directement ses au plus onze
   postings par multiway merge;
5. compter ensemble les occurrences du même candidat, appliquer le filtre de
   possession et le seuil $t$;
6. recertifier l'intersection des membres canoniques dans le warp;
7. seulement ensuite projeter vers les racines strictes gelées et émettre le
   vrai carrier.

Ce merge ne matérialise ni tous les hits, ni un `sort` par requête. Les requêtes
sont packées en chunks dont la somme $H_Q$ passe l'arène; une requête lourde
reçoit un bloc coopératif ou un refus avant écriture.

Pour rare-first, conserver deux vues : membres canoniques triés `PointId` pour
la recertification, et `point_rank[k]` immuable pour sélectionner les préfixes.
Réordonner les membres casserait le merge d'intersection actuel. Le reçu porte
le digest de l'ordre global; un mutant `order_per_query` doit mourir.

## P1 — reçus et fixtures manquants

`PrefixIndexReceipt::unique_candidates` inclut le self dans `prefix_query`,
alors que la sonde de masse incrémente le même champ après retrait du self.
Séparer `candidate_ids_including_self` et `candidate_pairs_after_filter`, ou
normaliser le point de comptage avant de comparer les outils.

Ajouter les mutants/fixtures :

- `future_visible` et `stage_query_sequentially`;
- `duplicate_posting` et `double_query_pair`;
- `order_per_query`;
- lot ex aequo avec fallback précoce et fast tardif, dont l'arête disparaît si
  le lot n'est pas entièrement stagé;
- paire sans extrémité dans $Q$, reçue uniquement par le sidecar `R--R`;
- campagne `PointId`, puis permutation globale commune rare-first, avec les
  mêmes couples recertifiés.

## P2 — hang hostile de la gate

La CLI accepte `universe=8..10`, mais la campagne tire un rang jusqu'à 11 puis
attend autant de valeurs distinctes
([`prefix_index_gate.cpp`](../prototype/prefix_index_gate.cpp#L109),
[`prefix_index_gate.cpp`](../prototype/prefix_index_gate.cpp#L163)). La boucle
ne termine pas :

~~~text
timeout 3s build/v3/mhgp3v_prefix_index_gate \
  --families 1 --sets 2 --universe 8 --max-order 3 \
  --min-pairs 0 --min-false 0
exit=124
~~~

Exiger `universe>=11` ou tirer le rang jusqu'à `min(11,universe)`. Avant un
kernel, valider aussi `k>=1`, membres uniques/triés/bornés, pointeurs, tailles et
sommes de compteurs. Le probe parallèle reste à un thread tant que la data race
du `CertifiedIndex` épinglée dans la réponse 50 k n'est pas corrigée.

## Tests reproduits

- build Release de `prefix_index_gate`, `postings_join_gate`,
  `saturated_pipeline` et `prefix_mass_probe` : vert;
- 11 CTests `prefix/hybrid-prefix` : 11/11 en 0,64 s;
- trois campagnes postings normales : 3/3 en 0,43 s;
- deux CTests de la sonde : 2/2 en 12,30 s;
- campagne `20 x 11`, cinq ordres : 3 004 générateurs, 4 457 requêtes,
  411 796 hits, 202 652 candidats comptés par le fold, 163 575 recertifications
  positives, 34 620 faux candidats, accord intégral;
- commande hostile `universe=8` : hang reproduit, `timeout` 124.

## Prochain geste conseillé

Corriger d'abord le hang et graver la possession canonique. Ajouter ensuite le
préflight $H_Q$ et le mode de sonde du vrai masque hybride. Ce reçu détermine la
taille des chunks et dit si le multiway merge device est admissible. Il est plus
informatif qu'une génération CPU 50 k et peut être obtenu sur les catalogues
déjà présents avant une nouvelle session G4.

GCP non utilisé.
