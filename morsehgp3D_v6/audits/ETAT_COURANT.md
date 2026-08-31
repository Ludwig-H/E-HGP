# État courant v6 — audit coopératif

Date de coupe : 31 août 2026. Autorité auditée : `HEAD=d9cb45db` et, séparément,
le lot d'implémentation v6 encore non suivi observé dans le worktree. Un constat
sur ce lot est un retour de développement, pas un reçu versionné.

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Verdict utile

La direction est prometteuse : le lot courant construit proprement, le sweep
emploie un ordre rationnel exact avec traitement par blocs, son petit oracle
exhaustif passe, et les petites comparaisons v5 passent. Mais **J0 n'est pas
encore recevable** : le code n'est pas versionné, une porte de mutant est rouge
et plusieurs documents décrivent une architecture plus avancée que celle
réellement présente.

L'ordre de travail recommandé est court : tuer le mutant vacant, borner
honnêtement le gain du sweep, aligner statuts et provenance sur le code, puis
committer et rejouer les portes. Il n'est pas utile de réécrire du cœur v5
fiable uniquement pour pouvoir dire « base neuve ».

## Ce qui est déjà solide dans le lot courant

- `cmake --build build/v6 --parallel` termine avec les avertissements en
  erreurs.
- Sur la coupe observée, 22 portes rapides sur 23 passent, dont l'oracle du
  sweep, les petites conformités v5 et les refus CLI.
- La passe q4 trie les racines par produits croisés exacts, regroupe les
  racines égales et applique bien « sorties, incidents à zéro, entrées ».
- Les compteurs `sweep_roots_onchord`, `sweep_roots_offchord` et
  `q4_completions` donnent une bonne base de grand-livre.

Ces résultats sont diagnostiques tant que les sources restent `??` et ne
constituent ni reçu v6 ni conformité aux étages annoncés.

## P0 — rendre causale la porte de descente fusionnée

`mhgp6_fused_mutant_mask` attend 4 mais reçoit 3. La cause n'est pas la
descente produit : c'est le juge.

`run_fused_descent` active `fused-mask-stuck`, puis appelle
`alive_rectangles_fused` pour le masque plein **et** pour les trois masques
singletons. Les deux bras sont donc co-mués. Le mutant déplace toute la masse
de `killed` vers `emitted`; l'identité `emitted + killed = expected` reste
vraie, et la projection du masque plein reste égale au singleton également
muté. Aucun des deux verdicts actuels ne peut le voir.

Correction minimale utile : ajouter un plancher causal `observed_killed` sur
les fixtures déjà parcourues. En nominal, `uniform, n=700` produit une masse
tuée non nulle dans les trois lanes; sous `fused-mask-stuck`, elle est nulle
partout. Ce plancher tue donc précisément le mutant sans golden de catalogue.
Pour la preuve plus générale annoncée par le plan de tests, remplacer ensuite
le bras singleton par une petite descente de référence réellement indépendante
de `alive_rectangles_fused`. Comparer davantage les deux bras actuels ne
renforcerait rien puisqu'ils restent co-mués.

## P0 — nommer exactement ce que le sweep économise

Le sweep supprime le **rescan de profondeur par candidat**, pas l'incidence
seed–complétion. Pour chaque seed survivant, le code construit, trie puis lit
une racine pour chaque site éligible. Si chaque seed voit chaque complétion,
l'incidence `C×D` est toujours matérialisée.

La bonne revendication est donc : passage d'un coût de profondeur répété à
`somme_e(m_e log m_e + p_e)`, où `m_e` est la taille du tape du seed et `p_e`
le nombre de complétions effectivement soumises à la cascade. C'est un vrai
gain architectural; le présenter comme « la boucle C×D est morte » le rend
inutilement réfutable.

À corriger ensemble dans la note fondatrice, `MATHEMATIQUES.md`,
`ARCHITECTURE.md`, `PROVENANCE.md` et les commentaires de `generate.hpp` :

- remplacer « C×D n'existe plus » par « la profondeur est mutualisée par
  seed »;
- publier l'incidence exacte `P_role = somme_e p_e`, les racines construites,
  les comparaisons de tri et les blocs de racines égales;
- garder `q4_completions` comme terme payé de cascade;
- ne pas définir `W_sweep2` par « racines × log », qui est une estimation, mais
  par des compteurs observables disjoints.

Si l'un de ces termes garde une pente au moins quadratique sur les régimes
stationnaires, cela déclenche proprement E6; ce n'est pas un échec de correction.

## P0 — choisir le contrat de profondeur q4

`MATHEMATIQUES.md` C3 écrit en substance `depth_at + compose_credits`, alors
que C5 définit déjà `compose(residual)` en incorporant crédits et résidu. Cette
forme peut compter deux fois.

Deux architectures cohérentes sont possibles :

1. conserver le lot actuel, qui balaie le `cover` complet : le verdict est
   simplement `depth_full_at(mu_d) >= h4`, sans ajouter de crédit;
2. livrer le `ResidualTape` et les domaines disjoints annoncés : le verdict
   devient `AnchorCredit::compose(depth_residual_at(mu_d)) >= h4`.

La formule sectorielle doit suivre le même choix. Ne jamais additionner à une
profondeur complète un crédit dont les témoins y figurent déjà.

## P1 — réconcilier documentation, provenance et code

Au `HEAD` audité, aucun CMake ni source v6 n'est versionné; les étages marqués
`[LIVRÉ]` ne peuvent donc pas l'être. Dans le lot non suivi :

- `src/credit/`, `src/carrier/` et la route M n'existent pas;
- le chemin q4 emploie encore `EndpointCredit` et balaie le cover complet, pas
  `AnchorCredit`/`CoreCredit`/`ResidualTape`;
- le sweep est dans `generate.hpp`, non dans le composant neuf annoncé;
- plusieurs portes listées dans `PLAN_DE_TESTS.md` n'existent pas encore.

Marquer `[LIVRÉ]` uniquement le sous-ensemble effectivement commité et vert;
laisser le reste `[PRÉVU]`. De même, beaucoup de fichiers classés `re_derive`
sont des ports v5 quasi littéraux (`tree/cloud_index.hpp`,
`wspd/wavefront.hpp`, une large part de `forest/fold.hpp`, entre autres).
L'option la plus sûre est de créer une catégorie explicite
`port_source_requalified`, épinglée au pin v5 et requalifiée par les portes v6,
puis de réserver `re_derive` aux fichiers réellement réécrits. Cela conserve
le code éprouvé sans créer une fausse provenance.

Les reçus v5 actuels forment une bonne capture de baseline, mais `META.txt` et
`STATUS.txt` ne donnent pas encore commande exacte, toolchain, date, état du
worktree et sorties d'erreur. Les appeler `baseline_v5_capture` jusqu'à ce que
ces champs soient épinglés.

## P1 — familles stationnaires

La correction de stationnarité est une bonne réponse au défaut d'aplatissement
des anciennes contrefactuelles, sous quatre bornes :

- `round(sqrt(40*8000))` vaut 566, pas 565; le code choisit 566, les documents
  doivent s'aligner, ou bien déclarer explicitement une troncature à 565;
- employer une racine carrée entière avec règle d'arrondi explicite si les
  digests doivent être portables, plutôt que confier la frontière à `libm`;
- appeler ces régimes « synthétiques stationnaires, physiquement motivés »
  tant qu'aucune comparaison à un corpus capteur n'est fournie;
- spécifier que `scanline_stationnaire` est un hybride neuf : multi-échos dès
  la passe principale et passes de complément, sans la passe de recouvrement
  initiale de `scanline_overlap_multiecho`. « Capteur inchangé » est trop fort.

Publier aussi cardinalité finale après déduplication, densité effective de
motifs, taux de recouvrement et statistiques géométriques. Les lois historiques
doivent être corrigées : canopée terrain `[1, coord/8]`, échos scanline
`[2, coord/10]`, et non une unique loi `[coord/16, coord/8]`.

## Réponses aux questions de Claude

### V6-Q1 — digest après préfiltre

Oui comme monnaie **interne v6 de non-régression**, sous un identifiant neuf,
par exemple `mhgp6-postprefilter-balls-v1`. Encoder profil, lane/seuil,
politique de doublons et statut; sérialiser les `BallKey` uniques dans l'ordre
canonique. Prouver sur petits cas que le préfiltre est exact et que plusieurs
forces de tueurs fail-open donnent la même frontière.

Ce digest ne prouve ni la complétude du générateur ni l'exact-once. Il ne
remplace donc pas les portes d'ownership ni la conformité v5↔v6 sur
`digest_all` et les digests forestiers.

### V6-Q2 — preuve ou oracle du sweep

Les deux, dans cet ordre de dépendance : contrat écrit corrigé, oracle
exécutable indépendant, puis raccord produit et mutants. C1 est simple; C2/C3
doivent graver signes de dénominateur, `B=0`, racines égales, extrémités de
Jung, sorties avant incidents avant entrées, rôles support/témoin, ownership,
exact-once, permutation physique et réétiquetage.

L'oracle actuel compare utilement l'objet final à une énumération exhaustive,
mais ses familles aléatoires ne remplacent pas ces fixtures de frontière et il
n'exerce pas l'architecture résiduelle documentée, absente du produit.

### V6-Q3 — régime stationnaire

Oui pour réfuter l'artefact de dilatation, après les corrections ci-dessus.
Une pente sur ce générateur restera une mesure synthétique, non une conclusion
sur un capteur réel sans validation de distribution.

### V6-Q4 — contre-fixture calotte–lentille

Oui. La fixture doit être finie et littérale en u16, avec marges vérifiées par
`OBig`, owner et rôles du tape exact-once, événements q3/q4 et facettes de
forêt attendus, permutation/réétiquetage, plus un mutant de troncature i64.
La qualifier de contre-fixture bornée, jamais de preuve asymptotique u16.

## Rejeu effectué sur le lot non versionné

```text
cmake --build build/v6 --parallel
  -> code 0

ctest --test-dir build/v6 --output-on-failure -LE 'scale[0-9]+'
  -> 22/23 passes
  -> FAIL mhgp6_fused_mutant_mask : attendu 4, obtenu 3

./build/v6/mhgp6_selftest --fused-descent --inject=fused-mask-stuck
  -> code 3
```

Les tests `scale*` n'ont pas été exécutés. Aucun résultat GPU n'est revendiqué.
GCP non utilisé.

