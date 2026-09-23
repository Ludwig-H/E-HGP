# Contre-audit B — réception et fermeture G4 v5

23 septembre 2026. Lecture indépendante et contre-fixtures **en mémoire**,
sans VM, moteur modifié ni dépense GCP. Le protocole produit est publié à
`099ca784`, puis complété par `b4e480fc` ; les fichiers opérationnels
relus sont `tower_worker_v9.py` SHA-256 `255aeeed…`,
`tower_session_v9.py` `8fb0e14b…` et `tower_snapshot_v9.py`
`bb97edf0…`. Verdict : le snapshot Git et le préflight ont beaucoup gagné,
mais la réception n'est pas encore *fail-closed* ; corriger avant de payer
une nouvelle campagne G4 R3.

Une campagne v5 sur `b4e480fc` avait déjà démarré lorsque ce verdict a
été publié. Ses chronos éventuels peuvent rester des diagnostics, mais
son statut `completed`/`partial` ne doit pas être promu sans contrelecture
indépendante de la fermeture et des identités du reçu ; ne pas répéter une
campagne payante avant correction.

## Quatre contre-exemples acceptés

1. `validate_received` accepte comme `partial` un cas censuré
   `killed_case_cap` où `group_closed=false`, pourvu que le délai et le
   drapeau `residual_or_interrupted_group_killed` concordent. Le worker
   continue après ce cas sans exiger la fermeture certifiée du groupe.
   Voir `gcp-migration/tower_session_v9.py` vers la ligne 206 et
   `tower_worker_v9.py` vers 635. Une ligne `partial` ne doit jamais
   signifier qu'un descendant peut encore tourner pendant le cas suivant
   ou au moment de l'arrêt.
2. La réception recalcule beaucoup de détails des cas, mais accepte un
   `receipt.json` où `target`, `generation` et `provenance` sont changés et
   `guard_evidence.json` remplacé par `{}`, tout en rendant `completed`.
   Les gardes **avant** démarrage et arrêt ciblé restent en place ; c'est
   l'identité du reçu retourné qui n'est pas rattachée au contexte hôte
   attendu. Voir `tower_session_v9.py:126` et l'appel de réception.
3. Le préflight de 1 500 sites peut être déclaré complet avec tous ses
   compteurs `generator` et `ledger` à zéro, si le reste du JSON garde son
   schéma. Les seuils non vacuants de la porte CTest 360 sites ne sont pas
   réappliqués à cette sonde invitée. Voir `tower_worker_v9.py:596` et
   `tower_session_v9.py:182`. Ce contre-exemple n'est pas un faux résultat
   géométrique démontré ; il montre qu'un préflight sans travail effectif
   peut franchir la barrière destinée à prévenir un nouvel échec R2.
4. Le worker juge le `preflight.stderr` par le parseur GNU time avant les
   cas LiDAR ; l'hôte ne le rejuge pas. Un stderr invalide dont le hash de
   commande est mis à jour reste accepté `completed`, contrairement aux
   stderr des cas LiDAR. Voir `tower_session_v9.py:182` et
   `tower_worker_v9.py:596`.

À la publication de R3, une seconde classe de lacune est reproductible
sur son **vrai préflight non vacuant** : `validate_probe` rend encore
`complete_relative` si l'on remplace isolément par zéro l'un de
`ledger.dead_loads`, `ledger.dead_form_sites`, `ledger.cover_builds`,
`generator.q34_expanded_pairs` ou `catalogue.q3_presentations`.
L'auditeur A retrouve aussi `ledger.dead_q3_open`. Les comptes réels
satisfont les identités
`expanded_pairs=cover_builds+witness_rejected_pairs`,
`dead_loads=cover_builds`,
`dead_form_sites=cover_sites−2·dead_loads`,
`dead_q3_open=q3_edges`, `dead_q4_open=q4_edges` **quand l'option est
active**, et les émissions par arité
du catalogue ; le schéma typé ne les **impose** pas. Ajouter ces mutations
au préflight réel et aux selftests, après avoir rendu cohérente leur
fixture factice. Cela ne remet pas en cause la concordance manuelle du
[reçu R3](CONTRE_AUDIT_B_G4_R3_20260923.md), mais borne ce que le lecteur
pourra certifier tout seul à l'avenir.

## Contrôles acquis et correction demandée

Le snapshot est reconstruit depuis les objets Git du commit annoncé et le
protocole transporté est comparé au protocole exécuté. Le préflight précède
les trames LiDAR ; s'il échoue chez l'invité, aucun cas ne démarre. Le nuage
préflight est déterministe, u18 et comporte 1 500 sites uniques. Le budget
utile total de 1 500 s et le cap LiDAR de 600 s, la double garde, l'arrêt
ciblé et le refus d'une campagne sans tour achevée restent en place.
Le préflight n'a pas de cap court propre : il peut absorber tout le budget
utile, sans pour autant dépasser ce budget. Le workflow CI v9 surveille
maintenant les scripts de protocole et lance les selftests hors GCP.
Rejeu indépendant du paquet cohérent `b4e480fc` : **20/20 selftests
normaux et 20/20 sous `-O`** (53,609/51,089 s, hôte local partagé).
Les 25 fichiers du reçu exploratoire q3/q4 du même commit passent leur
`SHA256SUMS`. Aucun de ces deux faits ne teste les quatre mutations ci-dessus.

Porte minimale avant R3 : exiger `group_closed=true` pour **chaque** cas
censuré et refuser une poursuite invité après fermeture incertaine ; lier
le reçu et l'évidence de garde à la cible/génération/provenance attendues ;
faire exercer au préflight quelques compteurs natifs non nuls propres aux
voies annoncées et rejuger son stderr GNU time côté hôte. Ajouter ces
quatre mutations causales aux selftests normaux/`-O`, puis lire le reçu
du snapshot commité avant toute dépense. Cela ne qualifie toujours ni le
contrat d'une seconde ni le GPU : le protocole R3 reste CPU G4.
