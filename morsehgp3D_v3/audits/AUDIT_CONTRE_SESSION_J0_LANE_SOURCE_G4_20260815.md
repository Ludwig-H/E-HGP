# Contre-audit de la session J0 `lane_source` sur G4

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

L'auditeur n'a lancé, interrogé, arrêté ou modifié aucune ressource GCP. Les
sessions décrites sont concurrentes. Les transcripts ont été lus localement.
Aucune Delaunay n'est utilisée et aucun résultat ci-dessous ne qualifie un
backend GPU : la VM G4 sert uniquement de machine CPU à 48 cœurs.

Snapshot statique final lu :

- `HEAD=3657289dcb2960c7e7605b12cfe9d9c34552935e` ;
- runner principal SHA-256
  `b6eac657537c04bda6702d47e154ac429518ed7bb58797b78f9cbac248d7d334` ;
- récupérateur SHA-256
  `e677239c0176b8daf63952f383efbd3a93cdc26668b43a70fc68e98700658f25` ;
- brut `rampe_j0.txt`, 78 lignes, SHA-256
  `69f79818f49fcae600fd54b8f896718adc5efe3eaad7467cbdbd0f7c70ad1c54`.

## Verdict

La fermeture générationnelle est globalement fail-closed et les deux
générations de calcul observées ont été arrêtées sur leur horodatage exact puis
certifiées `TERMINATED`. La rampe scientifique est **rouge** : dix runs, quatre
codes non nuls, pire rapport de coupure `0,940`, amas limités à 12 500 points.

Le brut est très informatif pour l'architecture, mais il n'est pas une mesure
J0 de l'objet : la source a déjà les P0 de coupure, owner q3 et shell du
contre-audit dédié. Ses compteurs peuvent simultanément omettre des ancres et
surcompter des candidats. Ils ne sont donc ni une borne inférieure ni une borne
supérieure globale des sorties exactes.

## 1. Chronologie et sécurité

1. Au pin `0195480`, la première session a échoué au build : un double
   antislash transmis à make est devenu une cible littérale `\`. Le trap a
   arrêté la génération `2026-08-14T13:59:42.143-07:00` et certifié
   `TERMINATED` avant tout benchmark.
2. `62b9c59` a réparé la ligne de build. La seconde session a construit les
   deux probes, passé `50/50` CTests en `0,70 s`, puis produit la rampe rouge.
   Sa génération `2026-08-14T14:02:58.255-07:00` a été arrêtée de façon ciblée
   et certifiée `TERMINATED`; le transcript signalait aussi zéro autre VM
   `project=e-hgp` active.
3. Comme le verdict précédait alors le `scp`, le code un a sauté la copie du
   brut. `bfef178` a inversé les étapes et ajouté un récupérateur. Sa première
   tentative de récupération a échoué fermé avant démarrage, car le TTL de clé
   95 minutes était hors de l'enveloppe d'une session GCE de 30 minutes.
4. `3657289` dérive maintenant le TTL. Le fichier brut est présent localement,
   mais le transcript live correspond encore à la tentative de récupération
   refusée, pas à la session qui l'a produit. L'historique Git conserve les
   états antérieurs ; le reçu fixe n'est pas encore un paquet atomique par
   génération.

Le runner principal vérifie la cible, le label, `g4-standard-48`, `SPOT`,
`STOP`, la durée GCE, l'arrêt invité et l'horodatage de génération. Son cleanup
n'appelle jamais un arrêt non versionné. Si un démarrage est tenté sans
handoff exploitable, il rend un blocage et n'usurpe pas une session concurrente.

## 2. Résultat brut utile, mais non certifié

Sur `uniform`, les compteurs tronqués sont presque linéaires aux trois tailles :

| `smax` | `n` | q2 | q3 | q4 | total | total/point | CPU 48 cœurs |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 6 | 12 500 | 230 752 | 526 687 | 195 892 | 953 331 | 76,266 | 1 s |
| 6 | 25 000 | 469 314 | 1 081 600 | 405 927 | 1 956 841 | 78,274 | 2 s |
| 6 | 50 000 | 952 789 | 2 216 329 | 835 876 | 4 004 994 | 80,100 | 4 s |
| 11 | 12 500 | 449 864 | 2 302 455 | 2 266 637 | 5 018 956 | 401,516 | 8 s |
| 11 | 25 000 | 919 567 | 4 754 695 | 4 704 961 | 10 379 223 | 415,169 | 19 s |
| 11 | 50 000 | 1 873 994 | 9 789 648 | 9 768 840 | 21 432 482 | 428,650 | 38 s |

Les exposants diagnostiques entre tailles successives valent environ
`1,04/1,03` pour `smax=11/6`. La baisse de `428,650` à `80,100` candidats par
point vaut un facteur `5,35`, pas le facteur douze supposé dans le plan. Même
sous `K=5`, le ledger compte quatre millions de candidats à 50 000 points.

La masse de travail reste très supérieure au ledger. À `uniform,50000` :

- `smax=11` : `75 780 216` ancres testées, `171 956 174` seeds aigus,
  `6 091 112 797` paires de lentille, `1 100 846 370` q4 bien centrés et un
  ratio `paires_lentille/q4=623,5` ;
- `smax=6` : les mêmes `75 780 216` ancres, `35 143 117` seeds aigus,
  `498 056 446` paires de lentille, `95 372 598` q4 bien centrés et un ratio
  `595,8`.

Ce reçu donne donc le verrou concret de la seconde : la route actuelle paie
des centaines de complétions par candidat et jusqu'à des milliards de couples
avant le census/payload. `Q4SeedAxisTopR4` peut retirer le produit avec l'apex,
mais le compteur `seeds_aigus` est partagé entre q3 et q4 et ne mesure même pas
encore le nombre autonome de `Q4Seed3`. Le prochain ledger doit séparer les
sources de lane avant d'attribuer un gain.

Sur les régimes difficiles, la rampe se ferme immédiatement :

| famille | `smax` | `n` | total candidats | paires lentille | rapport cutoff | temps | code |
|---|---:|---:|---:|---:|---:|---:|---:|
| terrain | 6 | 12 500 | 273 593 | 861 009 321 | 0,940 | 6 s | 3 |
| terrain | 11 | 12 500 | 916 498 | 2 522 307 318 | 0,940 | 19 s | 3 |
| eight_clusters | 6 | 12 500 | 863 573 | 6 266 706 925 | 0,864 | 35 s | 3 |
| eight_clusters | 11 | 12 500 | 4 373 796 | 24 135 659 695 | 0,926 | 147 s | 3 |

Aucun chiffre 25k/50k n'existe donc sur la deuxième famille obligatoire. La
session réfute sa propre porte J0 et confirme que la grille/produit de lentille
n'est pas la route industrielle.

## 3. Blocages scientifiques du parser

Le verdict `LISIBLE` ne demande ni `unresolved_pair_mass=0`, ni un owner q3
canonique, ni `I_B/U_B`, shell et `BallKey`. Son garde de coupure est le même
maximum observé déjà réfuté par `two_lines`. Un futur vert resterait donc sans
portée J0.

La porte n'exige que deux tailles par piste obligatoire, pas 50 000 ni trois
points permettant deux pentes. Elle utilise une seule seed. Les CTests distants
sont lancés sans `--no-tests=error`, leur commentaire annonce encore 23 portes
alors que 50 sont exécutées, et seul le dernier bloc de six lignes entre dans
le transcript.

## 4. Blocages industriels restant dans les scripts

- Le runner principal n'a pas de deadline globale englobant installation,
  build, CTests, rampe et copies. Les 55 minutes commencent seulement dans la
  rampe ; les SSH/scp/build/tests ne sont pas bornés et peuvent consommer les
  vingt minutes séparant la rampe de l'arrêt invité.
- `RUN_TIMEOUT` n'est pas validé avant son interpolation dans la commande
  distante ; `timeout` n'a pas de `--kill-after`.
- Le cleanup copie le transcript avant d'ajouter le message final
  `[ARRET NON CERTIFIE]` lorsque `stop_rc!=0`.
- Le reçu est un chemin fixe écrasable. Les tentatives successives ont déjà
  remplacé le transcript de calcul par celui d'une récupération refusée. Le
  tar dirty n'est pas conservé et aucun manifeste structuré ne lie atomiquement
  génération, tar, ELF, brut, verdict et arrêt.

La réparation minimale est un dossier par `generation+tar_sha`, écrit d'abord
en temporaire puis renommé atomiquement, avec copie des preuves rouges avant
verdict, deadline externe unique et manifest JSON hashant chaque artefact.

## 5. Porte suivante

La priorité n'est pas une troisième rampe avec un cutoff plus grand. Il faut :

1. recevoir la `NeutralPairPartition` ou des continuations avec masse conservée ;
2. séparer physiquement q2, q3 et q4, notamment leurs ledgers de seeds ;
3. réparer owner q3 et census `I_B/U_B/BallKey` ;
4. remplacer le produit q4 de lentille par la sélection axiale, puis mesurer
   visites et groupes par seed autonome ;
5. seulement alors refaire J0 avec trois tailles, plusieurs seeds et un reçu
   atomique.

Le contrat d'une seconde reste ouvert. Cette session ne l'a ni mesuré ni
réfuté sur GPU ; elle a localisé le travail inutile que l'architecture doit
supprimer.
