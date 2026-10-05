# Relecture légère de la tranche S6b (assemblage de la hiérarchie des supports)

5 octobre 2026, 11 h 58 UTC. Relecteur léger, aucune modification du worktree.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

GCP non utilisé.

- Commit relu : `66bf76b92` (parent `165def5ab`), HEAD détaché dans `build/v11-impl-l2`, worktree propre, non poussé.
- Référentiel : `DECISIONS_UTILISATEUR.md`, `docs/SORTIES.md` (§ 6 et la remarque sur l'ordre (rang, $S^*$) = `BallIdx`), `SPECIFICATION_FINALE.md` (§ 8.4, I5, I6, I11 ; § 9.1), apports `238734f1d`, `9cbf805c6` et `a65903a7b`.

## Verdict

**Aucun point bloquant.** On peut pousser après la petite retouche du README ci-dessous (facultative, mais le texte deviendra faux une fois poussé).

## Portes rejouées

Sur le build de l'implémenteur `build/v11-persist/b21-s6b` (Release, u21), sans reconstruction :
- `make -q` confirme que les binaires sont à jour ;
- `CTestTestfile.cmake` a été régénéré après `tests.cmake`.

Commande : `ctest -R mhgp11_supports_hierarchy -E ng00_k10 -j2`. Résultat : **20/20 réussies** en 90,6 s réelles (11 h 55 à 11 h 57 UTC).

| Porte | Durée locale (s) |
| --- | ---: |
| fixtures, determinism, permutation, sphere5, sphere9, admission, fault (et inventaires) | 2,6 au total |
| hierarchy_fraction et _opt | environ 19 chacune |
| scale8000, perm_scale8000 | 2,7 et 1,7 |
| scale16000, scale32000 | 4,5 et 9,1 |
| grid8000, perm_grid8000 | 10,3 et 8,7 |
| lidar ng00, ng01, ng02 à K5 | 4,9, 5,8 et 4,6 |

- Non rejouée : `lidar_ng00_k10` (long, W48, pour G4).
- `run_mutants.py --check` : `manifeste_ok module=supports mutants=15 plancher=15`. Les motifs sont uniques ; la campagne de mutants n'a pas été relancée.
- `check_style` : `style_ok fichiers=452`.
- `check_docs` : aucune ligne d'erreur sur README, SORTIES, PROVENANCE ni ARCHITECTURE de la v11.

## Points vérifiés à la lecture

- **Exactitude.**
  - Postordre sans pile : `2N - 1` pas, garde `2N`, `rank == N` contrôlé. Les enfants sont exigés croissants et de même parent.
  - Seaux stables : remplissage de la dernière boule à la première, avec la fin de chaque seau décrémentée. On obtient l'ordre (postordre, `BallIdx`) dans chaque seau. C'est l'ordre (rang, $S^*$) du fichier, par la remarque de SORTIES (M1) ; I11 le contrôle à l'échelle.
  - Contre-épreuve $S_{\mathrm{journal}}=\binom{m}{t}-N_t$ sur toutes les boules. La sonde la recompte en plus par fermeture brute (`fermetures == boules` partout, coquille ≤ 16).
  - Décalages `u64` aux sommes vérifiées. Il n'y a aucun flottant.
  - Comptes `u32` : $\binom{35}{12}$ < $2^{32}$, sans débordement possible sous les plafonds $p\leq 11$, $m\leq 24$.
- **Budget.**
  - `HierarchyAdmission::first` correspond octet pour octet à `allocate_first`. `second` vaut (20 + 4) S.
  - Les tampons par fil sont alloués en dernier : c'est ce qui rend les trois mutants d'admission observables (pic non nul sous le premier étage).
  - Refus `support_shell_capacity` de l'appel entier dans la pré-passe, sans allocation : la porte sphere9 vérifie un pic nul.
  - `admit` n'est pas une réservation ; c'est conforme au contrat de `MemoryBudget`.
- **Déterminisme.**
  - Chaque fil a ses registres, sommés après la jointure.
  - Les positions sont fixées avant les passes, et fill relit la clé écrite par count.
  - L'empreinte couvre toute la hiérarchie et le registre. Elle est identique sans Pool, à W1, W2 et W4, avec l'arbre sériel ou par lots, sous permutation et sous réétiquetage jusqu'à $2^{32}-1$. Les portes d'échelle comparent W1 (sans Pool), W2 et W4.
- **Vacuité.**
  - Planchers gravés : fixtures (≥ 66 ordres, ≥ 300 coquilles étendues, ≥ 100 boules multi-supports, fermetures = boules), fraction (11 planchers + ligne gravée), échelle (`--min-balls`), grid (`--min-extended`, `--min-multiple`) et LiDAR (`--min-extended=50`).
  - Le différentiel compare bien toute la structure normalisée à l'oracle S1 : nœuds, niveaux, centres, enfants, postordre, rôles, branches, supports et comptes par support. L'ordre natif est vérifié à part.

## À corriger (non bloquant)

1. `morsehgp3D_v11/README.md:132` : la phrase « l'assemblage `SupportHierarchy` (S6b) est commité localement, en attente de relecture » sera fausse dès le push. Correction attendue : « livré (S6b, `66bf76b92`), qualification G4 en attente », par exemple, comme pour S6a.

## Notes

- **Workers.** `sizes.workers` vaut `pool->size()` (`hierarchy.cpp:88`), et non le nombre de fils réellement actifs (au plus ⌈B/512⌉). C'est une surestimation sûre, écrite comme telle dans l'en-tête, mais elle peut refuser inutilement près de la limite à W48 : 48 × (2 Mio + 12 926 × 20 octets), soit environ 108 Mio pour une coquille de 24. À garder en tête pour la règle L2 et S7.
- **Ordre des boules dans le fichier.**
  - Le fichier publie les boules dans l'ordre (postordre, rang, $S^*$ lexicographique) ; la mémoire les tient en (postordre, rang, `BallIdx`).
  - L'égalité de ces deux ordres repose sur la remarque M1 de SORTIES et sur le tri canonique du catalogue. Aucune porte de S6b ne compare directement l'ordre natif à l'ordre $S^*$ de l'oracle : le différentiel renormalise par (postordre, niveau, centre).
  - Le lecteur S7, qui contrôle « boules triées », fermera ce point. À citer dans S7.
- **Mutant `supports_tri_pointid`.** Il mute un tri que l'implémenteur a ajouté et qui est neutre sur un code juste. C'est acceptable, puisque l'ordre publié devient un contrat de l'assemblage, mais ce tri coûte un `std::sort` par boule dans la passe fill.
- **Porte K10.** `lidar_ng00_k10` n'a qu'un code de sortie et `--min-balls`, sans `LINE` ni `--min-extended`. Il faudra graver la ligne après la session G4.
- **Bytecode.** Un `tests/support/__pycache__/mhgp11_gate.cpython-312.pyc` est apparu à 11 h 42 dans le worktree. Il est ignoré par `.gitignore` et ne touche pas le commit. Il vient sans doute d'une exécution sans `-B`, hors CTest.
- **Écarts déclarés acceptés.** `HierarchyTimings*` optionnel, colonne `support_cofaces`, générateur propre de `--grid`, troisième copie de `PythonRandom`, ligne u18 venue de l'oracle seul. `tree_k_sha256` (fin de I11) est reporté à S7, ce qui est cohérent : SORTIES § 8 le lie au manifeste.
- **Part sérielle.** La pré-passe, le postordre, les seaux et les sommes préfixes font environ 50 % de l'assemblage dès W4. La pré-passe lit le catalogue boule par boule en ordre `BallIdx`, et elle est parallélisable sans changer de sortie. C'est le levier si cet étage compte pour les 100 ms.
- **Pour G4.** La liste de l'implémenteur est confirmée : u18 natif, u24, ASan/UBSan, TSan (passes count et fill, registres par fil), les 15 mutants, la porte K10 à W1, W4 et W48 avec sa ligne à graver, et la mesure de l'assemblage à W1 et W48 à K5 et K10.
