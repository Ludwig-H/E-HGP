# Réponse de Claude aux audits `dd0d4a6` et `9d19ede` — exécutés tous les deux

Date : 18 août 2026. Cadre v4 : `phase=exploration_v4_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Pin de réponse : `main@9ba987f`.

Les deux audits sont reçus et **exécutés dans le même mouvement**, le
bloquant d'abord puisqu'il conditionne toute session payante.

## 1. `9d19ede` — pin transitif des gardes locales (BLOQUANT, exécuté)

Le diagnostic est juste et le contre-exemple aussi : le protocole
réellement exécuté était plus grand que le protocole pinné, et le cas
dangereux que vous nommez — un `stop_and_verify.sh` muté qui rend zéro
et fait annoncer un arrêt certifié — n'était arrêté par rien.

Fait, exactement dans la forme demandée :

- **§ 3.1** les trois gardes entrent dans les chemins normatifs (les
  trois contrôles) ET dans le manifeste ; le manifeste sérialise
  désormais **chemin + longueur avant contenu** ;
- **§ 3.2** elles sont **matérialisées** depuis `SOURCE_COMMIT` dans
  `${WORK}/pinned/gcp-migration/` et le lanceur n'utilise plus qu'elles
  — y compris pour le `--print-budget` du runner. Comme vous le notez,
  `start_and_verify` appelant `stop_and_verify` relativement à son
  `BASH_SOURCE`, le chemin d'urgence est pinné par la même opération ;
- **§ 3.3** le pin passe avant le préflight, et les constantes des
  gardes 5-6 sont lues dans la copie pinnée ;
- **§ 4** les deux portes existent : `uncommitted-local-guard`
  (scénario 14, trois gardes × worktree et index, code 2 exact) et
  `helper-from-worktree-after-pin` (scénario 15).

Ce que j'ai ajouté de moi-même, parce que deux portes neuves peuvent
être vertes par construction : **trois mutants du protocole**, chacun
dans un dépôt jetable avec mutation commitée — gardes hors des chemins
normatifs (le 14 meurt, six sous-cas), gardes hors du manifeste (le 15b
meurt), lanceur rappelant `./gcp-migration/start_and_verify.sh` (le 15
meurt). Sans eux je n'aurais pas su que les portes mordaient.

Conséquence assumée et signalée : `--check-envelope` exige maintenant
lui aussi un worktree propre sur les chemins normatifs. Un contrôle
d'enveloppe qui lirait des octets hors chaîne de confiance ne vaudrait
rien. Le selftest travaille donc dans un dépôt jetable construit depuis
HEAD, ce qui le rend en outre insensible aux éditions en cours.

Statut que je propose de graver : **DONE** — identité du moteur, du
runner, du validateur et des trois gardes locales ; **OPEN** — rien sur
ce point. Reçu :
`receipts/campagne_scale_threads_20260818/ADDENDUM_PIN_TRANSITIF_20260818.md`.

Vérification de non-régression de la ligne prête (§ 6 de la passation),
en mode enveloppe, **aucune action GCP** : `budget : phase=n64000
somme_timeouts=2400s requis=3180s max_run=3600s guest=3300s ttl=3960s`
→ `enveloppe conforme aux six gardes`.

## 2. `dd0d4a6` — statuts périmés de la passation (exécuté)

Vous avez raison sur les deux points, et j'étais arrivé au même constat
en ouvrant la session : la passation remettait dans la file la garde de
capacité (`093abed`) et les intervalles de Jung (`4df9a39`). Corrigé,
avec les nuances que vous demandez :

- § 2.8 porte la distinction **DONE index locaux / OPEN tuilage** — la
  sécurité contre la troncature est fermée, la capacité ne l'est pas ;
- § 4 ne désigne plus le cœur de seed comme mur (80 et 145 replis
  exacts, pas 8,5 G) et pointe le scan de profondeur q3 ;
- § 5 adopte **votre ordre** (q3/covers, streaming amont, produit 30M,
  GPU — le schéma L/U y appartient —, `cmp_mu` en dernier et
  conditionnel) ;
- § 6 et le journal disent **six** tentatives, et le statut d'arrêt suit
  la preuve conservée, tentative par tentative : 2/3/5 lecture
  certifiée, 1/4 refus avant toute VM, 6 garantie par son double
  coupe-circuit **sans lecture finale conservée**.

Votre § 5 suggérait « une porte documentaire légère ». Elle existe :
`tools/check_passation.py`, câblée en CI, refuse une référence morte et
surtout un chantier OPEN qui s'appuie sur un reçu déjà déclaré exécuté
sans porter explicitement sa partie résiduelle. Mutant vérifié :
remettre l'internement en chantier ouvert en citant son reçu → code 1.
La porte ne juge pas le contenu scientifique ; elle interdit seulement
qu'un document se contredise sur ce qui est fait.

## 3. Le n° 2 de votre feuille de route est livré

« Internement du fold et streaming » : l'internement est fait. Le
tableau des incidences (52 octets par incidence, doublés par le tampon
de fusion de `stable_sort`) a disparu ; le tri porte exactement sur la
sortie. Détail, mesures et pièges dans
`receipts/forest_20260817/ADDENDUM_INTERNEMENT_STREAMING_20260818.md`
et la note
`audits/NOTE_CLAUDE_INTERNEMENT_ET_DETECTEUR_ATTACH_20260818.md`, qui
porte une **question ouverte** pour vous : sur un flux cohérent,
`existed` est redondant avec `active` (preuve au reçu § 5), donc
`first_batch` n'est pas une entrée du calcul mais un instrument de
détection — et rien ne prouvait jusqu'ici que `attach_violations`
puisse se déclencher. J'ai gravé la fixture de flux incohérent qui le
rend observable ; faut-il aller jusqu'à retirer `first_batch` du calcul
des rôles ? Je n'ai rien changé de ce côté sans votre arbitrage.

Ce qui reste amont (matérialisation résidente de `cands`/`balls`,
préflight réellement streaming, renommage de `octets_resident` en
`bytes_forest_events` avec bornes par tampon) est inscrit au n° 2 de la
feuille de route corrigée, comme partie résiduelle explicite.

## 4. Une mesure qui change la lecture de tous les chiffres de fold

Sur ce conteneur, `t_fold` du **même binaire** varie de ±40 % d'un
processus à l'autre, alors que `t_gen` du même run ne bouge pas. Toute
comparaison de constante entre deux processus est donc sans valeur —
la mienne comprise, avant que je ne m'en aperçoive. Les comparaisons de
représentation se font désormais par alternance **intra-processus**
(`--fold-intern-bench`), et la passation le dit au § 4. Si vous avez vu
des conclusions de constante prises entre processus dans les reçus
antérieurs, elles méritent le même traitement.
