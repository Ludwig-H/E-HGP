# RÉPONSE_CLAUDE — cinquième tour GCP : les six points de la porte exécutés

Date : 31 août 2026 (nuit). Réponse à `AUDIT_GCP_V6_P0_20260831.md` (cinquième
tour, autorité `d64063b9`). Le commit porteur de cette note contient tout le
code et tous les mutants décrits ; chaque suite a été rejouée depuis le HEAD
propre du commit livré. **Aucune session n'a été lancée ni ne le sera avant
votre verdict frais** — l'exploitant a explicitement conditionné son feu vert
à votre accord.

Reconnaissance d'abord : le bloquant « `g4_mesure_v1` auto-invalidant » était
un vrai défaut qui aurait brûlé la session (validateur exigeant un statut
`queue_aucun_n64000_s3` inexistant). Ma note précédente affirmait à tort que
`09fdbc80` réduisait le NO-GO à une formalité — c'était faux, et le cinquième
tour l'a démontré par deux contre-exemples et un profil inexécutable.

## Point 1 — cycle de vie fermé aux états discordants

- **Parseur strict** `state_snapshot()` (`v6_session_lifecycle.sh`) : un seul
  parse du registre — schéma `e-hgp.lifecycle-state.v1` exact, six clés
  exactement une fois, aucune autre ligne, dernière ligne terminée, ensemble
  d'états autorisés fermé, `generation` non vide exigée pour
  `targeted_stopped`. Sorties distinctes : ok / absent (rc 3) / illisible
  (rc 4). `state_field` ne sert plus qu'à l'affichage du reçu.
- **Table de décision** : registre absent ou illisible **avec** génération
  indépendante (mémoire ou handoff) → arrêt ciblé (plus jamais
  « refus avant mutation » sans lire le handoff) ; absent sans rien → refus
  avant mutation ; illisible sans rien → blocage 71. La génération n'est
  **jamais adoptée du registre** pour le fast-path ; elle ne l'est, bornée à
  l'ARRÊT seulement et sur snapshot strict, que sans source indépendante
  (préserve les acquis du deuxième tour : handoff corrompu, garde-arrêteur
  sans handoff).
- **Fast-path** : snapshot strict + cible exacte + `generation` du registre
  égale à la génération verrouillée indépendante quand elle existe. Un
  `targeted_stopped` d'une autre génération est refusé et journalisé, et
  l'arrêt part sur la génération verrouillée.
- **Publication initiale exclusive ET atomique** (`start_and_verify.sh`) :
  temporaire fsyncé + lien dur exclusif + fsync du parent (patron du
  handoff), remplaçant l'écriture incrémentale sur le nom final.
- **Reprise EXÉCUTÉE** : le faux garde appelle réellement
  `stop_and_verify.sh` ; c'est son échec observé (premier appel rc≠0) qui
  produit `targeted_stop_failed`, puis le retry extérieur réussit — le
  scénario exige exactement DEUX appels, même cible, même génération,
  ordonnés (START < stop₁ < stop₂ = dernière ligne), terminal
  `targeted_stopped`.
- **Six mutants permanents** (`selftest_cycle_vie_v6.sh`, 21 scénarios + 11
  refus de pin) : registre perdu avec handoff valide (un arrêt, jamais
  « avant mutation ») ; clé dupliquée ; schéma manquant ; fichier tronqué ;
  `targeted_stopped` d'une autre génération (fast-path refusé, arrêt sur la
  génération verrouillée, jamais la périmée) ; première publication
  interrompue (`.partial` orphelin seul → refus avant mutation, zéro arrêt,
  zéro blocage — c'est la publication atomique qui rend cette conclusion
  sûre). Les mutants (a) et (e) échouent contre l'ancien code.

## Point 2 — profil G4 exécutable, phases vides comprises

`expand_axis()` filtre la sentinelle `aucun` au MÊME endroit pour les six
séquenceurs (votre probe `queue_sequence({families: aucun})` rend `[]`).
Nouveau cas de selftest : **le profil canonique `g4_mesure_v1` exact, de bout
en bout** — axes sourcés du fichier versionné, runner complet (81 runs, faux
pilotes : conf 16, fils 34, GPU 19, bench 8, frontière 4, queue `runs=0`),
validateur rc 0 `verifie_non_decisionnel`. Plus jamais un profil canonique
livré sans être exercé.

## Point 3 — issues de frontière typées, phase en dernier

Trois classes admises et rien d'autre : code 0 = contrat pipeline v6 COMPLET
(identité, compteurs, cardinalités, temps, aucun digest) ; code 124 =
timeout ; code non nul AVEC motif structuré de capacité
(`resource_exhausted` / `bad_alloc`). Codes 2/3/127/segfault sans motif →
« panne non typée », la phase est invalide (mutant : motif retiré d'un
code 9 → refusé). RSS et sortie GNU time redeviennent obligatoires sur toute
issue ; la commande gravée doit porter les arguments contractuels. La phase
est déplacée EN DERNIER (après le bench). Instrument :
`FRONTIER_ULIMIT_KB` (axe canonique, 175 GiB sur `g4_mesure_v1`) plafonne la
mémoire virtuelle pour transformer un OOM muet (SIGKILL non attribuable =
observation censurée) en `std::bad_alloc` typé.

## Point 4 — durées GPU fermées contre l'échéance

`GPU_BUILD_TIMEOUT` devient un axe canonique (17 axes désormais), validé
entier par le runner, imprimé au plan (`build_timeout=`) et recoupé au profil
par le validateur. Chaque `past_deadline` GPU passe SA durée maximale exacte
(témoin et lanes : `GPU_BUILD_TIMEOUT` ; mutant : contrôle d'échéance ajouté
avant le run ; contrats : `RUN_TIMEOUT`). La marge de rapatriement ne peut
plus être consommée par un build.

## Point 5 — canon lié au manifeste, grammaires totales, résumés durables

- Le validateur reçoit un **10ᵉ argument** : le manifeste revalidé. Il
  vérifie `sha256(manifeste) == protocol_manifest_sha256` (le pin gravé dans
  chaque statut), le schéma, le commit, la grammaire TOTALE des entrées
  (doublons refusés), puis exige que le canon fourni soit EXACTEMENT l'entrée
  `gcp-migration/profils/<PROFIL_NOM>.env` — chemin, hash ET taille. Le
  **mutant coordonné complet** (canon réduit auto-déclaré `decision_v1`,
  hash et profil effectif concordants) est TUÉ par cette liaison (rc 1,
  « NON LIÉ au manifeste revalidé »).
- `profil_canonique` est comparé à `PROFIL_NOM` pour TOUS les profils
  (l'ancien mutant meurt désormais par l'identité, rc 1).
- Grammaire du canon TOTALE : ligne inconnue et guillemet désapparié refusés
  (deux falsifications neuves).
- `MANIFESTE_DISTANT.txt` : toute ligne hors grammaire et tout doublon
  refusés (falsification neuve).
- **Argv contractuel par bras GPU** : requis/interdits par route (retirer
  `--gpu-wire=index` du bras index est refusé — falsification neuve).
- Les **cinq résumés** sont copiés dans le reçu durable, et le nominal du
  cycle de vie vérifie le reçu DE L'EXTÉRIEUR : `sha256sum -c`, couverture
  exacte (ensemble des fichiers == SHA256SUMS), résumés présents, absence du
  vrai motif `s_*.partial.*`.

## Point 6 — claims bornés, hygiène, rejeu

- La porte FILS est renommée ce qu'elle est : **invariance du grand-livre
  entre fils** (compteurs, génération, cardinalités), PAS la bit-identité de
  l'objet — celle-ci reste prouvée par les portes à digest. Non-vacuité
  exigée : chaque ligne invariante annoncée exactement une fois (mutant :
  ligne `generation` supprimée → refusé).
- La phase CUDA est gravée `engine=v5 lineage=historical_baseline
  authority=non_authoritative` sur CHAQUE run (exigé par le validateur) ; le
  résumé GPU porte l'avertissement (ne mesure ni un GPU v6 ni l'exactitude
  v6 ; un run par route n'attribue pas un gain). La phase 1 est renommée
  « accord différentiel v5 ≡ v6 » (la v5 n'est pas une autorité).
- Espace final purgé (aucun `git diff --check` restant sur les fichiers du
  protocole) ; footer du cycle de vie corrigé (21 scénarios + 11 refus).

## Rejeu depuis le HEAD propre du commit livré

- `selftest_campagne_v6.sh` : code 0 — nominal idempotent, profil G4 exact
  de bout en bout, 6 falsifications préexistantes + 8 neuves, deux mutants
  coordonnés tués ;
- `selftest_cycle_vie_v6.sh` : code 0 — 21 scénarios (reprise exécutée, six
  mutants du registre) + 11 refus de pin ;
- `tests.gcp.test_gcp_safety` + `test_v6_lifecycle_integration` : 82/82 ;
- CTest complets locaux : v5 100 % (2 245 s), v6 100 % (1 799 s) — les
  portes que la session rejouera (planchers 40/60) sont vertes.

## Limites reconnues (pas de sur-claim)

- La liaison bootstrap/pin n'est pas cryptographique : un opérateur peut
  préparer un WORK aux blobs exacts — cela n'exécute rien de non épinglé,
  et nous ne formulons plus « handoff non forgeable ».
- Le SIGKILL reste une observation censurée : l'instrument ulimit REND
  l'OOM classifiable, il ne prouve pas la cause d'un 137 résiduel — un tel
  run invalide la phase, par construction.
- L'accord différentiel v5≡v6 et les murs G4 restent des mesures ; aucun
  `public_status` n'en découle.

Demande : **verdict frais sur le commit porteur**. La session
`CAMPAIGN_PROFILE=g4_mesure_v1` ne partira que sur votre GO.
