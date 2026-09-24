# Contre-audit B — pool de tâches E2, avant qualification G4

24 septembre 2026. Lecture **sans modification du moteur** du commit local
propre `b37b49504c9b0a0384db6c6e178ad865b4b8d38e` du prototype E2 ;
ce n'est pas un reçu publié sur `main`. Le dernier reçu G4 reste R20.
En-têtes lus : `pool.hpp` SHA-256 `23b3c482389c9d7f77bed5764103e0bc1b8472fa5cc7a6bf99dfe116cfc9f0fa` ;
`full_ball_tower.hpp` SHA-256 `59d5a93a211010c2ed8c73125d2ff943174279fbaf502dc176f7256a41e7f7e3`.
Le contrat utilisateur est la tour FULL K1..5 **explicite** en 100 ms sur
trame SemanticKITTI sans sol, grille 1 mm ; K10, brut avec sol et les
autres régimes gardent leurs portes distinctes.

## Ce que le pool change réellement

`TaskPool` garde W−1 fils persistants pour les appels faits par le fil
propriétaire de `Builder::run` (`pool.hpp:252–258`,
`full_ball_tower.hpp:420–428`). Cela peut réduire les lancements répétés
de la phase statique. Il n'est **pas** encore un ordonnanceur global borné
à W : les K fils `run_orders_overlapped` exécutent `order_prepare_lean`
et lancent chacun leurs propres `parallel_items` de largeur jusqu'à W.
Le propriétaire garde le pool, tandis que ses ouvriers sont inaccessibles
aux runners ; les soumissions imbriquées utilisent elles aussi des fils
neufs, précisément pour éviter un blocage de file occupée.

À K5/W48, la borne de fils présents est donc 47 pool + 5 runners +
jusqu'à 5×48 auxiliaires + 1 pilote = **293**, contre une borne de 294
pour le chemin sans pool au même chevauchement. Ce sont des maxima par
le code, **pas** un pic mesuré sur LiDAR. Le nouveau gate compte les fils
créés cumulativement, pas leur maximum simultané. Le pool peut gagner du
temps de lancement malgré cette limite ; aucun chrono G4 E2 n'existe
encore. La voie à étudier pour un plafond effectif est un ordonnanceur
commun à tâches et dépendances explicites, ou des quotas de largeur entre
phase statique et runners ; une file imbriquée naïve peut se bloquer et
n'est pas une solution.

## Trois portes de refus et de comptabilité

1. `TaskPool::run` conserve `worker_error_` si le propriétaire et un
   ouvrier lèvent ensemble : il relance d'abord l'erreur propriétaire
   (`pool.hpp:179–184`) et n'échange jamais l'erreur ouvrière. Une
   soumission suivante sans panne relance alors cette vieille erreur.
   Un test causal de la primitive est `TaskPool(2)`, callback 0→A et
   callback 1→B synchronisés pour lever tous deux, puis callback propre :
   la deuxième génération doit réussir. Le
   [reproducteur de l'audit](pool_e2_exception_probe_20260924.cpp),
   compilé avec GCC 13/C++20/`-Werror` sur ce SHA, donne
   `first=owner_A`, puis **`second=worker_B`** avec code 1 : échec de
   réutilisation démontré pour la primitive. Les wrappers usuels
   `parallel_items/ranges` capturent leurs exceptions avant `run`, donc
   **aucune panne produit observée** n'en est déduite ; le gate actuel ne
   couvre pas cette forme brute.
2. `static_workers_created` signifie des créations effectives dans les
   reçus précédents, mais le code E2 y ajoute `lanes` retourné par
   `parallel_ranges` (`full_ball_tower.hpp:1896–1899`), désormais des
   **participants**, propriétaire inclus, lors du réemploi du pool.
   Le même W peut être ajouté à chaque K alors que W−1 fils n'ont été
   créés qu'une fois. Le nouveau gate exclut justement ce champ de sa
   comparaison déterministe. Renommer ce compteur en voies engagées ou
   préserver l'ancienne sémantique ; publier séparément créations et pic.
   `helper_threads` est un delta global de processus, exact seulement
   sans autre client concurrent de la primitive.
3. `Builder::run()` construit le pool avant `validate_catalogue()` et
   avant de refuser une largeur invalide (`full_ball_tower.hpp:420–437`).
   Sous entrée invalide **et** échec injecté de lancement, E2 peut donc
   rendre `full_ball_thread_launch_failed` là où la voie sans pool rend
   l'erreur d'entrée. Le gate de lancement ne croise que des entrées
   valides. Soit préserver la priorité d'entrée par validation préalable,
   soit documenter explicitement la nouvelle priorité et tester les deux
   causes simultanées. Ce n'est pas un faux succès connu.

Pour qualifier E2 : gel de SHA, porte de refus ci-dessus, TSan si le
runtime le permet, A/B même catalogue et mêmes digests complets, coûts
mur/CPU de chaque phase, créations **et pic** de fils, RSS, trois répétitions
sur G4 pour K5 puis K10. Le gate local dit lui-même que ses temps sont
indicatifs. R20 K5 sans sol valait encore 1,010–1,260 s de chaîne, dont
374–456 ms de FULL : même un FULL gratuit laisserait 636–804 ms hors
tour. E2 seul ne peut donc démontrer les 100 ms ; il faut aussi faire
baisser q3/q4, census et les autres étapes.

GCP non utilisé dans ce contre-audit ; aucune accélération E2 revendiquée.
