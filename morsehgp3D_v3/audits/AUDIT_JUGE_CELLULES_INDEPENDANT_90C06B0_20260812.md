# Contre-audit du juge rationnel centre-cell `90c06b0`

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot : `HEAD=90c06b0c436950d29f7617dd6a6765ddf3a8b7fa`.

## Verdict

Le nouveau juge est une **autorité arithmétique indépendante bornée crédible**
pour les supports q2--q4 sur le nuage qu'il reçoit. Il n'emploie ni les lifts,
ni les déterminants, ni la forme de puissance du sujet : il résout le centre
par élimination de Gauss en rationnels multiprécision, lit la positivité dans
les barycentriques et compare directement les distances rationnelles. Son
énumération exhaustive et son census global sont mathématiquement complets sous
coordonnées distinctes et dans sa borne `n<=45`.

Le claim et la gate sont néanmoins sur-vendus :

- sujet et juge partagent `prototype/cloud_families.hpp` et `mhgp::P3`; le juge
  est indépendant de la **géométrie du sujet**, pas de la fabrication de
  l'entrée ni du protocole;
- les quatre accords CTest sont des différentiels `n=32,smax=7`, pas un reçu
  du profil complet `smax=11`;
- la porte censée montrer que le juge voit le mutant ne l'exerce pas.

Le dernier point est P0 pour la réception du juge.

## 1. Vérification mathématique

Pour `U={u_0,...,u_(q-1)}`, le juge écrit
`c=u_0+sum_i lambda_i(u_i-u_0)` et résout exactement
`2(u_i-u_0).(c-u_0)=||u_i-u_0||^2`. La matrice est deux fois la matrice de Gram
des différences. Elle est inversible exactement lorsque `U` est affinement
indépendant.

Les coefficients barycentriques du centre sont
`(1-sum_i lambda_i,lambda_1,...,lambda_(q-1))`. Leur stricte positivité équivaut
à `c in relint conv(U)`, donc à un support propre positif. Le scan rationnel de
tout `X` construit alors exactement l'intérieur strict et le shell global. Le
rejet anticipé lorsque `p+q>smax` est sûr parce que ce support ne doit pas être
publié; aucun shell partiel n'est alors comparé.

Les produits scalaires tiennent dans `long long` sous le profil u16. La CLI du
juge accepte des coordonnées synthétiques plus larges via le générateur; toute
extension au-delà de la borne actuelle exige de refaire la borne avant les
multiplications natives.

## 2. Porte mutant vacueuse

La CMake lance le driver avec `--inject=rank-closed`, mais sans `--judge`. Le
sujet calcule et imprime bien ses identités mutées, puis refuse explicitement
cette combinaison en code 2. Le driver jette alors le stdout au lieu de le
soumettre au juge et transforme le retour du sujet en son propre code 2. CMake
marque pourtant le test `WILL_FAIL TRUE`, ce qui accepte ce refus comme succès.

Rejeu exact au `HEAD` :

```text
REFUS : le sujet rend 2
REFUS : un mutant sans juge ne prouve rien
rc=2
```

La porte `mhgp3v_centre_cell_independant_voit_le_mutant` ne prouve donc pas que
le juge voit une divergence; elle prouve seulement qu'une commande interdite
finit par échouer. Le juge lui-même est sensible : en lui donnant directement
le stdout mutant malgré le code sujet, il rend code 1 avec `510` vérités,
`504` identités sujet et `6` manquantes. C'est le câblage de la porte, pas
l'arithmétique du juge, qui est réfuté.

Réparation contractuelle :

1. autoriser le mutant seulement lorsque `--emit-identities` est actif, ou
   injecter une corruption ciblée du flux d'identités après un calcul sain;
2. exiger que le sujet termine et ferme `IDENTITES`;
3. exiger le code **1** du juge, jamais un non-zéro quelconque;
4. exiger la regex `DESACCORD du juge independant`;
5. ajouter un témoin négatif où sujet absent/refusé rend code 2 et où la porte
   doit rester rouge.

## 3. Portée réelle du `48/48`

Le registre contient `508` tests. La sélection centre-cell plus payload rend
`48/48` en `115,08 s`; la sortie non archivée observée en `/tmp` a pour SHA-256
`23fc8d7a2445a4c4c05d2d3ddac8801048720e6dd08dd753aca8c969d765cb41`.

Les quatre accords indépendants emploient `n=32`, `smax=7`, `seed=5` et les
familles `uniform`, `terrain`, `scanline_single_pass` et
`scanline_overlap_multiecho`. Des rejeux ciblés donnent respectivement
`1 150`, `510`, `437` et `354` identités, avec `222`, `25`, `14` et `11` q4 :
les lanes hautes ne sont pas vacantes. La sortie CTest résumée n'archive ni les
listes d'identités ni leur digest. Le nombre `777` du message de commit est
reproductible sur le défaut `uniform,n=30,smax=6,seed=11`; ce n'est pas le
paramétrage des quatre portes.

Avant de qualifier l'oracle borné :

- publier les comptes et digests d'identités par famille et par arité;
- imposer des planchers q2, q3 et q4 séparés;
- exercer `smax=11` sur au moins une taille bornée;
- tuer séparément dépendance affine, positivité, owner fermé, rang fermé,
  intérieur strict et shell nul;
- graver une fixture rationnelle manuelle indépendante du générateur partagé;
- borner les subprocess par timeout et archiver commande, sources, ELF et
  sortie brute.

## 4. Statut corrigé

Le statut acceptable aujourd'hui est :

`bounded_independent_rational_geometry_candidate_with_four_positive_differentials`.

Il n'est pas encore :

`mutation_sensitive_received_oracle`.

Cette correction ne retire rien aux quatre accords observés. Elle empêche
seulement qu'un `WILL_FAIL` satisfait par un refus soit présenté comme une
preuve de sensibilité scientifique.

GCP non utilisé.
