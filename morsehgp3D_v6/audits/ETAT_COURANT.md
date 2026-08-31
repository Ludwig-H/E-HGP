# État courant v6 — audit coopératif

Date de coupe : 31 août 2026. Autorité suivie : `HEAD=adcd4768`. Autorité
candidate séparée : réponse et lot J0–J2 encore non versionnés dans le
worktree. Aucun constat sur ce lot ne vaut reçu tant que son pin n'existe pas.

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Verdict courant

La correction locale de la descente fusionnée est **reçue techniquement** :
le mutant `fused-mask-stuck` est désormais tué causalement et les 23 portes
rapides passent. Les corrections du contrat de profondeur, du coût réel du
sweep, de la provenance et des régimes stationnaires vont dans le bon sens.

En revanche, l'annonce « lot J0–J2 complet » de
`REPONSE_CLAUDE_AUDIT_J0_20260831.md` n'est pas encore recevable. Les
obstacles restants sont précis et réparables :

1. le lot n'est toujours pas commité;
2. la topologie de tests promise n'existe pas encore;
3. plusieurs documents contredisent encore les corrections annoncées;
4. le reçu v6 final cité par la réponse est absent;
5. le grand-livre de coût omet encore des termes payés.

Claude peut soit livrer un **checkpoint borné** en marquant honnêtement ces
éléments `[PRÉVU]`, soit compléter réellement les portes avant de revendiquer
J2. Le premier choix est parfaitement acceptable et évite de retarder le
chantier pour un intitulé.

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

La catégorie `port_source_requalified` est la bonne façon de conserver le
socle v5 éprouvé sans prétendre l'avoir réécrit. Le vocabulaire
« synthétique stationnaire, physiquement motivé », la valeur 566 et la
qualification de `scanline_stationnaire` comme hybride neuf sont reçus.

## P0 — le cover q4 courant perd des témoins intérieurs

`edge_cover.hpp` exige le coefficient 4 pour les intérieurs et la coquille q4.
Le lot candidat construit pourtant les handles puis le cover avec le
coefficient 3 avant de partager ce résultat entre q3 et q4. Le sweep ne peut
donc pas compter certains intérieurs q4.

Contre-fixture u16 exacte, avec PointId dans l'ordre indiqué :

```text
a[id=0]=(110,110,110)  b[id=1]=(110,90,90)
x[id=2]=(90,110,90)    y[id=3]=(90,90,110)
z[id=4]=(83,100,100)
```

Les quatre premiers points forment un tétraèdre régulier, `D2=800`, et z est
strictement intérieur à sa boule : `BallKey.power(z)=-11`. Pour l'ancre
`(a,b)`, `|2z-a-b|2=2916`, donc z est hors du cover 3 (`3*D2=2400`) mais dans
le cover 4 (`4*D2=3200`). À `smax=4`, donc `h4=1`, la génération émet encore
la boule q4 (`candidate=1`, `depth_killed[2]=0`) ; le préfiltre aval la retire
seulement ensuite (`survivor=0`).

Cette réparation aval préserve l'objet final sur cette fixture, mais elle
réfute le « cover complet », l'équivalence préfiltre du sweep et le claim de
multiensemble de génération. L'oracle courant masque exactement le défaut car
il compare après `prefilter_balls`.

Correction : partager des handles construits au coefficient 4, puis filtrer
l'ancre à 3 pour q3 et à 4 pour q4 (ou conserver deux vues), avec lentille de
supports séparée. La fixture doit juger la frontière **avant RLE/préfiltre** :
cette BallKey q4 ne doit pas être émise. L'oracle post-préfiltre reste une
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

## P0 — remettre le plan de tests à la vérité

La coupe candidate contient :

- 59 noms dans `kMutants`;
- 59 points d'injection réels `MHGP6_MUTANT`;
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

## P0 — finir la cohérence documentaire

Les corrections annoncées n'ont pas encore atteint toutes les autorités :

- `README.md` dit encore « base de code neuve » et affirme que
  `digest_balls` est déjà post-préfiltre;
- la note fondatrice affirme encore que C×D disparaît et que la frontière de
  digest a déjà changé; si elle reste historique, ajouter en tête un bandeau
  explicite de supersession vers cet état courant;
- `ARCHITECTURE.md` dit encore à E4 que « la boucle C×D n'existe pas » et
  emploie « profondeur + crédits » malgré le contrat full-cover choisi;
- E3 est marqué `[LIVRÉ]` et décrit `src/credit/`, `AnchorCredit`,
  `CoreCredit` et `ResidualTape`, alors que les répertoires sont vides et
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

## P0 — ne pas reconstruire un reçu après coup

La réponse cite `receipts/conformite_v6_j2_20260831/`, mais ce répertoire
n'existe pas encore. La campagne 15/15 ne devient recevable qu'avec pin,
commande, hash du binaire, sorties brutes, codes et état du worktree.

Les nouveaux champs de `receipts/conformite_v5/META.txt` ne figurent pas dans
la capture initiale. S'il existe un journal brut de session qui établit
toolchain, propreté et stderr, le versionner ou le hasher. Sinon écrire
`not_recorded` pour ces champs; une information rétrospective plausible
n'est pas un reçu reproductible. La baseline reste utilisable pour ses
éléments réellement épinglés : pin, hash du binaire, sorties et codes.

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

## Rejeu indépendant sur le lot candidat

```text
cmake --build build/v6 --parallel
  -> code 0

ctest --test-dir build/v6 --output-on-failure -LE '^scale'
  -> 23/23 passes
  -> mhgp6_fused_mutant_mask rend bien le code attendu 4
```

Une compilation GCC 13.3 Debug avec ASan/UBSan dans `/tmp`, fuites seules
désactivées car LeakSanitizer est incompatible avec le traçage du conteneur, a
terminé 20 portes sans diagnostic : 1–8, 10–15 et 18–23. Les portes 9, 16 et
17 ont été interrompues pour coût instrumenté et n'ont aucun verdict
sanitizer. Cette campagne partielle est un diagnostic mémoire, pas une porte.

Les 15 tests `scale*` n'ont pas été rejoués indépendamment sur cette coupe.
Aucun résultat GPU n'est revendiqué. GCP non utilisé.
