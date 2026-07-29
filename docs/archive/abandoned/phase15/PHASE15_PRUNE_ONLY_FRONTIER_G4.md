# Phase 15 — falsification G4 de la frontière `prune-only`

> [!NOTE]
> Rapport historique scellé. Le prototype reste rejeté et son code reste retiré. Depuis le pivot consigné dans [PHASE15_PROGRESS.md](../../../validation/PHASE15_PROGRESS.md), le prochain gate global est le catalogue GPU exact des paires de rang fermé $K+1$; les recommandations ci-dessous ne sont que des contraintes héritées pour une éventuelle frontière ultérieure.

## Verdict

Le prototype compact du commit `c047e2f79343b5ad4e6a1168a4d859fc99661857` est correct comme falsificateur de partition, mais rejeté comme structure industrielle. Il ferme exactement la partition des paires non ordonnées sur tous les runs conclusifs et chaque prune GPU est rejoué par le prédicat exact CPU. Malgré un taux de prune de 99,634 % à 3 125 points et $K=2$, le rejeu résident prend 3 228,989 ms. La règle d'arrêt interdit donc les profils 6 250, 12 500 et 50 000 sur ce chemin.

Le code expérimental est retiré du `main` après conservation de ce rapport et des artefacts. Il ne devient ni une API produit, ni une variante supplémentaire. Le contexte administratif reste Phase 15, `backend=reference_cpu/cuda_proposal`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only` et `public_status=not_claimed`.

## Contrat réellement testé

Le diagnostic part de la racine contre elle-même et subdivise la partition bloc--bloc du LBVH Morton. Une diagonale produit trois enfants et un produit croisé en produit deux. Un seul kernel stackless traite tous les produits d'une vague. En mode `prune_only`, le GPU n'émet que les sous-arbres témoins dont la borne de $phi$ est strictement négative; les terminaux non stricts ne sont jamais rapatriés. Seuls les produits explicitement arrêtés comme `pruned` peuvent devenir des propositions.

Chaque reçu est ensuite recertifié avec `exact_diametral_phi_aabb_maximum_sign < 0`. Les antichaînes de reçus doivent être disjointes et leur cardinal cumulé doit atteindre $K$. Tout non-prune, budget ou cap est seulement subdivisé; aucun certificat `keep` et aucun fallback exhaustif CPU par produit ne sont admis. Chaque vague vérifie la conservation de masse et la fin exige `pruned_pair_mass + unclassified_leaf_pairs` égal à $\binom{n}{2}$ avec un résidu nul. Les `unclassified_leaf_pairs` sont énumérées mais ne constituent pas un catalogue scientifique.

Cette fermeture prouve que le falsificateur ne perd ni ne duplique une paire et que ses prunes publiées sont sûres. Elle ne produit ni triangles Gabriel, ni incidences Gamma$_2$, ni hiérarchie Morse.

## Résultats G4 récents

Tous les temps ci-dessous sont les rejeux résidents après une passe froide séparée. La cible est une RTX PRO 6000 Blackwell, CUDA 12.9, cubin AOT `sm_120`.

| Famille | Points | $K$ | Paires | Masse prunée | Feuilles non classées | Vagues | Visites LBVH | Synchronisations | Temps résident |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `uniform_latin` | 14 | 1 | 91 | 72 | 19 | 9 | 2 295 | 15 | 4,605 ms |
| `uniform_latin` | 14 | 2 | 91 | 55 | 36 | 9 | 2 855 | 15 | 4,814 ms |
| `uniform_latin` | 14 | 10 | 91 | 2 | 89 | 9 | 4 040 | 11 | 4,810 ms |
| `uniform_latin` | 257 | 1 | 32 896 | 32 288 | 608 | 18 | 1 042 796 | 30 | 216,909 ms |
| `uniform_latin` | 257 | 2 | 32 896 | 31 918 | 978 | 18 | 1 272 396 | 29 | 226,146 ms |
| `uniform_latin` | 257 | 10 | 32 896 | 28 595 | 4 301 | 18 | 2 675 731 | 29 | 278,603 ms |
| `eight_clusters` | 257 | 1 | 32 896 | 32 162 | 734 | 20 | 434 891 | 36 | 162,864 ms |
| `eight_clusters` | 257 | 2 | 32 896 | 31 715 | 1 181 | 20 | 510 784 | 36 | 166,430 ms |
| `uniform_latin` | 3 125 | 2 | 4 881 250 | 4 863 364 | 17 886 | 27 | 110 675 038 | 47 | 3 228,989 ms |

Le run 3 125/$K=2$ traite 281 131 produits, rapatrie 11 245 240 octets de contrôles et 3 616 672 octets de reçus. Il ferme sans cap ni résidu et prune 99,634 % de la masse, mais son coût est déjà 129 fois supérieur à la règle d'arrêt de 25 ms prévue à 12 500 points. La puissance géométrique du prune n'est donc pas le problème principal : la répétition vague hôte--kernel--contrôles, les 47 synchronisations et le parcours complet de l'arbre témoin pour trop de produits dominent.

Compute Sanitizer sur `uniform_latin`, 257 points et $K=2$ reproduit exactement les 31 918 paires prunées et les 978 feuilles non classées, avec zéro erreur et zéro fuite. Le test fake séparé du commit expérimental couvre un produit pruné et un non-pruné, zéro `keep`, uniquement des reçus stricts et le rejet déterministe d'une capacité inférieure à $P K$.

## Conséquence historique pour la structure produit

Le surrogate brut reste exclu comme autorité : les campagnes PDEL massives ont déjà montré 122 188 retards à $k=1$ et 39 441 à $k=2$ sur 50 000 points, puis 74 289 378 et 22 560 254 sur 30 000 001 points. La règle empirique $M=\lceil5k\ln n\rceil$ couvre les trois rangs de voisinage observés sur cette unique famille, mais reste seulement une graine de propositions.

Pour l'échéance de composantes comparée à PDEL, la voie la plus simple est l'EMST exact : son théorème de bottleneck connecte les trois sommets de tout triangle Gabriel au niveau source ou avant. Elle répond donc au critère unilatéral `gabriel_fusion_deadline_v1` sans Delaunay, y compris pour les sources testées à $k=2$. Elle ne remplace toutefois pas le catalogue Gabriel demandé par la sortie $k=2$.

La séparation par certificat reste une contrainte : Borůvka dual-tree entièrement résident sur le LBVH partagé pour l'EMST exact $k=1$; préfixe voisin local propositionnel puis flux exacts `pair`, `higher` et `extra_shell` pour le catalogue $k=2$. Un éventuel successeur de cette expérience ne doit pas répéter la boucle de subdivision hôte. Il doit emprunter l'autorité hôte et l'arène device déjà certifiées, maintenir la frontière et ses vagues sur le GPU, ne rapatrier qu'un transcript final borné et conserver `frontier_empty` comme unique fermeture exacte. Le gate actuel du catalogue de paires peut réutiliser ces contraintes, mais pas cette ordonnance rejetée. Aucune seconde copie des nœuds, table $n\times M$, triangulation de Delaunay ou matrice globale de toutes les paires n'est admissible.

## Session GCP et artefacts

La session gardée ciblait `devpod-gpu-exploration / europe-west4-ai1a / ehgp-blackwell-spot-ai1a`, modèle `g4-standard-48` `SPOT`, action `STOP`, `maxRunDuration=3600 s` et arrêt invité à 45 minutes. La génération exacte `2026-07-28T10:46:22.174-07:00` a été arrêtée puis certifiée `TERMINATED`; aucune autre VM `project=e-hgp` n'était active. La clé OS Login de session a été révoquée et son répertoire local supprimé.

Les artefacts bruts sont `phase15_prune_only_*_g4_c047e2f.json`; `phase15_prune_only_git_sha_c047e2f.txt` fixe le SHA complet. Le transcript Compute Sanitizer est `phase15_prune_only_n257_k2_memcheck_g4_c047e2f.txt` et la liste de cubins `phase15_prune_only_cuobjdump_g4_c047e2f.txt`. Les JSON portent `qualification_claimed=false`, `slo_claimed=false` et `public_status=null`.
