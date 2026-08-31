# État courant v6 — audit coopératif

Date de coupe : 31 août 2026. Autorité auditée :
`381ba60b44a3d36dff0ca28c269ded9dffa16080`, présente sur `main` et
`origin/main`. Le répertoire non suivi
`receipts/campagne_stationnaire_20260831/` est une capture distincte épinglée
sur `b17ca2cd`; il n'appartient pas à cette coupe et n'a pas été modifié.

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Verdict courant

Le commit `381ba60b` est reçu comme **checkpoint correctif cover q4 +
digests**, et non comme clôture de J3. Le défaut coefficient 3 est corrigé aux
deux chemins du cover q4, sa contre-fixture est permanente, la monnaie
post-préfiltre possède un tag neuf et un golden reproduit indépendamment, et
la conformité v5↔v6 juge désormais l'objet plutôt que la construction.

La configuration, le build Release et les **51/51 portes rapides** passent sur
ce SHA. Les 20 portes mutantes ajoutées rendent bien le code attendu 4. Le
corps du commit annonce aussi 15/15 portes d'échelle, mais aucun reçu brut
post-correctif ne les épingle : elles ne sont donc pas créditées par cette
contre-lecture.

Le prochain obstacle n'est plus le correctif mathématique. C'est la chaîne de
preuve de coût : deux compteurs dits « publiés » ne sortent pas du CLI, le
scan complet de passe 2 n'est pas compté, `pentes.py` accepte plusieurs
campagnes incomplètes, et le seul dossier de campagne présent mesure encore
`b17ca2cd`. Claude peut conserver le checkpoint et finir ces points sans
rouvrir le cover ou la sérialisation du digest.

## Progrès reçus

### Cover q4

`generate_candidates` choisit maintenant le coefficient 3 pour q3 et 4 pour
q4, aussi bien pour `rect_cover_handles` que pour
`anchor_cover_from_handles`. La lentille de supports reste séparée.

La porte `mhgp6_cover_coef4` grave le tétraèdre régulier et son témoin
intérieur hors cover 3 : nominal `raw=0/rle=0/survivantes=0`; sous
`q4-cover-coef3`, `raw=1/rle=1/survivantes=0` et code 4. Cette porte prouve
causalement la mort à la génération. Les extensions handles/requête directe,
permutation et PointId restent utiles, mais ne bloquent pas ce correctif.

### Monnaies de digest

Les trois rôles sont enfin disjoints :

- `digest_candidates_v5_compat` conserve le tag v4 et décrit les candidats
  uniques post-RLE ; c'est un diagnostic de construction, pas un verdict
  d'objet ;
- `digest_postprefilter` porte le tag
  `mhgp6-digest-v1:postprefilter-candidates` et signe, sans profondeur, les
  records survivants dans leur ordre canonique ;
- `digest_all` et les digests forestiers restent les juges v5↔v6 de l'objet.

Le golden `uniform, n=400`, avec un et quatre fils, est bien
`97be65b6e0c66e3b3b2262510bd7274f8e557ae4bdb78024467c1f9ee05c4d72`.
Le run courant produit `boules_uniques=105076`,
`mortes_profondeur=1134` et `survivantes=103942`. Le mutant `rle-drop` est
correctement repointé sur cette frontière.

### Portes et architecture

La coupe CMake contient 66 tests : 51 portes rapides et 15 tests `scale`.
L'ajout de la boucle de 20 mutants est efficace. La racine des familles
stationnaires utilise maintenant `floor_sqrt`, et plusieurs claims historiques
ont été correctement requalifiés en diagnostics ou chantiers prévus.

## P0 de preuve — publier ce qui est réellement payé

`p_factor[3]` et `sweep_root_comparisons` sont déclarés, incrémentés et
agrégés dans `GenerateStats`, mais `print_run` ne les imprime pas et
`bench/pentes.py` ne les parse pas. Ils sont donc **câblés en mémoire, pas
publiés**. Corriger les sorties et le grand-livre avant toute pente.

La définition courante de `P_factor` ajoute `nA*nA+nB*nB`, alors que
`corner_histograms` saute les diagonales avant d'appeler
`universal_over_corners`. Si le terme désigne les évaluations coûteuses, sa
valeur exacte est `nA*(nA-1)+nB*(nB-1)`. Sinon, le renommer en itérations de
boucle incluant les diagonales et documenter cette différence.

Le compteur du tri compte les appels du comparateur de `std::sort`, pas les
comparaisons d'égalité lors de la formation des groupes. Il dépend du binaire
et de la bibliothèque standard : diagnostic recevable sous toolchain épinglée,
jamais golden sémantique inter-toolchains.

Enfin, `W_sweep2` manque : la boucle qui rescane tout `sc.cover` pour chaque
seed de passe 2 n'a aucun compteur. `P_role=q4_completions` ne compte que les
racines sur corde soumises à la cascade et ne remplace pas ce scan. Ajouter par
exemple `sweep_pass2_site_tests`, l'imprimer, le parser et alors seulement
publier `W_sweep2/W_sweep1`.

## P0 de reçu — rendre `pentes.py` réellement fail-closed

Le durcissement est partiel. Des falsifications sur copies temporaires donnent
encore le code 0 lorsque :

- `STATUS.txt` est absent, réduit à `DONE`, ou contient seulement la
  sous-chaîne `NOT_DONE_YET` ;
- une famille entière manque, car les familles attendues sont inférées des
  sorties présentes ;
- le compteur d'une graine entière manque, affiché alors comme `-` ;
- un fichier `.err` attendu n'existe pas.

Un compteur présent et nul est inversement déclaré « absent ». Quand un
compteur manque partout, le script rend bien 3, mais après avoir déjà imprimé
une table partielle. Le reçu b17 actuel illustre ce dernier cas : le script
émet douze lignes puis échoue sur `W_sweep1_tests_coeur`.

Correctif minimal recommandé :

1. déclarer dans le META la matrice exacte familles × tailles × graines ;
2. prévalider avant tout stdout un `STATUS.txt` dont la dernière ligne est
   exactement `DONE`, avec un unique `code=0` par tuple ;
3. exiger un `.txt` et un `.err` vide par tuple, puis recouper famille, n et
   seed avec l'identité imprimée dans le `.txt` ;
4. exiger chaque compteur dans chaque sortie, tout en distinguant absence,
   zéro légitime et pente mathématiquement indéfinie ;
5. graver ces rejets dans une porte Python dédiée.

## La campagne présente reste une baseline pré-correctif

`receipts/campagne_stationnaire_20260831/` contient réellement 36/36 sorties
pour `terrain_stationnaire`, `scanline_stationnaire`, `uniform` et
`eight_clusters`, 36 codes 0, 36 stderr vides et un `DONE` final. C'est une
baseline exploratoire utile.

Elle ne constitue toutefois pas le rejeu annoncé par la réponse Claude : son
META dit `pin=b17ca2cd`, ses sorties emploient l'ancienne monnaie
`digest_balls`, et elles ne peuvent contenir les nouveaux compteurs. Le META
ne porte encore ni commande, ni toolchain précise, ni état du worktree, ni
heures, ni manifeste de hashes des sorties. Ne pas l'écraser ou la relabeller
post-correctif.

Après l'instrumentation, créer un reçu neuf épinglé au SHA complet avec la
matrice explicitement annoncée. Si la cible est la porte à quatre familles du
grand-livre, la nommer comme telle ; si elle prétend exécuter tout le plan de
tests, ajouter les trois familles dilatées qui y figurent. Cette décision de
périmètre doit précéder le run.

## P1 — fermer le contrat digest sans changer son contenu

Le format du nouveau digest est reçu, mais trois petites finitions évitent des
preuves trompeuses :

- `t_census_ms` démarre avant le census et s'arrête après le calcul du digest
  post-préfiltre ; ce temps est donc aussi ajouté à `t_digest_ms`. Arrêter le
  chrono census immédiatement après le succès de `census_balls` ;
- la porte golden compare seulement le hash. Graver également les trois
  cardinalités `105076/1134/103942`, leur distinction et, sur cette fixture,
  la valeur attendue du diagnostic v5-compatible ;
- une divergence candidat est actuellement qualifiée automatiquement
  d'« attendue ». Elle doit rester non bloquante pour l'objet, mais être
  appelée « divergence diagnostique non jugée » ; les divergences causales
  attendues ont leur propre porte.

Le CLI ne publie rien après un statut d'échec, donc son comportement est sûr.
L'API peut en revanche retourner `digest_postprefilter` avant les gardes de
capacité et les folds. Soit calculer dans une chaîne locale puis publier les
digests seulement au statut terminal, soit documenter tous ces champs comme
provisoires et les vider sur chaque retour d'échec.

Des commentaires périmés restent à aligner : identité du multiensemble v5 en
tête de `generate.hpp` et `conformity_v5.cpp`, ancienne bascule conditionnelle
dans `PROVENANCE.md`, et commentaire CMake qui présente encore
`digest_balls` comme juge.

## P1 — remettre exactement les claims de portes à la vérité

La topologie exacte est : **60 noms** au registre, **63 sites** d'injection
produit et **25 noms distincts** exercés par une porte CTest. Les 27
occurrences `--inject=` comprennent deux doublons des portes sweep ; le mutant
i64 de l'oracle n'est pas un mutant produit. Remplacer partout `~30/60` par
`25/60`.

`PLAN_DE_TESTS.md` conserve deux lignes pour `linked_arcs` : l'ancienne
« barrière de sortie » nomme une cible inexistante et sur-promet, tandis que
la nouvelle « génération/census » borne correctement la portée. Fusionner sur
la seconde. La porte ne rejette pas encore les clés profondeur-zéro
excédentaires et son réétiquetage compare un set de BallKey, pas le
multiensemble `(BallKey, arité, niveau)`.

Les fixtures F1–F5 reçoivent les frontières du sweep, mais elles ne
requalifient ni `sector_kill`, ni `cell_grid` ; leurs mutants ne sont pas
exécutés et la grille ne se construit pas sur ces petits nuages. Garder ces
ports `pending`. `chord_kill` n'a pas davantage sa porte dédiée, même si le
contrôle de corde est effectivement traversé.

Deux autres autorités sont en avance sur leurs portes :

- `mhgp6_families_fixture` vérifie déterminisme, profil, unicité et
  cardinalité, mais ne grave aucun digest v4/v5 ou stationnaire ;
- la porte SHA vérifie les vecteurs FIPS, mais ne force pas encore les chemins
  SHA-NI et portable pour les comparer.

Aligner `PROVENANCE.md`, `REGIMES.md`, les commentaires de source et le plan
sur ces autorités réelles. Le gros du socle est « porté, partiellement
requalifié », pas globalement requalifié. Les labels CTest actuels sont
`gate` et `scale*`; aucun label `oracle` ou `slow` n'est configuré.

Enfin, les exécutables de fixtures doivent respecter leurs propres codes :
`mhgp6_cover_fixture --inject=rle-drop` rend actuellement 0 au lieu de 3 ou
d'un refus 2, et `mhgp6_sweep_fixtures --inject=` rend 3 au lieu de refuser
l'argument vide. Une validation locale des mutants cibles évitera qu'un CTest
mal câblé paraisse vert.

## Portée J3 conservée, sans bloquer le checkpoint

Les renforcements suivants restent utiles mais ne rouvrent pas les P0 reçus :

- `linked_arcs_u16` : égalité complète des clés admissibles après census,
  multiensemble complet sous réétiquetage, puis extension jusqu'aux facettes
  si le terme « sortie » est repris ;
- sweep F1/F4 : attacher les compteurs à l'ancre exacte et pinner les pertes
  mutantes ciblées plutôt que toute divergence d'objet ;
- cover : équivalence handles/requête directe et permutation à PointId
  conservés ;
- racine entière : cas immédiatement de part et d'autre de l'arrondi et du
  clamp.

## Rejeu indépendant

```text
cmake -S morsehgp3D_v6 -B build/v6 -DCMAKE_BUILD_TYPE=Release
  -> code 0

cmake --build build/v6 --parallel
  -> code 0

ctest --test-dir build/v6 --output-on-failure -LE '^scale'
  -> 51/51 passes, 100,64 s réelles

mhgp6_cover_fixture --inject=rle-drop
  -> code 0, défaut de contrat de porte reproduit

python3 bench/pentes.py <copies de campagne falsifiées sous /tmp>
  -> les cas incomplets décrits plus haut rendent encore 0
```

Les 15 tests `scale*` n'ont pas été rejoués indépendamment sur cette coupe.
Aucun résultat GPU n'est revendiqué. GCP non utilisé.
