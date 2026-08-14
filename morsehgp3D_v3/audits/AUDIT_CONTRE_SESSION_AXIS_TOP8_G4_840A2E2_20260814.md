# Contre-audit de la recette `session_axis_top8_g4.sh`

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot : `HEAD=840a2e28679aa3e5e3d8ec706daa680a52ac1bde`, fichier
`gcp-migration/session_axis_top8_g4.sh`, SHA-256
`a2f67c33503fc1db8b3c3faa05a6d2b172458bd762392e51a9555dc556d1ebf0`.

GCP n'a pas été utilisé. Cet audit est statique et la recette ne doit pas être
lancée avant réparation de la fermeture ciblée ci-dessous.

## Verdict

La recette est correctement étiquetée comme campagne CPU de réfutation sans
CUDA ni SLO. Elle n'est néanmoins pas recevable en l'état : son trap peut
arrêter une génération qu'elle n'a pas démarrée, sa matrice séquentielle n'est
pas compatible avec son propre coupe-circuit de 55/75/90 minutes, et son verdict
ne couvre pas les P0 sémantiques du noyau.

## P0 sécurité — arrêt non versionné quand `GENERATION` est vide

Le trap est armé avant `start_and_verify.sh`, avec `GENERATION=""`. Si le
démarrage refuse une VM déjà active, si le handoff est absent ou illisible, ou
si son parsing échoue, la branche de secours appelle
`stop_and_verify.sh --yes` **sans** `--expected-last-start-timestamp`. Le script
d'arrêt vérifie alors le nom et le label, mais aucune génération ; il peut donc
arrêter une session préexistante ou concurrente sur la cible par défaut.

Cette branche contredit à la fois le commentaire « exactement la génération
qu'elle a démarrée » et la règle impérative de non-mutation des autres sessions.
Elle annule aussi la protection déjà présente dans `start_and_verify.sh` : ce
dernier refuse précisément un arrêt automatique non versionné lorsque la
génération démarrée reste inconnue.

Réparation requise avant exécution : état explicite `start_attempted` /
`target_generation_known`, lecture et validation du handoff dans le trap, puis
arrêt uniquement avec l'horodatage exact. Si aucune génération ciblée ne peut
être prouvée, la recette doit signaler le projet, la zone, le nom, le dernier
état connu et la commande de contrôle ; elle ne doit jamais appeler l'arrêt
non versionné. Un échec antérieur à toute tentative de démarrage ne doit appeler
aucun arrêt.

Le transcript est en outre copié avant que l'erreur finale d'arrêt non certifié
soit ajoutée au journal. Le reçu copié peut donc omettre le fait bloquant ; la
copie finale doit suivre toute décision de cleanup.

## P0 faisabilité — 76 runs séquentiels sous un budget de 55 minutes

Les phases demandent 30 runs à `n=120`, 18 à `n=200`, 12 à `n=300` et 16 à
`n=200`, soit 76 runs séquentiels. Chaque run possède un timeout de 3 300 s,
alors que le calcul distant entier doit finir en 3 300 s, l'arrêt invité en
4 500 s et GCE en 5 400 s. La somme des timeouts autorisés atteint 250 800 s,
soit presque 70 h : aucune enveloppe de durée globale ne rend la matrice
terminable.

Le probe annonce lui-même une complexité exhaustive en puissance cinq. Par
rapport à `n=60`, les facteurs de travail nominaux sont 32 à `n=120`, environ
412 à `n=200` et 3 125 à `n=300`. Une seule taille maximale peut donc consommer
presque tout le budget. Il faut une rampe locale ou distante causale, un budget
global mesuré et un arrêt après le premier palier rouge ; les tailles/graines
suivantes ne sont ouvertes que si le débit observé prouve qu'elles tiennent
avant le coupe-circuit.

## P1 — la campagne ne reçoit pas encore le contrat q4

`manquants=0` et `census_faux=0` ne comparent aujourd'hui que la complétude des
racines retenues et le **cardinal** intérieur. Ils ne jugent ni les listes de
vrais `PointId` de `I_B/U_B`, ni le shell `insphere_j==0`, ni `RelevantGP`, ni
le primary entre les deux `Q4Seed3`, ni la multiplicité globale des
`SupportKey`. La mort par gaps est encore posée après le sweep exhaustif dans
le probe commis. Une grande campagne ne peut pas compenser ces absences.

Les planchers par run sont tous nuls. Le verdict agrégé exige une masse shallow
globale, mais une famille ou un seuil individuel peut rester vide et être masqué
par les autres runs. Chaque famille/phase doit publier ses propres planchers de
faces, racines, événements, ties et morts, avec exception déclarée et vérifiée
pour `two_lines`.

La bonne séquence est donc : recevoir localement IDs/shell, `DEAD_GAP`, overflow
fail-closed, primary/exact-once et options CLI ; ajouter au moins un CTest avec
seuil différent de sept ; mesurer une rampe bornée ; seulement alors produire
une recette G4 réaliste. Cette session restera un diagnostic CPU 48 cœurs, pas
une mesure GPU et encore moins le contrat bout-en-bout 50k sous une seconde.

