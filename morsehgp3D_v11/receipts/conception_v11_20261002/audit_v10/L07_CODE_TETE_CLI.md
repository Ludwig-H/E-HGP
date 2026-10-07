# L07 — Code de la tête, des points et des CLI de `morsehgp3D_v10` : audit pour la conception de la v11

2 octobre 2026, rédigé entre 06 h 55 et 08 h 31 UTC (heures lues par `date -u`). Auditeur : lentille L07.

```text
phase=exploration_v11_hors_registre (audit de la v10)
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_code
public_status=not_claimed
GCP non utilisé
```

Sujet : `morsehgp3D_v10/src/head/`, `src/points/`, `cli/mhgp10_cluster.cpp`, `tests/head/`, `tests/points/`, et les reçus
`receipts/audit_continu_20260929/point_condensation_20260930/`, `point_condensation_cover_r2_20260930/` et
`receipts/head_point_dendrogram_20260929/`, lus dans `/workspaces/E-HGP/build/v11-worktree` (HEAD `52687f8e5` au début
de la lecture, `986f75799` à la fin ; `git diff afb081774 HEAD` est vide sur `src/`, `cli/`, `tests/`, `bench/`, `cmake/`
et `reference/` de la v10). Aucun fichier du dépôt ni des dossiers privés n'a été modifié ; tout a été construit et exécuté sous
`/tmp/v11-audit/l07_code_tete_cli/`.

## 0. Réponse courte

1. **Le défaut de condensation est réel, il se reproduit par le vrai binaire sur de vrais nuages 3D, et il tient en
   une phrase** : la tête publiée fait sortir chaque point attaché à son propre niveau d'entrée et ne teste
   `min_cluster_size` qu'aux divisions géométriques (`src/head/head.cpp:73` à 80 contre 81 à 103). Plus petit cas
   d'API : un nœud, deux points, stabilité 3/2 au lieu de 1. Plus petit renversement EOM d'API : quatre points, trois
   nœuds, mcs = 2. Plus petits renversements **géométriques**, obtenus par `mhgp10_cluster` sur des coordonnées
   entières : six points (K = 2, entrée `core`, mcs = 3, racine permise) et **neuf points, racine exclue, donc dans la
   configuration par défaut** : le binaire rend trois clusters, l'oracle par coupes strictes, scikit-learn sur la même
   ultramétrique et la tête réparée de l'auditeur indépendant en rendent deux. Les reçus de l'auditeur disaient cette
   réalisation géométrique « non établie » ; elle l'est maintenant.
2. **Son effet dépend de l'entrée des points et du rapport mcs / K.** Avec l'entrée `cover`, en position générale,
   aucun point n'entre en différé (0 sur 2 000 et 0 sur 39 885 mesurés) : le défaut ne se déclenche jamais (0 cas
   sur 15 504 configurations synthétiques et 66 configurations LiDAR). Avec l'entrée `core`, tous les points entrent
   en différé et le défaut se déclenche dans 83 à 100 % des configurations ; les étiquettes changent dans 0,1 % des
   cas à mcs = √n, 1,6 à 3,2 % à mcs = 15 et 19,6 à 43,2 % à mcs = 5.
3. **Les scores historiques ne sont pas renversés.** Aux configurations des lots préenregistrés (mcs = √n), rejouées
   sur l'espace `dev` : lot C (`cover`) 0 étiquette changée ; lots A et B (`core`) au plus 1 scène sur 128, écart
   moyen d'ARI au plus 0,0004 ; famille « objet » (témoin `mreach` à entrée bord) au plus 0,002. La marge de décision
   était 0,02. En revanche tout usage à petit mcs (le régime des instances LiDAR) est touché : sur la trame
   08/000100, K = 2, mcs = 5, z = 1, l'ARI entre la partition publiée et la partition corrigée vaut 0,51. Quand les
   étiquettes changent, la tête corrigée est meilleure contre la vérité du générateur dans 3 810 cas sur 4 000.
4. **La tête n'est pas un objet exact.** Niveaux en doubles non correctement arrondis, rangs exacts distincts
   confondus (reproduit sur trois sites), λ par `std::pow`, stabilités sommées en binary64, comparaison EOM en
   flottant : sur 943 égalités exactes construites, la tête retient le parent 855 fois et les enfants 88 fois.
   Aucun domaine numérique : `--z=nan`, `--z=0`, `--z=-1`, `--mcs=0` rendent `status ok`.
5. **Le CLI n'est pas une frontière.** Sur 71 invocations de frontière : 10 plantages par signal (8 exceptions non
   rattrapées, 2 lectures hors bornes dont un nuage de trois points à K = 5), 22 valeurs invalides acceptées avec
   code 0, 4 options sans effet acceptées en silence, 7 refus sans aucun message, des sorties partielles laissées
   après un refus, l'entrée écrasée si la sortie porte le même nom, un fichier de configurations vide qui rend `ok`
   sans rien écrire.
6. **Ce qui est solide** : le contrat `PointDendrogram` (arbre N-aire de composantes, points attachés par rang,
   plateaux atomiques), l'indépendance de l'ordre K au catalogue (appel groupé = appels séparés, étiquettes **et**
   arbre exporté, octet pour octet), l'identité 1 fil = 4 fils, l'absence d'entrée différée en `cover`, et le
   recoupement par quatre chemins indépendants de la sémantique « à cohortes » (400 nuages sur 400).
7. **Trois têtes plus avancées existent hors du dépôt** : le raccord privé R2 (domaine numérique, CLI strict, sorties
   tout ou rien, mais même condensation), la réparation de l'auditeur et la condensation Python des batteries du
   1er octobre (toutes deux à cohortes). La v11 ne doit porter aucune ligne de `src/head/head.cpp` : elle doit porter
   la **sémantique à cohortes de rang exact**, ses fixtures et son oracle.

## 1. Périmètre lu

| Objet | Lignes | Lecture |
| --- | ---: | --- |
| `src/head/head.hpp`, `src/head/head.cpp` | 47 + 172 | ligne à ligne, puis port ligne à ligne vérifié bit pour bit (§ 2) |
| `src/points/dendrogram.hpp`, `dendrogram.cpp` | 36 + 33 | ligne à ligne |
| `cli/mhgp10_cluster.cpp` | 243 | ligne à ligne, puis 71 invocations |
| `src/tower/tower.cpp` l. 996 à 1017, 1520 à 1632, 1753 à 1865 ; `tower.hpp` | — | attaches `core` et `cover`, `point_dendrogram` (le reste de la tour relève de L06) |
| `src/arith/geometry.cpp:20`, `src/core/types.hpp`, `status.hpp`, `reasons.def`, `src/sched/pool.cpp:13` à 17 | — | ce que la tête et le CLI en consomment |
| `tests/head/mreach.hpp`, `mreach.cpp`, `mreach_cluster.cpp`, `test_condensation_vs_sklearn.py` | 26 + 201 + 119 + 78 | ligne à ligne |
| `tests/points/test_cover_entry.py`, `test_cover_band.py`, `test_cover_band_native.py`, `test_dev_quotas.py` | 133 + 132 + 152 + 144 | ligne à ligne |
| `tests/regression/test_batch_equivalence.py`, `test_level_collision.py`, `test_mreach_border.py`, `test_multiplicity_refusal.py` | 80 + 60 + 66 + 57 | lus (ils jugent le CLI et la tête) |
| `bench/synthetic/methods.py`, `metrics.py`, `run_test.py`, `run_campaign.py`, `scenes.py` (en-tête), `prereg/*.json` | — | lus : qui appelle le CLI, avec quelles têtes |
| `bench/frontier/cover_band.py`, `dev_quotas.py`, `README.md` | 96 + 94 + 70 | lus |
| `CMakeLists.txt`, `cmake/gates.cmake`, `cmake/run_expect.cmake` | 135 + 25 + 26 | lus |
| `docs/SPEC_V10.md`, `docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md`, `docs/conception/CLUSTER_v2.md` (§ 5, § 6, § 11, § 12), `PASSATION.md`, `README.md` | — | lus pour confronter ce qui est écrit à ce qui est codé |
| Reçus `point_condensation_20260930`, `point_condensation_cover_r2_20260930`, `head_point_dendrogram_20260929`, `head_direct_exit_20261001`, `point_plateau_condensation_20260930`, `point_exact_rank_coalescence_20260930`, `head_numeric_corrected`, `interfaces_corrected`, `g4_session4_j2c_20260929` | — | lus comme pistes ; ce qui en est cité ici a été rejoué, sauf mention « lu » |
| Privé : `build/v10-integration-r2/notes/*.md`, `build/v10-audit-full-hierarchy-20261002/.../cohort_repair/`, `build/v10-tour-vers-points/code/condense_pr.py` | — | lus ; la tête de l'auditeur a été compilée et confrontée (§ 5.4) |
| Rapports voisins L03 (constat 03) et L06 (constats 02, 10) | — | lus pour ne pas doubler ; rien n'en est repris sans rejeu |

## 2. Méthode

1. **Construction.** `cmake -S <worktree>/morsehgp3D_v10 -B /tmp/v11-audit/l07_code_tete_cli/build`, Release, g++ 13.3,
   trois fils de compilation, 0 avertissement. `mhgp10_cluster` : sha256 `5df023bd…713d` ; `mhgp10_mreach_cluster` :
   `1dab9a4f…efac`. `src/head/head.cpp` : sha256 `f583da40…4578`, identique octet pour octet à l'instantané pris par
   l'auditeur au commit `8bb4618e5` (le défaut est donc toujours dans le dépôt).
2. **Port de contrôle.** `preuves_l07_code_tete_cli/sources/l07_heads.hpp` contient un port ligne à ligne de
   `condense` et de la sélection de `cluster`. La sonde `l07_probe.cpp` exige, à chaque appel, que ce port rende les
   mêmes stabilités **bit pour bit**, les mêmes parents, sorties, λ, clusters retenus et étiquettes que la
   bibliothèque. Sur toutes les exécutions de cet audit (53 580 configurations synthétiques, 132 LiDAR, plus de
   deux millions de couples petit nuage × configuration) : **0 écart** (`port_bad = 0`). Ce que je dis de la tête publiée
   repose donc sur une lecture vérifiée.
3. **Tête à cohortes (référence).** Même parcours et même ordre de sommation que la tête publiée, avec une seule
   différence : dans un nœud, dès qu'une cohorte (points de même rang d'entrée) laisse une masse strictement
   inférieure à mcs, tout ce qui reste sort à ce λ. Variante N : la même sur le dendrogramme **normalisé** (nœuds de
   même rang que leur parent contractés ; points entrant exactement au rang du parent remontés au parent), ce qui
   est la sémantique des coupes strictes.
4. **Oracle indépendant.** `oracle_condense.py` ne lit pas l'arbre comme la tête : il calcule l'ultramétrique des
   points en rangs entiers, $d(x,y)=\max(e(x),e(y),\rho(\mathrm{ppac}(x,y)))$, puis condense par coupes strictes sur
   des ensembles de points (Fraction pour z pair, Decimal à 80 chiffres sinon, refus des quasi-égalités).
5. **Recoupement par quatre chemins** sur 400 petits nuages réels (8 à 40 points, grilles de pas 6, 12, 40 et 1 000,
   K dans {1, 2, 3, 5}, `core` et `cover`, mcs dans {2, 3, 5}, z dans {1, 2}, racine permise ou non) : sonde N,
   oracle O, binaire lié à la tête réparée de l'auditeur indépendant A (empreinte `f79850f8…3d4b`), et
   `sklearn.cluster.HDBSCAN(metric='precomputed', min_samples=1)` S sur la même ultramétrique. **N = O et A = O dans
   400 cas sur 400.** S = O seulement là où aucun plateau n'est binarisé (§ 4, constat 05).
6. **Mesures d'impact.** Les 128 scènes `dev` de 2 000 points et 64 des 128 scènes `dev` de 8 000 points
   (`run_campaign.plan('dev', …)`, mêmes générateur, graines et quantification que le banc ; **aucune graine `test`
   ni `test_v10b`**), trois trames LiDAR du contrat. Vérité du générateur, ARI_s et remplissage borné du banc
   (`bench/synthetic/metrics.py`, `methods.bounded_fill`).
7. **CLI.** 71 invocations de frontière (`cli_probe.py`), chacune sous une limite d'adresse de 6 Go (garde de l'audit
   sur machine partagée).
8. **Portes locales** (la CI GitHub ne construit pas la v10). Les huit portes du périmètre passent, 8 sur 8 en 319 s,
   `CTEST_EXIT 0` : `mhgp10_cover_band_structural`, `mhgp10_dev_quotas`, `mhgp10_head_condensation_vs_sklearn`,
   `mhgp10_regression_level_collision`, `mhgp10_points_cover`, `mhgp10_regression_batch_equivalence`,
   `mhgp10_regression_mreach_border`, `mhgp10_regression_multiplicity_refusal`. `mhgp10_unit` et les deux portes
   d'oracle relèvent d'autres lentilles et n'ont pas été lancées ici ; `tests/points/test_cover_band_native.py` n'est
   pas enregistré dans CMake (il exige des fondations et un exporteur externes).
9. **Limites.** Machine partagée chargée (charge 12 à 28 sur 8 cœurs) : aucun temps mesuré ici n'est une mesure de
   performance ; seuls les comptes et les rapports le sont. Les temps cités comme contrat viennent des reçus G4
   (lus). K = 10 n'a été rejoué qu'à 2 000 points, K = 1 et 8 que sur 14 scènes ; les tailles 16 000 et 32 000 des
   lots n'ont pas été rejouées. K = 10 n'a pas été rejoué sur LiDAR. La tête de l'auditeur et scikit-learn n'ont été
   confrontés que sur de petits nuages (n ≤ 60).

## 3. La chaîne réelle, reconstituée depuis le code

Notations : $L_K(a)$ est l'ensemble des centres dont la boule fermée de rayon carré $a$ contient au moins $K$ sites ; un
**niveau** est un rayon carré ; un **rang** est l'indice d'un niveau dans une table croissante ;
$\lambda(a)=a^{-z/2}$, soit $\lambda=r^{-z}$.

### 3.1 Attaches des points à la tour (`src/tower/tower.cpp:1520` à 1632)

- **`core`** (l. 1600 à 1627). Une requête des `kq` plus proches par site. À l'ordre K, le site x entre au niveau
  entier $D_K(x)$ (distance carrée à son K-ième plus proche, lui compris). Son nœud est l'ancêtre, au rang « nombre de
  niveaux du catalogue inférieurs ou égaux à $D_K(x)$ » (`RankIndex::at_most`, l. 1004 à 1017, exact), de la naissance
  atteinte par la descente de ses K plus proches. **Tous les points entrent donc à l'intérieur de la vie d'un nœud
  déjà créé** : 2 000 sur 2 000 sur une scène `dev` à K = 3, 39 863 sur 39 885 sur la trame 08/000000 à K = 5
  (mesuré sur l'export `--tree`).
- **`cover`** (l. 1541 à 1599). La « première boule couvrante » de x est la boule du catalogue de plus petit indice
  (ordre canonique, donc de plus petit niveau) dont la boule fermée contient x et pèse au moins K (l. 1550, 1561 à
  1572). x entre au niveau de cette boule (`point_cat_rank`, l. 1597), dans la composante qui contient son centre
  (`cover_node`, l. 1552 à 1560 : descente d'une K-partie quelconque de la boule, puis ancêtre au rang de la boule).
  En position générale cette boule contient exactement K sites (sinon on retire un site du support autre que x et la
  boule englobante rétrécit) : c'est une naissance d'ordre K, et x est attaché **à la feuille, au rang de création**.
  Mesuré : 0 entrée différée sur 2 000 et sur 39 885 points, tous les points attachés à une feuille, au plus K par
  feuille. Hors position générale (cosphéricités des grilles), l'entrée peut être interne : cas à 8 points du § 5.2.
- **`coverE`**, E de 1 à 9 (`--entry=cover1`, `--cover-extra`). Boule de poids au moins K + E ; `cover1` attache au
  nœud de fusion créé par cette boule, au rang de création (0 entrée différée, 0 attache à une feuille sur la même
  scène).
- **Départage** : à niveau égal, l'indice de boule tranche (l. 1569), donc le rang de Morton (constat L06-02).
  Aucune porte ne juge le **nœud** d'attache de `cover` : `tests/points/test_cover_entry.py:88` à 97 ne compare que le
  niveau $\alpha_K(x)^{2}$ à une force brute.
- **Multiplicités** : refusées par la tour (`tower.cpp:1158`) ; le poids de chaque point vaut 1 (l. 1862).

### 3.2 Dendrogramme de points (`src/points/dendrogram.hpp`, `tower.cpp:1753` à 1865)

`PointDendrogram` : table `level` de **doubles** strictement croissants ; par nœud `node_rank`, `parent`, CSR des
enfants (fusions N-aires, enfants créés avant les parents) ; par point `point_node`, `point_rank`, `point_weight`.
Contrat écrit (`dendrogram.hpp:3` à 8) : `node_rank[v] <= point_rank[x] <= node_rank[parent(v)]`.

`point_dendrogram` fusionne les rangs du catalogue (tri par dénombrement) et les niveaux entiers $D_K(x)$ par
comparaison exacte, avec un raccourci flottant à $10^{-9}$ relatif (l. 1774, 1813). Puis il **publie des doubles** :
`Level::approx()` vaut `to_double(num) / to_double(den)` (`src/arith/geometry.cpp:20`, deux arrondis puis une
division, donc pas l'arrondi correct), et deux niveaux exacts distincts dont les doubles coïncident ou s'inversent
partagent un rang (l. 1828 à 1839). `validate` (`dendrogram.cpp:5` à 31) contrôle la CSR dans un seul sens.

### 3.3 Condensation à `min_cluster_size` (`src/head/head.cpp:18` à 105)

Masse d'un nœud = poids de tous les points de son sous-arbre, **y compris ceux qui entrent après sa création**
(l. 29 à 32). Parcours de la racine vers les feuilles, un cluster courant par nœud :

1. les points attachés au nœud sortent **chacun à son propre rang d'entrée**, sans test de masse (l. 73 à 80) ;
2. si le nœud a des enfants, ceux de masse au moins mcs sont « gros » (l. 84 à 85) : deux gros ou plus, scission
   (chacun ouvre un cluster né au λ du nœud, les petits sortent à ce λ) ; un seul, il continue le cluster ; aucun,
   tout sort au λ du nœud (l. 86 à 102).

C'est le point 1 qui s'écarte de HDBSCAN : dans la hiérarchie implicite des points, le départ d'un point est une
division comme une autre, et dès que la masse restante passe sous mcs tout doit sortir **au même λ**. L'en-tête le
dit pourtant lui-même (`head.hpp:7` à 8 : « Un point attaché directement sort à son propre niveau d'entrée ») tout en
se déclarant « condensation HDBSCAN exacte » (`head.hpp:1`) : c'est une erreur de spécification, pas une faute de
frappe.

### 3.4 Stabilité, EOM, feuilles (`head.cpp:11` à 14, 107 à 147)

$\lambda(a)=a^{-z/2}$ par `std::pow` ; niveau nul : $+\infty$ (l. 12). Racine née à λ = 0. Stabilité = somme en
binary64, dans l'ordre du parcours, des $w\,(\lambda_{\mathrm{sortie}}-\lambda_{\mathrm{naissance}})$ (l. 60, 79, 90).
EOM : un cluster sans enfant est retenu d'office (l. 122 à 124, **sans condition de masse**) ; sinon les enfants
l'emportent si `sub > stability` en flottant (l. 127), le parent sinon ; la racine est écartée sauf
`allow_single_cluster` (l. 125, 146 à 147, deux lignes équivalentes à une). Feuilles : clusters sans enfant (l. 144).

### 3.5 Étiquettes et vote de couverture (`head.cpp:148` à 168, `cli/mhgp10_cluster.cpp:181` à 207)

Étiquette de l'arbre : premier ancêtre retenu du cluster d'où le point est sorti. Vote (`--label=vote`, entrée
`cover` seulement) : pour chaque site, ses boules couvrantes par indice croissant ; première boule dont le nœud est
dans un cluster ayant un ancêtre retenu. Or la première boule couvrante est celle qui porte l'attache du point
(`tower.cpp:1584`) et `node_cluster[point_node[x]]` égale `point_cluster[x]` (`head.cpp:72` à 77, 55 à 58) : **le
premier candidat du vote est l'étiquette de l'arbre**. Le vote ne peut donc que remplir du bruit. Mesuré : 0
étiquette non bruit changée sur 36 configurations de 2 000 points et sur la trame 08/000000 ; 508 bruits remplis sur
5 576 (9 %), 353 sur 3 002 sur la trame. Le fichier `.vote` n'existe pas à K = 1 dans un appel multi-K.

### 3.6 Remplissage (Python, hors moteur)

`bench/synthetic/methods.py:87` à 116 : `fill_noise` (plus proche point classé) et `bounded_fill` (plus proche point
classé si la distance-cœur du bruit ne dépasse pas ρ fois le quantile 95 % de celles du cluster ; k = max(K, 5)),
en flottant, par `scipy.cKDTree`. Ce sont des post-traitements euclidiens, étrangers à la tour.

### 3.7 Quatre têtes, une seule dans le dépôt

| Tête | Où | Départs de points sous mcs | Arithmétique | État |
| --- | --- | --- | --- | --- |
| publiée | `src/head/head.cpp` (`f583da40…`) | non testés | binary64 | dans le dépôt, fautive |
| raccord R2 | `build/v10-integration-r2/` (30 commits, `final.patch` de 7,4 Mo) | non testés (« tranche T2 », reporté) | binary64, domaine numérique gardé, CLI strict, sorties tout ou rien | privé, jamais importé |
| réparation de l'auditeur | `build/v10-audit-full-hierarchy-20261002/.../cohort_repair/overlay/` (`f79850f8…`) | cohortes, plateaux contractés | binary64 | privé, proposée le 2 octobre |
| `condense_pr.py` | `build/v10-tour-vers-points/code/` | cohortes | Fraction (z pair) ou Decimal à 80 chiffres, quasi-égalités comptées | privé ; a produit les batteries du 1er octobre |

Les trois têtes hors dépôt ne sont pas dans `morsehgp3D_v10/` (vérifié par `find`) ; les reçus G4 du 1er octobre ne
contiennent que des résumés.

### 3.8 Ce que jugent réellement les portes du périmètre

| Porte (toutes vertes ici) | Ce qu'elle établit | Ce qu'elle n'établit pas |
| --- | --- | --- |
| `mhgp10_head_condensation_vs_sklearn` | 360 partitions égales à scikit-learn sur l'atteignabilité mutuelle, K = 1 et 2, mcs dans {5, 15, 40}, EOM et feuilles, z = 1 | entrées différées, plateaux, z ≠ 1, `--allow-single`, entrée de la tour |
| `mhgp10_points_cover` | $\alpha_K(x)^{2}$ égal à une force brute sur deux nuages de 14 points (K = 2, 3, 4) ; entrée `cover` jamais plus tardive que `core` ; `cover` = `core` à K = 1 ; 1 fil = 4 fils | le nœud d'attache ; le contenu du vote |
| `mhgp10_regression_batch_equivalence` | groupé = séparé et 1 fil = 4 fils sur les étiquettes ; même étiquette d'arbre avec et sans `--label=vote` | l'arbre exporté ; le fichier `.vote` |
| `mhgp10_regression_level_collision` | plus de refus `rank_order`, niveaux publiés strictement croissants (scène de 8 000 points, K = 8) | l'identité exacte des rangs (constat 04) |
| `mhgp10_regression_mreach_border` | témoin bord : K = 1 égal au cœur, aucun refus, couverture au moins égale dans 90 % des cas | la justesse de l'entrée bord (aucun oracle) |
| `mhgp10_regression_multiplicity_refusal` | raison et statut du refus des doublons | — |
| `mhgp10_cover_band_structural`, `mhgp10_dev_quotas` | deux modules Python de `bench/frontier` (bras expérimental d'attache par bande, quotas de scènes) | rien du produit C++ ; `test_cover_band_native.py` n'est pas enregistré |

Le bras par bande (`bench/frontier/cover_band.py:88` à 93) attache un point à l'ancêtre commun de ses témoins, à
la date `max(première couverture, naissance de cet ancêtre)` : il produit des entrées différées dans des nœuds
internes, exactement le cas que la tête publiée traite mal.

## 4. Constats

Gravité : **bloquant** = rend faux ou invalide un résultat ou un contrat ; **majeur** = à traiter dans la conception
de la v11 ; **mineur** ; **info**. Vérification : *lu*, *exécuté*, *mesuré*. Les preuves nommées sans chemin sont sous
`/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l07_code_tete_cli/` (`sources/`, `sorties/`).

### L07_CODE_TETE_CLI-01 — La condensation publiée n'est pas celle de HDBSCAN (bloquant)

- **Fait.** `condense` paie chaque point attaché à son propre λ d'entrée et ne teste `min_cluster_size` qu'aux
  divisions entre enfants géométriques. Un cluster qui ne tient plus que par moins de mcs unités continue donc
  d'accumuler de la stabilité. Seuls les clusters **feuilles** de l'arbre condensé sont touchés (si un enfant est gros,
  la masse ne passe pas sous mcs avant la division) : la structure de l'arbre condensé est juste, les stabilités des
  feuilles sont gonflées, et l'EOM penche vers les petits clusters.
- **Preuve.** Lecture : `src/head/head.cpp:73` à 80 (aucun test de masse) contre 81 à 103. Exécution, API
  (`sorties/api_tete_publiee.txt`) : un nœud, deux points, stabilité 1,5 au lieu de 1 ; quatre points, la tête retient
  A et B, la condensation exacte retient la racine ; six points, racine exclue, trois clusters au lieu de deux.
  Exécution, **vrai binaire sur vrais nuages** (`sorties/cas_reels.jsonl`, § 5.2) : cinq cas, dont neuf points en
  configuration par défaut ; dans les cinq, binaire ≠ oracle = scikit-learn. Recherche (`sorties/recherche_petits_nuages.txt`) :
  54 renversements (couples nuage × configuration) sur 20 000 nuages de six points, 5 renversements racine exclue sur
  100 000 nuages de neuf points. Structure (`sorties/structure_de_l_arbre_condense.txt`) : sur 36 000 cas en entrée
  `core`, parents, masses et naissances identiques bit pour bit dans les 36 000 ; 39 072 stabilités de feuilles
  changées, 0 stabilité de cluster interne ; aucun point ne change de cluster de sortie ; 126 975 points sortent à
  un λ plus petit, aucun à un λ plus grand.
- **Vérification.** Lu, exécuté ; sémantique de référence recoupée par quatre chemins (400 sur 400).
- **Conséquence v11.** Ne pas porter `head.cpp`. Définir la condensation sur les **cohortes de rang exact** (§ 11.2),
  graver les cas du § 5.2 comme fixtures à code exact, avec le mutant « sans seuil de cohorte ». Corriger les phrases
  « condensation exacte » (`README.md:51`, `docs/SPEC_V10.md:70`, `head.hpp:1`) dans tout document repris.

### L07_CODE_TETE_CLI-02 — Impact mesuré : nul en `cover`, négligeable aux configurations historiques, fort à petit mcs (majeur)

- **Fait.** Voir les tableaux du § 5.3. En `cover` et `cover1` : 0 déclenchement sur 15 504 configurations
  synthétiques et 66 LiDAR. En `core` : déclenchement dans 83 à 100 % des configurations ; étiquettes changées dans
  4 cas sur 3 072 (n = 2 000, mcs = 45) et 1 sur 1 152 (n = 8 000, mcs = 89), mais 19,6 % (n = 2 000) et 43,2 %
  (n = 8 000) à mcs = 5. Témoin `mreach` à entrée bord (famille « objet » du lot C) : 1,3 % et 0,2 % à mcs = √n,
  jusqu'à 60,8 % à mcs = 5. Aux têtes exactes des lots préenregistrés, écart moyen d'ARI_s : lot C 0 ; lots A et B au
  plus +0,0004 ; famille « objet » au plus +0,0020. LiDAR, `core` : 30 configurations sur 66 changent ; pire cas,
  trame 08/000100, K = 2, mcs = 5, z = 1 : 1 187 clusters retenus au lieu de 995, ARI entre les deux partitions 0,51.
  Sur 4 000 cas à étiquettes changées, la tête corrigée améliore l'ARI_s contre la vérité dans 3 810, le dégrade
  dans 33.
- **Preuve.** `sorties/analyse_dev_n2000_table.txt`, `analyse_dev_n8000_table.txt`, `analyse_lidar.txt`, et les cas
  changés (`*_cas_changes.jsonl.gz`) ; sources `l07_probe.cpp`, `run_dev.py`, `analyse_dev.py`.
- **Vérification.** Mesuré (espace `dev`, K ≤ 10 à 2 000 points, K ≤ 5 à 8 000 points et sur LiDAR).
- **Conséquence v11.** Les décisions des lots A, B et C ne sont pas renversées par ce défaut ; elles restent à rejouer
  avec la tête de la v11 avant d'être citées (tailles 16 000 et 32 000 non rejouées ; K = 8 et 10 seulement à 2 000
  points). Toute mesure à
  petit mcs faite avec le binaire publié en entrée `core` ou avec le témoin bord est à écarter. Les écarts « tour
  contre MR₂-bord » de +0,002 à +0,010 sont du même ordre que l'effet du défaut : n'en lire que la parité.

### L07_CODE_TETE_CLI-03 — La seule porte de la tête ne peut pas voir le défaut ; ce qui était conçu n'a pas été livré (majeur)

- **Fait.** `mhgp10_head_condensation_vs_sklearn` juge la tête sur le témoin d'atteignabilité mutuelle, où chaque
  site est sa propre feuille et entre au rang de création de cette feuille (`tests/head/mreach.cpp:128` à 130) : aucun
  départ différé n'y existe (0 déclenchement sur 10 260 configurations du témoin `mr1core`). Elle ne joue que K = 1 et
  2, mcs dans {5, 15, 40}, racine exclue (`test_condensation_vs_sklearn.py:55` à 63). Il n'existe dans le dépôt ni
  référence exacte de condensation (`reference/hgp10_ref.py` s'arrête aux partitions de coupe), ni fixture de tête,
  ni mutant (`--inject` absent du code), ni juge du vote (`test_batch_equivalence.py:42` relit l'étiquette de l'arbre,
  pas le fichier `.vote` ; `test_cover_entry.py:114` ne le hache que s'il existe), ni juge de `--allow-single`. La
  conception `docs/conception/CLUSTER_v2.md` prévoyait pourtant une référence N-aire exacte (`ref/condense_ref.py`),
  cinq portes G1 à G5, quatorze mutants (§ 12.3), une sommation de Neumaier (§ 5.4), un plancher de λ (§ 6), un format
  `mhgp10_condensed_tree_v1` avec validateur : `grep` ne trouve aucun de ces noms sous `src/`, `cli/`, `tests/`, `bench/`.
- **Preuve.** Fichiers cités ; `sorties/analyse_dev_*_table.txt` (lignes `mr1core`) ; `sorties/ctest_portes_l07.txt`
  (les 8 portes locales sont vertes, défaut présent).
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Une porte de tête se juge sur l'objet que le moteur produit réellement (entrées différées,
  plateaux), contre un oracle par coupes strictes, avec planchers de couverture (nombre de cohortes franchissant mcs)
  et mutants tués. Un document de conception dont les portes ne sont pas livrées doit le dire.

### L07_CODE_TETE_CLI-04 — La tête décide en flottant sur des niveaux arrondis (majeur)

- **Fait.** (a) La table `level` est en doubles obtenus par deux arrondis et une division : deux niveaux exacts
  distincts peuvent partager un rang. Reproduit sur trois sites (0,0,0), (261120,2,0), (1,512,0) à K = 2 : la tour
  porte 17045913601 pour la paire et 304677557929031781183651845/17873935364259844 pour le triangle (strictement plus
  grand, l'arrondi correct les sépare d'un ulp), l'export `--tree` leur donne le même rang. (b) λ vient de
  `std::pow`, les stabilités d'une somme binary64 non compensée, la décision EOM de `sub > stability` : sur 943
  égalités exactes construites (z = 2), la tête retient le parent 855 fois et les enfants 88 fois. (c) Aucun
  domaine : `lambda_of` rend $+\infty$ au niveau nul ; `--z=nan`, `inf`, `-1`, `0`, `400` et `--mcs=0` rendent
  `status ok`. (d) Les têtes Python « exactes » relisent ces doubles (`%.17g`) : elles sont exactes relativement à
  l'export, pas aux niveaux rationnels.
- **Preuve.** `tower.cpp:1828` à 1839, `geometry.cpp:20`, `head.cpp:11` à 14, 60, 79, 90, 127 ;
  `sorties/coalescence_trois_sites.txt` ; `sorties/api_tete_publiee.txt` (bloc 6) ; `sorties/cli_probe.jsonl` (A14,
  A19 à A23, A25) ; reçu `receipts/audit_continu_20260929/point_exact_rank_coalescence_20260930` (lu, puis rejoué par le
  CLI) ; `build/v10-tour-vers-points/code/condense_pr.py:19` à 24 (lu).
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** La hiérarchie de points se transmet en **rangs entiers et niveaux rationnels exacts** ; les
  doubles sont une vue. La condensation est entière. La sélection compare des stabilités par arithmétique exacte
  (z pair) ou par intervalles certifiés, et **compte** les quasi-égalités ; le domaine de z et de mcs est validé à
  la frontière.

### L07_CODE_TETE_CLI-05 — Sur une hiérarchie à fusions N-aires, « condenser comme HDBSCAN » n'est pas défini (majeur)

- **Fait.** Les fusions à trois enfants ou plus sont structurelles dans la tour : 37,4 % des fusions sur la trame
  08/000000 à K = 5 (70 244 ternaires, 17 767 quaternaires), 34,7 % sur une scène `dev` à K = 3. scikit-learn
  binarise chaque plateau selon l'ordre de son arbre couvrant : sur l'ultramétrique des points, il n'égale l'oracle
  par coupes strictes que dans 238 cas sur 600 à mcs = 2 (les paires de plus proches voisins mutuels entrent
  ensemble et forment de faux clusters de taille 2), 598 sur 600 à mcs = 3, 596 sur 600 à mcs = 5 ; les écarts
  restants sont une fusion ternaire dont la petite branche est rattachée par scikit-learn à l'une des deux grandes
  (cas 119 de `diag_case119.py` : la paire {36, 46}). La tête publiée, elle, n'est pas invariante quand un plateau
  est factorisé par un nœud de durée nulle (reçu `point_plateau_condensation_20260930`, lu). Sur les sorties du
  moteur mesurées ici, aucun nœud n'a le rang de son parent et aucun point n'entre au rang du parent (0 sur 29 412
  configurations du moteur et sur les trois trames) : la normalisation ne change rien en pratique, mais elle est
  nécessaire à la définition.
- **Preuve.** `sorties/multifusions.txt`, `sorties/sklearn_sur_ultrametrique.txt`, `sorties/recoupement_quatre_chemins.txt`
  (S = O dans 150 cas sur 187 sur grilles à égalités), `sources/diag_case119.py`, `CLUSTER_v2.md` § 5.2 et 5.3 (lu).
- **Vérification.** Mesuré, exécuté.
- **Conséquence v11.** La v11 **définit** sa condensation (plateaux atomiques, coupes strictes) et la prouve contre
  son oracle. scikit-learn ne sert d'oracle de tête que sur des entrées sans plateau dangereux (lemme des plateaux
  sûrs de `CLUSTER_v2.md` § 5.3, à porter avec son taux publié) ; il reste l'adversaire des bancs, appelé tel quel.

### L07_CODE_TETE_CLI-06 — Le CLI `mhgp10_cluster` n'est pas une frontière (majeur)

- **Fait.** 71 invocations (§ 7). Dix se terminent par un signal : huit exceptions de `std::stoi`, `stoull`, `stod`,
  `stoul` ou `bad_alloc` non rattrapées (`--k=abc`, `--k=`, `--k=99999999999`, `--mcs=abc`, `--z=abc`, `--threads=abc`,
  `--threads=-1`, `--k-list=2,,3`) et deux lectures hors bornes (`--k-list=0,2` ; trois points à K = 5 :
  `orders[kk - 1]` sans contrôle, l. 173). Vingt-deux valeurs invalides rendent `status ok` (`--k=3x` lu 3, `--mcs=1e3`
  lu 1, `--mcs=-1` lu $2^{64}-1$, `--z=1,5` lu 1, `--k=11` et `12` au-delà de `kMaxOrder = 10` qui n'est lu nulle
  part, `--threads=4294967297` lu 1…). Quatre options sont sans effet et acceptées (`--label=vote` et `--cover-extra`
  en `core`, `--k` avec `--k-list`, `--mcs` avec `--configs`). Sept refus n'écrivent rien, ni sur la sortie standard
  ni sur l'erreur. `--configs` n'est lu qu'après le catalogue ; vide, il rend `ok` sans écrire de fichier ; une ligne
  mal formée arrête la lecture en silence ; une sélection inconnue vaut `eom`. Les sorties sont écrites au fil de
  l'eau : un refus à K = 2 laisse le fichier de K = 1, `--tree=OUT` remplace les étiquettes par l'arbre, la sortie
  peut écraser l'entrée, `--entry=core,core` écrit deux fois le même fichier. Le champ `clusters` du JSON ne décrit
  que la dernière configuration ; `head_s` inclut les écritures.
- **Preuve.** `cli/mhgp10_cluster.cpp:31`, 47 à 55, 75, 92 à 97, 108 à 109, 133 à 151, 166 à 173, 198, 208 à 219,
  234 à 236 ; `src/sched/pool.cpp:15` ; `sorties/cli_probe.jsonl`.
- **Vérification.** Lu, exécuté (le cas `--threads=-1` sous une limite d'adresse de 6 Go posée par l'audit).
- **Conséquence v11.** Un analyseur strict unique (jeton entier consommé, bornes, options exclusives), tous les
  paramètres validés **avant** toute lecture et tout calcul, refus typé toujours écrit, jamais de signal, sorties
  déclarées puis publiées en tout ou rien, manifeste des fichiers produits. Le raccord privé R2 en donne une
  réalisation (lue, non rejouée) ; elle est à requalifier, pas à copier.

### L07_CODE_TETE_CLI-07 — `validate(PointDendrogram)` laisse passer des objets que la tête lit hors bornes (mineur)

- **Fait.** Acceptés : un point attaché à la racine avec `point_rank` hors de la table des niveaux (lecture hors
  bornes dans `condense`) ; un enfant listé deux fois (masse doublée) ; un nœud absent de la CSR de son parent (ses
  points sont rendus bruit, la masse de la racine vaut 2 pour 4 points) ; des niveaux négatifs.
- **Preuve.** `src/points/dendrogram.cpp:13` à 18, 24 à 29 ; `sorties/api_tete_publiee.txt` (bloc 5).
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Soit un validateur complet (bijection parent/enfant, bornes de tous les rangs, niveaux finis
  et positifs), soit un type non forgeable construit par le producteur. Fixtures d'invalidité à code exact.

### L07_CODE_TETE_CLI-08 — Règles de la racine non tenues (mineur)

- **Fait.** (a) Sous `--allow-single`, une racine de masse inférieure à mcs est retenue : trois points, mcs = 5,
  étiquettes 0 0 0 (API et CLI). (b) Quand la racine seule est retenue, la tête étiquette tous les points ;
  `sklearn.cluster.HDBSCAN(allow_single_cluster=True)` applique un seuil de λ et rend 124 à 280 bruits sur 306. (c)
  `head.cpp:146` à 147 : deux lignes pour une condition.
- **Preuve.** `head.cpp:122` à 125, 146 à 147 ; `sorties/api_tete_publiee.txt` (bloc 4) ; `sorties/cli_probe.jsonl`
  (B11, A50) ; `sorties/allow_single_contre_sklearn.txt`.
- **Vérification.** Exécuté.
- **Conséquence v11.** Racine exclue par défaut ; si l'option existe, sa règle est écrite (éligibilité à masse au
  moins mcs, étiquetage) et jugée. Ne pas annoncer « sémantique de scikit-learn » pour une option non comparée.

### L07_CODE_TETE_CLI-09 — Le vote de couverture est un remplissage du bruit, coûteux et non jugé (mineur)

- **Fait.** Par construction (§ 3.5) le vote ne modifie aucune étiquette non bruit ; mesuré : 0 sur 36 configurations
  et sur une trame, 9 à 12 % des bruits remplis. Il exige une résolution par boule couvrante (jusqu'au nombre de
  boules, 1,3 million sur la trame) au lieu d'une par première boule (au plus n) ; l'étage tour passe de 1,5 à 1,9 s
  à 2,5 à 3,0 s en local (trois fils, machine chargée : ordre de grandeur seulement). Aucun fichier `.vote` à K = 1.
- **Preuve.** `cli/mhgp10_cluster.cpp:165`, 181 à 207 ; `tower.cpp:1577` à 1584 ; `sorties/vote_remplit_seulement_le_bruit.txt`,
  `sorties/cout_du_vote_lidar00.txt`.
- **Vérification.** Lu, exécuté, mesuré.
- **Conséquence v11.** Pas de vote dans le chemin du contrat de temps. Un remplissage, s'il est voulu, est une étape
  nommée, avec sa définition et son juge.

### L07_CODE_TETE_CLI-10 — Coût de la tête : quadratique au pire, et quatre à huit fois le budget conçu pour un seul ordre (majeur)

- **Fait.** `cluster` remonte les ancêtres cluster par cluster et point par point (`head.cpp:135` à 142, 155 à 160,
  162 à 168). Sur un peigne (une scission par feuille de 5 points), 40 005 points : 160 millions de pas, 0,7 à 0,9 s ;
  80 005 points : 640 millions de pas, 2,3 à 3,7 s (×4 par doublement). Sur LiDAR la profondeur de l'arbre condensé
  atteint 200 (trame 08/000100, `cover`, K = 5, mcs = 5) et le parcours 19 pas par point (trame 08/000200,
  mcs = 10). Reçu G4 (lu) : `head_s` = 0,020 à 0,024 s pour **un** ordre et **une** tête, écritures comprises,
  séquentiel ; la conception annonçait 3 à 5 ms pour cinq ordres (`CLUSTER_v2.md` § 11.1).
- **Preuve.** `sorties/api_tete_publiee.txt` (bloc 7), `sorties/analyse_lidar.txt` (colonnes `prof`, `marche_pt`,
  `marche_cl`) ; `receipts/g4_session4_j2c_20260929/results` (lu).
- **Vérification.** Exécuté, mesuré (comptes déterministes) ; temps G4 lus.
- **Conséquence v11.** Sélection et étiquettes en une passe parents-avant-enfants, sans conteneur par cluster ;
  cohortes triées par nœud ; étage chronométré hors écritures ; budget de la hiérarchie de points inscrit dans le
  contrat de 100 ms (un ordre ou tous : question ouverte Q6).

### L07_CODE_TETE_CLI-11 — Les versions corrigées sont hors du dépôt (majeur)

- **Fait.** § 3.7. Le dépôt publie la tête fautive ; le raccord R2 (domaine numérique, CLI strict, sorties tout ou
  rien, 82 portes) est une série de 30 commits jamais importée, qui reporte lui-même la condensation par cohortes ;
  la tête à cohortes existe deux fois hors dépôt (C++ de l'auditeur, Python des batteries du 1er octobre).
- **Preuve.** `build/v10-integration-r2/notes/tete.md`, `notes/A_FAIRE_APRES_RACCORD.md` (« reporté, tranche T2 »),
  `notes/reparation.md` § 8 ; `cohort_repair/README.md` ; `condense_pr.py` (en-tête) ; `find` sur le worktree.
- **Vérification.** Lu ; la tête de l'auditeur compilée et confrontée (400 sur 400).
- **Conséquence v11.** Une seule tête, dans le dépôt, avec son oracle ; tout résultat cité doit être reproductible
  depuis le dépôt. Les acquis de R2 et de `condense_pr.py` se portent explicitement, avec provenance.

### L07_CODE_TETE_CLI-12 — L'export `--tree` est une interface de fait, sans contrat (mineur)

- **Fait.** Texte sans version : niveaux en `%.17g`, nœuds « rang parent », points « id nœud rang 1 » (poids codé
  en dur, l. 231), pas d'enfants, pas de niveaux exacts, pas d'empreinte. C'est pourtant par lui que les têtes de
  recherche Python lisent la hiérarchie. Le nom du fichier dépend de la forme de l'appel (`TREE`, `TREE.k3`,
  `TREE.cover.k3`).
- **Preuve.** `cli/mhgp10_cluster.cpp:221` à 233 ; `sorties/groupe_contre_separe.txt`.
- **Vérification.** Lu, exécuté.
- **Conséquence v11.** Un format versionné de la hiérarchie de points (rangs, niveaux rationnels réduits, forêt,
  attaches, empreinte canonique), produit par un seul chemin, lu par le produit et par la recherche.

### L07_CODE_TETE_CLI-13 — Le témoin `mreach` recalcule la hiérarchie d'HDBSCAN et hérite du défaut (mineur)

- **Fait.** `tests/head/mreach.cpp` construit l'atteignabilité mutuelle par un Prim en $O(n^{2})$ et sert, à entrée
  bord, d'adversaire dans la famille « objet » du lot C. À entrée bord les points entrent en différé : le défaut s'y
  déclenche dans 61 à 100 % des configurations (étiquettes changées jusqu'à 60,8 %). scikit-learn 1.9.1 expose son
  arbre de liaison simple (`_single_linkage_tree_`, champs `left_node`, `right_node`, `value`, `cluster_size`).
- **Preuve.** `tests/head/mreach.cpp:25` à 65, 178 à 197 ; `prereg/PREREG_V10_COVER_C_20260929.json` (méthodes `mrb_K*`) ;
  `sorties/analyse_dev_*_table.txt` (lignes `mr2border`).
- **Vérification.** Lu, mesuré.
- **Conséquence v11.** L'adversaire « même tête sur la hiérarchie d'HDBSCAN » s'obtient en important l'arbre de
  scikit-learn dans le format de hiérarchie de la v11, pas en le recalculant.

### L07_CODE_TETE_CLI-14 — Ce qui se reproduit exactement (info)

- **Fait.** (a) Appel groupé = appels séparés, pour les étiquettes **et** pour l'arbre exporté, octet pour octet,
  que le catalogue soit construit à l'ordre 3, 4 ou 5 (deux entrées). (b) Les huit portes du périmètre passent en
  local. (c) Le port de contrôle égale la bibliothèque bit pour bit partout. (d) Entrée `cover` : aucune entrée
  différée, au plus K points par feuille. (e) N = O = A sur 400 nuages. (f) Permuter l'ordre d'entrée des points
  permute les étiquettes sans rien changer d'autre, numérotation des clusters comprise (24 configurations, 0 écart).
- **Preuve.** `sorties/groupe_contre_separe.txt`, `sorties/ctest_portes_l07.txt`, `sorties/recoupement_quatre_chemins.txt`,
  `sorties/permutation_de_l_entree.txt`.
- **Vérification.** Exécuté.
- **Conséquence v11.** Ce sont les propriétés à porter comme portes (§ 8).

## 5. Le défaut de condensation : reproduction, plus petits cas, impact

### 5.1 Plus petits cas d'API (`sorties/api_tete_publiee.txt`)

Niveaux = rayons carrés ; z = 1, donc $\lambda=1/\sqrt{a}$ ; poids 1.

| Cas | Dendrogramme | Tête publiée | Condensation exacte |
| --- | --- | --- | --- |
| 1 nœud, 2 points, mcs = 2 | une composante née à 1 ; points entrés à 1 et 4 | stabilité 1/2 + 1 = 3/2 | après le départ à 4 il reste 1 < 2 : tout sort à λ = 1/2, stabilité 1 |
| 3 nœuds, 4 points, mcs = 2, racine permise | A née à 1, points à 1 et 4 ; B née à 16, deux points à 16 ; fusion à 25 | S(A) = 11/10, S(B) = 1/10, somme 6/5 > S(R) = 4/5 : **A et B** | S(A) = 3/5, somme 7/10 ≤ 4/5 : **R** |
| 5 nœuds, 6 points, mcs = 2, racine exclue | le même avec fusion de R à 20,25 ; C née à 100 (deux points) ; racine globale à 1600 | S(A) = 19/18, somme 10/9 > S(R) = 71/90 : **A, B, C** | S(A) = 5/9, somme 11/18 < 71/90 : **R, C** |

Le cas à quatre points est le plus petit renversement possible à mcs = 2 (deux clusters de deux points). Il affine
celui de l'auditeur (cinq points, `point_condensation_20260930/README.md`), relu et recoupé.

### 5.2 Plus petits cas géométriques, par le vrai binaire (`sorties/cas_reels.jsonl`, `sorties/arbres_condenses_des_cas_reels.txt`)

Commande : `mhgp10_cluster IN OUT --k=2 --mcs=3 --z=2 --entry=core --threads=1` (plus `--allow-single` pour le premier).

**Six points, racine permise** : (2,0,1), (2,0,4), (2,0,9), (9,10,5), (10,6,5), (11,5,6). Les deux premiers
entrent à 9 dans une naissance née à 9/4, le troisième à 25 dans leur fusion née à 16 ; l'autre groupe de même
(entrées 3, 3 et 17 ; fusion à 7,5) ; les deux groupes fusionnent à 29.

| Cluster | Stabilité publiée | Stabilité exacte |
| --- | ---: | ---: |
| racine (6 points, née à λ = 0) | 6/29 ≈ 0,2069 | 6/29 |
| A (3 points) | 0,0616 | 12/725 ≈ 0,0166 |
| B (3 points) | 0,2220 | 36/493 ≈ 0,0730 |
| A + B | 0,2836 > 0,2069 : **A, B** | 0,0896 < 0,2069 : **racine** |

Binaire : `0 0 0 1 1 1`. Oracle, scikit-learn (ultramétrique précalculée), tête de l'auditeur : `0 0 0 0 0 0`. À mcs = 3
il faut au moins six points pour deux clusters : c'est le minimum.

**Neuf points, racine exclue (configuration par défaut du CLI)** : (1,12,11), (3,3,4), (3,6,10), (3,8,12), (4,1,4),
(7,2,1), (9,6,13), (14,3,15), (15,3,12).

| Cluster | Stabilité publiée | Stabilité exacte |
| --- | ---: | ---: |
| P (6 points) | 0,1148 | 1584/13795 ≈ 0,1148 |
| enfant de P (points 0, 2, 3) | 0,0884 | 5/623 ≈ 0,0080 |
| enfant de P (points 1, 4, 5) | 0,0468 | 39/1691 ≈ 0,0231 |
| somme des enfants | 0,1352 > 0,1148 : **enfants** | 0,0311 < 0,1148 : **P** |
| Q (points 6, 7, 8) | 0,1224 | 9/5890 ≈ 0,0015 |

Binaire : `1 2 1 1 2 2 0 0 0` (trois clusters). Oracle, scikit-learn, tête de l'auditeur : deux clusters,
{0, …, 5} et {6, 7, 8}. La stabilité du petit cluster Q est gonflée d'un facteur 80. Neuf points est le minimum à
mcs = 3 avec racine exclue (trois clusters de trois points).

**Dix points, z = 1, racine exclue** et **six points, z = 1, racine permise** : mêmes renversements (fichier de
preuves). **Huit points, entrée `cover`, K = 3, grille de pas 6** : (1,2,3), (2,2,0), (2,3,5), (3,4,3), (3,4,4),
(4,2,0), (4,4,0), (5,4,1), mcs = 4, z = 2, racine permise ; le point (1,2,3) entre à 2,25 dans une composante née à
1,5 (entrée interne, cosphéricité) ; stabilité publiée 1,683 au lieu de 1,016 ; binaire deux clusters, références un
seul. En position quasi générale (grille de pas 1 000), 0 déclenchement en `cover` sur 20 000 nuages.

Pourquoi K = 2, mcs = 2 ne déclenche jamais en `core` (0 sur 60 000 nuages) : x entre à $D_2(x)$, la distance carrée
à son plus proche voisin y, et y est alors déjà entré dans la même composante ($D_2(y)\leq D_2(x)$, le segment xy est
couvert) ; la première cohorte d'une composante compte donc toujours deux points.

### 5.3 Impact mesuré

Espace `dev` du banc (8 familles × 4 niveaux × 2 bruits × 2 répliques), EOM, z dans {1, ẑ, 3, 4, 5, 6}, racine
exclue. « Déclenché » : au moins une cohorte fait passer un nœud sous mcs en laissant un reste. « Changées » : la
partition publiée diffère de la partition à cohortes. ΔARI_s : moyenne sur **toutes** les configurations de
(cohortes − publiée), contre la vérité du générateur, sans remplissage.

**2 000 points, 128 scènes, K dans {2, 3, 5, 10}** (3 072 configurations par ligne ; `cover1` 336, sur 14 scènes) :

| Source | mcs | Déclenché | Étiquettes changées | ΔARI_s moyen |
| --- | ---: | ---: | ---: | ---: |
| `core` | 5 | 100 % | 603 (19,6 %) | +0,0115 |
| `core` | 15 | 99 % | 48 (1,6 %) | +0,0010 |
| `core` | 45 = √n | 95 % | 4 (0,1 %) | −0,00005 |
| `cover` | 5, 15, 45 | 0 % | 0 | 0 |
| `cover1` | 5, 15, 45 | 0 % | 0 | 0 |
| témoin `mreach` α = 1, entrée cœur | 5, 15, 45 | 0 % | 0 | 0 |
| témoin `mreach` α = 2, entrée bord | 5 | 61 % | 998 (32,5 %) | +0,0062 |
| témoin `mreach` α = 2, entrée bord | 15 | 79 % | 567 (18,5 %) | +0,0127 |
| témoin `mreach` α = 2, entrée bord | 45 | 77 % | 40 (1,3 %) | +0,0003 |

**8 000 points, 64 scènes, K dans {2, 3, 5}** (1 152 configurations par ligne) :

| Source | mcs | Déclenché | Étiquettes changées | ΔARI_s moyen |
| --- | ---: | ---: | ---: | ---: |
| `core` | 5 | 100 % | 498 (43,2 %) | +0,0163 |
| `core` | 15 | 100 % | 37 (3,2 %) | +0,0017 |
| `core` | 89 = √n | 83 % | 1 (0,1 %) | +0,0000 |
| `cover` | 5, 15, 89 | 0 % | 0 | 0 |
| témoin `mreach` α = 2, entrée bord | 5 | 97 % | 700 (60,8 %) | +0,0066 |
| témoin `mreach` α = 2, entrée bord | 15 | 100 % | 501 (43,5 %) | +0,0230 |
| témoin `mreach` α = 2, entrée bord | 89 | 65 % | 2 (0,2 %) | +0,0001 |

Par configuration, l'effet peut être grand : `core`, K = 2, mcs = 5, z = ẑ, 8 000 points : 52 scènes sur 64 changent,
ΔARI_s moyen +0,076. Sur les 4 000 cas changés : ΔARI_s moyen +0,043 (2 000 points) et +0,032 (8 000 points),
extrêmes −0,62 et +0,79 ; 3 810 améliorés, 33 dégradés, 157 égaux.

Lecture : le défaut ne touche que les dernières unités de masse (moins de mcs) de chaque cluster feuille. Il pèse
donc quand les feuilles de l'arbre condensé sont petites devant mcs, c'est-à-dire à petit mcs, et s'efface quand
chaque feuille est un amas de plusieurs centaines de points (mcs = √n sur ces scènes à huit groupes).

**Aux têtes exactes des lots préenregistrés** (mcs = √n ; remplissage b2 entre parenthèses) :

| Lot | Tête | 2 000 points : scènes changées, ΔARI_s | 8 000 points : scènes changées, ΔARI_s |
| --- | --- | --- | --- |
| A | `core`, EOM, z = ẑ, K = 2 | 1 sur 128, +0,0004 (+0,0002) | 0 sur 64 |
| A | `core`, EOM, z = ẑ, K = 3 | 0 | 0 |
| B | `core`, EOM, z = ẑ, K = 5 | 0 | 0 |
| B | `core`, feuilles, K = 8 et 10 | sans objet : la sélection par feuilles ne lit pas les stabilités | — |
| C | `cover`, EOM, z de 3 à 6 | 0 (K = 2, 3, 5, 10 ; K = 1 et 8 sur 14 scènes) | 0 (K = 2, 3, 5) |
| « objet » | bord, z = 4, K = 2 | 0 | 0 |
| « objet » | bord, z = 5, K = 3 | 5 sur 128, +0,0020 (+0,0017) | 1 sur 64, +0,0005 (−0,0007) |
| « objet » | bord, z = 6, K = 5 | 1 sur 128, +0,0004 (+0,0003) | 1 sur 64, +0,0014 (+0,0010) |
| « objet » | bord, z = 6, K = 10 | 3 sur 128, +0,0006 (+0,0004) | non rejoué |

La marge de décision des lots était 0,02 ; les écarts publiés des lots A et C vont de +0,031 à +0,092. **Ce défaut
ne renverse donc aucune décision historique.** Il n'établit pas pour autant ces décisions : elles restent celles
d'une tête qui n'est plus la bonne, à rejouer avec la tête de la v11.

**LiDAR** (trames 08/000000, 08/000100, 08/000200 sans sol ; K = 2 et 5 ; mcs dans {5, 10, 20, 50, 200} ; z = 1 et
3 ; pas de vérité : on mesure l'écart entre les deux têtes) :

| Entrée | mcs | Configurations EOM | Déclenché | Partition changée | Plus petit ARI entre publiée et cohortes |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cover` | 5 à 200 | 60 | 0 | 0 | 1 |
| `core` | 5 | 12 | 12 | 12 | 0,514 |
| `core` | 10 | 12 | 12 | 11 | 0,963 |
| `core` | 20 | 12 | 12 | 6 | 0,997 |
| `core` | 50 | 12 | 12 | 1 | 0,998 |
| `core` | 200 | 12 | 12 | 0 | 1 |

Sélection par feuilles (6 configurations par entrée) : aucune partition changée, comme attendu. Le facteur de
gonflement de la stabilité d'un cluster feuille atteint 3 177 (trame 08/000200, `core`, K = 5, mcs = 5, z = 3).

### 5.4 La sémantique de référence est-elle la bonne ? Recoupement

| Chemin | Nature | Accord avec l'oracle O |
| --- | --- | --- |
| N : `l07_heads.hpp`, cohortes sur dendrogramme normalisé | C++, parcours de l'arbre | 400 sur 400 |
| A : tête réparée de l'auditeur indépendant (`f79850f8…`), liée au CLI | C++, écrit par un autre | 400 sur 400 |
| S : scikit-learn sur l'ultramétrique, racine exclue | bibliothèque | 150 sur 187 sur grilles à égalités ; 598 et 596 sur 600 en position générale à mcs = 3 et 5 ; 238 sur 600 à mcs = 2 (plateaux binarisés, constat 05) |
| V : binaire publié | — | 398 sur 400 dans ce tirage (K = 1 et `cover` y dominent) ; 559 sur 600 à K = 2, `core`, mcs = 3 |

## 6. Arithmétique de la tête

| Grandeur | Type | Exactitude |
| --- | --- | --- |
| masses, test `masse >= mcs`, nombre de gros enfants | `u64` | exact |
| rangs des nœuds et des points | `u32` | exacts **relativement à la table de doubles** ; deux niveaux rationnels distincts peuvent partager un rang (trois sites, § 4 constat 04) |
| niveaux | `double` = `to_double(num) / to_double(den)` | non correctement arrondi ; sur le témoin à trois sites l'arrondi correct sépare les deux niveaux, `approx()` non |
| λ | `std::pow(level, -0.5 * z)` | à un ulp près selon la libm ; $+\infty$ au niveau nul |
| stabilité | somme `+=` de produits `w * (lam - birth)` dans l'ordre du parcours | non compensée ; l'ordre dépend de la numérotation des nœuds |
| décision EOM | `sub > t.stability[c]` | flottante, sans marge ; égalité exacte tranchée par l'arrondi |
| étiquettes | entiers | exactes une fois la sélection faite |

Expérience des égalités (`l07_api.cpp`, bloc 6) : deux feuilles de $m_A$ et $m_B$ points aux niveaux entiers a et b,
fusion au niveau s, z = 2, racine permise ; l'égalité $S_A+S_B=S_R$ équivaut à $(m_A b+m_B a)\,s=2(m_A+m_B)\,a\,b$. Sur
les 943 solutions avec a ≤ b ≤ 40, s ≤ 80, masses de 2 à 5, la règle écrite (égalité : le parent) est suivie 855 fois
et violée 88 fois ; par exemple a = 2, b = 5, s = 6, $m_A=4$, $m_B=5$ : $S_R=1{,}5$, $S_A+S_B$ calculé
1,3333333333333335 + 0,16666666666666677.

Il n'y a pas de `Decimal` dans le dépôt. Il n'apparaît que dans la tête Python privée `condense_pr.py` : Fraction
quand λ est rationnel (z pair ou niveaux carrés), Decimal à 80 chiffres sinon, avec comptage des quasi-égalités à
$10^{-60}$ relatif. Elle part des doubles de l'export : son exactitude est celle d'une fonction exacte d'entrées
arrondies. Son en-tête dit avoir dû corriger, le 1er octobre, des différences et des sommes faites dans le contexte
Decimal global à 28 chiffres au lieu de 80 (lu, non rejoué).

Ce que l'arithmétique exacte permettrait : pour z pair, $\lambda=a^{-z/2}$ est rationnel et toute comparaison de
stabilités se fait en entiers ; pour z impair, les stabilités sont des combinaisons rationnelles de racines
carrées, comparables par intervalles à précision croissante, avec refus ou règle déclarée quand l'intervalle ne
tranche pas.

## 7. CLI `mhgp10_cluster`

### 7.1 Analyse des arguments (71 invocations, `sorties/cli_probe.jsonl`)

| Classe | Nombre | Cas |
| --- | ---: | --- |
| Terminaison par signal | 10 | SIGABRT : `--k=abc`, `--k=`, `--k=99999999999`, `--mcs=abc`, `--z=abc`, `--threads=abc`, `--threads=-1`, `--k-list=2,,3` ; SIGSEGV : `--k-list=0,2`, trois points avec `--k=5` |
| Valeur invalide acceptée, code 0 | 22 | `--k=3x` ; `--k=11`, `--k=12` ; `--mcs=0`, `1`, `-1`, `1e3` ; `--z=nan`, `inf`, `-1`, `0`, `1,5`, `400` ; `--threads=4294967297` ; `--entry=cover0` ; `--cover-extra=-3` ; `--k-list=` ; `--configs` vide, à ligne mal formée, à sélection inconnue, à `-1 nan eom 7` ; fichier d'entrée à 5 octets en trop |
| Option sans effet, acceptée en silence | 4 | `--cover-extra` et `--label=vote` en `core` ; `--k` avec `--k-list` ; `--mcs` avec `--configs` |
| Collision de sorties non détectée | 4 | `--entry=core,core` ; `--k-list=3,3` ; `--tree=OUT` (l'arbre remplace les étiquettes) ; sortie = entrée (l'entrée de 480 octets devient 160 octets d'étiquettes) |
| Refus muet (code 2, rien sur aucune sortie) | 7 | aucun argument ; entrée absente ; `--configs` absent (après le catalogue) ; fichier vide ; coordonnée $2^{18}$ ; dossier de sortie absent (après tout le calcul) ; dossier de `--tree` absent |
| Sortie partielle après un refus | 2 (dont 1 déjà compté parmi les refus muets) | 30 sites cosphériques, `--k-list=1,2,5` : refus `shell_quotient_budget` à K = 2, `OUT.k1.0` reste ; `--tree` impossible : `OUT` reste |
| Refus typés corrects | 11 | JSON : `--k=0`, `-1`, `13` et `--entry=cover9` (`kmax_out_of_range`), position dupliquée (`multiplicity_unsupported`) ; message sur l'erreur standard : option inconnue, options avant les chemins, `--selection=EOM`, `--entry=` vide, `--entry=Cover`, `--label=foo` |
| Conformes ou limites acceptés | 12 | témoins conformes (7, dont l'option répétée : la dernière gagne) ; un point à K = 1 (`ok`, 0 cluster ; 1 cluster avec `--allow-single`) ; trois points à K = 3 ; trois points, mcs = 5, `--allow-single` : 1 cluster ; grille 3 × 3 × 3 à K = 1 à 5 : `ok` |

### 7.2 Formats et codes

- **Entrée** : triplets `u32` dans l'endianité de la machine, sans en-tête ; les octets en trop sont ignorés.
- **Étiquettes** : `i32` par point d'entrée, dans l'ordre d'entrée, −1 = bruit ; pas d'en-tête, pas de version, pas
  d'empreinte. Le site rend son étiquette à tous ses `PointId` (`cli/mhgp10_cluster.cpp:213` à 214).
- **Noms** : `OUT` pour un appel simple ; `OUT.k<K>.<i>` dès qu'il y a `--k-list` à plusieurs ordres ou `--configs`
  (même à une ligne) ; `OUT.<entrée>.k<K>.<i>` avec plusieurs entrées ; suffixe `.vote`. `--k-list=3` seul écrit `OUT`.
- **Compte rendu** : une ligne JSON sur la sortie standard ; `clusters` = dernière configuration du dernier ordre ;
  `catalogue_s` inclut la préparation du nuage et la création du pool, `head_s` les écritures.
- **Codes** : documentés 0, 2, 3 (`cli/mhgp10_cluster.cpp:18`) ; observés aussi les signaux 6 et 11. Le code 3 n'a pas
  été atteint.

### 7.3 Équivalence appel groupé / appels séparés

La porte `mhgp10_regression_batch_equivalence` (135 s ici, verte) compare les étiquettes de 2 scènes × 3 entrées ×
5 ordres × 2 têtes, à 1 et 4 fils. Complément de cet audit : sur une scène de 2 000 points, les étiquettes **et**
l'export `--tree` de l'ordre 3 sont identiques octet pour octet entre l'appel séparé (catalogue à l'ordre 3),
`--k-list=3,5` (ordre 5) et `--entry=<e>,cover1` (ordre 4), pour `core` et `cover`
(`sorties/groupe_contre_separe.txt`). L'ordre K de la tour ne dépend donc pas de l'ordre maximal du catalogue. Ce qui
n'est pas équivalent : les noms des fichiers et le champ `clusters`.

## 8. Ce qui est solide et mérite un port explicite en v11

1. **Le contrat `PointDendrogram`** (`src/points/dendrogram.hpp:1` à 29) : arbre N-aire enraciné de composantes,
   plateaux atomiques jamais binarisés, points attachés par (nœud, rang d'entrée, poids), enfants avant parents. La
   tête ne connaît pas le producteur : la même tête lit la tour ou une hiérarchie adverse. À porter avec trois
   changements : rangs et niveaux exacts, forme normale (§ 11.1), validateur complet.
2. **L'indépendance de l'ordre K** au catalogue et aux fils : groupé = séparé pour les étiquettes et pour l'arbre,
   1 fil = 4 fils (portes `mhgp10_regression_batch_equivalence`, `mhgp10_points_cover`, vertes ici). À porter comme
   portes, en y ajoutant la comparaison de la hiérarchie exportée.
3. **Le lemme de l'entrée `cover`** : en position générale la première boule couvrante est une naissance d'ordre K,
   donc aucun point n'entre en différé (mesuré : 0 sur 39 885). Conséquence utile au contrat de temps : la
   condensation y est une passe de masses puis une passe de divisions, sans tri de cohortes. À porter comme énoncé
   prouvé, avec le compteur d'entrées différées publié (il vaut 0 en position générale, il est positif sur les
   grilles : fixture à 8 points).
4. **La sémantique à cohortes de rang exact**, déjà écrite trois fois hors dépôt et recoupée ici par quatre chemins :
   contraction des nœuds de même rang, remontée au parent des entrées au rang du parent, cohortes atomiques,
   fermeture dès que la masse restante est strictement inférieure à mcs, masse égale à mcs conservée.
5. **L'oracle par coupes strictes** sur l'ultramétrique des points (`sources/oracle_condense.py`, indépendant de la
   forme de l'arbre) et les **fixtures** du § 5 : un nœud deux points ; quatre points ; six points d'API racine
   exclue ; nuages réels de six, neuf, dix points ; huit points en `cover` dégénéré ; trois sites à niveaux
   confondus ; le plateau factorisé de l'auditeur (`point_plateau_condensation_20260930`).
6. **Les règles de sélection qui sont justes** : racine exclue par défaut, égalité au parent, feuilles = clusters sans
   enfant, étiquette = premier ancêtre retenu du cluster de sortie.
7. **La restitution par site** : étiquettes rendues dans l'ordre d'entrée par la table site → `PointId`
   (`cli/mhgp10_cluster.cpp:212` à 214), numérotation des sites par Morton, donc invariante par permutation de
   l'entrée (vérifié : 24 configurations, 0 écart).
8. **Les statuts et les portes à code exact** : `Outcome`, `Result`, `reasons.def`, `MHGP10_CHECK`,
   `cmake/run_expect.cmake` (signal refusé, code précis, ligne attendue dans la même exécution), refus JSON
   `{"status","reason","k"}`.
9. **Le protocole du banc** : scènes à graines dérivées par espace (`dev`, `test`), ARI_s (bruit en singletons),
   remplissage borné déclaré comme post-traitement, refus comptés à 0, HDBSCAN appelé tel quel.
10. **Le tri par dénombrement des rangs** de `point_dendrogram` (reçu `head_point_dendrogram_20260929` : ×17 à K = 5,
    18 différentiels identiques, lu) : l'idée se porte, sur des rangs exacts.
11. **Du raccord privé R2** (lu, non rejoué) : l'analyseur strict unique, `OutputSet` tout ou rien, le lecteur unique
    de `--configs`, le domaine numérique conjoint de la tête, les campagnes de mutants de frontière. À porter comme
    idées requalifiées, pas comme code.

## 9. Ce qu'il ne faut pas refaire

1. Écrire « condensation exacte » sans oracle sur l'objet réellement produit, et juger la tête sur un témoin dont la
   structure évite le cas difficile.
2. Rendre la hiérarchie en doubles à l'entrée de la tête, puis décider des égalités sur ces doubles.
3. Décider une sélection en flottant sans marge ni compte des quasi-égalités ; laisser λ valoir $+\infty$.
4. Un CLI à `std::stoi` nus : jetons à demi lus, exceptions non rattrapées, options sans effet acceptées, `kMaxOrder`
   déclaré et jamais lu, indexation `orders[k - 1]` sans contrôle.
5. Écrire les sorties au fil de l'eau, avec des noms qui dépendent de la forme de l'appel, sans manifeste ; rendre
   `ok` quand rien n'a été demandé ; refuser sans rien dire.
6. Lire les paramètres (`--configs`) après le calcul coûteux.
7. Chronométrer un étage avec ses écritures de fichiers (`head_s`) ; publier dans le JSON un champ qui ne décrit que
   le dernier cas (`clusters`).
8. Concevoir une référence, cinq portes et quatorze mutants, puis livrer une porte sans le dire.
9. Laisser les correctifs dans un dépôt jetable et les têtes de recherche hors dépôt pendant que le dépôt publie la
   version fautive ; citer des résultats produits par du code qui n'y est pas.
10. Mettre dans le chemin du contrat de temps une option (le vote) qui recalcule une résolution par boule pour
    remplir 9 % du bruit, sans définition ni juge.
11. Remonter les ancêtres cluster par cluster et point par point ; un `std::vector` par cluster.
12. Recalculer en C++ la hiérarchie d'un adversaire (témoin `mreach`) pour un banc.
13. Un validateur partiel présenté comme la frontière entre producteur et tête.
14. Une option (`--allow-single`) annoncée « comme scikit-learn » et jamais comparée.

## 10. Questions ouvertes

- **Q1. Définition aux égalités.** Coupes strictes, plateaux atomiques, masse égale à mcs conservée, point entrant au
  rang d'une fusion compté hors de l'enfant : est-ce bien la définition voulue ? Elle est canonique et c'est celle des
  trois têtes réparées ; scikit-learn n'en est pas une réalisation (constat 05).
- **Q2. Exposant impair.** z = 1 (HDBSCAN) et z = 3 rendent les stabilités irrationnelles. Intervalles certifiés et
  règle déclarée en cas d'indécision, ou sélection de production restreinte à z pair ?
- **Q3. Quelle entrée pour le produit ?** `cover` ne produit aucune entrée différée en position générale mais dépend
  d'un départage par indice (L06-02, L03-04) ; `core` et les bandes en produisent partout. La tête doit être juste
  dans tous les cas ; le choix relève de L03.
- **Q4. Multiplicités.** La tête sait compter des poids, la tour refuse les doublons. Sémantique pondérée de bout en
  bout, ou dédoublonnage déclaré à l'entrée avec restitution par site ?
- **Q5. Racine.** `allow_single_cluster` doit-il exister ? Si oui : éligibilité à masse au moins mcs, et quelle règle
  d'étiquetage (celle de scikit-learn rend jusqu'à 92 % de bruit sur un amas unique) ?
- **Q6. Contrat de temps.** La hiérarchie de points entre-t-elle dans les 100 ms, pour un ordre ou pour tous ? Sur G4
  la tête publiée prend 20 à 24 ms par ordre, en séquentiel.
- **Q7. Remplissage et vote.** Produit ou recherche ? Le vote publié n'ajoute que du remplissage ; le « vote sur la
  tour condensée » proposé par l'utilisateur est un autre objet (L03).
- **Q8. Rejeu des lots.** Les lots A, B et C se rejouent-ils avec la tête de la v11 sur un nouvel espace de graines
  préenregistré ? Cet audit n'a touché que l'espace `dev`.
- **Q9. Sujet différentiel.** Le binaire v10 figé plante sur dix invocations et porte la tête fautive : quelles
  sorties de la v10 la v11 prend-elle comme témoin (étiquettes `cover` oui, étiquettes `core` à petit mcs non) ?
- **Q10. Numérotation des étiquettes.** L'ordre de création des clusters n'est pas un invariant ; une numérotation
  canonique (par plus petit `PointId`, par exemple) rendrait les sorties comparables octet pour octet entre têtes.

## 11. Recommandations pour la v11 : ce que la tête doit être

### 11.1 Trois objets, trois contrats

| Objet | Contenu | Arithmétique | Où |
| --- | --- | --- | --- |
| **H. Hiérarchie de points** | table des niveaux rationnels réduits (le rang est l'indice) ; forêt (`rang`, `parent`, enfants) ; attaches (`nœud`, `rang d'entrée`, `poids`) ; provenance (K, règle d'entrée, règle d'égalité) ; compteurs (entrées différées, fusions à 3 enfants et plus) ; empreinte canonique | entiers et rationnels exacts | produit C++ |
| **C. Hiérarchie condensée** | fonction de (H, mcs) : par cluster `parent`, `rang de naissance`, `masse`, liste des sorties `(rang, masse)` ; par point `(cluster, rang de sortie)` | entiers seulement : ni λ, ni z | produit C++ |
| **S. Sélection** | fonction de (C, échelle λ, règle) : stabilités, clusters retenus, étiquettes par lignée ; nombre de quasi-égalités | exacte (z pair) ou intervalles certifiés ; jamais un `>` nu sur des doubles | une règle par défaut certifiée dans le produit ; toutes les variantes en recherche Python |

**Forme normale de H** (à garantir par le producteur, à contrôler par le validateur) : aucun nœud n'a le rang de son
parent ; pour tout point x attaché à v, `rang(v) <= entrée(x) < rang(parent(v))` ; chaque nœud non racine figure une
fois et une seule dans la liste de son parent ; les rangs sont denses. Deux encodages d'une même hiérarchie ont alors
la même forme, donc la même condensation.

### 11.2 Définition de la condensation (à inscrire au registre des preuves avant de coder)

Domaine : poids entiers positifs, $\mathrm{mcs}\geq 2$. Soit $e(x)$ le rang d'entrée du point x et $\rho(v)$ le rang du nœud v.

Ultramétrique des points, en rangs entiers : $d(x,y)=\max(e(x),e(y),\rho(\mathrm{ppac}(x,y)))$ pour $x\neq y$, où ppac est le plus petit ancêtre commun des nœuds d'attache.

Un cluster C (la racine contient tous les points) se scinde au rang $r=\max_{x\neq y\in C}d(x,y)$ ; ses classes sont
celles de la relation $d<r$ (coupe stricte) ; une classe est **grosse** si sa masse est au moins mcs. Deux grosses ou
plus : chacune ouvre un cluster né au rang r, et tous les autres points sortent de C au rang r. Une seule : elle
continue C, les autres sortent au rang r. Aucune : tous les points de C sortent au rang r.

Stabilité : $S(C)=\sum_{x\in C}w_x\,(\lambda(s_C(x))-\lambda_0(C))$, où $s_C(x)$ est le rang auquel x quitte C (sortie ou passage dans un enfant) et $\lambda_0(C)$ le λ du rang de naissance, nul pour la racine.

Sur l'arbre en forme normale cela se calcule sans matrice : dans un nœud, les points attachés partent par
**cohortes** de rang décroissant ; après chaque cohorte, si la masse restante est strictement inférieure à mcs, tout
le reste (cohortes suivantes et sous-arbres) sort au rang de cette cohorte ; sinon on atteint le rang du nœud, où
les enfants sont les classes. Coût : une passe de masses, un tri des cohortes par nœud, une passe de divisions.

### 11.3 Ce qui relève du produit C++ et ce qui relève de la recherche Python

| Produit C++ (exact, porté, sous portes) | Recherche Python (hors chemin produit, lit les exports) |
| --- | --- |
| attaches et hiérarchie H en forme normale, validateur complet, empreinte | prototypes de règles d'entrée (bandes, variantes de `cover`) contre la référence exacte |
| condensation C par cohortes, en entiers | sélections candidates : grille de z, feuilles, proéminence, règles multi-K, vote sur la tour condensée |
| une sélection par défaut à comparaisons certifiées, étiquettes par lignée | remplissages (`bounded_fill`), métriques, bancs, préenregistrements |
| formats versionnés de H, C et des étiquettes, manifeste des sorties | scikit-learn appelé tel quel ; import de `_single_linkage_tree_` au format H pour « même tête sur la hiérarchie d'HDBSCAN » |
| CLI strict (§ 11.5) | oracle par coupes strictes, juges d'échantillon, analyses |

Règle de passage : une règle de recherche n'entre dans le produit qu'avec une définition, un oracle borné, des
fixtures d'égalité et des mutants tués. Le produit ne contient aucune décision flottante ; la recherche ne recalcule
aucune géométrie.

### 11.4 Portes à prévoir dès l'ouverture de la tête

1. **Fixtures à code exact** : les neuf du § 8 point 5, plus les fixtures d'invalidité du validateur (constat 07) et
   la racine sous mcs (constat 08).
2. **Oracle borné** par coupes strictes (n ≤ 40, K ≤ 5, `core`, `cover`, grilles à égalités et position générale),
   avec planchers : nombre de cohortes franchissant mcs, de plateaux à trois classes ou plus, d'égalités de masse à
   mcs. Les 400 nuages de `cross_check.py` en donnent le gabarit.
3. **Mutants tués** : pas de seuil de cohorte ; `<` remplacé par `<=` à mcs ; pas de remontée des entrées au rang du
   parent ; plateau binarisé ; racine sous mcs éligible ; λ lu au rang du nœud au lieu du rang d'entrée.
4. **Invariance** : permutation des `PointId` et de l'ordre d'entrée ; 1 fil = N fils ; groupé = séparé sur H, C et
   les étiquettes ; isométries de la grille (avec la règle d'égalité de l'entrée).
5. **scikit-learn** : égalité exigée seulement sous le lemme des plateaux sûrs, taux de plateaux sûrs publié ;
   ailleurs, divergence déclarée et comptée.
6. **Coût** : compteurs déterministes (pas de remontée, comparaisons, cohortes triées) bornés linéairement, jugés sur
   un peigne (fixture du constat 10) et aux tailles 8 000, 16 000, 32 000 ; temps lus sur G4, hors écritures.

### 11.5 CLI

Un seul analyseur : chaque valeur lue en entier (aucun suffixe toléré), bornée, typée ; options exclusives
déclarées ; toute option sans effet dans la configuration demandée est un refus. Validation complète, y compris des
fichiers de configuration et de la possibilité d'écrire chaque sortie, **avant** la lecture du nuage. `n >= K` et
`K <= kMaxOrder` contrôlés. Refus : une ligne JSON `{"status","reason",…}` et un code, jamais un signal, jamais le
silence. Sorties : ensemble déclaré, conflits refusés (sortie = entrée, deux sorties de même nom), écriture en
temporaires puis publication tout ou rien, manifeste (nom, taille, empreinte) ; mêmes noms quelle que soit la forme
de l'appel. Compte rendu : un enregistrement par (entrée, K, tête), temps par étage hors écritures.

### 11.6 Ordre conseillé

1. Écrire la définition du § 11.2 et la forme normale au registre des preuves ; graver les fixtures.
2. Porter l'oracle par coupes strictes dans `reference/` (c'est lui qui juge, y compris la v10 corrigée).
3. Écrire H (format, validateur, empreinte) sur la sortie de la tour, puis C, puis la sélection certifiée.
4. Rejouer contre la v10 : égalité attendue des étiquettes en `cover` (0 écart mesuré ici), écart attendu et borné
   en `core` (tableaux du § 5.3) ; tout autre écart est un défaut.
5. Mesurer l'étage sur G4, puis seulement ouvrir les variantes de recherche.

## 12. Preuves et reproduction

Tout est sous `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l07_code_tete_cli/` (moins de 0,5 Mo). Les
calculs volumineux sont restés sous `/tmp/v11-audit/l07_code_tete_cli/` (jetable).

| Fichier | Contenu |
| --- | --- |
| `sources/l07_heads.hpp` | port vérifié de la tête publiée ; tête à cohortes ; normalisation ; compteurs de marche |
| `sources/l07_probe.cpp` | sonde : modes `run` (tour ou témoin `mreach`, cinq têtes comparées) et `search` (petits nuages) |
| `sources/l07_api.cpp`, `sources/l07_show.cpp`, `sources/l07_struct.cpp` | sondes d'API (blocs 1 à 7) ; arbres condensés des cas réels ; structure de l'arbre condensé sous les deux têtes |
| `sources/oracle_condense.py` | oracle par coupes strictes sur l'export `--tree` |
| `sources/check_cases.py`, `cross_check.py`, `cross_check_sklearn.py`, `diag_sklearn.py`, `diag_case119.py` | cas réels ; recoupement à quatre chemins ; régime où scikit-learn est un oracle |
| `sources/gen_dev.py`, `run_dev.py`, `analyse_dev.py`, `analyse_lidar.py` | scènes `dev`, rejeu, dépouillement |
| `sources/cli_probe.py`, `vote_check.py`, `allow_single_check.py` | batterie du CLI ; vote ; racine seule |
| `sorties/api_tete_publiee.txt` | constats 01, 04, 07, 08, 10 |
| `sorties/cas_reels.jsonl`, `arbres_condenses_des_cas_reels.txt`, `tete_auditeur_sur_cas_reels.txt`, `recherche_petits_nuages.txt`, `structure_de_l_arbre_condense.txt` | § 5.2, constat 01 |
| `sorties/analyse_dev_n2000_table.txt`, `analyse_dev_n8000_table.txt`, `*_cas_changes.jsonl.gz`, `analyse_lidar.txt` | § 5.3 |
| `sorties/recoupement_quatre_chemins.txt`, `sklearn_sur_ultrametrique.txt`, `multifusions.txt` | § 5.4, constat 05 |
| `sorties/cli_probe.jsonl`, `groupe_contre_separe.txt`, `permutation_de_l_entree.txt` | § 7, constat 14 |
| `sorties/coalescence_trois_sites.txt`, `vote_remplit_seulement_le_bruit.txt`, `cout_du_vote_lidar00.txt`, `allow_single_contre_sklearn.txt` | constats 04, 09, 08 |
| `sorties/ctest_portes_l07.txt`, `sorties/environnement.txt` | les 8 portes locales du périmètre : 8 sur 8, 319 s ; versions et empreintes |
| `SHA256SUMS` | empreintes des fichiers ci-dessus |

Reproduction (aucune écriture dans le dépôt ; `PYTHONDONTWRITEBYTECODE=1` partout) :

```bash
W=/tmp/v11-audit/l07_code_tete_cli ; S=/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10
cmake -S $S -B $W/build -DCMAKE_BUILD_TYPE=Release && cmake --build $W/build --parallel 3
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I$S/src -I$S/tests/head -Isources \
    sources/l07_probe.cpp $S/tests/head/mreach.cpp $W/build/libmhgp10_core.a -lpthread -o $W/probe/l07_probe
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I$S/src -Isources sources/l07_api.cpp \
    $W/build/libmhgp10_core.a -lpthread -o $W/probe/l07_api && $W/probe/l07_api
python3 -B sources/check_cases.py $W/build
$W/probe/l07_probe search --n=9 --grid=16 --dim=3 --k=2 --entry=core --trials=100000 --seed=21 --mcs=3 --z=1,2 --show=1
python3 -B sources/cli_probe.py $W/build $W/cli/work
```

Les scripts Python portent en dur le chemin `/tmp/v11-audit/l07_code_tete_cli/` (binaires et sonde) : le
reconstruire avant de les relancer.
