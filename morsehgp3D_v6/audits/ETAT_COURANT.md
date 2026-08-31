# État courant v6 — audit coopératif

Date de coupe : 31 août 2026. Autorité auditée :
`518e270683129a4badea9df31400a97582528401`, présente sur `main` et
`origin/main`. La campagne non suivie
`receipts/campagne_decision_20260831/` tourne sur ce pin ; elle reste hors
verdict jusqu'à son `DONE`, ses hashes et ses contrôles terminaux.

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
et conformité v5↔v6 jugée sur les forêts et `digest_all`. Les correctifs
ultérieurs ne le rouvrent pas.

Le corpus `a1bfba3b` est **reçu comme baseline enrichie des champs présents**.
Sa matrice, ses sorties, ses hashes et les invariants contrôlés ferment. Les
formulations « grand-livre complet », validateur « fail-closed »,
`W_sweep2` « d'un ordre de grandeur sous » et « promesse de coût tenue » sont
refusées ; ce reçu n'est ni une décision J3 ni un GO.

`518e2706` est un progrès réel, mais n'exécute pas encore les « cinq P1 »
annoncés. Sont reçus : la racine entière pure, les itérations physiques q4,
la garde de capacité des indices du préfiltre, le buffering du validateur,
les bijections STATUS/`.txt`/`.err`, plusieurs rejets CTest, l'enrichissement
du golden et les corrections ciblées de documentation. Restent quatre
réserves prioritaires : causalité du mutant WSPD, ensemble exact des forêts
attendues, définition des nouvelles monnaies et preuves des contrats d'échec.

Ordre conseillé : corriger d'abord le mutant et le juge exact ; aligner
ensuite chaque compteur sur une définition unique et graver ses identités ;
fermer enfin les fixtures du validateur et des retours d'échec. Conserver la
campagne en cours comme sonde enrichie si elle termine proprement, sans lui
donner rétroactivement un statut décisionnel.

## Ce qui est reçu dans `518e2706`

- `isqrt64_pure` est une racine entière bit à bit sans libm. La frontière des
  familles stationnaires ne dépend donc plus d'une initialisation flottante.
- `candidates_capacity_ok` expose le plafond u32 de `Survivor::idx` et
  `run_pipeline` rend `resource_exhausted` avant `prefilter_balls`.
- `invalidate_provisional` ferme par construction les deux fuites observées
  sur les chemins census et défaut de fold : digests, `cards`, digests de
  forêt et totaux sont vidés.
- `pentes.py` compare maintenant les ensembles attendus de tuples STATUS et
  de fichiers `.txt/.err`, recoupe `family/n/seed/s/smax/threads`, exige les
  compteurs déclarés dont `P_factor_q2`, valide tout avant d'imprimer et
  bufferise les tables.
- `q4_core_iters` et `q4_pass2_iters` distinguent enfin les itérations de
  boucle complètes des évaluations non incidentes `W1/W2`. Les vecteurs par
  octave ferment sur `seeds_q4` et `W1` dans les sorties déjà observées.
- `H_rect` représente correctement la masse des handles une fois par
  rectangle et lane q3/q4. Les autres nouvelles monnaies demandent encore les
  corrections ci-dessous.
- `PROVENANCE.md` gèle les monnaies de digest ; `REGIMES.md` requalifie
  `linked_arcs_u16` en barrière bornée de génération/census ; deux tests ont
  désormais le label `oracle`.
- CMake enregistre 74 tests : 59 hors `scale` et 15 `scale`. Claude rapporte
  59/59 rapides, sans reçu brut versionné dans ce commit.

## Reçu grand-livre `a1bfba3b`

La matrice attendue contient exactement quatre familles, trois tailles et
trois graines : 36 tuples uniques, 36 codes 0, un `DONE` terminal, 36 stdout
et 36 stderr vides, sans `.txt/.err` supplémentaire. Les 36 hashes de stdout
ont été revérifiés. Le hash du binaire a été constaté avant un rebuild
ultérieur ; le reçu n'archive ni ce binaire ni le log d'un rebuild post-pin.
Chaque sortie contient dix lignes K, treize digests et le schéma nominal
attendu. Digests et cardinalités concordent avec la baseline antérieure sur
les mêmes tuples. `PENTES.txt` est la sortie exacte du script de son pin.

Réserves de lecture maintenues :

- uniform couvre au moins `[1,01 ; 1,20]`, pas `[1,03 ; 1,20]` ;
- `W2/W1` varie de 0,198 à 0,810. Le sous-compteur `W2` est donc 1,23× à
  5,04× plus petit sur ce corpus, jamais d'un ordre de grandeur ;
- `W1/W2` y comptent les évaluations non incidentes, pas les itérations
  complètes ;
- `P_factor_q2`, `H_scan`, le vrai `V_census` et plusieurs autres termes
  payés n'y sont pas analysés.

## P1 — le mutant WSPD ne perd toujours pas exactement un rectangle

Dans `alive_rectangles_fused`, `drop_flag` est construit **dans** la boucle
des vagues. Le mutant peut donc perdre jusqu'à un rectangle terminal par
vague, pas un rectangle sur toute la descente. En outre,
`ledger_emitted_mass` et `rect_alive` sont crédités avant le saut de sortie :
le grand-livre reste fermé et ne détecte pas la masse réellement absente.

La nouvelle porte tue bien le mutant, mais par divergence entre les bras
fusionné et singleton ; elle n'asserte ni `count_mutant = count_nominal - 1`
ni la cause « grand-livre ». Deux corrections recevables :

1. appliquer le drop test-only une seule fois, après la fusion ordonnée de la
   sortie, et graver littéralement le delta `-1` ; ou
2. maintenir un ledger indépendant reconstruit depuis les rectangles
   réellement remis, afin que l'omission soit aussi visible par l'invariant.

Aligner ensuite cette sémantique avec le mutant homonyme de
`wspd/wavefront.hpp`. La porte de conformité générale reste un complément,
pas la preuve de causalité.

## P1 — le juge n'exige pas encore l'ensemble exact des K

Le nouveau contrôle exige chaque K de `1` à `kmax_eff`, mais il ne refuse pas
les clés supplémentaires entre `kmax_eff + 1` et 10. Le commentaire « les K
au-delà de `kmax_eff` sont refusés » est faux : le chargeur ne connaît que le
domaine global `[1,10]`. Pour `n=2`, une référence contenant K1 correct **et**
K10 supplémentaire passe encore cette étape, puis K10 est ignoré.

Exiger l'égalité de l'ensemble des clés à `{1,...,kmax_eff}` après le run et
graver deux rejets distincts : K1 absent, puis K1 correct + K10 en trop à
`n=2`. La fixture actuelle, K10 seul à `n=400`, ne couvre que le premier cas.

## P1 — les nouvelles monnaies ne forment pas encore le grand-livre annoncé

Les sorties supplémentaires sont utiles, mais quatre noms ne correspondent
pas encore au contrat de `docs/GRAND_LIVRE.md` :

- `V_census` imprime `ExpandStats::depth`, alimenté uniquement par
  `ball_depth_at_least` pendant le **préfiltre count-only**. `ball_census`
  n'instrumente aucune visite : ce compteur n'est pas « préfiltre + census » ;
- `V_wspd` publie les nœuds visités et les évaluations de couples de coins.
  Le second champ n'est pas le nombre d'« appels témoins » demandé par la
  définition documentaire ;
- `M_anchor` n'a pas une population commune : q3 ajoute la taille du cover
  après W3/secteurs/grille, q4 l'ajoute avant W4/secteurs/grille. Il ne peut
  donc pas être comparé entre lanes ni appelé uniformément somme sur les
  ancres survivantes ;
- `H_scan` reste absent, bien que `AnchorScratch::visits` fournisse déjà une
  base d'instrumentation.

Le parser ne lit qu'une composante de `V_census`, ignore entièrement la ligne
d'octaves, et ne couvre toujours pas les comparaisons de regroupement, les
kills ventilés, `T_input` ou le HWM par rôle. `GRAND_LIVRE.md` marque encore
des compteurs désormais présents comme « candidat J3 » et n'a toujours pas
de ligne définissant `W_sweep2`.

Avant un nouveau run décisionnel, définir pour chaque monnaie la population,
le point d'incrément et une identité fermante. Pour la sonde q4, publier au
minimum par octave les seeds tuées cœur, tuées corde et survivantes ; déclarer
que le vecteur `ancres` actuel compte les entrées de `process_anchor_q4` après
le prétest par requête. Ajouter les sommes de vecteurs au validateur.

## P1/P2 — le validateur progresse, sa porte contient encore un faux test

La porte Python exerce utilement douze rejets, mais son cas « zéro légitime »
remplace `seeds_cellules=0/0`, déjà nul dans la fixture et **absent de la liste
des compteurs parsés**. Elle ne vérifie pas non plus que `-` apparaît : le test
est vacue. Mettre à zéro un compteur réellement parsé sur les trois tailles,
puis exiger le code 0 et la pente indéfinie imprimée.

Pour borner honnêtement « fail-closed », ajouter ensuite : famille META
dupliquée, entier META invalide sans traceback, identité/compteur/digest
dupliqué, digest hex invalide et fichier d'extension inattendue. Le script ne
vérifie actuellement ni les hashes du META ni les invariants numériques ; une
porte de **campagne décisionnelle** doit aussi imposer explicitement les
quatre familles, indépendamment du parser générique de pentes.

Le plan de tests annonce 28 noms de mutants exercés, mais les CTests n'en
référencent que 27 distincts : la nouvelle porte `wspd-drop-rect` double un
nom déjà présent dans la boucle de conformité.

## P2 — rendre le contrat d'échec permanent

Les anciens cas census et fold prennent maintenant la routine commune, mais
aucune porte n'inspecte les champs rendus. Le mutant K2 vérifie seulement un
statut divergent. Ajouter une fixture bibliothèque pour :

- census en échec avec digest raw demandé ;
- défaut K2 avec `fold_inflight={1,2,8}` ;
- tous les digests, `cards`, digests de forêt, totaux et politique explicite
  de `expand.events_by_k` ;
- sûreté des callbacks provisoires et terminaison de tous les ouvriers.

La réponse intégrée disait que `invalidate_provisional` est appelé sur chaque
retour après génération. Ce n'est pas littéral pour les retours grand-livre
et `invariant_jneg`, même si leurs champs concernés sont encore vides par
défaut. Appeler un finaliseur unique sur toute sortie non complète évitera que
ce fait accidentel devienne une nouvelle fuite.

Le plafond des candidats ferme un narrowing précis. Les conversions
`CloudIndex` vers `int`/i32/u32 et les additions u64 des nouveaux compteurs
restent ouvertes, comme Claude le reconnaît : déclarer leurs domaines et
tester leurs frontières sans allocation géante.

## Signal E6 et campagne active

Sur `a1bfba3b`, le pic vient des seeds éliminées en passe 1, pas de `W2` : à
la graine 5, la pente de `W1-W2` vaut 2,430 sur terrain et 3,010 sur scanline,
alors que `W2` vaut 1,320 et 1,156. Les anciennes monnaies donnent pour la
pente physique de passe 1 au pas 16000→32000 une borne
`[1,95285 ; 2,13987]` sur terrain et `[2,28553 ; 2,44951]` sur scanline. Le
second intervalle reste entièrement au-dessus de 2.

Ce signal justifie une sonde et bloque prudemment un GO. Il ne suffit pas à
une activation formelle : les dépassements viennent de la seule graine 5,
`REGIMES.md` déclare sans valeur toute conclusion mono-graine et aucun
agrégateur inter-graines n'a été préenregistré.

La campagne active porte déjà dans son META « grand-livre complet » et
« décision E6 ». Ces qualifications sont refusées avant même son résultat :
les monnaies ci-dessus et l'agrégateur restent ouverts. Ne pas jeter la
dépense CPU : si `DONE`, hashes, schéma et invariants ferment, conserver les
sorties comme **baseline de sonde enrichie au pin `518e2706`**, puis analyser
les octaves séparément. Aucun seuil nouveau ne doit être choisi après lecture
pour transformer cette capture en décision.

## Rejeux et statut

```text
381ba60b : configure/build Release -> code 0
381ba60b : portes rapides indépendantes -> 51/51
d153e1be : portes ciblées légères indépendantes -> 9/9
d153e1be : reçu Claude des portes scale -> 15/15, 903,41 s
a1bfba3b : campagne grand-livre -> 36/36, hashes de sorties et invariants vérifiés
518e2706 : registre CTest -> 74 tests, dont 59 hors scale et 15 scale
518e2706 : portes rapides rapportées par Claude -> 59/59, sans reçu brut
```

Le build et les portes de `518e2706` ne sont pas rejoués pendant la campagne
CPU concurrente afin de ne pas perturber sa capture. Aucun résultat GPU n'est
revendiqué. GCP non utilisé.
