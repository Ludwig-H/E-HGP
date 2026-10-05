# Relecture légère de la tranche S7 (`--sortie=supports`), commit local `bd7c8e130`

5 octobre 2026, 13 h 12 UTC (`date -u`). Relecteur léger ; rien n'a été modifié dans
`/workspaces/E-HGP/build/v11-impl-l2`. GCP non utilisé.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

## Verdict

**Rien de bloquant.** L'écrivain suit SORTIES § 6 à l'octet : en-tête de 136 octets, colonnes alignées sur 8 octets,
bourrage nul et aucun compte stocké. Le manifeste suit le § 8. Le masque 7035 est publié et protégé par une
assertion statique. Le lecteur dérive et contrôle ce que le § 6 déclare. Les portes ne sont pas vertes par vacuité :
lignes de couverture gravées à l'identique, planchers et comptes exacts à l'échelle.

Il reste quatre corrections mineures, aucune d'exactitude du produit, et des notes pour la session G4.

## Portes rejouées

Portes rejouées sur le build de l'implémenteur `/workspaces/E-HGP/build/v11-persist/b21-s7` (u21, Release), sans
reconstruction. Ce build est postérieur aux sources du commit, et le worktree est propre.

| Porte | Résultat | Durée |
| --- | --- | ---: |
| `mhgp11_cli_supports_oracle` | vert, ligne gravée identique (`nuages=211 … signatures=810 refus=4`, `controles=4736`) | ≈ 29 s |
| `mhgp11_cli_supports_oracle_opt` | vert | 27,7 s |
| `mhgp11_cli_supports_scale8000` | vert, ligne gravée identique (`noeuds=273655 boules=395667 … appels=12`) | 26,4 s |
| `mhgp11_cli_supports_lidar_ng02_k5` | vert (`etendues=354 multiples=8 … appels=12`, `controles=46`) | 56,9 s |
| oracle hors CTest sous `/usr/bin/python3 -S -B -O` (3.12) | même ligne de couverture, `cli_supports_oracle_ok` | 28 s |

Remarque : `mhgp11_python_gate` lance l'interprète du codespace sans `-S` (`cmake/gates.cmake`). Le rejeu sous
`-S -B -O` ci-dessus couvre la règle pour l'oracle ; les portes d'échelle n'importent que la bibliothèque standard.

## Lecture du code (constats vérifiés)

- `compute(SupportsRequest)` partage les étapes cloud, index et domain avec FULL (`prepare_domain`), donc l'ordre des
  refus est inchangé. Suivent `build_order` avec `order_params()`, puis `build_support_hierarchy` sur le Pool de la
  Session. `tree` vaut la durée de `build_order` moins `attach_ns`, qui est une mesure murale séquentielle de
  `attach_window` (`order_tree.cpp:89-95`), donc pas une somme par fil. Le pic de `build_order` est compté dans `tree`.
  Tout cela est documenté dans SORTIES § 3.
- `write_supports` : disposition calculée avant écriture, `reached()` contrôlé à chaque section, tampon de 8 Kio sur la
  pile, aucune allocation. Les gardes `supports_invariant` restent sans porte possible, ce qui est déclaré.
  `ball_count` est indexé par `post[v]`, conformément à `ball_offsets` (S6b).
- Manifeste : `births` et `merges` viennent de la forêt, `extended` de `m > qmin` et `multi` de `count >= 2`.
  `tree_k_sha256` est calculé sur `OrderTree::forest`. Le lecteur recompte tout depuis le fichier et recalcule la
  signature V2.
- Lecteur, prédicats entiers. J'ai vérifié les formules du circoncentre d'un triangle,
  $a+\frac{\lvert u\rvert^2(v\times w)+\lvert v\rvert^2(w\times u)}{2\lvert w\rvert^2}$, et du tétraèdre, ainsi que
  l'intérieur strict par les faces et la positivité des autres supports. Arité 2 : le milieu est le centre. Arité 3 :
  cocyclique, coplanaire et aigu. Arité 4 : intérieur strict.
  - `closure` compte les parties sans support de la réunion, puis complète par les sites hors support : c'est conforme
    au § 6.
  - L'équivalence « naissance ⟺ `strict_traces` = 0 » imposée par `_check_roles` est un théorème (lemme B et lemme C,
    point 1, MATHEMATIQUES § 10.4) ; elle n'est pas seulement l'implication écrite au § 6.
- Portes :
  - L'oracle compare toutes les clés de `COMPARED_NODE` et `COMPARED_BALL`, `prior`, `components` et les comptes de
    Gabriel compris, ainsi que les `ids`. W1 dans l'ordre direct et W4 dans l'ordre inverse alternent.
  - Le refus à 25 sites exige le code 2, `support_shell_capacity`, l'étape `compute`, `publication=none`, et ni D ni
    D.pending, tandis que FULL est conforme sur la même entrée.
  - Échelle : W1, W4 et une répétition, la permutation, et le réétiquetage avec `0xFFFFFFFF` (seule la colonne
    `point_id` change, et la porte vérifie qu'elle est l'image des anciens identifiants). La signature est égale à
    celle de FULL, avec les comptes exacts gravés.
- Mutants : les 8 `sp_*` ont un motif unique et tuent une propriété réelle. `sp_signature_v1` est tué par le recalcul
  du lecteur. `sp_masque_16379` est tué par le refus de `build_order`. Le nouveau motif de
  `ligne_refus_sortie_toujours_full` garde son sens : `named = true` imprime `"full"` à l'étape des options.

## Sondes adverses du lecteur

Les sondes ont tourné sur un nuage de 120 points dans [0,10)³ à K = 3 (664 coquilles étendues), en altérant le
fichier sans toucher au manifeste. Scratch dans `/tmp/v11-l2-verif-s7`, retiré.

| Altération | `read_supports` |
| --- | --- |
| rôle interne → fusion | détecté (fusion sans branche) |
| `p + 1` | détecté (hors de $W_K$) |
| entrée de PRIOR altérée | détecté (réunion des branches ≠ enfants) |
| rang de la racine + 1000 | détecté (première boule hors du rang) |
| `m + 1` sur une coquille régulière | non détecté par `read_supports` seul : attendu, la coquille n'est pas publiée ; `check_directory` le voit par le recompte de `kparties_reliees` tant que le manifeste n'est pas réécrit |
| `point_id` dupliqué | **non détecté** (voir à corriger 1) |

## À corriger (mineur, non bloquant)

1. `bench/mhgp11_formats.py:569` (`_check_sites`) : ajouter `need(len(set(f.point_id)) == f.n, …)`. Un `PointId`
   unique par site, avec un poids un, fait partie du contrat (SORTIES § 6 : « l'unique `PointId` du site » ; refus
   `duplicate_point_id` en amont). Le contrôle ne coûte rien et ferme un trou du lecteur.
2. `src/api/api.hpp:111` : le commentaire des étages fait 198 colonnes, c'est une ligne non repliée. Le replier
   comme les lignes voisines (≤ 120). Même chose pour la ligne 22 (122 colonnes). `check_style` ne mesure pas la
   largeur.
3. `bench/sorties_g4.py:183-184` (`decide`) : quand `missing` est vide mais que `prises < 3`, la raison affiche une
   liste de trames vide (« W48 absent pour  ou … »). Distinguer les deux raisons. C'est cosmétique, mais le texte ira
   dans un reçu.
4. `tests/mutants/api.json` : l'énoncé de S7 demandait des mutants « api et cli ». Les huit nouveaux sont tous dans
   `cli.json`, ce qui est acceptable puisque la porte qui les tue est CLI. Il faut le déclarer dans les écarts du
   rapport `impl_s7.md`, ou ajouter un mutant `api` (par exemple `Product::kind` toujours `full`, tué par
   `mhgp11_api_session_*`).

## Notes

- **Règle de L2, lecture de `tree`.** Pour `supports`, l'étage `tree` exclut `attach`, conformément au § 3. Sous
  L2b, `supports` paierait aussi un rattachement, depuis le journal posé dans `build_full`. La comparaison
  « tree contre tree » reste donc appariée, mais le coût réel de la voie par défaut est `tree + attach`.
  - Recommandation : que `sorties_g4.py` publie aussi, à titre descriptif, le ratio de `tree + attach` de `supports`
    contre `tree` de `full`.
  - Ce ratio n'entre pas dans la décision : la règle, écrite d'avance, porte sur `tree` seul et ne change pas.
- **Ligne oracle u18** (`nuages=209 exclus=2 ordres=957 … signatures=804`), gravée depuis l'oracle sans exécution
  native.
  - Elle est cohérente avec `mhgp11_supports_hierarchy_fraction` u18 (209, 957, 15246, 2639, 570).
  - La configuration `gcc_release` de `tools/g4_matrix.json` est en u18, et l'oracle porte le label `fast`
    (requis) : G4 la jouera en premier.
  - Si elle échoue, regarder d'abord `signatures` : 804 suppose que les 6 ordres exclus sont tous à K ≤ 4.
- **Budget G4.** `mhgp11_cli_supports_oracle` lance environ 1 800 processus CLI. Sous ASan/UBSan et TSan, prévoir
  plusieurs fois les 30 s locales, et vérifier `test_timeout_seconds` (900 s pour la porte). Les portes d'échelle
  et LiDAR (12 appels chacune, jusqu'à 111 s en local) sont à mettre dans le budget de 1 380 s de la matrice.
- Le lecteur ne vérifie pas que les rangs des nœuds sont denses. La signature V2 les recouvre indirectement, par
  l'égalité avec FULL et le juge indépendant de `mhgp11_cli_tree_signature`.
- Uniform 8000, 16000 et 32000 n'ont aucune coquille étendue. Les branches à plusieurs supports du lecteur ne sont
  exercées à l'échelle que par `boite12` et les trames LiDAR (81 à 354 coquilles étendues), ce qui suffit, avec des
  comptes gravés.
- `--sortie=supports` à K = 10 n'a pas de porte (écart déclaré). La règle de L2 mesure K = 10 sur ng00 à titre
  descriptif.
- `docs/implementation_status.toml` n'est pas touché : c'est correct pour une exploration v11.
