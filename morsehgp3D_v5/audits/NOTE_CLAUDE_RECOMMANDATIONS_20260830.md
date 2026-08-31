# Recommandations Claude au 30 août 2026 — ordre de travail et ce qui le justifie

Note Claude, 30 août 2026, ancrée au pin `9940668`. Cadre :
`phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Cette note **ne promeut aucun statut** et ne remplace pas `ETAT_COURANT.md`, qui
reste le verdict mutable. Elle ordonne les chantiers ouverts et dit, pour chacun,
ce qui est mesuré, ce qui est supposé, et quel serait le premier pas falsifiable.
Là où elle recoupe l'ordre de travail de l'auditeur, elle le suit.

**Contre-audit du 30 août :** l'ordre général reste utile, mais deux conclusions
sont corrigées ci-dessous. La sonde WSPD ne compare pas encore au symbole
produit et ne mesure pas un HWM ; la « correction » de `125,26 deg` confondait
deux angles valides. `ETAT_COURANT.md` porte le verdict à jour.

**Mise à jour `eba24b9a` :** le header, le parsing exact de `--s` et le plancher
des deux CLI sont maintenant commités ; le premier chantier est fermé pour la
voie CPU. La portée documentaire doit encore rester « `--s` exact », car les
autres options emploient toujours `atoi`/`atoll`. Le profiler q4 commité avec ce
correctif reste soumis aux gardes d'`ETAT_COURANT.md`.

## 0. Ce que j'ai vérifié moi-même sur ce pin

> [!NOTE]
> **Les trois premiers constats sont clos par `eba24b9a`**, poussé après la
> rédaction de cette note. Ils sont conservés pour expliquer la priorité
> initiale ; le quatrième reste un avertissement sur le régime de coût.

Quatre constats de premier main, reproductibles, qui cadrent les priorités.

- **Le build propre est cassé.** `tests/fold_bench.cpp` inclut
  `../src/core/parse.hpp`, absent du dépôt. `cmake --build` échoue donc sur la
  cible `mhgp5_fold_bench`, et la recette du `README.md` ne passe pas. Nuance
  factuelle par rapport à `ETAT_COURANT.md` § P1 : sur ce pin, **un seul**
  fichier l'inclut, pas trois.
- **Six portes échouaient.** `mhgp5_cli_refus_s_suffix` et
  `mhgp5_cli_refus_s_overflow` (code 0, attendu 2), plus les quatre portes
  `mhgp5_fold_bench_*` qui ne pouvaient pas se construire. Dénominateurs : 291
  CTests enregistrés et 274 sous le label `gate` au pin `119b80b0` ; 297 et 280
  après l'ajout des six portes de fusion. Le vert des autres portes ne prouve
  pas à lui seul « le cœur mathématique » ni le contrat Gamma.
- **La cause est localisée.** La garde du CLI est `opt.s < 1`
  (`cli/mhgp5.cpp`) : elle ne connaît pas le plancher 8. C'est `run_pipeline` qui
  refuse 7, d'où `--s=7` correct. Mais `atoll("8junk")` rend 8 et
  `atoll("9223372036854775808")` déborde vers `INT64_MAX` : les deux franchissent
  la garde bibliothèque.
- **Le débordement sélectionne le pire régime, en silence.** Mesuré à
  `n = 600` : à `--s=2^63`, `ancres == rect_alive` à l'unité près sur les trois
  lanes (17479/42294/45913), c'est-à-dire **un rectangle par paire**, et
  `ancres_w3`, `ancres_w4` et `ancres_hist` tombent **tous à zéro**. Tout
  l'élagage au niveau du bloc est désactivé. Une faute de frappe suffit.

**Fermeture vérifiée à `eba24b9a`.** `src/core/parse.hpp` est suivi ; une archive
Git propre construit toutes les cibles CPU sous `-Werror` ; les portes de
suffixe, débordement et frontière 7/8 passent. `--s=8junk`,
`--s=9223372036854775808` et `--s=7` rendent le code 2, `--s=8` rend 0. Le
régime dégénéré n'est donc plus atteignable par une faute lexicale sur `--s`,
mais `INT64_MAX` reste une entrée volontaire sans claim de coût.

Un cinquième constat, hors code : voir la section 5, qui décrit l'état des
lignées Git et une divergence résolue le 30 août.

## 1. Ordre recommandé

### 1. Fermer le build et le parsing de `--s` — **LIVRÉ (`eba24b9a`)**

`eba24b9a` ajoute `src/core/parse.hpp`, route `--s` dans les deux CLI et impose
le plancher 8. La conversion éventuelle de `--n`, `--smax`, `--seed`,
`--threads`, `--cell-min-sites`, `--shell-cap` et des options CUDA est un
chantier distinct ; elle ne doit pas être déclarée implicitement.

La provenance devra dire « parsing exact de `--s` » et non « des entiers de
CLI » tant que les autres options utilisent `atoi`/`atoll`.

### 2. Le patch de vérité contractuelle Gabriel/Gamma

C'est le P0 de l'auditeur et je le maintiens en tête après le build, parce qu'il
**sécurise le sens de toute mesure ultérieure** sans rien bloquer.

La v5 émet le sous-flot Gabriel horizontal puis les deltas d'un K-MST sur ses
facettes. Ce n'est ni le `hgp_reduced` exact du contrat Gamma, ni une hiérarchie
partielle munie d'une projection prouvée.

**Précision après vérification de la liste fautive.** La formule « forêt HGP
exacte » est une paraphrase d'audit, pas une citation du chantier. `README.md`
et `docs/ARCHITECTURE.md` ne nomment ni Gabriel ni K-MST ; ils surqualifient la
portée en annonçant les dix forêts horizontales HGP du manuscrit. Les textes
qui identifient réellement l'objet sont `docs/MATHEMATIQUES.md:129` (« la forêt
HGP = ce K-MST par K »), `docs/MATHEMATIQUES.md:1009` et
`src/forest/fold.hpp:74` (« payload hiérarchique complet »). Le patch doit viser
ces affirmations précisément, puis requalifier la portée générale du README et
de l'architecture.

Fermeture minimale, dans l'ordre de l'auditeur : renommer avec
`proof_basis=gabriel_positive_connectivity` et `forest_semantics=verified_events_only` ;
ajouter `reconstruction_contract_id` et `require_exact`, et refuser
atomiquement `require_exact=true` sur cette source ; graver
`gabriel-point-set-counterexample-5-points-v1` dans une porte v5 ; seulement
ensuite raccorder une source d'incidences silencieuses.

Aucun claim d'exactitude n'est recevable avant. Le travail de génération q3/q4
n'est pas bloqué par ce point — il est seulement mal nommé.

### 3. Le sweep de séparation `s` dans `{8, 9, 10}`

Bon marché, entièrement spécifié, et ouvert depuis deux jours. `s = 10` n'a
**aucun reçu** : les mentions `8/10` d'`ECHELLE.md` et `GPU.md` décrivent des
campagnes prévues. Théoriquement `Sep_10` implique `Sep_8`, donc le front à 10
raffine celui à 8 — plus de rectangles terminaux, mais chacun mieux séparé donc
plus facile à tuer en paquet. Les deux effets vont en sens contraire et rien ne
tranche.

Protocole exigé : entrée, graine, arbre, seuils, post-séparation et threads
figés ; digests finaux identiques ; puis publier séparément front WSPD brut,
masse après cœur, distributions non censurées de `h_coeur/h_a/h_b`, résiduel
q3/q4, temps par étage et HWM. Écrire `{8,10}` pour deux essais, jamais un
intervalle qualifié.

### 4. q4 : réparer `ChordPieces` avant d'interpréter quoi que ce soit

Divergence relevée par l'auditeur et confirmée dans le code
(`generate.hpp`, boucle de cœur de `process_anchor_q4`) : la route intégrée fait
`continue` sur les `P > 0` certifiés **avant** `chord.update`. Le commentaire
affirme « jamais témoin d'aucun morceau ». C'est valide pour le cœur à
`mu = 0`, mais **faux pour une corde**, où `v_j = 4P - (2j-4)*mu_chapeau*B` peut
devenir négatif hors du centre.

L'effet est **fail-open** — aucune fausse mort, l'objet est intact — mais
théorème, sonde `q4_chord_probe` et produit comptent alors trois choses
différentes, et `seeds_killed_chord` est ininterprétable. Fermer avec la fixture
entière et le mutant `chord-skip-positive` déjà spécifiés, dans les trois routes
(scalaire, shaped, device), avant `oneside_gate` et `w4_tape`.

### 5. La fusion de la descente WSPD, une fois son banc propre

Voir [`NOTE_CLAUDE_DESCENTE_WSPD_FUSIONNEE_20260830.md`](NOTE_CLAUDE_DESCENTE_WSPD_FUSIONNEE_20260830.md).
Dans les bras internes de la sonde, environ 58 % des visites de rectangles et
des appels au compteur de témoins sont évités. Mais la porte ne compare pas à
`alive_rectangles` et `collinear_seven` donne déjà `30/30/30` dans la sonde
contre `30/30/29` dans le produit, à cause de `smax_effective=9`. L'objection
mémoire ne tombe pas non plus : `size*sizeof(T)` n'est qu'une charge utile
minimale. Ce chantier reste prometteur, mais il n'est **pas prêt à recevoir**.

Il lui faut d'abord un bras A appelant directement le produit, des planchers et
mutants, la cinquième famille de mesure, puis un banc apparié contrebalancé et
une mesure des capacités simultanées ou du HWM aux tailles 16000 et 32000.

## 2. Une porte manquante sur le chemin produit

Le ledger de masse de paires n'est jamais évalué globalement par la descente
produit. `pair_mass` et `expected_pair_mass` vivent dans
`src/wspd/wavefront.hpp`, mais `wspd_wavefront` n'a que trois appelants, tous
dans `tests/`. Le produit passe par `alive_rectangles`, qui vérifie son ledger
post-séparation à partir d'une masse `base` déjà admise et ne retrouve jamais
`expected_pair_mass(ix)`.

L'égalité de masse est nécessaire, pas suffisante : deux omissions/duplications
compensatoires la laisseraient verte. Ajouter à `alive_rectangles` la somme
pondérée des rectangles émis **et tués**, la comparer à la masse attendue avec
un code exact, puis conserver en parallèle une porte de multiensemble sur de
petits arbres. Cette autorité doit précéder la fusion WSPD.

## 2 bis. Pistes WSPD issues d'un panel adversarial, classées

Six lentilles indépendantes, vingt propositions, trois réfutateurs chacune. Neuf
survivent sans réfutation. Ce qui suit est le classement, avec ce qui a été
mesuré et ce qui reste à falsifier. Rien n'est livré ; rien ne fait baisser une
pente — **aucun gain de complexité n'a été trouvé, par aucune lentille**.

**A. Hisser le tampon de pile de `count_universal_witnesses`.** La fonction
construit un `std::vector<Entry>` **à chaque appel**
(`src/spindle/witness_count.hpp:72`, vérifié), deux fois par rectangle et par
lane. Poste chiffré à 4,9–5,6 % du mur de la descente, à surface d'objet nulle
par construction. C'est aussi le **confondeur** de toutes les pistes qui
suppriment des appels : à faire **en premier**, sinon leur gain est mal
attribué.

**B. La descente unique à masque de lanes** — § 1.5 et la note dédiée. Le seul
mécanisme du dossier dont la neutralité soit *argumentée* et non seulement
constatée : les compteurs sont écrêtés par `min(c, h)`, la fermeture par lane est
monotone, et `corner64_universal_34` est monotone dans le bon sens
(`3H^2 <= Xi` implique `2H^2 <= Xi`, donc le drapeau q3 ne meurt jamais avant le
drapeau q4, et les valeurs finales ne dépendent pas du masque).

**C. Dériver les candidats diamétraux des handles déjà calculés** au lieu d'une
seconde traversée depuis la racine. Seule proposition à recevoir un verdict
« solide » des trois réfutateurs : preuve de contenance indépendante de la
géométrie, 448 configurations, 6,55 M rectangles, **zéro désaccord et zéro
doublon**, contre-familles incluses. Gain apparié : −6,4 % (q3) et −5,3 % (q4)
de la phase des corps, soit 1 à 2 % du mur. Réception : garder
`rect_diametral_candidates` comme **juge** et non le supprimer, sinon le mutant
`cover-rect-dmin` devient invisible sur le chemin produit.

**D. Une seule passe par rectangle pour q3 et q4**, le cover d'ancre n'étant
construit qu'une fois : 46 à 48 % des covers sont aujourd'hui dupliqués entre les
deux lanes. Mais le gain mesuré est de 4,6 à 5,7 % de la génération et **décroît
avec n** (part mutualisable 35 % à `uniform` 8000, 14,2 % à `terrain` 32000) :
c'est la part mutualisable qui décide, pas le taux de doublon. Dépend de B.

**E. Marche de témoins à deux phases** (report des sous-arbres élagués au lieu de
refaire la marche avec coins). Mécanisme correct, identité des valeurs vérifiée,
mais après B le résidu tombe à environ 10 % des nœuds, et deux modes de faute
*fail-closed* ont été exhibés — dont un qui produit des sur-crédits, donc
`need = 0`, donc des boules perdues. À ne pas tenter avant A et B.

**Deux gratuités.** `GenerateStats` calcule déjà `rect_visited[3]` et la WSPD
calcule `wave_peak` ; **aucun des deux n'est imprimé** par `print_run` (zéro
occurrence dans `src/pipeline/run.hpp`). Deux lignes donnent les deux compteurs
dont toute réception d'échelle a besoin.

**Ce qui est écarté, et pourquoi.** Le raccord `SepCell/SepTight` — la route
reçue pour prouver la borne — **change `digest_balls` sur 34 configurations sur
34** et casse six lignes de `receipts/conformite_v4/digests_v4.txt`
(`digest_all` reste intact). Il viole donc la contrainte dure d'immuabilité des
reçus, et le lemme d'empilement qui seul porterait le gain n'est pas écrit. La
conclusion recevable est inversée : **écrire ce lemme sur papier d'abord**, coût
nul et aucune ligne de code ; s'il ne ferme pas, rien n'a été payé ; s'il ferme,
il faudra assumer explicitement la requalification de la porte de conformité v4
plutôt que l'annoncer sous une « porte d'égalité des digests ».

Une proposition prétendait mesurer une pente sur `anchors` : elle est réfutée.
`anchors` est un **grand-livre fermé** (`+= nA*nB`, incrémenté avant tout
travail), pas un compteur de coût ; chronométré, il pèse 0,9 % du corps de lane.
Le compteur de travail est `hist_survivors`.

## 3. Corrections documentaires à faire indépendamment

- **La borne `O(s^3 n)` est affirmée dans trois en-têtes** (`wavefront.hpp`
  ligne 3, `cloud_index.hpp`, `ARCHITECTURE.md`) alors que
  `MATHEMATIQUES.md` § 5 la déclare `ouvert` pour la variante radix-Morton. Les
  aligner sur le statut le plus faible.
- **`docs/Q4_APRES_Q3.md` porte encore des quantités mal nommées ou corrigées
  depuis** : le critère ponctuel vaut
  `angle(azb)>125,264 deg`, tandis que la pointe a une demi-ouverture
  `54,736 deg` et une ouverture complète `109,471 deg` ; les 47 ancres
  représentent `0,71 %` des ancres échantillonnées (ou `1,34 %` des survivantes,
  `2,72 %` des longues) et non `5,5 %` ; et sur le bon dénominateur
  `faces_D`, le quotient conditionnel de complétion passe de `12,58` à `95,27`
  sur `terrain`, exposant sécant `0,73` et non `0,21` — ce qui **invalide** la
  recommandation « ne pas chercher côté complétion en premier » que ce document
  porte toujours. Le dernier commit n'a corrigé que `ETAT_COURANT.md` et le
  fichier `QUESTION_CLAUDE_*`.
- **`digest_balls` n'est pas invariant en `s`.** Vérifié : entre `s = 8` et un
  `s` débordé, les dix `digest_forest_K*` et `digest_all` sont identiques,
  mais `digest_balls` diffère. Le `README.md` présente la conformité v4≡v5
  comme prouvée par les deux ; une campagne de conformité doit donc épingler `s`
  exactement, sinon elle compare deux choses différentes.

## 4. Ce qui n'est pas une piste

Rappels utiles avant la prochaine itération sur la WSPD, tous déjà fermés
(`docs/PISTES_FERMEES.md`) : un plafond de population dans le critère terminal
(quadratique par construction) ; la scission du facteur le plus peuplé ; deux
arbres spatiaux coexistants ; une source kNN à petit préfixe pour les ancres q2 ;
une décomposition ternaire **symétrique** comme source q3 linéaire (Théorème 4).
Restent explicitement ouvertes : les WSSD approximatives, les sources
**asymétriques** ancre--tiers, la restriction par profondeur, l'arrangement local
de centres.

Et un piège de cadrage à garder en tête : **améliorer la WSPD ne peut pas
réduire le nombre de seeds.** Le balayage `s = 2..10` le laisse invariant à
l'unité près, digest identique. Tout gain WSPD porte sur l'amont, et est donc
borné par la part de l'amont — laquelle vaut 68 à 70 % de la génération sur
`uniform`, mais 16 % sur `scanline` à 32000.

## 5. Sur l'état du dépôt lui-même

**Divergence constatée puis résolue le 30 août.** Pendant la rédaction de cette
note, `main` était daté du 22 août, ne contenait pas `morsehgp3D_v5/`, et n'avait
**aucun ancêtre commun** avec la branche portant le chantier : cinquante et un
commits y étaient invisibles. Le commit `e4aaf12` intitulé « Merge
remote-tracking branch 'origin/main' » avait en réalité fusionné
`7e74e77 "Add files via upload"`, pas `main`.

`main` a d'abord été réinitialisé sur la lignée v5 à `119b80b0`, puis a avancé
sur les pins nommés par `ETAT_COURANT.md`. Il contient donc
`morsehgp3D_v5/` et la doctrine « commits sur `main` » d'`AGENTS.md` est de
nouveau satisfaite. La lignée v4 antérieure, dont
`bab37b9` était la tête, n'est plus atteignable depuis `main` mais **survit sur
`origin/claude/morsehgp3d-v4-reprise-nck0nk`** : aucun commit n'est perdu.

Ce constat est conservé ici parce qu'il explique pourquoi les reçus et audits
antérieurs au 30 août peuvent citer des pins introuvables depuis `main` selon la
date à laquelle ils ont été écrits. Une lecture d'historique croisant les deux
lignées doit nommer la branche, pas seulement le hash.

GCP non utilisé.
