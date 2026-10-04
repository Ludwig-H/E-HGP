# L0, tranche S0 : contrat produit et documentaire, réponse à l'auditeur (clé s0_docs)

4 octobre 2026, 21 h 03 UTC (heure lue par `date -u`). Agent `s0_docs` du workflow `wf_9ab81c6b-048`. Worktree
`build/v11-impl-l0`, détaché à `f98aeed67`. Rien n'est commité ; GCP non utilisé ; aucun build ni test natif.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

## 1. Fichiers

Chemins relatifs à `morsehgp3D_v11/`. Empreintes sha256 (16 premiers chiffres) à 21 h 04 UTC.

| Fichier | Nature | sha256 | Contenu |
| --- | --- | --- | --- |
| `docs/SORTIES.md` | nouveau, 527 lignes | `f4ed497eba6f15ef` | contrat produit complet (détail au § 2) |
| `docs/ARCHITECTURE.md` | modifié | `864c85cb1eb8d62c` | table des modules, lignes 48 et 50, § 7.2 |
| `cmake/modules.cmake` | modifié | `c60dd9c3737995f2` | `supports` inséré ; `MHGP11_DEPS_supports tower` ; `supports` ajouté aux dépendances d'`api` |
| `README.md` | modifié | `39f7a2ae4ea2da73` | section « Sortie paramétrée », clause de report de `plat`, entrée 5 de « Lire d'abord » |
| `docs/PROVENANCE.md` | modifié (ajout final) | `3783ddbd2a2cbd34` | section « Sortie paramétrée : ports annoncés » |
| `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md` | nouveau, 107 lignes | `ac17f514a0c0d8eb` | réponse point par point à l'audit `1bf4be68f` |
| `audits/README.md` | modifié (une ligne) | `5e6e9d591914a154` | ligne « Réponse courante du développeur », selon l'usage de `d597ed9ba` |

`docs/INDEX.md` n'a **pas** été modifié : ce n'est pas un index des documents, mais la note de l'index spatial global
(« Index global exact pour les descentes »).

Fichiers non touchés, écrits par les agents voisins : `reference/hgp11_ref/supports.py` et `reference/ref_mutants.py`
(s1_oracle), `docs/MATHEMATIQUES.md`, `docs/HIERARCHIE_POINTS.md` et le registre racine (s0_math, pas encore écrits à
21 h 03).

## 2. Ce que contient `docs/SORTIES.md`

- **§ 1.** L'exécutable `mhgp11` : cible `mhgp11_cli`, `OUTPUT_NAME mhgp11`. Paramètre obligatoire `--sortie`, une
  valeur par appel ; une valeur non livrée est refusée `parameter_out_of_range`. Options en français (§ 5 de la spec),
  avec domaines et défauts. Paramètres de moteur fixes.
- **§ 2.** Les quatre sorties : objet, fichier, format, tranche et livraison.
- **§ 3.** Refus et codes de sortie :
  - ordre déterministe des refus, avec l'étape du plan telle qu'implémentée ;
  - les six raisons annoncées et leur tranche ;
  - codes 0, 2 et 3 ;
  - ligne JSON de la sortie standard et protocole `retract()` si cette sortie échoue.
- **§ 4.** Conventions communes des formats.
- **§ 5.** `MHGP11FUL1`, inchangé.
- **§ 6.** `MHGP11SP` v1, **exactement** selon le § 2 de DECISIONS :
  - en-tête de 16 `u64` dans l'ordre de DECISIONS ;
  - sections `SITES`, `NODES`, `BALLS`, `SUPPORTS`, `PRIOR` ;
  - colonnes, alignement à 8 octets, ordres canoniques ;
  - table des quantités dérivées, avec leurs formules ;
  - contrôles du lecteur ;
  - lectures de $P_v$ et des instantanés ;
  - déclarations obligatoires : non-stabilité, omissions, pas le $K$-polyèdre de la Déf. 21, lemme H.
- **§ 7.** `MHGP11PT` et `MHGP11ET` annoncés (§ 6.4 et 6.5 de la spec).
- **§ 8.** Manifeste : § 6.6 de la spec, plus les agrégats de DECISIONS (`multi_support_balls`, somme et maximum de
  `kparties_reliees` et de `cofaces`) ; `tree_k_sha256`.
- **§ 9.** Transaction de dossier telle qu'implémentée (lu dans `io.hpp`, `directory.cpp` et `writer.cpp`) :
  - plan, création, écriture, commit, destructeur, `retract()` ;
  - les deux doubles échecs où un refus laisse un dossier publié complet ;
  - limites.
- **§ 10.** Invariance et déterminisme.
- **§ 11.** Livraisons L0 à L5 ; **règle de décision de L2 fixée d'avance** ; **clause de report de `plat`**.

Renvois à MATHEMATIQUES : par le titre de la section 10, « Hiérarchie des supports d'ordre K » (titre imposé à
s0_math par le workflow), sans aucune preuve recopiée. Le lemme B (rôles) et le lemme H (forme datée de P3) sont cités
par leur lettre.

## 3. Portes jouées

| Commande | Résultat |
| --- | --- |
| `python3 tools/check_docs.py` (racine du worktree), avant toute modification | 157 lignes, code 1 : ligne de base (`build/…/scratchpad/s0docs/check_docs_base.txt`) |
| la même, après les modifications | 157 lignes, **identiques** à la ligne de base (`diff` vide) |
| `check_docs.validate()` appliqué à mes six Markdown v11, que `check_docs.py` ne parcourt pas, liens vers `receipts/` résolus contre `origin/main` | **0 écart** sur les six fichiers (seules deux lignes antérieures du README visent des reçus absents du checkout partiel, présents sur `origin/main`) |
| `python3 -S morsehgp3D_v11/tools/check_style.py --root morsehgp3D_v11` | `style_ok fichiers=423`, code 0 (et sous `-O`) ; `fichiers=424` au dernier passage, après l'ajout de `reference/test_supports.py` par s1_oracle |
| lecture de la table par `check_style.read_architecture_table` | ordre `core … tower supports points head api`, `supports` → `tower`, aucun problème |
| témoin négatif : copie CMake avec `MHGP11_DEPS_supports catalogue` | `[table] MHGP11_DEPS_supports = ['catalogue'], document = ['tower']`, code 1 : l'écart est vu |
| `tests/support/test_check_style.py` sous `python3 -S -B`, puis sous `-O` | `check_style_ok controles=133`, code 0 dans les deux cas |
| `cmake -S morsehgp3D_v11 -B /tmp/v11-l0-s0_docs/build -DMHGP11_MODULES=reference` | configuration réussie, 77 portes enregistrées |
| `ctest -R '^mhgp11_style'` dans cette configuration | `mhgp11_style` et `mhgp11_style_opt` passent (2/2) |

**Module planifié sans dossier.** Il est admis : `[module]` ne juge que les dossiers présents de `src/`, et
`CMakeLists.txt` n'inclut que les `src/<m>/module.cmake` existants. C'est déjà le cas de `points`, `head` et `api`.
`ARCHITECTURE.md` le dit désormais explicitement.

## 4. Écarts à la spécification, et pourquoi

1. **Ordre des raisons nouvelles.** `support_shell_capacity` et `supports_invariant` (S6, L1) passent avant
   `environment_selftest` (S5, L2). La décision 24 de la spec plaçait `environment_selftest` en tête, mais sa règle est
   « dans l'ordre des livraisons », et le plan révisé met S6 avant S5. C'est dit dans SORTIES § 3.
2. **Ordre des refus du plan** : celui du code de `io`, et non celui du § 5 de la spec.
   - Implémenté : forme du chemin, parent absent (`output_unwritable`), conflits, parent non inscriptible.
   - La spec citait les conflits avant `output_unwritable`.
3. **« Aucun refus ne publie de dossier »** n'est pas absolu dans le code. Deux doubles échecs d'entrée-sortie peuvent
   laisser un dossier publié, complet :
   - `fsync` du parent puis retour arrière ;
   - `retract()` après l'échec de la sortie standard.

   Ils sont documentés (SORTIES § 9 et ARCHITECTURE § 7.2) et posés en question à l'auditeur.
4. **Invariance par permutation.** Elle ne vaut pas pour `plat` : les étiquettes sont rangées dans l'ordre d'entrée.
   Le § 2.8 de la spec disait « fichiers identiques » sans réserve. La translation est corrigée selon la critique : seuls
   les ensembles sont invariants.
5. **`budget_bytes` du manifeste** : `null` sans plafond. L'exemple de la spec portait 0, qui n'est qu'un gabarit.
6. **Agrégat `cofaces` du manifeste** : pris **par boule** (liaisons distinctes). DECISIONS dit « somme et maximum de
   `kparties_reliees` et de `cofaces` » sans préciser ; la critique (§ 2.1) visait les cofaces par support. Question
   ouverte.
7. **Rôle d'`io` dans la table** : sans « JSON déterministe ». S4 l'a renvoyé à `api` (`manifest.cpp`), choix approuvé
   par la vérification `verif_s4` (C3). SORTIES § 8 l'inscrit.
8. **`PROVENANCE.md`** annonce, outre E2 et les bancs Python de `points` et `plat`, trois autres ports que la spec
   nomme :
   - `enumerate.cpp` depuis `bench/catalogue_euler.hpp` (§ 3.3) ;
   - `write_full.cpp` depuis `bench/full_probe.cpp` (§ 3.5) ;
   - le repli exact de `num` depuis les bancs Python (§ 7.8).

   Rien n'est porté en L0, et c'est écrit.
9. **Format `MHGP11SP`** : il suit DECISIONS § 2, qui remplace le § 6.3 de la spec. Il n'a pas de colonne `key`,
   `node`, `qmin`, de compte, `post`, `size` ni `children`. L'en-tête garde 16 mots : `births` part, `Z` arrive. Ce
   n'est pas un écart à l'autorité, mais un écart au texte de la spec.

## 5. Questions ouvertes

1. **Agrégat `cofaces`** (voir l'écart 6) : faut-il aussi publier l'agrégat des incidences par support ? La question
   est posée à l'auditeur (réponse, D.1).
2. **`tree_k_sha256`** contient le `BallIdx` des naissances (définition du § 6.6 de la spec, gardée telle quelle). Or
   `MHGP11SP` v1 ne publie pas `BallIdx` : le lecteur ne peut pas recalculer l'empreinte depuis le seul fichier.
   Faut-il la redéfinir sur des quantités publiées ? Question posée à l'auditeur (D.2).
3. **Doubles échecs de la transaction** : l'exception est-elle acceptable (D.3) ?
4. **Juge E2 avec la fenêtre au lieu de `strong`** (D.4).
   - Notre lecture : une naissance est toujours forte ; un événement faible n'est jamais une naissance ; la $K$-partie
     descendue y est stricte.
   - Confirmation demandée à l'auditeur.
5. **Écart mineur dans le code livré**, non corrigé ici car hors de ma tâche :
   - `io.hpp:167-171` annonce que `retract()` refuse `output_unwritable` si le renommage échoue ;
   - le code (`directory.cpp:234-240`, via `rename_noreplace`) rend `output_conflict` sur `EEXIST` ou `ENOTEMPTY`.

   SORTIES décrit le code. Le commentaire est à aligner en S5.
6. **À fixer en S5** :
   - forme exacte de la ligne JSON de refus ;
   - syntaxe des décimaux de `--pas` et `--origine` ;
   - octets exacts du manifeste ;
   - `counts` de `full` ;
   - garde éventuelle contre une sortie standard redirigée vers une entrée (`>> ids.u32le`, `verif_s4` R2).
7. **Cohérence avec la section 10 de MATHEMATIQUES** : la consolidation doit vérifier que s0_math garde :
   - le titre « Hiérarchie des supports d'ordre K » ;
   - les lettres B (rôles) et H (forme datée de P3) ;
   - les noms `kparties_reliees`, `compressed_parts`, `strict_traces`, `components`, `cofaces` (boule et support) et
     `gabriel_cofaces`.

   Ce sont les noms de l'oracle `supports.py` lu à 20 h 57.
8. **Contrôles du lecteur** : la liste de DECISIONS est reprise telle quelle. Faut-il y ajouter le contrôle de la
   numérotation canonique ?
   - naissances triées par (rang, centre exact) ;
   - `ball_count` nul pour une feuille de $K=1$.

## 6. Vérifications de fond faites pour écrire le contrat

- **Feuilles de $K=1$** : la feuille $i$ est le site de rang $i$ en ordre lexicographique $(x,y,z)$ (`point_births`,
  `src/tower/forest_build.cpp`). Vérifié, comme le demandait DECISIONS.
- **Naissances à $K\geq 2$** : triées par (rang, centre exact lexicographique), par `num::compare_centers`. Fusions :
  par (niveau, plus petite naissance) ; enfants triés par `NodeIdx` (`forest_plateau.cpp`, `close`).
- **Ordre des boules dans un nœud.** (rang, $S^*$ lexicographique) coïncide avec l'ordre des `BallIdx`. Le catalogue
  trie en effet par (niveau, `std::array` $S^*$ complété par `kNone`) (`catalogue/assemble.cpp`, `less`). À niveau
  égal, aucun $S^*$ n'est une partie d'un autre : par M1, il porterait une boule de rayon strictement plus petit.
- **Bornes.**
  - $12\,926=\binom{24}{2}+\binom{24}{3}+\binom{24}{4}$ tient en `u16`.
  - $p\leq K-1\leq 11$ et $m\leq 24$ tiennent en `u8`.
  - `kMaxMebSites` vaut 12 (`tower/meb.hpp`), `CatalogueParams::kmax` va de 1 à 12, `kMaxWorkers` vaut 256.
- **`MHGP11FUL1`** : contenu relu dans `serialize` (`bench/full_probe.cpp`). Codage des mots et des entiers relu dans
  `bench/whole_input.hpp`.
- **Masque 16 379** : relu dans `bench/full_probe.cpp`. Il allume tous les bits de valeur 1 à 8192, sauf celui de
  valeur 4 (mémo), et comprend donc `concurrent_orders` (8192), que `build_order` refuse jusqu'à S11.
- **Empreintes des sources annoncées comme ports** (dans `PROVENANCE.md`) : les six fichiers de `bench/` sont
  identiques à `f98aeed67` et dans le worktree, et inchangés depuis `57dd21be1`.
