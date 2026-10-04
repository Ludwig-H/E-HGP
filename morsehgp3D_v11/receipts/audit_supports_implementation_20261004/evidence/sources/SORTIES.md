# Sorties de `mhgp11` : exécutable, formats, manifeste et transaction de dossier

4 octobre 2026, 20 h 54 UTC (heure lue par `date -u`). Contrat produit de la tranche S0 de la sortie paramétrée
(livraison L0). Ce document est normatif pour l'exécutable `mhgp11`, ses refus, ses formats, son manifeste et sa
publication ; il ne qualifie aucune implémentation. Les objets mathématiques (arbre d'ordre K, boules d'événement,
rattachement, rôles, supports positifs minimaux, comptes, instantanés) et leurs preuves sont dans
[MATHEMATIQUES.md](MATHEMATIQUES.md), section 10 « Hiérarchie des supports d'ordre K » ; aucune preuve n'est recopiée
ici.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

**Sources.** Les décisions de l'utilisateur du 4 octobre 2026 priment : tout natif, tous les supports positifs
minimaux énumérés sur toute la coquille, toutes les boules d'événement d'ordre K, aucune population publiée, compte
`kparties_reliees`, livraison `supports` puis `points` puis `plat`. Viennent ensuite la critique et le plan révisé,
puis la spécification finale du workflow de conception `wf_a7dbdf1a-21c`. Ces trois pièces sont hors dépôt
(`build/v11-persist/sortie_supports/`) ; ce document en est la transcription normative dans le dépôt. Code lu : module
`io` à `f98aeed67` (tranche S4), en-tête parapluie `tower/tower.hpp` à `257aabb92` (tranche S2). L'audit de conception
`1bf4be68f` est une contrainte : [section « Sortie supports »](../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md),
[reçu audit_supports_20261004](../receipts/audit_supports_20261004/README.md) ;
[réponse du développeur](../audits/REPONSE_CLAUDE_SUPPORTS_20261004.md).

**État.** Rien de ce qui est décrit ici n'est encore exécutable. L'exécutable, la façade `api` et les écrivains de
formats arrivent en L2 (§ 11). Existent seulement le module `io` (lecture, empreintes, transaction de dossier, § 9) et
l'en-tête public de la tour.

## 1. L'exécutable

- Un seul exécutable, `mhgp11`. Sa cible CMake s'appelle `mhgp11_cli`, avec `OUTPUT_NAME mhgp11`, car `mhgp11` est
  déjà la bibliothèque. `cli/mhgp11.cpp` n'inclut que la façade `api/api.hpp` ; `cli/cli.cmake` ne déclare que cette
  cible ; ses portes vont dans `tests/cli/tests.cmake`.
- Paramètre obligatoire `--sortie`, **une seule valeur par appel**. Une valeur n'est admise qu'une fois livrée avec ses
  portes (§ 11). Avant, elle est refusée `parameter_out_of_range`, avant tout effet.
- Options nommées en français, de forme `--clé=valeur`. Les clés JSON de la sortie standard et du manifeste restent en
  anglais, comme celles des bancs.

```text
mhgp11 --sortie=<full|supports|points|plat> --points=<x.u32le> --ids=<ids.u32le> --dossier=<D> --k=<K>
       [--fils=<W>] [--budget=<octets>] [--pas=<decimal>] [--origine=<x,y,z>]
       [--mcs=<M>] [--z=<1|2|3>] [--selection=<eom|feuilles>]          (plat seulement)
```

| Option | Domaine | Défaut |
| --- | --- | --- |
| `--sortie` | `full`, `supports`, `points` ou `plat`, parmi les valeurs déjà livrées | obligatoire |
| `--points` | fichier régulier, trois mots `u32` petit-boutistes $(x,y,z)$ par point | obligatoire |
| `--ids` | fichier régulier, un `PointId` `u32` petit-boutiste par point, dans le même ordre | obligatoire |
| `--dossier` | chemin du dossier de sortie $D$ ; ni $D$ ni `D.pending` ne doivent exister | obligatoire |
| `--k` | $1\leq K\leq 12$, puis $K\leq n$ ; pour `full`, ordre maximal (ordres $1$ à $K$) | obligatoire |
| `--fils` | 1 à 256, fil appelant compris ; aucune détection implicite de la machine | 1 |
| `--budget` | octets, entier strictement positif | illimité |
| `--pas`, `--origine` | décimaux exacts, recopiés tels quels dans le manifeste | non déclarés |
| `--mcs` | entier $\geq 2$ (`plat` seulement) | 20 |
| `--z` | 1, 2 ou 3 (`plat` seulement) | 1 |
| `--selection` | `eom` ou `feuilles` (`plat` seulement) | `eom` |

**Moteur.** Il n'existe aucune option de moteur (règle 6 d'[ARCHITECTURE.md](ARCHITECTURE.md)). Les paramètres sont
ceux, qualifiés, des sondes de banc : masque 16 379 de `bench/full_probe.cpp`, `leaf_size` 16 et `max_leaf` 256 comme
`bench/points_export.cpp`. Les sorties `supports`, `points` et `plat` construisent l'arbre d'ordre K seul
(`build_order`, tranche S3) avec les mêmes paramètres, à deux exceptions près : les ordres concurrents, que
`build_order` refuse jusqu'à la tranche S11, et les verticales, sans objet pour un seul ordre.

## 2. Les quatre sorties

| `--sortie` | Objet | Fichier de données | Format | Tranche, livraison |
| --- | --- | --- | --- | --- |
| `full` | tour FULL, ordres $1$ à $K$, avec verticales | `full.mhgp11ful1` | `MHGP11FUL1`, inchangé (§ 5) | S5, L2 |
| `supports` | arbre d'ordre K, boules de $W_K$ rattachées, tous leurs supports positifs minimaux | `supports.mhgp11sp` | `MHGP11SP` v1 (§ 6) | S7, L2 |
| `points` | hiérarchie de points $H^{r}_{K+1}$ à l'ordre K | `points.mhgp11pt` | `MHGP11PT` v1, annoncé (§ 7) | S9, L3 |
| `plat` | étiquettes plates tirées de la hiérarchie de points | `etiquettes.mhgp11et` | `MHGP11ET` v1, annoncé (§ 7) | S10, L4, reportable |

Chaque appel réussi publie un dossier $D$ qui contient ce fichier et `manifeste.json` (§ 8), selon la transaction du
§ 9. Les quatre sorties d'une même entrée et d'un même K portent la même empreinte d'arbre `tree_k_sha256`.

## 3. Refus et codes de sortie

**Ordre déterministe des refus.** Aucun refus ne publie de dossier, hors les deux doubles échecs du § 9.

1. Options : inconnue, répétée, absente, hors domaine ou propre à une autre sortie ; valeur de `--sortie` inconnue ou
   non livrée. Raison : `parameter_out_of_range`, avant tout effet.
2. Plan du dossier, **avant toute lecture** (§ 9) :
   - `parameter_out_of_range` : forme du chemin ;
   - `output_unwritable` : parent absent ou qui n'est pas un dossier ;
   - `output_conflict` : une entrée résolue est $D$, `D.pending` ou se trouve dessous, ou bien $D$ ou `D.pending`
     existe ;
   - `output_unwritable` : parent non inscriptible.
3. Session : `session_overhead`, `memory_budget`, `environment_selftest` (auto-test F5, raison annoncée).
4. Lecture (`io::read_u32le`) :
   - `input_unreadable` : fichier absent, illisible ou non régulier, tailles incohérentes, lecture incomplète, octet de
     trop ;
   - `index_overflow_u32` : au moins $2^{32}-1$ points ;
   - `memory_budget` : 16 octets par point, admis avant l'allocation.
5. Nuage : `empty_input`, `coordinate_out_of_domain`, `duplicate_point_id`. Positions répétées :
   `multiplicity_unsupported`, car la tour exige des sites de poids un.
6. $K>n$ : `parameter_out_of_range`.
7. Calcul :
   - ressources : `memory_budget`, `node_budget`, `catalogue_counter_overflow`, `tower_capacity`,
     `radical_sign_budget` (`points` et `plat`) ;
   - dégénérescences : `wide_leaf`, `support_shell_capacity` (`supports`) ;
   - invariants : `catalogue_invariant`, `tower_invariant`, `supports_invariant`, `points_invariant`,
     `head_invariant`, `arithmetic_invariant`, `budget_not_released`.
8. Écriture et publication (§ 9) : `output_unwritable` ; `output_conflict` si $D$ ou `D.pending` est apparu depuis le
   plan.

Entre deux refus d'un même étage, la fusion `merge` de `src/core/status.hpp` retient le plus petit K, puis la raison
placée la première dans `src/core/reasons.def`. Le refus ne dépend donc pas du nombre de fils.

**Raisons.** Toutes figurent dans `src/core/reasons.def` à `f98aeed67`, sauf six raisons annoncées. Chacune s'ajoutera
en fin de table, avec le code qui l'émet et la porte qui la provoque, dans l'ordre des livraisons :

| Raison | Statut | Module | Tranche |
| --- | --- | --- | --- |
| `support_shell_capacity` | `unsupported_degeneracy` | `supports` | S6 (L1) |
| `supports_invariant` | `invariant_violated` | `supports` | S6 (L1) |
| `environment_selftest` | `invariant_violated` | `api` | S5 (L2) |
| `radical_sign_budget` | `resource_exhausted` | `num` | S8 (L3) |
| `points_invariant` | `invariant_violated` | `points` | S9 (L3) |
| `head_invariant` | `invariant_violated` | `head` | S10 (L4) |

Cet ordre suit le plan révisé, où S6 (L1) précède S5 (L2). La spécification, qui livrait S5 d'abord, plaçait
`environment_selftest` en tête. Le rang d'une raison dans la table ne départage que deux refus au même K.

**Codes de sortie** (`exit_code`, `src/core/status.hpp`) :
- 0 : conforme ;
- 2 : refus ;
- 3 : invariant violé, c'est-à-dire tout statut `invariant_violated`, `budget_not_released` et
  `environment_selftest` compris.

Un arrêt par signal est toujours un échec. Les codes 1 et 4 appartiennent aux portes, jamais à l'exécutable.

**Sortie standard.** Exactement une ligne JSON, en succès comme en refus :

```json
{"phase":"mhgp11","output":"supports","status":"ok","reason":"none","coord_bits":21,"k":5,"workers":48,"sites":39885,"stages_ns":{"cloud":0,"index":0,"domain":0,"tree":0,"attach":0,"output":0,"write":0},"peaks_bytes":{"domain":0,"tree":0,"output":0},"counts":{"nodes":0,"balls":0,"supports":0}}
```

- Temps et nombre de fils vont dans cette ligne, **jamais dans le manifeste**.
- Étages de `stages_ns` :
  - `cloud` : lecture et préparation du nuage ;
  - `index` ;
  - `domain` : catalogue $\mathrm{Cat}_K$ ;
  - `tree` : forêts $1$ à $K$ de FULL pour `full`, arbre d'ordre K seul pour les trois autres sorties ;
  - `attach` : rattachement des boules (nul pour `full`) ;
  - `output` : produit (supports et assemblage, hiérarchie de points ou tête plate) ;
  - `write` : écriture et publication.
- En refus, la ligne porte le statut et la raison. Sa forme exacte est fixée en S5, avec la porte
  `mhgp11_cli_contract`.
- La ligne est écrite **après** la publication. Si son écriture ou la vidange de la sortie standard échoue, le dossier
  publié est retiré (`retract()`, § 9) et l'appel rend `output_unwritable` (code 2), avec un message sur la sortie
  d'erreur. Il n'y a donc jamais de code 0 sans ligne d'état, ni de dossier publié après un refus (hors le double
  échec du § 9). C'est la frontière R2 de la v10.

## 4. Conventions communes des formats binaires

- Petit-boutiste.
- Une section est stockée colonne par colonne (structure de tableaux). Chaque colonne commence sur une frontière de
  8 octets ; les octets de bourrage sont nuls. Une colonne se lit donc d'un seul tenant.
- `kNone` $=2^{32}-1$ ne sert de sentinelle qu'aux rangs denses (`NodeIdx`, `LevelRank`). Un `PointId` peut valoir
  $2^{32}-1$ : une colonne de `PointId` est toujours bornée par un compte.
- Les rangs publiés sont des `LevelRank` du catalogue $\mathrm{Cat}_K$ de l'appel. L'égalité de rangs équivaut à celle
  des niveaux, et l'ordre des rangs à celui des niveaux ; le rang 0 est le niveau nul. Des rangs de deux appels
  (autre entrée, autre K) ne se comparent pas.
- Ordres canoniques, tous géométriques :
  - sites : `SiteIdx`, rang de Morton des positions ;
  - nœuds : numérotation canonique de la forêt d'ordre K de FULL ([FULL_FORESTS.md](FULL_FORESTS.md)). Viennent
    d'abord les naissances, triées par (niveau, centre exact en ordre lexicographique) ; à $K=1$, ce sont les sites
    en ordre lexicographique $(x,y,z)$. Suivent les fusions, triées par (niveau, plus petite naissance descendante) ;
  - boules : (postordre du nœud de rattachement, rang, $S^*$ en ordre lexicographique) ;
  - supports : (arité, ordre lexicographique des `SiteIdx`).

## 5. `full.mhgp11ful1` : `MHGP11FUL1`, inchangé

Ce sont les octets de `serialize` (`bench/full_probe.cpp`), lus par `bench/full_semantic.py`. Seule l'écriture
change : elle devient transactionnelle (§ 9). Aucune version nouvelle n'est créée, pour garder la comparabilité avec les
reçus immuables.

Contenu :
- magie de 10 octets `MHGP11FUL1`, puis des mots `u64` petit-boutistes : `coord_bits`, $K$, nombre de sites, poids
  total ;
- par site : $x$, $y$, $z$, poids, puis ses `PointId` ;
- par ordre $k=1..K$ : $k$, naissances, nœuds, arêtes, racine ;
- puis, par nœud : parent, début et nombre d'enfants, niveau exact ;
- pour une naissance : son centre exact ;
- pour $k>1$ : la verticale ;
- enfin, la liste des enfants.

Les entiers exacts s'écrivent en mots : signe, nombre de mots, puis les mots. La porte `mhgp11_cli_full_identity`
exige le sha256 brut du dump de `mhgp11_full_bench` sur les mêmes entrées.

## 6. `supports.mhgp11sp` : `MHGP11SP` version 1 (normatif)

**Objet.** Les définitions et les preuves sont dans [MATHEMATIQUES.md](MATHEMATIQUES.md), section 10.
- L'arbre $T_K$ est la forêt d'ordre K de FULL, dans sa numérotation canonique.
- Toutes les boules de $W_K=\lbrace b\in\mathrm{Cat}_K : p+q_{\min}-1\leq K\leq p+m\rbrace$ sont publiées : naissances,
  fusions et liaisons internes, événements faibles compris.
- Chaque boule est rattachée à $\mathrm{att}(b)$, le nœud vivant à la coupe **fermée** $\lambda_b$, après fermeture de
  tout le plateau.
- Son rôle se décide par les rangs.
- Ses branches $\mathrm{ant}(b)$, prises à la coupe **stricte** et dédupliquées, sont publiées pour le rôle fusion
  seulement.
- Tous ses supports positifs minimaux $\mathcal{Q}_b$ sont publiés, énumérés sur **toute** la coquille $U_b$.
- Plafond de coquille étendue : 24 sites, constante de compilation. Au-delà, l'appel entier est refusé
  `support_shell_capacity` et rien n'est publié.
- **Aucune population** $P_b=I_b\cup U_b$ n'est publiée. Les sites des supports le sont donc explicitement.
- Aucun compte n'est stocké.

Principe de sobriété : ne stocker que ce qui ne se dérive pas.

**En-tête** (136 octets). Magie `MHGP11SP` (8 octets), puis 16 mots `u64`, dans cet ordre :

| Mot | Champ | Sens |
| ---: | --- | --- |
| 0 | `version` | 1 |
| 1 | `coord_bits` | 18, 21 ou 24 (profil de compilation) |
| 2 | `k` | l'ordre K |
| 3 | `n` | nombre de sites |
| 4 | `N` | nombre de nœuds |
| 5 | `root` | la racine, unique (`NodeIdx`) |
| 6 | `B` | nombre de boules, $\lvert W_K\rvert$ |
| 7 | `S` | nombre de supports |
| 8 | `Z` | somme des arités des supports |
| 9 | `A` | nombre de branches publiées |
| 10 à 14 | décalages | début, en octets depuis le début du fichier, de `SITES`, `NODES`, `BALLS`, `SUPPORTS`, `PRIOR` |
| 15 | taille | taille totale du fichier en octets : fin de la dernière colonne, bourrage compris |

**Sections**, dans cet ordre. Leurs colonnes se suivent dans l'ordre indiqué ; chacune est alignée sur 8 octets, avec
un bourrage nul. Une colonne vide n'occupe aucun octet.

| Section | Colonnes | Ordre des lignes |
| --- | --- | --- |
| `SITES` | `x u32[n]`, `y u32[n]`, `z u32[n]`, `point_id u32[n]` | `SiteIdx` (Morton) |
| `NODES` | `parent u32[N]` (`kNone` à la racine), `rank u32[N]`, `kind u8[N]` (0 feuille-site à $K=1$, 1 naissance de boule, 2 fusion), `ball_count u32[N]` (boules propres) | numérotation canonique |
| `BALLS` | `rank u32[B]` ($\lambda_b$), `prior_count u32[B]` (taille de $\mathrm{ant}(b)$ publiée : rôle fusion seulement, 0 sinon), `support_count u16[B]` ($\lvert\mathcal{Q}_b\rvert\leq 12\,926$), `role u8[B]` (0 naissance, 1 fusion, 2 interne), `p u8[B]`, `m u8[B]` | postordre du nœud de rattachement, puis rang, puis $S^*$ en ordre lexicographique |
| `SUPPORTS` | `arity u8[S]`, puis `sites u32[Z]` (lignes de `SITES`, croissantes dans chaque support) | par boule, dans l'ordre des boules ; dans une boule, par (arité, ordre lexicographique des `SiteIdx`), donc $S^*$ en tête |
| `PRIOR` | `node u32[A]` : $\mathrm{ant}(b)$ en numérotation canonique, croissant | par boule de rôle fusion, dans l'ordre des boules |

Remarques :
- Le postordre part de la racine et visite les enfants par `NodeIdx` croissant ; un nœud suit son sous-arbre. Les
  boules d'un sous-arbre forment donc une tranche contiguë, qui finit par les boules propres de sa racine.
- Dans un nœud, l'ordre (rang, $S^*$) coïncide avec celui des `BallIdx` du catalogue (niveau, puis $S^*$ complété par
  `kNone`). À niveau égal, en effet, le $S^*$ d'une boule n'est jamais une partie de celui d'une autre : par M1, il
  porterait une boule de rayon strictement plus petit.
- Bornes : $12\,926=\binom{24}{2}+\binom{24}{3}+\binom{24}{4}$ majore le nombre de supports d'une coquille de 24 sites ;
  $p\leq K-1\leq 11$ ; $m\leq 24$.

**Dérivé, jamais stocké.** L'API C++ et le lecteur en bibliothèque standard (`bench/mhgp11_formats.py`, tranches S5 et
S7) calculent ces quantités. On note $t=K-p$ et $N_j$ le nombre de parties à $j$ sites de $U_b$ qui contiennent un
support de $\mathcal{Q}_b$. $N_j$ se calcule à partir de $\mathcal{Q}_b$ et de $m$ seuls : pour ce compte, les sites
de coquille hors de tout support sont interchangeables.

| Quantité | Dérivation |
| --- | --- |
| enfants, postordre `post`, taille de sous-arbre `size` | depuis `parent` ; enfants par `NodeIdx` croissant |
| décalages | sommes préfixes de `ball_count` (en postordre), `support_count`, `arity`, `prior_count` |
| nœud de rattachement $\mathrm{att}(b)$ | parcours des nœuds en postordre, chacun prenant ses `ball_count` boules consécutives |
| $q_{\min}(b)$ | arité du premier support, $S^*$ |
| niveau exact d'une boule | rayon carré de la sphère circonscrite à son premier support $S^*$ |
| niveau d'un nœud | celui de sa première boule propre ; 0 pour une feuille de $K=1$ |
| site de la feuille $i$ à $K=1$ | le site de rang $i$ dans l'ordre lexicographique $(x,y,z)$ des lignes de `SITES` (`point_births`, `src/tower/forest_build.cpp`) |
| `has_support_geometry` | `kind` différent de 0 |
| `kparties_reliees` (boule) | $\binom{p+m}{K}$ |
| `compressed_parts` (boule) | $\binom{m}{t}$ |
| `strict_traces` (boule) | $\binom{m}{t}-N_t$ |
| `components` (boule ; nom de l'audit : `strict_global_components`) | taille de $\mathrm{ant}(b)$ : `prior_count` pour une fusion, 1 pour une boule interne, 0 pour une naissance |
| `cofaces` (boule) : liaisons distinctes | $\sum_{j}\binom{p}{K+1-j}N_j$ |
| `cofaces` (support $Q$) : incidences | $\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$, nul si $K+1<\lvert Q\rvert$ |
| `gabriel_cofaces` | support : $\binom{m-\lvert Q\rvert}{t+1-\lvert Q\rvert}$ ; boule : $N_{t+1}$ |

- `kparties_reliees` est le compte mis en avant : le nombre de $K$-parties de $P_b$. Elles sont toutes reliées à la
  coupe fermée $\lambda_b$. Il vaut $K+1$ à une jonction régulière, 1 à une naissance régulière, 4 au carré à $K=3$.
- Les liaisons de la thèse (Prop. 5) sont les $(K+1)$-parties, comptées par `cofaces`.
- La somme des `cofaces` des supports d'une boule compte des incidences $(Q,G)$, pas des cofaces distinctes.
- Les unions effectuées par le constructeur dépendent de l'ordre de traitement : elles ne sont jamais publiées.

**Ce que le lecteur contrôle**, sans la coquille :
- l'arbre est bien formé ;
- les rangs croissent strictement vers la racine ;
- les boules sont triées ;
- la première boule propre d'un nœud de `kind` 1 ou 2 est au rang du nœud ;
- chaque support est un support positif de la sphère de $S^*$ : ses sites sont sur cette sphère et son centre est dans
  l'intérieur relatif de leur enveloppe convexe (prédicats exacts en `Fraction`) ;
- les supports sont dans l'ordre canonique ;
- le rôle est cohérent avec les rangs (lemme B), et une naissance a `strict_traces` nul.

Le lecteur ne peut pas vérifier la complétude de $\mathcal{Q}_b$, puisque la coquille n'est pas publiée. Elle relève
des juges bornés (oracle S1) et natifs (juge d'échantillon, tranche S6).

**Lectures.** Notons $j=$ `post[v]` et $s=$ `size[v]`.
- Les boules du sous-arbre de $v$ sont celles des nœuds de postordre $j-s+1$ à $j$, une tranche contiguë.
- La réalisation par supports $P_v$ de $v$ (notation de `Zoltan/FoundationModel/JETON.md`, à ne pas confondre avec
  la population $P_b$ d'une boule) est l'union, sur ces boules, des $\mathrm{conv}\,Q$ pour $Q\in\mathcal{Q}_b$. On a
  donc $P_w\subseteq P_v$ pour tout $w$ du sous-arbre de $v$.
- Instantané au rang $r$, avec `rank[v]` $\leq r<$ `rank[parent[v]]` (sans borne supérieure à la racine) : les boules
  du sous-arbre strict, toutes de rang inférieur à `rank[v]`, plus le préfixe des boules propres de $v$ de rang au
  plus $r$.
- Seules des liaisons internes de $v$ ont un rang supérieur à `rank[v]` : un état est daté.

**Ce que cette réalisation n'est pas.** Ces déclarations sont obligatoires ; la section 10 de MATHEMATIQUES les
démontre ou les illustre.
- **Elle n'est pas stable aux cosphéricités.** Sur le cercle à quatre points de l'audit `1bf4be68f`, $\mathcal{Q}_b$
  passe de deux diamètres à un diamètre et un triangle strict, sous une perturbation qui tend vers 0. La boule ne
  change pas, mais le saut de Hausdorff vaut au moins $1/4$. La stabilité de FULL en rayon (P5) ne se transfère pas à
  ce support porteur, et aucune stabilité n'est revendiquée. `kparties_reliees` ne dépend que de $(p,m,K)$ : il échappe
  à cette instabilité.
- **Elle omet** les sites intérieurs et, sur une coquille étendue, les sites de coquille hors de tout support. Exemple :
  dans le triangle droit, l'origine est orpheline.
- **Elle n'est pas le $K$-polyèdre de la thèse** (Déf. 21), qui est un ensemble de points. Celui-ci reste
  reconstructible hors format, à partir du catalogue, par le lemme H : union des $P_b$ des boules de $W_K$ rattachées
  au sous-arbre et de niveau au plus $a$.
- Elle n'est pas non plus $\mathrm{conv}(U_b)$. Les réalisations de deux branches peuvent se recouvrir, et leurs
  intersections ne donnent pas la connectivité de FULL.

## 7. Formats annoncés

Ces deux formats sont annoncés par la spécification. Ils seront précisés puis figés avec leurs tranches et leurs
portes. Leurs conventions sont celles du § 4.

**`points.mhgp11pt` : `MHGP11PT` version 1** (tranche S9, L3).

| Section | Contenu |
| --- | --- |
| En-tête | magie `MHGP11PT` ; `version=1`, `coord_bits`, `k`, `m` (qualification : 1 si $K=1$, $K+1$ sinon), `kappa=1`, `n`, `L` (niveaux référencés), `W` (mots par niveau : 3 en u18 et u21, 4 en u24), `N`, `P` (plateaux), `Bk` (blocs), décalages |
| `SITES` | comme `MHGP11SP` |
| `LEVELS` | `rank u32[L]`, `num u64[W·L]`, `den u64[W·L]` : valeurs exactes non réduites des rangs référencés, triés |
| `NODES` | `parent u32[N]`, `rank u32[N]` : même numérotation que `MHGP11FUL1` et `MHGP11SP` |
| `HANGING` (par `SiteIdx`) | `t u32[n]`, `M u32[n]`, `Q u32[n]` (date $\sqrt{t}+\sqrt{M}-\sqrt{Q}$ ; $M=Q=0$ sans rival), `owner u32[n]`, `floor u32[n]`, `strict u8[n]` |
| `TREE` | `plateau_t u32[P]`, `plateau_M u32[P]`, `plateau_Q u32[P]` (strictement croissants en valeur exacte), `block_plateau u32[Bk]`, `block_parent u32[Bk]`, `site_block u32[n]`, `site_plateau u32[n]` |

Le schéma est celui de `PointTree` (`bench/points_flat.py`) et de `Hanging` (`bench/points_hierarchy.py`). Les
conventions $m(1)=1$ et $\kappa=1$ sont celles de [HIERARCHIE_POINTS.md](HIERARCHIE_POINTS.md).

**`etiquettes.mhgp11et` : `MHGP11ET` version 1** (tranche S10, L4, reportable).
- En-tête : magie `MHGP11ET`, `version=1`, `n_points`, `k`, `mcs`, `z`, `selection` (0 pour EOM, 1 pour les
  feuilles).
- Puis `labels i64[n_points]`, **dans l'ordre du fichier d'entrée**. Une étiquette vaut le plus petit `PointId` du
  cluster retenu, ou $-1$ pour le bruit.
- Les compteurs vont dans le manifeste.
- Le port n'emploie pas `out[pt.ids]` (`bench/points_flat.py`), qui suppose des `PointId` denses.

## 8. Manifeste `manifeste.json`

Le manifeste est écrit **en dernier**, après la fermeture contrôlée de tous les fichiers de données (§ 9). C'est le
seul témoin d'achèvement.

Règles de forme :
- un objet JSON, clés dans l'ordre fixe ci-dessous, entiers en décimal, aucun flottant ;
- aucun temps ni nombre de fils : le manifeste est identique à l'octet quel que soit W ;
- sa forme exacte en octets est fixée en S5, par sa porte de déterminisme ;
- l'écrivain JSON appartient à `api` (`manifest.cpp`), et non à `io`.

Exemple pour `supports`, présenté ici sur plusieurs lignes pour la lecture :

```json
{"schema":"ehgp.v11.output.v1","output":"supports","status":"complete","public_status":"not_claimed",
 "coord_bits":21,"k":5,"parameters":{"budget_bytes":null,"grid_step":null,"origin":null},
 "inputs":[{"name":"points","bytes":0,"sha256":"…"},{"name":"ids","bytes":0,"sha256":"…"}],
 "files":[{"name":"supports.mhgp11sp","format":"MHGP11SP","version":1,"bytes":0,"sha256":"…"}],
 "tree_k_sha256":"…",
 "counts":{"sites":0,"nodes":0,"births":0,"merges":0,"balls":0,"roles":{"birth":0,"merge":0,"internal":0},
           "supports":0,"arities":{"2":0,"3":0,"4":0},"extended_shells":0,"multi_support_balls":0,
           "max_supports_per_ball":0,"prior":0,"kparties_reliees":{"sum":0,"max":0},"cofaces":{"sum":0,"max":0}}}
```

- `parameters` :
  - `budget_bytes` : l'entier donné à `--budget`, ou `null` sans plafond ;
  - `grid_step`, `origin` : les chaînes de `--pas` et `--origine` recopiées, ou `null` ;
  - pour `plat`, s'y ajoutent `mcs`, `z` et `selection`.
- `inputs` : taille et SHA-256 des deux fichiers d'entrée, pris au fil de la lecture. `files` : taille et SHA-256 de
  chaque fichier de données, pris au fil de l'écriture.
- `counts` de `supports`, dans cet ordre :

  | Clé | Sens |
  | --- | --- |
  | `sites`, `nodes` | $n$ et $N$ |
  | `births`, `merges` | nœuds de `kind` 0 ou 1, puis nœuds de `kind` 2 |
  | `balls`, `roles` | $B$, puis ses boules par rôle |
  | `supports`, `arities` | $S$, puis ses supports par arité |
  | `extended_shells` | boules à coquille étendue, $m>q_{\min}$ |
  | `multi_support_balls` | boules qui ont au moins deux supports |
  | `max_supports_per_ball` | maximum de `support_count` |
  | `prior` | $A$ |
  | `kparties_reliees` | somme et maximum sur les boules |
  | `cofaces` | somme et maximum sur les boules des cofaces **par boule** (liaisons distinctes, pas incidences) |

  Un maximum sur un ensemble vide vaut 0.
- Les `counts` de `full`, `points` et `plat` sont fixés en S5, S9 et S10, selon les mêmes règles.

**`tree_k_sha256`.** C'est le SHA-256 de la suite d'octets suivante :
- les 8 octets ASCII `MHGP11TK`, puis $K$ et $N$ en `u64` petit-boutistes ;
- pour chaque nœud, dans l'ordre canonique : `parent`, `rank`, `birth_key` et nombre d'enfants, chacun en `u32`
  petit-boutiste. `birth_key` est la ligne de site d'une feuille à $K=1$, le `BallIdx` de la boule de naissance à
  $K\geq 2$, et `kNone` pour une fusion ;
- enfin, la liste concaténée des enfants des nœuds, dans l'ordre des nœuds, chacune croissante.

Elle est calculée par le moteur. Elle est commune à `full` (pour l'ordre maximal $K$), `supports`, `points` et `plat`
sur une même entrée et un même K. Le lecteur de `MHGP11SP` seul ne peut pas la recalculer, car `BallIdx` n'y est pas
publié. Elle sert à apparier des sorties.

## 9. Transaction de dossier, telle qu'implémentée dans `src/io/`

`io::OutputDirectory` et `io::FileWriter` (`src/io/io.hpp`, `src/io/directory.cpp`, `src/io/writer.cpp`) sont un port
du raccord R2 de la v10 ([provenance](PROVENANCE.md), section io), adapté à un dossier entier.

1. **Plan**, avant tout calcul et toute lecture : `plan(D, entrées)` ne crée rien. Un chemin `D/` désigne $D$. Les
   refus viennent dans cet ordre :
   - `parameter_out_of_range` : chemin vide ou d'au moins `PATH_MAX` octets ; nom final vide, `.` ou `..` ; nom de
     `D.pending` de plus de `NAME_MAX` octets ;
   - `output_unwritable` : parent absent ou qui n'est pas un dossier ;
   - `output_conflict` : une entrée, une fois son chemin résolu (liens symboliques suivis), est $D$, `D.pending` ou se
     trouve dessous. Une entrée sans chemin résolu n'est pas comparée : sa lecture la refusera ;
   - `output_conflict` : $D$ existe, lien symbolique pendant compris ; puis de même pour `D.pending`, par exemple un
     orphelin. Une erreur d'accès autre que l'absence rend `output_unwritable` ;
   - `output_unwritable` : parent non inscriptible ou non traversable.
2. **Création**, après le calcul : `create(nom)`.
   - `D.pending/` est créé à la première demande, ou au commit si aucun fichier n'a été demandé. Chaque fichier y est
     créé en exclusif (`O_EXCL`, `O_NOFOLLOW`), sous son nom final.
   - Un nom est de la forme `[a-z0-9_.]+`, d'au plus 64 octets, ne commence pas par `.` et n'est pas `manifeste.json`.
     Il y a au plus 8 fichiers de données. Hors de ces règles : `parameter_out_of_range`.
   - Un nom déjà créé, ou un `D.pending` apparu depuis le plan : `output_conflict`.
   - Toute erreur d'entrée-sortie est définitive : le dossier ne sera pas publié.
3. **Écriture** (`FileWriter`).
   - Mots petit-boutistes (`bytes`, `u32s`, `u64s`), et `pad8` jusqu'à la prochaine frontière de 8 octets.
   - Taille et SHA-256 sont tenus au fil de l'écriture ; tampon fixe de 64 Kio.
   - Une erreur est définitive (`output_unwritable`).
4. **Publication** : `commit(manifeste)`.
   - Chaque fichier de données est vidé, contrôlé, synchronisé, puis fermé : `fflush`, `ferror`, `fsync`, `fclose`.
   - `manifeste.json` est écrit **ensuite**, puis synchronisé et fermé ; enfin `D.pending` est synchronisé.
   - Un seul renommage publie le dossier : `renameat2(D.pending, D, RENAME_NOREPLACE)`, puis le parent est synchronisé.
     Un lecteur voit $D$ entier, ou ne le voit pas.
   - Refus du renommage :
     - $D$ apparu entre-temps : `output_conflict` ;
     - renommage sans remplacement indisponible (noyau ou système de fichiers), ou autre erreur :
       `output_unwritable`. Il n'y a **jamais** de `rename` POSIX, qui remplacerait un dossier vide.
   - Un commit refusé est définitif. `manifest_sha256()` rend l'empreinte du manifeste publié.
5. **Sans commit réussi**, le destructeur ferme les fichiers. Il retire ceux que l'objet a créés, puis `D.pending`
   s'il l'a créé. Il ne touche jamais $D$. Il ne retire jamais un `D.pending` qu'il n'a pas créé.
   - Un `D.pending` orphelin, laissé par un arrêt brutal, fait refuser l'appel suivant (`output_conflict`). Il n'est
     jamais retiré automatiquement.
   - Un arrêt brutal ne laisse jamais un $D$ partiel.
6. **Retrait après publication** : `retract()`, frontière R2.
   - Le CLI l'appelle si sa ligne de sortie standard ne peut pas être écrite après le commit.
   - $D$ est renommé en `D.pending` sans remplacement, puis le destructeur le retire comme un dossier jamais publié.
     L'objet ne publie plus rien ensuite.
   - Refus : rien n'est publié (`output_unwritable`), ou le renommage échoue (`output_unwritable`, ou
     `output_conflict` si un `D.pending` est apparu entre-temps). $D$ reste alors publié et complet.

**Les deux doubles échecs.** Ce sont les seuls cas où un refus laisse un dossier publié ; il est alors complet, et son
manifeste en fait foi.
- La synchronisation du parent échoue après le renommage. La publication est d'abord défaite : $D$ est renommé en
  `D.pending`, que le destructeur retire, et le refus est `output_unwritable`. Si ce retour échoue aussi, $D$ reste
  publié et complet. `committed()` rend alors vrai, `commit` rend `output_unwritable`, et `manifest_sha256()` reste
  nul.
- `retract()` échoue après l'échec de la sortie standard.

Dans les deux cas, l'appel rend le code 2.

Limites :
- la durabilité est celle que donnent les `fsync` ;
- les tampons de `stdio`, au plus 64 Kio par fichier ouvert, sont hors `MemoryBudget` ;
- le contrat reste celui de la trame entière en mémoire ([ARCHITECTURE.md](ARCHITECTURE.md), § 7.2).

## 10. Invariance et déterminisme

- Les fichiers de données et le manifeste sont identiques à l'octet quel que soit le nombre de fils (portes de
  déterminisme à W1, W4 et W48).
- Une **permutation** de l'entrée donne des fichiers `full`, `supports` et `points` identiques ; les étiquettes de
  `plat`, rangées dans l'ordre d'entrée, sont permutées de même. Le manifeste ne change que par les empreintes des
  entrées et, pour `plat`, celle du fichier d'étiquettes.
- Un **réétiquetage injectif** des `PointId` (non dense, $2^{32}-1$ compris) ne change que les `PointId` publiés : la
  colonne `SITES.point_id` de `MHGP11SP` et `MHGP11PT`, les `PointId` par site de `MHGP11FUL1`, et les étiquettes de
  `plat`.
- Une translation entière, une réflexion ou un échange d'axes change l'ordre de Morton et la numérotation canonique :
  seuls les **ensembles** sont invariants (nœuds comme ensembles de sites, supports comme ensembles de positions,
  niveaux).
- Les rotations ne sont pas couvertes : la quantification au millimètre casse l'équivariance.

## 11. Livraison, décisions écrites d'avance

| Livraison | Session | Tranches | État au 4 octobre 2026 |
| --- | --- | --- | --- |
| L0 | aucune | S0 (ce document, MATHEMATIQUES section 10, registre des preuves), S1 (oracle borné des supports) | en cours |
| L1 | G4 n° 1 | S2 (en-tête public de la tour), S3 (`build_order`, journal des graines, `WindowAttachment`, juge E2), S6 (module `supports`) | S2 livrée (`257aabb92`) |
| L2 | G4 n° 2 | S4 (`io`, avec `retract()`), S5 (`api`, `Session`, manifeste, `--sortie=full`), S7 (`--sortie=supports`, écrivain et lecteur `MHGP11SP`, mesure appariée) | S4 livrée (`f98aeed67`) |
| L2b | conditionnelle | journal des graines posé dans `build_full` | selon la règle ci-dessous |
| L3 | G4 n° 3 | S8 (`num`), S9 (`--sortie=points`) | — |
| L4 | G4 n° 4, reportable | S10 (`--sortie=plat`) | — |
| L5 | facultative | S11 (pipeline à un ordre), chantier 100 ms | — |

- Valeurs de `--sortie` admises : aucune avant L2 ; `full` puis `supports` en L2 ; `points` en L3 ; `plat` en L4.
  Toute autre est refusée `parameter_out_of_range`.
- La réponse écrite des deux auditeurs au contrat S0 précède toute ligne native de S3.
- Chaque tranche native se clôt par la matrice G4 et un reçu immuable.
- Aucune mesure ne promeut un statut public.

**Règle de décision de L2**, fixée le 4 octobre 2026, avant toute mesure.
- *Mesure.* Elle est appariée et se fait dans la session G4 gardée de L2. On relève l'étage `tree` de la ligne de
  sortie standard pour `--sortie=supports` (arbre d'ordre K seul par `build_order`, voie par lots) et pour
  `--sortie=full` (forêts $1$ à $K$ par `build_full`).
- *Configurations.* Trames ng00, ng01 et ng02 (séquence 08, sans sol, grille de 1 mm), $K=5$, W1 et W48, trois prises
  par configuration, avec des sorties identiques entre les prises. $K=10$ est mesuré une fois, sur ng00.
- *Statistique.* Par trame et par sortie, la médiane des trois prises de l'étage `tree` à W48.
- *Règle.* `build_order` reste la voie par défaut si, sur **au moins deux des trois trames**, la médiane de
  `supports` ne dépasse pas de plus de 10 % celle de `full`, c'est-à-dire si
  $T_{\mathrm{supports}}\leq 1{,}1\,T_{\mathrm{full}}$. Sinon, on livre **L2b**.
- *Portée.* W1 et $K=10$ sont publiés à titre descriptif ; ils n'entrent pas dans la décision.
- *L2b.*
  - Un pointeur facultatif vers le journal des graines est posé sur le constructeur d'ordre K de `build_full` et de
    `build_concurrent`.
  - `supports` prend alors l'arbre d'ordre K de `build_full`.
  - Porte : `MHGP11SP` identique à l'octet par les deux voies.
  - Qualification : TSan sur `mhgp11_tower_pipeline` et rejeu du manifeste des mutants.
  - Ni l'objet ni les octets de sortie ne changent.
- La voie pipeline à un ordre (S11, L5) ne devient la voie par défaut que sur reçu.

**Clause de report de `plat`.** La livraison L4 (tranche S10) peut être reportée. Le report se décide par écrit, dans
le [README](../README.md) et dans une note aux auditeurs, tant que la règle E1-bis reste ouverte
([SORTIE_PLATE.md](SORTIE_PLATE.md), § 2, point 2, et « Ouvert » du § 3.4) : il manque encore une règle qui garde les
séparations fugaces sans déchiqueter les objets à lignes de balayage.
- Le tokenizer de `Zoltan/FoundationModel/` n'a pas l'usage de cette sortie : sa condensation est à seuil relatif
  (`Zoltan/FoundationModel/SPECIFICATION.md`, § 2.2).
- D'ici là, `--sortie=plat` est refusé `parameter_out_of_range`. Le banc Python exact (`bench/points_flat.py`, porte
  `bench/points_flat_gate.py`) sert aux comparaisons avec HDBSCAN.
- Le report ne retire aucune exigence : quand L4 sera engagée, ses portes resteront celles de la spécification.
