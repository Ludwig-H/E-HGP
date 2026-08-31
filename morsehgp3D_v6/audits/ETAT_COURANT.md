# État courant v6 — audit coopératif

Date de coupe : 31 août 2026. Autorité auditée :
`b17ca2cd80464d14d83f79dfdf891354223904bf`, présent sur `main` et
`origin/main`. Les répertoires non suivis `morsehgp3D_v6/bench/` et
`morsehgp3D_v6/receipts/campagne_stationnaire_20260831/` appartiennent au
chantier J3 en cours : ils sont exclus de cette coupe et n'ont pas été touchés.

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Verdict courant

Le commit est un **checkpoint J0–J2 reproductible et utile**, mais pas encore
une réception du « pipeline v6 complet ». La correction locale de la descente
fusionnée est reçue techniquement : le mutant `fused-mask-stuck` est tué
causalement et les 23 portes rapides passent. Le reçu J2 existe, ses trois
hashes correspondent aux binaires du build courant, et son inventaire est bien de
23 portes rapides plus 15 conformités de taille.

La portée doit toutefois rester bornée. Un contre-exemple exact montre que le
sweep q4 balaie un cover coefficient 3 au lieu du coefficient 4 requis; le
préfiltre aval retire cette boule avant la frontière finale et masque ainsi la
faute locale de génération, sans que ce probe démontre l'égalité de l'objet
complet. Le digest conserve en outre la frontière v5 post-RLE alors que la
documentation annonce une monnaie post-préfiltre. Enfin, 4 mutants seulement
sur 59 sont réellement injectés par CTest, plusieurs blocs d'architecture marqués
`[LIVRÉ]` n'existent pas, et le grand-livre omet encore des coûts payés.

Ces défauts sont localisés et réparables. Claude peut conserver ce commit
comme **checkpoint borné**, corriger d'abord le cover q4 et les deux monnaies
de digest, puis aligner les claims/tests avant de nommer J2 complet.

## Corrections reçues

### Descente fusionnée

`run_fused_descent` possède maintenant deux détecteurs qui ne reposent plus
sur la comparaison de deux bras co-mués :

- toute lane émise doit avoir `core[q] < h_q`;
- sur `uniform, n=700`, chaque lane doit présenter une masse tuée non nulle.

Sous `fused-mask-stuck`, les deux détecteurs voient la surémission et la porte
rend 4. Le remplacement ultérieur du bras singleton par une référence
indépendante reste un renforcement utile, pas un bloqueur de ce mutant.

### Contrat du sweep

`MATHEMATIQUES.md` décrit maintenant le gain exact : le sweep mutualise le
rescan de profondeur; il ne supprime pas l'incidence seed–complétion. La forme
`O(m_e log m_e + p_e)` par seed survivant est cohérente avec le code, où
`p_e` reste payé par `q4_completions`.

Le choix mathématique C3 est sain : un chemin qui balaie réellement le cover
complet doit comparer directement `depth_full_at(mu_d) >= h4`, sans crédit
ajouté. L'implémentation courante ne satisfait toutefois pas encore cette
prémisse pour q4, comme le montre la contre-fixture ci-dessous. Le contrat
résiduel `compose(depth_residual_at(mu_d))` reste futur.

### Provenance et régimes

La catégorie `port_source_requalified` est la bonne taxonomie pour un bloc
porté dont les portes ont réellement été rejouées; elle ne doit pas encore
qualifier globalement tout le socle. La gate familles ne grave aucun golden,
le mutant `family-scanline-overshoot` n'est pas raccordé, la porte SHA ne
compare pas explicitement le chemin portable au chemin accéléré, et seuls 4
mutants sur 59 passent par CTest. Employer `port_source_pending_requalification`
pour ces blocs jusqu'à leur porte dédiée. Le vocabulaire « synthétique
stationnaire, physiquement motivé », la valeur 566 et la qualification de
`scanline_stationnaire` comme hybride neuf sont reçus.

## P0 — le cover q4 courant perd des témoins intérieurs

`edge_cover.hpp` exige le coefficient 4 pour les intérieurs et la coquille q4.
Le commit audité recalcule bien les handles pour chaque lane, mais
`generate.hpp` impose le coefficient 3 aux deux appels
`rect_cover_handles` et `anchor_cover_from_handles`, y compris sur la lane q4.
Le sweep ne peut donc pas compter certains intérieurs q4.

Contre-fixture u16 exacte, avec PointId dans l'ordre indiqué :

```text
a[id=0]=(110,110,110)  b[id=1]=(110,90,90)
x[id=2]=(90,110,90)    y[id=3]=(90,90,110)
z[id=4]=(83,100,100)
```

Les quatre premiers points forment un tétraèdre régulier, `D2=800`, et z est
strictement intérieur à sa boule : `BallKey.power(z)=-11`. Pour l'ancre
`(a,b)`, `|2z-a-b|2=2916`, donc z est hors du cover 3 (`3*D2=2400`) mais dans
le cover 4 (`4*D2=3200`). Un probe indépendant sur le commit audité donne :

```text
covers coef3_size=4 coef3_has_z=0 coef4_size=5 coef4_has_z=1
candidate_boundary raw_target=1 rle_target=1 q4_depth_killed=0
prefilter_boundary target_survivors=0 dead_depth=1
```

À `smax=4`, donc `h4=1`, la boule fautive est ainsi présente brute et après
RLE; le préfiltre aval retire cette BallKey seulement ensuite. Ce masquage
local réfute le « cover complet » et l'équivalence du filtre de profondeur à
la frontière de génération. Il ne prouve pas à lui seul une divergence avec
les candidats v5, qui peuvent partager le même défaut coefficient 3, ni une
divergence de l'objet final.
L'oracle courant masque exactement la faute car il compare après
`prefilter_balls`.

Correction minimale sûre : conserver les recalculs par lane et choisir le
coefficient 3 pour q3, 4 pour q4, aux deux appels. Cela évite d'élargir q3 et
de déplacer ses seuils ou son attribution de coût. Une mutualisation future
peut construire à 4, mais doit alors maintenir deux vues filtrées distinctes;
la lentille de supports reste séparée, car les nouveaux sites q4 sont des
témoins de profondeur et non de nouveaux supports.

La fixture doit appeler `generate_candidates` avant tri/RLE/préfiltre, en un
fil, et imposer zéro occurrence de cette BallKey q4. Sous un mutant
`q4-cover-coef3`, elle doit en observer exactement une brute et post-RLE, puis
zéro après préfiltre, avec code 4. Ajouter ensuite l'équivalence des covers
handles/direct-query aux coefficients 3 et 4, la route query contre handles,
et la permutation à PointId conservés. L'oracle post-préfiltre reste une
porte d'objet distincte.

## P0 — séparer maintenant les deux monnaies de digest

La « bascule le jour où » proposée par Claude n'est pas recevable : une même
monnaie/version ne peut pas changer de sémantique selon le premier échec d'une
optimisation. Les frontières diffèrent déjà dans
`receipts/conformite_v5/uniform_400.txt` : `boules_uniques=105076`,
`mortes_profondeur=1134`, `survivantes=103942`.

Geler dès ce jalon trois contrats :

- `digest_candidates_v5_compat`, tag v4, signe les candidats post-RLE et reste
  un diagnostic différentiel tant que l'égalité est attendue ;
- `digest_postprefilter_v6`, tag neuf, signe exactement les records
  `cands[s.idx]` survivants dans l'ordre canonique et sert de non-régression
  interne v6 ;
- la conformité d'objet v5↔v6 porte sur `digest_all` et chaque digest forestier.

La fixture `uniform_400` doit imposer `dead_depth>0`, les deux cardinalités
distinctes, le digest compat égal à la v5 et le digest v6 calculé sur exactement
103942 records. Aucun renommage conditionnel futur.

Le suffixe `v5_compat` qualifie la **sérialisation**, pas une égalité éternelle
des candidats. Un rejeu v5 avec `q4-cover-coef4` sur `uniform, n=8000` fournit
déjà le différentiel causal attendu : 3 134 427 candidats uniques deviennent
3 134 404, tandis que les 3 103 251 survivantes et `digest_all` restent
inchangés. La porte de conformité objet doit donc juger uniquement les forêts
et `digest_all`; la monnaie candidat rapporte séparément cette divergence de
construction au lieu de la transformer en faux échec produit.

Implémentation minimale : calculer le digest post-préfiltre pendant que
`surv` existe, sur la sous-suite canonique `cands[s.idx]`, sans inclure la
profondeur et sans copier les candidats. Avec le tag proposé
`mhgp6-digest-v1:postprefilter-candidates`, un calcul indépendant sur la coupe
donne pour `uniform_400` le golden
`97be65b6e0c66e3b3b2262510bd7274f8e557ae4bdb78024467c1f9ee05c4d72`.
Le graver seulement après le correctif cover final.

Le nom « multiensemble émis » doit aussi être corrigé : `digest_balls_v4`
signe actuellement les candidats **uniques post-RLE**, pas le multiensemble
brut émis. `run.hpp` libère ensuite `surv` et digère encore `cands`; il ne
peut donc pas, dans l'état courant, signer la frontière post-préfiltre annoncée.

## P1 — remettre le plan de tests à la vérité

La coupe auditée contient :

- 59 noms dans `kMutants`;
- 62 sites `MHGP6_MUTANT` couvrant ces 59 noms, trois noms ayant deux sites;
- seulement 4 noms injectés par une porte CTest :
  `fused-mask-stuck`, `sweep-nonstrict-depth`,
  `sweep-drop-exit-root` et `rle-drop`.

`PLAN_DE_TESTS.md` affirme pourtant « chaque nom = un point d'injection + une
porte code 4 » et cite `tests/mutants_gate.py`, qui n'existe pas. Il cite
aussi de nombreuses portes absentes : oracle OBig, comparateurs de niveaux,
fixtures de facteurs/crédits, `linked_arcs_u16`, juge de fold, parallélisme,
mutants historiques, entre autres.

Deux sorties propres :

1. **checkpoint borné** : présenter les 23 CTests comme une suite initiale,
   marquer le reste `[PRÉVU]` dans le plan et retirer « liste tenue à jour
   avec le code »;
2. **J2 complet** : porter les portes v5 correspondantes, ajouter les portes
   neuves du sweep, puis créer un contrôle automatique
   `registre == injections == mutants attendus à code 4`.

La simple égalité des trois ensembles de noms ne suffit pas : chaque mutant
doit encore être exécuté et effectivement tué. La fixture familles actuelle
ne grave aucun digest et n'exerce pas `family-scanline-overshoot`; elle teste
seulement déterminisme, profil, unicité et cardinalité. L'oracle OBig est
présent mais non appelé.

Le port peut commencer sans nouvelle source produit. Un rejeu direct de
`mhgp6_conformity` sur `eight_clusters, n=400` rend déjà le code 4 pour 20
mutants non raccordés :

```text
census-nonstrict genfilter-nonstrict depth-threshold-minus-one
range-add-max-le-zero skip-full-census binary-ties repr-ties attach-prebatch
drop-nonmerge dense-pointid canonical-is-uf-root q3-level-4g
q4-center-parity q4-eq-sign q4-i64-drop-factor q4-i64-pair-min
witness-no-lane-mask par-drop-shard par-drop-ball-chunk
fold-inject-a-failure-k2
```

Une boucle CMake ferait donc passer immédiatement la couverture exécutée à
24/59. Ne pas y inclure `fold-inject-b-exception-k3`, qui termine actuellement
par signal (`134`) : porter pour lui le juge d'in-flight dédié. Ensuite, 25
sources de portes v5 passent déjà la syntaxe après les seuls renommages de
namespace, macros et includes et offrent des juges connus pour 47 des 55 noms
non couverts. Ordre de rendement conseillé : `wspd_gate`, `q3_oracle`,
`cell_grid_oracle`, `prefix_gate`, puis spindle, fold in-flight et render.
Le contrôle textuel `mutants_gate.py` v5 vérifie la topologie; il ne remplace
jamais ces kills exécutés.

## P1 — finir la cohérence documentaire

Les corrections annoncées n'ont pas encore atteint toutes les autorités :

- `README.md` dit encore « base de code neuve » et affirme que
  `digest_balls` est déjà post-préfiltre;
- la note fondatrice affirme encore que C×D disparaît et que la frontière de
  digest a déjà changé; si elle reste historique, ajouter en tête un bandeau
  explicite de supersession vers cet état courant;
- `ARCHITECTURE.md` dit encore à E4 que « la boucle C×D n'existe pas » et
  emploie « profondeur + crédits » malgré le contrat full-cover choisi;
- E3 est marqué `[LIVRÉ]` et décrit `src/credit/`, `AnchorCredit`,
  `CoreCredit` et `ResidualTape`, alors que `src/credit/` est vide et
  le code emploie `EndpointCredit`;
- `PISTES_FERMEES.md` dit encore que la frontière post-préfiltre rend le
  diagnostic candidat caduc;
- `PLAN_DE_TESTS.md` décrit les portes prévues comme si elles existaient.

Scinder E3 est plus clair que le renommer globalement :
`[LIVRÉ] EndpointCredit + tueurs portés`, puis
`[PRÉVU] AnchorCredit/CoreCredit/ResidualTape`. Dans le README, remplacer
« base neuve » par « socle v5 porté et requalifié, génération q3/q4 neuve ».
L'objet doit rester qualifié comme payload différentiel
`verified_events_only`, jamais comme exactitude intégrale du manuscrit.

Deux claims v5 restent aussi à retirer : `REGIMES.md` et
`GRAND_LIVRE.md` disent encore que la super-quadraticité est « imputable à
la famille » et que q4 devient linéaire avec des exposants
`[1,003 ; 1,014]`. Les mesures v5 étaient non appariées, à une répétition;
cet intervalle décrivait les covers de deux terrains bornés, pas un théorème
causal sur le coût q4. Écrire « association diagnostique sur ces runs ».

## P1 — borner le reçu au matériau réellement capturé

Le reçu `receipts/conformite_v6_j2_20260831/` existe désormais. Les trois
SHA-256 de `BINAIRES.txt` correspondent exactement aux binaires du build courant
depuis la coupe, et CMake enregistre bien 38 tests : 23 rapides et 15 scale.
Les 15 références scale contiennent chacune le digest candidat, les dix
digests forestiers et `digest_all`; le comparateur les juge effectivement.

En revanche, `ctest_gate_final.txt` et `ctest_scale_final.txt`, qualifiés de
« sorties brutes », ne sont que des résumés de 4 et 10 lignes. Ils ne portent
ni commande, ni noms/statuts individuels, ni stdout avec les digests. Le reçu
dit seulement « le commit qui porte ce reçu » et ne capture pas l'état du
worktree au moment du run. Le lien au commit `b17ca2cd` est donc fortement
corroboré par les hashes et les mtimes, mais reconstruit après coup. Pour la
prochaine campagne, ajouter un `META` avec SHA littéral, commande, build type,
toolchain, état du worktree, heures, hashes des logs et codes; sinon renommer
ces fichiers `summary` et borner explicitement la preuve.

Les champs enrichis de `receipts/conformite_v5/META.txt` ne figuraient pas
dans la capture initiale. Les qualifier `reconstruit_a_posteriori` ou
`not_recorded`. De plus, ce META affirme que `STATUS.txt` contient les codes
des 15 grands et 8 petits runs, alors que `STATUS.txt` ne contient que les 15
grands. La baseline reste utilisable pour ses éléments réellement épinglés :
pin, hash du binaire, sorties présentes et codes effectivement enregistrés.

## P1 — fermer le grand-livre avant les pentes

`sweep_root_groups` et `P_role=q4_completions` sont de bons ajouts. Il
manque toutefois le nombre de comparaisons du tri des racines : c'est un terme
payé réel que `sweep_roots_onchord` seul ne mesure pas. Ajouter un compteur
`sweep_root_comparisons` dans le comparateur.

Plus largement, `GRAND_LIVRE.md` annonce comme publiés `W_sweep1`,
`H_rect`, `H_scan`, `M_anchor`, `V_R/C_R/P_R` et d'autres termes qui
n'ont pas encore de compteurs dans `GenerateStats` ni dans la sortie. Marquer
le tableau `candidat J3` ou les instrumenter avant toute pente. La formule
« préparation quasi linéaire » reste une cible conditionnelle tant que
`R`, `V_wspd`, les handles et les incidences ne sont pas bornés.

La route S possède en outre un terme quadratique non publié :
`corner_histograms` paie `|A|^2+|B|^2` pour chaque lane avant tout raccourci,
alors qu'`ARCHITECTURE.md` annonce un raccourci absent. Une grille
`[0,side)^3` face au singleton `(65535,65535,65535)` laisse un terminal
`m x 1` vivant pour `m=64,512,1728` et paie `m(m-1)` évaluations sur ce seul
rectangle. Ajouter `P_factor`, graver ce carré, ou livrer le raccourci/route M
avant toute qualification quasi linéaire.

Le chiffre isolé `terrain_stationnaire n=2000` de la réponse est un
diagnostic utile, pas un reçu ni une pente — qualification correctement
annoncée par Claude.

Enfin, la campagne J3 doit séparer le coût de fabrication des entrées. Les
familles stationnaires créent un nombre de motifs proportionnel à `n`, puis
évaluent chaque point contre tous ces motifs (`terrain_stationnaire_cloud` et
`ScanlineField::height`) : la génération est elle-même quadratique. Ajouter
`T_input` et un compteur `V_motif`, ou préparer/rastériser spatialement le
champ, avant d'attribuer une pente de bout en bout au pipeline MorseHGP3D.

## J3 actif — critères de clôture, sans verdict sur le worktree

Le travail J3 non versionné progresse utilement. Les 18 runs des deux familles
stationnaires couvrent déjà les tailles 8000/16000/32000 et les graines 3/4/5,
avec code 0, sorties structurées et stderr vide. Les cinq nouvelles portes
ciblées `linked_arcs`/sweep compilent et passent localement, dont trois mutants
attendus à code 4. Ce sont des diagnostics sur un worktree actif, pas encore
un reçu.

Avant de fermer la campagne :

- `pentes.py` doit exiger exactement 9/9 runs par famille, `DONE`, code 0,
  stderr vide, identité interne concordante et présence de chaque compteur
  requis. Le seuil courant `len(data) >= 6` publie à tort une table partielle;
- une regex ou un terme absent doit faire échouer la porte, pas disparaître
  silencieusement. Publier l'étendue inter-graines séparément pour chacun des
  deux pas, au lieu de mélanger les six pentes;
- les pentes observées sous 2 sur les compteurs actuellement extraits sont un
  signal positif, mais pas un GO : `P_factor`, `T_input/V_motif`, `H_rect`,
  `H_scan`, `M_anchor`, `W_sweep1`, `V_census` et les HWM manquent encore;
- `linked_arcs_u16` prouve l'inclusion exact-once des clés attendues, mais ne
  rejette pas encore les clés q3/q4 profondeur-zéro excédentaires. Comparer
  l'égalité complète produit/oracle; sous réétiquetage, comparer aussi
  multiplicités et arités, pas seulement le set de `BallKey`;
- cette porte s'arrête au census : elle établit une barrière quadratique de
  génération/census, pas encore une « barrière de sortie » ni `S_forest`.
  Borner le vocabulaire ou prolonger jusqu'à l'expansion/fold et graver les
  planchers de facettes annoncés;
- les fixtures sweep F1–F5 jugent encore après préfiltre. Elles ne contiennent
  ni la fixture coefficient 3/4 `(110,90,83)` de ce rapport, ni la
  calotte–lentille de coût avec `M_anchor/H_scan`. Ces trois portes sont
  complémentaires; aucune ne remplace les deux autres. Leur oracle réutilise
  en outre les formes q3/q4 produit : il est indépendant du contrôle du sweep,
  pas un oracle arithmétique entièrement indépendant. Sur F1/F4, relier les
  compteurs globaux à l'ancre visée ou ajouter un mutant ciblé avant de dire
  que ce chemin précis est causalement exercé.

La campagne active est épinglée sur `b17ca2cd`, donc avant la correction du
cover q4 et la séparation des digests. La conserver comme baseline
exploratoire, mais rejouer le reçu final après ces deux changements : le
premier modifie précisément la charge mesurée et le second le contrat de
sortie. Des pentes pré-correction ne peuvent pas recevoir le code corrigé.

Le prochain reçu doit enfin épingler le SHA complet, le runner ou sa commande,
l'état du worktree, la toolchain, les heures et les hashes des sorties. Aucun
claim de pente, de préparation quasi linéaire ou de J3 complet avant ce pin.

## P1 — la racine carrée reste flottante

`detail_round_isqrt_clamped` initialise encore `r` avec
`std::sqrt((double)m)`. Les boucles de correction rendent le résultat exact
sur le domaine courant, mais les commentaires « plus aucun libm » et « racine
entière » sont littéralement faux. Utiliser directement la primitive entière
`floor_sqrt` déjà disponible, puis appliquer la règle
`m > r*(r+1)`. Ajouter des fixtures autour des deux côtés de la frontière
d'arrondi et du clamp.

## Décisions V6-Q1 à V6-Q4

- Q1 : digest post-préfiltre accepté comme non-régression interne v6 sous un
  tag neuf, à activer dès maintenant à côté du diagnostic v5-compatible ; les
  deux frontières sont déjà distinctes sur `uniform_400`.
- Q2 : contrat écrit, puis oracle exécutable indépendant, puis raccord.
  Signes de dénominateur, `B=0`, racines égales, extrémités de Jung,
  incidents, ownership, exact-once, permutation et réétiquetage restent à
  graver en J3.
- Q3 : les familles stationnaires répondent au défaut de dilatation, avec une
  qualification synthétique et les statistiques de génération promises.
- Q4 : contre-fixture calotte–lentille finie en u16 acceptée, avec littéraux,
  marges OBig, rôles exact-once, objets attendus, permutation et mutant i64.

## Rejeu indépendant sur `b17ca2cd`

```text
cmake -S morsehgp3D_v6 -B build/v6 -DCMAKE_BUILD_TYPE=Release
  -> code 0

cmake --build build/v6 --parallel
  -> code 0

ctest --test-dir build/v6 --output-on-failure -LE '^scale'
  -> 23/23 passes
  -> 50,81 s réelles
  -> mhgp6_fused_mutant_mask rend bien le code attendu 4

sha256sum build/v6/{mhgp6,mhgp6_conformity,mhgp6_selftest}
  -> correspond exactement à BINAIRES.txt

python tools/check_docs.py
  -> 236 Markdown actifs validés dans le worktree courant

python tools/check_implementation_status.py
  -> 20 phases du registre validées; ce succès ne promeut pas la v6 hors registre

git diff --check
  -> code 0
```

Une compilation GCC 13.3 Debug avec ASan/UBSan dans `/tmp`, fuites seules
désactivées car LeakSanitizer est incompatible avec le traçage du conteneur, a
terminé 20 portes sans diagnostic : 1–8, 10–15 et 18–23. Les portes 9, 16 et
17 ont été interrompues pour coût instrumenté et n'ont aucun verdict
sanitizer. Cette campagne partielle est un diagnostic mémoire, pas une porte.

Les 15 tests `scale*` n'ont pas été rejoués indépendamment sur cette coupe :
leur résumé annonce 15/15 en 1398,91 s et l'inventaire CMake/références est
cohérent, sous les réserves probatoires du reçu ci-dessus. Aucun résultat GPU
n'est revendiqué. GCP non utilisé.
