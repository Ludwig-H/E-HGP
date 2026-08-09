# Audit courant de MorseHGP3D v3

Date du snapshot : 9 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_under_audit`,
`profile=quantized_u16_input_only`,
`mode=order_k_flats_owner_differential_and_gate_d_f0`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype et
ne vaut ni promotion produit, ni ouverture de phase. Les résultats concernent
le worktree courant construit sur le commit de base suivant :

| objet | empreinte |
| --- | --- |
| `HEAD` | `f3682632c490599aa6d74dae42e69038ac65f9b9` |
| `prototype/order_k_flats.hpp` | `76ab24712d77beb336caef6cbb63137ccfa3f5f81ec58cd5499a59889e7f1de1` |
| `prototype/flats_differential.cpp` | `a1d5f842407a09f68c948fe21653a83b38a60e5f046f62432a8997589ad1fd90` |
| `CMakeLists.txt` | `fdc00942cc8aed26f46c40ad3a95ef7be040d968ff819fd1ffb9368f171946c4` |
| `audits/check_gate_d_fold_f0.py` | `34149092cd1b06762085800ac9d575c0cb8022e3a1c273c7d1955d2f4e768294` |

## Verdict

**GO ciblé pour la correction de justesse du domaine `use_owner`; NO-GO pour
présenter la porte permanente comme un certificat complet, NO-GO pour un mode
globalement sans table, et NO-GO pour déclarer F0-A mathématiquement validée.**

La suppression silencieuse des chemins sans sommet propriétaire est corrigée.
Aucun contre-exemple n'a été trouvé sur 174 444 exécutions indépendantes qui
comparent statuts et payloads complets dans les quatre quadrants
`use_index` × `use_owner`. Le chemin owner indexé navigable garde désormais
`emitted` vide.

Deux verrous restent prioritaires :

1. la fixture permanente de domaine ne compare que le nombre de sphères et leur
   histogramme d'arités; son message « même catalogue » dépasse ce qu'elle
   vérifie ;
2. le juge F0 rejette une naissance composée de facettes activées dans le lot,
   alors que son contrat écrit l'autorise. La vérité Warshall et le sujet DSU
   partagent ce rejet, donc leur accord ne le révèle pas.

## Résultat positif — correction du domaine `use_owner`

Le live définit maintenant `owned_path` seulement si `use_owner` est demandé et
si l'émission est portée par un vrai sommet navigué. Les singletons sans index et
la voie directe n'ont pas de sommet de $P_U$ : ils utilisent légitimement le
repli `emitted`. La voie directe et la navigation sont exclusives; sous refus des
coordonnées dupliquées, une coquille singleton ne peut pas entrer en collision
avec une émission owner d'arité au moins deux.

Les sorties minimales sont rétablies dans les quatre quadrants :

| nuage | sortie attendue et obtenue | table `emitted` sans index / owner sans index / index / owner+index |
| --- | --- | --- |
| tétraèdre affine 3D | 11 sphères, arités 4 / 6 / 1 / 0 | 11 / 4 / 11 / **0** |
| triangle direct | 7 sphères, arités 3 / 3 / 1 / 0 | 7 / 7 / 7 / 4 |
| cône signé à cinq points | 22 sphères | 22 / 5 / 22 / **0** |

Une sonde indépendante a étendu la matrice au triangle, à quatre points
alignés, à cinq points coplanaires, au tétraèdre et au cône signé, avec ordres
hostiles et statuts de domaine. Sur une campagne plus large : 2 181 nuages,
174 444 exécutions, 5 821 968 records et 41 430 permutations, dont 40 320 du
cube et 120 du cône signé. Statut, ordre, support complet, arité, rang,
`members_begin`, membres, `beta` et classifications exactes concordent. Aucun
contre-exemple n'est trouvé.

Une mutation temporaire qui rétablit `owned_path=use_owner` est tuée : code
non nul, quatre singletons perdus sur le tétraèdre et non-singletons perdus sur
le triangle. Le correctif de justesse est donc crédité sur les empreintes de cet
audit.

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

## P1 de qualification — la porte owner permanente est trop faible

- La fixture « domaine owner » calcule `pstatus` mais ne le compare pas. Elle
  confronte seulement `(nombre de sphères, histogramme d'arités)`, sans imposer
  les vérités attendues 11 et 7, ni supports, rangs ou membres. Deux catalogues
  différents de même profil passent; le message « même catalogue » est donc
  abusif. Le triangle à trois points exerce en outre `kTooFewPoints`, pas
  `kAffineDimensionBelowThree` avec au moins quatre points.
- La nouvelle équivariance owner est positive, mais sa signature est un `set` de
  `(support, rang)`. Elle masque les multiplicités et ne transporte pas les
  membres. La sonde externe ferme ces cas sur le snapshot, pas la porte
  permanente.
- La fixture `owner_signed_cone` exigée par la note reste externe. Elle doit
  devenir permanente avec ses deux sommets candidats et son propriétaire
  attendu.
- Le différentiel owner partage avec sa référence la navigation, le census, la
  miniboule et la canonicalisation. Il établit une équivalence de
  déduplication relative à ces primitives, pas une exactitude géométrique
  indépendante.

## P2 — architecture, mémoire et coût

- `emitted` est réellement vide pour owner+index+navigable. Sans index, les
  singletons conservent $O(n)$ clefs; sur la voie directe, le repli peut rester
  en $\Theta(\text{sortie})$. `use_owner` est donc un mode hybride, pas une
  garantie globale sans table.
- `dedup_table_size` publie la taille finale, pas un high-water ni les octets
  alloués. La table ne décroît pas aujourd'hui, mais une mutation qui la vide en
  fin de calcul tromperait cette porte. Il faut mesurer le maximum lors des
  insertions et la mémoire complète.
- Pour les arités deux et trois, les scans imbriqués peuvent coûter
  $\Theta(m^4+m^3\lvert B_U\rvert)$ par sommet. La note demande de sélectionner
  directement les deux rayons extrêmes de l'intersection de demi-plans. Aucune
  porte à grande coquille ne borne encore ce coût.
- `owner_context` balaie encore les points même lorsque `use_owner=false`.
- La terminologie du cube est clarifiée dans le README et la note : quatre
  supports de cardinalité minimale, six supports inclusion-minimaux. Quelques
  commentaires du différentiel et du header emploient encore « minimal » sans
  préciser la notion. Le commentaire de la voie directe affirme aussi que le
  différentiel ne l'oppose jamais au mode owner, alors que la nouvelle fixture
  les confronte désormais, quoique partiellement.

### Sink et décisions de filiation — crédits

- `kSinkStopped` distingue l'arrêt volontaire d'une violation d'invariant. Le
  test exerce maintenant l'arrêt au germe et après un préfixe de trois sommets;
  l'intégration transactionnelle de ce préfixe au catalogue reste ouverte.
- Les directions hors `{-1,+1}`, une clef retour dépassée et une base divergente
  sont couvertes. `judge_admissible` recalcule les signes sans appeler le
  `pair_admissible` du sujet; cette indépendance locale est créditée.

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

Compilateur `g++ 13.3.0`, build `Release`. Résultat : **5/5**, 222,60 s de
temps mur avec deux tests en parallèle, 4 985 cas et zéro désaccord. La table
résiduelle maximale vaut zéro dans les cinq campagnes owner indexées.

| porte | cas | owner émises | refus support | refus autre sommet |
| --- | ---: | ---: | ---: | ---: |
| fixtures | 213 | 2 405 | 537 | 2 394 |
| generic | 1 186 | 63 757 | 3 148 | 153 640 |
| indexed tree | 220 | 3 027 | 540 | 4 610 |
| degenerate | 1 293 | 70 874 | 26 489 | 140 992 |
| cospherical | 2 073 | 80 410 | 3 542 | 167 775 |

Les planchers owner sont atteints dans ce snapshot. Ils restent couplés à une
seule option `--min-owner`, avec un seuil dérivé `min_owner/32` pour les refus
de support ; des minima séparés rendraient l'intention de couverture explicite.

### ASan et UBSan

Build temporaire `Debug -O1` avec
`-fsanitize=address,undefined -fno-omit-frame-pointer` :

| campagne | résultat |
| --- | --- |
| fixtures | 213 cas, zéro désaccord, aucun diagnostic |
| petite campagne | 226 cas, zéro désaccord, aucun diagnostic |

Trois petites campagnes Release supplémentaires rendent 343, 368 et 277 cas,
toutes avec zéro désaccord. Aucun diagnostic ASan, UBSan ou LeakSanitizer n'est
présent sur les deux campagnes instrumentées.

### Cône signé minimal

Sur les points
`[(0,0,2),(4,0,2),(1,3,2),(2,1,1),(2,1,3)]`, le probe trouve deux sommets
candidats et exactement un propriétaire : coquille `{0,1,2,4}`, intérieur
`{3}`. Le catalogue owner indexé est identique à la référence, avec 22 sphères,
et aucun sanitizer ne se déclenche. Les 120 permutations de la sonde externe
concordent. Ce résultat crédite la formule locale; il doit encore devenir une
fixture permanente du différentiel.

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
2. Renforcer la porte owner avec statut, payload complet, vérités 11/7, un vrai
   nuage affine de dimension basse avec au moins quatre points et des
   multiplicités conservées sous permutation.
3. Rendre permanente `owner_signed_cone` avec propriétaire attendu.
4. Remplacer les vérifications F0 contractuelles basées sur `assert` et
   séparer les primitives communes de l'oracle.
5. Implémenter la sélection linéaire des rayons owner et mesurer les grandes
   coquilles avant tout discours de passage à l'échelle.
6. Documenter le domaine hybride du repli, puis intégrer le sink au catalogue
   transactionnel et mesurer le high-water complet plutôt que la taille finale.

Tant que les points 1 et 2 ne sont pas fermés, les résultats positifs restent
des validations ciblées et non une autorisation de promotion.

GCP non utilisé.
