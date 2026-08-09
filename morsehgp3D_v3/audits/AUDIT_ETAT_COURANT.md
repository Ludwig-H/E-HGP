# Audit courant de MorseHGP3D v3

Date du snapshot : 9 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_under_audit`,
`profile=quantized_u16_input_only`,
`mode=order_k_flats_owner_differential_and_gate_d_f0`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype et
ne vaut ni promotion produit, ni ouverture de phase. Les résultats concernent le
worktree non committé construit sur :

| objet | empreinte |
| --- | --- |
| `HEAD` | `aec74398af189de0fce32fe6c31a4e304fda32c6` |
| `prototype/order_k_flats.hpp` | `4516125c93187e0ebeef8bac95143281207e6d598bb42adbf4f3f305ccc6c0d3` |
| `prototype/flats_differential.cpp` | `a10699538c98390e78f0e0fadb9114978d020c62a9df25e484568deb0a1a5173` |
| `CMakeLists.txt` | `fdc00942cc8aed26f46c40ad3a95ef7be040d968ff819fd1ffb9368f171946c4` |
| `audits/check_gate_d_fold_f0.py` | `34149092cd1b06762085800ac9d575c0cb8022e3a1c273c7d1955d2f4e768294` |

## Verdict

**NO-GO pour promouvoir le propriétaire comme remplacement général de
`emitted`, et NO-GO pour déclarer F0-A mathématiquement validée.** Deux P1 ont
des contre-exemples minimaux reproductibles :

1. `flat_catalogue(..., use_owner=true)` omet silencieusement des sphères hors du
   seul quadrant indexé et affine 3D exercé par le différentiel ;
2. le juge F0 rejette une naissance composée de facettes activées dans le lot,
   alors que son contrat écrit l'autorise. La vérité Warshall et le sujet DSU
   partagent ce rejet, donc leur accord ne le révèle pas.

Le verdict ne retire pas les résultats positifs : les cinq portes flats du
snapshot sont vertes avec zéro désaccord, les chemins exercés passent
ASan/UBSan, le cône signé choisit le bon propriétaire sur la fixture mathématique
demandée, et les nouvelles gardes de `decide_child` ainsi que le statut
`kSinkStopped` ferment des ambiguïtés réelles.

## P1 — domaine incomplet de `use_owner`

### Contre-exemple

`try_emit` appelle `try_emit_with` sans candidat ni sommet propriétaire. Le
filtre owner rejette ensuite tout candidat nul. Cela affecte deux chemins que le
juge n'exerce pas :

- sans index, les singletons passent par `try_emit` et disparaissent, même sur
  un nuage affine 3D navigable ;
- si le nuage a moins de quatre points ou une dimension affine inférieure à
  trois, toutes les arités supérieures à un passent par la voie directe et
  disparaissent, avec ou sans index.

Le statut public du nuage ne signale pas cette perte. Probe CPU, avec un
tétraèdre `[(0,0,0),(2,0,0),(0,2,0),(0,0,2)]` et un triangle
`[(0,0,0),(4,0,0),(1,3,0)]` :

| nuage | index | owner | sphères | arités 1 / 2 / 3 / 4 |
| --- | ---: | ---: | ---: | --- |
| tétraèdre | non | non | 11 | 4 / 6 / 1 / 0 |
| tétraèdre | non | oui | **7** | **0 / 6 / 1 / 0** |
| tétraèdre | oui | non | 11 | 4 / 6 / 1 / 0 |
| tétraèdre | oui | oui | 11 | 4 / 6 / 1 / 0 |
| triangle, voie directe | non | non | 7 | 3 / 3 / 1 / 0 |
| triangle, voie directe | non | oui | **0** | **0 / 0 / 0 / 0** |
| triangle, voie directe | oui | non | 7 | 3 / 3 / 1 / 0 |
| triangle, voie directe | oui | oui | **3** | **3 / 0 / 0 / 0** |

Le différentiel active le propriétaire seulement si `status == kOk`, toujours
avec `use_index=true`. Il ne voit donc aucun de ces quadrants.

### Fermeture attendue

Claude doit d'abord fixer le contrat de l'option : soit conserver un repli exact
sur les domaines sans sommet propriétaire, soit les refuser explicitement sans
catalogue partiel. La porte doit ensuite comparer les catalogues dans la matrice
`use_index` × `use_owner`, sur un nuage navigable, un petit nuage direct et un
nuage affine de dimension basse. Le tétraèdre et le triangle ci-dessus sont les
fixtures minimales.

## P1 — F0 rejette une naissance autorisée par son contrat

La note F0 définit une naissance par `q_R=0` dès que la composante porte une
`DirectHyperedge`. Elle précise qu'une composante de latents reliés par une
hyperarête relève bien de ce cas. Deux facettes `n0` et `n1` activées exactement
au niveau `a`, reliées par `DirectHyperedge(n0,n1)`, doivent donc produire une
naissance.

Le script courant ajoute au contraire la précondition « au moins un sommet
`R` ou `L` » dans `_truth_classify` et `_subject_classify`. La fixture ciblée
attend le rejet du cas `N_a--N_a`, puis la campagne de mutation considère son
acceptation comme une faute. Warshall et DSU rendent ainsi tous deux `error` :
l'accord est corrélé et le `PASS` final ne valide pas le contrat publié.

Deux fermetures sont possibles, mais elles ne sont pas interchangeables :

1. retirer la précondition partagée et conserver la sémantique écrite ;
2. démontrer qu'un invariant amont interdit toute hyperarête sans carrier
   strict, le documenter, puis le vérifier indépendamment avant la projection.

En l'état, aucun tel invariant n'est formulé. F0-A reste donc rouge malgré la
sortie `Gate_D_F0_kernel=PASS`.

## P2 — qualification encore insuffisante

### Propriétaire et mémoire

- Dans le quadrant indexé qualifié, les non-singletons évitent `emitted`, mais
  les `n` singletons y sont encore insérés. Le gain démontré est au plus une
  réduction de la table à $O(n)$, sans mesure de high-water ; la suppression
  complète n'est pas implémentée.
- Pour un support d'arité deux, `is_owner` essaie chaque point de la coquille,
  puis `owner_rays_ok` rescane toute la coquille. Le coût est
  $Theta(m^2)$ par paire et peut atteindre $Theta(m^4)$ pour les
  $Theta(m^2)$ paires d'un sommet. La note mathématique demande au contraire
  d'identifier les deux rayons extrêmes de l'intersection de demi-plans. Aucune
  porte à grande coquille ne borne encore ce coût.
- Le différentiel owner partage avec sa référence la navigation, le census, la
  miniboule et la canonicalisation. Il établit une équivalence de
  déduplication relative à ces primitives, pas une exactitude géométrique
  indépendante.
- L'équivalence owner n'est pas rejouée sous permutation. La fixture
  `owner_signed_cone` exigée par la note n'est pas permanente.

### Sink et décisions de filiation

- `kSinkStopped` distingue désormais correctement l'arrêt volontaire d'une
  violation d'invariant. Le test courant arrête toutefois le sink dès le germe ;
  il n'exerce pas un arrêt après un préfixe ni la branche issue d'un enfant.
- Les nouvelles fixtures directes ferment utilement les directions hors
  `{-1,+1}`, une clef retour dépassée et une base divergente. L'oracle et le
  sujet partagent encore `pair_admissible`, ce qui limite leur indépendance.

### Oracle F0

- Normalisation, validation projetée, construction des stamps et résolution des
  racines sont communes à Warshall et DSU. Un `Record` direct incohérent avec
  `raw_arity=11` et aucune source est accepté par les deux chemins. La nouvelle
  identité complète n'est donc pas oracle-vérifiée indépendamment.
- Une large part des obligations ciblées et des mutants repose sur `assert`.
  Sous `python3 -O`, ces contrôles disparaissent mais le script imprime encore
  `Gate_D_F0_kernel=PASS` avec les mêmes compteurs annoncés.
- L'exhaustivité porte sur le pool structurel fixe
  `{R0,R1,L0,N0,N1}` et au plus deux records du générateur. Elle n'est pas
  exhaustive sur les namespaces, provenances, arités, handles source ou records
  parallèles.
- Le mini-sujet transactionnel photographie seulement `next_commit_id` et
  `commits`. Il teste utilement son atomicité propre, mais pas encore le locator,
  la partition, la forêt, la couverture et les journaux revendiqués par
  l'architecture cible.

## Résultats positifs reproductibles

### Cinq portes flats Release

Commande :

```sh
cmake --build /tmp/mhgp3v-audit-release-2E6oBp -j2
ctest --test-dir /tmp/mhgp3v-audit-release-2E6oBp --output-on-failure -R '^mhgp3v_flats_(fixtures|generic|indexed_tree|degenerate|cospherical)$' -j2
```

Compilateur `g++ 13.3.0`, build `Release`. Résultat : **5/5**, 204,28 s de
temps mur avec deux tests en parallèle, 4 980 cas et zéro désaccord.

| porte | cas | owner émises | refus support | refus autre sommet |
| --- | ---: | ---: | ---: | ---: |
| fixtures | 212 | 2 405 | 537 | 2 394 |
| generic | 1 185 | 63 757 | 3 148 | 153 640 |
| indexed tree | 219 | 3 027 | 540 | 4 610 |
| degenerate | 1 292 | 70 874 | 26 489 | 140 992 |
| cospherical | 2 072 | 80 410 | 3 542 | 167 775 |

Les planchers owner sont atteints dans ce snapshot. Ils restent couplés à une
seule option `--min-owner`, avec un seuil dérivé `min_owner/32` pour les refus
de support ; des minima séparés rendraient l'intention de couverture explicite.

### ASan et UBSan

Build temporaire `Debug -O1` avec
`-fsanitize=address,undefined -fno-omit-frame-pointer` :

| campagne | résultat | temps |
| --- | --- | ---: |
| `--clouds 0 --min-cases 150` | 212 cas, zéro désaccord, aucun diagnostic | 32,576 s |
| `--clouds 12 --points 11 --coord 24 --smax 6 --seed 4242 --min-cases 250` | 291 cas, zéro désaccord, aucun diagnostic | 125,939 s |

`--min-owner` est resté à zéro pour séparer la sûreté mémoire de la couverture
statistique. Aucun diagnostic ASan, UBSan ou LeakSanitizer n'est présent.

### Cône signé minimal

Sur les points
`[(0,0,2),(4,0,2),(1,3,2),(2,1,1),(2,1,3)]`, le probe trouve deux sommets
candidats et exactement un propriétaire : coquille `{0,1,2,4}`, intérieur
`{3}`. Le catalogue owner indexé est identique à la référence, avec 22 sphères,
et aucun sanitizer ne se déclenche. Ce résultat crédite la formule locale ; il
doit devenir une fixture permanente et être rejoué sous permutation.

### Noyau F0, crédits limités

L'exécution normale rend :

```text
exhaustive=2168 accepted=1703 rejected=465
targeted=11 invalid=8 permutations=11 arity11=PASS
mutations=10 rollback_faults=5 allocator_mutant_killed=True
Gate_D_F0_kernel=PASS
```

Les clefs `RootSig` sont structurelles, les provenances identiques sont
agrégées, les duplicats contradictoires sont rejetés, les records parallèles
sont conservés, la double-attache vers deux racines est refusée et dix mutants
sont tués en exécution normale. Ces résultats restent utiles une fois le P1 de
sémantique corrigé.

## Ordre de fermeture recommandé à Claude

1. Aligner F0 sur la naissance `q_R=0`, ou publier et prouver la nouvelle
   précondition amont ; ajouter une vérité indépendante pour ce cas.
2. Fermer la matrice de domaine de `use_owner` sans omission silencieuse.
3. Rendre permanentes `owner_signed_cone`, le tétraèdre sans index et le
   triangle direct, puis rejouer owner sous permutations.
4. Remplacer les vérifications F0 contractuelles basées sur `assert` et
   séparer les primitives communes de l'oracle.
5. Implémenter la sélection linéaire des rayons owner et mesurer les grandes
   coquilles avant tout discours de passage à l'échelle.
6. Intégrer ensuite le sink au catalogue transactionnel, retirer réellement la
   table résiduelle et mesurer le high-water complet.

Tant que les points 1 et 2 ne sont pas fermés, les résultats positifs restent
des validations locales et non une autorisation de promotion.

GCP non utilisé.
