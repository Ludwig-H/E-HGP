# Recommandations Claude au 30 août 2026 — ordre de travail et ce qui le justifie

Note Claude, 30 août 2026, ancrée au pin `9940668`. Cadre :
`phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Cette note **ne promeut aucun statut** et ne remplace pas `ETAT_COURANT.md`, qui
reste le verdict mutable. Elle ordonne les chantiers ouverts et dit, pour chacun,
ce qui est mesuré, ce qui est supposé, et quel serait le premier pas falsifiable.
Là où elle recoupe l'ordre de travail de l'auditeur, elle le suit.

## 0. Ce que j'ai vérifié moi-même sur ce pin

> [!NOTE]
> **Les trois premiers constats sont clos par `eba24b9a`**, poussé après la
> rédaction de cette note. Je les conserve parce qu'ils expliquent pourquoi
> l'item 1 était en tête, et je les fais suivre de ma propre vérification de la
> fermeture. Le quatrième constat, lui, reste ouvert.

Quatre constats de premier main, reproductibles, qui cadrent les priorités.

- **Le build propre est cassé.** `tests/fold_bench.cpp` inclut
  `../src/core/parse.hpp`, absent du dépôt. `cmake --build` échoue donc sur la
  cible `mhgp5_fold_bench`, et la recette du `README.md` ne passe pas. Nuance
  factuelle par rapport à `ETAT_COURANT.md` § P1 : sur ce pin, **un seul**
  fichier l'inclut, pas trois.
- **Six portes échouent.** `mhgp5_cli_refus_s_suffix` et
  `mhgp5_cli_refus_s_overflow` (code 0, attendu 2), plus les quatre portes
  `mhgp5_fold_bench_*` qui ne peuvent pas se construire. Toutes les autres
  passent, y compris WSPD, secteurs, cellules, cordes, oracles bornés, mutants,
  parité batch et post-séparation, et `scale8000` est vert 6/6.
  **Le cœur mathématique tient ; c'est l'emballage qui est cassé.**
  Dénominateurs, à ancrer : 291 CTests enregistrés et 274 sous le label `gate`
  au pin `119b80b0` ; 297 et 280 depuis l'ajout des six portes de la sonde de
  fusion. Les six rouges sont les mêmes dans les deux comptages.
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

**Fermeture vérifiée à `eba24b9a`.** `src/core/parse.hpp` est suivi
(`git ls-files` le rend) ; `mhgp5_fold_bench` et `mhgp5` construisent tous deux
sous `-Werror` ; `mhgp5_cli_refus_s_suffix` et `mhgp5_cli_refus_s_overflow`
passent, et à la main `--s=8junk`, `--s=9223372036854775808` et `--s=7` rendent
tous le code 2 tandis que `--s=8` rend 0. Le quatrième constat — le régime
dégénéré atteint en silence par un `s` débordé — n'est donc plus atteignable
depuis les CLI ; il reste vrai de la primitive `alive_rectangles`, dont l'opt-in
test-only accepte encore un `s` arbitraire.

Un cinquième constat, hors code : voir la section 5, qui décrit l'état des
lignées Git et une divergence résolue le 30 août.

## 1. Ordre recommandé

### 1. Fermer le build et le parsing — deux fichiers — **LIVRÉ (`eba24b9a`)**

Ajouter `src/core/parse.hpp` au dépôt et router `--s`, `--n`, `--smax`,
`--seed`, `--threads`, `--cell-min-sites`, `--shell-cap` par un parsing entier
exact dans les deux CLI. Ce n'est pas une amélioration : c'est ce qui rend toute
mesure reproductible depuis Git. Tant que ce point n'est pas fermé, **tous les
verts sont obtenus sur un worktree non commité** et aucune campagne n'est
rejouable. Coût : une heure. Débloque : les six portes, et la réception de
`e6ed85df`.

La provenance devra dire « parsing exact de `--s` » et non « des entiers de
CLI » tant que les autres options utilisent `atoi`/`atoll`.

### 2. Le patch de vérité contractuelle Gabriel/Gamma

C'est le P0 de l'auditeur et je le maintiens en tête après le build, parce qu'il
**sécurise le sens de toute mesure ultérieure** sans rien bloquer.

La v5 émet le sous-flot Gabriel horizontal puis les deltas d'un K-MST sur ses
facettes. Ce n'est ni le `hgp_reduced` exact du contrat Gamma, ni une hiérarchie
partielle munie d'une projection prouvée.

**Précision sur la liste des fichiers fautifs.** `ETAT_COURANT.md` § P0 écrit que
`README.md`, `docs/ARCHITECTURE.md`, `docs/MATHEMATIQUES.md` § 7 et
`src/forest/fold.hpp` « appellent encore le K-MST du graphe de Gabriel la forêt
HGP exacte ». J'avais repris cette liste sans la vérifier ; contrôlée, elle est
imprécise sur deux de ses quatre entrées :

- la formule « la forêt HGP exacte » n'apparaît **nulle part** dans le chantier :
  zéro occurrence hors `ETAT_COURANT.md` lui-même. C'est une paraphrase de
  l'auditeur, pas une citation ;
- `README.md` et `docs/ARCHITECTURE.md` ne contiennent **aucune** occurrence de
  « Gabriel » ni de « K-MST ». Ils identifient la sortie aux « dix forêts
  horizontales HGP du manuscrit », ce qui reste une **sur-qualification
  sémantique** — le reproche tient — mais pas par le mécanisme allégué ;
- le texte réellement fautif est `docs/MATHEMATIQUES.md:129` (« **la forêt HGP =
  ce K-MST par K** », estampillé `theoreme_manuscrit`) et `:1009`, plus
  `src/forest/fold.hpp:74` où `ForestResult::deltas` porte le commentaire
  « le payload hierarchique complet ».

Cette correction ne change ni le verdict, ni la fermeture attendue : elle change
seulement **où poser le patch**. Viser `MATHEMATIQUES.md` § 1.2 et § 7.1 et
`fold.hpp:74` en premier ; requalifier `README.md` et `ARCHITECTURE.md` sur le
registre de la portée annoncée, pas sur celui du K-MST.

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
Environ 58 % des visites de rectangles et des appels au compteur de témoins sont
refaits à l'identique, la sonde et ses six portes sont livrées, et l'objection
mémoire est mesurée et tombe. Ce chantier est **prêt à recevoir** mais il arrive
en cinquième position délibérément : il ne change ni l'objet, ni la pente. Il ne
mérite pas de passer devant un verrou sémantique.

Il lui manque un banc apparié contrebalancé sur machine au repos, et
l'extension de `vague_pic` à 16000 et 32000.

## 2. Une porte manquante, trouvée en contre-lecture

Le ledger de masse de paires — l'identité qui atteste que la descente
**partitionne** les paires au lieu de les couvrir — n'est **jamais évalué sur le
chemin produit**.

`pair_mass` et `expected_pair_mass` n'existent que dans deux fichiers :
`src/wspd/wavefront.hpp` et `tests/wspd_gate.cpp`. Or `wspd_wavefront` n'a que
**trois appelants, tous dans `tests/`** (`wspd_gate.cpp:84`, `cover_gate.cpp:35`,
`spindle_gate.cpp:118`). La WSPD de production est `alive_rectangles`
(`src/pipeline/generate.hpp`), qui réimplémente la même descente et **ne porte
aucun ledger de masse** : elle n'expose qu'un ledger de raffinement
post-séparation, lequel vérifie `émis + tués == base` mais prend `base` comme
donnée, pas comme quantité à retrouver.

Autrement dit, la propriété la plus fondamentale de l'amont — chaque paire
apparaît exactement une fois — est gravée sur une primitive que le produit
n'appelle pas. Une divergence entre les deux descentes ne serait vue par aucune
porte.

Deux réserves pour ne pas surestimer ce constat. D'abord, l'identité de masse est
une condition **nécessaire de comptage**, pas une preuve de partition : deux
fautes compensatoires la laissent verte. Ensuite, une porte littérale existe
bien sur la primitive produit — `tests/postsep_refine_gate.cpp` développe le
multiensemble de couples — mais parent par parent et sur de petits arbres, ce
qui ne couvre pas l'identité globale.

Le raccord est bon marché : accumuler `sum |A| |B|` sur les rectangles émis
**et** sur les rectangles tués dans `alive_rectangles`, et comparer à
`expected_pair_mass(ix)` en fin de descente, sous porte à code exact. C'est
l'invariant que le raccord de fusion de descente (§ 1.5) devra de toute façon
préserver, et il vaut mieux qu'il existe avant.

## 3. Corrections documentaires à faire indépendamment

- **La borne `O(s^3 n)` est affirmée dans trois en-têtes** (`wavefront.hpp`
  ligne 3, `cloud_index.hpp`, `ARCHITECTURE.md`) alors que
  `MATHEMATIQUES.md` § 5 la déclare `ouvert` pour la variante radix-Morton. Les
  aligner sur le statut le plus faible.
- **`docs/Q4_APRES_Q3.md` porte encore trois chiffres corrigés depuis** :
  l'ouverture `W_4` vaut `109,47 deg` et non `125,26` ; les 47 ancres
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

`main` a depuis été réinitialisé sur la lignée v5 et pointe sur `119b80b0` ; il
contient donc `morsehgp3D_v5/` et la doctrine « commits sur `main` »
d'`AGENTS.md` est de nouveau satisfaite. La lignée v4 antérieure, dont
`bab37b9` était la tête, n'est plus atteignable depuis `main` mais **survit sur
`origin/claude/morsehgp3d-v4-reprise-nck0nk`** : aucun commit n'est perdu.

Ce constat est conservé ici parce qu'il explique pourquoi les reçus et audits
antérieurs au 30 août peuvent citer des pins introuvables depuis `main` selon la
date à laquelle ils ont été écrits. Une lecture d'historique croisant les deux
lignées doit nommer la branche, pas seulement le hash.

GCP non utilisé.
