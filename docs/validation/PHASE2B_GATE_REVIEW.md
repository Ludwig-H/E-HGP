# Revue de porte Phase 2B — clôture des prédicats exacts GPU

> [!IMPORTANT]
> Cette revue ferme la Phase 2B pour le backend `cuda_g4`, le profil prioritaire `hgp_reduced` et le mode `certified`. Elle qualifie les trois prédicats GPU livrés et leur repli CPU exact. Elle ne ferme ni la Phase 4 ni G2, ne prouve aucune complétude de catalogue ou de hiérarchie et ne promeut aucun résultat public vers `public_status=exact`.

## Décision

- Phase : `2B` — prédicats exacts GPU.
- Backend qualifié : `cuda_g4`, cible AOT `sm_120` sur G4.
- Profil prioritaire : `hgp_reduced`; la couche de prédicats reste commune et n'apporte aucune preuve nouvelle à `full_pi0`.
- Mode : `certified`.
- Porte d'entrée : satisfaite par les Phases 2A et 3 fermées.
- Verdict de sortie : **satisfait**.
- Effet opérationnel du commit de fermeture : Phase 2B `completed`; Phase 4 `in_progress`, puisque ses dépendances 2A, 2B et 3 sont fermées.

La décision repose sur les certificats de campagne, les oracles exacts, la vérification en lecture seule, les tests négatifs et la qualification CUDA réelle. Elle ne repose ni sur un benchmark moyen, ni sur le taux de filtrage GPU. Les certificats automatiques conservent volontairement `phase_gate_closed=false`, `qualification_claimed=false`, `public_status=null` et `scientific_result_claimed=false`; le runner prouve les exécutions, tandis que la présente revue porte la décision de dépôt.

## Périmètre livré

| prédicat | proposition GPU | décision publiée | repli fermé |
|---|---|---|---|
| `compare_squared_distances` | filtre FP64 par intervalles dirigés | signe GPU connu audité, sinon `unknown` | replay CPU multiprécision |
| `orientation_3d` | déterminant FP64 par intervalles dirigés | signe GPU connu audité, sinon `unknown` | replay CPU multiprécision |
| `power_bisector_side` | forme affine de labels triés par axe | signe GPU connu audité, sinon `unknown` | replay CPU multiprécision |

Le GPU ne publie jamais une égalité à partir de `unknown`. L'identifiant et la commande de replay restent autoritatifs sur l'hôte. Proposition flottante, décision certifiée, réduction hiérarchique et statut public restent quatre états distincts.

## Campagnes de fermeture

Les deux campagnes utilisent le schéma `1.1.0`, la portée `production`, la même configuration gelée et deux graines distinctes. Chacune contient 10 800 000 cas de base et 1 080 000 signes métamorphiques. La seconde campagne apporte donc seule 11 880 000 signes supplémentaires, au-delà du seuil de $10^7$ exigé par la feuille de route.

| mesure | Phase 2A rejouée | Phase 2B indépendante | agrégat |
|---|---:|---:|---:|
| cas de base | 10 800 000 | 10 800 000 | 21 600 000 |
| signes métamorphiques | 1 080 000 | 1 080 000 | 2 160 000 |
| signes totaux | 11 880 000 | 11 880 000 | 23 760 000 |
| signes GPU FP64 certifiés | 11 304 000 | 11 304 000 | 22 608 000 |
| `unknown` GPU transmis au CPU | 576 000 | 576 000 | 1 152 000 |
| décisions CPU multiprécision certifiées | 11 880 000 | 11 880 000 | 23 760 000 |
| zéros exacts | 216 000 | 216 000 | 432 000 |
| métamorphismes vérifiés | 1 080 000 | 1 080 000 | 2 160 000 |
| mauvais signes | **0** | **0** | **0** |
| inconnus restants | **0** | **0** | **0** |

Chaque campagne totalise 3 960 000 décisions par prédicat. Les relations métamorphiques ferment exactement sur 882 000 signes conservés et 198 000 signes opposés par corpus. Les compteurs de tri-state, de zéros, de replis et de replay CPU ferment sur le nombre de lignes; aucune fréquence d'étage n'est utilisée comme argument de correction.

### Racines ordonnées indépendantes

| campagne | corpus | oracle | CPU | GPU |
|---|---|---|---|---|
| Phase 2A | `76544b0c85c2f6602e560bd007762609e1feb080f5fe13a71ec4d0d3f31f294b` | `62a4f14bc4d25971d67d7c321bdea9091d45f7a17dc37d79a292fd34cd078210` | `dd42886ea67251f5a7193d52bfe34d2dcfb14e679bf67377dbd2d31d09cd53d9` | `7a96ef52b5e4f2280bb993c44a4760bf93c6a8589cde43d8373a599a0631c210` |
| Phase 2B | `ea0879fee945bd1827485fd26907c47ce3bed9f9764549984ac2a5468f736317` | `a54049017e7bfb1c97ca4ad1f0cbc03045bde51e2275e14a316e225e80eb03d6` | `a5dad711dfc9b211c29caa5e64421c3753de7cd68e3bb87ef74aa842e639d44b` | `bb7ea1a62aade9cd6c1ca9a7adfcf14d582665bc120ae36fe471ba57a1833738` |

La justification certifiée est `independent_seed_and_distinct_corpus_root`. Elle interdit de substituer un corpus à l'autre; elle ne revendique pas une disjonction cas par cas non démontrée.

## Reprise, provenance et échec fermé

- Le commit des campagnes longues est `39a011c92d67b6de9ba38d60dc884b926cc36c5f`, avec dépôt propre et empreinte d'arbre `6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`. Le commit propre et poussé qualifié pour la dernière mesure chaude est `9ad398b8b83c5477fb52ab7d357aff3309931d4f`.
- L'image est `sha256:e3d96c187ca405790227e02aef1a66ca47df0820bb6b2a86b097359105956d58`.
- Les exécutables GPU et CPU valent respectivement `da83aa5937dfed3fcbfe651173aede52dcc647b80d08481d6565b72bfca9fb20` et `fce527af88a6750ac719ffd082d9c2d28f3e78ae80bcdef21c534604cc433511`.
- Le générateur, la configuration et le runner valent respectivement `f75c22cb6ecb376953a463ecaf7244b516e5b79e4836016e1d928cffa7139cc8`, `984bc1c54afa7e96e4a8f0bd486f73cb95ddde6304d2217893bb3207a67a6c5e` et `40262e9cc2352c8aea353ca2c4f915c02d785e39c54776fbe0c9b35d7b1aca22`.
- Les 55 chunks de chaque campagne sont contigus et bornés à 196 608 cas. Le verrou POSIX, les écritures atomiques, les `fsync`, les hashes et les racines sont revérifiés à chaque reprise.
- La vérification finale monte checkpoints et exécutables en lecture seule, recalcule l'arbre avant et après, puis relit les deux certificats et leur agrégat sans mutation.

Le smoke de campagne couvre 660 signes, la reprise et les racines. Sa suite négative rejette notamment second écrivain, checkpoint incomplet, chunk altéré ou orphelin, racine ou compteur forgé, JSON non canonique, changement du dépôt ou d'un exécutable, relation métamorphique fausse, contradiction GPU et témoin de puissance non représentable. Le contrôle statique Phase 2B rejette 63 mutations, y compris les directions d'arrondi, signes de cofacteurs, réductions homogènes, affinité CUDA, sortie compacte, réutilisation résidentielle et altérations du chemin `warm_context_e2e`.

## Qualification CUDA réelle et contexte résident

Les commits qualifiés compilent avec NVCC 12.9.86 en profils CUDA release et audit pour `sm_120`. Les trois noyaux sont AOT, sans fast math; `ptxas` ne rapporte aucun spill. Les différentiels matériels de l'archive Phase 2A comptent 2 064 distances, 2 070 orientations et 2 079 bisecteurs de puissance. Respectivement 2 061, 2 067 et 2 073 signes GPU connus sont audités, tandis que 3, 3 et 6 cas sont transmis au CPU; tous terminent sans contradiction ni inconnu.

Le harness résident traite 1 984 entrées dans 16 lots pour chacun des chemins résident et éphémère. Il alterne les trois prédicats, les tailles grand–petit–grand, deux contextes, deux paires concurrentes et trois lots vides. Les 1 327 décisions GPU connues et 657 replis donnent exactement les mêmes décisions et compteurs. `compute-sanitizer --tool memcheck --leak-check full` termine avec `ERROR SUMMARY: 0 errors` et `LEAK SUMMARY: 0 bytes leaked`.

La mesure `warm_context_e2e` finale exécute, pour chaque prédicat, un warmup puis 31 répétitions mesurées de 65 536 cas sur un même contexte. Le chronomètre entoure l'API asynchrone jusqu'à `.get()` : validation, packing, transferts, noyau, repli CPU et synchronisation sont inclus; la génération des entrées est exclue.

| prédicat | min (ns) | p50 (ns) | p95 (ns) | max (ns) | MAD (ns) |
|---|---:|---:|---:|---:|---:|
| distance | 105 935 481 | 107 268 960 | 108 741 671 | 108 933 220 | 676 940 |
| orientation | 156 700 046 | 158 774 816 | 162 199 595 | 162 203 986 | 1 351 680 |
| puissance | 179 281 734 | 180 766 484 | 182 756 654 | 184 231 713 | 527 879 |

Pour chacun des trois prédicats, les 31 lots mesurés ferment sur 2 031 616 entrées : 1 354 421 signes GPU connus, 677 195 `unknown` transmis en 31 lots au repli CPU exact, 677 195 zéros exacts et aucun inconnu terminal. Distance et orientation décident leurs replis par expansions; puissance les décide en multiprécision. Cette fermeture exacte des compteurs qualifie le chemin mesuré, sans transformer les temps observés en preuve de correction ni en revendication de gain.

La cible est une NVIDIA RTX PRO 6000 Blackwell Server Edition, capacité de calcul 12.0, 97 887 MiB, pilote 580.159.03. Le harness vaut `3df18bfa8ee05d92b07acca759e276d90f2be7d33eced1eec9c577f18744b47b`; l'image vaut `sha256:e3d96c187ca405790227e02aef1a66ca47df0820bb6b2a86b097359105956d58`. Les workflows locaux `cpu-release` et `sanitizer` réussissent chacun 31 tests sur 31. Cette qualification établit correction, réutilisation, empoisonnement fermé, absence de fuite et exécution mesurée du contexte chaud sur la cible mono-GPU G4.

## Artefacts cryptographiques

| artefact | SHA-256 | rôle |
|---|---|---|
| archive qualifiée Phase 2A, 1 229 596 octets | `8fab713397e0dd5fc8462529b6f0a008ffbffaa4b9d7bfc511c88ca02a13b270` | build, différentiels, sanitizer, premier certificat et checkpoint de reprise |
| résultat final Phase 2A | `ba6442a874a29422c16e0a7888e003a3a96947ae37511d29297d24f346958539` | fermeture matérielle de la première session |
| archive qualifiée combinée Phase 2B, 1 250 799 octets | `702e64cc2194c6e01d1faf24696e6fdee87752c1787b9a18db969380b86dd905` | deux certificats, agrégat et vérification finale |
| résultat final Phase 2B | `efdd176dc9e78c4d1cd0004b1e800a5ee7314f6a4935a3f1e40b387d7d6226b0` | qualification, lifecycle et arrêt ciblé |
| certificat Phase 2A versionné | `67ec58c698a5a4234426bc528d2a6bf94688b8970e3416107320d1421e6f4f42` | premier corpus |
| certificat Phase 2B versionné | `49f42753549f7ccb5278ff35cdf0717388e9017b1539e751d50b5f49366840db` | corpus indépendant |
| agrégat versionné | `98b40b566cefc63b3edf37ce98e1b13749c4b6f76b3f6c332a77fdf7898cfbda` | 23 760 000 signes et huit racines |
| worker chaud hors dépôt, 3 392 octets | `9cc5fb5673ff7dbeccd9c39fd931085a8178da09f8418163a1a7c9983c13550c` | mesure G4 brute, encore marquée en attente de l'arrêt ciblé |
| qualification chaude versionnée, 4 215 octets | `ac59ffa0a1fde5ca14a79a62bc8579571324ab2848b5c832617ddd0fb89fe1c6` | worker imbriqué, lifecycle et arrêt ciblé certifié |

Les quatre JSON de preuve sont versionnés sous `morsehgp3d/tests/campaigns/phase2b_predicates_v1/`; avec la présente revue de porte, ils forment cinq preuves de fermeture versionnées. Les archives complètes et le worker chaud brut restent hors dépôt; leurs hashes, tailles, contenu manifesté et résultats finaux permettent d'en contrôler toute copie conservée.

## Évaluation explicite de la porte

| critère | preuve | décision |
|---|---|---|
| zéro différence sur le corpus Phase 2A | 11 880 000 signes rejoués, audités et vérifiés | go |
| au moins $10^7$ signes supplémentaires | 11 880 000 signes avec graine et racine de corpus distinctes | go |
| tout `unknown` GPU transmis au CPU | 1 152 000 replis agrégés, tous décidés; `remaining_unknown=0` | go |
| aucun signe GPU connu non audité | 22 608 000 propositions FP64 recoupées avec oracle et CPU multiprécision | go |
| reprise et agrégat fail-closed | schéma 1.1, 110 chunks, verrou, hashes, racines, vérification RO | go |
| contexte résident sûr | décisions et compteurs identiques, affinité mono-GPU, poison fermé, sanitizer sans erreur ni fuite | go |
| mesure `warm_context_e2e` | un warmup, 31 échantillons bruts par prédicat, statistiques rejouables et compteurs exacts fermés | go |
| environnement matériel reproductible | NVCC 12.9.86, AOT `sm_120`, image et commits liés, RTX PRO 6000 G4 | go |
| fermeture GCP | trois sessions utiles et trois essais ou diagnostics arrêtés sur leur génération exacte; état final `TERMINATED` | go |

**Verdict final : porte de sortie Phase 2B satisfaite.** Toute future divergence, rupture de replay ou modification des noyaux certifiants rouvrira cette qualification et imposera une nouvelle campagne liée au nouveau commit.

## Limites et optimisations différées

- Les expansions restent sur CPU lorsque le filtre GPU retourne `unknown`; la porte exige leur transmission et leur décision exacte, pas un taux de repli arbitraire.
- L'annulation exacte de l'intersection des labels du bisecteur de puissance est une optimisation prometteuse, mais elle n'appartient pas au commit qualifié. La fusion ultérieure de cette optimisation imposera de rejouer les certificats GPU concernés.
- La qualification porte sur une cible G4 mono-GPU. La sélection initiale du device dans un processus multi-GPU devra recevoir un contrat séparé avant extension de cible.
- Aucun 1-NN, top-$k$, shell complet, rang spatial, LBVH, catalogue ou forêt n'est qualifié ici.
- G2, G3, G4 et les portes de performance restent ouvertes. `full_pi0` reste `conditional` et M.1 n'est pas promu.
- Aucun résultat scientifique public n'est produit; `public_status` demeure nul dans les artefacts.

## GCP

Toutes les sessions ont ciblé `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, une `g4-standard-48` étiquetée `project=e-hgp`, en `SPOT`, avec `instanceTerminationAction=STOP`, `maxRunDuration=3600` et arrêt invité borné à 45 ou 50 minutes selon le protocole.

- Essai préparatoire : démarrage `2026-07-17T16:09:53.498-07:00`, arrêt certifié `2026-07-17T16:11:59.572-07:00`. La compilation avait réussi; l'attestation Docker a échoué fermé sur une différence de casse, puis la cible a été arrêtée avant toute campagne.
- Campagne Phase 2A : démarrage `2026-07-17T16:15:19.559-07:00`, arrêt certifié `2026-07-17T16:46:21.747-07:00`.
- Campagne Phase 2B : démarrage `2026-07-17T16:47:58.724-07:00`, arrêt certifié `2026-07-17T17:18:25.306-07:00`.
- Diagnostics du transport de la garde invitée : générations démarrées à `2026-07-17T23:36:08.707-07:00` puis `2026-07-17T23:54:21.694-07:00`; chacune a échoué avant le benchmark, puis `stop_and_verify.sh` a certifié la cible exacte `TERMINATED`. Elles ne portent aucune preuve scientifique.
- Mesure chaude finale : démarrage `2026-07-18T00:25:40.606-07:00`, arrêt certifié `2026-07-18T00:31:09.515-07:00`; arrêt invité armé à 45 minutes.

Chaque clé OS Login éphémère a été révoquée, vérifiée absente puis supprimée. Après chaque session, aucune autre VM active portant `project=e-hgp` n'a été observée. La dernière relecture indépendante du 18 juillet 2026 à `07:34:06Z` confirme la cible exacte `TERMINATED` et l'absence de toute autre cible active du projet E-HGP.

## Suite de la feuille de route

La Phase 4 devient active, backend initial `reference_cpu`, profil `hgp_reduced`, mode `certified`. Son premier sous-jalon est un oracle spatial brute-force exact. Le contrat doit retourner tous les co-minimiseurs en 1-NN et, pour le top-$k$, l'ensemble strictement inférieur au seuil ainsi que le shell d'égalité complet. Un futur heap GPU ne pourra chercher que le seuil : un second passage devra collecter tous les ex æquo avant le filtre de rang. Le LBVH viendra seulement après égalité différentielle avec cette référence.
