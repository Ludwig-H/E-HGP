# Porte privée de géométrie MEB à deux budgets — préparée, non exécutée

5 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=qualification_locale_privee_preparee`, `public_status=not_claimed`.

**Aucune compilation ni exécution C++ autorisée par cette préparation.**
Le GO courant porte sur le code privé et les selftests Python simulés ;
un GO distinct doit viser les pins finaux avant `--execute`.
Git et GCP non utilisés. Aucun produit, audit ou reçu historique modifié.

## Autorités et données

Le plan fermé `../v7_meb_dual_budget_prototype/geometry_plan/PROTOCOL.md`
porte le SHA 3e21a2066934923732375a65329b61d2f3bde73dd0ac5b546f4becb516de6f03.
Le prototype à deux budgets 0645aa00 est inclus sans copie ni modification.
Son ancien helper d6dbba reste la source exacte des formes/ordinal/niveau.
Le repli et les appels différentiels utilisent les headers F du reçu
522c950c et du CLI ee29d3d5, ce dernier seulement hashé, jamais exécuté.

`geometry_gate.cpp` porte explicitement les huit scènes et les 160 premiers
cas LCG du probe 42f5cde4. Il reprend sa boucle des 1 507 ordinaux avec un
compteur de rang indépendant de `choose`. Le fichier `additional_scenes.inc`
est une projection de données seulement : le runner exige son égalité
byte-exacte avec le rendu déterministe du JSON de scènes f66008d3.
Les résultats historiques de C/D/E/du probe ne sont pas réutilisés.

## Inventaires et juges

- 176 scènes admises n2..11, 352 ordres entrée/renversement plus 32 ordres
  de permutations exhaustives des trois scènes positives q2/q3/q4 : 384.
- 384 appels F distincts pour R, sans transmettre leur support au proposeur.
- 9 216 comparaisons principales : compteur legacy initial zéro, huit caps P
  et trois caps L=R-1,R,R+1. Les douze autres statistiques sont non nulles.
- 123 comparaisons frontières (borne du plan : 128) : c=7, MAX et c>L,
  P épuisé/partiel, repli forcé, séquence cumulative P7/L12 puis P0,
  et paramètres de pivots supérieurs à 16. Total différentiel : 9 339.
- Six formes valides de domaine, distinctes des comparaisons, exercent
  acuité/rang/centre non strict, shell q2 et intrus au-delà de i32.
- Deux événements sentinelles non vides sont comparés champ par champ,
  y compris données inutilisées et niveaux ; ils sont aussi observés
  avant chaque forme, sans allocation dans le callback.

Booléen, statut, raison, treize statistiques, clé, support complet, q,
trois limbs de niveau et dénominateur doivent être identiques à F.
Les gardes conservent la boule sentinelle sur refus legacy ; ils n'assimilent
pas ce cas au refus shell, qui peut écrire une boule. Le statut/raison
artificiels initiaux doivent également rester inchangés sur succès local.

Les scènes extrêmes nommées doivent obtenir leurs certificats rapides à
P401 : exactement 8 succès q2, 16 q3, 52 q4 dans cette sous-matrice.
Les q4 nommés exigent au moins deux pivots et un limb supérieur non nul.
Ce raccord a été ajouté après revue statique de la première révision :
les anciens totaux globaux seuls pouvaient masquer un repli de ces scènes.

## Compteurs et mutant causal

`proposal_forms`, `pair_selections`, `legacy_charges`, `certified` et
`fallback` agrègent seulement les **9 339 comparaisons**. Les six formes
directes sont comptées séparément par `direct_form_checks`. Les 384 appels
F donnant R et les 9 339 appels F de comparaison ne sont pas inclus dans
`actual_fallback_candidates` : A est exclusivement le delta legacy des
replis réellement exécutés par le prototype.

`certified` n'est pas un nombre de succès publics : L peut refuser après
certificat. Les réussites rapides sont comptées séparément par arité.
Les compteurs Work persistants ne sont jamais remis à zéro à l'intérieur
du proposeur ou d'un repli ; les fixtures cumulatives partagent leur State.

Le mutant `ChargeAfter=true` déplace la charge après la forme, sans bypass
MHGP7_TESTING. Le même juge doit préserver tous les terminaux et compteurs
géométriques, puis rendre 4 pour la seule cause `charge_not_prospective`.
En nominal, `prospective_violations=0`. Pour le mutant, il doit valoir
exactement `proposal_forms + direct_form_checks`, donc être non nul.
Le runner compare tous les autres champs des deux sorties, refuse les
champs manquants/supplémentaires/dupliqués, les nombres négatifs et les booléens
utilisés à la place d'entiers. Une divergence géométrique rend 1, pas 4.

## Runner préparé

`run_geometry.py` est un port borné de `run_dual_budget.py` 329ef731.
Sans `--execute`, il ne crée aucun fichier et ne démarre aucun sous-processus.
Le mode actif exige `--expected-runner-sha256` et crée exclusivement son
propre `run_20260905`, sans reprise ni écrasement. Durée totale 60 secondes,
CPU0 effectif et hérité, deux secondes réservées au nettoyage du seul groupe
enfant créé. INT/TERM/HUP suivent le chemin de fermeture capturé.

Six commandes prévues : version compilateur (0), compilation C++20/O2
stricte sans MHGP7_TESTING (0), nominal (0), charge-après (4), argument
inconnu (2), argument supplémentaire (2). Aucun CMake, pipeline, sanitizer,
benchmark ou CLI produit n'est exécuté. Les gardes de capture, pin F,
depfile, binaire, stdout/stderr et fermeture sont conservés du runner fermé.
Les bruts partiels restent présents si la future exécution échoue.

La préparation Python utilise seulement des mocks de Popen pour les essais
de timeout/interruption ; aucun résultat de processus C++ réel n'en découle.
Les durées de selftests ne sont pas des performances MEB.

## Limites explicites avant GO

Cette porte qualifie uniquement l'instanciation **Trace instrumentée**.
L'instanciation native `NoObserver` n'est pas exercée ; elle reste à vérifier
séparément, notamment hors chronométrage d'un futur microbenchmark.
Ce corpus peut montrer une conformité locale observée à F, pas une preuve
indépendante des prédicats partagés. Les 1 507 tuples sont exhaustifs pour
les ordinaux, pas pour les nuages u16. Pas de candidats hostiles hors domaine
injectés, pas de nouveau mutant shell/ordinal, pas de certification globale,
Gamma, transactions, composition de tour, CLI/budgets publics versionnés,
normalisation verticale ou déploiement industriel.
Aucun gain mono/multi-CPU/GPU, SLO 50k/1 s/100 ms ou passage massif revendiqué.
