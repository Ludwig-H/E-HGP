# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, après **77bcd0b8**, sur main. Écritures limitées à ce
dossier. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Raccord utile : partager B sans changer implicitement son ordre

La [section 9.4](P0_SOUS_RECTANGLES_ET_GROUPES.md#94-partager-les-arbres-b-sans-transférer-leur-borne-de-couverture)
complète le raccord Pool seul. Un préfixe de son ordre se couvre en
O(log |B|) dans l'arbre construit sur cet ordre. Dans un arbre géométrique
différent, la même sélection peut exiger |B|/2 racines singleton. Une
fixture collinéaire avec crédits conservateurs certifiés réalise ce motif ;
elle ne prétend pas reproduire une stratégie C++ actuelle.

Deux chemins sont explicités : partager un arbre immuable lié à l'ordre
exact du plan ; ou garder l'arbre géométrique et payer ses agrégats locaux
et la fragmentation effective. Les ancres de même crédit partagent leur
couverture. Un budget de racines peut déclencher un repli par paires,
à condition de décider avant tout census du descripteur. Il borne ce
stockage provisoire, pas les visites ni le travail aval.

Le [modèle](p0_factor_order_probe.py) et son [reçu](P0_FACTOR_ORDER_CHECKS.json)
passent normal/−O : 5 912 permutations/préfixes, 23 640 budgets,
9 511 replis, 14 129 couvertures engagées, deux mutants rejetés.
Ce résultat précise un choix d'objets pour les tâches parallèles ; aucun
gain de temps ou de mémoire globale n'est annoncé.

## Coordination et points déjà traités

Le nouvel auditeur B conserve son [dialogue](DIALOGUE_AUDITEUR_B.md), sa
[note de régime WSPD](REGIME_WSPD_20260914.md) et `wspd_regime_20260914/`.
Ses fichiers et mesures ne sont pas
repris ici. Notre contrelecture confirme son encadrement des prédicats
`P4(16) ⇒ P8(8) ⇒ P4(14)` ; il a précisé la formule réellement utilisée
par le code v4 et les conditions de transfert aux comptes de rectangles.
La demande détaillée est donc retirée. Le front pur v4 reste une étude
de régime, sans qualification du futur front v8 avec élimination précoce.

Le partage nuage/index est en cours chez le constructeur. La revue des
plans publiés ne trouve aucun autre tableau de taille n caché : crédits,
ordres et restrictions sont dimensionnés par les facteurs. Le raccord
permutation/inverse globale est déjà documenté par l'auditeur complémentaire.
La [collecte suspendable de §9.3](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
reste disponible pour l'étape des continuations, avec son reçu propre.

Le P2 lien/IPO est corrigé et documenté dans la
[publication des bornes préparées](../receipts/q2_prepared_bounds_20260914/README.md).
Ses détails quittent ce dialogue. Les fixtures et reçus encore référencés
restent conservés. Les points secondaires restent regroupés ici : Dual
à budget facultatif, maximum avec Tubes, NoCredit après restriction du
facteur opposé. P0, q3/q4, FULL et massif restent ouverts.
Le contrat 50k vise toute la tour K=1..10 sous une seconde sur G4,
avec repli 1..5 ; les composants mono locaux ne qualifient pas ce contrat.

Contrôles : modèle normal/−O identique, 540 Markdown actifs et registre
20 phases valides ; les deux Markdown indépendants sont contrôlés
explicitement. Le reçu vérifie les commandes, résultats et hash de source.

Réservation après 77bcd0b8, index constaté vide : DIALOGUE_COURANT.md,
P0_SOUS_RECTANGLES_ET_GROUPES.md, p0_factor_order_probe.py et
P0_FACTOR_ORDER_CHECKS.json dans ce dossier uniquement. Fenêtre close
au commit/push ; constructeur et autres auditeurs exclus. GCP non utilisé.
