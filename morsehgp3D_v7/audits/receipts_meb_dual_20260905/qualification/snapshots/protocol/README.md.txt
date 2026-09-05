# Prototype privé MEB à deux budgets — préparation non compilée

5 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=prototype_prive_non_integre`, `public_status=not_claimed`.

**Aucune compilation, exécution C++, mesure ni qualification effectuée.**
Ce dossier contient la variante privée, sa fixture source et un runner
de capture préparé ; aucun de leurs attendus C++ n'a été exécuté.
Le GO de préparation ne vaut pas GO d'exécution pendant les paires mono F.
GCP non utilisé ; aucun fichier produit, historique scellé, audit ou Git modifié.

## Autorités et raccord futur

La note `morsehgp3D_v7/docs/PROPOSITION_MEB_ET_BUDGETS.md` a été lue
intégralement ; son § 6.2 spécifie le deuxième budget prospectif. SHA256 :
`365e7a5dcde5a6d6fcd7d43e00d2f58f86efbf42279a779c62c7eaac7b54ec25`.

`pivot.hpp` inclut le prototype historique
`build/v7_meb_pivot_prototype/pivot.hpp`, SHA256
`d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5`,
sans le modifier. Il réutilise explicitement uniquement `Candidate`,
`point`, `form`, `ordinal` et `materialize`. Les anciennes routes
`propose`, `small_ball` et `miniball` ne sont pas appelées.
Le `Builder` de repli vient des headers produit F effectivement inclus,
pas d'un binaire D historique : une future compilation devra épingler cette
fermeture de sources et la comparer séparément à F. Rien n'est lié au CLI.

L'identifiant proposé est `reference_ordinal_plus_proposal_v1`.
`Limits::max_meb_proposal_supports` est explicite (zéro par défaut).
`Work::meb_proposal_supports` est un compteur de candidats effectivement
tentés ; le même `Work` appartient à une tentative/un ordre et persiste
entre toutes ses MEB, succès, refus legacy et replis compris.
Un futur raccord Builder, schéma, CLI, statistiques et reçus reste à écrire
et qualifier : cette variante ne modifie pas les anciens contrats publics.

## Invariants de la variante

- Le cap legacy déjà atteint refuse avant toute proposition, comme F.
- Un budget de proposition déjà atteint replie F avant la paire extrême.
- Chaque forme, initiale comprise, est chargée avant ses prédicats et sa
  construction. Le garde `counter >= limit` précède l'incrément et protège
  aussi la frontière `UINT64_MAX`. Une forme géométriquement rejetée paie.
- `small_ball` distingue rejet, accepté et épuisé. Le premier épuisement
  sort immédiatement des boucles imbriquées ; il ne devient pas un refus
  public. Le repli reçoit les compteurs legacy et la boule encore intacts.
- Les gardes n2..11, sous-ensemble au plus cinq, support au plus quatre,
  coquille finale exactement q, positions canoniques et niveaux littéraux
  sont conservés. Même une demande de `pivot_cap > 16` est bornée à 16.
- La finalisation du support accepté réemploie les constructions historiques
  de clé/niveau ; elle n'est pas une nouvelle proposition de support.

Depuis des compteurs nuls, candidats réels des replis F + formes proposées
<= charges legacy + charges proposition <= L + P. Avec des compteurs
injectés non nuls, raisonner sur les incréments. Ce n'est ni un plafond de
temps, d'instructions ou de RAM, ni une borne unique de la tour K1..10.
La sélection de paire, les puissances et la finalisation sont distinctes
du nombre de candidats. Les compteurs auxiliaires `pivots`, `certified`,
`fallback` restent des observations privées, pas des caps publics nouveaux.

## Fixture préparée et résultats attendus, non observés

Le triangle local est `(0,0,0), (2,2,0), (2,0,2)` dans cet ordre de sites.
Son support final q3 est à l'ordinal legacy 4 ; la proposition complète
essaie cinq formes. La source compare le booléen, le statut, la raison,
toutes les statistiques legacy, les champs littéraux de boule et la
sentinelle aux appels frais du `Builder` F inclus.

- Huit cas P dans {0,1,4,5}, L dans {1,4} : P formes, repli si P<5,
  certificat sinon ; refus public seulement selon le cap legacy L.
- Quatre appels avec le même Work, P=7, L=12 : compteurs proposition
  5,7,7,7 et legacy 4,8,12,12. Le deuxième appel épuise P au milieu
  de `small_ball` ; le troisième ne sélectionne aucune paire ; le
  quatrième refuse le cap legacy sans proposition ni nouveau repli.
- Frontières injectées proches de UINT64_MAX : une dernière charge de
  proposition puis repli, quatre charges F sans dépassement unsigned.
- L'observateur est appelé immédiatement avant chaque forme et vérifie
  la charge prospective ainsi que l'absence de mutation legacy pendant
  la proposition. Il n'effectue aucune allocation dans ses callbacks.
- Le mutant privé `ChargeAfter=true` déplace uniquement l'incrément après
  `form`. Les compteurs et terminaux restent identiques, mais le même juge
  causal doit le rejeter. Aucun raccourci n'est désactivé sous TESTING.

Invocations futures à capturer après GO séparé : sans argument, code 0 ;
`--mutant=charge-after`, code 4 ; argument inconnu ou supplémentaire,
code 2. Une autre assertion en échec rend 1. Ce sont des **attendus**, pas
des résultats. Aucun reçu d'exécution C++ n'est créé par la préparation.

## Runner préparé, GO d'exécution encore requis

`run_dual_budget.py` ne lance aucun sous-processus et n'écrit aucun fichier
sans `--execute`. Le mode actif exige aussi son SHA256 relu via
`--expected-runner-sha256`. Le dossier de sortie `run_20260905` est fixe,
créé exclusivement, jamais repris ni écrasé, y compris après un échec.

Les six commandes prévues sont la version du compilateur, la compilation
C++20/O2/-Wall/-Wextra/-Wpedantic/-Werror, puis le nominal (0), le mutant
charge-après (4) et deux refus d'arguments (2). Aucun MHGP7_TESTING,
sanitizer, CMake, pipeline ou CLI produit n'est exécuté. Chaque argv,
intention préalable, PID/PGID possédé, stdout/stderr bruts et code de sortie
est conservé séparément ; le juge exige aussi les lignes causales exactes.

Le runner impose CPU0 par affinité héritée et une échéance cumulée de
60 secondes, dont deux réservées au nettoyage. Chaque enfant a une session
propre ; sur incident, TERM puis KILL ciblent son seul groupe, avec attente
du leader et vérification de disparition du groupe. Les signaux INT/TERM/HUP
entrent dans le même chemin fermé. Les contrôles de préparation ci-dessous
ne simulent ni une garantie réelle du noyau ni un test de timeout C++.

Le reçu de build F et son CLI (seulement hashé), les sources F déclarées
par ce reçu, les formes historiques, la note, la variante, la fixture et
le compilateur sont épinglés avant/après. Le depfile MMD doit contenir la
fermeture locale connue, notamment les deux prototypes et le vrai header
F ; une dépendance inconnue ou modifiée est refusée. Le binaire local est
hashé après compilation, avant chaque commande et à la clôture. Le reçu
terminal est `completed` seulement si commandes, sorties et stabilités sont
conformes ; sinon il est `failed`, et tous les bruts partiels sont conservés.

`selftest.py` contient uniquement des gardes Python synthétiques : preview
inerte, arguments littéraux, create-only, JSON dupliqué, pins F en lecture
seule, depfile, sorties causales et faux enfants en timeout/interruption.
Tout Popen de ces scénarios est un mock ; aucune compilation n'est autorisée
par ces selftests. Leurs résultats éventuels restent séparés des attendus C++.

Cette fixture n'est pas une qualification exhaustive q2/q3/q4, des 1 507
ordinaux, de tous les caps, du catalogue Gamma ou de la tour. Les portes
et mutants listés au § 7 de la note restent des obligations futures.
Aucun gain mono, SLO 50k/1 s/100 ms, passage massif ou GPU n'est revendiqué.
