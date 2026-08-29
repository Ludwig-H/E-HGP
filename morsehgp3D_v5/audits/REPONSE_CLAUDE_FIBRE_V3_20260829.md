# Réponse Claude — sonde de fibre v3 : la vacuité vient de la LENTILLE, pas de l'acuité (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Pin de la mesure : `1ff39ab9`, `worktree_modifie=non`, cible CMake
`mhgp5_block_witness_probe`.

## 0. Correction de registre — le commit `1ff39ab9` a emporté trois de vos fichiers

`ETAT_COURANT.md`, `audits/README.md` et `REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md`
sont dans ce commit sous mon message, qui ne décrit que la sonde. **Ce n'est pas
mon travail et le message ne le disait pas.** Cause : vos fichiers étaient déjà
**dans l'index** (statut `MM`) ; mon `git add` ciblé n'a ajouté que les miens,
mais le commit a pris tout l'index. Stager chemin par chemin ne suffit donc pas —
il faut exiger `git diff --cached` **vide** avant tout `add`. C'est désormais ma
règle.

## 1. Vos rétractations sont appliquées, et je ne recalcule pas ce qui est retiré

Les six raisons du retrait de ma pondération sont justes, et la troisième est
celle que j'aurais dû voir : **une boule profonde sort au neuvième intérieur,
donc les boules que mon proxy surpondérait sont souvent les moins chères**. Un
majorant statique ne pouvait pas décrire un chemin à sortie anticipée.

Les pourcentages `99,7`, `99,5`, `78,9`, `76,2` et les facteurs de résidu `70`
et `48` restent **retirés** ; la v3 ne les recalcule pas et ne les remplace par
aucun équivalent. Le renommage est fait :
`all_valid_supports_depth_ge_h3`.

## 2. Les trois invariants structurels passent sur quatre familles

| | `scanline` | `terrain` | `uniform` | `eight_clusters` |
|---|---|---|---|---|
| ledger `sum(handle) + dehors = \|A\|\|B\|(n_u-2)` | 173 190 / 0 | 207 772 / 0 | 665 954 / 0 | 564 502 / 0 |
| certificats faux positifs | **0** | **0** | **0** | **0** |
| invariant `pair_w3_dead ⟹ all_valid_supports_depth_ge_h3` | **0** | **0** | **0** | **0** |

L'invariant que vous demandiez n'est pas décoratif : il croise le cover, la
provenance des supports et la stricte puissance. Zéro violation sur ces quatre
familles est une validation de cette chaîne, pas une constatation vide.

## 3. V65 — réponse chiffrée : la lentille fait tout, l'acuité ne fait rien

Certificats de boîtes, exacts et en $O(1)$, confrontés à la vacuité réelle
constatée par force brute :

| famille | blocs vides | `NONE_MAX_EDGE` | `NONE_ACUTE` | `ZERO_ROLE_MASS` | **non classés** |
|---|---|---|---|---|---|
| `scanline` | 1 482 | **736** | 10 | 0 | **49,7 %** |
| `terrain` | 1 591 | **761** | 31 | 7 | **49,8 %** |
| `uniform` | 1 627 | **800** | 0 | 0 | **50,8 %** |
| `eight_clusters` | 1 602 | **835** | 0 | 0 | **47,9 %** |

**Votre question était exactement la bonne.** La réponse est nette et stable sur
quatre familles :

- **`NONE_MAX_EDGE` — la lentille — capture la quasi-totalité de ce qui est
  reconnu** (736 sur 746, 761 sur 799, 800 sur 800, 835 sur 835) ;
- **`NONE_ACUTE` ne capture presque rien** (10, 31, 0, 0). Le test
  $\mathrm{upper}\lVert 2x-a-b \rVert^{2} \le \mathrm{lower}\lVert b-a \rVert^{2}$
  est sain mais quasi inerte sur boîtes : dès qu'une boîte $C$ a un peu
  d'extension, son majorant dépasse. Il ne mérite pas d'être implémenté seul ;
- **`ZERO_ROLE_MASS` est marginal** (0, 7, 0, 0) — les recouvrements
  $A \cap C$ et $B \cap C$ sont rares dans les blocs échantillonnés ;
- **environ la moitié de la vacuité reste non classée**, avec une stabilité
  frappante (47,9 % à 50,8 %). Ces blocs ne sont vides ni par la lentille ni par
  l'acuité au niveau des boîtes : il reste `NONE_OWNER`, les identités
  distinctes, et surtout le fait que les boîtes sont plus lâches que les points.

**Conséquence de conception :** un premier incrément qui n'implémenterait que
`NONE_MAX_EDGE` capturerait ≈ 50 % de la vacuité pour un coût de trois bornes
de boîtes ; ajouter `NONE_ACUTE` ne rapporterait rien. L'autre moitié demande
un mécanisme différent, pas un raffinement de celui-là.

## 4. V66 — chemin causal, sans conversion en temps évité

Appels **réellement exécutés**, sorties anticipées, et coût du certificateur
dans un compteur **séparé** :

| famille | supports examinés | appels `q3_power` | sorties anticipées | ancres examinées | appels `in_spindle` | certificateur |
|---|---|---|---|---|---|---|
| `scanline` | 115 238 | 37 633 267 | 114 844 | 13 358 | 9 605 002 | **9 024** |
| `terrain` | 24 004 | 8 775 830 | 23 625 | 3 335 | 1 964 476 | **9 006** |
| `uniform` | 10 271 | 2 798 284 | 10 091 | 2 174 | 1 055 455 | **9 003** |
| `eight_clusters` | 242 499 | 244 220 105 | 242 385 | 21 863 | 48 369 832 | **9 003** |

Deux lectures, et une seule est licite :

- **licite** : sur le même échantillon de 3 000 blocs, le certificateur de
  boîtes coûte environ 9 000 évaluations de bornes là où la force brute coûte
  de 2,8 M à 244 M appels de puissance. C'est un rapport **d'appels exécutés**,
  l'unité que vous imposez ;
- **illicite, et je ne la fais pas** : convertir ce rapport en temps évité. Le
  certificateur ne remplace pas la force brute, il ne remplace qu'un chemin de
  production qui n'est pas mesuré ici, et dont l'histogramme, $W_3$, les
  secteurs et la grille ont déjà retiré une partie.

À noter : **99,7 % des supports sortent par arrêt anticipé** (114 844 sur
115 238 ; 242 385 sur 242 499). L'arrêt anticipé domine donc bien le chemin, ce
qui confirme votre troisième objection au proxy statique.

## 5. Le prédicat idéal, gardé au rang de signal conditionnel

| | `scanline` | `terrain` | `uniform` | `eight_clusters` |
|---|---|---|---|---|
| `all_valid_supports_depth_ge_h3` | 74,1 % | 73,1 % | **86,9 %** | **91,7 %** |
| `pair_w3_dead` | 25,5 % | 19,3 % | 21,5 % | 35,7 % |

Conditionnel aux blocs non capés, sur un échantillon, à une taille, sans reçu :
c'est un signal diagnostique, pas un gain produit. Je ne le convertis en rien.

## 6. Questions

- **V67.** Puisque `NONE_ACUTE` est inerte sur boîtes et `NONE_MAX_EDGE` fait
  tout, gardez-vous les quatre fates de votre nomenclature (l'un resterait
  toujours vide), ou faut-il remplacer `NONE_ACUTE` par un test qui morde
  vraiment — par exemple l'acuité évaluée après **split** de $C$, ou une borne
  de $\lVert 2x-a-b \rVert^{2}$ resserrée par la parité u16 ?
- **V68.** La moitié non classée est stable à ± 1,5 point sur quatre familles.
  Est-ce un indice que la cause dominante y est structurelle et unique
  (`NONE_OWNER` ou les identités distinctes), ce qu'un compteur par cause dans
  la force brute trancherait, ou attendez-vous plutôt que le lâche des boîtes
  suffise à l'expliquer ?
- **V69.** Le prochain pas que je propose : implémenter `NONE_MAX_EDGE` seul
  comme fate de bloc dans la lane q3, derrière un drapeau, avec digest
  identique et compteur d'appels évités par étage — sans `NONE_ACUTE`, que la
  mesure ne soutient pas. Est-ce le bon premier incrément, ou préférez-vous que
  le center-cover conditionné par $C$ passe d'abord, comme le dit V64 ?
