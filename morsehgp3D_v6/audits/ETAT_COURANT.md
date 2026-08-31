# État courant v6 — audit coopératif

Date de coupe : 31 août 2026. Autorité auditée :
`d153e1be560ea3f182f29fbdc852f4dadb98c0bc`, présente sur `main` et
`origin/main`. La campagne non suivie
`receipts/campagne_grandlivre_20260831/` tourne sur ce SHA ; elle reste hors
verdict jusqu'à son `DONE` et ses contrôles terminaux.

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Verdict

Le checkpoint mathématique `381ba60b` reste **reçu** : coefficient 4 sur les
deux covers q4, contre-fixture causale permanente, digest post-préfiltre neuf
et conformité v5↔v6 jugée sur les forêts et `digest_all`. Il n'est pas rouvert
par les réserves ci-dessous.

Le reçu `122e9c57` est reçu comme **baseline descriptive post-correctif**. Sa
matrice, ses sorties, ses hashes et ses ledgers sont valides. Son claim
« E6 déclenché proprement » est requalifié en **signal de triage pour la sonde
E6** : les deux dépassements viennent de la seule graine 5, et le compteur
nommé `W_sweep1` ne compte pas encore toutes les entrées parcourues.

`d153e1be` apporte des progrès utiles, mais ne « ferme » pas la chaîne de
preuve de coût. Il publie trois compteurs attendus et durcit plusieurs portes ;
il laisse toutefois cinq P1 concrets dans les contrats d'échec, de capacité,
de validation de campagne, de WSPD et de comptage. Claude peut corriger ce lot
sans rouvrir le cover, les digests ou le sweep mathématique.

Ordre conseillé : fermer d'abord les faux certificats et retours d'échec,
puis rendre le validateur et les compteurs exacts, puis utiliser la campagne
en cours comme baseline enrichie. La sonde d'ancres lourdes est justifiée ; un
nouveau run de décision ne l'est qu'après ces corrections.

## Ce qui est reçu dans `d153e1be`

- `p_factor` est imprimé et sa valeur est corrigée en
  `nA*(nA-1)+nB*(nB-1)`, conformément aux appels réellement effectués par
  `corner_histograms` ; `tri_comparaisons` et `tests_passe2` sont également
  imprimés.
- `t_census_ms` s'arrête avant le digest post-préfiltre.
- Le narrowing du juge `--n=4294967696` / `--threads=4294967300` est fermé :
  le cas reproduit rend maintenant le code 2.
- Le chargeur refuse les hex invalides, les doublons de K, les K hors
  `[1,10]` et plusieurs références tronquées.
- `linked_arcs_u16` compare désormais dans les deux directions les clés q3/q4
  à profondeur zéro après census et compare le multiensemble
  `(BallKey, arité, niveau)` sous réétiquetage. Sa portée reste bornée à
  `n={2,4,8,16}` et à génération→census.
- Les neuf familles ont un digest de flux gravé à `n=2000`, graine 3 ; le
  mutant `family-scanline-overshoot` est raccordé. La porte SHA exerce le
  chemin portable et, quand disponible, SHA-NI.
- Les fixtures cover et sweep refusent respectivement un mutant hors cible et
  `--inject=` vide.
- Le CMake expose 68 tests : 53 rapides et 15 `scale`. Le reçu
  `receipts/portes_echelle_20260831/` rapporte 15/15 en 903,41 s sur le lot
  pré-commit correspondant ; les temps de machine partagée ne sont pas des
  mesures. Une contre-lecture légère a rejoué 9/9 portes ciblées.

## Reçu stationnaire `122e9c57`

La capture est structurellement complète : quatre familles, trois tailles,
trois graines, soit 36 tuples uniques, 36 codes 0, un `DONE` terminal, 36
stdout et 36 stderr vides. Les 36 hashes de stdout ont été revérifiés ; le
hash du binaire a été constaté avant son remplacement par le build suivant.
Chaque sortie contient les dix lignes K1–K10, les treize digests attendus et
un `rss_max_kb` terminal.

Les invariants suivants ferment sur les 36 sorties : sommes des cardinalités
par K, `boules_uniques = mortes_profondeur + survivantes`, partitions des
ancres et des seeds, partition des verdicts q4 et, pour chaque lane,
`émis + tués = n*(n-1)/2`. `PENTES.txt` est octet pour octet la sortie du
`bench/pentes.py` épinglé à `381ba60b`.

Réserves de lecture :

- la borne uniform annoncée `[1,03 ; 1,20]` est fausse ; les 26 compteurs
  extraits sont au moins dans `[1,01 ; 1,20]` ;
- « tous les termes publiés » est faux : cette baseline omet notamment
  `p_factor`, les comparaisons de tri, `W_sweep2` et plusieurs champs déjà
  sortis ;
- les valeurs de `tests_coeur` sont exactes : terrain graine 5 donne
  `1,481/2,081`, scanline graine 5 `1,280/2,407`. Elles justifient un
  diagnostic de queue, pas une pente établie, car `REGIMES.md` déclare toute
  conclusion mono-graine sans valeur ;
- la provenance est forte pour la capture, mais pas « complète » au sens
  source→binaire : le META épingle le pin, le hash binaire et un sous-ensemble
  propre du worktree, sans log d'un rebuild après le commit.

Ne pas régénérer rétroactivement ce reçu avec le nouveau schéma. Le conserver
comme baseline immuable des 26 compteurs qu'il contient.

## P1 — centraliser l'invalidation des résultats sur échec

Le commentaire « aucun champ de digest n'est publié sur un retour d'échec »
est faux. Les `clear()` ajoutés ne couvrent ni tous les retours ni tous les
champs provisoires.

Deux probes minimales sur `d153e1be` :

```text
fold-inject-a-failure-k2 : status=4 raw=0 candidates=0 postpref=0
                          forest_slots=11 forest1=64 all=0 cards1_events=1160

census shell_cap=4 + diagnostic raw : status=2 raw=64 candidates=0
                                      postpref=0 forest_slots=0
```

Un défaut K2 expose donc encore la forêt et les cardinalités K1 ; un défaut de
census expose encore le digest raw. Choisir un contrat unique : soit tous ces
champs sont explicitement provisoires, soit une routine terminale commune vide
digests, forêts, cartes et totaux sur **chaque** retour non complet. Graver les
deux cas ci-dessus ainsi que la sûreté `fold_inflight`.

## P1 — capacité : refuser par statut, avant le narrowing

La garde ajoutée à `prefilter_balls` évite le cast u32, mais lance
`std::length_error`. `run_pipeline` ne l'intercepte pas : la voie produit sort
du contrat `PipelineStatus` au lieu de rendre `resource_exhausted`. Extraire un
helper de capacité testable, faire la garde dans le pipeline avant l'appel et
conserver une défense interne cohérente.

Les autres narrowings restent ouverts : `CloudIndex` convertit les
cardinalités/plages en `int`/i32 et les offsets en u32. Les agrégations u64 des
compteurs, dont `p_factor` et les comparaisons de sweep, peuvent aussi déborder
silencieusement. Déclarer les plafonds, saturer avec un drapeau ou passer les
sommes en u128 ; tester les frontières sans allocation géante.

Le chargeur du juge a encore un trou pour un petit `kmax` : il vérifie seulement
`forest.size() >= kmax_eff`, pas l'ensemble exact `{1,...,kmax_eff}`. À `n=2`,
une référence avec le bon `digest_all` et seulement `digest_forest_K10` peut
éviter toute comparaison K1. Exiger chaque clé attendue et graver
missing-K1/extra-K10.

## P1 — `pentes.py` n'est pas encore fail-closed

Le durcissement ferme les défauts les plus simples, mais pas la recette
annoncée :

- un tuple STATUS supplémentaire, même `code=1`, est ignoré ; les fichiers
  `.txt/.err` supplémentaires le sont aussi ;
- l'identité recoupée omet `s`, `smax`, `threads` et le mode digest annoncés ;
- les compteurs sont validés famille par famille puis la table est imprimée :
  une famille tardive invalide laisse donc un stdout partiel ;
- `P_factor_q2` n'est pas analysé alors que q2 paie aussi
  `corner_histograms` ;
- aucune porte Python CTest ne grave matrice exacte, extra tuple, fichier
  manquant, compteur nul, compteur absent ou absence de stdout partiel.

Construire toute la matrice en mémoire, comparer exactement les ensembles de
tuples/fichiers, valider toutes les identités et tous les compteurs, puis
seulement imprimer. Le reçu en cours doit être refusé s'il manque un champ du
nouveau schéma ; l'ancienne baseline reste liée au script de son pin.

## P1 — définir exactement le coût avant de conclure E6

`q4_core_site_tests` et le nouveau `sweep_pass2_site_tests` sont incrémentés
après le rejet des trois indices du seed. Ils comptent les évaluations
éligibles, pas toutes les entrées de `sc.cover` parcourues comme le promet
« sites scannés » / « rescan complet ». Compter les itérations avant le
`continue` et, séparément, les prédicats coûteux après celui-ci, ou renommer
les termes. Une pente du sous-compteur ne prouve pas la pente du scan complet.

Le grand-livre n'est pas complet : `V_wspd` au sens nœuds + appels témoins,
les comparaisons de regroupement des racines, `H_rect`, `H_scan`, `M_anchor`,
`V_census` et plusieurs kills restent absents ou candidats. Le parser omet
aussi q2. La campagne active `campagne_grandlivre_20260831/` sera donc au plus
une **baseline enrichie des champs présents**, jamais une décision J3 ni une
preuve de grand-livre complet. Elle peut être conservée si provenance, matrice,
hashes et invariants ferment.

## P1 — la nouvelle porte WSPD tue une annihilation triviale

Le point produit existe désormais, ce qui est un progrès. Mais la condition
`mutant && lout[c].empty()` laisse `lout[c]` vide après le premier saut et
supprime donc **tous** les rectangles terminaux de chaque chunk, pas « un
rectangle vivant ». Le même nom a une autre sémantique dans
`wspd/wavefront.hpp`, où un `drop_pending` ne perd qu'un élément.

Employer un booléen `drop_pending` à portée déclarée, puis ajouter une porte
littérale `count_mutant = count_nominal - 1`, grand-livre fermé et ownership
indépendant de chaque paire/masque/cœur. La conformité qui tue une sortie
presque annihilée est un smoke test, pas la requalification de la descente
fusionnée.

## P1/P2 — claims et documentation encore en avance

La réponse Claude supprimée après incorporation affirmait avoir modifié
`PROVENANCE.md`, fusionné les deux lignes `linked_arcs` du plan et remplacé
les comptes de mutants. `git show --stat d153e1be` ne contient aucun fichier
de `docs/`, et les textes restent inchangés : ancienne bascule conditionnelle
du digest, deux lignes linked-arcs, et « barrière de sortie » active.

Autres corrections factuelles :

- `floor_sqrt` corrige exactement l'approximation, mais initialise encore avec
  `std::sqrt`; les claims « jamais libm » de `families.hpp`/`REGIMES.md` sont
  faux tant qu'un isqrt réellement entier n'est pas employé ;
- `linked_arcs_u16` exhibe un motif borné de croissance quadratique jusqu'à
  n=16 ; il ne prouve pas à lui seul une asymptotique ni une « sortie » forêt ;
- la topologie courante est 60 noms, **64 callsites réels** (63 sous `src/`,
  un sous `oracle/`) et 27 noms distincts exercés par CTest. Le « 63 sites »
  de la réponse est antérieur au nouveau callsite produit ;
- les 68 tests n'ont que les labels `gate` et `scale*`; aucun label `oracle`
  ou `slow` n'est configuré.

Les nouveaux rejets du juge, des fixtures, de capacité et du validateur sont
des changements sémantiques : ils doivent recevoir leurs propres portes CTest,
pas seulement des probes manuels.

## Renforcements non bloquants du checkpoint

- golden post-préfiltre : graver aussi `105076/1134/103942` et le digest
  candidat v5-compatible ;
- cover : équivalence handles/requête directe et permutation PointId ;
- sweep : pinner la perte exacte de chaque mutant, pas toute divergence
  d'objet ;
- API de fold : clarifier que les callbacks K sont provisoires jusqu'au statut
  global, ou fournir un événement terminal commit/abort ;
- racine stationnaire : cas immédiatement autour de l'arrondi et du clamp.

## Rejeux et statut

```text
381ba60b : configure/build Release -> code 0
381ba60b : portes rapides indépendantes -> 51/51
d153e1be : portes ciblées légères de contre-lecture -> 9/9
d153e1be : reçu Claude des portes scale -> 15/15, 903,41 s
122e9c57 : campagne stationnaire -> 36/36, hashes et ledgers vérifiés
```

La suite rapide 53/53 est rapportée par Claude mais n'a pas encore un reçu
brut indépendant sur `d153e1be`. Aucun résultat GPU n'est revendiqué. GCP non
utilisé.
