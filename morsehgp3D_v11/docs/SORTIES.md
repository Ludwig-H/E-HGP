# Sorties de `mhgp11` : exécutable, formats, manifeste et transaction de dossier

4 octobre 2026, 20 h 54 UTC, révisé à 22 h 40 UTC après la réponse de l'auditeur mathématique (`aef7182b3`,
questions D.1 à D.4) ; heures lues par `date -u`. Contrat produit de la tranche S0 de la sortie paramétrée
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
[reçu audit_supports_20261004](../receipts/audit_supports_20261004/README.md) ; ses réponses suivantes aussi
(`de4ab58a8`, `aef7182b3`, questions D.1 à D.4, [reçu audit_supports_implementation_20261004](../receipts/audit_supports_implementation_20261004/README.md)) ;
[réponse du développeur](../audits/REPONSE_CLAUDE_SUPPORTS_20261004.md).

**État.** Intégrés le 5 octobre 2026 (commits locaux de l'intégration L1, qualification G4 en attente) : l'exécutable
`mhgp11` et la façade `api` pour `--sortie=full` (tranche S5), et les primitives du module `supports` (tranche S6a,
§ 6). Livrés ensuite le 5 octobre 2026, qualification G4 en attente : l'assemblage de la hiérarchie des supports
(tranche S6b, § 6), puis `--sortie=supports` (tranche S7) : écrivain `MHGP11SP`, manifeste de supports, lecteur
`bench/mhgp11_formats.py` et préparation de la mesure appariée (`bench/sorties_g4.py`, § 11). Livrée ensuite le
5 octobre 2026 (livraison L3, commit local, qualification G4 en attente) : `--sortie=points` (tranche S9), format
`MHGP11PT` version 1 normatif (§ 7), module `points` et décision $K=n$ (§ 3, étape 6). Livrée ensuite le
5 octobre 2026 (livraison L4, qualification G4 en attente) : `--sortie=plat` (tranche S10), format `MHGP11ET`
version 1 normatif (§ 7), module `head` (tête plate certifiée, [sortie plate](SORTIE_PLATE.md), § 5). Existent aussi le module `io` (lecture, empreintes, transaction de dossier, § 9) et l'en-tête public de la
tour. Ce qui est fixé par S5 est signalé « fixé par S5 » ci-dessous.

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

**Forme des valeurs** (fixée par S5). Entiers (`--k`, `--fils`, `--budget`) : chiffres décimaux ASCII seulement, sans
signe, zéros de tête admis, jeton entier, borne vérifiée avant la conversion. Décimaux (`--pas`, `--origine`) :
`[0-9]+(\.[0-9]+)?`, au plus 64 octets ; `--pas` est strictement positif ; chaque coordonnée de `--origine` peut porter
un `-` initial, et `--origine` donne trois décimaux séparés par des virgules. Ils sont recopiés tels quels, sans
normalisation : `0.0010` et `0.001` donnent deux manifestes différents.

**Moteur.** Il n'existe aucune option de moteur (règle 6 d'[ARCHITECTURE.md](ARCHITECTURE.md)). Les paramètres sont
ceux, qualifiés, des sondes de banc : masque 16 379 de `bench/full_probe.cpp`, `leaf_size` 16 et `max_leaf` 256 comme
`bench/points_export.cpp`. Les sorties `points` et `plat` construisent l'arbre d'ordre K seul (`build_order`, tranche
S3) avec les mêmes paramètres, à deux exceptions près : les ordres concurrents, que `build_order` refuse jusqu'à la
tranche S11, et les verticales, sans objet pour un seul ordre. Depuis la livraison L2b (§ 11), la sortie `supports`
tire l'arbre d'ordre K de FULL (`build_order_full`, masque 16 379) : voir ci-dessous.

**Voie de `supports` depuis L2b** (5 octobre 2026, commit local, qualification G4 en attente). La règle de L2 a décidé
`livrer_L2b` (§ 11). `compute(supports)` construit FULL au masque 16 379, ordres $1$ à $K$ et verticales, avec le
journal des graines posé sur le seul constructeur de l'ordre $K$ : le pilote dans la voie non concurrente, la tâche de
publication unique de l'ordre $K$ dans la voie concurrente, par étages ou en pipeline. Les capacités du journal sont
admises avant toute allocation, par le majorant de `build_order`. Après la fin, les autres ordres et les verticales de
l'ordre $K$ sont rendus au budget. Viennent ensuite le balayage du lemme D et les contrôles I1 à I4. L'objet, le
fichier `MHGP11SP` et le manifeste ne changent pas d'un octet, et le manifeste ne nomme pas la voie : les portes
`mhgp11_api_supports_route*` l'exigent par les deux voies, avec des registres du journal égaux. `build_order` reste
disponible et jugé (portes I10, S3 et `points`). La ligne d'état garde `tree` (FULL et extraction), `attach`
(balayage), `output` et `write` séparés ; le pic de `tree` est celui de FULL.

**Arbre d'ordre K seul** (tranche S3, intégrée en L1 le 5 octobre 2026, qualification G4 en attente). `build_order`
refuse (`parameter_out_of_range`) les trois options sans objet pour un ordre seul, au lieu de les ignorer : ordres
concurrents (bit 8 192 du masque), verticales parallèles (128) et réemploi des verticales régulières (1 024). La façade
(S7) les retire du masque 16 379 : l'arbre d'ordre K seul prend le masque 7 035 (`api_detail::order_params`,
`kOrderMask`, avec une assertion statique de la différence), et FULL garde 16 379 (mesure appariée du § 11, audit
`238734f1d`). C'est la seule différence entre les moteurs des deux sorties ; éteindre ces options dans FULL changerait
la référence qualifiée, ce qui n'est pas fait. Les autres paramètres sont honorés, ce que la porte
`mhgp11_tower_order_same_params` exige (table de populations consultée, mémo interrogé, lookup dense construit ;
mutants `table_population_ignoree`, `memo_ordre_ignore`, `lookup_dense_ignore`) : sans elle, la règle de L2
comparerait à FULL une autre configuration que celle annoncée. La tour peut refuser `tower_capacity` quand une cellule
a plus de $2^{32}-1$ traces strictes, sans qu'aucun plafond de coquille soit imposé à FULL.

## 2. Les quatre sorties

| `--sortie` | Objet | Fichier de données | Format | Tranche, livraison |
| --- | --- | --- | --- | --- |
| `full` | tour FULL, ordres $1$ à $K$, avec verticales | `full.mhgp11ful1` | `MHGP11FUL1`, inchangé (§ 5) | S5, L2 |
| `supports` | arbre d'ordre K, boules de $W_K$ rattachées, tous leurs supports positifs minimaux | `supports.mhgp11sp` | `MHGP11SP` v1 (§ 6) | S7, L2 |
| `points` | hiérarchie de points $H^{r}_{K+1}$ à l'ordre K | `points.mhgp11pt` | `MHGP11PT` v1, normatif (§ 7) | S9, L3 |
| `plat` | étiquettes plates tirées de la hiérarchie de points | `etiquettes.mhgp11et` | `MHGP11ET` v1, normatif (§ 7) | S10, L4 |

Chaque appel réussi publie un dossier $D$ qui contient ce fichier et `manifeste.json` (§ 8), selon la transaction du
§ 9. Les quatre sorties d'une même entrée et d'un même K portent la même empreinte d'arbre `tree_k_sha256`.

## 3. Refus et codes de sortie

**Ordre déterministe des refus.** Un refus pris avant la publication (étapes 1 à 8) ne publie aucun dossier. Un
refus constaté après la publication (étape 9) retire le dossier publié ; si ce retrait échoue, le dossier reste
publié et complet, et l'appel le déclare par l'état distinct `published_complete` (§ 9). Un code de refus ne signifie
donc jamais, à lui seul, « rien n'est publié ».

1. Options : inconnue, répétée, absente, hors domaine ou propre à une autre sortie ; valeur de `--sortie` inconnue ou
   non livrée. Raison : `parameter_out_of_range`, avant tout effet.
2. Sortie standard, puis plan du dossier, **avant toute lecture** (§ 9) :
   - sortie standard fermée ou ouverte en lecture seule : `output_unwritable` ; sortie standard désignant l'un des
     fichiers d'entrée (même périphérique et même inode, liens symboliques suivis) : `output_conflict`. Ce contrôle
     est livré par S5 : l'état de la sortie standard est lu avant même les options, sans rien ouvrir, et toute ligne
     de refus va alors sur la sortie d'erreur, à l'étape 1 comprise ;
   - `parameter_out_of_range` : forme du chemin ;
   - `output_unwritable` : parent absent ou qui n'est pas un dossier ;
   - `output_conflict` : une entrée résolue est $D$, `D.pending` ou se trouve dessous, ou bien $D$ ou `D.pending`
     existe ;
   - `output_unwritable` : parent non inscriptible.
3. Session : `session_overhead`, `memory_budget`, `environment_selftest` (auto-test F5, raison annoncée).
4. Lecture (`io::read_u32le`), dans cet ordre :
   - `input_unreadable` : fichier absent, illisible ou non régulier, tailles incohérentes ;
   - `index_overflow_u32` : au moins $2^{32}-1$ points ;
   - `memory_budget` : 16 octets par point, admis avant l'allocation ;
   - `input_unreadable` : lecture incomplète, octet de trop.
5. Nuage, dans l'ordre de `prepare_cloud` : `empty_input`, `coordinate_out_of_domain`, `memory_budget` (tri),
   `duplicate_point_id`, `memory_budget` (tableaux du nuage). Positions répétées : `multiplicity_unsupported`, car la
   tour exige des sites de poids un.
6. $K>n$ : `parameter_out_of_range`. Le nombre de sites n'est connu qu'après la préparation du nuage, dont le
   `memory_budget` précède donc ce refus.
   **Décision $K=n$ pour `points` et `plat`** (tranche S9, reprise par S10 ; audit général `a65903a7b`, « avant L3 » ; réponse du développeur,
   [REPONSE_CLAUDE_SUPPORTS_20261004](../audits/REPONSE_CLAUDE_SUPPORTS_20261004.md), section G). Pour $K\geq 2$, la
   qualification des points à $m=K+1$ ne peut pas réussir quand $K=n$ : aucune composante n'atteint $K+1$ sites. La
   sortie `points` refuse donc $K\geq n$ dès $K\geq 2$ (`parameter_out_of_range`, à cette même étape, sans dossier ni
   `D.pending`), au lieu d'abaisser $m$ en silence ou de publier des points inactifs. $K=1$ reste admis à tout $n\geq 1$
   avec $m(1)=1$ (liaison simple, entrées à 0). `full` et `supports` gardent $K=n$ : l'unique naissance $B(X)$ y est
   correcte. `plat`, tirée de la même hiérarchie de points, suit la même règle. Portes : `mhgp11_cli_points` (refus à
   $n=2$, 5 et 8 ; $K=1$ admis à $n=1$ et 2), `mhgp11_points_unit_refusals`, `mhgp11_cli_plat` (refus à $n=2$ et 5 ;
   $K=1$ admis à $n=1$, tout bruit).
7. Calcul, notamment :
   - ressources : `memory_budget`, `node_budget`, `index_overflow_u32` (catalogue), `catalogue_counter_overflow`,
     `tower_capacity`, `radical_sign_budget` (`points` et `plat`) ;
   - dégénérescences : `wide_leaf`, `support_shell_capacity` (`supports`) ;
   - invariants : `catalogue_invariant`, `tower_invariant`, `supports_invariant`, `points_invariant`,
     `head_invariant`, `arithmetic_invariant`, `task_exception` (`sched`).
8. Écriture et publication (§ 9) : `output_unwritable` ; `output_conflict` si $D$ ou `D.pending` est apparu depuis le
   plan. L'API refuse aussi, avant toute création de fichier et toute écriture du rapport, le produit d'une autre
   `Session`, puis une provenance incohérente avec le nuage du produit (tailles différentes de $12n$ et $4n$ octets,
   budget déclaré nul ; § 8) : `parameter_out_of_range`. Le CLI n'a qu'une `Session` et remplit la provenance depuis
   sa lecture.
9. Après la publication : fermeture de la `Session` (`api::finish` : `budget_not_released`, code 3), puis écriture de
   la ligne d'état sur la sortie standard (`output_unwritable`, code 2). Un refus de cette étape retire le dossier
   publié (`api::withdraw`, `retract()`, § 9).

Entre deux refus d'un même étage, la fusion `merge` de `src/core/status.hpp` retient le plus petit K, puis la raison
placée la première dans `src/core/reasons.def`. Le refus ne dépend donc pas du nombre de fils.

**Raisons.** Toutes figurent dans `src/core/reasons.def` à `f98aeed67`, sauf six raisons annoncées. Chacune s'ajoutera
en fin de table, avec le code qui l'émet et la porte qui la provoque, dans l'ordre d'intégration de sa tranche. Le
tableau suit l'ordre prévu ; si l'intégration en suit un autre, c'est `reasons.def` qui fait foi, et le tableau et la
copie gravée de `tests/core/status_test.cpp` sont mis à jour au même commit :

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
L'intégration L1 du 5 octobre 2026 a suivi cet ordre : `support_shell_capacity` et `supports_invariant` (S6a), puis
`environment_selftest` (S5), sont en fin de `reasons.def`, avec la copie gravée de `tests/core/status_test.cpp`.
La tranche S8 (L3, 5 octobre 2026) a ajouté ensuite `radical_sign_budget`, au même commit que son émetteur
(`src/num/radical.cpp`, `src/num/big.cpp`) et ses portes (`mhgp11_num_big`, `mhgp11_num_radical`). La tranche S9 (L3,
5 octobre 2026) a ajouté `points_invariant`, au même commit que son émetteur (`src/points/`) et la copie gravée de
`tests/core/status_test.cpp` (30 raisons). La tranche S10 (L4, 5 octobre 2026) a ajouté `head_invariant`, au même
commit que son émetteur (`src/head/`) et la copie gravée (31 raisons).

**Codes de sortie** (`exit_code`, `src/core/status.hpp`) :
- 0 : conforme ;
- 2 : refus ;
- 3 : invariant violé, c'est-à-dire tout statut `invariant_violated`, `budget_not_released` et
  `environment_selftest` compris.

Un arrêt par signal est toujours un échec. Les codes 1 et 4 appartiennent aux portes, jamais à l'exécutable.

**Sortie standard.** Exactement une ligne JSON, en succès comme en refus. Forme fixée par S5 (porte
`mhgp11_cli_contract`), ici pour `full` :

```json
{"phase":"mhgp11","output":"full","status":"ok","reason":"none","coord_bits":21,"k":5,"workers":48,"sites":39885,"stages_ns":{"cloud":0,"index":0,"domain":0,"tree":0,"attach":0,"output":0,"write":0,"total":0},"peaks_bytes":{"cloud":0,"index":0,"domain":0,"tree":0,"output":0,"write":0},"counts":{"nodes":0,"births":0,"edges":0},"publication":"published_complete","manifest_sha256":"…"}
{"phase":"mhgp11","output":"full","status":"invalid_input","reason":"output_conflict","stage":"plan","coord_bits":21,"publication":"none","manifest_sha256":null}
```

- Succès : clés `phase`, `output`, `status` (`ok`), `reason` (`none`), `coord_bits`, `k`, `workers`, `sites`,
  `stages_ns` (`cloud`, `index`, `domain`, `tree`, `attach`, `output`, `write`, `total`), `peaks_bytes` (`cloud`,
  `index`, `domain`, `tree`, `output`, `write`), `counts`, `publication` (`published_complete`), `manifest_sha256`,
  dans cet ordre. Les `counts` de `full` sont les totaux des ordres 1 à K : `nodes`, `births`, `edges`. Ceux de
  `supports` (fixés par S7) : `nodes` ($N$), `balls` ($B$), `supports` ($S$), `prior` ($A$), égaux au manifeste.
  Ceux de `points` (fixés par S9) : `nodes` ($N$), `levels` ($L$), `plateaus` ($P$), `blocks` ($B_k$), `delayed`
  (sites à rival, $M\neq 0$), égaux au manifeste. Pour `points`, l'étage `output` est la pendaison et l'arbre de points
  (`points::hang`).
- Refus : clés `phase`, `output` (`null` à l'étape des options, le nom de la sortie ensuite), `status`, `reason`,
  `stage` (`options`, `plan`, `session`, `read`, `compute`, `publish`, `close`, `report`), `coord_bits`,
  `publication` (`none` ou `published_complete`), `manifest_sha256` (`null` ou l'empreinte du manifeste publié),
  toujours présentes, dans cet ordre. La ligne va sur la sortie standard si celle-ci est utilisable et l'accepte
  entière, sinon sur la sortie d'erreur (sortie fermée, en lecture seule, égale à une entrée, ou en échec, `/dev/full`
  compris).
- `SIGPIPE` et `SIGXFSZ` sont ignorés : un tube sans lecteur, une sortie pleine ou une limite de taille de fichier
  rendent une erreur d'écriture et un refus (`output_unwritable`), jamais un arrêt par signal qui laisserait un
  `D.pending` orphelin.

- Temps et nombre de fils vont dans cette ligne, **jamais dans le manifeste**.
- Étages de `stages_ns` :
  - `cloud` : lecture et préparation du nuage ;
  - `index` ;
  - `domain` : catalogue $\mathrm{Cat}_K$ ;
  - `tree` : forêts $1$ à $K$ de FULL pour `full` ; pour `supports` (L2b), FULL avec le journal sur l'ordre $K$ et
    l'extraction de l'ordre $K$ (`build_order_full`) ; arbre d'ordre K seul pour `points` et `plat` (`build_order`) ;
    pour ces trois sorties, sans le balayage du rattachement (durée moins `attach_ns`) ;
  - `attach` : rattachement des boules, diagnostic `attach_ns` de `build_order_full` ou de `build_order` (balayage du
    lemme D et contrôles du produit, tranche S3) ; nul pour `full` ; son pic est compté dans celui de `tree` ;
  - `output` : produit (supports et assemblage, hiérarchie de points ou tête plate) ; pour `supports`,
    `build_support_hierarchy` (pré-passe du plafond, postordre, passes count et fill) ; vide pour `full` ;
  - `write` : écriture et publication.
- En refus, la ligne porte le statut et la raison et, si un dossier reste publié (§ 9), l'état `published_complete`
  avec l'empreinte du manifeste publié (forme ci-dessus).
- La ligne est écrite **après** la publication et la fermeture de la `Session`. Si son écriture ou la vidange de la
  sortie standard échoue, le dossier publié est retiré (`retract()`, § 9) et l'appel rend `output_unwritable`
  (code 2), avec la ligne de refus sur la sortie d'erreur. Il n'y a donc jamais de code 0 sans ligne d'état. Un
  dossier ne reste publié après un refus que dans les doubles échecs du § 9, et l'état `published_complete` le déclare
  alors. C'est la frontière R2 de la v10.

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

## 6. `supports.mhgp11sp` : `MHGP11SP` version 2 (normatif)

**Décision de l'utilisateur du 6 octobre 2026.** Il ne faut surtout pas représenter tous les supports du niveau K, mais
seulement ceux de l'arbre couvrant minimal d'ordre K. Choix retenu : les arêtes de Kruskal, avec S\* seul.
- Seules les boules qui changent l'arbre d'ordre K sont publiées : **naissances et fusions**.
- Une liaison interne relie des K-parties déjà dans un même nœud ; elle fermerait un cycle, et elle est retirée.
- Chaque boule publiée porte **un seul support**, $S^*$, d'arité $q_{\min}$, lu dans le catalogue.
- $\mathcal{Q}_b$ n'est plus énuméré. Il n'y a donc plus de plafond de coquille à 24 sites, plus de brouillon de
  fermeture, et plus de comptes du lemme G dans la sortie.
- Seul plafond restant : $m\leq 255$ (colonne `u8`), sinon `support_shell_capacity`.
- La version 1, décrite ci-dessous pour mémoire, publiait $W_K$ entière et $\mathcal{Q}_b$ entier. L'assemblage la
  garde comme sélection `Selection::all` de `build_support_hierarchy`, pour les portes de la bibliothèque ; la sortie
  publiée utilise `Selection::spanning`. Le lecteur `bench/mhgp11_formats.py` relit les deux versions.

**Version 2 : différences avec la version 1.**
- En-tête : `version` = 2, et `S` = `B`.
- `BALLS` : plus de colonne `support_count` ; `role` vaut 0 (naissance) ou 1 (fusion).
- `SUPPORTS` : `arity u8[B]` ($q_{\min}$), puis `sites`, soit $S^*$ de chaque boule dans l'ordre des boules.
- Manifeste : fichier en `version` 2. Le bloc `counts` vaut `sites`, `nodes`, `births`, `merges`, `balls`, `roles`
  (`birth`, `merge`), `supports`, `arities`, `extended_shells` et `prior`. Les agrégats de $\mathcal{Q}_b$ et des
  liaisons internes (`internal`, `multi_support_balls`, `max_supports_per_ball`, `kparties_reliees`, `cofaces`) sont
  retirés.
- Tout le reste est inchangé : `SITES`, `NODES` (`ball_count` compte les seules boules publiées), `PRIOR`, l'ordre
  des boules, le rattachement, la signature `tree_k_sha256`.

**Version 1 (pour mémoire). Objet.** Les définitions et les preuves sont dans
[MATHEMATIQUES.md](MATHEMATIQUES.md), section 10.
- L'arbre $T_K$ est la forêt d'ordre K de FULL, dans sa numérotation canonique.
- Toutes les boules de $W_K=\lbrace b\in\mathrm{Cat}_K : p+q_{\min}-1\leq K\leq p+m\rbrace$ sont publiées : naissances,
  fusions et liaisons internes, événements faibles compris.
- Chaque boule est rattachée à $\mathrm{att}(b)$, le nœud vivant à la coupe **fermée** $\lambda_b$, après fermeture de
  tout le plateau.
- Son rôle se décide par les rangs.
- Ses branches $\mathrm{ant}(b)$, prises à la coupe **stricte** et dédupliquées, sont publiées pour le rôle fusion
  seulement.
- Tous ses supports positifs minimaux $\mathcal{Q}_b$ sont publiés, énumérés sur **toute** la coquille $U_b$.
- Plafond de coquille étendue : 24 sites, constante de compilation. Au-delà, l'appel entier est refusé, au plus tard
  par `support_shell_capacity` (la tour peut refuser avant, par `tower_capacity` ou `memory_budget`), et rien n'est
  publié.
- Calcul de $\mathcal{Q}_b$ : en deux passes, compter puis remplir, à positions fixes ; le brouillon de fermeture est
  admis dans le budget avant les tâches ([ARCHITECTURE.md](ARCHITECTURE.md), § 7.1). Un refus vaut pour l'appel
  entier, sans export partiel. Son coût est publié à part, dans l'étage `output` de la ligne d'état (§ 3).
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
| `gabriel_cofaces` | support : $\binom{m-\lvert Q\rvert}{t+1-\lvert Q\rvert}$, nul si $t+1<\lvert Q\rvert$ ; boule : $N_{t+1}$ |

- `kparties_reliees` est le compte mis en avant : le nombre de $K$-parties de $P_b$. Elles sont toutes reliées à la
  coupe fermée $\lambda_b$. Il vaut $K+1$ à une jonction régulière, 1 à une naissance régulière, 4 au carré à $K=3$.
- Les liaisons de la thèse (Prop. 5) sont les $(K+1)$-parties, comptées par `cofaces`.
- La somme des `cofaces` des supports d'une boule compte des incidences $(Q,G)$, pas des cofaces distinctes.
- La somme de `kparties_reliees` sur les boules compte des incidences $(b,F)$, pas des $K$-parties distinctes : sur la
  ligne $(0,0),(1,0),(2,0)$ à $K=2$, les trois boules donnent $1+1+3=5$ pour 3 paires.
- La somme des `cofaces` des boules compte des liaisons distinctes, car une liaison n'a qu'une boule minimale ; elle
  se limite aux boules de $W_K$, et n'est pas le nombre de toutes les liaisons de $\Gamma_K$.
- Les unions effectuées par le constructeur dépendent de l'ordre de traitement : elles ne sont jamais publiées.

**API native des primitives** (tranche S6a, `src/supports/supports.hpp`, intégrée en L1 le 5 octobre 2026 ;
l'assemblage est décrit plus bas, tranche S6b).
- `ball_supports` rend $\mathcal{Q}_b$ d'une boule du catalogue dans l'ordre publié, $S^*$ en tête, et sa fermeture
  $N_0..N_m$ (`Closure`, jamais stockée). $\mathcal{Q}_b$ ne dépend pas de $K$.
- `ball_shape` (ou `make_shape`) donne la forme $(p,m,q_{\min},K)$ ; `ball_counts` rend les comptes de la boule
  (`BallCounts` : `kparties_reliees`, `compressed_parts`, `strict_traces`, `cofaces`, `gabriel_cofaces`) ;
  `support_cofaces` et `support_gabriel_cofaces` ceux d'un support, nuls pour une arité hors de 2..4 ou supérieure à
  $m$.
- Refus : `parameter_out_of_range` pour un `BallIdx` hors du catalogue ; `support_shell_capacity` par `check_shell`,
  seul contrôle du plafond ($m>24$), pour la boule comme pour la pré-passe d'un appel entier ; `supports_invariant`
  pour un brouillon trop court ou plus de `out.size()` supports, une fermeture étrangère à la forme, une forme hors du
  domaine de `Shape` (dont une boule hors de $\mathrm{Cat}_K$), et les gardes d'invariant du catalogue sans porte
  possible ([provenance](PROVENANCE.md), section supports). Un tel refus signale un domaine corrompu, jamais une
  entrée.
- La coquille régulière ($m=q_{\min}$) rend $\lbrace S^*\rbrace$ sans revalider ses `SiteIdx` : le catalogue fait
  foi (G2).
- `SupportLedger` est un registre de mesure, jamais une décision, et il n'est pas protégé : un registre par fil,
  sommé par `SupportLedger::add` après la jointure.

**Assemblage** (tranche S6b, `build_support_hierarchy(const OrderTree&, MemoryBudget&, sched::Pool*,
HierarchyTimings*)`, 5 octobre 2026, qualification G4 en attente). Il rend `SupportHierarchy`, la hiérarchie en
mémoire que l'écrivain `MHGP11SP` (S7) sérialisera.
- Contenu : `post` et `subtree_size` par `NodeIdx` ; `ball_offsets` ($N+1$, indexés par rang de postordre) ; les
  boules de $W_K$ dans l'ordre (postordre du nœud de rattachement, rang, `BallIdx`) avec leurs comptes en mémoire
  (`Ball` : `kparties_reliees`, `compressed_parts`, `strict_traces`, `cofaces`, `gabriel_cofaces`, `components`) ;
  `support_offsets` et `supports` dans l'ordre publié, `support_cofaces` (incidences par support) ;
  `prior_offsets` et `prior`, rôle fusion seulement ; le registre `SupportLedger` de la passe count. Rien de cela
  n'entre tel quel dans le fichier : seules les colonnes de la table ci-dessus y sont écrites.
- Étapes : pré-passe du plafond sur **toutes** les coquilles étendues de $W_K$, avant toute allocation ; admission ;
  postordre itératif depuis la racine, enfants par `NodeIdx` croissant, sans pile ; tri des boules par seaux stables ;
  passe count parallèle ($\mathcal{Q}_b$, comptes, contre-épreuve $S_{\mathrm{journal}}=\binom{m}{t}-N_t$) ; sommes
  préfixes vérifiées des décalages `u64` ; passe fill à positions fixes (même position, même `BallIdx` ; même nombre
  de supports qu'en count, sinon `supports_invariant`). Sorties identiques à l'octet quel que soit le Pool.
- Admission (formule `HierarchyAdmission`, recalculée par la porte) : avant la passe count, les sorties connues, un
  temporaire de $4B$ octets et, **par fil actif**, le registre, le brouillon de fermeture ($8\cdot 2^{w-6}$ octets
  pour la plus grande coquille étendue $w$ de $W_K$, 2 Mio à $w=24$) et la liste temporaire de supports
  (`support_capacity(w)` entrées de `sizeof(Support)`, au plus 12 926) ; avant la passe fill, les supports et leurs
  incidences. Un refus d'admission précède toute allocation.
- Refus : `support_shell_capacity` pour l'appel entier si une boule de $W_K$ a $m>24$, avant tout calcul de
  $\mathcal{Q}_b$ et sans rien allouer ; `memory_budget` ; `supports_invariant` (contre-épreuve du journal, nombre de
  supports entre les passes, et gardes d'arbre et de rattachement sans porte possible) ; ceux des primitives et du
  Pool. Aucun résultat partiel ; les diagnostics `HierarchyTimings` ne sont écrits qu'au succès.

**Écrivain et lecteur** (tranche S7, 5 octobre 2026). `src/api/write_supports.cpp` écrit ce format à neuf, sans
source portée : tailles et décalages calculés avant l'écriture et contrôlés à chaque section, valeurs par paquets sur la
pile, `SITES.point_id` égal à l'unique `PointId` du site (poids un). Le lecteur `bench/mhgp11_formats.py`
(`read_supports`, classe `SupportsFile`) dérive tout ce qui est déclaré dérivé ci-dessus et contrôle tout ce qui suit ;
`check_directory` recompte les agrégats du manifeste (§ 8) et recalcule `tree_k_sha256` depuis le fichier seul. Les
prédicats exacts sont écrits en entiers : centre $C/D$ ($D>0$) et rayon carré $\mathrm{num}/D^2$ de la sphère de
$S^*$, puis égalités et signes stricts, ce qui équivaut aux prédicats en `Fraction`. $N_j$ est compté sur la réunion des
supports d'une boule (parties sans support, par branchement), puis complété par les sites de coquille hors de tout
support, interchangeables. Le lecteur exige aussi la numérotation canonique (naissances par (niveau, centre), fusions
par (niveau, plus petite naissance)), des rangs cohérents avec les niveaux exacts (égalité et ordre), les sites en ordre
de Morton strict, et l'ordre (rang, $S^*$) des boules d'un nœud, $S^*$ complété par `kNone` : c'est l'ordre des
`BallIdx` du catalogue (remarque M1 ci-dessus), que la mémoire de l'assemblage suit. Portes :
`mhgp11_cli_supports_oracle` (le fichier lu égale le vidage canonique de l'oracle S1), `mhgp11_cli_supports_scale*`,
`mhgp11_cli_supports_lidar_*` (`tests/cli/tests.cmake`).

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
  ce support porteur, et aucune stabilité n'est revendiquée. `kparties_reliees` ne dépend que de $(p,m,K)$ : à
  $(p,m,K)$ fixés, il ne voit pas ce saut. Il n'est pas stable pour autant : sortir un site de la coquille change $m$
  sans changer ni la boule ni $\mathcal{Q}_b$, et le compte d'une boule diamétrale passe ainsi de 3 à 1 à $K=2$
  (section 10.9 de MATHEMATIQUES).
- **Elle omet** les sites intérieurs et, sur une coquille étendue, les sites de coquille hors de tout support. Exemple :
  dans le triangle droit, l'origine est orpheline.
- **Elle n'est pas le $K$-polyèdre de la thèse** (Déf. 21), qui est un ensemble de points. Celui-ci reste
  reconstructible hors format, à partir du catalogue, par le lemme H. À $K\geq 2$, c'est l'union des $P_b$ des boules
  de $W_K$ rattachées au sous-arbre et de niveau au plus $a$ ; à $K=1$, c'est l'ensemble des feuilles du sous-arbre,
  une feuille ne possédant aucune boule.
- Elle n'est pas non plus $\mathrm{conv}(U_b)$. Les réalisations de deux branches peuvent se recouvrir, et leurs
  intersections ne donnent pas la connectivité de FULL.

## 7. `points.mhgp11pt` : `MHGP11PT` version 1, et `etiquettes.mhgp11et` : `MHGP11ET` version 1 (normatifs)

**`points.mhgp11pt` : `MHGP11PT` version 1** (tranche S9, L3 ; écrivain `src/api/write_points.cpp`, lecteur
`read_points` de `bench/mhgp11_formats.py`). Conventions du § 4 : petit-boutiste, colonnes alignées sur 8 octets,
bourrage nul, une colonne vide n'occupe aucun octet. Les conventions $m(1)=1$ et $\kappa=1$ sont celles de
[HIERARCHIE_POINTS.md](HIERARCHIE_POINTS.md), § 9 ; l'objet est $H^{r}_{K+1}$ (§ 3 du même document).

| Section | Colonnes, dans cet ordre |
| --- | --- |
| En-tête (144 octets) | magie `MHGP11PT` ; 17 × `u64` : `version=1`, `coord_bits`, `k`, `m` (qualification : 1 si $K=1$, $K+1$ sinon), `kappa=1`, `n`, `L` (rangs référencés), `W` (mots par niveau : 3 en u18 et u21, 4 en u24, comme l'export `MHGP11PH`), `N`, `P` (plateaux), `Bk` (blocs), décalages de `SITES`, `LEVELS`, `NODES`, `HANGING`, `TREE`, taille totale |
| `SITES` | `x u32[n]`, `y u32[n]`, `z u32[n]`, `point_id u32[n]`, comme `MHGP11SP` |
| `LEVELS` | `rank u32[L]` (strictement croissants, le rang 0 compris) ; `num u64[W·L]` ; `den u64[W·L]` : valeur exacte **non réduite** $\ell=\mathrm{num}/\mathrm{den}$ de chaque rang référencé, $W$ mots petit-boutistes par valeur, niveau après niveau |
| `NODES` | `parent u32[N]` (`kNone` à la racine), `rank u32[N]` : même numérotation que `MHGP11FUL1` et `MHGP11SP` |
| `HANGING` (par `SiteIdx`) | `t u32[n]`, `M u32[n]`, `Q u32[n]` (date $\sqrt{\ell_t}+\sqrt{\ell_M}-\sqrt{\ell_Q}$ ; $M=Q=0$ sans rival) ; `owner u32[n]` ; `floor u32[n]` ; `strict u8[n]` |
| `TREE` | `plateau_t u32[P]`, `plateau_M u32[P]`, `plateau_Q u32[P]` ; `block_plateau u32[Bk]` ; `block_parent u32[Bk]` (`kNone` à une racine) ; `site_block u32[n]` ; `site_plateau u32[n]` |

Sens des colonnes (port de `Hanging`, `bench/points_hierarchy.py`, et de `PointTree`, `bench/points_flat.py`) :
- rangs référencés : ceux de toutes les colonnes de rangs (`NODES.rank`, `t`, `M`, `Q`, `floor`, plateaux) et le rang 0
  (niveau nul) ; un lecteur obtient tout niveau publié depuis le fichier seul ;
- `owner` : nœud vivant à la date à la coupe fermée, plus haut ancêtre du point de première couverture qualifiée de
  rayon de naissance au plus la date ; `floor` : plus grand rang du catalogue $\mathrm{Cat}_K$ de niveau au plus le carré
  de la date ; `strict` : la date est strictement entre deux niveaux ;
- arbre de points : plateaux strictement croissants en valeur exacte ; un plateau de niveau du catalogue vaut
  $(r,0,0)$, un plateau de date $(t,M,Q)$ avec $M>Q$ (celle du premier site du groupe en ordre des `SiteIdx`). Un
  bloc naît d'une fusion d'au moins deux blocs non vides (ses enfants : les blocs dont il est le parent) ou d'une
  entrée ; au plus $2n-1$ blocs. Un site non strict entre au plateau $(\mathrm{floor},0,0)$, un site strict au plateau
  de sa date exacte.

Le lecteur contrôle : en-tête et disposition ; sites ; rangs et niveaux strictement croissants (l'ordre des rangs est
celui des niveaux) ; arbre bien formé (racine en dernier, naissances puis fusions d'au moins deux enfants) ;
$t\leq Q<M$ et $t\leq\mathrm{floor}\leq M$ avec rival, $\mathrm{floor}=t$ non strict sans rival ; propriétaire vivant
au plancher ; forme des blocs et des plateaux ; et, en lecture exacte (sommes de racines par classes de carrés puis
encadrements entiers, bibliothèque standard), le plancher et le drapeau strict de chaque site, l'égalité de la date
d'un site strict à celle de son plateau, et l'ordre strict des plateaux. Il ne peut pas vérifier la maximalité du
plancher (le rang suivant du catalogue n'est pas publié), ni le choix du rival : ils relèvent de l'oracle et du
différentiel natif (`mhgp11_points_oracle`, `mhgp11_points_vs_python`).

`tree_k_sha256` n'est pas recalculable depuis `MHGP11PT`, qui ne porte pas $S^*$ : le manifeste le publie, et
`mhgp11_cli_points` l'exige égal à celui de `--sortie=supports` à même entrée et même $K$.

**`etiquettes.mhgp11et` : `MHGP11ET` version 1** (tranche S10, L4 ; écrivain `src/api/write_flat.cpp`, lecteur
`read_flat` de `bench/mhgp11_formats.py`).
- En-tête de 56 octets, petit-boutiste : magie `MHGP11ET`, puis six `u64` : `version=1`, `n_points`, `k`, `mcs`, `z`,
  `selection` (0 pour EOM, 1 pour les feuilles).
- Puis `labels i64[n_points]` (complément à deux), **dans l'ordre du fichier d'entrée**, sans bourrage : la taille vaut
  $56+8n$ exactement. Une étiquette vaut le plus petit `PointId` du cluster retenu (de 0 à $2^{32}-1$), ou $-1$ pour le
  bruit.
- Les compteurs vont dans le manifeste.
- Le port n'emploie pas `out[pt.ids]` (`bench/points_flat.py`), qui suppose des `PointId` denses : les étiquettes sont
  rangées par `PointId` (`head::in_input_order`).
- Sens : condensation au critère A de l'arbre de points de `MHGP11PT` (même calcul, même $K$), sélection EOM N-aire à
  $\varphi(r)=r^{-z}$ ou feuilles, racine exclue, parent sur égalité **certifiée** ; décisions par encadrements entiers
  puis repli exact ; au-delà de son budget, refus `radical_sign_budget` de l'appel entier ([sortie plate](SORTIE_PLATE.md),
  § 5).

Le lecteur contrôle l'en-tête (version, $1\leq K\leq 12$, mcs $\geq 2$, $z$ dans 1..3, sélection), la taille exacte, les
étiquettes dans $[-1,2^{32}-1]$, et, avec le manifeste, l'égalité de l'en-tête aux paramètres et du nombre de $-1$ au
compte `noise`. La règle du plus petit `PointId` exige les entrées : `mhgp11_cli_plat` la contrôle.

## 8. Manifeste `manifeste.json`

Le manifeste est écrit **en dernier**, après la fermeture contrôlée de tous les fichiers de données (§ 9). C'est le
seul témoin d'achèvement.

Règles de forme :
- un objet JSON, clés dans l'ordre fixe ci-dessous, entiers en décimal, aucun flottant ;
- aucun temps ni nombre de fils : le manifeste est identique à l'octet quel que soit W ;
- sa forme exacte en octets est fixée par S5 : clés dans l'ordre ci-dessous, sans espace, entiers en décimal, saut de
  ligne final ; c'est la forme canonique de `json.dumps(objet, separators=(',', ':'))`, contrôlée à l'octet par le
  lecteur `bench/mhgp11_formats.py` ;
- l'écrivain JSON appartient à `api` (`manifest.cpp`), et non à `io`.

Exemple pour `supports`, présenté ici sur plusieurs lignes pour la lecture :

```json
{"schema":"ehgp.v11.output.v1","output":"supports","status":"complete","public_status":"not_claimed",
 "coord_bits":21,"k":5,"parameters":{"budget_bytes":null,"grid_step":null,"origin":null},
 "inputs":[{"name":"points","bytes":0,"sha256":"…"},{"name":"ids","bytes":0,"sha256":"…"}],
 "files":[{"name":"supports.mhgp11sp","format":"MHGP11SP","version":2,"bytes":0,"sha256":"…"}],
 "tree_k_sha256":"…",
 "counts":{"sites":0,"nodes":0,"births":0,"merges":0,"balls":0,"roles":{"birth":0,"merge":0},
           "supports":0,"arities":{"2":0,"3":0,"4":0},"extended_shells":0,"prior":0}}
```

- `parameters` :
  - `budget_bytes` : l'entier donné à `--budget`, ou `null` sans plafond ;
  - `grid_step` : la chaîne de `--pas` recopiée, ou `null` ; `origin` : le tableau des trois chaînes de `--origine`,
    ou `null` ;
  - pour `plat`, s'y ajoutent `mcs`, `z` et `selection`.
- `inputs` : taille et SHA-256 des deux fichiers d'entrée, pris au fil de la lecture. `files` : taille et SHA-256 de
  chaque fichier de données, pris au fil de l'écriture.
- Cohérence exigée par la publication (audit `abc30ed06`) : pour un produit de $n$ points, `inputs` porte $12n$ et
  $4n$ octets, et `budget_bytes` est `null` ou strictement positif. L'API refuse toute autre provenance
  (`parameter_out_of_range`, étape 8 du § 3) ; le CLI la remplit depuis sa lecture. Les empreintes déclarées ne sont pas
  vérifiables depuis la provenance seule. La porte `mhgp11_api_publish_reader` fait relire par le lecteur officiel un
  dossier publié par l'API, et refuse sans sortie les provenances incohérentes.
- `counts` de `supports` (version 2, arbre couvrant d'ordre K, depuis le 6 octobre 2026), dans cet ordre :

  | Clé | Sens |
  | --- | --- |
  | `sites`, `nodes` | $n$ et $N$ |
  | `births`, `merges` | nœuds de `kind` 0 ou 1, puis nœuds de `kind` 2 |
  | `balls`, `roles` | $B$, puis ses boules par rôle : `birth` (une par nœud de naissance si $K\geq 2$, aucune à $K=1$) et `merge` |
  | `supports`, `arities` | $S=B$, puis les $S^*$ par arité |
  | `extended_shells` | boules à coquille étendue, $m>q_{\min}$ |
  | `prior` | $A$ |

  La version 1 portait aussi `roles.internal`, `multi_support_balls`, `max_supports_per_ball`, `kparties_reliees` et
  `cofaces`. Ce sont des agrégats de $\mathcal{Q}_b$ et des liaisons internes, qui ne sont plus publiés.
- `counts` de `full` (fixés par S5) : `sites`, `points`, puis `orders`, une entrée par ordre $k=1..K$ :
  `{"k","births","nodes","edges","root"}`, les en-têtes d'ordre de `MHGP11FUL1`. Son fichier est déclaré
  `{"name":"full.mhgp11ful1","format":"MHGP11FUL1","version":1,…}`.
- `counts` de `points` (fixés par S9), dans cet ordre, tous recomptés par le lecteur depuis `MHGP11PT` : `sites`
  ($n$), `nodes` ($N$), `qualification` ($m$), `kappa` (1), `levels` ($L$), `plateaus` ($P$), `blocks` ($B_k$),
  `root_blocks` (blocs sans parent), `delayed` (sites à rival), `strict` (sites strictement entre deux niveaux). Son
  fichier est déclaré `{"name":"points.mhgp11pt","format":"MHGP11PT","version":1,…}`.
- `counts` de `plat` (fixés par S10), dans cet ordre : `sites` ($n$), `nodes` ($N$), `clusters` (clusters condensés,
  racine virtuelle comprise), `selected` (clusters retenus), `noise` (points étiquetés $-1$), `decisions` (comparaisons
  EOM), `exact` (décisions tranchées par le repli exact), `equalities` (égalités certifiées, le parent l'emporte),
  `unbracketed` (plateaux sans encadrement utile). Son fichier est déclaré
  `{"name":"etiquettes.mhgp11et","format":"MHGP11ET","version":1,…}` ; ses `parameters` portent en plus `mcs`, `z` et
  `selection` (`"eom"` ou `"feuilles"`).

**`tree_k_sha256`**, signature de l'arbre d'ordre K, version 2 (réponse D.2 de l'auditeur, `aef7182b3`). C'est le
SHA-256 de la suite d'octets suivante, sans bourrage, entiers petit-boutistes :
- les 8 octets ASCII `MHGP11TK`, puis cinq `u64` : la version de la signature, 2 ; `coord_bits` ; $K$ ; $n$ ; $N$ ;
- les 32 octets bruts du SHA-256 de la géométrie : 8 octets ASCII `MHGP11GX`, `coord_bits` et $n$ en `u64`, puis
  $x$, $y$, $z$ en `u32` pour chaque site, dans l'ordre des `SiteIdx`. Aucun `PointId` n'y entre ;
- pour chaque nœud, dans l'ordre canonique : `parent` et `rank` en `u32` (`kNone` à la racine) ; `kind` et l'arité
  $a$ de sa naissance en `u8` ; les $a$ lignes de `SITES` de sa naissance en `u32`, croissantes ; le nombre d'enfants
  en `u32`, puis les enfants, croissants, en `u32`. La naissance d'une feuille de $K=1$ est la ligne de son site
  ($a=1$) ; celle d'une naissance de boule est $S^*$ ($a=q$) ; une fusion n'en a pas ($a=0$).

Propriétés :
- Elle ne contient ni `PointId` ni `BallIdx` : un réétiquetage ou une permutation de l'entrée ne la change pas.
- Elle se recalcule depuis le seul `MHGP11SP` : `SITES`, `NODES`, puis $S^*$ comme premier support de la première
  boule propre d'un nœud de `kind` 1, et la ligne de site d'une feuille de $K=1$ par l'ordre lexicographique (§ 6).
- Le moteur la calcule pour les quatre sorties, `full` compris, sur l'ordre maximal $K$ ; $S^*$ vient alors du
  catalogue. Elle est commune aux quatre sorties d'une même entrée et d'un même $K$.
- Elle ne se recalcule pas depuis le seul `MHGP11FUL1`, qui porte les centres des naissances et non $S^*$ ; les octets
  de `MHGP11FUL1` ne changent pas pour elle.
- C'est une empreinte structurée, qui sert à apparier des sorties : ni un certificat de correction géométrique, ni une
  preuve d'absence de collision. Le SHA-256 du fichier, dans `files`, reste une autre clé.
- Le schéma `ehgp.v11.output.v1` du manifeste fixe la version 2 de la signature. La version 1 de la spécification,
  qui reposait sur le `BallIdx` des naissances, absent du format, n'est jamais publiée.
- Implémentée par S5 (`api::tree_k_sha256`). Le champ publié est jugé par `mhgp11_api_session_tree_digest`
  (sérialisation indépendante, $K=1$ à 4, et valeurs gravées de l'auditeur par profil) et par
  `mhgp11_cli_tree_signature` (champ publié par l'exécutable contre une sérialisation en bibliothèque standard des
  trois fixtures de l'auditeur, à $K=1$ et $K=2$).
- Sortie `supports` (S7) : le moteur la calcule sur l'arbre d'ordre K seul (`OrderTree::forest`, même forêt que
  `FullTower::order(K)`, porte I10). Le lecteur la recalcule depuis `MHGP11SP` (`SupportsFile.tree_signature`) et
  `check_directory` l'exige égale au champ publié. `mhgp11_cli_supports_oracle` l'exige égale entre `--sortie=full` et
  `--sortie=supports` à $K=1$ à 4 sur 810 ordres ; les portes d'échelle et LiDAR, à $K=5$.

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
   - Un commit refusé est définitif. `manifest_sha256()` rend l'empreinte du manifeste dès que celui-ci est fermé,
     même si une étape ultérieure échoue (D.3 de l'auditeur). Corrigé par S5 : `commit_steps` l'affecte juste après
     la fermeture du manifeste, avant le renommage (porte `mhgp11_io_transaction_noreplace`, mutant
     `empreinte_manifeste_apres_publication`) ; elle ne dit pas à elle seule que $D$ est publié, `committed()` le
     dit.
5. **Sans commit réussi**, le destructeur ferme les fichiers. Il retire ceux que l'objet a créés, puis `D.pending`
   s'il l'a créé. Il ne touche jamais $D$. Il ne retire jamais un `D.pending` qu'il n'a pas créé.
   - Un `D.pending` orphelin, laissé par un arrêt brutal, fait refuser l'appel suivant (`output_conflict`). Il n'est
     jamais retiré automatiquement.
   - Un arrêt brutal ne laisse jamais un $D$ partiel.
6. **Retrait après publication** : `retract()`, frontière R2.
   - Le CLI l'appelle sur tout refus constaté après le commit (§ 3, étape 9) : fermeture de la `Session` en échec
     (`budget_not_released`), ou ligne de sortie standard impossible à écrire.
   - $D$ est renommé en `D.pending` sans remplacement, puis le destructeur le retire comme un dossier jamais publié.
     L'objet ne publie plus rien ensuite.
   - Refus : rien n'est publié (`output_unwritable`), ou le renommage échoue (`output_unwritable`, ou
     `output_conflict` si un `D.pending` est apparu entre-temps). $D$ reste alors publié et complet.

**Les doubles échecs.** Ce sont les seuls cas où un refus laisse un dossier publié ; il est alors complet, et son
manifeste en fait foi.
- La synchronisation du parent échoue après le renommage. La publication est d'abord défaite : $D$ est renommé en
  `D.pending`, que le destructeur retire, et le refus est `output_unwritable`. Si ce retour échoue aussi, $D$ reste
  publié et complet ; `committed()` rend alors vrai et `commit` rend `output_unwritable`.
- `retract()` échoue après un refus de l'étape 9 du § 3 : la ligne de sortie standard (`output_unwritable`,
  code 2) ou la fermeture de la `Session` (`budget_not_released`, code 3).

**État publié distinct** (réponse D.3 de l'auditeur, `aef7182b3`). Dans ces cas, le code de sortie reste celui du
refus, mais l'appel déclare l'état `published_complete`, distinct d'un refus sans publication : dans le résultat de
l'API, et dans la ligne de refus, sur la sortie standard si elle peut être écrite, sinon sur la sortie d'erreur. Il y
joint l'empreinte du manifeste publié, que `manifest_sha256()` conserve (étape 4). Ce n'est ni un succès de
durabilité ni une sortie partielle. Forme fixée par S5 : l'API rend `api::Publication` (issue, état, empreinte) par
`publish`, `withdraw` et `finish`, et la ligne de refus finit par `publication` et `manifest_sha256` (§ 3). Les trois
doubles échecs sont joués, localement et sur G4 :
- synchronisation du parent puis retour arrière en échec, retrait de l'API refusé : bibliothèque préchargée de test
  `tests/cli/io_fault_preload.cpp`, hors sanitizers, dans `mhgp11_cli_contract` ; elle ne décode que
  `SYS_renameat2`, avec ses cinq arguments typés, et tout autre appel `syscall` invalide explicitement la porte ;
- ligne d'état en échec puis retrait refusé : tube plein, puis `D.pending` créé dès que $D$ apparaît, sans crochet,
  dans `mhgp11_cli_contract` ;
- fermeture de la `Session` en échec puis retrait refusé : `mhgp11_api_session_after_publish`, au niveau de l'API,
  car le CLI ne peut pas la provoquer.

Limites :
- la durabilité est celle que donnent les `fsync` ;
- les tampons de `stdio`, au plus 64 Kio par fichier ouvert, sont hors `MemoryBudget` ;
- le contrat reste celui de la trame entière en mémoire ([ARCHITECTURE.md](ARCHITECTURE.md), § 7.2).

## 10. Invariance et déterminisme

- Les fichiers de données et le manifeste sont identiques à l'octet quel que soit le nombre de fils (portes de
  déterminisme à W1, W2, W4 et W48 : `mhgp11_cli_full_determinism` ; W1, W8 et W48 à 8 000 points sur G4).
- Une **permutation** de l'entrée donne des fichiers `full`, `supports` et `points` identiques ; les étiquettes de
  `plat`, rangées dans l'ordre d'entrée, sont permutées de même. Le manifeste ne change que par les empreintes des
  entrées et, pour `plat`, celle du fichier d'étiquettes.
- Un **réétiquetage injectif** des `PointId` (non dense, $2^{32}-1$ compris) ne change que les `PointId` publiés : la
  colonne `SITES.point_id` de `MHGP11SP` et `MHGP11PT`, les `PointId` par site de `MHGP11FUL1`, et les étiquettes de
  `plat`, recalculées par leur règle (plus petit `PointId` du cluster) sur les nouveaux identifiants. Les fichiers ne
  sont donc pas identiques à l'octet : on compare les structures après transport des identifiants, et les empreintes
  de provenance du manifeste se recalculent. `tree_k_sha256` ne change pas.
- Une **translation entière** laisse invariants la numérotation canonique (colonnes de `NODES`), les rangs, les rôles,
  les listes propres comme ensembles et les ensembles $\mathcal{Q}_b$, à la translation près : naissances et fusions
  se trient par des niveaux et des centres comparés dans l'ordre lexicographique, que la translation conserve. Seuls
  les ordres qui passent par les `SiteIdx`, rangs de Morton, peuvent changer : lignes de `SITES`, $S^*$, ordre des
  boules de même rang dans un nœud, ordre des supports d'une boule.
- Une **réflexion** ou un **échange d'axes** transporte l'objet, mais peut changer la numérotation canonique : seuls les
  ensembles sont alors invariants (nœuds comme ensembles de sites, supports comme ensembles de positions, niveaux).
- Les rotations ne sont pas couvertes : la quantification au millimètre casse l'équivariance.

## 11. Livraison, décisions écrites d'avance

| Livraison | Session | Tranches | État au 4 octobre 2026 |
| --- | --- | --- | --- |
| L0 | aucune | S0 (ce document, MATHEMATIQUES section 10, registre des preuves), S1 (oracle borné des supports) | en cours |
| L1 | G4 n° 1 | S2 (en-tête public de la tour), S3 (`build_order`, journal des graines, `WindowAttachment`, juge E2), S6 (module `supports`) | S2 livrée (`257aabb92`) |
| L2 | G4 n° 2 | S4 (`io`, avec `retract()`), S5 (`api`, `Session`, manifeste, `--sortie=full`), S7 (`--sortie=supports`, écrivain et lecteur `MHGP11SP`, mesure appariée) | S4 livrée (`f98aeed67`) ; S5 et S7 commitées localement le 5 octobre, qualification G4 et mesure en attente |
| L2b | conditionnelle, déclenchée | journal des graines posé dans `build_full` | règle évaluée le 5 octobre (`livrer_L2b`) ; L2b livrée (`0810962ac`, `311ef5e3c`), **qualifiée sur G4** le 6 octobre (reçu `developpement_20261005/qualification_finale`) |
| L3 | G4 n° 3 | S8 (`num`), S9 (`--sortie=points`) | S8 et S9 livrées le 5 octobre, **qualifiées sur G4** le 6 octobre (même reçu) |
| L4 | G4 n° 4, reportable | S10 (`--sortie=plat`) | S10 livrée le 5 octobre, **qualifiée sur G4** le 6 octobre (même reçu) |
| L5 | facultative | S11 (pipeline à un ordre), chantier 100 ms | — |

- Valeurs de `--sortie` admises : aucune avant L2 ; `full` puis `supports` en L2 ; `points` en L3 ; `plat` en L4.
  Toute autre est refusée `parameter_out_of_range`. `full` est admise par le code depuis l'intégration de S5
  (5 octobre 2026), `supports` depuis S7 (5 octobre 2026) ; leur qualification relève de la session G4 de L2.
  `points` est admise depuis S9 (5 octobre 2026) ; sa qualification relève de la session G4 de L3. `plat` est admise
  depuis S10 (5 octobre 2026) ; sa qualification relève de la session G4 de L4.
- Un commit natif des tranches S3, S5 et S6, dont les brouillons ont été écrits en parallèle de L0, exige
  l'intégration des réponses de l'auditeur mathématique (faite pour `aef7182b3`) et les portes de la tranche ; sa
  qualification exige la matrice G4 et un reçu. La relecture du contrat S0 est demandée aux deux auditeurs : leurs
  remarques sont intégrées comme corrections dès leur publication.
- Chaque tranche native se clôt par la matrice G4 et un reçu immuable.
- Aucune mesure ne promeut un statut public.

**Règle de décision de L2**, fixée le 4 octobre 2026, avant toute mesure.
- *Mesure.* Elle est appariée et se fait dans la session G4 gardée de L2. On relève l'étage `tree` de la ligne de
  sortie standard pour `--sortie=supports` (arbre d'ordre K seul par `build_order`, voie par lots) et pour
  `--sortie=full` (forêts $1$ à $K$ par `build_full`).
- *Configurations.* Trames ng00, ng01 et ng02 (séquence 08, sans sol, grille de 1 mm), $K=5$, W1 et W48, trois prises
  par configuration, avec des sorties identiques entre les prises. $K=10$ est mesuré une fois, sur ng00. Une prise
  dont les fichiers ou le manifeste diffèrent des autres invalide la mesure : c'est un défaut, à corriger avant toute
  décision.
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
- *Décision* (reçu `receipts/developpement_20261005/qualification_sorties`) : rapports supports/full de l'étage `tree`
  à W48 de 1,21, 1,18 et 1,09 ; une seule trame sous 1,10, donc **L2b est livrée**.
- *Livraison L2b* (5 octobre 2026, commit local) :
  - `build_order_full` (`src/tower/order_tree.cpp`) ;
  - journal posé par `OrderLog` dans `build_forests` et `build_concurrent` ; la voie pipeline n'en tolère qu'un ;
  - voie `full_tower` de `api_detail::compute_supports`.
  - Portes :
    - `mhgp11_tower_order_full` : identité avec `build_order` sur sept voies de FULL, dont le pipeline à W12 ;
    - `mhgp11_api_supports_route_oracle` : nuages de l'oracle, W1 et W3 ;
    - `mhgp11_api_supports_route_scale*` et `mhgp11_api_supports_route_lidar_*_k5` : W1 et W4 ;
    - cinq mutants du manifeste `tower`.
  - Restent sur G4 : TSan sur `mhgp11_tower_pipeline` et `mhgp11_tower_order_full`, campagne des mutants `tower`,
    mesure `bench/sorties_g4.py` refaite.
- La voie pipeline à un ordre (S11, L5) ne devient la voie par défaut que sur reçu.
- *Outil* (S7) : `bench/sorties_g4.py` joue cette mesure (trames, W, passes à froid puis prises à chaud, `full` et
  `supports` alternés, étages `tree`, `attach`, `output`, `write` relevés à part, identité des fichiers et des
  manifestes entre prises et entre W, `tree_k_sha256` commun) et rend un document JSON prêt pour un reçu, avec la
  décision de la règle, évaluée seulement à W48 sur les trois trames avec trois prises et des sorties identiques. Un
  essai local (W1, W4) valide l'outil, jamais la règle.

**Clause de report de `plat`.** La livraison L4 (tranche S10) peut être reportée.
- *Qui décide.* L'utilisateur, par une décision écrite que le développeur consigne dans le [README](../README.md) et
  dans une note aux auditeurs.
- *Quand.* Tant que le § 3.4 de [SORTIE_PLATE.md](SORTIE_PLATE.md) porte la mention « Ouvert », c'est-à-dire tant qu'il
  manque une règle qui garde les séparations fugaces sans déchiqueter les objets à lignes de balayage. L'expérience
  E1-bis du § 2, point 2 (V1, V2, $\kappa=2$, `first`, `cover`), n'est pas cette condition.
- Le tokenizer de `Zoltan/FoundationModel/` n'a pas l'usage de cette sortie : sa condensation est à seuil relatif
  (`Zoltan/FoundationModel/SPECIFICATION.md`, § 2.2).
- L4 a été engagée le 5 octobre 2026, dans l'ordre choisi par l'utilisateur (supports, points, puis plat) : aucun
  report n'a été décidé. La sortie publie la ligne LiDAR du § 3.4 de [SORTIE_PLATE.md](SORTIE_PLATE.md) par défaut
  (EOM, $z=1$, mcs 20), $z=2$ et les feuilles par option ; la mention « Ouvert » de ce paragraphe porte sur la règle de
  sélection, pas sur la sortie. Le banc Python exact (`bench/points_flat.py`, porte `bench/points_flat_gate.py`) sert
  toujours aux comparaisons avec HDBSCAN, et de référence au différentiel `mhgp11_head_vs_python`.
- Le report ne retire aucune exigence : quand L4 sera engagée, ses portes resteront celles de la spécification.
